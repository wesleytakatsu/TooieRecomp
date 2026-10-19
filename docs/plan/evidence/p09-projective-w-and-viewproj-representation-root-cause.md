---
title: "P09 projective-W and viewProj representation root cause"
description: "VERSION=3 COMBINED_RELATIVE is INVALID; PR23 still REFUTED; working class is projective clip.w / P-V gauge. Witness v4 pending recapture."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-10
tags: [tooie-recomp, p09, kazooie, feather, interpolation, clip-w, viewproj, projective, pr23]
---

# P09 projective-W and viewProj representation root cause

Schema: `P09_PROJECTIVE_W_AND_VIEWPROJ_REPRESENTATION_ROOT_CAUSE`.

No GPU readback, copy, layout, sync, staging, or occlusion query.
No production interpolation change. No live override. No PR22/23
port. Game not launched by this agent.

```text
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
WEIGHTS_MUTATED: NO
RENDER_PATH_MUTATED: NO
GENERATED_RECOMPILED_CODE_EDITED: NO
```

Authoritative VERSION=3 capture
(`/tmp/tooie-feather-temporal-coherency-status.txt`):

```text
VERSION=3
LIVE_EXPLOSION_COUNT=199
MAX_LIVE_EXPANSION=1776.6
SIMPLE_EXPLOSION_COUNT=199
MAX_SIMPLE_EXPANSION=1772.3
PR23_SIMPLE: REFUTED
ZERO_EFFECTIVE_VELOCITY_COUNT=168090
VIEW_MATCH_STABLE=YES
PROJECTION_MATCH_STABLE=YES
RELATIVE_R=mul(V,W)
RELATIVE_CLIP_ORDER=NEITHER_MATCHES_LIVE_ENDPOINTS
RELATIVE_TRANSFORM_ENDPOINTS_VALID=NO
COMBINED_RELATIVE_SIMPLE_EXPLOSION_COUNT=2370
ENDPOINT_CLIP_LERP_EXPLOSION_COUNT=195
MAX_ENDPOINT_CLIP_LERP_RATIO=2104.51
CAMERA_POSE_INVERSE_EXPLOSION_COUNT=199
CAMERA_MODEL_MOTION_CORRELATED=NO
ENDPOINT_RELATIVE_MOTION_SMALLER_THAN_INDEPENDENT_WORLD_CAMERA_MOTION=NO
EXPLOSION_B_CLASS_COUNT=175
EXPLOSION_C_CLASS_COUNT=24
INTERMEDIATE_W_NEAR_ZERO_COUNT=0
RT64_CLIP=mul(mul(V,P),W)*pos
```

Exact live raster (`PROVEN_STATIC`, do not algebraically reorder):

```text
viewProjMat = mul(viewMatrix, projMatrix)
tfPos = mul(mul(viewProjMat, worldMat), float4(pos - vel*(1-w), 1))
```

Sources: `ProjectionProcessor::processScene` then
`RSPProcessCS.hlsl` (`tfPos` then `ndcPos = xyz / (w,-w,w)`).

## VERSION=3 relative counterfactual (`PROVEN` invalid)

```text
VERSION3_RELATIVE_COUNTERFACTUAL_VALID: NO
CURRENT_ROOT_CLASS (prior label): CORRECTED/SUPERSEDED
  INDEPENDENT_VIEW_AND_WORLD_INTERPOLATION_OF_COUPLED_RELATIVE_MOTION
  is NOT proven. COMBINED_RELATIVE_* used R=mul(V,W) whose clip
  order matches neither live endpoint. Counts 2370 vs live 199 are
  not causal.
```

The prior camera/world cancellation story is `STRONGLY_WEAKENED`:
relative translation is **larger** on explosions than on sane
triangles (`MEAN_REL_TRANS_EXP=364` vs `SANE=444` is not smaller;
`ENDPOINT_RELATIVE_MOTION_SMALLER...=NO`).

## PR23 (`PROVEN` still refuted)

```text
PR23_SIMPLE_FIX_STATUS: REFUTED
```

SIMPLE matches live (199/199, max 1772 vs 1776). Velocity is always
zero. View/projection identity pairing is stable. No cross-bone
identity pairing.

## Dual B/C on VERSION=3 (`PROVEN` counts, not a projection-bug proof)

B = CURRENT viewProj + live world. C = live viewProj + CURRENT world.

```text
EXPLOSION_B_CLASS_COUNT: 175
EXPLOSION_C_CLASS_COUNT: 24
```

Worst case live=1776.6, ratioB=1.04143, ratioC=1348.94, ratioS=1772.3.
CURRENT complete viewProj removes most dumped explosions in offline
analysis. That does **not** prove a projection bug. It only shows
the interpolated `viewProjMat` is the larger term in this capture.

## Task 2 from VERSION=3 NDC (`STRONG_INFERENCE`, clip.w not dumped)

Dumped endpoints are near antipodes. Example 1776.6×
(`curId=1551`, bone 104, world 127):

```text
ndcC ≈ (-0.00962, +0.00974, -0.01585)
ndcP ≈ (+0.00937, -0.00902, +0.01473)
ndcI ≈ (+0.30, -0.30, +0.48) … (+0.54, -0.53, +0.86)
```

Homogeneous lerp of those clips still explodes (195/199, max 2104).
`INTERMEDIATE_W_NEAR_ZERO_COUNT=0` used `|w|<1e-4` on the **live
matrix** clip, not a scale-aware metric, and clip-lerp samples with
`|w|<1e-4` were dropped from the 195 count. Do not treat that zero
as evidence against projective-W collapse.

```text
PREV_CUR_CLIP_W_SIGN_CHANGE_VERTEX_COUNT: NOT_CAPTURED
LIVE_NORMALIZED_W_MIN: NOT_CAPTURED
LIVE_NORMALIZED_W_P01: NOT_CAPTURED
EXPLODING_VERTEX_NORMALIZED_W_MIN: NOT_CAPTURED
EXPLOSION_WITH_W_SIGN_CHANGE_COUNT: NOT_CAPTURED
EXPLOSION_WITH_NORMALIZED_W_LT_0_1_COUNT: NOT_CAPTURED
EXPLOSION_WITH_NORMALIZED_W_LT_0_01_COUNT: NOT_CAPTURED
EXPLOSION_WITH_NORMALIZED_W_LT_0_001_COUNT: NOT_CAPTURED
PROJECTIVE_W_COLLAPSE_CORRELATES_WITH_EXPLOSION: UNKNOWN
ENDPOINT_CLIP_LERP_FAILS_DUE_TO_W_SIGN_CROSSING: UNKNOWN
ENDPOINT_CLIP_LERP_FAILS_DUE_TO_PROJECTIVE_SCALE_DISCONTINUITY: UNKNOWN
VIEWPROJ_PROJECTIVE_SCALE_DISCONTINUITY_OBSERVED: NOT_CAPTURED
VIEWPROJ_SIGN_EQUIVALENCE_FLIP_OBSERVED: NOT_CAPTURED
EXPLOSION_COUNT_CURRENT_VIEW_ONLY: NOT_CAPTURED
EXPLOSION_COUNT_CURRENT_PROJECTION_ONLY: NOT_CAPTURED
EXPLOSION_COUNT_CURRENT_COMPLETE_VIEWPROJ: NOT_CAPTURED
FULL_VIEWPROJ_SIMPLE_EXPLOSION_COUNT: NOT_CAPTURED
FULL_VIEWPROJ_DECOMPOSE_EXPLOSION_COUNT: NOT_CAPTURED
FULL_VIEWPROJ_INTERPOLATION_MATERIALLY_BETTER: NOT_CAPTURED
```

Witness schema bumped to `P09_COHERENCY_SCHEMA_VERSION=4`. Analysis
only. Does not feed render. Adds clip.w / `normalizedW`, view-vs-proj
split, full-viewProj SIMPLE/DECOMPOSE, and matrix gauge.

## Task 6–7 — Tooie P/V scale (`PROVEN_STATIC` + capture pending)

Native `func_800E44FC` (`src/game/transform_hooks.cpp`):

1. `func_800CB124` builds P (near/far/fov; entire P `*= 0.5`).
2. `func_800CA6E8` returns `viewport+0x164` as `scale`.
3. `func_800E443C` builds guest combined `T_S * V_rot * P` with
   `inv_scale = 1/scale`.
4. `func_80018BC4` tests mlMtx elements against S15-ish bounds; on
   hit, `scale *= 2` and `func_800E443C` is rebuilt.
5. Submitted RT64 path: `gSPMatrix(P, LOAD)` then
   `gSPMatrix(V_view, MUL)` with `V_view = T_S * V_rot`.
   P is **not** rebuilt on overflow.

```text
TOOIE_PV_SCALE_IS_COMPENSATING_PAIR: NO
  P does not track CA6E8/overflow. Overflow shrinks V only.
  Product VP changes homogeneous gauge; NDC of a still camera
  can stay equivalent (HYPOTHESIS until v4/scale dump).
P08_PV_SPLIT_ENDPOINT_EQUIVALENT: YES
  Split is constructed so V*P matches the combined guest VP
  used for D_8012D558 (P08 matrix fix).
P08_PV_SPLIT_INTERPOLATION_CONTINUOUS: UNKNOWN
  Signature original-FIXED / high-rate-interp-GLITCH /
  global-CURRENT-FIXED is the expected interpolation-unstable
  representation. Not proven which factor (V gauge, P lerp,
  clip.w) until VERSION=4.
TOOIE_SCALE_CHANGES_BETWEEN_ANOMALY_ENDPOINTS: NOT_CAPTURED
TOOIE_OVERFLOW_BRANCH_DIFFERS_BETWEEN_ANOMALY_ENDPOINTS: NOT_CAPTURED
TOOIE_PV_PROJECTIVE_GAUGE_DISCONTINUITY: HYPOTHESIS YES
  Overflow is a discrete ×2 gauge on V with P held fixed.
```

Passive CPU witness: `/tmp/tooie-pv-scale-status.txt` when
`TOOIE_DIAG_FEATHER_TEMPORAL_COHERENCY=1` or `TOOIE_DIAG_PV_SCALE=1`.

## Task 8 — BanjoRecomp (`PROVEN_STATIC`)

`patches/camera_transform_tagging.c`:

- LOAD P + rotate MULs on the projection stack.
- `gEXSetViewMatrixFloat` with `guTranslateF` of gameplay camera
  **world position** (identity view for ortho).
- Projection group `G_EX_INTERPOLATE_SIMPLE` with stable IDs from
  `projection_transform_tagging.c`.

Banjo does **not** fold `inv_scale` camera translation into the
interpolated VIEW the way Tooie `T_S` does.

```text
BANJO_USES_EXPLICIT_FLOAT_VIEW: YES
BANJO_AVOIDS_TOOIE_PV_SCALE_GAUGE: YES
```

Do not assume IDs or SIMPLE are the P09 fix.

## Production

```text
PRODUCTION_FIX_CANDIDATE_FOUND: NO
NEW_PASSIVE_CAPTURE_REQUIRED: YES
```

No visual override. No projection-current / world-current /
global-current production behavior. No PR22/23.

## Next capture

Rebuild TooieRecompiled (picks up RT64 witness v4 + scale note),
then:

```text
rm -f /tmp/tooie-feather-temporal-coherency-status.txt /tmp/tooie-pv-scale-status.txt
TOOIE_DIAG_FEATHER_TEMPORAL_COHERENCY=1 ./build/TooieRecompiled
```

Read `VERSION=4` plus `/tmp/tooie-pv-scale-status.txt`. No other
diag flags. No GPU readback.

```text
NEXT_SINGLE_ACTION: rebuild TooieRecompiled; recapture TOOIE_DIAG_FEATHER_TEMPORAL_COHERENCY=1; read VERSION=4 and pv-scale status
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
NO_COMMIT: YES
NO_PUSH: YES
```
