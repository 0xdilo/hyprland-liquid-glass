#pragma once

#include <optional>

#include <hyprland/src/desktop/view/LayerSurface.hpp>

namespace LiquidGlass::LayerGeometry {

[[nodiscard]] inline std::optional<CBox> computeLayerBox(PHLLS layer, PHLMONITOR monitor) {
    if (!layer || !monitor || !layer->visible())
        return std::nullopt;

    auto box = layer->surfaceLogicalBox();
    if (!box)
        return std::nullopt;

    box->translate(-monitor->m_position);
    box->scale(monitor->m_scale);
    box->round();
    return box;
}

} // namespace LiquidGlass::LayerGeometry
