---
title: "P08 — attract-loop crash after DEMO2"
description: "Deterministic SIGSEGV after the second gameplay demo: bsrest_entrypoint_11 LOOKUP_FUNC(0x808003FC) binds to baflameline_entrypoint_5 (overlay VRAM collision). No production fix."
status: evidence
phase: P08
updated: 2026-08-16
tags: [tooie-recomp, p08, attract, demo2, sigsegv, bsrest, baflameline, overlay, lookup]
---

# P08 — attract-loop crash after DEMO2

```text
BASE_HEAD:                 0fb2b55dd3ba46823b4d9291d33c772b2cfd0ab8
MODEL:                     Grok 4.6 HIGH
MODE:                      DIAGNOSIS ONLY (no production fix)
CRASH_PREEXISTS_CURRENT_COLLISION_WORK: YES
4dbcbad / 0fb2b55 blamed:  NO (REFUTED as introducers)
FIX_IMPLEMENTED:           NO
```

This is the current P08 blocker. Fire-egg collision, cinematic-facing, and
T10 results are preserved and not reopened.

## 1. User observation vs actual sequence

User expected:

```text
INTRO → GAMEPLAY DEMO 1 → GAMEPLAY DEMO 2 → INTRO → DEMO 3 → ...
```

`gsattract` table `D_808003D0` first cycle (6-byte records) is:

| sub | map | name |
| ---: | ---: | --- |
| 0 | `0x014F` = 335 | `MAP_14F_IOH_WOODED_HOLLOW` |
| 1 | `0x0116` = 278 | `MAP_116_TDL_INSIDE_MOUNTAIN` |
| 2 | `0x013A` = 314 | `MAP_13A_CCL_CENTRAL_CAVERN` |
| 3 | `0x018B` = 395 | `MAP_18B_CS_STARTUP_INTRO` |

The crash is **not** “DEMO2 gameplay is corrupt”. It is the first unique
event of the third attract step (CCL init) after DEMO2 (TDL) completes.

`USER_EXPECTED_INTRO_AFTER_DEMO2`: **CORRECTED/SUPERSEDED** — original
attract inserts CCL (314) before CS intro (395). The process dies during
that CCL/world/player init, so the user never sees intro return.

## 2. Clean reproduction

```sh
TOOIE_AUTO_START=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json \
timeout -k 2s 360s ./build/TooieRecompiled
```

```text
CURRENT_CRASH_REPRODUCIBLE: YES
EXIT_CODE:                  139
SIGNAL:                     SIGSEGV
DURATION:                   ~99s
LAST_OVL_LOAD:              idx=148 bashoes
```

GDB (`[Game] MAIN`):

```text
FAULTING_HOST_FUNCTION:  func_800EE7F8 + 12
FAULTING_INSN:           mov (%rcx,%rax,1),%eax
FAULT_ACCESS:            READ
FAULT_ADDRESS:           rcx=0x80000000 + rax=rdram+0x74
                         (guest a1 = 0x74, non-canonical host VA)
CURRENT_GUEST_FUNCTION:  func_800EE7F8
CURRENT_GUEST_PC:        0x800EE7F8
THREAD:                  [Game] MAIN
```

Guest call chain (GDB):

```text
gsworldDll_entrypoint_1
→ func_800F732C
→ func_800F482C
→ func_800F3880
→ func_80098E64
→ bs_setState
→ recomp_syscall_handler          (ovl=84 bsrest ep=11)
→ bsrest_entrypoint_11
→ LOOKUP_FUNC(0x808003FC)
→ baflameline_entrypoint_5        <-- wrong overlay
→ func_800EE7F8                   FAULT (lwc1 0(a1), a1=0x74)
```

## 3. Transitions T1 / T2 / T3

Compact `[tooie-xition]` markers (always-on, change-only) on HEAD plus
`ovl_unload` lines. Instrumented run: `/tmp/tooie-p08-attract/xition-repro.log`
(not committed). EXIT=139, same signature.

```text
T1 = INTRO → DEMO1 Wooded Hollow 335
     mode=3 map=335 entr=8 warp=0 then warp=3
     → mode=2 → mode=5
     attract stage=0 sub=0
     bsrest ep=11: NOT ENTERED

T2 = DEMO1 → DEMO2 TDL Inside Mountain 278
     mode=5 map=278 entr=7 warp=0 then warp=3
     → mode=2 → mode=5
     attract stage=0 sub=1
     bsrest ep=11: NOT ENTERED

T3 = DEMO2 → next attract record CCL 314
     mode=5 map=314 entr=8 warp=0 then warp=3
     → mode=2 overlay burst (same 635/178/…/148 set as historical)
     → FIRST bsrest_entrypoint_11
     lookup808003FC = baflameline_entrypoint_5  (flameline=1)
     a0=801d7e40 (player) a1=00000074
     SIGSEGV before mode=5 returns
```

```text
LAST_EVENT_IDENTICAL_TO_WORKING_TRANSITIONS:
  gsattract advance + warp 3 + mode=2 world reload
  (T1 and T2 both complete this and reach mode=5)

FIRST_UNIQUE_EVENT_BEFORE_CRASH:
  ovl=84 ep=11 during T3 mode=2 player init
  LOOKUP_FUNC(0x808003FC) == baflameline_entrypoint_5

CRASH_PHASE: F
  (next-scene / player initialization after DEMO2 teardown;
   overlays for the next world already loaded; not DEMO2 gameplay)
```

`LAST_MODE=2` `LAST_MAP=0` (transition; last real map 314)
`LAST_WARP=0` `LAST_ATTRACT_INDEX=stage=0 sub=2`
`LAST_ACTIVE_OVERLAY=148 bashoes` then syscall `84:11`

## 4. First incorrect call (PROVEN)

Original `.bsrest` at `0x808003FC` is an **unnamed** helper:

```text
808003FC  addiu  sp,sp,-24
80800400  sw     ra,20(sp)
80800404  jal    func_800A0C20
80800408  sw     a0,24(sp)
          beqz   v0, ret
          move   a1,zero
          jal    func_800A0D14
          lui    a2,0x3F00      ; 0.5f
```

`baflameline_entrypoint_5` is **also** at VRAM `0x808003FC` (different overlay):

```text
lw     a0, 0x84(a0)
jal    func_800EE7F8
addiu  a0, a0, 0xC
```

Original `func_800EE7F8` (matches generated; READ-ONLY compare):

```text
lwc1 f4, 0(a1) / swc1 f4, 0(a0)
lwc1 f6, 4(a1) / swc1 f6, 4(a0)
lwc1 f8, 8(a1) / jr ra ; swc1 f8, 8(a0)
```

Generated `bsrest_entrypoint_11` (`RecompiledFuncs/funcs_139.c`):

```c
LOOKUP_FUNC(0x808003FC)(rdram, ctx);
```

Same LOOKUP in `bsrest_entrypoint_3` and `_7`. There is **no**
`func_808003FC_bsrest`. The cf31166 same-section JAL fix cannot emit a
direct call because the target is not a Tooie symbol.

Live: `get_function(0x808003FC) == baflameline_entrypoint_5` on the
faulting syscall (`flameline=1`).

```text
CALLER_GUEST_PC:          0x80800668  (bsrest_entrypoint_11 jal)
TARGET_GUEST_ADDRESS:     0x808003FC
LOOKUP_RESULT:            baflameline_entrypoint_5
OVERLAY_LOADED_FOR_TARGET: baflameline (130) loaded once early;
                          last mapped text at crash is bashoes (148)
TARGET_OVERLAY_LOADED:    NO (not the active 0x80800000 occupant)
ORIGINAL_MIPS_MATCH:      NO (wrong callee)
```

This is the same *class* as [[p08-overlay-rmips26-binding-diagnosis]]
(intra-overlay JAL + shared `0x80800000` `func_map`), but a residual:
unnamed same-section targets that collide with another overlay's **named**
entrypoint still go through `LOOKUP_FUNC`.

`LOOKUP_FUNC(0)` (resolved earlier) is a different defect and is not reopened.

## 5. Lifetime

Not a proven use-after-free of a heap object.

```text
FAULTING_OBJECT_OR_POINTER:  ctx->r5 / $a1 = 0x74
ALLOCATED_AT:                leftover GPR (never a KSEG0 vec3f)
FIRST_USED_AT:               func_800EE7F8 lwc1 0(a1) via wrong callee
FREED_OR_INVALIDATED_AT:     NA
FAULTING_USE_AT:             func_800EE7F8 + 0
GENERATION / OVERLAY OWNER:  baflameline static VRAM 0x808003FC
                             wins lookup; local .bsrest helper never generated
```

`a0=0x801d7e40` is a plausible player pointer. The dest of the *correct*
helper would have been that actor. The crash is the first load from the
wrong function's `a1`.

## 6. Range / bisect

```text
KNOWN_BAD:   337f6dd / 3aa3ebb / current HEAD
             (later-attract EXIT=139 already recorded; this session
             proves the same signature on 0fb2b55)
KNOWN_GOOD:  not established
FIRST_BAD_COMMIT: not isolated
```

`RecompiledFuncs/` is gitignored, so generated LOOKUP lines cannot be
blamed. Generator commit `cf31166` (same-section JAL direct-bind) is an
ancestor of HEAD and is **incomplete** for unnamed colliding targets.
Live binary bisect of attract-era docs/fix commits would only restate
BAD: those trees still carry this generated LOOKUP.

No GOOD ancestor for `INTRO → DEMO1 → DEMO2 → next INTRO visible` was
found. Do not invent one.

`4dbcbad` / `0fb2b55` (collision-volume work) are **REFUTED** as
introducers.

## 7. Root

```text
ROOT_CAUSE_LEVEL: A
ROOT_CAUSE:       bsrest jal 0x808003FC is an unnamed same-section
                  helper; generated LOOKUP_FUNC binds
                  baflameline_entrypoint_5; that function treats the
                  player as a flameline object and reads a1=0x74
CAUSAL_LINK:      PROVEN (GDB + original MIPS + generated C + live
                  get_function flameline=1 on the crashing syscall)
FIX_OWNER_CANDIDATE:
  N64Recomp: emit unnamed intra-overlay functions and same-section
             direct calls (extend cf31166)
  or Tooie metadata: add a .bsrest symbol at 0x808003FC and regen
FIX_IMPLEMENTED:  NO
```

A null-guard on `func_800EE7F8` or skipping rest would hide the SIGSEGV
without restoring original `func_800A0C20` / `func_800A0D14` semantics.

## 8. Tests

```text
stick_angle_thunk_test:                 PASS
matrix_submission_test:                 PASS
input_direction_test:                   PASS
bcf28_helpers_test:                     PASS
core1_ra_wrappers_test:                 PASS
TooieRecompiled build:                  PASS
tests/attract_demo2_transition_jal_test.py: FAIL
  (documents current LOOKUP in ep 11/3/7; expected until generator/metadata fix)
```

```sh
python3 tests/attract_demo2_transition_jal_test.py
```

## 9. Markers left in tree

Always-on compact `[tooie-xition]` in `recomp_syscall_handler` and
`[tooie] ovl_unload:` in `ovl_unload`. Same style as existing
`ovl_load` prints. No `TOOIE_DIAG` timing path. No `RecompiledFuncs` edit.
