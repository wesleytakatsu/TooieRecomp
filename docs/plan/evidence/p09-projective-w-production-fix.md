---
title: "P09 projective-W production candidate"
description: "CLIP_LERP/NDC_LERP closed by envelope capture; JOIN/phase counters added; production shader unchanged."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-14
tags: [tooie-recomp, p09, interpolation, clip-w, near-clip, production]
---

# P09 projective-W production candidate

## CURRENT (2026-09-14) — P09_ARTICULATED_TEMPORAL_COHERENCE_ROOT

Superseded as the active interpolation hunt by
[[p09-articulated-temporal-coherence-root]]. CLIP_LERP does not beat
POLICY_A; envelope shows POLICY_A mostly valid. Production
`RSPProcessCS` still per-vertex CURRENT on W-sign. JOIN/phase
instrumentation lives in the same CPU binding diag.

## Prior (2026-09-13) — P09_SAFE_ENDPOINT_SCREENSPACE_INTERPOLATION_FORENSIC

Authoritative residual capture is **VALID** (`33020` triangles).
User visual: high-rate `GLITCH`; global CURRENT `PASS`; shader-local
CURRENT `PASS`; selective endpoint-W CURRENT `FAIL`; W-sign marker
`PASS`. `GPU_W_SIGN_BRANCH_EXECUTES: PROVEN`.

```text
ZERO_CROSS_IS_DOMINANT_RESIDUAL_CLASS: YES
TRIANGLE_CURRENT_SOLVES_DOMINANT_RESIDUAL: NO
NON_CROSSING_CLASS_HAS_VISUALLY_SIGNIFICANT_DEFORMATION: YES
POLICY_A_IMPROVES_CATASTROPHES_BUT_NOT_VISUALLY_CORRECT: YES
  LIVE max vs CURRENT 121.17; POLICY_A max 13.57; zero-cross max 6.89
  CROSS_0=32561 CROSS_1=142 CROSS_2=28 CROSS_3=289
```

Do **not** invent more W-crossing fallbacks. The remaining defect is
screen-space overshoot of interpolate-VP × interpolate-world even
when endpoint W is same-sign.

`TOOIE_DIAG_RSP_ENDPOINT_BINDING=1` now also records CPU-only:

- `POLICY_CLIP_LERP`: `tfPos = lerp(prevClip,curClip,w)` if
  `prevW*curW > 0`, else CURRENT. This **is** a valid `RSPProcessCS`
  `tfPos` (same divide). Not enabled in the shader.
- `POLICY_NDC_LERP`: lerp shader-comparable NDC XY when both
  endpoints have finite same-sign W, else CURRENT. Kept
  **CPU-only**; a well-defined reconstruction would be
  `W=lerp(prevW,curW,w)`, `tf=(ndc.x*W, -ndc.y*W, ndc.z*W, W)`
  matching `xyz/(w,-w,w)`. Not applied on GPU.
- LIVE NDC distance to the prev→cur segment (overshoot fractions).
- Edge ratio vs `max(prevEndpointEdge, curEndpointEdge)`.
- Rigid-coherence vs lerp of endpoint edge lengths.

Production `RSPProcessCS` unchanged. No GPU readback.

```text
CLIP_LERP_MAX_EDGE_RATIO_VS_CURRENT: NOT_CAPTURED
CLIP_LERP_P95: NOT_CAPTURED
CLIP_LERP_P99: NOT_CAPTURED
CLIP_LERP_GT_1_5: NOT_CAPTURED
CLIP_LERP_GT_2: NOT_CAPTURED
CLIP_LERP_MAX_EDGE_RATIO_VS_ENDPOINT_ENVELOPE: NOT_CAPTURED
NDC_LERP_MAX_EDGE_RATIO_VS_CURRENT: NOT_CAPTURED
NDC_LERP_P95: NOT_CAPTURED
NDC_LERP_P99: NOT_CAPTURED
NDC_LERP_GT_1_5: NOT_CAPTURED
NDC_LERP_GT_2: NOT_CAPTURED
NDC_LERP_MAX_EDGE_RATIO_VS_ENDPOINT_ENVELOPE: NOT_CAPTURED
ZERO_CROSS_SCREENSPACE_OVERSHOOT_CORRELATES_WITH_STRETCH: NOT_CAPTURED
CLIP_LERP_RIGID_COHERENCE: NOT_CAPTURED
NDC_LERP_RIGID_COHERENCE: NOT_CAPTURED
BEST_TEMPORAL_POSITION_POLICY: NONE
PRODUCTION_FIX_UPDATED: NO
```

Build (this session): `cmake --build build --target TooieRecompiled
window_display_test tintro_projection_policy_test` exit 0;
`window_display_test` 38/38; `tintro_projection_policy_test` 7/7;
Tooie and RT64 `git diff --check` PASS. Game not launched.

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled` (normal high-rate) and paste
`CLIP_LERP_*`, `NDC_LERP_*`, `ZERO_CROSS_LIVE_NDC_OVERSHOOT_*`,
`*_VS_ENDPOINT_ENVELOPE`, `*_RIGID_*`,
`BEST_TEMPORAL_POSITION_POLICY`.

## Prior (2026-09-13) — P09_RESIDUAL_TRIANGLE_DIAGNOSTIC_REPAIR

Authoritative capture had `SESSION_INTERMEDIATE_FEATHER_VERTEX_COUNT=63805`
and `SESSION_TARGET_DL_MASK=0x3fff` but `RESIDUAL_TRIANGLE_COUNT=0`.
That was **not** "no residual deformation". The CPU triangle
accumulator never accepted V4-visible faces.

```text
RESIDUAL_TRIANGLE_ACCUMULATOR_ENTRY_REACHED: NO (prior capture)
FIRST_DEAD_GATE: p09RspEndpointBindingNote
  rt64_rsp_processor.cpp
  1) anyFeather world-bit filter (V4 does not use this)
  2) residualTriangleCount gated on curOk && eC >= kP09EdgeFloor
  3) triangle walk required lastIntermediate before any skip counters
WHY_VERTEX_COUNTS_NONZERO_BUT_TRIANGLE_COUNT_ZERO: vertex loop
  walks drawData positions; residual required target-DL faceIndices
  AND feather world bit AND finite CURRENT edge floor. V4 counts
  every exact-target 0x62B face.
RESIDUAL_TRIANGLE_SOURCE: V4_REUSED
V4_VISIBLE_TRIANGLE_SELECTION_EQUIVALENT: YES
  (p09V4VisibleTargetIndex = p09V7ExactTargetIndex + V4 extra logicals;
   no anyFeather skip)
RESIDUAL_COUNTER_RESET_SCOPE: SESSION (g_p09RspEndpointBinding;
  last-* fields reset per process(), residual histograms are not)
RESIDUAL_SESSION_ACCUMULATION_CORRECT: YES
RESIDUAL_DIAGNOSTIC_VALID: NO until recapture (prior capture INVALID)
PRODUCTION_POLICY_CHANGED: NO
```

Repair is CPU diagnostic only. `RSPProcessCS` W-sign policy,
force-CURRENT, W-crossing marker, sidecar, and interpolation are
unchanged. Recapture:

`TOOIE_DIAG_RSP_ENDPOINT_BINDING=1 ./build/TooieRecompiled`

If intermediate feathers and mask `0x3fff` still yield
`RESIDUAL_TRIANGLE_COUNT=0`, status emits
`RESIDUAL_DIAGNOSTIC_VALID=NO`.

## Prior (2026-09-12) — P09_RESIDUAL_VISUAL_DEFORMATION_AFTER_POLICY_A

User visual (authoritative):

```text
NORMAL_HIGH_RATE: GLITCH
SELECTIVE_POLICY_A_VISUAL: FAIL
SHADER_LOCAL_ALL_CURRENT: PASS   (TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1)
GPU_W_SIGN_BRANCH_MARKER: A / PASS   (TOOIE_DIAG_RSP_MARK_W_CROSSING=1)
GPU_W_SIGN_BRANCH_EXECUTES: PROVEN
```

The marker `tfPos.x += 0.70 * tfPos.w` is only binary proof that
the branch runs on visible geometry. Do not treat apparent jump
size as a GPU measurement.

The remaining glitch is **not** required to be a 5×/50×/1000×
explosion. User: magnitude tracks apparent character size /
camera distance; if the character occupies ~1/10 of screen
height, the feather defect can extend another ~1/20 to ~1/10 of
the screen (~0.5×–1× apparent character size). That is obvious
to a human and far below `kP09ExplosionRatio = 5`.

### Task 6 — old max metric

```cpp
auto noteExplosion = [&] (float ratio, uint64_t &count, float &maxRatio) {
    if (ratio >= kP09ExplosionRatio) {  // 5.0
        count++;
        maxRatio = std::max(maxRatio, ratio);
        return true;
    }
    return false;
};
```

`POLICY_A_MAX_EXPANSION_RATIO` is updated **only** inside that
filter. With `POLICY_A_EXPLOSION_COUNT=0`, max stays `0` even if
every triangle is 1.5×–3× ALL_CURRENT.

```text
OLD_POLICY_A_MAX_METRIC_WAS_FILTERED: YES
OLD_POLICY_A_MAX_ONLY_UPDATED_ON_EXPLOSION: YES
POLICY_A_EXPLOSION_COUNT=0 DOES_NOT_PROVE POLICY_A_VISUALLY_CORRECT
```

The previous ratio was also vs `eBase = max(CURRENT, PREV)` edge,
not vs ALL_CURRENT.

### Tasks 1–5 — CPU residual distribution (instrumented, not recaptured)

Same default-off `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1` path now, for
every visible intermediate 0x62B feather triangle with finite
CURRENT:

- LIVE vs ALL_CURRENT
- POLICY_A vs ALL_CURRENT (`prevW*curW <= 0` → CURRENT else LIVE)
- POLICY_TRIANGLE_CURRENT (any vertex crosses → all three CURRENT)
- crossing-vertex class 0/1/2/3
- vertex NDC XY distance from ALL_CURRENT

Unfiltered percentiles and counts `>1.05 … >5`. Production shader
not changed. Triangle-consistent CURRENT is **counterfactual only**.

Visually-significant (CPU, not explosion-only): max edge ratio
`>= 1.50` **or** max vertex NDC distance `>= 0.10` (~1/20 of
screen if NDC span is 2). Near-equivalent to ALL_CURRENT: max
edge `<= 1.05` and max vertex NDC `< 0.02`.

```text
POLICY_A_MAX_EDGE_RATIO_VS_CURRENT: NOT_CAPTURED
POLICY_A_P95_EDGE_RATIO_VS_CURRENT: NOT_CAPTURED
POLICY_A_P99_EDGE_RATIO_VS_CURRENT: NOT_CAPTURED
POLICY_A_RATIO_GT_1_1_COUNT: NOT_CAPTURED
POLICY_A_RATIO_GT_1_2_COUNT: NOT_CAPTURED
POLICY_A_RATIO_GT_1_5_COUNT: NOT_CAPTURED
POLICY_A_RATIO_GT_2_COUNT: NOT_CAPTURED
MIXED_1_OR_2_CROSSING_TRIANGLES_EXIST: NOT_CAPTURED
MIXED_TRIANGLES_CORRELATE_WITH_RESIDUAL_STRETCH: NOT_CAPTURED
TRIANGLE_CURRENT_MAX_EDGE_RATIO_VS_CURRENT: NOT_CAPTURED
TRIANGLE_CURRENT_P99_EDGE_RATIO_VS_CURRENT: NOT_CAPTURED
TRIANGLE_CURRENT_MATERIALLY_REDUCES_RESIDUAL_DEFORMATION: NOT_CAPTURED
ZERO_CROSS_MAX_EDGE_RATIO: NOT_CAPTURED
ZERO_CROSS_P99_EDGE_RATIO: NOT_CAPTURED
NON_CROSSING_CLASS_HAS_VISUALLY_SIGNIFICANT_DEFORMATION: NOT_CAPTURED
CPU_POLICY_A_VISUALLY_EQUIVALENT_TO_ALL_CURRENT: NOT_CAPTURED
NEXT_PRODUCTION_FIX_CLASS: UNKNOWN
PRODUCTION_FIX_UPDATED: NO
```

Do not pick A/B/C until the recapture fills those keys.

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled` at normal high-rate (no force-CURRENT,
no W-crossing marker). Read
`/tmp/tooie-rsp-endpoint-binding-status.txt`.

## Prior (2026-09-12) — P09_HUMAN_OBVIOUS_GPU_BRANCH_MARKER

The first `TOOIE_DIAG_RSP_MARK_W_CROSSING=1` marker used
`tfPos.xy += (0.20,-0.12)*tfPos.w` (~10% of screen width plus a
small Y shift). That is **too subtle** for binary visual
validation.

Same default-off env, same branch gate, same CURRENT selection.
Only the marker magnitude/axis changed: after `tfPos = curClip`,

```text
tfPos.x += 0.70 * tfPos.w
```

NDC is `xyz / (w, -w, w)`, so `ndc.x` always increases by `0.70`
(~35% of screen width, mid of the 25%–50% target). Multiply by
clip.w, not `abs(w)`, so a negative W does not reverse the jump.
Bounded, deterministic, finite; no NaN/Inf; no huge clip values;
no extra plane; no GPU readback; no new env var.

```text
HUMAN_OBVIOUS_BRANCH_MARKER_IMPLEMENTED: YES
MARKER_EFFECT: crossing vertices jump sideways by about one third of the screen
EXPECTED_IF_BRANCH_FIRES: feathers (or other crossing parts) obviously jump/distort sideways vs the normal glitch
EXPECTED_IF_BRANCH_DOES_NOT_FIRE: same high-rate feather glitch as a normal run
USER_TEST_COMMAND: TOOIE_DIAG_RSP_MARK_W_CROSSING=1 ./build/TooieRecompiled
PRODUCTION_POLICY_CHANGED: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

## Prior (2026-09-12) — P09_SELECTIVE_GPU_BRANCH_EXECUTION_PROOF

Authoritative live CPU (user):

```text
SESSION_INTERMEDIATE_FEATHER_VERTEX_COUNT=68770
ENDPOINT_SIGN_CROSS_COUNT=926
SESSION_INTERMEDIATE_FEATHER_SIDECAR_CROSS_COUNT=926
SESSION_INTERMEDIATE_FEATHER_WITNESS_CROSS_COUNT=926
POLICY_A_EXPLOSION_COUNT=0
POLICY_A_MAX_EXPANSION_RATIO=0
POLICY_A_EXPLOSION_WITH_ENDPOINT_CROSS_COUNT=0
POLICY_A_EXPLOSION_WITH_SAME_SIGN_ENDPOINT_COUNT=0
```

Visual still: normal high-rate `GLITCH`;
`TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1` `PASS`.

### Task 1 — selective policy is default-on (`PROVEN_STATIC`)

`RSPProcessCS` always compiles and executes

```text
if (prevClipW * curClipW <= 0) tfPos = curClip
```

on a normal `./build/TooieRecompiled` run. No `#ifdef`, no
descriptor-valid flag, no `processProjections`/`processTransforms`
gate, no frame-map gate around that assignment. The only extra
runtime input is push-constant `forceEndpointCurrent`, set from
`TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` and **default 0**. That flag
ORs *into* CURRENT; it does not disable the sign test.

```text
SELECTIVE_POLICY_DEFAULT_ON: YES
SELECTIVE_POLICY_REQUIRES_DIAGNOSTIC_FLAG: NO
SELECTIVE_POLICY_RUNTIME_GATING: none around the W-sign assignment; dispatch only if vertexCount>0; forceEndpointCurrent default 0
FORCE_CURRENT_DIAGNOSTIC_BYPASSES_SELECTIVE_GATING: YES
SELECTIVE_EXECUTION_BUG_FOUND: NO
SELECTIVE_EXECUTION_FIX_IMPLEMENTED: NO
```

### Task 2 — control flow

NORMAL: live `tfPos = VP*W*(pos-vel*(1-curW))`; compute `curClip` /
`prevClipW` from endpoint t22/t23 × `pos`; if `force` (0) skip;
else if `prevW*curW<=0` `tfPos=curClip`; NDC `xyz/(w,-w,w)`; write
`dstPos`.

FORCE: same until the first if: `force!=0` → `tfPos=curClip` for
every vertex; W-sign not consulted.

`FIRST_CONTROL_FLOW_DIFFERENCE`: `forceEndpointCurrent` makes
CURRENT unconditional; production still requires the W-sign test.

### Task 3 — predicate arithmetic

GPU uses `prevClip.w` / `curClip.w` from
`mul(mul(endpointVP[i*2(+1)], endpointWorld[j*2(+1)]), float4(pos,1))`,
same order/indices/`endpointPos` (no vel) as CPU sidecar POLICY_A.
No `abs()`. Product `<= 0` is ordered (`OpFOrdLessThanEqual`).
Matching finite W cannot false-negative the CPU crossing set
(underflow yields extra *positives*). NaN fails both CPU and GPU.

```text
GPU_PREDICATE_FORMULA_MATCHES_CPU: YES
GPU_PREV_W_SOURCE_MATCHES_CPU: YES
GPU_CUR_W_SOURCE_MATCHES_CPU: YES
GPU_PREDICATE_CAN_FALSE_NEGATIVE_FOR_CPU_CROSSING: NO
```

GPU buffer contents vs CPU vectors remain not independently proven
without readback; FORCE PASS already shows GPU CURRENT clip is
visually usable.

### Task 4 — branch marker (default-off)

`TOOIE_DIAG_RSP_MARK_W_CROSSING=1` sets push-constant
`markWCrossing`. First marker: NDC `(+0.20, -0.12)` via
`tfPos.xy += (0.20,-0.12)*tfPos.w`. Too subtle; superseded by
the human-obvious `clip.x += 0.70 * clip.w` marker. Complementary
`TOOIE_DIAG_RSP_FORCE_NONCROSS_CURRENT` **not** implemented.

```text
BRANCH_MARKER_DIAGNOSTIC_IMPLEMENTED: YES
BRANCH_MARKER_TOO_SUBTLE: YES
USER_TEST_COMMAND: SUPERSEDED
PRODUCTION_POLICY_CHANGED: NO
GPU_READBACK_USED: NO
```

## Prior (2026-09-12) — P09_INTERMEDIATE_PROJECTIVE_COLLAPSE_POLICY

User live bind recapture (authoritative):

```text
SESSION_INTERMEDIATE_DISPATCH_COUNT=1660
SESSION_INTERMEDIATE_FEATHER_VERTEX_COUNT=51240
SESSION_INTERMEDIATE_FEATHER_SIDECAR_CROSS_COUNT=768
SESSION_INTERMEDIATE_FEATHER_WITNESS_CROSS_COUNT=768
INTERMEDIATE_WITNESS_CROSS_SIDECAR_NO_CROSS_COUNT=0
SESSION_CURRENT_FEATHER_SIDECAR_CROSS_COUNT=707
SESSION_TARGET_DL_MASK=0x3fff
SESSION_FEATHER_WORLD_MASK=0x1ff
SIDECAR_CONTENT_MATCHES_WITNESS: PROVEN
```

Visual:

```text
RSPPROCESS_TFPOS_CAUSAL: PROVEN
SHADER_LOCAL_CURRENT_VISUAL_RESULT: PASS
CURRENT_ENDPOINT_SIGN_POLICY_VISUAL_RESULT: FAIL
ENDPOINT_SIGN_CROSSING_ONLY_POLICY: VISUALLY INSUFFICIENT
```

`TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1` (`tfPos = CURRENT` in
`RSPProcessCS` only) removes the glitch. The production
`prevW*curW <= 0` branch does not.

Offline VERSION=4 dumped explosions (48/75, `kMaxAnomalies`): every
dumped triangle has **all three** vertices with `prevW*curW < 0`.
Replaying POLICY_A on those 48 (CURRENT clip when endpoints cross,
else live NDC, VERSION=4 `maxEdgeXY` / `kExplosionRatio=5`) yields
**0 explosions**, max ratio 1. The remaining 27 undumped explosions
are `EXPLOSION_WITH_W_SIGN_CHANGE` at triangle granularity (`any`
vertex), so mixed same-sign vertices are **not** proven absent.
That dump is a different session than the 1660-dispatch recapture.

Live POLICY_A/B/live-sign counts for the new session are **not** in
the previous status file. Default-off
`TOOIE_DIAG_RSP_ENDPOINT_BINDING=1` now also reconstructs, CPU-only:

- `prevClip` / `curClip` from sidecar endpoint VP×world × endpoint pos
- `liveClip` from the same interpolated `modViewProj` / `lerpWorld`
  and `pos - vel*(1-curWeight)` as `RSPProcessCS` before fallback
- vertex `normalizedLiveW` histograms split by endpoint sign
- triangle POLICY_A / live-sign / POLICY_B(T) explosion replay

No VERSION=5 temporal witness. Shader production condition
unchanged (`PRODUCTION_FIX_UPDATED: NO`) because live
`POLICY_A_EXPLOSION_COUNT` is still `NOT_CAPTURED`; inventing a
numeric threshold without that replay would violate the stop rule.

```text
POLICY_A_EXPLOSION_COUNT: NOT_CAPTURED (live); 0 on VERSION=4 dumped 48/75
POLICY_A_MAX_EXPANSION_RATIO: NOT_CAPTURED (live); 1.0 on dumped 48
SAME_SIGN_ENDPOINT_RESIDUAL_EXPLOSION_COUNT: NOT_CAPTURED
MIN_NORMALIZED_LIVE_W: NOT_CAPTURED (live); VERSION=4 session 0.00773333
MIN_NORMALIZED_LIVE_W_ON_POLICY_A_EXPLOSION: NOT_CAPTURED
MAX_NORMALIZED_LIVE_W_ON_POLICY_A_EXPLOSION: NOT_CAPTURED
LIVE_SIGN_POLICY_EXPLOSION_COUNT: NOT_CAPTURED
LIVE_SIGN_POLICY_MAX_RATIO: NOT_CAPTURED
MINIMUM_ZERO_EXPLOSION_THRESHOLD: NOT_CAPTURED
FALLBACK_PERCENT_AT_MINIMUM_THRESHOLD: NOT_CAPTURED
SELECTIVE_PRODUCTION_POLICY_FOUND: NO
PRODUCTION_FIX_UPDATED: NO
GPU_READBACK_USED: NO
```

## Prior — VERSION=4 candidate and bind proof

Authoritative VERSION=4 (user capture). No GPU readback. No new
diagnostic schema. `TOOIE_DIAG_FORCE_CURRENT_FRAME` remains
diagnostic-only.

```text
PROJECTIVE_W_SIGN_CROSSING_ROOT_STATUS: PROVEN
EXPLOSION_COUNT: 75
EXPLOSION_WITH_W_SIGN_CHANGE_COUNT: 75
PR23_SIMPLE: REFUTED
P_V_PROJECTIVE_GAUGE_DISCONTINUITY: REFUTED
PROJECTION_INTERPOLATION_ALONE: REFUTED
VIEW_DOMINANCE: 63/75 (CURRENT_VIEW_ONLY 75→12)
```

## Task 1 — RT64 near clipping (`PROVEN_STATIC`)

`gEXSetNearClipping` maps to `RSP::setNoN(!nearClipping)`. Raster
`depthClipEnabled = !NoN`. Tooie gameplay ucode is F3DEX2.NoN, so
`NoN=true` and near clipping is off. Banjo explicitly sets
`gEXSetNearClipping(FALSE)` (same: depth clamp, not homogeneous clip).

`RSPProcessCS` perspective-divides in compute
(`ndc = xyz / (w,-w,w)`) **before** raster. GPU/PS near-Z tests
cannot clip a triangle that already exploded in XY from `w≈0`.
Enabling `gEXSetNearClipping(TRUE)` would change original NoN
behavior and still would not clip W-sign streaks.

```text
TOOIE_GAMEPLAY_NEAR_CLIPPING_ENABLED: NO
RT64_EXISTING_NEAR_CLIP_HANDLES_W_SIGN_CROSSING: NO
NEAR_CLIPPING_IS_VALID_PRODUCTION_FIX: NO
```

`gEXSetViewMatrixFloat` is Banjo camera-translation tagging. VERSION=4
already refuted P/V gauge. It does not clip interpolated samples
whose clip.w crosses 0. Not ported.

## Production fix

Generic per-vertex rule in `RSPProcessCS`:

- Keep interpolated `viewProj * world * pos` for normal samples.
- If game-frame endpoints satisfy `prevClip.w * curClip.w <= 0`,
  replace **that vertex position** with the current endpoint
  (`curViewProj * curWorld * pos`).
- No model / bone / DL hardcodes.
- Original-frame `w=1` is a no-op (fallback equals current).

```text
PRODUCTION_FIX_CLASS: PER_VERTEX_CURRENT_ENDPOINT_ON_W_SIGN_CROSSING
PRODUCTION_FIX_IMPLEMENTED: YES
GLOBAL_CURRENT_STILL_DIAGNOSTIC_ONLY: YES
NORMAL_INTERPOLATION_PRESERVED_FOR_NON_CROSSING_CASES: YES
MODEL_SPECIFIC_HACK_USED: NO
NEW_DIAGNOSTIC_SCHEMA_ADDED: NO
```

## Failed visual (2026-09-12)

```text
PRODUCTION_FIX_VISUAL_RESULT: FAIL
FALLBACK_EXECUTION_LAYER: RSP_SHADER
PRODUCTION_PREV_CLIP_MATCHES_WITNESS_PREV_CLIP: NOT_LIVE_PROVEN
PRODUCTION_CUR_CLIP_MATCHES_WITNESS_CUR_CLIP: NOT_LIVE_PROVEN
EXPECTED_W_SIGN_CROSSING_VERTEX_COUNT: 1292
PRODUCTION_FALLBACK_PREDICTED_TRIGGER_COUNT: 1292
PRODUCTION_FALLBACK_EXPECTED_TO_FIRE: YES
RSP_SHADER_HAS_BOTH_TRUE_ENDPOINT_VIEWPROJ_MATRICES: YES (t22 sidecar; t17 interpolated)
RSP_SHADER_HAS_BOTH_TRUE_ENDPOINT_WORLD_MATRICES: YES (t23 sidecar; t18 interpolated)
PER_VERTEX_SIGN_TEST_CAN_BE_CORRECTLY_DONE_AT_CURRENT_SHADER_STAGE: YES (via sidecars only)
PER_VERTEX_FALLBACK_OFFLINE_EXPLOSION_COUNT: 0 (48/75 dumped; all 3 verts cross)
TRIANGLE_CURRENT_FALLBACK_OFFLINE_EXPLOSION_COUNT: 0
MAX_PER_VERTEX_FALLBACK_RATIO: 1.0
MAX_TRIANGLE_CURRENT_FALLBACK_RATIO: 1.0
PER_VERTEX_POLICY_MATHEMATICALLY_SUFFICIENT: YES
TRIANGLE_CONSISTENCY_REQUIRED: NO
SMALLEST_COHERENT_FALLBACK_GRANULARITY: VERTEX
GLOBAL_CURRENT_CPU_EXPLOSION_COUNT: 0
SIGN_CROSSING_SELECTIVE_POLICY_COVERS_ALL_GLOBAL_CURRENT_FIXED_CASES: YES (on exploding tris)
REVISED_PRODUCTION_FIX_IMPLEMENTED: NO
```

t17/t18 remain interpolated `modViewProj` / `lerpWorld`. CPU
`endpointViewProjTransforms` / `endpointWorldTransforms` are true
game-frame endpoints (`prev=slot*2`, `cur=slot*2+1`) from the same
sources as VERSION=4 `makeQPrev`/`makeWPrev`. SPIRV names
`endpointViewProj`/`endpointWorld` are in the rebuilt blob. Indexed
raster uses `screenPosBuffer` from `RSPProcessCS`. Per-vertex CURRENT
would zero the dumped explosions. Visual FAIL does **not** prove the
GPU CURRENT branch executed and failed to replace `tfPos`.

## Binding / dispatch proof (2026-09-12)

```text
FAILED_VISUAL_IMPL_CLASS: UNKNOWN
VISIBLE_FEATHER_USES_RSPPROCESSCS: YES
VISIBLE_FEATHER_USES_EXPECTED_SHADER_VARIANT: YES
RSPPROCESS_WRITES_FINAL_SCREENPOS_FOR_TARGET: YES
SCREENPOS_OVERWRITTEN_AFTER_RSPPROCESS: NO
T22_DESCRIPTOR_MAPPING_CORRECT: YES
T23_DESCRIPTOR_MAPPING_CORRECT: YES
DESCRIPTOR_LAYOUT_INCLUDES_22_23: YES
DESCRIPTOR_WRITE_FOR_22_EXECUTED: YES
DESCRIPTOR_WRITE_FOR_23_EXECUTED: YES
BOUND_BUFFER_22_IS_ENDPOINT_VIEWPROJ: YES
BOUND_BUFFER_23_IS_ENDPOINT_WORLD: YES
ENDPOINT_VIEWPROJ_PAIR_LAYOUT_VALID: YES (CPU fill: size == 2 * viewProj count)
ENDPOINT_WORLD_PAIR_LAYOUT_VALID: YES (CPU fill: size == 2 * world count)
ENDPOINT_VIEWPROJ_UPLOAD_SIZE_VALID: YES (same BufferUploader batch as t17)
ENDPOINT_WORLD_UPLOAD_SIZE_VALID: YES (same BufferUploader batch as t18)
ENDPOINT_VIEWPROJ_DESCRIPTOR_RANGE_VALID: YES (offset 0, whole buffer, stride 64)
ENDPOINT_WORLD_DESCRIPTOR_RANGE_VALID: YES
VIEWPROJ_SLOT0_IS_PREVIOUS: YES
VIEWPROJ_SLOT1_IS_CURRENT: YES
WORLD_SLOT0_IS_PREVIOUS: YES
WORLD_SLOT1_IS_CURRENT: YES
MODIFIED_RSPPROCESS_SHADER_IS_RUNTIME_PIPELINE: YES
OLD_SHADER_VARIANT_CAN_STILL_BE_SELECTED: NO
SHADER_VARIANT_SELECTION_EXPLAINS_VISUAL_FAIL: NO
CURRENT_TFPOS_REACHES_DSTPOS_IF_BRANCH_TAKEN: YES
DSTPOS_LATER_OVERWRITTEN: NO
PLUMBING_FIX_IMPLEMENTED: NO
GPU_READBACK_USED: NO
```

Indexed raster binds `outputBuffers.screenPosBuffer` as vertex slot 0.
`RasterVS` converts that `iPosition` to clip; it does not rebuild
`viewProj*world*pos`. `RSPWorldCS` writes world-space buffers only.
`RSPModifyCS` overwrites `screenPos` only for `modifyPosUints`
(`G_MODIFYVTX`); prior P09 evidence closed that for 0x62B feathers.

SPIR-V `RSPProcessCSBlobSPIRV` (build `src/shaders/RSPProcessCS.hlsl.spirv.h`,
size 12312) names `endpointViewProj`/`endpointWorld` with Binding 22/23
set 0. `ShaderLibrary::setupCommonShaders` creates the sole
`rspProcess` compute pipeline from that blob. `RSPProcessor::recordCommandList`
binds `shaderLibrary->rspProcess.pipeline`.

CPU pair writes (not comments):
`endpointViewProjTransforms[transformsIndex * 2] = prevView*prevProj`,
`*2+1 = curView*curProj`; world `*2 = prevTransform`, `*2+1 = curTransform`.
Inverted pair order would still detect `prev.w * cur.w <= 0` (commutative)
and only swap which endpoint is used as CURRENT.

Default-off CPU bind proof (no GPU readback/sync):
`TOOIE_DIAG_RSP_ENDPOINT_BINDING=1` writes
`/tmp/tooie-rsp-endpoint-binding-status.txt` from
`RSPProcessor::process` after the t22/t23 `setBuffer` calls.

## Live bind vs VERSION=4 (2026-09-12)

User capture: t22/t23 **buffer identities** match the uploaders.
`TARGET_DISPATCH_COUNT=4913`, last `TARGET_VERTEX_COUNT=2037`,
`CPU_PREDICTED_CROSSING_VERTEX_COUNT=0`. VERSION=4:
`PREV_CUR_CLIP_W_SIGN_CHANGE_VERTEX_COUNT=1292` on accumulated
visible 0x62B face vertices (`VISIBLE_FACE_VERTEX_COUNT=204555`).

```text
BINDING_DIAG_USES_SAME_POSITION_AS_V4: NO
BINDING_DIAG_USES_SAME_VIEWPROJ_INDEX_AS_V4: YES (same viewProjIndices[g] when the vertex is in both sets)
BINDING_DIAG_USES_SAME_WORLD_INDEX_AS_V4: YES (same worldIndices[g] when the vertex is in both sets)
BINDING_DIAG_USES_SAME_PREV_VIEWPROJ_AS_V4: NOT_LIVE_PROVEN
BINDING_DIAG_USES_SAME_CUR_VIEWPROJ_AS_V4: NOT_LIVE_PROVEN
BINDING_DIAG_USES_SAME_PREV_WORLD_AS_V4: NOT_LIVE_PROVEN
BINDING_DIAG_USES_SAME_CUR_WORLD_AS_V4: NOT_LIVE_PROVEN
BINDING_DIAG_TARGET_POPULATION_EQUIVALENT_TO_V4: NO
FIRST_DIFFERENCE: last-dispatch overwrite of all-workload vertices vs accumulated 0x62B visible-face vertices; sidecar uses pos (no vel) and <=0; V4 uses pos-vel at prev weight 0 and <0, only intermediate renders
SIDECAR_PREV_VIEWPROJ_CONTENT_EQUALS_V4: NOT_LIVE_PROVEN
SIDECAR_CUR_VIEWPROJ_CONTENT_EQUALS_V4: NOT_LIVE_PROVEN
SIDECAR_PREV_WORLD_CONTENT_EQUALS_V4: NOT_LIVE_PROVEN
SIDECAR_CUR_WORLD_CONTENT_EQUALS_V4: NOT_LIVE_PROVEN
SIDECAR_FRAME_OWNERSHIP_CORRECT: NOT_LIVE_PROVEN
ASPECT_ADJUSTMENT_MISMATCH: NOT_LIVE_PROVEN (V4 global 1/aspect; sidecar per-projection AUTO)
SIDECAR_CONTENT_FIX_IMPLEMENTED: NO
```

The previous diagnostic reset `CPU_PREDICTED_*` every
`RSPProcessor::process` and rewrote the status file, so 0 is the
**last** dispatch only. `rt64_state.cpp` RDRAM path also calls
`process()` with `curFrameWeight=1` and no frame map. Min/max world
1..117 does not prove the nine feather worlds were sampled.

Construction (static, not live-proven equal to V4): after
`GameFrame::match`, `ProjectionProcessor::process` writes
`endpointViewProj[i*2]=prevView*aspect(prevProj)` from
`workloadMap.prevWorkloadIndex` + `viewProjMap.prevTransformIndex`,
and `i*2+1` current pre-lerp; `TransformProcessor::process` writes
`endpointWorld[t*2]` from `transformMap.prevTransformIndex` when
mapped. Same sources as V4 `makeQPrev`/`makeWPrev` **when** that
projection is processed and the transform is mapped. Identity init
copies current into both slots until then.

The same env now keeps session sidecar/witness A/B on those CPU
vectors (`SESSION_*`, `MAX_*_CLIP_W_DIFF`, DL/world masks).

## Live sidecar vs witness (2026-09-12)

User recapture with session counters:

```text
SESSION_SIDECAR_CROSS_COUNT=108569
SESSION_WITNESS_CROSS_COUNT=108569
WITNESS_CROSS_SIDECAR_NO_CROSS_COUNT=0
SIDECAR_CROSS_WITNESS_NO_CROSS_COUNT=0
MAX_PREV_CLIP_W_DIFF=0
MAX_CUR_CLIP_W_DIFF=0
MAX_PREV_VIEWPROJ_ABS_DIFF=0
MAX_CUR_VIEWPROJ_ABS_DIFF=0
MAX_PREV_WORLD_ABS_DIFF=0
MAX_CUR_WORLD_ABS_DIFF=0
SESSION_FEATHER_SIDECAR_CROSS_COUNT=1698
SESSION_FEATHER_WITNESS_CROSS_COUNT=1698
SESSION_TARGET_DL_MASK=0x3fff
SESSION_FEATHER_WORLD_MASK=0x1ff
OUT_OF_RANGE_ENDPOINT_INDEX_COUNT=0
SIDECAR_CONTENT_MATCHES_WITNESS: PROVEN
TARGET_POPULATION_COVERAGE: PROVEN
```

Do not change endpoint construction, pair order, aspect, or W-sign
policy.

## Intermediate-only counters (2026-09-12)

`TOOIE_DIAG_RSP_ENDPOINT_BINDING=1` now also writes:

```text
SESSION_INTERMEDIATE_DISPATCH_COUNT
SESSION_INTERMEDIATE_FEATHER_VERTEX_COUNT
SESSION_INTERMEDIATE_FEATHER_SIDECAR_CROSS_COUNT
SESSION_INTERMEDIATE_FEATHER_WITNESS_CROSS_COUNT
SESSION_CURRENT_FEATHER_SIDECAR_CROSS_COUNT
INTERMEDIATE_WITNESS_CROSS_SIDECAR_NO_CROSS_COUNT
```

Live values are `NOT_CAPTURED` until the next binding recapture.
`INTERMEDIATE_FEATHER_CROSSINGS_EXIST: NOT_CAPTURED`.

If that recapture is `NO`, VERSION=4's 75 explosions at
intermediate `w=0.5` are still the interpolated
`viewProj*world*pos` sample near `clip.w=0`. Endpoint
`prev.w*cur.w` can stay same-sign while independent VIEW/WORLD
lerps pass through `w≈0`; the production branch would then not
fire on those interpolated samples. VERSION=4 counted W-sign
change on accumulated visible 0x62B faces at intermediate
renders; it is not a session union of `w=0`/`w=1` dispatches.

## Upload / GPU visibility (static, 2026-09-12)

CPU fill: `ProjectionProcessor::process` then `upload` (t17 + t22);
`TransformProcessor::process` then `upload` (t18 + t23). Both
`submit()` memcpy into `uploadBuffer`. `FramebufferRenderer::recordSetup`
issues COPY barriers, `copyBufferRegion` upload→default, then
`commandListAfterBarriers` with `RenderBarrierStage::ALL` (includes
COMPUTE). `RSPProcessor::recordCommandList` dispatches after that
copy. `waitForUploaders()` completes CPU memcpy before
`execute()`. `workloadGraphicsWorker->wait()` completes GPU before
the next overwrite of the same `BufferPair`. No extra sync added.

```text
ENDPOINT_VIEWPROJ_UPLOAD_BEFORE_DISPATCH: YES
ENDPOINT_WORLD_UPLOAD_BEFORE_DISPATCH: YES
TRANSFER_TO_COMPUTE_VISIBILITY_PROVEN: YES
ENDPOINT_BUFFER_LIFETIME_COVERS_DISPATCH: YES
BUFFER_REUSE_CAN_OVERWRITE_ENDPOINTS_BEFORE_USE: NO
GPU_SYNCHRONIZATION_ADDED: NO
```

t22/t23 share the same uploader path as working interpolated
t17/t18. No unique endpoint lifetime bug is `PROVEN`. If
`processProjections`/`processTransforms` is false, those sidecars
are not re-uploaded that tick; high-rate matched frames take the
upload path.

## SPIR-V (production blob before force diagnostic)

Disassembled `build/src/shaders/RSPProcessCS.hlsl.spirv.c`
(12312 bytes). Bindings: `endpointViewProj` 22, `endpointWorld`
23, `dstPos` 19. Pair index `*2` / `*2+1`. `OpMatrixTimesMatrix`
+ `OpVectorTimesMatrix` for prev and cur; `OpCompositeExtract` `.w`;
`OpFMul`; `OpFOrdLessThanEqual` vs `0`. True block recomputes
CURRENT clip. `OpPhi` selects interpolated vs CURRENT. After the
`tfPos.w==0` epsilon hack, NDC + viewport → `OpStore` `dstPos`.

```text
SPIRV_W_SIGN_TEST_PRESENT: YES
SPIRV_CURRENT_CLIP_SELECTION_PRESENT: YES
SPIRV_SELECTED_TFPOS_REACHES_DSTPOS: YES
```

## Shader-local force CURRENT diagnostic (2026-09-12)

Tasks 1–3 did not identify a proven upload/SPIR-V reason for the
remaining visual fail. Default-off
`TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1` sets a push-constant
flag so `RSPProcessCS` assigns `tfPos = endpoint CURRENT clip`
for every processed vertex. Does not change projection/transform
interpolation, global weights, or present interpolation.
Production W-sign policy is unchanged when the env is unset.

```text
SHADER_LOCAL_FORCE_CURRENT_DIAGNOSTIC_IMPLEMENTED: YES
PRODUCTION_POLICY_CHANGED: NO
```

## Next

```text
NEXT_SINGLE_ACTION: TOOIE_DIAG_RSP_MARK_W_CROSSING=1 ./build/TooieRecompiled; report A (obvious sideways jump) or B (same as normal glitch). No GPU readback.
CLEANUP_READY: NO
COMMIT_READY: NO
```
