---
title: "P09 V5 real call and matchTransform witness"
description: "Snout V5 capture: 5/5 call match, 34 real matchTransform, no proven AUTO identity failure."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-03
tags: [p09, rt64, transform-interpolation, runtime-witness]
---

# P09 V5 real call and matchTransform witness

Canonical reusable facts: [[../../reverse-engineering/systems/transform-interpolation]].
Banjo snout is closed as the primary P09 witness. Kazooie feathers:
[[p09-kazooie-carry-banjo-head-feathers]].

## PROVEN — workload binding and real RT64 stages

The V5 capture postdates the rebuilt executable and reports
`P09_SECONDARY_SWAY_SINGLE_MTX_IDENTITY_V5`. The live Banjo snout witness is
resource `0x62B`, node `+0x11CB8`, and follows
`DCF48 -> ADE2C -> func_80019AA0 -> guMtxF2L -> G_MTX`.

`RSP::matrixCommon` supplies `state->workloadId`, while
`RSP::setVertexCommon` can supply `workload.workloadId == 0` at this seam.
The matrix workload is the temporal-match identity; a zero vertex workload
must remain separate and must not overwrite it.

```text
MATRIX_WORKLOAD_NONZERO: 2379
VERTEX_WORKLOAD_ZERO: 2379
VALID_WORKLOAD_PRESERVED: 2379
VALID_WORKLOAD_OVERWRITE_BLOCKED: 2379
BOUND_TO_GAMEFRAME_MATCH: 39
BOUND_TO_CALL_STAGE: 3
BOUND_TO_TRANSFORM_PREREQUISITE: 3
BOUND_TO_CANDIDATE_STAGE: 39
BOUND_TO_MATCHTRANSFORM: 34
REAL_CALL_MATCH_RATE: 5/5
TRANSFORM_PREREQUISITE: 5/5 eligible
REAL_CANDIDATES: 77 valid
REAL_MATCHED: 34
REAL_UNMATCHED: 5 ONE_TO_ONE_PREVIOUS_ALREADY_USED
PROVEN_AUTO_IDENTITY_FAILURE: NO
PROVEN_INTERPOLATION_FAILURE: NO
BANJO_SNOUT_ROOT: NO_P09_CAUSAL_FAILURE_OBSERVED
MISSING_GROUP_CAUSAL_STATUS: HYPOTHESIS
```

Two selected rows whose diagnostic call/range differs from the previous
row are context aliases, not proven transform cross-matches.

No V6 snout diagnostic. No Banjo-style matrix groups from this capture.
