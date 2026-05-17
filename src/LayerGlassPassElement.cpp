#include "LayerGlassPassElement.hpp"

#include <algorithm>

#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprutils/math/Misc.hpp>

#include "Config.hpp"
#include "GlassRenderer.hpp"
#include "LayerGeometry.hpp"

using namespace Render::GL;

CLayerGlassPassElement::CLayerGlassPassElement(const SLayerGlassPassData& data) : m_data(data) {}

std::vector<UP<IPassElement>> CLayerGlassPassElement::draw() {
    if (!LiquidGlass::g_state || !LiquidGlass::enabled() || !m_data.layer || !m_data.state)
        return {};

    const float alpha = std::clamp(m_data.alpha, 0.0F, 1.0F);
    if (alpha <= 0.001F)
        return {};

    auto& shaderManager = LiquidGlass::g_state->shaderManager;
    shaderManager.initializeIfNeeded();
    if (!shaderManager.initialized())
        return {};

    const auto monitor = g_pHyprRenderer->m_renderData.pMonitor.lock();
    if (!monitor || !g_pHyprRenderer->m_renderData.currentFB)
        return {};

    auto box = LiquidGlass::LayerGeometry::computeLayerBox(m_data.layer, monitor);
    if (!box)
        return {};

    CBox layerBox = *box;
    CBox transformedBox = layerBox;
    const auto transform = Math::wlTransformToHyprutils(Math::invertTransform(monitor->m_transform));
    transformedBox.transform(transform, monitor->m_transformedSize.x, monitor->m_transformedSize.y);
    if (transformedBox.width <= 1.0 || transformedBox.height <= 1.0)
        return {};

    const auto blurStrength = std::clamp(LiquidGlass::configFloat(LiquidGlass::CFG_BLUR_STRENGTH, LiquidGlass::DEFAULT_BLUR_STRENGTH), 0.0F, 4.0F);
    const int downscale = blurStrength >= LiquidGlass::GlassRenderer::BLUR_DOWNSCALE_THRESHOLD ? LiquidGlass::GlassRenderer::BLUR_DOWNSCALE_MAX : 1;

    auto source = g_pHyprRenderer->m_renderData.currentFB;
    if (!LiquidGlass::GlassRenderer::sampleBackground(m_data.state->sampleFramebuffer, source, transformedBox, m_data.state->samplePaddingRatio, downscale))
        return {};
    auto* sourceGL = GLFB(source);
    if (!sourceGL)
        return {};

    const float blurRadius = blurStrength * LiquidGlass::GlassRenderer::BLUR_RADIUS_SCALE / static_cast<float>(downscale);
    const int blurIterations =
        std::clamp(static_cast<int>(LiquidGlass::configInt(LiquidGlass::CFG_BLUR_ITERATIONS, LiquidGlass::DEFAULT_BLUR_ITERATIONS)), 1, 5);
    const int viewportWidth = static_cast<int>(monitor->m_transformedSize.x);
    const int viewportHeight = static_cast<int>(monitor->m_transformedSize.y);
    LiquidGlass::GlassRenderer::blurBackground(m_data.state->sampleFramebuffer, blurRadius, blurIterations, sourceGL->getFBID(), viewportWidth, viewportHeight);

    const float cornerRadius = LiquidGlass::layerCornerRadius() * monitor->m_scale;
    LiquidGlass::GlassRenderer::applyGlassEffect(m_data.state->sampleFramebuffer, source, layerBox, transformedBox, alpha, cornerRadius, 3.0F,
                                                 m_data.state->samplePaddingRatio, m_data.maskTexture);
    return {};
}

std::optional<CBox> CLayerGlassPassElement::boundingBox() {
    if (!m_data.layer)
        return std::nullopt;

    const auto monitor = g_pHyprRenderer->m_renderData.pMonitor.lock();
    auto box = LiquidGlass::LayerGeometry::computeLayerBox(m_data.layer, monitor);
    if (!box || !monitor)
        return std::nullopt;

    box->expand(static_cast<double>(LiquidGlass::GlassRenderer::SAMPLE_PADDING_PX) / monitor->m_scale);
    return box;
}

bool CLayerGlassPassElement::needsLiveBlur() {
    return false;
}

bool CLayerGlassPassElement::needsPrecomputeBlur() {
    return false;
}

bool CLayerGlassPassElement::disableSimplification() {
    return false;
}

bool CLayerGlassPassElement::undiscardable() {
    return false;
}

ePassElementType CLayerGlassPassElement::type() {
    return EK_CUSTOM;
}
