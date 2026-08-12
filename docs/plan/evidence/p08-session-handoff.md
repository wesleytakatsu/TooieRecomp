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
- Checkpoint base/parent: `28634288c1c17ac0cb2a75371d911e1889f33908`
  (slot7 readiness loop correction). Arbitration docs land on top of this.
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
ROOT_CAUSE_PROVEN: YES
```

O mecanismo que limpa `table[2].word0` foi integralmente reconstruído estaticamente:
- O despachante de eventos `func_8001ED68` é registrado em `D_800411DC + 0x8` por `func_8001EB10`.
- A síntese de áudio (`func_800128C0 → func_800129FC → func_80021574`) executa chamada indireta (`jalr client->handler`).
- `func_8001ED68` consome a delta-queue e chama `func_8001EE34`, que no término da voz (opcode `0x200`) chama `func_800200B8 → func_800205D0`.
- `func_800205D0` zera `*obj->0x30 = 0` (`table[2].word0 = 0`), permitindo que `func_800C269C(7)` retorne 1 e `func_800C2AB8` termine.
- **Falha no TooieRecomp:** `func_800C2AB8` roda em busy-wait na thread MAIN enquanto a thread de áudio / processamento de síntese não é despachada concorrentemente no host, gerando inanição do clearer.

## CURRENT_FIRST_DIVERGENCE

MAIN entra em `func_800C2AB8` no path de transição de modo (`func_800A76F4(5) → func_800A7380(1) → func_800C2AB8`) com uma voz de áudio ativa vinculada ao slot 7 (`table[2].word0 = 0x8005C168`), e o busy-wait síncrono no host impede o progresso do scheduler de áudio que chamaria `func_800205D0`.


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
2. Config identity at `object+0x08` (DeepSeek saw `0x801301E0`) — field dump.
3. Whether original expects pre-clear before C2AB8 (A) or async clear during wait (C).
4. Whether host audio/event thread that jalr-dispatches into `1ED68`/`1EE34` is runnable while MAIN spins.
5. Meaning of any observation at `0x8004E5A8` (not found statically).
6. BaKey only after attract init progresses — possible downstream only.

## NEXT_SINGLE_ACTION

Run the **minimal post-freeze discriminator** in
[[p08-slot7-object-lifecycle-arbitration#17-next-single-action-discriminator-only--do-not-implement-fix]]:

- dump object `+0x08/+0x34/+0x44/+0x45/+0x30` and config blob
- if needed later: one authorized lightly-instrumented run counting
  `20790`/`1EE34`/`205D0` hits + host thread identity

No fix. No force-clear. No BaKey. No Scheduler C reopen.

## FILES_TO_READ_NEXT

1. [[p08-slot7-object-lifecycle-arbitration]] — **CURRENT arbitration**
2. [[p08-attract-actor-loop-diagnosis]] — freeze mechanism / slot7
3. [[p08-type4-transition-semantics-audit]] — type-4 complete, warp consumed
4. [[p08-intro-to-demo-transition-diagnosis]] — historical timeline
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
EVIDENCE_CHANGED         = YES (lifecycle arbitration)
```
