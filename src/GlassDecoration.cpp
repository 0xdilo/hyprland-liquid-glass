#include "GlassDecoration.hpp"

#include <algorithm>

#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprutils/math/Misc.hpp>

#include "Config.hpp"
#include "GlassPassElement.hpp"
#include "GlassRenderer.hpp"
#include "Globals.hpp"
#include "WindowGeometry.hpp"

using namespace Render::GL;

CGlassDecoration::CGlassDecoration(PHLWINDOW window) : IHyprWindowDecoration(window), m_window(window) {}

SDecorationPositioningInfo CGlassDecoration::getPositioningInfo() {
    SDecorationPositioningInfo info;
    info.priority = 10000;
    info.policy = DECORATION_POSITION_ABSOLUTE;
    info.desiredExtents = {{0, 0}, {0, 0}};
    return info;
}

void CGlassDecoration::onPositioningReply(const SDecorationPositioningReply&) {}

void CGlassDecoration::draw(PHLMONITOR monitor, float const& alpha) {
    if (!LiquidGlass::g_state || !LiquidGlass::enabled())
        return;

    if (alpha <= 0.001F)
        return;

    g_pHyprRenderer->m_renderPass.add(makeUnique<CGlassPassElement>(CGlassPassElement::SGlassPassData{this, alpha}));

    const auto window = m_window.lock();
    if (!window)
        return;

    const auto workspace = window->m_workspace;
    const bool workspaceAnimating = workspace && !window->m_pinned && workspace->m_renderOffset->isBeingAnimated();
    if (workspaceAnimating)
        damageEntire();

    const auto currentPosition = window->m_realPosition->value();
    const auto currentSize = window->m_realSize->value();
    if (currentPosition != m_lastPosition || currentSize != m_lastSize) {
        damageEntire();
        m_lastPosition = currentPosition;
        m_lastSize = currentSize;
    }
}

PHLWINDOW CGlassDecoration::owner() {
    return m_window.lock();
}

void CGlassDecoration::renderPass(PHLMONITOR monitor, float alpha) {
    if (!LiquidGlass::g_state || !LiquidGlass::enabled())
        return;

    if (alpha <= 0.001F)
        return;

    auto& shaderManager = LiquidGlass::g_state->shaderManager;
    shaderManager.initializeIfNeeded();
    if (!shaderManager.initialized())
        return;

    const auto window = m_window.lock();
    if (!window || !monitor || !g_pHyprRenderer->m_renderData.currentFB)
        return;

    auto box = LiquidGlass::WindowGeometry::computeWindowBox(window, monitor);
    if (!box)
        return;

    CBox windowBox = *box;
    CBox transformedBox = windowBox;
    const auto transform = Math::wlTransformToHyprutils(Math::invertTransform(monitor->m_transform));
    transformedBox.transform(transform, monitor->m_transformedSize.x, monitor->m_transformedSize.y);
    if (transformedBox.width <= 1.0 || transformedBox.height <= 1.0)
        return;

    const auto blurStrength = std::clamp(LiquidGlass::configFloat(LiquidGlass::CFG_BLUR_STRENGTH, LiquidGlass::DEFAULT_BLUR_STRENGTH), 0.0F, 4.0F);
    const int downscale = blurStrength >= LiquidGlass::GlassRenderer::BLUR_DOWNSCALE_THRESHOLD ? LiquidGlass::GlassRenderer::BLUR_DOWNSCALE_MAX : 1;

    auto source = g_pHyprRenderer->m_renderData.currentFB;
    if (!LiquidGlass::GlassRenderer::sampleBackground(m_sampleFramebuffer, source, transformedBox, m_samplePaddingRatio, downscale))
        return;
    auto* sourceGL = GLFB(source);
    if (!sourceGL)
        return;

    const float blurRadius = blurStrength * LiquidGlass::GlassRenderer::BLUR_RADIUS_SCALE / static_cast<float>(downscale);
    const int blurIterations =
        std::clamp(static_cast<int>(LiquidGlass::configInt(LiquidGlass::CFG_BLUR_ITERATIONS, LiquidGlass::DEFAULT_BLUR_ITERATIONS)), 1, 5);
    const int viewportWidth = static_cast<int>(monitor->m_transformedSize.x);
    const int viewportHeight = static_cast<int>(monitor->m_transformedSize.y);
    LiquidGlass::GlassRenderer::blurBackground(m_sampleFramebuffer, blurRadius, blurIterations, sourceGL->getFBID(), viewportWidth, viewportHeight);

    const float monitorScale = monitor->m_scale;
    const float cornerRadius = window->rounding() * monitorScale;
    const float roundingPower = window->roundingPower();

    LiquidGlass::GlassRenderer::applyGlassEffect(m_sampleFramebuffer, source, windowBox, transformedBox, alpha, cornerRadius, roundingPower,
                                                 m_samplePaddingRatio);
}

eDecorationType CGlassDecoration::getDecorationType() {
    return DECORATION_CUSTOM;
}

void CGlassDecoration::updateWindow(PHLWINDOW) {
    damageEntire();
}

void CGlassDecoration::damageEntire() {
    const auto window = m_window.lock();
    if (!window)
        return;

    const auto workspace = window->m_workspace;
    auto box = window->getWindowMainSurfaceBox();
    if (workspace && workspace->m_renderOffset->isBeingAnimated() && !window->m_pinned)
        box.translate(workspace->m_renderOffset->value());
    box.translate(window->m_floatingOffset);

    const auto monitor = window->m_monitor.lock();
    const float scale = monitor ? monitor->m_scale : 1.0F;
    box.expand(static_cast<double>(LiquidGlass::GlassRenderer::SAMPLE_PADDING_PX) / scale);
    g_pHyprRenderer->damageBox(box);
}

eDecorationLayer CGlassDecoration::getDecorationLayer() {
    return DECORATION_LAYER_BOTTOM;
}

uint64_t CGlassDecoration::getDecorationFlags() {
    return DECORATION_NON_SOLID;
}

std::string CGlassDecoration::getDisplayName() {
    return "LiquidGlass";
}
