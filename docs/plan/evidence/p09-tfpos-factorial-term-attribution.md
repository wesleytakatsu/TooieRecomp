---
title: "P09 tfPos factorial term attribution"
description: "CPU 2x2x2x2 LIVE/CURRENT ablation of RSPProcessCS VIEW, PROJECTION, WORLD, POSITION. CLCL is the captured safe class."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-15
tags: [tooie-recomp, p09, interpolation, join, tfpos, factorial, view, projection, world, velocity]
---

# P09 tfPos factorial term attribution

Authoritative prior capture closed world-matrix non-rigidity as the
dominant P09 cause (`USER_OBSERVED` / `RUNTIME_CPU`):

```text
WORLD_DECOMPOSE_BOTH_VALID_COUNT=123630
WORLD_DECOMPOSE_INVALID_COUNT=0
WORLD_MAPPED_PAIR_COUNT=123630
WORLD_UNMAPPED_PAIR_COUNT=0
LIVE_WORLD_MAX_ORTHOGONALITY_ERROR=0.126582444
LIVE_WORLD_MAX_SHEAR=0.0212038085
LIVE_NONRIGIDITY_CORRELATES_WITH_JOIN_ERROR=NO
LIVE JOIN MAX/P95/P99=48860.3867 / 0.408273101 / 0.905023575
RIGID_WORLD MAX/P95/P99=48902.5703 / 0.408487856 / 0.904102862
RIGID_WORLD_MATERIALLY_REDUCES_JOIN_ERROR=NO
PREV_CURRENT_WORLD_SEMANTIC_IDENTITY_STABLE=YES
NEW_TEMPORAL_IDENTITY_MISMATCH_FOUND=NO
WORLD_NONRIGIDITY_ROOT_CAUSE=REFUTED
```

Do not reopen PR22. Visual: LIVE FAIL, POLICY_A FAIL, shader-local
ALL CURRENT tfPos PASS. Remaining root is one or more LIVE vs
CURRENT terms inside the RSPProcessCS position equation.

Production shader/policy unchanged. Game not launched this session.

## Task 1 — exact LIVE / CURRENT equations (`PROVEN`)

`src/shaders/RSPProcessCS.hlsl` (RT64 HLSL column-vector `mul(M,v)`):

```text
LIVE_TFPOS_EQUATION:
  tfPos = mul(mul(viewProjMat, worldMat),
              float4(pos - vel * (1 - curFrameWeight), 1))
  viewProjMat = viewProjTransforms[vp]   // uploaded modViewProjTransforms
  worldMat    = worldTransforms[world]   // uploaded lerpWorldTransforms

CURRENT_TFPOS_EQUATION:
  tfPos = mul(mul(endpointViewProj[vp*2+1], endpointWorld[world*2+1]),
              float4(pos, 1))
```

Do not algebraically reorder. CPU diagnostic uses the same
`hlslpp::mul(mul(VIEWPROJ, WORLD), pos4)` convention.

Independent factors:

| Factor | LIVE | CURRENT |
| --- | --- | --- |
| VIEW | `modViewTransforms[vp]` = `RigidBody::lerp(curFrameWeight, prevView, curView, slerp=true)` | `curViewEndpoint` = `viewTransforms[vp]` after debugger, before view lerp |
| PROJECTION | `modProjTransforms[vp]` = `lerpMatrix(adjPrev, adjCur, curFrameWeight)` iff `lerpTranslation \|\| lerpRotation`, else CURRENT | `curProjEndpoint` = aspect-adjusted `projTransforms[vp]` before proj lerp |
| WORLD | `lerpWorldTransforms[world]` = `RigidBody::lerp(curFrameWeight, prev, cur, slerp=true)` | `endpointWorld[world*2+1]` |
| POSITION | `pos - vel * (1 - curFrameWeight)` | `pos` |

Combined LIVE viewProj is `hlslpp::mul(LIVE_VIEW, LIVE_PROJ)` in
`ProjectionProcessor`, uploaded as `modViewProjTransforms`. Combined
CURRENT viewProj is `hlslpp::mul(curViewEndpoint, curProjEndpoint)`
stored in `endpointViewProj[vp*2+1]`.

```text
TFPOS_DIFFERING_FACTORS: VIEW / PROJECTION / WORLD / POSITION_TERM
```

## Task 2 — 2x2x2x2 CPU factorial

CPU-only, visible 0x62B intermediate samples already walked by the
endpoint-binding diagnostic. Sixteen policies named
`{L,C}{L,C}{L,C}{L,C}` for VIEW, PROJECTION, WORLD, POSITION.

- Both VIEW+PROJ LIVE → production `modViewProjTransforms`
- Both VIEW+PROJ CURRENT → production `endpointViewProj[cur]`
- Mixed VIEW/PROJ → `mul(selectedView, selectedProj)` (same
  ProjectionProcessor order)
- WORLD CURRENT → `endpointWorld[cur]`
- WORLD LIVE → `lerpWorldTransforms`
- POSITION CURRENT → `float4(pos,1)`
- POSITION LIVE → `float4(pos-vel*(1-w),1)`

CCCC is required to reproduce existing ALL_CURRENT CPU geometry
within 1e-4 NDC. Runtime:

```text
FACTORIAL_ALL_CURRENT_MATCHES_EXISTING_CURRENT: YES
MAX_FACTORIAL_CURRENT_POSITION_DIFF: 0
```

## Task 3–5 — metrics / factor / interaction

Each policy emits JOIN max/P95/P99, JOIN phase max/P99, edge ratio
vs endpoint envelope max/P99, NONFINITE_COUNT. MAX is retained
(sample cap does not drop extrema).

Single/pair/triple CURRENT substitutions are labeled. Smallest
CURRENT set matching ALL_CURRENT **class** (pathological JOIN/gap
outliers, not bit-identical geometry) is computed after recapture.

```text
LLLL JOIN MAX/P99: 11.6197119 / 0.908135533
CLLL (CURRENT VIEW) JOIN MAX/P99: 39.400425 / 1.01485932
LLCL (CURRENT WORLD) JOIN MAX/P99: 11.3336315 / 0.927387238
LCLL (CURRENT PROJECTION): essentially LIVE
CLCL (CURRENT VIEW+WORLD, LIVE PROJ/POS) JOIN MAX/P99: 1.01003206 / 1.00002396
STRONGEST_SINGLE_FACTOR: WORLD
STRONGEST_PAIR: VIEW_WORLD
MINIMUM_CURRENT_FACTOR_SET_MATCHING_ALL_CURRENT_BEHAVIOR: CLCL
FACTORIAL_BEST_POLICY: CLCL
NEXT_PRODUCTION_FIX_CLASS: DO_NOT_IMPLEMENT_CURRENT_VIEW_WORLD
```

Do not ship CURRENT VIEW+WORLD. Discriminator:
[[p09-view-world-interaction-root-discriminator]].

## Task 6 — position / velocity audit

Counted on the same 0x62B intermediate walk. JOIN subset =
logical bones with `p09FeatherWorldBit != 0`. Nonzero means
`sqrt(vx^2+vy^2+vz^2) > 0`. If both JOIN and target velocity are
exactly zero, POSITION is algebraically irrelevant.

```text
TARGET_POSITION_SAMPLE_COUNT: 66495
TARGET_NONZERO_VELOCITY_COUNT: 0
JOIN_NONZERO_VELOCITY_COUNT: 0
POSITION_TEMPORAL_TERM_CAN_AFFECT_P09: NO
```

## Task 7 — VIEW vs PROJECTION temporal path (`PROVEN` static)

```text
VIEW_INTERPOLATION_METHOD:
  RigidBody::lerp(curFrameWeight, prevView, curView, slerp=true)
  Default camera decompose is off (game_frame projectionDecompose
  starts false; “usually look better with simple interpolation”).
  lerpDecompose false → lerpMatrixComponents (element-wise 3x3 +
  translation if flags allow).

PROJECTION_INTERPOLATION_METHOD:
  After the same aspect column scale as CURRENT.
  IFF rigidBody.lerpTranslation || lerpRotation:
    lerpMatrix(adjustedPrevProj, curProj, curFrameWeight)
    (component-wise 4x4)
  ELSE CURRENT proj (no temporal mix).

VIEW_AND_PROJECTION_USE_SAME_TEMPORAL_WEIGHT: YES
  Both use ProjectionProcessor ProcessParams.curFrameWeight.

VIEW_AND_PROJECTION_ENDPOINT_PAIR_IDENTITY_STABLE: YES
  One GameFrameMap::ViewProjectionMap.prevTransformIndex feeds
  both prev view and prev proj. Shared pair by construction.
```

Do not reopen rigidity. Temporal attribution is the remaining
question.

## Task 8 — production candidate

Not chosen. Shader unchanged. No W thresholds, no triangle
CURRENT, no CLIP_LERP/NDC_LERP, no rigid-world production patch.

## Validation

```text
cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test
TooieRecompiled: linked
window_display_test: 38/38 PASS
tintro_projection_policy_test: 7/7 PASS
Tooie HEAD: ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf (dirty, pre-existing)
RT64 HEAD: 6f1c2d99a4ea571c139f449c326fd176ba8f3496 (dirty; diagnostic only)
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
PRODUCTION_FIX_UPDATED: NO
CPU_DIAG_SHARED_STATE_LOCKED: YES
STATUS_WRITE_ATOMIC: YES
```

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1 ./build/TooieRecompiled`
and paste `/tmp/tooie-rsp-endpoint-binding-status.txt` factorial
keys listed in HANDOFF.
