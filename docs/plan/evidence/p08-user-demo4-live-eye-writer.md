---
title: "P08 — USER_DEMO_4 pre-ncbapivot live-eye writer discriminator"
description: "Last writer of follow-camera node+0x74 before USER_DEMO_4 glidx-1 ncbapivot enter. Node init MATCH. func_80110770 / func_800EE7F8 are faithful copiers. First bit difference is the computed source vector of set74 ordinal 2 during the mode=2 bind ncbapivot tick."
status: current
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-21
tags: [p08, user-demo-4, ncbapivot, node-0x74, func_80110770, func_800EE7F8, func_801108A0, func_80111FD4, angrylion, rt64]
---

# P08 — USER_DEMO_4 live-eye writer discriminator

## 1. Verdict

```text
NCBAPIVOT_NODE_INITIALIZATION_MATCH:   YES (n74/n80 zero then 801108A0 MATCH)
NODE74_AT_NODE_INIT:                   00000000,00000000,00000000 both
NODE74_WRITE_COUNT_BEFORE_FIRST_EP3:   2
  ord 1: 801108A0 init copy   MATCH
  ord 2: first ncbapivot tick DIFF (this is the last writer before glidx-1 enter)

FIRST_NODE74_BIT_DIFFERENCE_WRITE_ORDINAL: 2

NODE74_LAST_WRITER_PC:                 0x80110778 (jal 800EE7F8; a0=node+0x74)
NODE74_LAST_WRITER_FUNCTION:           func_80110770
NODE74_LAST_WRITER_CALLER:             func_80111FD4 RA 0x80112034
                                       (called from ncbapivot_entrypoint_3
                                        mode=2 glidx=0 bind tick)

FUNC_80110770_SEMANTIC_JOIN:           PASS
  a0 = camera node
  a1 = source vec3
  body = func_800EE7F8(a0+0x74, a1)

SOURCE_VECTOR_MATCH:                   NO  (ordinal 2)
DEST_BEFORE_MATCH:                     YES (ordinal 2 dest_before is the MATCH init pose)
DEST_AFTER_MATCH:                      NO  (faithful copy of differing source)
FUNC_800EE7F8_FAITHFUL:                YES (dst_post == src on every captured call)

NODE74_SOURCE_STORAGE:                 ncbapivot stack sp+0x48 (guest 80045558)
NODE74_SOURCE_FIRST_DIFFERENT_FIELD:   x 7 ULP and z 4 ULP; y MATCH 43c49665
NODE74_SOURCE_WRITER_PC:               ncbapivot 800EE780 then 80111FD4/80110770
NODE74_SOURCE_WRITER_FUNCTION:         ncbapivot_entrypoint_3 computed follow-eye
NODE74_SOURCE_WRITER_CALLER:           func_80110588 jalr D_801247A0[type+4]

PRE_EP3_CALL_ORDER_MATCH:              YES
FIRST_PRE_EP3_ORDER_DIFFERENCE:        NONE

FOLLOW_NODE_SEED_SOURCE_CLASS:         computed follow-eye on first
                                       ncbapivot_entrypoint_3 (mode=2 bind)
SOURCE_OBJECT:                         ncbapivot stack sp+0x48, not the selected
                                       record, not the ncpod/ncpoducam object
SOURCE_PRODUCER:                       ncbapivot GET n74 → 800EFB24/800EEBF0
                                       chain → 800EE780 add.s vec → 80111FD4
                                       (8010FFD0 nonzero path) → 80110770

NODE80_SOURCE_SHARED_WITH_NODE74:      YES (same bind tick, set80 ordinal 2)
NODE80_FIRST_DIFFERENCE:               y 2 ULP at set80 ordinal 2
                                       RT 4399bcf0  AN 4399bcee

A_CANDIDATE:                           NO (80110770 / 800EE7F8)
ROOT_CLASS:                            B-NCBAPIVOT-INPUT at glidx-1 enter;
                                       the glidx-1 seed is a faithful copy of
                                       the already-different computed eye from
                                       the prior (mode=2 glidx=0) ncbapivot tick
FIX_IMPLEMENTED:                       NO
EGG_PARITY_DEFECT:                     PROVEN_BUT_DEFERRED
```

The previously published claim that the first ncbapivot tick *outputs* the seed is **CORRECTED/SUPERSEDED** in scope: that was the **mode=5 glidx-1** tick, whose `node+0x74` already differed at enter ([[p08-user-demo4-ncbapivot-first-instruction]]). The last writer before that enter is `func_80110770` during the **mode=2 glidx=0** bind tick. At *that* tick's enter, `n74`/`n80`/target still MATCH. `func_80110770` then copies an already-different computed source. `func_800EE7F8` is not an A-candidate.

Do not treat warp-in `ncpod`/`ncpoducam` as the live-eye seed. Their record (`c36cfad8,442c4e0b,c517c58b`) is a different object. The follow node is seeded from selected-record pose `c3a20000,43200000,c4d0e000` (MATCH), then overwritten by the first ncbapivot compute.

---

## 2. Environment

```text
BASE_HEAD:     c3677d1f0ff51e95aed85d4a7d16366faf899bb7
RT64:          TOOIE_DIAG_ATTRACT=1 TOOIE_AUTO_START=1 TOOIE_UD4_FIRST_TICK=1
               timeout 300s ./build/TooieRecompiled
               EXIT 0 (FIRST_TICK_STOP after mode=5 glidx-1 ncbapivot leave)
ANGRYLION:     mupen64plus --emumode 0
               Video: angrylion's RDP Plus
               RSP:   cxd4 sse2
               CPU:   Pure Interpreter
               hook: tools/p08_attract_egg_hook.h (local mupen copy)
               EXIT 0
ROM_SHA1:      af1a89e12b638b8d82cc4c085c8e01d4cba03fb3  (Mupen cart)
Angrylion:     ORACLE ONLY; TooieRecomp remains RT64
```

Writer window latches at first `map==235` (warp-in) and stays on through the mode=2 bind (where `MEM_H(map)` is 0) until FIRST_TICK_STOP. Generated C was not edited.

---

## 3. Timeline (both oracles, same order)

```text
T0  ncpod 783 / ncpoducam 785 warp-in, mode=3 map=235
    selected rec live eye c36cfad8,442c4e0b,c517c58b (not the follow node)
T1  ncbapivot ep0/ep2 on a leftover node (not 80348f60 / 80348420)
T2  ncbapivot ep1 a0= follow node
    n74 = 00000000,00000000,00000000
    n80 = 00000000,00000000,00000000
    NCBAPIVOT_NODE_INITIALIZATION_MATCH: YES
T3  func_801108A0(node)
      8010FFB0 = CA8B4(*node+0x64) identity
      set74 ord1 src = c3a20000,43200000,c4d0e000 MATCH
      set80 ord1 src = 43b150a3,433ef928,00000000 MATCH
    800EE7F8 dst_post == src both
T4  ncbapivot ep2 / ep1 still carry that MATCH pose
T5  ncbapivot ep3 FIRST execution, mode=2 glidx=0
    ENTER n74 MATCH c3a20000,43200000,c4d0e000
    ENTER n80 MATCH 43b150a3,433ef928,00000000
    ENTER tgt  MATCH c2a80000,42700001,c4e7e000
    set74 ord2 dest_before MATCH; source DIFF; dest_after = source
    set80 ord2 dest_before MATCH; source y DIFF 2 ULP; dest_after = source
T6  ncbapivot ep3 mode=5 glidx=1 ENTER already has the seed
    (this is the previously published "first" tick)
```

```text
PRE_EP3_CALL_ORDER_MATCH:       YES
FIRST_PRE_EP3_ORDER_DIFFERENCE: NONE
```

Semantic glidx 1 is **not** the first `ncbapivot_entrypoint_3` on this node. The bind tick is mode=2 glidx=0.

---

## 4. Write watch: node+0x74

Follow node: RT `80348f60` / AN `80348420` (heap relocates; join by bits).

| ord | when | function | dest_before | source | dest_after | MATCH |
|---:|---|---|---|---|---|---|
| 1 | mode=2 bind, 801108A0 | 80110770 | `00000000,00000000,00000000` | `c3a20000,43200000,c4d0e000` | = source | YES |
| 2 | mode=2 first ep3 | 80110770 | `c3a20000,43200000,c4d0e000` | RT `c43efa34,43c49665,c4a6b794` AN `c43efa3b,43c49665,c4a6b798` | = source | NO |
| 3 | mode=5 glidx 1 (after enter) | 80110770 | seed, y+=1 ULP both | y-step only | = source | y-step MATCH path |
| 4 | same tick | 80110770 | identity | identity | identity | — |

Ordinal 3/4 are **after** glidx-1 enter and are not the last pre-enter writer.

AN RA for ordinal 2: `80112034` = delay-slot return of `jal 80110770` at `8011202C` inside `func_80111FD4` (the `8010FFD0` **nonzero** path: `80113640` then copy `80111FD4`'s `a1`).

`ncbapivot_entrypoint_3` calls `func_80111FD4(node, sp+0x48)`. `sp+0x48` is the GET of `n74` later updated by `func_800EE780` (three `add.s`: dest = a1 + a2). Guest address of that source on both oracles: `80045558`.

---

## 5. func_80110770 / func_800EE7F8 contract

Static:

```text
func_80110770:
  jal func_800EE7F8
  addiu a0, a0, 0x74
```

Executed: every captured `800EE7F8` under set74/set80 has `dst_post == src` bit-for-bit, including the differing ordinal-2 write.

```text
FUNC_800EE7F8_FAITHFUL: YES
A_CANDIDATE_FUNCTION:   none here
```

Do **not** investigate `800EE7F8` further.

---

## 6. Source vector (one producer backward)

Ordinal-2 source is already the published glidx-1 seed:

| field | RT64 | ANGRYLION | ULP |
|---|---|---|---|
| x | `c43efa34` | `c43efa3b` | 7 |
| y | `43c49665` | `43c49665` | 0 |
| z | `c4a6b794` | `c4a6b798` | 4 |

That vector is **not** the selected camera record (still warp-in default `45226124,453b8006,458c9fee` at glidx 1) and **not** the ncpod warp-in object.

It is produced on the first ncbapivot tick **after** entry inputs `n74` / target MATCH, by the overlay body (`800EFB24` camera−player, `800EEF94`/`sqrtf`, `800EEBF0`, `800EE780` add). Instruction-level join of that tick was **not** taken here: `80110770` already received differing bits, so this task stops at that producer identity.

`node+0x80` ordinal 2 is the same tick (`80110790` from ncbapivot `sp+0x60` after `80110A68`). Shared upstream: YES.

---

## 7. Layers

```text
FIRST_BIT_LEVEL_DIFFERENCE:       set74 ordinal 2 dest_after / source
                                  (mode=2 first ncbapivot tick)
FIRST_VISIBLE_DIFFERENCE:         demo-sync ~glidx 41 / 0.085 deg
FIRST_GAMEPLAY_CAUSAL_DIFFERENCE: walk-start amplification of that seed
```

Do not normalize 4–7 ULP.

---

## 8. Closed / deferred

```text
EGG_PARITY_DEFECT:     PROVEN_BUT_DEFERRED
FIX_IMPLEMENTED:       NO
P08_STATUS:            ACTIVE_MANUAL_VALIDATION_PENDING
P09_STATUS:            PENDING_NOT_FORMALLY_OPEN
```

Do not reopen CA8B4, CA628, E1A58, Rice Z-buffer, old gl385/gl392, `func_80013B70`, or C4C34. Do not treat glidx-1 ncbapivot arithmetic as the first producer: that tick's `n74` is already the ordinal-2 copy.

Bind-tick join is in [[p08-user-demo4-bind-tick-follow-eye]]: enter MATCH; first A-candidate is `func_800137F4`/`func_80013818` yaw sincos (`006d5329` vs `006d5328`). No production fix.
