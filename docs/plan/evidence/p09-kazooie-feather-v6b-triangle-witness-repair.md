---
title: "P09 Kazooie feather V6B triangle-witness repair"
description: "V6 TRI=0 was a diagnostic key/filter failure; V6B starts from per-vertex worldIndices with raw GBI/RSP counters."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-04
tags: [tooie-recomp, p09, kazooie, feather, rt64, vertex, triangle]
---

# P09 Kazooie feather V6B triangle-witness repair

CPU/static plus diagnostic repair only. The game was not run in this
checkpoint. No GPU readback, synchronization, copy, staging, or query. No
production interpolation or geometry change.

```text
PROVENANCE: USER_OBSERVED V6 CAPTURE + RT64 SOURCE + STATIC_ROM cart 0x62B
GPU_READBACK_USED: NO
PRODUCTION_FIX: NO
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V6B
TEMPORAL_MATRIX_INTERPOLATION_CAUSAL_STATUS: REFUTED_FOR_VISIBLE_FEATHER_CHAIN
SIMPLE_INTERPOLATION_CAUSAL_STATUS: REFUTED_AS_PRIMARY_FIX
FULL_VISIBLE_CHAIN_AUTO_IDENTITY_STATUS: CORRECT
VISIBLE_REQUIRED_MATCH_DROP_COUNT: 0
```

## Authoritative V6 runtime failure (`PROVEN` user capture)

Schema `P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V6` post-dated the executable.
`/tmp/tooie-kazooie-feather-v6-tri.csv` did not exist.
`TRACKED_BONES_WITH_TRI_MASK=0x00`. `FEATHER_TRI_TOTAL=0` for bones
`79, 100–105`.

V4/V5B already had GMTX / VERTEX / POSTMATCH masks `0x7F`. Do not
interpret TRI=0 as “feather geometry was not submitted”.

## Phase 1 — actual F3DEX2 triangle dispatch (`PROVEN_STATIC`)

```text
F3DEX2_TRI1_DISPATCH: opcode 0x05 → GBI_F3DEX2::tri1
  → RSP::drawIndexedTri(v0,v1,v2, false, "TRI1")
F3DEX2_TRI2_DISPATCH: opcode 0x06 → GBI_F3DEX2::tri2
  → two drawIndexedTri(..., false, "TRI2_0" / "TRI2_1")
ACTUAL_TRIANGLE_SUBMIT_FUNCTION: RSP::drawIndexedTri(uint32_t,uint32_t,uint32_t,bool,const char*)
V6_HOOKED_FUNCTION: same 5-argument overload after worldIndices min/max
V6_HOOK_IS_ON_ACTUAL_RUNTIME_PATH: YES
```

The 3-argument overload only forwards to the 5-argument one. There is no
separate `drawIndexedTris` batch path. `cullBothMask` can return before
the V6 hook; V6B counts handler entry before that return.

## Phase 2 — V6 counters were post-filter (`PROVEN_STATIC`)

```text
V6_RAW_TRIANGLE_ENTRY_COUNTER_EXISTS: NO
FEATHER_TRI_TOTAL_IS_PRE_FILTER: NO
FIRST_V6_FILTER_BEFORE_TRI_COUNTER: g_secondary_sway_enabled, then
  FeatherWorldHit(match/matrix workload, worldIndex); if all three miss
  and token==0, return. FEATHER_TRI_ROWS increments only after that.
```

TRI=0 does **not** prove `drawIndexedTri` never ran.

## Phase 3 — FeatherWorldHit key (`PROVEN_STATIC`)

```text
FEATHER_WORLD_HIT_INSERT_SEAM: secondary_sway_rt64_vertex_event
  → remember_feather_world_hit after setVertexCommon creates a world transform
FEATHER_WORLD_HIT_KEY: (match_workload_id else matrix_workload_id,
  rt64_transform_index)
TRI_LOOKUP_KEY: (workload.workloadId at drawIndexedTri, per-vertex worldIndices)
KEYS_STRUCTURALLY_IDENTICAL: NO
WORKLOAD_CAN_CHANGE_BEFORE_TRIANGLE: YES
TRANSFORM_INDEX_CAN_CHANGE_BEFORE_TRIANGLE: YES (leftover cache / pop)
WORLDINDEX_IS_BETTER_LOOKUP_ID_THAN_ACTIVE_CURTRANSFORM: YES
```

Vertex event **preserves** `matrix_workload_id` into `match_workload_id`
and ignores a different G_VTX `workload.workloadId`. Triangle submit uses
the current draw workload. `p09DiagnosticMatrixToken` is set to 0 after
the first world-transform G_VTX, so the V6 token fallback is dead for
every triangle after that vertex load.

## Phase 4 — active-transform pre-filter (`PROVEN_STATIC`)

```text
V6_REQUIRES_ACTIVE_TRANSFORM_HIT_BEFORE_VERTEX_INSPECTION: NO
V6_DESIGN_BUG: WORLDHIT_KEYED_BY_MATRIX_WORKLOAD_NOT_DRAW_WORKLOAD
  AND_TOKEN_CLEARED_BEFORE_TRIANGLE
```

V6 did inspect per-vertex `worldIndices` first. The lookup still could
not see leftover or same-transform triangles when the stored workload
disagreed, and token=0 then discarded the row.

## Phase 5–7 — corrected order and DL context

```text
CORRECTED_TRIANGLE_CLASSIFICATION_ORDER:
  G_TRI1/G_TRI2
  → drawIndexedTri entry (raw counters)
  → per-vertex worldIndices
  → worldIndex→bone (vertex workload, then matrix workload, then any)
  → active curTransformIndex as additional inclusion
  → token / tracked DL as last inclusion
  → CLASS
CURRENT_DL_CONTEXT_SOURCE: tooie_ws_get_current_dl_context
  (Interpreter G_DL 0xDE push / G_ENDDL 0xDF pop)
CURRENT_DL_CONTEXT_RUNTIME_RELIABLE: YES as resolved RDRAM address,
  not as GeoCmd dl index 0x5AA. V6B derives gfx_index from
  model_bin+0x2670 when a hit supplies model_bin.
```

## Phase 8 — `0x03000040` (`PROVEN` command order, segment base not proven)

Cart `0x62B` DLs `0x5AA` / `0x57C` / `0x534` / `0x592` each execute
`G_DL dest=0x03000040` **before** that DL's own `G_VTX`.

```text
SEGMENT3_BASE_STATUS: RUNTIME_REQUIRED
DL_03000040_STATIC_TARGET: segmented 0x03000040; Gfx+0x40 only IF
  segment 3 == Gfx[0] (model+0x2670)
DL_03000040_FIRST_COMMANDS: IF that mapping, G_TRI2 then G_TRI2 then
  G_TRI1 then texture setup, then G_VTX 0x010000A0
LEFTOVER_TRI_BEFORE_VTX_STATIC_STATUS: POSSIBLE
```

`segment 3 == Gfx[0]` is **not** assumed: root `G_DL 0x03000010` at
`Gfx[5]` would then call `Gfx+0x10` and re-enter that `G_DL`.

## Phase 9–10 — V6B

Default-off. Schema `P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V6B`. CSV
`/tmp/tooie-kazooie-feather-v6b-tri.csv`. Status exposes:

```text
RAW_TRI_HANDLER_COUNT
RAW_TRI1_COUNT
RAW_TRI2_COUNT
TRI_WITH_ANY_TRACKED_WORLDINDEX_COUNT
TRI_WITH_TRACKED_ACTIVE_COUNT
TRI_WITH_TRACKED_DL_COUNT
TRI_CLASSIFIED_COUNT
TRACKED_BONES_WITH_TRI_MASK
```

Classification still uses `MATCH_ACTIVE` / `MIXED_JOIN` /
`STALE_OR_LEFTOVER` / `MIXED_AND_NE_ACTIVE`.

```text
V6B_IMPLEMENTED: YES
NEW_SCHEMA: P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V6B
CAPTURE_CAN_LOCALIZE_ZERO_ROW_FAILURE: YES
```

Read-out:

- A: `RAW_TRI1/TRI2=0` and `RAW_TRI_HANDLER_COUNT=0` → GBI/RSP not reached
- B: handler > 0, `TRI_WITH_ANY_TRACKED_WORLDINDEX_COUNT=0` → no mapped
  worldIndex
- C: tracked worldIndex > 0, `TRI_WITH_TRACKED_DL_COUNT=0` → DL context
  not in known gfx indices
- D: `TRI_CLASSIFIED_COUNT>0` and CSV rows exist
