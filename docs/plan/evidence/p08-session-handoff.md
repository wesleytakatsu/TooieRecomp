---
title: "P08 — session handoff: intro para attract demo"
description: "Estado autossuficiente após fix de binding de func_800BCF28: C2AB8 e BCF28 resolvidos; current first product divergence is func_80013A7C."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, handoff, intro, attract, preemption, patch4, bcf28]
---

# P08 — session handoff: INTRO → ATTRACT DEMO

## CURRENT_HEAD

- Branch: `main`.
- Base commit: `870bd840c776c5b8a4e1025e7d9eff73defe8e8e`.
- Implementation commit containing [[p08-bcf28-binding-fix]].

## CURRENT_PHASE

P08 — RT64, input e frontend. PATCH 4 is accepted and default ON. The downstream
`func_800BCF28` binding defect is **RESOLVED** by [[p08-bcf28-binding-fix]]:
`0x800BCCC0` and `0x800BCCD8` are bound to exact Tooie-native recomp helpers
in `src/game/boot_hw_stubs.cpp`, removing them from `kLookupGapAddrs`.
Title/file-select input continua diferido.

## CURRENT_PRODUCT_STATE

- **`func_800BCF28` SIGSEGV RESOLVED** (2026-08-14): exact Tooie-native helpers
  bound at `0x800BCCC0` and `0x800BCCD8` in `src/game/boot_hw_stubs.cpp`.
  `func_800BCF28`, `bakey_heldCount`, `bainput_func_80097B4C`, `basetup_entrypoint_2`,
  and `gsworldDll_entrypoint_2` execute cleanly. See [[p08-bcf28-binding-fix]].
- Intro visual completa (céu, logo, Jiggy).
- Attract request correto; type-4 completa; warp consumido; mode-5 alcançado.
- Hybrid F' startup behavior is independently **CLASS A** in the local
  regenerated binary; rollback OFF reproduces the old freeze.
- `func_800C2AB8` is behaviorally resolved with PATCH4 default ON.
- Clean-build correction tracks the empty `RecompiledPatches` scaffolds.
- Gameplay demo **não** começa: novo blocker downstream em `func_80013A7C`
  (`Failed to find function at 0x00000000`).
- `RecompiledFuncs` é gerado — não editar.

## CURRENT_BLOCKER

**`Failed to find function at 0x00000000` in `func_80013A7C`** (downstream after
successful `gsworldDll` / `basetup` / `func_800EA170` execution).
The old `func_800BCF28` SIGSEGV is completely resolved.

## PROVEN_ROOT_MECHANISM (old C2AB8)

```text
YES — synchronous dependency cycle / busy-wait, starved because MAIN
held the host execution token in a pure MIPS loop:
  table[2].word0 != 0
  -> C269C(7)==0
  -> C2AB8 outer loop infinite
  -> AUDIO cannot run -> 205D0 never clears table[2]
```

## ROOT_CAUSE_STATUS

```text
C2AB8_ROOT_CAUSE_PROVEN:  YES (PATCH 4, CLASS A)
BCF28_ROOT_CAUSE_PROVEN:  YES (Level A)
BCF28_FIX_STATUS:         IMPLEMENTED + VALIDATED (TOOIE_NATIVE)
```

## CURRENT_FIRST_DIVERGENCE

`func_80013A7C` lookup failure (`0x00000000`) reached during downstream world
init after `func_800BCF28` completion.

## RESOLVED_DO_NOT_REOPEN

1. Overlay data relocation / `LOOKUP_FUNC(0)`.
2. Actor manager null from artificial spawn.
3. `func800D6E54` as population pass.
4. Resident overlay / gcsection stale mapping.
5. Scheduler queue insert/remove / `No threads left` (without new proof).
6. IPL3 integrity gate.
7. R_MIPS_26 same-section / `do_dma` SIGSEGV.
8. Type-4 incomplete / warp gated as clean-freeze blocker.
9. Warp not consumed as clean-freeze blocker.
10. `func_80026500` list walk as current hang PC.
11. **`func_800A907C -> func_8001EE34` as object driver** — REFUTED.
12. **"table[2] object NÃO está agendado"** — REFUTED.
13. **C2AB8 cooperative freeze as current blocker** — RESOLVED by PATCH 4.
14. Hybrid F' architecture / PATCH 1 / 2 / 3 / 3.5 internals.
15. **`func_800BCF28` SIGSEGV / `0x800BCCC0` lookup_gap_stub** — RESOLVED by [[p08-bcf28-binding-fix]].

Do not reopen these without new evidence.

## OPEN_QUESTIONS

1. Exact creation epoch/call chain RA for the historical table[2] object.
2. Generic non-RA JR still lacks safepoint coverage (deferred).
3. Timer IM7 / PI completion remain approved approximations (deferred).
4. Root cause of `LOOKUP_FUNC(0)` call site inside `func_80013A7C` (next task).

## NEXT_SINGLE_ACTION

**Independent Grok 4.6 High review of this narrow fix before diagnosing the next blocker.**

## FILES_TO_READ_NEXT

0. [[p08-bcf28-binding-fix]] — **CURRENT:** BCF28 helper bindings implemented & validated
1. [[p08-bcf28-crash-diagnosis]] — root cause diagnosis (level A)
2. [[p08-preemption-patch4-clean-build-independent-review]] — verdict B; delivery accepted
3. [[p08-preemption-patch4-validation]] — implementer evidence, preserved
4. `docs/HANDOFF.md`, `docs/plans/BT/08-rt64-input-and-frontend.md`

## CHECKPOINT SCOPE

```text
CODE_CHANGED             = YES (src/game/boot_hw_stubs.cpp, tests/bcf28_helpers_test.cpp, CMakeLists.txt)
RUNTIME_CODE_CHANGED     = NO
GENERATOR_CODE_CHANGED   = NO
PATCH_CODE_CHANGED       = NO
RECOMPILEDFUNCS          = NOT EDITED (untouched)
```
