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

**P08 ACTIVE INTEGRATION BLOCKER:** PATCH 4 is behaviorally CLASS A, but is
not distributable from the stated tracked state.

Independent reconstruction regenerated 4,298 safepoints deterministically.
The same binary crossed `func_800C2AB8` with default ON and reproduced the old
freeze with OFF. However, a strict clean clone cannot build because required
ignored `RecompiledPatches` scaffolds have no tracked regeneration step.
Verdict: **C — correction required**. See
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

**Correct PATCH 4 tracked clean-build reproducibility.** The clean checkout
must recreate every required `RecompiledPatches` input and build without
hidden local files. Re-review that correction before investigating
`func_800BCF28`.

Não forçar clear; não BaKey; não reabrir Scheduler C / Hybrid F'; não editar
`RecompiledFuncs`; não resetar o dirty do runtime.

## Estado do produto validado

1. Launcher/RT64/Vulkan e auto-start OK.
2. Música inicial OK.
3. Intro 3D + logo + Jiggy OK.
4. Attract request + type-4 complete + warp consumido OK.
5. C2AB8 behavior resolved locally (same-binary CLASS A); PATCH 4 delivery
   not accepted until clean tracked build is reproducible.
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
This downstream point is not the next action until PATCH 4 distribution is
corrected.

## Exonerated (current)

Frame-sync, event-3, cothemedll, type-4 stall, warp non-consumption,
`func_80026500` hang, scheduler corruption (not proven).

## Files to read

1. [[plan/evidence/p08-preemption-patch4-independent-review]] (**CURRENT** — verdict C)
2. [[plan/evidence/p08-preemption-patch4-validation]] (implementer evidence)
3. [[plan/evidence/p08-preemption-patch35-rcp-independent-review]] (PATCH 3.5 RCP accepted)
4. [[plan/evidence/p08-preemption-implementation-plan]]
5. [[plan/evidence/p08-session-handoff]]
6. [[plans/BT/08-rt64-input-and-frontend]]
