---
title: "P08 — session handoff: intro para attract demo"
description: "Estado autossuficiente após o fix nativo dos wrappers RA de core1: C2AB8, BCF28 e o lookup-zero de func_80013A7C resolvidos."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, handoff, intro, attract, preemption, patch4, bcf28, 80013a7c]
---

# P08 — session handoff: INTRO → ATTRACT DEMO

## CURRENT_HEAD

- Branch: `main`.
- Base commit: `a6b024fcb321aab9b64258089d53696fac54d79b` (pre-fix).
- Fix evidence: [[p08-core1-ra-wrapper-fix]].
- Diagnosis: [[p08-80013a7c-lookup-zero-diagnosis]] (resolved by the fix).

## CURRENT_PHASE

P08 — RT64, input e frontend. PATCH 4 is accepted and default ON. The
`func_800BCF28` binding fix is **ACCEPTED (verdict B)**. The
`func_80013A7C` lookup-zero is **RESOLVED** by Tooie-native strong-symbol
overrides of the proven core1 RA-wrapper cluster. Title/file-select input
continua diferido.

## CURRENT_PRODUCT_STATE

- **`func_800BCF28` SIGSEGV RESOLVED and independently accepted (verdict B)**
  (2026-08-14): Tooie-native helpers bound at `0x800BCCC0` and `0x800BCCD8`.
- Intro visual completa (céu, logo, Jiggy).
- Attract request correto; type-4 completa; warp consumido; mode-5 alcançado.
- Hybrid F' startup behavior is independently **CLASS A** in the local
  regenerated binary; rollback OFF reproduces the old freeze.
- `func_800C2AB8` is behaviorally resolved with PATCH4 default ON.
- Clean-build correction tracks the empty `RecompiledPatches` scaffolds.
- **`func_80013A7C` lookup-zero RESOLVED** (Tooie-native RA wrappers;
  [[p08-core1-ra-wrapper-fix]]).
- `RecompiledFuncs` é gerado — não editar.

## CURRENT_BLOCKER

Old `func_80013A7C` `LOOKUP_FUNC(0)` is **RESOLVED**.

New first high-level signature (NOT diagnosed): after native `func_80013A7C`,
a 150s clean run continues with live audio and overlay loads; opening probe
stays at `mode=3 / map=0 / warp=0 / attr=0`. Mode-5 / map-335 / visible
attract were not observed. No new crash string. See
[[p08-core1-ra-wrapper-fix]].

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
80013A7C_FIX_STATUS:       RESOLVED (Tooie-native strong symbols)
```

## CURRENT_FIRST_DIVERGENCE

Non-fatal post-`80013A7C` continuation: opening `mode=3 / map=0 / warp=0`,
no mode-5/attract observed in 150s, no `LOOKUP_FUNC(0)`. Not diagnosed.

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
16. **`func_80013A7C` LOOKUP_FUNC(0) / core1 RA wrappers** — RESOLVED.

Do not reopen these without new evidence.

## OPEN_QUESTIONS

1. Exact creation epoch/call chain RA for the historical table[2] object.
2. Generic non-RA JR still lacks safepoint coverage (deferred).
3. Timer IM7 / PI completion remain approved approximations (deferred).

## NEXT_SINGLE_ACTION

**Independent Gemini 3.7 Flash HIGH review of this narrow RA-wrapper fix**
before diagnosing the next blocker.

## FILES_TO_READ_NEXT

0. [[p08-core1-ra-wrapper-fix]] — **CURRENT:** implementer evidence
1. [[p08-80013a7c-lookup-zero-diagnosis]] — Level A diagnosis (resolved)
2. [[p08-bcf28-binding-fix-independent-review]] — verdict B; BCF28 fix accepted
3. [[p08-preemption-patch4-clean-build-independent-review]] — verdict B; PATCH 4 accepted
4. `docs/HANDOFF.md`, `docs/plans/BT/08-rt64-input-and-frontend.md`

## CHECKPOINT SCOPE

```text
CODE_CHANGED             = YES (Tooie-native RA wrappers + test)
RUNTIME_CODE_CHANGED     = NO
GENERATOR_CODE_CHANGED   = NO
PATCH_CODE_CHANGED       = NO
RECOMPILEDFUNCS          = NOT EDITED (untouched)
```
