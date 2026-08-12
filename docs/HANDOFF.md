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

> **ARBITRAGEM LIFECYCLE (Grok 4.5 HIGH, 2026-08-12):** criador
> `func_80020790`, clearer `func_800205D0` via `func_800200B8` ←
> `func_8001EE34`; backref `object+0x30` limpa `table[2]`; `+0x34` é countdown
> (init 2, **não** prova origem). C2AB8 **não** avança o objeto (progresso A
> pré-clear e/ou C assíncrono). Origem INTRO vs ATTRACT: **UNKNOWN**. Gemini
> `ROOT_CAUSE YES` / DeepSeek `A907C→1EE34`: **REJECT**.
> `ROOT_CAUSE_PROVEN=NO`. Evidência:
> [[plan/evidence/p08-slot7-object-lifecycle-arbitration]].

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
