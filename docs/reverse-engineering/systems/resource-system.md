---
title: "Resource system"
description: "How Tooie asset IDs become loaded pointers. Player body is not D674C-first."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-09-02
tags: [reverse-engineering, resources]
---

# Resource system

## Cart table (`PROVEN` `STATIC_ROM`)

- Index: `0x5188 + id*4`
- Payload ROM: `0x12B24 + (entry>>8)*4`
- Skip 2 bytes, raw deflate (`wbits=-15`)
- Flag `0x16` → animation-like; flag `0x10` + magic `0000000B` → model

## Runtime lookup

`func_800D674C`: resource-table key → loaded pointer (`PROVEN`). Used by
backpack (`babackpack_entrypoint_1`). **Not** the player-body getter on
the overlay-118 walk path.

Player draw (`func_80817180`):

```text
jal 801039E4     // a0 = *PlayerState; v0 = model_bin  (NOT D674C)
jal 800DE448     // 5th arg model_bin at 16(sp)
```

`func_801039E4` role: player `model_bin` getter on the overlay-118 path
(`STRONG_INFERENCE`). Independent core path (`PROVEN` generated C):

```text
func_80092444
  jal 80092B04(a0=PlayerState, a1=1)
  jal 800DE448 at 0x800926D8  // 5th arg = 80092B04 v0
```

`func_80092B04`: `id = lh(*(PlayerState+0x50)+0xC)`. If `id==0x607` then
`func_80093F7C`; else `func_800D674C(id)` (`PROVEN`). Resource ID is at
that halfword, **not** in DE448 argument registers.

Appearance → animation: `badata_entrypoint_27` =
`D_80801448[appearance]` (`PROVEN`). Separate from body selection.

Appearance → `BaData_1[appearance].unk2` AssetId: `func_808006AC`
(`PROVEN`, stride 8). Whether that AssetId is the live body for BK walk
is **not** sealed (`UNKNOWN` / do not promote).

Overlay 118 packed `u16` at `+0xC160` = `0x61F` among other transform
bodies (`PROVEN` bytes). **Consumer of that table:** `UNKNOWN`.

Kazooie body store (`PROVEN` `STATIC_MIPS`): `8080BFAC` writes `0x61C`
or `0x83A` when `a1==0xB`.

## Modding relevance

**PROVEN ROLE:** ID → deflate blob; D674C for many attached models.

**POTENTIAL MODDING USE:** swap IDs in tables; replace payload; hook
`func_800DE448` 5th argument for live replacement. Player body swap
should target `801039E4` / overlay-118 draw, not a noisy D674C dump.
