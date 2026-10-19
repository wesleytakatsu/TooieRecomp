---
title: "Transform interpolation"
description: "Articulated WORLD attachments break under decomposed lerp; a generic multi-world GameCall selector can target 16-entry lerp. Default-off; not shipped."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-10-16
tags: [reverse-engineering, interpolation, rt64]
---

# Transform interpolation

## Current (2026-10-16) — Tooie-only default-ON workaround (Phase 1)

As of 2026-10-16, TooieRecomp installs `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1` by default prior to renderer initialization (`apply_default_before_renderer_init()` in `main.cpp`) whenever the environment variable is unset. This enables the shader-local `tfPos` CURRENT bypass as an operational presentation workaround for normal gameplay runs without requiring manual flags. Note that the upstream RT64 codebase default remains OFF; this default-ON behavior is exclusive to Tooie integration. Older evidence documenting the control as default-OFF reflects pre-workaround historical state. Any explicit environment value (including `"0"` to rollback to original interpolation, or `""`) is preserved. Full details, rollback instructions, and Phase 2 FPS preset plans are documented in [[../../plan/evidence/p09-default-current-workaround-and-fps-plan]].

## Prior (2026-10-15) — proven diagnostic controls (endpoint vs global)

Two diagnostic controls established the causal baseline for high-rate vertex stretch (`USER_OBSERVED` `PASS`):

- `TOOIE_DIAG_FORCE_CURRENT_FRAME=1`: global weights control in `rt64_workload_queue.cpp:24-34,455-456`. Forces `curFrameWeight=1`, `prevFrameWeight=0` across Projection, Transform, Tile, LookAt, RSP, and Vertex domains. Presentation cadence, extra presents, and `GameFrame::match` remain intact. Diagnostic-only; repeats current frame geometry without interpolation. Provenance: [[../../plan/evidence/p09-temporal-coherency-root-cause]], [[../../plan/evidence/p09-high-rate-force-current-frame]].
- `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1`: shader-local control in `rt64_rsp_processor.cpp:60-67,7161` and `RSPProcessCS.hlsl:78-84`. Bypasses position interpolation by directly assigning `tfPos = endpointViewProj · endpointWorld · pos` (CURRENT). Does NOT alter global weights, present cadence, or other domain interpolation. Fog derives from `tfPos.z/w` and thus follows CURRENT; texgen `tc` continues to use `curFrameWeight`. Position interpolation is bypassed (does not preserve continuous position interpolation). Provenance: [[../../plan/evidence/p09-projective-w-production-fix]], [[../../plan/evidence/p09-causal-baseline-and-fix-direction]].

## Prior (2026-09-17) — generic multi-world GameCall selector

`PROVEN` (`OTHER`, static RT64): before WORLD lerp, a workload can mark
WORLD indices whose GameCalls use more than one distinct vertex
`worldIndices` slot. That is generic (no model/bone/DL hardcode).
`TransformGroup` alone is too coarse when Tooie WORLD stays on
`G_EX_ID_AUTO`. `minWorldMatrix`/`maxWorldMatrix` is a dense range, not
a distinct set.

Default-off production candidate
`TOOIE_TEST_P09_SELECTIVE_ELEMENTWISE_WORLD` applies
`lerpMatrix(PREV, CURRENT, w)` (all 16 entries) to those WORLD indices
only. Other WORLDs keep `RigidBody::lerp`. VIEW/PROJECTION unchanged.
Visual acceptance is `NOT_CAPTURED`. Provenance:
[[../../plan/evidence/p09-generic-selective-world-interpolation-production-design]].

Root class is unchanged
(`NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`).
Do not default-on. Failed W-sign clip snap is not production.

## Prior (2026-09-16) — composed WORLD interpolation vs attachments

`PROVEN` (`USER_OBSERVED`, repaired V2 CPU pairing): independent
nonlinear interpolation of already-composed WORLD matrices breaks
parent/child attachments that agree at both endpoints.

```text
ROOT_CAUSE_CLASS=
NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS
LIVE_WORLDSPACE_JOIN_MAX=5.45340872
ELEMENTWISE_WORLDSPACE_JOIN_MAX=1.00010824
ELEMENTWISE_WORLD_POINT_LINEARITY_PROVEN=YES
ELEMENTWISE_WORLD_FIXES_ALL_CORRECTED_PATHOLOGIES=YES
ELEMENTWISE_WORLD_INTRODUCES_MATERIAL_NONRIGIDITY=YES
```

Do **not** ship global elementwise WORLD interpolation. RT64 production
WORLD path remains `RigidBody::lerp` TRS + bundled quaternion
(`hlslpp::slerp` with `1-weight`) when `TransformGroup.decompose` is
valid (default true for WORLD). VIEW/camera groups default
`decompose=false` and stay elementwise. There is no generic parent WORLD
hierarchy in `TransformProcessor`.

The bundled quaternion interior law is nonstandard
(`NONSTANDARD_WORLD_SLERP_LAW_CORRELATES_WITH_PATHOLOGY=YES`) but is
**not** proven sufficient. Isolated `STANDARD_SLERP_WORLD` is
implemented as a CPU diagnostic and is `NOT_CAPTURED`. Provenance:
[[../../plan/evidence/p09-elementwise-world-interpolation-causal-discriminator]],
[[../../plan/evidence/p09-standard-slerp-world-production-candidate-discriminator]].

## Prior (2026-09-16) — CPU diagnostic helper matches SPIR-V

`PROVEN` (`OTHER`, static SPIR-V plus independent CPU algebra): production
Vulkan RSPProcessCS still lowers tfPos to `OpMatrixTimesMatrix(W,Q)` then
`OpVectorTimesMatrix(p,WQ)`. P09 CPU diagnostics now evaluate
`mul(pos4,mul(WORLD,VIEWPROJ))`. Physical WORLD is `mul(pos4,WORLD)`.
This is a measurement repair, not a production interpolation change.
Offline synth fixtures A/B/C and a 2026-09-16 DXC recompile of
RSPProcessCS confirm the match. Game root remains `UNRESOLVED`. Provenance:
[[../../plan/evidence/p09-cpu-diagnostic-shader-semantics-repair]],
[[../../plan/evidence/p09-cpu-shader-matrix-semantics-and-homogeneous-audit]].

## Prior (2026-09-15) — CPU/shader matrix action correction

`PROVEN` (`OTHER`, static SPIR-V plus independent CPU algebra): RT64 stores
four matrix rows contiguously; affine translation occupies row 3. Upload
copies these bytes without transposition. The current Vulkan RSPProcessCS
binary uses RowMajor/MatrixStride 16/ArrayStride 64 and lowers HLSL tfPos
to `OpMatrixTimesMatrix(W,Q)` then `OpVectorTimesMatrix(p,WQ)`, where CPU
Q=`V*P`. Correct CPU evaluation is `mul(p,mul(W,Q))`.

`CORRECTED/SUPERSEDED`: prior CPU factorial/pose expression
`mul(mul(Q,W),p)` was not equivalent to shader semantics. WORLD xyz-only
metrics also applied W from the wrong side. For verified affine inputs,
correct WORLD homogeneous w is 1; old translation-to-w observations are
diagnostic artifacts. General projective equality requires proportional
4D vectors and appropriate clip-domain handling; no hierarchy fix follows.

Old CPU metric-based causal attributions below are historical, superseded
as shader proofs. The user visual FORCE_CURRENT result is independent and
retained. Game root `UNRESOLVED`. Pose accepted-count and 0/152 fix rate do
not establish a shader-valid subset. Provenance:
[[../../plan/evidence/p09-cpu-shader-matrix-semantics-and-homogeneous-audit]].

## Prior (2026-09-15) — validated camera-pose VIEW diagnostic

`ROOT_CAUSE_CLASS=UNRESOLVED`. Mapping-coherent fallback remains `REFUTED`
for the captured pathology. The specified CPU arm
`EXACT_ORDER_VALIDATED_CAMERA_POSE_VIEW` is implemented (default-off) and
not yet captured. Provenance:
[[../../plan/evidence/p09-exact-order-validated-camera-pose-view]],
[[../../plan/evidence/p09-independent-architecture-root-cause-review]].

`PROVEN` (`OTHER`, static/CPU): default VIEW interpolation acts directly on
inverse-camera matrix entries. WORLD interpolates composed transforms.
`RigidBody::lerp` calls have correct endpoint direction, 0 PREV and 1 CURRENT.
The bundled quaternion helper is compensated by `1-weight` but follows a
nonstandard interior rotation law, not mathematical constant-speed SLERP.
Whether this causes the visible defect is `UNKNOWN`.

`PROVEN` proof limitation: the CPU WORLD JOIN helper discards w from
`WORLD*pos4`; near-unity xyz gap ratios do not establish equality/coherence
of the complete homogeneous inputs. The exact diagnostic order remains
`((VIEW*PROJECTION)*WORLD)*pos4`. A physical row-affine relative-pose
diagnostic must not replace that expression or move PROJECTION.

Pose JOIN metrics, pathological coverage, relative camera/object path
deviation, WORLD homogeneous w, and WORLD SLERP-law deltas are
`NOT_CAPTURED` until user recapture. No production change.

## Prior (2026-09-15)

`RSPProcessCS` interpolated `tfPos` is causal (`PROVEN`). The
16-way LIVE/CURRENT factorial of VIEW/PROJECTION/WORLD/POSITION
is captured (`USER_OBSERVED`): the minimum CURRENT set matching
ALL_CURRENT is CLCL (CURRENT VIEW+WORLD, LIVE PROJECTION,
LIVE POSITION). POSITION velocity is exactly 0. PROJECTION is
not decisive. World-matrix non-rigidity remains `REFUTED`.
Do not implement CURRENT VIEW+WORLD. Whether the VIEW×WORLD
interaction is independent world-hierarchy interpolation,
separate VIEW×WORLD interpolation, or both is `NOT_CAPTURED`
until the CPU discriminator recapture. Provenance:
[[../../plan/evidence/p09-view-world-interaction-root-discriminator]],
[[../../plan/evidence/p09-tfpos-factorial-term-attribution]].

```text
TFPOS_FACTORIAL_BEST_POLICY: CLCL
STRONGEST_PAIR: VIEW_WORLD
POSITION_TEMPORAL_TERM_CAN_AFFECT_P09: NO
VIEW_WORLD_INTERACTION_ROOT_CLASS: NOT_CAPTURED
GENERIC_HIERARCHY_INFORMATION_AVAILABLE: NO
```

## Prior (2026-09-12)

`RSPProcessCS` interpolated `tfPos` is causal (`PROVEN`). Forcing
that clip to endpoint CURRENT fixes the visible feather glitch.
Endpoint `prevW*curW <= 0` is `VISUALLY INSUFFICIENT`. Sidecar
content still matches the CPU witness. Whether residual explosions
are same-sign projective collapse of live W is `NOT_CAPTURED` until
the binding-diag POLICY_A replay is recaptured. Provenance:
[[../../plan/evidence/p09-projective-w-production-fix]].

```text
RSPPROCESS_TFPOS_CAUSAL: PROVEN
ENDPOINT_SIGN_CROSSING_ONLY_POLICY: VISUALLY INSUFFICIENT
PRODUCTION_FIX_CLASS: PER_VERTEX_CURRENT_ENDPOINT_ON_W_SIGN_CROSSING
TOOIE_GAMEPLAY_NEAR_CLIPPING_ENABLED: NO
```

## Prior (2026-09-12) — VERSION=4 candidate

VERSION=4: 75 live explosions, all with `prevClip.w * curClip.w <= 0`.
PR23 SIMPLE and Tooie P/V scale-gauge discontinuity are `REFUTED`.
Live sidecar CPU vectors match the same witness (`PROVEN`). The
per-vertex CURRENT endpoint fallback remains the production
candidate; visual FAIL is not explained by sidecar content, SPIR-V
omission, or a proven unique upload-lifetime bug. Provenance:
[[../../plan/evidence/p09-projective-w-production-fix]].

```text
PROJECTIVE_W_SIGN_CROSSING_ROOT_STATUS: PROVEN
PRODUCTION_FIX_CLASS: PER_VERTEX_CURRENT_ENDPOINT_ON_W_SIGN_CROSSING
TOOIE_GAMEPLAY_NEAR_CLIPPING_ENABLED: NO
```

## Prior (2026-09-09)

VERSION=2: 51 live explosions, 51 SIMPLE. World DECOMPOSE vs SIMPLE
is `REFUTED` as the captured cause. Live interpolates VIEW and WORLD
independently (`clip = mul(mul(V,P),W)*pos`). Dual B/C signatures
remain. Recapture witness v3 for combined-relative / camera-pose
counts. Provenance:
[[../../plan/evidence/p09-relative-view-world-interpolation-root-cause]].

```text
DECOMPOSE_VS_SIMPLE_CAUSAL_STATUS: REFUTED
PR23_SIMPLE_FIX_STATUS: REFUTED
CURRENT_ROOT_CLASS: INDEPENDENT_VIEW_AND_WORLD_INTERPOLATION_OF_COUPLED_RELATIVE_MOTION
WORLD_MATRIX_INTERPOLATION_CAUSAL_STATUS: CORRECTED/SUPERSEDED
```

## Prior (2026-09-09) — DECOMPOSE / SIMPLE candidate

High-framerate walking stretch is `USER_OBSERVED` `FIXED` under global
CURRENT and `SAME` under V5C. V5C only froze the **token-bound first
G_VTX** of each tracked GeoCmd2 bone. JOIN `G_POPMTX` parent verts
allocate a new `worldIndex` and still interpolate. Passive CPU
reconstruction of the live intermediate equation explodes 58.596×;
offline current-world analysis removes the typical case.

Live untagged worlds use default `TransformGroup`
(`matrixId=G_EX_ID_AUTO`, `decompose=true`). `RigidBody::lerp` then
decomposes **already-composed** bone worlds. That is
`G_EX_INTERPOLATE_DECOMPOSE`. `G_EX_INTERPOLATE_SIMPLE` is
component-wise `lerpMatrixComponents` and is **not** selected
without `gEXMatrixGroup`.

```text
WORLD_MATRIX_INTERPOLATION_CAUSAL_STATUS: SUPPORTED
TEMPORAL_MATRIX_INTERPOLATION_CAUSAL_STATUS: CORRECTED/SUPERSEDED
  (no longer REFUTED for every concrete world slot used by visible triangles)
```

Do not treat `PAIR_CHANGE` as proven wrong pairing. Do not
productionize `TOOIE_DIAG_FORCE_CURRENT_FRAME`. Do not add mixed-time
CURRENT visual overrides. SIMPLE recapture is
[[../../plan/evidence/p09-boned-interpolation-counterfactual]].
Provenance:
[[../../plan/evidence/p09-world-occurrence-identity-root-cause-audit]],
[[../../plan/evidence/p09-passive-high-rate-temporal-coherency-witness]],
[[../../plan/evidence/p09-temporal-coherency-root-cause]].

## Post-triangle raster contract (2026-09-05, PROVEN static source)

At inspected RT64 `6f1c2d99` plus protected diagnostics, `drawIndexedTri`
appends workload-global vertex indices to `DrawData.faceIndices`. `State::flush`
copies DrawCall metadata into GameCall; `State::fullSync` assigns each indexed
mesh a face-array slice. Neither rewrites per-vertex `worldIndices`. Normal
indexed raster drawing has no downstream world-index compaction or baseVertex
rebase (`drawIndexedInstanced(..., faceIndicesStart, 0, 0)`).

`DrawData.worldIndices` is `uint16_t`, uploaded with a 2-byte stride and R16_UINT
view. `RSPProcessCS` loads `Buffer<uint> srcWorldIndices[v]` and directly selects
`worldTransforms[index]`. It computes raster screen position from local position,
optional local velocity, world matrix, projection and viewport. RasterVS consumes
that computed screen-position buffer. `RSPWorldCS` separately computes world
position/normal/motion and is not the ordinary raster POSITION source.

Matrix arrays remain indexed by the current workload's transform number.
TransformProcessor remaps the *previous matrix source* using `prevTransformIndex`
and builds current/previous output vectors in current-index order; it does not
remap vertex world indices. Upload range offsets preserve array positions.
`RSP::modifyVertex` copies all vertex streams, including worldIndex, when modifying
a used slot; old face indices persist. Screen XY/Z modifications are separately
replayed by RSPModifyCS and are not measured by V6C.

NO_INTERP for the seven feather matrices does not turn off projection processing,
resource uploads or render replay. Ordinary TransformGroup vertex interpolation
is SKIP and G_VTX initializes local velocity to zero; shader support for velocity
does not establish an active feather vertex-interpolation path.

V7 VERSION=3 16-generation window (2026-09-07): `GameFrame::set` binds
one workload; `match` pairs the previous ring slot
(`(ring-1) mod 4`). All 32 RENDER_SELECTION rows are mapped/matched
with `process_transforms=1`. Face slices stay on that workload.
Draw-temporal association for the 14 visible feather DLs is `CLOSED`
([[../../plan/evidence/p09-post-position-draw-temporal-and-depth-state-forensic]]).

**CORRECTED evidence scope:** accepted V6C own provenance is not all-geometry or
final-upload proof. V6C runs before real index append; unresolved root joins and
untargeted distal joins remain. `AFTER_RSP_TRIANGLE_SUBMISSION` is not an established
first-bad layer. Root UNKNOWN. No renderer hypothesis is promoted into a stable
contract. Full source spans and provenance:
[[../../plan/evidence/p09-post-triangle-architectural-review]].

## Corrected live `0x62B` single-G_MTX path (`PROVEN_RUNTIME`)

The live Banjo snout is not the earlier dual-matrix `E2180` case.  The
sibling-aware runtime witness proves:

```text
0x62B + 0x11CB8 (type 2, bone index 11, boneAnimID 42)
  -> func_800DCF48
  -> func_800ADE2C(D_8012C968, 11)
  -> func_80019AA0(0x8012CE58, ADE2C result)
  -> mlMtxApply
  -> guMtxF2L / G_MTX
  -> push, child list, pop
```

`func_800ADE2C` selects the local bone matrix at
`MEM_W(D_8012C968) + 0x50 + (index << 6)` (snout index 11 → `+0x2C0`;
Kazooie plume index 30 → `+0x7D0`).  The slot is written by
`func_800AE820` before draw.  ADE2C does not receive a precomposed
model-view matrix.  `func_80019AA0(a0=0x8012CE58, a1=ADE2C)` multiplies
that local matrix by the current model float matrix and writes the result
to the next 0x40-byte slot of the **mlMtx float stack** whose cursor lives
at **`D_8007BCB0`** (arena base `0x8007B4F0` via `mlMtxSet`).  The function
is void; the completed float matrix is `MEM_W(D_8007BCB0)` after return.

`mlMtxApply` (`func_80018C28`) then:

```text
a1 = entry a0                 // Gfx Mtx dest / later G_MTX operand
a0 = MEM_W(D_8007BCB0)        // float stack top
guMtxF2L(a0=float src, a1=Mtx dest)   // 0x8002ED00; libultra order
```

DCF48 `s0` is the **Gfx context**, not the GeoCmd node.
`mlMtxApply(a0=lw(s0+4))` and `G_MTX DA380002` both use that gfx Mtx
pointer; DCF48 then does `s0+4 += 0x40`.  After children it `mlMtxPop`
(`cursor -= 0x40`) and emits `G_POPMTX`.  `D_8007BCD0` is **not** this
cursor (`REFUTED`). V2 NaNs from that address are diagnostic, not game
evidence.

The capture proves 2,243 DCF48 accepts and 2,243 ADE2C current-matrix hits.
It does not capture the full 16-word matrix, the RT64 command identity, or a
previous matrix.  Guest-matrix semantic validity and the RT64 interpolation
result therefore remain `UNKNOWN`.

## RT64 ungrouped single-G_MTX identity (`PROVEN_STATIC`)

RT64 decodes an ordinary F3DEX2 `G_MTX` through `GBI_F3DEX2::matrix` into
`RSP::matrix`, then `RSP::matrixCommon`.  The latter applies LOAD or MUL and
PUSH to the CPU model-matrix stack and records the segmented and physical
guest addresses.  When vertices are processed, RT64 snapshots the effective
world transform and associates it with the current `TransformGroup`.

Without `gEXMatrixGroup`, the group remains the default `matrixId =
G_EX_ID_AUTO`.  Frame matching does not require a stable guest matrix
address: the call hash folds the group IDs, and AUTO groups opt into
transform matching; matching then pairs transform ranges by their position in
the matched draw call and updates the previous/current rigid-body data.
Physical addresses are retained as workload metadata and for address-based
group editing, but are not themselves the frame-stable logical identity for
an ungrouped AUTO transform.  Consequently, rotating temporary slots alone
cannot establish RT64 identity churn.  A changed command/call order could
still affect matching, but the current capture does not measure that order or
the matrix-stack depth.

`gEXMatrixGroup` is an explicit group-stack command.  It supplies a stable
`matrixId` and interpolation properties to subsequent world transforms; it
affects ordinary single `G_MTX` submissions, but does not remove the physical
address bookkeeping.  Whether the live symptom requires such an explicit
group remains unproven.

## RT64 world-transform seam (`PROVEN_STATIC`)

F3DEX2 `G_VTX` (`0x01`) → `RSP::setVertex` →
`RSP::setVertexCommon<true, sizeof(Vertex)>`. A world transform is
created in `setVertexCommon` when `modelViewProjChanged` /
`modelViewProjInserted`. DrawCall / TransformRange are later
`State::flush` / triangle submit. `G_MTX` itself does not create a
draw call.

Therefore a diagnostic that binds only at `G_MTX` and then drops the
token before the first `G_VTX` cannot observe `matchTransform`. V3's
scalar `p09DiagnosticMatrixToken` was overwritten by unmatched child
bone `G_MTX` (104/105) before any feather `G_VTX`. Class:
`TOKEN_EXPIRES_BEFORE_CHILD_DL`. Opcode hook is **not** wrong
(`USES_SET_VERTEX_COMMON: YES`).

## Kazooie feather V4 runtime result (`PROVEN_RUNTIME`)

The V4 capture for the live Kazooie feather (`0x62B+0x124A0`, index 30,
`boneAnimID=103`) proves that the modelview-parallel diagnostic token survives
the nested child-matrix topology. It records 1,874 matched feather G_MTX token
enqueues and 1,874 same-depth restore aliases with zero restore ambiguity, then
observes the VERTEX/world-transform path in `setVertexCommon`. The identity
CSV is capped and therefore its 308 MATRIX, 921 VERTEX, and 307 RESTORE rows
are not whole-run counts; callback-time `VERTEX_COUNT=0` is expected because
`setVertexCommon` creates the world-transform record before appending vertices.

The real RT64 path is also reached: 328 CALL contexts are hash-matched, all
328 transform prerequisites are eligible, 500 candidates are valid, and 126
`matchTransform` calls complete. The seven real unmatched decisions are only
one-to-one allocator collisions (five current-transform-used and two
previous-transform-used), not semantic or candidate rejection. Across the 13
selected semantic feather contexts there are no cross-context matches or
semantic match failures.

This is a reusable correction to V3's interpretation: zero VERTEX rows in V3
was diagnostic token lifetime failure, while V4's nonzero VERTEX path and real
`matchTransform` results show that the current AUTO identity path is working for
the observed feather. Explicit identity groups are therefore not required to
make these matches. A possible remaining visual difference is the
post-match rigid-body interpolation mode supplied by an explicit group, but
that is only a bounded A/B hypothesis and is not a production root.

Provenance: [[../../plan/evidence/p09-kazooie-feather-v4-capture-analysis]].

## V4 one-to-one drops and isolated post-match A/B (`PROVEN_STATIC`)

The seven V4 `REAL_UNMATCHED` rows are allocator decisions, not semantic
identity failures: five are `CURRENT_ALREADY_MAPPED` and two are
`PREVIOUS_ALREADY_USED`. The V4 witness does not serialize the final
transform-to-DL/triangle association, so it cannot prove that any one row
removes interpolation from the visible `0x5AA` geometry. No drop is clearly
causal; the seven remain token-level `UNKNOWN` for visual causality.

The static post-match path is:

```text
GameFrame::match
  -> candidate selection / one-to-one checks
  -> GameFrame::matchTransform
  -> RigidBody::updateLinear/updateAngular/updatePerspective/updateDecomposition
  -> TransformProcessor::setup
  -> RigidBody::lerp(prevFrameWeight, prevTransform, curTransform, true)
```

An ordinary ungrouped `G_MTX` uses `G_EX_ID_AUTO`, decomposition enabled, AUTO
component modes, and AUTO ordering. AUTO IDs still use the AUTO
`matchScene`/candidate/`matchTransform` path; they do not enter the explicit
stable-ID map. `RigidBody::lerp` uses decomposed interpolation when valid and
falls back to component-wise matrix interpolation otherwise. AUTO components
use RT64's component heuristics; explicit `INTERPOLATE` forces a component to
lerp.

Banjo's `gEXMatrixGroupSimpleNormal` is not an ID-only change. It also sets
`decompose=false`, component modes to `INTERPOLATE`, tile interpolation to
`INTERPOLATE`, and ordering to `LINEAR`. Therefore an AUTO-ID SIMPLE group is a
valid comparable A/B, but it does not change only interpolation properties in
the literal sense. The first Tooie experiment instead uses a narrower RT64
post-match seam: after the existing pair is selected, a default-off
`TOOIE_DIAG_FEATHER_INTERP_SIMPLE=1` override applies only to the proven
`0x62B/+0x124A0` bone-30/anim-103 feather token. It changes decomposition and
the component interpolation modes, while preserving matrix identity, call
hash, candidate selection, previous-transform selection, and existing V4
telemetry.

This remains diagnostic plumbing, not a production tag or renderer policy.

## Kazooie feather V5 full-chain witness (`IMPLEMENTED_STATIC`)

The earlier SIMPLE A/B was `ROOT_ONLY`: its selector matched only live
`0x62B/+0x124A0`, index 30, `boneAnimID=103`. The user's `SAME` result
therefore refutes that isolated root experiment only. The bounded V4 token
classifier now covers the seven proven `0x62B` head-feather identities:
`79, 100, 101, 102, 103, 104, 105`; no arbitrary transform is included.

`TransformProcessor::process` now exposes the already selected RT64
previous/current world transforms, `prevFrameWeight`, `curFrameWeight`, and
the final CPU matrix to a default-off diagnostic CSV. This is a post-match
witness; it does not affect identity matching or pair selection.

`TOOIE_DIAG_FEATHER_NO_INTERP=1` is a second default-off seam. V5B applied
it to seven matched feather tokens (`79, 100–105`) and replaced the
render-time temporal result with the selected current guest world
transform. V5C extends the same seam to JOIN parents 107 and 108.
G_MTX identity, call hashes, candidate selection, one-to-one allocation,
previous-transform selection, and guest animation remain unchanged.

See [[../../plan/evidence/p09-kazooie-feather-v5-full-chain]] for the static
ownership map, CSV schema, and validation evidence.

## Kazooie feather V5B / V6 (`PROVEN_RUNTIME` + `PROVEN_STATIC`)

V5B full-chain `TOOIE_DIAG_FEATHER_NO_INTERP=1` covered bones
`79, 100–105` (masks `0x7F`, `RESULT==CURRENT` 211/211). User visual
`SAME`. That set omitted JOIN parents 107/108. V5C froze all nine
visible-triangle transforms (`79,100–105,107,108`). User visual
`SAME`. Therefore
`TEMPORAL_MATRIX_INTERPOLATION_CAUSAL_STATUS:
REFUTED_FOR_ALL_TRANSFORMS_DIRECTLY_USED_BY_VISIBLE_FEATHER_TRIANGLES`.
SIMPLE remains `REFUTED_AS_PRIMARY_FIX` for the 103-only test. AUTO
identity is `CORRECT`. V5C:
[[../../plan/evidence/p09-feather-no-interp-v5c]].

Original-framerate graphics is `USER_OBSERVED` `FIXED`. High-framerate
dependence is `CONFIRMED`. V5C was **not** a global current-frame test
(`V5C_WAS_EQUIVALENT_TO_GLOBAL_CURRENT_FRAME: NO`). The remaining
discriminator is extra-frame rendering with CURRENT weights versus
extra-frame resource reuse. Diagnostic:
`TOOIE_DIAG_FORCE_CURRENT_FRAME=1`
([[../../plan/evidence/p09-high-rate-force-current-frame]]).

Per-vertex RT64 state: `setVertexCommon` writes
`worldIndices=curTransformIndex`, `viewProjIndices=curViewProjIndex`,
F3DEX2 `vel=0`. `drawIndexedTri` uses those stored indices, not the live
modelview. `DrawCall.min/maxWorldMatrix` is the min/max of the triangle's
vertex indices. `popMatrix` does not flush and does not clear the vertex
cache. Displayed `RSPWorldCS` position with `vel=0` is current
`worldMats[index]` only.

Banjo explicit matrix groups (PR #22) remain `ABSENT` in Tooie and are
not a ready tagging port. PR #23 SIMPLE is **not** the next action after
V5C `SAME`. High-framerate extra-frame vs weight interpolation is the
open discriminator.
[[../../plan/evidence/p09-banjorecomp-transform-tagging-portability-audit]]. F3DEX2 `G_TRI1`/`G_TRI2` call `RSP::drawIndexedTri`. The V6
triangle CSV was empty because `FeatherWorldHit` was keyed by
matrix/match workload while `drawIndexedTri` looks up
`workload.workloadId`, and `p09DiagnosticMatrixToken` is cleared at the
first G_VTX that creates a world transform. V6B classifies from
per-vertex `worldIndices` without requiring that token
(`P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V6B`). Provenance:
[[../../plan/evidence/p09-kazooie-feather-v6b-triangle-witness-repair]].

V4 keeps the matched token on `p09DiagnosticMatrixTokenStack[modelview
depth]`. Unmatched child `G_MTX` writes 0 into the **new** slot only.
`popMatrix` restores the parent slot. Type-0x18 same-workload
same-depth `G_MTX` is a one-shot restore alias for destD / `dl=0x5AA`.
This is observability plumbing, not a production group tag.

Banjo `func_803387F8` is the same bone / `gSPMatrix` / child / pop
skeleton **plus** `gEXMatrixGroup` (`G_EX_INTERPOLATE_SIMPLE` when
boned). Explicit groups associate the transform before vertex and would
remove the diagnostic's VERTEX dependency (`GROUP_TAGGING_WOULD_REMOVE_DIAGNOSTIC_VERTEX_DEPENDENCY: YES`). That is **not** proof groups
fix the visual glitch. Future A/B seams (not enabled): instance =
`DE448` `model_bin`; offset = GeoCmd rel `0x124A0` or bone index 30;
push after DCF48 `G_MTX DA380002`; pop with DCF48 `G_POPMTX`.

## Bounded single-G_MTX identity witness (`IMPLEMENTED_STATIC`)

The default-off `TOOIE_DIAG_SECONDARY_SWAY=1` path instruments the proven
`0x62B+0x11CB8` DCF48 event. Schema
`P09_SECONDARY_SWAY_SINGLE_MTX_IDENTITY_V3` captures the float product at
post-`func_80019AA0` `MEM_W(D_8007BCB0)`, the `guMtxF2L` N64 Mtx at the
Gfx dest (`mlMtxApply` entry `a0`), and a separate RT64 match CSV at
`GameFrame::matchTransform` plus end-of-`GameFrame::match` unmatched flush.
The V2 identity CSV filled with MATRIX/NEW and VERTEX before matching ran;
that is not evidence of AUTO mismatch.

RT64 consumes that token at `RSP::matrixCommon`, records the F3DEX2 guest
display-list command address, matrix flags, model-stack depth, group-stack
identity, segmented/physical matrix addresses, and workload ID; it then
records the first transform materialized by `setVertexCommon`. The existing
`GameFrame::matchTransform` seam records a `MATCHED` event with current and
previous workload/transform indices. The identity CSV is capped at 512
events and all callbacks are inert when the diagnostic is disabled. This is
`IMPLEMENTED_STATIC`, not runtime proof: the next capture must establish
whether the token reaches RT64 and whether frame matching is stable.

The exact AUTO matching key is the compatible draw-call hash (color combiner,
other mode, geometry mode, triangle count, and the ordered hash of matrix
group IDs), followed by same-position transform candidates within matched call
ranges and one-to-one valid transform matching. Matrix-stack depth affects
the effective transform and command order, but is not itself a stable guest
address key. Guest matrix-address stability is not required.

## Banjo single-G_MTX analog (`PROVEN_STATIC`)

Banjo's `func_803387F8` is the closest structural analog: it obtains one bone
matrix, calls `mlMtx_push_multiplied_2`, emits `mlMtxApply` plus
`G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW`, traverses the child list, and
pops afterward.  It also inserts `set_model_matrix_group` around that G_MTX;
the group ID is derived from the model transform ID plus the bone/list offset.
Tooie's live `func_800DCF48` path has the same single-matrix hierarchy but no
equivalent explicit group command in the observed path.  That is a real
structural difference (`EXPLICIT_MATRIX_GROUP_GAP: YES`). Tagging is
not a ready production port. Interpolation of JOIN parents 107/108 was
not covered by V5B; see V5C.

P09 cares which **first bad layer** is live. Do not overfit to E2AA4.

Live `0x62B` Banjo snout **encoding** is compact type-2 → `func_800DCF48`
→ **single** `G_MTX` via `func_800ADE2C` (`STRONG_INFERENCE` for the
exact `+0x11CB8` node until DCF48 counts it). BanjoRecomp dual-mtx group
tagging is `NOT_APPLICABLE` on that path until a capture shows E2180 for
the snout node. Interpolation work stays blocked on live snout
transform-owner execution.

## GeoCmd dual-mtx (`PROVEN` mechanism; **not** live `0x62B` snout)

E2180 emits previous `DB060008` then current `DB060004`. RT64 can
interpolate that pair if tagged. Tooie has **no** BanjoRecomp-style
`set_model_matrix_group` on this path (`PROVEN` absence of that helper
in Tooie). Causal link to the walk-sway glitch: `HYPOTHESIS` and only
if some other live GeoCmd path (not the compact `0x62B` snout) uses
E2180. Live `0x62B` snout dual-mtx: `NO`.

## ExtraSegments (`UNKNOWN` in Tooie C)

| Item | Tooie | BK analog |
| --- | --- | --- |
| Parse function | `UNKNOWN` | — |
| Header discriminator | `gtype` bit `0x40` at `+0xA` `PROVEN` | — |
| List offset | header `+0xC` `STRONG_INFERENCE` | — |
| Bone transform | `UNKNOWN` | `func_802E6BD0(BKModelUnk28List, BKVertexList, AnimMtxList)` |
| Previous/current handling | `UNKNOWN` | identity G_MTX + group on LOADDL |
| Interpolation tagging | **ABSENT** | SKIP vertex interp when `cur_model_uses_ex_vertex` |
| Banjo snout | `0x61F` idx 4 parent 18; no GeoCmd `0x02` | — |

Tooie `0x61F` `+0x28` is **zero**. BK `model_bin->unk28` ExtraSegments
slot does **not** transfer (`CORRECTED`).

`func_800DE498` gtype tests `0x2/0x80/0x100/0x4` are **not** this path.

Scan of core ROM `lh/lhu 10(rs)` then `andi 0x40`: zero hits. The `0x40`
test may live in overlay code or a copied gtype (`UNKNOWN`).

## Root classes (P09)

| ID | Class | When favored |
| --- | --- | --- |
| A | `BAWOBBLE_STATE_BUG` | still possible |
| B | `BONE_ADDITIVE_TRANSFORM_BUG` | `800E0B8C` / `800AF04C` still possible |
| C | `EXTRASEGMENTS_INTERPOLATION_BUG` | if live `0x61F` |
| D | `GEOCMD_DUAL_MTX_TAGGING_BUG` | if live `0x62A` |
| E | `COMMON_INTERPOLATION_IDENTITY_BUG` | covers both |
| F | `OTHER` | — |

```text
MOST_LIKELY_FIRST_BAD_LAYER: INDEPENDENT_VIEW_AND_WORLD_INTERPOLATION
CURRENT_ROOT_CANDIDATE: COUPLED_RELATIVE_VIEW_WORLD_FAILURE
ROOT_CONFIDENCE: HIGH
MISSING_GROUP_CAUSAL_STATUS: PR23_SIMPLE_REFUTED_FOR_CAPTURED_EXPLOSIONS
WORLD_MATRIX_INTERPOLATION_CAUSAL_STATUS: CORRECTED/SUPERSEDED
TEMPORAL_MATRIX_INTERPOLATION_CAUSAL_STATUS: CORRECTED/SUPERSEDED
PROJECTION_ONLY_ISOLATION: INVALID_TEMPORALLY_INCOHERENT
```

BK `func_803387F8` + `set_model_matrix_group`: Tooie missing-equivalent
is `PROVEN` as architecture. PR #22 tagging is not a ready production
port. V5C visual `SAME` closed the nine-bone TransformProcessor freeze.
[[../../plan/evidence/p09-feather-no-interp-v5c]],
[[../../plan/evidence/p09-high-rate-force-current-frame]].

## Modding relevance

**PROVEN ROLE:** dual G_MTX pair on GeoCmd `0x02`.

**POTENTIAL MODDING USE:** tagging / interpolation policy once the live
body class is proven. Do not ship a tagging patch in this round.

## P09 witness address and argument correction (`PROVEN`, 2026-09-03)

The live compact type-2 snout reaches `func_80019AA0` and then
`mlMtxApply`/`guMtxF2L`. The result cursor is **`D_8007BCB0`**, not
`D_8007BCD0`. `guMtxF2L` is libultra `(float src, Mtx dest)`.

```text
func_80019AA0:
  a0 = 0x8012CE58 (parent/model float matrix)
  a1 = ADE2C result (local bone float matrix)
  a2 = MEM_W(D_8007BCB0) + 0x40
  affine 3x4 product written at a2; last column [0,0,0,1]
  D_8007BCB0 updated to a2
  return: void

mlMtxApply:
  entry a0 = lw(gfx_ctx+4)   // N64 Mtx dest AND G_MTX operand
  0x80018C2C: a1 = a0
  0x80018C3C: a0 = MEM_W(D_8007BCB0)
  guMtxF2L(a0=float source, a1=N64 Mtx dest)

func_800DCF48:
  s0 = a0 (Gfx context)
  0x800DCF8C: mlMtxApply(a0 = MEM_W(s0+4))
  0x800DCFAC: G_MTX operand = MEM_W(s0+4)
  s0+4 += 0x40
  children DE2A4; mlMtxPop; G_POPMTX 0x40
```

RT64 AUTO matching is **not** done at `RSP::matrixCommon`. Call pairing is
`GameFrame::buildCallHashMap` / `hashFromCall` (combiner, other mode,
geometry mode, triangle count, xor-hash of group `matrixId`s). Transform
pairing for `G_EX_ID_AUTO` is same-position world-matrix indices inside
hash-matched calls, then `computeTransformMatch` (position / orientation /
screen-space; mirrored-determinant reject), then one-to-one
`GameFrame::matchTransform`. Explicit IDs use `buildTransformIdMap` (AUTO
skipped). The live `tooie_diag_rt64_match` seam is `matchTransform`; V2
observed no MATCH rows because the identity CSV saturated on MATRIX/VERTEX
and the 64-slot token ring expired before frame match.

## P09 V4 real match witness (`IMPLEMENTED_STATIC`, 2026-09-03)

The V4 diagnostic extends the existing guest→G_MTX token into the actual
`GameFrame` temporal decision. It records compatible call-hash pairs,
transform prerequisite rejects, `computeTransformMatch` candidate validity and
metrics, one-to-one selection, and the final `matchTransform` callback. The
semantic context is explicit: call hash, scene-projection index, call index,
world-transform range, projection type/index, matrix IDs, and workload IDs.

`REAL_UNMATCHED` rows are finalized after `GameFrame::match` and carry the
observed rejection reason. `FLUSH_SENTINEL` rows are secondary cleanup only;
they are not causal match results. This is diagnostic instrumentation, not a
production interpolation change. The witness remains default-off and CPU-only.

```text
SCHEMA: P09_SECONDARY_SWAY_SINGLE_MTX_IDENTITY_V4
RT64_MATCH_WITNESS: REAL_MATCHTRANSFORM
RT64_CALL_MATCH_WITNESS: ENABLED
GUEST_MATRIX_WITNESS: D_8007BCB0_VALIDATED
MATCH_REJECTION_REASONS: NO_PREVIOUS_CALL_MATCH, RANGE_POSITION_MISMATCH,
  TRANSFORM_MATCHING_DISABLED, MATRIX_ID_MISMATCH,
  MATRIX_ID_NOT_AUTO_OR_ORDERED, DETERMINANT_MIRROR_REJECT,
  ONE_TO_ONE_CURRENT_ALREADY_USED, ONE_TO_ONE_PREVIOUS_ALREADY_USED,
  MATCHTRANSFORM_TOKEN_NOT_BOUND, NO_CANDIDATE_OBSERVED
FLUSH_SENTINEL_ROWS: SECONDARY_ONLY
MISSING_GROUP_CAUSAL_STATUS: HYPOTHESIS
```

Evidence: [[../../plan/evidence/p09-v4-real-match-witness]].

## P09 V5 workload identity semantics (`CORRECTED`, 2026-09-03)

`RSP::matrixCommon` reports `state->workloadId`, the identity consumed by
`GameFrame::match` and `matchTransform`. `RSP::setVertexCommon` reports
`workload.workloadId`; at the observed snout seam that value is zero. These
are different argument sources, so vertex context must not overwrite a valid
matrix workload.

V5 stores `matrix_workload_id`, `vertex_workload_id`, and
`match_workload_id` separately. Precedence is a non-zero matrix workload over
a zero vertex workload. `REAL_UNMATCHED` is emitted only from an observed
non-selected decision; finalization and flush fallbacks are
`SYNTHETIC_FINALIZATION_ONLY`. This remains diagnostic-only.

```text
SCHEMA: P09_SECONDARY_SWAY_SINGLE_MTX_IDENTITY_V5
MATRIXCOMMON_WORKLOAD_SOURCE: state->workloadId
SETVERTEXCOMMON_WORKLOAD_SOURCE: workload.workloadId
AUTHORITATIVE_MATCH_WORKLOAD_ID: non-zero matrix workload
TOKEN_WORKLOAD_PRECEDENCE_RULE: MATRIX_NONZERO > VERTEX_ZERO
REAL_UNMATCHED_REQUIRES_REAL_MATCH_DECISION: YES
NO_CANDIDATE_OBSERVED_SEMANTICS: DIAGNOSTIC_ONLY_FINALIZATION_FALLBACK
REAL_MATCHTRANSFORM_WITNESS_RETAINED: YES
```

## P09 V5 runtime result (`PROVEN`, 2026-09-03)

The first V5 capture postdates the rebuilt executable and proves that the
workload-binding correction reaches the real RT64 match path. It records 5/5
call-hash matches, 5/5 eligible transform prerequisites, 77 valid candidates,
34 selected `matchTransform` decisions, and 5 real one-to-one rejections
(`ONE_TO_ONE_PREVIOUS_ALREADY_USED`). The 2,340 finalization-only rows are
synthetic diagnostic cleanup, not real unmatched calls.

The two selected rows whose previous call/range differs from the current
diagnostic context are not a proven AUTO identity failure. RT64 pairs calls by
`hashFromCall`, builds transform observations by transform-index pairs, and
then calls `GameFrame::matchTransform`; call index/range metadata is not itself
the transform-match key. No candidate-validity, determinant, position,
orientation, matrix-ID, or token-binding failure was observed.

```text
V5_REAL_CALL_MATCH_RATE: 5/5
V5_REAL_MATCHTRANSFORM: 34
V5_REAL_UNMATCHED: 5_ONE_TO_ONE_PREVIOUS_ALREADY_USED
V5_PROVEN_AUTO_IDENTITY_FAILURE: NO
V5_PROVEN_INTERPOLATION_FAILURE: NO
V5_MISSING_GROUP_CAUSAL_STATUS: HYPOTHESIS
```

The reusable next witness is the same generic V5 seam applied to a
Kazooie head-feather `G_MTX`. Guest filter is now
`P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V1` (primary `0x62B+0x124A0` idx 30
anim 103, fallback `0x61C+0xF558` / `0x83A+0xD408`). Banjo snout is
closed as the primary token. No production interpolation change.

See [[../characters/kazooie]] and
[[../../plan/evidence/p09-kazooie-carry-banjo-head-feathers]].

## P09 V4 runtime capture correction (`CORRECTED`, 2026-09-03)

The first V4 runtime capture is valid for the guest-to-G_MTX proof but does
not yet prove an RT64 call or transform rejection. It contains 2,236
`REAL_UNMATCHED` rows, with zero `CALL`, `TRANSFORM_PREREQUISITE`,
`CANDIDATE`, `DECISION`, or `MATCHED` rows; every finalized row is the
diagnostic fallback `NO_CANDIDATE_OBSERVED`.

The captured identity CSV separates the arguments that caused this result:
`RSP::matrixCommon` reports workload IDs 773, 774, and onward through
`state->workloadId`, while `RSP::setVertexCommon` reports workload ID 0
through `workload.workloadId`. The vertex callback overwrites the token's
correct matrix workload with zero. `GameFrame::matchScene` then compares
nonzero current workload IDs against that zero and filters the token before
the real call witness can record it. This is a **V4 diagnostic argument/
binding defect**, not evidence of an RT64 AUTO-matching failure.

The guest path remains proven in this capture: 512/512 sampled rows show the
composed float source identity and all 2,236 consumed tokens have matching
source/destination G_MTX operands; two additional tokens expired before
consumption. The next bounded implementation action is to preserve the
matrix-event workload identity when the vertex callback supplies zero, then
rebuild and capture once; no production interpolation change is justified.

## P09 feather V3 diagnostic identity (`PROVEN`, 2026-09-04)

The diagnostic token must retain separate `mlMtxApply` float-source and
`guMtxF2L`/G_MTX destination fields. RT64 identity lookup uses the N64 Mtx
destination encoded by G_MTX, not the float source pointer. The existing
callback address representation is already the same 32-bit guest form, so no
normalization is required for this comparison.

The feather witness also preserves a nonzero matrix workload when a vertex
event supplies workload zero. A fallback vertex binding is valid only for a
unique pending token with the same nonzero matrix workload; broad latest-token
association is not valid. This is diagnostic plumbing, not a production
interpolation rule.

For scale-heavy animation, determinant alone is insufficient: decompose the
3x3 basis by its basis lengths, normalize the rotation, and compare unit
quaternions with hemisphere continuity (`dot(q_current, q_previous) < 0`
means negate the current quaternion). Record orthogonality error and angular
delta before attributing a visual discontinuity to interpolation.

## P09 V3 capture boundary (`PROVEN`, 2026-09-04)

The current Kazooie feather capture reaches the real MATRIX/G_MTX consumer:
2421 tokens use the F2L destination as the G_MTX identity and report zero
address-normalization failures. The primary semantic identity remains
`0x62B/+0x124A0`, idx30, anim103. The raw destination pointer rotates through
a 15-entry Mtx ring, so raw pointer stability is not a valid model-instance
identity test.

The same capture produces no VERTEX event and therefore no GameFrame call,
candidate, decision, or MatchTransform row. This is a proven diagnostic
observability boundary, not evidence of an RT64 AUTO rejection. Any future
semantic matching claim must first prove a MATRIX→VERTEX handoff in the same
nonzero workload context; zero `RT64_MATCH_NO_TOKEN` aggregate data cannot be
used as a feather-specific failure count.
## V5 runtime coverage correction (2026-09-04)

The V5 no-interpolation capture does not yet establish full visible feather
coverage. The runtime token/match path observed only `0x62B` boneAnimID 103;
the six `Sibling62B` identities were rejected by the diagnostic DCF48
acceptance gate before token creation. The single 103 post-match row did use
the current matrix exactly, but the visual result `SAME` cannot refute
temporal matrix interpolation for the complete feather chain. See
`docs/plan/evidence/p09-kazooie-feather-v5-full-chain.md`.
