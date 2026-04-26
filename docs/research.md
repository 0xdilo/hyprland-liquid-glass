# Research Notes

- Apple describes Liquid Glass as a dynamic material that reflects and refracts surroundings, bends and shapes light, and uses blur, highlights, shadows, and fluid response for separation.
- Apple's WWDC25 design session frames the core visual cue as lensing: the material bends, concentrates, and shapes light in real time rather than only scattering it like older blur materials.
- Apple describes the material as adaptive. Its tint, shadow, dynamic range, and contrast shift with the content behind it, with stronger lensing and deeper shadows for larger or thicker glass forms.
- Apple's guidance treats Liquid Glass as a functional navigation/control layer, not a general content-layer texture. Glass should float above content, preserve legibility, and avoid stacking glass on glass.
- The shuding/liquid-glass reference implements a similar lensing idea with an SVG displacement map generated from a rounded-rect SDF. It confirms that strong interior displacement, not only edge blur, is a major part of the perceived effect.
- Hyprland's built-in `decoration:screen_shader` runs at the end of rendering, so it cannot target a single window by itself.
- Hyprland's plugin API supports window decorations and render pass elements. This plugin uses a bottom-layer decoration per window so the shader can sample the actual framebuffer behind each window.
- For future shader work, the safest next step is to add stronger SDF-based body displacement inside the existing window decoration pass. Avoid layer-surface hooks until they have a minimal test harness, because malformed pass injection can crash Hyprland.

Sources:

- https://developer.apple.com/videos/play/wwdc2025/219/
- https://developer.apple.com/videos/play/wwdc2025/356/
- https://developer.apple.com/videos/play/wwdc2025/310/
- https://developer.apple.com/videos/play/wwdc2025/323/
- https://developer.apple.com/documentation/TechnologyOverviews/adopting-liquid-glass
- https://github.com/shuding/liquid-glass
- https://wiki.hypr.land/Configuring/Variables/
- https://wiki.hypr.land/Plugins/Development/Getting-Started/
