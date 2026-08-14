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

**P08 ACTIVE INTEGRATION BLOCKER:** PATCH 4 is accepted. Behavior remains
independently CLASS A, historical verdict **C** is preserved, and the tracked
clean-build finding is resolved by independent verdict B; see
[[plan/evidence/p08-preemption-patch4-clean-build-independent-review]].

Independent reconstruction regenerated 4,298 safepoints deterministically.
The same clean-built binary crossed `func_800C2AB8` with default ON and
reproduced the old freeze with OFF. The original distribution blocker (ignored
untracked scaffolds) is **RESOLVED**. Historical review:
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

**Implement the proven minimal `func_800BCF28` fix.** Root cause level A: the
JAL target `0x800BCCC0` / `0x800BCCD8` is a real unnamed element accessor with
no generated function; `lookup_gap_stub` in `src/game/boot_hw_stubs.cpp`
returns `v0=0`, so `func_800BCF28` dereferences a null base → SIGSEGV at
`rdram+0x8000000c`. Diagnosis: [[plan/evidence/p08-bcf28-crash-diagnosis]].
Do not reopen Hybrid F' / PATCH 1–4. PATCH 4 stays ON.

## Estado do produto validado

1. Launcher/RT64/Vulkan e auto-start OK.
2. Música inicial OK.
3. Intro 3D + logo + Jiggy OK.
4. Attract request + type-4 complete + warp consumido OK.
5. C2AB8 behavior and delivery resolved (clean-built same-binary CLASS A;
   clean tracked Clang build PASS; PATCH 4 accepted by verdict B).
6. **First gameplay demo ausente: `func_800BCF28` SIGSEGV — root cause PROVEN
   (A): `lookup_gap_stub` masks real accessor `0x800BCCC0`/`0x800BCCD8`
   (v0=0 → null deref).** [[plan/evidence/p08-bcf28-crash-diagnosis]].
   Fix not implemented.
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
  **permanentemente** (PATCH 4 CLASS A; novo SIGSEGV em `func_800BCF28`,
  **root cause PROVEN** — ver NEXT)
- `func_800BCF28` SIGSEGV como divergence não-diagnosticada — **diagnosed**
  (root cause A; fix pendente)

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

1. [[plan/evidence/p08-bcf28-crash-diagnosis]] (**CURRENT** — root cause A)
2. [[plan/evidence/p08-preemption-patch4-clean-build-independent-review]] (verdict B; PATCH 4 accepted)
2. [[plan/evidence/p08-preemption-patch4-clean-build-correction]] (implementer correction)
3. [[plan/evidence/p08-preemption-patch4-independent-review]] (historical verdict C preserved; finding resolved)
4. [[plan/evidence/p08-preemption-patch4-validation]] (implementer behavioral evidence)
5. [[plan/evidence/p08-preemption-implementation-plan]]
6. [[plan/evidence/p08-session-handoff]]
7. [[plans/BT/08-rt64-input-and-frontend]]
