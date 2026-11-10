---
title: "CPU vertex lighting: light sources, light sets and the per-vertex formula"
description: "Map and actor point lights, the directional light, per-object light sets and the integer formula the game uses to write lit vertex colors on the CPU; where a host bridge can read them."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-11-10
tags: [reverse-engineering, lighting, models, vertex-color, normals]
---

# CPU vertex lighting

How Banjo-Tooie lights models: on the CPU, writing vertex colors, with point lights from the map file and from actors
plus one directional light. Companion to [player lighting](player-lighting.md) (split-screen ownership of output
vertices) and [model renderer](model-renderer.md).

Sources: the decomp files of this area are all `GLOBAL_ASM` (`decomp/src/core2/1EE7960.c` 8010E070..8010E7F4,
`1EE8100.c` 8010E810..8010EECC, `1E8CC00.c` 800B3310..800B38E8, `1EA1DA0.c` the light table and `_gclightsDll`
wrappers, `1E7BAB0.c` 800A235C; BSS in `decomp/src/core2/core2bss1.s:760-794`), so the evidence is the instruction
addresses in the recompiled C (read only). Investigation 2026-11-10 (static reading; no runtime capture yet).

## Pipeline

```text
map load      gclightsDll_entrypoint_2 parses the map "lights" chunk -> point lights in D_8012AB00,
              ambient D_8012AB04, directional light D_8012AB1C (color) / D_8012AB28 (direction), D_8012AB10 (unknown)
actors        800C8A98 create / 800C8CB8 position / 800C8D4C radii / 800C8E84 color / 800C8B84 release (same table)
per player    func_800991B0 -> 800A2060 (tint step) ... 800A2534 (clears "set built this frame")
per draw      players: 800923A8 -> 800A2540 -> 800A235C (light set, once per frame) -> 800DF660(1, 0, set, alpha)
              actors:  80106A98 -> 80106C4C (cached set) / 800DF660(mode, count, set, alpha) / 800DF720(record buffer)
model draw    func_800DE498 -> table 0x80123030[mode] = {init, batch, end}; mode 1 = 8010E2C4, 8010E4C4, 8010E7F4
              geo vertex-batch commands 800DE1C4 / 800DD65C call batch(matrix, &firstVtx, count)
```

## Light sources

### Point-light table `D_8012AB00` (0x1C bytes per entry, ids 1-based)

| Off | Type | Meaning | Status and evidence |
| --- | --- | --- | --- |
| +0x00 | s16[3] | world position | `PROVEN`: `800C8800` converts it to float; setter `800C8CB8` |
| +0x06 | s16 | inner radius (full intensity) | `PROVEN`: `800C8898`; setter `800C8D4C` |
| +0x08 | s16 | outer radius (cutoff); forced to inner+1 when equal | `PROVEN`: `800C8D4C` |
| +0x0A | u8[3] | RGB | `PROVEN`: `800C8900`; setter `800C8E84` |
| +0x10 | f32 | falloff slope `1/(outer-inner)` | `PROVEN`: `div.s` at `0x800C8E3C` |
| +0x14 | u8 | persistent / from the map file | `STRONG_INFERENCE` |
| +0x15 | u8 | slot in use and class (0 free, 0x0B dynamic, 0x0D map default, map tag-7 byte) | `STRONG_INFERENCE` |
| +0x16 | u8 | dirty (1 changed, 2 released) | `STRONG_INFERENCE` |
| +0x17 | u8 | enabled; 0 reports color 0,0,0 | `PROVEN` for the color effect (`0x800C8920`) |
| +0x18 | u8 | second "include" flag (entry used if +0x17 or +0x18) | `PROVEN` as gate in `800C85A0`; meaning `UNKNOWN` |
| +0x19 | u8 | position/radius changed | `STRONG_INFERENCE` |

The iterator also calls a filter at `*(0x8011A9C0 + idx*4)` (ROM data): `UNKNOWN`.

Origins, `PROVEN`:

- Map file "lights" chunk (`gclightsDll_entrypoint_2`): tag 1 point light {tag 2 position, tag 3 radii, tag 4 color,
  tag 7 class}; tag 5 ambient (`800C8C90` -> `D_8012AB04`); tag 6 -> `D_8012AB10` (role `UNKNOWN`); tag 9 directional
  light {tag 2 direction (normalized), tag 4 color} via `800C8F08`; tag 8 creates lights that are released at once
  (`UNKNOWN` why); tag 0xA ignored.
- About 59 actor call sites of `800C8A98`: flames, light halos, spotlights, factory lights, glow objects, jigsaws,
  Grunty fireballs, bosses, lava, volcano bits, TNT boxes, Mumbo attacks and others.

There is no light-type field: point lights with linear falloff, plus one global directional light.

### Light set (`800B3310(n)`, size 0x18 + n*0x1C)

| Off | Type | Meaning | Status |
| --- | --- | --- | --- |
| +0x00 | s32[3] | ambient RGB, 256 = 1.0 | `PROVEN`: `800B38C8` copies `D_8012AB04` |
| +0x0C | s32 | capacity | `PROVEN` |
| +0x10 | ptr | end of used records | `PROVEN` |
| +0x14 | ptr | capacity end | `PROVEN` |
| +0x18 | rec[n] | gathered lights, nearest first | `PROVEN` |

Gathered record (0x1C bytes), `PROVEN` from `800B3494(set, pos, objRadius)`:

| Off | Type | Meaning |
| --- | --- | --- |
| +0x00 | f32[3] | unit vector from the object toward the light, world axes |
| +0x0C | s32[3] | attenuated RGB |
| +0x18 | f32 | distance (sort key and weight) |

```text
for each enabled light L:
    d = L.pos - pos; dist = |d|
    if (L.outer + objRadius <= dist) skip
    D = dist - objRadius
    color = (D <= L.inner) ? L.rgb : trunc(L.rgb * (1 - L.slope * (D - L.inner)))
    insert by distance (800B3390), the farthest dropped when full
```

The directional light (`800B36C0`) is added when a slot remains: color `D_8012AB1C`, direction `D_8012AB28`, distance
that of the nearest point light or 500.0. `PROVEN`.

## Formula (mode 1)

### Init `8010E2C4(ctx, count, set, matrix)`

Scratch context at `0x80137060` (one global, draws are serial): +0x04 ambient s32[3], +0x10 bone-local flag
(`ctx+0x20 != 0 && ctx+0x34 == 0`), +0x11 light count, +0x14 set (capacity 4), +0x18/+0x1C first/end record. `PROVEN`.

1. A prebuilt set (players, cached actor sets) is copied (`800B37E4`); otherwise `8010E204` gathers with the center and
   radius of the output vertex list header transformed by the matrix-stack top, and a negative count adds the
   directional light.
2. `8010E070`: unless bone-local, each record direction is rotated into model space by the **transpose** of the
   stack-top 3x3 (`800192FC`) and rescaled to length 256. With two or more lights each color is weighted by
   `1 - dist_i / sum(dist)`.

### Per vertex batch `8010E4C4(matrix, &firstVtx, count)`

One index `i` addresses the output `Vtx[i]` (render context +0x1C, 0x18-byte header, stride 16), the normal `N[i]`
(+0x30, s8 x/y/z + pad, length about 127) and the lighting record `R[i]` (+0x2C, 10 bytes: s16 accumulator[3], u8
base[3], pad). `PROVEN` (`0x8010E50C..0x8010E564`). Integer only:

```text
if (light count == 0):                                    // 0x8010E568
    Vtx[i].rgb = low byte of (base * ambient) >> 8        // no clamp
else:
    acc = (s16) ambient
    for each light:
        L = bone-local ? rotate(transpose(boneMatrix), dir) rescaled to 256 (8010E3C4) : dir
        dot = N . L                                       // 32-bit
        if (dot > 0) acc += (s16)((dot * color) >> 15)    // s16, can wrap
    Vtx[i].rgb = min(255, (base * acc) >> 8)              // 0x8010E6E4 sra 8, 0x8010E6E8 slti 0x100
                                                           // alpha untouched
```

So `color = base * (ambient + sum(light * max(0, N.L))) / 256`, saturated at 255, no specular, no distance term here
(falloff happens when gathering). `PROVEN`.

Normals are never transformed per vertex; lights are moved into the normals' space. That the stack-top and bone
matrices are model-to-world without the view rotation is `STRONG_INFERENCE` (`8010E204` compares the transformed
center with world light positions); runtime check pending.

Base color: asset vertex RGB x optional palette x tint >> 8, built by `glpackvtx_entrypoint_0`; the record buffers are
cached and shared per (model, palette, tint) by `801007B8`. `PROVEN`.

## Normals

- Static models: `model_bin + u32(model_bin+0x38)`. `PROVEN`.
- CPU-skinned lit models: `dbanim_entrypoint_1` (only in lighting mode 1) rotates group normals by the bone matrix and
  writes `trunc(n * 127)` at the same index as the position. `PROVEN`.
- When such a model has no private normal buffer, it may write into the shared asset array: `HYPOTHESIS`.

## Users

- `800DF660(mode, count, set, alpha)` arms the next model draw (`D_8012CA08` count, `D_8012CA0C` set, `D_8012CA11`
  mode, `D_8012C838` = 1). Callers: players (`800A2540`), generic actors (`80106A98`), factory trash belt, EMP spell,
  angler boss, inflatable boss. `PROVEN`.
- Players: `PlayerState+0xB4 -> +2 == 0` gives a flat tint only; otherwise `800A235C` builds the set once per frame
  with ambient = `D_8012AB04` x tint >> 8, position `*(player+0x50)+0x64`, radius `*(player+0x50)+0x70`, plus the
  directional light; the nearest color moves toward its target by at most 40 per call. `PROVEN`.
- `PlayerState+0x108` is a **tint** applied to the map ambient and the directional color (`800A2314`, `800A22C8`),
  stepped by 40.0 per call (`800A2060`); [player lighting](player-lighting.md) calls it the player's lighting input,
  which stays true. `PROVEN`.
- Generic actors: light count in `Actor+0x7B` bits 6-7; mode 3 if `Actor+0x96 < 0`, mode 2 if `Actor+0x74` bit 25,
  else mode 1. `PROVEN`.
- Mode 2 = {`8010EECC`, `8010EBE4`, ...}, per-vertex point lighting with distances: `HYPOTHESIS`. Mode 3: `UNKNOWN`
  (the table at `0x80123030` is ROM data).

## RSP lighting (`G_LIGHTING`)

- No CPU-emitted geometry mode sets `0x00020000`; no light count or light `G_MOVEMEM` is emitted. The only light-slot
  moves are `gSPLookAtX/Y` (`0x800DED20`/`0x800DED40`) for environment-mapped models (model header `+0xA & 4`).
  `PROVEN` for CPU code.
- Kazooie's asset display lists clear `G_LIGHTING` ([geocmd](geocmd.md), [Kazooie](../characters/kazooie.md)).
- Normal shading is CPU vertex color with `G_LIGHTING` off: `STRONG_INFERENCE`. Environment-mapped models probably
  set `G_LIGHTING | G_TEXTURE_GEN` in their own lists (texgen needs it); their RSP light state is `UNKNOWN`.

## Where a host can read it

All inputs are plain RDRAM globals: the table `D_8012AB00`, `D_8012AB04` (ambient), `D_8012AB1C`/`D_8012AB28`
(directional), the per-player set `(PlayerState+0xB4)->+4` and the tint `PlayerState+0x108`. Tap points:

1. Once per player and frame: after `func_800991B0` (already wrapped by `tooie_dev_on_player_update` in
   `src/game/tooie_dev_warp_hook.cpp`).
2. Per draw: after `func_8010E2C4`; `0x80137060..0x8013707C` then holds what the formula uses (ambient, records in model
   space, weighted colors, count, bone-local flag). `func_800DE498` is also wrapped.
3. Map load: `gclightsDll_entrypoint_2`.

## Open questions (need a runtime capture)

1. Contents of the callback table `0x80123030` and the filter table `0x8011A9C0`.
2. Whether the stack-top and bone matrices include the view rotation.
3. Ranges of ambient, colors and accumulators; whether the s16 accumulator or the unclamped ambient-only path ever
   overflows.
4. Share of bone-batched, CPU-skinned and static lit draws per frame.
5. Map-file lights versus actor lights per map; the role of `D_8012AB10`; why tag-8 lights are released.
6. RSP light state of environment-mapped models.
