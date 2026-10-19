---
title: "P09 call-local WORLD interpolation architecture review"
description: "Static RT64 dataflow, seam-gated matrix aliases and vertex ownership; no production change."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-24
tags: [tooie-recomp, p09, rt64, interpolation, architecture]
---

# P09 call-local WORLD interpolation architecture review

## Scope and evidence

Static/CPU review only. No game, build, GPU operation, production edit, commit or push. Tooie HEAD `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf`; inspected RT64 HEAD `6f1c2d99a4ea571c139f449c326fd176ba8f3496`. Dirty worktrees were preserved. User-provided visual outcomes are observations, not a new local reproduction. The repaired V2 elementwise cause is accepted from [[p09-elementwise-world-interpolation-causal-discriminator]] and [[p09-cpu-diagnostic-shader-semantics-repair]]. Selector leakage and workload counts are in [[p09-selective-world-selector-granularity-audit]]; second visual outcome was supplied with this review request.

## Exact dataflow

1. `RSP::setVertexCommon` appends a WORLD matrix and one group/metadata entry, then appends vertex attributes including a single `uint16_t worldIndices[vertex]` (sibling RT64 `src/hle/rt64_rsp.cpp:603-643, 770-805`). `TransformGroup` identifies a matrix slot for matching; it is not a per-use policy.
2. `RSP::drawIndexedTri` resolves RSP cache indices to global vertex indices and appends three `faceIndices`. The same global vertex can appear in later faces/calls; the `used[]` copy-on-write in `modifyVertex` protects guest edits, not differing render interpolation policies (`rt64_rsp.cpp:1293-1333, 885-913`).
3. Each `GameCall` owns `meshDesc.faceIndicesStart` and `callDesc.triangleCount`; these delimit its face slice. `GameCall` does not own a vertex slice (`src/hle/rt64_game_call.h:16-38`). Thus `GameCall -> faceIndices -> vertex -> worldIndices` is the exact current CPU join (`src/render/rt64_transform_processor.cpp:343-397`).
4. Current/previous frame matching maps original WORLD slots using GameCall ranges and `worldTransformGroups` (`src/hle/rt64_game_frame.cpp:663-705`). `TransformProcessor::process` walks calls to make a `multiWorld[worldIndex]` mask, then loops WORLD slots once and fills one `lerpWorldTransforms[t]` (and previous/inverse-transpose partners) per slot (`src/render/rt64_transform_processor.cpp:551-650`). **First policy collapse** is the write to `multiWorld[worldIndex]`; final collapse is the single output matrix at `t`.
5. `State::fullSync` updates draw ranges and uploads vertex/index arrays and original matrices before frame interpolation (`src/hle/rt64_state.cpp:1136-1173`; `src/hle/rt64_workload.cpp:931-958`). `WorkloadQueue` later invokes `TransformProcessor::process/upload` before `RSPProcessor::process` (`src/hle/rt64_workload_queue.cpp:474-494, 549-575`). Transform upload uses `worldTransforms.size()` as its range (`src/render/rt64_transform_processor.cpp:653-690`).
6. `RSPProcessor` binds uploaded `worldIndicesBuffer` and `worldTransformsBuffer` and processes each vertex, without a GameCall key (`src/render/rt64_rsp_processor.cpp:6638-6679`). The shader consumes the per-vertex WORLD index under corrected row-vector `mul(pos4, mul(WORLD, VIEWPROJ))` semantics. GameCall identity survives elsewhere for draw submission, but not at the WORLD lookup.

`LAST_LAYER_WITH_GAMECALL_IDENTITY`: CPU `GameCall` face-slice walk, available at `State::fullSync` before draw-data upload and still inspectable by `TransformProcessor`; the shader's vertex lookup has none.

`FIRST_LAYER_WHERE_WORLD_POLICY_BECOMES_SLOT_GLOBAL`: `p09MarkMultiWorldGameCallMatrices`/`p09MarkMultiOnlyWorldMatrices` marking a WORLD-index mask; `TransformProcessor::process` materializes that choice in one `lerpWorldTransforms[t]`.

## Feasibility and scheduling gate

`CALL_LOCAL_DUPLICATED_WORLD_MATRIX_FEASIBLE=CONDITIONAL`. RT64's CPU vectors, per-call face slices and indexed WORLD buffer support aliases. A clone must retain an alias to its original slot, use that original's frame-map entry/endpoints, and bypass independent matching. The current `GameCall` min/max WORLD range must remain the matching source; adding clones to that range would change hashing/matching. Audit every other range consumer before implementation.

`WORLD_BUFFER_CAN_GROW_WITH_DUPLICATED_ENTRIES=YES_CONDITIONAL`: the upload buffer range and current/previous/inverse-transpose/endpoint vectors must include the aliases consistently. `worldIndices` is `uint16_t`, so aliases must remain below 65536 and allocation failures must fall back to the original rigid policy. Do not append only `lerpWorldTransforms` while a diagnostic/consumer assumes equal sizes. The current render-time upload range is derived from `worldTransforms.size()`.

`GAMECALL_WORLDINDEX_REMAP_FEASIBLE=YES_CONDITIONAL`: rewrite face references to cloned vertices where a shared global vertex would otherwise change another call. Mutating a shared `worldIndices[v]` in place is unsafe. `State::fullSync` already uploads draw-data before `TransformProcessor` knows previous endpoints, so a render-time remap alone is too late. The smallest timing-compatible path is a CPU preparation pass before `updateDrawDataRanges`/`uploadDrawData`: reserve/remap only conservative *candidate* call-use aliases, upload them through the existing path, then use previous/current endpoints in `TransformProcessor` to certify the seam. Failed certifications fill the aliases with the same rigid result as the original. This keeps face remapping stable and requires no added GPU transfer/synchronization path. `GameFrame` matching must keep original slot identity and alias metadata must survive until interpolation. This is an architecture proposal, not verified implementation behavior.

`SHADER_CHANGE_REQUIRED=NO` for the alias path: the shader already indexes WORLD by vertex. This assumes all active WORLD consumers and companion buffers accept the extended indices; that audit is an implementation gate.

## Vertex sharing

`VERTEX_CAN_BE_SHARED_ACROSS_DIFFERENT_POLICY_GAMECALLS=YES` structurally: `faceIndices` stores global vertex IDs; GameCall stores only a face start/count; no rule allocates a unique vertex per call. A concrete mixed-policy shared vertex count was not captured in this review.

`VERTEX_DUPLICATION_MAY_BE_REQUIRED=YES`. For each `(original vertex, alias WORLD slot)` used by a selected call, clone all parallel vertex attributes once, substitute the clone ID only in that call's `faceIndices` slice, and set the clone's WORLD index. Reuse a clone only when policy, original vertex and alias are identical. Keep original vertex/indices for rigid callers. `EXACT_VERTEX_DUPLICATION_LAYER=CPU Workload::drawData preparation before State::fullSync updateDrawDataRanges/uploadDrawData`. Check vertex output count, modification references, raw triangle path and all per-vertex arrays before enabling.

## Generic seam criterion

For endpoints `pA,WA` and `pB,WB` with distinct WORLD matrices, test `mul(pA,WA_prev) ≈ mul(pB,WB_prev)` and the same for current, in physical WORLD space using finite/affine checks and a scale-aware tolerance. If both endpoints agree, linearity makes elementwise WORLD interpolation retain that endpoint equality at every intermediate `t`. Endpoint coincidence alone is *not* proof of a logical attachment: intersecting or overlapping unrelated geometry can satisfy it.

`GENERIC_SAME_CALL_SEAM_DISCOVERY_FEASIBLE=CONDITIONAL`: build endpoint candidates from a call's indexed triangles and distinct WORLD uses. Prefer shared/adjacent edges with matched endpoint pairs and consistent orientation; one arbitrary nearby point is insufficient. Calls with no such evidence keep rigid policy.

`GENERIC_CROSS_CALL_SEAM_DISCOVERY_FEASIBLE=CONDITIONAL`: face slices and vertex positions are available within the workload, but no explicit cross-call seam identity exists. Compare compatible calls in the same workload/projection and require paired edge or multiple coherent endpoint matches at both frames. A single coincident point cannot be promoted to a proven seam. Separate workloads need additional cross-workload matching and are outside this prototype.

`SPATIAL_HASH_REQUIRED_FOR_CROSS_CALL=YES_FOR_BOUNDED_SEARCH`: hash WORLD endpoints into scale-aware cells, inspect neighboring cells, then verify exact two-endpoint distances and adjacency/provenance. It is an accelerator, not proof.

`SEAM_IDENTITY_CONFIDENCE_CRITERION`: finite row-vector WORLD endpoints; distinct original slots; a matched edge or at least two coherent endpoint pairs in compatible geometry; coincidence at PREV and CURRENT with scale-aware tolerance; stable original-to-previous mapping; reject ambiguous many-to-many matches, isolated crossings and missing endpoints. If topology cannot distinguish an accidental overlap, classify `UNKNOWN` and retain rigid interpolation. The tolerance and observed false-positive rate need an offline CPU fixture before visual promotion.

## Options and recommendation

| Option | Correctness and cross-call reach | Cost and invasiveness | Decision |
| --- | --- | --- | --- |
| A: all WORLDs in selected GameCalls | Solves slot leakage and can span calls if calls are selected; still gives nonrigidity to unrelated uses inside selected calls. | Low matrix count per call, but excess CPU and GPU matrix work; vertex sharing still needs C. | Insufficient as final policy. |
| B: seam-certified `(GameCall, WORLD)` aliases | Narrower policy and cross-call capable when both call uses are proven; must conservatively reject ambiguous overlaps. Still deforms other vertices in the same call-use, so inspect connected geometry. | CPU spatial join and small matrix alias buffers; compatible with existing indexed lookup after schedule repair. | Preferred decision rule. |
| C: clone vertex entries for conflicting uses | Necessary to prevent a shared vertex's one WORLD index from leaking policy between calls; supports B. | Additional parallel vertex data, face rewrite and GPU vertex processing; most invasive CPU-side portion. | Conditional companion to B, not independent policy. |
| D: shader/per-vertex branch | Can express fine per-vertex choice, including shared call cases only after separate vertices/metadata; does not solve seam discovery. | Shader ABI/branching and wider risk across games/backends. | Defer. |

`FINAL_DESIGN_GAME_SPECIFIC=NO`. No model, bone, display-list or game ID is a selector.

`DUPLICATED_MATRIX_COUNT_BOUND`: at most one alias per certified/candidate `(workload, GameCall, original worldIndex)` use, plus originals; not a multiplier on every transform. Candidate preallocation means the actual bound is the number of structurally plausible call-use pairs, which must be measured. With 233 calls / 42 multi-WORLD calls / 117 WORLD slots / 101 broad-selected slots, no observed seam count is available. `EXPECTED_MEMORY_OVERHEAD_CLASS=LOW` is a conditional expectation for narrow candidates, not a measured result. Each alias adds a 64-byte matrix to each relevant CPU/GPU matrix stream; vertex clones add parallel attributes and processed output. Set a per-workload cap and rigid fallback.

`PROPOSED_ARCHITECTURE_CAN_COVER_CROSS_GAMECALL_SEAMS=YES_CONDITIONAL` for compatible calls in one workload/projection. It can cover separate parent/child calls or calls reusing an original WORLD slot when distinct paired WORLD uses and geometry provide evidence. It cannot infer a hidden attachment from a lone point or missing previous match. `SNOUT_SPECIFIC_ROOT=UNPROVEN`; no old dual-matrix claim is revived.

`FINAL_DESIGN_LIMITS_ELEMENTWISE_TO_PROVEN_SEAM_USES=YES_AS_DESIGN_INTENT`: only certified call-use aliases switch to elementwise; otherwise alias and original both use rigid interpolation. This does not prove the Klungo/Mumbo regressions disappear; their causes remain `UNKNOWN`. Because an alias applies to every selected vertex using it, certification must also examine unwanted deformation away from the seam before production promotion.

`RECOMMENDED_PROTOTYPE`: CPU-side B+C, default-off. In `State::fullSync`, identify conservative same-call seam candidates, reserve `(GameCall, original WORLD)` aliases and clone/remap only conflicting vertices before existing upload. Preserve an alias-to-original map. In `TransformProcessor`, use original frame-map endpoints; certify both-frame seams, fill aliases elementwise only when certified, otherwise copy the original rigid result. First prototype scope is same workload/projection; extend the same spatial join to cross-GameCall candidates after false-positive/overhead counts are reviewed. No shader branch or new GPU transfer path. Abort safely to rigid when index, capacity, matching, ambiguity or affine guards fail.

`IMPLEMENTATION_FILES`: RT64 `src/hle/rt64_state.cpp`, `src/hle/rt64_workload.h`, `src/hle/rt64_game_frame.cpp` (only if alias entries affect matching), `src/render/rt64_transform_processor.cpp`; inspect `src/hle/rt64_workload.cpp` for range/upload invariants and `src/render/rt64_rsp_processor.cpp`/`src/render/rt64_vertex_processor.cpp` for all alias consumers. No Tooie production file or HLSL change intended.

`IMPLEMENTATION_DATAFLOW`: existing GameCall face slice -> CPU candidate seam/ownership plan -> pre-upload vertex/matrix aliases and face remap -> original GameFrame match -> two-endpoint certification -> rigid originals and certified elementwise aliases -> existing WORLD upload -> existing RSP WORLD-index lookup.

`REQUIRED_NEW_CPU_METADATA`: bounded alias records `{workload, framebuffer pair, projection, GameCall, original WORLD, alias WORLD, candidate endpoint/edge pairs, original vertex -> clone map}`, plus certification state. Keep records outside the original call-matching identity; do not invent game-specific IDs.

`REQUIRED_WORLD_BUFFER_CHANGES`: append aliases coherently to original/current, interpolated previous/current, inverse-transpose and endpoint WORLD arrays consumed by active paths; extend existing upload range, enforce `uint16_t` ceiling, preserve original call-matching ranges.

`REQUIRED_VERTEX_REMAP_CHANGES`: clone all required parallel vertex attributes for conflicting call uses; rewrite only the owning GameCall's `faceIndices` span before the existing draw-data upload. Recompute ranges/output sizes through existing code and verify raw/modified vertex cases.

`ROOT_CAUSE_CLASS=NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`; `CURRENT_WORLD_INDEX_FIX_STATUS=INSUFFICIENT_GRANULARITY`; `PRODUCTION_FIX_UPDATED=NO`; `CLEANUP_READY=NO`; `COMMIT_READY=NO`; `GPU_READBACK_USED=NO`; `GPU_SYNCHRONIZATION_ADDED=NO`; `NO_COMMIT=YES`; `NO_PUSH=YES`.

`NEXT_SINGLE_ACTION`: implement only the default-off CPU alias/remap prototype above, beginning with a dry-run count of candidate aliases, shared vertices, index/capacity guards and two-endpoint seam certifications before enabling the visual remap. Grok 4.6 Medium may implement after this review; no production promotion without those counts and a clean visual discriminator.
