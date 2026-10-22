---
title: "BK — UI Layer"
status: current
area: "specs/BK"
updated: 2026-08-08
tags:
  - banjo-recomp
  - ui
  - launcher
---

# BK — UI Layer (Historical Reference Specification)

> [!WARNING]
> **Reference Specification Only:** This document describes the upstream Banjo-Kazooie (`BanjoRecomp`) UI contracts for architectural and design reference. **In TooieRecomp, no external BanjoRecomp checkout is used.** All UI assets, fonts, icons, stylesheets, and launcher animations are vendored locally in `assets/` and `ui_assets/` (managed by CMake target `TooieUIAssets`). For the active TooieRecomp UI specification, see [`docs/specs/Tooie/window-ui-and-config.md`](../Tooie/window-ui-and-config.md).

User-facing UI: the launcher menu, the animated intro, the in-game config
tabs, theming and fonts. All built on `RecompFrontend`'s **recompui**
framework (`lib/RecompFrontend/recompui`, linked from `CMakeLists.txt`), the
same UI framework used by `rt64`-based ports.

## RecompUI framework

- `recompui/recompui.h`, `recompui/config.h`,
  `recompui/renderer.h`, `elements/ui_theme.h`.
- `recompui::register_ui_exports()` in `main` exports UI callbacks into the
  recompiled world (menu on top of the 3D viewport).
- `recompui::renderer` drives RT64 rendering, window sizing, and texture
  packs (`recompui/renderer.h`).

## Launcher menu (`main.cpp` `on_launcher_init`)

- `recompui::LauncherMenu` is the shell around the options list.
- `init_game_options_menu(game_id, mod_game_id, display_name, thumbnail,
  GameOptionsMenuLayout::Center)` builds the play/mod launcher UI; BK uses
  `supported_games[0]` (the single BK entry).
- `add_default_options()` + 30% width, right-aligned (FlexEnd), flat border
  radius (0), transparent hover/focus styles.
- Menu container: 1440dp wide, centered via
  `set_translate_2D(50%, 0)` / `set_right(50%)`; options column pinned bottom
  center. `remove_default_title()` drops the stock heading.
- `register_launcher_init_callback`/`register_launcher_update_callback` are
  wired to `on_launcher_init` and `launcher_animation_update`.

## Launcher animation (`launcher_animation.cpp`)

A scene graph of animated SVGs driven by keyframe curves — no video asset,
pure transform animation:

- **`AnimatedSvg`** = an `recompui::Svg` element + position/scale/rotation
  keyframe vectors, each with `AnimationData` (keyframe index, loop point,
  seconds, `InterpolationMethod`).
- **Interpolation**: `Linear` or `Smootherstep`
  (`t*t*t*(t*(6t-15)+10)`), applied in `interpolate_value`.
- `create_animated_svg` wraps `context.create_element<Svg>` at an absolute
  position; `update_animated_svg` re-derives translate/scale/rotation every
  frame.
- **Scene**: `JiggyColor.svg`, `JiggyShine.svg`, `JiggyHole.svg`,
  `Banjo.svg`, `Kazooie.svg` (in a wrapper), 4 drifting `CloudN.svg`, and a
  `Logo.svg`; the wrapper is a container that later slides to make room for
  the options.
- Timing constants drive the choreography: jiggy scales up (0→0.75s),
  `jiggy_move_over` slide (≈1.25s→2s), shine sweep (≈2.6s→3.4s), then the
  wrapper translates/transitions to reveal the menu. Looping done via
  `loop_keyframe_index` (e.g. clouds, hole bob).
- **Skip**: `SDL_AddEventWatch` (`launcher_event_watch`) flags any key/mouse/
  controller input → `skip_animation_next_update` jumps `delta_time` to
  `animation_skip_time` (10s), so the animation fast-forwards to its end.
- Menu options stay `set_enabled(false)` + transparent until the wrapper
  animation completes, then fade in.

## Config tabs (`src/game/config.cpp`, `banjo::init_config`)

RecompUI builds the in-game settings menu programmatically:

- Config dir = `recompui::file::get_app_folder_path()` (created if missing);
  registered as the recomp config path in `main`.
- `GeneralTabOptions`: `has_rumble_strength=true`,
  `has_gyro_sensitivity=false`, `has_mouse_sensitivity=false` (gyro/mouse
  features are present in the runtime but disabled in BK).
- Tabs: `create_general_tab` → `create_graphics_tab` →
  `create_controls_tab` → `create_sound_tab` → `create_mods_tab` →
  `finalize()`.
- Options carry a `<recomp-color primary>` HTML-ish rich-text description;
  enum options are `{id, key, label}` triples with a default, number options
  `(min, max, step, ...)`; `add_option_hidden_dependency` makes the analog
  camera sensitivity option hidden while the analog camera is off.
- See [Input](input.md) for the control mappings and [Runtime](runtime.md)
  for how `banjo::get_*` getters read the values back.

## Theme (`src/main/theme.cpp`)

- `recomptheme::set_custom_theme()` fills the full `recompui::theme::color`
  palette — dark modal background (`Background1..3`, `ModalOverlay`), text
  (full alpha ladder `TextA5..TextA80`), semantic colors `Primary`,
  `Secondary`, `Warning`, `Danger`, `Success` (+`L`/`D`/`A*` variants),
  `Border*`, greys `BW05..BW90`.
- Border radii: `radius_sm/md/lg` = 12/18/24.
- Typography presets per `theme::Typography` (`Header1..3`, `LabelLG..XS`,
  `Body`) with size/weight/letter-spacing.

## Fonts & assets

- `register_primary_font("InterVariable.ttf", "Inter Variable")` +
  `register_extra_font("Suplexmentary Comic NC.ttf")` (used for launcher
  options).
- Bundled SVGs and fonts lived in upstream `BanjoRecomp/assets/` (in TooieRecomp, these are vendored locally in `assets/`); `promptfont.h`
  embeds the prompt bitmap font for in-game text. Launcher icons include a
  random-seeded easter egg (`std::srand`).

## Mod / texture pack UI hooks

- `.rtz` files are registered as a mod container whose only content type is
  the RT64 texture pack (`rt64.json`), togglable at runtime
  (`recomp::mods::register_mod_content_type`, `enable_texture_pack` /
  `disable_texture_pack` / `reorder_texture_pack` in `main.cpp`).

See [Runtime](runtime.md), [Input](input.md), [Mods](mods.md).
