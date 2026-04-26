#include "GlassPassElement.hpp"

#include "GlassDecoration.hpp"
#include "GlassRenderer.hpp"
#include "WindowGeometry.hpp"

#include <hyprland/src/render/OpenGL.hpp>

CGlassPassElement::CGlassPassElement(const SGlassPassData& data) : m_data(data) {}

void CGlassPassElement::draw(const CRegion&) {
    if (!m_data.decoration)
        return;

    m_data.decoration->renderPass(g_pHyprOpenGL->m_renderData.pMonitor.lock(), m_data.alpha);
}

std::optional<CBox> CGlassPassElement::boundingBox() {
    if (!m_data.decoration)
        return std::nullopt;

    const auto owner = m_data.decoration->owner();
    const auto monitor = g_pHyprOpenGL->m_renderData.pMonitor.lock();
    auto box = LiquidCock::WindowGeometry::computeWindowBox(owner, monitor);
    if (!box || !monitor)
        return std::nullopt;

    box->expand(static_cast<double>(LiquidCock::GlassRenderer::SAMPLE_PADDING_PX) / monitor->m_scale);
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
