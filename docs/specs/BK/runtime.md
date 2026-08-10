---
title: "BK — Runtime"
status: current
area: "specs/BK"
updated: 2026-08-08
tags:
  - banjo-recomp
  - runtime
---

# BK — Runtime

How `BanjoRecomp` boots, registers recompiled code, and wires overlays,
patches and game features through `N64ModernRuntime` / `RecompFrontend`.

## `src/main`

Entry point of the native port.

- `main.cpp` — SDL window/setup, launcher, ROM picking (dialog), starts the
  recompiled game thread.
- `launcher_animation.cpp` — animated background for the main menu.
- `theme.cpp`, `theme.h` — theming of the launcher UI.
- `register_overlays.cpp` — declares the game's overlay table (from
  `RecompiledFuncs/recomp_overlays.inl`).
- `register_patches.cpp` — registers the patch ELF binary
  (`RecompiledPatches/recomp_overlays.inl`, `bk_patches_bin`).

## `src/game`

Logical game-side code that runs inside the recompiled game world.

- `config.cpp` — runtime settings (resolution, framerate, audio, input).
- `rom_decompression.cpp` — reads assets out of the user-provided ROM at
  runtime.
- `recomp_api.cpp` — exposes host functions to recompiled game code.
- `recomp_data_api.cpp` — provides the save/memory-mapping APIs
  (uses `slot_map.h`, `recomp_data.h`).
- `recomp_extension_api.cpp` — modding/extension API to register object
  extensions etc.

## Overlay / patch registration

```cpp
void banjo::register_bk_overlays() {
    // recompiled game funcs
    recomp::overlays::overlay_section_table_data_t sections { ... };
    recomp::overlays::overlays_by_index_t overlays { ... };
    recomp::overlays::register_overlays(sections, overlays);
}

void banjo::register_bk_patches() {
    extern const char bk_patches_bin[];
    recomp::overlays::register_patches(bk_patches_bin, bk_patches_bin_size, ...);
    recomp::overlays::register_base_exports(export_table);
    recomp::overlays::register_base_events(event_names);
    recomp::overlays::register_manual_patch_symbols(manual_patch_symbols);
}
```

## Include headers exposed to game logic

- `banjo_config.h`, `banjo_debug.h`, `banjo_game.h`, `banjo_launcher.h`,
  `banjo_sound.h`, `banjo_support.h`, `promptfont.h`, `recomp_files.h`,
  `recomp_data.h`.

## Boot & ROM handling (`rom_decompression.cpp`)

BK is shipped with the **compressed** cartridge and inflated to RDRAM at
boot, only in the build `banjo.us.v10.decompressed.z64`. `decompress_bk`
produces the decompressed game image the recompiler needs:

- 16 fixed overlays (text+data pairs) are inflated with `miniz`
  (`decompress_bkzip` -> `mz_inflate`, raw deflate), sizes from a 4-byte
  header; overlay order 3/4 is swapped to match the ref layout.
- `bk_on_init` pokes `MEM_W(0, 0x80000310) = 6103` and does the same
  `recomp::do_rom_read` the game expects at `0x80000000`.

For a TooieRecomp this layer is replaced by the `N64ModernRuntime` boot — the
game surface is identical.

## Host functions (`recomp_api.cpp`)

Host API exported back into recompiled code (`register_base_export`):

- **input/context**: `recomp_update_inputs`, `recomp_get_gyro_deltas`,
  `recomp_get_mouse_deltas`, `recomp_set_right_analog_suppressed`.
- **OS/config**: `recomp_puts`, `recomp_exit`, `recomp_error`,
  `recomp_abort`, `recomp_time_us`.
- **options getter**: `recomp_get_target_framerate`,
  `recomp_get_window_resolution`, `recomp_get_target_aspect_ratio`,
  `recomp_get_cutscene_aspect_ratio`, `recomp_get_bgm_volume`,
  `recomp_get_analog_cam_sensitivity`, `recomp_get_resolution_scale`,
  `recomp_high_precision_fb_enabled`.
- **input query**: `recomp_get_inverted_axes`,
  `recomp_get_analog_inverted_axes`, `recomp_get_flying_and_swimming_inverted_axes`,
  `recomp_get_first_person_inverted_axes`, `recomp_get_analog_cam_enabled`,
  `recomp_get_note_saving_enabled`, `recomp_get_jinjo_saving_enabled`,
  `recomp_get_right_analog_inputs`.
- **overlays**: `recomp_load_overlays`, `recomp_load_overlays_by_rom`.
- **hash**: `recomp_xxh3`.
- **device emulation**: `osPfsInit_recomp` (returns `PFS_ERR_DEVICE`).

## Audio

- `banjo_sound.h` + `sound_patches.c` in patches.
- Audio callbacks (`ultramodern::audio_callbacks_t` in `main.cpp`):
  - `queue_samples(int16_t*, count)` — the game's audio output. Endianness
    xor is undone by swapping the two channels while converting s16→f32;
    main volume from `recompui::config::sound::get_main_volume()` is applied at
    0.5/32768 scale. Four duplicated frames at chunk boundaries are kept so
    resampling interpolates across endpoints (`duplicated_input_frames=4`),
    dropped again on playback (`discarded_output_frames`). `SDL_ConvertAudio`
    resamples to the output device rate; if the queue grows past ~100ms, queued
    samples are *skipped* in powers of two to stop latency buildup
    (`skip_factor`, `skip_ratio`). `SDL_QueueAudio` pushes the result.
  - `get_frames_remaining()` — converts queued bytes to N64 sample frames at
    60 VI intervals and reports "some frames into the future" so the game
    never underflows and pops.
  - `set_frequency(freq)` — game's effective sample rate; rebuilds the
    `SDL_AudioCVT` converter.
- `reset_audio(48000)` on startup opens an `AUDIO_F32` device with a small
  buffer (0x100), `SDL_AudioCVT`-based resampling (via
  `SDL_BuildAudioCVT`); Windows forces the WASAPI driver.
- RSP: `get_rsp_microcode` returns `n_aspMain` (the recompiled ASP microcode
  from [RSP](rsp.md)) for `M_AUDTASK` only.

## Renderer / RT64

- `ultramodern::renderer::callbacks_t.create_render_context` builds a
  `recompui::renderer` context with `PresentationMode::PresentEarly`.
- `ultramodern::gfx_callbacks_t`: `create_gfx` (SDL video/gamepad init, SDL
  hints), `create_window` (window handle per platform), `update_gfx`
  (calls `recompinput::handle_events` once a frame).
- Rendering, texture packs, window size, and RT64 `rt64.json` content type
  handled by `recompui::renderer` (see [UI](ui.md)).
- Graphics/Camera/Renderback:
  - `rt64` is the rendering engine (D3D12/Vulkan/Metal), wired through the
    `rt64` submodule; presentation mode `PresentEarly`.
- Threads: `recomp::threads::callbacks_t` provides
  `get_game_thread_name` (maps OSThread ids to names: INIT, DEFRAG, AUDIO,
  RESET, MAIN, CONT, RUMBLE, plus priority 150→`PIMGR`, 80→`VIMGR`).

## Notable runtime files

- `overloaded.h`, `ovl_patches.hpp` — helpers.
- `recomp_data_api.cpp` uses `LockedMap` to track per-object extension data
  safe across threads.

## Tooie relation

A `TooieRecomp` copies this layout almost exactly: same
`main.cpp`/`launcher`/`game` split, same `register_overlays`/`patches`
entry points — only the `.inl` recompiled blob and symbol sources differ.

See [Patches](patches.md), [Build](build.md),
[TooieRecomp roadmap](../../roadmap/tooie-recomp.md).