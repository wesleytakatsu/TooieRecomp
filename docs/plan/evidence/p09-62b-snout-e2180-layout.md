---
title: "P09 0x62B snout layout and E2180 argument semantics"
description: "Independent parse of asset 0x62B. Snout GeoCmd02 is 0x11CB8 not 0x14960. E2180 a1 is the GeoCmd node. Split-reject witness prepared."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-02
tags: [tooie-recomp, p09, snout, geocmd, e2180, runtime-cpu]
---

# P09 0x62B snout layout and E2180 argument semantics

CPU/static only. No game run this round. No production fix, commit, or
push. Artifact: `/tmp/p09-62b-snout-e2180-layout-*`.

## Asset `0x62B` (`PROVEN` `STATIC_ROM`)

Cart table `0x5188+id*4`, payload `0x12B24+(entry>>8)*4`, skip-2 raw
deflate. Flag `0x10`. Decompressed size **78200** (`0x13178`). Magic
`0000000B`, `gtype=0`, bone count **66**, GeoCmd list at header `+0x04`
= `0x119F0`.

`0x14960` is **past the end** of this blob. That offset is the `0x62A`
snout (89792 bytes, 68 bones, GeoCmd02 INDEX 10).

## Snout identity on `0x62B` (`PROVEN` geometry)

Bone table: anim 42 INDEX **11**, parent INDEX 9 (anim 18 neck), rest
`(0, 99.98, 34.86)`. Same muzzle translation as `0x62A` INDEX 10.

GeoCmd `0x02` node at **`0x11CB8`**, size `0x50`, `+9=11`, `+0xA=42`.
Following commands include type `0x11` DLs then type `0x18` terminator.
Path class is GeoCmd02 → `func_800E2AA4` → `func_800E2180` dual-mtx
(`PROVEN` membership of that node class; live E2180 a1 still unproven
until the repaired witness runs).

## E2180 arguments (`PROVEN` generated C)

| Arg | Register | Semantics |
| --- | --- | --- |
| 0 | `a0`/`r4` | Gfx packet pointer (`s0` in E2AA4) |
| 1 | `a1`/`r5` | GeoCmd node (E2AA4/E2A3C/E2C54/E2E48 pass their `a1`) |
| 2 | `a2`/`r6` | forwarded to `func_800AF5FC` as child/DL index |

`CURRENT_WORD0_EQ_2` and `HWORD(+0xA)==42` are correct **if** `a1` is
that raw GeoCmd02 node. DE498 starts DE2A4 at `model_bin + *(model_bin+4)`,
so walker nodes are **inside** `model_bin`. Span filtering is valid for
that path; it is still possible other E2180 callers pass a derived
object. The previous capture's aggregate `WRONG_NODE` cannot say which
predicate failed.

Previous/current slots (`PROVEN`):

- current: `func_800AF5E4` = `a0 + lw(a0+0x14)`
- previous: `func_800AF5D8` = `a0 + lh(a0+0x12)`

ROM placeholders on the `0x62B` snout node are inside the 0x50-byte
command, not a full `Mtx`. Runtime may rewrite them.

## Diagnostic repair

`TOOIE_DIAG_SECONDARY_SWAY=1` (default off):

- retired `0x14960`
- accept in-span `rel==0x11CB8` **or** type 2 + boneAnimID 42 **or** type 2 + INDEX 11
- split reject counters + bounded `/tmp/tooie-secondary-sway-e2180-tuples.csv`

Tagging remains `HYPOTHESIS`. `ROOT_CONFIDENCE: LOW`.

`GPU_READBACK_USED: NO`
