---
title: "BT P10 — Geração 100% Kit-Free e Restauração Integral da UI de Menu"
description: "Evidência técnica da validação da geração estática kit-free provada por Sonnet em sandbox bubblewrap (420/420 arquivos idênticos byte a byte, 1159 passos de build, 8 testes PASS) e reconciliação da UI restaurada pelo root conforme correção formal do usuário."
status: current
type: evidence
project: TooieRecomp
date: 2026-10-21
tags: [p10, kitfree, n64recomp, symbols-route, restored-ui, bubblewrap, proof, rom-assets, suplexmentary, toolchain]
---

# BT P10 — Geração 100% Kit-Free e Restauração Integral da UI de Menu

## 1. Visão Geral e Contexto das Decisões

Esta nota registra a conclusão e prova técnica de dois marcos críticos da fase P10:
1. **Eliminação do Kit de Compiladores Legado (Decisão D2 do Usuário / Sonnet):** O usuário autorizou a eliminação do kit de compiladores IDO 5.3 / gcc 2.7.2 como pré-requisito normal de compilação. A geração estática de código C (`RecompiledFuncs/`) passou a ler diretamente a imagem da ROM descompactada (`decompressed.us.z64`) e os metadados de símbolos comutados (`metadata/tooie.us.syms.toml`), sem exigir compilação de ELF MIPS nem ferramentas MIPS externas.
2. **Restauração Integral da Interface de Usuário (Correção D1/D2 do Usuário / Root):** O usuário esclareceu formalmente que a restrição de recursos próprios/livres se aplicava estritamente aos **dados de jogo da ROM** (texturas, áudio, geometria, scripts). A interface de menu, animações, ícones, fontes tipográficas e logo desenvolvidos previamente foram autorizados para preservação e hospedagem local no repositório. O root restaurou todos os 44 arquivos originais do backup de candidatos (`candidate-backup`), superando a substituição interina neutra (774x272).

---

## 2. Prova Causal da Geração Kit-Free (`PROVEN`)

A validação da geração kit-free foi executada e comprovada em sandbox estrita de isolamento bubblewrap (`tools/offline_sandbox.sh`, bwrap, sem acesso à rede, diretório `/home` vazio, checkouts irmãos invisíveis, árvore extraída de tarball de código-fonte sem metadados `.git`, tendo como única entrada explícita a cópia da ROM do usuário):

- **Execução do Pipeline:** `tools/build_linux.sh preflight verify extract recomp` completado com exit code 0 em 90 segundos.
- **Identidade Byte a Byte das Funções Recompiladas:** Todos os 420 arquivos em `RecompiledFuncs/` (417 arquivos C + `lookup.cpp` + `funcs.h` + `recomp_overlays.inl`) são **rigorosamente idênticos em SHA-256 à linha de base gerada pela rota antiga de ELF** (`new_gen.sha256 == baseline_gen.sha256`; `diff -rq` vazio). Nenhum arquivo `.elf` ou pasta `decomp-work` foi utilizado ou criado.
- **Microcode RSP:** `tools/run_rsprecomp.sh` gerou `rsp/n_aspMain.cpp` idêntico byte a byte ao código-fonte rastreado no repositório (SHA-256 `31d946c6cb8ca72f9ef0...`).
- **Compilação Completa e Testes:** `tools/build_linux.sh configure build test package` completou com exit code 0 (1159/1159 passos de compilação Clang/Ninja; 8 testes unitários host executados com status `PASS`).
- **Smoke Tests de Execução:** Processo executado em ambiente isolado (20 segundos cada):
  - Modo apenas launcher: processo permanece vivo e responsivo.
  - Modo autostart com ROM descompactada preparada (`bt.n64.us.1.0.z64` no diretório de configuração + `TOOIE_AUTO_START=1`): processo permanece vivo e registra no log `"TOOIE_AUTO_START: starting game"`.
- **Comportamento Negativo (Falha Segura):** A ausência ou fornecimento de ROM descompactada zerada em `tools/run_recomp.sh` resulta em falha imediata com exit 1, mantendo `RecompiledFuncs/` intocado (420 arquivos antes e depois). ROMs incorretas ou ausentes falham no preflight antes de qualquer escrita.
- **Teste de Seções Estáticas Nulas:** `tests/n64recomp_zero_size_static_test.sh` passou com status `PASS` (quirk das seções `.cosection` e `.dbpalette` contornado sem alterar a numeração `static_344`).

---

## 3. Análise Causal: Falha da Rota Antiga de Símbolos e Correção

A investigação técnica detalhada em `kitfree-investigation.md` e implementada por Sonnet diagnosticou exatamente por que a rota de arquivo de símbolos (`from_symbol_file`) do gerador upstream `N64Recomp` falhava em reproduzir o código:

1. **Perda da Flag Relocatable:** Seções sem entradas de relocação em metadados perdiam a flag relocatable, causando falha dura em seções de dados estáticos como `.cosection`.
2. **Numeração de Seções ELF:** A numeração original das seções no ELF (`static_N_` e `RELOC_*(N, ...)`) não era preservada.
3. **Ordem de Emissão de Funções:** A ordenação sequencial de funções no arquivo de saída divergia da rota ELF.
4. **Descarte de Relocações de Runtime:** 74.613 de 104.964 entradas de relocação (seções cruzadas e relocações `R_MIPS_32`) eram descartadas pela rota de símbolos upstream.
5. **Nome da ROM (`get_rom_name`):** A lógica da função dependia do caminho do ELF em vez de metadados explícitos.

### Correção no Owning Layer (`lib/N64ModernRuntime/N64Recomp`)

A correção foi aplicada diretamente no gerador vendorizado:
- Arquivos modificados: `lib/N64ModernRuntime/N64Recomp/src/{main.cpp, config.cpp, config.h}`.
- Patch versionado criado: `vendor/patches-local/n64recomp-symbols-file-fidelity.patch` (SHA-256 `ddd85cc85831b2503b1951054cb92315b0122659b1603...`).
- Registro em `vendor/patches-applied/ORDER.tsv` atualizado (19 patches vendorizados ativos; verificação de reversão `tools/vendor_verify.sh --reverse-check` com status `OK`).
- Metadados regenerados via `tools/generate_metadata.py --dump-context`: `metadata/tooie.us.syms.toml` (10.800.523 bytes, SHA-256 `4e0845ca12735ade3ba3...`), contendo apenas nomes, endereços, tamanhos e descritores de seções/relocações (zero código ou dados proprietários da ROM).
- Adicionado verificador automatizado `tools/verify_generated.sh` e manifesto de saída `metadata/recomp-output.sha256` (420 hashes). O arquivo duplicado obsoleto `metadata/n64recomp_dump.raw.toml` foi removido.

---

## 4. Disciplina Epistêmica e Arbitragem

Conforme diretrizes de governança do repositório:
- **Não Inferência de Identidade do Payload ELF:** O fato de a saída em C ser idêntica em ambas as rotas prova a fidelidade dos metadados e do gerador; **não prova por si só que o payload `.text` do ELF original é idêntico a fatias da ROM**, visto que o gerador lê as palavras de função diretamente da ROM em ambas as rotas.
- **Causa Real da Falha Inicial de Símbolos:** A falha no teste comparativo inicial decorreu de um aborto parcial (`rc=1`) provocado pela perda da flag relocatable em `.cosection`, e não por defasagem na idade dos metadados (`tooie.us.syms.toml`). Sonnet corrigiu formalmente essa alegação da investigação preliminar.
- **Subtotais de Relocação:** A aparente discrepância na soma dos subtotais (59.889 + 18.967 != 74.613) decorre de sobreposição entre conjuntos de filtros analíticos, não de contagem errônea de relocações.

---

## 5. Restauração Integral da Interface de Usuário (Root)

Conforme a correção formal do usuário:
- **Restauração de Arquivos:** O root executou a restauração de 44 arquivos a partir de `/home/wesley/.cache/tooie-rom-assets-review-20261021/candidate-backup` para a árvore de trabalho (`root-ui-restore.json`).
- **Identidade Visual Restaurada:**
  - Logo do Launcher: `ui_assets/TooieLogo.svg` com dimensões e viewBox originais (`0 0 7300 2450`), escala `7300.0f * 0.106f` x `2450.0f * 0.106f`, apresentando o título em duas linhas ("Banjo-Tooie" / "Recompiled").
  - Artes vetoriais e animação: `Banjo.svg`, `Kazooie.svg`, `Cloud1-4.svg`, `JiggyColor.svg`, `JiggyHole.svg`, `JiggyShine.svg` e `Logo.svg`. Animação completa em `src/main/launcher_animation.cpp`.
  - Fontes tipográficas: `Suplexmentary Comic NC.ttf` restaurada; fontes abertas `InterVariable.ttf`, `NotoEmoji-Regular.ttf` e `promptfont.ttf` mantidas com avisos de licença SIL OFL 1.1.
  - Ícones de interface: Ícones de navegação, configurações e controles (`assets/icons/` incluindo subdiretório `VizMap/`).
- **Superação da Interface Neutra Interina:** A geometria neutra (774x272 / 29 assets / Inter-only) foi formalmente **SUPERSEDED**. O binário gerado durante o teste interino neutro (`5c2c8df2c8165f7bb2b54e4f93b2198db51be34b47eeba597723fd05afedc181`) foi superado e não deve ser publicado como binário final.
- **Gerenciamento de Assets no CMake:** A regra do alvo gerenciado `TooieUIAssets` em `CMakeLists.txt` foi mantida intacta, garantindo que `<build>/assets` seja limpo e sincronizado em cada build do executável com proteção contra compilação in-source (`CMAKE_BINARY_DIR != CMAKE_SOURCE_DIR`).
- **Status da Compilação da UI Restaurada e Empacotamento Final:** O root executou a compilação do executável com a UI restaurada (`root-restored-ui-build-result.json`), completando configure e build com sucesso (`ROOT_BUILD_EXIT=0`). O binário gerado (SHA-256 `0cfcd79206efc74b74e26836ce468c69bcadaf2c3950b40eb31971946026892b`) é rigorosamente idêntico ao binário pré-redesenho. A imagem visual do launcher foi inspecionada (`launcher-restored-menu.png` / `root-restored-menu-visual.json`, PID próprio capturado com exit 0). O pacote final para Linux foi gerado e verificado com sucesso: `TooieRecompiled-linux-x86_64-73bfd43-dirty-private-local.tar.gz` (SHA-256 `4f702d2ba0a0472345f6f1317bfe75eb0df462e96b2352d6d875edaa5c4ebcf9`, 104 arquivos regulares, `tools/verify_package.sh` exit 0, UI exata, zero ROM). Preflight kit-free do root exit 0. A prova limpa kit-free (420 arquivos idênticos, 1159 passos, 8 testes) utilizou o interim neutro 5c2 em sandbox isolada, separada da prova do build com UI restaurada (sem fusão de escopos).

---

## 6. Invariantes de Sistema e Estado de Entrega

1. **Status do Glitch de Interpolação:** O gate técnico de glitch de interpolação articulada P09 permanece **FAILED** (`passed: false`). O padrão de produção no menu Graphics permanece strictly `CURRENT` / 30 FPS (`interpolation: false`). O atalho F8 permanece desativado. As fases P09 e P10 permanecem formalmente abertas.
2. **Status Windows 11 x64:** Status permanece `WINDOWS_BUILD_PENDING_NATIVE_HOST`. Nenhum executável PE/DLL de jogo existe nos artefatos. A geração via WSL2 é kit-free (`tools/build_linux.sh preflight verify extract recomp`) e o código gerado é consumido nativamente no Windows via `.\tools\windows\Build-Tooie.ps1 -Stage ImportGenerated`.
3. **Requisito Estrito da ROM Descompactada:** O launcher exige estritamente a imagem descompactada gerada por `tools/extract_rom.sh` (`build-linux/extract/decompressed.us.z64`, tamanho `35098224` bytes, SHA-1 `af479771ff5e91ce3732f3d84146614ab71d8d79`, XXH3 `9F843AD3455EA395`). ROMs de cartucho comprimidas (XXH3 `00F70DE5F2D70EA2`) são rejeitadas.
4. **Estágio Mantenedor Isolado:** A compilação da ROM para ELF MIPS e a regeneração de metadados (`tools/build_linux.sh decomp`) foram isoladas como estágio estritamente de mantenedor, dependente de `TOOIE_TOOLCHAIN_KIT`.
5. **Disciplina de Git:** Nenhuma alteração foi staged, commitada ou enviada ao repositório remoto. O working tree local sujo permanece preservado.
6. **Arquivo-Fonte de Entrega e Checksum:** O arquivo-fonte de entrega é gerado a partir do estado documental final; o checksum fica no sidecar externo adjacente e em `dist/rom-assets-20261021/VALIDACAO.json`, sem autorreferência embutida no arquivo.
7. **Próxima Ação Única:** `NEXT_SINGLE_ACTION` é a transferência do código-fonte e bundle privado gerado para ambiente Windows 11 x64 para execução do build nativo; nenhum commit ou push nesta sessão.
