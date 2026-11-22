---
title: "Models"
description: "Geometry resources. ExtraSegments vs GeoCmd 0x02 is the live-path fork."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-11-12
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

## Native creature texture resources (`STATIC_ROM`)

The following identities are supported by the native model payloads and the
[public ROM resource map](https://hack64.net/wiki/doku.php?id=banjo_tooie:rom_map).
This is a resource inventory, not a runtime capture proving which alternate
model every actor chooses in each map.

| Resource IDs | Identity | Evidence classification |
| --- | --- | --- |
| `0x707`, `0x90C`, `0x90D` | Klungo; the latter two are bruised variants | `PROVEN` resource identity; `chklungospiral` descriptor selects `0x707`, `chklungoleave` selects `0x90D` |
| `0x8BE`, `0x901`, `0x8C2` | Skeletal Gruntilda variants and separate eye resource | `PROVEN` resource identity; variant use in each scene remains `UNKNOWN` here |
| `0x8B5`, `0x8C3`, `0x90E`, `0x90F` | Bottles playing cards, burned Bottles, angel Bottles and devil Bottles | `PROVEN` resource identity; devil-form availability during ordinary story play remains `UNKNOWN` here |
| `0x671` | Green Ugger | `PROVEN` resource identity |
| `0x65E`, `0x6D7` | Pterodactyl enemy resources | `PROVEN` resource identity; whether both variants appear in Spiral Mountain remains `UNKNOWN` here |

`chbottlesparty` names an actor using resource `0x781`; this resource is the
house door, not the Bottles character mesh. An actor's name alone therefore
does not establish the identity of its model resource. The above 13 models
yield 92 distinct native RT64 texture hashes in offline display-list decoding.
Their bodies also contain untextured triangles colored by vertices. A color
replacement pack affects the sampled textures only; adding detail to those
untextured body surfaces requires a separate model change. These statements
are `PROVEN` for the decoded resources, not visual approval of HD replacements.

## Animated texture segments (`PROVEN`, `STATIC_ROM` / `STATIC_MIPS`)

The model header field `+0x2C` is an offset to an optional texture-animation
descriptor. A nonzero offset points to seven consecutive big-endian records,
each containing `s16 frame_stride_bytes`, `s16 frame_count` and `f32 frame_rate`.
`func_800D6A14` passes this descriptor to `func_800DBBF8`; a zero stride disables
that group. `func_800DBC68` advances the group's floating-point phase using
delta time and wraps it by the declared frame count.

`func_800DB9FC` returns `frame_stride_bytes * (s32)phase` for an active group.
The renderer binds group `i` to segment `15 - i`, in addition to the normal
texture-data base in segment 2. The corresponding `gSPSegment` command words
start at `0xDB06003C` and decrease by four per group. Therefore a display-list
texture-image command addressing segments 9 through 15 is a valid animated
texture source; an extractor restricted to segment 2 omits those frames.

The stride covers the complete frame payload, including any palette. For USA
model `0x775`, the first record is `(2080, 2, 1.0)`: two 64 by 64 CI4 frames,
each with a 32-byte palette followed by 2048 image bytes. Treating this as one
64 by 128 image with a single palette does not reproduce both native frames.
This is distinct from actor-driven renderer selector changes.

Primary paths: `decomp/src/core2/1EB52C0.c` (`func_800DB9FC`,
`func_800DBBF8`, `func_800DBC68`), model-load path `func_800D6A14`, and
`func_800DE498` at guest `0x800DE8D8` through `0x800DE98C`. Offline decoding of
42 Witchyworld scene/prop models enumerated their declared animation frames
without an unmatched texture descriptor; this does not establish runtime
appearance of replacement textures.

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
those pixels. Rareware and Dolby retain original USA payloads through asset-tail relocation. The Italian HD title
instead uses the complete localized `0x8A8` model with distinct RT64 identities for all 28 textures: the project toggles
one red-channel low bit in an alpha-zero texel of each tile. Alpha and every nonzero-alpha texel remain unchanged.
The complete HD atlas is split by actual display-list vertex loads, not texture index order. Host extraction parity
establishes Rareware/Dolby byte identity; offline hashing matches all 28 USA hashes to the reference HD pack and
establishes disjoint Italian hashes. Final HD appearance requires game testing.

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

## Mumbo's Skull exterior (`PROVEN`, `STATIC_ROM`)

Resource `0x648` contains the exterior building: the golden skull, crooked blue
windows, stone base, chimney and red feathers. Its native material table has
eight CI4 descriptors, each 64×64. Static display-list extraction resolves all
eight materials without missing descriptors and produces eight distinct RT64
v5 identities. An offline rendering of the native geometry matches the building
in the user's Glitter Gulch Mine screenshots.

An HD inventory restricted to the mine's scene model `0x1D25` excludes these
building materials. Include resource `0x648` explicitly when auditing that
location. This establishes asset identity and static coverage; runtime draw
coverage and replacement-texture appearance remain subject to gameplay review.
