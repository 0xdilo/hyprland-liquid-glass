# Hyprland Liquid Glass

LiquidGlass is a Hyprland plugin that renders a per-window liquid glass material. It samples the framebuffer behind each window, applies a small Gaussian frost blur, then adds refraction, chromatic separation, lens distortion, and a restrained edge highlight.

The effect is rendered as a window decoration under the client surface. The plugin also lowers managed window alpha slightly so normal opaque apps can show the material without requiring app-specific transparency.

## Requirements

- Hyprland with plugin support
- Hyprland headers matching the running compositor
- `pkg-config`
- C++23 compiler
- `make`

Hyprland plugins are ABI-sensitive. Rebuild this plugin whenever Hyprland updates.

## Install With Hyprpm

```bash
hyprpm add https://github.com/0xdilo/hyprland-liquid-glass
hyprpm enable liquidglass
hyprpm reload
```

## Manual Build

```bash
make
hyprctl plugin load "$PWD/liquidglass.so"
```

For a persistent manual load, add the built plugin path to `hyprland.conf`:

```ini
plugin = /absolute/path/to/liquidglass.so
```

## Config

All options live under `plugin:liquidglass:` in `hyprland.conf`.

```ini
plugin:liquidglass {
    enabled = 1

    # Optional. No applications are excluded by default.
    # Use this for media players, browsers, or apps that should stay fully opaque.
    exclude_classes = mpv,helium

    window_opacity = 0.88
    glass_opacity = 0.66

    blur_strength = 0.28
    blur_iterations = 2

    refraction_strength = 1.50
    chromatic_aberration = 1.25
    lens_distortion = 1.25

    fresnel_strength = 0.12
    specular_strength = 0.18
    edge_thickness = 0.028

    # RRGGBBAA
    tint_color = 0xb8d8ff00

    brightness = 0.62
    contrast = 1.30
    saturation = 1.22
    vibrancy = 0.60
    adaptive_dim = 0.55
    adaptive_boost = 0.00
}
```

### Options

| Option | Default | Notes |
| --- | ---: | --- |
| `enabled` | `1` | Enables or disables the plugin effect. |
| `exclude_classes` | empty | Comma-separated window classes to leave untouched. Matching is case-insensitive and checks current and initial class. |
| `window_opacity` | `0.88` | Alpha applied to managed window surfaces so the glass backing can show through. Set to `1.0` to avoid forcing opacity. |
| `glass_opacity` | `0.66` | Strength of the rendered glass backing. |
| `blur_strength` | `0.28` | Radius multiplier for the sampled background blur. |
| `blur_iterations` | `2` | Number of horizontal/vertical blur passes. Higher values cost more GPU time. |
| `refraction_strength` | `1.50` | Edge refraction amount. |
| `chromatic_aberration` | `1.25` | Color channel separation near refractive edges. |
| `lens_distortion` | `1.25` | Center lens distortion amount. |
| `fresnel_strength` | `0.12` | Edge glow strength. |
| `specular_strength` | `0.18` | Diagonal highlight strength. |
| `edge_thickness` | `0.028` | Width of the refractive edge band relative to the window size. |
| `tint_color` | `0xb8d8ff00` | Glass tint as `RRGGBBAA`. Alpha `00` disables tint. |
| `brightness` | `0.62` | Brightness applied to sampled glass. |
| `contrast` | `1.30` | Contrast applied to sampled glass. |
| `saturation` | `1.22` | Saturation applied to sampled glass. |
| `vibrancy` | `0.60` | Extra saturation for already colorful sampled content. |
| `adaptive_dim` | `0.55` | Dims bright sampled content to keep text readable. |
| `adaptive_boost` | `0.00` | Brightens dark sampled content. |

## Performance Notes

LiquidGlass renders only around windows that use the material. It samples a padded region behind each affected window, blurs that sample in an offscreen framebuffer, and composites the final glass pass back into the current render pass.

For lower GPU cost, reduce `blur_iterations`, `blur_strength`, or `glass_opacity`. For a subtler effect on opaque apps, increase `window_opacity`.

## Development

```bash
make clean
make
hyprctl plugin unload "$PWD/liquidglass.so"
hyprctl plugin load "$PWD/liquidglass.so"
```

Check Hyprland config errors after changing options:

```bash
hyprctl configerrors
```

## License

MIT
