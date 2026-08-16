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

## 10. Independent structural review (2026-08-16) — NO FIX

This section preserves the Grok diagnosis and records the independent static
arbitration. Conclusions are classified explicitly.

### Original call and generated call — PROVEN

```text
ORIGINAL_CALLER:              bsrest_entrypoint_11
ORIGINAL_CALLER_PC:           0x80800668
ORIGINAL_TARGET:              0x808003FC
ORIGINAL_TARGET_SECTION:      .bsrest (0x80800000..0x80800940)
ORIGINAL_TARGET_SYMBOL:       UNNAMED in the ELF function symbol table;
                              the BT decomp source names its local helper
                              func_808003FC_bsrest
ORIGINAL_TARGET_FIRST_INSTRUCTIONS:
  0x808003FC addiu sp,sp,-24
  0x80800400 sw ra,20(sp)
  0x80800404 jal func_800A0C20
  0x80800408 sw a0,24(sp)
  0x8080040C beqz v0,0x80800420
  0x80800410 lw a0,24(sp)
  0x80800414 move a1,zero
  0x80800418 jal func_800A0D14
  0x8080041C lui a2,0x3F00
ORIGINAL_TARGET_CALLS:        func_800A0C20; conditional func_800A0D14
ORIGINAL_CALL_TARGET_PROVEN:  YES
```

The target is in the same `.bsrest` section as the caller. The current
generated `bsrest_entrypoint_11` (and also `_3` and `_7`) emits
`LOOKUP_FUNC(0x808003FC)`, with no generated local body for that address.

```text
GENERATED_CALL_FORM: LOOKUP
GENERATED_TARGET:   LOOKUP_FUNC(0x808003FC)
RESOLVE_JAL_DECISION_PATH:
  relocatable same-section pre-scan -> no Match because no known generated
  function exists at 0x808003FC in .bsrest ->
  use_lookup_for_all_function_calls=true -> Ambiguous -> LOOKUP_FUNC
FIRST_GENERATOR_DECISION_CAUSING_AMBIGUOUS:
  the same-section fast path requires an existing non-reimplemented,
  non-ignored, non-stubbed function symbol; it does not create one.
```

### Collision and Level-A arbitration — PROVEN

```text
VRAM_808003FC_COLLIDERS:
  baflameline_entrypoint_5          (.baflameline)
  func_808003FC_chnastycavemen      (.chnastycavemen)
  .bsrest target                    unnamed local helper
LIVE_LOOKUP_RESULT:                 baflameline_entrypoint_5
EXPECTED_SECTION_LOCAL_TARGET:     unnamed .bsrest helper at 0x808003FC
```

At the failing syscall, `get_function(0x808003FC)` reports the
`baflameline_entrypoint_5` binding. That callee reaches `func_800EE7F8`, whose
first operation reads `MEM_W(ctx->r5, 0)`. The wrong callee inherits guest
`a1=0x74`; this is a register/provenance consequence of wrong dispatch, not a
freed object.

```text
WRONG_CALLEE_PROVEN:       YES
GENERATED_LOOKUP_CAUSAL:   YES
RUNTIME_COLLISION_CAUSAL:  YES
A1_0x74_IS_NOT_UAF:        YES
ROOT_CAUSE_LEVEL:           A
ROOT_CAUSE:                 an unnamed relocatable same-section JAL target is
                            forced through a shared VRAM lookup and binds to a
                            named function from another overlay.
```

### Bounded structural scope audit — PROVEN / INFERENCE

The audit scanned the linked Tooie ELF's 849 canonical-VRAM relocatable
overlay sections and 66,323 `R_MIPS_26` sites. A site was vulnerable only when
its caller was relocatable, its target was inside the caller's section, there
was no named function at that target in that section, and a named function
existed at the same VRAM in another overlay.

```text
TOTAL_UNNAMED_SAME_SECTION_JAL_TARGETS: 17 sites
UNIQUE_UNNAMED_TARGETS:                 7 addresses
TOTAL_COLLIDING_UNNAMED_TARGETS:        7 addresses
TOTAL_CURRENT_LOOKUP_CALLS_AT_RISK:    17 sites (lookup-for-all policy)
AFFECTED_OVERLAYS:                      .bsrest only
UNIQUE_TARGETS:
  0x80800000, 0x8080003C, 0x80800080, 0x808000E4,
  0x8080020C, 0x8080032C, 0x808003FC
```

Representative `.bsrest` sites include callers at `0x808004C8` and
`0x8080058C` targeting `0x808003FC`, plus the failing caller at
`0x80800668`. The six other addresses are the same unnamed-local pattern in
the same overlay. This proves a class of vulnerable sites; it does not claim
that all 17 are independently observed runtime crashes.

### cf31166 contract and owner arbitration — PROVEN / INFERENCE

```text
CF31166_CONTRACT_CORRECT:       YES
CF31166_MISSING_CASE:           same-section target has no known generated
                                function symbol, so Match is impossible and
                                CreateStatic is bypassed by lookup-all fallback
IS_THIS_A_BUG_IN_CF31166_IMPLEMENTATION: NO
IS_THIS_AN_UNSUPPORTED_METADATA_CASE:    YES
```

cf31166 implements direct binding for a known target function in the same
relocatable section. It does not promise to infer a body for an unnamed
target. N64Recomp already has the required `CreateStatic` path: it creates a
stable `static_<section>_<address>` C identifier, obtains the end at the next
known function/static or section boundary, and emits the body.

```text
OPTION_A_ONE_OFF_METADATA: REJECT
  Fixes 0x808003FC only; the audit leaves the other six local targets.
OPTION_B_SYNTHETIC_METADATA: REJECT as primary owner
  Feasible only with conservative boundary derivation; it duplicates logic
  already present in N64Recomp and risks inventing bodies for non-entry labels.
OPTION_C_N64RECOMP: ACCEPT
  Allow relocatable same-section CreateStatic before the global lookup-all
  fallback; preserve lookup behavior for non-relocatable sections and hooks.
FIX_OWNER: N64RECOMP
RECOMMENDED_FIX_DESIGN:
  In resolve_jal, let an unnamed target inside a relocatable current section
  take the existing CreateStatic path, then regenerate all output.
MINIMAL_FILES:
  N64Recomp/src/recompilation.cpp (external checkout)
  TooieRecomp/patches/runtime/n64recomp-tooie.patch (versioned owner patch)
  tests/attract_demo2_transition_jal_test.py (static contract)
GENERATED_C_EDIT_REQUIRED: NO
FIX_IMPLEMENTED: NO
```

### Historical discrepancy — UNKNOWN / PROVEN negative

The historical cf31166 run recorded approximately 70 reports / 140 seconds /
179k syscalls and `last_ovl=635:2`, but did not record the later
`Wooded Hollow 335 -> TDL 278 -> CCL 314 / bsrest ep11` transition. No old
binary or generated-output snapshot proving that exact route is available.

```text
CF31166_140S_RUN_SEQUENCE_PROVEN: NO
CF31166_AS_KNOWN_GOOD:             NO
UNTRACKED_GENERATED_STATE_MAY_EXPLAIN: UNKNOWN
```

The supported classification is “the old run did not prove the exact
sequence,” not “cf31166 was known-good” and not a guessed binary-state bisect.

### Regression contract — INFERENCE

The static test should assert that every vulnerable target selected by the
audit, including `.bsrest` `0x808003FC`, emits no `LOOKUP_FUNC` and that the
generated local function body exists. The N64Recomp `static_<section>_<address>`
name is an implementation detail; the original address is the stable key.
A generic audit should fail if any relocatable same-section unnamed target
with a cross-overlay VRAM collision remains lookup-bound.

The runtime regression remains: `INTRO -> demo1 -> demo2 -> CCL -> startup
intro continues without crash`, with no guard, attract-index forcing, or state
skipping.
