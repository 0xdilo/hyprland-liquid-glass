#pragma once

namespace LiquidGlass::Shaders {

inline constexpr const char* LIQUID_GLASS_FRAG = R"GLSL(
#version 300 es
precision highp float;

in vec2 v_texcoord;

uniform sampler2D tex;
uniform sampler2D maskTex;
uniform int useMask;
uniform vec2 fullSize;
uniform float radius;
uniform float roundingPower;

uniform float brightness;
uniform float contrast;
uniform float vibrancy;

uniform float refractionStrength;
uniform float chromaticAberration;
uniform float fresnelStrength;
uniform float specularStrength;
uniform float glassOpacity;
uniform float edgeThickness;
uniform vec2 uvPadding;
uniform vec3 tintColor;
uniform float tintAlpha;
uniform float lensDistortion;
uniform float saturation;
uniform float adaptiveDim;
uniform float adaptiveBoost;

layout(location = 0) out vec4 fragColor;

float roundedBoxSdf(vec2 point, vec2 halfSize, float cornerRadius) {
    vec2 q = abs(point) - halfSize + vec2(cornerRadius);
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - cornerRadius;
}

vec2 glassUvFromPixel(vec2 pixel, vec2 size) {
    vec2 normalized = clamp(pixel / max(size, vec2(1.0)), vec2(0.0), vec2(1.0));
    return mix(uvPadding, vec2(1.0) - uvPadding, normalized);
}

vec3 sampleGlassPixel(vec2 pixel, vec2 size) {
    return texture(tex, glassUvFromPixel(pixel, size)).rgb;
}

void main() {
    vec2 size = max(fullSize, vec2(1.0));
    float minDim = max(min(size.x, size.y), 1.0);
    float cornerRadius = clamp(radius, 0.0, minDim * 0.5);

    vec2 pixel = v_texcoord * size;
    vec2 center = size * 0.5;
    vec2 glassCoord = pixel - center;
    float sdf = roundedBoxSdf(glassCoord, size * 0.5, cornerRadius);
    float antialias = max(fwidth(sdf), 0.75);
    float shapeAlpha = 1.0 - smoothstep(-antialias, antialias, sdf);

    float layerMask = 1.0;
    if (useMask == 1)
        layerMask = smoothstep(0.18, 0.48, texture(maskTex, v_texcoord).a);

    if (shapeAlpha * layerMask <= 0.001)
        discard;

    float inverseSdf = max(-sdf / minDim, 0.0);
    float coordLength = length(glassCoord);
    vec2 radial = coordLength > 0.0001 ? glassCoord / coordLength : vec2(0.0);

    float distortionDepth = clamp(edgeThickness * 5.0, 0.02, 0.45);
    float distortionStrength = clamp(lensDistortion * refractionStrength * 0.1135, 0.0, 0.45);
    float distanceFromCenter = 1.0 - clamp(inverseSdf / distortionDepth, 0.0, 1.0);
    float distortion = 1.0 - sqrt(max(1.0 - distanceFromCenter * distanceFromCenter, 0.0));
    vec2 offset = distortion * radial * size * 0.5 * distortionStrength;
    vec2 samplePixel = pixel - offset;

    float edge = smoothstep(0.0, 0.02, inverseSdf);
    float chromaticShift = clamp(chromaticAberration * 3.3333333, 0.0, 12.0);
    vec2 shift = radial * edge * chromaticShift;
    vec3 color = vec3(
        sampleGlassPixel(samplePixel - shift, size).r,
        sampleGlassPixel(samplePixel, size).g,
        sampleGlassPixel(samplePixel + shift, size).b
    );

    float glassTint = clamp(brightness * 1.0227273, 0.0, 2.0);
    color *= vec3(glassTint);
    color = mix(color, color * tintColor, clamp(tintAlpha, 0.0, 1.0));

    float materialAlpha = clamp(glassOpacity / 0.78, 0.0, 1.0);
    fragColor = vec4(clamp(color, 0.0, 1.0), materialAlpha * shapeAlpha * layerMask);
}
)GLSL";

inline constexpr const char* GAUSSIAN_BLUR_FRAG = R"GLSL(
#version 300 es
precision highp float;

in vec2 v_texcoord;

uniform sampler2D tex;
uniform vec2 direction;
uniform float blurRadius;

layout(location = 0) out vec4 fragColor;

void main() {
    float radius = max(blurRadius, 0.001);
    float sigma = max(radius / 3.0, 0.001);
    int samples = min(int(ceil(radius)), 8);

    vec4 color = texture(tex, v_texcoord);
    float total = 1.0;

    for (int i = 1; i <= 8; ++i) {
        if (i > samples)
            break;

        float x = float(i);
        float weight = exp(-(x * x) / (2.0 * sigma * sigma));
        vec2 delta = direction * x;

        color += texture(tex, v_texcoord + delta) * weight;
        color += texture(tex, v_texcoord - delta) * weight;
        total += 2.0 * weight;
    }

    fragColor = color / total;
}
)GLSL";

} // namespace LiquidGlass::Shaders
