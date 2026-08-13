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

> **RECONSTRUÇÃO SCHEDULER/DRIVER (2026-08-12):** O ciclo causal de limpeza de `table[2].word0` foi integralmente provado estaticamente:
> - O cliente `SoundManager` (`D_800411DC`) é registrado por `func_8001EB10` com handler `func_8001ED68` em `+0x8`.
> - A síntese de áudio por frame (`func_800128C0 → func_800129FC → func_80021574`) executa `jalr client->handler` (`func_8001ED68`).
> - `func_8001ED68` desempilha eventos expirados da delta-queue e chama `func_8001EE34(event)`.
> - Para opcode `0x200`, `func_8001EE34` chama `func_800200B8 → func_800205D0`, que zera `*obj->0x30` (`table[2].word0 = 0`).
> - No N64 original, a thread de áudio (OSThread 4) preempta MAIN e avança a síntese. No TooieRecomp, o spin síncrono de MAIN em `func_800C2AB8` impede a execução do dispatcher de áudio, causando inanição do clearer. Evidência: [[plan/evidence/p08-slot7-scheduled-event-driver]].
>
> **VALIDAÇÃO DINÂMICA CLEAN FREEZE (2026-08-12):** 3 clean freezes determinísticos
> (`d=179285/179325/179310`) confirmam — **CLASS A** — que o objeto de `table[2]`
> **ESTÁ agendado** na delta-queue do SoundManager (`*(0x800411DC)=0x8007E9C0`,
> listhdr `+0x14`): entrada na HEAD com `payload.p4 == table[2].word0` e
> **delta-key 0 (devida agora)**; handler `mgr+0x08 = func_8001ED68` (MATCH);
> `func_8001EE34` pending. O contexto de dispatch (thread `[Game] AUDIO`) está
> **estacionado em `LightweightSemaphore::waitWithPartialSpinning`** e os
> contadores de áudio do diag ficam **congelados** no freeze
> (`rsp_aud=1529`, `queue_samp=2305888`) — MAIN gira em `func_800C2AB8` sem
> ceder, então `func_8001ED68/1EE34/200B8/205D0` não podem rodar.
> Evidência: [[plan/evidence/p08-slot7-scheduled-event-validation]].


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
