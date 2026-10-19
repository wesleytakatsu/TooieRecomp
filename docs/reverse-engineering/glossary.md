---
title: "RE glossary"
description: "Status labels, provenance, and proposed semantic aliases."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-09-09
tags: [reverse-engineering, glossary]
---

# RE glossary

## Status

- `PROVEN`
- `STRONG_INFERENCE`
- `HYPOTHESIS`
- `UNKNOWN`
- `REFUTED`
- `CORRECTED`

## Provenance

- `STATIC_ROM` — decompressed/cart asset bytes
- `STATIC_MIPS` — overlay or core instruction listing
- `RUNTIME_CPU` — default-off CPU witness
- `USER_OBSERVED` — visual/gameplay report
- `BANJORECOMP_STRUCTURAL_ANALOG` — Banjo-Kazooie recomp/decomp shape
- `OTHER`

## Proposed aliases

These names are documentation-only (`PROPOSED_ALIAS`). Do **not** rename
production / decomp / generated symbols in this round.

| Current symbol | Proposed alias | Status |
| --- | --- | --- |
| `func_800E0B8C` | `model_bone_apply_additive_euler` | `PROPOSED_ALIAS` |
| `func_800E0E94` | `model_bone_consume_additive_euler` | `PROPOSED_ALIAS` |
| `func_800AF04C` | `model_bone_hierarchy_compose` | `PROPOSED_ALIAS` |
| `func_800DE448` | `model_draw_submit` | `PROPOSED_ALIAS` |
| `func_800DE498` | `model_draw_geolist` | `PROPOSED_ALIAS` |
| `func_800DE2A4` | `geocmd_sibling_walker` | `PROPOSED_ALIAS` |
| `func_800DCF48` | `geocmd02_compact_bone_push` | `PROPOSED_ALIAS` |
| `func_800ADE2C` | `bone_index_current_matrix` | `PROPOSED_ALIAS` |
| `func_800E2AA4` | `geocmd02_fat_dual_mtx_push` | `PROPOSED_ALIAS` |
| `func_800E2180` | `geocmd_emit_dual_mtx` | `PROPOSED_ALIAS` |
| `bawobble_entrypoint_8` | `secondary_walk_sway_update` | `PROPOSED_ALIAS` |
| `func_808001C0_bawobble` | `secondary_walk_sway_apply` | `PROPOSED_ALIAS` |
| `func_80800000_badata` | `player_appearance_index` | `PROPOSED_ALIAS` |
| `badata_entrypoint_27` | `player_appearance_animation_id` | `PROPOSED_ALIAS` |

## Terms

- **boneAnimID** — `u16` at bone record `+0xC`. This is what
  `func_800E0B8C` `part` matches. `PROVEN`.
- **bone INDEX** — position in the model's bone table, not the anim ID.
- **GeoCmd tag** (`0x0A`) — attach/refpoint. **Not** an RT64 matrix
  group and **not** identity-equal to a boneAnimID. `part 3 == tag 5`
  is `REFUTED`.
- **ExtraSegments** — Tooie model `gtype` bit `0x40` at header `+0xA`.
  CPU-skinned vertices + GeoCmd `0x03` DLs; **no** GeoCmd `0x02` bone
  push on `0x61F`. `PROVEN` for those headers. Tooie C symbol name:
  `UNKNOWN`.
- **GeoCmd compact type 2** — 16-byte bone header on live `0x62B`
  (`byte(+8)==0x10`, INDEX at `+9`, boneAnimID at `+0xA`). Handler
  `func_800DCF48`. Snout `+0x11CB8` is a **sibling** in the neck child
  list, not a `func_800DE2A4` list head. **Not** the fat dual-mtx
  encoding consumed by `func_800E2AA4`. **Not** BK GeoCmd2 layout
  (`unk8` is header size here, not a child byte offset).
- **BK `gEXMatrixGroup` tagging** — BanjoRecomp PR #22/#23 explicit
  matrix-group IDs (`cur_drawn_model_transform_id + offset`) around
  GeoCmd2/3/1. **ABSENT** in Tooie runtime. P09 visible-feather cause
  `ALREADY_REFUTED_BY_P09`. Not an RT64 AUTO identity failure.
- **Feather DLs on `0x62B`** — rigid-bone F3DEX2 `G_VTX` under DCF48
  `G_MTX` (types `0x11`/`0x18`). **Not** ExtraSegments CPU-skinned
  vertices (`unk28` / PR #26 class).
- **generated LOOKUP_FUNC callsite** — emitted as
  `LOOKUP_FUNC(vram)(rdram, ctx)` with **no** `ctx->r31` write.
  `ctx->r31` is **not** a generic host-callsite identity (`PROVEN`).
