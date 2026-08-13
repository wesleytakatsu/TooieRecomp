---
title: "P08 — session handoff: intro para attract demo"
description: "Estado autossuficiente do blocker pós-Jiggy e ponto exato para a próxima investigação."
status: evidence
phase: P08
updated: 2026-08-12
tags: [tooie-recomp, p08, handoff, intro, attract, type-4, slot7, lifecycle]
---

# P08 — session handoff: INTRO → ATTRACT DEMO

## CURRENT_HEAD

- Branch: `main`.
- Checkpoint base/parent: `e4c6c0a2763341146f6d79293fea3db3e3168576`
  (slot7 scheduled-event contract; validation docs land on top).
- Confirm with `git rev-parse HEAD` after pull.

## CURRENT_PHASE

P08 — RT64, input e frontend. Prioridade: abrir a first attract/gameplay demo
após intro/Jiggy. Title/file-select input continua diferido.

## CURRENT_PRODUCT_STATE

- Intro visual completa (céu, logo, Jiggy).
- Attract request correto (`map=335`, `entrance=8`, `kind=2`).
- Transition type-4 **completa**; `warp=3` **consumido**; mode-5 alcançado.
- Gameplay demo **não** começa: black freeze na init do mode-5/attract.
- `RecompiledFuncs` é gerado — não editar.

## CURRENT_BLOCKER

**Kinematics slot-7 readiness busy-wait** in `func_800C2AB8`.

```text
slot7 (0x80128F10): enabled=1, mode=3, substate=2
table base 0x80128B08; table[2].word0 = live pool object (e.g. 0x8005C168)
func_800C269C(7)==0 forever -> pending!=0 -> C2AB8 never returns
```

## PROVEN_ROOT_MECHANISM

```text
YES — synchronous dependency cycle / busy-wait:
  table[2].word0 != 0
  -> C269C(7)==0
  -> C2AB8 outer loop infinite
  -> MAIN stuck in mode-5 init (A76F4 -> A7380 -> C2AB8)
```

`func_80026500` as hang PC: **REFUTED**.

## ROOT_CAUSE_STATUS

```text
ROOT_CAUSE_PROVEN: YES (mechanism)
```

Mecanismo que limpa `table[2].word0` reconstruído estaticamente e agora
**validado dinamicamente em 3 clean freezes** (ver
[[p08-slot7-scheduled-event-validation]]):

- O objeto pendente de `table[2]` **ESTÁ agendado** na delta-queue do
  SoundManager (`*(0x800411DC)=0x8007E9C0`, listhdr `+0x14`): entrada na HEAD
  com `payload.p4 == table[2].word0` e **delta-key 0 (devida AGORA)**.
- Handler registrado `mgr+0x08 = 0x8001ED68` (MATCH); `func_8001EE34` pending.
- Contexto de dispatch = thread `[Game] AUDIO` (guest OSThread 4) via
  `func_800128C0 → func_800129FC → func_80021574 → jalr mgr->handler`.
- A thread AUDIO está **estacionada em `LightweightSemaphore::waitWithPartialSpinning`**
  (scheduler cooperativo do runtime); contadores de áudio do diag ficam
  **congelados** durante o freeze (`rsp_aud=1529`, `queue_samp=2305888`).
- MAIN gira em `func_800C2AB8` sem ceder → dispatch de áudio não pode rodar →
  `func_8001ED68/1EE34/200B8/205D0` não executam → `table[2].word0` não zera.
- **CLASS A**: callback agendado e due, mas contexto de dispatch não pode rodar.

## CURRENT_FIRST_DIVERGENCE

MAIN entra em `func_800C2AB8` no path de transição de modo (`func_800A76F4(5) → func_800A7380(1) → func_800C2AB8`) com o evento de áudio do objeto de `table[2]` **ainda agendado e devido** (delta 0) na delta-queue do SoundManager; o busy-wait síncrono no host estaciona a thread AUDIO (scheduler cooperativo), impedindo o dispatch `func_8001ED68 → func_8001EE34 → func_800205D0` que limparia `table[2].word0`.

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
11. **`func_800A907C -> func_8001EE34` as object driver** — REFUTED (static).
12. **"table[2] object NÃO está agendado" / "C — event never registered"** —
    **REFUTED** (clean dynamic: entrada na HEAD da delta-queue, delta 0).

Do not reopen these without new evidence.

## MODEL_DISAGREEMENTS

```text
MODEL_DISAGREEMENT:
Gemini:
  STALE_INTRO audio SFX; ROOT_CAUSE_PROVEN YES; clearer 205D0 via 200B8;
  no clearer before C2AB8.

DeepSeek:
  LIVE attract object; countdown +0x34 still 2; 205D0 never ran; free/active
  lists valid; ROOT_CAUSE_PROVEN NO; driver A907C->1EE34.

ARBITRATION (Grok 4.5 HIGH):
  CLASS C — BOTH PARTIAL.
  ACCEPT: creator 20790, clearer 205D0<-200B8<-1EE34, backref +0x30,
          +0x34 countdown init 2, C2AB8 cannot self-progress,
          CLEARER_BEFORE_C2AB8=NO on transition path,
          dynamic integrity/live/no-205D0.
  REJECT: Gemini ROOT_CAUSE_PROVEN YES; DeepSeek A907C->1EE34;
          STALE_INTRO or ATTRACT origin as proven.
  OBJECT_ORIGIN: UNKNOWN.
  C2AB8 progress source: A and/or C (not B).
  Full write-up: [[p08-slot7-object-lifecycle-arbitration]]
```

## OPEN_QUESTIONS

1. Exact creation epoch/call chain RA for this clean-run object (intro / attract / A76F4 handoff).
2. Whether original expects pre-clear before C2AB8 (A) or async clear during wait (C) — dynamic points to **A (due + dispatch starved)**, origin epoch still open.
3. Host fix location: how to let the audio/event dispatch progress while MAIN spins (runtime scheduling / thread model), NOT a game-code clear.
4. Whether `0x0400`/`0xAB38` opcode event semantics require a chained re-schedule before the 0x200 completion (order of the 2 queued entries).
5. Meaning of any observation at `0x8004E5A8` (not found statically).
6. BaKey only after attract init progresses — possible downstream only.

## NEXT_SINGLE_ACTION

With class A proven (event scheduled + due; dispatch context AUDIO parked), the
root cause is host-side: the audio/event dispatch context cannot run while MAIN
busy-waits. Next investigation (no fix yet):

- decide where the async audio/event dispatch should be serviced in the host
  (runtime thread/event model) vs where MAIN's C2AB8 spin should yield;
- re-check the two queued entries (delta 0 + delta 13294/2997) order vs the
  intended 0x0400→0x200 chained completion.

No fix. No force-clear. No BaKey. No Scheduler C reopen.

## FILES_TO_READ_NEXT

1. [[p08-slot7-scheduled-event-validation]] — **CURRENT dynamic validation (class A)**
2. [[p08-slot7-scheduled-event-driver]] — Gemini static reconstruction
3. [[p08-slot7-object-lifecycle-arbitration]] — arbitration (origin open)
4. [[p08-attract-actor-loop-diagnosis]] — freeze mechanism / slot7
5. `docs/HANDOFF.md`, `docs/plans/BT/08-rt64-input-and-frontend.md`

## CLEAN-FREEZE REPRO

```sh
TOOIE_AUTO_START=1 \
TOOIE_DIAG=1 \
TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json \
./build/TooieRecompiled
```

Run clean → confirm freeze (`d` stable ≥6s, black) → attach only after freeze.

## CHECKPOINT SCOPE

```text
CODE_CHANGED             = NO
RUNTIME_CODE_CHANGED     = NO
GENERATOR_CODE_CHANGED   = NO
PATCH_CODE_CHANGED       = NO
CORE_INCLUDED            = NO
BUILD_ARTIFACT_INCLUDED  = NO
EVIDENCE_CHANGED         = YES (scheduled-event dynamic validation, class A)
```
