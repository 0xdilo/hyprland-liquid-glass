#pragma once

#include <hyprland/src/helpers/math/Math.hpp>
#include <hyprland/src/render/Framebuffer.hpp>
#include <hyprland/src/render/Texture.hpp>

namespace LiquidGlass::GlassRenderer {

inline constexpr int SAMPLE_PADDING_PX = 96;
inline constexpr float BLUR_RADIUS_SCALE = 6.25f;
inline constexpr float BLUR_DOWNSCALE_THRESHOLD = 0.35f;
inline constexpr int BLUR_DOWNSCALE_MAX = 2;

bool sampleBackground(SP<Render::IFramebuffer>& sampleFramebuffer, SP<Render::IFramebuffer> sourceFramebuffer, CBox box, Vector2D& outPaddingRatio,
                      int downscale);
void blurBackground(SP<Render::IFramebuffer> sampleFramebuffer, float radius, int iterations, GLuint callerFramebufferID, int viewportWidth,
                    int viewportHeight);
void applyGlassEffect(SP<Render::IFramebuffer> sampleFramebuffer, SP<Render::IFramebuffer> targetFramebuffer, CBox& rawBox, CBox& transformedBox, float alpha,
                      float cornerRadius, float roundingPower, const Vector2D& paddingRatio, SP<Render::ITexture> maskTexture = nullptr);

} // namespace LiquidGlass::GlassRenderer
