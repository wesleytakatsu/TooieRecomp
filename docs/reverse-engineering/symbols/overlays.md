---
title: "Overlays"
description: "Overlays that currently matter for player/body/secondary animation."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-09-02
tags: [reverse-engineering, overlays]
---

# Overlays

All gameplay overlays share guest VRAM `0x80800000`. A ROM dump of one
overlay is not a dump of every `8080xxxx` symbol.

| Overlay | Name | Role | Status |
| --- | --- | --- | --- |
| 118 | `badata` (ba cluster includes draw at `80817180`) | appearance / anim tables; player DE448 | `PROVEN` name `badata`; draw function in same ROM image `STATIC_MIPS` |
| 113 | `babackpack` | attached backpack model | `PROVEN` |
| 162 | `bawobble` | ROM `0x01F6EDD0`; `unkC==2` gate; `func_800E0B8C` writer | `PROVEN` |

Decompressed ROM overlay 118 at `0x1F5BC80`. `0x61F` appears as packed
`u16` at `+0xC160`. `0x62A` / `0x62B` have **zero** hits in the first
256 KiB of that overlay (`STATIC_ROM`).

See [[../systems/overlays]].
