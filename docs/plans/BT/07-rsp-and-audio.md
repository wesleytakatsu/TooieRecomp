---
title: "BT P07 — RSP e áudio"
description: "Inventaria e integra os microcódigos RSP e o pipeline de áudio necessários por Banjo-Tooie."
status: active
type: implementation-stage
project: TooieRecomp
phase: P07
order: 7
depends_on: P06
updated: 2026-08-09
tags: [banjo-tooie-Recomp, rsp, audio, microcode, cic6105]
---

# BT P07 — RSP e áudio

## Resultado esperado

Toda tarefa RSP observada tem implementação conhecida; áudio e música são
produzidos sem underrun, corrupção ou perda de sincronização.

## Tarefas

- [x] `P07-T01` Instrumentar e inventariar tipos, endereços e variantes de `OSTask` usados por Tooie.
- [x] `P07-T02` Comparar microcódigos Tooie com `n_aspMain` usado pelo BanjoRecomp.
- [x] `P07-T03` Criar configurações RSPRecomp para cada variante necessária.
- [x] `P07-T04` Implementar dispatch estrito que rejeite tarefa desconhecida com diagnóstico útil.
- [x] `P07-T05` Integrar fila, frequência, conversão e latência de áudio do runtime.
- [x] `P07-T06` Validar música inicial (parcial: intro confirmada pelo usuário; SFX/fala/pause/hotplug ainda abertos).
- [ ] `P07-T07` Executar soak test para detectar underrun, drift e vazamento de buffer.
- [x] `P07-T08` Documentar patches de tooling / anti-piracy host (CIC/SI, labels RSP).

## O que foi implementado (2026-08-09)

### Inventário estático

Arquivo: `TooieRecomp/metadata/rsp_tasks.inventory.md`

| type | uso | host |
| ---: | --- | --- |
| 1 `M_GFXTASK` | F3DEX2 NoN / L3DEX2 | RT64 HLE |
| 2 `M_AUDTASK` | `n_aspMain` | recompilado `rsp/n_aspMain.cpp` |

Scheduler: `banjo-tooie-Recomp/src/core1/1E2B840.c`.

### Microcódigo Tooie ≠ BK

| | Banjo-Kazooie | Banjo-Tooie |
| --- | ---: | ---: |
| text size | `0xC60` | **`0x17D0`** |
| jump table | 14 alvos | **17** alvos |
| alvos compartilhados | — | **0** |

- Config: `TooieRecomp/n_aspMain.us.rev0.toml` (ROM **decompressed** `0x1E4F3B0`).
- Regenerar: `./tools/run_rsprecomp.sh` (= RSPRecomp + `fix_rsprecomp_labels.py`).
- Labels: RSPRecomp mascara ramos com `0x1FFF`; text Tooie ultrapassa IMEM →
  pós-processo corrige `L_005C` → `L_205C` etc. (tooling, T08).

### Dispatch + pipeline SDL

- `get_rsp_microcode`: `M_AUDTASK` → `n_aspMain`; desconhecidos com log rico.
- `TOOIE_LOG_RSP=1` inventaria tasks no path SP.
- Áudio: `SDL_OpenAudioDevice(nullptr, …)` = **dispositivo default do SO**
  (Pulse/PipeWire). Sem device → message box e falha.

### Diagnóstico e destravamento de boot (bloqueadores de áudio)

Problema observado: tela parada + silêncio no headset default.

| Achado | Detalhe |
| --- | --- |
| Device | Stream `TooieRecompiled` → sink FELITRON (default). **Não** era device errado. |
| PCM | `TOOIE_DIAG`: `peak=0` no hang; depois do fix `peak≈7k–9k`, `nonzero` alto. |
| Hang #1 | `gldbDll_entrypoint_0` em `osRecvMesg(BLOCK)`: `__osSiRawStartDma` era **no-op**. |
| Fix #1 | SI DMA: espelho PIF 64 B + `ultramodern::send_si_message()`. |
| Hang #2 | Stream `gsmaker` infinito: CIC 6105 com LUT errada → asset lixo. |
| Fix #2 | Algoritmo **X-Scale/mupen** (`n64_cic_nus_6105`) no write SI; zera `0x2e/0x2f`. |

Stack de hang documentado: `gsworldDll:2 → gsmaker:2 → gldbDll:0` (CIC/SI).
Após fix: `leave ovl=726`, stack `(idle)`, boot avança até transição.

Ferramentas:

- `TOOIE_DIAG=1` — contadores syscall / rsp_aud / queue / peak / hang stack
  (`include/tooie_diag.h`, `src/game/tooie_diag.cpp`).
- Evidência monorepo: `docs/plan/evidence/p07-boot-hang-audio-diag.md`.

### Validação auditiva (usuário)

- **2026-08-09:** música **inicial** toca corretamente no default do sistema
  após restart com `TOOIE_AUTO_START=1`.
- Ainda não coberto de forma sistemática: SFX, fala, pause, cutscenes,
  hotplug de áudio, soak longo (T07).

## Residual

- **P07-T06 (resto):** SFX, fala, pause, cutscenes, troca de dispositivo.
- **P07-T07:** soak underrun/drift.
- Visual title/menu completo e input → **P08**.

## Gate de saída

- [x] Nenhuma tarefa RSP desconhecida no boot coberto (`M_AUDTASK` no SP path).
- [x] Música inicial validada (ouvido) no device default.
- [ ] Sessão longa + transições sem underrun/drift documentados.
- [x] Sem dispositivo de áudio: falha controlada documentada.

## Arquivos principais (repo TooieRecomp)

| path | papel |
| --- | --- |
| `n_aspMain.us.rev0.toml` | config RSPRecomp |
| `rsp/n_aspMain.cpp` | microcódigo recompilado |
| `tools/run_rsprecomp.sh` | regenerar n_aspMain |
| `tools/fix_rsprecomp_labels.py` | labels >0x1FFF |
| `tools/compare_asp_microcodes.py` | Tooie vs BK |
| `src/main/main.cpp` | dispatch RSP + SDL audio |
| `src/game/boot_hw_stubs.cpp` | SI + CIC 6105 + scheduler hooks |
| `src/game/tooie_diag.cpp` | `TOOIE_DIAG` |
| `metadata/rsp_tasks.inventory.md` | inventário OSTask |
| `plan/evidence/p07-boot-hang-audio-diag.md` | evidência boot/áudio |

## Próxima etapa

[[08-rt64-input-and-frontend]]

## Investigação e Fix do tic tic — 2026-10-19

- **Fase**: Investigação de Áudio P07 (P07-T06 parcial, P07-T07 aberto) — ref: [[plan/evidence/p07-audio-tic-tic-investigation-20261019]].
- **Status Geral**: Fix de produção aplicado e `USER_CONFIRMED_FOR_TESTED_INTRO_RECORDING` (sem claim global de campanha).
- **Causa Raiz & Correção**: Cálculo de fila em `main.cpp` usava parâmetros guest; corrigido para taxa e canais host.
- **Evidência Runtime**: Diagnóstico capturou 47 skips espúrios antes do fix; pós-fix registrou 0/579 skips (SHA pré/pós idêntico).
- **Validação Auditiva**: Usuário ouviu clipe corrigido gravado via Pulse (`intro-queue-time-fix.wav`) e atestou sumiço do tic na intro.
- **Diagnóstico Integrado**: Infraestrutura bounded default-off validada via unit tests (`tooie_audio_capture_test` exit 0).
- **Rastreabilidade**: REQ-AUD-05 parcialmente validado para a intro; P07-T07 PENDING/UNTESTED (soak de longo prazo pendente).
- **Próximo Passo**: Testes focados em gameplay ativo (SFX, diálogos, pausa) com binário limpo sem diagnósticos.

## Integração do Controle de Volume BGM (Delimitado) — 2026-10-19

- **Fase**: P07 / P08 Funcionalidade de Áudio Delimitada — ref: [[../../plan/evidence/p08-settings-functionality-20261019]], [[../../reverse-engineering/systems/audio-sequence-volume]].
- **Status da Funcionalidade**: `IMPLEMENTED` e `VALIDATED` para escalonamento de volume de música (BGM).
- **Portão de Fase P07**: **NÃO ENCERRADO GLOBALMENTE**. A tarefa de soak test `P07-T07` permanece `PENDING / UNTESTED`. O roteamento de clipes de voz (se usam sequence player ou SoundPlayer) permanece `UNKNOWN`. O caminho de efeitos sonoros (`func_80020BE0`, SoundPlayer, evento 3) permanece intocado. Nenhuma alegação de paridade global de áudio de campanha é feita.
- **Arquitetura Integrada (PROVEN)**:
  - 6 players de sequência em `0x80079FA0` (stride `0x274`, `seqp+0x1E0`); volume autorado s16 em `player+0`; 6 tracks em `0x801357D0` (stride `0x50`, track id s16 em `+0x28`).
  - Despacho CSP em `func_80025ED0` (`AL_SEQP_VOL_EVT`, tipo 10 para música). Proposta anterior de disparo duplo em `17404` REFUTADA.
  - Overrides em `src/game/boot_hw_stubs.cpp`:
    1. `tooie_bgm_on_csp_set_vol` (`80025ED0`): wrapper que intercepta chamadas CSP e escala `r5` uma única vez para os 6 seqps reconhecidos via `apply_unscaled(authored, percent)`. Ponteiros desconhecidos passam sem modificação.
    2. `tooie_bgm_on_update` (`800FB968`): executado após o update guest original; lê `player+0` dos slots ativos (`track+0x28 >= 0`) e reposta via `func_80025ED0` usando cópia local de `recomp_context` estritamente quando o ganho live sofre alteração. Em 100% contínuo, zero eventos adicionais são gerados. Fades autorados nunca são mutados em RDRAM.
- **IPC do Launcher & Validação em Runtime**:
  - Processo filho do preview de música no launcher Linux consome volume via datagram `socketpair` (`CLOEXEC`, stdin dup 0, monitor pai de 5ms com envio de 1 byte via `MSG_NOSIGNAL`).
  - Validação acústica via null sink dedicado `tooie_bgm_menu_qa`:
    - Preview de launcher (mesmo processo ativo mantido): `preview-100.wav` (RMS .0522), `preview-0-real.wav` (RMS .0000017, peak 1 LSB), `preview-50-real.wav` (RMS .0167), `preview-100-restored.wav` (RMS .0373).
    - Restauração ao vivo em gameplay: `game-0.wav` (RMS 0, peak 0) restaurado ao vivo para `game-100-restored.wav` (RMS .0528, peak .3998) sem reiniciar o executável.
- **Confirmação auditiva (2026-10-20)**: em `final-game-0.wav` com BGM 0%, o usuário confirmou "Apenas efeitos/vozes" (`USER_OBSERVED / USER_CONFIRMED_FOR_TESTED_DEMO_CLIP`); conclusão restrita ao trecho testado.
- **Próxima Ação Única da tarefa delimitada**: concluir revisão e publicar o checkpoint autorizado; P07-T07 permanece pendente.
