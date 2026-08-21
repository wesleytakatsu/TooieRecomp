---
title: "P08 — USER_DEMO_4 bind-tick computed follow-eye discriminator"
description: "Instruction/call join of the first ncbapivot_entrypoint_3 (mode=2 glidx=0). The sincos index split is now classified as MIPS round.w.s ties-to-even, with a reusable N64Recomp/runtime correction documented in p08-user-demo4-sincos-semantics-fix."
status: current
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-21
tags: [p08, user-demo-4, ncbapivot, bind-tick, sp+0x48, func_800EF214, func_800137F4, func_80013818, angrylion, rt64]
---

# P08 — USER_DEMO_4 bind-tick follow-eye discriminator

## 1. Verdict

```text
BIND_TICK_ENTRY_INPUTS_COMPLETE:     YES (executed path through 800EF214)
BIND_TICK_ENTRY_INPUTS_MATCH:        YES
FIRST_BIND_TICK_INPUT_DIFFERENCE:    NONE

FOLLOW_EYE_CALL_CHAIN:
  ncbapivot_entrypoint_3
    80112524  GET tgt/player → sp+0x6C     MATCH
    801107F0  GET n74 → sp+0x48            MATCH
    800EE7F8  copy sp+0x6C → sp+0x28       MATCH
    800EFB24  sp+0x54 = n74 − player       MATCH
    800EEF94 / sqrtf  length               MATCH
    800F1A88 / 80013B70 / 80013B7C         MATCH
    80115358 / 80115750 / 80112C3C / 801125E8  MATCH
    800EEBF0
      800EFD24  zero dest                  MATCH
      800EF214  spherical offset into sp+0x54
        800137AC / 80013818  sin/cos pitch MATCH
        800137C4               cos pitch   MATCH
        800137F4 / 80013818    sin yaw     DIFF  ← FIRST
        80013788               cos yaw     B (leftover f12 already DIFF)
    800EE780  sp+0x48 = player + sp+0x54   faithful add of already-different offset
    80111FD4 / 80110770                    faithful copy to node+0x74

FIRST_DIVERGENT_CALL:                func_800137F4 (jal 80013818)
CALL_INPUTS_BIT_IDENTICAL:           YES
CALL_OUTPUTS_BIT_IDENTICAL:          NO
FIRST_DIVERGENT_INSTRUCTION_PC:      0x80013848 trunc.w.s (AN)
                                     RT table index 006d5329 vs AN 006d5328
FIRST_DIVERGENT_OPCODE:              trunc.w.s / cvt.w.s (COP1 fmt=16 fn=12/13 path)
                                     preceded by mul.s 0x80013844
RT64_INPUT_BITS:                     f12=4399bcf1 f14=3b360b61 f16=43b40000
ANGRYLION_INPUT_BITS:                f12=4399bcf1 f14=3b360b61 f16=43b40000
RT64_OUTPUT_BITS:                    f2/f0 sin=bf4b29c4  idx=006d5329  f12leftover=39fbf133
ANGRYLION_OUTPUT_BITS:               f2/f0 sin=bf4b29cc  idx=006d5328  f12leftover=39fba17a

SP48_X_FIRST_DIVERGENCE:             800EF214 mul.s 0x800EF268 using 137F4 sin
                                     RT c429fa34  AN c429fa3b  (then 800EE780 → c43efa34 / c43efa3b)
SP48_Y_FIRST_DIVERGENCE:             NONE (43c49665 both; 137C4 cos pitch MATCH)
SP48_Z_FIRST_DIVERGENCE:             800EF214 mul.s 0x800EF280 using 13788 cos
                                     of 13818 leftover f12 (same yaw sincos)
                                     RT 440250d9  AN 440250d0  (then 800EE780 → c4a6b794 / c4a6b798)

FIRST_PROVEN_CAUSAL_DIFFERENCE:      800137F4/80013818 yaw sincos table index
                                     006d5329 vs 006d5328 (8 ULP on sin)
ROOT_CLASS:                          A-CANDIDATE-HELPER (pre-fix)
A_CANDIDATE:                         YES
A_CANDIDATE_FUNCTION:                func_80013818 via func_800137F4
A_CANDIDATE_PC:                      0x80013848 (round.w.s); helper entry 0x800137F4
FIX_IMPLEMENTED:                     YES (MIPS round.w.s ties-to-even; see p08-user-demo4-sincos-semantics-fix)
POST_FIX_BIND_TICK_OUTPUTS_MATCH:    YES (idx 006d5328, sin bf4b29cc, sp48/n74 c43efa3b,43c49665,c4a6b798)
IDLE_FOLLOW_CAM_BIT_IDENTITY_GLIDX_1_38: YES after this fix (CA8B4/CA628)
CAMERA_FEEDBACK_ACCUMULATION:        STILL_PRESENT
FIRST_REMAINING_BIT_DIFFERENCE:      dump glidx 39 ncbapivot_entrypoint_3 output
                                     (player pos/yaw/ideal MATCH; see
                                     [[p08-user-demo4-post-round-full-state-discriminator]])
EGG_PARITY_DEFECT:                   PROVEN (FIRE vs BLUE)
EGG_INDEPENDENT_FROM_ROUND_W_S:      YES
EGG_INDEPENDENT_FROM_REMAINING_CAMERA_STATE: UNKNOWN
NEXT_SINGLE_ACTION:                  instruction-join ncbapivot_entrypoint_3 at dump glidx 39; do not reopen round.w.s
```

Pitch sincos (`800137AC` / first `80013818`) is bit-identical. `func_80013B70` / `func_80013B7C` MATCH this tick — do not reopen them. `800EFB24`, `sqrtf`, `800EE780` are faithful. `800EE7F8` remains a faithful copier.

x and z of `sp+0x48` are the **same** yaw-sincos error, not two independent computations. y MATCH.

---

## 2. Environment

```text
BASE_HEAD:     00691f212b2cc66633cb977ccf16394b4b8184dc
RT64:          TOOIE_DIAG_ATTRACT=1 TOOIE_AUTO_START=1 TOOIE_UD4_FIRST_TICK=1
               timeout 300s ./build/TooieRecompiled
               EXIT 0 (FIRST_TICK_STOP after mode=2 glidx=0 ncbapivot leave)
ANGRYLION:     mupen64plus --emumode 0
               Video: angrylion's RDP Plus
               RSP:   cxd4 sse2
               CPU:   Pure Interpreter
               hook: tools/p08_attract_egg_hook.h (local mupen copy)
               EXIT 0
ROM_SHA1:      af1a89e12b638b8d82cc4c085c8e01d4cba03fb3  (Mupen cart)
Angrylion:     ORACLE ONLY; TooieRecomp remains RT64
```

Generated C was not edited. Wrappers remain env-gated. Trace window is only the first haunted-latch `ncbapivot_entrypoint_3`.

---

## 3. Complete bind-tick entry inputs

Follow node: RT `803486f0` / AN `80348420` (heap relocates; join by bits). Frame `80045510` both.

| field | RT64 | ANGRYLION | MATCH |
|---|---|---|---|
| n74 | `c3a20000,43200000,c4d0e000` | same | YES |
| n80 | `43b150a3,433ef928,00000000` | same | YES |
| tgt | `c2a80000,42700001,c4e7e000` | same | YES |
| player pos | `c2a80000,36800000,c4e7e000` | same | YES |
| p14+0xC / +0x10 | `4465cf49` / `44716a44` | same | YES |
| p3c+0 / +4 | `41aa0106` / `41fa0106` | same | YES |
| p4c+0xC | `80000000` | same | YES |
| p60+0 / +8 | `00000000` | same | YES |
| *5c | `00000000` | same | YES |
| f0 / f12 | `00000000` / `c28b1496` | same | YES |
| rm | nearest (`fegetround=0`) | nearest (`fcr31[1:0]=00`) | YES |

Extra AN `fcr31=01000800` is not treated as the discriminator (same as prior ticks).

```text
BIND_TICK_ENTRY_INPUTS_COMPLETE:  YES
BIND_TICK_ENTRY_INPUTS_MATCH:     YES
FIRST_BIND_TICK_INPUT_DIFFERENCE: NONE
```

---

## 4. Call tree to `sp+0x48`

Child sequence through `800EE780` MATCH (names and order). Nested `800EFB24` flood after `80111FD4` is **after** the follow-eye write and is out of scope.

```text
T0  enter n74/tgt/player MATCH
T1  80112524  va0 = tgt  c2a80000,42700001,c4e7e000  MATCH  → sp+0x6C
T2  801107F0  va0 = n74  c3a20000,43200000,c4d0e000  MATCH  → sp+0x48
T3  800EE7F8  copy player to sp+0x28  MATCH
T4  800EFB24  dest=c3700000,42c80000,43380000  MATCH
T5  800EEF94/sqrtf  f0=439f42c3  MATCH
T6  800F1A88 xz-hypot sqrtf f0=4397355e MATCH
    80013B70 f12=c3700000 f14=43380000 a2=4397355e → f0=4399bcf1 MATCH
    80013B7C f12=42c80000 f14=4397355e → f0=41926129 MATCH
    sp+0x34 = 41926129,4399bcf1,439f42c3 MATCH
T7  80115358 a1=439f42c3 → f0=4465cf49 MATCH
    80115750 a1=41926129 → f0=41aa0106 MATCH
    80112C3C a1=4399bcf1 → f0=4399bcf1 MATCH
    801125E8 → f0=80000000 MATCH
T8  800EEBF0(a0=sp+0x54, a1=sp+0x34, a2=4465cf49) enter MATCH
T9  800EFD24 zeros dest MATCH
T10 800EF214 enter MATCH:
      a0=80045564 a1=41aa0106 a2=4399bcf1 a3=4465cf49
      dest=00000000,00000000,00000000
T11 800137AC f12=41aa0106 → f0=3f6e97cf f2=3eb99299 MATCH
    nested 80013818 pitch MATCH (idx a0=00078e45 both)
T12 800137C4 leave f0=3eb99299 MATCH
T13 800137F4 f12=4399bcf1 MATCH → sin DIFF   FIRST
    nested 80013818 yaw:
      enter f12=4399bcf1 f14=3b360b61 f16=43b40000
      trunc.w.s index RT 006d5329 AN 006d5328
      sin f2 RT bf4b29c4 AN bf4b29cc
T14 80013788 leftover f12 already DIFF (B)
T15 800EF214 leave dest:
      RT c429fa34,43a69665,440250d9
      AN c429fa3b,43a69665,440250d0
T16 800EE780 dest = player + offset (faithful)
      RT c43efa34,43c49665,c4a6b794
      AN c43efa3b,43c49665,c4a6b798
```

```text
PRE_EP3_CALL_ORDER_MATCH: YES (this tick)
```

---

## 5. First same-input / different-output

`func_800EF214` is the first **call** whose vector dest differs, but its first children (`137AC`/`13818` pitch, `137C4`) MATCH. The first helper with complete matching inputs and different outputs is:

```text
FIRST_DIVERGENT_CALL:       func_800137F4  (RA wrapper; jal func_80013818)
NESTED_CORE:                func_80013818  (sincos table interpolator)
CALL_INPUTS_BIT_IDENTICAL:  YES
  f12 = 4399bcf1   (yaw from 13B70, MATCH)
  f14 = 3b360b61   (table constant 0x800416D4)
  f16 = 43b40000   (360.0)
CALL_OUTPUTS_BIT_IDENTICAL: NO
  RT  f2=bf4b29c4  idx=006d5329  leftover f12=39fbf133
  AN  f2=bf4b29cc  idx=006d5328  leftover f12=39fba17a
```

Host `func_800137F4` (Tooie-native RA wrapper in `boot_hw_stubs.cpp`) does `f0 = f2` after `func_80013818`. Angrylion MIPS does the same `mov.s $f0, $f2` in the `jr` delay. The consumed sin used by `800EF214` `mul.s` at `0x800EF268` is therefore `f0`/`f2` above.

`func_80013788` (cos) is **not** an independent A-candidate: its `f12` is already the differing leftover from this `13818`.

Pitch `80013818` with `f12=41aa0106` produced identical `f0=3f6e97cf f2=3eb99299 idx=00078e45`. The helper is not globally divergent; the yaw invocation is.

AN FPU at `0x80013844` `mul.s $f4, $f12, $f16` → `4adaa651`; `0x80013848` trunc → `006d5328`. RT was not instruction-traced inside `13818` (host `137F4` calls it directly, bypassing LOOKUP wrap). The integer index after that trunc is the first architectural integer that differs.

```text
INPUTS_BIT_IDENTICAL:            YES
FIRST_DIVERGENT_INSTRUCTION_PC:  0x80013848
FIRST_DIVERGENT_OPCODE:          trunc.w.s (AN); RT index already 006d5329
A_CANDIDATE:                     YES
A_CANDIDATE_FUNCTION:            func_80013818 via func_800137F4
```

---

## 6. `sp+0x48` field provenance

`800EE780` is three `add.s`: dest = player (`sp+0x28` MATCH) + offset (`sp+0x54`).

| field | offset (sp+0x54) RT / AN | follow-eye (sp+0x48) RT / AN | origin |
|---|---|---|---|
| x | `c429fa34` / `c429fa3b` | `c43efa34` / `c43efa3b` | `mul.s` of MATCH hypot-xz * **137F4 sin** |
| y | `43a69665` / `43a69665` | `43c49665` / `43c49665` | MATCH `137C4` cos pitch * distance |
| z | `440250d9` / `440250d0` | `c4a6b794` / `c4a6b798` | `mul.s` of MATCH hypot-xz * **13788 cos** of leftover f12 |

Same scalar error. Not two independent bugs.

---

## 7. Layers

```text
FIRST_BIT_LEVEL_DIFFERENCE:       800137F4/80013818 yaw sincos
                                  (idx 006d5329 vs 006d5328; sin 8 ULP)
                                  then 800EF214 dest / 800EE780 sp+0x48
                                  then faithful 80110770 node+0x74
FIRST_VISIBLE_DIFFERENCE:         demo-sync ~glidx 41 / 0.085 deg
FIRST_GAMEPLAY_CAUSAL_DIFFERENCE: walk-start amplification of that seed
```

Do not normalize 4–8 ULP.

---

## 8. Closed / deferred

```text
EGG_PARITY_DEFECT:     INDEPENDENT_PROVEN_PARITY_DEFECT
FIX_IMPLEMENTED:       YES (bind-tick round.w.s only)
P08_STATUS:            ACTIVE_MANUAL_VALIDATION_PENDING
P09_STATUS:            PENDING_NOT_FORMALLY_OPEN
```

Do not reopen CA8B4, CA628, E1A58, Rice Z-buffer, old gl385/gl392, `func_80013B70` (MATCH this tick), or C4C34. Do not treat `800EE780` / `800EFB24` / `800EE7F8` as A-candidates. Do not reopen `round.w.s` ties-to-even: the bind-tick yaw invocation now matches ANGRYLION_CONTROL.

## 9. Post-fix live replay (2026-08-21)

`TOOIE_UD4_FIRST_TICK=1` on DISPLAY=:0 / RADV POLARIS10. Bind-tick leave now matches Angrylion bit-for-bit:

```text
POST_FIX_TABLE_INDEX_MATCH: YES  006d5328 / 006d5328
POST_FIX_SIN_BITS_MATCH:    YES  bf4b29cc / bf4b29cc
POST_FIX_SP48_MATCH:        YES  c43efa3b,43c49665,c4a6b798
POST_FIX_NODE74_MATCH:      YES  c43efa3b,43c49665,c4a6b798
```

Full USER_DEMO_4 pad 877/877. First remaining demo-sync split is still glidx 41 `camctrl` 306.0416 vs 306.1269 (0.0853°), then 12.0481° @50 and 112.6789° @100. That recurrence is **STILL_PRESENT**; the bind-tick seed is no longer the live discriminator. RT64 egg remains `cheggnormal` 539 BLUE against Angrylion `cheggfire` 535 FIRE.

ROUND.W.S architectural checkpoint: `5854872c79a7cab1c9395ddcd173cece449ffe11`. `CAMERA_COMPLETE_FIX: NO`. `CAMERA_FEEDBACK_ACCUMULATION: STILL_PRESENT`.

Next bounded action: instruction-join the first walking-player `ncbapivot` at dump glidx 39. Do not reopen `round.w.s`.
