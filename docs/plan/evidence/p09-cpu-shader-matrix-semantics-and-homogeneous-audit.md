---
title: "P09 CPU shader matrix semantics and homogeneous audit"
description: "Static SPIR-V proves CPU diagnostic orientation/order mismatch; synthetic confirmation; no production change."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-15
tags: [p09, rt64, shader, spirv, matrix, architecture, forensic]
---

# P09 CPU shader matrix semantics and homogeneous audit

Role: `P09_CPU_SHADER_MATRIX_SEMANTICS_AND_HOMOGENEOUS_AUDIT`.
Bounded objective P09-RES-05: static/CPU review, no runtime counterfactual.

## Verdict

**PROVEN: current CPU factorial and pose clip evaluators do not implement
the compiled shader's mathematical action.** Select **A: repair diagnostic
semantics before further captures**. No diagnostic or production code was
changed in this review. The visible game's root remains `UNRESOLVED`.

For CPU-stored matrices W, V, P and Q = V P:

```text
current CPU diagnostic: ((V P) W) p_column
compiled RSPProcessCS:  p_row (W Q) = p_row (W (V P))
correct CPU helper:    hlslpp::mul(p, hlslpp::mul(W, Q))
```

Changing only the final vector direction is insufficient: Q W must also
become W Q. Transposing only W is insufficient. Do not transpose production
buffers or change the production HLSL expression.

## Provenance and scope

- Tooie HEAD: `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf` (dirty).
- RT64 HEAD: `6f1c2d99a4ea571c139f449c326fd176ba8f3496` (dirty).
- RT64 root: `/home/wesley/projetos/N64-Recomp/BanjoRecomp/lib/rt64`.
- Static/CPU artifacts: `/tmp/p09-cpu-shader-audit-20260915/`.
- Existing capture copied to `capture-status.txt`; original mtime
  `2026-09-15T20:22:57.963Z`. Its pose counts and maxima match the supplied
  request. No game was launched for this review.
- This is proof for the current local Vulkan SPIR-V artifact, including
  its existing endpoint diagnostic branches; no claim about uninspected
  DXIL/MSL binaries or GPU floating-point bitwise reproducibility.
- Prior review: [[p09-independent-architecture-root-cause-review]].
- Pose implementation: [[p09-exact-order-validated-camera-pose-view]].

## 1. Representation and upload chain

Source references below are relative to RT64 unless explicitly stated.

| Stage | Verified source and action |
| --- | --- |
| Guest conversion | `src/common/rt64_common.cpp:125-141`: halfword endian correction `j ^ 1`, 16.16 conversion, constructor entries in row order; no matrix transpose. `src/hle/rt64_rsp.cpp:273-275` passes this to `matrixCommon`. |
| CPU construction | `src/common/rt64_math.cpp:77-80`: translation is `m[3].xyz`; `:141-142` tests last column `(0,0,0,1)` for affine storage. |
| Matrix stacks | `src/hle/rt64_rsp.cpp:193-241`, `:554-556`, `:613-628`: hlslpp products populate VIEW, PROJ, VIEWPROJ, WORLD; extraction branches can have different semantics, so affine WORLD must be verified per sample. |
| CPU layout | `src/common/rt64_hlslpp.h` selects hlslpp; `src/shared/rt64_hlsl.h:221-249` defines interop `float m[4][4]` and entry-preserving conversions to/from hlslpp. Synthetic raw-memory tests verify both representations use 64 bytes, four contiguous rows of four floats, translation at offsets 48/52/56. |
| CPU mul | `src/contrib/hlslpp/include/hlsl++/matrix_float.h:881`, `:1016`, `:1093-1098`: matrix-vector is ordinary row dot vector; vector-matrix is ordinary vector dot columns; matrix multiplication preserves argument order. |
| Display matrices | `src/render/rt64_projection_processor.cpp:132-161`: LIVE VIEW and PROJ produce Q=`mul(V,P)`; endpoint Qs are constructed likewise. WORLD interpolation is in `rt64_transform_processor.cpp`. |
| Upload source | `rt64_projection_processor.cpp:223-234` uploads `modViewProjTransforms`; `rt64_transform_processor.cpp:118-140` selects `lerpWorldTransforms` or original `worldTransforms`. Endpoint vectors use the same 64-byte element representation. |
| Byte transfer | `src/render/rt64_buffer_uploader.cpp:70-81`, `:137-148`: `memcpy` and byte-range buffer copy; no transpose or per-element conversion. These operations were inspected, not executed by this review. |
| Descriptors | `src/render/rt64_descriptor_sets.h:551-552`: structured bindings 17/18; `rt64_rsp_processor.cpp:5899-5902` binds 64-byte structured views, also endpoint bindings 22/23. |
| HLSL | `src/shaders/RSPProcessCS.hlsl:42-45`: undecorated `StructuredBuffer<float4x4>`; no `row_major` keyword. `:59-72` loads Q/W and computes `mul(mul(Q,W),p)`. |
| Compilation | Tooie `build/build.ninja` uses bundled DXC, `-spirv -fspv-target-env=vulkan1.0 -fvk-use-dx-layout -E CSMain -T cs_6_3`; no `-Zpr`. `file_to_c` embeds the SPV; `rt64_shader_library.cpp:29`, `:519` selects the SPIR-V blob for Vulkan. |

HLSL's default column-major buffer interpretation corresponds to the
transpose of the CPU mathematical matrix. DXC represents the HLSL matrix
in transposed form in SPIR-V and reverses the operations. The emitted
**RowMajor decoration is on that SPIR-V representation**: it does not mean
the HLSL source and CPU matrix have identical logical orientation.

```text
CPU_MATRIX_STORAGE_CONVENTION: ROW_CONTIGUOUS_ROW_AFFINE; 64 bytes; T in row 3
GPU_SHADER_MATRIX_STORAGE_CONVENTION: HLSL default column-major; SPIR-V RowMajor, MatrixStride=16, ArrayStride=64
CPU_MUL_MATRIX_VECTOR_SEMANTICS: y_i=sum_j M_ij*p_j on CPU-stored entries
SHADER_MUL_MATRIX_VECTOR_SEMANTICS: for uploaded M, y_j=sum_i p_i*M_ij; HLSL logical M is CPU M transpose
CPU_SHADER_MATRIX_ACTION_EQUIVALENT: NO
```

## 2. Static SPIR-V proof

`spirv-dis` was unavailable (command failed; not counted as validation).
Bundled DXC successfully emitted disassembly with `-Fc` while compiling the
exact current shader with the project flags. Exit code 0.

Recompiled SPV and existing `build/src/shaders/RSPProcessCS.hlsl.spv` have
the identical SHA-256:
`a8030768476991d17a826a4aea2e81b7d75b6614198d4f10092c289331719a8f`.
The generated `RSPProcessCS.hlsl.spirv.c` signed-byte array was decoded
independently: all 12852 embedded bytes also match that SPV and hash.

`/tmp/p09-cpu-shader-audit-20260915/RSPProcessCS.spvasm`:

```text
106-109: DescriptorSet 0, viewProjTransforms Binding 17, worldTransforms Binding 18
157:     OpDecorate %_runtimearr_mat4v4float ArrayStride 64
159:     OpMemberDecorate %type_StructuredBuffer_mat4v4float 0 MatrixStride 16
160:     OpMemberDecorate %type_StructuredBuffer_mat4v4float 0 RowMajor
309-310: %139 = load from viewProjTransforms[index]   ; Q
311-312: %141 = load from worldTransforms[index]      ; W
349:     %178 = OpMatrixTimesMatrix %mat4v4float %141 %139
358:     %187 = OpCompositeConstruct %v4float %184 %185 %186 %float_1
359:     %188 = OpVectorTimesMatrix %v4float %187 %178
```

`%187` contains `pos - vel*(1-curFrameWeight)`, with w=1. Endpoint branches
also multiply WORLD by VIEWPROJ and then vector by matrix (`%198/%199`,
`%204/%205`). Other `OpMatrixTimesVector` instructions in this shader are
normal/lighting operations, not the tfPos chain. No explicit transpose
is required in this chain because the lowering already reverses operands.

```text
RSPPROCESS_SPIRV_OPERATION: OpMatrixTimesMatrix(W,Q), then OpVectorTimesMatrix(p,WQ)
RSPPROCESS_SPIRV_MATRIX_DECORATION: RowMajor; MatrixStride=16; ArrayStride=64; set=0 bindings=17,18,22,23
SPIRV_CONFIRMS_CPU_DIAGNOSTIC_ORDER_AND_ORIENTATION: NO
```

## 3. Independent synthetic test

`synthetic.cpp` links the actual RT64 `rt64_math.cpp` and bundled hlslpp.
Side A uses diagnostic hlslpp calls. Side B copies the matrix bytes and
uses independent scalar loops implementing SPIR-V row-major loads, W Q,
and vector-times-matrix; it does not call hlslpp multiplication.

Both GCC `-O2` (hlslpp scalar) and build-matching Clang `-O3 -DNDEBUG`
(hlslpp SIMD) compile and run with exit code 0; results agree:

| Test, p=(1,2,3,1) | Current CPU | SPIR-V algebra |
| --- | --- | --- |
| T=(10,20,30) | (1,2,3,141) | (11,22,33,1) |
| W=S(2,3,4) Rz(+90deg) T(10,20,30), Q=I | (4,-3,12,141) | (4,22,42,1) |
| Same W; Q=T(-2,5,7) S(0.5,2,1) | (2,-6,12,191) | (1,54,49,1) |

Raw W bytes as floats: `0 2 0 0 / -3 0 0 0 / 0 0 4 0 / 10 20 30 1`.
Corrected hlslpp `mul(p,mul(W,Q))` produces `(1,54,49,1)` in the last test.
The same test through `interop::float4x4` returns identical wrong/corrected
CPU results, and verifies `interop_size=64`, `interop_bytes_match=YES`.
Integer/power-of-two values make the comparison exact for these cases.

```text
CPU_SYNTHETIC_RESULT: (4,-3,12,141)
SPIRV_SEMANTIC_SYNTHETIC_RESULT: (4,22,42,1)
CPU_SPIRV_SYNTHETIC_MATCH: NO
TRANSLATION_TO_W_RESULT_IS_EXPECTED_FOR_PRODUCTION_SHADER: NO
TRANSLATION_TEST_INTERPRETATION: C; correct row-affine storage, wrong CPU diagnostic multiplication direction (and wrong composite operand order)
```

The CPU fourth row dot p is `10*1+20*2+30*3+1=141`. This is neither a
production homogeneous-W anomaly nor proof of shader miscompilation.

## 4. P09_CPU_HELPER_AUDIT

Line references in this table are `src/render/rt64_rsp_processor.cpp`.
Classifications describe current use, not merely similarly named math.

| Helper / use | Classification | Reason |
| --- | --- | --- |
| p09MakeQCurrent/Prev, p09FactorViewProj; matrix selectors (112-265) | EXACT_SHADER_EQUIVALENT (provisioning only, when selected sources match upload) | Q=V P is correct CPU buffer construction; selecting W does not itself apply W to a point. This does not certify every fallback's binding identity. |
| p09FactorialClip (268-278) | INVALID_FOR_SHADER_EQUIVALENCE | Applies Q W to column p instead of row p to W Q. All 16 factorial arms affected. |
| Inline PREV/CURRENT/LIVE, sidecar/witness (4062-4084, 4139, 4389-4409), rigid and endpoint policies | INVALID_FOR_SHADER_EQUIVALENCE | Same wrong product/direction; agreement between CPU witnesses shares the error. |
| p09SameCohortCompositeClip (426-442), p09CohortAlignedLiveClip (599-614), p09MappingCoherentLiveClip (688-696) | INVALID_FOR_SHADER_EQUIVALENCE | Compose Q W and apply matrix to column position. |
| p09MulPos3 (951-953), WORLD JOIN inputs | INVALID_FOR_SHADER_EQUIVALENCE | Drops w from wrong-sided W p; excludes translation from xyz and transposes rotation action. Not a valid physical-world metric either. |
| p09PoseDerivedLiveClip (848-851) | INVALID_FOR_SHADER_EQUIVALENCE | Same Q W / column-p error; order-preserved status is hardcoded, not proof. |
| p09ExactOrderValidatedPoseView / validation (699-846) | AMBIGUOUS | Row-affine pose algebra is appropriate, but validation aggregates different failure reasons and does not validate actual recomposed endpoints. Its clip consumer is invalid. |
| rLive=Wlive Vlive, rPose=Wlive Vpose (4580-4583), matrix construction alone | PHYSICAL_ROW_AFFINE_DIAGNOSTIC_ONLY | Correct physical model-to-camera composite; not yet a point or full projected clip. |
| relative-path point evaluation and object radius (4584-4600) | INVALID_FOR_SHADER_EQUIVALENCE | Calls mul(R,p); radius mixes wrong-sided xyz with physical row-3 translation. Correlation is not physically interpretable. |
| WORLD_H.w (4603-4606) | INVALID_FOR_SHADER_EQUIVALENCE | W p column result is not production homogeneous WORLD position. |
| Standard SLERP, WORLD angle-law delta (707-743, 4608-4622) | PHYSICAL_ROW_AFFINE_DIAGNOSTIC_ONLY | Quaternion-angle comparison independent of clip error; pathological subgroup labels still come from invalid clips. |
| p09ClipNdcFromClip (974-988) | PHYSICAL_ROW_AFFINE_DIAGNOSTIC_ONLY | Pre-viewport diagnostic divide; production flips Y and clamps exact w=0, whereas CPU excludes near-zero w. Uniform Y reflection preserves XY distances and phase, but rejection policy and input clip errors matter. |
| p09EffectivePhaseXY (1170), p09NoteJoinMetrics (1260), p09JoinRelValue (1293), p09NoteWorldJoinMetrics (1281) | AMBIGUOUS as shader evidence | Mathematically defined distance/chord statistics; no shader-equivalence or attachment proof, current inputs invalid. |
| Endpoint pairing (1720-1735) | AMBIGUOUS | Chooses closest sum of PREV/CURRENT screen gaps; does not prove two vertices are the same physical attachment anchor. Pairing must be recomputed after semantic repair. |
| Endpoint envelopes / safe-class (4851 onward; 2845-2855) | AMBIGUOUS as shader evidence | Endpoint-normalized statistics fed invalid clips; comparator is not geometry truth. |
| Legacy MODELVIEW arm (5196-5209), hierarchy local/recompose/clip (5416-5478), CLIP_LERP/NDC_LERP policies | INVALID_FOR_SHADER_EQUIVALENCE | Legacy MODELVIEW and hierarchy use column-convention products on row storage; clip/ndc policies inherit invalid endpoint positions. Retire their causal conclusions; do not revive these counterfactuals. |

```text
FACTORIAL_CPU_TFPOS_SHADER_EQUIVALENT: NO
POSE_VIEW_CPU_TFPOS_SHADER_EQUIVALENT: NO
WORLDSPACE_JOIN_CPU_METRIC_PHYSICALLY_VALID: NO
```

## 5. Full homogeneous attachment criterion

At each of PREV, CURRENT and LIVE use the selected matrices and corresponding
local attachment positions: `Hp=p_parent_row Wparent`, `Hc=p_child_row Wchild`.
Equivalently use uploaded HLSL logical `transpose(W)` on column positions.

For verified affine W and local w=1, production world w=1. Require finite
Hp/Hc, both w approximately 1 under a declared numeric tolerance, and full
affine point agreement (xyz in physical world units). In this verified
case xyz equality is sufficient because w is fixed. Scale/skew in the
upper 3x3 alone do not make w variable.

For genuinely non-affine matrices, require nonzero homogeneous vectors and
projective equality, e.g. all `Hp_i*Hc_j-Hp_j*Hc_i` vanish within a
scale-aware tolerance. For finite w compare xyz/w too. Negative common
scale is equivalent abstractly in projective space but is not automatically
equivalent for graphics clip inequalities; preserve the production clip
sign/domain. w=0 requires separate direction-at-infinity treatment.
World agreement only implies clip agreement when subsequent Q is shared.

```text
CORRECT_WORLD_ATTACHMENT_EQUALITY_CRITERION: compare p_row*W in full 4D; affine w=1 plus xyz equality; otherwise finite/nonzero projective proportionality with clip-domain checks
XYZ_ONLY_WORLD_JOIN_WAS_SUFFICIENT: NO
HOMOGENEOUS_ATTACHMENT_DIVERGENCE_REMAINS_PLAUSIBLE: YES (conditional/unmeasured for non-affine samples; not supported by old WORLD_H.w)
```

The old WORLD JOIN is invalid principally because its xyz was not physical
world xyz, not merely because it discarded w. Under proven affine inputs
a hidden production-world-w divergence is algebraically excluded. No
hierarchy production fix is reopened. Existing nearest-screen pairs are
not certified attachment anchors.

## 6. Pose validity and coverage

80535 counts vertex samples, not 80535 independent camera matrices.
`NONRIGID_CAMERA=27300` (33.8983%) increments when either inverse VIEW
fails `p09ValidateRigidCameraPose`, or decomposition fails. Reasons are
collapsed: exact `isMatrixAffine`, decomposition validity, scale/skew/
perspective thresholds 1e-3, positive determinant with 0.05 tolerance.
The status contains no per-reason breakdown or offending matrices.

**UNKNOWN which reasons account for the 27300 samples.** They cannot be
called genuine camera scale/skew/perspective solely from that counter.
The row-affine orientation used by the validator is appropriate; the
shader mismatch does not prove a transposed validator. Exact affine tests
after floating-point inversion are a potential false-rejection boundary,
not a demonstrated explanation of the capture. Projection-to-view
decomposition/guest quantization can also contribute; unmeasured here.

The reported endpoint difference is specifically
`maxabs(inverse(inverse(V))-V)`, lines 817-824. It is **not**
`poseView(0)-PREV_VIEW` or `poseView(1)-CURRENT_VIEW`. The code discards
near-unit scale/skew during recompose but never checks the endpoints of
that actual path. Endpoint error does not gate `valid=true`.

A 1000-case synthetic rigid Z-camera sweep with translation
(12345,-6789,4567) reproduces `0.001953125` inverse-roundtrip max error on
both compilers without camera scale/skew; this is compatible with ordinary
float32 rounding at large coordinates. It does not attribute the exact
capture maximum without the matrices. All 1000 synthetic inverses passed
the exact affine test; this test does not explain the 27300 rejects.

```text
POSE_VIEW_NONRIGID_RATE_EXPLANATION: AGGREGATED_VALIDATOR_REJECTION; specific cause UNKNOWN, not proof of legitimate nonrigid cameras
POSE_VIEW_ENDPOINT_MISMATCH_EXPLANATION: double-inverse floating-point residual exceeds absolute 1e-4; actual pose endpoints were not tested; exact capture attribution UNKNOWN
POSE_VIEW_ARM_VALID_FOR_53235_SAMPLES: NO (as a production shader causal discriminator)
DIRECT_VIEW_MATRIX_INTERPOLATION_SUFFICIENT_TRIGGER_FOR_VALID_SUBSET: UNPROVEN
```

The implementation accepted 53235 samples and observed 0/152 fixes among
its accepted pathological pairs (142 excluded, 51.7007% pair coverage).
Those are valid records of **the old CPU experiment**, not a 0/152 production
shader result. Neither a complete nor subset refutation of a VIEW cause
follows from this invalid clip evaluator.

## 7. Metric bias

```text
JOIN_METRIC_ENDPOINT_BIAS: gap_live / max(gap_prev,gap_cur,floor); either exact endpoint necessarily scores <=1
PHASE_METRIC_ENDPOINT_BIAS: projection onto each PREV-to-CURRENT XY chord; exact CURRENT gives phase 1 for both, delta 0 (nondegenerate chords)
ENVELOPE_METRIC_ENDPOINT_BIAS: max live triangle edge / max(max prev edge,max current edge); either endpoint scores <=1; degenerates skipped
CLCL_SAFE_CLASS_IS_INDEPENDENT_CAUSAL_PROOF: NO
```

The safe comparator uses CCCC JOIN max and p99: gates
`max(2*CCCC.max+0.05,1)` and `max(2*CCCC.p99+0.05,0.5)`. It does not test
point equality with CCCC, pixels, full 3D attachment, coverage identity,
or physical motion. The reported CLCL 1.0064 is close to an endpoint;
CLCL is not exactly CCCC when LIVE P differs. Safe does not prove benign
interior geometry. Curved valid motion can leave an endpoint envelope or
have different screen-chord phase; large scores flag investigation only.

User visual FORCE_CURRENT success remains separate `USER_OBSERVED`
evidence for sensitivity to production tfPos. The CPU orientation error
does not invalidate that actual shader intervention, but it invalidates
using the 16-arm CPU table to isolate its causal term.
Zero position velocity is an input observation unaffected by matrix
orientation. The supplied projection-only non-causality conclusion is
retained as bounded prior context, not reopened here; the old CPU
factorial cannot independently prove that exclusion for the shader.

## 8. Clean conceptual root state

- `PROVEN`: diagnostic matrix orientation/composition mismatch. This is a
  measurement defect, not a demonstrated game-rendering root.
- `CORRECTED/SUPERSEDED`: claims that old factorial/pose/relative/WORLD_H
  metrics are exact shader semantics; hierarchy xyz-only ratios as proof.
- `REFUTED` retained in the existing experiment's bounded sense: temporal
  cohort mismatch and mapping-coherence fallback as explanations of its
  aligned CPU pathology. No reopening or new counterfactual. Their old
  geometry statistics cannot establish a new shader causal exclusion.
- Nonstandard WORLD SLERP is mathematically real. Its all-sample angle
  distribution remains an observation, but old pathology labels are
  contaminated. Dominance is unsupported/weakened; **not proven globally
  REFUTED** by the supplied subgroup correlation. Do not prioritize it.
- Direct VIEW interpolation had been considered weakened by the pose
  experiment; that weakening is now conditional/superseded as a production
  conclusion. The valid production-subset test has not been performed.
- `VIEW_WORLD_ROOT_CLASS=CROSS_STREAM_TEMPORAL_COHORT_MISMATCH` and
  `VIEW_WORLD_INTERACTION_ROOT_CLASS` with the same value in the preserved
  status file are stale. Do not use them. They were not rewritten in the
  original capture or runtime code; the diagnostic repair must retire them.

```text
ROOT_CAUSE_CLASS: UNRESOLVED; CPU_DIAGNOSTIC_MATRIX_SEMANTICS_MISMATCH_PROVEN
REMAINING_HYPOTHESES:
1. A production temporal transform/interpolation defect affecting tfPos remains supported by separate FORCE_CURRENT visual evidence; VIEW versus WORLD versus their coupling is not isolated by the invalid CPU factorial.
2. Physical attachment/interior projected-geometry divergence remains unmeasured under the corrected convention; homogeneous-only WORLD divergence requires non-affine inputs and is not established by the old w statistics.
```

## 9. One next action — A

Mechanically repair the **existing CPU diagnostic semantics**, without
adding another arm or touching HLSL/production interpolation:

1. One CPU clip evaluator: `Q=mul(V,P)` where a factorial needs Q;
   `clip=mul(pos4,mul(W,Q))`. LIVE/witness paths use their actual uploaded Q.
   Preserve the matrix-product grouping W Q used by this SPIR-V. Repair
   every PREV/CURRENT/LIVE/factorial/pose endpoint consumer, not just LLLL.
2. Physical WORLD: `mul(pos4,W)`; camera-relative point:
   `mul(pos4,mul(W,V))`. Recompute reference positions, radius, pair selection,
   envelopes, phase and pathology labels from corrected inputs. Do not mix
   old endpoint references with corrected candidate clips.
3. Retire incompatible legacy arms/claims rather than invent replacements.
   Preserve diagnostic near-zero-w rejection as explicitly diagnostic;
   distinguish raw clip parity from shader viewport/clamp behavior.
4. Keep the pose gate uncertified unless its actual recomposed endpoint
   path is checked at 0/1 with scale-aware tolerances and reject reasons
   are separated. Do not silently relax gates to improve coverage.
5. Version the diagnostic status semantics; mark old metrics superseded,
   remove stale cross-stream root claims, validate with the independent
   synthetic translation and noncommuting composite fixtures **before**
   requesting any new game capture. Do not claim bitwise GPU equality.

These are acceptance details of a single measurement-repair task, not
authorization to run it in this architecture review.

```text
NEXT_SINGLE_ACTION: Repair existing CPU diagnostic matrix action to p_row*(W*Q), rebuild references/pairing, and validate offline before any further capture
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium for the precisely specified mechanical diagnostic repair; return to GPT-6 Astra if new architectural ambiguity appears
PRODUCTION_FIX_UPDATED: NO
CLEANUP_READY: NO
COMMIT_READY: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

## Reproduction and verification

Static shader command (same project flags, new output paths only):

```sh
RT64_ROOT=/home/wesley/projetos/N64-Recomp/BanjoRecomp/lib/rt64
LD_LIBRARY_PATH="$RT64_ROOT/src/contrib/dxc/lib/x64" rtk proxy "$RT64_ROOT/src/contrib/dxc/bin/x64/dxc-linux" -spirv -fspv-target-env=vulkan1.0 -fvk-use-dx-layout -I"$RT64_ROOT/src" -E CSMain -T cs_6_3 "$RT64_ROOT/src/shaders/RSPProcessCS.hlsl" -Fo /tmp/p09-cpu-shader-audit-20260915/RSPProcessCS.spv -Fc /tmp/p09-cpu-shader-audit-20260915/RSPProcessCS.spvasm
rtk proxy g++ -std=c++17 -O2 -I"$RT64_ROOT/src" -I"$RT64_ROOT/src/contrib/hlslpp/include" /tmp/p09-cpu-shader-audit-20260915/synthetic.cpp "$RT64_ROOT/src/common/rt64_math.cpp" -o /tmp/p09-cpu-shader-audit-20260915/synthetic
rtk proxy /tmp/p09-cpu-shader-audit-20260915/synthetic
rtk proxy clang++ -std=c++17 -O3 -DNDEBUG -I"$RT64_ROOT/src" -I"$RT64_ROOT/src/contrib/hlslpp/include" /tmp/p09-cpu-shader-audit-20260915/synthetic.cpp "$RT64_ROOT/src/common/rt64_math.cpp" -o /tmp/p09-cpu-shader-audit-20260915/synthetic-clang
rtk proxy /tmp/p09-cpu-shader-audit-20260915/synthetic-clang
```

All five successful compile/run commands above returned 0. The negative
equivalence result is the test finding, not a test execution failure.
No full game build or gameplay tests were needed/performed.

Source hashes at audit:

```text
rt64_rsp_processor.cpp SHA256 439f714de868b1b3af11b662ae78adbda33bc65cb782dfeb33ef2ad26700cc29
RSPProcessCS.hlsl SHA256 5159782f633dc248eb4002dca48195634a9a49d23f6bd921fd594ebe13ca18e6
rt64_math.cpp SHA256 d32abc9572001870b4144ffa49e832589858de0830dbb0d008761ad15a76364b
RT64 git diff --binary SHA256 82fb7c4227e3bb23a6cd5f3c4058275d9130b5ed2473d8466b92452e7bc9147f
```

Existing dirty Tooie/RT64 work is protected. This review changes only docs
and creates static/CPU artifacts in `/tmp`; no generated recompilation
output, dependency source, production fix, commit or push.

Closing checks:

- `rtk proxy git diff --check`: exit 0 in both Tooie and RT64.
- New untracked audit: no trailing-whitespace diagnostics; `git diff
  --no-index --check /dev/null <audit>` returns 1 for the differing file.
- RT64 dirty-diff SHA-256 remains exactly the value above after the review.
- All requested final field names are present in this report.
- `markdown_vault_audit`: ran across 256 files; 2 errors and 71 warnings
  remain. Both errors are outside this change: an existing code-like link
  in `p09-projective-w-production-fix.md:154` and the existing `plan/evidence`
  directory link in `VAULT.md:6`. No issue targets this new audit. This is
  not a globally clean vault result; unrelated cleanup was not performed.
- Tooie worktree remains dirty: 17 tracked modified paths and 78 untracked
  status entries at verification. This task adds the audit and edits only
  HANDOFF, session handoff, P09 phase, transform-interpolation RE, VAULT,
  and supersession notices in the prior review/pose evidence.
- Stable specs were reviewed for applicability; no new production contract
  was introduced. Canonical matrix knowledge is updated in RE. No material
  code change occurred, so source reindexing is unnecessary.
