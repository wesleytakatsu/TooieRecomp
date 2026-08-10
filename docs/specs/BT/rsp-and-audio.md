---
title: "BT — RSP e áudio"
status: current
area: "specs/BT"
updated: 2026-08-09
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

## Validação

- Smoke técnico: milhares de `M_AUDTASK` sem tipo desconhecido.
- **Ouvido (2026-08-09):** música inicial correta no device default.

## Referências

- Plano: [[../../plans/BT/07-rsp-and-audio]]
- Inventário: `TooieRecomp/metadata/rsp_tasks.inventory.md`
- Evidência hang/CIC: `docs/plan/evidence/p07-boot-hang-audio-diag.md`
