#pragma once

#include <hyprland/src/desktop/view/LayerSurface.hpp>
#include <hyprland/src/render/Texture.hpp>
#include <hyprland/src/render/pass/PassElement.hpp>

#include "Globals.hpp"

class CLayerGlassPassElement : public IPassElement {
  public:
    struct SLayerGlassPassData {
        PHLLS layer;
        LiquidGlass::SLayerGlassState* state = nullptr;
        SP<Render::ITexture> maskTexture;
        float alpha = 1.0F;
    };

    explicit CLayerGlassPassElement(const SLayerGlassPassData& data);

    std::vector<UP<IPassElement>> draw() override;
    bool needsLiveBlur() override;
    bool needsPrecomputeBlur() override;
    bool disableSimplification() override;
    bool undiscardable() override;
    std::optional<CBox> boundingBox() override;
    ePassElementType type() override;

    const char* passName() override {
        return "CLayerGlassPassElement";
    }

  private:
    SLayerGlassPassData m_data;
};
