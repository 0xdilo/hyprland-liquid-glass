#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <hyprland/src/helpers/signal/Signal.hpp>
#include <hyprland/src/render/Framebuffer.hpp>
#include <hyprland/src/plugins/HookSystem.hpp>

#include "ShaderManager.hpp"

class CGlassDecoration;

namespace LiquidGlass {

struct SGlobalState {
    CShaderManager shaderManager;
    CFramebuffer blurTempFramebuffer;
    std::vector<WP<CGlassDecoration>> decorations;
    std::vector<CHyprSignalListener> listeners;
    CFunctionHook* renderPassHook = nullptr;
    CFunctionHook* renderWindowHook = nullptr;
    CFunctionHook* windowOpaqueHook = nullptr;
};

inline UP<SGlobalState> g_state;

void notifyError(std::string_view message);

} // namespace LiquidGlass
