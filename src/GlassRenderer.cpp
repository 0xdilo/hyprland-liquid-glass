#include "GlassRenderer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

#include <GLES3/gl32.h>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprutils/math/Misc.hpp>

#include "Config.hpp"
#include "Globals.hpp"

namespace LiquidGlass::GlassRenderer {

static void uploadToneUniforms() {
    const auto& uniforms = g_state->shaderManager.glassUniforms;
    const auto& shader = g_state->shaderManager.glassShader;

    shader->setUniformFloat(SHADER_BRIGHTNESS, std::clamp(configFloat(CFG_BRIGHTNESS, DEFAULT_BRIGHTNESS), 0.0F, 2.0F));
    shader->setUniformFloat(SHADER_CONTRAST, std::clamp(configFloat(CFG_CONTRAST, DEFAULT_CONTRAST), 0.0F, 2.5F));
    shader->setUniformFloat(SHADER_VIBRANCY, std::clamp(configFloat(CFG_VIBRANCY, DEFAULT_VIBRANCY), 0.0F, 1.0F));
    glUniform1f(uniforms.saturation, std::clamp(configFloat(CFG_SATURATION, DEFAULT_SATURATION), 0.0F, 2.0F));
    glUniform1f(uniforms.adaptiveDim, std::clamp(configFloat(CFG_ADAPTIVE_DIM, DEFAULT_ADAPTIVE_DIM), 0.0F, 1.0F));
    glUniform1f(uniforms.adaptiveBoost, std::clamp(configFloat(CFG_ADAPTIVE_BOOST, DEFAULT_ADAPTIVE_BOOST), 0.0F, 1.0F));
}

void sampleBackground(CFramebuffer& sampleFramebuffer, CFramebuffer& sourceFramebuffer, CBox box, Vector2D& outPaddingRatio, int downscale) {
    const int pad = SAMPLE_PADDING_PX;
    const int fullWidth = static_cast<int>(box.width) + 2 * pad;
    const int fullHeight = static_cast<int>(box.height) + 2 * pad;
    const int sampleWidth = std::max(1, fullWidth / downscale);
    const int sampleHeight = std::max(1, fullHeight / downscale);

    if (sampleFramebuffer.m_size.x != sampleWidth || sampleFramebuffer.m_size.y != sampleHeight)
        sampleFramebuffer.alloc(sampleWidth, sampleHeight, sourceFramebuffer.m_drmFormat);

    int srcX0 = static_cast<int>(box.x) - pad;
    int srcX1 = static_cast<int>(box.x + box.width) + pad;
    int srcY0 = static_cast<int>(box.y) - pad;
    int srcY1 = static_cast<int>(box.y + box.height) + pad;

    const int framebufferWidth = static_cast<int>(sourceFramebuffer.m_size.x);
    const int framebufferHeight = static_cast<int>(sourceFramebuffer.m_size.y);

    int dstX0 = 0;
    int dstY0 = 0;
    int dstX1 = sampleWidth;
    int dstY1 = sampleHeight;

    const float xScale = static_cast<float>(sampleWidth) / static_cast<float>(fullWidth);
    const float yScale = static_cast<float>(sampleHeight) / static_cast<float>(fullHeight);

    if (srcX0 < 0) {
        dstX0 += static_cast<int>(static_cast<float>(-srcX0) * xScale);
        srcX0 = 0;
    }
    if (srcY0 < 0) {
        dstY0 += static_cast<int>(static_cast<float>(-srcY0) * yScale);
        srcY0 = 0;
    }
    if (srcX1 > framebufferWidth) {
        dstX1 -= static_cast<int>(static_cast<float>(srcX1 - framebufferWidth) * xScale);
        srcX1 = framebufferWidth;
    }
    if (srcY1 > framebufferHeight) {
        dstY1 -= static_cast<int>(static_cast<float>(srcY1 - framebufferHeight) * yScale);
        srcY1 = framebufferHeight;
    }

    outPaddingRatio = Vector2D(static_cast<double>(pad) / static_cast<double>(fullWidth), static_cast<double>(pad) / static_cast<double>(fullHeight));

    g_pHyprOpenGL->setCapStatus(GL_SCISSOR_TEST, false);

    glBindFramebuffer(GL_FRAMEBUFFER, sampleFramebuffer.getFBID());
    glClearColor(0.0F, 0.0F, 0.0F, 0.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, sourceFramebuffer.getFBID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sampleFramebuffer.getFBID());
    glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void blurBackground(CFramebuffer& sampleFramebuffer, float radius, int iterations, GLuint callerFramebufferID, int viewportWidth, int viewportHeight) {
    auto& shaderManager = g_state->shaderManager;
    if (radius <= 0.0F || iterations <= 0 || !shaderManager.initialized())
        return;

    const int width = static_cast<int>(sampleFramebuffer.m_size.x);
    const int height = static_cast<int>(sampleFramebuffer.m_size.y);

    auto& blurTempFramebuffer = g_state->blurTempFramebuffer;
    if (blurTempFramebuffer.m_size.x != width || blurTempFramebuffer.m_size.y != height)
        blurTempFramebuffer.alloc(width, height, sampleFramebuffer.m_drmFormat);

    static constexpr std::array<float, 9> FULLSCREEN_PROJECTION = {
        2.0F, 0.0F, 0.0F, 0.0F, 2.0F, 0.0F, -1.0F, -1.0F, 1.0F,
    };

    const auto& uniforms = shaderManager.blurUniforms;
    auto shader = g_pHyprOpenGL->useShader(shaderManager.blurShader);
    shader->setUniformMatrix3fv(SHADER_PROJ, 1, GL_FALSE, FULLSCREEN_PROJECTION);
    shader->setUniformInt(SHADER_TEX, 0);
    glUniform1f(uniforms.radius, radius);
    glBindVertexArray(shader->getUniformLocation(SHADER_SHADER_VAO));
    g_pHyprOpenGL->setViewport(0, 0, width, height);
    glActiveTexture(GL_TEXTURE0);

    for (int iteration = 0; iteration < iterations; ++iteration) {
        glBindFramebuffer(GL_FRAMEBUFFER, blurTempFramebuffer.getFBID());
        sampleFramebuffer.getTexture()->bind();
        glUniform2f(uniforms.direction, 1.0F / static_cast<float>(width), 0.0F);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindFramebuffer(GL_FRAMEBUFFER, sampleFramebuffer.getFBID());
        blurTempFramebuffer.getTexture()->bind();
        glUniform2f(uniforms.direction, 0.0F, 1.0F / static_cast<float>(height));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, callerFramebufferID);
    glBindVertexArray(0);
    g_pHyprOpenGL->setViewport(0, 0, viewportWidth, viewportHeight);
}

void applyGlassEffect(CFramebuffer& sampleFramebuffer, CFramebuffer& targetFramebuffer, CBox& rawBox, CBox& transformedBox, float alpha, float cornerRadius,
                      float roundingPower, const Vector2D& paddingRatio, SP<CTexture> maskTexture) {
    auto& shaderManager = g_state->shaderManager;
    const auto& uniforms = shaderManager.glassUniforms;

    const auto transform = Math::wlTransformToHyprutils(Math::invertTransform(g_pHyprOpenGL->m_renderData.pMonitor->m_transform));
    Mat3x3 matrix = g_pHyprOpenGL->m_renderData.monitorProjection.projectBox(rawBox, transform, rawBox.rot);
    Mat3x3 glMatrix = g_pHyprOpenGL->m_renderData.projection.copy().multiply(matrix);
    glMatrix.transpose();

    glBindFramebuffer(GL_FRAMEBUFFER, targetFramebuffer.getFBID());
    glActiveTexture(GL_TEXTURE0);
    sampleFramebuffer.getTexture()->bind();

    auto shader = g_pHyprOpenGL->useShader(shaderManager.glassShader);
    shader->setUniformMatrix3fv(SHADER_PROJ, 1, GL_FALSE, glMatrix.getMatrix());
    shader->setUniformInt(SHADER_TEX, 0);
    glUniform1i(uniforms.maskTexture, 1);
    glUniform1i(uniforms.useMask, maskTexture ? 1 : 0);
    shader->setUniformFloat2(SHADER_FULL_SIZE, static_cast<float>(transformedBox.width), static_cast<float>(transformedBox.height));
    shader->setUniformFloat(SHADER_RADIUS, cornerRadius);
    shader->setUniformFloat(SHADER_ROUNDING_POWER, roundingPower);

    glUniform1f(uniforms.refractionStrength, std::clamp(configFloat(CFG_REFRACTION_STRENGTH, DEFAULT_REFRACTION_STRENGTH), 0.0f, 1.5f));
    glUniform1f(uniforms.chromaticAberration, std::clamp(configFloat(CFG_CHROMATIC_ABERRATION, DEFAULT_CHROMATIC_ABERRATION), 0.0f, 1.5f));
    glUniform1f(uniforms.fresnelStrength, std::clamp(configFloat(CFG_FRESNEL_STRENGTH, DEFAULT_FRESNEL_STRENGTH), 0.0f, 2.0f));
    glUniform1f(uniforms.specularStrength, std::clamp(configFloat(CFG_SPECULAR_STRENGTH, DEFAULT_SPECULAR_STRENGTH), 0.0f, 2.0f));
    glUniform1f(uniforms.glassOpacity, std::clamp(configFloat(CFG_GLASS_OPACITY, DEFAULT_GLASS_OPACITY), 0.0f, 1.0f) * alpha);
    glUniform1f(uniforms.edgeThickness, std::clamp(configFloat(CFG_EDGE_THICKNESS, DEFAULT_EDGE_THICKNESS), 0.005f, 0.20f));
    glUniform1f(uniforms.lensDistortion, std::clamp(configFloat(CFG_LENS_DISTORTION, DEFAULT_LENS_DISTORTION), 0.0f, 1.5f));
    glUniform2f(uniforms.uvPadding, static_cast<float>(paddingRatio.x), static_cast<float>(paddingRatio.y));

    const auto tint = static_cast<std::uint64_t>(configInt(CFG_TINT_COLOR, DEFAULT_TINT_COLOR));
    glUniform3f(uniforms.tintColor, static_cast<float>((tint >> 24U) & 0xFFU) / 255.0F, static_cast<float>((tint >> 16U) & 0xFFU) / 255.0F,
                static_cast<float>((tint >> 8U) & 0xFFU) / 255.0F);
    glUniform1f(uniforms.tintAlpha, static_cast<float>(tint & 0xFFU) / 255.0F);

    uploadToneUniforms();

    if (maskTexture) {
        glActiveTexture(GL_TEXTURE1);
        maskTexture->bind();
        glActiveTexture(GL_TEXTURE0);
    }

    glBindVertexArray(shader->getUniformLocation(SHADER_SHADER_VAO));
    g_pHyprOpenGL->scissor(rawBox);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    g_pHyprOpenGL->scissor(nullptr);
}

} // namespace LiquidGlass::GlassRenderer
