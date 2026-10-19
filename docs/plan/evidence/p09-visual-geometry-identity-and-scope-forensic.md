---
title: "P09 visual geometry identity and scope forensic"
description: "The six closed right-plume DLs do not cover left plume or crest; V6B already drew those DLs; current witness is incomplete."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-07
tags: [p09, kazooie, feather, geocmd, identity, forensic]
---

# P09 visual geometry identity and scope forensic

Bounded objective: test whether the six right-plume DLs closed by V7 /
Q' / RSPProcess actually cover the user-visible Kazooie head-feather
glitch, or whether the visual witness belongs to other `0x62B` feather
geometry. Static ROM walk plus existing captures. No game. No new
diagnostic. No production fix. No GPU readback or added
synchronization. No commit. No push.

Do not continue linearly deeper into the renderer. Do not treat Q-witness
class C as `ROOT_CLASS`. Banjo snout causality stays closed.

```text
VISUAL_TARGET_IDENTITY_STATUS: B
CURRENT_WITNESS_IS_INCOMPLETE: YES
CURRENT_SIX_DLS_COVER_RIGHT_PLUME_COMPLETELY: YES
CURRENT_SIX_DLS_COVER_LEFT_PLUME: NO
CURRENT_SIX_DLS_COVER_CREST: NO
CURRENT_RIGHT_PLUME_ONLY_WITNESS_SUFFICIENT_TO_COVER_USER_OBSERVATION: NO
VISUAL_GEOMETRY_SCOPE_CLOSED: NO
CURRENT_BEST_ROOT_CLASS: POST_HANDOFF_RENDER_PATH_BUG_UNRESOLVED
ROOT_CONFIDENCE: MEDIUM that the six-DL witness is incomplete; LOW for the graphical root
PRODUCTION_FIX_CANDIDATE_FOUND: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
```

Checkout: Tooie `ff5b6d9` dirty. Asset `/tmp/asset_062B.bin` (78200
bytes, magic `0xB`, GeoCmd root `+0x119F0`). Existing captures used
offline: `/tmp/tooie-kazooie-feather-v6b-tri.csv` (4096 TRI rows),
`/tmp/tooie-kazooie-feather-postmatch.csv`,
`/tmp/tooie-kazooie-feather-rt64-identity.csv`,
`/tmp/tooie-kazooie-feather-matrix.csv`.

## 1. Display-tree parent correction (`PROVEN` `STATIC_ROM`)

Walking compact type-2 `+0x12320` (INDEX 25, boneAnimID 107,
`lbu(+8)==0x10`, `lw(+4)==0`) shows the GeoCmd parent of both plumes
**is** 107. Head 108 is the first child. Its sibling continue
(`next=0x160`) reaches `+0x12490` then right plume `+0x124A0` then left
plume `+0x12590`. Crest 79 is a **child of head 108**, not a sibling of
the plumes.

Previous statements that listed 100–105 as GeoCmd children of head 108
are `CORRECTED/SUPERSEDED`: that listing followed 108's sibling
continue, which is 107's child list, not 108's children.

```text
+0x12320 type=0x02 idx=25 anim=107          # skeletal+display parent of plumes
  child +0x12330 type=0x02 idx=26 anim=108  # Kazooie head
    child type-0xA, type-0x11 dl=0x48F (head own)
    child +0x12368 type=0x02 idx=27 anim=79 # CREST
    then head extras 0x4BD/0x4DE, type-0xA, beak 47/48, dl=0x50B
  sibling +0x12490 type=0x11 dl=0x524       # not a tracked feather bone
  sibling +0x124A0 type=0x02 idx=30 anim=103 # RIGHT PLUME
  sibling +0x12590 type=0x02 idx=33 anim=100 # LEFT PLUME (next=0)
```

Skeletal parent of 103 and 100 remains index 25 / anim 107. GeoCmd
parent and skeletal parent now agree for the plumes.

## 2. All visible feather DLs (`PROVEN` `STATIC_ROM`)

Gfx address = `model_bin+0x2670+dl_index*8`. Type `0x11` own; type
`0x18` join (`G_VTX` child, `G_POPMTX`, `G_VTX` parent, mixed tris).
Zero `G_MODIFYVTX` on every listed DL.

| BONE | ROLE | GEOCMD_OFFSET | DL_INDEX | VERTEX_SOURCE_RANGES | PARENT | CHILD | VISIBLE_GEOMETRY_ROLE |
| ---: | --- | --- | --- | --- | --- | --- | --- |
| 103 | right root | `+0x124A0` | — | — | 107 | 104 | type-2 transform |
| 103 | own | `+0x12580` | `0x5AA` | n=8 v0=0 `0x010037D0` | 107 | — | OWN proximal right |
| 103 | join 107↔103 | `+0x12550` | `0x592` | n=4 v0=0 `0x01003750`; POP; n=4 v0=28 `0x01003790` | 107 | — | JOIN |
| 104 | right mid | `+0x124B0` | — | — | 103 | 105 | type-2 transform |
| 104 | own | `+0x12540` | `0x57C` | n=8 v0=0 `0x010036D0` | 103 | — | OWN mid right |
| 104 | join 103↔104 | `+0x12510` | `0x564` | n=4 v0=0 `0x01003650`; POP; n=4 v0=28 `0x01003690` | 103 | — | JOIN |
| 105 | right tip | `+0x124C0` | — | — | 104 | — | type-2 transform |
| 105 | own | `+0x124D0` | `0x534` | n=12 v0=0 `0x01003510` | 104 | — | OWN distal right |
| 105 | join 104↔105 | `+0x124E0` | `0x54C` | n=4 v0=0 `0x010035D0`; POP; n=4 v0=28 `0x01003610` | 104 | — | JOIN |
| 100 | left root | `+0x12590` | — | — | 107 | 101 | type-2 transform |
| 100 | own | `+0x12670` | `0x636` | n=8 v0=0 `0x01003B10` | 107 | — | OWN proximal left |
| 100 | join 107↔100 | `+0x12640` | `0x61E` | n=4 v0=0 `0x01003A90`; POP; n=4 v0=28 `0x01003AD0` | 107 | — | JOIN |
| 101 | left mid | `+0x125A0` | — | — | 100 | 102 | type-2 transform |
| 101 | own | `+0x12630` | `0x608` | n=8 v0=0 `0x01003A10` | 100 | — | OWN mid left |
| 101 | join 100↔101 | `+0x12600` | `0x5F0` | n=4 v0=0 `0x01003990`; POP; n=4 v0=28 `0x010039D0` | 100 | — | JOIN |
| 102 | left tip | `+0x125B0` | — | — | 101 | — | type-2 transform |
| 102 | own | `+0x125C0` | `0x5C0` | n=12 v0=0 `0x01003850` | 101 | — | OWN distal left |
| 102 | join 101↔102 | `+0x125D0` | `0x5D8` | n=4 v0=0 `0x01003910`; POP; n=4 v0=28 `0x01003950` | 101 | — | JOIN |
| 79 | crest | `+0x12368` | — | — | 108 | — | type-2 transform |
| 79 | own | `+0x12378` | `0x4A6` | n=7 v0=0 `0x01003010` | 108 | — | OWN crest |
| 79 | join 108↔79 | `+0x12388` | `0x4B0` | n=5 v0=0 `0x01003080`; POP; n=5 v0=27 `0x010030D0` | 108 | — | JOIN |

```text
ALL_VISIBLE_FEATHER_DLS_ENUMERATED: YES
```

Not in the seven-bone set, do not silently add as targets: head own
`0x48F`; head extras `0x4BD`/`0x4DE`/`0x50B`/`0x524`; beak 47/48;
bone 99 own/join `0x64C`/`0x65D` (V6B sometimes labelled `BONE_ACTIVE=100`
as leftover `MATCH_ACTIVE` — that is a classification leak, not ownership).

## 3. Subtrees

```text
RIGHT_PLUME_GEOMETRY:
  107 → 103 (own 0x5AA JOIN) → 104 (own 0x57C JOIN) → 105 (own 0x534 JOIN)
  DLs: 0x5AA OWN, 0x592 JOIN, 0x57C OWN, 0x564 JOIN, 0x534 OWN, 0x54C JOIN
  nested G_DL 0x03000040 on all six; 0 triangles from that nested call (V6C)

LEFT_PLUME_GEOMETRY:
  107 → 100 (own 0x636 JOIN) → 101 (own 0x608 JOIN) → 102 (own 0x5C0 JOIN)
  DLs: 0x636 OWN, 0x61E JOIN, 0x608 OWN, 0x5F0 JOIN, 0x5C0 OWN, 0x5D8 JOIN
  nested G_DL 0x03000040 on all six; distinct vertex banks from the right

CREST_GEOMETRY:
  108 → 79 (own 0x4A6 JOIN 0x4B0)
  nested G_DL 0x03000010 (same class as head 0x48F / snout, not plume 0x40)

CURRENT_SIX_DLS_COVER_RIGHT_PLUME_COMPLETELY: YES
CURRENT_SIX_DLS_COVER_LEFT_PLUME: NO
CURRENT_SIX_DLS_COVER_CREST: NO
```

## 4. What the current six actually draw

| DL | VISIBLE_REGION | CLASS |
| --- | --- | --- |
| `0x5AA` | bone 103 own proximal right-plume mesh (4 tris, 8 verts) | OWN |
| `0x57C` | bone 104 own mid right-plume mesh (4 tris, 8 verts) | OWN |
| `0x534` | bone 105 own distal right-plume tip (8 tris, 12 verts) | OWN |
| `0x592` | join triangles 103↔107 (4 tris, slots 0–3 + 28–31) | JOIN |
| `0x564` | join triangles 104↔103 | JOIN |
| `0x54C` | join triangles 105↔104 | JOIN |

```text
DL_5AA_VISIBLE_REGION: proximal right plume (bone 103 own)
DL_57C_VISIBLE_REGION: mid right plume (bone 104 own)
DL_534_VISIBLE_REGION: distal right plume tip (bone 105 own)
DL_592_VISIBLE_REGION: right-plume root join 103↔107
DL_564_VISIBLE_REGION: right-plume mid join 104↔103
DL_54C_VISIBLE_REGION: right-plume distal join 105↔104
RIGHT_PLUME_VISUAL_IDENTITY_CONFIDENCE: HIGH
```

Identity is from GeoCmd boneAnimID + type-0x11/0x18 role + V6B
`GFX_INDEX`×`BONE_*` crosstab (`0x5AA`/`0x57C`/`0x534` are
`MATCH_ACTIVE` on 103/104/105; joins are `MIXED_AND_NE_ACTIVE`).

## 5. Left and crest counterparts

```text
BONE100_OWN_DL: 0x636
BONE101_OWN_DL: 0x608
BONE102_OWN_DL: 0x5C0
LEFT_JOIN_ROOT_DL: 0x61E   # 107↔100
LEFT_JOIN_MIDDLE_DL: 0x5F0 # 100↔101
LEFT_JOIN_DISTAL_DL: 0x5D8 # 101↔102
BONE79_OWN_DL: 0x4A6
BONE79_JOIN_DL: 0x4B0
BONE79_PARENT: head 108 (GeoCmd + skeletal display child)
CREST_VERTEX_RANGES: own n=7 v0=0 0x01003010; join n=5 v0=0 0x01003080 then n=5 v0=27 0x010030D0
CREST_TRIANGLE_COUNT: static 3 own + 6 join; V6B 186 + 372 TRI rows
```

## 6. User-visible description

The user phrase is "head feathers" while moving. It does not prove
right-only. Left is a complete mirror 3-bone plume of similar V6B
triangle volume (right own+join 1736 rows; left 1710). Crest is a
shorter but live tuft on the head (558 V6B rows).

```text
USER_VISIBLE_REGION_CAN_BE_RIGHT_PLUME: YES
USER_VISIBLE_REGION_CAN_BE_LEFT_PLUME: YES
USER_VISIBLE_REGION_CAN_BE_CREST: YES
CURRENT_RIGHT_PLUME_ONLY_WITNESS_SUFFICIENT_TO_COVER_USER_OBSERVATION: NO
```

V7 spec already marked left/crest `OUT_OF_SCOPE`. Closing RSPProcess on
the six right DLs cannot close the visual identity.

## 7. Existing captures (`PROVEN_FOR_CAPTURE`)

```text
EXISTING_CAPTURE_HAS_LEFT_PLUME_GEOMETRY: YES
EXISTING_CAPTURE_HAS_CREST_GEOMETRY: YES
```

V5B / postmatch / RT64 identity: bones 79 and 100–102 have AUTO identity
`CORRECT`, GMTX/POSTMATCH/NO_INTERP masks `0x7F`, visual `SAME`. That
refutes temporal **matrix** interpolation for the whole seven-bone set;
it does **not** close post-handoff CPU/RSPProcess for left/crest DLs.

V6B TRI `GFX_INDEX` (this capture, 4096 rows, `DL_DEPTH=1`):

| GFX_INDEX | TRI rows | bone / class |
| --- | ---: | --- |
| `0x534` | 496 | 105 `MATCH_ACTIVE` |
| `0x5C0` | 490 | 102 `MATCH_ACTIVE` |
| `0x4B0` | 372 | 79 mixed join |
| `0x54C`/`0x564`/`0x57C`/`0x592`/`0x5AA` | 248 each | right |
| `0x5D8`/`0x5F0`/`0x608`/`0x61E`/`0x636` | 244 each | left |
| `0x4A6` | 186 | 79 `MATCH_ACTIVE` |

Left and crest **do draw**. V7 and Q' CSVs do not admit those DLs.

Do not request another identity capture: V6B already names `GFX_INDEX`
and bones for all 14 feather DLs.

## 8. Asymmetry

```text
RIGHT_LEFT_STRUCTURALLY_SYMMETRIC: YES
IMPORTANT_ASYMMETRIES:
  - distinct DL indices and segment-1 vertex banks (not shared tables)
  - right is the earlier 107 sibling; left next=0 ends that list
  - crest is one bone, parent 108, nested G_DL 0x03000010, join n=5 v0=27
    (plumes: three bones, parent 107, nested 0x03000040, join n=4 v0=28)
CREST_STRUCTURALLY_DISTINCT: YES
```

Same class otherwise: compact type-2 `func_800DCF48` single `G_MTX`,
type-0x11 own after nested child bones, type-0x18 join POP+restore,
F3DEX2 `G_VTX`/`G_TRI2`, no `G_MODIFYVTX`.

## 9. Other P09 secondary geometry (static pattern only)

```text
COMMON_SECONDARY_GEOMETRY_PATTERN:
  compact type-2 DCF48 single-G_MTX articulated child bones plus
  type-0x18 join (child VTX, POPMTX, parent VTX, mixed tris).
  Not ExtraSegments. Not E2180 dual-mtx.
RIGHT_PLUME_HAS_PATTERN: YES
LEFT_PLUME_HAS_PATTERN: YES
CREST_HAS_PATTERN: YES (shorter: one bone, join slots 0–4/27–31)
MUMBO_OR_BACKPACK_ANALOGUE_EXISTS: UNKNOWN for Mumbo (feather nodes still
  UNKNOWN); NO for backpack (overlay babackpack 0x621 / E2180 dual-mtx).
```

Do not reopen Banjo snout. Snout is the same DCF48 class but its **first**
child is type-0x18 (`dl=0xF2`), not a nested three-bone plume.

## 10. Draw identity / filter miss

```text
VISIBLE_POLYGON_TO_SOURCE_DL_MAPPING_ONE_TO_ONE: YES
  for submitted plume/crest triangles versus GFX_INDEX.
  Nested G_DL 0x03000040 / 0x03000010 is shared setup; V6C showed
  0x03000040 → 0x0011C870 with zero tracked triangles. Join DLs mix
  parent/child slots but still submit under that join's Gfx index.
CAN_CURRENT_SIX_DL_FILTER_MISS_VISUALLY_RELEVANT_FEATHER_TRIANGLES: YES
  Static mechanism: left six Gfx (0x5C0/0x5D8/0x5F0/0x608/0x61E/0x636)
  and crest pair (0x4A6/0x4B0) are distinct G_DL targets. They are not
  nested inside the six right DLs. A current/source-DL filter on
  0x534/0x54C/0x564/0x57C/0x592/0x5AA never sees them.
```

A left-plume polygon cannot appear with a right-plume source-DL identity
via nested `G_DL`. Shared nested `0x03000040` does not carry extra plume
tris (V6C).

## 11–15. Decision

```text
VISUAL_GEOMETRY_SCOPE_CLOSED: NO
VISUAL_TARGET_IDENTITY_STATUS: B
WHY: left plume is a complete parallel six-DL tree that V6B already
  submits at nearly the same triangle volume as the right; crest is a
  live two-DL tuft on head 108. User observation is not right-only.
  V7/Q'/RSPProcess closed only the right six. Other feather geometry is
  equally able to be the visible glitch.
ADDITIONAL_TARGET_BONES: 100, 101, 102, 79
ADDITIONAL_TARGET_DLS: 0x5C0, 0x5D8, 0x5F0, 0x608, 0x61E, 0x636, 0x4A6, 0x4B0
CAN_REUSE_V7_EXISTING_INFRASTRUCTURE: YES
  (same physical(model_bin)+0x2670+logical*8 admission; add eight
   exact addresses; keep independent per-DL quotas)
BEST_VISUAL_IDENTITY_WITNESS: NONE
  V6B already identifies GFX_INDEX × bone. Do not add a screen-bbox
  witness. Do not GPU-readback.
EXPECTED_CAPTURE_COUNT: 0 for identity; 1 later only if a repaired
  V7-class handoff is extended to the eight DLs (not this turn)
NEXT_NON_POSITION_ASSUMPTION: N/A (status is B, not A)
```

Do not implement the eight-DL extension in this turn.

```text
PRODUCTION_FIX_CANDIDATE_FOUND: NO
NEXT_SINGLE_ACTION: extend the existing V7 exact-admission / CPU-handoff
  comparison to the eight additional DLs (left six + crest pair) using
  the current V7 infrastructure; do not instrument RSPProcess; do not
  GPU-readback; do not implement in this forensic turn
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
```
