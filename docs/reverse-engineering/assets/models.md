---
title: "Models"
description: "Geometry resources. ExtraSegments vs GeoCmd 0x02 is the live-path fork."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-09-02
tags: [reverse-engineering, models]
---

# Models

Header (`PROVEN` `STATIC_ROM`):

- `+0x00` magic `0000000B`
- `+0x04` GeoCmd list offset (`PROVEN` DE498)
- `+0x0A` `gtype` (`0x40` = ExtraSegments)
- `+0x18` offset to anim/bone setup
- setup `+4` `u16` bone count

| ID | gtype | bones | GeoCmd 0x02 | Role | Live walk? |
| --- | --- | --- | --- | --- | --- |
| `0x61F` | `0x40` | 47 | none | BK ExtraSegments body | `STRONG_INFERENCE` |
| `0x62A` | 0 | 68 | yes (snout idx 10) | high-detail Banjo | `WEAKENED` (absent ovl118) |
| `0x62B` | 0 | 66 | yes @ `0x11CB8` snout idx 11 anim 42; Kazooie-on-back type-2 head `0x12330` anim 108, plumes `0x124A0` idx 30 anim 103 | Banjo+Kazooie GeoCmd body; live walk | `PROVEN_FOR_CAPTURE` |
| `0x621` | 0 | 14 | unknown here | backpack | gameplay backpack `PROVEN` |
| `0x61C` | 0 | 58 | yes (crest idx 11 anim 69 @ `0xF558`) | solo Kazooie | `TRANSFORM_B` store `8080C014` |
| `0x83A` | 0 | 47 | yes (crest idx 8 anim 69 @ `0xD408`) | reduced Kazooie | `8080BFAC` alternate |
| `0x665` | 0 | 41 | yes (feathers) | Mumbo Skull | n/a |
| `0x854` | `0x40` | 41 | none | Mumbo ExtraSegments | n/a |

If live Banjo is `0x61F`, snout does **not** use `func_800DCF48` /
`func_800E2AA4` / `func_800E2180`. Live capture `0x62B` uses compact
type-2 `func_800DCF48`, not E2AA4.

## Selection (`STATIC_MIPS` / overlay bytes)

| Resource | Selector | Condition | Appearance | State | Expected BK land walk |
| --- | --- | --- | --- | --- | --- |
| `0x61F` | overlay 118 packed `u16` `+0xC160`; consumer `UNKNOWN` | BK land / ExtraSegments | `0x6` (`TRANSFORM_1_BK`) | walk clip `0x56` | **YES** (`MEDIUM`) |
| `0x62A` | GeoCmd high-detail Banjo | not in ovl118 first 256 KiB | unknown here | unknown | **NO** (`MEDIUM` absence) |
| `0x62B` | GeoCmd Banjo variant | same | unknown | unknown | **NO** (`MEDIUM` absence) |
| `0x61C` | `8080BFAC` `sh 0x61C` when `a1==0xB` and `8009EA2C==0` | `TRANSFORM_B_KAZOOIE` | `0x10` | clip `0x13A` | **NO** (Kazooie form) |
| `0x83A` | same function, `8009EA2C!=0` | reduced Kazooie | `0x10` | same | **NO** |

`STATIC_BK_WALK_RESOURCE = 0x61F` (`MEDIUM`). Table consumer still
untraced. Do not raise to `HIGH` from overlay bytes alone.
