#pragma once

#include <hyprland/src/render/pass/PassElement.hpp>

class CGlassDecoration;

class CGlassPassElement : public IPassElement {
  public:
    struct SGlassPassData {
        CGlassDecoration* decoration = nullptr;
        float alpha = 1.0F;
    };

    explicit CGlassPassElement(const SGlassPassData& data);

    std::vector<UP<IPassElement>> draw() override;
    bool needsLiveBlur() override;
    bool needsPrecomputeBlur() override;
    std::optional<CBox> boundingBox() override;
    bool disableSimplification() override;
    bool undiscardable() override;
    ePassElementType type() override;

    const char* passName() override {
        return "CGlassPassElement";
    }

  private:
    SGlassPassData m_data;
};
