---
title: "P08 USER_DEMO_4 — 0x800BCE84 live-input join"
description: "Instruction-level paired RT64/Angrylion capture of the exact memory word consumed by 0x800BCE84 at USER_DEMO_4 glidx 39."
status: current
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-22
tags: [p08, user-demo4, bce84, bakey, bainput, 800979d8, live-input, angrylion, rt64]
---

# P08 USER_DEMO_4 — `0x800BCE84` live-input join

Final semantic evidence for the corrected `0x800979D8` mapping. The mapping is
generic and remains in the owning source layer; this note records the raw
runtime join, not a generated-code edit.
`RecompiledFuncs/*` were not edited. No commit / no push.

```text
MODEL:                               Grok 4.6 Medium
BASE_HEAD:                           01cf8255399c942cceac1c113972f1633afd3089
WORKTREE:                            DIRTY — preserved
FIX_IMPLEMENTED:                     YES (owning source layer; generated C untouched)
COMMIT:                              NO
PUSHED:                              NO
```

## 0. Worktree classification

| class | files |
| --- | --- |
| A. Luna `0x800979D8` production | `src/game/boot_hw_stubs.cpp` (`tooie_bainput_func_800979D8` control flow **unchanged**) |
| B. helper tests | `tests/bcf28_helpers_test.cpp` (pre-existing; still PASS) |
| C. RT64 diagnostic probes | `src/game/boot_hw_stubs.cpp` wrap of `LOOKUP_FUNC(0x800BCE84)` + join logs |
| D. oracle diagnostic hooks | `tools/p08_attract_egg_hook.h`, `tools/oracle/angrylion-control/patches/mupen64plus-core-p08-instrumentation.patch` |
| E. evidence/docs | this note + handoff/review updates |
| F. unrelated / pre-existing | other dirty docs from earlier sessions |

## 1. Static `0x800BCE84` (ELF `banjotooie_decompressed.elf`)

```text
800BCE84_START_PC:               0x800BCE84
800BCE84_END_PC:                 0x800BCEBC
ARGUMENTS:                       a0 = unkfunc_800BCE84* (BaKey.unk0)
                                 a1 = ButtonId
800BCE84_RETURN_REGISTER:        v0
800BCE84_EXACT_MEMORY_LOAD_PC:   0x800BCEAC   lw v0, 0(t0)
800BCE84_LOAD_BASE:              t0 = 0x800BCCC0(a0, MEM_BU(a0+a1+8))
800BCE84_LOAD_OFFSET:            0
800BCE84_LOAD_WIDTH:             32-bit unsigned (lw)
800BCE84_MASK:                   0x1 (andi t1, v0, 1; delay-slot move v0, t1)
```

Loads:

| PC | opcode | base | offset | width |
| --- | --- | --- | --- | --- |
| `800BCE98` | `lbu a1, 8(t7)` delay of jal | `t7=a0+a1` | `+8` | u8 (`unk8[button]`) |
| `800BCEAC` | `lw v0, 0(t0)` | `t0` | `0` | u32 |

Arithmetic: `jal 800BCCC0` → `v0 = a0 + unk8[button]*56 + 0x24`; `sll t9, button, 2`; `addu t0, v0, t9`; `andi 1`.

```text
800BCE84_EXACT_PSEUDOCODE:
  ring = *(u8*)(obj + 8 + button)          /* unk8[button] */
  slot = obj + ring * 56 + 0x24            /* __get_button_data_ptr */
  word = *(u32*)(slot + button * 4)        /* BCE84_SOURCE_FIELD */
  return word & 1
```

Do **not** call this `ButtonState.pressed` until the packed-word layout is
named; the consumed value is `BCE84_SOURCE_FIELD` = LSB of that u32.

## 2. Object passed from `bakey_held`

```text
BAKEY_HELD_PC:                   0x800919F4
800BCE84_CALL_PC:                0x80091A18  (jal; delay lw a0, 0(key))
800979D8 JAL bakey_held:         0x800979E8
800BCE84_A0_PROVENANCE:          PlayerState+0x40 → BaKey*;
                                 a0 = BaKey.unk0 = *(u32*)key
800BCE84_OBJECT_OWNER:           BaKey.unk0 (unkfunc_800BCE84 / button-data header)
800BCE84_OBJECT_OFFSET_FROM_BAINPUT:
                                 not a BaInput field; sibling via PlayerState
                                 (input at +0xA4, key at +0x40)
BCE84_OBJECT_SEMANTIC_JOIN:      PASS
```

Guest pointers relocate (RT `obj=8034c3d0` vs AN `obj=803426d0`) but the
struct relationship, button id, consumed pad, and load offset `obj+0x90`
match.

## 3. Target join

```text
TARGET_JOIN:                     PASS
TARGET_STAGE_SUB:                stage=0 sub=4 (USER_DEMO_4 / MAP_235)
TARGET_GLIDX_RT:                 39
TARGET_GLIDX_AN:                 39
TARGET_PAD_RT:                   (-48,-46,0001)
TARGET_PAD_AN:                   (-48,-46,0001)
TARGET_BUTTON_ID:                13 (BUTTON_C_RIGHT)
TARGET_979D8_CALL_ORD_RT:        8
TARGET_979D8_CALL_ORD_AN:        8
```

Join keys: stage/sub, consumed glrecord pad, button_id, `0x800979D8` ordinal
8 (the C-Right / `in_id=1` call). glidx alone was not used.

## 4. Central gate — exact BCE84 sample

Host artifacts (not committed):

```text
RT64:     /tmp/p08-rt64-bce84-join.log
          sha256 0c11c0f46ddae83a963890fad5fadcc2c9136939fcb886b878d799fc33179910
AN cam:   /tmp/p08-angrylion-ud4-cam.log
          sha256 47626f988f691f163414d0b19011b24dd90000a909a667f9e268085d72e90ad8
excerpts: /tmp/p08-walkstart-bce84-join/
```

AN Pure Interpreter at guest PC `0x800BCEAC` (before `lw`):

```text
[ud4-bce84] runtime=AN stage=0 sub=4 glidx=39 pad=(-48,-46,0001) btn=13
 obj=803426d0 key=80341a54 load_pc=800BCEAC addr=80342760 ring=1 t8=13
 raw=00000115 loaded=00000115 masked=1
 old_probe_word=00000000 old_probe_bit0=0
```

RT64 wrap of generated `func_800BCE84` (same formula as `lw` at `800BCEAC`):

```text
[ud4-bce84] runtime=RT stage=0 sub=4 glidx=39 pad=(-48,-46,0001) btn=13
 obj=8034c3d0 key=8034b594 self=8034b200 load_pc=800BCEAC addr=8034c460 ring=1
 raw=00000115 loaded=00000115 masked=1
 old_probe_word=00000000 old_probe_bit0=0
 leave ret=00000001
```

```text
BCE84_OBJECT_RT:                 8034c3d0
BCE84_OBJECT_AN:                 803426d0
BCE84_LOAD_ADDR_RT:              8034c460   (obj + 0x90)
BCE84_LOAD_ADDR_AN:              80342760   (obj + 0x90)
BCE84_RAW_BYTES_RT:              00000115
BCE84_RAW_BYTES_AN:              00000115
BCE84_LOADED_VALUE_RT:           0x00000115
BCE84_LOADED_VALUE_AN:           0x00000115
BCE84_MASKED_VALUE_RT:           1
BCE84_MASKED_VALUE_AN:           1
BCE84_RETURN_RT:                 1
BCE84_RETURN_AN:                 1
BCE84_RETURN_MATCH:              YES
```

`0x90 = 1*56 + 0x24 + 13*4`. **PROVEN.**

## 5. Old `bs_pressed=0` probe — CORRECTED/SUPERSEDED

The previous RT diagnostic did:

`MEM_W(0x24 + button_id * 4, BaKey.unk0)` = **ring 0** word.

```text
OLD_BS_PRESSED_PROBE_ADDRESS:    obj + 0x24 + btn*4 = obj + 0x58 (btn=13)
OLD_BS_PRESSED_PROBE_OFFSET:     0x24 + 13*4 = 0x58
OLD_BS_PRESSED_PROBE_OBJECT:     BaKey.unk0 (same object as BCE84 a0)
OLD_BS_PRESSED_PROBE_PHASE:      0x800979D8 enter, before bakey_held
OLD_PROBE_EQUALS_BCE84_SOURCE:   NO
OLD_PROBE_CLASSIFICATION:        WRONG_OFFSET
                                 (ring[0] instead of ring[unk8[button]])
```

At the target, `unk8[13]=1`, so the live word is at `+0x90` (`0x115`, LSB 1)
while ring 0 at `+0x58` is `0`. Same object, wrong ring slot. Not a wrong
phase and not a reconstructed host-only value: both runtimes show
`old_probe_word=0` and `BCE84_SOURCE_FIELD=0x115`.

## 6. bakey_held dynamic join

```text
PREV_STATE_RT:                   0
PREV_STATE_AN:                   0
PREV_STATE_MATCH:                YES
BCE84_EXECUTED_RT:               YES
BCE84_EXECUTED_AN:               YES
BAKEY_HELD_RETURN_RT:            1
BAKEY_HELD_RETURN_AN:            1
BAKEY_HELD_RETURN_MATCH:         YES
```

## 7. `0x800979D8`

```text
979D8_INPUTS_MATCH:              YES (a1=13, a2=1, prev_state=0, pad MATCH)
979D8_ENABLED_BEFORE_RT:         0xffffffbf
979D8_ENABLED_BEFORE_AN:         not word-logged on AN enter; RT after-mask
                                 path leaves 0xffffffbf
979D8_BAKEY_RESULT_RT:           1
979D8_BAKEY_RESULT_AN:           1
979D8_ENABLED_AFTER_RT:          0xffffffbf
979D8_ENABLED_AFTER_AN:          (leave v0=2 implies bit 1 remains set)
979D8_RETURN_RT:                 2
979D8_RETURN_AN:                 2
979D8_RETURN_MATCH:              YES
```

Oracle raw return is `2`, matching the static enabled-mask contract.

## 8. Downstream (event confirmation only)

```text
ROTATE_RIGHT_MATCH:              YES (80097B94 v0=2 both)
80110EFC / 80110E00 / 801111D0:  execute on both
P60_C_AFTER_RT:                  1
P60_C_AFTER_AN:                  1
P60_E_AFTER_RT:                  2
P60_E_AFTER_AN:                  2
P60_8_AFTER_RT:                  5
P60_8_AFTER_AN:                  5
NCBAPIVOT_BRANCH_MATCH:          YES (p60+8=5 at enter, sub=4 glidx=39)
```

`801111D0` log formatting still differs (RT wrap prints f12 `0.0200`; AN
prints a1 `45.0`). That is **not** the BCE84 gate; after-write p60 matches.

## 9. Classification — CASE A

```text
LIVE_INPUT_EQUIVALENCE:          PROVEN
FIRST_DISCRIMINATOR:             none on this joined event
ROOT_CLASS:                      B-800979D8-NATIVE-SEMANTICS
ROOT_CAUSE_LEVEL:                PROVEN for the helper chain
OLD_BS_PRESSED_DIAGNOSTIC:       CORRECTED/SUPERSEDED
P08_STATUS:                      ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                      PENDING / NOT_FORMALLY_OPEN
```

Focused unit test: `bcf28_helpers_test` **PASS** after probe add.

## 10. Immutable paired recapture (2026-08-22T07:07Z)

Artifact-integrity recapture. Production `tooie_bainput_func_800979D8`
control flow was not changed. RT env-gated join/BCE84 diagnostics were
restored for this capture only. No commit / no push.

```text
CAPTURE_DIR:           /tmp/p08-979d8-final-20260822T070735Z
SHA256SUMS:            /tmp/p08-979d8-final-20260822T070735Z/SHA256SUMS
SHA256_VERIFY_FIRST:   PASS
SHA256_VERIFY_SECOND:  PASS
FILES_MODIFIED_AFTER_HASH: NO (chmod a-w only)
TARGET_JOIN:           PASS (stage/sub, pad, button_id=13, 979D8 ord=8)
BCE84_RAW:             RT=00000115 AN=00000115 MATCH
BCE84_RETURN:          RT=1 AN=1 MATCH
BAKEY_HELD:            RT=1 AN=1 MATCH
979D8_RETURN:          RT=2 AN=2 MATCH
P60_C/E/+8 AFTER:      RT 1/2/5  AN 1/2/5 MATCH
NCBAPIVOT_BRANCH:      MATCH (p60+8=5)
FINAL_ARTIFACT_INTEGRITY: PASS
800979D8_DYNAMIC_ROOT_CHAIN: PROVEN (bounded USER_DEMO_4 event; not global)
```

Hashes (do not re-copy logs; verify with `sha256sum -c SHA256SUMS`):

```text
f174997fff608540dcfc26ef988ca8b9cf0f8f8701a26fda05ab524b55a2c5da  metadata.txt
db22b240f205cbcafa21d58a1350d1a60463f2c21d285729e9d2741428997840  rt64.log
80c4c42b25d187a13ea30366003fbd71876a229a5b5b002b7ca1923ca2619985  oracle.log
cbc4845fb585e03581f862e82afd318b31f6070bb6026d540d3592202e6bdb8f  oracle-eggs.log
16c659f0ffab746678c509244d424989d62340c98937e3e0133fc95f61d50e00  oracle-motion.csv
c69c6ad2b1fd98b9e432385d265c416935da047596d11bfb52693bb057db8937  oracle-console.log
e0e8da47ee13c3d3a60b5099f9bb213649479a669a0c14d29bf2b841cdb59914  rt64-target-excerpt.txt
985cb18381c77cb8f82e80a43946e1c9d502d2264858b5e3e1cc8ed4a6edaa02  oracle-target-excerpt.txt
```

P08 remains **ACTIVE / MANUAL_VALIDATION_PENDING**. P09 remains
**PENDING / NOT_FORMALLY_OPEN**. USER_DEMO_5 / Talon Trot was not opened.
