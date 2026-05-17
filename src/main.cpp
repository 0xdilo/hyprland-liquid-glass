#include <algorithm>
#include <any>
#include <chrono>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

// Hyprland's render pass internals are private, so the hook needs access here.
// Keep standard headers above this macro to avoid rewriting libstdc++ internals.
#define private public
#include <hyprland/src/render/pass/Pass.hpp>
#include <hyprland/src/render/pass/SurfacePassElement.hpp>
#undef private

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/debug/log/Logger.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Renderer.hpp>

#include "Config.hpp"
#include "GlassDecoration.hpp"
#include "GlassPassElement.hpp"
#include "Globals.hpp"
#include "LayerGlassPassElement.hpp"

namespace LiquidGlass {

using RenderPassRenderFn = CRegion (*)(Render::CRenderPass*, const CRegion&);
using RenderWindowFn = void (*)(Render::IHyprRenderer*, PHLWINDOW, PHLMONITOR, const Time::steady_tp&, bool, Render::eRenderPassMode, bool, bool);
using RenderLayerFn = void (*)(Render::IHyprRenderer*, PHLLS, PHLMONITOR, const Time::steady_tp&, bool, bool);
using WindowOpaqueFn = bool (*)(Desktop::View::CWindow*);

static RenderPassRenderFn g_originalRenderPassRender = nullptr;
static RenderWindowFn g_originalRenderWindow = nullptr;
static RenderLayerFn g_originalRenderLayer = nullptr;
static WindowOpaqueFn g_originalWindowOpaque = nullptr;

static bool isExcludedWindow(PHLWINDOW window);
static bool isIncludedLayer(PHLLS layer);
static bool isFullscreenLayerOverlay(PHLLS layer, PHLMONITOR monitor);
static bool shouldOverrideLayerSurface(CSurfacePassElement* surface);
static SLayerGlassState* layerStateFor(PHLLS layer);
static void closeWindow(PHLWINDOW window);
static CGlassDecoration* glassDecorationFor(PHLWINDOW window);

static CRegion damageWithGlassBoxes(Render::CRenderPass* self, const CRegion& damage, bool& hasGlass) {
    auto expandedDamage = damage.copy();

    for (const auto& elementData : self->m_passElements) {
        auto* glass = dynamic_cast<CGlassPassElement*>(elementData->element.get());
        auto* layerGlass = dynamic_cast<CLayerGlassPassElement*>(elementData->element.get());
        if (!glass && !layerGlass)
            continue;

        hasGlass = true;
        const auto box = glass ? glass->boundingBox() : layerGlass->boundingBox();
        if (box)
            expandedDamage.add(*box);
    }

    return expandedDamage;
}

void notifyError(std::string_view message) {
    const std::string fullMessage = std::string("[liquidglass] ") + std::string(message);
    Log::logger->log(Log::ERR, "{}", fullMessage);
    if (g_pluginHandle)
        HyprlandAPI::addNotification(g_pluginHandle, fullMessage, CHyprColor(0xFFFF3355), 8000);
}

static CRegion hookRenderPassRender(Render::CRenderPass* self, const CRegion& damage) {
    if (g_state && g_pluginHandle && enabled()) {
        const float opacity = windowOpacity();
        const float matchedLayerOpacity = layerOpacity();
        for (auto& elementData : self->m_passElements) {
            auto* surface = dynamic_cast<CSurfacePassElement*>(elementData->element.get());
            if (surface && surface->m_data.pWindow && !isExcludedWindow(surface->m_data.pWindow)) {
                // LiquidGlass provides its own sampled blur. Leaving Hyprland's
                // surface blur enabled can make blur:xray show wallpaper only.
                surface->m_data.blur = false;

                if (opacity < 0.999F)
                    surface->m_data.alpha = std::min(surface->m_data.alpha, opacity);
            }

            if (shouldOverrideLayerSurface(surface)) {
                surface->m_data.blur = false;

                if (matchedLayerOpacity < 0.999F)
                    surface->m_data.alpha = std::min(surface->m_data.alpha, matchedLayerOpacity);
            }
        }
    }

    if (g_state && g_pluginHandle && enabled()) {
        bool hasGlass = false;
        auto expandedDamage = damageWithGlassBoxes(self, damage, hasGlass);
        if (hasGlass)
            return g_originalRenderPassRender(self, expandedDamage);
    }

    return g_originalRenderPassRender(self, damage);
}

static bool rendersMainPass(Render::eRenderPassMode mode) {
    return mode == Render::RENDER_PASS_ALL || mode == Render::RENDER_PASS_MAIN;
}

static float effectiveWindowAlpha(PHLWINDOW window) {
    return window->effectiveAlpha();
}

static bool shouldInjectFullscreenGlass(PHLWINDOW window, PHLMONITOR monitor, Render::eRenderPassMode mode, bool standalone) {
    return g_state && g_pluginHandle && enabled() && window && monitor && !standalone && rendersMainPass(mode) && window->m_isMapped && !window->m_fadingOut &&
           !window->isHidden() && window->isEffectiveInternalFSMode(FSMODE_FULLSCREEN) && !isExcludedWindow(window);
}

static void hookRendererRenderWindow(Render::IHyprRenderer* self, PHLWINDOW window, PHLMONITOR monitor, const Time::steady_tp& time, bool decorate,
                                     Render::eRenderPassMode mode, bool ignorePosition, bool standalone) {
    if (shouldInjectFullscreenGlass(window, monitor, mode, standalone)) {
        const float alpha = effectiveWindowAlpha(window);
        if (alpha > 0.001F) {
            if (auto* decoration = glassDecorationFor(window))
                self->m_renderPass.add(makeUnique<CGlassPassElement>(CGlassPassElement::SGlassPassData{decoration, alpha}));
        }
    }

    g_originalRenderWindow(self, window, monitor, time, decorate, mode, ignorePosition, standalone);
}

static bool shouldInjectLayerGlass(PHLLS layer, PHLMONITOR monitor, bool popups, bool lockscreen) {
    if (!g_state || !g_pluginHandle || !enabled() || !layer || !monitor || popups || lockscreen || !layer->visible() || layer->m_alpha->value() <= 0.001F ||
        !isIncludedLayer(layer))
        return false;

    return !isFullscreenLayerOverlay(layer, monitor);
}

static void hookRendererRenderLayer(Render::IHyprRenderer* self, PHLLS layer, PHLMONITOR monitor, const Time::steady_tp& time, bool popups, bool lockscreen) {
    if (shouldInjectLayerGlass(layer, monitor, popups, lockscreen)) {
        if (auto* state = layerStateFor(layer)) {
            const auto surface = layer->wlSurface() ? layer->wlSurface()->resource() : nullptr;
            const auto maskTexture = surface ? surface->m_current.texture : nullptr;
            self->m_renderPass.add(
                makeUnique<CLayerGlassPassElement>(CLayerGlassPassElement::SLayerGlassPassData{layer, state, maskTexture, layer->m_alpha->value()}));
        }
    }

    g_originalRenderLayer(self, layer, monitor, time, popups, lockscreen);
}

static bool hookWindowOpaque(Desktop::View::CWindow* window) {
    if (g_state && g_pluginHandle && enabled() && windowOpacity() < 0.999F && !isExcludedWindow(window->m_self.lock()))
        return false;

    return g_originalWindowOpaque(window);
}

static std::string lower(std::string value) {
    std::ranges::transform(value, value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

static std::string_view trim(std::string_view value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
        value.remove_prefix(1);
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
        value.remove_suffix(1);
    return value;
}

static bool commaListContains(std::string_view list, std::string_view needle) {
    const std::string wanted = lower(std::string(needle));
    std::string_view rest = list;

    while (!rest.empty()) {
        const auto comma = rest.find(',');
        const auto token = trim(rest.substr(0, comma));
        if (!token.empty() && lower(std::string(token)) == wanted)
            return true;

        if (comma == std::string_view::npos)
            break;
        rest.remove_prefix(comma + 1);
    }

    return false;
}

static bool isExcludedWindow(PHLWINDOW window) {
    if (!window)
        return true;

    const auto excludes = configString(CFG_EXCLUDE_CLASSES, DEFAULT_EXCLUDE_CLASSES);
    return commaListContains(excludes, window->m_class) || commaListContains(excludes, window->m_initialClass);
}

static bool isIncludedLayer(PHLLS layer) {
    if (!layer)
        return false;

    const auto namespaces = configString(CFG_LAYER_NAMESPACES, DEFAULT_LAYER_NAMESPACES);
    return commaListContains(namespaces, layer->m_namespace);
}

static bool isFullscreenLayerOverlay(PHLLS layer, PHLMONITOR monitor) {
    if (!layer || !monitor)
        return false;

    const auto box = layer->surfaceLogicalBox();
    if (!box)
        return false;

    return box->width >= monitor->m_size.x * 0.90 && box->height >= monitor->m_size.y * 0.90;
}

static bool shouldOverrideLayerSurface(CSurfacePassElement* surface) {
    if (!surface || !surface->m_data.pLS || surface->m_data.popup || !isIncludedLayer(surface->m_data.pLS))
        return false;

    auto monitor = surface->m_data.pMonitor.lock();
    if (!monitor)
        monitor = surface->m_data.pLS->m_monitor.lock();

    return !isFullscreenLayerOverlay(surface->m_data.pLS, monitor);
}

static SLayerGlassState* layerStateFor(PHLLS layer) {
    if (!g_state || !layer)
        return nullptr;

    std::erase_if(g_state->layerStates, [](const auto& state) { return !state || state->layer.expired(); });

    const auto found = std::ranges::find_if(g_state->layerStates, [&](const auto& state) {
        const auto locked = state->layer.lock();
        return locked && locked == layer;
    });

    if (found != g_state->layerStates.end())
        return found->get();

    auto state = makeUnique<SLayerGlassState>();
    state->layer = layer;
    auto* raw = state.get();
    g_state->layerStates.emplace_back(std::move(state));
    return raw;
}

static CFunctionHook* installHook(const std::string& demangledNeedle, const std::string& methodName, void* hookFunction, void** originalOut) {
    const auto matches = HyprlandAPI::findFunctionsByName(g_pluginHandle, methodName);
    const auto found = std::ranges::find_if(matches, [&](const SFunctionMatch& match) { return match.demangled.find(demangledNeedle) != std::string::npos; });

    if (found == matches.end())
        throw std::runtime_error("failed to find hook target " + demangledNeedle);

    auto* hook = HyprlandAPI::createFunctionHook(g_pluginHandle, found->address, hookFunction);
    if (!hook || !hook->hook())
        throw std::runtime_error("failed to install hook " + demangledNeedle);

    *originalOut = hook->m_original;
    return hook;
}

static bool hasDecoration(PHLWINDOW window) {
    return std::ranges::any_of(window->m_windowDecorations, [](const auto& decoration) { return decoration && decoration->getDisplayName() == "LiquidGlass"; });
}

static CGlassDecoration* glassDecorationFor(PHLWINDOW window) {
    if (!window)
        return nullptr;

    for (const auto& decoration : window->m_windowDecorations) {
        if (decoration && decoration->getDisplayName() == "LiquidGlass")
            return dynamic_cast<CGlassDecoration*>(decoration.get());
    }

    return nullptr;
}

static void removeWindow(PHLWINDOW window) {
    if (!window)
        return;

    for (const auto& decoration : window->m_windowDecorations) {
        if (decoration && decoration->getDisplayName() == "LiquidGlass") {
            window->removeWindowDeco(decoration.get());
            break;
        }
    }

    closeWindow(window);
}

static void addWindow(PHLWINDOW window) {
    if (!window || isExcludedWindow(window) || hasDecoration(window))
        return;

    auto decoration = makeUnique<CGlassDecoration>(window);
    g_state->decorations.emplace_back(decoration);
    decoration->m_self = decoration;
    HyprlandAPI::addWindowDecoration(g_pluginHandle, window, std::move(decoration));
}

static void closeWindow(PHLWINDOW window) {
    std::erase_if(g_state->decorations, [&window](const auto& decoration) {
        const auto locked = decoration.lock();
        return !locked || locked->owner() == window;
    });
}

static void syncWindow(PHLWINDOW window) {
    if (!window)
        return;

    if (!enabled() || isExcludedWindow(window))
        removeWindow(window);
    else
        addWindow(window);
}

static void syncWindows() {
    for (const auto& window : g_pCompositor->m_windows)
        syncWindow(window);
}

static void addConfigValues() {
    clearConfigValues();

    const bool ok = addIntConfig(CFG_ENABLED, 1) && addStringConfig(CFG_EXCLUDE_CLASSES, DEFAULT_EXCLUDE_CLASSES) &&
                    addStringConfig(CFG_LAYER_NAMESPACES, DEFAULT_LAYER_NAMESPACES) && addFloatConfig(CFG_WINDOW_OPACITY, DEFAULT_WINDOW_OPACITY) &&
                    addFloatConfig(CFG_LAYER_OPACITY, DEFAULT_LAYER_OPACITY) && addFloatConfig(CFG_LAYER_CORNER_RADIUS, DEFAULT_LAYER_CORNER_RADIUS) &&
                    addFloatConfig(CFG_BLUR_STRENGTH, DEFAULT_BLUR_STRENGTH) && addIntConfig(CFG_BLUR_ITERATIONS, DEFAULT_BLUR_ITERATIONS) &&
                    addFloatConfig(CFG_REFRACTION_STRENGTH, DEFAULT_REFRACTION_STRENGTH) &&
                    addFloatConfig(CFG_CHROMATIC_ABERRATION, DEFAULT_CHROMATIC_ABERRATION) && addFloatConfig(CFG_FRESNEL_STRENGTH, DEFAULT_FRESNEL_STRENGTH) &&
                    addFloatConfig(CFG_SPECULAR_STRENGTH, DEFAULT_SPECULAR_STRENGTH) && addFloatConfig(CFG_GLASS_OPACITY, DEFAULT_GLASS_OPACITY) &&
                    addFloatConfig(CFG_EDGE_THICKNESS, DEFAULT_EDGE_THICKNESS) && addIntConfig(CFG_TINT_COLOR, DEFAULT_TINT_COLOR) &&
                    addFloatConfig(CFG_LENS_DISTORTION, DEFAULT_LENS_DISTORTION) && addFloatConfig(CFG_BRIGHTNESS, DEFAULT_BRIGHTNESS) &&
                    addFloatConfig(CFG_CONTRAST, DEFAULT_CONTRAST) && addFloatConfig(CFG_SATURATION, DEFAULT_SATURATION) &&
                    addFloatConfig(CFG_VIBRANCY, DEFAULT_VIBRANCY) && addFloatConfig(CFG_ADAPTIVE_DIM, DEFAULT_ADAPTIVE_DIM) &&
                    addFloatConfig(CFG_ADAPTIVE_BOOST, DEFAULT_ADAPTIVE_BOOST);

    if (!ok)
        throw std::runtime_error("failed to register liquidglass config values");
}

} // namespace LiquidGlass

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    using namespace LiquidGlass;

    g_pluginHandle = handle;

    const std::string clientHash = __hyprland_api_get_client_hash();
    const std::string compositorHash = __hyprland_api_get_hash();
    if (clientHash != compositorHash) {
        notifyError("version mismatch: rebuild against the running Hyprland headers");
        throw std::runtime_error("Hyprland ABI mismatch");
    }

    g_state = makeUnique<SGlobalState>();
    addConfigValues();

    g_state->renderPassHook = installHook("Render::CRenderPass::render(", "render", reinterpret_cast<void*>(hookRenderPassRender),
                                          reinterpret_cast<void**>(&g_originalRenderPassRender));
    g_state->renderWindowHook = installHook("Render::IHyprRenderer::renderWindow(", "renderWindow", reinterpret_cast<void*>(hookRendererRenderWindow),
                                            reinterpret_cast<void**>(&g_originalRenderWindow));
    g_state->renderLayerHook = installHook("Render::IHyprRenderer::renderLayer(", "renderLayer", reinterpret_cast<void*>(hookRendererRenderLayer),
                                           reinterpret_cast<void**>(&g_originalRenderLayer));
    g_state->windowOpaqueHook =
        installHook("Desktop::View::CWindow::opaque(", "opaque", reinterpret_cast<void*>(hookWindowOpaque), reinterpret_cast<void**>(&g_originalWindowOpaque));

    g_state->listeners.emplace_back(Event::bus()->m_events.window.open.listen([](PHLWINDOW window) { syncWindow(window); }));
    g_state->listeners.emplace_back(Event::bus()->m_events.window.destroy.listen([](PHLWINDOW window) { closeWindow(window); }));
    g_state->listeners.emplace_back(Event::bus()->m_events.config.reloaded.listen([] { syncWindows(); }));

    HyprlandAPI::reloadConfig();
    syncWindows();

    return {std::string(PLUGIN_NAME), std::string(PLUGIN_DESCRIPTION), std::string(PLUGIN_AUTHOR), std::string(PLUGIN_VERSION)};
}

APICALL EXPORT void PLUGIN_EXIT() {
    using namespace LiquidGlass;

    if (!g_state)
        return;

    for (auto& decoration : g_state->decorations) {
        const auto locked = decoration.lock();
        if (!locked)
            continue;

        const auto window = locked->owner();
        if (window)
            window->removeWindowDeco(locked.get());
    }

    g_pHyprRenderer->m_renderPass.removeAllOfType("CGlassPassElement");
    g_pHyprRenderer->m_renderPass.removeAllOfType("CLayerGlassPassElement");
    g_state->shaderManager.destroy();
    g_state.reset();
    clearConfigValues();
    g_pluginHandle = nullptr;
}

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}
