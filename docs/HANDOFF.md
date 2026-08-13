---
title: "Handoff — TooieRecomp"
description: "Estado global corrente do port, com prioridade P08 no handoff intro para attract demo."
status: current
type: handoff
project: TooieRecomp
updated: 2026-08-12
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

> **AUDITORIA ESTRUTURAL DE PREEMPÇÃO (2026-08-12):** O gap entre o hardware N64 e o runtime foi integralmente mapeado:
> - **N64 Original:** Interrupts habilitados (`Status.IE = 1`) durante `func_800C2AB8`; interrupção de áudio/timer acorda `OSThread 4` (Priority 50) que preempta imediatamente `OSThread 6` MAIN (Priority 20) via hardware context switch.
> - **N64ModernRuntime (`ultramodern`):** Escalonamento puramente cooperativo com token único de execução. `external_messages` só são processadas quando a thread atual chama a API do runtime (`osRecvMesg`, `osSendMesg`, `osYieldThread`).
> - Como `func_800C2AB8` é um loop MIPS puro em código guest, MAIN nunca cede o token e AUDIO permanece estacionada em `LightweightSemaphore::waitWithPartialSpinning`.
> - Evidência: [[plan/evidence/p08-guest-preemption-runtime-audit]].



Descrição canônica:

> The opening completes through Jiggy and attract request; type-4 completes and
> warp 3 is consumed; mode-5 init freezes with MAIN busy-waiting in
> `func_800C2AB8` because slot 7 stays pending on a live `table[2]` pool object
> that was never cleared by `func_800205D0`.

Handoff técnico: [[plan/evidence/p08-session-handoff]].

## NEXT

Discriminador mínimo pós-freeze (sem fix):

1. dump do objeto em `table[2]` (`+0x08/+0x34/+0x44/+0x45/+0x30` + config);
2. se autorizado depois: contadores de `20790` / `1EE34` / `205D0` + thread.

**RESOLVIDO (2026-08-12, clean dynamic):** o discriminador do objeto `table[2]`
foi capturado em 3 clean freezes: objeto agendado na delta-queue do SoundManager
(head, delta 0, `payload.p4 == table[2].word0`), handler `func_8001ED68`
registrado, `func_8001EE34` pending, thread AUDIO estacionada (scheduler
cooperativo) e contadores de áudio congelados → **CLASS A** (callback agendado e
devido, mas contexto de dispatch não pode rodar). Próximo passo: decidir o ponto
de serviço do dispatch de áudio no host / onde MAIN deveria ceder; re-check da
ordem dos 2 eventos enfileirados (delta 0 + delta 13294/2997).

Não forçar clear; não BaKey; não reabrir Scheduler C sem prova nova.

## Estado do produto validado

1. Launcher/RT64/Vulkan e auto-start OK.
2. Música inicial OK.
3. Intro 3D + logo + Jiggy OK.
4. Attract request + type-4 complete + warp consumido OK.
5. First gameplay demo **ausente** (freeze mode-5/slot7).
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

1. [[plan/evidence/p08-session-handoff]]
2. [[plan/evidence/p08-slot7-object-lifecycle-arbitration]]
3. [[plan/evidence/p08-attract-actor-loop-diagnosis]]
4. [[plan/evidence/p08-type4-transition-semantics-audit]]
5. [[plans/BT/08-rt64-input-and-frontend]]
