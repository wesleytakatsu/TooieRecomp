---
title: "P09 post-CPU-handoff structural forensic — first plausible RSPProcess layer"
description: "Static audit after V7 VERSION=2: CPU handoff is correct; target DLs have no G_MODIFYVTX; first remaining writer is RSPProcessCS using post-processor matrices."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-07
tags: [p09, kazooie, feather, rt64, rspprocess, rspmodify, projection]
---

# P09 post-CPU-handoff structural forensic

Bounded objective: locate the first plausible post-handoff cause of the
Kazooie feather movement glitch. Static / CPU-side only. No game, no
production fix, no V8 implementation, no GPU readback or added
synchronization, no commit, no push.

V7 VERSION=2 remains the closed CPU-handoff result. Do not reopen exact
target identity, model-bin bridge, VTX/join provenance, CPU
`faceIndices`, fullSync face slice, upload source, worldIndex/viewProj
CPU data, selected matrix source, or simple/temporal world-matrix
interpolation as the primary cause unless new primary evidence
contradicts it.

```text
CPU_HANDOFF_STATUS: CORRECT (authoritative V7 VERSION=2)
FIRST_DATA_DIVERGENCE: UNKNOWN_AFTER_CPU_HANDOFF
FIRST_UNEXPLAINED_LAYER: RSPPROCESSCS (shader inputs may already include ProjectionProcessor-modified Q)
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
PRODUCTION_FIX: NO
```

Checkout: Tooie `ff5b6d9` dirty; RT64 `6f1c2d9` dirty sibling
`BanjoRecomp/lib/rt64`. Asset dump `/tmp/asset_062B.bin` (78200 bytes)
used only to disassemble static DLs.

## Position dataflow (current source)

Proven CPU arrays after V7: `posFloats` stride 3, `velFloats` stride 3
and zero, `worldIndices` uint16, `viewProjIndices` uint16,
`worldTransforms`, `viewProjTransforms`.

Render replay then:

1. `ProjectionProcessor::process/upload` copies then may lerp/aspect-adjust
   into `modViewProjTransforms` and uploads that over
   `viewProjTransformsBuffer`.
2. `TransformProcessor::process/upload` builds `lerpWorldTransforms` /
   `prevWorldTransforms` and uploads `lerpWorldTransforms` when a previous
   frame pointer exists.
3. `Workload::resetRSPOutputBuffers` then `RSPProcessor::process` once per
   workload (`vertexStart = computedSize/16 = 0` after reset).
4. `FramebufferRenderer::recordSetup` copies uploads, then
   `RSPProcessor::recordCommandList`: `RSPProcessCS` then optional
   `RSPModifyCS`.
5. Optional `VertexProcessor` / `RSPWorldCS` writes **world** pos/norm/vel
   buffers. Raster POSITION does not read those buffers.
6. `drawIndexedInstanced(faceCount*3, 1, faceIndicesStart, 0, 0)` with
   vertex slot 0 = `screenPosBuffer`.
7. `RasterVS` converts N64 screen XY to framebuffer NDC, reapplies
   homogeneous W, screenScale/offset, optional prim-depth Z.

```text
POST_HANDOFF_POSITION_DATAFLOW:
  CPU SoA (V7-correct)
    -> ProjectionProcessor Q' = modViewProjTransforms[viewProjIndex]
    -> TransformProcessor W' = lerpWorldTransforms[worldIndex] (or original if no prev)
    -> RSPProcessCS screenPos[v] = viewport(Q' * W' * float4(P - V*(1-curWeight), 1))
    -> RSPModifyCS may overwrite screen XY or Z for modifyPosUints entries only
    -> RasterVS(screenPos[faceIndex])  // no extra world/proj
FIRST_POST_HANDOFF_POSITION_WRITER: RSPProcessCS (dstPos = screenPos)
LAST_POSITION_WRITER_BEFORE_RASTER: RSPModifyCS if modifyCount>0 else RSPProcessCS
RSPWORLDCS_RELEVANT_TO_RASTER_POSITION: NO
```

## RSPProcessCS indexing

`vertexOffsetIndex = vertexStart + SV_DispatchThreadID`.
`vertexStart` is reset to 0 per workload immediately before `process`.
`vertexCount = drawVertexCount`. Extra threads in the 64-wide group
return early. Source loads use the same global index: pos/vel `*3`,
world/viewProj `Buffer<uint>` over `R16_UINT`. Shader
`worldTransforms[srcWorldIndices[v]]` and
`viewProjTransforms[srcViewProjIndices[v]]`.

```text
RSPPROCESS_VERTEX_INDEX_FORMULA: vertexStart + dispatchVertex; vertexStart=0 on ordinary path
RSPPROCESS_WORLDINDEX_LOAD: srcWorldIndices[vertexOffsetIndex]  // R16_UINT zero-extended
RSPPROCESS_VIEWPROJ_INDEX_LOAD: srcViewProjIndices[vertexOffsetIndex]
RSPPROCESS_WORLD_MATRIX_SELECTION: worldTransforms[transformIndex]  // GPU buffer = selected lerp or original
RSPPROCESS_VIEWPROJ_SELECTION: viewProjTransforms[viewProjIndex]    // GPU buffer = ProjectionProcessor modViewProj
RSPPROCESS_INDEXING_STRUCTURALLY_CORRECT: YES on the per-workload reset path
RSPPROCESS_PLAUSIBLE_TARGET_LOCAL_FAILURE: NO structural off-by-one/base/stride found; remaining risk is selected Q'/W' contents, not index arithmetic
```

V7 sampled `viewproj_index=4/4/4` on `OWN_105_0x534`; render selection
dumps `RENDER_VIEWPROJ_REFERENCED index=4` as the moving camera matrix.
Index 0 dumped as identity is the unused slot-0 upload, not the feather Q.

## Velocity

F3DEX2 `setVertexCommon<true>` writes `vel=0`. Default
`TransformGroup.vertexInterpolation = G_EX_COMPONENT_SKIP`.
`GameFrame::matchTransform` can fill `velFloats` **after** V7 fullSync
only when `vertexInterpolation != SKIP` and position hashes differ.
Tooie original DLs do not emit `gEXMatrixGroup` vertex-interpolation
overrides on this path (`STRONG_INFERENCE`).

RSPProcess formula with `V=0` reduces to `P` regardless of
`curFrameWeight`.

```text
TARGET_VELOCITY_ZERO_ELIMINATES_RSPPROCESS_VERTEX_VELOCITY_AS_PRIMARY_CAUSE: YES
  (conditional on vertexInterpolation remaining SKIP; not reopened as generic interpolation)
```

## Post-handoff matrix reconstruction

`POST_HANDOFF_MATRIX_REINTERPOLATION_EXISTS: YES`

| Location | Inputs | Covered by V5B/V7? |
| --- | --- | --- |
| `TransformProcessor::process` `rigidBody.lerp` into `lerpWorldTransforms` | prev/cur world, weights | YES — V5B `NO_INTERP` on bones 79,100–105 visually `SAME`; keep **closed** as primary |
| `ProjectionProcessor::processScene` view lerp; optional proj lerp; aspect `adjustProjectionMatrix`; upload `modViewProjTransforms` | prev/cur view/proj, `viewProjMap`, weights, aspect | **NO** — V5B only equalized world results; V7 compared pre-processor viewProj CPU arrays |

World-matrix interpolation stays closed. Projection reconstruction is a
new post-handoff Q writer. It is movement-compatible and a **weak** fit
for feather-only distortion because sampled feather `viewProjIndex` is
the shared referenced camera index 4.

## RSPModifyCS

CPU `RSP::modifyVertex` is reached only from F3DEX2 `G_MODIFYVTX` (opcode
`0x02`; F3DEX `0xB2`). Attributes:

- `G_MWO_POINT_RGBA` `0x10` — color, no `modifyPosUints`
- `G_MWO_POINT_ST` `0x14` — UV, no `modifyPosUints`
- `G_MWO_POINT_XYSCREEN` `0x18` — `modifyPosUints` (bit0=0), overwrites screen XY
- `G_MWO_POINT_ZSCREEN` `0x1C` — `modifyPosUints` (bit0=1), overwrites screen Z

Shader does not write clip W / reciprocal W.

If `used[slot]`, modify copy-on-writes a new global vertex; old
`faceIndices` keep the old global. Modify dispatch runs after Process
with existing compute/graphics barriers. No synchronization was added.

Static `/tmp/asset_062B.bin` disassembly (`Gfx` base `model+0x2670`,
logical `*8`):

| DL | Sequence after setup | `G_MODIFYVTX` |
| --- | --- | --- |
| `0x534` | `G_DL 0x03000040`, `G_VTX n=12 v0=0`, four `G_TRI2`, ENDDL | 0 |
| `0x54C` | nested DL, `G_VTX n=4 v0=0`, `G_POPMTX`, `G_VTX n=4 v0=28`, two `G_TRI2` | 0 |
| `0x564` | same join pattern | 0 |
| `0x57C` | nested DL, `G_VTX n=8 v0=0`, two `G_TRI2` | 0 |
| `0x592` | same join pattern as `0x54C` | 0 |
| `0x5AA` | nested DL, `G_VTX n=8 v0=0`, two `G_TRI2` | 0 |
| nested `0x03000040` if `seg3=Gfx[0]` | leftover TRI then `G_VTX 0x010000A0` | 0 |

Zero plausible `G_MODIFYVTX` (`where` in `{0x10,0x14,0x18,0x1C}`) in
`Gfx[0]..vertex tables`. Other models in the same workload can still
emit modify records; they address **their** global indices, not these
target vertices.

```text
TARGET_RSP_MODIFY_COMMANDS_STATICALLY_POSSIBLE: NO
TARGET_534_RSPMODIFY_REACHABLE: NO
TARGET_54C_RSPMODIFY_REACHABLE: NO
TARGET_564_RSPMODIFY_REACHABLE: NO
TARGET_57C_RSPMODIFY_REACHABLE: NO
TARGET_592_RSPMODIFY_REACHABLE: NO
TARGET_5AA_RSPMODIFY_REACHABLE: NO
RSPMODIFY_USES_VERTEX_CACHE_SLOT_IDENTITY: YES (dstIndex is RSP cache slot; payload stores globalIndex)
SLOT_REUSE_CAN_TARGET_WRONG_VERTEX: UNKNOWN in general; NO on this target path (no modify)
TARGET_PATH_HAS_MODIFY_AFTER_SLOT_REUSE: NO
```

Join slot reuse (`0–3` then `28–31`) is by design and already proven at
V7. It is not a modify-slot bug.

## ProjectionProcessor / RasterVS

`PROJECTIONPROCESSOR_CAN_CHANGE_TARGET_POSITION: YES` via Q' and
viewport-associated `viewProjIndex`. Temporal state: previous view/proj
and `RigidBody` lerp flags. Articulated-local-only distortion:
`WEAK` / `UNKNOWN` given shared index 4.

`RasterVS` input is `float4` screen position. No additional world or
projection. Same global index as RSPProcess (`baseVertex=0`). Cannot by
itself create feather-only corruption unless `screenPos` is already
wrong or prim-depth Z is used (Z only).

```text
RASTERVS_APPLIES_ADDITIONAL_WORLD_MATRIX: NO
RASTERVS_APPLIES_ADDITIONAL_PROJECTION: NO
RASTERVS_CAN_USE_DIFFERENT_INDEX_THAN_RSPPROCESS: NO
RASTERVS_CAN_CREATE_LOCAL_FEATHER_ONLY_CORRUPTION: NO
```

## Discrimination

Movement-only (highest first):

1. ProjectionProcessor view/proj interpolation — `STRONGLY_COMPATIBLE` movement, `WEAK` localized
2. TransformProcessor world lerp — `STRONGLY_COMPATIBLE` both, **REFUTED** by V5B as primary
3. `matchTransform` velocity — `STRONGLY_COMPATIBLE` if enabled; `WEAK` here (`SKIP`, V=0)
4. RSPModify — `WEAK`/`INCOMPATIBLE` (static commands absent)
5. Modify after slot reuse — `INCOMPATIBLE` (no modify)
6. RSPProcess indexing — `INCOMPATIBLE` with movement-only (would be permanent)
7. RasterVS indexing — `INCOMPATIBLE` with local-only

Localized-geometry (highest first):

1. Per-bone W' (TransformProcessor) — natural; **REFUTED** as primary
2. RSPProcess using those W' — same
3. Join mixed W on `0x54C/0x564/0x592` — natural; CPU provenance `CORRECT`
4. RSPModify / slot reuse — would be local **if present**; statically absent
5. Shared Q' projection — `WEAK`
6. RasterVS — `INCOMPATIBLE`

## Root and next witness

No proven deterministic production bug. Do not implement a fix.

```text
CURRENT_BEST_ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: MEDIUM that RSPProcessCS is the first remaining position writer and that RSPModify is not on the six target DLs; LOW for a causal renderer defect
PRODUCTION_FIX_CANDIDATE_FOUND: NO
BEST_NEXT_WITNESS: CPU reconstruction of RSPProcess inputs AFTER TransformProcessor+ProjectionProcessor (W', Q', P, V, viewProjIndex) versus the V7-proven handoff (W, Q, P, V). Optionally include one non-feather body control vertex that shares the draw.
WHY_MAX_INFORMATION_GAIN: distinguishes “shader is fed already-divergent Q'/W'” from “GPU RSPProcess/Raster diverges from those inputs” without GPU readback
CAN_BE_CPU_OR_COMMAND_STREAM_ONLY: YES
EXPECTED_CAPTURE_COUNT: 1
```

Do not create a broad V8. Do not investigate `RSPProcessCS` as a
hand-written GPU bug, and do not move the investigation into
`RasterVS` as a primary, until that CPU reconstruction exists.

Do not treat leftover `byte_mismatch` as identity. Do not reopen V7
CPU equality.
