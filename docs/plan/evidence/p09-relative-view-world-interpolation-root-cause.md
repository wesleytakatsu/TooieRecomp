---
title: "P09 relative view×world interpolation root cause"
description: "VERSION=3 COMBINED_RELATIVE INVALID; prior coupled-relative label CORRECTED/SUPERSEDED. See projective-W evidence."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-10
tags: [tooie-recomp, p09, kazooie, feather, interpolation, view, world, relative, pr23]
---

# P09 relative view×world interpolation root cause

**CORRECTED/SUPERSEDED (2026-09-10):** VERSION=3 landed.
`RELATIVE_TRANSFORM_ENDPOINTS_VALID=NO` and
`RELATIVE_CLIP_ORDER=NEITHER_MATCHES_LIVE_ENDPOINTS`. Do **not**
treat `INDEPENDENT_VIEW_AND_WORLD_INTERPOLATION_OF_COUPLED_RELATIVE_MOTION`
as proven. `COMBINED_RELATIVE_*` is invalid for causal interpretation.
Canonical continuation:
[[p09-projective-w-and-viewproj-representation-root-cause]].

```text
VERSION3_RELATIVE_COUNTERFACTUAL_VALID: NO
PR23_SIMPLE_FIX_STATUS: REFUTED
LIVE_EXPLOSION_COUNT: 199
SIMPLE_EXPLOSION_COUNT: 199
EXPLOSION_B_CLASS_COUNT: 175
EXPLOSION_C_CLASS_COUNT: 24
ENDPOINT_CLIP_LERP_EXPLOSION_COUNT: 195
CAMERA_MODEL_MOTION_CORRELATED: NO
```

Schema: `P09_RELATIVE_VIEW_WORLD_INTERPOLATION_ROOT_CAUSE` (historical).

No GPU readback, copy, layout, sync, staging, or occlusion query.
No production interpolation change. No live override. No PR22/23
implementation. Game not launched by this agent.

```text
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
WEIGHTS_MUTATED: NO
RENDER_PATH_MUTATED: NO
GENERATED_RECOMPILED_CODE_EDITED: NO
```

Authoritative VERSION=2 capture
(`/tmp/tooie-feather-temporal-coherency-status.txt`):

```text
EXPLOSION_COUNT_LIVE_MODE=51
EXPLOSION_COUNT_SIMPLE_COUNTERFACTUAL=51
MAX_EXPANSION_LIVE_MODE=24.0403
MAX_EXPANSION_SIMPLE_COUNTERFACTUAL=24.4392
LIVE_EXPLOSION_STILL_SIMPLE_COUNT=51
CPU_EXPLOSION_REMOVED_BY_SIMPLE_ANALYSIS=NO
VIEW_MATCH_STABLE=YES
PROJECTION_MATCH_STABLE=YES
ZERO_EFFECTIVE_VELOCITY_COUNT=105885
NONZERO_EFFECTIVE_VELOCITY_COUNT=0
UNEXPECTED_WORLD_TRANSFORM_COUNT=0
CROSS_IDENTITY_COUNT=0
```

## Task 1 — prior labels (`PROVEN` correction)

```text
DECOMPOSE_VS_SIMPLE_CAUSAL_STATUS: REFUTED
PR23_SIMPLE_FIX_STATUS: REFUTED
PR23_SIMPLE_POLICY_CAUSAL_FIX: REFUTED_FOR_CAPTURED_P09_EXPLOSIONS
WORLD_MATRIX_INTERPOLATION_CAUSAL_STATUS: CORRECTED/SUPERSEDED
  (world interpolation alone is not the root)
DECOMPOSE_BREAKS_ARTICULATED_WORLD_RELATIONSHIP: REFUTED
  (as a sufficient explanation of the captured explosions)
CURRENT_ROOT_CLASS: INDEPENDENT_VIEW_AND_WORLD_INTERPOLATION_OF_COUPLED_RELATIVE_MOTION
```

Do not implement PR23 SIMPLE tagging as the production fix.
Do not implement PR22 merely to enable SIMPLE.

## Dual attribution signatures (`PROVEN` on VERSION=2 NDC)

Counterfactuals already in the capture:

- B = CURRENT viewProj + live interpolated world
- C = live interpolated viewProj + CURRENT world
- S = SIMPLE world under live viewProj

Among the 48 dumped expansions, two exclusive patterns:

1. `curId=974` (majority): `ratioB ≈ 0.77–1.10`, `ratioC ≈ 4.8–12.8`,
   `ratioS ≈ live`. Example live=12.198, B=0.978, C=12.822, S=12.155.
   CURRENT viewProj removes the explosion; CURRENT world does not.
2. `curId=1019/1138`: `ratioB ≈ 6.4–6.7`, `ratioC ≈ 0.78–1.006`,
   `ratioS ≈ live`. Example live=6.876, B=6.454, C=1.006, S=6.878.
   CURRENT world removes it; CURRENT viewProj does not.

Hypothesis A (two unrelated classes) is not required. Hypothesis B
(one coupled View×World relative failure whose residual looks like
B or C depending on which independent lerp is the larger error)
is the working class. SIMPLE matching live on both signatures is
the proof that world-lerp policy is not the discriminator.

## Task 2 — RT64 composition (`PROVEN_STATIC`)

Exact RT64 order (`hlslpp::mul` is left×right; translation is row 3):

```text
clip = mul(mul(Q, W), pos)
Q    = mul(V, P)     // ProjectionProcessor
RSP  PROJECTION LOAD  -> proj (or viewproj decompose)
     PROJECTION MUL affine non-identity -> view
```

```text
RELATIVE_TRANSFORM_ENDPOINTS_VALID: NOT_CAPTURED
LIVE_RELATIVE_TRANSFORM_INTERPOLATED_SEPARATELY: YES
```

Live interpolates V and W independently then composes. Combined
R=mul(V,W) endpoints were **not** in the VERSION=2 text dump.

## Tasks 3–6 — offline counterfactuals (`IMPLEMENTED`, capture pending)

Witness schema bumped to `P09_COHERENCY_SCHEMA_VERSION=3`.
Analysis-only. Does not feed render. Computes:

- combined relative SIMPLE/DECOMPOSE of R0=mul(V0,W0), R1=mul(V1,W1)
  then clip with interpolated P (order chosen by endpoint NDC error)
- homogeneous clip lerp of true endpoints (not a production path)
- camera-pose interpolation: C=inverse(V), interpolate C, Vpose=inverse(Cinterp)
- camera vs model translation/rotation deltas on explosion vs sane triangles

```text
LIVE_SEPARATE_VW_EXPLOSION_COUNT: 51
COMBINED_RELATIVE_SIMPLE_EXPLOSION_COUNT: NOT_CAPTURED
COMBINED_RELATIVE_DECOMPOSE_EXPLOSION_COUNT: NOT_CAPTURED
ENDPOINT_CLIP_LERP_EXPLOSION_COUNT: NOT_CAPTURED
CAMERA_POSE_INVERSE_EXPLOSION_COUNT: NOT_CAPTURED
MAX_LIVE_SEPARATE_VW_RATIO: 24.0403
MAX_COMBINED_RELATIVE_SIMPLE_RATIO: NOT_CAPTURED
MAX_COMBINED_RELATIVE_DECOMPOSE_RATIO: NOT_CAPTURED
MAX_ENDPOINT_CLIP_LERP_RATIO: NOT_CAPTURED
MAX_CAMERA_POSE_INVERSE_RATIO: NOT_CAPTURED
RELATIVE_TRANSFORM_INTERPOLATION_MATERIALLY_BETTER: NOT_CAPTURED
CAMERA_POSE_INTERPOLATION_MATERIALLY_BETTER: NOT_CAPTURED
DIRECT_VIEW_MATRIX_INTERPOLATION_EQUIVALENT_TO_CAMERA_POSE_INTERPOLATION: NOT_CAPTURED
CAMERA_MODEL_MOTION_CORRELATED: STRONG_INFERENCE (user walk vs idle; numeric pending v3)
ENDPOINT_RELATIVE_MOTION_SMALLER_THAN_INDEPENDENT_WORLD_CAMERA_MOTION: NOT_CAPTURED
CORRELATION_SUPPORTS_USER_LOCOMOTION_OBSERVATION: STRONG_INFERENCE
NEW_PASSIVE_CAPTURE_REQUIRED: YES
```

## Task 5 — view as inverse camera (`PROVEN_STATIC` + pending numeric)

Tooie `func_800E44FC` builds `V_view = T_S * V_rot` with
`T_S` = uniform inv-scale and `-translation * inv_scale` (camera
translation on the projection-stack view). That is an inverse
camera transform (plus scale).

RT64 `ProjectionProcessor` interpolates that VIEW with
`RigidBody::lerp` (default AUTO → DECOMPOSE). Direct lerp of V is
not equivalent to interpolating C=inverse(V) then inverting, once
translations are large.

```text
VIEW_IS_INVERSE_CAMERA_TRANSFORM: YES
```

## Task 7 — Tooie P/V split (`PROVEN_STATIC`)

`src/game/transform_hooks.cpp` native override:

- `gSPMatrix(P, PROJECTION|LOAD)`
- `gSPMatrix(V_view, PROJECTION|MUL)`
- modelview identity

Comment: split to avoid RT64 combined-viewproj decompose while
keeping guest `D_8012D558`. RT64 then stores a real VIEW and
interpolates it independently of MODELVIEW worlds.

```text
TOOIE_PV_SPLIT_SEMANTICALLY_CORRECT_FOR_RT64_INTERPOLATION: YES
  (P LOAD + affine V MUL is the RT64 view-fill path)
TOOIE_CUSTOM_PROJECTION_PATH_CONTRIBUTES_TO_FAILURE: STRONG_INFERENCE YES
  (camera translation lives in interpolatable VIEW; no Banjo-class
   gEXSetViewMatrixFloat; no projection-group SIMPLE)
```

The split is not a misclassification of P as V. It is a Tooie-native
P/V emission that lets RT64 interpolate camera and character worlds
as independent rigid bodies.

## Task 8 — Banjo comparison (`PROVEN_STATIC`)

Banjo `patches/camera_transform_tagging.c`:

- same style P LOAD + rotation MULs on the projection stack
- `gEXSetViewMatrixFloat` with gameplay camera **world translation**
  (“isolated view matrix … counteract the camera translation”)
- `gEXMatrixGroup(..., G_EX_INTERPOLATE_SIMPLE, ..., G_MTX_PROJECTION, ...)`
  with stable `PROJECTION_GAMEPLAY_TRANSFORM_ID`

`patches/projection_transform_tagging.c` only assigns those IDs.

```text
BANJO_PROJECTION_TAGGING_ALONE_WOULD_FIX_P09: NO
BANJO_HAS_RELEVANT_CAMERA_VIEW_INTERPOLATION_DIFFERENCE: YES
  (gEXSetViewMatrixFloat + SIMPLE projection group; not IDs alone)
```

## Task 10 — production

No production patch. Candidate **class** if VERSION=3 shows combined
relative or camera-pose interpolation collapsing the 51 explosions:

Banjo-class camera/view coupling (`gEXSetViewMatrixFloat` and/or
interpolating the combined relative transform), not world SIMPLE,
not 0x62B special cases, not global CURRENT.

```text
PRODUCTION_FIX_CANDIDATE_FOUND: NO
CLEANUP_READY: NO
COMMIT_READY: NO
```

## Next capture

Rebuild with the v3 witness (protected RT64 dirty tree), then:

```text
rm -f /tmp/tooie-feather-temporal-coherency-status.txt
TOOIE_DIAG_FEATHER_TEMPORAL_COHERENCY=1 ./build/TooieRecompiled
```

Read `VERSION=3` and the `COMBINED_RELATIVE_*` / `CAMERA_POSE_*` /
`ENDPOINT_CLIP_LERP_*` / motion keys. No other diag flags.
