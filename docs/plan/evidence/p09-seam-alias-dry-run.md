---
title: "P09 seam-alias dry-run prototype"
description: "Default-off CPU plan of call-local WORLD aliases and vertex clones. No draw-data or shader change."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-24
tags: [tooie-recomp, p09, rt64, interpolation, dry-run]
---

# P09 seam-alias dry-run prototype

## Scope

Implements only the default-off CPU dry-run from
[[p09-call-local-world-interpolation-architecture-review]]. No matrix
aliases, no `worldIndices` remap, no vertex clones, no shader edit, no
elementwise enablement, no VIEW/PROJECTION change. The game was not
launched. `SNOUT_SPECIFIC_ROOT=UNPROVEN`.

Toggle: `TOOIE_DIAG_P09_SEAM_ALIAS_DRY_RUN=1`.
Output: `/tmp/tooie-p09-seam-alias-dry-run.txt` (session aggregates plus
the latest workload; overwritten atomically).

## Layer answers (static, `PROVEN` from the current RT64 sources)

`GAMECALL_IDENTITY_AVAILABLE_AT_DRY_RUN_LAYER=YES`. The dry-run walks
`FramebufferPair -> Projection -> GameCall` face slices
(`meshDesc.faceIndicesStart`, `callDesc.triangleCount`) inside
`TransformProcessor::process`, the same join the selector audit uses.

`FRAME_MATCH_AVAILABLE_FOR_ORIGINAL_WORLD=YES`. It reads
`GameFrameMap::TransformMap::mapped` and `prevTransformIndex` for the
original slot, then the previous workload's `worldTransforms`. Unmapped
slots cannot certify a seam.

`ALIAS_INSERTION_POINT_BEFORE_UPLOAD_CONFIRMED=YES`. A future materialization
would append aliases and rewrite only certified face corners in
`State::fullSync` before `updateDrawDataRanges` / `uploadDrawData`
(`rt64_state.cpp` around the existing upload). This prototype does not
insert there. Certification stays in `TransformProcessor::process` because
that is where previous and current WORLD endpoints already exist. Aliases
must remain outside `GameCall.minWorldMatrix` / `maxWorldMatrix` so
`GameFrame` matching keeps the original slot.

`CROSS_CALL_SPATIAL_HASH_IMPLEMENTED=YES`. A fixed 4096-slot hash of
current-frame `mul(localPos4, WORLD)` positions (cell 4, 26-neighborhood)
only retrieves neighbors. Certification is a separate both-frame test.
`SPATIAL_HASH_IS_RETRIEVAL_ONLY=YES`.

## Seam rule

Row-vector product matches RSP vertex load: `hlslpp::mul(float4(x,y,z,1), WORLD)`.
Distinct original WORLD slots are required. A pair certifies only when both
endpoints are finite and affine (`|w-1| <= 1e-3`) and the world-space
distance is within `max(0.25, max(localMag, worldMag) * 1e-4)` at PREV and
at CURRENT, and both slots have a real previous-frame match.

`SEAM_IDENTITY_KEY=workloadId,fbPair,projection,gameCall,vertexOccurrence,originalWorld,pairedVertex,pairedWorld`.

`AMBIGUOUS_MATCHES_DEFAULT_TO_RIGID=YES`. Two or more distinct partners
within tolerance demote the point to rigid. No elementwise alias is planned
for that geometry.

`ALIAS_PLAN_KEY=workloadId,fbPair,projection,gameCall,originalWorldIndex,ELEMENTWISE_SEAM_USE`.
One planned alias per certified `(GameCall, original world)`. Only seam
endpoint vertices would be remapped. Other vertices in that call that use
the same world stay on the original index and `RigidBody::lerp`.

`CERTIFIED_ALIAS_IS_PER_SEAM_USE_NOT_WHOLE_WORLD_SLOT=YES`.
`UNRELATED_GEOMETRY_IN_SAME_GAMECALL_REMAINS_RIGID_BY_DESIGN=YES`.

## Vertex storage

`faceIndices` stores global vertex ids. `worldIndices` is one `uint16_t`
per stored vertex. The dry-run counts, and does not clone:

- `SHARED_VERTEX_REFERENCE_COUNT`: stored vertices referenced from two or
  more GameCalls.
- `SHARED_VERTEX_DIFFERENT_POLICY_COUNT`: those shared vertices whose
  references disagree (seam vs rigid).
- `FACE_CORNER_REFERENCE_CAN_BE_REMAPPED_WITHOUT_VERTEX_CLONE_COUNT`: seam
  corners whose stored vertex has no rigid reference, so the existing
  `worldIndices` entry could change in place.
- `PHYSICAL_SHARED_VERTEX_REQUIRES_CLONE_COUNT` and
  `VERTEX_CLONE_REQUIRED_COUNT`: stored vertices with both a seam reference
  and a rigid reference. Those two counts match because each mixed vertex
  needs one extra stored copy. Face-corner identity is the other count.

Raw triangle streams (`triPosFloats`) are not indexed by this walk.
`RAW_TRI_PATH_EXCLUDED=YES`.

## Buffers and matching

`WORLD_INDEX_IS_16_BIT=YES` (`DrawData::worldIndices` is `uint16_t`;
`DrawCall::minWorldMatrix` / `maxWorldMatrix` are `uint16_t`).
Hypothetical count is original slots plus planned aliases. It fits while
that sum is `<= 65536`.

`ALIAS_CAN_INHERIT_ORIGINAL_FRAME_MATCH=YES`.
`ALIAS_REQUIRES_NEW_FRAME_MATCH_ENTRY=NO`.
`ALIAS_TEMPORAL_IDENTITY_DESIGN`: the alias record stores
`originalWorldIndex` and would read that slot's
`GameFrameMap::TransformMap`. It does not allocate a match key.

Companion streams that would have to grow together, and do not grow now:
`worldTransforms`, `lerpWorldTransforms`, `prevWorldTransforms`,
`invTWorldTransforms`, `endpointWorldTransforms`, and the matching
`TransformProcessor::upload` ranges plus the `State::fullSync`
`worldTransforms` range. Vertex clones would also extend the parallel
attribute ranges in `Workload::uploadDrawData` and the output buffers sized
from `vertexCount()`. `faceIndices` would be rewritten in place, not grown.
`modifyPosUints` stays keyed by the original vertex index unless a clone is
materialized later.

Overhead estimate (not yet measured on a game session): each alias is
`6 * 64` bytes across those matrix streams; each clone is `55 + 88` bytes
of attributes plus output buffers. Class thresholds: `LOW` under 64 KiB,
`MEDIUM` under 1 MiB, otherwise `HIGH`.

## Bounds

Per workload caps, with `BOUNDS_SATURATED` when hit: 2048 calls, 96
cross-world edges per call, 1024 provisional pairs, 3072 spatial samples
(24 per call, stride 4), 8 nearest cross-call tests, 1024 alias records,
vertex policy audit through 262144 vertices. Counts are lower bounds when
saturated. No per-vertex stdout and no GPU readback or synchronization.

## Runtime counts

`SAME_CALL_*`, `CROSS_CALL_*`, alias, clone, and session cost fields are
`NOT_CAPTURED`. This session did not launch the game.

`GENERIC_CROSS_CALL_CERTIFIED_SEAMS_EXIST=NOT_CAPTURED`.

## Validation

Tooie HEAD `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf`. RT64 HEAD
`6f1c2d99a4ea571c139f449c326fd176ba8f3496` plus the existing dirty worktree.
The only RT64 edit for this task is `src/render/rt64_transform_processor.cpp`.

```text
cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test
exit 0
./build/window_display_test          38/38 PASS, exit 0
./build/tintro_projection_policy_test 7/7 PASS, exit 0
```

`RENDERING_BEHAVIOR_CHANGED=NO`.
`ROOT_CAUSE_CLASS=NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`.
`CURRENT_WORLD_INDEX_FIX_STATUS=INSUFFICIENT_GRANULARITY`.
`PRODUCTION_FIX_UPDATED=NO`.
`CLEANUP_READY=NO`.
`COMMIT_READY=NO`.
`GPU_READBACK_USED=NO`.
`GPU_SYNCHRONIZATION_ADDED=NO`.
`NO_COMMIT=YES`.
`NO_PUSH=YES`.

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_P09_SEAM_ALIAS_DRY_RUN=1 ./build/TooieRecompiled`
on the character scene, then read `/tmp/tooie-p09-seam-alias-dry-run.txt`.
Leave the WORLD-index visual toggles off. Do not add real matrix duplication
or `worldIndices` remapping from this note.
