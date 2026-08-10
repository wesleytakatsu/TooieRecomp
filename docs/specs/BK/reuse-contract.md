---
title: "BK — Contrato de reutilização para TooieRecomp"
description: "Separa componentes reutilizáveis do BanjoRecomp de dados e comportamento exclusivos de Banjo-Kazooie."
status: current
type: specification
project: BanjoRecomp
tags: [banjo-kazooie, banjorecomp, architecture, reuse]
---

# BK — Contrato de reutilização para TooieRecomp

Esta especificação define o BanjoRecomp como referência arquitetural, não como
um executável capaz de aceitar outra ROM.

## Componentes reutilizáveis

| Componente | Contrato aproveitável |
| --- | --- |
| CMake e layout do executável | ligação do código gerado, runtime, frontend e assets livres |
| `N64ModernRuntime` | RDRAM, threads, eventos, áudio, entrada e ciclo de execução |
| `RT64` | interpretação de display lists e apresentação por API gráfica moderna |
| `RecompFrontend` | launcher, configuração, arquivos do usuário e mensagens |
| SDL/input | teclado, controles, mappings, rumble e hotplug |
| API de mods | exports, eventos, conteúdo e carregamento de extensões |
| infraestrutura de patches | patch ELF, exports, eventos e símbolos manuais |
| CI/empacotamento | build Linux, Flatpak e validação de artefatos |

## Pontos de integração verificados

O entrypoint nativo registra, nesta ordem conceitual:

1. identidade e caminhos do programa;
2. jogos e hashes de ROM aceitos;
3. exports do runtime/frontend;
4. tabelas de overlays e patches;
5. configuração do jogo;
6. callbacks de RSP, renderização, áudio, entrada, janela, eventos, erros e
   threads;
7. `recomp::start`.

`register_bk_overlays` entrega `section_table`, número de seções e overlays por
índice ao runtime. `register_bk_patches` registra o ELF de patches, exports,
eventos e símbolos manuais. Tooie precisa gerar equivalentes próprios.

## Elementos que devem ser substituídos

- SHA/hash, título, ID e revisão da ROM BK.
- Descompressor BKZip e offsets da ROM BK.
- `banjo.us.rev0.toml`, mapas `BanjoRecompSyms` e código recompilado.
- Entry point, estado inicial de memória e tabela de seções.
- Tabelas, índices, carregamento e patches de overlays BK.
- Seleção de microcódigo RSP assumindo somente `n_aspMain`.
- Patches de câmera, saves, framerate, objetos e gráficos específicos de BK.
- Nome, ID, ícones, tema, textos e opções específicas do frontend.

## Regras de adaptação

- Cada dado específico de jogo deve ser gerado ou mantido no projeto Tooie.
- Não carregar ROM BT com estruturas/tabelas BK, mesmo quando o layout parecer
  semelhante.
- Reutilizar APIs de runtime; não copiar implementações internas sem necessidade.
- Introduzir patches apenas com bug/limitação reproduzível e teste associado.
- Manter o preset fiel sem melhorias como referência de paridade.

## Fontes locais

- [`src/main/main.cpp`](../../../../BanjoRecomp/src/main/main.cpp)
- [`src/main/register_overlays.cpp`](../../../../BanjoRecomp/src/main/register_overlays.cpp)
- [`src/main/register_patches.cpp`](../../../../BanjoRecomp/src/main/register_patches.cpp)
- [`src/game/rom_decompression.cpp`](../../../../BanjoRecomp/src/game/rom_decompression.cpp)
- [`banjo.us.rev0.toml`](../../../../BanjoRecomp/banjo.us.rev0.toml)
- [`BUILDING.md`](../../../../BanjoRecomp/BUILDING.md)

## Relações

- Índice BK: [[index]]
- Runtime BK: [[runtime]]
- Configuração BK: [[config]]
- Plano de runtime Tooie: [[../../plans/BT/05-native-runtime-and-boot]]
