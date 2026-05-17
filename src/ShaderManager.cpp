#include "ShaderManager.hpp"

#include <string>

#include <hyprland/src/render/OpenGL.hpp>

#include "Globals.hpp"
#include "Shaders.hpp"

using namespace Render::GL;

namespace LiquidGlass {

bool CShaderManager::compileGlassShader() {
    if (!glassShader->createProgram(g_pHyprOpenGL->m_shaders->TEXVERTSRC, Shaders::LIQUID_GLASS_FRAG, true, true)) {
        notifyError("failed to compile liquid glass shader");
        return false;
    }

    const auto program = glassShader->program();
    glassUniforms.refractionStrength = glGetUniformLocation(program, "refractionStrength");
    glassUniforms.chromaticAberration = glGetUniformLocation(program, "chromaticAberration");
    glassUniforms.fresnelStrength = glGetUniformLocation(program, "fresnelStrength");
    glassUniforms.specularStrength = glGetUniformLocation(program, "specularStrength");
    glassUniforms.glassOpacity = glGetUniformLocation(program, "glassOpacity");
    glassUniforms.edgeThickness = glGetUniformLocation(program, "edgeThickness");
    glassUniforms.uvPadding = glGetUniformLocation(program, "uvPadding");
    glassUniforms.tintColor = glGetUniformLocation(program, "tintColor");
    glassUniforms.tintAlpha = glGetUniformLocation(program, "tintAlpha");
    glassUniforms.maskTexture = glGetUniformLocation(program, "maskTex");
    glassUniforms.useMask = glGetUniformLocation(program, "useMask");
    glassUniforms.lensDistortion = glGetUniformLocation(program, "lensDistortion");
    glassUniforms.saturation = glGetUniformLocation(program, "saturation");
    glassUniforms.adaptiveDim = glGetUniformLocation(program, "adaptiveDim");
    glassUniforms.adaptiveBoost = glGetUniformLocation(program, "adaptiveBoost");

    return true;
}

bool CShaderManager::compileBlurShader() {
    if (!blurShader->createProgram(g_pHyprOpenGL->m_shaders->TEXVERTSRC, Shaders::GAUSSIAN_BLUR_FRAG, true, true)) {
        notifyError("failed to compile gaussian blur shader");
        return false;
    }

    const auto program = blurShader->program();
    blurUniforms.direction = glGetUniformLocation(program, "direction");
    blurUniforms.radius = glGetUniformLocation(program, "blurRadius");

    return true;
}

void CShaderManager::initializeIfNeeded() {
    if (m_initialized || m_failed)
        return;

    if (!g_pHyprOpenGL || !g_pHyprOpenGL->m_shaders) {
        notifyError("OpenGL renderer is not ready");
        m_failed = true;
        return;
    }

    m_initialized = compileGlassShader() && compileBlurShader();
    m_failed = !m_initialized;
}

void CShaderManager::destroy() noexcept {
    if (glassShader)
        glassShader->destroy();
    if (blurShader)
        blurShader->destroy();
    m_initialized = false;
}

} // namespace LiquidGlass
