---
title: "P09 temporal cohort mismatch causal proof"
description: "User recapture showed 12 VIEW-mapped/WORLD-unmapped calls. CPU now classifies pathology by cohort class and runs COHORT_ALIGNED_LIVE without CURRENT snap. Recapture required. No production fix."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-15
tags: [tooie-recomp, p09, interpolation, join, view, world, cohort, clcl, aligned]
---

# P09 temporal cohort mismatch causal proof

Authoritative previous capture (`USER_OBSERVED`):

```text
COHORT_CALL_COUNT=12908
COHORT_BOTH_MAPPED_COUNT=12896
COHORT_VIEW_MAPPED_WORLD_UNMAPPED_COUNT=12
COHORT_WORLD_MAPPED_VIEW_UNMAPPED_COUNT=0
COHORT_NEITHER_MAPPED_COUNT=0
COHORT_VIEW_MATCH_PREV_WORKLOAD_MISMATCH_COUNT=0
VIEW_WORLD_PREV_TEMPORAL_COHORT_ALIGNED=NO
VIEW_WORLD_CURRENT_TEMPORAL_COHORT_ALIGNED=YES
VIEW_WORLD_ENDPOINT_TEMPORAL_COHORT_ALIGNED=NO
VIEW_WORLD_USE_SAME_NUMERIC_WEIGHT=YES
VIEW_WORLD_WEIGHT_HAS_SAME_TEMPORAL_INTERVAL=NO
LLLL_PATHOLOGICAL_SAMPLE_COUNT=288
CLLL CURRENT VIEW only = 278
LLCL CURRENT WORLD only = 14
CLCL CURRENT VIEW + CURRENT WORLD = 288
CLCL_COUNTERFACTUAL_VALID=YES
SAME_COHORT_COMPOSITE_MATCHES_SAFE_CLASS=NO
```

Do **not** ship CLCL. Do not reopen W thresholds/crossing,
CLIP_LERP, NDC_LERP, hierarchy, world rigidity, velocity,
projection-only, PR22, or full projective matrix lerp.

## Task 1 — mismatch → pathology correlation (`NOT_CAPTURED`)

CPU JOIN now classifies every accepted target GameCall/sample:

| Class | Meaning |
| --- | --- |
| A | VIEW+WORLD PREV both mapped, same prev workload |
| B | VIEW mapped, WORLD unmapped |
| C | WORLD mapped, VIEW unmapped |
| D | both unmapped |
| E | both mapped, explicit different prev workload IDs |

Per class the status dump reports GameCall/vertex/JOIN counts,
LLLL pathological JOIN count, JOIN MAX/P99, edge-envelope MAX/P99,
and small normalized `|W|` count, plus:

```text
PATHOLOGY_ON_ALIGNED_COHORT_COUNT
PATHOLOGY_ON_MISALIGNED_COHORT_COUNT
ALIGNED_COHORT_PATHOLOGY_RATE
MISALIGNED_COHORT_PATHOLOGY_RATE
TEMPORAL_COHORT_MISMATCH_CORRELATES_WITH_PATHOLOGY
```

Verdict rule (status): misaligned pathology rate ≥ 5× aligned
rate, or class B JOIN MAX is catastrophic vs class A. The 12
class-B calls are dumped as `MISMATCH_CALL_00`… so recapture can
show whether they own the rare JOIN catastrophes.

```text
TEMPORAL_COHORT_MISMATCH_CORRELATES_WITH_PATHOLOGY: NOT_CAPTURED
PATHOLOGY_ON_ALIGNED_COHORT_COUNT: NOT_CAPTURED
PATHOLOGY_ON_MISALIGNED_COHORT_COUNT: NOT_CAPTURED
MISALIGNED_COHORT_PATHOLOGY_RATE: NOT_CAPTURED
ALIGNED_COHORT_PATHOLOGY_RATE: NOT_CAPTURED
```

## Task 2 — why WORLD is unmapped (`STRONG_INFERENCE` static)

Production pairing (`PROVEN` static):

- `GameFrame::match` sets `WorkloadMap.prevWorkloadIndex =
  prevFrame.workloads[w]` (ordinal slot).
- WORLD `TransformMap` is filled only by matrixId linear walk on
  that ordinal prev. Unmapped worlds make
  `TransformProcessor` copy current as both endpoints.
- VIEW `ViewProjectionMap.prevWorkloadIndex` is scene-matched
  (`prevScene.projections[p].workloadIndex`).
- At interp time `ProjectionProcessor::processScene` still reads
  PREV VIEW from `workloads[WorkloadMap.prevWorkloadIndex]`,
  using `ViewProjectionMap.prevTransformIndex`.

Previous capture: `COHORT_VIEW_MATCH_PREV_WORKLOAD_MISMATCH_COUNT=0`.
So VIEW's scene prev workload **is** the ordinal prev on that run.
The 12 failures are **not** “WORLD interpolating a different
workload than VIEW”. WORLD has `TransformMap.mapped=false` and
snaps current→current while VIEW still interpolates.

```text
WORLD_PREV_MAPPING_FAILURE_REASON: NOT_CAPTURED
  (static candidate: WORLD_TRANSFORM_UNMAPPED_SNAPS_CURRENT)
IS_WORLD_USING_ORDINAL_PREV_WORKLOAD_WHERE_VIEW_USES_SCENE_MAPPING: NOT_CAPTURED
  (static: NO on the 12 if view-match prev == ordinal prev;
   WORLD is unmapped, not using a prev transform at all)
```

Recapture dumps matrixId, ordering, unique hits in view-prev vs
ordinal-prev for those 12 calls.

## Task 3 / 4 — COHORT_ALIGNED_LIVE (`instrumented`, not captured)

Counterfactual (CPU only, no CURRENT snap):

```text
((LIVE_VIEW * LIVE_PROJ) * lerp(WORLD_PREV_FROM_VIEW_COHORT, CUR_WORLD, w)) * LIVE_POS
```

WORLD PREV is:

1. existing `TransformMap.prevTransformIndex` if WORLD is mapped
   into the same prev workload VIEW uses;
2. otherwise the unique `matrixId` match in that prev workload;
3. otherwise **unmappable** (no invented index, no CURRENT snap).

If view-prev == ordinal-prev, (2) is the same search
`GameFrame::match` already failed, so the 12 class-B calls are
likely `COHORT_ALIGNED_UNMAPPABLE`. Status then reports
`COHORT_ALIGNED_UNMAPPABLE_PATHOLOGICAL_COUNT`. Safe-class is
**NO** if any pathological JOIN pair was unmappable.

```text
COHORT_ALIGNED_VALID_SAMPLE_COUNT: NOT_CAPTURED
COHORT_ALIGNED_UNMAPPABLE_SAMPLE_COUNT: NOT_CAPTURED
COHORT_ALIGNED_JOIN_MAX: NOT_CAPTURED
COHORT_ALIGNED_JOIN_P99: NOT_CAPTURED
COHORT_ALIGNED_MAX_JOIN_PHASE_DELTA: NOT_CAPTURED
COHORT_ALIGNED_P99_JOIN_PHASE_DELTA: NOT_CAPTURED
COHORT_ALIGNED_MAX_EDGE_RATIO_VS_ENDPOINT_ENVELOPE: NOT_CAPTURED
COHORT_ALIGNED_P99_EDGE_RATIO_VS_ENDPOINT_ENVELOPE: NOT_CAPTURED
COHORT_ALIGNED_SMALL_W_COUNT: NOT_CAPTURED
COHORT_ALIGNED_LIVE_MATCHES_SAFE_CLASS: NOT_CAPTURED
COHORT_ALIGNED_VS_CLCL_JOIN_MAX: NOT_CAPTURED
COHORT_ALIGNED_VS_CLCL_PHASE_MAX: NOT_CAPTURED
COHORT_ALIGNED_VS_CLCL_ENVELOPE_MAX: NOT_CAPTURED
COHORT_ALIGNED_RETAINS_INTERMEDIATE_WORLD_COUNT: NOT_CAPTURED
COHORT_ALIGNED_RETAINS_INTERMEDIATE_VIEW_COUNT: NOT_CAPTURED
COHORT_FIX_PRESERVES_INTERPOLATION: NOT_CAPTURED
```

## Task 5 — generic production design (`gated`)

Do **not** implement until recapture shows:

- `TEMPORAL_COHORT_MISMATCH_CORRELATES_WITH_PATHOLOGY=YES`
- `COHORT_ALIGNED_LIVE_MATCHES_SAFE_CLASS=YES`
- `COHORT_ALIGNED_UNMAPPABLE_PATHOLOGICAL_COUNT=0`

Static audit of existing maps:

```text
GENERIC_FIX_REQUIRED_DATA_ALREADY_AVAILABLE: PARTIAL
  ViewProjectionMap.prevWorkloadIndex exists.
  TransformMap has prevTransformIndex + mapped only (no prevWorkloadIndex).
  WorkloadMap.prevWorkloadIndex is ordinal.
  TransformGroup.matrixId is available at match time.
GENERIC_FIX_LAYER: GameFrame::match (primary) / TransformProcessor (consumer)
```

`GENERIC_FIX_DESIGN` (candidate only):

1. Keep VIEW interpolation as-is.
2. WORLD interpolation must take PREV from the same previous
   workload cohort VIEW/scene mapping selected
   (`ViewProjectionMap.prevWorkloadIndex` when mapped, else
   `WorkloadMap.prevWorkloadIndex`).
3. Resolve WORLD PREV by the existing unique `matrixId` linear
   match into that workload. Do not invent an index. Do not
   hardcode Tooie / 0x62B / bone IDs.
4. If no unique semantic match exists, leave WORLD unmapped
   (current endpoints). Do **not** snap VIEW to CURRENT as
   production (that is CLCL).
5. If recapture shows the 12 class-B calls are unmappable in the
   VIEW prev workload as well, this routing fix **cannot**
   interpolate them. Stop; do not ship a Tooie special case.

Cross-layer ownership: `GameFrame::match` currently matches
worlds before/independently of scene prev-workload identity.
Putting `TransformMap.prevWorkloadIndex` on the map is a small
schema add; rematching unmapped worlds after `matchScene` is the
mechanical patch. That is **not** trivial lifetime/ownership, so
another pass should implement only after the counterfactual is
decisive.

## Closed directions

Do not reopen: W crossing/thresholds, triangle CURRENT,
transform/draw/component CURRENT, CLIP_LERP, NDC_LERP,
hierarchy interpolation, world rigidity, velocity,
projection-only, PR22 occurrence pairing, projective full-matrix
lerp.

## Validation this session

```text
Tooie HEAD: ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf
RT64 HEAD:  6f1c2d99a4ea571c139f449c326fd176ba8f3496
cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test
TooieRecompiled: link OK (rt64_rsp_processor.cpp rebuilt)
window_display_test: 38/38 PASS
tintro_projection_policy_test: 7/7 PASS
git diff --check Tooie: PASS
git diff --check RT64 rsp_processor: PASS
PRODUCTION_FIX_UPDATED: NO
CLEANUP_READY: NO
COMMIT_READY: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

Game not launched this session.

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled` and paste
`TEMPORAL_COHORT_MISMATCH_CORRELATES_WITH_PATHOLOGY`,
`PATHOLOGY_ON_*_COHORT_COUNT`, `*_PATHOLOGY_RATE`,
`COHORT_CLASS_B_*`, `MISMATCH_CALL_*`,
`WORLD_PREV_MAPPING_FAILURE_REASON`,
`IS_WORLD_USING_ORDINAL_PREV_WORKLOAD_WHERE_VIEW_USES_SCENE_MAPPING`,
`COHORT_ALIGNED_*`, `COHORT_ALIGNED_LIVE_MATCHES_SAFE_CLASS`,
`COHORT_FIX_PRESERVES_INTERPOLATION`.
