---
title: "Animations"
description: "Animation resource identities that have been typed."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-09-08
tags: [reverse-engineering, animation]
---

# Animations

| ID | Owner | Role | Status |
| --- | --- | --- | --- |
| `0x56` | Banjo/BK | walk | `PROVEN` |
| `0xEC` | Mumbo | move | `PROVEN` |
| `0x13A` | Kazooie | paddle (`ASSET_13A_ANIM_BSKAZPADDLE_UNKNOWN`) | `PROVEN` |
| `0x15` (21) | combined 0x62B walk | 60-frame loop; `|v_xz|` rate | `PROVEN_FOR_CAPTURE` |
| `0x16` (22) | combined 0x62B walk-start | 30-frame once; bone107 scale 0.31→1 | `PROVEN_FOR_CAPTURE` |
| `0xC` (12) | combined 0x62B locomotion loop | 120-frame loop | `PROVEN_FOR_CAPTURE` |
| `0x6F` (111) | combined 0x62B idle | 110-frame once, 5.5 s | `PROVEN_FOR_CAPTURE` |

Selection: `badata_entrypoint_27` → `D_80801448[func_80800000_badata()]`.
Default BK land walk appearance index `0x6` → `0x56`. `PROVEN`.

See [[../systems/animation]] and [[../corrections]].
