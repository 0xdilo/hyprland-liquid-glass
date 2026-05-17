#include "GlassPassElement.hpp"

#include "GlassDecoration.hpp"
#include "GlassRenderer.hpp"
#include "WindowGeometry.hpp"

#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Renderer.hpp>

using namespace Render::GL;

CGlassPassElement::CGlassPassElement(const SGlassPassData& data) : m_data(data) {}

std::vector<UP<IPassElement>> CGlassPassElement::draw() {
    if (!m_data.decoration)
        return {};

    m_data.decoration->renderPass(g_pHyprRenderer->m_renderData.pMonitor.lock(), m_data.alpha);
    return {};
}

std::optional<CBox> CGlassPassElement::boundingBox() {
    if (!m_data.decoration)
        return std::nullopt;

    const auto owner = m_data.decoration->owner();
    const auto monitor = g_pHyprRenderer->m_renderData.pMonitor.lock();
    auto box = LiquidGlass::WindowGeometry::computeWindowBox(owner, monitor);
    if (!box || !monitor)
        return std::nullopt;

    box->expand(static_cast<double>(LiquidGlass::GlassRenderer::SAMPLE_PADDING_PX) / monitor->m_scale);
    return box;
}

bool CGlassPassElement::needsLiveBlur() {
    return false;
}

bool CGlassPassElement::needsPrecomputeBlur() {
    return false;
}

bool CGlassPassElement::disableSimplification() {
    return false;
}

bool CGlassPassElement::undiscardable() {
    return false;
}

ePassElementType CGlassPassElement::type() {
    return EK_CUSTOM;
}
