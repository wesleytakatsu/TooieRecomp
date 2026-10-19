---
title: "P09 exact-order validated camera-pose VIEW arm"
description: "CPU EXACT_ORDER_VALIDATED_CAMERA_POSE_VIEW on the same LLLL samples. Recapture required. No production fix."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-15
tags: [tooie-recomp, p09, interpolation, camera, pose, slerp, join, view]
---

# P09 exact-order validated camera-pose VIEW arm

## Supersession notice (2026-09-15)

See [[p09-cpu-shader-matrix-semantics-and-homogeneous-audit]]. The capture
now exists: 80535 samples, 53235 accepted, 27300 validator rejects,
0/152 apparent pathological fixes. However, static SPIR-V proves that
the claimed production-equivalent clip equation below is incorrect on
CPU row storage. The endpoint check measures double inversion, not the
actual recomposed pose endpoints. Historical NOT_CAPTURED fields and
recapture instructions below describe the implementation checkpoint only.
Repair existing diagnostic semantics before another capture; no new arm
or production fix is authorized by this evidence.

Role: `P09_IMPLEMENT_EXACT_ORDER_VALIDATED_CAMERA_POSE_VIEW`.
Authoritative prior review: [[p09-independent-architecture-root-cause-review]].
`ROOT_CAUSE_CLASS=UNRESOLVED`. Do **not** implement production.

## What was implemented

One default-off CPU arm beside the existing endpoint-binding factorial,
gated by `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1`. Shader and production
interpolation are unchanged.

For each accepted target sample the arm reuses the LLLL factorial
identity:

- raw mapped PREV VIEW (`p09PrevView` = previous workload
  `viewTransforms[prevIndex]`, not processed display-sample matrices)
- raw CURRENT VIEW (`viewTransforms[vp]`)
- LIVE PROJECTION
- LIVE WORLD
- LIVE position
- workload / GameCall / viewProj index / world index / `curFrameWeight`

It does **not** reuse RT64's bundled quaternion helper as the SLERP
control. The existing `1-weight` compensation is left untouched.

## Camera validation

`Cprev = inverse(PREV_VIEW)`, `Ccur = inverse(CURRENT_VIEW)`.

A sample is valid only if both inverses are finite, both poses are
row-affine, rotation determinant is proper and positive, scale is
approximately unit, and skew/perspective are negligible. Invalid
samples are excluded from pose-policy JOIN/envelope/phase results and
counted in coverage. There is no silent normalize and no matrix-lerp
fallback.

## Pose interpolation and exact order

Valid poses interpolate translation linearly and rotation with the
standard two-coefficient shortest-arc SLERP (nlerp when quaternions
are nearly identical). `Cpose(w)` is inverted to
`POSE_DERIVED_LIVE_VIEW`. Clip evaluation is only:

```text
((POSE_DERIVED_LIVE_VIEW * LIVE_PROJECTION) * LIVE_WORLD) * LIVE_POS4
```

`POSE_VIEW_PRODUCTION_ORDER_PRESERVED=YES` is hardcoded by that
expression. WORLD is not replaced. Projection is not moved.

The reference SLERP helper is asserted on a synthetic 0→120° Z
rotation at w = 0, 0.25, 0.5, 0.75, 1 before diagnostic use.

## Same pairing and pathology label

Pose NDC positions feed the existing JOIN pairing and
`p09NoteJoinMetrics` / envelope / phase helpers. Safe-class uses the
existing `matchesAllCurrent` comparator. Pathology is the existing
label: LLLL gap ratio ≥ 1.5 and CLCL ≤ 1.05.

Auxiliary diagnostics only (never fed to clip):

- physical `Rlive = Wlive * Vlive`, `Rpose = Wlive * Vpose`
- full homogeneous `WORLD_H = LIVE_WORLD * pos4` including w
- WORLD rotation law delta: RT64 `lerpTransforms` vs standard SLERP
  on the same PREV/CURRENT world endpoints (observation only; no WORLD
  counterfactual)

## Capture keys (not yet captured)

```text
POSE_VIEW_REFERENCE_SLERP_ENDPOINTS_CORRECT: NOT_CAPTURED
POSE_VIEW_REFERENCE_SLERP_MIDPOINT_CORRECT: NOT_CAPTURED
POSE_VIEW_TOTAL_SAMPLE_COUNT: NOT_CAPTURED
POSE_VIEW_VALID_SAMPLE_COUNT: NOT_CAPTURED
POSE_VIEW_INVALID_INVERSE_COUNT: NOT_CAPTURED
POSE_VIEW_NONRIGID_CAMERA_COUNT: NOT_CAPTURED
POSE_VIEW_ENDPOINTS_MATCH: NOT_CAPTURED
POSE_VIEW_PRODUCTION_ORDER_PRESERVED: YES
POSE_VIEW_JOIN_MAX: NOT_CAPTURED
POSE_VIEW_JOIN_P99: NOT_CAPTURED
POSE_VIEW_MAX_JOIN_PHASE_DELTA: NOT_CAPTURED
POSE_VIEW_P99_JOIN_PHASE_DELTA: NOT_CAPTURED
POSE_VIEW_MAX_EDGE_RATIO_VS_ENDPOINT_ENVELOPE: NOT_CAPTURED
POSE_VIEW_P99_EDGE_RATIO_VS_ENDPOINT_ENVELOPE: NOT_CAPTURED
POSE_DERIVED_VIEW_MATCHES_SAFE_CLASS: UNKNOWN_NOT_CAPTURED
LLLL_POSE_TEST_PATHOLOGICAL_COUNT: NOT_CAPTURED
POSE_VIEW_PATHOLOGICAL_VALID_COUNT: NOT_CAPTURED
POSE_VIEW_PATHOLOGICAL_INVALID_COUNT: NOT_CAPTURED
POSE_VIEW_PATHOLOGICAL_FIXED_COUNT: NOT_CAPTURED
POSE_VIEW_PATHOLOGICAL_NOT_FIXED_COUNT: NOT_CAPTURED
POSE_VIEW_PATHOLOGICAL_COVERAGE_PERCENT: NOT_CAPTURED
RELATIVE_CAMERA_OBJECT_PATH_DEVIATION_CORRELATES_WITH_PATHOLOGY: UNKNOWN_NOT_CAPTURED
WORLD_HOMOGENEOUS_W_CORRELATES_WITH_PATHOLOGY: UNKNOWN_NOT_CAPTURED
WORLD_SLERP_LAW_DELTA_DEG_P95: NOT_CAPTURED
WORLD_SLERP_LAW_DELTA_DEG_P99: NOT_CAPTURED
PATHOLOGICAL_WORLD_SLERP_LAW_DELTA_DEG_P95: NOT_CAPTURED
NONSTANDARD_WORLD_SLERP_LAW_CORRELATES_WITH_PATHOLOGY: UNKNOWN_NOT_CAPTURED
ROOT_CAUSE_CLASS: UNRESOLVED
```

If pose JOIN matches safe class with high/full pathological coverage,
direct VIEW-matrix interpolation is strongly supported as a sufficient
trigger for this CPU population. Do not immediately modify production;
return the smallest generic design for review after capture.

If pose JOIN is not safe class with strong valid coverage, camera-pose
reconstruction is not sufficient. Keep `ROOT_CAUSE_CLASS=UNRESOLVED`
and rank remaining hypotheses: coupled VIEW/WORLD interior motion law,
nonstandard quaternion interpolation law, homogeneous/space
interpretation. Do not add another counterfactual automatically.

## Closed directions

Do not reopen: hierarchy, W policy, mapping-coherent production,
CLCL production, CLIP_LERP, NDC_LERP, RT64 quaternion helper
endpoint reversal, WORLD standard-SLERP counterfactual, GPU
readback.

## Validation this session

```text
Tooie HEAD: ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf
RT64 HEAD:  6f1c2d99a4ea571c139f449c326fd176ba8f3496
cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test
TooieRecompiled: link OK (rt64_rsp_processor.cpp rebuilt)
window_display_test: 38/38 PASS
tintro_projection_policy_test: 7/7 PASS
git diff --check Tooie intended docs/header: PASS
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

## Next single action

```text
TOOIE_DIAG_RSP_ENDPOINT_BINDING=1 ./build/TooieRecompiled
```

Paste `/tmp/tooie-rsp-endpoint-binding-status.txt` keys listed above.
No GPU readback. No commit.
