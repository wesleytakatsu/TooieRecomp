---
title: "Plano — Seleção de idioma (Inglês / Português do Brasil / FR / DE / ES)"
description: "Plano de trabalho da opção de idioma: pacote pt-BR embutido via bundle TLPB e fonte procedural, importação in-game de FR/DE/ES a partir da ROM europeia em C++, e mapeamento de acentos."
status: active
type: plan
project: TooieRecomp
updated: 2026-10-27
tags: [tooie-recomp, plan, language, localization, text, font]
---
# Plano — Seleção de idioma (Inglês / Português do Brasil / FR / DE / ES)

Documento de trabalho temporário (atualizado em 2026-10-27). Ao concluir, o contrato final vai para uma spec em
`docs/specs/Tooie/` e este arquivo é removido. Estado corrente geral: [[HANDOFF]].

## 1. Objetivo

Opção **Game Language** no menu General (launcher e menu ESC no jogo) com **English (original da ROM)**,
**Português (Brasil)** (embutido), **Français**, **Deutsch** e **Español** (importados). Diálogos e menus aparecem no
idioma escolhido com renderização correta de caracteres acentuados.

## 2. O que já sabemos (análise de 2026-10-27)

| Fato | Classe |
|---|---|
| A tradução (`banjo-tooie-pt-br`, v0–v4, autor CLEHPTON) é patch para a ROM **europeia** `Banjo-Tooie (Europe) (En,Fr,De,Es).z64` (SHA-1 `93bf2fac1387320ad07251cb4b64fd36bac1d7a6`; CRC32 no BPS v4 `0x1ec12f5a`). | `PROVEN` |
| O patch v4 só altera dados (ROM `0xBB2B7E`–`0xC77904`, ~800 KB), nenhum código. | `PROVEN` |
| Cada asset de texto europeu tem 4 idiomas: `04` + 4 offsets u16 LE + seções. O autor sobrescreveu a seção 0 (inglês) com pt-BR; FR/DE/ES ficam intactos. | `PROVEN` |
| O asset de texto USA é `01 03 00` + uma seção. A seção tem o mesmo formato nas duas versões (grupos `[n][tipo len dados]`). | `PROVEN` |
| Tabela de assets USA: índice `0x5188 + id*4`, payload `0x12B24 + (entry>>8)*4`, tamanho = próxima entrada − atual, 2 bytes + deflate cru. Textos têm flag `0x13`, IDs `0x0CDA`–`0x1C6B`. | `PROVEN` |
| O bundle pt-BR (`assets/languages/pt-BR.tlpb`, ~309 KB) contém 2.230 seções traduzidas completas (kind 0), 4 overlays (kind 1: por registro de texto USA uma flag manter/substituir sem copiar inglês), 37 byte runs (209 bytes) de patch da fonte 0 aplicados sobre a fonte 0 USA do jogador após verificação de tamanho + CRC32, e o charset (73) + mapa de acentos (30) da fonte 1 europeia. Zero texto ou arte da Rare. | `PROVEN` |
| A fonte 1 pt-BR é gerada em tempo de execução a partir da fonte 1 USA (`0x0C22`, 51 quadros IA8, altura 23 preenchida para 31 com 8 linhas transparentes no topo) mais 23 quadros compostos por código procedural (`include/tooie_font_sprite.h`): Ã Á À É È Í Ì Ç Õ Ó Ò Ü Ú Ù ¿ ¡ ª ° Ê Ï Ô Â e a vírgula, na ordem dos quadros 51–73 do tradutor, usando máscaras de acento com cores de preenchimento e contorno da fonte; ¿/¡ são ?/! rotacionados; vírgula é apóstrofo transladado. | `PROVEN` |
| O texto do tradutor usa significados de código distintos na fonte 0 e na fonte 1 (ex.: `i` = Â em nomes de lugares na fonte 1; `h` mostra vírgula; `*`/`+` não mostram nada na fonte 1). Reproduzir as identidades de quadro garante fidelidade visual ao build europeu do autor. | `PROVEN` |
| O importador C++ nativo (`include/tooie_language_import.h`) extrai FR/DE/ES da ROM europeia (validada por tamanho e CRC32) gerando pacotes `.tlp` atômicos diretamente no jogo, com prompt localizado, sem precisar de Python para os jogadores. | `PROVEN` |
| A ROM do runtime é um `std::vector` (`lib/N64ModernRuntime/librecomp/src/pi.cpp`), lido por DMA sem limite fixo de 32 MiB. | `PROVEN` (código) |

## 3. Decisões e limites

- **Português (Brasil) vem embutido na distribuição.** O bundle `assets/languages/pt-BR.tlpb` (~309 KB) é distribuído
  com o projeto e a release (autorização do autor CLEHPTON obtida pelo dono do projeto). Contém apenas o trabalho do
  tradutor e dados próprios; zero texto ou arte da Rare. No boot, o jogo constrói o pacote pt-BR em memória a partir do
  bundle + ROM USA do jogador, sem exigir ROM europeia nem arquivo `.tlp`.
- **Importação no jogo para Francês, Alemão e Espanhol.** Ao selecionar FR, DE ou ES em Settings > General > Game Language
  sem o respectivo pacote `.tlp` instalado, um diálogo nativo explica que a ROM europeia é necessária apenas para
  extrair aquele idioma uma única vez (o jogo sempre executa a partir da ROM USA). O usuário seleciona a ROM pelo seletor de
  arquivos do sistema; o importador C++ nativo valida o CRC32, extrai o pacote atômico `<app>/languages/<tag>.tlp` e
  informa sucesso ou falha na língua do idioma. Não é necessário Python para os jogadores.
- **Sem suporte à ROM europeia como jogo executável.** O jogo continua sendo o executável recompilado USA; a ROM europeia
  é fonte temporária de extração de dados.
- **Aplicação na imagem da ROM antes do boot.** O TooieRecomp reescreve a tabela de assets na cópia em memória
  (`recomp::get_rom()` → `recomp::set_rom_contents()`), sem tocar em código gerado (`RecompiledFuncs/`).
- **Trocar o idioma vale ao reiniciar o jogo.** Troca ao vivo em tempo de execução permanece fora do escopo.
- **`tools/build_language_packs.py` é ferramenta de desenvolvedor.** Constrói o bundle `pt-BR.tlpb` e pacotes de
  referência `.tlp` para testes.

## 4. Arquitetura

```text
[pt-BR]: assets/languages/pt-BR.tlpb (embutido) + ROM USA do jogador
                   │
                   ▼  boot (se Idioma = pt-BR)
src/game/language_pack.cpp: sintetiza pacote pt-BR em memória + gera fonte 1
proceduralmente (include/tooie_font_sprite.h) + aplica patch na fonte 0

[FR/DE/ES]: Seleção na UI -> falta <app>/languages/<tag>.tlp
                   │
                   ▼  prompt nativo no idioma
Usuário escolhe ROM europeia -> include/tooie_language_import.h (C++) extrai e
grava atomicamente <app>/languages/<tag>.tlp (validação de tamanho + CRC32)
                   │
                   ▼  boot seguinte
src/game/language_pack.cpp: lê <tag>.tlp do disco
                   │
                   ▼  (caminho comum de reescrita em memória)
Copia ROM USA em memória, realoca o rabo de assets a partir de 0x0C21 para o fim
da imagem buffer (~35 MB -> ~49.7 MB), atualiza offsets na tabela de assets, set_rom_contents()
                   │
                   ▼
src/game/text_font_map.cpp: decodifica acentos e mapa da fonte 1 europeia
```

## 5. Fases

### Fase 0 — Pré-requisitos (usuário) — CONCLUÍDA (2026-10-27)

- [x] ROM europeia verificada por SHA-1 (`93bf2fac1387320ad07251cb4b64fd36bac1d7a6`).
- [x] Autorização do autor CLEHPTON para distribuição do trabalho no bundle obtida pelo dono do projeto.

### Fase 1 — Prova de conceito e engenharia reversa da fonte — CONCLUÍDA (2026-10-27)

- [x] **RE do texto e da fonte**: ID de texto = ID de asset; tabela lida por `*(0x8011B900)`; fontes em `0x8011B560`; sprite IA8 da fonte 0; decodificação de códigos de escape de acento na fonte 1 europeia. `PROVEN`.
- [x] **Reescrita no boot**: `src/game/language_pack.cpp`, callback `on_rom_loaded_callback` no librecomp, expansão da ROM em memória e teste unitário `tooie_language_pack_test` PASS.
- [x] **Validação inicial**: textos de teste em memória aplicados e verificados.

### Fase 2 — Pacotes de idioma, bundle TLPB e importador C++ — CONCLUÍDA (2026-10-27)

- [x] **Bundle pt-BR (`.tlpb`)**: empacota seções kind 0, overlays kind 1, patch de fonte 0 e tabelas de mapeamento sem dados da Rare; `tooie_language_bundle_test` PASS.
- [x] **Importador C++ nativo**: `include/tooie_language_import.h` extrai FR, DE e ES da ROM europeia sem Python; saídas equivalentes ao gerador de referência; `tooie_language_import_test` PASS.
- [x] **Gerador offline de referência**: `tools/build_language_packs.py` atualizado como ferramenta de desenvolvedor; `tools/test_build_language_packs.py` PASS.

### Fase 3 — Opção na UI e fluxo de importação — CONCLUÍDA (2026-10-27)

- [x] **UI de importação e configuração**: `src/game/language_import_ui.cpp` gerencia prompts localizados, diálogo do sistema para seleção da ROM e caixas de mensagem de sucesso/erro. Configuração persistida em `general.json`.

### Fase 4 — Fontes procedurais e acentos — CONCLUÍDA (2026-10-27)

- [x] **Geração procedural de fonte 1**: `include/tooie_font_sprite.h` expande a fonte 1 USA para altura 31 e sintetiza 23 quadros acentuados usando máscaras vetoriais com as cores da fonte; alvo de preview `tooie_font_preview` (PPM); `tooie_font_sprite_test` PASS.
- [x] **Patch da fonte 0**: 37 byte runs aplicados sobre a fonte 0 USA com validação de CRC32.
- [x] **Mapeamento de caracteres**: `src/game/text_font_map.cpp` com suporte às tabelas europeias; `tooie_text_font_map_test` PASS.

### Fase 5 — Validação e documentação

- [x] **Validação no Windows (2026-10-27)**:
  - Testes unitários PASS (`tooie_language_pack_test`, `tooie_text_font_map_test`, `tooie_language_import_test`, `tooie_language_bundle_test`, `tooie_font_sprite_test`, `tools/test_build_language_packs.py`).
  - Em jogo: pt-BR aplicado sem pasta `languages/` (2.236 assets), confirmação no menu de pausa exibe "NÃO" com Ã sintetizado.
  - Importação de FR pelo launcher: diálogo em francês, Cancel restaurou inglês, Confirm + ROM europeia gravou `fr.tlp`, mensagem de sucesso e boot em francês.
  - Importação de DE com ROM australiana: mensagem de erro em alemão, opção revertida; repetição com ROM europeia gerou sucesso.
  - Fallback gracioso com log e mensagem sem falhas; integridade de saves confirmada.
- [ ] **Validação no Linux**: validação pendente a ser executada pelo usuário em PC Linux nativo.
- [x] **Documentação**: specs atualizadas (`language-packs.md`, `asset-and-licensing-manifest.md`), RE (`text-and-fonts.md`), `HANDOFF.md` e READMEs multilíngues.

## 6. Riscos

| Risco | Mitigação |
|---|---|
| Ã/Õ redesenhados alterarem também o "A" | Quadros acentuados gerados em índices independentes (51–73) na fonte 1 e gerados via máscaras procedurais |
| A tabela europeia tem IDs diferentes da USA | Regras estruturais de alinhamento com fallback gracioso para inglês em divergências |
| Imagem de ROM maior que o esperado por algum código | Expansão dinâmica em memória monitorada (~49.7 MB); lida normalmente via DMA no librecomp |
| Seleção de ROM incorreta na importação | Validação rigorosa de tamanho de arquivo e hash CRC32 antes de qualquer extração |

## 7. Pendências

- Validação no Linux pelo usuário em hardware nativo.
