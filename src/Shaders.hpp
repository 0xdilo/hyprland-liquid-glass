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

float superLength(vec2 value, float power) {
    value = abs(value);
    return pow(pow(value.x, power) + pow(value.y, power), 1.0 / power);
}

float roundedBoxSdf(vec2 point, vec2 halfSize, float cornerRadius, float power) {
    vec2 q = abs(point) - halfSize + cornerRadius;
    return superLength(max(q, 0.0), power) + min(max(q.x, q.y), 0.0) - cornerRadius;
}

float hash21(vec2 point) {
    return fract(sin(dot(point, vec2(127.1, 311.7))) * 43758.5453123);
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
    float cornerPower = clamp(roundingPower, 1.0, 10.0);

    vec2 pixel = v_texcoord * size;
    vec2 center = size * 0.5;
    vec2 halfSize = center;
    float dist = roundedBoxSdf(pixel - center, halfSize, cornerRadius, cornerPower);
    float shapeAlpha = 1.0 - smoothstep(-1.35, 1.35, dist);

    float layerMask = 1.0;
    if (useMask == 1)
        layerMask = smoothstep(0.18, 0.48, texture(maskTex, v_texcoord).a);

    if (shapeAlpha * layerMask <= 0.001)
        discard;

    float inside = clamp(-dist / edgePx, 0.0, 1.0);
    float edge = 1.0 - smoothstep(0.0, 1.0, inside);

    float dx = roundedBoxSdf(pixel + vec2(1.0, 0.0) - center, halfSize, cornerRadius, cornerPower) -
               roundedBoxSdf(pixel - vec2(1.0, 0.0) - center, halfSize, cornerRadius, cornerPower);
    float dy = roundedBoxSdf(pixel + vec2(0.0, 1.0) - center, halfSize, cornerRadius, cornerPower) -
               roundedBoxSdf(pixel - vec2(0.0, 1.0) - center, halfSize, cornerRadius, cornerPower);
    vec2 normal = normalize(vec2(dx, dy) + 0.0001);

    vec2 glassUv = mix(uvPadding, vec2(1.0) - uvPadding, v_texcoord);
    vec2 texel = 1.0 / size;

    vec2 centered = v_texcoord - 0.5;
    float depth = clamp(-dist / max(minDim * 0.45, 1.0), 0.0, 1.0);
    float bodyLens = smoothstep(0.015, 0.76, depth) * (1.0 - edge * 0.18);
    float shoulderLens = smoothstep(0.0, 1.0, inside) * (1.0 - smoothstep(0.52, 1.0, inside));
    float clearCore = smoothstep(0.18, 0.82, inside);

    vec2 bodyOffset = -centered * bodyLens * lensDistortion * 0.044;
    vec2 shoulderOffset = -normal * shoulderLens * lensDistortion * 0.030 * texel * minDim;
    vec2 edgeOffset = -normal * edge * refractionStrength * 0.058 * texel * minDim;
    vec2 offset = edgeOffset + shoulderOffset + bodyOffset;

    float dispersion = chromaticAberration * (edge * 0.28 + shoulderLens * 0.07);
    vec3 color = sampleGlass(glassUv, offset, dispersion);
    vec3 baseColor = texture(tex, glassUv).rgb;
    float baseLum = dot(baseColor, vec3(0.2126, 0.7152, 0.0722));
    color = adjustColor(color);

    float brightBackdrop = smoothstep(0.62, 0.96, baseLum);
    float darkBackdrop = 1.0 - smoothstep(0.08, 0.38, baseLum);
    vec3 adaptiveTint = mix(tintColor, mix(vec3(0.88, 0.94, 1.0), vec3(0.30, 0.42, 0.56), brightBackdrop), 0.22);
    float adaptiveTintAlpha = tintAlpha + 0.035 * clearCore + 0.055 * darkBackdrop + 0.045 * brightBackdrop;
    color = mix(color, adaptiveTint, clamp(adaptiveTintAlpha, 0.0, 0.22));

    float fresnel = pow(edge, 2.2) * fresnelStrength;
    float upperRim = edge * (1.0 - smoothstep(-0.18, 0.72, normal.y));
    float lowerRim = edge * smoothstep(0.25, 0.95, normal.y);
    vec3 edgeGlow = mix(color, vec3(0.74, 0.88, 1.0), 0.44);
    color = mix(color, edgeGlow, clamp(fresnel * (0.16 + upperRim * 0.22), 0.0, 0.42));

    float topLight = (1.0 - smoothstep(0.15, 1.0, v_texcoord.y)) * smoothstep(0.0, 0.65, v_texcoord.x);
    float diagonal = 1.0 - smoothstep(0.03, 0.35, abs(v_texcoord.y - (0.16 + v_texcoord.x * 0.16)));
    float caustic = smoothstep(0.10, 0.84, bodyLens) * (1.0 - smoothstep(0.82, 1.0, bodyLens));
    float innerSheen = diagonal * topLight * (edge * 0.36 + shoulderLens * 0.22 + caustic * 0.18);
    color += specularStrength * innerSheen * vec3(0.58, 0.72, 0.90);
    color += specularStrength * upperRim * vec3(0.34, 0.48, 0.64);

    float lowerShadow = smoothstep(0.54, 1.0, v_texcoord.y) * (lowerRim * 0.30 + shoulderLens * 0.08);
    float rimShadow = shoulderLens * fresnelStrength * (0.035 + brightBackdrop * 0.045);
    color *= 1.0 - lowerShadow - rimShadow;

    float grain = (hash21(floor(pixel)) - 0.5) * 0.018 * (0.4 + brightBackdrop * 0.6);
    color += grain * clearCore;

    float bodyAlpha = mix(0.34, 0.46, brightBackdrop) * clearCore;
    float edgeAlpha = edge * (0.30 + fresnelStrength * 0.14);
    float shoulderAlpha = shoulderLens * 0.18;
    float causticAlpha = caustic * 0.10;
    float materialAlpha = clamp(bodyAlpha + edgeAlpha + shoulderAlpha + causticAlpha, 0.0, 0.78);
    fragColor = vec4(clamp(color, 0.0, 1.0), glassOpacity * shapeAlpha * layerMask * materialAlpha);
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
