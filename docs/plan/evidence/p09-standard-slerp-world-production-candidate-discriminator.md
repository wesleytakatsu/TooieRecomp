---
title: "P09 standard SLERP WORLD production-candidate discriminator"
description: "CPU-only STANDARD_SLERP_WORLD arm on repaired V2 pairing. Rotation-only standard SLERP; other TRS factors match production. No production change. Recapture required."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-16
tags: [p09, rt64, interpolation, world, slerp, quaternion, join, diagnostic]
---

# P09 standard SLERP WORLD production-candidate discriminator

Role: `P09_STANDARD_SLERP_WORLD_PRODUCTION_CANDIDATE_DISCRIMINATOR`.

Bounded objective: one isolated CPU counterfactual on already-repaired V2
shader-equivalent pairing. No production interpolation change. No game
launch in this task.

Authoritative status schema: `P09_RSP_ENDPOINT_BINDING_V2_SHADER_EQUIVALENT`.

## Prior capture used as motivation (not re-measured here)

User recapture of `ELEMENTWISE_WORLD_LERP` (`PROVEN`, `USER_OBSERVED`):

```text
ROOT_CAUSE_CLASS=
NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS

LIVE_WORLDSPACE_JOIN_MAX=5.45340872
LIVE_WORLDSPACE_JOIN_P99=1.1533041
ELEMENTWISE_WORLDSPACE_JOIN_MAX=1.00010824
ELEMENTWISE_WORLDSPACE_JOIN_P99=0.999981344
ELEMENTWISE_WORLD_POINT_LINEARITY_PROVEN=YES
ELEMENTWISE_WORLD_PATHOLOGICAL_TOTAL=156
ELEMENTWISE_WORLD_PATHOLOGICAL_FIXED_COUNT=156
ELEMENTWISE_WORLD_PATHOLOGICAL_NOT_FIXED_COUNT=0
ELEMENTWISE_WORLD_FIXES_ALL_CORRECTED_PATHOLOGIES=YES
ELEMENTWISE_WORLD_MATERIALLY_REDUCES_ARTICULATED_GAP=YES
ELEMENTWISE_WORLD_INTRODUCES_MATERIAL_NONRIGIDITY=YES

WORLD_SLERP_LAW_DELTA_DEG_P50=0
WORLD_SLERP_LAW_DELTA_DEG_P95=0.0395646803
WORLD_SLERP_LAW_DELTA_DEG_P99=0.0559529066
WORLD_SLERP_LAW_DELTA_DEG_MAX=9.12444973
PATHOLOGICAL_WORLD_SLERP_LAW_DELTA_DEG_P50=0.0559529066
PATHOLOGICAL_WORLD_SLERP_LAW_DELTA_DEG_P95=2.74147248
PATHOLOGICAL_WORLD_SLERP_LAW_DELTA_DEG_MAX=9.12273312
NONSTANDARD_WORLD_SLERP_LAW_CORRELATES_WITH_PATHOLOGY=YES
```

Do **not** promote global elementwise WORLD interpolation to production.
The quaternion-law correlation is not yet a sufficient cause. This task
isolates rotation interpolation only.

## Implementation (CPU diagnostic only)

RT64 file: `BanjoRecomp/lib/rt64/src/render/rt64_rsp_processor.cpp`.

Reference SLERP (`p09StandardSlerp`) is explicit shortest-arc
constant-speed interpolation. It does **not** call `hlslpp::slerp`.
Offline analytic check at `w={0,0.25,0.5,0.75,1}` on a 120° Z rotation:

```text
STANDARD_WORLD_SLERP_REFERENCE_VALID=YES
p09_cpu_shader_clip_semantics: 14/14 passed
```

`STANDARD_SLERP_WORLD` for each PREV/CURRENT WORLD pair and
`curFrameWeight`:

1. Production `DecomposedTransform` of both endpoints.
2. Production `RigidBody` coordinate-flip bias when determinants disagree.
3. Translation / scale / skew / perspective from `lerpTransforms` with
   `lerpRotation=false` (same `weight` as production non-rotation factors).
4. Rotation **only**: `p09StandardSlerp(q0,q1,weight)`.
5. Production `recomposeMatrix`.

If either decompose is invalid, the arm does not invent a quaternion; it
records a decompose-fail and does not count that sample as a standard-SLERP
JOIN/pathology success.

Clip (shader-equivalent V2):

```text
Qlive = mul(LIVE_VIEW, LIVE_PROJECTION)
clip  = mul(LIVE_POS4, mul(STANDARD_SLERP_WORLD, Qlive))
STANDARD_SLERP_WORLD_CLIP_SEMANTICS=
ROW_VECTOR_POS_TIMES_STANDARD_SLERP_WORLD_TIMES_LIVE_VIEWPROJ
```

Physical attachment:

```text
parentH = mul(parentLocalPos4, STANDARD_SLERP_PARENT_WORLD)
childH  = mul(childLocalPos4, STANDARD_SLERP_CHILD_WORLD)
```

LIVE VIEW, LIVE PROJECTION, LIVE POSITION are unchanged.

## Decision after recapture (do not implement production then)

- **CASE A**: fixes all corrected pathologies AND world-space JOIN max
  ≤ 1.05 AND rigidity substantially better than elementwise
  → `ROOT_CAUSE_CLASS=NONSTANDARD_QUATERNION_INTERPOLATION_IN_COMPOSED_WORLD_IS_DOMINANT_TRIGGER`
- **CASE B**: improves JOIN/pathology but does not fix essentially all
  → keep `NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`;
  quaternion law is a contributor, not the root
- **CASE C**: little effect → same broad root class; no new causal arm

## Production option audit (static; no option shipped)

Call graph (`PROVEN`, `OTHER`, static):

- WORLD live matrix: `TransformProcessor::process` →
  `TransformMap.rigidBody.lerp(curFrameWeight, prev, cur, slerp=true)` →
  `lerpTransforms` → `hlslpp::slerp(a, ±b, 1-weight)` when decompose valid.
- VIEW/camera: `GameFrame` defaults `projectionDecompose=false` unless a
  non-AUTO/IGNORE `matrixId` is present. Default camera path is
  `lerpMatrixComponents` (elementwise), **not** the quaternion helper.
- `TransformGroup.decompose` defaults `true` for WORLD groups and is set
  from GBI `matrixId(...)`.
- No generic parent WORLD hierarchy is interpolated in
  `TransformProcessor`. Previous hierarchy CPU arm is superseded.

| Option | Generic data already available? | Blast radius | Game-specific knowledge? | Preserves attachment mathematically? | Preserves rigid-body quality? |
| --- | --- | --- | --- | --- | --- |
| **A** Correct standard SLERP in current decomposed WORLD path | Yes: `lerpTransforms` / `RigidBody::lerp` / decompose | All decompose-valid `RigidBody::lerp` callers. VIEW default is decompose-off, so cameras stay on elementwise unless a projection group opts in | No | **No** in general: independently interpolated composed WORLDs remain nonlinear in parent∘child | Yes when scale/skew stay near unit |
| **B** Elementwise composed-WORLD interpolation | Yes: 16-entry lerp already used as decompose-fail fallback and as default VIEW | `lerpWorldTransforms` for decompose-valid WORLD groups | No | **Yes** (`ELEMENTWISE_WORLD_POINT_LINEARITY_PROVEN`) | **No** (material nonrigidity `PROVEN` on last capture) |
| **C** Selective `decompose=false` / matrix-lerp metadata | Yes: `TransformGroup.decompose` and component AUTO/INTERPOLATE flags | Per group; cameras already use this default | Needs a **generic** attachment/articulation rule, not Tooie/0x62B/bone hardcodes | Yes for groups switched to matrix lerp | Mixed |
| **D** Local/hierarchical interpolation | **No** generic parent WORLD chain in RT64 transform metadata | Would invent hierarchy | Would require game-specific bone trees | Yes if parent/child locals were interpolated | Yes if locals are rigid |

Do **not** choose D. Do not hardcode Tooie, model `0x62B`, bones, DLs, or feathers.

`BEST_GENERIC_PRODUCTION_OPTION=UNRESOLVED_PENDING_CAPTURE` until the
STANDARD_SLERP recapture. If CASE A, the smallest generic production
change is **A restricted to WORLD `RigidBody::lerp` / `lerpTransforms`
rotation**, not a global hlslpp helper change that could later affect a
decompose-on camera group.

## Offline validation (this task)

```text
TOOIE_HEAD: ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf (dirty; pre-existing)
RT64:       6f1c2d99a4ea571c139f449c326fd176ba8f3496 (dirty; pre-existing)
BANJORECOMP: ec859632cfa584e2272d32e46051543429a2baf4
rt64_rsp_processor.cpp SHA-256:
  3b983f56d76d42db12d3103c926d0394b5b5afc68aa0012071bce2da64fd5de1
cmake --build build --target TooieRecompiled                 exit 0
cmake --build build --target window_display_test \
        tintro_projection_policy_test                         exit 0
./build/window_display_test                                   38/38 PASS
./build/tintro_projection_policy_test                         7/7 PASS
./build/p09_cpu_shader_clip_semantics_test                    14/14 PASS
git diff --check (intended Tooie/RT64 paths)                  empty
GAME_LAUNCHED=NO
GPU_READBACK_USED=NO
GPU_SYNCHRONIZATION_ADDED=NO
PRODUCTION_FIX_UPDATED=NO
```

Production files not modified: `RSPProcessCS.hlsl`,
`TransformProcessor` interpolation, `ProjectionProcessor` interpolation,
`rt64_math.cpp` `lerpTransforms`, `hlslpp` quaternion helper.

## Runtime keys

STANDARD_SLERP_* capture metrics are `NOT_CAPTURED` until one V2 recapture.

```text
STANDARD_WORLD_SLERP_REFERENCE_VALID=YES
STANDARD_SLERP_WORLD_CLIP_SEMANTICS=
ROW_VECTOR_POS_TIMES_STANDARD_SLERP_WORLD_TIMES_LIVE_VIEWPROJ
STANDARD_SLERP_WORLDSPACE_JOIN_MAX=NOT_CAPTURED
STANDARD_SLERP_WORLDSPACE_JOIN_P99=NOT_CAPTURED
STANDARD_SLERP_WORLD_JOIN_MAX=NOT_CAPTURED
STANDARD_SLERP_WORLD_JOIN_P99=NOT_CAPTURED
STANDARD_SLERP_WORLD_PATHOLOGICAL_TOTAL=NOT_CAPTURED
STANDARD_SLERP_WORLD_PATHOLOGICAL_FIXED_COUNT=NOT_CAPTURED
STANDARD_SLERP_WORLD_PATHOLOGICAL_NOT_FIXED_COUNT=NOT_CAPTURED
STANDARD_SLERP_WORLD_PATHOLOGICAL_FIX_RATE=NOT_CAPTURED
STANDARD_SLERP_WORLD_FIXES_ALL_CORRECTED_PATHOLOGIES=NOT_CAPTURED
STANDARD_SLERP_WORLD_MATERIALLY_REDUCES_ARTICULATED_GAP=NOT_CAPTURED
STANDARD_SLERP_WORLD_MAX_ORTHOGONALITY_ERROR=NOT_CAPTURED
STANDARD_SLERP_WORLD_MAX_SCALE_DEVIATION=NOT_CAPTURED
STANDARD_SLERP_WORLD_MAX_SHEAR=NOT_CAPTURED
STANDARD_SLERP_WORLD_DETERMINANT_RANGE=NOT_CAPTURED
STANDARD_SLERP_WORLD_PRESERVES_RIGIDITY_BETTER_THAN_ELEMENTWISE=NOT_CAPTURED
ROOT_CAUSE_CLASS=
NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS
BEST_GENERIC_PRODUCTION_OPTION=UNRESOLVED_PENDING_CAPTURE
PRODUCTION_FIX_UPDATED=NO
```

## Next

```text
USER_TEST_COMMAND:
TOOIE_DIAG_RSP_ENDPOINT_BINDING=1 ./build/TooieRecompiled
NEXT_SINGLE_ACTION: one V2 recapture containing STANDARD_SLERP_WORLD
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
CLEANUP_READY: NO
COMMIT_READY: NO
NO_COMMIT: YES
NO_PUSH: YES
```
