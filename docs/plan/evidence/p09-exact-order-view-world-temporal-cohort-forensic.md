---
title: "P09 exact-order VIEW×WORLD temporal cohort forensic"
description: "Hierarchy REFUTED. Previous MODELVIEW reordered projection. CPU now traces VIEW/WORLD source cohorts, exact-order LLLL/CLLL/LLCL/CLCL attribution, W cancellation, and SAME_COHORT_COMPOSITE without reorder. Recapture required."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-15
tags: [tooie-recomp, p09, interpolation, join, view, world, cohort, factorial, clcl, composite]
---

# P09 exact-order VIEW×WORLD temporal cohort forensic

Authoritative user recapture closed independent world-space
interpolation as the dominant P09 cause:

```text
WORLDSPACE_JOIN_PAIR_COUNT=107082
PREV_WORLDSPACE MAX/P95/P99=1 / 1 / 1
CURRENT_WORLDSPACE MAX/P95/P99=1 / 1 / 1
LIVE_WORLDSPACE MAX/P95/P99=1.00591028 / 0.999999166 / 1
LIVE_WORLD_INTERPOLATION_BREAKS_ARTICULATED_ATTACHMENTS=NO
INDEPENDENT_WORLD_HIERARCHY_INTERPOLATION=REFUTED
```

Do not pursue hierarchy as a production fix. The hierarchical
counterfactual's large numbers are irrelevant because LIVE_WORLD
already preserves attachments.

Exact-order factorial remains valid (`USER_OBSERVED`):

```text
LLLL JOIN MAX/P99=5673.36084 / 0.958222866
CLLL (CURRENT VIEW only) JOIN MAX/P99=12.584487 / 1.02001762
LLCL (CURRENT WORLD only) JOIN MAX=6683.48779
CLCL JOIN MAX/P99=1.01412618 / 1.00001514
CLCL MATCHES_ALL_CURRENT=YES
STRONGEST_SINGLE_FACTOR=VIEW
STRONGEST_PAIR=VIEW_WORLD
MINIMUM_CURRENT_FACTOR_SET_MATCHING_ALL_CURRENT_BEHAVIOR=CLCL
POSITION velocity=0
```

Do **not** implement CURRENT VIEW+WORLD as production.

## Previous MODELVIEW counterfactual (`PROVEN` static)

The previous diagnostic reported

```text
MODELVIEW_RT64_CONVENTION=
  hlslpp::mul(VIEW,WORLD)_then_hlslpp::mul(interpVW,LIVE_PROJ)*pos4
```

Production tfPos is

```text
MODELVIEW_PRODUCTION_TFPOS_ORDER=
  hlslpp::mul(mul(mul(VIEW,PROJ),WORLD),pos4)
```

That counterfactual reordered projection. It is not a valid causal
test of VIEW×WORLD temporal interaction.

```text
PREVIOUS_MODELVIEW_COUNTERFACTUAL_PRESERVED_PRODUCTION_ORDER: NO
PREVIOUS_MODELVIEW_RESULT_VALID_FOR_ROOT_CAUSE: NO
```

Do not use its catastrophic numbers as evidence against VIEW×WORLD
temporal interaction.

## Task 1 — temporal source cohorts (`PROVEN` static + instrumented)

VIEW interpolation (`ProjectionProcessor::processScene`):

- CURRENT: `drawData.viewTransforms[vp]` of the current workload
- PREV: `workloads[workloadMap.prevWorkloadIndex].viewTransforms[viewProjections[vp].prevTransformIndex]`
- Match-time PREV workload is now stored on
  `ViewProjectionMap.prevWorkloadIndex` from
  `prevScene.projections[p].workloadIndex` (scene pairing by
  view+proj difference, not matrixId ordinal)

WORLD interpolation (`TransformProcessor::process`):

- CURRENT: `drawData.worldTransforms[t]` of the current workload
- PREV: `workloads[workloadMap.prevWorkloadIndex].worldTransforms[transforms[t].prevTransformIndex]`
- Match-time PREV is the same ordinal pair
  `curFrame.workloads[w] ↔ prevFrame.workloads[w]`

At interpolation time both streams of one GameCall read PREV from
the same `WorkloadMap.prevWorkloadIndex`. CURRENT both come from
the current workload. Remaining runtime mismatch classes:

- VIEW mapped XOR WORLD mapped (one stream has a real interval,
  the other snaps current→current)
- VIEW match-time `prevWorkloadIndex` ≠ interpolation
  `WorkloadMap.prevWorkloadIndex` (scene pairing vs ordinal
  workload pairing)

CPU status now dumps last sources and those counts.

```text
VIEW_PREV_SOURCE: NOT_CAPTURED
VIEW_CURRENT_SOURCE: NOT_CAPTURED
WORLD_PREV_SOURCE: NOT_CAPTURED
WORLD_CURRENT_SOURCE: NOT_CAPTURED
VIEW_WORLD_PREV_TEMPORAL_COHORT_ALIGNED: NOT_CAPTURED
VIEW_WORLD_CURRENT_TEMPORAL_COHORT_ALIGNED: NOT_CAPTURED
VIEW_WORLD_ENDPOINT_TEMPORAL_COHORT_ALIGNED: NOT_CAPTURED
```

Static: CURRENT alignment is the same current workload for a
GameCall (`STRONG_INFERENCE`). PREV alignment is the same interp
prev workload if both mapped, else mismatch.

## Task 2 — weight semantics (`PROVEN` static)

`WorkloadQueue::threadRenderFrame` passes one `curFrameWeight` /
`prevFrameWeight` pair into ProjectionProcessor, TransformProcessor,
and RSPProcessor.

```text
VIEW_INTERPOLATION_WEIGHT: ProcessParams.curFrameWeight
WORLD_INTERPOLATION_WEIGHT: ProcessParams.curFrameWeight
VIEW_WORLD_USE_SAME_NUMERIC_WEIGHT: YES
VIEW_WORLD_WEIGHT_HAS_SAME_TEMPORAL_INTERVAL: NOT_CAPTURED
```

Same numeric weight is proven. Same logical interval is true only
if both streams are mapped onto the same prev workload; that is
the Task 1 runtime count.

## Task 3 / 4 / 5 / 6 — CPU exact-order probes (`instrumented`)

Never reorder matrices. Pathological JOIN class:

- LLLL JOIN rel ≥ `kP09VisuallySignificantEdge` (1.5)
- CLCL JOIN rel ≤ `kP09NearCurrentEdge` (1.05)

SAME_COHORT_COMPOSITE (CPU diagnostic only, not production):

```text
M_prev = (PREV_VIEW * LIVE_PROJ) * PREV_WORLD
M_cur  = (CUR_VIEW  * LIVE_PROJ) * CUR_WORLD
clip   = lerpMatrix(M_prev, M_cur, curFrameWeight) * pos4
```

Uses existing `lerpMatrix` (elementwise 4x4). Does not decompose
as TRS. Does not move PROJECTION.

CLCL construction (`PROVEN` static):

```text
CURRENT VIEW  = viewTransforms[vp]
LIVE PROJ     = modProjTransforms[vp] (or projTransforms)
CURRENT WORLD = endpointWorld[world*2+1]
multiply      = hlslpp::mul(mul(mul(VIEW,PROJ),WORLD),pos4)
```

```text
LLLL_PATHOLOGICAL_SAMPLE_COUNT: NOT_CAPTURED
LLLL_PATHOLOGICAL_FIXED_BY_CLLL_COUNT: NOT_CAPTURED
LLLL_PATHOLOGICAL_FIXED_BY_LLCL_COUNT: NOT_CAPTURED
LLLL_PATHOLOGICAL_FIXED_BY_CLCL_COUNT: NOT_CAPTURED
VIEW_WORLD_INTERACTION_MANIFESTS_AS_HOMOGENEOUS_W_CANCELLATION: NOT_CAPTURED
SAME_COHORT_COMPOSITE_JOIN_MAX: NOT_CAPTURED
SAME_COHORT_COMPOSITE_JOIN_P99: NOT_CAPTURED
SAME_COHORT_COMPOSITE_MAX_JOIN_PHASE_DELTA: NOT_CAPTURED
SAME_COHORT_COMPOSITE_P99_JOIN_PHASE_DELTA: NOT_CAPTURED
SAME_COHORT_COMPOSITE_MAX_EDGE_RATIO_VS_ENDPOINT_ENVELOPE: NOT_CAPTURED
SAME_COHORT_COMPOSITE_P99_EDGE_RATIO_VS_ENDPOINT_ENVELOPE: NOT_CAPTURED
SAME_COHORT_COMPOSITE_MATCHES_SAFE_CLASS: NOT_CAPTURED
CLCL_COUNTERFACTUAL_VALID: NOT_CAPTURED
VIEW_WORLD_ROOT_CLASS: NOT_CAPTURED
```

W-cancellation is causal characterization only. Do not resurrect
W-crossing fallback.

## Diagnostic performance

Status writes remain ~1 Hz (`CPU_DIAG_STATUS_WRITE_THROTTLED_1S`).
Sampling still every RSP process. Residual sample cap 262144.
No GPU readback / copy / sync.

## Closed directions

Do not reopen: hierarchy interpolation, W-crossing fallback,
threshold policies, triangle CURRENT, coherent CURRENT scopes,
CLIP_LERP, NDC_LERP, world non-rigidity, velocity, projection as
independent cause, PR22 wrong occurrence pairing.

## Validation this session

```text
Tooie HEAD: ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf
RT64 HEAD:  6f1c2d99a4ea571c139f449c326fd176ba8f3496
cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test
TooieRecompiled: link OK (rt64_rsp_processor.cpp + rt64_game_frame.cpp rebuilt)
window_display_test: 38/38 PASS
tintro_projection_policy_test: 7/7 PASS
git diff --check RT64 rsp_processor/game_frame: PASS
PRODUCTION_FIX_UPDATED: NO
CLEANUP_READY: NO
COMMIT_READY: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

Game not launched this session.

Superseded operationally by
[[p09-temporal-cohort-mismatch-causal-proof]] (class A–E
correlation + COHORT_ALIGNED_LIVE). Keep this note as the
exact-order / SAME_COHORT_COMPOSITE provenance.
