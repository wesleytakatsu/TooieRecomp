---
title: "P09 elementwise WORLD interpolation causal discriminator"
description: "ELEMENTWISE_WORLD_LERP V2 recapture: linearity proven, all corrected pathologies fixed, material nonrigidity. Do not ship elementwise WORLD."
status: captured
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-16
tags: [p09, rt64, interpolation, world, elementwise, join, diagnostic]
---

# P09 elementwise WORLD interpolation causal discriminator

Role: `P09_ELEMENTWISE_WORLD_INTERPOLATION_CAUSAL_DISCRIMINATOR`.

Bounded objective: one CPU-only counterfactual on already-repaired V2
shader-equivalent pairing. No production interpolation change. No game
launch in this task.

Authoritative status schema: `P09_RSP_ENDPOINT_BINDING_V2_SHADER_EQUIVALENT`.
Old CPU matrix semantics remain superseded.

## Prior capture used as motivation (not re-measured here)

User recapture after the shader-equivalent repair:

```text
STRONGEST_SINGLE_FACTOR=WORLD
FACTORIAL_BEST_POLICY=LLCL
LLLL_PATHOLOGICAL_SAMPLE_COUNT=144
LLLL_PATHOLOGICAL_FIXED_BY_CLLL_COUNT=0
LLLL_PATHOLOGICAL_FIXED_BY_LLCL_COUNT=144
LIVE_WORLDSPACE_JOIN_MAX=3.80023479
PREV/CURRENT_WORLDSPACE_JOIN_MAX=1
WORLD_ATTACHMENT_NONAFFINE_SAMPLE_COUNT=0
LIVE_WORLD_INTERPOLATION_BREAKS_ARTICULATED_ATTACHMENTS=YES
```

CURRENT WORLD alone fixed every corrected pathological sample. Physical
WORLD action is `worldPoint = mul(pos4, WORLD)`. Hypothesis under test:
independent nonlinear TRS/quaternion interpolation of already-composed
WORLD matrices breaks parent/child attachments that agree at both
endpoints.

This task does **not** reopen hierarchy, MODELVIEW, W-threshold, camera-pose,
CLIP_LERP/NDC_LERP, mapping/cohort, or GPU diagnostics.

## Implementation (CPU diagnostic only)

RT64 file: `BanjoRecomp/lib/rt64/src/render/rt64_rsp_processor.cpp`.

For every relevant raw PREV/CURRENT WORLD pair:

```text
ELEMENTWISE_WORLD[i][j] =
    (1 - curFrameWeight) * PREV_WORLD[i][j]
  + curFrameWeight       * CURRENT_WORLD[i][j]
```

No decompose. No quaternion. No `RigidBody::lerp`. Affine remainder is
counted with last-column tolerance `kP09WorldAffineWEps`.

Clip (shader-equivalent V2):

```text
Qlive = mul(LIVE_VIEW, LIVE_PROJECTION)
clip  = mul(LIVE_POS4, mul(ELEMENTWISE_WORLD, Qlive))
ELEMENTWISE_WORLD_CLIP_SEMANTICS=
ROW_VECTOR_POS_TIMES_ELEMENTWISE_WORLD_TIMES_LIVE_VIEWPROJ
```

Physical attachment:

```text
parentH = mul(parentLocalPos4, ELEMENTWISE_PARENT_WORLD)
childH  = mul(childLocalPos4, ELEMENTWISE_CHILD_WORLD)
```

Linearity check on each corrected attachment endpoint:

```text
expected    = lerp(mul(pos4, PREV_WORLD), mul(pos4, CUR_WORLD), t)
elementwise = mul(pos4, ELEMENTWISE_WORLD)
```

Scale-aware tolerance: `max(1e-4, 1e-5 * mag)`.

Status ~1 Hz, bounded accumulators, mutex, atomic tmp+rename. No
per-vertex stdout. No GPU readback/sync/query.

## Offline validation (this task)

```text
TOOIE_HEAD: ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf (dirty; pre-existing)
RT64:       6f1c2d99a4ea571c139f449c326fd176ba8f3496 (dirty; pre-existing)
BANJORECOMP: ec859632cfa584e2272d32e46051543429a2baf4
rt64_rsp_processor.cpp SHA-256:
  858c462fe16cfca59b55ba38c6ca31e7bd3bd164e28c404343ac79ab6eb0f329
cmake --build build --target TooieRecompiled                 exit 0
cmake --build build --target window_display_test \
        tintro_projection_policy_test                         exit 0
./build/window_display_test                                   38/38 PASS
./build/tintro_projection_policy_test                         7/7 PASS
git diff --check (intended Tooie/RT64 paths)                  empty
GAME_LAUNCHED=NO
GPU_READBACK_USED=NO
GPU_SYNCHRONIZATION_ADDED=NO
PRODUCTION_FIX_UPDATED=NO
```

Production files not modified by this task: `RSPProcessCS.hlsl`,
`TransformProcessor` interpolation, `ProjectionProcessor` interpolation.

## Recapture (2026-09-16, USER_OBSERVED)

```text
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
ROOT_CAUSE_CLASS=
NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS
```

Do **not** promote global elementwise WORLD interpolation. Next isolated
arm: [[p09-standard-slerp-world-production-candidate-discriminator]].

## Runtime keys (historical pre-recapture)

```text
ELEMENTWISE_WORLD_CLIP_SEMANTICS=
ROW_VECTOR_POS_TIMES_ELEMENTWISE_WORLD_TIMES_LIVE_VIEWPROJ
ELEMENTWISE_WORLD_POINT_LINEARITY_MAX_DIFF=NOT_CAPTURED
ELEMENTWISE_WORLD_POINT_LINEARITY_PROVEN=NOT_CAPTURED
ELEMENTWISE_WORLDSPACE_JOIN_MAX=NOT_CAPTURED
ELEMENTWISE_WORLDSPACE_JOIN_P95=NOT_CAPTURED
ELEMENTWISE_WORLDSPACE_JOIN_P99=NOT_CAPTURED
ELEMENTWISE_WORLD_JOIN_MAX=NOT_CAPTURED
ELEMENTWISE_WORLD_JOIN_P99=NOT_CAPTURED
ELEMENTWISE_WORLD_MAX_JOIN_PHASE_DELTA=NOT_CAPTURED
ELEMENTWISE_WORLD_P99_JOIN_PHASE_DELTA=NOT_CAPTURED
ELEMENTWISE_WORLD_MAX_EDGE_RATIO_VS_ENDPOINT_ENVELOPE=NOT_CAPTURED
ELEMENTWISE_WORLD_P99_EDGE_RATIO_VS_ENDPOINT_ENVELOPE=NOT_CAPTURED
ELEMENTWISE_WORLD_PATHOLOGICAL_TOTAL=NOT_CAPTURED
ELEMENTWISE_WORLD_PATHOLOGICAL_FIXED_COUNT=NOT_CAPTURED
ELEMENTWISE_WORLD_PATHOLOGICAL_NOT_FIXED_COUNT=NOT_CAPTURED
ELEMENTWISE_WORLD_PATHOLOGICAL_FIX_RATE=NOT_CAPTURED
ELEMENTWISE_WORLD_FIXES_ALL_CORRECTED_PATHOLOGIES=NOT_CAPTURED
ELEMENTWISE_WORLD_MATERIALLY_REDUCES_ARTICULATED_GAP=NOT_CAPTURED
ELEMENTWISE_WORLD_MAX_ORTHOGONALITY_ERROR=NOT_CAPTURED
ELEMENTWISE_WORLD_MAX_SCALE_DEVIATION=NOT_CAPTURED
ELEMENTWISE_WORLD_MAX_SHEAR=NOT_CAPTURED
ELEMENTWISE_WORLD_DETERMINANT_RANGE=NOT_CAPTURED
ELEMENTWISE_WORLD_INTRODUCES_MATERIAL_NONRIGIDITY=NOT_CAPTURED
ROOT_CAUSE_CLASS=UNRESOLVED
```

Decision after recapture (do not implement production in that recapture):

- If linearity `YES` AND gap reduction `YES` AND all 144 pathologies fixed
  `YES` →
  `ROOT_CAUSE_CLASS=NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`
- Else → `ROOT_CAUSE_CLASS=UNRESOLVED` and architecture review before another
  causal arm.

## Production options (not chosen, not implemented)

Only discuss after a successful ELEMENTWISE recapture:

- **A)** Elementwise WORLD interpolation. Generic, preserves linear attachment
  constraints, can introduce shear/scale wander (measure, do not auto-reject).
- **B)** Local/hierarchical transform interpolation **if** a generic parent
  chain exists in RT64 metadata. Previous hierarchy arm was invalid
  column-convention code and is not this test.
- **C)** Selective interpolation policy from transform metadata (decompose
  off, skip slerp, or elementwise) for articulated/attachment WORLD pairs.

Do not pick or ship one unless a later task finds one mechanically obvious
and generic.

## Next

```text
USER_TEST_COMMAND:
TOOIE_DIAG_RSP_ENDPOINT_BINDING=1 ./build/TooieRecompiled
NEXT_SINGLE_ACTION: one recapture of the V2 diagnostic containing ELEMENTWISE_WORLD_LERP
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
CLEANUP_READY: NO
COMMIT_READY: NO
NO_COMMIT: YES
NO_PUSH: YES
```
