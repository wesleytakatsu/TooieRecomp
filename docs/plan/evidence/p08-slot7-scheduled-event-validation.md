---
title: "P08 — slot7 scheduled-event dynamic validation (clean freeze, class A)"
description: "Validação dinâmica do contrato do scheduler reconstruído estaticamente: o objeto pendente de table[2] ESTÁ agendado na delta-queue do SoundManager (D_800411DC+0x14), o handler func_8001ED68 está registrado, e o contexto de dispatch (thread [Game] AUDIO) está estacionado e não pode rodar enquanto MAIN gira em func_800C2AB8."
status: evidence
phase: P08
updated: 2026-08-12
tags: [tooie-recomp, p08, slot7, scheduled-event, clean-freeze, scheduler, audio-thread, C2AB8, class-A]
---

# P08 — SLOT7 SCHEDULED EVENT DYNAMIC VALIDATION

## Meta

```text
MODEL:        DeepSeek V4 Flash — HIGH
BASE_HEAD:    e4c6c0a2763341146f6d79293fea3db3e3168576
MODE:         DYNAMIC CLEAN FREEZE ONLY (no debugger/hook/counter before freeze)
RUNS:         3 clean deterministic runs (freeze d=179285/179325/179310)
METHOD:       supervisor pai (ptrace_scope=1): roda o binário original, confirma
              freeze natural (syscall d estável >=6s), PTRACE_ATTACH SOMENTE
              depois do freeze, lê /proc/pid/mem (nunca muta guest memory)
OBJECTIVE:    Validar dinamicamente o contrato reconstruído pelo Gemini:
              o objeto pendente de table[2] está agendado? qual callback?
              func_8001EE34 está pending? qual atraso? qual contexto despacha?
              esse contexto pode rodar enquanto MAIN gira em C2AB8?
```

## Respostas resumidas (prompt §1)

```text
Is the pending table[2] object scheduled?      YES — head da delta-queue do
                                               SoundManager (0x8005Cxxx, delta=0)
What callback is registered for it?            func_8001ED68 (mgr+0x08, confirmado
                                               0x8001ED68) -> dispatch func_8001EE34
Is func_8001EE34 pending?                      YES — evento enfileirado com opcode
                                               válido de func_8001EE34
What is its remaining delay?                   delta-queue head = 0 (devido AGORA)
Which context/thread should dispatch it?       [Game] AUDIO (guest OSThread 4) via
                                               func_800128C0 -> 129FC -> 21574 -> jalr
Can that context run while MAIN spins in C2AB8? NO — thread AUDIO estacionada em
                                               LightweightSemaphore::waitWithPartialSpinning;
                                               contadores de áudio congelados
```

## Ground truth (como exigido)

```text
run clean                    -> YES (3 runs, binário original, sem instrumentação)
confirm natural freeze       -> YES (syscall d estável >= 8s, black screen pós-intro)
attach ONLY after d stable   -> YES (PTRACE_ATTACH depois do freeze; leitura apenas)
sem debugger/hook/counter    -> YES (nenhum antes do freeze)
mutate guest memory          -> NO
clearer manual               -> NO
force callback               -> NO
code change                  -> NO
```

## Blocker reconfirmed (§3)

Snapshot idêntico nos 3 runs (determinístico):

```text
slot7  0x80128F10  +0x78=0x01026C00  enabled=1  mode=3  substate=2  +0x62=63
table[2]  0x80128B18  word0 = 0x8005C168/0x8005C120/0x8005C1B0 (live pool obj)
                     word1 = 1
object +0x30 = 0x80128B18  (backref exato para table[2])
object +0x34 = 2           (countdown inicial)
object +0x44 = 0x05        (flags 0x1|0x4)
object +0x45 = 0x00/0x02   (phase)
object +0x08 = 0x801301E0  (config SFX)
MAIN host RIP = func_800C2AB8+0x1ba  (busy-wait do dispatcher de kinematics)
```

Mecanismo confirmado: `table[2].word0 != 0` → `func_800C269C(7)==0` → `pending!=0`
→ loop externo de `func_800C2AB8` infinito. **MESMO mecanismo documentado.**

## SCHEDULE ROOT / QUEUE (§4)

```text
SCHEDULE_ROOT:   *(0x800411DC) = 0x8007E9C0  (SoundManager / g_sound_manager)
                 delta-queue listhdr = mgr + 0x14 = 0x8007E9D4
HEAD:            listhdr[8] = 0x8005E928 (run A) / 0x8005E8B8 (run B) / 0x8005EA94 (run C)
TAIL:            node com next==0 (0x8005E8B8 / 0x8005EA40)
ENTRY_COUNT:     2 entries caminhadas (delta-queue viva e não vazia)
INTEGRITY:       OK — prev/next encadeados e terminam em NULL; sem ciclo;
                 prev(head) == listhdr+8 == 0x8007E9DC; head->prev = 0x8007E9DC
                 confirmado em 2 walks completos (runs A/C)
```

Layout do nó (30B): `+0x00 next, +0x04 prev, +0x08 key(delta), +0x0C payload[16]`.

## EVENT ENTRY associada ao objeto de table[2] (§5)

```text
EVENT_ENTRY:     node na HEAD da delta-queue (0x8005E928 / 0x8005EA94)
TARGET_OBJECT:   payload.p4 == table[2].word0 (0x8005C168 / 0x8005C120) — CONFIRMADO
CALLBACK:        mgr+0x08 handler = func_8001ED68 (0x8001ED68) — MATCH
CALLBACK_SYMBOL: func_8001ED68 (evento com opcode que func_8001EE34 despacha;
                 não inferido pelo tipo do objeto — lido do registro real mgr+0x08)
ARG:             payload.p8 = 0x8027AB44 (id/config do evento)
REMAINING_DELTA: 0 (head entry; delta-key @ node+0x08 == 0 → DEVEDO AGORA)
FLAGS/STATE:     objeto vivo; backref +0x30 = &table[2]; countdown +0x34 = 2
```

`FUNC_8001EE34_REGISTERED: YES` (estático PROVEN: 1ED68→1EE34; dinamicamente o
handler mgr+0x08 vale exatamente 0x8001ED68, e há evento enfileirado para ele).

## FUNC_8001EE34 (§6)

```text
FUNC_8001EE34_REGISTERED: YES  (handler registrado mgr+0x08 = 0x8001ED68,
                                que despacha func_8001EE34 para eventos não-0x20)
FUNC_8001EE34_PENDING:     YES  (evento na delta-queue com opcode de func_8001EE34,
                                 p4 == objeto de table[2], delta 0)
DELAY:                     0    (head da delta-queue; devido imediatamente)
```

Lido do callback pointer REAL (`mgr+0x08`), não inferido pelo tipo do objeto.

## EXECUTION CONTEXT (§7)

Inventory de threads host no freeze (run B):

```text
THREAD            STATE             PRIORITY   PC
[Game] MAIN       RUNNING (guest)   -          func_800C2AB8+0x1ba  (busy-wait)
[Game] AUDIO      PARKED host       -          LightweightSemaphore::waitWithPartialSpinning
                                                (stack RA 0xb1b6c8 = semaphore wait do
                                                 runtime; NÃO executa guest code)
[Game] SCHEDULE   PARKED host       -          idem (semaphore wait)
[Game] IDLE / VIMGR / CIC6105 / JOY / MOTOR / TIDY
                  PARKED host       -          espera de semáforo/mensagem
```

```text
EXPECTED_DISPATCH_CONTEXT_CURRENTLY_ABLE_TO_RUN: NO
WHY:  a thread guest AUDIO (OSThread 4) está estacionada em um wait de
      semáforo do scheduler cooperativo do runtime (LightweightSemaphore),
      aguardando ser retomada. Ela não está rodando código guest.
```

## MAIN BUSY-WAIT vs DISPATCHER (§8)

```text
CAN_SCHEDULE_DISPATCH_PROGRESS_WHILE_MAIN_IN_C2AB8: NO
```

Estado real (não apenas estático):

- AUDIO thread: parked em `LightweightSemaphore::waitWithPartialSpinning`
  (stack host 0xb1b6c8 dentro do moodycamel::LightweightSemaphore — o wait do
  scheduler cooperativo). Não roda.
- Contadores de áudio do diag **congelados** no freeze: `rsp_aud=1529`,
  `queue_calls=1528`, `queue_samp=2305888` (estáveis em 4+ relatórios
  consecutivos; antes do freeze avançavam).
- MAIN em `func_800C2AB8` **não cede** (spin puro, sem syscall/osRecvMesg/
  osSendMesg/yield) → o runtime cooperativo nunca retoma AUDIO → a síntese de
  áudio (`func_800128C0 → func_800129FC → func_80021574 → jalr mgr->handler =
  func_8001ED68`) não avança → `func_8001EE34 → func_800200B8 → func_800205D0`
  nunca executa → `table[2].word0` nunca é zerado.

Isso corresponde ao contrato do audit estático Gemini (dispatch na thread de
áudio inibido pelo spin síncrono de MAIN), agora com prova dinâmica de thread
estacionada + contadores congelados.

## TIME SOURCE (§9)

```text
CURRENT_TIME/TICK:   driver audio +0x1C = 1155520 / 1154608 / 1154960 (amostras)
                     (0x8006A158; ALSynthDriver clock em samples)
                     driver+0x20 = 1155520 / 1154600 / 1154968
EVENT_DUE_TIME/DELTA: head entry key = 0  (devido agora)
mgr+0x4C next_delta: 13003 / 2997 / 2997 (último retorno do func_8001ED68;
                     evento de cabeça recém-inserido com delta 0)
EVENT_CURRENTLY_DUE: YES  (delta-key 0 na entrada de cabeça que referencia o
                           objeto de table[2])
```

A entrada está **overdue/due** mas não executa → evidência importante de que o
dispatch (thread AUDIO) está impossibilitado de rodar.

## OBJECT ORIGIN (§10) — UNKNOWN

Sem metadata/caller identity suficiente no snapshot: `config=0x801301E0`
(SFX config, compatível com item/Jiggy conforme audit Gemini) e countdown=2
(NÃO prova origem isoladamente). `OBJECT_ORIGIN: UNKNOWN`.

## DISCRIMINADOR OPCIONAL (§11)

Não necessário: o snapshot distinguir a causa sem instrumentação.

## CLASSIFICAÇÃO (§12)

```text
PRIMARY_CLASS: A — callback (func_8001EE34) agendado E due (delta 0 na head),
                   mas o contexto de dispatch (thread [Game] AUDIO) não pode
                   rodar enquanto MAIN gira em func_800C2AB8.

Rejeitados:
  B — não: a entrada está DUE (delta 0), não "not yet due"
  C — não: o evento/callback ESTÁ registrado (mgr+0x08 = 0x8001ED68) e enfileirado
  D — não: binding correto (handler == func_8001ED68)
  E — não: a entrada ainda está na fila (não removida prematuramente)
  F — não: o callback nunca rodou (AUDIO parked; contadores congelados)
  G — não: existe contrato de progressão assíncrona (dispatch de áudio)
```

## Correlação com arbitragem / audits anteriores

- Confirma dinamicamente a reconstrução Gemini do driver
  (`func_8001EB10 → func_800128C0 → func_800129FC → func_80021574 →
  func_8001ED68 → func_8001EE34 → func_800200B8 → func_800205D0`).
- Confirma `CLEARER_BEFORE_C2AB8: NO` no caminho de transição (evento ainda
  agendado quando C2AB8 entra; não pré-limpo).
- Class A era uma das duas opções abertas na arbitragem
  (`C2AB8_EXPECTED_PROGRESS_SOURCE: A and/or C — NOT B`); este snapshot
  **prova o ramo A** para este run (evento due + dispatch impossibilitado),
  e o mecanismo assíncrono C (dispatch de áudio) é o que deveria ter limpo.
- `func_80026500` como PC do hang: permanece REFUTADO (não é o PC de hang).

## Tabela de prova

| item | status |
| --- | --- |
| objeto de table[2] agendado na delta-queue (head, delta 0) | PROVEN (dinâmico, 3 runs) |
| handler mgr+0x08 = func_8001ED68 (MATCH) | PROVEN (dinâmico) |
| func_8001EE34 pending (evento na fila com opcode válido) | PROVEN (dinâmico + estático) |
| contexto de dispatch = thread [Game] AUDIO | PROVEN (estático + nome de thread host) |
| AUDIO thread parkada em semaphore wait (não roda) | PROVEN (dinâmico, stack host) |
| contadores de áudio congelados no freeze | PROVEN (dinâmico, diag) |
| dispatch não pode avançar enquanto MAIN gira em C2AB8 | PROVEN (dinâmico) |
| entrada due mas não executada (overdue) | PROVEN (dinâmico: delta 0 + congelada) |
| ROOT_MECHANISM (C2AB8 busy-wait / slot7 pending) | PROVEN (já documentado) |
| ROOT_CAUSE (dispatch de áudio inibido) | PROVEN (mecanismo; ran A confirmado) |
| object origin (intro/attract/handoff) | UNKNOWN |

```text
CODE_CHANGED            = NO
RUNTIME_CODE_CHANGED    = NO
GENERATOR_CODE_CHANGED  = NO
PATCH_CODE_CHANGED      = NO
CORE_INCLUDED           = NO
BUILD_ARTIFACT_INCLUDED = NO
```

Artefatos (não versionados): `/tmp/opencode/p08/sup4_*`, `sup5_*`, `sup6_*`
(supervisores + logs + diag de 3 clean runs).

=== END EVIDENCE ===
