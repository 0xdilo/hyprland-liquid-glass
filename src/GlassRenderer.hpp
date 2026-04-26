#pragma once

#include <hyprland/src/helpers/math/Math.hpp>
#include <hyprland/src/render/Framebuffer.hpp>

namespace LiquidCock::GlassRenderer {

inline constexpr int SAMPLE_PADDING_PX = 96;
inline constexpr float BLUR_DOWNSCALE_THRESHOLD = 0.35f;
inline constexpr int BLUR_DOWNSCALE_MAX = 2;

void sampleBackground(CFramebuffer& sampleFramebuffer, CFramebuffer& sourceFramebuffer, CBox box, Vector2D& outPaddingRatio, int downscale);
void blurBackground(CFramebuffer& sampleFramebuffer, float radius, int iterations, GLuint callerFramebufferID, int viewportWidth, int viewportHeight);
void applyGlassEffect(CFramebuffer& sampleFramebuffer, CFramebuffer& targetFramebuffer, CBox& rawBox, CBox& transformedBox, float alpha, float cornerRadius,
                      float roundingPower, const Vector2D& paddingRatio);

} // namespace LiquidCock::GlassRenderer
