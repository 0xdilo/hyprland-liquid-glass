#pragma once

#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/render/Framebuffer.hpp>
#include <hyprland/src/render/decorations/IHyprWindowDecoration.hpp>

class CGlassDecoration : public IHyprWindowDecoration {
  public:
    explicit CGlassDecoration(PHLWINDOW window);
    ~CGlassDecoration() override = default;

    SDecorationPositioningInfo getPositioningInfo() override;
    void onPositioningReply(const SDecorationPositioningReply& reply) override;
    void draw(PHLMONITOR monitor, float const& alpha) override;
    eDecorationType getDecorationType() override;
    void updateWindow(PHLWINDOW window) override;
    void damageEntire() override;
    eDecorationLayer getDecorationLayer() override;
    uint64_t getDecorationFlags() override;
    std::string getDisplayName() override;

    [[nodiscard]] PHLWINDOW owner();
    void renderPass(PHLMONITOR monitor, float alpha);

    WP<CGlassDecoration> m_self;

  private:
    PHLWINDOWREF m_window;
    SP<Render::IFramebuffer> m_sampleFramebuffer;
    Vector2D m_samplePaddingRatio;
    Vector2D m_lastPosition;
    Vector2D m_lastSize;
};
