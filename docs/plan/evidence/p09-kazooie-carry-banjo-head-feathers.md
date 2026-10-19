---
title: "P09 Kazooie carry-Banjo head feathers — static map + V5 token bind"
description: "TRANSFORM_B_KAZOOIE selects 0x61C; combined 0x62B has type-2 head plumes; V1 feather token reuses V5 RT64 match."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-03
tags: [p09, kazooie, feathers, gmtx, interpolation]
---

# P09 Kazooie carry-Banjo head feathers

No user capture this round. Cart ROM + decompressed overlay 118 only.
GPU unused. Banjo snout remains closed as the primary P09 witness.

```text
KAZOOIE_CARRY_BANJO_HEAD_FEATHER_GLITCH: USER_OBSERVED
LOCOMOTION_ONLY_CORRELATION: USER_OBSERVED
SECONDARY_SWAY_CHARACTERIZATION: USER_OBSERVED
```

Do not infer a mechanism from those labels.

## Carry-Banjo game state

`func_80800000_badata` appearance `0x10` is `TRANSFORM_B_KAZOOIE`
(`PROVEN` table: `D_80801448[0x10]=0x13A` paddle/Kazooie clip).

Overlay 118 `8080BFAC` (`STATIC_MIPS`):

```text
if a1 == 0xB:
  jal 0x8009EA2C
  if v0 != 0: sh 0x83A
  else:       sh 0x61C
```

`a1==0xB` is the Kazooie transform/slot (`STRONG_INFERENCE`; matches
appearance `0x10` / transform B). Live body for that branch is **`0x61C`**,
or reduced **`0x83A`** when the predicate is nonzero.

Talon Trot / “Kazooie carrying Banjo on her back” as a **locomotion of
`TRANSFORM_1_BK`** is still `UNKNOWN` at runtime. Combined GeoCmd body
`0x62B` (live BK walk `PROVEN_FOR_CAPTURE`) **contains** a full Kazooie
on-back type-2 subtree. That is the best candidate if the reproduction
stays on the combined BK model.

`0x61F` ExtraSegments still has bones 15–24 / anim 100–108, **zero**
GeoCmd `0x02`. It is **not** proven live for this witness.

```text
KAZOOIE_CARRY_BANJO_STATE_OWNER: PlayerState / overlay 118 badata
KAZOOIE_CARRY_BANJO_STATE_ID: appearance 0x10 TRANSFORM_B_KAZOOIE (PROVEN clip);
  Talon Trot locomotion of TRANSFORM_1_BK UNKNOWN
LIVE_MODEL_SELECTION_FUNCTION: 8080BFAC stores 0x61C/0x83A when a1==0xB;
  player draw still 801039E4 / 80092B04 -> DE448
STATIC_CANDIDATE_RESOURCE: 0x61C (solo), 0x83A (reduced), 0x62B (combined type-2)
RESOURCE_0X61F_ROLE: ExtraSegments BK body; Kazooie-on-back bones; NOT live walk
RESOURCE_0X61F_CONTAINS_CARRY_BANJO_KAZOOIE: geometry YES (on-back), live NO
BEST_LIVE_RESOURCE_CANDIDATE: 0x62B if combined BK/Talon Trot; 0x61C if TRANSFORM_B
BEST_LIVE_RESOURCE_CONFIDENCE: MEDIUM
```

## Head-feather geometry (`PROVEN` `STATIC_ROM`)

### Combined `0x62B` (66 bones, compact type-2)

Kazooie head `idx 26` `anim 108` @ `+0x12330`. Children:

| rel | INDEX | boneAnimID | role |
| --- | --- | --- | --- |
| `0x12368` | 27 | 79 | crest analog |
| `0x12410` / `0x12420` | 28 / 29 | 47 / 48 | beak |
| `0x124A0` / `+B0` / `+C0` | 30 / 31 / 32 | 103 / 104 / 105 | right head plume |
| `0x12590` / `+A0` / `+B0` | 33 / 34 / 35 | 100 / 101 / 102 | left head plume |

Primary token: **`+0x124A0` INDEX 30 anim 103** (first segment of a
three-bone plume; same pattern as Mumbo 42–50). Child type `0x11`
`dl=0x534` `tri=0x351`.

GeoCmd listing under head 108 is the **display tree**. Skeletal parent of
103 and 100 is index 25 / boneAnimID 107 (`CORRECTED`). Chains are
`103→104→105` and `100→101→102`.

`HEAD_FEATHER_TRANSFORM_COUNT` on `0x62B`: **7** (79 + 100–105).

### Solo `0x61C` (58 bones)

Head `idx 7` `anim 108` @ `+0xF368`. Dedicated extra: **`+0xF558`
INDEX 11 anim 69** crest, child type `0x11` `dl=0x1FC`. Beak is 8–10 /
42–44. No anim 100–105 bones; side tufts may be type `0x0C` DLs on the
head (`HYPOTHESIS`).

### Reduced `0x83A` (47 bones, gtype 0)

Crest **`+0xD408` INDEX 8 anim 69**. No Banjo sub-skeleton (not a
combined carry mesh).

## Render class

Compact `hdr=0x10` type-2, same encoding as live Banjo snout. Live
`0x62B` type-2 handler is `func_800DCF48` (`PROVEN_RUNTIME` for snout;
`STRONG_INFERENCE` for these sibling nodes in the same tree/table).

```text
GEOCMD_TYPE: 2 compact
ACTUAL_HANDLER: func_800DCF48 (STRONG_INFERENCE for feather nodes)
MATRIX_FUNCTION: ADE2C + 19AA0 + mlMtxApply/guMtxF2L
MATRIX_CLASS: SINGLE_G_MTX
USES_E2180: NO
USES_DCF48: YES (class)
USES_EXPLICIT_MATRIX_GROUP: NO
G_MTX_FLAGS: PUSH|LOAD|MODELVIEW (same DCF48 emitter)
```

Do not inherit E2AA4/E2180. ExtraSegments `0x61F` is a different class.

## Secondary producer

Not traced to a Kazooie-specific writer. `bawobble` remains Banjo hip
`part=3`. Feather sway producer `LOW` / `UNKNOWN`.

## V5 bind

Existing token → `matrixCommon` → `GameFrame::match` is reused.
Guest filter is DCF48 `rel+INDEX+anim` for the primary plume/crest.
Schema `P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V1`. Snout `0x11CB8` is **not**
enqueued.

```text
PRODUCTION_FIX: NO
GPU_READBACK_USED: NO
```
