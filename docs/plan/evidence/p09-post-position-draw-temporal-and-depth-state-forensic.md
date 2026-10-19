---
title: "P09 post-position draw-temporal and depth-state forensic"
description: "Face/draw/GameFrame association and inherited depth state are closed as local mismatch for the 14 visible feather DLs."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-07
tags: [p09, kazooie, feather, draw-temporal, depth, occlusion]
---

# P09 post-position draw-temporal and depth-state forensic

Bounded objective: decide whether draw temporal association or
depth/occlusion state can explain the movement-only Kazooie visible
feather glitch after position and culling are closed. Static RT64
source plus existing V3 / Q captures only. No game. No new
instrumentation. No production fix. No GPU readback or added
synchronization. No commit. No push. Do not reopen a position writer
or the culling forensic.

Authoritative closed results remain
[[p09-bounded-crest-post-handoff-transferability-audit]] (position
path `CLOSED`) and
[[p09-visible-feather-culling-winding-face-visibility-forensic]]
(culling status `A`, `MISSING_TRIANGLES_OBSERVED: NO`).

Checkout: Tooie `ff5b6d9` dirty. BanjoRecomp `ec85963`. RT64
`6f1c2d99` protected diagnostics. Asset `/tmp/asset_062B.bin`
(78200 bytes). Captures used offline:

- V3: `/tmp/tooie-kazooie-feather-v7-cpu-handoff-status.txt` + `.csv`
- Q: `/tmp/tooie-kazooie-feather-postprocess-q-v1-status.txt`

```text
DRAW_TEMPORAL_ASSOCIATION_STATUS: CLOSED
DEPTH_OCCLUSION_STATE_STATUS: CLOSED
PRODUCTION_FIX_CANDIDATE_FOUND: NO
NEW_CAPTURE_REQUIRED: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
```

## 1. Post-handoff draw temporal dataflow (`PROVEN` source)

Current path (not the 2026-09-05 architectural summary):

```text
POST_HANDOFF_DRAW_TEMPORAL_DATAFLOW:
  RSP::drawIndexedTri
    -> Workload.drawData.faceIndices.push_back (V7 APPEND)
    -> DrawCall.triangleCount++
  State::flush
    -> GameCall.callDesc = drawCall
    -> FramebufferPair::addGameCall (current Projection)
  State::fullSync
    -> running faceIndex from drawRanges.faceIndices.first
    -> GameCall.meshDesc.faceIndicesStart = faceIndex
       faceIndex += triangleCount * 3   (viewport projections)
    -> V7 INITIAL_HANDOFF (same workload)
    -> Workload::updateDrawDataRanges / uploadDrawData
    -> initial viewProjTransforms + worldTransforms uploads
    -> State::advanceWorkload (assigns public workloadId)
    -> WorkloadQueue::advanceToNextWorkload
    -> next slot Workload::begin -> reset + diagnosticGeneration++
  WorkloadQueue::renderThreadLoop
    -> wait writeCursor != threadCursor
    -> GameFrame cur/prev rotate (2 slots)
    -> GameFrame::set(*this, &processCursor, 1)   // one workload per frame
    -> if targetRate>0 or RT: GameFrame::match (ordinal w)
    -> threadRenderFrame
         ProjectionProcessor / TransformProcessor when prev.matched
         RSPProcessCS on THIS workload's drawData
         FramebufferRenderer::addFramebuffer(curWorkload, fbPairIndex)
         drawIndexedInstanced(count, 1, faceIndicesStart, 0, 0)
    -> GPU execute + wait
    -> threadAdvanceWorkloadId then threadAdvanceBarrier
```

Identities:

| Identity | Owner | Notes |
| --- | --- | --- |
| `diagnosticGeneration` | `Workload::begin` atomic | V7 generation; 828–843 in capture |
| public `workloadId` | `State::advanceWorkload` after fullSync | INITIAL_HANDOFF logs 0; RENDER_SELECTION has gen+1 |
| ring slot | `writeCursor` in 4-slot array | `WORKLOAD_QUEUE_SIZE=4` |
| GameCall | `flush` into current fbPair/projection | `callIndex` then `gameCallCount++` |
| GameFrame | 2-slot `gameFrames` | `set(..., 1)` one workload |
| previous/current workload | `GameFrame::match` ordinal `w` | `prevWorkloadIndex = prevFrame.workloads[w]` |
| drawCall index | GameCall in projection | capture: one exclusive index per target DL |
| `meshDesc.faceIndicesStart/count` | fullSync running counter | V7 compared APPEND offset to this slice |
| vertex / matrix range | same `Workload.drawData` | no post-V7 identity rebase |
| framebuffer pair | `RDP::checkFramebufferPair` color+depth addresses | capture `fb_pair=3` |
| depth target | `RenderFramebufferKey` from that pair | dummy depth if no zCmp/zUpd |

```text
FACEINDICES_WORKLOAD_OWNER: the producing Workload (writeCursor slot)
DRAW_WORKLOAD_OWNER: same Workload; GameCall.meshDesc is a slice of it
GAMEFRAME_WORKLOAD_SELECTION: curFrame.set(processCursor) only
TRANSFORM_WORKLOAD_SELECTION: TransformProcessor walks curFrame.workloads;
  previous matrices from mapped prevWorkload, written in current-t order
```

## 2. Face vs frame association (`PROVEN` source + capture)

Face indices live in `Workload.drawData`. fullSync assigns
`faceIndicesStart` on GameCalls of that same object. Render replay
passes `curWorkload` into `FramebufferRenderer` and draws
`faceIndicesStart` from that workload's index buffer.

`TransformProcessor` may read previous-workload matrices, but it
emits `lerpWorldTransforms[t]` / `prevWorldTransforms[t]` in **current
index order**. `RSPProcessCS` still uses current `worldIndices[v]`.
That is interpolation, already closed as a localized position root
(V5B). It is not a face slice from generation N drawn with GameCall
metadata from generation N+k.

```text
FACE_AND_TRANSFORM_CAN_REFER_TO_DIFFERENT_GENERATIONS: NO
  (ownership). Previous matrices are a mapped interpolation operand
  only; shader index is current t.
FACE_AND_DRAW_METADATA_CAN_REFER_TO_DIFFERENT_GENERATIONS: NO
FACE_RANGE_REBASED_AFTER_V7: NO vertex identity rebase.
  faceIndicesStart is a running slice into the same vector.
VERTEX_RANGE_REBASED_AFTER_V7: NO
DRAW_RANGE_CAN_REFERENCE_ADJACENT_GAMECALL: NO (capture)
OFF_BY_ONE_OR_RING_REUSE_RISK: NO demonstrated; barrier after GPU wait
```

Lifetime:

```text
FACE_BUFFER_LIFETIME_GUARANTEE: live until Workload::begin/reset of that slot
VERTEX_BUFFER_LIFETIME_GUARANTEE: same
TRANSFORM_BUFFER_LIFETIME_GUARANTEE: same; render upload replaces GPU view
DRAW_METADATA_LIFETIME_GUARANTEE: GameCall vector reset on begin
DEPTH_TARGET_LIFETIME_GUARANTEE: RenderTargetManager keyed by guest address;
  not freed per workload
CAN_RING_SLOT_BE_REUSED_BEFORE_TARGET_DRAW_COMPLETES: NO
  advanceToNextWorkload stalls while nextWriteCursor==barrierCursor;
  barrier advances after threadRenderFrame GPU execute+wait
```

Known static predicate (already recorded in
[[p09-post-triangle-architectural-review]]): `TransformProcessor::upload`
selects lerp arrays when `prevFrame != nullptr`, while `process` fills
them only if the workload is mapped. Ordinary `set(...,1)` + `match`
before render supplies a mapped current workload. The 16-generation
window never hit the unmapped upload path (`workload_mapped=1`,
`process_transforms=1` on all 32 RENDER_SELECTION rows).

## 3. GameFrame matching (`PROVEN` source + capture)

```text
GAMEFRAME_MATCH_KEY: ordinal index w inside GameFrame.workloads
  (comment: "we assume the workloads will be detected in the same order")
MATCH_CAN_FAIL: YES if w >= prevFrame.workloads.size() (skip that w)
FAILURE_FALLBACK: that workload stays unmapped; TransformProcessor
  copies current matrices to prev; RSPProcess still uses current faces
MATCH_CAN_SELECT_WRONG_PREVIOUS_WORKLOAD: NO under set(..., 1)
  prevWorkloadIndex is prevFrame.workloads[0], the previous ring slot
```

When current and previous frames exist, `match` sets `mapped=true` and
pairs transform IDs by sorted `transformIdMap` keys. When mapping does
not exist, interpolation is skipped for that transform (`mapped=false`
uses current for both). Capture: every target generation is mapped to
`(ring_slot - 1) mod 4`.

```text
TARGET_DRAW_TEMPORAL_ASSOCIATION_STRUCTURALLY_CORRECT: YES
```

## 4. Depth command semantics (`PROVEN` `STATIC_ROM` + source)

RT64 raster depth is **otherMode**, not `G_ZBUFFER`. `G_ZBUFFER` is not
referenced in the inspected RT64 C++/HLSL. PSO:

`zCmp = !copy && otherMode.zCmp() && zMode != ZMODE_DEC`
`zUpd = !copy && otherMode.zUpd()`

`FramebufferPair.depthRead/Write` is the OR of those bits on its
GameCalls.

All 14 feather DLs preserve `G_ZBUFFER` (`GEO and=0xC0FFFB`). They do
**not** set zCmp / zUpd / ZMODE / zSource.

Plume DLs (all twelve) patch only:

- `G_SETOTHERMODE_H` size=2 bitoff=14 data=`0x8000` → `G_TT_RGBA16`
- `G_SETOTHERMODE_L` size=2 bitoff=0 data=`0x1` → `G_AC_THRESHOLD`
- restore both to 0 after triangles

Crest `0x4A6`/`0x4B0` and head `0x48F`: **zero** SETOTHERMODE.
Torso `0xBC`: TEXTLUT on/off only, no SOM_L.
None of the 16 DLs contain `G_SETPRIMDEPTH`, `G_SETZIMG`,
`G_RDPSETOTHERMODE`, or `G_CULLDL`. Nested `0x03000040` / `0x03000010`
remain `RUNTIME_REQUIRED`; hypothesized `Gfx[0]` has TEXTLUT only.

```text
RIGHT_DEPTH_STATE: INHERITED otherMode zCmp/zUpd/ZMODE/zSource;
  DL does not set them. G_ZBUFFER preserved, unused by RT64 raster.
LEFT_DEPTH_STATE: same
CREST_DEPTH_STATE: same (no SOM at all)
BODY_CONTROL_DEPTH_STATE: same inheritance; 0x48F no SOM;
  0xBC TEXTLUT only
FEATHER_DEPTH_STATE_SAME_AS_BODY: YES for z bits (none of these DLs set them)
FEATHER_DL_SETS_DEPTH_STATE_ITSELF: NO
INHERITED_DEPTH_STATE_SOURCE: caller RSP otherMode.L snapshotted into
  DrawCall at loadDrawState (OtherMode change)
DEPTH_STATE_CAN_CHANGE_BETWEEN_RIGHT_LEFT_CREST: NO in the 14 DLs
DEPTH_STATE_CAN_CHANGE_FRAME_TO_FRAME_DURING_MOVEMENT: only if the
  caller otherMode changes; that would also hit 0x48F/0xBC
TARGET_PATH_HAS_G_SETPRIMDEPTH: NO
FEATHER_USES_PRIMITIVE_DEPTH: NO in the 14 DLs
DEPTH_VALUE_SOURCE: RSPProcess screen.z (PIXEL) unless caller already
  set G_ZS_PRIM (not present in these DLs)
DEPTH_VALUE_DEPENDS_ONLY_ON_ALREADY_CLOSED_POSITION_PATH: YES
  (PIXEL path). Primitive-depth override is absent here.
```

Asymmetry that is **not** depth: plumes enable `G_AC_THRESHOLD` during
their triangles; crest/head do not. That belongs under color/alpha,
not this forensic's depth status.

## 5. Depth buffer / framebuffer association (`PROVEN` source)

`RDP::checkFramebufferPair` creates a pair from the current
`colorImage.address` and `depthImage.address`. Render key:

`RenderFramebufferKey(colorTargetKey(address,width,siz,Color),
                      depthTargetKey(address,width,siz,Depth))`

`getTargetsFromPair` uses that pair only. If the pair has no
depthRead/Write, a dummy depth target is bound (not a previous
workload's depth image).

```text
TARGET_COLOR_TARGET_SELECTION: fbPair.colorImage.address of the current workload
TARGET_DEPTH_TARGET_SELECTION: fbPair.depthImage.address if zCmp|zUpd,
  else dummy
COLOR_AND_DEPTH_TARGET_PAIRING_KEY: RenderFramebufferKey of that pair
CAN_DRAW_USE_CURRENT_COLOR_WITH_STALE_OR_WRONG_DEPTH: NO for pairing
CAN_DEPTH_TARGET_BELONG_TO_DIFFERENT_WORKLOAD_OR_FRAME: NO as a mismatched
  pair. The RenderTarget object is address-keyed and reused across frames
  (N64 framebuffer identity).
DEPTH_IMAGE_CHANGE_HANDLING: depthImage.changed -> submitFramebufferPair
  + addFramebufferPair with the new address
TARGET_DEPTH_BUFFER_CLEAR_SEMANTICS: FILL/clear ops on the manager;
  not per-GameCall. Preservation of a guest depth image is intentional.
CAN_STALE_DEPTH_SURVIVE_INTO_NEXT_RELEVANT_DRAW: YES as N64 preserve
CAN_FEATHER_GEOMETRY_BE_OCCLUDED_BY_PREVIOUS_FRAME_DEPTH: possible only
  as scene-wide preserved depth, not a feather-local bind bug
```

Do not infer a bug from preservation. A wrong-depth-image pairing would
also hit Banjo torso / Kazooie head on the same `fb_pair=3`.

## 6. Draw order (`PROVEN_FOR_CAPTURE` + GeoCmd)

V7 INITIAL_HANDOFF, 16 generations, exclusive `game_call` per target,
all `fb_pair=3` `projection=1`:

| DL | gc | region |
| ---: | ---: | --- |
| `0x4A6` | 52 | crest OWN |
| `0x4B0` | 53 | crest JOIN |
| `0x534` | 63 | right tip OWN |
| `0x54C` | 64 | right tip JOIN |
| `0x564` | 65 | right mid JOIN |
| `0x57C` | 66 | right mid OWN |
| `0x592` | 67 | right root JOIN |
| `0x5AA` | 68 | right root OWN |
| `0x5C0` | 69 | left tip OWN |
| `0x5D8` | 70 | left tip JOIN |
| `0x5F0` | 71 | left mid JOIN |
| `0x608` | 72 | left mid OWN |
| `0x61E` | 73 | left root JOIN |
| `0x636` | 74 | left root OWN |

That matches DCF48 deepest-first: crest (child of head 108) before
right plume (sibling of 108 under 107) before left plume. Head `0x48F`
is a child of 108 before crest (gc < 52, not in V7). Torso `0xBC` is
earlier in the same `0x62B` model.

```text
FEATHER_DRAW_ORDER: crest 52–53, right 63–68, left 69–74 (stable)
HEAD_BODY_DRAW_ORDER: 0x48F before crest; 0xBC earlier than the 14
ORDER_IS_STABLE_ACROSS_FRAMES: YES (same gc every gen)
CAN_JOIN_OR_PLUME_DRAW_BE_REORDERED_RELATIVE_TO_HEAD: NO in this window
```

If inherited z-write is on, later plumes can occlude earlier crest/head
when they overlap in Z. That is legitimate self-occlusion during
articulation, not a reorder bug.

## 7. Existing captures (`PROVEN_FOR_CAPTURE`)

V7 VERSION=3: `APPEND_FACE/POS/VEL/WORLD/VIEWPROJ/MATRIX_MATCH=1040`,
mismatch 0. `INITIAL_HANDOFF_MISMATCH` lines: 0. `INITIAL_HANDOFF_OK`:
224 (14 targets × 16 gens). `RENDER_SELECTION`: 32 (16 gens × 2
interpolated display weights `0/0.5` and `0.5/1`).

Per target: `triangle_count` equals static TRI count; one `game_call`;
`fb_pair=3`; `projection=1`; `unassigned=0`; no face-range overlap
across different GameCalls.

RENDER_SELECTION: `prev_frame_present=1`, `prev_frame_matched=1`,
`workload_mapped=1`, `process_transforms=1`, source `LERP`,
`previous_workload_index = (ring_slot-1) mod 4` on every row.

Q witness: target and body share `viewProjIndex=4`; not a frame-selection
split.

```text
EXISTING_CAPTURES_CAN_CLOSE_DRAW_TEMPORAL_ASSOCIATION: YES
GENERATION_MISMATCH_COUNT: 0
DRAW_RANGE_OWNERSHIP_MISMATCH_COUNT: 0
FRAME_SELECTION_MISMATCH_COUNT: 0
```

Limits: INITIAL_HANDOFF `finalized_workload_id=0` because the id is
assigned after the log. RENDER_SELECTION `wid=gen+1`. Use
`diagnosticGeneration` as the association key. The CSV does not dump
the guest depth-image address; pairing is closed from source plus the
constant `fb_pair=3`.

## 8. Movement-only ranking

High → low compatibility with “only while secondary geometry moves,
some frames look correct, localized feathers, body not equivalently
corrupted”:

1. **not this layer** — wrong GameFrame/workload association (`REFUTED`
   for the 16-gen window)
2. ring-slot lifetime/reuse — `NO` (GPU wait + barrier)
3. wrong face/draw range ownership — `NO` (exclusive gc, exact TRI counts)
4. wrong depth framebuffer pairing — `NO` (same fbPair key)
5. wrong inherited depth state as a *local* mismatch — `WEAK` (unset
   in the 14 DLs; shared with `0x48F`/`0xBC`)
6. stale depth contents — scene-wide if real; poor feather-only fit
7. legitimate self-occlusion / draw order — possible visual, not a
   proven renderer defect; order is stable

```text
TEMPORAL_DEPTH_DRAW_COMPATIBILITY_RANKING:
  association REFUTED; ring reuse REFUTED; range ownership REFUTED;
  depth pairing REFUTED as mismatch; inherited z state WEAK as local
  cause; stale depth WEAK as feather-only; self-occlusion possible
  expected geometry
```

## 9. Status

```text
DRAW_TEMPORAL_ASSOCIATION_STATUS: CLOSED
DEPTH_OCCLUSION_STATE_STATUS: CLOSED
CURRENT_BEST_ROOT_CLASS: VISIBLE_FEATHER_POSITION_PIPELINE_CLOSED_ROOT_ELSEWHERE
ROOT_CONFIDENCE: HIGH for temporal association (source + 16-gen capture);
  HIGH for depth as a local mismatch (DLs do not set z bits; pairing
  is fbPair-local; depth value is the closed PIXEL path)
PRODUCTION_FIX_CANDIDATE_FOUND: NO
BEST_NEXT_WITNESS: none
CAN_BE_CPU_COMMAND_STREAM_ONLY: N/A
EXPECTED_CAPTURE_COUNT: 0
NEW_CAPTURE_REQUIRED: NO
NEXT_VISUAL_ASSUMPTION: color/alpha/coverage (plumes uniquely set
  G_AC_THRESHOLD) or texture-coordinate corruption that looks
  geometric, or frame presentation/compositing
NEXT_SINGLE_ACTION: bounded static/command-stream audit of
  color/alpha/coverage and texture-coordinate paths on the 14 DLs;
  no new position, cull, or temporal witness; no GPU readback
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```
