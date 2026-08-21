---
title: "P08 — USER_DEMO_4 ncbapivot first-tick instruction discriminator"
description: "ANGRYLION_CONTROL vs RT64_CONTROL join of ncbapivot_entrypoint_3 on USER_DEMO_4 glidx 1. Complete entry inputs do not match: node+0x74 / +0x80 already differ before the first instruction. CASE A / B-NCBAPIVOT-INPUT. No production fix."
status: current
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-21
tags: [p08, user-demo-4, ncbapivot, first-tick, node-0x74, func_80110770, func_80110790, angrylion, rt64]
---

# P08 — USER_DEMO_4 ncbapivot first-tick instruction discriminator

## 1. Verdict

```text
NCBAPIVOT_OVERLAY:                 781
NCBAPIVOT_ENTRYPOINT:              3
ENTRY_PC:                          0x80800098 (stub 0x80089488)
RETURN_PC:                         0x8080027C / caller RA 0x8011061C
CALLER:                            func_80110588 jalr D_801247A0[type+4] @ 0x80110614
EXECUTED_CHILD_SEQUENCE:           177/177 MATCH
NCBAPIVOT_PC_SEQUENCE_MATCH:       YES (call tree + 80111FD4 v0=0 skip)
FIRST_PC_SEQUENCE_DIFFERENCE:      NONE

CAMERA_RECORD_INPUTS_MATCH:        YES (rec eye/src2 still warp-in default)
PLAYER_INPUTS_MATCH:               YES (pos/yaw/ideal and node+0x30+4 tgt)
GLOBAL_INPUTS_MATCH:               YES among captured live globals
STACK_INPUTS_MATCH:                YES (empty frame at entry)
FPR_ENTRY_INPUTS_MATCH:            YES at first=1 enter (f0=45e72cee f12=4338d84c)
FCSR_MATCH:                        rounding nearest both (RT fegetround=0;
                                   AN fcr31 bits[1:0]=00). Extra AN fcr31
                                   0x01000800 is not treated as the discriminator.
OTHER_NCBAPIVOT_STATE_MATCH:       NO

FIRST_ENTRY_INPUT_DIFFERENCE:      node+0x74 (live eye) and node+0x80 (dest)
  n74 x  RT c43efa34  AN c43efa3b  7 ULP
  n74 y  RT 43c49665  AN 43c49665  MATCH
  n74 z  RT c4a6b794  AN c4a6b798  4 ULP
  n80 x  RT 43a95fee  AN 43a95fee  MATCH
  n80 y  RT 4399bcf0  AN 4399bcee  2 ULP
  n80 z  00000000 both

FIRST_DIVERGENT_CALL:              NONE inside this tick
FIRST_DIVERGENT_INSTRUCTION_PC:    N/A (entry inputs already differ)
INPUTS_BIT_IDENTICAL:              NO
A_CANDIDATE:                       NO

FIRST_EYE_X_DIVERGENT_PC:          already present at ep3 enter as n74[0]
FIRST_EYE_Z_DIVERGENT_PC:          already present at ep3 enter as n74[2]
FIRST_CAMCTRL_DIVERGENT_PC:        not created from identical inputs this tick;
                                   downstream of n74

ROOT_CLASS:                        B-NCBAPIVOT-INPUT
FIRST_DIFFERING_INPUT:             camera node +0x74 (and +0x80 y)
WRITER_PC:                         0x80110778 (func_80110770 → 800EE7F8 a0=node+0x74)
                                   0x80110798 (func_80110790 → 800EE7F8 a0=node+0x80)
PRODUCER_FUNCTION:                 func_80110770 during mode=2 glidx=0
                                   ncbapivot (see [[p08-user-demo4-live-eye-writer]])
FIX_IMPLEMENTED:                   NO
EGG_PARITY_DEFECT:                 PROVEN_BUT_DEFERRED
```

`CAMERA_RECORD_INITIALIZATION_MATCH=YES` from [[p08-user-demo4-ca8b4-first-seed]] covered the selected **record** (warp-in `45226124,453b8006,458c9fee`). That is **not** the ncbapivot live eye. Live eye is `node+0x74`. That field already carries the previously published 7/4 ULP seed **before** the first instruction of `ncbapivot_entrypoint_3`.

Do not treat glidx-1 ncbapivot arithmetic, `sqrtf` (`0x8002DCA0`), or overlay-local `add.s` as the first discriminator of this tick. Those helpers run with already-different `n74`.

---

## 2. Environment

```text
BASE_HEAD:     75b12145d35db2857e08d44cd57cfef191a2dcbb
RT64:          TOOIE_DIAG_ATTRACT=1 TOOIE_AUTO_START=1 TOOIE_UD4_FIRST_TICK=1
               timeout 240s ./build/TooieRecompiled
               EXIT 0 (FIRST_TICK_STOP after ncbapivot leave)
ANGRYLION:     mupen64plus --emumode 0
               Video: angrylion's RDP Plus
               RSP:   cxd4 sse2
               CPU:   Pure Interpreter
               hook: tools/p08_attract_egg_hook.h (local mupen copy)
               EXIT 0 (FIRST_TICK_STOP)
ROM_SHA1:      af1a89e12b638b8d82cc4c085c8e01d4cba03fb3  (Mupen cart)
               TooieRecomp uses the decompressed build ROM (unchanged)
Angrylion:     ORACLE ONLY; TooieRecomp remains RT64
```

Generated C was not edited. Wrappers remain env-gated.

---

## 3. Entrypoint / executed range

```text
NCBAPIVOT_OVERLAY:     781
NCBAPIVOT_ENTRYPOINT:  3
ENTRY_PC:              0x80800098
STUB_PC:               0x80089488
RETURN_PC:             0x8080027C
CALLER:                func_80110588 @ 0x80110614 jalr $v0
                       RA 0x8011061C
AN relocated body:     first_body_pc=0x80336500
```

`*node+0x5C = 0` both oracles → waypoint helper skipped (`L80800108`, `sp+0x40 = 0`).

`func_80111FD4` leave `v0=0` both → `beql` to `L80800234` (no `801138A0` / `80115580`). Call-tree path MATCH.

Child sequence 177/177 identical (names and order). Nested instruction counter on Angrylion from stub to RA: 67995 (includes callees; not overlay-body-only).

```text
NCBAPIVOT_PC_SEQUENCE_MATCH:  YES
FIRST_PC_SEQUENCE_DIFFERENCE: NONE
```

---

## 4. Executed read set (this path)

`ncbapivot_entrypoint_3` itself reads:

```text
NCBAPIVOT_ENTRY_READ_SET:
  a0                  camera node (s0)
  PC 808000E4  lw t6, 0x5C(s0)     node+0x5C  waypoint ptr  MATCH *5c=0
  PC 808000E8  lw t7, 0x0(t6)      skipped-equivalent (*5c=0)
  children then read:
    node+0x04          8011607C store target
    node+0x30,+4       80112524 player/target pos   MATCH c2a80000,42700001,c4e7e000
    node+0x74          801107F0 / 80110A68 live eye  DIFF
    node+0x80          80110790 dest                 DIFF y
    rec D_80127708[sel] still warp-in default         MATCH
```

Do not dump the rest of the camera struct. `func_80800008_ncbapivot` is a trampoline to `func_80112524`.

Static dedicated writers of the differing fields:

```text
func_80110770  jal 800EE7F8; addiu a0, a0, 0x74   SET live eye
func_80110790  jal 800EE7F8; addiu a0, a0, 0x80   SET dest
```

After `ncbapivot` returns, `func_80110588` copies `node+0x74` and `node+0x80` (`800EE7F8`) then calls `func_8010FE14`. That is why the selected **record** still holds warp-in bits at ncbapivot leave, then CA8B4 later sees the follow-cam eye. Do not reopen CA8B4/CA628.

---

## 5. Entry snapshot (first=1, before first instruction)

| field | RT64 | ANGRYLION | MATCH |
|---|---|---|---|
| rec eye | `45226124,453b8006,458c9fee` | same | YES |
| rec src2 | `c1f00000,41f00000,00000000` | same | YES |
| player pos | `c2a80000,00000000,c4e7e000` | same | YES |
| yaw/ideal | `42d80000` / `42d80000` | same | YES |
| tgt node+0x30+4 | `c2a80000,42700001,c4e7e000` | same | YES |
| *5c | `0` | `0` | YES |
| f0 / f12 | `45e72cee` / `4338d84c` | same | YES |
| a3 | `80045518` | same | YES |
| n74 | `c43efa34,43c49665,c4a6b794` | `c43efa3b,43c49665,c4a6b798` | NO |
| n80 | `43a95fee,4399bcf0,00000000` | `43a95fee,4399bcee,00000000` | NO |
| rounding | nearest | nearest | YES |

Heap pointers (`a0` node, `a1`/`a2`) relocate and are not joined by address.

AN fires the stub PC twice (`first=1` then `first=0`). Live FPRs on the second stub are clobbered (`f0=00000113`); that is a hook artifact. Semantic join uses `first=1`.

---

## 6. This tick does not create the seed

Call tree (compact):

```text
ncbapivot_entrypoint_3
  -> 8011607C(a1=1)
  -> 80113978(a1=1, a2=0)
  -> 80112524            # player pos
  -> 801107F0            # copy n74
  -> 800EE7F8 / 800EFB24 # vec sub (camera-player) already different
  -> 800EEF94 / sqrtf    # length of that vec; inputs already differ
  -> 800F1A88
  -> 80115358 / 80115750 / 80112C3C / 801125E8
  -> 800EEBF0 / 800EE780
  -> 80111FD4            # v0=0 both; skip 138A0
  -> 80110A68 / 800A516C
  -> 80110790            # writes n80 = already-different dest
```

`800EFB24` leave `va0` (camera−player) is `c429fa34,…,440250d8` vs `c429fa3b,…,440250d0` — the n74 ULP minus a matching player. Subsequent `sqrtf` therefore also differs; it is **not** same-input/different-output.

`80110790` enter dest already equals pre-entry `n80` on each runtime. This tick preserves the dest ULP.

`n74.y` steps `43c49665 → 43c49666` on **both** oracles during the tick (`80111FD4` path). `n74.x/z` keep the entry seed.

No FPU bit-level capture of an identical-input instruction: complete inputs are not identical.

---

## 7. Classification

```text
CASE A / ROOT_CLASS: B-NCBAPIVOT-INPUT
FIRST_BIT_LEVEL_DIFFERENCE:     node+0x74 at first ncbapivot ep3 enter
                                (glidx 1, before first instruction)
FIRST_VISIBLE_DIFFERENCE:       demo-sync ~glidx 41 / 0.085 deg
FIRST_GAMEPLAY_CAUSAL_DIFFERENCE: walk-start amplification of that seed
                                (unchanged from [[p08-user-demo4-ca8b4-first-seed]])
```

The glidx-1 CA8B4/ncbapivot **output** reported previously is **CORRECTED/SUPERSEDED** as the first bit-level difference: it is a copy/preserve of `n74`/`n80` that already differed at enter.

---

## 8. Producer backtrace (stopped)

Trace only the earliest differing input:

```text
node+0x74 writer:  func_80110770  PC 80110778
node+0x80 writer:  func_80110790  PC 80110798
```

The **call** of those setters that first wrote the differing bits is **before** this windowed ep3. Captured in [[p08-user-demo4-live-eye-writer]]: last writer before this enter is `func_80110770` ordinal 2 during the **mode=2 glidx=0** bind ncbapivot tick; `800EE7F8` is a faithful copier of an already-different computed `sp+0x48` eye. Node init / `801108A0` MATCH. Bind-tick arithmetic: [[p08-user-demo4-bind-tick-follow-eye]] (`800137F4`/`80013818` A-candidate). Do not reopen CA8B4, CA628, E1A58, Rice Z-buffer, 13B70, or C4C34.

---

## 9. Closed / deferred

```text
EGG_PARITY_DEFECT:     PROVEN_BUT_DEFERRED
FIX_IMPLEMENTED:       NO
P08_STATUS:            ACTIVE_MANUAL_VALIDATION_PENDING
P09_STATUS:            PENDING_NOT_FORMALLY_OPEN
```
