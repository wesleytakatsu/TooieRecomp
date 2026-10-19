---
title: "P09 live GeoCmd type-2 dispatch vs E2AA4 negative reachability"
description: "E2AA4 raw 0 with live 0x62B. Compact type-2 is DCF48 single G_MTX. Fat dual-mtx E2AA4 demoted."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-02
tags: [tooie-recomp, p09, geocmd, e2aa4, dcf48, snout]
---

# P09 live GeoCmd type-2 dispatch vs E2AA4 negative reachability

CPU/static + existing capture only. No GPU readback. No production fix,
commit, or push.

Authoritative capture: `/tmp/tooie-secondary-sway-status.txt`

```text
DE448_TOTAL_SEEN: 29414
DE448_IN_PLAYER_SCOPE_HITS: 2178
DURABLE_0X62B_REGISTER_HITS: 4106
LIVE_RESOURCE: 0x62B
LAST_MODEL_BIN: 0x802249F0
E2AA4_RAW_ENTRY_COUNT: 0
E2AA4_WITH_REGISTERED_0X62B_COUNT: 0
E2AA4_SNOUT_ACCEPT_COUNT: 0
```

Failure is **handler reachability of `func_800E2AA4`**, not 0x62B
registration / snout offset / type / INDEX / boneAnimID / model span.

## Phase 1 — handler table

Walker `func_800DE2A4` (`PROVEN` generated C `funcs_400.c`):

- type = `lw(node+0)` (full word; table index)
- handler = `lw(*(D_8012CF8C) + type*4)` then `LOOKUP_FUNC` (`jalr`)
- sibling = `node + lw(node+4)`; continue `0` ends that list
- children: **not** walked by DE2A4

`D_8012CF8C` (`0x8012CF8C`) is loaded from `D_80123198[mode]` by
`func_800DF4B0(a0=mode)` and by init `func` at `0x800DE3E8` which copies
`D_80123198[0]`.

ROM `.data` pointers for the three tables were **not** isolated this
round (cart image contains `0x800DCF48` as code, not proven as the
table slot). Live type-2 pointer is the census field
`DE2A4_LAST_TYPE2_HANDLER` (new diagnostic; not yet captured).

```text
LIVE_GEOCMD_HANDLER_TABLE: D_8012CF8C -> D_80123198[mode]
TABLE_ADDRESS: 0x80123198 (pointer array); selected table at *D_8012CF8C
TYPE2_TABLE_INDEX: 2
TYPE2_STATIC_HANDLER: UNKNOWN (slot not dumped); encoding uniquely matches func_800DCF48
TYPE2_RUNTIME_HANDLER: func_800DCF48 expected (STRONG_INFERENCE); confirm DE2A4_LAST_TYPE2_HANDLER==0x800DCF48
TYPE2_HANDLER_IS_E2AA4: NO (live 0x62B; E2AA4_RAW=0)
```

Among 37 generated `LOOKUP_FUNC(0x800DE2A4)` sites, **only**
`func_800DCF48` uses `lbu(node+8)` as the child stride. Other recursor
sites use `lh(+8)`, which on `0x62B` type-2 would be `0x1000` and miss
the real children at `+0x10`.

## Phase 2 — multiple tables / modes

```text
MULTIPLE_GEOCMD_HANDLER_TABLES: YES
TYPE2_HANDLER_DEPENDS_ON_MODEL_FLAGS: NO (not proven; type word is the index)
TYPE2_HANDLER_DEPENDS_ON_RUNTIME_MODE: YES (DF4B0 selects table 0/1/2)
TYPE2_HANDLER_SELECTION_RULE: table[lw(node+0)] after DF4B0/init select
0X62B_ACTIVE_HANDLER_TABLE: D_80123198[0] at model init (0x800DE3E8); other
  callers set mode 1 (func_800BE7F8) or 2 (gcmapsects / overlay). Player
  DE448 path is expected to remain on the model-init table until a census
  proves otherwise.
```

## Phase 3 — walker vs `0x11CB8` bytes

Bytes `00000002 00000050 100b002a 00000000`:

```text
WALKER_COMMAND_TYPE_READ: lw(node+0) full word
WALKER_COMMAND_SIZE_READ: not a size; lw(node+4) is sibling continue
WALKER_CHILD_POINTER_OR_STRIDE: compact handler uses node+lbu(node+8); 0x10
WALKER_TYPE_VALUE_FOR_0X11CB8: 2
WALKER_WOULD_DISPATCH_0X11CB8_AS_TYPE2: YES
```

`word(+4)==0x50` is sibling continue covering children (next sibling at
`0x11D08` type `0x0C`), **not** a fat dual-mtx struct size.

## Phase 4 — static reachability

Compact recurse from GeoCmd root `0x119F0`:

```text
root 0x119F0 type2 idx0 anim43
 -> hip 0x11A00 type2 idx1 anim3
 -> torso 0x11C30 type2 idx8 anim1
 -> neck 0x11C40 type2 idx9 anim18
 -> snout 0x11CB8 type2 idx11 anim42
```

No LOD word on the compact header. DCF48 still walks children if the
anim-mtx list pointer is null; it only skips the G_MTX push.

```text
X62B_SNOUT_NODE_REACHABILITY_STATIC: ALWAYS (if compact type-2 handler recurses)
SNOUT_PARENT_BRANCH_CONDITION: none on type-2 continue; parent chain as above
SNOUT_LOD_OR_VISIBILITY_GATE: none in compact header; mtx push gated by D_8012C968
NORMAL_BK_WALK_EXPECTS_SNOUT_NODE_EXECUTION: YES
```

## Phase 5 — visible geometry vs bone-42 node

Snout children (at `+0x10`):

- `0x11CC8` type `0x18` continue `0x30`
- `0x11CF8` type `0x11` continue `0` (DL-class node)

Type `0x0C` at `0x11D08` is the **next sibling** of the bone-42 node
(neck list), not a snout child.

```text
VISIBLE_SNOUT_DL: GeoCmd type 0x11 node @ 0x11CF8 (plus type 0x18 @ 0x11CC8)
VISIBLE_SNOUT_DL_OWNER_NODE: 0x11CF8 / 0x11CC8
VISIBLE_SNOUT_TRANSFORM_OWNER_NODE: 0x11CB8 (compact type 2 bone INDEX 11)
VISIBLE_SNOUT_BONE_ANIM_ID: 42
VISIBLE_SNOUT_RUNTIME_HANDLER: transform=func_800DCF48; draw=type 0x11/0x18 handlers (table[0x11]/table[0x18], not dumped)
VISIBLE_SNOUT_DRAW_PATH: DE448 -> DE498 -> DE2A4 -> DCF48(type2 chain) -> DE2A4(children) -> type 0x18/0x11
SNOUT_GEOMETRY_NODE_EQUALS_BONE42_NODE: NO (PARTIAL if counting the parent bone as owner of child DLs)
```

## Phase 6 — E2180 callers

Static `jal 0x800E2180` sites:

| CALLER | GEOCMD TYPE/CLASS | NODE STRUCTURE | MATRIX SEMANTICS | CAN_DRAW_BONE42/SNOUT |
| --- | --- | --- | --- | --- |
| `func_800E2AA4` | fat type-2 candidate | fat dual-mtx | prev+curr E2180 | NO on live 0x62B (raw 0) |
| `func_800E2A3C` | other GeoCmd class | uses E1DF4/E204C then E2180 | dual-mtx | not compact 0x62B type-2 |
| `func_800E2C54` | other GeoCmd class | E2180 | dual-mtx | same |
| `func_800E2E48` | other GeoCmd class | E2180 | dual-mtx | same |

```text
MOST_LIKELY_LIVE_SNOUT_E2180_CALLER: NONE (compact path does not call E2180)
CONFIDENCE: HIGH
```

Prior E2AA4 mapping was wrong because it treated type word `2` as a
unique global handler. Type `2` is a **table index**; compact `0x62B`
nodes are 0x10-byte headers that require `lbu(+8)` recursion, which
E2AA4 never does.

## Phase 7 — E2AA4 map audit

```text
E2AA4_GUEST_ADDRESS: 0x800E2AA4
E2AA4_GENERATED_SYMBOL: func_800E2AA4
E2AA4_ADD_LOADED_FUNCTION_ADDRESS: 0x800E2AA4 -> __wrap_func_800E2AA4
E2AA4_CURRENT_RUNTIME_MAPPING: LOOKUP_FUNC(jalr table) and LOOKUP_FUNC(0x800E2AA4) from fat callers
E2AA4_RUNTIME_MAPPING_CORRECT: YES (wrap would have counted RAW; RAW=0 means not entered)
IS_E2AA4_ACTUALLY_CALLED_BY_LOOKUP_FUNC_AT_RUNTIME: NO (this capture)
E2AA4_CALL_MECHANISM: JALR_TABLE (walker) + LOOKUP_FUNC (direct jals from fat path)
```

Walker cannot call the generated `func_800E2AA4` except through
`LOOKUP_FUNC` of the table word. Direct generated call of the walker
itself is also LOOKUP-wrapped.

## Phase 8 — actual handler for `0x62B+0x11CB8`

```text
COMMAND_DECODER: func_800DE2A4 lw(node+0)
HANDLER_TABLE: *D_8012CF8C (D_80123198[mode])
HANDLER_INDEX: 2
HANDLER_ADDRESS: expected 0x800DCF48
HANDLER_FUNCTION: func_800DCF48
CHILD/DRAW FUNCTION: DE2A4(node+0x10) then type 0x18 / 0x11 handlers
MATRIX FUNCTION: func_800ADE2C(bone INDEX) then G_MTX PUSH DA380002
X62B_SNOUT_ACTUAL_HANDLER: func_800DCF48 (STRONG_INFERENCE; census pending)
X62B_SNOUT_ACTUAL_MATRIX_FUNCTION: func_800ADE2C
X62B_SNOUT_ACTUAL_DRAW_FUNCTION: type 0x11/0x18 table handlers (UNKNOWN symbols)
X62B_SNOUT_USES_E2180: NO
X62B_SNOUT_USES_DUAL_MTX: NO
X62B_SNOUT_USES_E2AA4: NO
```

Status: `STATIC_PATH_CANDIDATE` for E2AA4. Compact DCF48 is
`STRONG_INFERENCE` until `DE2A4_LAST_TYPE2_HANDLER` prints `0x800DCF48`.

## Phase 9 — tagging root

```text
BANJO_SNOUT_ACTUAL_MATRIX_CLASS: SINGLE_G_MTX (ADE2C / DA38)
BANJO_SNOUT_MISSING_GROUP_TAG: NOT_APPLICABLE
INTERPOLATION_TAGGING_ROOT_PLAUSIBILITY: LOW
ROOT_CONFIDENCE: LOW
CURRENT_MOST_LIKELY_LAYER: BONE/HIERARCHY TRANSFORM (DCF48/ADE2C)
```

## Phase 10 — other parts

```text
BANJO_BACKPACK_DUAL_MTX_STATUS: OPEN (independent babackpack / 0x621 witness; not proven via snout E2AA4)
MUMBO_FEATHERS_DUAL_MTX_STATUS: UNKNOWN (do not inherit Banjo snout handler)
KAZOOIE_FEATHERS_DUAL_MTX_STATUS: UNKNOWN (do not inherit Banjo snout handler)
SHARED_RENDER_ROOT_STATUS: REFUTED as shared live E2AA4/E2180; wobble hip-3 may still be shared upstream
```

## Phase 11 — next witness (do not run this round)

Default-off diagnostic already moved:

- census: wrapped `func_800DE2A4` records type and table handler
- filter: wrapped `func_800DCF48` `a1 == registered 0x62B bin + 0x11CB8`
  type 2 INDEX 11 boneAnimID 42
- sample: nested `func_800ADE2C` current matrix after `__real`

```text
NEXT_RUNTIME_WITNESS_SEAM: func_800DCF48 + nested func_800ADE2C
NEXT_SNOUT_FILTER: registered 0x62B + node rel 0x11CB8 + type 2 + idx 11 + bone 42
NEXT_FILTER_CONFIDENCE: HIGH
WITNESS_CAN_DISTINGUISH_GUEST_VS_INTERPOLATION: PARTIAL (guest current mtx only; no prev pair)
```

Confirm first: `DE2A4_LAST_TYPE2_HANDLER == 0x800DCF48` and
`DCF48_SNOUT_ACCEPT_COUNT > 0`.

## Phase 12

```text
E2AA4_PRIMARY_SNOUT_WITNESS: DEMOTED
```

Keep E2AA4 wrap as a negative-control counter.

`GPU_READBACK_USED: NO`
