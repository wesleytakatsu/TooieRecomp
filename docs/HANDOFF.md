---
title: "Handoff — TooieRecomp"
description: "Estado global corrente do port, com prioridade P08 no handoff intro para attract demo."
status: current
type: handoff
project: TooieRecomp
updated: 2026-08-13
tags: [tooie-recomp, handoff, p08, intro, attract, slot7, lifecycle]
---

# Handoff — TooieRecomp

## Prioridade atual

**P08 ACTIVE INTEGRATION BLOCKER:** PATCH 4 is behaviorally CLASS A. Independent
review verdict **C** is preserved. Implementer correction now tracks the empty
`RecompiledPatches` scaffolds and reproduces a clean Clang build; see
[[plan/evidence/p08-preemption-patch4-clean-build-correction]]. That
correction is **not independently accepted yet**.

Independent reconstruction regenerated 4,298 safepoints deterministically.
The same binary crossed `func_800C2AB8` with default ON and reproduced the old
freeze with OFF. The original distribution blocker (ignored untracked
scaffolds) has an implementer fix pending review. See
[[plan/evidence/p08-preemption-patch4-independent-review]].

> **PATCH 4 (2026-08-13):** Tooie-scoped startup ON;
> `ULTRAMODERN_GUEST_PREEMPTION=0` rollback. Regen real (4298 safepoints).
> ON: C2AB8 retorna, áudio avança, crash em `func_800BCF28`.
> OFF: freeze antigo (d/audio/ovl estáveis ≥58s). Sem yield/clear.
> Evidência: [[plan/evidence/p08-preemption-patch4-validation]].



Descrição canônica do freeze histórico pré-PATCH4:

> The opening completes through Jiggy and attract request; type-4 completes and
> warp 3 is consumed; mode-5 init freezes with MAIN busy-waiting in
> `func_800C2AB8` because slot 7 stays pending on a live `table[2]` pool object
> that was never cleared by `func_800205D0`.

Handoff técnico: [[plan/evidence/p08-session-handoff]].

## NEXT

**Independently review the PATCH 4 clean-build correction** before
investigating `func_800BCF28`. Evidence:
[[plan/evidence/p08-preemption-patch4-clean-build-correction]].

Não forçar clear; não BaKey; não reabrir Scheduler C / Hybrid F'; não editar
`RecompiledFuncs`; não resetar o dirty do runtime.

## Estado do produto validado

1. Launcher/RT64/Vulkan e auto-start OK.
2. Música inicial OK.
3. Intro 3D + logo + Jiggy OK.
4. Attract request + type-4 complete + warp consumido OK.
5. C2AB8 behavior resolved locally (same-binary CLASS A). Implementer
   clean tracked build now PASS; PATCH 4 delivery not accepted until the
   correction is independently reviewed.
6. First gameplay demo **ausente**: attract/world init reaches **SIGSEGV** in
   `func_800BCF28`; cause not investigated in this review.
7. Title/file-select input diferido.

## Arquitetura e regras

TooieRecomp: `N64Recomp` + `N64ModernRuntime` + `RT64` + `RecompFrontend`.
Não editar `RecompiledFuncs` gerado. Repo Git: `TooieRecomp/`, branch `main`.

## Resolved blockers / do not reopen

- Overlay data relocation / `LOOKUP_FUNC(0)`
- Actor manager artificial spawn
- Resident overlay / gcsection
- Scheduler queue (sem nova prova)
- IPL3 integrity gate
- R_MIPS_26 / do_dma
- Type-4 incomplete / warp gated (clean freeze)
- `func_80026500` as hang PC
- `func_800A907C → func_8001EE34` as object driver (refutado)
- **Freeze de C2AB8 como blocker terminal** — superado causalmente e agora
  **permanentemente** (PATCH 4 CLASS A; novo SIGSEGV em `func_800BCF28`)

## Current expected flow

```text
intro -> Jiggy -> gsattract -> map 335/entrance 8 -> type-4 complete
 -> warp 3 consumed -> A9924 -> A76F4(5) -> mode 5 -> A9A14 -> gsattract ep1
 -> gameplay demo
```

Observed: C2AB8 crossed; crash in `func_800BCF28` (gsworld/basetup/BaKey).
This downstream point is not the next action until the PATCH 4 clean-build
correction is independently reviewed.

## Exonerated (current)

Frame-sync, event-3, cothemedll, type-4 stall, warp non-consumption,
`func_80026500` hang, scheduler corruption (not proven).

## Files to read

1. [[plan/evidence/p08-preemption-patch4-clean-build-correction]] (**CURRENT** — implementer correction; pending review)
2. [[plan/evidence/p08-preemption-patch4-independent-review]] (verdict C preserved)
3. [[plan/evidence/p08-preemption-patch4-validation]] (implementer behavioral evidence)
4. [[plan/evidence/p08-preemption-patch35-rcp-independent-review]] (PATCH 3.5 RCP accepted)
5. [[plan/evidence/p08-preemption-implementation-plan]]
6. [[plan/evidence/p08-session-handoff]]
7. [[plans/BT/08-rt64-input-and-frontend]]
