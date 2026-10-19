---
title: "Overlay systems"
description: "Shared 0x80800000 VRAM; overlay 118 player draw, overlay 113 backpack, overlay 162 bawobble."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-09-02
tags: [reverse-engineering, overlays]
---

# Overlay systems

All gameplay overlays map to guest VRAM `0x80800000`. A dump of overlay
118 is not a dump of overlay 113 even if both use `8080xxxx`.

| Overlay | Name | Role | Status |
| --- | --- | --- | --- |
| 118 | `badata` (+ player draw in same ROM image) | appearance, anim table, extra DE448 | `PROVEN` |
| 113 | `babackpack` | attached backpack | `PROVEN` |
| 162 | `bawobble` | secondary hip additive (`unkC==2`) | `PROVEN` (conditional) |
| core | `func_800F5008` / `func_80092444` | `D_80135490` player-table DE448 | `PROVEN` |

Decompress ROM overlay 118 at `0x1F5BC80` (`build/bt.n64.us.1.0.z64`).
The **cart** overlay 118 copy is compressed garbage — never dump it.

`0x61F` in first 256 KiB: `+0xC160` (packed transform list) and
`+0x26CD0` (sequential ID list — **ignore**). `0x62A`/`0x62B`: zero hits.

See [[../symbols/overlays]].
