#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

#include <hyprland/src/config/values/types/FloatValue.hpp>
#include <hyprland/src/config/values/types/IntValue.hpp>
#include <hyprland/src/config/values/types/StringValue.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

namespace LiquidGlass {

inline HANDLE g_pluginHandle = nullptr;

inline constexpr const char* PLUGIN_NAME = "liquidglass";
inline constexpr const char* PLUGIN_DESCRIPTION = "Liquid Glass inspired per-window shader decoration";
inline constexpr const char* PLUGIN_AUTHOR = "lain";
inline constexpr const char* PLUGIN_VERSION = "0.3.0";

inline constexpr const char* CFG_ENABLED = "plugin:liquidglass:enabled";
inline constexpr const char* CFG_EXCLUDE_CLASSES = "plugin:liquidglass:exclude_classes";
inline constexpr const char* CFG_LAYER_NAMESPACES = "plugin:liquidglass:layer_namespaces";
inline constexpr const char* CFG_WINDOW_OPACITY = "plugin:liquidglass:window_opacity";
inline constexpr const char* CFG_LAYER_OPACITY = "plugin:liquidglass:layer_opacity";
inline constexpr const char* CFG_LAYER_CORNER_RADIUS = "plugin:liquidglass:layer_corner_radius";
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
inline constexpr const char* DEFAULT_LAYER_NAMESPACES = "quickshell";
inline constexpr float DEFAULT_WINDOW_OPACITY = 0.90f;
inline constexpr float DEFAULT_LAYER_OPACITY = 1.0f;
inline constexpr float DEFAULT_LAYER_CORNER_RADIUS = 12.0f;
inline constexpr float DEFAULT_BLUR_STRENGTH = 0.32f;
inline constexpr Hyprlang::INT DEFAULT_BLUR_ITERATIONS = 2;
inline constexpr float DEFAULT_REFRACTION_STRENGTH = 1.15f;
inline constexpr float DEFAULT_CHROMATIC_ABERRATION = 0.90f;
inline constexpr float DEFAULT_FRESNEL_STRENGTH = 0.46f;
inline constexpr float DEFAULT_SPECULAR_STRENGTH = 0.38f;
inline constexpr float DEFAULT_GLASS_OPACITY = 0.78f;
inline constexpr float DEFAULT_EDGE_THICKNESS = 0.040f;
inline constexpr Hyprlang::INT DEFAULT_TINT_COLOR = 0xb8d8ff00;
inline constexpr float DEFAULT_LENS_DISTORTION = 1.15f;
inline constexpr float DEFAULT_BRIGHTNESS = 0.88f;
inline constexpr float DEFAULT_CONTRAST = 1.16f;
inline constexpr float DEFAULT_SATURATION = 1.14f;
inline constexpr float DEFAULT_VIBRANCY = 0.32f;
inline constexpr float DEFAULT_ADAPTIVE_DIM = 0.32f;
inline constexpr float DEFAULT_ADAPTIVE_BOOST = 0.10f;

inline std::unordered_map<std::string, SP<Config::Values::CIntValue>> g_intConfigValues;
inline std::unordered_map<std::string, SP<Config::Values::CFloatValue>> g_floatConfigValues;
inline std::unordered_map<std::string, SP<Config::Values::CStringValue>> g_stringConfigValues;

inline bool addIntConfig(const char* name, Hyprlang::INT fallback) {
    auto value = makeShared<Config::Values::CIntValue>(name, name, static_cast<Config::INTEGER>(fallback));
    if (!HyprlandAPI::addConfigValueV2(g_pluginHandle, value))
        return false;

    g_intConfigValues[name] = value;
    return true;
}

inline bool addFloatConfig(const char* name, float fallback) {
    auto value = makeShared<Config::Values::CFloatValue>(name, name, static_cast<Config::FLOAT>(fallback));
    if (!HyprlandAPI::addConfigValueV2(g_pluginHandle, value))
        return false;

    g_floatConfigValues[name] = value;
    return true;
}

inline bool addStringConfig(const char* name, std::string_view fallback) {
    auto value = makeShared<Config::Values::CStringValue>(name, name, Config::STRING(fallback));
    if (!HyprlandAPI::addConfigValueV2(g_pluginHandle, value))
        return false;

    g_stringConfigValues[name] = value;
    return true;
}

inline void clearConfigValues() {
    g_intConfigValues.clear();
    g_floatConfigValues.clear();
    g_stringConfigValues.clear();
}

inline Hyprlang::INT configInt(const char* name, Hyprlang::INT fallback) {
    const auto found = g_intConfigValues.find(name);
    if (found == g_intConfigValues.end() || !found->second)
        return fallback;

    return static_cast<Hyprlang::INT>(found->second->value());
}

inline float configFloat(const char* name, float fallback) {
    const auto found = g_floatConfigValues.find(name);
    if (found == g_floatConfigValues.end() || !found->second)
        return fallback;

    return static_cast<float>(found->second->value());
}

inline std::string configString(const char* name, std::string_view fallback) {
    const auto found = g_stringConfigValues.find(name);
    if (found == g_stringConfigValues.end() || !found->second)
        return std::string(fallback);

    return found->second->value();
}

inline bool enabled() {
    return configInt(CFG_ENABLED, 1) != 0;
}

inline float windowOpacity() {
    return std::clamp(configFloat(CFG_WINDOW_OPACITY, DEFAULT_WINDOW_OPACITY), 0.05f, 1.0f);
}

inline float layerOpacity() {
    return std::clamp(configFloat(CFG_LAYER_OPACITY, DEFAULT_LAYER_OPACITY), 0.05f, 1.0f);
}

inline float layerCornerRadius() {
    return std::clamp(configFloat(CFG_LAYER_CORNER_RADIUS, DEFAULT_LAYER_CORNER_RADIUS), 0.0f, 128.0f);
}

} // namespace LiquidGlass
