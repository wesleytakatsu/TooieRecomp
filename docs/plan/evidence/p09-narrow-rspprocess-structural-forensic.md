---
title: "P09 narrow RSPProcessCS structural forensic"
description: "Static audit after V7, RSPModify absence, V=0, V5B world-lerp refute, and Q' witness: RSPProcess has no remaining credible target-local failure."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-07
tags: [p09, kazooie, feather, rspprocess, forensic]
---

# P09 narrow RSPProcessCS structural forensic

Bounded objective: determine whether `RSPProcessCS` still contains a
plausible target-local failure after CPU handoff, RSPModify, velocity,
world-lerp, and ProjectionProcessor Q' have been eliminated or strongly
refuted. Static analysis first. Offline use of the existing Q' CSV.
No game. No production fix. No new witness. No GPU readback or added
synchronization. No commit. No push.

Do not use Q-witness class C as `ROOT_CLASS`.

```text
CPU_HANDOFF_STATUS: CORRECT (V7 VERSION=2)
PROJECTIONPROCESSOR_PRIMARY_CAUSE: REFUTED_FOR_CURRENT_WITNESS
TARGET_RSP_MODIFY_COMMANDS_STATICALLY_POSSIBLE: NO
RSPPROCESS_STATUS: A
RSPPROCESS_STATICALLY_CLOSED: YES
CURRENT_BEST_ROOT_CLASS: POST_HANDOFF_RENDER_PATH_BUG_UNRESOLVED
ROOT_CONFIDENCE: MEDIUM for RSPProcess close; LOW for overall visual root
PRODUCTION_FIX_CANDIDATE_FOUND: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
```

Checkout: Tooie `ff5b6d9` dirty; RT64 `6f1c2d9` dirty sibling
`BanjoRecomp/lib/rt64`. Capture used:
`/tmp/tooie-kazooie-feather-postprocess-q-v1-status.txt` and
`/tmp/tooie-kazooie-feather-postprocess-q-v1.csv` (512 rows: 384 target,
128 body).

## 1. Target RSPProcess effective formula (`PROVEN` source)

`src/shaders/RSPProcessCS.hlsl` `CSMain`:

```text
tfPos = mul(mul(viewProjTransforms[viewProjIndex],
                worldTransforms[worldIndex]),
            float4(P - V*(1-curFrameWeight), 1))
if tfPos.w == 0: tfPos.w = 1e-6
ndc = tfPos.xyz / float3(tfPos.w, -tfPos.w, tfPos.w)
screen = ndc * rspViewportVector[viewProjIndex].scale
       + rspViewportVector[viewProjIndex].translate
dstPos = float4(screen, tfPos.w)
```

Closed substitutions for the six plume DLs:

- `V = 0` → `P - V*(1-curFrameWeight) = P`
- Q' vs Q_current screen delta is negligible on the Q witness
  (`TARGET_LARGE_QPRIME_DELTA=0/384`) and is a **shared** camera slot
  (`viewProjIndex=4` for all 512 samples)

```text
TARGET_RSPPROCESS_EFFECTIVE_FORMULA:
  clip = mul(mul(Q, W'[worldIndex]), float4(P, 1))
  ndc  = (clip.x/clip.w, clip.y/(-clip.w), clip.z/clip.w)
  screenXY = ndc.xy * vp.scale.xy + vp.translate.xy
  with Q = Q' ~= Q_current for screen purposes
```

Remaining inputs that can make two otherwise-equivalent vertices differ
on screen: **P** and **W'[worldIndex]**. Shared across target and body:
Q/viewProjIndex, viewport[viewProjIndex], `curFrameWeight`, shader,
dispatch `vertexStart=0` on the per-workload reset path.

Fog, texgen, lighting, and vertex color do not write `dstPos`.

## 2. Remaining target-local variables

| VARIABLE | SOURCE | SCOPE | V7_PROVEN_CORRECT | POST_V7_MODIFIED | CAN_EXPLAIN_LOCAL_FEATHER_ONLY_FAILURE |
| --- | --- | --- | --- | --- | --- |
| P | `posFloats` stride 3 → `Buffer<float>` | PER_VERTEX | YES | NO | NO (handoff closed; intended mesh) |
| V | `velFloats` | PER_VERTEX | YES (0) | only if vertexInterpolation≠SKIP; Tooie path SKIP | NO (collapses to P) |
| worldIndex | `worldIndices` `uint16` `R16_UINT` | PER_VERTEX | YES | NO rebase | NO (same namespace as shader) |
| W' | `lerpWorldTransforms[i]` or `worldTransforms[i]` | PER_DRAW index i | YES bytes/source | temporal lerp only | NO as primary (V5B) |
| viewProjIndex | `viewProjIndices` `R16_UINT` | PER_VERTEX | YES (=4) | NO | NO (shared with body) |
| Q' | `modViewProjTransforms[4]` | PER_VIEWPROJ | selection YES | ProjectionProcessor | NO (Q witness; shared; ~0 screen delta) |
| viewport | `rspViewports[viewProjIndex]` | PER_VIEWPROJ | n/a | NO per-vertex | NO (shared index 4) |
| global/dispatch index | `vertexStart + SV_DispatchThreadID` | PER_VERTEX | structurally YES | `vertexStart=0` after reset | NO |
| flags (fog/lookAt/light) | per-vertex uints | PER_VERTEX | n/a | NO | NO for position |
| transform mode | none in RSPProcess besides V | GLOBAL | n/a | NO | NO |
| curFrameWeight | push constant | PER_WORKLOAD | n/a | YES | NO (V=0; shared) |

Do not reopen V7 identities, RSPModify, world-lerp as primary, or Q' as
localized primary.

## 3. Matrix layout / multiplication

```text
CPU_WORLD_MATRIX_LAYOUT: interop::float4x4 is float m[4][4] (C row-major storage). hlslpp::mul(M,v) is column-vector (row_i·v). Fingerprints print last-row translation m[3][0..2].
GPU_WORLD_MATRIX_LAYOUT: StructuredBuffer<float4x4>; no pack_matrix pragma in tree; HLSL default column-major packing.
CPU_VIEWPROJ_LAYOUT: same interop type as world.
GPU_VIEWPROJ_LAYOUT: same StructuredBuffer<float4x4> as world.
RSPPROCESS_MULTIPLICATION_ORDER: mul(mul(Q, W), float4(P,1))  // column vector
EXPECTED_N64_TRANSFORM_ORDER: vertex * W * Q (row-vector) which equals Q * W * v if matrices are stored/interpreted consistently.
MATRIX_LAYOUT_OR_MUL_CONVENTION_MISMATCH: NO for production (uniform shader; body draws at full size).
MATRIX_CONVENTION_LOCALIZED_ARTICULATION_FIT: NONE
```

A packing transpose, if present, applies to every matrix in the same
buffer class. It cannot preferentially hit plume bones while leaving
torso/head intact. CPU Q-witness absolute screens collapsed (see §11)
and must not be treated as GPU screen truth; that is a witness
reconstruction limitation, not a remaining RSPProcess target-local bug.

## 4. WorldIndex namespace

```text
WORLDINDEX_CPU_NAMESPACE: drawData.worldIndices[globalVertex] indexes drawData.worldTransforms / lerpWorldTransforms
WORLDINDEX_SHADER_NAMESPACE: srcWorldIndices[vertexOffsetIndex] indexes worldTransforms StructuredBuffer
WORLDINDEX_REBASE_AFTER_V7: NO
WORLDINDEX_POINTS_TO_SAME_W_PROVEN_BY_V7: YES
```

Upload range is `{0, worldTransforms.size()}`. Shader uses the index
directly. `R16_UINT` zero-extends into `Buffer<uint>`. Close the rebase
hypothesis.

Q CSV world indices (not reopened as identity bugs): body torso `81`,
Kazooie head `114`, plume `124–131`.

## 5. TransformProcessor non-temporal rewrite

`TransformProcessor::process` when previous frame is mapped:

- `rigidBody.lerp` → `lerpWorldTransforms` (temporal; **closed** as
  primary by V5B `NO_INTERP` visual `SAME`)
- `inverse` + `transpose` → `invTWorldTransforms` (normals / lighting /
  `RSPWorldCS`, **not** raster `screenPos`)

When unmapped: copies original W into lerp and still builds invT.
Upload of position matrices is `lerpWorldTransforms` if `prevFrame !=
nullptr`, else original `worldTransforms`. Sampled Q rows all have
`process_transforms=1` and `w_source=LERP_WORLD`.

```text
TRANSFORMPROCESSOR_NON_TEMPORAL_MATRIX_REWRITE: YES for invT only (not RSPProcess position). Position W' rewrite is the temporal lerp/decompose path.
NON_TEMPORAL_W_REWRITE_LOCALIZED_FIT: NONE as primary
```

Parent-107 uniform scale ~0.417→1 remains a `PROVEN` dataflow fact, not
a root (`TOO_STRONG` to call erroneous). V5B already ran without
temporal rewrite and the visual stayed `SAME`.

## 6. Precision / types

```text
RSPPROCESS_PRECISION_PATH:
  P,V: Buffer<float> float32 stride 3
  W,Q: StructuredBuffer<float4x4> float32
  clip/ndc/screen: float32
  dstPos: RWStructuredBuffer<float4>
  no float16, no packed position, no integer screen in this shader
TARGET_CAN_HIT_NUMERIC_EDGE_CASE: UNKNOWN in the abstract; not observed in 512 CPU samples
```

Exact `clip.w == 0` is replaced by `1e-6`. Near-zero (non-exact) W is
not guarded.

## 7. Clip-W / perspective divide

```text
RSPPROCESS_CLIP_W_HANDLING: if (tfPos.w == 0.0f) tfPos.w = 1e-6f; then divide
CAN_NEAR_ZERO_W_CREATE_LARGE_SCREEN_DISTORTION: YES (theoretical; only exact 0 is snapped)
CAN_PLUME_TARGETS_REACH_NEAR_ZERO_W_WHILE_BODY_DOES_NOT: NO on the 512 CPU samples; GPU absolute clip.w not read
EXISTING_Q_WITNESS_CONTAINS_CLIP_W_OR_SUFFICIENT_INPUTS: YES for CPU clip.w (screen_* .w); NO for GPU clip.xyzw or viewport scale/translate
```

Offline CPU `screen_prime.w` (clip.w):

| set | n | min \|w\| | max \|w\| | sign |
| --- | ---: | ---: | ---: | --- |
| target | 384 | 5.77e8 | 2.58e9 | all + |
| body | 128 | 7.12e7 | 4.28e8 | 96 +, 32 − |

Zero samples with `|w| < 100`. Body, not plume, is the set with mixed
sign. Absolute CPU clip.w is inflated by the witness mul/layout and is
**not** GPU ground truth, but it does not show plume-only near-zero W.

## 8. Viewport / screen conversion

```text
VIEWPORT_IS_SHARED_BY_TARGET_AND_BODY: YES
VIEWPORT_TEMPORAL_OR_RANGE_BUG_PLAUSIBLE: NO as target-local (same viewProjIndex 4)
LOCALIZED_FEATHER_FIT: NONE
```

Shader: `rspViewportVector[viewProjIndex]`. RasterVS then converts N64
screen XY to framebuffer NDC and reapplies homogeneous W; `screenScale`
defaults to `{1,1}` and is per-projection batch, not per-plume-vertex.

## 9. Target vs body RSPProcess differences

Same workload class, same shader, same Q/viewProj 4, same viewport slot,
`V=0`, `vertexStart=0`.

```text
TARGET_VS_BODY_RSPPROCESS_DIFFERENCES:
  P (mesh-local) — intended
  worldIndex / W' (torso 81, head 114, plume 124–131) — intended articulated bones
  clip.w magnitude (CPU recon; plume larger than body) — not a position-writer defect; not near-zero
```

Compatibility with the visual:

| difference | MOVEMENT_ONLY | LOCALIZED_TO_ARTICULATED_GEOMETRY | HIGH_FREQUENCY_VISUAL_DISTORTION |
| --- | --- | --- | --- |
| P | weak (static tables) | yes (different mesh) | no by itself |
| W' animation | yes (intended) | yes (different bones) | only if W is wrong; V7+V5B closed that as primary |
| shared Q/viewport | yes (camera) | no | no as feather-only |

## 10. Temporal discrimination

```text
SURVIVING_MECHANISM_TEMPORAL_DRIVER: none inside RSPProcess after closed evidence.
```

Movement still changes guest W (and camera Q). Guest W identity is V7
`CORRECT`; temporal W lerp is V5B-refuted as primary; Q is shared and
Q'-delta is negligible for localization. A candidate with no
target-local movement-dependent input is ranked weak. RSPProcess itself
has none left.

Same-vertex CPU screens across Q-witness generations 780–787 differ by
at most 0.001 N64 units (print quantization / collapsed recon). That is
**not** a GPU temporal measurement.

## 11. Existing Q-witness CSV (offline)

```text
EXISTING_CAPTURE_CAN_TEST_RSPPROCESS_NUMERICS_OFFLINE: PARTIAL
  YES: relative Q counterfactuals, shared viewProj, CPU clip.w distribution, worldIndex sets
  NO: GPU clip.xyz, GPU clip.w, full 4x4, P, viewport scale/translate
```

Status file already classified Q as C (negligible Q' vs current). All
512 samples: `viewproj_index=4`, `BODY_CONTROL_SHARES_TARGET_Q=YES`,
`TARGET_LARGE_QPRIME_DELTA=0`, `BODY_LARGE_QPRIME_DELTA=0`.

CPU reconstructed `screen_prime.xy` for **all** identities sits at
`~152.019, 113.996` (range 0.001). Torso/head/plume W translations still
differ by ~100 world units. Treat absolute CPU screens as
**not GPU-equivalent**. Relative Q' vs Q_current comparison remains
valid as a same-convention counterfactual and is **not** a localized
feather discriminator anyway (shared Q).

Do not request another RSPProcess capture for clip.w or Q.

## 12–15. Verdict

```text
RSPPROCESS_STATUS: A
RSPPROCESS_STATICALLY_CLOSED: YES
WHY: after V=0, no G_MODIFYVTX, no worldIndex rebase, V5B world-lerp
  closed, Q' shared and negligible, viewport shared, float32 path with
  only exact-0 W snap, the only target-local RSPProcess inputs are P and
  W' which V7 already proved correct. No remaining credible target-local
  RSPProcess mechanism.
TOP_RSPPROCESS_MECHANISM: NONE
CURRENT_BEST_ROOT_CLASS: POST_HANDOFF_RENDER_PATH_BUG_UNRESOLVED
ROOT_CONFIDENCE: MEDIUM (RSPProcess close); LOW (overall visual root)
BEST_NEXT_WITNESS: NONE
WHY_THIS_SINGLE_VALUE_OR_COMPARISON_IS_DECISIVE: n/a — do not instrument RSPProcess further
EXPECTED_CAPTURE_COUNT: 0
NEXT_UNCLOSED_ASSUMPTION: that the user-visible wobble is produced by these six right-plume DLs as a post-handoff position-writer defect. RasterVS is passive aside from shared screenScale. RSPModify absent. Next work must test geometry identity / which draw is on-screen during the glitch / a different 0x62B path, without GPU readback.
```

Do not jump to GPU readback. Do not implement V8. Do not implement
another RSPProcess witness.

## Validation this turn

Docs-only forensic. No code. No game. No build.

```text
PRODUCTION_FIX_CANDIDATE_FOUND: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NO_COMMIT: YES
NO_PUSH: YES
```
