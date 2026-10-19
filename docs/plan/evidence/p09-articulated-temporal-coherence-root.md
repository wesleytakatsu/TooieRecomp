---
title: "P09 articulated temporal coherence root"
description: "JOIN_PAIR_COUNT=0 dead-gate audit; PER_DRAW envelope independent of JOIN; CPU JOIN pairing repaired."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-14
tags: [tooie-recomp, p09, interpolation, join, articulated, coherency]
---

# P09 articulated temporal coherence root

## JOIN recapture abort (2026-09-14)

Live JOIN builder is VALID (`JOIN_PAIR_COUNT=53946`) but the
process aborted during status write. Forensic:
[[p09-diagnostic-heap-crash-forensic]]. Do not infer
`BEST_COHERENT_SCOPE` from the truncated file. POLICY_A JOIN
outliers drop; ordinary P95/P99 and visual POLICY_A do not
establish a production coherent scope.

## JOIN diagnostic repair (2026-09-14)

Authoritative prior capture: residual VALID, JOIN INVALID.

```text
RESIDUAL_DIAGNOSTIC_VALID=YES
RESIDUAL_TRIANGLE_COUNT=34060
JOIN_PAIR_COUNT=0
JOIN_COINCIDENT_PAIR_COUNT=0
PER_*_CURRENT_VERTEX_PERCENT=NOT_CAPTURED
LIVE_MAX_EDGE_RATIO_VS_ENDPOINT_ENVELOPE=9825.12598
PER_TRANSFORM_MAX_EDGE_RATIO_VS_ENDPOINT_ENVELOPE=9825.12598
COMPONENT_MAX_EDGE_RATIO_VS_ENDPOINT_ENVELOPE=9825.12598
PER_DRAW_MAX_EDGE_RATIO_VS_ENDPOINT_ENVELOPE=1.0262562
PER_DRAW_P99_EDGE_RATIO_VS_ENDPOINT_ENVELOPE=1
```

### Task 1 — why JOIN_PAIR_COUNT was zero

```text
JOIN_PAIR_BUILDER_REACHED: YES
FIRST_JOIN_PAIR_DEAD_GATE: BanjoRecomp/lib/rt64/src/render/rt64_rsp_processor.cpp
  p09RspEndpointBindingNote / per-triangle edge loop
  if (ja.world == jb.world) continue;
  then p09FeatherWorldBit(ja.world)==0
WHY_JOIN_PAIR_COUNT_ZERO: the builder only emitted a pair when one
  residual triangle edge mixed two concrete worldIndices AND both
  indices were the V4 bone IDs 79,100,101,102,103,104,105,107,108.
  Those IDs are bones, not RT64 world slots. V7 already maps
  world→bone from OWN DLs then JOIN DLs. Most V4 faces are
  single-world; V7 JOIN attachment is child G_VTX slots 0-3 plus
  parent slots 28-31 after G_POPMTX (crest 0-4 / 27-31), which need
  not share a triangle.
```

### Task 2 — mixed-triangle assumption

```text
CURRENT_JOIN_DETECTION_ASSUMPTION_VALID: NO
JOIN_NEEDS_CROSS_FACE_OR_CROSS_DRAW_PAIRING: YES
```

V6C/V7 inventory: JOIN DLs `0x54C,0x564,0x592,0x61E,0x5F0,0x5D8,0x4B0`
are the attachment topology. Articulated bones:

- RIGHT `107 -> 103 -> 104 -> 105`
- LEFT `107 -> 100 -> 101 -> 102`
- CREST `108 -> 79`

CPU diagnostic now pairs nearest endpoint-NDC verts across those
parent/child bone lists after world→bone bind. No production
model-specific path.

### Task 3 — PER_DRAW counterfactual with JOIN_PAIR_COUNT=0

```text
PER_DRAW_COUNTERFACTUAL_EXECUTED: YES
PER_DRAW_COUNTERFACTUAL_SELECTION_RULE: if any accepted residual
  vertex of a GameCall has prevW*curW<=0, every vertex of that
  GameCall uses CURRENT clip/NDC; else LIVE. Edge vs envelope is
  then maxEdge(drawN)/max(prevEdge,curEdge) on every residual
  triangle, not on JOIN pairs.
PER_DRAW_RESULT_IS_INDEPENDENT_OF_JOIN_PAIR_BUILDER: YES
PER_DRAW_RESULT_TRUSTWORTHY: NO
  (geometry math for that rule is real; CURRENT-snap percent was
  NOT_CAPTURED because denom required p09FeatherWorldBit(worldIndex).
  PER_DRAW can look near-perfect by snapping whole draws to CURRENT.)
```

### Task 4–5 — snap percent and TRANSFORM/COMPONENT

Previous output could not emit `PER_DRAW_CURRENT_VERTEX_PERCENT`.
TRANSFORM and COMPONENT matched LIVE because `worldNeedsCurrent` /
`p09FeatherComponentMask` keyed concrete world indices as if they
were bones, so those scopes almost never triggered (option B:
grouping identity broken; option C: path applied but identity
never matched). PER_DRAW triggered on W-cross without that gate.

New keys (session + LAST_): `PER_DRAW_TOTAL_VISIBLE_VERTEX_COUNT`,
`PER_DRAW_CURRENT_VERTEX_COUNT`, `PER_DRAW_CURRENT_VERTEX_PERCENT`,
`PER_DRAW_TOTAL_GAMECALL_COUNT`, `PER_DRAW_CURRENT_GAMECALL_COUNT`,
`PER_DRAW_CURRENT_GAMECALL_PERCENT`, `PER_DRAW_IS_EFFECTIVELY_ALL_CURRENT`
(YES if percent>=85), `PER_*_TRIGGER_COUNT`.

```text
PER_DRAW_TOTAL_VISIBLE_VERTEX_COUNT: NOT_CAPTURED (needs recapture)
PER_DRAW_CURRENT_VERTEX_COUNT: NOT_CAPTURED
PER_DRAW_CURRENT_VERTEX_PERCENT: NOT_CAPTURED
PER_DRAW_TOTAL_GAMECALL_COUNT: NOT_CAPTURED
PER_DRAW_CURRENT_GAMECALL_COUNT: NOT_CAPTURED
PER_DRAW_CURRENT_GAMECALL_PERCENT: NOT_CAPTURED
PER_DRAW_IS_EFFECTIVELY_ALL_CURRENT: NOT_CAPTURED
PER_TRANSFORM_TRIGGER_COUNT: NOT_CAPTURED
PER_DRAW_TRIGGER_COUNT: NOT_CAPTURED
COMPONENT_TRIGGER_COUNT: NOT_CAPTURED
PER_TRANSFORM_COUNTERFACTUAL_VALID: NO (identity was broken; repaired, not recaptured)
COMPONENT_COUNTERFACTUAL_VALID: NO (same)
JOIN_DIAGNOSTIC_VALID: NO until recapture shows target geometry AND JOIN_PAIR_COUNT>0
```

### Task 6–8 — repair scope

CPU-only in `rt64_rsp_processor.cpp`. Percentile sort is cached per
accum (one sort per write, not per P50/P95/P99). Production
`RSPProcessCS` unchanged. Do not promote PER_DRAW until recapture
shows JOIN/phase drop, envelope still good, and CURRENT snap is a
limited fraction.

```text
PRODUCTION_POLICY_CHANGED: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
```

Build: `TooieRecompiled`; `window_display_test` 38/38;
`tintro_projection_policy_test` 7/7.

## Capture that closed CLIP_LERP / NDC_LERP (2026-09-14)

Authoritative residual `31720`. User visual unchanged:

```text
NORMAL_HIGH_RATE: GLITCH
SELECTIVE_W_CROSS_CURRENT: FAIL
GPU_W_SIGN_BRANCH_MARKER: PASS
RSPPROCESS_SHADER_LOCAL_ALL_CURRENT: PASS
```

Vs ALL_CURRENT:

```text
LIVE MAX=2496.50366 P95=1.45789349 P99=8.8198452 GT_5=368
POLICY_A MAX=11.0653267 P95=1.30360758 P99=2.31760049 GT_5=15
NDC_LERP MAX=216.515732 P99=4.2963419 GT_5=258
```

Vs endpoint envelope `max(prevEdge, curEdge)`:

```text
POLICY_A MAX=1.46042883 P95=1 P99=1
CLIP_LERP MAX=1.46299326 P95=1 P99=1.02593112
```

```text
ALL_CURRENT_COMPARISON_OVERSTATED_RESIDUAL: YES
ENDPOINT_ENVELOPE_SHOWS_POLICY_A_GEOMETRY_MOSTLY_VALID: YES
CLIP_LERP_RIGID_COHERENCE: BAD
NDC_LERP_RIGID_COHERENCE: BAD
ZERO_CROSS_SCREENSPACE_OVERSHOOT_CORRELATES_WITH_STRETCH: NO
BEST_TEMPORAL_POSITION_POLICY: NONE
CLIP_LERP_NDC_LERP_CLOSED: YES
```

Do **not** implement CLIP_LERP or NDC_LERP. Do **not** hunt a better
independent per-vertex interpolation formula. ALL_CURRENT is
artifact-free but is not the expected interpolated geometry; a
triangle 2× CURRENT is not necessarily invalid if PREVIOUS also has
that size.

The remaining hypothesis is **relative temporal incoherence** between
neighboring articulated pieces (JOIN mixed worlds), not isolated
triangle size vs CURRENT.

## JOIN / phase / coherent-scope instrumentation

CPU-only, same `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1` path in
`BanjoRecomp/lib/rt64/src/render/rt64_rsp_processor.cpp`. No GPU
readback. Production `RSPProcessCS` unchanged.

JOIN pairs = mixed-world edges among logical transforms
`79,100,101,102,103,104,105,107,108` on V4-visible 0x62B triangles.

Relative error = screen-space gap / `max(prevGap, curGap, edgeFloor)`.
Phase = scalar along the endpoint NDC motion segment; JOIN delta is
`abs(parentPhase-childPhase)`.

Offline coherent CURRENT scopes (not applied on GPU):

- VERTEX = POLICY_A
- TRANSFORM = if any vertex of a world crosses, CURRENT for that world
- DRAW = if any vertex of a GameCall crosses, CURRENT for that call
- COMPONENT = right `107-103-104-105`, left `107-100-101-102`,
  crest `108-79` (107 snaps both plumes)

```text
JOIN_PAIR_COUNT: NOT_CAPTURED
LIVE_JOIN_* / POLICY_A_JOIN_* / CURRENT_JOIN_*: NOT_CAPTURED
LIVE_MAX_JOIN_PHASE_DELTA: NOT_CAPTURED
LIVE_P99_JOIN_PHASE_DELTA: NOT_CAPTURED
POLICY_A_MAX_JOIN_PHASE_DELTA: NOT_CAPTURED
POLICY_A_P99_JOIN_PHASE_DELTA: NOT_CAPTURED
ARTICULATED_ATTACHMENT_GAP_OBSERVED: NOT_CAPTURED
ATTACHMENT_GAP_CORRELATES_WITH_USER_GLITCH_CLASS: NOT_CAPTURED
PER_TRANSFORM_COHERENT_CURRENT_REDUCES_JOIN_ERROR: NOT_CAPTURED
PER_DRAW_COHERENT_CURRENT_REDUCES_JOIN_ERROR: NOT_CAPTURED
COMPONENT_COHERENT_CURRENT_REDUCES_JOIN_ERROR: NOT_CAPTURED
BEST_COHERENT_SCOPE: NONE
```

## Generic RT64 grouping (not a production hack)

```text
GENERIC_RT64_GROUPING_AVAILABLE: YES
```

Already present, independent of model 0x62B:

- `TransformGroup` — interpolation policy / `matrixId` / decompose
- `DrawData.worldTransformGroups[worldIndex]` — world → group
- `GameCall` — draw ownership (`minWorldMatrix`/`maxWorldMatrix`,
  `sourceDl`, triangle range)
- per-vertex `worldIndices`

PR22 is grouping/tagging infrastructure only. Do not resurrect it as
the cause. A future coherent-scope fallback could key off GameCall or
worldIndex/TransformGroup without a Kazooie-specific hack.

## Diagnostic timing (Task 7)

`TOOIE_DIAG_RSP_ENDPOINT_BINDING=1` runs on every `RSPProcessor::process`:
walks all vertices and V4-visible triangles, then `fopen`/`fprintf`
and **sorts residual sample vectors for percentiles on every write**.

That sits on the CPU path that produces interpolated frames. RT64
already has `PresentQueue::skipInterpolation` when presents are
skipped. Causality of the user's "worse glitch / larger LIVE
outliers" during this diag is not proven.

```text
CPU_DIAGNOSTIC_CAN_PERTURB_FRAME_TIMING: YES
VISUAL_WORSENING_CAUSED_BY_DIAGNOSTIC: PLAUSIBLE
```

Do **not** use this heavy diagnostic for future visual validation.
CPU metric recapture is still required for JOIN keys.

## Production

```text
PRODUCTION_FIX_UPDATED: NO
PRODUCTION_FIX_CLASS: NONE
NEXT_PRODUCTION_FIX_CLASS: COHERENT_SCOPE_CURRENT_FALLBACK
  (TRANSFORM / DRAW / COMPONENT — pick after JOIN recapture)
CLEANUP_READY: NO
COMMIT_READY: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
```

`NEXT_SINGLE_ACTION`: CPU-only recapture
`TOOIE_DIAG_RSP_ENDPOINT_BINDING=1 ./build/TooieRecompiled`
(normal high-rate walk, 0x62B). Paste `JOIN_DIAGNOSTIC_VALID`,
`JOIN_PAIR_COUNT`, JOIN/phase keys, PER_DRAW CURRENT percents,
`PER_*_TRIGGER_COUNT`, `BEST_COHERENT_SCOPE` from
`/tmp/tooie-rsp-endpoint-binding-status.txt`.
Do not treat that run as visual ground truth.
