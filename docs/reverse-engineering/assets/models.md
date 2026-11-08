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

## Startup branding (`PROVEN`, `STATIC_ROM`)

The canonical USA asset table and decoded embedded textures identify:

| Asset | Role | Decoded model bytes | Embedded textures |
| --- | --- | ---: | --- |
| `0x632` | Rareware logo | 25,504 | 7 CI4 textures, each 64×64 |
| `0x8A8` | Banjo-Tooie title | 118,664 | 28 RGBA32 textures, each 32×32 |
| `0x8AC` | Dolby logo | 88,740 | 21 RGBA32 textures, each 32×32 |

Rulesless v0.9 modifies embedded textures in all three models. Decoding its `0x632` textures shows the contributor
names visible on the user's replacement plaque; its `0x8A8` changes only part of the title's tiles, adding Italian
branding. These are data replacements, not a font rendering defect. Original embedded bytes must be restored together
to retain USA texture identity; re-encoding a modified model into a different compressed stream does not restore
those pixels. The Italian bundle therefore omits all three model overrides, preserving their original USA payloads
through asset-tail relocation. Host extraction parity establishes byte identity; final HD appearance requires game testing.

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
