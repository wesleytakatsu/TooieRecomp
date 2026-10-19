---
title: "P09 BanjoRecomp transform-tagging portability audit"
description: "BK PR22/23/26/46/117 tagging vs Tooie 0x62B: architectural gap is real; P09-causal port is not."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-09
tags: [tooie-recomp, p09, kazooie, feathers, snout, transform-tagging, banjorecomp, pr22, interpolation]
---

# P09 BanjoRecomp transform-tagging portability audit

**Later correction (occurrence identity, 2026-09-09):** V5C still did
not freeze JOIN-POP (and other extra) world slots. Interpolation of
concrete AUTO world matrices is `REOPENED` as the CPU causal term.
PR #22 identity is relevant again as **architecture**, not as proven
wrong-pairing of the 44 explosions. PR #23 SIMPLE addresses
decomposed lerp of composed bones. Do not port yet. SIMPLE recapture is
[[p09-boned-interpolation-counterfactual]]. See
[[p09-world-occurrence-identity-root-cause-audit]].

**Later correction (V5C, 2026-09-09):** V5B did not freeze JOIN parents
107/108. `REFUTED_FOR_VISIBLE_FEATHER_CHAIN` in this audit is
`CORRECTED/SUPERSEDED` for interpolation. PR #22 tagging remains not a
ready production port. PR #23 is a candidate **after** V5C visual.
See [[p09-feather-no-interp-v5c]].

Static / command-stream comparison only. No new visual instrumentation.
No GPU readback, copy, layout, sync, staging, or occlusion query. No
production patch. No `RecompiledFuncs/**` edit. No user screen recording.

```text
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NEW_CAPTURE_REQUIRED: NO
VISUAL_RECORDING_REQUIRED_NOW: NO
PRODUCTION_FIX: NO
GENERATED_RECOMPILED_CODE_EDITED: NO
```

Authoritative P09 negatives already in force (do not reopen):

- guest bone rotations/matrices valid
- scale bug `REFUTED`
- AUTO identity correct for bones 79, 100–105
- `VISIBLE_REQUIRED_MATCH_DROP_COUNT = 0`
- V5B `TOOIE_DIAG_FEATHER_NO_INTERP=1` on all 7 visible feather bones
- `FEATHER_GLITCH_WITH_NO_INTERP_V5B: SAME`
- SIMPLE / matrix interpolation `REFUTED_AS_PRIMARY_FIX` /
  `REFUTED_FOR_VISIBLE_FEATHER_CHAIN`
- CPU handoff of the 14 feather DLs `CORRECT`
- RSPProcess closed as target-local cause; RSPModify unused; RasterVS
  passive

Provenance of those negatives:
[[p09-kazooie-feather-v6-post-matrix-vertex-path]],
[[p09-kazooie-feather-v4-ab-static-audit]],
[[p09-kazooie-feather-v4-capture-analysis]],
[[p09-v5-real-call-match]],
[[../../reverse-engineering/systems/transform-interpolation]].

BanjoRecomp sources (checkout
`/home/wesley/projetos/N64-Recomp/BanjoRecomp`):

- `patches/transform_tagging.c`
- `patches/marker_transform_tagging.c`
- `patches/transform_ids.h`
- commits named in the task: PR #22 `8b9ce251…`, PR #23 `7e7d5d49…`,
  PR #26 `8d91c1d4…`, PR #46 `768de1bd…`, PR #117 `036aea9e…`

Tooie sources:

- `src/game/transform_hooks.cpp`
- `patches/transform_ids.h`
- `patches/patches.h`
- `patches/` (headers only; no `*_transform_tagging.c`)
- canonical RE: [[../../reverse-engineering/systems/geocmd]],
  [[../../reverse-engineering/systems/model-renderer]],
  [[../../reverse-engineering/systems/transform-interpolation]]

## 1. BanjoRecomp mechanism (`PROVEN_STATIC`)

### 1.1 Actor / model base ID

`cur_drawn_model_transform_id` is a process-wide current base. It is
**not** derived from GeoCmd. Callers set it around a draw:

| Caller | File | Base ID |
| --- | --- | --- |
| `player_draw` | `patches/marker_transform_tagging.c:21-37` | `BANJO_TRANSFORM_ID_START` (`0x10000000`) |
| `__marker_draw` | same file `:40-84` | `MARKER_TRANSFORM_ID_START + spawn_index * MARKER_TRANSFORM_ID_COUNT` |
| map / prop / HUD / particle / sky / projection | sibling `*_transform_tagging.c` | ranges in `patches/transform_ids.h` |

Each range reserves `MARKER_TRANSFORM_ID_COUNT` (256) slots so one
actor can own many bone/DL/sort IDs without colliding with another
actor.

If the base is `0` and interpolation is not globally skipped,
`set_model_matrix_group` returns `FALSE` and **no** `gEXMatrixGroup`
is emitted (`patches/transform_tagging.c:198-236`).

### 1.2 `cur_model_transform_id_offset`

Reset to `0` at the start of `modelRender_draw` geo walk
(`patches/transform_tagging.c:920-924`), immediately before
`func_80339124(..., model_bin + geo_list_offset_4)`.

It then advances **once per logical node that needs its own ID**,
independent of whether that node currently draws:

| GeoCmd | Handler | Offset action |
| --- | --- | --- |
| GeoCmd2 bone | `func_803387F8` | `++` then `cur_model_uses_bones = TRUE` (`:271-272`) |
| GeoCmd1 sort, `unk20 & 1` | `func_803385BC` | `++` before the single child (`:318`) |
| GeoCmd1 sort, forward | `func_803385BC` | `++` before child `unk22`, `++` before child `unk24` (`:336-342`) |
| GeoCmd1 sort, reverse visual | `func_803385BC` | same two increments, but **process** `unk22` then `unk24` while **drawing** reverse via `gSPBranchList` (`:373-411`) |
| GeoCmd0 billboard | `func_803384A8` | `++` (`:412`) |
| GeoCmd3 DL | `func_80338904` | **does not** increment; uses the current offset (or map geo-list byte offset) |

Non-map group ID:

```text
group_id = cur_drawn_model_transform_id + cur_model_transform_id_offset
```

Map models instead use geo-command byte offset from the geo-list base
(`set_model_matrix_group` `:204-206`) so IDs stay unique even without
the counter.

### 1.3 Where `gEXMatrixGroup` is created / pushed / popped

Push sites (all `G_MTX_MODELVIEW`):

1. **Model root** in `modelRender_draw` (`:891-905`): SkipAll, or
   DecomposedVerts (map without ExtraSegments verts), or
   DecomposedNormalTcs. Then geo walk. Pop at `:932-934`.
2. **Bone (GeoCmd2)** after `gSPMatrix PUSH|LOAD|MODELVIEW`
   (`func_803387F8` `:278-281`). Pop after `gSPPopMatrix` (`:292-295`).
3. **DL (GeoCmd3)** after an **identity** `gSPMatrix PUSH|MUL|MODELVIEW`
   (`func_80338904` `:250-251`). Pop after `gSPDisplayList` (`:258-261`).
4. **Billboard (GeoCmd0)** around the billboard matrix (`:425-436`).

`pop_model_matrix_group` is always `gEXPopMatrixGroup(..., MODELVIEW)`
(`:238-240`).

PR #23 lives inside `set_model_matrix_group`: if `cur_model_uses_bones`,
interpolation mode is `G_EX_INTERPOLATE_SIMPLE`; otherwise
`G_EX_INTERPOLATE_DECOMPOSE` (`:218-226`). Vertex/texcoord interpolate
only for maps that are not ExtraSegments.

### 1.4 What PR #22 actually solved

`func_803385BC` docstring (`:302-404`):

> process sorted geo commands in a consistent order while still drawing
> them in the original order. This allows a consistent ID scheme even
> when sorting changes.

Original BK GeoCmd1 chooses child order from a camera-facing sign
(`f14`). When `f14 < 0` the **visual** order is reversed. A naive
`++offset` in draw order would then assign last frame's child-A ID to
this frame's child-B.

PR #22:

1. Always increments the offset in **logical** child order (`unk22`
   then `unk24`).
2. When visual order is reversed, still **processes** `unk22` then
   `unk24` (so IDs and nested groups are allocated in that order).
3. Inserts three `gSPBranchList` commands so the **GPU command stream**
   still runs `unk24` then `unk22`.

That is a **logical-identity vs visual-order** split. It is not a
skinning fix and not a SIMPLE-vs-DECOMPOSE policy.

### 1.5 PR #26 / #46 / #117 (CPU skinning class)

`modelRender_draw` (`:852-867`) calls `recomp_apply_cpu_skinning` only
when `model_bin->unk28 != NULL` (or map float overrides). That path
uploads high-precision `gEXSetVertexSegment` position/velocity and sets
`cur_model_uses_ex_vertex`. PR #26 is that higher-precision buffer.
PR #46 is the velocity segment. PR #117 (`bkrecomp_setup_custom_skinning`
from `player_draw`) is Banjo form-specific CPU skinning (e.g. seal).

This class is ExtraSegments / `unk28` vertex skinning, **not** rigid
bones that submit ordinary `G_VTX` under a bone `G_MTX`.

## 2. Tooie 0x62B renderer (`PROVEN` structure; layouts are **not** BK copies)

Do **not** treat BK GeoCmd byte layouts as Tooie layouts.

### 2.1 Draw entry

| BK | Tooie | Proof |
| --- | --- | --- |
| `modelRender_draw` | `func_800DE448` → `func_800DE498` | [[../../reverse-engineering/systems/model-renderer]] `PROVEN` |
| `func_80339124` geo walk | `func_800DE2A4` sibling walker, jalr `*(D_8012CF8C + type*4)` | [[../../reverse-engineering/systems/geocmd]] `PROVEN` generated C |
| geo list at `model_bin->geo_list_offset_4` | `model_bin + *(u32*)(model_bin+4)` | live `0x62B` root `+0x119F0` `PROVEN_RUNTIME` [[p09-live-0x62b-tree-reachability]] |

Player 0x62B submit (`PROVEN`):

```text
func_800E9F20 → func_800F5008 / func_800F50D0
  → func_80098BA8 → func_80092444
    → func_800DE448   // 5th arg = model_bin
```

### 2.2 GeoCmd type equivalences (must be proven, not assumed)

| BK command | BK layout | Tooie live `0x62B` | Tooie handler | Equivalence |
| --- | --- | --- | --- | --- |
| GeoCmd2 bone | `unk8` = child **byte offset**; `unk9` = bone index | compact **16-byte** header: `lw(+0)=2`, `lw(+4)=sibling continue`, `lbu(+8)=0x10` **header size**, `lbu(+9)=INDEX`, `lh(+0xA)=boneAnimID`, children at `+0x10` | `func_800DCF48` (`PROVEN_RUNTIME` DE2A4 last type-2 = `0x800DCF48`) | **same job** (push bone MTX, walk children, pop). **different layout**. BK `unk8` as child offset would mis-parse Tooie `+8==0x10`. |
| GeoCmd3 DL | `unk8` = index into `modelRenderDisplayList`; identity MTX + **new group** + `gSPDisplayList` | type `0x11` `lh(+8)` = DL index; `G_DL` only. Type `0x18` = same `G_DL` plus trailing restore `G_MTX`. Join `G_POPMTX` lives **inside** the DL. | `func_800DD65C` / `func_800DD998` | **same job** (submit a model DL under the current bone). **not** BK type 3, **no** identity MTX, **no** per-DL group. Type `0x18` join is Tooie-specific. |
| GeoCmd1 sort | two children `unk22`/`unk24`, camera-facing `f14`, `unk20` flags | **not present** on the documented live `0x62B` player/Kazooie tree (types on snout/plume/crest: `0x02`, `0x0A`, `0x11`, `0x18`, `0x0C`) | none identified on that tree | **NOT_APPLICABLE** to the 14 feather DLs and the live snout. A Tooie sort type may exist on other models (`UNKNOWN` globally). |
| GeoCmd0 billboard | camera yaw/pitch wrap | not required for 0x62B feathers | — | `IRRELEVANT_TO_TARGET` |

Kazooie visible feather bones and DLs (`PROVEN` `STATIC_ROM`
[[../../reverse-engineering/systems/geocmd]],
[[../../reverse-engineering/characters/kazooie]]):

```text
model 0x62B
right plume 107 → 103 → 104 → 105
left  plume 107 → 100 → 101 → 102
crest       108 → 79
DLs: 0x4D8,0x4F0,0x508,0x520,
     0x534,0x54C,0x564,0x57C,0x592,0x5AA,
     0x5BE,0x5D6,0x5EE,0x606
     (runtime gfx indices for left/crest also 0x5C0/0x5D8/…/0x4A6/0x4B0)
```

Bone path (`PROVEN_RUNTIME` for snout; `STRONG_INFERENCE` same class for
feathers, later V4/V5B `PROVEN_RUNTIME` for 79, 100–105):

```text
func_800DCF48
  func_800ADE2C(anim_mtx_list, INDEX)
  func_80019AA0(parent, local) → mlMtxApply / guMtxF2L
  G_MTX PUSH|LOAD|MODELVIEW   // DA380002, SINGLE matrix
  func_800DE2A4(node + lbu(+8))
  G_POPMTX
```

No `gEXMatrixGroup` on this path
([[../../reverse-engineering/systems/transform-interpolation]]
`PROVEN_STATIC` absence of `set_model_matrix_group`).

### 2.3 What Tooie *does* have that looks like BK tagging

| Artifact | Role in Tooie |
| --- | --- |
| `patches/transform_ids.h` | Byte-copy of BK ID ranges. `extern cur_drawn_model_transform_id` is **never defined or written** by Tooie C/C++ (`PROVEN` repo search: only this header). |
| `patches/patches.h` `gEXMatrixGroup*` macros | Header-only. **No** Tooie `.c`/`.cpp` emits `gEXMatrixGroup` / `gEXPopMatrixGroup`. `RECOMP_PATCH` is defined and unused. `patches/` contains **no** `transform_tagging.c`. |
| `src/game/transform_hooks.cpp` | `func_800E44FC` / `func_800E4640` **projection split** (P then V). Not model/bone tagging. |

Live 0x62B world transforms therefore stay on RT64 default
`matrixId = G_EX_ID_AUTO` with AUTO component/decompose/order
([[p09-kazooie-feather-v4-ab-static-audit]], V4/V5 runtime).

## 3. Equivalence table

| BK mechanism | Tooie equivalent | Status | Evidence |
| --- | --- | --- | --- |
| Per-actor/model base transform ID (`player_draw` / `__marker_draw`) | none | **ABSENT** | no writer of `cur_drawn_model_transform_id`; header-only extern |
| `cur_model_transform_id_offset` reset + per-bone/sort/billboard increment | none | **ABSENT** | no symbol in Tooie sources |
| GeoCmd2 `gEXMatrixGroup` around bone `G_MTX` | `func_800DCF48` single `G_MTX` only | **ABSENT** (push/pop of **GBI matrix** is PRESENT; **group** is ABSENT) | DCF48 vs `func_803387F8` |
| GeoCmd3 identity MTX + per-DL group | type `0x11`/`0x18` `G_DL` under parent bone | **ABSENT** as group; **PARTIAL** as DL submit | `func_800DD65C` / `func_800DD998` |
| GeoCmd1 logical-order ID + visual-order branch (PR #22) | no proven sort node on 0x62B target tree; no rewriter anywhere | **ABSENT** globally; **NOT_APPLICABLE** to 0x62B feathers/snout | geocmd 0x62B type census; no `func_803385BC` analog |
| PR #23 SIMPLE iff `cur_model_uses_bones` | AUTO decompose + AUTO components | **ABSENT** as explicit policy | V4 static group decode |
| Root `gEXMatrixGroup` in `modelRender_draw` | none | **ABSENT** | `func_800DE448`/`DE498` have no EX group |
| CPU skinning `unk28` + `gEXSetVertexSegment` (PR #26/#46/#117) | ExtraSegments `0x61F` is **not** live walk body; 0x62B `gtype=0` GeoCmd | **NOT_APPLICABLE** to 0x62B feather DLs | live resource 0x62B; ExtraSegments analog `CORRECTED` |
| Projection tagging (`camera_transform_tagging.c`) | `transform_hooks.cpp` P/V split, not group IDs | **PARTIAL** / different problem | P08 projection work; out of P09 bone scope |

## 4. Answers A–F

### A) Stable logical matrix-group identity per bone/DL/sort (PR #22)?

**ABSENT.** Tooie has no base ID, no offset counter, no GeoCmd1
logical-order allocator, and no per-bone/per-DL `gEXMatrixGroup`.
RT64 identity for 0x62B is `G_EX_ID_AUTO` plus same-position matching
inside hash-matched draw calls
([[../../reverse-engineering/systems/transform-interpolation]]).

That is a real architectural gap. It is **not** a proven P09 visual
root (see D, E, and V5B).

### B) Does Tooie depend on RT64 AUTO where BK uses explicit groups?

**YES** on the live 0x62B bone path. BK `func_803387F8` emits an
explicit group after each bone `G_MTX`. Tooie `func_800DCF48` does not.
V4/V5/V5B already measured that AUTO matching for bones 79, 100–105
is `CORRECT` (`VISIBLE_REQUIRED_MATCH_DROP_COUNT=0`).

### C) Sort-node / order-dependent identity issue that survives V5B/V7?

**NO** for the P09 0x62B feather/snout target.

- Documented 0x62B plume/crest/snout nodes are `0x02` / `0x11` / `0x18`
  / `0x0A` / `0x0C`, not BK GeoCmd1.
- If AUTO **slot** reassignment plus interpolation were the wobble,
  V5B (current guest matrix, interpolation bypassed, masks `0x7F`,
  `RESULT==CURRENT` 211/211) would have changed the picture.
  User visual: `SAME`.
- V7 closed CPU handoff / join / face / workload association for the
  14 DLs independently of group IDs.

A global Tooie sort type, if it exists on **other** models, remains
`UNKNOWN` and is outside this target.

### D) Frame-to-frame group-identity reassignment despite AUTO witness CORRECT?

**Not as a surviving P09 cause.** AUTO never uses the explicit-ID map;
pairing is call-hash + range position. That *could* in principle swap
slots when draw order changes. The V4/V5 witnesses already classified
real unmatched rows as one-to-one allocator collisions, not
`MATRIX_ID_MISMATCH`. V5B then displayed `CURRENT` for every covered
feather transform. Remaining wobble is therefore not “wrong previous
matrix under a stolen ID”.

A still-open **non-interpolation** AUTO effect (for example a later
presentation pass that is not V5B-covered) is `UNKNOWN` and is **not**
supplied as a BanjoRecomp-ready fix.

### E) Does PR #22 affect anything V5B would not have refuted?

PR #22’s purpose is stable IDs so **temporal interpolation** pairs the
same logical node across frames when visual sort flips. V5B turns that
interpolation off for the seven visible feather bones and the glitch
stays. Secondary PR #22 effects (call-hash xor of explicit IDs, per-DL
identity groups from GeoCmd3) would also change interpolated results
or AUTO slot occupancy; they do not rewrite guest current matrices,
vertex provenance, or CPU handoff, all of which are already `CORRECT`.

Classification for P09 visible feathers/snout:
`ALREADY_REFUTED_BY_P09` as primary cause.

Do **not** propose “port PR #22” or “use SIMPLE interpolation” as the
next fix.

### F) Are PR #26 / #46 / #117 applicable to the feather DLs?

**NO.** The 14 feather DLs are rigid-bone F3DEX2 geometry: type
`0x11`/`0x18` `G_DL` → `G_VTX` under the current DCF48 bone matrix,
including Tooie-specific join `G_POPMTX` inside type `0x18`. They are
not ExtraSegments / `unk28` CPU-skinned vertices. Live walk body is
GeoCmd `0x62B` (`gtype=0`), not `0x61F`. `velFloats=0` on F3DEX2
(`PROVEN_STATIC`). High-precision skinned position/velocity segments
are the wrong class.

## 5. Concrete structural differences (not “BK has more patches”)

1. **Tooie compact type-2 header is not BK GeoCmd2.** Child walk is
   `node + lbu(+8)` with `+8` = header size `0x10`, not a BK-style
   child offset. A literal port of `func_803387F8` onto Tooie bytes
   would mis-walk 0x62B.
2. **Tooie emits no `gEXMatrixGroup` on the bone or DL path.** BK
   wraps bone, DL, billboard, and model root. Tooie 0x62B is ungrouped
   AUTO `G_MTX` + inherited DLs.
3. **Tooie type `0x18` join DLs pop inside the Gfx and restore with a
   same-depth `G_MTX`.** BK GeoCmd3 does not. This is Tooie-specific
   and already V7-closed for CPU handoff.
4. **No GeoCmd1 sort rewriter** exists in Tooie, and the 0x62B feather
   tree does not use that command class.
5. **CPU skinning / ExtraSegments** is a different Tooie model class
   (`gtype` bit `0x40`), not live 0x62B feathers.

## 6. Causal classification

| Difference | Class | Why |
| --- | --- | --- |
| Missing explicit bone/DL `gEXMatrixGroup` (AUTO vs BK tagged) | `ALREADY_REFUTED_BY_P09` | V4 AUTO `CORRECT`; V5B NO_INTERP `SAME` |
| Missing PR #22 sort-stable IDs | `ALREADY_REFUTED_BY_P09` for interpolation; `IRRELEVANT_TO_TARGET` on 0x62B feathers (no sort node) | V5B; type census |
| Missing PR #23 SIMPLE-on-bones | `ALREADY_REFUTED_BY_P09` | root SIMPLE `SAME`; V5B full chain `SAME` |
| Missing PR #26/#46/#117 CPU skinning | `IRRELEVANT_TO_TARGET` | rigid-bone DLs, not `unk28` |
| Compact type-2 vs BK GeoCmd2 layout | `IRRELEVANT_TO_TARGET` (port hazard, not a missing fix) | DCF48 already walks 0x62B correctly |
| Type `0x18` join | `ALREADY_REFUTED_BY_P09` as tagging gap | V7 join roles `CORRECT` |
| Header-only `transform_ids.h` / unused macros | `IRRELEVANT_TO_TARGET` | dead BK copy, not a runtime path |
| `transform_hooks.cpp` P/V split | `IRRELEVANT_TO_TARGET` | projection, not bone groups |
| Non-interpolation AUTO side effects after V5B (presentation) | `UNKNOWN` | not a BanjoRecomp-ready tagging port; not justified as PR #22 |

No difference remains `POSSIBLY_CAUSAL` as a BanjoRecomp tagging port.

## 7. Experiment

None. There is no surviving `POSSIBLY_CAUSAL` tagging delta that a
default-off CPU counter on 0x62B feathers would discriminate better
than V4/V5B already did.

Do not add a production or diagnostic `gEXMatrixGroup` port in the
next turn.

## 8. If no surviving causal port

BanjoRecomp does **not** contain a ready architectural fix for P09.
The missing tagging is real and would still be needed later for a
faithful high-FPS interpolation port, but it does not explain the
current movement-coupled wobble after V5B.

Next unclosed assumption (already the operational `NEXT_SINGLE_ACTION`,
not reopened here): the remaining user-visible motion may be the
**authored** clip-22 bone 107 scale ramp and 108 nod versus original
N64/Angrylion, not a Tooie renderer identity bug.
[[p09-animation-clip-transition-and-authored-key-forensic]].

Do not call GPU bug. Do not GPU-readback. Do not start a tagging
implementation.

## Required verdict

```text
BANJORECOMP_READY_FIX_FOR_P09: NO
PR22_EQUIVALENT_IN_TOOIE: ABSENT
EXPLICIT_MATRIX_GROUP_GAP: YES
SORT_IDENTITY_GAP: NO
CPU_SKINNING_APPLICABLE_TO_FEATHER_DLS: NO
SURVIVING_CAUSAL_DIFFERENCE: NONE
MINIMAL_NONVISUAL_EXPERIMENT: NONE
VISUAL_RECORDING_REQUIRED_NOW: NO
NEXT_RECOMMENDED_MODEL: Luna
```

`SORT_IDENTITY_GAP: NO` means: no surviving sort-identity issue on the
P09 0x62B feather/snout target. The PR #22 rewriter is still
architecturally `ABSENT` in Tooie (`PR22_EQUIVALENT_IN_TOOIE: ABSENT`).

`EXPLICIT_MATRIX_GROUP_GAP: YES` is the architectural fact. Its P09
causal status is `ALREADY_REFUTED_BY_P09`, not a license to implement.
