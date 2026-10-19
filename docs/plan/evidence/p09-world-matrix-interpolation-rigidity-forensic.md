---
title: "P09 world-matrix interpolation rigidity forensic"
description: "Production worldMat is RigidBody TRS+SLERP; user recapture REFUTED world non-rigidity as P09 JOIN root."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-14
tags: [tooie-recomp, p09, interpolation, join, rigidity, world, slerp, rt64]
---

# P09 world-matrix interpolation rigidity forensic

Authoritative JOIN recapture (user, complete):

```text
JOIN_DIAGNOSTIC_VALID=YES
JOIN_PAIR_COUNT=80190
JOIN_COINCIDENT_PAIR_COUNT=80190
LIVE_JOIN MAX/P95/P99=1858.22 / 0.164 / 0.648
POLICY_A_JOIN MAX/P95/P99=1.170 / 0.164 / 0.633
LIVE phase MAX/P95/P99=46.393 / 0.079 / 1.434
POLICY_A phase MAX/P95/P99=0.121 / 0.005 / 0.015
BEST_COHERENT_SCOPE=NONE
PER_*_COHERENT_CURRENT_REDUCES_JOIN_ERROR=NO
```

POLICY_A almost eliminates temporal phase disagreement. Ordinary
spatial JOIN barely moves. Coherent CURRENT scopes are closed.

This session audits whether interpolated **world** matrices are
non-rigid intermediates. Production interpolation is **not**
changed. Game not launched.

## Task 1 — static production math (`PROVEN`)

RSPProcessCS `worldMat` is `worldTransforms[transformIndex]`.
`TransformProcessor::upload` fills that buffer from
`lerpWorldTransforms` when a previous frame exists.

`TransformProcessor::process` sets:

```text
prevMatrix = transformMap.rigidBody.lerp(prevFrameWeight, prev, cur, slerp=true)
curMatrix  = transformMap.rigidBody.lerp(curFrameWeight,  prev, cur, slerp=true)
lerpWorldTransforms <- curMatrix
endpointWorld[t*2+0] = uninterpolated PREV
endpointWorld[t*2+1] = uninterpolated CURRENT
```

`RigidBody::lerp`:

- if `lerpDecompose` and both `DecomposedTransform` valid:
  `lerpTransforms` + `recomposeMatrix` (quaternion SLERP when
  `slerp=true`, translation lerp, scale/skew lerp if flags)
- else: `lerpMatrixComponents` = **element-wise 3x3 lerp**
  (`lerpMatrix3x3`) + translation lerp

`TransformGroup.decompose` defaults **true**. Tooie
`tooie_diag_rt64_match_override` is diagnostic-only and returns
false unless secondary-sway SIMPLE is enabled.

RSPProcessCS then does:

```text
tfPos = mul(mul(viewProjMat, worldMat),
            float4(pos - vel * (1 - curFrameWeight), 1))
```

That is **already-interpolated world** plus object-space velocity.
It is not a second element-wise lerp of PREV/CURRENT world on GPU.

```text
WORLD_MATRIX_INTERPOLATION_METHOD:
  CPU RigidBody::lerp(curWeight, prev, cur, slerp=true) uploaded as
  worldMat; GPU multiplies interpolated world * velocity-adjusted pos
WORLD_MATRIX_INTERPOLATION_IS_ELEMENTWISE: NO
  (default decompose path; fallback YES)
WORLD_MATRIX_INTERPOLATION_PRESERVES_RIGIDITY_BY_CONSTRUCTION: NO
  rotation SLERP preserves SO(3) when decompose is valid, but
  scale/skew AUTO follow rotation AUTO (FIXME always interpolate)
  and the invalid-decompose fallback is element-wise 3x3
```

Prior explosion-class work
[[p09-relative-view-world-interpolation-root-cause]] already
`REFUTED` world interpolation as the **explosion** root. This
task is JOIN **spatial** error after POLICY_A phase repair, a
different claim.

## Task 2–5 — endpoint vs LIVE vs RIGID_WORLD

CPU diagnostic (`TOOIE_DIAG_RSP_ENDPOINT_BINDING=1`) now measures
the affine 3x3 of PREV / CURRENT / LIVE / RIGID_WORLD for target
logical bones `79,100,101,102,103,104,105,107,108` (RT64 row
basis, same as `extract3x3` / `decomposeMatrix`).

`RIGID_WORLD` (CPU only): decompose PREV/CURRENT; lerp
translation; shortest-arc SLERP; lerp scale only if an endpoint
is not unit; lerp skew only if an endpoint has skew; recompose.
Live view/projection unchanged. If decompose fails, the pair is
counted invalid (not silently treated as rigid).

JOIN correlation bins: relative error `>0.1 / >0.25 / >0.5 / >1.0`
vs LIVE parent/child orthogonality.

User recapture (`RUNTIME_CPU`, 2026-09-14):

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

LIVE interpolation can introduce some non-rigidity, but it does
not explain JOIN catastrophes. Do not implement a rigid-world
production patch. Next: [[p09-tfpos-factorial-term-attribution]].

Do not treat production `RigidBody::lerp` as already answering
the JOIN question. It can still interpolate scale/skew, fall back
to element-wise 3x3, and interpolates parent/child worlds
independently (joint constraint is a separate hypothesis).

## Task 6 — view (isolated)

Cameras default `projectionDecompose = false` (“usually look
better with simple interpolation”). View uses
`RigidBody::lerp` which then hits `lerpMatrixComponents`.
Projection uses full `lerpMatrix` (element-wise 4x4) when view
linear/angular lerp is on.

```text
VIEW_MATRIX_INTERPOLATION_IS_ELEMENTWISE: YES
VIEW_INTERPOLATION_NONRIGIDITY_MAY_CONTRIBUTE: YES
```

WORLD was not mixed with VIEW in the counterfactual.

## Task 7 — temporal identity

No new bone/world mapping evidence of wrong PREV/CURRENT pairing.
PR22 is not reopened.

```text
PREV_CURRENT_WORLD_SEMANTIC_IDENTITY_STABLE: YES
NEW_TEMPORAL_IDENTITY_MISMATCH_FOUND: NO
```

Status file now also emits mapped/unmapped counts, decompose
valid/invalid, endpoint translation delta, min quat abs-dot.

## Task 8 — RT64 infrastructure

```text
GENERIC_RIGID_INTERPOLATION_INFRASTRUCTURE_AVAILABLE: YES
  DecomposedTransform, decomposeMatrix, lerpTransforms, slerp,
  recomposeMatrix, RigidBody, TransformGroup
BEST_IMPLEMENTATION_LAYER: CPU_TRANSFORM_PROCESSOR
```

Do **not** implement a rigid-world production patch.
`WORLD_NONRIGIDITY_ROOT_CAUSE=REFUTED`. Next is
[[p09-tfpos-factorial-term-attribution]].

## Validation

```text
cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test
TooieRecompiled: linked
window_display_test: 38/38 PASS
tintro_projection_policy_test: 7/7 PASS
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
PRODUCTION_FIX_UPDATED: NO
CPU_DIAG_SHARED_STATE_LOCKED: YES
STATUS_WRITE_ATOMIC: YES
```

`NEXT_SINGLE_ACTION`: superseded by
[[p09-tfpos-factorial-term-attribution]] recapture.
