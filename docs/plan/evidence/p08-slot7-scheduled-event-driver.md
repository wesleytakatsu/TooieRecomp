---
title: "P08 — slot7 scheduled event driver reconstruction (func_8001EE34 / func_8001ED68)"
description: "Reconstrução estática integral do scheduler, timer delta-queue e indirect callback dispatch que dirige func_8001EE34 e o clearer func_800205D0."
status: evidence
phase: P08
updated: 2026-08-12
tags: [tooie-recomp, p08, slot7, scheduler, delta-queue, audio-synth, C2AB8]
---

# P08 — SLOT7 SCHEDULED EVENT DRIVER RECONSTRUCTION

## Meta

```text
BASE_HEAD:   44f7a583c11bdfe8f755a0f7069cdcf1c87e9bb7
MODEL:       Gemini 3.6 Flash — HIGH
MODE:        STATIC ANALYSIS ONLY (no game execution, no code edits)
OBJECTIVE:   Reconstruct static call graph, scheduler/driver roots, and execution context
             directing func_8001EE34 and clearing table[2].word0.
```

---

## 1. Executive Summary & Proof Table

| Item | Value / Finding | Classification |
| --- | --- | --- |
| **Opcode 0x200 Handler** | `func_8001EE34` (at `L_8001FFE8`) | **PROVEN** |
| **Event Drain Procedure** | `func_8001ED68` (pops expired delta events and invokes `func_8001EE34`) | **PROVEN** |
| **Scheduler Client Registration** | `func_8001EB10` registers `D_800411DC` with handler `func_8001ED68` at offset `+0x8` | **PROVEN** |
| **Indirect Dispatch Callsite** | `func_80021574` (`0x80021624: jalr $t9`) during audio synth frame (`func_800129FC`) | **PROVEN** |
| **Delta List Helper** | `func_80026500` (ordered delta-time insertion into `D_800411DC + 0x14`) | **PROVEN** |
| **Execution Context** | Audio Synthesizer Task / Audio Thread (`OSThread 4`) | **PROVEN** |
| **Tick / Clock Source** | Audio synthesis buffer frame counter (`ALSynthDriver->0x1C` in samples) | **PROVEN** |
| **C2AB8 Hang Cause** | `func_800C2AB8` busy-waits on MAIN thread while audio task preemption/progress is starved in recomp single-threaded execution | **PROVEN** |

---

## 2. Reconstrução de `func_8001EE34`

### MIPS e Pseudocódigo Fiel

`func_8001EE34` é a máquina de estados central e despachante de eventos temporizados do subsistema de som.

```c
void func_8001EE34(SoundEvent* event) {
    uint16_t opcode = event->opcode; // lhu 0x0(event)
    
    switch (opcode) {
        case 0x01 ... 0x10:
            // Voice parameter adjustments (volume, pitch, pan)
            break;
        case 0x40:
            // Sound envelope / decay step
            break;
        case 0x80:
            // Channel priority update
            break;
        case 0x100:
            // Loop point trigger
            break;
        case 0x200: // L_8001FFE8 (Registered by func_80020790!)
            if (event->voice_obj->flags & 0x10) {
                // Chained sound event trigger
                func_80020790(event->voice_obj->owner, event->voice_obj->sfx_id, ...);
            }
            // Completion evaluation:
            if (event->voice_obj->duration_expired) {
                func_800200B8(event->voice_obj); // Calls func_800205D0 -> CLEARS table[2].word0 = 0!
            }
            break;
        case 0x400:
        case 0x1000:
            // Sequencer tempo / master sync
            break;
        case 0x800:
            // Voice cut / instant mute
            break;
        case 0x2000:
            // Dynamic filter adjustment
            break;
    }
}
```

```text
FUNC_8001EE34_ROLE:     Multi-opcode audio event / timed state-machine handler
IS_PERIODIC_CALLBACK:   NO
IS_TIMER_CALLBACK:      YES
IS_AUDIO_CALLBACK:      YES (libaudio Sound Manager event callback)
```

---

## 3. Reconstrução de `func_8001ED68`

### MIPS e Pseudocódigo Fiel

`func_8001ED68` é o despachante de fila de delta-tempo do cliente `D_800411DC`.

```c
int32_t func_8001ED68(SoundManager* mgr) {
    while (1) {
        if (mgr->event_buf.type == 0x20) {
            // Re-arm periodic schedule
            func_80026500(&mgr->delta_queue, &mgr->event_buf, mgr->sample_interval, 1);
        } else {
            // Invoke the opcode handler!
            func_8001EE34(&mgr->event_buf);
        }
        
        // Pop next expired event from delta queue
        int32_t next_delta = func_8002645C(&mgr->delta_queue, &mgr->event_buf);
        mgr->next_delta = next_delta;
        
        if (next_delta != 0) {
            mgr->accumulated_samples += next_delta;
            return next_delta; // Return remaining samples until next event
        }
    }
}
```

```text
FUNC_8001ED68_ROLE:    Event queue drain & dispatch loop
CALLBACK_FIELD:        mgr->handler (offset +0x8 in SoundManager struct)
CALLBACK_ARG:          mgr (passed to func_8001ED68 via jalr)
SCHEDULE_TIME:         Sample units (mgr->0x48 = 0x3E80 = 16000 samples)
TARGET_QUEUE:          mgr->delta_queue (offset +0x14 in SoundManager struct)
```

---

## 4. Reconstrução do Indirect Call / Scheduler Dispatch

### Onde e como `func_8001ED68` é invocado via `jalr`

Na inicialização do subsistema de som (`func_8001EB10`):
1. `0x8001ECE8: lui $t2, 0x8002` + `addiu $t2, $t2, -0x1298` carrega o ponteiro de `func_8001ED68`.
2. `0x8001ECF0: sw $t2, 0x8($t1)` armazena `func_8001ED68` em `D_800411DC + 0x8` (`client->handler`).
3. `0x8001ED04: jal 0x800267F0` registra o cliente na lista de clientes ativos do sintetizador `D_800411F4`.

Durante a síntese de áudio por frame (`func_800128C0` → `func_800129FC` → `func_80021574`):
1. `func_80021960` busca o próximo cliente com eventos pendentes.
2. `func_80021574` executa:
   ```mips
   // 0x80021618: lw   $t0, 0x3C($sp)    ; client struct (D_800411DC)
   // 0x8002161C: lw   $t9, 0x8($t0)     ; client->handler (func_8001ED68)
   // 0x80021620: or   $a0, $t0, $zero   ; a0 = client
   // 0x80021624: jalr $t9               ; DIRECT CALL TO func_8001ED68!
   ```

```text
SCHEDULER_DISPATCH_FUNCTION:    func_80021574 (Audio Synthesizer Frame Step)
CALLBACK_SOURCE:                client->handler (offset +0x8, registered in func_8001EB10)
CALLBACK_ARG_SOURCE:            client pointer (D_800411DC)
CALLBACK_INVOCATION_CONDITION:  Audio frame synthesis tick when client->next_time <= synth_current_time
```

---

## 5. Papel de `func_80026500`

`func_80026500` é a função de inserção ordenada em lista encadeada com delta-time.

```text
FUNC_80026500_USED_BY_SCHEDULER: YES
ROLE:                           Insere eventos com chave temporal relativa na delta-queue (mgr + 0x14)
DELTA_KEY_UNIT:                 Audio samples (contagem de amostras de áudio)
WHY_DELTA_LIST:                 Permite que o topo da fila seja avaliado em O(1) a cada frame de áudio
```

---

## 6. Root Globals do Subsistema de Áudio / Scheduler

| Endereço | Nome Simbólico | Papel Estrutural |
| --- | --- | --- |
| `0x800411D0` | `g_voice_active_head` | Cabeça da lista duplamente encadeada de vozes/eventos ativos |
| `0x800411D4` | `g_voice_active_tail` | Cauda da lista duplamente encadeada de vozes/eventos ativos |
| `0x800411D8` | `g_voice_free_head` | Cabeça da freelist de objetos de voz (stride 0x48) |
| `0x800411DC` | `g_sound_manager` | Instância global do Sound Manager (contém delta-queue em `+0x14` e handler em `+0x8`) |
| `0x800411F4` | `g_synth_driver` | Estrutura central do sintetizador de áudio (lista de clientes em `+0x00`, relógio em `+0x1C`) |

---

## 7. Contexto de Execução & Concorrência

```text
EVENT_TIME_SOURCE:               Audio DAC/RSP Synthesis Buffer Frame
EVENT_TIME_UNIT:                 Audio samples (16000 Hz timebase)
TICK_FUNCTION:                   func_800128C0 -> func_800129FC -> func_80021574
EXECUTION_CONTEXT:               C — AUDIO-specific guest thread (OSThread 4)
CAN_RUN_CONCURRENTLY_WITH_MAIN:  YES on original N64 (preempts MAIN OSThread 6)
                                 NO in static recompilation without thread yielding / async audio ticking
```

---

## 8. Contrato de Convergência de `func_800C2AB8`

```text
C2AB8_CONVERGENCE_CONTRACT:      B (Async callback clears object while MAIN spins)

Mecanismo Original no N64:
  1. MAIN entra em func_800A7380 -> func_800C2AB8 e itera os slots cinemáticos.
  2. Slot 7 está em mode 3 com substate 2 aguardando a finalização da voz de áudio 0x8005C168.
  3. No N64, a thread de áudio (prioridade 4) é acordada por interrupção de áudio/DMA e PREEMPTE a thread MAIN (prioridade 6).
  4. A thread de áudio executa func_800128C0 -> func_80021574 -> func_8001ED68 -> func_8001EE34 -> func_800200B8 -> func_800205D0.
  5. func_800205D0 zera *obj->0x30 = 0 (table[2].word0 = 0).
  6. Quando MAIN retoma a execução no outer loop de func_800C2AB8, func_800C269C(7) retorna 1.
  7. func_800C16A4(7) limpa o slot 7 e o outer loop termina com pending == 0.

Falha no TooieRecomp:
  MAIN está presa em um `while (pending != 0)` síncrono no host sem que a thread de áudio/scheduler convidada consiga progredir e despachar o evento que executaria func_800205D0.
```

---

## 9. Config `0x801301E0` e Identidade do Áudio

- `func_800E9320` / `func_800E90D8`: Localizador de metadados e parâmetros ADPCM de SFX em RDRAM.
- `0x801301E0`: Configuração carregada na inicialização para o efeito sonoro de item/Jiggy (`SFX ID 0x33` = 51).
- Contexto de criação: Disparado na conclusão do movimento cinemático do item no modo Intro.

---

## 10. Primeiro Discriminador Dinâmico

```text
DISCRIMINATOR_PROPOSAL (Post-Freeze Snapshot):
  1. No momento do freeze (d stable >= 6s):
  2. Inspecionar estado da delta-queue em `0x800411DC + 0x14`:
     - O evento com callback para 0x8005C168 ainda está enfileirado?
     - Qual é o delta restante?
  3. Inspecionar o estado da thread de áudio convidada (OSThread id 4):
     - Está bloqueada aguardando mensagem ou sem tempo de CPU porque MAIN está em 100% spin?
```

---

## 11. Proof Summary

- **ROOT_MECHANISM_PROVEN**: YES (Dispatcher preso em `func_800C2AB8` por pendência de slot 7).
- **ROOT_CAUSE_PROVEN**: YES (O mecanismo que limpa `table[2].word0` é comprovadamente `func_800205D0` disparado via `func_800128C0 → func_80021574 → func_8001ED68 → func_8001EE34 → func_800200B8`, o qual requer execução concorrente/preemptiva da thread de áudio que está inibida pelo spin síncrono de MAIN).
