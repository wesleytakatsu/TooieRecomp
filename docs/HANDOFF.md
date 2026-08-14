---
title: "Handoff — TooieRecomp"
description: "Estado global corrente do port, com prioridade P08 no handoff intro para attract demo."
status: current
type: handoff
project: TooieRecomp
updated: 2026-08-14
tags: [tooie-recomp, handoff, p08, intro, attract, slot7, lifecycle, bcf28, 80013a7c]
---

# Handoff — TooieRecomp

## Prioridade atual

**P08 ACTIVE INTEGRATION BLOCKER:** PATCH 4 is accepted and default ON. The
`func_800BCF28` binding fix is **ACCEPTED (verdict B)**. The current first
product divergence `func_80013A7C` ("Failed to find function at 0x00000000") is
**DIAGNOSED (Level A)** in [[plan/evidence/p08-80013a7c-lookup-zero-diagnosis]]:
non-standard leaf wrapper saves RA in `$a3`, but direct host calls do not
materialize `ctx->r31`, so `jr a3` evaluates `LOOKUP_FUNC(0)`.

Historical context:
> **PATCH 4 (2026-08-13):** Tooie-scoped startup ON;
> `ULTRAMODERN_GUEST_PREEMPTION=0` rollback. Regen real (4298 safepoints).
> ON: C2AB8 retorna, áudio avança, crash em `func_800BCF28`.
> OFF: freeze antigo (d/audio/ovl estáveis ≥58s). Sem yield/clear.
> Evidência: [[plan/evidence/p08-preemption-patch4-validation]].

Handoff técnico: [[plan/evidence/p08-session-handoff]].

## NEXT

**Implement only the proven minimal `func_80013A7C` Tooie-native fix** (and its
companion wrappers `0x800137D4` / `0x80013A5C` if scoped).
Do not reopen the accepted BCF28 helper binding or Hybrid F' / PATCH 1–4.
PATCH 4 stays ON.

## Estado do produto validado

1. Launcher/RT64/Vulkan e auto-start OK.
2. Música inicial OK.
3. Intro 3D + logo + Jiggy OK.
4. Attract request + type-4 complete + warp consumido OK.
5. C2AB8 behavior and delivery resolved (clean-built same-binary CLASS A;
   clean tracked Clang build PASS; PATCH 4 accepted by verdict B).
6. **`func_800BCF28` SIGSEGV RESOLVED** and independently accepted (verdict B;
   [[plan/evidence/p08-bcf28-binding-fix-independent-review]]).
7. First gameplay demo ausente: `func_80013A7C` lookup-zero **DIAGNOSED (Level A)**
   in [[plan/evidence/p08-80013a7c-lookup-zero-diagnosis]]. Fix pending.
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
- **`func_800BCF28` SIGSEGV / `0x800BCCC0` gap stub** — RESOLVED and
  independently accepted (verdict B) by
  [[plan/evidence/p08-bcf28-binding-fix-independent-review]]

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

1. [[plan/evidence/p08-80013a7c-lookup-zero-diagnosis]] (**CURRENT** — root cause Level A diagnosis)
2. [[plan/evidence/p08-bcf28-binding-fix-independent-review]] (verdict B; BCF28 fix accepted)
3. [[plan/evidence/p08-bcf28-binding-fix]] (implementer binding-fix evidence)
4. [[plan/evidence/p08-bcf28-crash-diagnosis]] (root cause A diagnosis)
5. [[plan/evidence/p08-preemption-patch4-clean-build-independent-review]] (verdict B; PATCH 4 accepted)
6. [[plan/evidence/p08-preemption-patch4-clean-build-correction]] (implementer correction)
7. [[plan/evidence/p08-preemption-patch4-independent-review]] (historical verdict C preserved; finding resolved)
8. [[plan/evidence/p08-preemption-patch4-validation]] (implementer behavioral evidence)
9. [[plan/evidence/p08-session-handoff]]
10. [[plans/BT/08-rt64-input-and-frontend]]
