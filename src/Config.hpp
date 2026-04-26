#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

#include <hyprland/src/plugins/PluginAPI.hpp>

namespace LiquidGlass {

inline HANDLE g_pluginHandle = nullptr;

inline constexpr const char* PLUGIN_NAME = "liquidglass";
inline constexpr const char* PLUGIN_DESCRIPTION = "Liquid Glass inspired per-window shader decoration";
inline constexpr const char* PLUGIN_AUTHOR = "lain";
inline constexpr const char* PLUGIN_VERSION = "0.1.0";

inline constexpr const char* CFG_ENABLED = "plugin:liquidglass:enabled";
inline constexpr const char* CFG_EXCLUDE_CLASSES = "plugin:liquidglass:exclude_classes";
inline constexpr const char* CFG_WINDOW_OPACITY = "plugin:liquidglass:window_opacity";
inline constexpr const char* CFG_BLUR_STRENGTH = "plugin:liquidglass:blur_strength";
inline constexpr const char* CFG_BLUR_ITERATIONS = "plugin:liquidglass:blur_iterations";
inline constexpr const char* CFG_REFRACTION_STRENGTH = "plugin:liquidglass:refraction_strength";
inline constexpr const char* CFG_CHROMATIC_ABERRATION = "plugin:liquidglass:chromatic_aberration";
inline constexpr const char* CFG_FRESNEL_STRENGTH = "plugin:liquidglass:fresnel_strength";
inline constexpr const char* CFG_SPECULAR_STRENGTH = "plugin:liquidglass:specular_strength";
inline constexpr const char* CFG_GLASS_OPACITY = "plugin:liquidglass:glass_opacity";
inline constexpr const char* CFG_EDGE_THICKNESS = "plugin:liquidglass:edge_thickness";
inline constexpr const char* CFG_TINT_COLOR = "plugin:liquidglass:tint_color";
inline constexpr const char* CFG_LENS_DISTORTION = "plugin:liquidglass:lens_distortion";
inline constexpr const char* CFG_BRIGHTNESS = "plugin:liquidglass:brightness";
inline constexpr const char* CFG_CONTRAST = "plugin:liquidglass:contrast";
inline constexpr const char* CFG_SATURATION = "plugin:liquidglass:saturation";
inline constexpr const char* CFG_VIBRANCY = "plugin:liquidglass:vibrancy";
inline constexpr const char* CFG_ADAPTIVE_DIM = "plugin:liquidglass:adaptive_dim";
inline constexpr const char* CFG_ADAPTIVE_BOOST = "plugin:liquidglass:adaptive_boost";

inline constexpr const char* DEFAULT_EXCLUDE_CLASSES = "";
inline constexpr float DEFAULT_WINDOW_OPACITY = 0.88f;
inline constexpr float DEFAULT_BLUR_STRENGTH = 0.28f;
inline constexpr Hyprlang::INT DEFAULT_BLUR_ITERATIONS = 2;
inline constexpr float DEFAULT_REFRACTION_STRENGTH = 1.50f;
inline constexpr float DEFAULT_CHROMATIC_ABERRATION = 1.25f;
inline constexpr float DEFAULT_FRESNEL_STRENGTH = 0.12f;
inline constexpr float DEFAULT_SPECULAR_STRENGTH = 0.18f;
inline constexpr float DEFAULT_GLASS_OPACITY = 0.66f;
inline constexpr float DEFAULT_EDGE_THICKNESS = 0.028f;
inline constexpr Hyprlang::INT DEFAULT_TINT_COLOR = 0xb8d8ff00;
inline constexpr float DEFAULT_LENS_DISTORTION = 1.25f;
inline constexpr float DEFAULT_BRIGHTNESS = 0.62f;
inline constexpr float DEFAULT_CONTRAST = 1.30f;
inline constexpr float DEFAULT_SATURATION = 1.22f;
inline constexpr float DEFAULT_VIBRANCY = 0.60f;
inline constexpr float DEFAULT_ADAPTIVE_DIM = 0.55f;
inline constexpr float DEFAULT_ADAPTIVE_BOOST = 0.00f;

inline Hyprlang::INT configInt(const char* name, Hyprlang::INT fallback) {
    auto* value = HyprlandAPI::getConfigValue(g_pluginHandle, name);
    if (!value)
        return fallback;

    return *static_cast<Hyprlang::INT*>(value->dataPtr());
}

inline float configFloat(const char* name, float fallback) {
    auto* value = HyprlandAPI::getConfigValue(g_pluginHandle, name);
    if (!value)
        return fallback;

    return static_cast<float>(*static_cast<Hyprlang::FLOAT*>(value->dataPtr()));
}

inline std::string configString(const char* name, std::string_view fallback) {
    auto* value = HyprlandAPI::getConfigValue(g_pluginHandle, name);
    if (!value)
        return std::string(fallback);

    const auto* raw = static_cast<Hyprlang::STRING>(value->dataPtr());
    return raw ? std::string(raw) : std::string(fallback);
}

inline bool enabled() {
    return configInt(CFG_ENABLED, 1) != 0;
}

inline float windowOpacity() {
    return std::clamp(configFloat(CFG_WINDOW_OPACITY, DEFAULT_WINDOW_OPACITY), 0.05f, 1.0f);
}

} // namespace LiquidGlass
