# Research Notes

- Apple describes Liquid Glass as a dynamic material that reflects and refracts surroundings, bends and shapes light, and uses blur, highlights, shadows, and fluid response for separation.
- Hyprland's built-in `decoration:screen_shader` runs at the end of rendering, so it cannot target a single window by itself.
- Hyprland's plugin API supports window decorations and render pass elements. This plugin uses a bottom-layer decoration per window so the shader can sample the actual framebuffer behind each window.

Sources:

- https://developer.apple.com/videos/play/wwdc2025/219/
- https://developer.apple.com/documentation/TechnologyOverviews/adopting-liquid-glass
- https://wiki.hypr.land/Configuring/Variables/
- https://wiki.hypr.land/Plugins/Development/Getting-Started/
