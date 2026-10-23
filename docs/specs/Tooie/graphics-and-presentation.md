---
title: "TooieRecomp Spec — Graphics, Presentation and High-Framerate Interpolation"
description: "Technical specification for RT64 integration, widescreen culling hooks, projection policies, host endpoint-CURRENT fallback, transform interpolation guards, and presentation controls."
status: current
area: "specs/Tooie"
updated: 2026-10-23
tags: [specs, tooierecomp, graphics, rt64, widescreen, interpolation, tfpos, fps, resolution, aspect-ratio]
---

# Graphics, Presentation and High-Framerate Interpolation Specification

## 1. Subsystem Overview

TooieRecomp renders original Nintendo 64 display lists through `RT64`, translating F3DEX2 commands to modern Vulkan 1.2+ (Linux) or Direct3D 12 (Windows default) pipelines while injecting native transforms to support widescreen viewports and high-framerate presentation.

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
|  - Native 16:9 View Volume: TOOIE_TEST_WS_NATIVE_PLANES (production default ON) |
|  - Map Geometry: func_800CAA24 (H-plane n_cam_x /= k, fallback/attract)         |
|  - Object Side-Plane: func_800CACEC (Actor frustum expand, fallback/attract)   |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                      RT64 Workload Queue & Transform Processor                  |
|  - Matrix matching across successive workloads                                  |
|  - Target Rate vs viOriginalRate evaluation (pacing constraint)                 |
|  - Target Rate Policy: Interpolation OFF = Manual 30; ON = user rate (min 60)   |
|  - P09 Guards: camera cut, segment/joint coherence, spike guard, speed gate     |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                     RSPProcess Compute Shader (RSPProcessCS.hlsl)               |
|  - endpoint-CURRENT via RT64::endpointCurrentActive() (Interpolation OFF = on)  |
|  - tfPos bypassed to CURRENT vertex positions; fog derives from tfPos.z/w       |
|  - Interpolation option ON = off; explicit env value overrides                  |
+---------------------------------------------------------------------------------+
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Status |
| :--- | :--- | :--- | :--- |
| **REQ-GFX-01** | Aspect-Aware Map Culling | `func_800CAA24` scales horizontal culling plane (`n_cam_x /= k`). | `IMPLEMENTED` |
| **REQ-GFX-02** | Aspect-Aware Object Culling | `func_800CACEC` expands culling planes during allowlisted draw passes (`func_800B97A4` kind 3, `func_8011447C` kind 2). | `IMPLEMENTED` |
| **REQ-GFX-03** | Cutscene Aspect Policy | Preconditions projection matrix via `compensation_factor`. | `IMPLEMENTED` |
| **REQ-GFX-04** | tfPos CURRENT Fallback | Active whenever Interpolation is OFF (default); host-controlled via `RT64::requestHostEndpointCurrent`. | `IMPLEMENTED` |
| **REQ-GFX-05** | Diagnostic Env Override | Explicit `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` overrides the option. | `IMPLEMENTED` |
| **REQ-GFX-06** | Interpolation Graphics Option | Persisted boolean option in `graphics.json`. OFF locks to Manual 30; ON unlocks user rate (min 60). | `IMPLEMENTED` |
| **REQ-GFX-07** | Below-Native Presentation Decimation | Present-only decimation preserving guest updates, audio, and ACK signals. | `IMPLEMENTED` |
| **REQ-GFX-08** | Top-Left Present FPS Counter | Visible present FPS overlay toggle in Graphics menu; measures swapchain presents. | `IMPLEMENTED` |
| **REQ-GFX-09** | Resolution 3x & 4x Scaling | Adds `Original3x` (720p) and `Original4x` (960p) base options with multiplicative downsampling. | `IMPLEMENTED` |
| **REQ-GFX-10** | Aspect Ratio Window Auto-Resize | Original 4:3 and Expand 16:9 auto-resize host window dimensions in windowed mode. | `IMPLEMENTED` |
| **REQ-GFX-11** | Intro Cinematic Walls | 16:9 projection compensation implemented; edge geometry and ultrawide remain open. | `PARTIAL` |

---

## 3. Interface & Mathematical Contracts

### 3.1 Aspect-Aware Frustum Culling
1. **Native View Volume Widescreen Planes (`TOOIE_TEST_WS_NATIVE_PLANES`)**:
   - Production mechanism (default `1` / ON): The volume aspect ratio field at `0x8012D500+0xE8` is temporarily widened while horizontal boundary vector generators (`0x800CA1B0`, `0x800CA218`, `0x800CA2A8`, registered via `add_loaded_function` in `boot_hw_stubs.cpp`) execute:
     $$\text{aspect}_{\text{temp}} = \text{aspect} \times \left(\frac{\text{target\_aspect}}{\text{source\_aspect}}\right) \times \text{lateral\_margin}$$
     and restored immediately afterwards.
   - Projection matrix generation at `0x800CB124` remains at base 4:3 so RT64 handles widescreen projection cleanly.
   - Native game culling tests evaluate against the widened 16:9 planes.
   - When native planes are active, the per-test expansion factor $k$ in `func_800CAA24` and `func_800CACEC` collapses to $1.0$ in `tooie_map_vis_k_for_volume` (`src/game/tooie_ws_frustum.h`) to prevent double-widening.
   - Attract Demo Mode: In attract demos (`0x80127632 == 5`), native planes are deactivated and per-test expansions ($k > 1$, margin 1.15, radius 1.25) become active.
   - Kill switch: Setting `TOOIE_TEST_WS_NATIVE_PLANES=0` disables native planes and falls back to isolated per-test expansions.
   - Verification: Host test `tests/p08_ws_frustum_test.cpp`.
2. **Per-Test Map Geometry Culling (`func_800CAA24`)**:
   - Scales the horizontal camera-space plane normal: $n_{\text{cam}, x}' = n_{\text{cam}, x} / k$.
   - The wrapper `tooie_wrap_func_800CAA24` only executes the evaluation when `orig == 0` (only promotes rejections to acceptances).
   - Controlled via `TOOIE_TEST_MAP_VIS_ASPECT_AWARE` (default ON; value starting with `'0'` disables).
3. **Per-Test Dynamic Object Culling (`func_800CACEC`)**:
   - Expands horizontal bounding planes for dynamic actors during allowlisted pure draw passes (`func_800B97A4` kind 3, `func_8011447C` kind 2).
   - Spatial occupancy and fog callers (`0x8011458C`, `chlightfader`) retain unexpanded culling.
   - Controlled via `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE` (default ON; value starting with `'0'` disables).
4. **Widescreen Margin and Radius Controls**:
   - `TOOIE_TEST_WS_DRAW_MARGIN`: Lateral margin scaling factor applied to horizontal boundary vector opening angles (default `1.15`, clamp `[1.0, 2.0]`).
   - `TOOIE_TEST_WS_DRAW_RADIUS_SCALE`: Radius multiplier applied to bounding sphere tests during manual per-test expansions (default `1.25`, clamp `[1.0, 2.0]`). Inactive when native planes are active.
5. **Experimental & Diagnostic Cull Overrides (REFUTED as production fixes)**:
   - `TOOIE_TEST_WS_MODEL_DRAW_ASPECT`: Experimental forced draw pass for `func_800DE498`. Only enables if explicitly set to `"1"` AND `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE != 0`. `REFUTED` as a fix (causes actor freeze at screen edges and drawing through walls; superseded by native planes).
   - `TOOIE_TEST_WS_GEO_SPHERE_ASPECT`: Experimental draw-only walk of GeoCmd type 14 subtrees. Only enables if explicitly set to `"1"`.
   - `TOOIE_DIAG_WS_CULL_CENSUS=<path>`: Diagnostic census file logger.

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
Validated via `tests/tintro_projection_policy_test.cpp`.

### 3.3 Shader-Local CURRENT `tfPos` Fallback & Host Switch
```cpp
namespace RT64 {
    void requestHostEndpointCurrent(bool endpointCurrent); // config callback
    void latchHostEndpointCurrent();                       // render thread boundary
    bool endpointCurrentActive();                          // env override > latched host request > off
}
```
- **Owner**: `install_interpolation_policy` (`include/tooie_menu_policy.h`) receives the persisted `interpolation` setting and invokes `RT64::requestHostEndpointCurrent(!enabled)`.
- **Inert Guards**: All P09 transform interpolation guards are completely inert when `endpointCurrentActive()` is true (which is the default state when Interpolation is OFF).
- **Environment Override**: `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` explicit non-empty value (first char `!= '0'`) forces CURRENT; explicit `"0"` enables interpolation. Unset or empty defers to the menu option.
- **Forced Rate Flag**: `TOOIE_TEST_INTERPOLATION_FPS` accepts `60` or `120` to force Interpolation ON at that rate.
- **Interpolation Matching & Guard Controls (P09)**:
  - `TOOIE_TEST_P09_ORDERED_MATCH`: Hash-ordered transform pairing for untagged (`G_EX_ID_AUTO`) WORLD transforms. Default ON; `"0"` reverts to greedy matching.
  - `TOOIE_TEST_P09_ALIGN_MATCH`: Sequence alignment with gaps via dynamic programming for asymmetric hash groups. Default ON; `"0"` disables.
  - `TOOIE_TEST_P09_CAMERA_CUT`: Geometric camera cut detection for AUTO view matrices. Large angular/linear steps suppress camera interpolation for the frame. Default ON; `"0"` disables.
  - `TOOIE_TEST_P09_SEGMENT_COHERENCE`: Conditional release of translation interpolation for segmented model parts. Default ON; `"0"` disables.
  - `TOOIE_TEST_P09_JOINT_COHERENCE`: Skeletal joint hierarchy inference based on relative pivot invariance. Binds child origin to parent coordinate space. Default ON; `"0"` disables.
  - `TOOIE_TEST_P09_SPIKE_GUARD`: Connected-component geometry deformation checks uniting mesh triangles and inferred
    joints. Renders flagged objects with elementwise matrix lerp fallback first; snaps partition to CURRENT only if
    edges remain stretched. Default ON; `"0"` disables.
  - `TOOIE_TEST_P09_SPIKE_ELEMENTWISE`: Controls elementwise matrix lerp fallback on spike-flagged objects. Default ON;
    `"0"` snaps to CURRENT only.
  - `TOOIE_TEST_P09_TRANSLATION_COHERENCE`: Linear velocity coherence gate. Default ON; `"0"` disables.
  - `TOOIE_TEST_P09_GATE_SPEED_ONLY`: Controls whether linear velocity gate clamps translation speed only. Default ON; `"0"` disables.
  - `TOOIE_TEST_P09_CLOSURE_GUARD`: Convex elementwise matrix lerp limiting. Default OFF (`0`); only `"1"` enables.
  - `TOOIE_TEST_P09_SELECTIVE_ELEMENTWISE_WORLD` / `MULTI_ONLY_ELEMENTWISE_WORLD`: Selective closure guards. Default OFF.
- **Diagnostic Controls**:
  - `TOOIE_DIAG_P09_CAMERA_CUT` / `TOOIE_DIAG_P09_JOINT`: Diagnostic logging flags. Note: parsed via `getenv != nullptr` (any value turns them ON).
  - `TOOIE_DIAG_P09_ROD_CLASSIFY=<path>`: Classifies stretched triangles after guard processing and logs connected
    object metrics (`OBJ` rows). Read-only.
  - `TOOIE_DIAG_P09_PAIR_AUDIT=<path>`: Outputs pairwise transform correspondence evaluations.
- **Compute Shader Operation (`RSPProcessCS.hlsl`)**:
  - `tfPos` evaluates directly at CURRENT vertex positions:
    `tfPos = mul(mul(endpointViewProj[...], endpointWorld[...]), endpointPos);`
  - Fog calculation derives from `tfPos` (`max(tfPos.z, 0) / tfPos.w`), ensuring depth matches mesh geometry without lag.
- **Verification**: `tests/rt64_host_endpoint_current_test.cpp`, `tests/p09_ordered_match_test.cpp`, `tests/p09_world_interpolation_guard_test.cpp`.

---

## 4. Framerate Control & Presentation Decimation

### 4.1 Framerate Policy
- **Framerate Policy Enforcement**:
  - Interpolation OFF (default): Locked to Manual 30 (`kForcedRefreshRate = 30`). Framerate UI controls disabled with label "Locked to 30 FPS".
  - Interpolation ON: Framerate controls enabled. Minimum framerate coerced to 60 FPS (or user-configured rate).
  - Dedicated 8-preset selector widget is not implemented.

### 4.2 Below-Native Presentation Decimation (REQ-GFX-07)
- **Problem**: When `targetRate < viOriginalRate` (e.g. 20 or 30 FPS target on a 60 Hz VI scene), RT64 historically emitted one presentation per native workload without pacing.
- **Implementation (`rt64_present_decimator.h` & `rt64_present_queue.cpp`)**:
  - `PresentDecimator` integer accumulator (`ticks += targetRate`, `ticks >= originalRate`) computes exact average presentation rates with zero float drift.
  - Active condition: `manualRate && !paused && targetRate > 0 && viOriginalRate > 0 && targetRate < viOriginalRate`.
  - Skipped presentations route through `skipInterpolation()`, preserving all guest updates, audio processing, and VI synchronization signals.
  - Thread safety: Configuration parameters accessed under `configurationMutex`.
- **Verification**: `tests/rt64_present_decimator_test.cpp`.

### 4.3 Top-Left Present FPS Counter (REQ-GFX-08)
- **Metric**: Counts swapchain presents that successfully return `true` (`RT64::presentSuccessCount`). Excludes skipped presents and failed frames. Does NOT represent guest logic speed.
- **Under CURRENT Fallback**: Intermediate presents repeat the current vertex pose. The counter reflects display swapchain cadence, not high-framerate motion.
- **Configuration**:
  - Setting: `show_fps_counter` in `graphics.json`.
  - UI entry: "Show Present FPS" checkbox in Graphics options (`src/game/config.cpp`). Default `false` (OFF).
  - Overlay: Render-thread callback via `recompui::set_overlay_update_callback`.
  - Meter: `tooie::PresentRateMeter` computes integer presents per second over a 1000 ms sliding window.
- **Verification**: `tests/tooie_fps_meter_test.cpp`.

### 4.4 Resolution Scaling & Downsampling (REQ-GFX-09)
- `Original3x` (base 720p) and `Original4x` (base 960p) supported in `ultramodern::renderer::Resolution`.
- Multiplicative downsampling: `resolutionMultiplier = base * ds`, `downsampleMultiplier = ds`.
- Patch: `vendor/patches-applied/runtime/n64modernruntime-resolution-3x-4x.patch`.

### 4.5 Aspect Ratio Window Auto-Resize (REQ-GFX-10)
- In windowed mode, selecting `Original (4:3)` or `Expand (16:9)` auto-resizes the host window while keeping height.
- UI queues target aspect ratio via `tooie_menu::request_window_aspect`, processed on the SDL event thread during `update_gfx`.
- Dimensions derived via `tooie_menu::aspect_window_size`, accounting for window borders (`SDL_GetWindowBordersSize`) and display bounds (`SDL_GetDisplayUsableBounds`).
- Verification: `tests/tooie_menu_policy_test.cpp`.

### 4.6 Interpolated-Frame PNG Burst Diagnostic
- The F8 capture hotkey and `src/game/tooie_interp_burst.cpp` were removed.
- Dormant diagnostic infrastructure remains in `rt64_interp_burst.h` and helper `pictures_dir_from` in `include/tooie_interp_burst.h`.
- Host test: `tests/p09_interp_burst_test.cpp`.

---

## 5. Architectural References

Detailed reverse-engineering models and transform interpolation mechanisms are documented in:
- **[Transform Interpolation System](../../reverse-engineering/systems/transform-interpolation.md)**: Current RT64 interpolation pipeline, pairing, P09 guards, and refuted hypotheses.
- **[Operational Handoff](../../HANDOFF.md)**: Current rendering guardrails and active visual residuals.