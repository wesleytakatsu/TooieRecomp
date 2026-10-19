---
title: "P09 Kazooie feather V6 post-matrix vertex path"
description: "V5B full-chain NO_INTERP is visually SAME; static RT64 G_VTX/triangle path and the V6 triangle witness."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-04
tags: [tooie-recomp, p09, kazooie, feather, rt64, vertex, triangle, interpolation]
---

# P09 Kazooie feather V6 post-matrix vertex path

CPU/static plus diagnostic implementation only. The game was not run in this
checkpoint. No GPU readback, synchronization, copy, staging, or query. No
production interpolation or geometry change.

```text
PROVENANCE: USER_OBSERVED V5B + STATIC_ROM cart 0x62B + RT64 source
GPU_READBACK_USED: NO
PRODUCTION_FIX: NO
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V6
LATER_RESULT: V6 runtime TRI=0 was a witness-key failure, not missing geometry.
See [[p09-kazooie-feather-v6b-triangle-witness-repair]].
```

## Authoritative V5B negative result (`PROVEN` user visual + runtime schema)

**Later correction (V5C, 2026-09-09):** this seven-bone freeze omitted JOIN
parents 107 and 108. `REFUTED_FOR_VISIBLE_FEATHER_CHAIN` is
`CORRECTED/SUPERSEDED`. See [[p09-feather-no-interp-v5c]].

Full-chain `TOOIE_DIAG_FEATHER_NO_INTERP=1` covered seven bones
`79, 100, 101, 102, 103, 104, 105`. Masks GMTX / POSTMATCH / NO_INTERP were
all `0x7F`. Intermediate samples `170/170` and total `211/211` had
`RESULT_MATRIX == CURRENT_MATRIX` (`MAX_RESULT_CURRENT_ERROR=0`). AUTO
identity remained `CORRECT` with `VISIBLE_REQUIRED_MATCH_DROP_COUNT=0`.

User visual: `FEATHER_GLITCH_WITH_NO_INTERP_V5B: SAME`.

```text
TEMPORAL_MATRIX_INTERPOLATION_CAUSAL_STATUS: INCOMPLETE_COVERAGE_V5B (CORRECTED; 107/108 not frozen)
SIMPLE_INTERPOLATION_CAUSAL_STATUS: REFUTED_AS_PRIMARY_FIX (103-only test)
TEMPORAL_MATRIX_INTERPOLATION_ACTUALLY_BYPASSED_SEVEN_CHILD_BONES: YES
```

The earlier SIMPLE experiment was root-only (103) and also `SAME`. Do not
treat that SIMPLE test as a full-chain result. Do not reopen matrix identity
or AUTO previous selection.

This does **not** refute every temporal renderer behavior. It refutes
temporal **matrix** interpolation of the seven visible feather transforms as
the cause of the observed glitch.

## Phase 1 — `RSP::setVertexCommon` writes (`PROVEN_STATIC`)

F3DEX2 `G_VTX` (`0x01`) → `RSP::setVertex` →
`RSP::setVertexCommon<true, sizeof(Vertex)>` in
`BanjoRecomp/lib/rt64/src/hle/rt64_rsp.cpp`.

`setVertex` resolves `fromSegmentedMasked(address)`, `memcpy`s into
`RSP::vertices[dstIndex]`, then calls `setVertexCommon`.

Per loaded vertex, `setVertexCommon` appends to the current `Workload`:

| Field | Source |
| --- | --- |
| `posFloats` | copied object-space `Vertex.{x,y,z}` |
| `normColBytes` | `Vertex.color` |
| `viewProjIndices` | `curViewProjIndex` |
| `worldIndices` | `curTransformIndex` (`uint16_t`) |
| `fogIndices` / `lightIndices` / `lightCounts` / `lookAtIndices` | current RSP fog/light/lookAt |
| `indices[slot]` | new global vertex index |
| `used[slot]` | `false` |
| `velFloats` / `tcVelFloats` | `0` (`addEmptyVelocity=true` for F3DEX2) |
| `posTransformed` / `posScreen` | current `modelViewProjMatrix` + viewport |

A world transform is created only when `modelViewProjChanged` or
`modelViewProjInserted`: `curTransformIndex = worldTransforms.size()`, then
`worldTransforms.emplace_back(mul(modelMatrixStack.top, extended.viewProjMatrix))`.
`G_MTX` itself does not create the transform.

```text
SETVERTEXCOMMON_WRITES: posFloats, normColBytes, viewProjIndices,
  worldIndices=curTransformIndex, fog/light/lookAt indices, RSP cache
  indices[slot], vel=0, posScreen from current MVP
VERTEX_MODELVIEW_ID_SOURCE: extended.curModelMatrixIdGroupIndex
  (AUTO unless gEXMatrixGroup); snapshotted into worldTransformGroups
VERTEX_WORLD_MATRIX_INDEX_SOURCE: RSP::curTransformIndex at G_VTX time
VERTEX_TRANSFORM_INDEX_SOURCE: same curTransformIndex (worldIndices)
VERTEX_WORKLOAD_SOURCE: workloadQueue.workloads[writeCursor].workloadId
```

There is no independent previous-frame vertex identity stored on this F3DEX2
path. Texture/material state lives on the DrawCall, not on the vertex.

## Phase 2 — Feather vertices are static (`PROVEN` `STATIC_ROM`)

Cart `0x62B` skip-2 raw deflate, 78200 bytes. Gfx base `model+0x2670`.
F3DEX2 `G_VTX` decode: `n=p0(12,8)`, `v0=p0(1,7)-n`. Prior “`0x5AA` v0=8”
is `CORRECTED`: `w0=0x01008010` → `n=8`, `v0=0`.

| DL | GeoCmd | G_VTX | Source |
| --- | --- | --- | --- |
| `0x5AA` | 103 own `+0x12580` | n=8 v0=0 slots 0–7 | `0x010037D0` (segment 1) |
| `0x57C` | 104 own `+0x12540` | n=8 v0=0 slots 0–7 | `0x010036D0` |
| `0x534` | 105 own `+0x124D0` | n=12 v0=0 slots 0–11 | `0x01003510` |
| `0x592` | 103 join `+0x12550` | n=4 v0=0 slots 0–3 then POP then n=4 v0=28 slots 28–31 | `0x01003750` then `0x01003790` |
| `0x564` | 104 join | same 0–3 / 28–31 pattern | `0x01003650` / `0x01003690` |
| `0x54C` | 105 join | same 0–3 / 28–31 pattern | `0x010035D0` / `0x01003610` |

All those addresses are model segment-1 static vertex tables, not a CPU
skinned producer.

```text
FEATHER_VERTICES_STATIC_OR_DYNAMIC: STATIC
DL_5AA_VERTEX_SOURCE: segment1 0x010037D0 n=8 v0=0 slots 0-7
DL_57C_VERTEX_SOURCE: segment1 0x010036D0 n=8 v0=0 slots 0-7
DL_534_VERTEX_SOURCE: segment1 0x01003510 n=12 v0=0 slots 0-11
DL_592_VERTEX_SOURCE: segment1 0x01003750 slots 0-3 (child) then 0x01003790 slots 28-31 (parent)
```

Each of those DLs also `G_DL dest=0x03000040` **before** its own `G_VTX`.
If runtime segment 3 equals Gfx[0] (`model+0x2670`), dest is `Gfx+0x40`
(`+0x26B0`) and that chunk starts with leftover `G_TRI2` on slots
`4,5,6,7,8,2,1,9` then later `G_VTX n=8 v0=0 addr=0x010000A0`. Runtime
segment-3 base remains `UNKNOWN` without a DL-address witness.

## Phase 3 — Vertex interpolation vs matrix interpolation (`PROVEN_STATIC`)

F3DEX2 `addEmptyVelocity` writes `vel=0`. `RSPWorldCS` displayed position is
`mul(worldMats[srcIndices[i]], pos - vel*(1-curFrameWeight))`. With `vel=0`
the displayed position is the **current** `worldMats[per-vertex index]` only.
`prevWorldMats` feeds `dstVel` motion vectors, not the displayed position.

V5B already set `prevMatrix = curMatrix = curTransform` for the seven bones,
so even the motion-vector path is equalized for those transforms.

```text
FEATHER_VERTEX_INTERPOLATION_ACTIVE: NO for displayed position (vel=0)
VERTEX_INTERPOLATION_FUNCTION: RSPWorldCS (GPU); no separate CPU vertex-history matcher
VERTEX_INTERPOLATION_MODE: F3DEX2 empty velocity; world matrix lerp is TransformProcessor::RigidBody
VERTEX_TEMPORAL_PATH_CAN_RUN_WHEN_MATRIX_NO_INTERP: NO for displayed pos; motion-vector prevWorldMats is already equalized by V5B
```

V5B would not miss an independent vertex-history path because none exists on
this F3DEX2 CPU path. Remaining temporal behavior is later GPU / range
matching, not per-vertex lerp.

## Phase 4 — Transform index vs active modelview (`PROVEN_STATIC`)

Reloaded cache slots receive `worldIndices.emplace_back(curTransformIndex)`
at the G_VTX that loads them. Nested 103→104→105 therefore bind 105 verts
to 105's transform, 104's own mesh to 104, 103's own mesh to 103.

Join DLs **intentionally** mix: first G_VTX under the child modelview
(slots 0–3), `G_POPMTX`, second G_VTX under the parent (slots 28–31), then
triangles that reference both.

```text
VERTEX_TRANSFORM_INDEX_MATCHES_ACTIVE_MODELVIEW: YES for vertices loaded after the active G_MTX
BONE103_VERTEX_TRANSFORM_INDEX_STATUS: own 0x5AA slots 0-7 bind at G_VTX under destD/103
BONE104_VERTEX_TRANSFORM_INDEX_STATUS: own 0x57C slots 0-7 bind under 104
BONE105_VERTEX_TRANSFORM_INDEX_STATUS: own 0x534 slots 0-11 bind under 105
JOIN_DL_VERTEX_TRANSFORM_INDEX_STATUS: MIXED_BY_DESIGN (0-3 child, 28-31 parent)
```

Mismatch is runtime-capable only for **un-reloaded** leftover slots (nested
`0x03000040` candidate) and for join geometry. It is not inferred from the
visual glitch.

## Phase 5 — Vertex-cache reuse (`PROVEN` `STATIC_ROM` + `PROVEN_STATIC` RT64)

```text
FEATHER_VERTEX_CACHE_SLOT_MAP:
  0x5AA: load 0-7, TRI 0-7
  0x57C: load 0-7, TRI 0-7
  0x534: load 0-11, TRI 0-11
  0x592: load 0-3 child, POP, load 28-31 parent, TRI mix 0-3+28-31
SAME_VERTEX_CACHE_SLOT_REUSED_ACROSS_BONES: YES (own meshes all reuse 0-N)
CACHE_RELOAD_UPDATES_TRANSFORM_STATE: YES (memcpy + setVertexCommon writes new worldIndices and new global indices[slot])
```

A stale transform index on a **reloaded** slot is not supported by this code
path. Stale association remains possible on slots that are referenced by
triangles without a reload.

## Phase 6–7 — Triangle submit and batching (`PROVEN_STATIC`)

`G_TRI1`/`G_TRI2` → `RSP::drawIndexedTri`. Each corner uses
`worldIndices[globalIndex]` from the vertex, **not** the live modelview.
`DrawCall.minWorldMatrix` / `maxWorldMatrix` expand to the min/max of those
per-vertex indices. `State::flush` is driven by `checkDrawState` attribute
changes, not by `popMatrix`.

```text
TRIANGLE_TRANSFORM_SOURCE: per-vertex worldIndices[globalIndex]
DRAW_CALL_TRANSFORM_RANGE_SOURCE: min/max of those worldIndices on the accumulating DrawCall
MIXED_TRANSFORM_TRIANGLES_POSSIBLE: YES
FEATHER_DRAW_CALL_CONTAINS_MULTIPLE_BONE_TRANSFORMS: YES on join DLs; UNKNOWN on own DLs if leftover nested tris share the flush
BATCH_PRESERVES_PER_VERTEX_TRANSFORM: YES for GPU position (worldMats[index]); the DrawCall range is matching/batch metadata, not a single matrix for all verts
```

Snout join `dl=0xF2` is the same mixed-slot class (child 0–5, parent 27–31).
Mixed join batching is therefore **not unique** to feathers.

## Phase 8 — Type-0x18 restore (`PROVEN` `STATIC_MIPS` + `PROVEN_STATIC` RT64)

Handler `func_800DD998`: emit `G_DL`, then trailing `G_MTX DA380002`.
The DL itself does child `G_VTX`, `G_POPMTX D8380002`, parent `G_VTX`, tris.

`RSP::popMatrix` decrements `modelMatrixStackSize`, sets
`modelViewProjChanged=true`, restores `p09DiagnosticMatrixToken` from the
modelview-parallel stack. It does **not** flush and does **not** clear
`RSP::vertices` / `worldIndices`.

```text
TYPE18_RESTORE_MATRIX_SEMANTICS: G_POPMTX inside the join DL; trailing G_MTX DA380002 is same-depth LOAD|PUSH restore of the popped bone
TYPE18_RESTORE_VERTEX_STATE_SEMANTICS: vertex cache is not cleared; only reloaded slots change worldIndices
TYPE18_RESTORE_FLUSHES_DRAW_CALL: NO
TYPE18_CAN_LEAVE_STALE_VERTEX_TRANSFORM_ASSOCIATION: YES for unreloaded slots; NO for reloaded 0-3 and 28-31
```

## Phase 9 — Snout control

Snout first GeoCmd child is type-0x18 (`dl=0xF2`) so the first `G_VTX` is
immediately after destA. Snout nested call is `0x03000010`. If segment 3 is
Gfx[0], that chunk **includes** `G_VTX n=10 addr=0x01000000` before the tris
that feather `0x03000040` would execute as leftovers.

```text
FIRST_POST_MATRIX_DIFFERENCE_FROM_SNOUT: nested type-2 104/105 chain plus G_DL 0x03000040 skip-into-leftover-TRI versus snout first-child type-0x18 and G_DL 0x03000010
SNOUT_VERTEX_TOPOLOGY_COMPLEXITY: one join mix (0-5 / 27-31) and nested 0x03000010 that loads verts before those tris
FEATHER_VERTEX_TOPOLOGY_COMPLEXITY: three nested bones, three join mixes, own meshes reuse slots 0-N, nested 0x03000040 leftover-TRI candidate
```

## Phase 10 — Candidate ranking (evidence, not the video)

The half-frame distal distortion is **not** used as proof. Ranking is
structural:

1. Nested `0x03000040` leftover / shared-geo tris under the wrong cache
   contents — `HIGH` structural if segment 3 is Gfx[0]; runtime untested.
2. Per-vertex `worldIndex` stale on unreloaded slots — runtime-capable;
   reload path updates correctly.
3. Mixed-transform DrawCall range — `PROVEN` on joins; snout analog exists,
   so weak as the unique cause; V5B already froze the seven matrices.
4. Type-0x18 restore stale association — by design for unreloaded slots;
   join reloads the slots it draws.
5. Independent vertex interpolation — `REFUTED` for displayed position.
6. Banjo explicit matrix groups / SIMPLE — `LOW` for the current root.

## Phase 11–12 — V6 witness (`IMPLEMENTED_STATIC`)

One bounded seam: `RSP::drawIndexedTri` after min/max update, default-off
via `TOOIE_DIAG_SECONDARY_SWAY=1`. Filter: `FeatherWorldHit` map filled from
the existing vertex event (`workload_id` + `transform_index` → bone), with
token fallback only for leftover tris before the first G_VTX of a bone.
CSV `/tmp/tooie-kazooie-feather-v6-tri.csv` (cap 4096). No render change.

Classes:

| CLASS | Meaning |
| --- | --- |
| `MATCH_ACTIVE` | all three `worldIndices` equal `curTransformIndex` |
| `MIXED_JOIN` | mixed worldIndices, expected on type-0x18 joins |
| `STALE_OR_LEFTOVER` | uniform worldIndex ≠ current transform (leftover cache / nested skip) |
| `MIXED_AND_NE_ACTIVE` | mixed and none equals current |

```text
NEXT_WITNESS_TARGET: per visible feather triangle, cache slot + global index + worldIndex vs expected bone
NEXT_WITNESS_CAN_DISTINGUISH:
  A vertex transform assignment wrong before submit (STALE_OR_LEFTOVER on own DLs)
  B assignment correct but batch/range wrong (MATCH_ACTIVE verts inside DRAW_MIN!=DRAW_MAX beyond join)
  C all CPU geometry state correct → later renderer (own DLs MATCH_ACTIVE, joins MIXED_JOIN only)
V6_IMPLEMENTED: YES
V6_DIAGNOSTIC_SEAM: RSP::drawIndexedTri after worldIndices min/max
V6_EXPECTED_TO_CLASSIFY_FIRST_POST_MATRIX_LAYER: YES
```

## BanjoRecomp relevance

```text
BANJORECOMP_MATRIX_GROUP_FIX_RELEVANCE_TO_CURRENT_ROOT: LOW
```

AUTO identity is already correct. SIMPLE root-only and full-chain NO_INTERP
were both visually `SAME`. Do not port groups merely because Banjo has them.
