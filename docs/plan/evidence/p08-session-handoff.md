---
title: "P08 — session handoff: intro para attract demo"
description: "Estado autossuficiente após diagnóstico do lookup-zero em func_80013A7C: C2AB8 e BCF28 resolvidos; root cause de func_80013A7C PROVEN (Level A)."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, handoff, intro, attract, preemption, patch4, bcf28, 80013a7c]
---

# P08 — session handoff: INTRO → ATTRACT DEMO

## CURRENT_HEAD

- Branch: `main`.
- Base commit: `bbdc304ba717b0c30372365343270e7ccae54ba1`.
- Diagnosis evidence: [[p08-80013a7c-lookup-zero-diagnosis]].

## CURRENT_PHASE

P08 — RT64, input e frontend. PATCH 4 is accepted and default ON. The
`func_800BCF28` binding fix is **ACCEPTED (verdict B)**. The current first
product divergence `func_80013A7C` ("Failed to find function at 0x00000000") is
**DIAGNOSED (Level A)**: non-standard leaf wrapper saves RA in `$a3`, but direct
host C calls do not materialize `ctx->r31`, so `jr a3` evaluates `LOOKUP_FUNC(0)`.
Title/file-select input continua diferido.

## CURRENT_PRODUCT_STATE

- **`func_800BCF28` SIGSEGV RESOLVED and independently accepted (verdict B)**
  (2026-08-14): Tooie-native helpers bound at `0x800BCCC0` and `0x800BCCD8`.
- Intro visual completa (céu, logo, Jiggy).
- Attract request correto; type-4 completa; warp consumido; mode-5 alcançado.
- Hybrid F' startup behavior is independently **CLASS A** in the local
  regenerated binary; rollback OFF reproduces the old freeze.
- `func_800C2AB8` is behaviorally resolved with PATCH4 default ON.
- Clean-build correction tracks the empty `RecompiledPatches` scaffolds.
- **`func_80013A7C` lookup-zero DIAGNOSED (Level A)**: root cause proven; fix
  pending.
- `RecompiledFuncs` é gerado — não editar.

## CURRENT_BLOCKER

**`Failed to find function at 0x00000000` in `func_80013A7C`** — ROOT CAUSE PROVEN
(Level A): `func_80013A7C` performs `move a3, ra; jal 80013abc; jr a3`. Direct
host calls do not write `ctx->r31`, leading to `ctx->r7 == 0` and `LOOKUP_FUNC(0)`.
Identical to precedent `func_800137F4`. See [[p08-80013a7c-lookup-zero-diagnosis]].

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
C2AB8_ROOT_CAUSE_PROVEN:   YES (PATCH 4, CLASS A)
BCF28_ROOT_CAUSE_PROVEN:   YES (Level A; ACCEPTED verdict B)
80013A7C_ROOT_CAUSE_PROVEN: YES (Level A)
80013A7C_FIX_STATUS:       DIAGNOSED (pending implementation)
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
15. **`func_800BCF28` SIGSEGV / `0x800BCCC0` lookup_gap_stub** — RESOLVED and accepted.

Do not reopen these without new evidence.

## OPEN_QUESTIONS

1. Exact creation epoch/call chain RA for the historical table[2] object.
2. Generic non-RA JR still lacks safepoint coverage (deferred).
3. Timer IM7 / PI completion remain approved approximations (deferred).

## NEXT_SINGLE_ACTION

**Implement only the proven minimal `func_80013A7C` Tooie-native fix** (and its
companion wrappers `0x800137D4` / `0x80013A5C` if scoped).

## FILES_TO_READ_NEXT

0. [[p08-80013a7c-lookup-zero-diagnosis]] — **CURRENT:** root cause Level A diagnosis
1. [[p08-bcf28-binding-fix-independent-review]] — verdict B; BCF28 fix accepted
2. [[p08-bcf28-binding-fix]] — BCF28 helper bindings fix evidence
3. [[p08-preemption-patch4-clean-build-independent-review]] — verdict B; PATCH 4 accepted
4. `docs/HANDOFF.md`, `docs/plans/BT/08-rt64-input-and-frontend.md`

## CHECKPOINT SCOPE

```text
CODE_CHANGED             = NO (diagnosis only)
RUNTIME_CODE_CHANGED     = NO
GENERATOR_CODE_CHANGED   = NO
PATCH_CODE_CHANGED       = NO
RECOMPILEDFUNCS          = NOT EDITED (untouched)
```
