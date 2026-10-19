---
title: "P09 mapping-coherence fallback counterfactual"
description: "CPU MAPPING_COHERENT_LIVE: interpolate VIEW+WORLD only when both have a valid PREV pair. Recapture required. No production fix."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-15
tags: [tooie-recomp, p09, interpolation, join, view, world, cohort, clcl, mapping-coherent]
---

# P09 mapping-coherence fallback counterfactual

Authoritative previous capture (`USER_OBSERVED`):

```text
COHORT_CALL_COUNT=12908
COHORT_BOTH_MAPPED_COUNT=12896
COHORT_VIEW_MAPPED_WORLD_UNMAPPED_COUNT=12
COHORT_WORLD_MAPPED_VIEW_UNMAPPED_COUNT=0
COHORT_NEITHER_MAPPED_COUNT=0
COHORT_VIEW_MATCH_PREV_WORKLOAD_MISMATCH_COUNT=0
LLLL_PATHOLOGICAL_SAMPLE_COUNT=288
CLCL CURRENT VIEW + CURRENT WORLD = 288
SAME_COHORT_COMPOSITE_MATCHES_SAFE_CLASS=NO
```

Do **not** ship CLCL. Do not reopen W thresholds/crossing,
CLIP_LERP, NDC_LERP, hierarchy, world rigidity, velocity,
projection-only, PR22, or full projective matrix lerp.

## Static correction (`STRONG_INFERENCE`)

The 12 class-B GameCalls are **not** proven as:

```text
VIEW prev workload A vs WORLD prev workload B
```

Static pairing:

- VIEW `ViewProjectionMap.mapped=true` interpolates PREV → CURRENT.
- WORLD `TransformMap.mapped=false`.
- `TransformProcessor` copies CURRENT as both WORLD endpoints.
- `ProjectionProcessor` still fetches PREV VIEW through ordinal
  `WorkloadMap.prevWorkloadIndex`.
- Previous capture: scene/match PREV workload == ordinal PREV
  (`COHORT_VIEW_MATCH_PREV_WORKLOAD_MISMATCH_COUNT=0`).

Candidate temporal defect:

```text
VIEW: PREV -> CURRENT
WORLD: CURRENT -> CURRENT
same numeric curFrameWeight on unequal intervals
```

Exact-order factorial already proved CLCL
(CURRENT VIEW, LIVE PROJECTION, CURRENT WORLD, LIVE POSITION)
is the safe class. On class B, production WORLD is already
CURRENT, so CURRENT VIEW recreates CLCL locally without globally
disabling interpolation.

## Task 1 — existing diagnostics preserved (`PROVEN` static)

Still present:

- class A–E cohort correlation
- `MISMATCH_CALL_*`
- `COHORT_ALIGNED_LIVE` (unmappable; does **not** synthesize WORLD PREV)
- pathology rates / small W / JOIN / envelope / mapping failure reason

## Task 2 — MAPPING_COHERENT_LIVE (`instrumented`, not captured)

CPU only. Exact production order:

```text
((VIEW * LIVE_PROJECTION) * WORLD) * LIVE_POSITION
```

| Class | Policy |
| --- | --- |
| A VIEW+WORLD mapped | production LIVE VIEW and LIVE WORLD |
| B VIEW mapped, WORLD unmapped | CURRENT VIEW, production WORLD, LIVE PROJ, LIVE POS |
| C VIEW unmapped, WORLD mapped | CURRENT VIEW and CURRENT WORLD |
| D both unmapped | production if already CURRENT/CURRENT; verify |
| E both mapped, different PREV | do not guess; record separately |

Does not alter projection. Does not use W thresholds.

```text
MAPPING_COHERENT_TOTAL_SAMPLE_COUNT: NOT_CAPTURED
MAPPING_COHERENT_CHANGED_SAMPLE_COUNT: NOT_CAPTURED
MAPPING_COHERENT_CHANGED_GAMECALL_COUNT: NOT_CAPTURED
MAPPING_COHERENT_PERCENT_SAMPLES_CHANGED: NOT_CAPTURED
MAPPING_COHERENT_IS_LOCALIZED: NOT_CAPTURED
MAPPING_COHERENT_JOIN_MAX: NOT_CAPTURED
MAPPING_COHERENT_JOIN_P99: NOT_CAPTURED
MAPPING_COHERENT_MAX_JOIN_PHASE_DELTA: NOT_CAPTURED
MAPPING_COHERENT_P99_JOIN_PHASE_DELTA: NOT_CAPTURED
MAPPING_COHERENT_MAX_EDGE_RATIO_VS_ENDPOINT_ENVELOPE: NOT_CAPTURED
MAPPING_COHERENT_P99_EDGE_RATIO_VS_ENDPOINT_ENVELOPE: NOT_CAPTURED
MAPPING_COHERENT_SMALL_NORMALIZED_W_COUNT: NOT_CAPTURED
MAPPING_COHERENT_LIVE_MATCHES_SAFE_CLASS: NOT_CAPTURED
```

## Task 5 — pathology causal test (`NOT_CAPTURED`)

```text
LLLL_PATHOLOGICAL_SAMPLE_COUNT: NOT_CAPTURED
PATHOLOGICAL_ON_B_VIEW_MAPPED_WORLD_UNMAPPED_COUNT: NOT_CAPTURED
PATHOLOGICAL_CLASS_B_FIXED_BY_MAPPING_COHERENT_COUNT: NOT_CAPTURED
PATHOLOGICAL_CLASS_B_NOT_FIXED_COUNT: NOT_CAPTURED
PATHOLOGICAL_OUTSIDE_CLASS_B_COUNT: NOT_CAPTURED
CLASS_B_MAPPING_ASYMMETRY_EXPLAINS_CATASTROPHIC_GLITCH: NOT_CAPTURED
```

## Task 6 / 7 — vs CLCL and COHORT_ALIGNED_LIVE (`NOT_CAPTURED`)

```text
MAPPING_COHERENT_VS_CLCL_JOIN_MAX: NOT_CAPTURED
MAPPING_COHERENT_REPRODUCES_CLCL_SAFETY_ON_MISMATCHES: NOT_CAPTURED
MAPPING_COHERENT_PRESERVES_NORMAL_INTERPOLATION_ELSEWHERE: NOT_CAPTURED
CLASS_B_WORLD_PREV_RECOVERABLE: NOT_CAPTURED
```

If class B WORLD PREV is unmappable in VIEW's cohort
(`CLASS_B_WORLD_PREV_RECOVERABLE=NO`) and mapping-coherent
succeeds, the generic fallback is temporal endpoint coherence,
not searching a different workload.

## Task 8 — generic production feasibility (`PROVEN` static)

```text
GENERIC_MAPPING_COHERENCE_FLAGS_AVAILABLE_AT_FIX_LAYER: YES
  ViewProjectionMap.mapped and TransformMap.mapped exist on
  GameFrameMap::WorkloadMap, indexed by viewProj index and world
  index. RSPProcessor already reads both per vertex via
  drawData.viewProjIndices[g] and drawData.worldIndices[g].
  GameCall itself does not store the pair.
BEST_GENERIC_FIX_LAYER: RSPProcessor
POTENTIAL_GENERIC_FIX_DESIGN:
  Per vertex or per GameCall, if VIEW mapped XOR WORLD mapped,
  snap both streams to CURRENT for that sample. Keep LIVE
  projection and LIVE position. Do not snap a shared VIEW
  globally in ProjectionProcessor (that VIEW is reused by
  class-A GameCalls). Do not hardcode Tooie/model/bones.
```

Do **not** implement production until recapture shows:

- `TEMPORAL_COHORT_MISMATCH_CORRELATES_WITH_PATHOLOGY=YES`
- `CLASS_B_MAPPING_ASYMMETRY_EXPLAINS_CATASTROPHIC_GLITCH=YES`
- `MAPPING_COHERENT_LIVE_MATCHES_SAFE_CLASS=YES`
- `MAPPING_COHERENT_PRESERVES_NORMAL_INTERPOLATION_ELSEWHERE=YES`

Then:

```text
ROOT_CAUSE=ASYMMETRIC_TEMPORAL_MAPPING_VIEW_INTERPOLATED_WORLD_CURRENT
```

If correlation fails or mapping-coherent fails: GPT-6 Astra
architecture review.

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
`MAPPING_COHERENT_*`,
`TEMPORAL_COHORT_MISMATCH_CORRELATES_WITH_PATHOLOGY`,
`PATHOLOGICAL_ON_B_*`,
`CLASS_B_MAPPING_ASYMMETRY_EXPLAINS_CATASTROPHIC_GLITCH`,
`CLASS_B_WORLD_PREV_RECOVERABLE`,
`COHORT_ALIGNED_*`,
`MISMATCH_CALL_*`.
