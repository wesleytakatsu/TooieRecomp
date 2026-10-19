---
title: "P09 independent architecture root cause review"
description: "Exact-order temporal semantics, camera pose, quaternion law and homogeneous metric audit. Visual root unresolved; one CPU pose discriminator specified."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-15
tags: [p09, rt64, interpolation, camera, architecture, forensic]
---

# P09 independent architecture root cause review

## Supersession notice (2026-09-15)

See [[p09-cpu-shader-matrix-semantics-and-homogeneous-audit]]. Static
SPIR-V and independent CPU tests prove that this review's assumed CPU
clip order/orientation was not shader-equivalent. Its metric-based VIEW
attribution and proposed pose discriminator are superseded as shader
causal evidence. Quaternion-law observations remain separate mathematical
findings. Preserve this document as provenance; use the new audit's next
action: repair measurement semantics before more captures.

Role: `P09_INDEPENDENT_ARCHITECTURE_ROOT_CAUSE_REVIEW`.
Bounded objective: P09-RES-05 architecture reconstruction; no production or
runtime diagnostic implementation, no game launch, no GPU operations.

## Verdict

`ROOT_CAUSE_CLASS=UNRESOLVED`.

The capture establishes a VIEW-dominated temporal interaction, **not** a
proven camera-pose defect. CLCL removes both non-current transform deviations;
with essentially unchanged projection and zero velocity it reconstructs the
current endpoint. The comparison metrics explicitly use that endpoint as a
reference. Its safe classification is therefore partly guaranteed by their
definition, not independent proof of which interpolation algorithm is wrong.

New source/math findings (`PROVEN`, CPU/static; causality `UNKNOWN`):

1. VIEW directly interpolates inverse-camera matrix entries, not camera pose.
2. The bundled quaternion helper has reversed endpoints; RT64 compensates
   with `1-weight`. Endpoints are correct, but the resulting interior rotation
   is **not mathematical constant-speed SLERP**.
3. The existing WORLD JOIN diagnostic returns only `xyz` of `WORLD*pos4`.
   In its exact CPU convention, translation occupies row 3 and can affect
   the discarded `w`. Its near-unity result does not establish equality or
   coherence of the complete homogeneous positions subsequently projected.

These findings do not reopen hierarchy fixes, mapping fallbacks, or W policies.

## Evidence and scope

- Tooie HEAD: `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf`, dirty before review.
- RT64 HEAD: `6f1c2d99a4ea571c139f449c326fd176ba8f3496`, dirty before review.
- Existing capture: `/tmp/tooie-rsp-endpoint-binding-status.txt`, modified
  `2026-09-15T13:44:46.277Z`, 45,940 bytes.
- Capture SHA-256:
  `1679f7b5fc39b030c094763a86d06441aa469d08b4c1f468176334b37a48bd65`.
- Source of the visual PASS/FAIL observations: user report, not a new run.
- Capture confirmed locally: 11,708 mapped class-A calls, 65,520 class-A
  JOINs, 249 aligned pathological JOINs, zero misaligned pathological JOINs;
  90 class-B JOINs; mapping-coherent policy does not match safe class.
- No raw per-sample endpoint matrices/vertices are emitted by the current
  endpoint-binding status writer. That capture cannot be replayed into a
  new pose policy. New pose percentiles and correlations are `NOT_CAPTURED`.

Prior state: [[p09-mapping-coherence-fallback-counterfactual]],
[[p09-exact-order-view-world-temporal-cohort-forensic]]. Their pending
mapping recapture instructions are superseded by the supplied/latest capture.

## 1. Temporal semantics reconstructed from source

Paths below are relative to `../BanjoRecomp/lib/rt64/` and refer to the dirty
checkout reviewed, not merely its HEAD.

| Source | Meaning |
| --- | --- |
| `src/hle/rt64_workload_queue.cpp:1160`, `:1272`, `:1294` | Default weights 0/1; generated frames advance display ticks and clamp sample times. `prevFrameWeight` is the previous rendered sample time, not generally `1-curFrameWeight`. The paused-debugger branch uses that complement. |
| `src/hle/rt64_workload_queue.cpp:463`, `:479`, `:560` | Projection, transform and RSP processors receive the same current sample weight. Processors run before the RSP stage. |
| `src/hle/rt64_game_frame.h:45` | WorkloadMap names previous workload; ViewProjectionMap and TransformMap identify prior matrix indices and carry RigidBody history. |
| `src/hle/rt64_game_frame.cpp:515` | VIEW matching obtains submitted view endpoints; AUTO camera decomposition defaults OFF at line 524. |
| `src/render/rt64_projection_processor.cpp:74`, `:138` | VIEW PREV is the mapped prior workload's submitted view matrix; CURRENT is current submitted view. LIVE is `rigidBody.lerp(curFrameWeight,prev,cur,true)`. |
| `src/render/rt64_projection_processor.cpp:132`, `:146`, `:159` | Both projection endpoints receive the current aspect adjustment. P is elementwise interpolated if view translation/rotation interpolation is enabled; otherwise copied CURRENT. Q is exactly `mul(V,P)`. |
| `src/render/rt64_transform_processor.cpp:49` | WORLD PREV is the mapped prior submitted world; CURRENT is this workload's world. LIVE uses the same-weight RigidBody interpolation of already composed world matrices, not local bone transforms. |
| `src/render/rt64_transform_processor.cpp:84` | An unmapped transform within a mapped workload duplicates CURRENT for both endpoints and LIVE. No claim about all unmatched-workload branches is needed for the aligned pathological population. |
| `src/hle/rt64_rsp.cpp:796`, `:907` | Each appended vertex carries worldIndices/viewProjIndices; vertex duplication retains them. |
| `src/hle/rt64_game_frame.cpp:620` | GameCall matching supplies world-matrix ranges for correspondence. It is not the final per-vertex matrix selection. |
| `src/render/rt64_projection_processor.cpp:230`, `src/render/rt64_transform_processor.cpp:119` | Processor outputs supply matrix-buffer data. |
| `src/render/rt64_rsp_processor.cpp:5305` | RSPProcessor binds position, velocity, matrix-index and transform buffers and forwards weights. |
| `src/shaders/RSPProcessCS.hlsl:57`, `:72` | Each vertex selects its own indices and evaluates the requested tfPos expression. |

**RT64 model:** interpolate submitted absolute transforms between adjacent
matched game frames to reconstruct intermediate display frames. This is not
integration of relative camera/object velocity. RigidBody velocity fields help
choose interpolation behavior; they do not replace endpoint interpolation.

Important naming distinction: `prevWorldTransforms` / `prevViewTransforms`
after processing can describe the previous **display sample** at
`prevFrameWeight`. They are not necessarily the PREV **game endpoint**.
Use mapped source endpoints/endpoint sidecars for a counterfactual.

Independent absolute camera/object interpolation is **CONDITIONAL**:
it is a valid rendering architecture when spaces, identities, time laws and
the desired object/camera motion are consistent. It need not equal interpolation
of a relative transform, reproduce a camera tracking constraint between
samples, or keep projected vertices within endpoint screen envelopes.
Matching cohorts and numeric weights alone do not guarantee those properties.

## 2. Camera-space interpolation audit

Keep the requested production/CPU expression unchanged:

```text
F(V,P,W,p) = ((V * P) * W) * p
```

For physical interpretation of the **row-affine storage convention**, write
camera pose `C=[R,0; t,1]`, so `V=C^-1=[R^T,0; -t R^T,1]`.
Direct entry interpolation gives

```text
A(w) = (1-w) R0^T + w R1^T
b(w) = -((1-w) t0 R0^T + w t1 R1^T)
```

Pose interpolation followed by inverse instead gives
`[R(w)^T,0; -t(w)R(w)^T,1]`. Both its rotation basis and translation can
differ. Even decomposing V and interpolating its translation would not in
general recover the camera-center path `t(w)`.

For a pure rotation of angle theta, the midpoint of the two bases contracts
the perpendicular plane by `abs(cos(theta/2))`; the rotation axis is unchanged.
This can change relative motion and projective results. It does **not** prove
the observed samples have a sufficiently large effect. No endpoint rotation
angles or pose errors per pathological sample are available in the status.

Quantification from the existing capture: CURRENT VIEW reduces maximum JOIN
phase delta from 6.13435841 to 0.460992455 and maximum envelope ratio from
22.820406 to 10.5004148, but JOIN max remains 7.7939992. These maxima may
belong to different samples; their differences are not a paired causal effect.

## 3. Weight direction: correct endpoints, nonstandard interior law

`src/common/rt64_math.cpp:430` linearly interpolates translation, scale, skew
and perspective in PREV→CURRENT order. Disabled components copy CURRENT.
At lines 447/455 it calls quaternion `slerp(a,b,1-weight)` (with sign choice).

The bundled implementation at
`src/contrib/hlslpp/include/hlsl++/quaternion.h:149` computes, before subsequent
normalization, `q0 + k*(q1-q0)` using only the first sine coefficient.
After RT64's complement, for sign-adjusted quaternion dot `cos(theta)`,

```text
k = sin(weight*theta) / sin(theta)
q = normalize(q0 + k*(q1-q0))
```

This has the correct endpoints (0 PREV, 1 CURRENT), but is not the usual
`sin((1-w)*theta)/sin(theta)*q0 + sin(w*theta)/sin(theta)*q1` formula.
Do not delete `1-weight` as a supposed reversal fix: that would reverse
the endpoints for this bundled helper. Do not reuse this helper unexamined
as the mathematically correct SLERP control.

A CPU-only executable linked the **actual** `rt64_math.cpp`, compiled with
GCC and the project's `rt64_hlslpp.h` scalar selection, and used a synthetic
0→120 degree Z rotation:

| Weight | RT64 angle (degrees) | Constant-speed SLERP |
| --- | --- | --- |
| 0 | 0 | 0 |
| 0.25 | 33.8488197 | 30 |
| 0.5 | 70.2078171 | 60 |
| 0.75 | 100.150551 | 90 |
| 1 | 120 | 120 |

`PROVEN`: endpoint direction is correct; advertised SLERP time law is not.
`UNKNOWN`: whether this defect materially contributes to the captured glitch.
This synthetic large rotation is not a measurement of game rotation angles.
Same numeric weights therefore do not imply identical physical interpolation
phase across direct VIEW interpolation and WORLD's quaternion path.

Velocity convention is correct: `rt64_game_frame.cpp:1008` writes
`velocity=currentPosition-previousPosition`. Thus `pos-vel*(1-w)` is
`(1-w)*previousPosition+w*currentPosition`. The measured zero velocities
remove this factor entirely for this sample population.

## 4. Matrix-space and WORLD JOIN proof boundary

`lerpMatrixComponents`, `src/common/rt64_math.cpp:177`, treats upper 3x3 as
basis, row 3 xyz as translation, and last-column entries as perspective.
This is internally consistent with RT64's row-affine matrix storage.
It does not make direct inverse-transform interpolation equivalent to pose
interpolation.

The effective CPU expression is still `((V*P)*W)*p`. Under this expression,
`p09MulPos3` (`src/render/rt64_rsp_processor.cpp:766`) drops the fourth
component of `mul(W,p)`. The standalone actual-library test confirms:

```text
W = RT64::matrixTranslation((10,20,30))
p = (1,2,3,1)
hlslpp::mul(W,p) = (1,2,3,141)
```

Consequently the reported WORLD xyz can remain unchanged while the input
4-vector to V/P changes substantially. A finite WORLD xyz distance ratio
near 1 does not imply equal homogeneous positions or equal attachment points.
Even in ordinary Euclidean coordinates, a bounded nonzero gap can amplify
under perspective; a ratio of 1 is not a zero-gap proof.

If two complete homogeneous positions are equal and use the same V/P,
linearity guarantees equal clip positions. A common camera cannot separate
literally identical 4D inputs. This is a constraint on interpreting the
existing metrics, **not evidence for a hierarchy production change**.

For physical camera-relative diagnostics in row-affine storage use
`R_relative = W * inverse(C) = W * V`, acting on row points. This is an
auxiliary pose-space diagnostic only. It must never replace the stipulated
`((V*P)*W)*p` clip calculation or move P through W. Conversely, interpreting
the literal CPU column action as a Euclidean pipeline would require column
affine matrices and a compatible projection placement. These two conventions
must not be silently interchanged. No CPU/shader binary-layout defect is
claimed proven by this review.

## 5. Why CLCL requires CURRENT WORLD

Let `dV=Vlive-Vcur`, `dW=Wlive-Wcur`, with P fixed to LIVE and zero velocity.
Distributivity, **without changing factor order**, gives the exact identity:

```text
F(Vlive,P,Wlive,p) - F(Vcur,P,Wcur,p)
  = ((dV*P)*Wcur)*p
  + ((Vcur*P)*dW)*p
  + ((dV*P)*dW)*p
```

CURRENT VIEW removes the first and mixed terms but retains the WORLD term.
CURRENT WORLD removes the second and mixed terms but retains the VIEW term.
CLCL removes all three. Projective division is nonlinear, so neither their
screen-space effects nor maxima need add monotonically. The data support
VIEW-dominant sensitivity and a remaining WORLD contribution; they do not
identify that contribution as faulty hierarchy or mismatched cohorts.

The metric definitions explain additional endpoint bias:

- `p09NoteJoinMetrics`, `rt64_rsp_processor.cpp:1014`, divides the measured
  gap by `max(previousGap,currentGap,epsilon)`. At CURRENT it is necessarily
  at most 1.
- `p09EffectivePhaseXY`, line 932, projects each sample onto its own endpoint
  chord. At CURRENT every valid phase equals 1, so the pair phase delta is 0.
  Away from endpoints, unequal chord phases are not proof of unequal clocks.
- The envelope metric similarly uses endpoint edges. The verified CCCC
  maxima are exactly JOIN=1, phase=0, envelope=1. CLCL differs slightly
  because its P remains LIVE (max JOIN=1.00383699, phase=0.00280433893,
  envelope=1.00292218).

Answer to A/B/C/D: **B remains compatible; A and C remain hypotheses**.
A is supported as factor sensitivity, not as root attribution. C has a
concrete possible mechanism in different interpolation time laws, not cohorts.
D includes the homogeneous/metric interpretation limitation above. Nothing
here establishes an architectural requirement to snap WORLD with VIEW.

For zero velocity and the same endpoint position, prior same-cohort full
matrix lerp is algebraically endpoint clip lerp. Its failure is not independent
evidence against every possible pose-space reconstruction. Neither is proposed.

## 6. Camera pose counterfactual and prior evidence

An older pose arm already exists: `rt64_p09_temporal_coherency.cpp:1857`
inverts V endpoints, interpolates C, inverts again, and calls
`transformPos(mul(Cinv,Pi),Wi,...)`. **That pose arm preserves order**;
the neighboring COMBINED_RELATIVE arm's invalid order does not invalidate it.
The older evidence [[p09-projective-w-and-viewproj-representation-root-cause]]
records 199 camera-pose explosions versus 199 LIVE explosions. This weakens
a generic “invert/decompose fixes it” claim.

However its helper at line 421 uses the same nonstandard quaternion routine
and silently falls back to matrix lerp on decomposition failure. It also
predates the current JOIN sample schema. It is not a measured result for the
new mathematically validated pose control or today's JOIN percentiles.

```text
POSE_VIEW_JOIN_MAX: NOT_CAPTURED
POSE_VIEW_JOIN_P95: NOT_CAPTURED
POSE_VIEW_JOIN_P99: NOT_CAPTURED
POSE_VIEW_MAX_JOIN_PHASE_DELTA: NOT_CAPTURED
POSE_VIEW_P99_JOIN_PHASE_DELTA: NOT_CAPTURED
POSE_VIEW_MAX_EDGE_RATIO_VS_ENDPOINT_ENVELOPE: NOT_CAPTURED
POSE_VIEW_P99_EDGE_RATIO_VS_ENDPOINT_ENVELOPE: NOT_CAPTURED
POSE_DERIVED_VIEW_MATCHES_SAFE_CLASS: UNKNOWN / NOT_CAPTURED
RELATIVE_CAMERA_OBJECT_PATH_DEVIATION_CORRELATES_WITH_PATHOLOGY: UNKNOWN / NOT_CAPTURED
```

A YES/NO answer to either unmeasured classification would be fabricated.

## 7. One next CPU counterfactual, specified but not implemented

`NEXT_CPU_COUNTERFACTUAL=EXACT_ORDER_VALIDATED_CAMERA_POSE_VIEW`.

Add one default-off CPU arm beside the current endpoint-binding factorial:

1. Select the same raw PREV/CURRENT V endpoints and the identical per-vertex
   LIVE P/W/position, workload, GameCall, indices and weight as LLLL.
2. Invert V endpoints. Validate finite inverse, row-affine structure, positive
   proper rotation, unit scale and negligible skew for a rigid camera model.
   Count invalid/nonrigid cases and retain them in the coverage report;
   do not silently replace them with CURRENT or label fallback samples safe.
3. Interpolate camera translation linearly and rotation by explicit shortest-arc
   mathematical SLERP, with a near-equal normalized-linear limit. Verify 0,
   0.25, 0.5, 0.75 and 1 against analytic rotations before capture. Preserve
   exact endpoint matrices at 0/1. If material camera scale/skew exists, this
   rigid control is inconclusive for those samples; do not normalize it away.
4. Invert interpolated C; evaluate **only**
   `((POSE_DERIVED_LIVE_VIEW*LIVE_PROJ)*LIVE_WORLD)*LIVE_POS4`.
   Use the existing JOIN/envelope/phase metrics and unchanged sample pairing.
5. In that same arm, record physical `Rlive=Wlive*Vlive` and
   `Rpose=Wlive*Vpose`. Define normalized action deviation on the same local
   vertices as RMS Euclidean camera-space difference divided by the maximum
   of previous/current object radius and a declared model-unit floor.
   This avoids mixing translation units with dimensionless matrix coefficients.
   Retain full homogeneous `W*p4`, including w, as metadata; never feed the
   auxiliary relative product to the production-order clip evaluator.
6. Report requested seven metrics, invalid counts, eligible JOIN coverage,
   and paired deviation distributions for the pre-existing pathology label
   (LLLL gap ratio >=1.5 and CLCL <=1.05). Include sample count and effect
   size/rank correlation, stratified by workload/weight; a large global maximum
   alone is not correlation. Safe-class YES requires the existing safe-class
   comparator plus complete pathological-sample coverage, not selective success.

**Expected discrimination:** a safe pose-only arm supports direct VIEW
interpolation as a sufficient trigger for this CPU population, despite LIVE
WORLD. A validated failing pose arm rules out that repair as sufficient and
leaves a coupled/interior interpolation law or metric/space interpretation
issue. Partial improvement quantifies contribution but does not authorize a
production change. This is one arm; no WORLD replacement arm, relative matrix
lerp, W policy, or extra render experiment is proposed.

## 8. Validation and reproducibility

No runtime diagnostic source was added; no production source was edited.
TooieRecompiled / window_display_test / tintro_projection_policy_test were
`NOT_RUN` because this is a documentation/static review with an isolated
CPU math check, not a diagnostic implementation checkpoint.

The isolated test compiled C++ from stdin in context-mode's temporary working
directory, using `g++ -std=c++17 -O0`, includes
`../BanjoRecomp/lib/rt64/src/common` and
`../BanjoRecomp/lib/rt64/src/contrib/hlslpp/include`, linked
`../BanjoRecomp/lib/rt64/src/common/rt64_math.cpp`, then ran that executable.
Compilation and execution exit codes: 0. No game or renderer was linked/run.
Synthetic assertions/data: identity and 120-degree Z quaternion in two valid
DecomposedTransforms; call `lerpTransforms(...,w,true,true,true,true,true,true)`
at the five weights above and print `2*atan2(q.z,q.w)` in degrees. Translation
test: `mul(matrixTranslation(float3(10,20,30)),float4(1,2,3,1))`.

For camera pose with identity PREV and CURRENT translation (10,20,30),
120-degree Z rotation, the midpoint direct VIEW differs from inverse of the
analytic midpoint pose by entrywise L1 sum **16.366**. The initial console
label said max-abs, but `matrixDifference` at `rt64_math.cpp:132` sums absolute
entries; the correct interpretation here is L1, not maximum. This is synthetic
non-equivalence evidence only, not a captured glitch metric.

Final checks:

- `rtk git diff --check`: exit 0 in Tooie.
- `rtk git -C /home/wesley/projetos/N64-Recomp/BanjoRecomp/lib/rt64 diff --check`:
  exit 0.
- `markdown_vault_audit` at its configured vault root: completed, 2 errors and
  71 warnings. The two errors are existing broken links in untouched
  `plan/evidence/p09-projective-w-production-fix.md:154` and `VAULT.md:6`.
  No errors reported for this review. Initial `path=docs` call failed because
  the server is already rooted at docs; the root call above succeeded.
- Before/after aggregate SHA-256 of sorted path+file bytes for non-docs,
  nonignored Git-listed files remained identical: Tooie 146 files,
  `278cf393585617d7347594f900105b889d2a18fb401746a1d35f6db77bfd15b9`;
  RT64 305 files,
  `4df8c6d2309051355f8e012fd481bcdd08635b19037fd9dd3d70ac067a62e516`.
- Both worktrees remain dirty with pre-existing changes. This review adds
  this evidence document and updates handoffs, P09 phase and canonical RE
  navigation/state only. No staging, commit, push, cleanup or dependency edits.

Reindex is unnecessary: no material code changes. No stable spec was changed:
no visual root or new production contract has been proven.

## Required architecture fields

```text
RT64_INTERPOLATION_MODEL: ABSOLUTE_ENDPOINT_INTERPOLATION_FOR_DISPLAY_FRAME_RECONSTRUCTION
VIEW_TEMPORAL_SEMANTICS: MAPPED_PREVIOUS_GAME_VIEW_TO_CURRENT_GAME_VIEW; DIRECT_MATRIX_INTERPOLATION_BY_DEFAULT
WORLD_TEMPORAL_SEMANTICS: MAPPED_PREVIOUS_COMPOSED_WORLD_TO_CURRENT_COMPOSED_WORLD; DECOMPOSED_COMPONENTS_OR_MATRIX_FALLBACK
INDEPENDENT_VIEW_WORLD_INTERPOLATION_IS_ARCHITECTURALLY_VALID: CONDITIONAL
VIEW_IS_INTERPOLATED_IN_VIEW_SPACE_DIRECTLY: YES
CAMERA_POSE_INTERPOLATION_THEN_INVERSE_EQUIVALENT_TO_CURRENT_VIEW_LERP: NO_IN_GENERAL
DIRECT_VIEW_MATRIX_LERP_CAN_CREATE_PROJECTIVE_RELATIVE_MOTION_ERROR: YES_MATHEMATICALLY; CAPTURE_CAUSALITY_UNKNOWN
POSE_DERIVED_VIEW_MATCHES_SAFE_CLASS: UNKNOWN_NOT_CAPTURED
RELATIVE_CAMERA_OBJECT_PATH_DEVIATION_CORRELATES_WITH_PATHOLOGY: UNKNOWN_NOT_CAPTURED
WHY_CLCL_REQUIRES_CURRENT_WORLD: CLLL_RETAINS_Vcur_P_dW; CLCL_REMOVES_ALL_TRANSFORM_DEVIATIONS_AND_RETURNS_TO_ENDPOINT_REFERENCE
VIEW_WEIGHT_DIRECTION_CORRECT: YES_FOR_INTERPOLATED_COMPONENTS
WORLD_WEIGHT_DIRECTION_CORRECT: YES_FOR_INTERPOLATED_COMPONENTS
VIEW_WORLD_WEIGHT_DIRECTION_IDENTICAL: YES_ENDPOINT_DIRECTION; NOT_IDENTICAL_INTERIOR_MOTION_LAWS
WEIGHT_CONVENTION_BUG_FOUND: NO_ENDPOINT_REVERSAL; NONSTANDARD_QUATERNION_INTERPOLATION_LAW_PROVEN_SEPARATELY
VIEW_INTERPOLATED_IN_WRONG_SPACE: UNPROVEN_AS_BUG; NOT_CAMERA_POSE_INTERPOLATION
WORLD_INTERPOLATED_IN_WRONG_SPACE: UNPROVEN
MATRIX_SPACE_MISMATCH_CAN_EXPLAIN_CLCL: YES_AS_POSSIBILITY; NOT_ESTABLISHED_FOR_VISUAL_BUG
NEXT_CPU_COUNTERFACTUAL: EXACT_ORDER_VALIDATED_CAMERA_POSE_VIEW
ROOT_CAUSE_CLASS: UNRESOLVED
PRODUCTION_FIX_UPDATED: NO
CLEANUP_READY: NO
COMMIT_READY: NO
NEXT_SINGLE_ACTION: IMPLEMENT_ONE_CPU_ONLY_VALIDATED_CAMERA_POSE_VIEW_ARM_FOR_LATER_USER_CAPTURE; DO_NOT_LAUNCH_GAME
NEXT_RECOMMENDED_MODEL: GPT-6 Astra for architecture review; Grok 4.6 Medium afterwards for the specified diagnostic implementation/recapture
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

Remaining hypotheses only:

1. Direct inverse-camera entry interpolation changes the relevant relative path.
2. Camera/object interior motion laws fail a coupled trajectory constraint
   (including the demonstrated quaternion law defect), despite aligned endpoints.
3. The homogeneous/space interpretation and endpoint-normalized CPU metrics
   incompletely characterize the visual defect.

No hypothesis above is promoted to a production root or a stable spec.
