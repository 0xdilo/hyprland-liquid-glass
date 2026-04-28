#pragma once

#include <GLES3/gl32.h>

#include <hyprland/src/render/Shader.hpp>

namespace LiquidGlass {

struct SGlassUniforms {
    GLint refractionStrength = -1;
    GLint chromaticAberration = -1;
    GLint fresnelStrength = -1;
    GLint specularStrength = -1;
    GLint glassOpacity = -1;
    GLint edgeThickness = -1;
    GLint uvPadding = -1;
    GLint tintColor = -1;
    GLint tintAlpha = -1;
    GLint maskTexture = -1;
    GLint useMask = -1;
    GLint lensDistortion = -1;
    GLint saturation = -1;
    GLint adaptiveDim = -1;
    GLint adaptiveBoost = -1;
};

struct SBlurUniforms {
    GLint direction = -1;
    GLint radius = -1;
};

class CShaderManager {
  public:
    void initializeIfNeeded();
    void destroy() noexcept;

    [[nodiscard]] bool initialized() const {
        return m_initialized;
    }

    SP<CShader> glassShader = makeShared<CShader>();
    SP<CShader> blurShader = makeShared<CShader>();
    SGlassUniforms glassUniforms;
    SBlurUniforms blurUniforms;

  private:
    bool m_initialized = false;
    bool m_failed = false;

    [[nodiscard]] bool compileGlassShader();
    [[nodiscard]] bool compileBlurShader();
};

} // namespace LiquidGlass
