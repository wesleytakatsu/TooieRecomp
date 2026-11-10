---
title: "TooieRecomp Spec — Testing Framework and Measurement Oracle"
description: "Technical specification for the host unit test harness, regression test suite, and the ANGRYLION_CONTROL software-rendered ground truth oracle."
status: current
area: "specs/Tooie"
updated: 2026-11-09
tags: [specs, tooierecomp, tests, oracle, angrylion, regression]
---

# Testing Framework and Measurement Oracle Specification

## 1. Testing Framework Architecture

TooieRecomp enforces a strict verification hierarchy designed to validate guest behavior at the narrowest useful level first (unit tests), then integration/runtime, before conducting end-to-end game reproductions.

```text
+---------------------------------------------------------------------------------+
|                       Host Unit & Regression Tests (tests/)                     |
|  - Standalone C++ executables compiling specific subsystems in isolation        |
|  - Arithmetic semantics, window math, projection policy, preemption logic      |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                      Runtime Contract Verification (Recomp)                     |
|  - BCF28 helper bindings, Core1 RA overrides, stick-angle thunk verification    |
|  - rt64_host_endpoint_current_test (host latch & environment override)          |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|               ANGRYLION_CONTROL Ground Truth Oracle (tools/oracle/)             |
|  - Mupen64Plus Pure Interpreter + Angrylion RDP Plus + CXD4 LLE RSP (Linux)     |
|  - Bit-accurate software rasterization; golden frame & depth comparisons        |
+---------------------------------------------------------------------------------+
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Status |
| :--- | :--- | :--- | :--- |
| **REQ-TST-01** | Standalone Unit Tests | Tests execute directly on host without requiring game boot or graphics presentation. | `IMPLEMENTED` |
| **REQ-TST-02** | FPU Semantic Testing | `mips_rounding_test` validates IEEE 754 roundTiesToEven behavior. | `IMPLEMENTED` |
| **REQ-TST-03** | Host Endpoint Test | `rt64_host_endpoint_current_test` validates latching and environment overrides. | `IMPLEMENTED` |
| **REQ-TST-04** | Oracle Bit-Accuracy | `ANGRYLION_CONTROL` captures golden Z-buffer and pad state on Linux. | `IMPLEMENTED` |
| **REQ-TST-05** | Zero Oracle in Release | Angrylion and Mupen binaries must never be packaged into release client. | `IMPLEMENTED` |

---

## 3. Host Automated Test Suite Registry

Host tests are marked `EXCLUDE_FROM_ALL`. Tests that exercise guest wrappers use `boot_hw_stubs.cpp` and the unified
`TOOIE_WRAPPED_SYMBOLS` wrapping contract; independent subsystem tests do not require those guest stubs.

### 3.1 Test Execution Commands
- **Linux (`tools/build_linux.sh test`)**: Runs 19 core verification tests:
  - `tooie_story_coop_test` (production policy, native creation/allocator/descriptor/layout/teardown/activity, input epochs, form handoff and Mini-Sub dispatch; synthetic world/assets)
  - `tooie_menu_policy_test`
  - `tooie_menu_config_test`
  - `tooie_texture_streaming_test`
  - `tintro_projection_policy_test` (production context gate and dynamic window-aspect policy)
  - `tooie_object_distance_test` (native model cutoff/reset and actor gates with production distance/config policy)
  - `tooie_analog_camera_test` (production config/input policy and scoped native orbit/controller regression)
  - `recompinput_player_slots_test` (real SDL controller slots/profiles and per-read right-stick C reservation)
  - `rt64_host_endpoint_current_test`
  - `p09_interp_burst_test`
  - `p09_world_interpolation_guard_test`
  - `p09_local_pair_snapshot_test`
  - `p09_exact_use_correlation_test`
  - `rt64_present_decimator_test`
  - `tooie_dev_warp_test` (Dev Mode warp state machine, saved-position JSON, map names)
  - `tooie_dev_forms_test` (per-player selector and native handoff wrappers, with lifecycle stand-ins)
  - `tooie_glowbo_collection_test` (native actor dispatch/loop, deferred cleanup and shared inventory regression)
  - `tooie_dev_enemy_ai_test` (native perception/target/chase and Shovel update with scoped J1/J2 integration)
  - `tooie_dev_travel_menu_test` (native draw queue and Warp Pad/Silo callback regression, real Dev Mode guards)
  - `tooie_graphics_api_test` (platform-aware graphics API selection policy)
- **Windows (`tools\windows\Build-Tooie.ps1 -Stage Test [-AllTests]`)**:
  - Default runs 36 required verification tests (including Windows-only `rt64_hidden_window_test` and `tooie_startup_test`): the 19 above, `tooie_projection_reload_test`, `rt64_widescreen_bounds_test`, `rom_cart_import_test`, `tooie_texture_zip_mod_test`,
    `tooie_dev_cheats_test`, `tooie_expanded_memory_test` and the 5 language tests
    (`tooie_language_pack_test`, `tooie_text_font_map_test`, `tooie_language_import_test`, `tooie_font_sprite_test`,
    `tooie_language_bundle_test`). The Linux list does not include the language tests, the ZIP mod test, the cheats
    test or the expanded memory test yet.
  - Flag `-AllTests` runs all non-argument tests. Argument-driven tests report `SKIP` when invoked without input data.

### 3.2 Catalogue of Automated Test Targets

| Target | Source File | Scope / Purpose |
| :--- | :--- | :--- |
| `rt64_widescreen_bounds_test` | `tests/rt64_widescreen_bounds_test.cpp` | Real RT64 State/WorkloadQueue/RSP triangle submission without GPU/device/window/game; retained lateral Y bounds, upper/lower rows, depth-update policy, face-index identity, vertical scissor and Original/Manual/orthographic guards. `--disabled` checks the startup kill switch in a separate process. Synthetic geometry; visual gameplay acceptance remains separate. |
| `tooie_projection_reload_test` | `tests/tooie_projection_reload_test.cpp` | Native `func_800BC948` cache constructor and `func_800BCC90` with production projection hooks; checks both 48-byte banks, untouched surrounding bytes, frame-arena P/V display list, segment-8 path and reset. Matrix services are synthetic; no renderer or visual acceptance. |
| `tooie_texture_zip_mod_test` | `tests/tooie_texture_zip_mod_test.cpp` | Synthetic ZIP roots and relative metadata reads, ambiguity/invalid roots, real `ModContext` scanning, default manifest, `.rtz` regression and texture-only detection with an `.nrm` control; no ROM or game assets |
| `tooie_dev_warp_test` | `tests/tooie_dev_warp_test.cpp` | Dev Mode warp: teleport state machine (same map, other map, load detection, timeout), `dev_warps.json` round trip and malformed input, map names |
| `tooie_glowbo_collection_test` | `tests/tooie_glowbo_collection_test.cpp` | Executes real native collection forwarding, actor event/class dispatch, Glowbo callback/update, state setter, actor loop/range arithmetic, inventory arithmetic and deletion marking. Reproduces repeated credit without the hook, then checks J1/J2 near/far, repeated contacts, effect cleanup order/exactly once, prior collected state, sector gate, Mega Glowbo flag/dialogue/distinct counter, other classes/events and original-mode pass-through. World/assets, extension storage, save/UI/effect services and final actor-list reclamation are synthetic; no map, ROM or GPU is used. |
| `tooie_story_coop_test` | `tests/tooie_story_coop_test.cpp` | Production catalog/guards and lifecycle wrappers; real native creation, normal/descriptor allocators, pre-return activity, viewport layout, ascending destruction/teardown, Goliath reversion and Mini-Sub story/replay result dispatch. Synthetic model/input/world/subsystem leaves; no renderer, ROM or live challenge validation. |
| `tooie_dev_forms_test` | `tests/tooie_dev_forms_test.cpp` | Independent menu input/confirmation/cancellation and repeat, all 17 catalog selections, Clockwork exclusion, Dragon/Mumbo order, Golden Goliath 120-second entry seed and restoration of the prior handoff, native-wrapper handoff isolation, reversion precedence, Dragon variant without save-flag writes, viewport publication and reset. Links `RecompiledFuncs` to execute native reload, same-ID cache and auxiliary Dragon selection for direct normal/Dragon round trips on each player; synthetic model pairs/tables and asset-install/lifecycle stand-ins, not a gameplay/render test |
| `tooie_dev_cheats_test` | `tests/tooie_dev_cheats_test.cpp` | Dev Mode cheats: D-pad bits/names, per-player chord history and requests gated by unlock/enable/direction, simultaneous J1/J2 requests, disabled/reset/held-input suppression, invalid owner bounds, Fly/Blaster allowed states, actual game-thread executor with guest PlayerStates and native lifecycle stand-ins, J2 target isolation, independent Blaster exit/model cleanup, loading/Warp/map-reset drops; JSON round trip/defaults/fallbacks and two-player option/map gates. Does not execute native flight/Blaster animations, physics or rendering |
| `tooie_dev_enemy_ai_test` | `tests/tooie_dev_enemy_ai_test.cpp` | Real native focus/selector/perception/range/chase and Shovel update; scoped J2 acquisition/pursuit/attack target vector; Ground callback and native controller target-position path; Fly shared update/native pursuit predicate; both chase queries including entry 1's direct internal call; form exclusion, native activation flags, cache restoration, same-tick arbitration, nested scopes, hysteresis, eight-family routing, disabled/native-multiplayer/missing-player/other-class guards and reset. Player/world/animation/behavior services and five update bodies are stand-ins; no real map, attack damage, camera, projectile or renderer acceptance |
| `tooie_dev_travel_menu_test` | `tests/tooie_dev_travel_menu_test.cpp` | Real native sorted queue insertion, vector clear, queue dispatch, actor draw gate and Warp Pad/Silo draw callbacks; reproduces loss after J2 collection, then checks missing-menu replay, deduplication, native pass restoration, other classes, closed/deleted/replaced/reset/next-frame cleanup, J1-only collection and actual Dev Mode lock/settings, one-viewport, attract/native-multiplayer/missing-player guards. Actor lookup/extension storage, visibility input, memcpy service and final option renderer are synthetic; no GPU, map, controller or visual gameplay acceptance |
| `rt64_host_endpoint_current_test` | `tests/rt64_host_endpoint_current_test.cpp` | Validates host endpoint-current latch semantics and environment overrides. |
| `rt64_hidden_window_test` | `tests/rt64_hidden_window_test.cpp` | Windows-only: exercises real RT64 fullscreen/windowed transitions on hidden and visible Win32 windows in a private test desktop. Checks visibility, mode and restored bounds without a ROM, game runtime or input injection. |
| `window_display_test` | `tests/window_display_test.cpp` | Tests display index clamping, dimension sanitizing, and aspect ratio math. |
| `tintro_projection_policy_test` | `tests/tintro_projection_policy_test.cpp` | Executes production projection policy/window calculation with simulated host services: story/intro context, dynamic dimensions (4:3 through ultrawide), Original/Expand, invalid sizes and scene transitions. |
| `tooie_object_distance_test` | `tests/tooie_object_distance_test.cpp` | Production scopes and real Config with native model cutoff/reset/alpha, actor scheduling/radius and retention comparisons, spatial collector plus radial cell rejection, and sprite preparation/cutoff/fade. Extended distance coverage includes Expanded Memory gating (requires active expanded memory in process for 2x/4x/Unlimited, auto-enables option in config with effective Original until restart), runtime low-memory guard fallback (falls back to native when total free < 768 KiB or largest free block < 256 KiB), and switch `TOOIE_TEST_DISTANCE_HEAP_GUARD`. Covers Original/2x/4x/Unlimited, physical distance/zoom, model/sprite gradients, small-model threshold continuity, stable scripted alpha across omitted draws/map/view changes, player/global UI exclusions, native AABB/empty-cell rejection, unscoped helper isolation, flags/zero-radius/init/nested-event semantics, entering/during/leaving story co-op forcing Original on all paths with saved enum and graphics.json unchanged (611 assertions), mode guards and config Apply/Cancel/persistence/defaults. Model probes stop at segment conversion before geometry; synthetic RDRAM/model/grid inputs, camera/frustum, player/resource/descriptor services and sqrt. No rendered map or universal class/performance acceptance. |
| `tooie_expanded_memory_test` | `tests/tooie_expanded_memory_test.cpp` | Expanded memory latching and persistence, distance gating, low-memory guard, guest RAM range helper (`tooie_memory::contains`/`contains_alias`/`aligned`), and real recompiled `heap_setup`/alloc/free execution on the expanded region (`0x8080C000..0x81000000`); executed across 4 isolated processes. |
| `p09_exact_use_correlation_test` | `tests/p09_exact_use_correlation_test.cpp` | Tests seam-edge diagnostic keys, plan building, and memberships. |
| `p09_ordered_match_test` | `tests/p09_ordered_match_test.cpp` | Tests draw-call hash-ordered transform pairing. |
| `p09_world_interpolation_guard_test`| `tests/p09_world_interpolation_guard_test.cpp` | Tests P09 world transform interpolation guards. |
| `p09_local_pair_snapshot_test` | `tests/p09_local_pair_snapshot_test.cpp` | Tests transform pairing snapshot capture. |
| `p09_interp_burst_test` | `tests/p09_interp_burst_test.cpp` | Tests frame capture buffer formatting. |
| `rt64_present_decimator_test` | `tests/rt64_present_decimator_test.cpp` | Tests integer accumulator presentation rate decimation. |
| `tooie_fps_meter_test` | `tests/tooie_fps_meter_test.cpp` | Tests sliding-window presentation rate metering. |
| `tooie_menu_policy_test` | `tests/tooie_menu_policy_test.cpp` | Tests menu config coercion and framerate option interactions. |
| `tooie_texture_streaming_test` | `tests/tooie_texture_streaming_test.cpp` | Graphics > Texture Streaming with the real librecomp Config: fresh and legacy configs load Off, invalid values load Off, a saved level loads and reaches the renderer sink, UI changes reach it only on Apply and save the level name, Cancel reverts. Renderer stand-in; no game/window. |
| `tooie_menu_config_test` | `tests/tooie_menu_config_test.cpp`, `tests/graphics_window_mode_test.cpp` | Tests serialization and config sink callbacks. Links the real frontend graphics tab and shortcut with librecomp JSON storage; verifies Fullscreen/Windowed persistence across fresh config loads, a single renderer update per shortcut, pending-edit isolation and menu Apply. Uses a private config registry, temporary files and a renderer stand-in; no game/window is started. |
| `rom_cart_import_test` | `tests/rom_cart_import_test.cpp` | Tests retail cartridge ROM loading and in-memory decompression. |
| `stick_angle_thunk_test` | `tests/stick_angle_thunk_test.cpp` | Tests `func_80013B70` normalization across cardinals and diagonals. |
| `bcf28_helpers_test` | `tests/bcf28_helpers_test.cpp` | Verifies `func_800BCF28` binding fix and gap stub replacements. |
| `core1_ra_wrappers_test` | `tests/core1_ra_wrappers_test.cpp` | Verifies return address preservation across core1 overlay transitions. |
| `graphics_failure_test` | `tests/graphics_failure_test.cpp` | Verifies structured error reporting and exit on graphics initialization failure. |
| `input_direction_test` | `tests/input_direction_test.cpp` | Verifies bidirectional stick input mapping and cardinal quadrant alignment. |
| `matrix_submission_test` | `tests/matrix_submission_test.cpp` | Verifies display list matrix push/pop semantics and fixed-point conversions. |
| `mips_rounding_test` | `tests/mips_rounding_test.cpp` | Verifies IEEE 754 nearest-even rounding in floating-point to integer conversions. |
| `preemption_t15` | `tests/preemption_t15.cpp` | Verifies Hybrid F' preemption thread scheduling. |
| `p08_ws_frustum_test` | `tests/p08_ws_frustum_test.cpp` | Tests 16:9 widescreen view volume plane adjustments. |
| `p09_cpu_shader_clip_semantics_test`| `tests/p09_cpu_shader_clip_semantics.cpp` | Verifies CPU diagnostic shader matrix multiplication order. |
| `tooie_audio_capture_test` | `tests/tooie_audio_capture_test.cpp` | Tests audio stream buffer capture. |
| `tooie_bgm_volume_test` | `tests/tooie_bgm_volume_test.cpp` | Tests BGM sequence player volume scaling. |
| `tooie_bgm_hooks_test` | `tests/tooie_bgm_hooks_test.cpp` | Tests audio driver sequence update intercept hooks. |
| `tooie_audio_mod_test` | `tests/tooie_audio_mod_test.cpp` | Tests audio manifest rejection and PCM passthrough; with a private stereo 48 kHz MP3 fixture, validates decoding, loops, retriggers, gains, selectors, phase-preserving re-enable, native layer mixing at 1:20 and rapid reversals. An optional third argument points to an extracted private audio pack and validates its four actual MP3s, including surface/underwater contribution at a common 1:20 cursor. With `TOOIE_AUDIO_MOD_LOG=<file>` and the same path as a second argument, verifies that only nonzero mixing plus simulated successful queue acceptance emits `PLAY`; `-` skips log assertions. An optional fourth argument writes representative PCM for an ON/OFF byte comparison. This host test does not open an audio device. |
| `tooie_demo7_drift_test` | `tests/tooie_demo7_drift_test.cpp` | Tests attract demo kinematic monitoring. |
| `tooie_controller_ports_test` | `tests/tooie_controller_ports_test.cpp` | Tests N64 controller port policy for single-player and local multiplayer. |
| `recompinput_player_slots_test` | `tests/recompinput_player_slots_test.cpp` | Real SDL virtual controller assignment/profiles; four C directions with reservation on/off, remapped logical Z priority, merged fallback, physical C preservation, unrelated buttons and independent player routing. |
| `tooie_analog_camera_test` | `tests/tooie_analog_camera_test.cpp` | Production radial deadzone/proportional integration/config callbacks, Apply/Cancel/defaults/persistence, guest ownership/mode guards and actual native string controller plus pitch/yaw filters. First-person flag revokes reservation per owner, including nonmatching J2 camera/port; stale samples are discarded and smoothing restarts without requiring an orbit update during aim. Synthetic memory, target/math services and collision sink; no native first-person zoom execution, game or rendered-world acceptance. |
| `tooie_language_pack_test` | `tests/tooie_language_pack_test.cpp` | Tests TLPK parse/serialize and asset-table rewrite. |
| `tooie_text_font_map_test` | `tests/tooie_text_font_map_test.cpp` | Tests USA and European glyph mapping (`func_800D35D0`). |
| `tooie_language_import_test` | `tests/tooie_language_import_test.cpp` | Tests FR/DE/ES pack extraction from European + USA images. |
| `tooie_font_sprite_test` | `tests/tooie_font_sprite_test.cpp` | Tests font sprite parse/serialize and generated pt-BR font 1. |
| `tooie_language_bundle_test` | `tests/tooie_language_bundle_test.cpp` | Tests TLPB v1/v2 parsing, text/font generation, graphic deltas, CRC guards and asset relocation. Optional private fixtures compare real bundles with reference packs. |

### 3.3 Auxiliary and Python Test Scripts
- `tests/attract_demo2_transition_jal_test.py`: Validates disassembly JAL targets in attract transitions.
- `tests/n64recomp_zero_size_static_test.sh`: Validates static function end-boundary sizing in N64Recomp.
- `tools/test_rom_check.py`: Validates ROM checksum algorithms.
- `tools/test_syscall_decode.py`: Validates core2 syscall decode tables.
- `tools/test_metadata_invariants.py`: Verifies metadata consistency across table definitions.
- `tools/test_build_language_packs.py`: Tests the developer language-pack builder (`tools/build_language_packs.py`).

---

## 4. ANGRYLION_CONTROL Measurement Oracle

To establish bit-accurate ground truth independent of host graphics drivers, TooieRecomp uses `ANGRYLION_CONTROL` (detailed in [Angrylion Control Oracle](angrylion-control-oracle.md)):
- **Location**: `tools/oracle/angrylion-control/`.
- **Scripts**:
  - `run.sh`: Main invocation wrapper running Mupen64Plus with Angrylion RDP Plus and CXD4 LLE RSP. Requires `TOOIE_CART_Z64`.
  - `scripts/build.sh`: Builds local Mupen64Plus core and plugins on Linux.
  - `scripts/fetch.sh`: Fetches pinned repository dependencies.
  - `scripts/status.sh`: Verifies oracle build freshness and binary hashes.
- **Contract & Scope**:
  - Acts exclusively as an offline measurement oracle and regression detector.
  - Captures golden Z-buffer depth arrays, player coordinates, and pad state.
  - **Invariant**: TooieRecomp renders strictly via RT64; Angrylion is never bundled into the production game client.

## TK64 startup and independent audio tests

The Windows required suite includes `tooie_startup_test`, which exercises the real Win32 supervisor with
fake renderer workers: normal failure, crash recovery, readiness, effective API logging, preference preservation,
legacy/corrupt config handling and no game-start event. This is process/config proof, not GPU validation.

`tooie_launcher_audio_test <MP3>` is an argument-driven test using real `dr_mp3` and SDL dummy output. It covers
playback, repeated loops, bounded shutdown/restart and invalid input; copyrighted input stays private. Its SDL observation
shim is injected into the test executable only, with no production measurement hook. Build it explicitly
with the documented CMake build environment and run it with a user-supplied MP3. It is not part of the argument-free suite.

Real four-backend startup, visual/gameplay tests and loop listening remain `PENDING_USER`; Linux `NOT_RUN`.
See [startup diagnostics and manual procedure](startup-and-backends.md).

## 5. Experimental split-screen manual coverage

The retired `docs/future/two-player-split-screen.md` checkpoint is available in Git history. Production behavior is
specified in [Dev Mode](window-ui-and-config.md#45-dev-mode-tab); native mechanisms live in
[warp/player state](../../reverse-engineering/systems/warp-and-player-position.md),
[lighting](../../reverse-engineering/systems/player-lighting.md) and
[transformations](../../reverse-engineering/systems/player-transformations.md).
The table below preserves useful acceptance work; consolidation itself executes no game test and grants no new PASS.

| Area | Current evidence boundary | Remaining manual acceptance |
| --- | --- | --- |
| Basic local co-op | Windows user accepted two controlled players, shared map transitions, J2 collection and attacks | Check normal entrance/return, delayed J2 release and option Off on the next load; no full gameplay guarantee |
| Travel UI | Native queue/callback regression; visual acceptance with J2 distant still `PENDING_USER` | J1 opens Silo and Warp Pad with J2 distant/near; readable single list, navigate/cancel, correct destination, both players transition; repeat with Dev Mode locked and one player |
| Glowbo | Reported distant-J2 duplication accepted fixed on Windows | Mega Glowbo and Jolly Roger's Lagoon cases remain `NOT_RUN`; collect once with J2 distant, shared count rises once, actor disappears |
| Enemy targeting | Reported Windows case accepted fixed; eight audited families | Green/winged enemies in Jinjo Village and buried cowboy in mine; swap J1/J2 proximity, hold target during attack, two separated enemies, projectiles/cameras, map reset; remaining families/maps unverified |
| Character selection | Selector, Dragon same-model refresh and per-player shortcuts accepted for reported Windows cases | Two simultaneous menus, independent confirm/cancel, viewport clipping, Humba entry/exit and native reversion, persistence and secondary-model cases |
| Golden Goliath | Native 120 s seed implemented; no per-frame refill | Full countdown/expiry remains unverified; two simultaneous Goliaths share native HUD timer 2 |
| Fly/Breegull Blaster | Per-owner Windows shortcuts accepted | Repeat simultaneous use and staggered exits, verify other owner/models/camera unaffected; exhaustive form/map combinations unverified |
| Lighting | Instrumented Windows A/B and reported story visual issue accepted fixed | Specific primary/LOD/secondary models and transformations still need full playtest; allocation logging alone is insufficient |
| Camera | Analog exploration and native aiming zoom accepted for tested Windows cases | Exhaustive player/form/map coverage and original J2 orientation concern remain unverified; do not assume the older observation persists |
| Doors/death | Historical limits: J2 cannot trigger doors/map transitions; J2 death affects the shared player | Separate gameplay changes would be required; Warp Pads/Silos remain J1-only by user choice |
| Minigames | Room policy implemented; partial user acceptance; bird train/cart race leaves J2 accessible | MG01..MG23 acceptance matrix below; race extension deferred in [[future/implementation-ideas]] |
| Linux | This feature's native Linux playtest `NOT_RUN` | Run separately on the user's Linux PC; Windows/harness results do not validate Linux |

Before manual runs, back up saves/configs; close the game window instead of choosing "Save and Quit".
For selector checks, enable Dev Mode, open both player menus, select different forms, then test the original Humba
flows and range reversion. For shortcut checks, use the configured directions and release between presses.
Optional allocation capture remains documented in [player lighting](../../reverse-engineering/systems/player-lighting.md#manual-capture).


### 5.1 Minigame acceptance matrix

Stage ID is a manual test identifier, never a replacement guest flag number or a runtime event predicate.

| ID | Family / stage | Catalog flag | Primary story room | Native multiplayer room / related variant |
| --- | --- | --- | --- | --- |
| MG01 | Mayan Kickball quarterfinal | `0xBE3` | `0xC6` | `0x17F` |
| MG02 | Mayan Kickball semifinal | `0xBE4` | `0xC8` | `0x17F` |
| MG03 | Mayan Kickball final | `0xBE5` | `0xC9` | `0x17F`; `0xCA..0xCE` unresolved aliases |
| MG04 | Ordnance Storage | `0xBE6` | `0xDC` | `0x164`, `0x165` |
| MG05 | Dodgems 1 versus 1 | `0xBE7` | `0xDE` | `0x186` |
| MG06 | Dodgems 1 versus 2 | `0xBE8` | `0xDF` | `0x186` |
| MG07 | Dodgems 1 versus 3 | `0xBE9` | `0xE0` | `0x186` |
| MG08 | Hoop Hurry | `0xBEA` | `0xE5` | `0x17C` |
| MG09 | Balloon Burst | `0xBEB` | `0xE4` | `0x17B` |
| MG10 | Saucer of Peril | `0xBEC` | `0x124` | SOP variants `0x13B`, `0x13C`; shared return `0xD6`/route 6 |
| MG11 | Mini-Sub Challenge | `0xBED` | `0x181` | `0x182` |
| MG12 | Chompa's Belly | `0xBEE` | `0x123` | `0x183` |
| MG13 | Clinker's Cavern | `0xBEF` | `0x162` | Shared FPS machinery; no separate MP Clinker room established |
| MG14 | Twinkly Packing | `0xBF0` | `0x10A` | `0x17D` |
| MG15 | Colosseum Kickball quarterfinal | `0xBF1` | `0x12E` | `0x180` |
| MG16 | Colosseum Kickball semifinal | `0xBF2` | `0x12F` | `0x180` |
| MG17 | Colosseum Kickball final | `0xBF3` | `0x130` | `0x180` |
| MG18 | Pot O' Gold | `0xBF4` | `0x13D` | No separate MP room established |
| MG19 | Trash Can Germs | `0xBF5` | `0x137` | `0x185` |
| MG20 | Zubbas' Hive | `0xBF6` | `0x139` | `0x188` |
| MG21 | Tower of Tragedy round 1 | `0xBF7` | `0x15F` | `0x189` |
| MG22 | Tower of Tragedy round 2 | `0xBF8` | `0x15F` | `0x189` |
| MG23 | Tower of Tragedy round 3 | `0xBF9` | `0x15F` | `0x189` |

Named identities: `PROVEN / STATIC_SOURCE`. Family/room associations are `STRONG_INFERENCE` unless
[[reverse-engineering/systems/minigame-controllers]] supplies a direct native map branch/retry.
MP columns describe related native map names, not a claim that
all individual replay modes enter that map. The story policy must also work in replay without assuming its route.

Each `MG01..MG23` starts `NOT_RUN`. For each stage, record the following columns separately:

```text
story entry; preparation/instructions; active play; J2 input/contact isolation;
win; loss/timeout; retry; result/menu readability; leave; restored story pair;
individual replay if accessible; native MP if applicable; native/debug-Off regression
```

Mark unsupported scenarios `N/A` with a reason; inaccessible ones `BLOCKED`/`NOT_RUN`, not PASS. Do not warp into
the arena and claim this proves its story entry/camera transition. Do not alter save unlocks to make a cell pass.

For controls, try J2 movement, attack and debug shortcut during the event. J1 must retain the native event controls
and scoring. Verify no hidden J2, second shadow, pickup credit, projectile, AI response or camera contribution.
On return, confirm J2 is controllable, privately lit, can collect one Glowbo once, can be pursued by a supported
enemy, and can use Analog Camera with Z+C priority if that option is enabled.

For Saucer, record the entire ride, intermediate loads and end/abort/retry paths, including result/score dialogue
in 0xD6 with route 6 and the deliberate wait for the next eligible recreation after it clears to 0. For both Kickballs, finish all
three rounds rather than testing only the first. For quiz, cover all three rounds in the same room. For Packing,
verify room exploration before starting remains single-player under the selected room-wide policy.

Use existing save/config backup procedure. Never choose Save and Quit as a test harness shortcut; it writes the save.

### 5.2 Event-specific manual discriminators

These criteria are not executed results; all individually unconfirmed cases remain `NOT_RUN`.

| Family | Additional checks |
| --- | --- |
| Mayan Kickball | Run all three stages, loss/retry on each, final reward and lobby return. Repeat with J2 pressing attack/movement/debug shortcut. Native MP 0x17F must retain its original human count, including one human. |
| Ordnance Storage | Test entry from 0x163, countdown, target destruction, timeout, retry, success, and exit back to 0x163. Validate 0x164 and 0x165 separately in native multiplayer and accessible individual replay. |
| Dodgem Challenge | Validate each opponent count, win/loss/retry, bumper interaction, balloons/score HUD, stage unlock progression and 0xDD return. Also test native MP with one and multiple humans. |
| Hoop Hurry | Test waiting before start, event start, scoring, timer expiry, success, retry in the same map and departure. Verify there is no delayed J2 release after waiting more than two seconds. |
| Balloon Burst | Test both success and timeout, same-map retry, event camera transitions, exit and restored exploration pair; test native MP separately. |
| Saucer of Peril | Record every native map load across an entire ride, all intermediate segments, normal finish, abort, failure, retry and individual replay. Verify ordinary Witchyworld 0xD6 entry before the ride is still co-op, return/results remain native single-player, and a later eligible creation restores co-op. A test of only the starting room is insufficient. |
| Mini-Sub Challenge | Story cave entry, start, target score, success/failure, repeat and leave; verify ordinary 0x1A9 before/after. Test native 0x182 count=1 and >1 separately. Exercise the replay frontend choices independently of story retry/exit; the test must not assume every result callback loads a map. |
| Chompa's Belly | Start from actual Chompa entry, shoot each target type, exercise score/time finish and loss, same-room repeat, exit and restored pair. Verify both story and accessible replay. |
| Clinker's Cavern | Actual sewer/cavern transition, FPS setup, all target kills, timeout, completion, exit and restored normal co-op. Also run an unrelated FPS area as a negative control. |
| Twinkly Packing | Stand in room before starting for >2 seconds; start challenge; collect/deposit all relevant Twinkly variants; win/loss/retry; leave. Confirm selected room-wide policy with no premature J2 release. |
| Colosseum Kickball | Run each round including loss/retry and final victory, and ensure original Colosseum scoring remains intact. Test native MP 0x180 with one and multiple humans. |
| Pot O' Gold | Activate/access through normal story progression, enter, start, hit targets, timeout/win, repeat if offered, and leave. Validate exterior co-op before/after. |
| Trash Can Germs | Actual entry/start, germ interaction, success and timeout/failure, repeat and exit. Confirm no hidden J2 projectile/attack survives the transition. Test native MP 0x185 separately. |
| Zubbas' Hive | Enter in the legitimate story form, start, score/waves, timeout/loss, success, repeat and exterior return. Exercise native MP count 1 and >1. |
| Tower of Tragedy | Exercise all three rounds separately, answer input, scoreboard, loss/elimination and final success, and the transition out. Individual replay round selection and native MP 0x189 require independent coverage. |
