---
title: "Structs / offsets"
description: "Seeded field map. Only offsets with evidence."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-09-02
tags: [reverse-engineering, structs]
---

# Structs / offsets

## Model header

| Offset | Field | Status |
| --- | --- | --- |
| `+0x00` `u32` | magic `0xB` | `PROVEN` |
| `+0x04` `u32` | GeoCmd list offset (`model_bin + this` is DE2A4 `a1`) | `PROVEN` `func_800DE498` |
| `+0x0A` `u16` | gtype; bit `0x40` ExtraSegments | `PROVEN` |
| `+0x0C` `u32` | vertex/DL region offset (not the GeoCmd root on `0x62A`/`0x62B`) | `STRONG_INFERENCE` |
| `+0x18` `u32` | anim/bone setup offset | `PROVEN` |

Setup `+4` `u16` = bone count (`PROVEN`).

Tooie ExtraSegments is **not** BK `model_bin->unk28` at `+0x28`
(`0x61F` `+0x28` is zero). `CORRECTED` vs BK layout.

## Bone record (stride `0x10`)

| Offset | Field | Status |
| --- | --- | --- |
| `+0x0` `f32[3]` | rest xyz | `PROVEN` |
| `+0xC` `u16` | boneAnimID | `PROVEN` |
| `+0xE` `s16` | parentIndex | `PROVEN` |

## GeoCmd `0x02` bone

| Offset | Field | Status |
| --- | --- | --- |
| `+0x00` `u32` | command type (walker index; `2` here) | `PROVEN` `STATIC_ROM` + `func_800DE2A4` |
| `+0x04` `u32` | sibling continue (`0` ends that sibling list; children may still exist) | `PROVEN` |
| `+0x08` `u8` | compact header size (`0x10` on `0x62B`) | `PROVEN` `func_800DCF48` `lbu` |
| `+0x09` `u8` | bone INDEX | `PROVEN` |
| `+0x0A` `u16` | boneAnimID | `PROVEN` |

On live `0x62B` this is a **compact** 0x10-byte header. Children start at
`node+0x10`. Sibling continue `word(+4)` is walked **inside** `func_800DE2A4`;
the snout at `+0x11CB8` is a sibling, not a list head. Do **not** overlay
fat dual-mtx slots `+0x12`/`+0x14` on that header.

Decompressed `0x62B` extent is `0x13178`. Do not treat heap/allocator
size as the model byte range.

## Player draw object (overlay 118 `80817180` `s0`)

| Offset | Field | Status |
| --- | --- | --- |
| `+0` | pointer passed to `func_801039E4` | `PROVEN` `STATIC_MIPS` |
| `+4` | position (DE448 `a0`) | `PROVEN` |
| `+0x44` | DE448 `a1` | `PROVEN` |
| `+0x7A` `u16` | render branch (`>>14 == 3` vs other) | `PROVEN` as branch field; locomotion meaning `UNKNOWN` |

## PlayerState kinematics and state struct (`*(player + 0x120)`)

| Offset | Type | Field | Status |
| --- | --- | --- | --- |
| `+0x00` | `s32` | `bs_prev` (previous state) | `PROVEN` `funcs_350.c` / `0x8009E5C8` |
| `+0x04` | `s32` | `bs_cur` (current state) | `PROVEN` `funcs_350.c` / `0x8009E5C8` |
| `+0x08` | `s32` | `bs_next` (next/pending state) | `PROVEN` `funcs_350.c` / `0x8009E5C8` |

- State 116 (`0x74`): `bsrest` overlay 84 (ROM `0x01F33AF0`, RAM `0x80800000`).
- `bs_setState` entry point is `0x8009E5C8` (`0x8009E5D8` was an interior instruction).

## Spatial candidate list entry (stride `0x0C` / 12 bytes)

Used by `func_800CD820` swept sphere candidate query:

| Offset | Type | Field | Status |
| --- | --- | --- | --- |
| `+0x00` | `u32*` | marker pointer (e.g. `0x80132f60`) | `PROVEN` `func_800CD820` |
| `+0x04` | `u32` | candidate link / metadata | `PROVEN` |
| `+0x08` | `u32` | candidate link / metadata | `PROVEN` |

- Stride is exactly 12 bytes.
- Note: The first entry in the candidate list is **NOT** necessarily the first qualifying candidate.

## Collision marker header (`0x80132f60` candidate)

Candidate marker object passed to collision query and resolution:

| Offset | Type | Field | Status |
| --- | --- | --- | --- |
| `+0x10` | `u32` | descriptor / link word (observed `0x00180D74`) | `PROVEN` |
| `+0x14` | `u16` | marker class ID type 1 (observed `0x0671`) | `PROVEN` |
| `+0x18` | `u16` | marker class ID type 2 (observed `0x0E3F`) | `PROVEN` |
| `+0x1A` | `u16` | actor index word (`index = lhu(+0x1A) >> 5`) | `PROVEN` `func_80106790` |
| `+0x28` | `u32` | state word (observed `0x0E000202` / `0x0E800202` / `0x4E800202`) | `PROVEN` (bit 30 meaning UNKNOWN) |

- Specific class name corresponding to IDs `0x0671` / `0x0E3F` remains `UNKNOWN`.

## Global actor vector component record (`*D_80136EE0`, stride `0x9C`)

Array record resolved by `func_80106790` via `vector_at(*D_80136EE0, lhu(marker+0x1A) >> 5)`:

| Offset | Type | Field | Status |
| --- | --- | --- | --- |
| `+0x04` | `f32` | bound center X | `PROVEN` `8008A018` (when descriptor bit 1 SET) |
| `+0x08` | `f32` | bound center Y | `PROVEN` `8008A018` (when descriptor bit 1 SET) |
| `+0x0C` | `f32` | bound center Z | `PROVEN` `8008A018` (when descriptor bit 1 SET) |
| `+0x10` | `u32` | pointer to getter function returning descriptor (invoked via `func_80100368` `jalr *(a0+0x10)`) | `PROVEN` `func_80100368` |

- Record stride is exactly `0x9C` (156 bytes).
- Resolved by `vector_at(vec, idx)`: `begin = *(vec + 4) + idx * *(vec)`.
- `+0x10`: Pointer to getter function returning descriptor (e.g. `chdiggerground` getter `80085C98` returning descriptor `D_80800408`), NOT pointer to descriptor. Caller `800FF62C` invokes `func_80100368` (`jalr *(record+0x10)`) to get the descriptor, then calls returned `desc+0xC` / `+0x10` for the actual actor update.
- Center calculation: `_subaddiecustomhits_entrypoint_5` (`0x8008A018`) checks descriptor word `*(*(*a0) + 8)`. If bit 1 is SET, copies `+4/+8/+0xC` floats directly; else calls `func_800EC75C` custom offset calculation.
- `+0x60`: Raw 32-bit word observed differing between Current (`0x0004006c`) and Oracle (`0x0003006c`) at first sampled state PRE80. Semantic role (flags, counter, state, or timer) is strictly `UNKNOWN`; do not promote to a field role without owning source evidence.
- Epistemic note: Relation of bound center to entity base/world position is `UNKNOWN` pending descriptor bit 1 branch resolution.

## Renderer processing context (`func_800DE498` `s0`)

Written conditionally upon visibility accept (`v0 != 0`) inside `func_800DE498`:

| Offset | Type | Field | Status |
| --- | --- | --- | --- |
| `+0x14` | `u32` | renderer processing context word | `PROVEN` `funcs_326.c` |
| `+0x18` | `u32` | renderer processing context word | `PROVEN` `funcs_326.c` |
| `+0x1C` | `u32` | renderer processing context word | `PROVEN` `funcs_326.c` |

- **Critical Epistemic Boundary**: `s0` is the **renderer processing context**, NOT an actor state structure. These stores are renderer context writes, NOT actor state mutations (`PROVEN` distinction; do not conflate).

## Global renderer callback and visibility registers

| Address | Type | Name / Role | Status |
| --- | --- | --- | --- |
| `0x8012C940` | `u32` | visibility test result (`v0` from `func_800E3E8C`) | `PROVEN` `func_800DE498` |
| `0x8012C9A8` | `u32` | registered pre-draw callback function pointer (e.g. `func_801015D0`) | `PROVEN` `func_800DF47C` / `func_800DE498` |
| `0x8012C9AC` | `u32` | registered pre-draw callback context argument (`Actor*` pointer) | `PROVEN` `func_800DF47C` / `func_800DE498` |
| `0x8012C850` | `u32[]` | global model/texture animation selector array (indices 1 and 2 updated via `func_800DF744`) | `PROVEN` `func_80101A50` |

- Set via `func_800DF47C(callback, context_arg)` in `func_80101808` prior to model drawing.

## Actor struct pre-draw fields (`Actor*`)

Fields accessed during actor pre-draw traversal (`func_801015D0` -> `func_80101934` -> `func_80101A50`):

| Offset | Type | Field | Status |
| --- | --- | --- | --- |
| `+0x74` | `u32` | pre-draw state index word; `unk74 & 0xF` indexes jump table `D_801243E8` | `PROVEN` `func_80101934.s` |
| `+0x78` | `u32` | animation control word; checked by `func_80101A50` | `PROVEN` `func_80101A50.s` |
| `+0x7A` | `u8` | animation state byte; mutated conditionally via `sb $t2, 0x7A($a2)`: `(byte & 0xFFCF) \| 0x10` or `0x20` | `PROVEN` `func_80101A50.s` |

- **Epistemic Classification**:
  - The byte mutation at `+0x7A`, control flow gating, and global LCG RNG consumption (`func_800DC0C0`) on random test `$f0 < 0.03f` are **`PROVEN`**.
  - Semantic role as facial/eye-blink animation is **`STRONG_INFERENCE`** (suggested by 3% probability check and selector indices 1/2, but without explicit asset/animation binding proof in source).
