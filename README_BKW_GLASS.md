# Liquid glass and console settings

Open **Settings → BKW → Personalization**. Enable **Liquid glass**, choose **Transparency**
(0–100%) and **Blur** (0–4). Changes apply immediately. Blur 0 keeps the glass
surfaces without background filtering. Higher levels soften a wider area.

The material follows the BKW-CLOUD config card: a faint diagonal white wash,
a brighter upper rim and a subtle lower rim around the complete rounded edge.
Transparency controls the color tint independently of these reflections, with
a lighter tint at intermediate values. Fully covered child controls reuse the
parent's blurred backdrop instead of replacing its tint with the raw scene.
Backdrop coverage resets each frame and whenever the viewport changes; clipped
panels are reused only under the same clip, and rounded edges remain separate.

The UI color picker in Graphics uses RGB while glass is enabled. Its previous alpha is
preserved and used again after glass is disabled. Text and color swatches stay
sharp. Glass applies to the menu, both over the game and over menu backgrounds.
Only the inside of each rounded glass panel samples the blurred scene. The
background outside panels stays sharp, and scroll clipping is respected.
The blur uses the current frame on the GPU, with a downsample/upsample pyramid;
it does not capture screenshots to disk or blend old menu frames.
Vulkan and OpenGL 3.3+/OpenGL ES 3 support the blur. Older renderers keep the
translucent surfaces. The effect is off by default.

```text
bkw_ui_glass 1
bkw_ui_glass_transparency 65
bkw_ui_glass_blur 2
```

Every client config variable also has a `bkw_` console name. Existing names that
already start with `bkw_` stay unchanged. For the rest, prepend `bkw_` to the
complete original name, including its prefix:

| Original | BKW alias |
| --- | --- |
| `cl_showfps` | `bkw_cl_showfps` |
| `ui_color` | `bkw_ui_color` |
| `gfx_refresh_rate` | `bkw_gfx_refresh_rate` |
| `bc_motion_blur` | `bkw_bc_motion_blur` |

The same rule covers `tc_` settings. Original commands and binds continue to work.
Aliases use the original validation, change callbacks and permissions. `toggle`,
`+toggle` and `reset` accept either name. Config files save one canonical value,
so aliases do not create duplicate settings. Server-only settings are excluded.

Before release, check the graphics effect with MSAA on/off, resizing, opening
and closing the in-game menu, color editing, and a switch between Vulkan and
OpenGL. Console regression tests cover aliases, change callbacks, read-only
values, quoted strings, toggles and setting ranges.

Fast Build also runs the actual client under Xvfb/Mesa with OpenGL and Vulkan,
with MSAA off/on. `scripts/test_menu_glass.py` compares screenshots of a static
checkerboard background: blur must change the interior and preserve the outer
border. The `glass-rendering` artifact contains the images and client logs.
