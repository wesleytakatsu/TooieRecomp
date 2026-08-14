---
title: "Handoff — TooieRecomp"
description: "Estado global corrente do port, com prioridade P08 no handoff intro para attract demo."
status: current
type: handoff
project: TooieRecomp
updated: 2026-08-14
tags: [tooie-recomp, handoff, p08, intro, attract, slot7, lifecycle, bcf28]
---

# Handoff — TooieRecomp

## Prioridade atual

**P08 ACTIVE INTEGRATION BLOCKER:** PATCH 4 is accepted and default ON. The
`func_800BCF28` binding defect is **RESOLVED** by [[plan/evidence/p08-bcf28-binding-fix]]:
`0x800BCCC0` and `0x800BCCD8` are bound to exact Tooie-native recomp helpers
in `src/game/boot_hw_stubs.cpp`, removing them from `kLookupGapAddrs`.

Historical context:
> **PATCH 4 (2026-08-13):** Tooie-scoped startup ON;
> `ULTRAMODERN_GUEST_PREEMPTION=0` rollback. Regen real (4298 safepoints).
> ON: C2AB8 retorna, áudio avança, crash em `func_800BCF28`.
> OFF: freeze antigo (d/audio/ovl estáveis ≥58s). Sem yield/clear.
> Evidência: [[plan/evidence/p08-preemption-patch4-validation]].

Handoff técnico: [[plan/evidence/p08-session-handoff]].

## NEXT

**Independent Grok 4.6 High review of the narrow `func_800BCF28` binding fix**
before diagnosing the next downstream blocker (`func_80013A7C`).
Do not reopen Hybrid F' / PATCH 1–4. PATCH 4 stays ON.

## Estado do produto validado

1. Launcher/RT64/Vulkan e auto-start OK.
2. Música inicial OK.
3. Intro 3D + logo + Jiggy OK.
4. Attract request + type-4 complete + warp consumido OK.
5. C2AB8 behavior and delivery resolved (clean-built same-binary CLASS A;
   clean tracked Clang build PASS; PATCH 4 accepted by verdict B).
6. **`func_800BCF28` SIGSEGV RESOLVED** (Level A fix implemented and validated in
   [[plan/evidence/p08-bcf28-binding-fix]]).
7. First gameplay demo ausente: novo blocker downstream em `func_80013A7C`
   (`Failed to find function at 0x00000000`).
8. Title/file-select input diferido.

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
  **permanentemente** (PATCH 4 CLASS A)
- **`func_800BCF28` SIGSEGV / `0x800BCCC0` gap stub** — RESOLVED by
  [[plan/evidence/p08-bcf28-binding-fix]]

## Current expected flow

```text
intro -> Jiggy -> gsattract -> map 335/entrance 8 -> type-4 complete
 -> warp 3 consumed -> A9924 -> A76F4(5) -> mode 5 -> A9A14 -> gsattract ep1
 -> gameplay demo
```

Observed: C2AB8 crossed; `func_800BCF28` completes; world init executes;
stops at `func_80013A7C` (`Failed to find function at 0x00000000`).

## Exonerated (current)

Frame-sync, event-3, cothemedll, type-4 stall, warp non-consumption,
`func_80026500` hang, scheduler corruption (not proven), `func_800BCF28` binding.

## Files to read

1. [[plan/evidence/p08-bcf28-binding-fix]] (**CURRENT** — BCF28 helper bindings fix)
2. [[plan/evidence/p08-bcf28-crash-diagnosis]] (root cause A diagnosis)
3. [[plan/evidence/p08-preemption-patch4-clean-build-independent-review]] (verdict B; PATCH 4 accepted)
4. [[plan/evidence/p08-preemption-patch4-clean-build-correction]] (implementer correction)
5. [[plan/evidence/p08-preemption-patch4-independent-review]] (historical verdict C preserved; finding resolved)
6. [[plan/evidence/p08-preemption-patch4-validation]] (implementer behavioral evidence)
7. [[plan/evidence/p08-session-handoff]]
8. [[plans/BT/08-rt64-input-and-frontend]]
