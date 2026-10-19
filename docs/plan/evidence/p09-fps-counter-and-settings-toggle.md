---
title: "P09 — Top-left Present FPS counter and settings toggle"
description: "Top-left present FPS counter and Graphics settings toggle verified; RmlUi overlay hook and RT64 present success counter documented; unit tests PASS; runtime QA not performed."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-10-17
tags: [p09, fps, counter, settings, overlay, rt64, recompfrontend, present-cadence]
---

# P09 — Top-left Present FPS counter and settings toggle

## 1. Context & User Directive

Under explicit user authorization:
- A visible top-left FPS counter and an extra settings toggle are authorized for TooieRecomp.
- The counter must measure **actual successful present cadence** (frames queued to the swapchain where `present()` succeeded), NOT the selected target rate, NOT the guest VI rate, and NOT simulation or motion rate.
- The toggle must default to **OFF** (`false`), persist through the owning configuration (`graphics.json`), and introduce no unnecessary hotkeys or extra telemetry stats.
- Existing frontend/RT64 overlay mechanisms should be reused rather than introducing parallel custom HUD or UI frameworks.
- The default-ON CURRENT `tfPos` workaround (`tooie_rsp_endpoint_current`), simulation/audio timing, below-native presentation decimator (`PresentDecimator`), and existing user settings must remain completely unchanged.
- Investigation reports `counter-grok.json` and `counter-sonnet.json` in `/home/wesley/.cache/tooie-fps-investigation` were thoroughly analyzed and verified.

---

## 2. Analysis of Investigation Reports

### 2.1 Grok 4.7 Read-Only Architecture Analysis (`counter-grok.json`)
- **Metric Verification**:
  - `ext.swapChain->present(...)` return value is the only true success gate.
  - `presentTimestamp` in `rt64_present_queue.cpp:508` is recorded *before* `present()` returns, and serves as the pacing clock for `preciseSleepUntil` when `targetRate > viOriginalRate`. It is not a success counter or display metric.
  - `presentProfiler` (`rt64_state.cpp:2465-2487`) logs frame presentation time in milliseconds for developer mode / ImPlot, not a top-left label.
  - `frameCounters.presented` is an interpolation counter; `recomp_get_target_framerate` returns user target; `viOriginalRate` is the guest VI rate (formally unmeasured). None of these measure present rate.
- **Overlay Evaluation**:
  - RT64 developer-mode inspector requires `developer_mode` on and renders heavy ImPlot / ImGui windows; not suitable for a clean, unobtrusive player HUD.
  - `GetRenderHookDraw()` operates on the swapchain framebuffer before present; drawing text there requires direct font rendering in RT64.
- **Config Extension Point**:
  - `add_graphics_options` in `src/game/config.cpp:164` extends the RecompUI graphics tab and persists with `recomp::config::Config`.

### 2.2 Claude Sonnet 5.5 Implementation Analysis (`counter-sonnet.json`)
- **Implemented Mechanism**:
  - Added "Show Present FPS" boolean option to `add_graphics_options` in `src/game/config.cpp`, default `false`, persisted in `graphics.json`. Change callback updates an atomic flag `tooie::set_fps_overlay_enabled()`.
  - Added atomic counter `RT64::presentSuccessCount` incremented strictly upon successful `swapChain->present()` in `rt64_present_queue.cpp`. Skipped presents (including below-native decimator skips) and failed presents do not increment.
  - Added clean RmlUi overlay hook `recompui::set_overlay_update_callback` in `RecompFrontend` (`recompui/src/base/ui_state.cpp`). Called on the render thread once per presented frame in `draw_hook()`. It is not a shown context (unaffected by `hide_all_contexts()` and captures no input).
  - Implemented `include/tooie_fps_meter.h`, `include/tooie_fps_overlay.h`, and `src/game/fps_overlay.cpp`.
  - Added host unit test `tests/tooie_fps_meter_test.cpp` validating sliding 1-second window calculation, rounding, zero presents, stalls > 3 s, backwards clock, and resets.
  - Exported standalone versioned patches for external submodules:
    - `patches/frontend/recompfrontend-overlay-update-callback.patch`
    - `patches/rt64/rt64-present-success-counter.patch`

---

## 3. Verified Implementation Details

### 3.1 Settings Option & Persistence
- **Location**: `src/game/config.cpp` (inside `add_graphics_options`), `include/banjo_config.h` (`banjo::configkeys::graphics::show_fps_counter`).
- **Option Label**: `"Show Present FPS"`
- **Option Description**: `"Shows the rate of frames actually presented to the display in the top-left corner. This is not the game's simulation or animation rate."`
- **Default Value**: `false` (OFF).
- **Persistence**: Automatically saved to and loaded from `graphics.json` by `recomp::config::Config`.
- **Change Callback**:
  ```cpp
  config.add_option_change_callback(
      banjo::configkeys::graphics::show_fps_counter,
      [](recomp::config::ConfigValueVariant cur_value, recomp::config::ConfigValueVariant, recomp::config::OptionChangeContext) {
          tooie::set_fps_overlay_enabled(std::get<bool>(cur_value));
      }
  );
  ```
- **Registration**: `banjo::init_config()` calls `tooie::register_fps_overlay()`.

### 3.2 User Settings Directions
1. Open the in-game or launcher configuration menu.
2. Navigate to the **Graphics** tab.
3. Locate the **"Show Present FPS"** checkbox under the graphics settings.
4. Check the box to enable the counter; uncheck to disable (default is OFF).
5. The setting is saved immediately in `graphics.json` and takes effect without restarting the game.

### 3.3 Counter Mechanism & Cadence Meter
- **RT64 Present Success Counter**:
  - Header: `src/hle/rt64_present_stats.h` defines `extern std::atomic<uint64_t> presentSuccessCount;`.
  - Queue integration: In `src/hle/rt64_present_queue.cpp:513`:
    ```cpp
    swapChainValid = ext.swapChain->present(swapChainIndex, &waitSemaphore, 1);
    if (swapChainValid) {
        presentSuccessCount.fetch_add(1, std::memory_order_relaxed);
    }
    ```
  - Skipped presentations (e.g. from `PresentDecimator`) and failed presents do not increment the counter.
- **Cadence Meter (`tooie::PresentRateMeter` in `include/tooie_fps_meter.h`)**:
  - Maintains a sliding monotonic window (nominal 1 second / 1000 ms).
  - Computes rounded integer rate: `(sample_delta * 1000 + window_ms / 2) / window_ms`.
  - Handles pauses and stalls: if a sample gap exceeds 3 seconds (`kStallGapMs = 3000`), the meter resets its window and reports no valid value (displays `"-- FPS"`).
  - Handles non-monotonic clock jumps or backwards counter resets gracefully.
- **Overlay UI Rendering (`src/game/fps_overlay.cpp`)**:
  - Uses `recompui::set_overlay_update_callback` executing in `draw_hook()` on the render thread.
  - Positioned 16 dp from the top-left corner with dark backing (`rgba(0,0,0,160)`).
  - Displays `"%u FPS"` when valid, `"-- FPS"` when stalled/initializing.
  - Updates RmlUi element text only when the rounded integer value changes, avoiding unnecessary DOM/layout reflows.

---

## 4. Epistemic Limits & Critical Behavioral Distinctions

### 4.1 Present Cadence vs. Simulation and Motion Rate
- **Actual Present Cadence**: The counter measures frames successfully delivered to the display swapchain per second.
- **Pose Repeat Limitation under CURRENT Workaround**:
  - The default-ON palliative workaround (`tooie_rsp_endpoint_current`) forces vertex positions (`tfPos`) to CURRENT.
  - When the selected presentation target is higher than the native guest update rate (e.g., target 60 FPS while guest scene updates at ~30 Hz), RT64 generates extra presents.
  - Because `tfPos` is CURRENT, these intermediate presents **repeat the current vertex pose**.
  - As a result, the counter will accurately read **~60 FPS**, while character and object motion visibly updates at **~30 Hz**.
  - The counter must NEVER be referred to as "simulation speed" or "motion rate"; it reflects strictly display present frequency.
- **Ceiling Effects**: `present()` returning true confirms submission to the swapchain queue; compositor or vsync pacing at the OS/driver layer represents the real ceiling.
- **Native Logic Rate**: `viOriginalRate` remains formally `UNKNOWN` (unmeasured).

### 4.2 Runtime QA Boundary
- **Compile & Unit Proof Only**: All verification in this checkpoint consists of static source analysis, patch integrity checks, compilation gates, and isolated host unit tests.
- **In-Game Runtime QA Not Performed**: The game executable (`TooieRecompiled`) was NOT launched during this verification. No interactive visual confirmation of the on-screen overlay in live gameplay is claimed.

---

## 5. Build, Test & Patch Evidence

### 5.1 Host Unit Test Evidence
- **Command**: `./build/tooie_fps_meter_test`
- **Exit Code**: `0`
- **Output**: `tooie_fps_meter_test PASS`
- **Test Scope**:
  - Exact 1-second cadence calculation.
  - Midpoint rounding (e.g. 59.5 rounds to 60).
  - Zero present increments.
  - Resync on sample gaps > 3 seconds (stalls).
  - Non-monotonic counter or clock handling.
  - Reset behavior on disable.

### 5.2 Patch Verification & Hashes
Both external dependency patches apply cleanly and are verified reverse-checkable:

| Patch File | Target Repository | Commit Base | SHA256 Hash |
| :--- | :--- | :--- | :--- |
| `patches/frontend/recompfrontend-overlay-update-callback.patch` | `BanjoRecomp/lib/RecompFrontend` | `d0d90ba` (clean) | `37266d0bd13d42c0b8f174c0118b6a9b2490784c5880f05e6ee37ab811cd48a7` |
| `patches/rt64/rt64-present-success-counter.patch` | `BanjoRecomp/lib/rt64` | `6f1c2d99` (dirty) | `173c9b7cafc4b9750fb610b56faca8125399679a874d1c159f6c53e420c31c68` |

- **Reverse Application Check**:
  ```bash
  git -C ../BanjoRecomp/lib/RecompFrontend apply --reverse --check /home/wesley/projetos/N64-Recomp/TooieRecomp/patches/frontend/recompfrontend-overlay-update-callback.patch
  git -C ../BanjoRecomp/lib/rt64 apply --reverse --check /home/wesley/projetos/N64-Recomp/TooieRecomp/patches/rt64/rt64-present-success-counter.patch
  ```
  Result: Exit code `0` for both.

### 5.3 Worktree & Git Hygiene Checks
- `git diff --check`: Exit code `0` on `TooieRecomp`, `RecompFrontend`, and `rt64`.
- Pre-existing dirty state in `rt64` (31 files +13869/-45) and `N64ModernRuntime` was fully preserved.
- No code, config, build, or git mutations were made during this documentation task.

---

## 6. Single Next Action
`NEXT_SINGLE_ACTION`: Perform user/root authorized runtime QA by launching `./build/TooieRecompiled`, toggling "Show Present FPS" in Graphics options, and visually confirming top-left overlay cadence and pose-repeat behavior.
