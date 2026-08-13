---
title: "BT — RSP e áudio"
status: current
area: "specs/BT"
updated: 2026-08-13
tags:
  - banjo-tooie
  - rsp
  - audio
  - n_aspMain
  - cic6105
---

# BT — RSP e áudio

## Microcódigos

| blob | uso | host |
| --- | --- | --- |
| `n_aspMain` | `M_AUDTASK` (mixer n_audio) | `RSPRecomp` → `rsp/n_aspMain.cpp` |
| `rspboot` | boot de OSTask | libultra/recomp |
| `gspF3DEX2.NoN.fifo` | `M_GFXTASK` default | RT64 HLE |
| `gspL3DEX2.fifo` | `M_GFXTASK` line | RT64 HLE |
| `gspF3DEX2.fifo` | no ROM; scheduler BT não usa | — |

## RSPRecomp (Tooie)

Config: `TooieRecomp/n_aspMain.us.rev0.toml`

```toml
text_offset = 0x1E4F3B0   # decompressed.us.z64 (não baserom comprimido)
text_size   = 0x17D0
text_address = 0x04001080
```

- Jump table: 17 halfwords em data +`0x0E` (ver `tools/compare_asp_microcodes.py`).
- **Não reutilizar** `n_aspMain.cpp` de Banjo-Kazooie (tamanho e dispatch diferentes).
- Regenerar: `./tools/run_rsprecomp.sh` (RSPRecomp + fix de labels `0x1FFF`).

## Dispatch

`get_rsp_microcode` (`src/main/main.cpp`):

- `M_AUDTASK` → `n_aspMain`
- `M_GFXTASK` → não deve chegar (RT64); log + `nullptr` se chegar
- outro → log + `nullptr`

`TOOIE_LOG_RSP=1` inventaria o path SP.

## Áudio host

- `SDL_OpenAudioDevice(nullptr, …)` → **default do SO** (Pulse/PipeWire no Linux).
- `queue_samples` / `get_frames_remaining` / `set_frequency`.
- Conversão int16 → float, swap L/R, anti-latency skip.
- Sem device: message box + exit.

Verificação de roteamento: stream PipeWire `TooieRecompiled` no sink default
(não confundir silêncio de boot com device errado).

## CIC 6105 e SI (pré-requisito de assets / música)

`gldbDll` descriptografa assets com challenge/response CIC-NUS-6105 via SI:

1. `__osSiRawStartDma` deve completar SI (`send_si_message`) — no-op = hang eterno.
2. Espelho PIF 64 B + algoritmo X-Scale/mupen em write com challenge em `0x30`.
3. LUT errada ⇒ stream `gsmaker` não termina ⇒ tela parada e PCM zero.

Implementação: `src/game/boot_hw_stubs.cpp` (`tooie___osSiRawStartDma`,
`n64_cic_nus_6105`).

## Diagnóstico

```sh
TOOIE_AUTO_START=1 TOOIE_DIAG=1 ./TooieRecompiled
# peak/nonzero > 0 e stack=(idle) após boot saudável
```

## Sound Manager e Client Event Scheduler

O subsistema de áudio de Banjo-Tooie implementa um scheduler de eventos temporizados por delta-queue associado ao driver sintetizador (`D_800411F4`):

1. **Inicialização (`func_8001EB10`):** Aloca o pool de instâncias de voz (stride `0x48`, freelist `D_800411D8`) e inicializa o cliente global `SoundManager` (`D_800411DC`). Registra o handler de eventos `func_8001ED68` em `client->0x8`.
2. **Síntese por Frame (`func_800128C0 → func_800129FC → func_80021574`):** O driver avança o relógio de amostras (`driver->0x1C`) e despacha via chamada indireta (`0x80021624: jalr $t9`) o handler do cliente registrado (`func_8001ED68`).
3. **Despacho de Eventos (`func_8001ED68`):** Desempilha eventos expirados da fila de delta-tempo (`D_800411DC + 0x14`, inseridos via `func_80026500`) e invoca a máquina de estados `func_8001EE34(event)`.
4. **Finalização de Vozes e Clearer (`func_8001EE34 → func_800200B8 → func_800205D0`):** Para eventos de opcode `0x200` expirados, `func_800205D0` zera o ponteiro de retro-referência (`*obj->0x30 = 0`), liberando eventuais dependências de tabelas externas (como `table[substate].word0` na cinemática).

## Validação

- Smoke técnico: milhares de `M_AUDTASK` sem tipo desconhecido.
- **Ouvido (2026-08-09):** música inicial correta no device default.
- **Clean freeze dinâmico (2026-08-12):** o contrato do scheduler (delta-queue
  `D_800411DC + 0x14`, handler `func_8001ED68`, dispatch `func_8001EE34`) foi
  validado em 3 freezes determinísticos: o objeto pendente de `table[2]` está
  agendado na delta-queue (head, delta 0), e o contexto de dispatch (thread
  `[Game] AUDIO`) fica estacionado enquanto MAIN gira em `func_800C2AB8`
  (class A). Ver `docs/plan/evidence/p08-slot7-scheduled-event-validation.md`.
- **Causal (2026-08-12):** experimento limpo reversível prova o gap de
  preempção: `yield_self_1ms` em `func_800C2AB8` deixa AUDIO rodar e
  `func_800205D0` limpar `table[2].word0`. Ver
  `docs/plan/evidence/p08-guest-preemption-causal-validation.md`.
- **Design (2026-08-13):** Hybrid F' + spec de implementação em
  `docs/plan/evidence/p08-preemption-implementation-plan.md`.
  PATCH 3.5 landed cause-aware hold-until-eligible. Current RCP/MI mask
  is per native guest OSThread (TLS). **PATCH 4 behavior (2026-08-13):**
  Tooie regenerates 4,298 safepoints and starts Hybrid F' ON. Same-binary
  ON/OFF independently proves CLASS A; AUDIO advances through the former
  freeze window. The exact PATCH 4 `table[2].word0 == 0` value is inferred,
  while resolution of C2AB8's pending condition is proven. PATCH 4 delivery
  is not accepted until ignored `RecompiledPatches` build inputs are
  reproducible from tracked state. Ver
  `docs/plan/evidence/p08-preemption-patch4-independent-review.md`.
  Downstream `func_800BCF28` remains the next product divergence and was not
  diagnosed in this review.

## Referências

- Plano: [[../../plans/BT/07-rsp-and-audio]]
- Inventário: `TooieRecomp/metadata/rsp_tasks.inventory.md`
- Evidência hang/CIC: `docs/plan/evidence/p07-boot-hang-audio-diag.md`
- Evidência scheduler/slot7: `docs/plan/evidence/p08-slot7-scheduled-event-driver.md`
