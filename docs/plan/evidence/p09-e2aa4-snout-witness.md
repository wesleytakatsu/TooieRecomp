---
title: "P09 true 0x62B span and E2AA4 snout witness"
description: "Allocator span 0x20000 was not the 0x62B asset. Snout identity moves to func_800E2AA4. Generic E2180 discovery demoted."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-02
tags: [tooie-recomp, p09, snout, geocmd, e2aa4, e2180, runtime-cpu]
---

# P09 true 0x62B span and E2AA4 snout witness

CPU/static only. No game run this round. No production fix, commit, or
push.

## Span bug (`PROVEN` vs latest capture)

Registered `model_bin=0x80226130`. Decompressed `0x62B` size `0x13178`.
True range `[0x80226130, 0x802392A8)`. Expected snout
`a1=0x80226130+0x11CB8=0x80237DE8`.

The diagnostic used `rel < 0x20000` (`INFERRED_RANGE` / not asset size).
The only “inside” E2180 tuple was `a1=0x80241E50`, `rel=0x1BD20`,
`WORD0=0x000A0004`. That pointer is **past** `0x802392A8`.
`E2180_NODE_INSIDE_0X62B_COUNT=216` did **not** test true asset
boundaries. `OLD_MODEL_SPAN_BOUNDARY_CORRECT: NO`.

No retained E2180 tuple was near `0x80237DE8`
(`NEAREST_OFFSET_DELTA=0xA068`).

## E2AA4 (`PROVEN` generated C)

`a0` Gfx packet, `a1` raw GeoCmd node (saved at `0x5C($sp)`), `a2`
forwarded. `LOOKUP_FUNC(0x800E2180)` with the same `a1`. Walker
`func_800DE2A4` jalr is `LOOKUP_FUNC(handler)`; `--wrap` alone does not
see type-2 dispatch. `add_loaded_function(0x800E2AA4)` is required.

`0x62B` snout first 16 bytes:
`0000000200000050100b002a00000000`. Guest `word(+0)==2`,
`byte(+9)==11`, `hword(+0xA)==42`.

Walker path (sibling/nesting via stride): neck GeoCmd02 `@0x11C40`
INDEX 9 anim 18 stride `0x288` contains snout `@0x11CB8` INDEX 11 anim
42. Handler table index is GeoCmd type (`2` → `func_800E2AA4`).

Previous/current: same bone, two temporal Mtx addresses
(`AF5D8=node+lh(+0x12)`, `AF5E4=node+lw(+0x14)`). `AF5FC=node+a2*12+0x1C`
child/DL slot. Matrices are readable at E2AA4 from those fields.

## Diagnostic

`TOOIE_DIAG_SECONDARY_SWAY=1` (default off):

- true asset end = `model_bin + 0x13178`
- primary seam `func_800E2AA4`
- accept `a1 == bin+0x11CB8` and type 2 / INDEX 11 / bone 42
- tiny TLS only around that accepted call into wrapped E2180
- generic E2180 discovery `DEMOTED`

Tagging remains `HYPOTHESIS`. `ROOT_CONFIDENCE: LOW`.

`GPU_READBACK_USED: NO`
