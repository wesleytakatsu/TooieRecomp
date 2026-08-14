---
title: "P08 — session handoff: intro para attract demo"
description: "Estado autossuficiente após review independente do fix BCF28 (verdict B): C2AB8 e BCF28 aceitos; current first product divergence is func_80013A7C."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, handoff, intro, attract, preemption, patch4, bcf28]
---

# P08 — session handoff: INTRO → ATTRACT DEMO

## CURRENT_HEAD

- Branch: `main`.
- Base commit: `0bb38cc84147701654552e1b714f4a8053e4f481`.
- Binding-fix commit reviewed in [[p08-bcf28-binding-fix-independent-review]]
  (verdict B). Implementation evidence remains [[p08-bcf28-binding-fix]].

## CURRENT_PHASE

P08 — RT64, input e frontend. PATCH 4 is accepted and default ON. The
`func_800BCF28` binding fix is **ACCEPTED (verdict B)** by
[[p08-bcf28-binding-fix-independent-review]]: `0x800BCCC0` and `0x800BCCD8`
are bound to Tooie-native recomp helpers in `src/game/boot_hw_stubs.cpp`.
Nonblocking integer-domain / test notes only. Title/file-select input
continua diferido.

## CURRENT_PRODUCT_STATE

- **`func_800BCF28` SIGSEGV RESOLVED and independently accepted (verdict B)**
  (2026-08-14): Tooie-native helpers bound at `0x800BCCC0` and `0x800BCCD8`.
  Independent run: `func_800BCF28` entered; `0x800BCCC0(a0=0x801D2560,a1=0)`
  expected `v0=0x801D2584`; `basetup` / `gsworldDll` ep2 continue; old
  `0x800BCF58` SIGSEGV absent. See [[p08-bcf28-binding-fix-independent-review]].
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
BCF28_FIX_STATUS:         ACCEPTED (verdict B; TOOIE_NATIVE)
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

**Diagnose `func_80013A7C` / `Failed to find function at 0x00000000`.**
Do not reopen the accepted BCF28 helper binding or PATCH 4 / Hybrid F'.

## FILES_TO_READ_NEXT

0. [[p08-bcf28-binding-fix-independent-review]] — **CURRENT:** verdict B; BCF28 fix accepted
1. [[p08-bcf28-binding-fix]] — implementer binding-fix evidence (preserved)
2. [[p08-bcf28-crash-diagnosis]] — root cause diagnosis (level A, preserved)
3. [[p08-preemption-patch4-clean-build-independent-review]] — verdict B; PATCH 4 accepted
4. `docs/HANDOFF.md`, `docs/plans/BT/08-rt64-input-and-frontend.md`

## CHECKPOINT SCOPE

```text
CODE_CHANGED             = NO (this checkpoint is the independent review)
RUNTIME_CODE_CHANGED     = NO
GENERATOR_CODE_CHANGED   = NO
PATCH_CODE_CHANGED       = NO
RECOMPILEDFUNCS          = NOT EDITED (untouched)
BCF28_REVIEW_VERDICT     = B
```
