---
title: "Banjo"
description: "Current knowledge of Banjo / BK body, bones, walk animation, and secondary sway."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-09-03
tags: [reverse-engineering, banjo, characters]
---

# Banjo

Default land form is BK (`TRANSFORM_1_BK`). Appearance index `0x6`
(`PROVEN`, `func_80800000_badata`). Solo Banjo land is appearance `0xF`
(`PROVEN`); that is **not** Mumbo.

## Proven role

| Item | Value | Status |
| --- | --- | --- |
| Walk animation | resource `0x56` | `PROVEN` `STATIC_ROM` |
| Body during walk | `0x62B` live capture; static `0x61F` MEDIUM table | live `PROVEN_FOR_CAPTURE`; static `STRONG_INFERENCE` |
| High-detail GeoCmd body | `0x62A` / `0x62B` | `PROVEN` as geometry; **absent** from overlay 118 first 256 KiB |
| Backpack (attached) | `0x621` (+ state variants) | `PROVEN` overlay 113 |
| Secondary locomotion sway | `bawobble` → `func_800E0B8C(part=3)` | `PROVEN` |

`0x56` is **not** Banjo geometry (`CORRECTED`). See [[../corrections]].

## Bone hierarchy (BK ExtraSegments `0x61F` / GeoCmd `0x62A` where noted)

`func_800E0B8C` `part` is **boneAnimID** (`PROVEN`). Bone INDEX is a
different number.

| boneAnimID | Role | INDEX on `0x61F` | INDEX on `0x62A` | INDEX on `0x62B` | Status |
| --- | --- | --- | --- | --- | --- |
| 3 | hip | 1 | 1 | 1 | `PROVEN` |
| 1 | torso | — | 2 | 8 | `PROVEN` as next in chain |
| 18 | neck | — | 9 | 9 | `PROVEN` as parent of snout |
| 42 | snout / muzzle | 4 | 10 | 11 | `PROVEN` geometry `HIGH` |

Known chain (`PROVEN`):

```text
hip 3 → torso 1 → neck 18 → snout 42
```

Wobble writes hip 3. Snout 42 inherits that Euler. Relation:

```text
WOBBLE_PART3_DRIVES_SNOUT: INDIRECT
part 3 == GeoCmd tag 5: REFUTED identity
```

On `0x62A`, GeoCmd `0x0A` tag 5 is the **right hand** (bone 5 / anim 16).

## Render class fork (live ID `0x62B` for the submitted walk)

| Live body | Snout path | Uses `func_800E2AA4` / `func_800E2180` |
| --- | --- | --- |
| `0x61F` | ExtraSegments bone INDEX 4, parent neck 18; verts in GeoCmd `0x03` DLs | **NO** |
| `0x62A` | GeoCmd `0x02` snout @ `0x14960` INDEX 10 | **UNKNOWN** live (static fat/dual-mtx candidate) |
| `0x62B` | compact GeoCmd `0x02` snout @ `0x11CB8` INDEX 11 (sibling in neck child list, not a DE2A4 list head); live type-2 slot `func_800DCF48` (`PROVEN_RUNTIME`); ADE2C single-G_MTX path (`PROVEN_RUNTIME`) | **NO** (E2AA4 raw 0; `STATIC_PATH_CANDIDATE` only) |

Witness (default-off): the 2026-09-03 CPU capture records the live tree at
`model_bin=0x80226940`, root `+0x119F0`, and the snout at runtime pointer
`0x802385F8` / `+0x11CB8` in the neck sibling list. It records
`DCF48_REL_0X11CB8=2243` and `DCF48_SNOUT_ACCEPT_COUNT=2243`, proving the
snout→`func_800DCF48` edge at runtime. The child setup node `+0x11CC8`
(type `0x18`) and visible DL node `+0x11CF8` (type `0x11`) are also present.
The same capture records `ADE2C_SNOUT_SAMPLES=2243` with identity
`SINGLE_G_MTX`; the witness samples the current slot only and does not prove
RT64 interpolation identity. E2AA4 remains a non-live alternate path for
this capture. Do not use `0x62A` offset `0x14960` or allocator span `0x20000`
as the live `0x62B` snout identity.

## bawobble

Locomotion-dependent secondary animation. Update:
`bawobble_entrypoint_8` if `*(PlayerState+0x154)+0xC == 2`. Apply:
`func_808001C0_bawobble` → `func_800E0B8C(part=3, type=3)`. Enable:
`bawobble_entrypoint_6`. Not every walk frame.

See [[../systems/secondary-animation]].

## Modding relevance

**PROVEN ROLE:** hip-targeted additive Euler while walking; snout is a
descendant bone on the composed hierarchy.

**POTENTIAL MODDING USE** (`HYPOTHESIS`, not a shipped API):

- change when sway runs (`unkC` / entrypoint 8)
- retarget `part` to another boneAnimID
- replace `0x61F` body or child geometry once live ID is confirmed
- do **not** assume GeoCmd `0x02` part edits apply to ExtraSegments snout
