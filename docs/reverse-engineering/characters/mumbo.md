---
title: "Mumbo"
description: "Current knowledge of Mumbo geometry vs animation; feather nodes remain UNKNOWN."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-11-12
tags: [reverse-engineering, mumbo, characters]
---

# Mumbo

## Proven role

| Item | Value | Status |
| --- | --- | --- |
| GeoCmd geometry | `0x665` (`gtype=0`, 41 bones) | `PROVEN` `STATIC_ROM` |
| ExtraSegments pair | `0x854` (`gtype=0x40`, 41 bones, no GeoCmd `0x02`) | `PROVEN` `STATIC_ROM` |
| Overlay 118 packed pair | `0x665` / `0x854` at `+0xC184` | `STRONG_INFERENCE` (list neighbourhood) |
| Appearance `TRANSFORM_D_MUMBO` land | index `0x11` | `PROVEN` decomp `ba/data.c` |
| `0xEC` | animation (magic `00000028`) | `PROVEN`; **not** a model (`CORRECTED`) |

Do not document `0xEC` as Mumbo geometry.

Solo Banjo appearance `0xF` is **not** Mumbo. Treating overlay-118
`C148 + appearance*4` as a proven model index is `REFUTED` as HIGH
confidence (that alignment would map `0xF` onto Mumbo `0x665`).

## User-observed P09 residual

Do not treat Banjo snout E2AA4 as a shared dual-mtx root for these
parts. Live `0x62B` snout is compact single G_MTX (`func_800DCF48`).

Head feathers glitch while walking/moving (`USER_OBSERVED`). Exact
feather node / boneAnimID: `UNKNOWN`. Dual-mtx/E2180 for Mumbo feathers:
`UNKNOWN` / **not inherited** from Banjo snout.

Which live Mumbo body (`0x665` vs `0x854`) is used during that walk:
`UNKNOWN`. Fingerprint in the secondary-sway witness can classify both
from magic/gtype/bones if a Mumbo DE448 hits the player RA filter (it
may not; filter is overlay-118 player draw).

## Modding relevance

### Texture coverage and TMEM (`PROVEN`, `STATIC_ROM`)

A census of the stored USA display lists finds 663 triangles in each resource:
`0x665` has 551 texture-enabled and 112 vertex-colored triangles; `0x854`
has 663 vertex-colored triangles and no native texture loads. These are full
stored-list counts, not a capture of a selected live frame.

The texture-enabled `0x665` path includes 64×64 CI4 images occupying all
2,048 bytes of lower TMEM. The 1792..2047 pixel reservation used by the
Banjo body extension is therefore not safe to assume for this resource.
Texture replacement by native RT64 hashes does not require changing its mesh.
The textureless `0x854` resource has no native pixel region to overwrite,
but adding UVs there is a separate model replacement. This static coverage
does not establish which resource a live NPC/player draw selects.

**PROVEN ROLE:** two geometry resources (GeoCmd vs ExtraSegments) plus
a locomotion animation `0xEC`.

**POTENTIAL MODDING USE:** replace `0x665` or `0x854`; do not retarget
animation IDs as meshes.
