---
title: "TooieRecomp Spec — Graphics, Presentation and High-Framerate Interpolation"
description: "Comprehensive technical specification for RT64 integration, widescreen culling hooks, projection policies, the shader-local CURRENT tfPos workaround, and planned FPS presets."
status: current
area: "specs/Tooie"
updated: 2026-10-22
tags: [specs, tooierecomp, graphics, rt64, widescreen, interpolation, tfpos, fps, resolution, aspect-ratio]
---

# Graphics, Presentation and High-Framerate Interpolation Specification

## 1. Subsystem Overview

TooieRecomp renders original Nintendo 64 display lists through `RT64`, translating F3DEX2 commands to modern Vulkan 1.2+ pipelines while injecting native transforms to support widescreen viewports and high-framerate presentation.

```text
+---------------------------------------------------------------------------------+
|                       Guest Display List Submission                             |
|  - Viewport Setup & Matrices: func_800E4504                                     |
|  - Matrix Reload / Cache: func_800E4640, func_800E4628                          |
|  - Cutscene Aspect Policy: tooie_tintro_projection_policy::apply                |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                       Aspect-Aware Frustum Culling Hooks                        |
|  - Map Geometry: func_800CAA24 (H-plane n_cam_x /= k, production default ON)    |
|  - Object Side-Plane: func_800CACEC (Actor frustum expand, production ON)       |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                      RT64 Workload Queue & Transform Processor                  |
|  - Matrix matching across successive workloads                                  |
|  - Target Rate vs viOriginalRate evaluation (pacing constraint)                 |
|  - Target Rate Policy: Interpolation OFF = Manual 30; ON = user rate (min 60)    |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                     RSPProcess Compute Shader (RSPProcessCS.hlsl)               |
|  - endpoint-CURRENT via RT64::endpointCurrentActive() (Interpolation OFF = on)   |
|  - tfPos bypassed to CURRENT vertex positions; fog derives from tfPos.z/w       |
|  - Interpolation option ON = off; explicit env value still overrides            |
+---------------------------------------------------------------------------------+
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Implementation Status |
| :--- | :--- | :--- | :--- |
| **REQ-GFX-01** | Aspect-Aware Map Culling | `func_800CAA24` scales horizontal culling plane (`n_cam_x /= k`). | `IMPLEMENTED` / `VALIDATED` |
| **REQ-GFX-02** | Aspect-Aware Object Culling | `func_800CACEC` expands culling planes during draw passes (pure `func_800B97A4` + preserved `func_8011447C`). | `IMPLEMENTED` / `VALIDATED` |
| **REQ-GFX-03** | Cutscene Aspect Policy | Preconditions projection matrix via `compensation_factor`. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-GFX-04** | tfPos CURRENT Workaround | Active whenever the Interpolation graphics option is OFF (default); host-controlled via `RT64::requestHostEndpointCurrent`, no setenv. See §3.3. | `IMPLEMENTED` / `VALIDATED` (2026-10-21 runtime; supersedes the startup `setenv` default) |
| **REQ-GFX-05** | Diagnostic Env Override | An explicit `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` (`0`/`1`) in the environment overrides the Interpolation option; unset/empty defers to the option. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-GFX-06** | Interpolation Graphics Option | Graphics option `interpolation` (bool, persisted in `graphics.json`, default/missing/invalid = OFF). OFF locks presentation to Manual 30 (`kForcedRefreshRate`) with the CURRENT endpoint; ON unlocks the Framerate controls and selects Manual 60 when the stored rate would generate no intermediate frames; applies at runtime on Apply without restart. User-authorised 2026-10-21 (supersedes the old frozen 30 FPS lock). Does not claim the articulated-glitch gate passed. | `IMPLEMENTED` / `VALIDATED` (see [[../../plan/evidence/p09-menu-interpolation-toggle-20261021]]) |
| **REQ-GFX-07** | Below-Native Presentation Decimation | Present-only decimation preserving guest updates, audio, and ACK signals. | `IMPLEMENTED` / `STATICALLY_VERIFIED` |
| **REQ-GFX-08** | Intro Cinematic Walls | Eliminates geometry edge holes in SCENE_A and SCENE_B. | `PLANNED` / `NOT_IMPLEMENTED` |
| **REQ-GFX-09** | Resolution 3x & 4x Scaling | Adds `Original3x` (720p) and `Original4x` (960p) base options with multiplicative downsampling. Auto resolution uses integer window VI scale and disables downsampling. | `IMPLEMENTED` / `VALIDATED` (see [[../../plan/evidence/p08-settings-functionality-20261019]]) |
| **REQ-GFX-10** | Aspect Ratio Window Auto-Resize | Original 4:3 and Expand 16:9 auto-resize host window dimensions in windowed mode, keeping current height. | `IMPLEMENTED` / `VALIDATED` (see [[../../plan/evidence/p08-settings-functionality-20261019]]) |

---

## 3. Interface & Mathematical Contracts

### 3.1 Aspect-Aware Frustum Culling
1. **Map Geometry (`func_800CAA24`)**:
   - Math: Let $k = \frac{\text{target\_aspect}}{\text{original\_aspect}}$ (where original is $\frac{4}{3}$).
   - The horizontal camera-space plane normal is scaled: $n_{\text{cam}, x}' = \frac{n_{\text{cam}, x}}{k}$.
   - Configuration: Controlled via `TOOIE_TEST_MAP_VIS_ASPECT_AWARE` (default `1`). Setting `0` forces original 4:3 culling.
2. **Object Frustum (`func_800CACEC`)**:
   - Expands horizontal bounding planes specifically for dynamic actor and prop draws.
   - Restricts expansion to allowlisted pure DRAW callers (`func_800B97A4` scope kind 3, `func_8011447C` scope kind 2). Generic DRAW scope on `func_800E3E8C` is dropped to exclude mixed-renderer callers (`func_800DE498`).
   - Callers evaluating spatial occupancy or fog (`0x8011458C`, `chlightfader`) retain original unexpanded culling.
   - Configuration: Controlled via `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE` (default `1`). Setting `0` restores original.
   - Parity resolution note: The downstream simulation divergence in USER_DEMO1 was resolved by removing generic DRAW scope from `func_800E3E8C` and allowlisting pure draw `func_800B97A4` (kind 3) alongside preserved `func_8011447C` (kind 2), while excluding mixed renderer `func_800DE498` (which stays original `v0`). Under default-ON (binary A6A2), USER_DEMO1 restores 355-post trajectory, record words, RNG, and post 355 Fire Egg hit. Widescreen edge pop preservation in gameplay is visually confirmed for the tested area by direct user QA ("Sim, continua funcionando", USER_REPORTED, VISUALLY_CONFIRMED_FOR_TESTED_AREA; broader game parity remains untested residual).

### 3.2 Cutscene Projection Compensation (`tooie_tintro_projection_policy`)
```cpp
namespace tooie_tintro_projection_policy {
    inline float source_aspect_from_projection(float p00, float p11) {
        if (!std::isfinite(p00) || !std::isfinite(p11) || std::fabs(p00) < 0.001f) return 0.0f;
        const float aspect = std::fabs(p11 / p00);
        return std::isfinite(aspect) ? aspect : 0.0f;
    }

    inline float compensation_factor(float target_aspect, float source_aspect) {
        constexpr float kViAspect = 4.0f / 3.0f;
        if (!std::isfinite(target_aspect) || !std::isfinite(source_aspect) ||
            target_aspect <= kViAspect || source_aspect <= 0.0f) return 1.0f;
        return std::min(target_aspect, source_aspect) / kViAspect;
    }

    void apply(uint8_t* rdram, float proj[4][4]);
}
```
Validated via `tests/tintro_projection_policy_test.cpp` (7/7 assertions PASS).

### 3.3 Shader-Local CURRENT `tfPos` Workaround (P09 Phase 1) and Host Switch
```cpp
// RT64 (patches/rt64/rt64-p09-host-endpoint-current-toggle.patch): src/hle/rt64_host_endpoint_current.h
namespace RT64 {
    void requestHostEndpointCurrent(bool endpointCurrent); // any thread (config callback)
    void latchHostEndpointCurrent();                       // render thread, once per game workload
    bool endpointCurrentActive();                          // env override > latched host request > off
}
```
- **Owner**: `install_interpolation_policy` (`include/tooie_menu_policy.h`) sinks the APPLIED `interpolation` value; `src/game/config.cpp` forwards it as `requestHostEndpointCurrent(!enabled)`. No startup `setenv`; `tooie_rsp_endpoint_current` (module, test, CMake target) was removed.
- **Readers (all use `endpointCurrentActive()`)**: `RSPProcessor::process` (`processCB.forceEndpointCurrent`), `p09WorldGuardEndpointCurrentActive`, `interpBurst::forceEndpointEnv`. The old per-site static getenv caches no longer exist.
- **Environment Precedence**: `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` explicit `0`/`1` wins over the option (legacy test override, read once); unset/empty defers to the host. `TOOIE_TEST_INTERPOLATION_FPS=60|120` forces Interpolation ON at that rate regardless of the option and logs `[tooie] TOOIE_TEST_INTERPOLATION_FPS=... overrides the Interpolation setting`.
- **Interpolation Matching & Diagnostic Controls (P09)**:
  - `TOOIE_TEST_P09_ORDERED_MATCH`: Controls structural draw call sequence matching for untagged (`G_EX_ID_AUTO`) WORLD transforms in RT64 `GameFrame::matchScene`. Default ON; setting `0` restores the original greedy origin-cost matching.
  - `TOOIE_DIAG_P09_ROD_CLASSIFY=<arquivo>`: Diagnostic output file path (default-OFF). When configured, evaluates every triangle of interpolated workloads after guard processing and classifies stretched geometry (rendered edge $> 2\times$ CURRENT edge and $> 4$ units) across categories I, U, P, C, and N into the specified file. Strictly read-only; never alters rendering.
- **Runtime telemetry**: RT64 prints `[tooie-interp] workload=N endpointCurrent=E generate=G targetRate=T viRate=V displayFrames=D` on every change of that tuple and one `first interpolated render ... curW=` line per change; never per frame. `endpointCurrent=1` with `curW<1` is masked interpolation (CURRENT repeats poses); real interpolation is `endpointCurrent=0`.
- **Shader Effect (`RSPProcessCS.hlsl:78-93`)**:
  - `tfPos` bypasses temporal interpolation, evaluating directly at CURRENT vertex positions:
    `tfPos = mul(mul(endpointViewProj[...], endpointWorld[...]), endpointPos);`
  - In `RSPProcessCS.hlsl:86-93`, fog calculation derives directly from `tfPos`:
    `const float fogAlpha = ((max(tfPos.z, 0) / tfPos.w) * rspFog.mul + rspFog.offset);`
    Because `tfPos` also drives fog, snapping `tfPos` to CURRENT ensures fog depth remains perfectly aligned with current mesh geometry without temporal lag or shearing.
  - View, projection, and texgen continue normal temporal handling.
- **Verification**: `tests/rt64_host_endpoint_current_test.cpp` (env unset/0/1, latch semantics) and `tests/tooie_menu_config_test.cpp`; runtime proof in [[../../plan/evidence/p09-menu-interpolation-toggle-20261021]]. The former `tooie_rsp_endpoint_current_test` was removed with its module.
- **Versioned Clean-Pin Patches & Provenance**: `patches/rt64/rt64-p09-host-endpoint-current-toggle.patch` (SHA256 `b96e2b703ae0d4782b2debf1178584355590865a7a17d7d74a128745cd5d8f15`, stacked after the guard/burst patches; replay-checked with `git apply --check` on the clean chain) and `patches/rt64/rt64-rsp-endpoint-current.patch` (SHA256: `49925480c2d1736c790ddf96152844951f6fe27a150ab1cc110347d557c0d813`), integrated into vendored `lib/rt64` against clean RT64 pin `6f1c2d99a4ea571c139f449c326fd176ba8f3496` (`tools/apply_runtime_patches.sh` retired; provenance tracked in `vendor/patches-applied/ORDER.tsv`, verified via `tools/vendor_verify.sh --reverse-check`).
- **Workaround vs. Permanent Fix Classification**:
  - The `tfPos` CURRENT bypass (active while Interpolation is OFF, the default) is strictly a **palliative workaround**. It prevents visible mesh tearing (such as Kazooie's wing feathers and Banjo's backpack/snout) by evaluating vertex positions at CURRENT.
  - **Continuous vertex position interpolation at 60+ FPS is NOT IMPLEMENTED**.
  - The permanent architectural fix resolving `NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS` remains **NOT IMPLEMENTED**. Experimental articulated guards (translation coherence and closure guard in `patches/rt64/rt64-p09-world-interpolation-guard.patch`, with Mechanism A linear speed gate in `rt64_rigid_body.{h,cpp}`) were evaluated across iterations 1-3 in October 2026. While Mechanism A is `PROVEN_SOURCE` and matched GPU pairs exist for the title scene (wl 1000-2500), independent verification gates remain failed (`gate.json: passed = false`, `resumed-gate.json: passed = false`) due to unverified build logs, a fatal SIGSEGV (-11) crash in `runs/ong2` during attract demo gameplay, unversioned working tree diagnostic state, and absent non-player articulated and 60 FPS gameplay verification. The default (Interpolation OFF) keeps the CURRENT endpoint and Manual 30 FPS (`kForcedRefreshRate = 30`); turning Interpolation ON is an explicit user choice that exposes those experimental, still-gated glitches (`NOT` a fix, technical gate `passed: false`).

---

## 4. Phase 2 Exact FPS Presets & Below-Native Presentation Decimation

### 4.1 Target Presets & Forced 30 FPS Policy Lock
Requested user presets: `{20, 30, 45, 60, 70, 100, 120, 144}` FPS.
- **Frontend UI Option & Policy Enforcement (REQ-GFX-06)**: `tooie_menu::install_interpolation_policy` ties the persisted `interpolation` bool to the Framerate options. OFF: Manual/30, rate controls disabled with `Locked to 30 FPS`. ON: controls enabled; `coerce_refresh` keeps the stored rate unless it is Original or Manual <= 30, which becomes Manual 60. Legacy `TOOIE_TEST_INTERPOLATION_FPS` remains an explicit override.
- **Policy Enforcement Contract**: The UI (temporary) view is coerced on every change and is reverted to the applied rate on cancel; the applied view is coerced and sunk to RT64 on load (after the last of the three options has loaded, so a stored 120 survives boot) and when the `interpolation` option is applied (Apply button, graphics `requires_confirmation`). Callbacks resolve `recompui::config::get_graphics_config()` per call (vector reallocation). The RT64 request is latched at the next workload boundary: no restart and no `setenv`. Dedicated 8-preset selector widget remains `PLANNED` / `NOT_IMPLEMENTED`. Host test: `tests/tooie_menu_config_test.cpp`.

### 4.2 Below-Native Presentation Decimation (REQ-GFX-07)
- **Problem**: In upstream RT64 (`rt64_workload_queue.cpp:1175`), when `targetRate <= viOriginalRate` (e.g. 20 or 30 FPS target on a 60 Hz VI scene), RT64 emitted one presentation per native workload with `displayFrames = 1` and no sleep or pacing. Below-native target was an effective no-op.
- **Implementation (`rt64_present_decimator.h` & `rt64_present_queue.cpp`)**:
  - `PresentDecimator` integer accumulator (`ticks += targetRate`, `ticks >= originalRate`) computes exact average presentation rates for divisors (20/60, 30/60, 20/30) and non-divisors (45/60, 70/60, 100/60) with zero float drift and no integer overflow (`uint32_t`).
  - Active condition: `manualRate && !paused && targetRate > 0 && viOriginalRate > 0 && targetRate < viOriginalRate`.
  - Manual target clamped to swapchain rate when known; applies also if panel rate is below VI rate.
- **Workload & ACK Flow (Static Proof)**:
  - Skipped presentations route through the existing catch-up skip path in `PresentQueue::threadLoop()`: `skipInterpolation()`, `notifyPresentId(present)`, framebuffer-operation release, and `threadAdvanceBarrier`.
  - Every guest workload is fully consumed; guest logic, audio processing, and VI events are never slept or stalled. Workload ACK signals flow unimpeded.
- **Concurrency & Data Race Elimination**:
  - `viOriginalRate` write in `rt64_workload_queue.cpp:1021` is protected under `std::scoped_lock(ext.sharedResources->configurationMutex)`.
  - Reads in `rt64_present_queue.cpp` occur under `configurationMutex`. Lock hierarchy is strictly preserved: `configurationMutex` is a leaf lock across both workload and present threads, preventing lock inversion.
- **Cadence Resets**:
  - Catch-up skip explicitly resets the decimator (`presentDecimator = PresentDecimator()`).
  - Paused presents pass `paused == true` into `shouldPresent()`, resetting cadence so the first unpaused frame displays immediately without dropping.
- **Verification & Status**:
  - Validated by host test `tests/rt64_present_decimator_test.cpp` (`rt64_present_decimator_test` PASS exit 0).
  - Versioned patch: `patches/rt64/rt64-present-fps-decimator-and-counter.patch` (SHA256: `6d746f27ed1bd3132dcea8464adeb25dcfac4ad38502bf92f402ea6e3a19836c`), integrated into vendored `lib/rt64` against clean RT64 pin `6f1c2d99a4ea571c139f449c326fd176ba8f3496` (`tools/apply_runtime_patches.sh` retired; provenance tracked in `vendor/patches-applied/ORDER.tsv`). (Supersedes the older incremental `patches/rt64/rt64-present-rate-decimator.patch` for clean-pin apply; see [[../../plan/evidence/checkpoint-20261019-review-and-patch-replay]]).
  - Status: `IMPLEMENTED`, `STATICALLY_VERIFIED`.

### 4.3 Above-Native Rates & Motion Smoothness Limit
- When `targetRate > viOriginalRate`, RT64 generates interpolated frames (`displayFrames > 1`).
- **Palliative Workaround Interaction**: While Interpolation is OFF the CURRENT endpoint forces vertex positions to CURRENT (`tfPos`) and intermediate presents repeat the current vertex pose; with Interpolation ON (`endpointCurrent=0`) real interpolated poses are rendered, with the open glitch gate. Higher presentation rates (e.g. 100, 120, 144 FPS) present at the higher rate but cannot produce smooth motion for articulated meshes until the permanent interpolation fix is implemented.

### 4.4 Top-Left Present FPS Counter & Graphics Settings Toggle (REQ-GFX-08)
- **Purpose**: Provides an unobtrusive, on-screen display of the rate of frames actually presented to the display swapchain in the top-left corner.
- **Metric Fidelity**:
  - Counts only calls where `swapChain->present(...)` successfully returns `true` (`RT64::presentSuccessCount`).
  - Decimator skips and failed presents are excluded.
  - Does NOT display the user-selected target rate (`recomp_get_target_framerate`), does NOT display the guest VI rate (`viOriginalRate`), and does NOT represent game simulation or animation speed.
- **Pose Repeat Limitation under CURRENT Workaround** (Interpolation OFF):
  - Because the `tfPos` CURRENT workaround evaluates vertex positions at the current guest state, presentation rates above the native guest update rate repeat the same vertex pose across intermediate presents.
  - The counter reading near 60 FPS while visible character motion updates at ~30 Hz reflects actual swapchain presentation cadence, not high-framerate motion interpolation.
- **User Configuration & Directions**:
  - Setting key: `banjo::configkeys::graphics::show_fps_counter` in `include/banjo_config.h`.
  - UI entry: "Show Present FPS" checkbox in the Graphics options tab (`src/game/config.cpp:add_graphics_options`).
  - Default: `false` (OFF). Persisted automatically in `graphics.json`.
  - Usage directions: Navigate to the Graphics settings tab, check "Show Present FPS" to activate the overlay immediately; uncheck to deactivate. No restart required.
- **Overlay & Meter Architecture**:
  - RecompFrontend hook: `recompui::set_overlay_update_callback` (`ui_state.cpp`), executing in `draw_hook()` on the render thread. It is an always-on render-thread overlay, unaffected by `hide_all_contexts()` and capturing zero input.
  - Sliding meter: `tooie::PresentRateMeter` (`include/tooie_fps_meter.h`) computes rounded integer presents per second over a 1000 ms sliding window. Stalls > 3 s reset the window and display `"-- FPS"`.
  - Render style: 16 dp from top-left, dark backing (`rgba(0,0,0,160)`), text updated only when rounded integer changes.
- **Versioned Patches & Provenance**:
  - `patches/frontend/recompfrontend-overlay-update-callback.patch` (SHA256: `37266d0bd13d42c0b8f174c0118b6a9b2490784c5880f05e6ee37ab811cd48a7`): Integrated into vendored `lib/RecompFrontend` against pin `d0d90ba49f46f4896aaeda362056c21b1e342561` (`tools/apply_runtime_patches.sh` retired; provenance tracked in `vendor/patches-applied/ORDER.tsv`).
  - `patches/rt64/rt64-present-fps-decimator-and-counter.patch` (SHA256: `6d746f27ed1bd3132dcea8464adeb25dcfac4ad38502bf92f402ea6e3a19836c`): Integrated into vendored `lib/rt64` against pin `6f1c2d99a4ea571c139f449c326fd176ba8f3496`.
  - Historical provenance note: The older incremental patch `patches/rt64/rt64-present-success-counter.patch` is SUPERSEDED for clean-pin apply (fails with exit 1 on clean pin); the cumulative decimator-and-counter patch replaces it for clean clones. See [[../../plan/evidence/checkpoint-20261019-review-and-patch-replay]].
- **Verification & Status**:
  - Unit test: `tests/tooie_fps_meter_test.cpp` (`tooie_fps_meter_test` PASS exit 0).
  - Reverse patch application: `git apply --reverse --check` exit 0.
  - Status: `IMPLEMENTED`, `UNIT_TESTED`. (Separate from the unimplemented 8-preset selector widget `REQ-GFX-06`; in-game runtime QA is NOT performed).

### 4.5 Resolution 3x, 4x and Downsampling Scaling (REQ-GFX-09)
- **Resolution Enums**: `Original3x` (base 720p) and `Original4x` (base 960p) are defined in `ultramodern::renderer::Resolution` and registered in config serialization (`patches/runtime/n64modernruntime-resolution-3x-4x.patch`).
- **Downsample Multiplier Math**:
  - Base multiplier: `Original` = 1x, `Original2x` = 2x, `Original3x` = 3x, `Original4x` = 4x.
  - Downsampling multiplies on top: `resolutionMultiplier = base * ds`, `downsampleMultiplier = ds`.
  - For example, Original 3x with Downsampling 2x renders internally at 1440p and downsamples to 720p.
- **Auto Resolution**: Uses integer window VI scaling (not adaptive framerate); downsampling is disabled when Auto is active.
- **Verification**: Host unit tests `tests/tooie_menu_policy_test.cpp` and runtime screenshots (`general-disabled-fixed.png`, `graphics-16x9-4x.png`, `intro-4x-30fps.png`) in [[../../plan/evidence/p08-settings-functionality-20261019]].

### 4.6 Aspect Ratio Window Auto-Resize (REQ-GFX-10)
- **User Confirmed Preference**: Selecting `Original (4:3)` or `Expand (16:9)` in the Aspect Ratio option automatically resizes the application window in windowed mode while retaining current height.
- **Thread Boundary Architecture**:
  - UI callbacks cannot invoke SDL functions directly without risking deadlocks or race conditions.
  - Callbacks queue the target aspect ratio into an atomic slot (`tooie_menu::request_window_aspect`).
  - Consumed on the SDL event thread during `update_gfx` (`src/main/main.cpp`).
  - Derives dimensions via `tooie_menu::aspect_window_size`, accounting for window decorations (`SDL_GetWindowBordersSize`), and clamps within display usable bounds (`SDL_GetDisplayUsableBounds`).
  - Ignored when in Fullscreen or Borderless mode.
- **Verification**: Validated by `tests/tooie_menu_policy_test.cpp` PASS exit 0 and runtime screenshots (`graphics-4x3.png`, `graphics-16x9-4x.png`, `fullscreen-original.png`).

### 4.7 Remaining Gates
1. Implement the 8-preset user selector widget in RecompFrontend UI (`REQ-GFX-06`).
2. Perform runtime QA to verify presentation intervals against pose-change intervals, confirming audio and guest timing stability without clock distortion. (Runtime QA not performed; no actual in-game FPS measurements or speed guarantees claimed).
3. Perform runtime visual QA of the top-left FPS counter overlay across gameplay maps and menus.

---

## 4.8 Interpolated-Frame PNG Burst (F8 hotkey REMOVED 2026-10-21)

- **User-disabled**: the F8 hotkey (SDL event watch, `tooie::register_interp_burst`, `src/game/tooie_interp_burst.cpp`) was removed at user request; nothing requests a burst now, there is no replacement hotkey or capture UI, and `TooieRecompiled` registers no capture key. The RT64 capture infrastructure (`rt64_interp_burst.h`, `rt64-p09-interp-burst-capture.patch`) and its host test `p09_interp_burst_test` are retained as dormant diagnostics; `include/tooie_interp_burst.h` keeps only the pure `pictures_dir_from` helper.
- Historical (still true when the hotkey existed): one bounded burst of 20 numbered `PRE_VI_RESOLVED_COLOR_TARGET` PNGs with manifest/status, only for frame-level generated, matched, intermediate-weight frames without forced CURRENT; readbacks perturb timing. Evidence: [[../../plan/evidence/p09-interpolated-frame-burst-20261020]]. Removal proof (no artifact after F8): [[../../plan/evidence/p09-menu-interpolation-toggle-20261021]].

## 5. Architectural Causal Knowledge & Refuted Hypotheses

Investigation across P09 evidence documents has established the following ground truth:
1. **Proven Root Causes**:
   - `UNTAGGED_WORLD_GREEDY_TRANSFORM_MATCH_WRONG_PAIRING` (`PROVEN_SOURCE` in `lib/rt64/src/hle/rt64_game_frame.cpp`): Tooie does not tag matrices with gEX (`G_EX_ID_AUTO`). RT64 origin-dominated greedy cost matching in `GameFrame::matchScene` cannot distinguish bones sharing geometric pivots (e.g., Kazooie crest feathers) or instanced mesh pieces. Wrong pairs poison inherited velocity for subsequent frames in a feedback cascade, interpolating bones between distinct spatial endpoints and tearing geometry into elongated rods. The ordered transform matcher (`lib/rt64/src/hle/rt64_p09_ordered_match.h`, controlled via `TOOIE_TEST_P09_ORDERED_MATCH`) resolves this via submission-order pairing per draw-call hash.
   - `NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`: Independent matrix interpolation across multi-transform meshes shears articulated attachments (feathers, snout).
   - `MECHANISM_A_COSINE_AMPLIFICATION_HOLDS_TRANSLATION` (`PROVEN_SOURCE` in `rt64_rigid_body.cpp:30-34`): `curVelMag /= dotCurVel` executes before `VelocityTolerance=5`. When directional changes reach $\ge 90^\circ$, `dotCurVel` clamps to $10^{-6}$, artificially amplifying velocity and disabling `lerpTranslation`. The bone holds translation at `CURRENT` while rotation slerps, tearing joint triangles跨-WORLD into elongated rods ("rods"). Fixed in owning layer via `lerpTranslationSpeedOnly` and world interpolation guard.
   - `PLUME_BURST_TEXTURE_TO_BUFFER_COPY_MISSING` (`PROVEN_SOURCE` in `patches/rt64/rt64-plume-texture-to-buffer-copy.patch`): Upstream `plume` lacked texture->buffer branch in `copyTextureRegion`, crashing burst readback on clean pin. Resolved via versioned patch.
   - `ONG2_TEARDOWN_RACE` (`PROVEN_SOURCE` in `librecomp/src/recomp.cpp`): Guest OSThreads continue overlay unload during static destructor execution in `exit()`, causing race condition crash on shutdown. Harness mitigation applied; product race remains open.
2. **Heuristics & Boundaries**:
   - `MECHANISM_B_ELEMENTWISE_CLOSURE_GUARD`: `HYPOTHESIS / CLASS-LEVEL HEURISTIC`. Convex elementwise lerp prevents joint edges from stretching $>1.25\times\text{refLength}+1.0$; acts as a class-level boundary, not an anatomical constraint solver.
   - `TECHNICAL_GATE_STATUS`: `passed: false` (`technical-gate.json`, Claude Haiku 5.5, 7 blockers B1-B7: circular stretch metric, scope mislabel with guard inert under default CURRENT/30, lack of non-player GPU pair, dirty core crash binding, post-plume unit tests unlogged, ~14k dirty RT64 lines distinction, and absent 60 FPS USER_OBSERVED gameplay check).
   - `PRODUCTION_INVARIANT`: Default presentation remains strictly CURRENT/30 (`TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1`, `tooie_menu::kForcedRefreshRate = 30`). P09 remains `PENDING / NOT_FORMALLY_OPEN`.
3. **Refuted Hypotheses**:
   - *Vertex Geometry Displacement Match Cost*: `REFUTED` as transform matcher cost (small 3-4 vertex bones match incorrectly across distinct skeleton parts).
   - *RDRAM Vertex Buffer Address Transform Identity*: `REFUTED` as bone identity (buffers are shared across multiple matrix instances).
   - *Hierarchy Interpolation*: `REFUTED` (live world-space JOINs preserve attachments).
   - *World-Matrix Non-Rigidity*: `REFUTED` as dominant cause (rigidity decomposes cleanly 123630/0).
   - *Cross-Stream Temporal Cohort Mismatch*: `REFUTED` (pathologies occur on cohort-aligned workloads).
   - *Bound-Sprite View Cull*: `REFUTED` (`func_800CB8F0` is occupancy query, `draw_fn=NONE`).
   - *Type-6 Static Vis Cull*: `REFUTED` (`func_800EFFB4` is Euclidean distance, not aspect-sensitive).
