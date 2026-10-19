---
title: "P09 Kazooie feather V3 diagnostic implementation"
description: "Bounded implementation of the 0x62B feather RT64 token-to-vertex witness and scale-normalized rotation telemetry."
status: current
type: evidence
project: TooieRecomp
updated: 2026-09-04
tags: [p09, kazooie, feather, rt64, matrix, vertex, rotation, diagnostic]
---

# P09 Kazooie feather V3 diagnostic implementation

## Scope

This is a diagnostic-only implementation. It does not change animation,
matrix grouping, interpolation, rendering, or GPU behavior. The game was not
run in this round.

## Corrected token identity

The token now stores the two distinct guest pointers:

```text
FLOAT_SOURCE_PTR: mlMtxApply float source at D_8007BCB0
N64_MTX_DEST_PTR: mlMtxApply/F2L destination, encoded by G_MTX
RT64_PRIMARY_KEY: N64_MTX_DEST_PTR == G_MTX matrix operand
```

The previous V2 implementation populated both source and destination with the
G_MTX destination. That was sufficient for its old identity capture but was
semantically incorrect. No address normalization is introduced because the
existing RT64 callback reports the same 32-bit guest address representation as
the G_MTX operand.

## Bounded MATRIX-to-VERTEX binding

The explicit RT64 token remains the primary binding. If a feather `VERTEX`
event arrives without that token, the diagnostic may bind it only when exactly
one pending 0x62B feather token has the same nonzero matrix workload. A zero
vertex workload cannot overwrite the nonzero matrix workload. Ambiguous or
zero-workload fallback candidates are rejected and counted.

The event records the actual vertex start/count, draw range, projection, and
workload fields. This makes the next capture capable of distinguishing “no
vertex event”, “vertex arrived under another context”, and “token reached the
real match stages”.

## Scale-normalized rotation witness

AE8A8 and AE820 now emit bounded quaternion summaries for parent index 25 /
boneAnimID 107, the bone103 local animation quaternion, and the final bone103
matrix. AE820 final matrices are decomposed by basis lengths; the normalized
rotation is converted to a unit quaternion, determinant and orthogonality
error are recorded, and the primary feather uses a consecutive-frame angular
delta. Quaternion hemisphere continuity uses the sign of the dot product with
the previous primary quaternion, so `q` and `-q` cannot create a false jump.

The producer CSV gains:

```text
PARENT_ROTATION_Q,LOCAL_ROTATION_Q,FINAL_ROTATION_Q,
FINAL_UNIFORM_SCALE,ROTATION_DELTA_RAD,
ROTATION_ORTHOGONALITY_ERROR,ROTATION_DETERMINANT,
QUATERNION_SIGN_FLIPPED
```

The implementation remains bounded by the existing producer row cap and is
default-off behind `TOOIE_DIAG_SECONDARY_SWAY=1`.

## Validation

```text
cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test -j2: PASS
window_display_test: 38/38 PASS
tintro_projection_policy_test: 7/7 PASS
git diff --check: PASS
game capture: NOT RUN (required)
GPU readback/synchronization: NO
commit/push: NO
```

## Classification

```text
FEATHER_TOKEN_BOUND_TO_VERTEX: UNKNOWN until the next runtime capture
FEATHER_TOKEN_CAN_REACH_GAMEFRAME_MATCH: YES (path instrumented)
FEATHER_TOKEN_CAN_REACH_MATCHTRANSFORM: YES (existing generic path reused)
TOKEN_BINDING_CONFIDENCE: MEDIUM (static; runtime feather VERTEX still pending)
SCALE_NORMALIZED_ROTATION_CAPTURED: YES
PARENT_CHILD_ROTATION_WITNESS_IMPLEMENTED: YES
REAL_FEATHER_MATCH_WITNESS_IMPLEMENTED: YES
NEXT_CAPTURE_CAN_CLASSIFY_A_B_C: YES
ROOT_CONFIDENCE: MEDIUM; no production fix justified
```

## Next single action

Run one fresh V3 capture with the current executable. Classify whether the
guest normalized rotation is discontinuous, whether RT64 semantic matching is
incorrect, or whether both are correct and the defect is later in rendering.
