---
title: "TooieRecomp Spec — Graphics, Presentation and High-Framerate Interpolation"
description: "Technical specification for RT64 integration, widescreen culling hooks, projection policies, host endpoint-CURRENT fallback, transform interpolation guards, and presentation controls."
status: current
area: "specs/Tooie"
updated: 2026-11-01
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
| **REQ-GFX-12** | Object Distance | Persisted Original/2x/4x/Unlimited policy coordinating cell selection, models, sprites and actor gates; Windows user-validated cases, broader map/performance coverage unverified. | `EXPERIMENTAL` |

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

### 3.1.1 Object distance

- Graphics registers `object_distance` with values `Original`, `Double`, `Quadruple`, `Unlimited`, displayed as
  Original/2x/4x/Unlimited. Default/missing/invalid values select Original. Load and Apply atomically publish the
  applied enum; temporary edits and Cancel do not change the running policy. No restart is required.
- Enhancement applies only in mode `3`, excluding native multiplayer submode `0x1B`. Other modes, including
  recorded attract demos, retain native distances. Original passes through the existing integration.
- A call-local scope around `func_800DE498` preserves physical distance. At its `func_800E42C0` query, the final
  per-draw limit is extended; the getter's return, projection/FOV and world positions remain native. The default
  size-dependent cutoff is resolved as `min(default * view_scale, max(4000, size_limit * view_scale))`, accounting
  for the native 4000-unit fast path without introducing a jump inside the extended range. Custom positive limits
  are multiplied directly; zero/invalid limits keep native behavior. Unlimited uses a large finite cutoff.
- Native model distance alpha uses the original 700-unit transition at the extended limit, multiplying existing
  scripted transparency. There is no appearance cache, extra temporal ramp or reset caused by an omitted draw.
  Unlimited puts the radial fade boundary beyond ordinary map coordinates; scripted fades remain native.
- Player model submissions (including their attached model calls) and draws with native frustum testing disabled
  keep their native cutoff/alpha. Camera planes, native callbacks and renderer reset remain active.
  No model limit persists beyond its native per-draw reset.
- Spatial collection `func_800D3104` extends the camera-cell search and `func_800D2F44`'s radial cell test together.
  Only the exact native-stack invocations of the distance scalar receive 2x/4x; Unlimited uses a finite scalar
  of 32768 so the native bounds cover the map's existing grid. Grid extents, cell eligibility and AABB/frustum
  checks stay native. These scopes do not mutate viewport FOV, projection, cell size or world positions.
- World-sprite preparation `func_800E1C00` temporarily extends its input limit and enables its native 500-unit
  depth fade. Its default-limit projected-size shortcut is bypassed for enhanced world sprites, avoiding another
  cutoff (including when a multiplied limit equals the default sentinel). Native depth, negative-depth rejection,
  callbacks and prepared alpha/position remain native. Input limit/flags are restored after preparation. Player
  draw scopes, global UI viewport 4, other modes and nonpositive limits pass through unchanged.
- The native actor scheduling gate `func_8010038C` retains activity flags and initialization side effects. Its
  exact `func_800EFFB4` invocation receives the multiplied float radius, or `100000000` for Unlimited. This
  finite radius exceeds normal map-coordinate spans and avoids infinities or 16-bit radius overflow.
- `func_80100230` retains its native resource-eviction decisions, including zero-radius special semantics;
  only its matching `func_800EEB40` result is divided by the squared multiplier, or set to zero for Unlimited.
  The gate stack and actor-position pointer must match; unrelated distance/attack/interaction queries pass through.
- Extended actor scheduling/retention uses the closer live player's point in experimental story split-screen.
  Guest scratch storage is scoped and stack restored; no global player-focus mutation is used. The existing enemy
  target integration remains in the scheduling chain.
- Camera planes, coordinate-safety rejection, sector visibility, map geometry, fog, save/collection state and
  class-specific conditions are preserved. Unlimited removes the **covered radial limits**, not every possible
  reason for hiding an entity. Additional actor updates can advance world state earlier; this is an enhancement,
  not a promise of original simulation parity. Universal class/map coverage and performance are unverified.
- Placed map props can use model or sprite paths without an actor record. Their spatial selection and renderer
  limits are covered by the same policy. Additional class/map/performance coverage remains unverified;
  this does not bypass every reason an entity can be hidden. See
  [[reverse-engineering/systems/model-renderer#Spatial collection and sprite paths (`PROVEN` static code)]].
- Implementation: `src/game/tooie_object_distance.cpp` and `tooie_object_distance_config.cpp`; no generated C
  or vendored dependency edits. Regression: `tooie_object_distance_test`, using native functions and real Config.

### Widescreen framebuffer bounds

RT64 retains the vertical extent of submitted perspective triangles wholly outside the native horizontal
scissor when output is configured as Expand or Manual above 4:3. Their bounds use the native scissor's
horizontal span for framebuffer bookkeeping, then intersect the original vertical scissor normally.
This prevents source-width rejection from omitting rows needed by the expanded view when framebuffer
dimensions and write regions are inferred. Color and depth-update bounds follow the same policy.

The policy changes metadata only: face indices, vertex transforms, actual draw-call scissors, viewport,
depth comparison and native game visibility predicates remain unchanged. Original, Manual at/below 4:3,
orthographic draws and vertically disjoint geometry retain the existing path. Expand may conservatively
reserve extra rows even in a narrow window; actual raster clipping remains active.
`TOOIE_TEST_WS_DRAW_BOUNDS=0` restores the previous bounds bookkeeping at process startup.
Patch: `vendor/patches-local/rt64-widescreen-draw-bounds.patch`.

### Projection reload command budget

`func_800E4640` emits exactly one eight-byte command, matching native callers' allocation contracts.
With split P/V active, it calls a frame-local display list containing P LOAD, V MUL and ENDDL, allocated
as one additional aligned 64-byte block in the same dynamic matrix arena. The initial projection submission
is unchanged. The native segment-8 path remains one display-list call, and clearing the projection cache
also invalidates the saved reload list. The combined-matrix fallback remains available before initialization.

Native `func_800BC948` builds two reload lists in each 48-byte bank. The integration must not emit two
inline matrix commands per reload: that writes 56 bytes and overlaps the next bank. The compact reload
uses 40 bytes. `tooie_projection_reload_test` executes the native builder with production hooks and checks
both banks, surrounding bytes, P/V ordering, segment 8 and cache reset, without starting the game.
This memory-safety contract does not establish the cause of a particular widescreen visual defect.

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
The exception requires **both** mode 3 and T_INTRO map 395. Ordinary story cinematics, including ability explanations,
keep their guest projection and the normal RT64 aspect adjustment; they receive no additional horizontal compensation.
The target remains dynamic: `calculate_target_aspect_ratio` reads the current window dimensions for Expand, with a
4:3 lower bound and invalid-size fallback. Original keeps 4:3. No fixed 16:9 target is introduced, and resizing requires
no cached policy reset. The existing T_INTRO formula above is preserved.

`tests/tintro_projection_policy_test.cpp` compiles production `apply` and the window-aspect calculation, simulating
only window/config services. It covers ordinary story/intro contexts, Original/Expand, 4:3, 5:4, portrait, 16:10, 16:9,
ultrawide, irregular and invalid sizes, plus scene/map transitions. This is a host regression, not rendered-game proof.
Investigation and validation limits: [[reverse-engineering/systems/view-volume#Cinematic projection compensation scope]].

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

### 3.4 RT64 CPU Cost With Many Draw Calls

Scenes with about a thousand small draw calls (Glitter Gulch Mine crystals, 990 draws of ~4 triangles) dropped to 9-12
FPS on Windows D3D12 with the GPU mostly idle. Two RT64 stages were the cost; both are now behavior-identical
optimizations carried as local vendor patches:

- **Matching check sets** (`GameFrame::matchScene`, `rt64-perf-match-checkset-keys.patch`): upstream visited every
  (current, previous) call pair sharing a call hash. `hashFromCall` hashes render state and triangle count, not geometry
  or texture, so one group reached ~95,000 call pairs for 300-640 unique transform pairs. The sets are now built from
  the distinct per-call keys of each hash group: transforms by (workload, first world matrix, matrix count), tiles by
  (tile count, tile position, tile matching, TMEM hash), look-at by resolved look-at index. The resulting
  `transformCheckSet`, `tileCheckSet` and `lookAtCheckSet` are identical to the all-pairs loop.
- **TMEM texture hashing** (`TextureManager::uploadTexture`, `rt64-perf-tmem-hash-cache.patch`): the game reloads TMEM
  before nearly every draw, so `TMEMHasher::hash` ran ~900 times per frame (most with TLUT, which parses every byte)
  although the same contents recur. A cache keyed by a 128-bit XXH3 of the full 4096-byte TMEM plus every hash input
  field returns the earlier 64-bit hash; it holds at most 65,536 entries and is cleared when full.

Measured at the mine spot (D3D12, GTX 1650, 2026-10-30): check-set construction 12 ms average / 39 ms peak per scene to
1.8 ms; TMEM hashing 14 ms to 1.1 ms per frame; the scene went from 9-12 FPS to 60 FPS. Equivalence was checked at
runtime with a temporary verification build that ran the original code next to the new one: 0 mismatches for both
stages (the verification code is not kept). Upstream RT64 had no fix as of `d9817c6` (2026-10-23).

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

## Startup API selection

The vendored renderer includes TK64's DX11/OpenGL additions while retaining existing RT64 presentation/interpolation
changes. Windows selects a usable backend before the launcher, with supervised startup recovery. Requirements,
effective backend reporting and validation limits: [Launcher Startup and Backends](startup-and-backends.md).
