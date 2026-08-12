---
title: "Plano mestre — Banjo-Tooie nativo no Linux"
description: "Plano executável para transformar os insumos BanjoRecomp e banjo-tooie-Recomp em um port nativo completo de Banjo-Tooie para Linux."
status: active
type: plan
project: TooieRecomp
platform: linux-x86_64
updated: 2026-08-12
source_path: banjo-tooie-Recomp
source_repo: wesleytakatsu/banjo-tooie-Recomp
upstream_repo: Mr-Wiseguy/banjo-tooie
tags:
  - banjo-tooie-Recomp
  - static-recompilation
  - linux
  - plan
aliases:
  - "Plano TooieRecomp"
  - "Banjo-Tooie Linux"
---

# Plano mestre — Banjo-Tooie nativo no Linux

Este é o ponto de entrada do trabalho. As especificações descrevem o que já
existe; as etapas em [[../plans/BT/index|Planos de implementação BT]] descrevem
o que deve ser produzido e como provar que está pronto.

## Objetivo

Entregar um executável Linux x86-64 de Banjo-Tooie gerado por recompilação
estática, usando a ROM legítima do usuário apenas como fonte de dados. O port
deve preservar o comportamento do N64, integrar `N64ModernRuntime` e `RT64`, e
funcionar sem emulador.

## Decisão de arquitetura

```text
Banjo-Tooie NTSC-U Rev. 0
            |
            v
banjo-tooie-Recomp: decompress + Splat + ELF + símbolos + overlays + relocations
            |
            v
N64Recomp + RSPRecomp
            |
            v
C/C++ gerado + patches nativos específicos de Tooie
            |
            v
N64ModernRuntime + RecompFrontend + RT64
            |
            v
TooieRecompiled (Linux x86-64)
```

Não é uma troca de ROM dentro do BanjoRecomp. O BanjoRecomp é a implementação
de referência para runtime, frontend, renderização, entrada, patches e
empacotamento; cada dado específico de Banjo-Kazooie precisa ser substituído.
Veja [[../specs/BK/reuse-contract|Contrato de reutilização do BanjoRecomp]] e
[[../specs/BT/recomp-readiness|Prontidão do banjo-tooie-Recomp para recompilação]].

## Estado inicial verificado

| Item | Estado | Evidência |
| --- | --- | --- |
| ROM NTSC-U Rev. 0 em `.z64` | pronto | SHA-1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`, 32 MiB, ID `NB7E` |
| Fork `banjo-tooie-Recomp` | presente | `origin` = `wesleytakatsu/banjo-tooie-Recomp`, `upstream` = `Mr-Wiseguy/banjo-tooie` |
| Baseline BT | verificada | `master`/`origin/master`, commit `25033be6ec9e35975483f064d086391d2381ca4c`, 0 ahead/0 behind do `upstream/master`, worktree limpo |
| Insumos BT | presentes no fork | decompressor, Splat, definições de ELF, símbolos, relocations, overlays e syscalls; artefatos gerados ainda dependem de setup/build |
| Fonte `BanjoRecomp` | presente | runtime, RT64, RSP, frontend, patches e build Linux de referência |
| Toolchain host básica | **pronta (P01)** | MIPS cross, fmt/toml11/SDL2/lld, venv Python, tools BT, N64Recomp/RSPRecomp |
| Submódulos BT + BK | **inicializados** | pins do baseline; 0 não inicializados em BT |
| Projeto `TooieRecomp` | **scaffold P03** | `TooieRecomp/` com configs, gerador e metadata; executável ainda em P05 |
| Baseline P00 | **done** | [[baseline-manifest|manifesto]] + `scripts/validate-baseline.sh` PASS |
| Toolchain P01 | **done** | [[../plans/BT/01-linux-toolchain|etapa]] + `docs/plan/evidence/p01-*` |
| ROM/ELF P02 | **done** | matching decomposto OK; ELF `build/us/banjotooie_decompressed.elf` |
| Metadados P03 | **done** | `TooieRecomp/metadata/` + dump N64Recomp; projeto `TooieRecomp/` criado |
| Recomp P04 | **done** | `RecompiledFuncs/` (~21k funcs) + `libRecompiledFuncs.a` Clang |
| Runtime P05 | **done** | `TooieRecompiled` linka RT64; launcher/Vulkan OK; frame de jogo → P06 |
| Overlays P06 | **done** | overlays/syscalls funcionais; lifecycle pós-P06 corrigido, mas gameplay ainda não alcançado |
| RSP/áudio P07 | **active** | intro áudio OK; soak/SFX residual |
| RT64/input P08 | **active (intro → attract blocker)** | intro/Jiggy/type-4/warp OK; freeze em C2AB8 slot7 (`table[2]` pool object never cleared); lifecycle arbitration aberta (origem/root cause) |

O nome `Mr-Wiseguy/banjo-tooie` identifica o upstream. Os caminhos e comandos
locais deste plano devem usar `banjo-tooie-Recomp/`. Desenvolvimento e
pushes devem ocorrer no fork (`origin`); o remote `upstream` serve para
sincronização e comparação.

## Definição de “rodar perfeitamente”

O trabalho só está concluído quando todos os critérios de
[[definition-of-done|Definição de pronto]] estiverem satisfeitos. Em resumo:

- o jogo completa o boot e uma campanha inteira sem crash ou softlock;
- gráficos, áudio, entrada, saves, overlays e transições preservam o original;
- o timing original é a referência e modos de FPS alto não alteram gameplay;
- o build é reproduzível em Linux limpo e gera pacote redistribuível sem ROM;
- testes, hashes, logs e versões ficam registrados como evidência.

## Caminho crítico

| Ordem | Etapa | Gate de saída | Estado |
| ---: | --- | --- | --- |
| 00 | [[../plans/BT/00-baseline-and-decisions|Baseline e decisões]] | ambiente, hashes e versões registrados | done |
| 01 | [[../plans/BT/01-linux-toolchain|Toolchain Linux]] | dependências e submódulos validados | done |
| 02 | [[../plans/BT/02-rom-decompression-and-elf|ROM, decomposição e ELF]] | `make setup` e ELF reproduzíveis | done |
| 03 | [[../plans/BT/03-recompiler-metadata|Metadados do recompilador]] | símbolos, seções, overlays e relocations gerados | done |
| 04 | [[../plans/BT/04-static-recompilation|Recompilação estática]] | C gerado compila sem símbolos ausentes | done |
| 05 | [[../plans/BT/05-native-runtime-and-boot|Runtime e boot]] | executável alcança o primeiro frame | done |
| 06 | [[../plans/BT/06-overlays-and-syscalls|Overlays e syscalls]] | carregar, executar, mover e descarregar overlays funciona | done |
| 07 | [[../plans/BT/07-rsp-and-audio|RSP e áudio]] | microcódigos conhecidos e áudio estável | active |
| 08 | [[../plans/BT/08-rt64-input-and-frontend|RT64, entrada e frontend]] | logo/menu/gameplay renderizam e controles funcionam | active (first attract demo bloqueada por type-4 incompleta) |
| 09 | [[../plans/BT/09-gameplay-parity-and-enhancements|Paridade e melhorias]] | campanha, saves e timing aprovados | pending |
| 10 | [[../plans/BT/10-linux-packaging-and-release|Empacotamento e release]] | pacote Linux limpo, reproduzível e sem assets do jogo | pending |

## Marcos de integração

1. **M0 — Insumos confiáveis:** ROM, commits, submódulos e toolchain fixados.
2. **M1 — Compile-only:** `RecompiledFuncs` gerado e compilado.
3. **M2 — First frame:** runtime inicializa RDRAM, entra no código recompilado e
   apresenta um frame.
4. **M3 — Logo da Rare:** CPU, RSP, RT64, áudio e entrada atravessam o boot.
5. **M4 — Gameplay:** uma área jogável carrega e troca overlays corretamente.
6. **M5 — Paridade:** campanha e saves aprovados em uma matriz de regressão.
7. **M6 — Release:** artefato Linux reproduzível e pronto para usuário final.

## Estratégia de execução

- Trabalhar uma etapa por vez e não iniciar a próxima sem o gate anterior.
- Registrar cada comando reproduzível no documento da etapa correspondente.
- Tratar arquivos gerados como produtos de ferramentas, não como fontes
  mantidas manualmente.
- Manter patches funcionais separados da camada de compatibilidade obrigatória.
- Preservar primeiro timing, resolução e comportamento originais; melhorias
  entram somente depois da paridade.
- Fixar SHAs dos repositórios e hashes dos artefatos usados em cada marco.
- Executar comandos BT pelo caminho `banjo-tooie-Recomp/`; não recriar nem
  depender do checkout excluído `banjo-tooie/`.
- Abrir branches de implementação no fork e manter `upstream` como referência.

## Riscos prioritários

| Risco | Efeito | Mitigação |
| --- | --- | --- |
| Modelo de overlays mais complexo que BK | chamadas inválidas e corrupção de RDRAM | gerar tabelas a partir do ELF/ROM e testar load/unload isoladamente |
| Relocations ou syscalls incompletos | falhas ao entrar em código dinâmico | validar cobertura e abortar geração diante de tipo desconhecido |
| Microcódigo RSP diferente | ausência de áudio ou tarefas desconhecidas | inventariar `OSTask`, recompilar cada variante e manter erro explícito |
| Dependência implícita de frame | física/timers incorretos em FPS alto | obter paridade no timing original antes de desacoplar updates |
| Divergência do decomp | símbolos e offsets inconsistentes | fixar commit, reconstruir ROM matching e guardar hashes |
| Dependência acidental de assets | release juridicamente impróprio | teste automático que rejeita ROM e assets no pacote |

## Fonte de verdade

- Especificações BK: [[../specs/BK/index]]
- Especificações BT: [[../specs/BT/index]]
- Etapas implementáveis: [[../plans/BT/index]]
- Critérios finais: [[definition-of-done]]
- Baseline P00: [[baseline-manifest]]
- Registro anterior, agora supersedido: [[../roadmap/tooie-recomp]]

## Evidência P08 corrente

- [[evidence/p08-type4-transition-semantics-audit]] — auditoria estática +
  clean-freeze type-4 snapshot (8/8 runs): type-4 completa, campos
  `0.9/3.5/2883` corrigidos para `+0x0C/+0x18/+0x10`, warp consumido; blocker
  downstream no mode-5/attract init.
- [[evidence/p08-session-handoff]] — estado autossuficiente e próximo single
  action para um novo chat.
- [[evidence/p08-intro-to-demo-transition-diagnosis]] — timeline histórica;
  narrativa antiga de type-4 incompleta marcada como SUPERSEDED.
- [[evidence/p08-visual-validation/README]] — inventário/interpretação humana
  dos frames da intro ao preto pós-Jiggy.

## Atualização de estado

Ao concluir uma tarefa, marque o checkbox no arquivo da etapa e anexe a
evidência na seção `Evidências`. Ao fechar uma etapa, altere `status` para
`done` e atualize sua linha neste plano. Isso permite ao Markdown Vault MCP
extrair tarefas, montar briefings e recuperar o contexto pelo campo `phase`.
