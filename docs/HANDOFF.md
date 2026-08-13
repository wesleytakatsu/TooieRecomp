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

**P08 ACTIVE BLOCKER:** first attract/gameplay demo não começa após intro/Jiggy.

Estado clean-freeze (determinístico):

- type-4 **completa**; `warp=3` **consumido**; mode-5 alcançado;
- MAIN presa em `func_800A76F4 → func_800A7380 → func_800C2AB8`;
- único pending: kinematics **slot 7** (`substate=2`, `table[2].word0` = live
  pool object, p.ex. `0x8005C168`) → `func_800C269C(7)==0` para sempre.

> **VALIDAÇÃO CAUSAL DO GAP DE PREEMPÇÃO (2026-08-12):** experimento limpo
> reversível prova causalmente o gap. Strong override temporário de
> `func_800C2AB8` chamando a primitiva existente `yield_self_1ms(rdram)` por
> passada com pending: a thread AUDIO rodou (contadores de áudio avançaram o run
> todo), `func_800205D0` limpou `table[2].word0` (0x8005C1B0 → 0x00000000), o
> slot 7 foi dispensado por `func_800C16A4`, `func_800C2AB8` RETORNOU e o
> attract progrediu até um **SIGSEGV downstream NOVO** em `func_800BCF28`
> (chain gsworldDll/basetup/input, fora do escopo). **CLASS A — CAUSAL
> CONFIRMATION.** Patch revertido; nenhum código permanece.
> Evidência: [[plan/evidence/p08-guest-preemption-causal-validation]].



Descrição canônica:

> The opening completes through Jiggy and attract request; type-4 completes and
> warp 3 is consumed; mode-5 init freezes with MAIN busy-waiting in
> `func_800C2AB8` because slot 7 stays pending on a live `table[2]` pool object
> that was never cleared by `func_800205D0`.

Handoff técnico: [[plan/evidence/p08-session-handoff]].

## NEXT

**PATCH 3.5 DESIGN COMPLETE.** PATCH 3 accepted (review B).
PATCH4_READY NO. Flag OFF. Sem regen Tooie.
Próximos passos:

1. **Implement PATCH 3.5** segundo
   [[plan/evidence/p08-preemption-patch35-design]].
   MUST: X1 annotated-hold + MTC0/mask-change service.
   TEST: X1-A..F, M1/M2, T04+, T07+, P3-X1/X2/X4/X6.
   DEFER: generic JR, function-entry safepoints, status-init, TLS epoch.
   Não habilitar a flag. Não regenerar Tooie. Não implementar Hybrid F
   DeepSeek ([[plan/evidence/p08-permanent-preemption-design]], SUPERSEDED).
2. **Investigar `func_800BCF28`** só depois do PATCH 4 aterrar.
3. Então: gsattract ep1 / demo, depois input title/file-select.

Não forçar clear; não BaKey; não reabrir Scheduler C; não editar
`RecompiledFuncs`; não resetar o dirty do runtime.

## Estado do produto validado

1. Launcher/RT64/Vulkan e auto-start OK.
2. Música inicial OK.
3. Intro 3D + logo + Jiggy OK.
4. Attract request + type-4 complete + warp consumido OK.
5. First gameplay demo **ausente**: o freeze de C2AB8 foi **causalmente
   resolvido em experimento** (attract progrediu até world init), mas agora um
   **SIGSEGV downstream** em `func_800BCF28` bloqueia o demo.
6. Title/file-select input diferido.

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
- **Freeze de C2AB8 como blocker terminal** — superado causalmente em
  experimento (attract progride; novo SIGSEGV downstream em `func_800BCF28`)

## Current expected flow

```text
intro -> Jiggy -> gsattract -> map 335/entrance 8 -> type-4 complete
 -> warp 3 consumed -> A9924 -> A76F4(5) -> mode 5 -> A9A14 -> gsattract ep1
 -> gameplay demo
```

Observed: freeze inside `A76F4(5) → A7380 → C2AB8` (slot7 / table[2]).

## Exonerated (current)

Frame-sync, event-3, cothemedll, type-4 stall, warp non-consumption,
`func_80026500` hang, scheduler corruption (not proven).

## Files to read

1. [[plan/evidence/p08-preemption-patch35-design]] (**CURRENT**)
2. [[plan/evidence/p08-preemption-patch3-independent-review]]
3. [[plan/evidence/p08-preemption-patch3-validation]]
4. [[plan/evidence/p08-preemption-patch2-independent-review]]
5. [[plan/evidence/p08-preemption-implementation-plan]]
6. [[plan/evidence/p08-preemption-design-adversarial-review]]
7. [[plan/evidence/p08-permanent-preemption-design]] (SUPERSEDED)
8. [[plan/evidence/p08-guest-preemption-causal-validation]]
9. [[plan/evidence/p08-session-handoff]]
10. [[plans/BT/08-rt64-input-and-frontend]]
