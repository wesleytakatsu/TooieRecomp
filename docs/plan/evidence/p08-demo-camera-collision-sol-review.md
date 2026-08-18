---
title: "P08 — Independent review of the alleged camera collision differential"
description: "Original-MIPS arbitration that refutes the func_800C4C34 collision interpretation."
status: current
type: evidence
project: TooieRecomp
tags: [p08, camera, projection, mips, differential, arbitration]
---

# P08 — Independent review of the alleged camera collision differential

- Date: 2026-08-18
- Model: GPT-5.6 Sol X-HIGH
- Base HEAD: `bd2181f6268e70cf8434618ed1b673efe1309ada`

## 0. Verdict

**REFUTED (ROOT_CAUSE_LEVEL R).** The measured Original/Recomp return
difference is real, and its downstream counter/control-flow consequence is
real, but it is not a collision-query result. `func_800C4C34` performs
camera-space transform, perspective projection, and viewport/depth clipping.
It does not read collision world state or iterate collision candidates.

The previous `q_hit_v0`, CLEAR/HIT, collision primitive, candidate-set, and
ray-triangle interpretation is **CORRECTED/SUPERSEDED**. No production fix was
made.

## 1. Reviewed evidence and alignment

- `MEASURED_ORIGINAL`: `/tmp/p08-clearance-original.csv`.
- `MEASURED_RECOMP`: `/tmp/p08-clearance-recomp.csv`.
- Original MIPS: `build/us/banjotooie_decompressed.elf`, disassembled over
  `0x800C4C34..0x800C4E54` (137 instructions).
- Generated path: read-only `RecompiledFuncs/funcs_34.c` and
  `RecompiledFuncs/funcs_71.c`.

The relevant first invocation at consumed glidx 70 is ordered identically:
Q0, Q1, Q2, Q3. Q1 and Q2 return `0` in Original and `1` in Recomp at that
first measured invocation. The same persistent difference later drives the
counter threshold at glidx 384. Thus 384 is the first visible camera effect,
not the first measured return divergence.

## 2. Original-MIPS contract of `func_800C4C34`

The original function runs from `0x800C4C34` through the delay slot at
`0x800C4E54`. Its helper chain is:

```text
800A8984  select the slot's point/transform source
800CA7E4  copy a vec3
800EFB24  subtract vec3s (camera-relative coordinates)
800CA7AC  select matrix at source + 0xA4
800F23D0  transform vec3 by matrix
800A8A04  obtain viewport integer bounds/offsets
800CA79C  read camera float at +0xF4
800CA6FC  read camera float at +0xE8
```

It then:

1. classifies transformed depth against `D_80125894` and writes the result
   through `a3`;
2. computes projected screen coordinates with single-precision MIPS
   arithmetic;
3. checks the projected pair against viewport bounds;
4. returns `v0 = 1` only for an in-bounds projected point and applies viewport
   offsets to the output pair; otherwise it returns `0`.

There is no collision root/list pointer, spatial cell, object list, triangle,
material/mask test, candidate loop, or ray/triangle helper. Original MIPS and
the generated call targets agree throughout this path.

## 3. `func_800E8918` return arbitration

`func_80113D30` calls `func_800E8918` at `0x80113EE0`, stores that function's
return byte at `0x80113EF0`, and resets/increments each `word_3C` counter based
on whether the byte is nonzero/zero. This counter logic and the later
`half_0C` consequence remain **PROVEN**.

However, `func_800E8918` does not forward `func_800C4C34`'s `v0`. It selects a
persistent 56-byte projection record, calls `func_800C4C34` internally, may
update record fields, and finally returns:

```text
0x800E8A10: lh v0, 0x34(s0)
```

Therefore the CSV column `q_hit_v0` records the selected projection record's
signed halfword at `+0x34`. Calling `0` CLEAR and `1` HIT was an unsupported
semantic promotion.

Static search identifies `func_800E82EC` as the relevant family writer
candidate: it stores multiple classifications to `s0+0x34` over
`0x800E851C..0x800E866C`. Dynamic object identity and the first differing
writer input have not yet been captured, so this is the next discriminator,
not a proven root.

## 4. Query-coordinate sensitivity review

At glidx 384, Q1 Recomp minus Original is:

```text
start delta = (+0.010254, +0.000122, +0.054199), norm 0.055160593
dir delta   = (+0.000000, +0.000000, +0.000007), norm 0.000007000
dest delta  = (+0.010254, +0.000122, +0.054199), norm 0.055160593
max component delta = 0.054199
```

These inputs are close but not bit-identical. A collision-boundary margin is
not applicable because no collision primitive is evaluated. The preserved
data also do not isolate whether this small coordinate delta, camera/matrix
state, viewport state, or earlier persistent-record state first changes
`record+0x34`. A controlled projection-state replay would be meaningful; the
requested OW/OQ collision decomposition would not.

## 5. Requested collision arbitration fields

```text
RECOMP_HIT_PRIMITIVE:                 NONE
SAME_LOGICAL_TRIANGLE_PRESENT_ORIGINAL: UNKNOWN / NOT APPLICABLE
CANDIDATE_SET_MATCH:                  UNKNOWN / NOT APPLICABLE
CANDIDATE_ORDER_MATCH:                UNKNOWN / NOT APPLICABLE
COLLISION_WORLD_STATE_MATCH:          UNKNOWN / NOT CONSULTED
Q1_NEAREST_COLLISION_MARGIN:          NOT APPLICABLE
OW_OQ / OW_RQ / RW_OQ / RW_RQ:        NOT TESTED (invalid collision model)
FIRST_INTERSECTION_SCALAR_DIFFERENCE: NOT APPLICABLE
CALL_TARGET_CHAIN_MATCH:              YES
```

Q3's glidx-384 return is zero while its counter remains below threshold due to
the prior nonzero glidx-383 return; the historical-counter observation is
confirmed. Q2 follows the same non-collision projection-record path, although
its exact upstream producer difference remains untraced.

## 6. Root classification and next discriminator

```text
ROOT_CAUSE_LEVEL:       R
ROOT_CAUSE:             collision-helper attribution refuted; first preserved
                        divergence is selected func_800E8918 record +0x34
FIX_OWNER_CANDIDATE:    camera projection/visibility record subsystem (pending)
FIX_IMPLEMENTED:        NO
```

`NEXT_SINGLE_ACTION`: on the first relevant glidx-70 invocation, match the
selected `func_800E8918` record semantically in Original and Recomp (slot,
record base/index, camera/matrix inputs), then trace every store to that exact
record's `+0x34` halfword. Compare the first writer event and its inputs in
`func_800E82EC`; do not return to collision geometry or triangle tracing.
