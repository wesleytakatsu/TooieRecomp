---
title: "Etapas de implementação — TooieRecomp"
description: "Índice ordenado das etapas necessárias para entregar Banjo-Tooie nativo no Linux."
status: active
type: implementation-index
project: TooieRecomp
updated: 2026-08-11
source_path: banjo-tooie-Recomp
source_repo: wesleytakatsu/banjo-tooie-Recomp
upstream_repo: Mr-Wiseguy/banjo-tooie
tags:
  - banjo-tooie-Recomp
  - implementation
  - linux
  - index
---

# Etapas de implementação — TooieRecomp

Execute as notas na ordem. Cada uma define entradas, tarefas, artefatos,
verificação e gate de saída; checkboxes são deliberadamente usados para que
`markdown_vault_extract_tasks` produza a fila de trabalho.

A fonte BT ativa é o fork em `banjo-tooie-Recomp/`. O repositório original
`Mr-Wiseguy/banjo-tooie` é o remote `upstream`, não um segundo checkout local.

| Ordem | Etapa | Resultado | Status |
| ---: | --- | --- | --- |
| 00 | [[00-baseline-and-decisions]] | baseline reproduzível | done |
| 01 | [[01-linux-toolchain]] | toolchain validada | done |
| 02 | [[02-rom-decompression-and-elf]] | ROM decomposta e ELF matching | done |
| 03 | [[03-recompiler-metadata]] | metadados consumíveis pelo N64Recomp | done |
| 04 | [[04-static-recompilation]] | código recompilado compila | done |
| 05 | [[05-native-runtime-and-boot]] | executável chega ao primeiro frame | done |
| 06 | [[06-overlays-and-syscalls]] | código dinâmico funciona | in_progress |
| 07 | [[07-rsp-and-audio]] | tarefas RSP e áudio corretos | active (música inicial OK) |
| 08 | [[08-rt64-input-and-frontend]] | renderização, entrada e UI integradas | active |
| 09 | [[09-gameplay-parity-and-enhancements]] | campanha e timing aprovados | pending |
| 10 | [[10-linux-packaging-and-release]] | release Linux reproduzível | pending |

## Convenções

- `Pxx-Tyy` é um identificador de tarefa estável.
- `status` aceita `pending`, `active`, `blocked` ou `done`.
- “Gate de saída” é obrigatório; sem ele, a próxima etapa não começa.
- Evidências devem conter comando, resultado, commit e hash quando aplicável.
- Decisões arquiteturais atualizam também [[../../plan/index|o plano mestre]].

## Contexto

- Plano mestre: [[../../plan/index]]
- Critérios finais: [[../../plan/definition-of-done]]
- Referência BK: [[../../specs/BK/index]]
- Insumos BT: [[../../specs/BT/index]]

### Nota P06 (2026-08-09)

Dispatch de overlay syscalls implementado e validado em runtime (decode ROM +
tabela gerada + `load_overlays` + call/return). Boot host carrega core1/core2
sem inflate na imagem descomprimida. `ovl_load` no heap, scheduler/ACK e restore
aninhado estão implementados; lifecycle e relocations receberam correções
adicionais durante P08. Detalhe: [[06-overlays-and-syscalls]].

### Nota P07 (2026-08-09)

`n_aspMain` Tooie (RSPRecomp `0x17D0`), dispatch `M_AUDTASK`, SI+CIC-6105
(host) para destravar `gldb`/`gsmaker`/`gsworld`. Música **inicial validada
por ouvido** no sink default. Residual: SFX/pause/hotplug/soak (T06 resto +
T07). Detalhe: [[07-rsp-and-audio]].

### Nota P08 (2026-08-12)

RT64/Vulkan: intro/Jiggy OK; type-4 completa e warp consumido no clean freeze;
mode-5 alcançado. Freeze: MAIN em `func_800C2AB8` (slot 7 pending,
`table[2].word0` = pool object vivo). **Validação dinâmica do scheduler
(3 clean freezes):** o objeto de `table[2]` ESTÁ agendado na delta-queue do
SoundManager (head, delta 0), handler `func_8001ED68` registrado, `func_8001EE34`
pending; thread `[Game] AUDIO` estacionada em `LightweightSemaphore` wait e
contadores de áudio congelados → **CLASS A** (callback agendado e devido, mas
contexto de dispatch não pode rodar). Criador `func_80020790`, clearer
`func_800205D0`; origem do objeto (intro/attract) ainda UNKNOWN.
**Próximo:** decidir ponto de serviço do dispatch de áudio no host (sem fix).
Input title/file-select diferido. Evidência:
[[../../plan/evidence/p08-slot7-scheduled-event-validation]],
[[../../plan/evidence/p08-slot7-scheduled-event-driver]],
[[../../plan/evidence/p08-slot7-object-lifecycle-arbitration]],
[[../../plan/evidence/p08-session-handoff]],
[[../../plan/evidence/p08-attract-actor-loop-diagnosis]],
[[../../plan/evidence/p08-type4-transition-semantics-audit]]. Detalhe:
[[08-rt64-input-and-frontend]].
