---
title: "P08 — session handoff: intro para attract demo"
description: "Estado autossuficiente do blocker pós-Jiggy e ponto exato para a próxima investigação."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, handoff, intro, attract, type-4, slot7, lifecycle, preemption]
---

# P08 — session handoff: INTRO → ATTRACT DEMO

## CURRENT_HEAD

- Branch: `main`.
- Checkpoint base/parent: `9bce91d2d4c7e44b4e198d234abaed95cd2863d4`
  (`docs(p08): finalize preemption implementation plan`). This session
  lands PATCH 1 (stubs + T01) on top.
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
ROOT_CAUSE_PROVEN: YES (causally, via clean dynamic experiment)
```

A causa raiz foi integralmente demonstrada estruturalmente, validada
dinamicamente E agora **provada causalmente por experimento limpo reversível**
(ver [[p08-guest-preemption-causal-validation]]):

- No N64 original, a interrupção de áudio preempta imediatamente o loop CPU de MAIN (`func_800C2AB8`), pois `Status.IE = 1` e Priority(AUDIO)=50 > Priority(MAIN)=20.
- No `N64ModernRuntime`, o agendamento é puramente cooperativo; `external_messages` só são consumidas em chamadas de API (`osRecvMesg`, `osSendMesg`, `osYieldThread`).
- Como `func_800C2AB8` é um loop MIPS puro sem chamadas de runtime, MAIN retém o token de execução host indefinidamente, mantendo a thread AUDIO estacionada em `LightweightSemaphore::waitWithPartialSpinning` e impedindo a execução de `func_800205D0`.
- **EXPERIMENTO CAUSAL (2026-08-12):** strong override temporário de
  `func_800C2AB8` chamando a primitiva existente `yield_self_1ms(rdram)` por
  passada com pending: AUDIO rodou (contadores de áudio avançaram o run todo),
  `func_800205D0` limpou `table[2].word0` (0x8005C1B0 -> 0x00000000), slot7 foi
  dispensado por `func_800C16A4`, `func_800C2AB8` RETORNOU e o attract progrediu
  até um SIGSEGV downstream NOVO em `func_800BCF28` (input/world init, fora do
  escopo). Patch revertido; nenhum código permanece.

## CURRENT_FIRST_DIVERGENCE

Inanição do runtime de threads guest: MAIN retém o execution token do host em `func_800C2AB8` sem ceder CPU cooperativamente, impedindo o processamento da fila de mensagens externas que acordaria a thread AUDIO (prioridade 50). **CAUSALLY PROVEN** por experimento limpo: com uma oportunidade cooperativa (`yield_self_1ms`) o chain `func_8001ED68 → func_8001EE34 → func_800200B8 → func_800205D0` limpa `table[2].word0` e C2AB8 retorna.


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

1. **NOVO BLOCKER downstream:** após C2AB8 retornar (com a prova causal), o
   attract/world init progride até SIGSEGV determinístico em `func_800BCF28`
   (chain gsworldDll_entrypoint_2 → basetup_entrypoint_2 → func_800A4B08 →
   func_80097B4C → bakey_heldCount → func_800BCF28; RIP off 0x656d11). Fora do
   escopo desta tarefa; próximo passo natural.
2. Exact creation epoch/call chain RA for this clean-run object (intro / attract / A76F4 handoff).
3. Whether original expects pre-clear before C2AB8 (A) or async clear during wait (C) — dynamic points to **A (due + dispatch starved)**, origin epoch still open.
4. Host fix location: runtime scheduler opportunity (generic preemption) —
   PROVEN needed. Hybrid F' APPROVED. PATCH 1 (stubs, flag OFF, T01)
   landed — [[p08-preemption-patch1-validation]]. Do not implement
   DeepSeek Hybrid F. Do not start PATCH 2 before independent review.
5. Whether `0x0400`/`0xAB38` opcode event semantics require a chained re-schedule before the 0x200 completion (order of the 2 queued entries).
6. Meaning of any observation at `0x8004E5A8` (not found statically).
7. BaKey only after attract init progresses — possible downstream only.

## NEXT_SINGLE_ACTION

**PATCH 1 LANDED.** Independent review of PATCH 1, then PATCH 2
(runtime service: publish/enqueue/requeue, real service, intmask
translations, T02–T10/T12). Do not start PATCH 3/4.

Não implementar DeepSeek Hybrid F. Não investigar `func_800BCF28`.
Sem force-clear, BaKey ou reopen do Scheduler C. Sem editar
`RecompiledFuncs` à mão. Sem resetar o dirty do runtime.

## FILES_TO_READ_NEXT

1. [[p08-preemption-patch1-validation]] — **CURRENT: PATCH 1 result**
2. [[p08-preemption-implementation-plan]] — remaining PATCH 2–4 spec
3. [[p08-preemption-design-adversarial-review]] — approved architecture F'
4. [[p08-permanent-preemption-design]] — DeepSeek Hybrid F, SUPERSEDED
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
EVIDENCE_CHANGED         = YES (implementation plan for Hybrid F')
```
