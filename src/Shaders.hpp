#pragma once

namespace LiquidGlass::Shaders {

inline constexpr const char* LIQUID_GLASS_FRAG = R"GLSL(
#version 300 es
precision highp float;

in vec2 v_texcoord;

uniform sampler2D tex;
uniform vec2 fullSize;
uniform float radius;

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
    vec2 q = abs(point) - halfSize + cornerRadius;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - cornerRadius;
}

vec3 adjustColor(vec3 color) {
    float lum = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color = mix(vec3(lum), color, saturation);

    float brightMask = smoothstep(0.55, 0.90, lum);
    float darkMask = 1.0 - smoothstep(0.10, 0.45, lum);
    color = mix(color, color * (1.0 - adaptiveDim), brightMask * adaptiveDim);
    color = mix(color, color + (1.0 - color) * adaptiveBoost, darkMask * adaptiveBoost);

    color = (color - 0.5) * contrast + 0.5;
    color *= brightness;

    float vibMask = smoothstep(0.05, 0.85, max(max(color.r, color.g), color.b) - min(min(color.r, color.g), color.b));
    color = mix(vec3(dot(color, vec3(0.299, 0.587, 0.114))), color, 1.0 + vibrancy * vibMask);
    return clamp(color, 0.0, 1.0);
}

vec3 sampleGlass(vec2 uv, vec2 offset, float dispersion) {
    vec2 rUv = clamp(uv + offset * (1.0 + dispersion), 0.0, 1.0);
    vec2 gUv = clamp(uv + offset, 0.0, 1.0);
    vec2 bUv = clamp(uv + offset * (1.0 - dispersion), 0.0, 1.0);
    return vec3(texture(tex, rUv).r, texture(tex, gUv).g, texture(tex, bUv).b);
}

void main() {
    vec2 size = max(fullSize, vec2(1.0));
    float minDim = min(size.x, size.y);
    float edgePx = max(edgeThickness * minDim, 1.0);
    float cornerRadius = clamp(radius, 0.0, minDim * 0.5);

    vec2 pixel = v_texcoord * size;
    vec2 center = size * 0.5;
    vec2 halfSize = center;
    float dist = roundedBoxSdf(pixel - center, halfSize, cornerRadius);
    float shapeAlpha = 1.0 - smoothstep(-1.0, 1.0, dist);

    if (shapeAlpha <= 0.001)
        discard;

    float inside = clamp(-dist / edgePx, 0.0, 1.0);
    float edge = 1.0 - smoothstep(0.0, 1.0, inside);

    float dx = roundedBoxSdf(pixel + vec2(1.0, 0.0) - center, halfSize, cornerRadius) -
               roundedBoxSdf(pixel - vec2(1.0, 0.0) - center, halfSize, cornerRadius);
    float dy = roundedBoxSdf(pixel + vec2(0.0, 1.0) - center, halfSize, cornerRadius) -
               roundedBoxSdf(pixel - vec2(0.0, 1.0) - center, halfSize, cornerRadius);
    vec2 normal = normalize(vec2(dx, dy) + 0.0001);

    vec2 glassUv = mix(uvPadding, vec2(1.0) - uvPadding, v_texcoord);
    vec2 texel = 1.0 / size;

    float lens = (1.0 - edge) * lensDistortion * 0.018;
    vec2 centered = v_texcoord - 0.5;
    vec2 lensOffset = centered * lens;
    vec2 edgeOffset = -normal * edge * refractionStrength * 0.055;
    vec2 offset = (edgeOffset + lensOffset) * texel * minDim;

    float dispersion = chromaticAberration * edge * 0.55;
    vec3 color = sampleGlass(glassUv, offset, dispersion);
    color = adjustColor(color);
    color = mix(color, tintColor, tintAlpha);

    float fresnel = pow(edge, 2.6) * fresnelStrength;
    vec3 edgeGlow = mix(color, vec3(0.42, 0.58, 0.78), 0.36);
    color = mix(color, edgeGlow, clamp(fresnel * 0.28, 0.0, 0.32));

    float topLight = smoothstep(1.0, 0.15, v_texcoord.y) * smoothstep(0.0, 0.65, v_texcoord.x);
    float diagonal = smoothstep(0.9, 0.1, abs(v_texcoord.y - (0.18 + v_texcoord.x * 0.18)));
    color += specularStrength * diagonal * topLight * edge * 0.22 * vec3(0.68, 0.78, 0.90);

    float innerShadow = smoothstep(0.60, 1.0, v_texcoord.y) * edge * 0.22;
    color *= 1.0 - innerShadow;

    fragColor = vec4(clamp(color, 0.0, 1.0), glassOpacity * shapeAlpha);
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
