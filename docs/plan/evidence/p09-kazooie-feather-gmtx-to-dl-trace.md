---
title: "P09 Kazooie feather G_MTX to visible DL and RT64 vertex seam"
description: "Static reconstruction of 0x62B+0x124A0 through type-0x11/0x18 DLs; V3 zero VERTEX rows are a modelview token-stack defect."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-04
tags: [tooie-recomp, p09, kazooie, feathers, geocmd, gmtx, rt64, vertex]
---

# P09 Kazooie feather G_MTX to visible DL and RT64 vertex seam

CPU/static only. No game run. No GPU readback. No production fix.

Asset: cart `0x62B` skip-2 raw deflate, decompressed 78200 (`0x13178`),
already extracted at `/tmp/p09-kazooie-carry-banjo-feather-20260903T125000Z/asset_062B.bin`.

```text
PROVENANCE: STATIC_ROM + STATIC_MIPS (generated C) + RT64 source
GPU_READBACK_USED: NO
PRODUCTION_FIX: NO
```

## Phase 1 — GeoCmd subtree rooted at `+0x124A0`

Compact type-2 (`hdr=0x10`). Handler `func_800DCF48` (`PROVEN` class;
snout runtime `PROVEN`; feather same table `STRONG_INFERENCE`).
Children at `node+0x10`. Sibling continue `lw(+4)`.

Type `0x11` handler `func_800DD65C`. Type `0x18` handler `func_800DD998`.

**CORRECTED:** the previously cited “type-0x11 child of +0x124A0”
(`dl=0x534`) is **not** a direct child of bone 103. It is the child of
bone 105 (`+0x124D0`). Bone 103's own type-0x11 is `+0x12580` `dl=0x5AA`.

```text
PRIMARY_FEATHER_SUBTREE:
  +0x124A0 type=0x02 handler=func_800DCF48 idx=30 anim=103 next=0xF0
    child +0x124B0 type=0x02 DCF48 idx=31 anim=104 next=0xA0
      child +0x124C0 type=0x02 DCF48 idx=32 anim=105 next=0x50
        child +0x124D0 type=0x11 DD65C dl=0x534 tri=0x351 next=0x10
        sibling +0x124E0 type=0x18 DD998 dl0=0x54C next=0
      sibling +0x12510 type=0x18 DD998 dl0=0x564 next=0x30
      sibling +0x12540 type=0x11 DD65C dl=0x57C tri=0x36D next=0
    sibling +0x12550 type=0x18 DD998 dl0=0x592 next=0x30
    sibling +0x12580 type=0x11 DD65C dl=0x5AA tri=0x37D next=0
  sibling +0x12590 type=0x02 idx=33 anim=100  (left plume; not this token)

PRIMARY_FEATHER_TRANSFORM_NODE: 0x124A0
PRIMARY_FEATHER_VISIBLE_GEOCMD: 0x12580 (type 0x11 own mesh) and 0x12550 (type 0x18 join);
  tip 0x124D0 and mid 0x12540 inherit 103 as ancestor
PRIMARY_FEATHER_VISIBLE_DL_INDEX: 0x5AA (own type-0x11); join 0x592; mid 0x57C; tip 0x534
PRIMARY_FEATHER_VISIBLE_DL_ADDRESS_FORMULA:
  runtime G_DL = osVirtualToPhysical(*(D_8012C960) + (dl_index<<3) + 8)
  static Gfx = model_bin + 0x2670 + dl_index*8
  (DL header at model+0x2668 count=0xB48; Gfx[0] at +0x2670)
PRIMARY_FEATHER_VISIBLE_DL_CONFIDENCE: HIGH
```

## Phase 2 — Display lists

Type `0x11` emits `G_DL` `0xDE00` (call) to that Gfx, not vertices at
CPU time. Type `0x18` emits `G_DL` **then** `G_MTX DA380002` restore.

All four feather DLs are F3DEX2 `G_VTX=0x01` then `G_TRI2`/`G_TRI1`.
Each type-0x11/0x18 DL also `gSPDisplayList` to segmented `0x03000040`
(shared setup/geo; nested depth 1). Type-0x18 DLs contain `G_POPMTX`
`D8380002` between two `G_VTX` loads (join verts in child space, then
parent space).

```text
FEATHER_DL_COMMAND_SEQUENCE (type-0x11 dl=0x5AA @ model+0x53C0):
  G_GEOMETRYMODE, G_GEOMETRYMODE, G_TEXTURE, TLUT/tile load,
  G_SETOTHERMODE_H, G_RDPPIPESYNC, G_SETCOMBINE, G_SETOTHERMODE_L,
  G_DL 0x03000040, G_VTX n=8 v0=0 addr=0x010037D0, G_TRI2, G_TRI2,
  restore othermode, G_ENDDL
FIRST_VERTEX_COMMAND_OFFSET: model+0x5440 (dl 0x5AA); tip 0x5090; mid 0x52D0; join 0x5380
FIRST_TRIANGLE_COMMAND_OFFSET: model+0x5448 (dl 0x5AA)
NESTED_DISPLAY_LIST_DEPTH: 1
NESTED_DISPLAY_LIST_TARGETS: 0x03000040 (feather); snout uses 0x03000010
DOES_PRIMARY_DL_CONTAIN_VERTEX_DIRECTLY: YES
```

Snout type-0x11 `dl=0x102` @ `model+0x2E80` is the same class: setup,
nested `G_DL 0x03000010`, `G_VTX`, triangles, `G_ENDDL`.

## Phase 3 — Matrix lifetime

`func_800DCF48`: `G_MTX PUSH|LOAD|MODELVIEW` (`DA380002`), `DE2A4` children,
`G_POPMTX` (`D8380002`).

Bone 103's first child is **another type-2** (104), not geometry. RSP order:

```text
G_MTX destA (103)
  G_MTX destB (104)
    G_MTX destC (105)
      G_DL dl=0x534   # verts under 105
      G_DL dl=0x54C   # VTX, POP 105, VTX parent, tris; then type18 G_MTX restore
    G_POP 105
    G_DL join 104; G_MTX restore 104
    G_DL dl=0x57C
  G_POP 104
  G_DL join 103 (VTX under destA, POP 103, VTX parent); G_MTX destD restore
  G_DL dl=0x5AA       # own mesh under destD
G_POP 103
```

```text
BONE103_GMTX_PUSH_POINT: DCF48 G_MTX DA380002 (destA = gfx mtx ring)
BONE103_GMTX_POP_POINT: DCF48 G_POPMTX D8380002; type-0x18 join also POPMTX inside dl=0x592
VISIBLE_DL_EXECUTES_BETWEEN_PUSH_AND_POP: YES
NESTED_DL_INHERITS_BONE103_MODELVIEW: YES (as current or as ancestor)
INTERVENING_MODELVIEW_MATRIX_COUNT: 2 child G_MTX (104, 105) plus type-0x18 restores
INTERVENING_MODELVIEW_REPLACEMENT: YES
BONE103_TRANSFORM_ACTUALLY_APPLIES_TO_VISIBLE_FEATHER_GEOMETRY: YES
CONFIDENCE: HIGH
```

## Phase 4 — RT64 vertex path

F3DEX2 `G_VTX` (`0x01`) → `RSP::setVertex` → `setVertexCommon<true, sizeof(Vertex)>`.
World transform is created in `setVertexCommon` when `modelViewProjChanged`.
DrawCall is a later State flush / triangle submit.

V3 stored one global `p09DiagnosticMatrixToken`. Every `matrixCommon`
**overwrites** it with `tooie_diag_rt64_matrix`'s return. Child bone 104/105
G_MTX return 0 and clear the token **before any G_VTX**. `setVertexCommon`
only calls `tooie_diag_rt64_vertex` if that global is nonzero, so V3 logged
zero VERTEX rows even though G_VTX exists.

```text
ACTUAL_VERTEX_GBI_OPCODE: 0x01 G_VTX (F3DEX2)
ACTUAL_VERTEX_HANDLER: RSP::setVertex
ACTUAL_RT64_VERTEX_FUNCTION: RSP::setVertexCommon
USES_SET_VERTEX_COMMON: YES
WHY_V3_SET_VERTEX_WITNESS_SAW_ZERO_ROWS: TOKEN_EXPIRES_BEFORE_CHILD_DL
  (global token clobbered by intervening child-bone G_MTX; not a wrong opcode)
```

## Phase 5 — Command order

```text
FEATHER_COMMAND_ORDER: MIXED
VERTEX_BINDING_SEMANTICALLY_REQUIRED: YES
  (RT64 materializes worldTransforms at setVertexCommon, not at G_MTX)
```

Type-0x11: `MATRIX → (nested G_DL) → VERTEX → TRIANGLE`.
Type-0x18: `VERTEX (child) → POPMTX → VERTEX (parent) → TRIANGLE → G_MTX restore`.

## Phase 6 — RT64 transform / call construction

```text
FEATHER_TRANSFORM_CREATED_AT: setVertexCommon when modelViewProjChanged
FEATHER_TRANSFORM_RANGE_CREATED_AT: DrawCall min/max world-matrix as triangles submit
FEATHER_DRAW_CALL_CREATED_AT: State::flush / triangle path (not G_MTX)
FEATHER_WORLD_MATRIX_INDEX_SOURCE: worldTransforms.size() at first G_VTX after matrix change
CORRECT_TOKEN_BINDING_SEAM: setVertexCommon with modelview-stacked token
  (plus type-0x18 same-depth restore G_MTX alias for destD / dl=0x5AA)
```

## Phase 7 — Snout control

Snout `+0x11CB8` first child is type `0x18` (`+0x11CC8` `dl=0xF2`), then
type `0x11` (`+0x11CF8` `dl=0x102`). First G_VTX is inside that type-0x18
DL **immediately** after destA, before any other bone G_MTX. V5 therefore
bound VERTEX. Feather 103's first child is type-2 bone 104.

```text
SNOUT_VISIBLE_DL_COMMAND_ORDER: G_MTX destA → G_DL type18 (G_VTX first) → G_DL type11
FEATHER_VISIBLE_DL_COMMAND_ORDER: G_MTX destA → G_MTX 104 → G_MTX 105 → G_DL geometry
FIRST_STRUCTURAL_DIFFERENCE: 103's first GeoCmd child is nested bone G_MTX, not a DL with G_VTX
SNOUT_REQUIRES_VERTEX_BIND: YES (RT64 transform seam) but no intervening bone G_MTX
FEATHER_REQUIRES_VERTEX_BIND: YES and requires token to survive nested bone G_MTX
```

## Phase 8 — BanjoRecomp / DK64 analog

Banjo `func_803387F8`: bone matrix, `gSPMatrix` PUSH|LOAD|MODELVIEW,
`set_model_matrix_group` (`G_EX_INTERPOLATE_SIMPLE` when boned), child
walk, pop + `gEXPopMatrixGroup`. Group id = model instance base +
deterministic transform offset (geo offset for maps).

Tooie compact DCF48 is the same bone/DL/pop skeleton **without** an
explicit group. Type-0x18 join POP+restore is Tooie-specific compact
skinning.

```text
BANJO_BONE_CHILD_DL_TOPOLOGY_SIMILAR_TO_TOOIE_FEATHER: YES
BANJO_NEEDS_VERTEX_FOR_MATRIX_IDENTITY: NO
EXPLICIT_GROUP_ASSOCIATES_TRANSFORM_BEFORE_VERTEX: YES
GROUP_TAGGING_WOULD_REMOVE_DIAGNOSTIC_VERTEX_DEPENDENCY: YES
```

Not proof that groups fix the visual glitch.

## Phase 9–11 — V4 token lifetime

```text
NEW_TOKEN_LIFETIME_RULE: p09DiagnosticMatrixTokenStack[modelview depth] = matched G_MTX token;
  unmatched child G_MTX stores 0 on the new slot and must not overwrite parent slots;
  G_POPMTX restores the parent slot; type-0x18 same-workload same-depth G_MTX is a one-shot restore alias
NEW_TOKEN_BINDING_SEAM: setVertexCommon (stack top) after destA is current again, and destD restore
NESTED_DL_SAFE: YES
TOKEN_AMBIGUITY_POSSIBLE: NO for a single primary token (restore is one-shot, dest-keyed first)
FEATHER_V4_IMPLEMENTED: YES
V4_EXPECTED_TO_REACH_GAMEFRAME_MATCH: YES
V4_EXPECTED_TO_REACH_MATCHTRANSFORM: YES
CONFIDENCE: HIGH for observability; LOW for graphical root
TOOIE_STABLE_MODEL_INSTANCE_ID_SEAM: DE448 model instance / durable model_bin
TOOIE_DETERMINISTIC_TRANSFORM_OFFSET_SEAM: GeoCmd node rel (0x124A0) or bone index 30
TOOIE_BONE_GROUP_PUSH_SEAM: func_800DCF48 after G_MTX DA380002, before DE2A4
TOOIE_BONE_GROUP_POP_SEAM: func_800DCF48 with G_POPMTX D8380002
CAN_BUILD_BANJO_STYLE_AB_LATER: YES
```

No Banjo-style A/B enabled. No production group tagging.

V4 files (diagnostic only, default-off):

- `src/game/tooie_diag.cpp` / `include/tooie_diag.h` — dest-keyed first
  match, type-0x18 restore alias, `EVENT=RESTORE` / `MATCH_STATUS=ALIAS`
- BanjoRecomp `lib/rt64/src/hle/rt64_rsp.h` /
  `lib/rt64/src/hle/rt64_rsp.cpp` — `p09DiagnosticMatrixTokenStack`
  parallel to modelview depth; unmatched child writes 0 on the new slot;
  `popMatrix` restores the parent slot; `setVertexCommon` still binds

Canonical promotion: [[../../reverse-engineering/systems/geocmd]],
[[../../reverse-engineering/characters/kazooie]],
[[../../reverse-engineering/systems/transform-interpolation]],
[[../../reverse-engineering/corrections]].
