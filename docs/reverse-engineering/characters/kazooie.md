---
title: "Kazooie"
description: "Solo 0x61C / reduced 0x83A / combined 0x62B; 14 visible feather DLs; downstream closed; guest compose parent*child XYZW."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-09-08
tags: [reverse-engineering, kazooie, characters]
---

# Kazooie

## Current evidence scope (2026-09-08)

Guest current-matrix compose is `PROVEN` Hamilton `parent ⊗ child` on
XYZW records in `func_800AE8A8` (product error ≤ 1.6e-7 rad, 991
frames). Additive bytes 8/9 are 0 on this path. Bone 107 supplies the
uniform scale and the large local rotation steps; those steps are
`PROVEN` authored clip keys / walk↔idle transitions, not a sampler
bug (`p09-animation-clip-transition-and-authored-key-forensic`).
Plume roots 103/100 stay locally smooth; crest 79 local is nearly
identity; head 108 has an authored nod on clip 22. Combined-walk clip
ids include `0x15` (loop) and `0x16` (30-frame start). Downstream V7
position, culling, draw-temporal, depth, and alpha/texture stay
`CLOSED` as local mismatch. Details:
`p09-upstream-current-bone-transform-semantic-forensic`,
`p09-visible-feather-alpha-coverage-texture-forensic`,
`p09-post-position-draw-temporal-and-depth-state-forensic`,
`p09-visible-feather-culling-winding-face-visibility-forensic`,
`p09-bounded-crest-post-handoff-transferability-audit`.

## Proven role

| Item | Value | Status |
| --- | --- | --- |
| Solo geometry | model `0x61C` (`gtype=0`, 58 bones, compact GeoCmd `0x02`) | `PROVEN` `STATIC_ROM` |
| Reduced solo | model `0x83A` (`gtype=0`, 47 bones) | `PROVEN` `STATIC_ROM`; overlay 118 `a1==0xB` alternate |
| Combined BK on-back | type-2 subtree inside live-walk `0x62B` (head 108, plumes 100–105, crest 79) | `PROVEN` `STATIC_ROM` |
| ExtraSegments on-back | bones 15–24 inside `0x61F` (no GeoCmd `0x02`) | `PROVEN` layout; **not** live walk |
| `0x13A` | animation `ASSET_13A_ANIM_BSKAZPADDLE_UNKNOWN` | `PROVEN`; **not** a model |

Appearance `0x10` (`TRANSFORM_B_KAZOOIE`) selects clip `0x13A`. Overlay
118 `8080BFAC` stores **`0x61C`** or **`0x83A`** when `a1==0xB`.

Live body while **Kazooie carries Banjo**: `0x62B` is
`PROVEN_FOR_CAPTURE` (model `0x80226410` in the V2 producer witness).
`0x61C`/`0x83A` remain valid solo/reduced candidates in other states; do not
assume `0x61F` is live.

## Head feathers (`PROVEN` geometry)

| Resource | Node | INDEX | boneAnimID | Role |
| --- | --- | --- | --- | --- |
| `0x62B` | `+0x124A0` | 30 | 103 | primary right plume (token); chain root |
| `0x62B` | `+0x124B0` / `+0x124C0` | 31 / 32 | 104 / 105 | right plume children of 103 |
| `0x62B` | `+0x12590`… | 33–35 | 100–102 | left plume (`100` chain root) |
| `0x62B` | `+0x12368` | 27 | 79 | crest analog (child of head 108) |
| `0x61C` | `+0xF558` | 11 | 69 | crest (solo dedicated extra) |
| `0x83A` | `+0xD408` | 8 | 69 | crest |

Bone parent of both plume chains is index 25 / boneAnimID 107, **not**
head 108. GeoCmd parent is the same 107 node (`+0x12320`): head 108 is
the first child; plumes are later siblings. Earlier listings that put
100–105 under head 108 followed 108's sibling continue and are
`CORRECTED/SUPERSEDED`. Crest 79 **is** a GeoCmd child of head 108.

Live carry-Banjo capture (`PROVEN_FOR_CAPTURE`): model `0x80228690`,
primary node `0x8023AB30` = `+0x124A0`, 1977 accepts, compact
`func_800DCF48` single `G_MTX`. `SOURCE_MATCHES_COMPOSED` and
`G_MTX_OPERAND_MATCH` 512/512. Frames 1420–1424 local/composed 3x3 are
uniform scale ~0.417→1, not singular (`TOO_STRONG` to call degenerate).

Render class on `0x61C`/`0x62B`/`0x83A` type-2: compact single `G_MTX`
via `func_800DCF48` (`PROVEN_RUNTIME` for the V1 primary feather). Not
E2180. Not ExtraSegments.

Local 4x4 writer: `func_800AE820` into `AnimMtxList+0x50+(index<<6)`.
Record/slot quaternion order is **XYZW**. `func_800AE8A8` does
`q_slot = q_parent ⊗ q_record` (Hamilton) and
`s_slot = s_parent * s_record`. V2/V3 plus the 2026-09-08 offline
reanalysis: bone 103 raw scale is unit; parent idx25 / boneAnimID 107
supplies uniform scale into 100–105 and 79 composed slots. `AF04C` /
`AEDA0` are not reached (`a3+8/9=0` on 18,210 producer rows). Additive
sway on these bones is `CLOSED` as unused, not `UNKNOWN`. Clip/sample
time is the remaining guest seam.

## User-observed P09 residual

```text
KAZOOIE_CARRY_BANJO_HEAD_FEATHER_GLITCH: USER_OBSERVED
LOCOMOTION_ONLY_CORRELATION: USER_OBSERVED
SECONDARY_SWAY_CHARACTERIZATION: USER_OBSERVED
```

Mechanism not inferred from observation. Default-off witness:
`P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V6B`. Guest matrix/rotation remain valid
(`FIRST_BAD_GUEST_STAGE: NONE_OBSERVED`). AUTO identity for bones
`79, 100–105` is `CORRECT`. Full-chain
`TOOIE_DIAG_FEATHER_NO_INTERP=1` V5B covered bones `79, 100–105`
(masks `0x7F`) and was visually `SAME`. JOIN parents 107 and 108 were
**not** in that set. Interpolation of every world transform used by
visible feather triangles is **not** refuted (`CORRECTED`). V5C
extends the same default-off seam to
`79, 100–105, 107, 108`. SIMPLE remains `REFUTED_AS_PRIMARY_FIX` for
the 103-only test.
Visible plume vertices are static segment-1 tables, not a skinned producer.
V3 `VERTEX_BIND_COUNT=0` remains `TOKEN_EXPIRES_BEFORE_CHILD_DL`.

## Visible feather DLs (`PROVEN` `STATIC_ROM`)

Right plume (V7 six-DL set):

| Node | Type | DL index | Role |
| --- | --- | --- | --- |
| `+0x12580` | `0x11` | `0x5AA` | bone 103 own mesh |
| `+0x12550` | `0x18` | `0x592` | bone 103 join 107↔103 |
| `+0x12540` | `0x11` | `0x57C` | bone 104 own mesh |
| `+0x12510` | `0x18` | `0x564` | bone 104 join 103↔104 |
| `+0x124D0` | `0x11` | `0x534` | bone 105 tip mesh |
| `+0x124E0` | `0x18` | `0x54C` | bone 105 join 104↔105 |

Left plume (mirror; V7 VERSION=3 admits these six DLs):

| Node | Type | DL index | Role |
| --- | --- | --- | --- |
| `+0x12670` | `0x11` | `0x636` | bone 100 own mesh |
| `+0x12640` | `0x18` | `0x61E` | bone 100 join 107↔100 |
| `+0x12630` | `0x11` | `0x608` | bone 101 own mesh |
| `+0x12600` | `0x18` | `0x5F0` | bone 101 join 100↔101 |
| `+0x125C0` | `0x11` | `0x5C0` | bone 102 tip mesh |
| `+0x125D0` | `0x18` | `0x5D8` | bone 102 join 101↔102 |

Crest (child of head 108; V7 VERSION=3 admits `0x4A6`/`0x4B0`). Own/join
DLs have zero `G_MODIFYVTX`. Nested `G_DL 0x03000010` runs **before**
`G_VTX`. Live V3: `viewProjIndex=4`, velocity 0, worldIndex 115 (own) /
115+116 (join). Join slots 0–4 / 27–31 do not survive into
`RSPProcessCS` after global-vertex handoff.

| Node | Type | DL index | Role |
| --- | --- | --- | --- |
| `+0x12378` | `0x11` | `0x4A6` | bone 79 own mesh |
| `+0x12388` | `0x18` | `0x4B0` | bone 79 join 108↔79 |

Address: `model_bin+0x2670+idx*8`. Plume type-0x11 DLs contain `G_VTX`
directly after nested `G_DL 0x03000040`. Crest uses nested `0x03000010`.
Bone 103's first GeoCmd child is nested type-2 104, not a DL. See
[[../systems/geocmd]].

All fourteen plus `0x48F`/`0xBC` start with `G_GEOMETRYMODE and=0xC0FFFB`
then `and=0xFFFFFF set=0x00200004`. That **preserves** `G_CULL_*` and
sets shade/smooth only. Twenty-seven other `0x62B` DLs use `0xC0F9FB` to
clear cull. No `G_CULLDL` on the fourteen.

Static G_VTX (`PROVEN` `STATIC_ROM`, F3DEX2 `n=p0(12,8)` `v0=p0(1,7)-n`;
prior `0x5AA v0=8` is `CORRECTED`):

| DL | n / v0 / slots | Segment-1 source |
| --- | --- | --- |
| `0x5AA` | 8 / 0 / 0–7 | `0x010037D0` |
| `0x57C` | 8 / 0 / 0–7 | `0x010036D0` |
| `0x534` | 12 / 0 / 0–11 | `0x01003510` |
| `0x592` | 4 / 0 / 0–3 then 4 / 28 / 28–31 | `0x01003750` / `0x01003790` |
| `0x636` | 8 / 0 / 0–7 | `0x01003B10` |
| `0x608` | 8 / 0 / 0–7 | `0x01003A10` |
| `0x5C0` | 12 / 0 / 0–11 | `0x01003850` |
| `0x4A6` | 7 / 0 / 0–6 | `0x01003010` |
| `0x4B0` | 5 / 0 / 0–4 then 5 / 27 / 27–31 | `0x01003080` / `0x010030D0` |

Join DLs `G_POPMTX` between the two loads, then mix child and parent slots
in the same triangles. Own meshes reuse slots 0–N across bones. Left and
right use **distinct** vertex banks.

Ordinary same-model body controls on live `0x62B` (`PROVEN` `STATIC_ROM`,
not plume/snout/crest): Banjo torso type-0x11 `+0x11C20` `dl=0xBC`
(parent anim 3 INDEX 1); Kazooie head 108 type-0x11 `+0x12358`
`dl=0x48F`. Same Gfx formula as the plume DLs.

## Modding relevance

**PROVEN ROLE:** solo `0x61C`; reduced `0x83A`; combined plumes on `0x62B`.

**POTENTIAL MODDING USE:** retarget plume bones 100–105 / crest 69/79;
do not treat `0x13A` as a mesh.

## V5B full-chain no-interpolation (`PROVEN_RUNTIME` + `USER_OBSERVED`; coverage `CORRECTED`)

V5 coverage was incomplete (only 103). V5B repaired the sibling gate for
`79, 100–105`. That capture had GMTX/POSTMATCH/NO_INTERP masks `0x7F` and
`RESULT==CURRENT` 211/211. Visual result `SAME`. It did **not** cover
JOIN parents 107/108, so it does not refute interpolation of every
transform used by visible feather triangles. V5C:
`p09-feather-no-interp-v5c`. V5B details:
`p09-kazooie-feather-v6-post-matrix-vertex-path`.
