---
title: "P09 post-triangle architectural review and bounded V7 specification"
description: "Static RT64 audit: incomplete join evidence, direct world-index consumption, and the earliest CPU upload witness."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-05
tags: [p09, kazooie, rt64, geometry, architectural-review]
---

# P09 post-triangle architectural review

Bounded objective: `P09-RES-03 / IDENTIFY_THE_FIRST_PLAUSIBLE_POST_DRAWINDEXEDTRI_CAUSAL_LAYER`.
Static inspection and existing-file analysis only. V7 is **NOT_IMPLEMENTED**.
No game, build, fix test, GPU readback, synchronization change, commit, or push.
P09 remains formally pending; this does not close P08 or open P09.

## Decision and evidence boundary

**CORRECTED:** `FIRST_BAD_LAYER=AFTER_RSP_TRIANGLE_SUBMISSION` is not established.
The accepted V6C own-mesh provenance results survive this review. They do not
prove all CPU geometry, final face-index accumulation, matrix upload selection,
or the displayed position. The first *unverified downstream representation* is
the `faceIndices` slice assigned to a `GameCall` by `State::fullSync`, followed
by the actual upload sources. No specific renderer defect is proven causal.

```text
JOIN_UNRESOLVED_IS_BLOCKER: YES
WHY: blocks an exclusive downstream root claim, not this static architectural audit
JOIN_CAN_EXPLAIN_PRIMARY_VISUAL_ARTIFACT: UNKNOWN
FIRST_REMAINING_CPU_LAYER: faceIndices accumulation -> fullSync GameCall mesh slice -> upload source selection
ROOT_CLASS_AFTER_REVIEW: UNKNOWN / INCOMPLETE_JOIN_AND_CPU_HANDOFF_EVIDENCE
ROOT_CONFIDENCE: LOW for causal localization; HIGH for the inspected static dataflow
```

The root/basal join can stretch its own connecting surface if one endpoint
uses the wrong source or matrix. It cannot modify the independently submitted
tip mesh's coordinates. If the observation specifically established movement
of the own `0x534` vertices, `0x592` alone could not explain that movement.
The supplied visual description does not establish that triangle-level
ownership. Also, the distal joins `0x54C` and `0x564` are real geometry in the
proven right-plume subtree and are not classified as JOIN by V6C. Three correct
own meshes therefore suffice to justify a *parallel line of static reasoning*
downstream, but not to exclude the connecting surfaces as the visual witness.
No exact display-frame parity is inferred.

### Existing V6C: what is actually available

The checkout has V6 and V6B evidence notes, but no V6C evidence note before
this review. The user's supplied V6C aggregate results are accepted as runtime
evidence; local `/tmp/tooie-secondary-sway-status.txt` confirms schema V6C,
2,796,534 raw triangles, 14,880 own matches, 468 expected joins, incomplete
coverage, and the segment-3 target/result. The supplied per-DL totals are:

| DL | Accepted aggregate result | Scope |
| --- | --- | --- |
| `0x5AA` | 3,720 own matches, zero stale | 103 own mesh |
| `0x57C` | 3,720 own matches, zero stale | 104 own mesh |
| `0x534` | 7,440 own matches, zero stale | 105 own mesh |
| `0x592` | 3,720 total; 468 expected; 3,252 unresolved; zero stale/unexpected | root join only |

`0x03000040 -> 0x0011C870`, zero nested triangles and zero leftover triangles,
remains **REFUTED** as the leftover-triangle root for this path/capture.
Neither broad transform ranges nor V5B matrix interpolation are reopened.

Offline inspection **PROVEN** on the available CSV:

- 240 sampled rows: 192 OTHER/UNRESOLVED, 24 JOIN/EXPECTED,
  16 OWN_105/MATCH, four OWN_104/MATCH, four OWN_103/MATCH.
- Zero sampled JOIN/UNRESOLVED rows. All 24 sampled joins have each corner's
  `WORLD == LOAD_ACTIVE`, and `LOAD_DL == CURRENT_DL`.
- All sampled `WORKLOAD` values are zero. The CSV does not serialize global
  vertex indices: `IDX_A/B/C` are **source-local vertex indices**.
- No final CPU upload payload or mapped upload bytes are present in this CSV.

The exact missing evidence is therefore known before prioritizing renderer
experiments: unresolved target joins need their actual source tuple, load
generation, global vertex, matrix identity and matrix bytes, including both
sides of `G_POPMTX`; distal joins need explicit target coverage. Sampling must
be reserved per target DL/class. An aggregate unresolved count cannot recover
those lost rows.

The V6C implementation explains the limit (`src/game/tooie_diag.cpp:4524–4739`):
only `0x592` becomes JOIN; `history_hit` requires a mapped bone for all three
corners; a shared per-class sample quota records early OTHER/UNRESOLVED rows.
JOIN_EXPECTED_MIX checks slot groups, not independently expected parent/child
matrix values. Sampled parent and child corners even carry the same semantic
bone label 103 while using distinct world indices. This is a coverage/strength
qualification, **not contradictory evidence against the accepted matches**.

Another important boundary correction: the current V6C hook in
`rt64_rsp.cpp:1205–1244` runs **before** the cull-both return, draw-state flush,
front-face index swap, and `faceIndices.push_back`. It observes inputs to
submission. A new witness must anchor after the actual append.

## Revisions and source anchors

Paths below are relative to the indicated checkout, and line numbers refer to
the dirty source inspected on 2026-09-05, not an upstream-only revision.

| Checkout | HEAD | State |
| --- | --- | --- |
| TooieRecomp | `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf` | protected pre-existing diagnostics/docs/config changes |
| `../BanjoRecomp` | `ec859632cfa584e2272d32e46051543429a2baf4` | reference only |
| `../BanjoRecomp/lib/rt64` | `6f1c2d99a4ea571c139f449c326fd176ba8f3496` | protected diagnostics; no edits by this review |
| RT64 `src/contrib/plume` | `d890ac899e505fb30040e037a4037cdeca68f033` | pre-existing modified `plume_vulkan.cpp`; preserved |

The RSP jCodeMunch index initially lagged the V6C edits; it was refreshed before
using the current hook placement. Shader files are outside the indexed C/C++
set and were inspected directly through context-mode.

| ID | RT64 source anchors | Contract inspected |
| --- | --- | --- |
| S1 | `src/hle/rt64_rsp.cpp:589–655,770–842,868–968,1180–1374` | world-matrix append, vertex SoA append, copy-on-modify, actual triangle append |
| S2 | `src/hle/rt64_state.cpp:466–538,802,952–986,1136–1163,1580–1595,1787–1789,2667–2675` | flush, face slice, initial uploads, producer completion, workload ID assignment |
| S3 | `src/hle/rt64_workload.h:25–98,143–163`; `rt64_workload.cpp:16–97,149–277` | arrays, ranges, persistent buffer pairs, reset |
| S4 | `src/hle/rt64_game_frame.cpp:110–198,308–349,619–674,940–1016`; `rt64_transform_group.h:12–23` | frame map, matching ranges, velocity gate |
| S5 | `src/render/rt64_transform_processor.cpp:31–121` | selected current/previous matrix arrays and upload predicates |
| S6 | `src/render/rt64_projection_processor.cpp:31–140,179–190` | projection/view interpolation and uploads |
| S7 | `src/render/rt64_buffer_uploader.cpp:19–21,66–114,137–149,169–173` | source pointer lifetime, stride, offset, copy and wait |
| S8 | `src/render/rt64_rsp_processor.cpp:18–95`; `rt64_vertex_processor.cpp:18–66` | compute bindings and dispatch range |
| S9 | `src/shaders/RSPProcessCS.hlsl:22–66,127–133`; `RSPWorldCS.hlsl:15–44`; `RSPModifyCS.hlsl:15–35`; `RasterVS.hlsl:14–36` | real shader consumption and position equations |
| S10 | `src/render/rt64_framebuffer_renderer.cpp:1283–1324,2032–2066,2218–2251,2487–2493` | draw views, setup order, indexed draw arguments |
| S11 | `src/hle/rt64_workload_queue.h:26–96`; `rt64_workload_queue.cpp:38–84,302–402,839–870,882–929,977–1007,1166–1170` | ring ownership, temporal rendering, existing waits |

## Actual post-drawIndexedTri chain

```text
POST_DRAWINDEXEDTRI_CALL_CHAIN:
  RSP::drawIndexedTri
    -> optional projection/state flush and loadDrawState
    -> front-cull winding swap; RSP indices[slot] -> globalIndices[3]
    -> workload.drawData.faceIndices.push_back(globalIndices[i]) x3
    -> drawCall min/max(worldIndices[globalIndex]); triangleCount++
  State::flush
    -> GameCall.callDesc = drawCall
    -> FramebufferPair::addGameCall -> current Projection.gameCalls
  State::fullSync
    -> GameCall.meshDesc.faceIndicesStart = running faceIndex
    -> faceIndex += triangleCount * 3 (viewport projections)
    -> Workload::updateDrawDataRanges / uploadDrawData
    -> initial viewProjTransforms + worldTransforms uploads
    -> BufferUploader::submit / updateResources / threadUpload
    -> initial command-list copies and existing completion waits
    -> State::advanceWorkload / WorkloadQueue::advanceToNextWorkload
  WorkloadQueue::renderThreadLoop
    -> GameFrame::set(one workload) / match(current, previous)
    -> threadRenderFrame
    -> ProjectionProcessor::process/upload when required
    -> TransformProcessor::process/upload when prevFrame.matched
    -> resetRSPOutputBuffers / RSPProcessor::process
    -> optional VertexProcessor::process (world/motion path)
    -> FramebufferRenderer::addFramebuffer
    -> recordSetup: uploader copies -> RSPProcessCS -> RSPModifyCS
       -> optional RSPWorldCS
    -> submitRasterScene -> drawIndexedInstanced(count,1,indexStart,0,0)
    -> RasterVS consumes computed screenPosBuffer

FIRST_STATE_OBJECT_AFTER_TRIANGLE: existing Workload.drawData + State.drawCall
FIRST_PERSISTENT_TRIANGLE_REPRESENTATION: three uint32_t global indices appended to DrawData.faceIndices
```

This is a call/dataflow chain; producer upload and render-thread replay are
separate phases. DrawCall is not a per-bone vertex structure. The workload's
parallel arrays exist before the triangle and outlive an individual flush.

## Stages that can affect the result

`B` changes position, `C` selects a vertex/matrix/range, `D` uses temporal state,
`A` is metadata/batching, and `E` cannot directly move vertices on this path.
Possible failure categories are not findings of failure.

| STAGE / class | FUNCTION | INPUT -> OUTPUT | CAN_CHANGE_POSITION | USES_WORLD_INDEX | USES_PREVIOUS_FRAME | CAUSAL_PLAUSIBILITY |
| --- | --- | --- | --- | --- | --- | --- |
| Index accumulation / C | `RSP::drawIndexedTri` | slots -> faceIndices | selects corners | range reads only | NO | earliest unverified append, V6C is before it |
| Batched call / A | `State::flush` | accumulating DrawCall -> GameCall | NO | preserves min/max metadata | NO | no identity rewriting found |
| Mesh slice / A,C | `State::fullSync` | ordered counts -> faceIndicesStart | selects triangles | NO | NO | earliest derived downstream range; unmeasured, no demonstrated error |
| Upload packing / C | `Workload::uploadDrawData`, `BufferUploader` | SoA arrays/ranges -> upload/default buffers | wrong bytes could | copies uint16 stream | reusable allocation only | inspect selected bytes; no packing defect found |
| AUTO matching / D | `GameFrame::match` | current/previous call ranges -> maps | indirectly through processors | transform ranges | YES | identity failure remains refuted for seven feathers |
| Matrix generation / B,D | `TransformProcessor::process` | current/previous matrices -> current-indexed arrays | YES | one output per current transform | YES | V5B refutes matrix lerp as primary cause |
| Matrix source selection / C | `TransformProcessor::upload` | selected vector pointer/count -> matrix buffer | wrong selection could | same numeric slot | YES | still downstream of V5B witness; no live mismatch proven |
| Projection / B,D | `ProjectionProcessor` | view/proj/aspect/maps -> modViewProjTransforms | YES | uses viewProjIndex instead | YES | survives NO_INTERP; shared projection weak fit for local-only distortion |
| Vertex velocity / B,D | `GameFrame::matchTransform` | paired local vertices -> velFloats | YES if enabled | matrix vertex ranges | YES | default vertexInterpolation=SKIP; no active feather path established |
| RSP compute / B,C | `RSPProcessCS` | local pos, velocity, indices, matrices -> screenPos | YES | direct read | frame weight; selected matrices | first arithmetic consumer for raster position |
| Screen modification / B | `RSPModifyCS` | modifyPosUints -> overwrite screenPos XY/Z | YES | NO; global vertex index | NO | concrete alternate position writer; feather overlap UNKNOWN |
| World/motion compute / B,D,E | `RSPWorldCS` | same local arrays + current/prev matrices -> worldPos/norm/vel | YES in world outputs | direct read | YES | raster POSITION uses RSPProcess output, not this worldPos |
| Extended depth test / C,E | `RSPVertexTestZCS` / extended call | face indices/depth -> testZ index buffer | visibility/index selection | NO | no previous mesh selection | extended-path applicability unobserved; not evidence of a feather remap |
| Raster setup / B,C | `addFramebuffer`, `submitRasterScene`, `RasterVS` | screenPos + face slice + scale/offset -> clip position | YES | NO | current calls | wrong draw/view could matter; baseVertex is zero |
| Clip/cull/depth/material / E | raster backend | triangles + render state -> coverage/color | NO vertex deformation | NO | target reuse possible | can alter apparent silhouette; CPU position equality cannot exclude this |
| Resource/descriptor reuse / C,D | queue/uploader/backend | retained buffers and bindings -> execution | wrong resource could | binding selection | rotating workloads/targets | no observed race; existing waits guard ordinary reuse |

## worldIndex and matrix storage

```text
WORLDINDEX_FIRST_CONSUMER: after submit, uploadDrawData copies it; RSPProcessor binds it; RSPProcessCS reads it for raster position
WORLDINDEX_TO_MATRIX_LOOKUP: transformIndex=srcWorldIndices[vertexStart+vertexIndex]; worldMat=worldTransforms[transformIndex]
WORLDINDEX_REMAP_AFTER_TRIANGLE: NO (normal indexed F3DEX2 raster path)
WORLDINDEX_REMAP_FUNCTION: NONE
WORLDINDEX_REMAP_CAN_COLLIDE_OR_ALIAS: NO remap exists on this path
WORLD_INDEX_STABLE_UNTIL_RENDER: YES within one workload generation, by inspected design
WORLD_MATRIX_STORAGE_LIFETIME: append during G_VTX; retain in Workload; initial original-array upload; per-render selected-array upload; reset only on workload reuse
MATRIX_INDEX_REBASE_EXISTS: NO for current world indices
CURRENT_PREVIOUS_INDEX_ALIGNMENT_REQUIRED: YES at shader-facing buffers; NO for raw previous workload indices
```

S1 assigns `curTransformIndex=uint16_t(worldTransforms.size())` and appends the
matrix when the model-view changes; the inserted-MVP branch similarly appends
its recovered world transform. Each G_VTX appends coordinates and the current
world index together. Vectors can reallocate, but a numeric index survives
reallocation. There is no persistent raw matrix pointer in each DrawCall.
The 16-bit capacity is an upstream representational limit, not a discovered
post-triangle rebase or new explanation for the accepted own indices.

`worldTransforms` persists until workload reset. Upload descriptions retain
`.data()` pointers only for the asynchronous copy interval. Initial uploads
use the original matrix array (S2). Render replay replaces the same
`worldTransformsBuffer` with `lerpWorldTransforms` (S5) when matching is active.
For every current index `t`, `TransformProcessor` looks up a potentially
different previous index through `workloadMap.transforms[t].prevTransformIndex`.
It appends current and previous results **in current t order**. Thus the
shader's `prevWorldMats[t]` does not mean raw previous workload slot `t`.

### A real static inconsistency, but not a supported root here

S5 `process` requires both a previous frame and `workloadMap.mapped` before
filling `lerpWorldTransforms`/`prevWorldTransforms`. Its `upload` selects these
arrays solely when the previous frame pointer is non-null. With an unmapped
workload, the arrays were cleared but the upload count is the original matrix
count; retained vector capacity could supply old bytes, or null data could
skip the upload. **PROVEN static predicate mismatch; UNKNOWN general impact.**

However S11 currently calls `curFrame.set(...,1)`. S4 maps workloads by ordinal
when the previous frame contains that ordinal. S11 processes transforms only
if the previous frame is matched, and matching is called before render replay.
The ordinary steady-state single-workload path therefore supplies a mapped
current workload. A configuration transition is a narrower static possibility:
if `requiresFrameMatching` becomes false after a matched previous frame,
`GameFrame::set` clears the current map, `match` is skipped, yet
`processTransforms=prevFrame.matched` can still be true for that frame. This
would explain a transition, not the reported frequent locomotion artifact
without additional evidence. No target occurrence of the bad predicate
combination was found. **INFERENCE: weak candidate for this artifact.**
Record the predicates in V7; do not patch them or rank this as a causal find.

## Draw calls, copies, and formats

```text
DRAWCALL_PRESERVES_PER_VERTEX_WORLDINDEX: YES
DRAWCALL_REWRITES_VERTEX_TRANSFORM_IDENTITY: NO
DRAWCALL_MATRIX_RANGE_ONLY_METADATA: YES for raster matrix selection; ranges also drive temporal matching, so not inert globally
POST_RSP_VERTEX_REMAP_EXISTS: NO for already-submitted ordinary indexed triangles
VERTEX_REMAP_FUNCTION: NONE downstream; RSP::modifyVertex copies a used slot for future triangles
WORLDINDEX_COPIED_WITH_VERTEX: YES in that copy-on-modify path; downstream SoA streams are uploaded separately at the same global index
CAN_VERTEX_AND_WORLDINDEX_BECOME_DESYNCHRONIZED: UNKNOWN as a bug; no such transformation or demonstrated mismatch found
FINAL_CPU_VERTEX_STRUCTURE: DrawData SoA: posFloats[3*v], velFloats[3*v], attributes and per-vertex indices
FINAL_CPU_INDEX_STRUCTURE: vector<uint32_t> faceIndices, three workload-global vertex indices per triangle
FINAL_CPU_WORLDINDEX_STORAGE: vector<uint16_t> DrawData.worldIndices
FINAL_CPU_WORLDMATRIX_STORAGE: vector<interop::float4x4> worldTransforms or selected lerpWorldTransforms; separate current-indexed prevWorldTransforms
SHADER_WORLDINDEX_INPUT: RSPProcessCS Buffer<uint> srcWorldIndices at t8; RSPWorldCS Buffer<uint> srcIndices at t4
SHADER_MATRIX_LOOKUP_EXPRESSION: worldTransforms[srcWorldIndices[vertexOffsetIndex]]
WORLDINDEX_CPU_FORMAT: uint16_t, 2 bytes per vertex
WORLDINDEX_GPU_FORMAT: formatted R16_UINT buffer view, 2-byte upload stride
WORLDINDEX_SHADER_FORMAT: Buffer<uint> typed unsigned load, zero-extended from R16_UINT
FORMAT_OR_STRIDE_MISMATCH_POSSIBLE: NO mismatch in the inspected declarations/bindings; backend implementation failure remains untested
```

S7 computes `srcOffset=first*stride`, copies to the **same destination offset**,
and records a copy to that default-buffer offset. Reallocation expands capacity
and sets `first=0` to reupload the prefix. It does not compress or rebase indices.
This is incremental upload, not vertex deduplication. Actual upload ranges after
`updateResources` matter more than the original requested ranges.

`RSP::modifyVertex` appends position, velocity, attributes and world/view indices
before changing `indices[slot]` if the slot was already used. Old face indices
remain unchanged. Screen modify commands are a separate replay stream and must
be checked for target-vertex overlap; V6C does not measure them.

Raster input is **GPU-computed** float4 screen position (stride 16), float2 UV
(stride 8), and float4 color (stride 16), not a CPU-packed interleaved vertex
with a world-index attribute. The index view is R32_UINT. Draw uses
`drawIndexedInstanced(triangleCount*3,1,faceIndicesStart,0,0)` (S10):
`faceIndicesStart` selects a slice; it does not subtract a base vertex.

## Position and surviving temporal paths

```text
VERTEX_POSITION_FORMULA:
  v = vertexStart + dispatchVertex
  P = posFloats[3*v : 3*v+3]
  V = velFloats[3*v : 3*v+3]
  W = selectedWorldMatrices[worldIndices[v]]
  Q = selectedViewProjMatrices[viewProjIndices[v]]
  clip = mul(mul(Q,W), float4(P - V*(1-currentWeight),1))
  screen.xyz = clip.xyz / (clip.w,-clip.w,clip.w) * viewport.scale + viewport.translate
  screen.w = clip.w
  optional RSPModifyCS overwrites screen XY or Z
  RasterVS converts screen -> framebuffer NDC, restores homogeneous w,
  applies screenScale/screenOffset, and optionally primitive depth
POST_TRIANGLE_TEMPORAL_STATE_EXISTS: YES
POST_TRIANGLE_TEMPORAL_STAGES: GameFrame maps; matrix/projection processing; optional local velocity; lookAt/tile interpolation; workload/target replay
POST_TRIANGLE_TEMPORAL_PATH_SURVIVES_NO_INTERP: YES
```

V5B replaces both current/previous world results for the seven transforms. It
does not disable `ProjectionProcessor`, framebuffer replay, resource uploads,
or the shader's local-velocity term. Default `TransformGroup.vertexInterpolation`
is SKIP (S4); ordinary G_VTX initializes velocity to zero (S1). The existing
evidence that no independent feather vertex interpolation is active is retained.
The formula's velocity term is not evidence to reopen it. V7 checks zero velocity
as a handoff invariant and reports a nonzero value as contradictory new evidence.
Texture/lookAt interpolation changes texture/shading, not local position.

RSPWorld's previous position contributes motion vectors/world rendering; the
ordinary raster draw consumes RSPProcess's screen position. A good world-position
computation alone would not prove the displayed raster position.

## Frame rotation, threads, and Banjo comparison

```text
POST_TRIANGLE_FRAME_ROTATING_STATE: four Workload slots; two GameFrame slots; per-display-frame weights and target replay; reused BufferPairs and descriptors
FRAME_PARITY_DEPENDENT_DATA: current/previous GameFrame slot selection; no parity-based world-index remap found
ANY_CPU_VISIBLE_DOUBLE_BUFFER_INDEX_RISK: UNKNOWN as an actual defect; no unguarded target reuse demonstrated
WORKLOAD_PRODUCER_CONSUMER_MODEL: RSP/State produces and uploads a Workload; render thread consumes current plus retained previous workload; uploader threads copy retained CPU pointers
VERTEX_BUFFER_LIFETIME_SAFE: YES by ordinary inspected ownership/wait protocol; not runtime-proven for every backend/error path
WORLD_MATRIX_BUFFER_LIFETIME_SAFE: YES by that protocol on the mapped path; conditional upload-selection defect noted separately
FRAME_RING_REUSE_CAN_CAUSE_WRONG_GEOMETRY: UNKNOWN for this case; would require a violated lifetime/binding invariant
BANJO_POST_TRIANGLE_WORKAROUND_RELEVANT: NO identified
TOP_MECHANISM_GENERALIZES_TO_OTHER_PARTS: UNKNOWN
```

Workload reset clears arrays and sets `workloadId=0`; State assigns the public
ID at `advanceWorkload`, after triangle production. A live draw-time workload
ID is therefore not a sufficient identity for a new diagnostic. Slot reuse
also makes a bare ring index insufficient. This is a **proven diagnostic-key
hazard**, not proof that renderer data aliases.

The producer waits against `barrierCursor` before reuse. The renderer retains
the prior frame, waits for uploaders before execute, waits on its worker, and
advances the barrier after the nonpaused workload finishes (S11). S2 also waits
for initial upload completion before queue publication. The uploader's pending
descriptions are protected by its work mutex; source storage must remain stable
until its existing wait. These are substantive protections against the proposed
race. This review adds none and does not infer a synchronization bug from visual
alternation. The dirty Plume backend was preserved; a full Vulkan lifetime audit
is outside this bounded CPU/dataflow review, so backend safety is not certified.

Local BanjoRecomp `patches/transform_tagging.c:213–224,449–476,521–572,860–863,938–939`
has matrix tags and high-precision CPU skinning position/velocity segments with
two source-position slots. Those change inputs **before** RSP vertex processing.
Map vertex interpolation is selectively enabled, character vertex interpolation
otherwise skipped there. No Banjo patch of RT64 world-index compaction, face-index
rebasing, or upload packing was identified. Both checkouts use this local RT64.
The static feather tables are not that dynamic skinning path; none of these
Banjo features is a demonstrated workaround for this residual.

A wrong per-corner matrix or upload payload could affect articulated children
and mixed-transform joins in other models. That structural possibility does not
establish a common cause for backpack, snout, Mumbo and Kazooie. No top causal
mechanism is selected yet.

## Ranked hypotheses

HYPOTHESIS_RANKING: the following ordered table; no causal mechanism is proven.

Support rates evidence of a mechanism relevant to this path, **not probability
or a causal verdict**. NONE means no mechanism found in the inspected path.

| Rank / mechanism | STATIC_SUPPORT | EXPLAINS_ARTICULATED_LOCAL_GLITCH | EXPLAINS_GOOD_BAD_FRAME_PATTERN | CONTRADICTED_BY_P09_EVIDENCE | Verdict |
| --- | --- | --- | --- | --- | --- |
| 1. Unresolved/untargeted join inputs | MEDIUM | YES, connecting surfaces; exact witness UNKNOWN | UNKNOWN | NO | first evidence gap, not a proven fault |
| 2. Face-slice or CPU upload payload/selection mismatch | LOW | YES if target-specific | UNKNOWN | NO | first downstream discriminator; no mismatch seen |
| 3. Target screen-modify replay | LOW | YES if target indices overlap | UNKNOWN | NO | actual independent writer; occurrence unmeasured |
| 4. Projection/viewport selection or processing | LOW | UNKNOWN; normally shared body effect | YES, temporal stage exists | NO | survives NO_INTERP, weak locality fit |
| 5. Resource/descriptor/ring lifetime failure | LOW | UNKNOWN | YES if a reuse invariant fails | NO | existing waits oppose a casual race inference |
| 6. TransformProcessor unmapped-workload upload predicate | LOW here | UNKNOWN, likely wider workload effect | UNKNOWN | NO | static mismatch, ordinary caller path makes trigger unlikely |
| 7. GPU world-index format/stride mismatch | NONE | YES if it existed | UNKNOWN | NO | inspected formats agree |
| 8. Post-RSP vertex/world-index compaction or matrix rebasing | NONE | YES if it existed | UNKNOWN | NO | no such normal-path stage found |
| Closed: semantic AUTO/matrix lerp, leftover `03000040` | NONE as remaining candidates | UNKNOWN | UNKNOWN | YES | REFUTED within stated capture scopes |

## Bounded V7 implementation specification

The full, reviewable implementation contract is
[[p09-feather-v7-cpu-handoff-spec]]. It defines the exact fields, three small
records, target/coverage limits, identity scheme, acceptance criteria and stop
rules. It is **NOT_IMPLEMENTED**.

```text
BEST_NEXT_WITNESS_SEAM: State::fullSync after meshDesc.faceIndicesStart and updateDrawDataRanges, before uploadDrawData
WHY_EARLIEST: first derived draw slice plus complete initial upload sources; compare with a post-append baseline
V7_IMPLEMENTATION_SPEC: bounded right-plume CPU handoff witness; see linked contract
EXPECTED_INFORMATION_GAIN: HIGH
NEXT_IMPLEMENTATION_MODEL: GPT-5.6 Luna / Grok 4.6 Medium
SHOULD_USE_ASTRA_FOR_NEXT_IMPLEMENTATION: NO
```

The contract specifies APPEND, INITIAL_HANDOFF and RENDER_SELECTION records;
CPU source equality alone cannot force a shader/backend root.

## Cheaper checks, validation, and handoff

```text
STATIC_OR_EXISTING_CAPTURE_CHECK_AVAILABLE: YES, partial only
EXISTING_CAPTURE_FINAL_UPLOAD_EQUALITY_AVAILABLE: NO
GPU_READBACK_USED: NO
COMMIT: NONE
PUSH: NONE
NEXT_SINGLE_ACTION: implement the bounded V7 right-plume CPU handoff witness above, retaining unresolved join coverage as an explicit gate
```

The existing CSV already supports the 24-join load-DL/world-equals-load-active
checks performed here. Static inspection establishes the format, direct lookup,
zero baseVertex and lack of normal-path compaction without another game run.
It cannot compare final uploads: there are no global vertex indices or final
payloads, and the unresolved target rows were not retained. The postmatch CSV
has matrices and weights but not final upload selection or a unique
same-generation vertex join. Do not combine independent captures by frame number.

The reproducible offline command and inspected artifact hashes are retained in
[[p09-feather-v7-cpu-handoff-spec#Review-time offline verification]].

The active phase's references and stable-contract applicability were reviewed;
no stable spec change is warranted by an unresolved causal hypothesis.
Canonical static dataflow belongs
in [[../../reverse-engineering/systems/transform-interpolation]]; the segment-3
correction belongs in [[../../reverse-engineering/systems/geocmd]].

Prior evidence: [[p09-kazooie-feather-v5-full-chain]],
[[p09-kazooie-feather-v6-post-matrix-vertex-path]],
[[p09-kazooie-feather-v6b-triangle-witness-repair]], [[p09-session-handoff]].

### Completed validation and worktree state

- Offline CSV analysis above: completed successfully through context-mode Python;
  observed counts/invariants match the report. No new runtime observation.
- `rtk proxy git diff --check` in TooieRecomp: **PASS**, exit 0.
- Historical RT64 whitespace check: **PASS**, exit 0; current sources are vendored under `lib/rt64/`.
- `markdown_vault_audit` on each of the eight documentation paths changed here:
  zero errors and zero lint issues. Both new evidence files and phase/RE pages
  have zero warnings. The two pre-existing large handoff files retain their
  size warnings; no unrelated cleanup was performed.
- Game/build/fix tests: **SKIPPED**, expressly outside this static review.
- Worktrees remain **DIRTY**. No staging, commit or push; HEADs above unchanged.
  This review changed only eight documentation files: these two new evidence
  files, global/session handoffs, active phase, and transform/geocmd/Kazooie RE.

Non-document before/after file digests matched for both checkouts; exact digests
and scope are recorded in the linked offline verification section. Plume was not
edited. Index refresh changed tool cache only; no code required a graph rebuild.
