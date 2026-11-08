---
title: "Handoff — TooieRecomp"
description: "Estado atual do TooieRecomp por área: build Linux/Windows, runtime, render, idiomas, release, pendências e próxima ação."
status: current
type: handoff
project: TooieRecomp
updated: 2026-11-08
tags: [tooie-recomp, handoff, status]
---
# Handoff — TooieRecomp

Estado corrente para um agente ou chat novo. Investigações antigas, planos concluídos (multijogador, idiomas) e
evidências ficam no histórico do git (`git log -- <caminho>`). Planos antigos foram consolidados: propostas não
implementadas em [[future/implementation-ideas]], contratos atuais nas specs, conhecimento nativo no RE e
validação pendente na spec de testes. Não há plano de execução ativo na raiz. Contratos estáveis:
[[specs/index]]. Conhecimento do jogo: [[reverse-engineering/README]].

## Estado por área

| Área | Estado | Referência |
| --- | --- | --- |
| Build standalone (vendorizado, kit-free) | Linux e Windows 11 x64 nativo; `VENDOR VERIFY OK` (54 patches) | [[specs/Tooie/build-and-generator]], [[specs/Tooie/packaging-and-dependencies]] |
| ROM / extração | ROM canônica `Banjo-Tooie (USA).z64` (privada, git-ignored) → imagem descomprimida exigida pelo launcher | [[specs/Tooie/asset-and-licensing-manifest]] |
| Runtime e overlays | dispatch de syscalls de overlay, `ovl_load`/`ovl_unload` host-aware; dedupe de `loaded_sections`; callback `on_rom_loaded` no librecomp | [[specs/Tooie/runtime-and-overlays]] |
| Áudio | `n_aspMain` + fila de saída corrigida; BGM controlável; protótipo nativo de ZIP `audio.json`; usuário testou por cópia na pasta mods e logs confirmam os três MP3s; camadas superfície/água sincronizadas host-testadas e com relato positivo do usuário no Windows; instalação pela UI exige tela inicial | [[specs/Tooie/audio-system]], [[specs/Tooie/modding-and-extensions]] |
| Render 4:3 / widescreen | 16:9 por planos de culling nativos (produção) | [[specs/Tooie/graphics-and-presentation]], [[reverse-engineering/systems/view-volume]] |
| Interpolação (60/120 FPS) | pareamento ordenado + guardas P09 + spike guard elementwise (produção) | [[specs/Tooie/graphics-and-presentation]], [[reverse-engineering/systems/transform-interpolation]] |
| Desempenho RT64 (cenas com ~1000 desenhos) | check sets do pareamento por chaves distintas e cache do hash de TMEM (patches `rt64-perf-*`, resultado idêntico); ponto da Glitter Gulch Mine de 9-12 para 60 FPS no Windows D3D12; o usuário jogou e validou no Windows (2026-10-30); Linux `NOT_RUN` | [[specs/Tooie/graphics-and-presentation]] |
| Entrada / UI / config | SDL2, multijogador local até 4 (opção default Off), menu ESC, FPS overlay, botão GitHub | [[specs/Tooie/input-and-controllers]], [[specs/Tooie/window-ui-and-config]] |
| Idiomas do jogo | EN; pt-BR/FR/DE/ES em `assets/languages/*.tlpb`, exigindo apenas ROM USA; perfil europeu IA8 corrige letras amarelas. Build Windows e 34/34 testes; usuário confirmou letras visíveis. FR observado; aceite específico DE/ES pendente | [[specs/Tooie/language-packs]], [[reverse-engineering/systems/text-and-fonts]] |
| Portáteis (aarch64, PortMaster) | build atrás de `TK64_MOBILE` com TK64 em OpenGL ES, na `main` desde a PR #2 (`05f04f8`); validado no XU10 com `libmali`; Panfrost relatado bom pelo usuário | [[specs/Tooie/handheld-portmaster]] |
| Modding | `.rtz` e ZIP original com raiz interna RT64; cópia manual, instalação pela interface e ativação/desativação de BT Reloaded 0.1.1 validadas pelo usuário no Windows (2026-10-30); Linux e reordenação `NOT_RUN`; resto da API herdado e não validado | [[specs/Tooie/modding-and-extensions]] |
| Dev Mode (menu ESC) | aba com aviso a aceitar; `Warp` salva, renomeia, exclui e teleporta para posições (`dev_warps.json`), inclusive entre mapas pelo `gcgoto` do jogo (posiciona 2 s depois de a fase nova carregar); validado nos demos e com save real no Windows pelo usuário; salvar com o jogo pausado (Start) `NOT_RUN`. `Cheats`: Debug Button (entrada nova dos controles, padrão R3/T) + direção do D-pad escolhida por cheat (`dev_cheats.json`); cheats `Fly` (estado de decolagem `0x23` do pad de voo) e `Breegull Blaster` (liga com `0xC8`, desliga com `0xC9` e recarrega o modelo), validados pelo usuário no Windows (2026-10-30); o Warp não anda com o Blaster ligado. Navegação do modal Warp presa ao modal (cima/baixo: posições; lados: Teleport/Rename/Delete; rolagem segue o foco; alavanca direita rola; Back/B fecha o modal; patch `recompfrontend-ui-scroll-top`): build e testes Windows, teste no jogo `PENDING_USER` | [[specs/Tooie/window-ui-and-config]], [[reverse-engineering/systems/warp-and-player-position]] |
| Testes | Windows `Build-Tooie.ps1 -Stage Test`: 31 obrigatórios (inclui câmera analógica/entrada SDL real, bounds do RSP, bancos de recarga P/V, distância de objetos, 5 de idioma, regressões de projeção/Glowbo/IA/menu de viagem e Win32 `rt64_hidden_window_test`); Linux `build_linux.sh test`: 19/19 PASS em `313e287` (2026-11-06); remoto registra relato geral de funcionamento pelo usuário, sem matriz por API | [[specs/Tooie/testing-and-oracle]] |

## Idiomas em arquivos (2026-11-08)

- Bundles locais FR/DE/ES gerados da EUR verificada, com 2.233/2.230/2.234 entradas de texto; reconstrução Python e
  paridade C++ contra os packs de referência passaram. `pt-BR.tlpb` preservado byte a byte. Fonte 1 gerada em IA8,
  74 glifos, com perfis europeu e pt-BR separados. Packs legados IA4 são normalizados em memória.
- Revisão estática na branch `codex/bundled-game-languages`, criada de `main` em `fdfbce1`: removidos helper
  `usa_image_path` sem chamadores e mapa pt-BR não consumido no gerador. As quatro unidades C++ de integração
  passaram compilação de sintaxe com diagnósticos de código não usado/inalcançável habilitados, sem avisos do projeto;
  Python verificado por AST. Build Windows atualizado, exit 0; nenhuma nova execução de testes ou do jogo.
  Arquivos preexistentes de análise/atmosfera preservados. Revisão concluída neste checkpoint; sem push.
- Seleção pelo General não oferece mais importação EUR. Bundle tem prioridade sobre cache antigo; ausência/corrupção
  mostra instrução de reparar os arquivos e restaura a opção anterior. Só a ROM USA é necessária na instalação local.
- Windows: `Build-Tooie.ps1 -Stage Test -Jobs 8` passou 34/34; `-Stage Build -Jobs 8` passou após a mudança final da UI.
  `tools/test_build_language_packs.py`: 7/7; paridade real de cada bundle e previews dos dois perfis passaram.
  `LC_ALL=C bash tools/vendor_verify.sh --reverse-check` via Git Bash: exit 0, 8.339 arquivos e 54 patches, `VENDOR VERIFY OK`.
  `git diff --check`: exit 0; 64 wikilinks dos documentos relacionados resolvidos. `markdown_vault_audit` indisponível
  neste host (servidor local configurado para caminho Linux); `graft build` atualizado com exit 0.
  Executável atualizado: `build-win/TooieRecompiled.exe`, SHA-256
  `783129bbc2a796703bf92ec187611593c0e1115c41deaf7e7ed1c2fcd0587686`.
- Sessão Windows isolada por `portable.txt`, sem ROM EUR nem pasta de packs importados: título de cenário francês
  em amarelo e pausa em francês observados. Primeiro ensaio falhou por recursos de UI ausentes no cwd isolado;
  repetir com `assets/`, `ui_assets/` e `recompcontrollerdb.txt` resolveu. Launcher normal abriu em D3D12.
  Usuário confirmou que as letras amarelas apareceram e parece tudo correto. Isso não prova todas as telas/idiomas.
- Controle da interface interrompido pelo Escape físico. Pedido posterior: testes no jogo ficam com o usuário;
  não iniciar/controlar novas sessões para testar sem novo pedido. Configs/saves reais conferidos por hashes, sem diferenças.
  Sessão de teste deixada aberta ao usuário; dados isolados em `build-win/language-qa-lcy_4o5w/`.
- FR/DE/ES são textos oficiais proprietários extraídos localmente; não são tradução comunitária como pt-BR.
  Proveniência identificada no manifesto de empacotamento; nenhuma ROM/arte de fonte foi embutida.
  Checkpoint FR/DE/ES versionado nesta branch; sem push, PR ou publicação. Italiano externo, texturas localizadas e os fallbacks de texto permanecem fora desta entrega.
  Linux/PortMaster e aceite individual DE/ES: `NOT_RUN`.

## Workspace e validação pós-merge (2026-11-06)

- PRs #2 (portátil), #3 (link do pacote de texturas BT Reloaded) e #4 (correção do J2 no splitscreen) mescladas com
  merge commit; `main` em `4a59160`. Branches mescladas removidas do GitHub; restam `main`, `feat/dev-mode` (os dois
  commits de música por ZIP, integrados ao PR #9, ainda sem merge) e a antiga `cursor/cloud-agent-1787466750256-ha9al`.
- Um só clone local no Windows (`TooieRecomp`). Os clones `TooieRecomp-win`, `TooieRecomp-xu10` e `TooieRecomp-audio`
  foram inventariados: nenhum tinha código fora da `main` além de `feat/dev-mode`; bundle git, diffs e arquivos ignorados
  ficaram num arquivo local fora do repositório.
- Windows em `4a59160`: Check, Verify (420 arquivos), Test 31/31 e Build passaram; worker DX12 abriu a janela do jogo
  por 25 s com `TOOIE_AUTO_START`, sem mudar config/saves do usuário.
- XU10 (ROCKNIX, Panfrost) em `4a59160`: build cruzado, pacote `dev-v0.1.5-7-g4a59160` (114 arquivos, manifesto
  conferido no aparelho), backup `/storage/tooie-before-4a59160.tgz`. 60 s em OpenGL ES 3.1 (Mesa 26.2.4) com o jogo
  iniciado, RSS 116-251 MB, sem erro no log; 1.609 arquivos do usuário iguais (14 saves/configs/ROM).
- Splitscreen do J2 na `main`: confirmado pelo usuário no Windows; no XU10 não testado (precisa de dois controles).

## Release

- Versão `0.1.6`; [pré-release Windows x64 e PortMaster publicada](https://github.com/wesleytakatsu/TooieRecomp/releases/tag/v0.1.6)
  em 2026-11-07. Tag anotada `v0.1.6` em `0f7230a2ee9a33c622eb17cbca1d808deecc2d05` (`main`, após as PRs #9 e #10).
  Notas em inglês e português brasileiro: [[releases/v0.1.6]]. Windows: 34/34 testes, build Release, auditoria dos 111
  arquivos aprovada. PortMaster: build cruzado (114 arquivos, glibc 2.35), não executado num aparelho nesta versão.
  Assets baixados do GitHub conferidos byte a byte com os locais. Linux x64 fica para o PC Linux do usuário.
- `BanjoTooieRecompiled-v0.1.6-Windows-x64.zip`: 23182600 bytes, SHA-256
  `48087dfff0aaa4b6a7e295df91796505c6472a6ac52472740a5199341aee7c39`.
- `BanjoTooieRecompiled-v0.1.6-Linux-arm64-PortMaster.zip`: 12223339 bytes, SHA-256
  `cc015b3cc5e8fce5940d26231727f7159ac40348129176fe3a24de72cec40ed2`. `BUILD-INFO.txt` dos dois pacotes: commit da tag,
  versão `0.1.6`, zero arquivos sujos.
- Versão fonte `0.1.5`; [pré-release Windows x64, Linux x64 e PortMaster publicada](https://github.com/wesleytakatsu/TooieRecomp/releases/tag/v0.1.5)
  em 2026-11-06, a partir de `feat/handheld` (PR #2) antes do merge, a pedido do usuário. Tag anotada `v0.1.5` aponta
  para `313e2871c71b8a496a7806dbb0c7bf80cffab663`, no histórico da `main` (PR #2 mesclada com merge commit, `05f04f8`).
  A v0.1.4 e a v0.1.5 têm a regressão do J2 no splitscreen da história (ver Pendências); a correção está só na `main`. Notas em inglês e português brasileiro: [[releases/v0.1.5]]. Windows: Check, Verify (420 arquivos), 31/31
  testes, build Release e auditoria dos 111 arquivos aprovados; exe do pacote abriu em DX12 por 25 s sem mudar a config
  do usuário. PortMaster: build cruzado no Windows, manifesto conferido, instalado no XU10 (Panfrost) com backup
  `/storage/tooie-before-313e287.tgz`; 60 s em OpenGL ES sem erro, 1.609 arquivos do usuário e 15 saves/configs iguais.
  Os quatro arquivos Windows/PortMaster baixados do GitHub foram conferidos; Linux x64 adicionado depois, da mesma tag, com hashes remotos verificados.
- `BanjoTooieRecompiled-v0.1.5-Windows-x64.zip`: 23084311 bytes, SHA-256
  `255daa10cac853b2e7b5a2b6ff3ef25ef59e8cd02adea0fd795f063d58a8502f`; executável
  `52c7ada19a3370790d9b5f31f11f2441b38fa914189ae4f896c16902ec222a9f`.
- `BanjoTooieRecompiled-v0.1.5-Linux-arm64-PortMaster.zip`: 12144570 bytes, SHA-256
  `2f5c5fdb552b31c2ffd03007e3127d3f384c14f49d5a65555a1b57a5b7e669d6`; binário aarch64
  `ef593051e9100e91a5fdf6335c321dee3efb2a06ca4fa7dd2894d40f2f4150fe`. `BUILD-INFO.txt` dos dois pacotes: commit da tag, versão `0.1.5`, zero arquivos sujos.
- Versão `0.1.4`; [pré-release Windows x64 publicada](https://github.com/wesleytakatsu/TooieRecomp/releases/tag/v0.1.4)
  em 2026-11-02, após o merge da PR #1. Tag `v0.1.4` aponta para `5621b25499675893174f38e68fe30a9cbfe88baa`.
  Notas em inglês e português brasileiro: [[releases/v0.1.4]]. Suíte local Windows 30/30, 420 arquivos gerados,
  integridade vendorizada, build Release e auditoria dos 111 arquivos do pacote aprovados. ZIP e `.sha256` publicados,
  baixados do GitHub e comparados byte a byte com os locais; pacote baixado revalidado. Sem novo playtest em jogo.
  `v0.1.3`, `v0.1.2` e `v0.1.1` continuam publicadas. Músicas/texturas privadas não integram a distribuição.
- ZIP `BanjoTooieRecompiled-v0.1.4-Windows-x64.zip`: 23056086 bytes, SHA-256
  `1e1e6f5cccc2ce38af7cd5356f3f5573a1660e62eb2f0edc1bf90f011943e9ec`.
  Executável: SHA-256 `17a757627fb80baab04cc34982cab2a0effbf764c6130b90a13664a297888646`.
  `BUILD-INFO.txt` confirma versão `0.1.4`, commit da tag e `tooie_tree_dirty_files: 0`.
  O manifesto de licenças mantém 37 entradas `UNKNOWN`/`UNVERIFIED`; auditoria valida cobertura/bytes, não resolve
  essas pendências de atribuição já documentadas. Sem pacote Linux novo.
- `v0.1.0-p08` permanece como release anterior, sem substituição dos seus arquivos. As notas publicadas dessa release
  registram pacotes Windows e Linux com idiomas; validação completa de gameplay no Linux continua pendente.
- Nomes de pacote: `BanjoTooieRecompiled-v<ver>-<OS>-x64`; cada arquivo tem `.sha256`.

## Portáteis (2026-11-05, base `ea5e31f`; na `main` pela PR #2)

- **Desktop não muda nas fases 1-2 (`PROVEN`):** `tools/handheld/check_desktop_unchanged.py` confirma 24 fontes idênticos à base com `TK64_MOBILE` desligado (a fase 3 muda o desktop de propósito). O `.exe` Windows
  deste código, comparado com o de `ea5e31f`, tem `.text` e `.rdata` idênticos; diferem 299 bytes, o carimbo PE e 37 identificadores de namespace anônimo derivados do caminho.
  Windows `-Stage Test`: 30/30. Linux x64: compilação, 19 testes host e início do guest passaram no checkpoint local abaixo; aceite visual completo continua pendente.
- **Pacote no XU10 com `libmali`, pasta separada, instalação nova:** ROM original convertida e jogo aberto na mesma execução, Demo 1 igual ao PC em 420 linhas, 3 demos iguais até o fim
  da gravação, 24,0 FPS com cache quente; sem ROM e com ROM errada o launcher avisa. Sem OOM do kernel. Com o cache vazio, o vigia de teste (limite 150 MB) fechou uma rodada a 149 MB; a outra rodada de 300 s chegou a
  168 MB, ambas com o EmulationStation em cerca de 320 MB.
- **Panfrost:** o usuário relatou em 2026-11-05 que testou e estava bom. A instalação nova com cache vazio no Panfrost não foi medida aqui (`NOT_RUN`); compilar os ubershaders a frio já travou o aparelho antes.
- **Instalado no XU10 (Panfrost), 2026-11-05:** `/storage/roms/ports/tooie` e `/storage/roms/ports/Banjo-Tooie.sh` vêm do zip `0643d950…` (binário `b4761415`), com saves, configurações e cache mantidos.
  Cópias anteriores: `TooieRecompiled.diag-25b67462` e `Banjo-Tooie.sh.before-pkg` na mesma pasta. A pasta de teste `tooie-pkgtest` e sua cópia da ROM foram apagadas.
- **Limites:** só XU10 com ROCKNIX; sem caminho KMSDRM no backend; `port.json` sem screenshot.
- **Causa `UNKNOWN`:** falta de memória na primeira rodada depois de trocar de driver, e o crescimento do EmulationStation (96 para mais de 320 MB).
- **Unificação (fase 1):** o trabalho do clone `TooieRecomp-xu10` entrou na `main`; o clone deixou de existir (ver Workspace). Patches `vendor/patches-local/rt64-tk64-mobile.patch` e
  `recompfrontend-weak-no-interface-msaa.patch` em `ORDER.tsv`, `UPSTREAMS.tsv` e no manifesto (`VENDOR VERIFY OK`, 50 patches). Próximas fases (menu de API por plataforma, melhorias do
  portátil no desktop, release e CI) entraram na mesma PR #2.
- **Presets (fase 2):** `CMakePresets.json` com `windows-x64`, `linux-x64` e `linux-arm64-handheld`; `Build-Tooie.ps1`, `build_linux.sh` e
  `tools/handheld/build-handheld.{ps1,sh}` configuram por eles. Ferramentas em `tools/handheld/` (antes `tools/xu10/`), chip em `devices/rk3326.cmake`
  (XU10, R36S). `build_linux.sh` com preset: o usuário testou no Linux x64 em 2026-11-06 e relatou que funciona (relato geral, sem detalhe por API). Build cruzado a partir de host Linux: não confirmado.
- **Menu de API (fase 3):** Settings -> Graphics -> Graphics API lista Auto e só as APIs compiladas para o alvo e aceitas pelo
  probe do aparelho (Windows DX12/Vulkan/DX11/OpenGL, Linux Vulkan/OpenGL, portátil OpenGL ES). Valor salvo de outra
  plataforma vira Auto; Linux Auto usa OpenGL sem dispositivo Vulkan. Launcher PortMaster não força mais a API. Windows
  31/31; XU10 (Panfrost) iniciou OpenGL ES sem variável e com `d3d12` salvo. Menu na tela do Windows: `PENDING_USER`; Linux: o usuário testou no Linux x64 em 2026-11-06 e relatou que funciona (relato geral, sem detalhe por API).
  Contrato: [[specs/Tooie/startup-and-backends]]. Conhecido, anterior a esta fase: `TOOIE_GRAPHICS_LOG` derruba os
  workers Windows (0xC0000409; um binário sem esta fase também cai), então `Diagnose-Graphics.ps1` não funciona hoje.
- **Ganhos do portátil no desktop (fase 4):** um só `plume_gles.cpp` (o do portátil; varredura de índices e flushes de
  Panfrost só em ES; MSAA segue desligado no GL desktop) e contagem de pixels alterados na CPU em todos os builds. Windows
  (AMD), 4 pares contra `36e2f4a`: CPU do processo DX12 18/16% -> 13/11% de um núcleo, OpenGL 67/84% -> 58/46%; 58-60 FPS
  no GL. XU10 (Panfrost) igual ao instalado. Perfil Weak não é escolhido no desktop (só ES), então MSAA da UI, res/AA e
  shaders especializados do Weak não mudam o desktop (`CORRECTED`: a estimativa anterior dizia que o OpenGL desktop usava
  Weak). Linux desktop: o usuário testou no Linux x64 em 2026-11-06 e relatou que funciona (relato geral, sem detalhe por API). Contrato: [[specs/Tooie/graphics-and-presentation]].
- **Release e CI (fase 5):** pacote PortMaster com nome de release `BanjoTooieRecompiled-v<ver>-Linux-arm64-PortMaster.zip`
  (`TOOIE_RELEASE_VERSION` ou `--release-version`). CI pública `public-handheld-cross` (sysroot Debian 12, build cruzado de `rt64` +
  testes puros, testes via `qemu-aarch64`) e job privado `private-game-build-handheld` (ROM, disparo manual, só checksums).
  `public-handheld-cross` passou nas PRs #2, #3 e #4; o job privado continua `NOT_RUN`. Os 9 testes também passaram no XU10. `rom_cart_import_test` não
  compila para aarch64 (falta `sse2neon.h` no include desse teste), fora da lista.

## Pendências conhecidas

- Minigames e cooperativo: implementada suspensão por sala para as 15 famílias/23 etapas, segmentos Saucer,
  aliases Mayan conservadores e salas MP relacionadas. Subtipo nativo `0x1B` e pedidos diferentes de um jogador
  permanecem nativos. Witchyworld `0xD6` só recusa promoção no retorno Saucer com seletor nativo 6.
  Preferência salva continua ligada; J2 volta no próximo carregamento elegível, sem criação/destruição no meio da sala.
  `tooie_story_coop` verifica proveniência de alocação e identidade; auxiliares não viram J2. Regressão desde
  `50bd9a2` (v0.1.4/v0.1.5) corrigida em 2026-11-06: o jogo trocou o ponteiro de J1 na tabela `0x80135490` sem
  teardown (`PROVEN` por log no mapa `0xAF`, `801F5DA0` -> `801F55C0`; compactação de heap `STRONG_INFERENCE`), o par
  caía e J2 perdia IA, itens, dano e atalhos. Par confirmado agora segue o slot quando o novo objeto mantém o `+0x184`
  da criação; retirada real e outro jogador no slot continuam fora. Regressão falhou antes e passou depois (9345
  verificações), suíte Windows 31/31; usuário confirmou J2 funcionando no Windows. Linux `NOT_RUN`. Atividade de construção
  e serialização de saída têm escopos próprios. Formas, iluminação, IA, Glowbo, menus, Warp e distância usam a
  identidade apropriada. Entrada host publicada pela porta nativa, com rearme após neutralização; controles nativos
  continuam intactos. Reservas/suavização analógicas são limpas na recriação/retirada. Sem novos logs/métricas no jogo.
  Windows: regressão cooperativa (9339 verificações na época; 9345 hoje) e helpers de integração passaram no checkpoint da implementação
  (`50bd9a2`); esses resultados não substituem o aceite dos eventos no jogo.
  Aceite parcial do usuário no Windows (2026-11-02): alguns eventos funcionaram, sem identificação individual.
  Limitação confirmada pelo usuário: na corrida de trenzinho com o pássaro, J2 continua acessível. Identidade
  formal/mapa/caminho nativo dessa corrida não foram capturados; não atribuir o relato a um dos 23 eventos.
  Usuário decidiu manter o comportamento atual. Correção dessa corrida adiada; não ampliar blacklist de mapas.
  Matriz manual completa, câmera/resultado do Saucer e Linux continuam `NOT_RUN`; não registrar aceite universal.
  Plano antigo retirado: detalhes nativos em [[reverse-engineering/systems/minigame-controllers]], critérios em
  [[specs/Tooie/testing-and-oracle#5.1 Minigame acceptance matrix]] e corrida adiada em [[future/implementation-ideas]].
  Contrato atual: [[specs/Tooie/window-ui-and-config]]; RE: [[reverse-engineering/systems/minigames]].
- Analog Camera: General disponibiliza On/Off (padrão Off), sensibilidade e inversão da câmera em terceira pessoa.
  Entrada proporcional/radial com suavização pelo tempo nativo; Z lógico restaura os bindings C do analógico
  imediatamente na leitura do controle. C físico/teclado e demais bindings preservados. Amostras por porta,
  movimento por jogador; resolve separadamente jogador, câmera nativa (oito slots) e controle. Não muda foco global.
  Integração nos filtros yaw/pitch dos controladores string/pan/pivot, antes da colisão nativa. Modos especiais,
  minigames em salas dedicadas, Blaster, primeira pessoa, câmeras bloqueadas/não suportadas e attract seguem nativos.
  Mira/zoom: guarda explícita `BA_FLAG_17_FIRST_PERSON_VIEW` libera os C do analógico por jogador.
  Revogação descarta amostra e suavização, inclusive sem update orbital durante a mira; reentrada usa amostra nova.
  Regressão falhou antes e passou depois: 107 verificações de câmera, incluindo J1/J2 com porta/câmera distintas;
  suíte Windows 30/30 e build passaram. Usuário confirmou o funcionamento do ajuste de mira/zoom no Windows
  (2026-11-02); aceite restrito aos casos testados, sem matriz exaustiva por jogador/mapa. Linux: `NOT_RUN`.
  Guarda atual do guest é refeita antes de mover; entrada host usa a última elegibilidade publicada por jogador.
  Sem novos logs/contadores no jogo. Regressões permanentes fora do executável; cobertura sintética não comprova
  sensação visual, colisão real ou todos os desafios/transformações. Usuário confirmou o funcionamento no Windows
  (2026-11-01); aceite limitado aos casos testados. Linux: `NOT_RUN`.
  Patch `recompfrontend-per-player-camera-input.patch`; verificação vendor: 8335 arquivos, 48 patches.
  Executável atualizado em `build-win/TooieRecompiled.exe`; agente não iniciou/controlou o jogo.
  Revisão final sem instrumentação temporária no caminho da câmera; removidos scripts locais de inspeção da ROM/
  código gerado e de preparação do patch/relatório. Regressões permanentes continuam fora do executável do jogo.
  Removida a supressão global antiga sem consumidores, seu setter guest não registrado e o estado `first_poll`
  sem uso. A leitura dos eixos deixa de consultar essa flag atômica; reserva por jogador/Z+C permanece funcional.
  Removidos também três observadores antigos da câmera sem chamadores, seus logs e onze variáveis/contadores órfãos.
  Contrato: [[specs/Tooie/input-and-controllers#3.3.1 Analog Camera]];
  RE: [[reverse-engineering/systems/camera-controls]].

- Persistência do modo de janela: `PROVEN` que F11/Alt+Enter aplicava o modo apenas em memória, sem gravar
  `graphics.json`. Atalho agora salva valores confirmados e aplica pelo callback, mantendo outras edições pendentes.
  Regressão com o frontend/librecomp reais falhou antes e passou depois: ambos os modos sobrevivem à recriação da
  config; menu Apply e isolamento de edições pendentes cobertos. Windows 19/19 e build passaram; Linux `NOT_RUN`.
  Patch: `vendor/patches-local/recompfrontend-persist-window-mode.patch`. Reabertura manual: `PENDING_USER`.

- Inicialização/TK64: integração standalone do TK64 `6476b4b` preserva patches atuais; APIs DX12/Vulkan/DX11/OpenGL.
  Supervisor Windows inicializa uma candidata por processo e mantém a instância que chegou ao launcher; timeout de
  180 s antes do handshake; falha/crash anterior ao menu tenta a próxima. Sem boot guest antes de Iniciar jogo.
  Seletor em Settings -> Graphics usa Apply/Cancel nativos e salva `graphics_backend` em `graphics.json` para a
  próxima abertura. Rótulos compactos Auto/DX12/Vulkan/DX11/OpenGL. Auto percorre cadeia completa DX12 -> Vulkan ->
  DX11 -> OpenGL até encontrar compatibilidade, respeitando regras de driver/Wine. Descrição indica API
  efetiva; nenhum seletor ou botão de reinício no launcher. Botão de seleção de música removido. Música de pausa usa MP3
  privado independente; processo guest oculto e IPC removidos. Diagnóstico opt-in: `tools/windows/Diagnose-Graphics.ps1`.
  Teste real do supervisor com renderers simulados passou; suíte Windows 20/20 e build passaram. Decoder/SDL dummy
  também testados. Nenhum jogo iniciado pelo agente. Abertura, quatro APIs, loop auditivo e gameplay: `PENDING_USER`;
  hardware antigo real e Linux `NOT_RUN`. Contrato: [[specs/Tooie/startup-and-backends]].
  Seletor de GPU (2026-11-07): opção GPU em Settings -> Graphics no Windows com 2+ GPUs; Auto tenta todas as APIs na
  GPU de alto desempenho (dedicada) antes da integrada; falhas registradas por API+GPU; exe exporta os sinais NVIDIA/AMD
  para OpenGL. Patch `rt64-host-gpu-selection`. No Windows real (GTX 1650 + AMD) o Auto renderizou na NVIDIA nas 4 APIs
  e a escolha AMD renderizou na AMD (DX12/Vulkan/DX11), medido por contadores de GPU por processo. Linux sem seletor.
  Erro final (2026-11-06): quando nenhuma API funciona, a janela diz isso em PT-BR (Windows em português) ou inglês,
  lista DirectX 12/Vulkan/DirectX 11/OpenGL com o motivo de cada uma (não encontrada, não iniciou, fechou, não
  respondeu) e orienta instalar o driver de vídeo e reiniciar; conferida no exe real. Sem ROM, o launcher abre normal
  com Load ROM (modo portátil com pasta vazia).
  Aquecimento oculto (2026-11-06): o worker desenha o launcher escondido e mudo por 60 frames e 1 s antes de ficar
  pronto; no Windows real o DX12 caiu sem a janela aparecer e o Vulkan abriu. Log opt-in grava cada linha numa escrita
  só (supervisor e worker não se misturam mais).
  Fallback após o launcher (2026-11-06): worker pronto fica em prova enquanto o launcher está aberto e por 15 s após
  Iniciar jogo; crash nesse período abre o launcher na próxima API, e o Auto passa a tentar por último as APIs que
  falharam (`startup.json` `failed_apis`). Crash depois da prova é propagado. Teste do supervisor com 2 rodadas novas
  passou; no Windows real (DX12 da GTX 1650 perde o device logo após o launcher), DX12 caiu e o Vulkan abriu, e a
  abertura seguinte foi direto ao Vulkan. Com `TOOIE_GRAPHICS_LOG`, que derruba os workers, o supervisor percorre as
  quatro APIs. Linux continua sem supervisor (`NOT_IMPLEMENTED`).
  Revisão removeu logger de frames/tempos TK64, print de hover, captura PCM do executável e varredura diagnóstica
  de pico/silêncio por buffer. Contador do teste de áudio fica apenas no harness; log opt-in da API foi preservado.
- Mods de música (PR #9 mesclada em `dded8a6`; correção do build do portátil na PR #10): suporte a `audio.json`
  com 32 canais, até 16 camadas por sequência (máscaras sobrepostas, pesos por mínimos quadrados não negativos),
  `bpm` por arquivo (a camada segue a posição em batidas do jogo e só substitui no andamento da área), loop próprio
  por camada, carregamento sob demanda para pacotes grandes (`TOOIE_AUDIO_MOD_CACHE_MB`, padrão 384) e volta à música
  original quando um MP3 sem loop acaba. Mapa completo do álbum HD em `tools/audio/banjo-tooie-hd.json` (193 faixas,
  198 regras: 13 sequências com camadas por área, músicas inteiras e jingles), gerado por gravação nativa
  (`TOOIE_TEST_AUDIO_RENDER`) e comparação com o álbum (`tools/audio/analysis/`); pacote montado com
  `tools/build_audio_pack.py` a partir do álbum do usuário (995 MB, nunca versionado nem publicado).
  Teste no jogo com o pacote (`TOOIE_TEST_AUDIO_RENDER_MOD=1`, Windows): todas as variantes de área e uma amostra de
  músicas e jingles tocam o MP3 previsto, alinhado (correlação 0,99, atraso ~0); jingles conferidos por comparação curta.
  Ficam no original: áreas sem faixa no álbum (MT `0xE108`, GGM `0x80600000`, Atlantis
  `0xE001`), GI debaixo d'água a 142/137 BPM (álbum só a 130) e Trash Can (álbum gravado a 125 BPM, jogo 116).
  Loops: 94 de 124 com emenda ≥0,95; Near Space Zone (0,53) e Drained Palace (0,82) pedem audição. Não testado:
  transições reais no jogo (entrar/sair da água, rampa de andamento), campanha completa, Linux e portátil.
  `tooie_audio_mod_test`, `tooie_bgm_hooks_test`, `tooie_bgm_volume_test` na suíte obrigatória (34/34). MP3/ZIP
  continuam privados: o release publica só o suporte; o link do pacote é colocado pelo usuário.

- Validação no Linux (feita pelo usuário no PC Linux; nunca WSL) de widescreen, interpolação, desempenho, multijogador e
  idiomas: `NOT_RUN`. Depois dela, gerar e publicar pacotes Linux novos na release.
- `tools/build_linux.sh test` ainda não roda os 5 testes de idioma, o teste ZIP nem o de cheats (só o Windows os executa).
- Paredes do intro em 16:9 (`T_INTRO`) e ultrawide (> 16:9): resíduo separado, não tratado.
- Cortes laterais no widescreen: usuário aprovou a correção no Windows (2026-11-01), nos casos de coletáveis,
  inimigos e sombras em Jinjo Village, inclusive cortes perto/acima do meio. `PROVEN`: RSP enviava triângulos
  das faixas extras, mas descartava sua extensão Y ao calcular regiões/dimensões do framebuffer.
  Expand/Manual >4:3 preservam esses limites; índices, scissor de desenho, câmera, regras de profundidade
  e culling permanecem iguais. Original/Manual <=4:3 e ortográfico preservados. Chave de desligamento:
  `TOOIE_TEST_WS_DRAW_BOUNDS=0`. Patch `rt64-widescreen-draw-bounds.patch` com proveniência/manifest.
  Correção independente do estouro P/V preservada: recargas usam 40 bytes nos bancos nativos de 48.
  Removidos capturador temporário, hooks/F8, script de coleta, fixture diagnóstica, logger/contadores de
  escala P/V e print de registro de hooks. As duas regressões permanentes não integram o executável do jogo.
  Windows após limpeza: suíte 27/27, build, três helpers de integração e regressão com a chave desligada passaram.
  Confirmada ausência das strings do diagnóstico removido no executável e de suas referências/target.
  Vendor: 8335 arquivos, 47 patches no reverse-check. Outros mapas/backends e Linux não validados.
  RE: [[reverse-engineering/systems/view-volume#Lateral framebuffer bounds after triangle submission]].
- Distância de objetos: Graphics -> Object Distance, Original/2x/4x/Unlimited, padrão Original, persistida
  em `graphics.json`; Load/Apply publica o valor, Cancel preserva a política aplicada. História (modo 3),
  fora do multijogador nativo; attract segue nativo. Usuário aprovou a revisão no Windows (2026-11-01), após
  retestar os problemas relatados de inimigos/coletáveis e placa; aceite restrito aos casos testados.
  Política coordena limite do modelo, atualização/retenção do ator, seleção de células e corte radial anterior
  à fila, além do limite próprio de sprites. Distância física, FOV/projeção, AABB/setores/flags e callbacks
  permanecem nativos. Fades de 700 unidades (modelos) e 500 (sprites); sem cache/fade temporal por omissão de desenho.
  Jogador/UI global/modos excluídos mantêm limites nativos. No split, atualização/retenção ampliada considera
  o jogador vivo mais próximo, sem mudar foco. Cobertura/performance universal e Linux permanecem `UNKNOWN` / `NOT_RUN`.
  Logger, captura CSV, observadores de ator/conversão, script temporário e suas chamadas foram removidos;
  regressão isolada permanece fora do executável. Limpeza validada: Windows 25/25 testes, regressão 382
  verificações, três helpers de integração e build passaram; executável atualizado sem diagnósticos da opção.
  Contrato: [[specs/Tooie/graphics-and-presentation#3.1.1 Object distance]].
  RE: [[reverse-engineering/systems/model-renderer#Coordinated distance policy (`PROVEN` fixture; Windows user-validated)]].
- Cenas esticadas em 1 jogador: usuário relata na explicação de habilidade do Jamjars, com retorno normal depois.
  `PROVEN` no harness: gate antigo aplicava compensação de `T_INTRO` a mapas comuns no modo 3; regressão falhou
  antes e passou após exigir modo 3 **e** mapa 395. Cenas comuns mantêm projeção nativa + ajuste normal RT64.
  Auto/Expand continua seguindo dimensões atuais da janela, inclusive 4:3; nenhuma proporção 16:9 é imposta.
  Regressão compila política/cálculo de janela reais; serviços host e matrizes são simulados. Cobre Original/Expand,
  4:3, 5:4, retrato, 16:10, 16:9, ultrawide, tamanhos irregulares/inválidos e transições, preservando a fórmula do intro.
  Windows: regressão 1200/1200, suíte 24/24 e build passaram. Sem novos logs/métricas no jogo.
  Usuário aprovou a correção visual no Windows (2026-11-01), restrita ao caso relatado; matriz visual exaustiva de
  proporções/intro não confirmada. `STRONG_INFERENCE` como causa; valores da cena ao vivo `UNKNOWN`. Linux `NOT_RUN`.
  RE: [[reverse-engineering/systems/view-volume#Cinematic projection compensation scope]].
- Pular o matching de frames do RT64 com Interpolation OFF: `REFUTED` (processo encerrou sem log na troca de cena do
  attract); causa `UNKNOWN`.
- Captura burst `TOOIE_TEST_INTERP_BURST_AT_WORKLOAD` encerrou o processo no Windows/D3D12 (1×, `UNKNOWN`).
- Wrappers de diagnóstico `ud4`/`ud5` não-macro ainda registrados (custo baixo; logs desligados por padrão).
- **Splitscreen experimental**: dois jogadores em tela dividida no Dev Mode
  (J2 controlável, pega itens e ataca; uso pai/filho aprovado pelo usuário). IA J1/J2 aprovada no caso relatado; portas pendentes.
  Iluminação: vértices de saída próprios por modelo de jogador; A/B instrumentado no attract Windows eliminou alias
  simultâneo e sobrescrita RGB. `PROVEN` no teste visual do usuário no Windows (2026-10-31): problema gráfico resolvido;
  captura manual terminou com exit 0. Linux e playtest específico de transformações/modelo secundário `NOT_RUN`.
  Diagnósticos temporários por desenho removidos; captura leve opt-in disponível em
  `tools/windows/Test-SplitScreenLighting.ps1` (backup + logs). Contrato: [[specs/Tooie/window-ui-and-config]];
  RE: [[reverse-engineering/systems/player-lighting]].
- IA no splitscreen: primeiro build não resolveu os inimigos relatados. Leitura externa, sem alterar processo,
  confirmou Ground/Fly na Jinjo Village (322), J1/J2 vivos e foco 0; estavam fora da lista original. Revisão cobre
  oito famílias: `ch2dbaddy`, Crystal, Flower, New Shovel, Seaweed, Hillbilly, Digger Ground/Fly.
  Alvo por ator, percepção nativa e arbitragem `suchase` separadas por jogador; não escreve foco global. Gate nativo
  dessas classes considera J2 próximo sem ignorar flags de atividade. Reinstala callbacks físicos/canônicos por
  identidade de overlay; Fly entra pelo dispatcher do controlador compartilhado. `suchase` entrypoint 1 chamava 0
  internamente e escapava do isolamento; regressão falhou antes e passou com escopo nos dois entrypoints. Criação de
  jogadores limpa estado por mapa. `PROVEN` no harness: update da pá, callback/controller Ground e update/predicado
  Fly selecionam J2; oito rotas, escopos/cache, retenção/histerese, máscara, guards e alcance passam. Serviços de
  animação/movimento/mundo/jogador e cinco outros updates simulados. Usuário confirmou funcionamento da correção
  revisada no Windows (2026-10-31); aceite restrito ao caso relatado, sem bateria exaustiva de famílias/mapas.
  Cowboy=Hillbilly: `STRONG_INFERENCE`, sem amostra ao vivo na mina.
  Suíte Windows 22/22 e build passaram; outros inimigos/bosses não cobertos; Linux `NOT_RUN`. Sem logs/métricas
  novos no jogo; scripts, fixture e logs temporários da investigação removidos.
  RE: [[reverse-engineering/systems/warp-and-player-position]]; roteiro: [[specs/Tooie/testing-and-oracle#5. Experimental split-screen manual coverage]].
- Menus de viagem no splitscreen: usuário confirma que J1 abre Warp Pads/Silos e seleciona destinos, mas a lista
  fica invisível. Deve continuar exclusivo de J1. `PROVEN` estaticamente: ambos desenham via `gcnewoption` no pass `3`,
  global após as viewports; filas compartilhadas são refeitas por câmera. Usuário testou Silo com J2 junto de J1 e
  o menu apareceu. Regressão nativa reproduziu perda após limpar fila; correção retém apenas menus ativos dessas
  classes da coleta de J1 e executa os callbacks ausentes no pass global, sem duplicar os presentes na fila de J2.
  Guard exige Dev Mode desbloqueado/opção ligada, história, dois jogadores/viewports; desligado segue nativo.
  Sem alterar foco, culling de geometria ou controles de viagem de J1; sem logs/métricas no jogo. Windows 23/23,
  três helpers e build passaram; aparência do build corrigido `PENDING_USER`; gate exato da sessão original ainda `UNKNOWN`.
  RE: [[reverse-engineering/systems/warp-and-player-position#Warp Pad and Silo destination menu drawing]].
- Glowbo no splitscreen: usuário relatou coleta repetida pelo J2 afastado; J1 coleta corretamente. `PROVEN` no
  harness nativo isolado que crédito imediato e cleanup adiado podem se separar quando o gate de atualização usa
  a posição de J1. Causa da sessão: `STRONG_INFERENCE`, sem captura dos flags/mapa reais. Correção implementada no
  evento residente: callback original credita; estado coletado termina limpeza nativa uma vez, inclusive fora de
  alcance/setor de J1; contatos repetidos não creditam novamente. Restrita à classe Glowbo e splitscreen experimental,
  sem logs/contadores no jogo. Usuário confirmou a correção no Windows (2026-10-31); chave temporária de A/B removida.
  Regressão nativa e suíte Windows 21/21 passaram. Mega Glowbo/JRL específicos e Linux `NOT_RUN`.
  RE: [[reverse-engineering/systems/warp-and-player-position]]; teste/roteiro: [[specs/Tooie/testing-and-oracle#5. Experimental split-screen manual coverage]].
- Seletor de transformações por jogador implementado para teste: Dev Mode desbloqueado, seção **Character selection**
  com chave, dropdown de direção e botões **Open for Player 1/2**; sem exigir recarga de mapa após desbloquear.
  R3 físico ou Debug Button + direção configurada (baixo por padrão), direcional/analógico para selecionar, A confirma, B cancela.
  Revisão visual: linhas com nome/descrição e dropdowns alinhados, botões dos jogadores com larguras iguais, conteúdo
  com rolagem vertical. Direção persistida em `character_menu_direction`; JSON antigo mantém padrão baixo.
  Usuário aprovou visualmente a nova organização na captura de 2026-10-31; quatro direções e prioridade têm testes host.
  Build final Windows e 18/18 testes host passaram; rolagem compilada, inspeção em janelas menores `NOT_RUN`.
  A inspeção automática foi interrompida por Escape; nenhuma outra entrada automática foi feita na interface. Overlay por viewport, entrada e pedidos
  independentes, 17 entradas (Clockwork removido após crash relatado; Banjo/Dragon Kazooie em segundo). Handoff nativo preservado por jogador entre
  cenários; rotinas de alcance/reversão da Humba não foram alteradas. Build Windows e harness/testes host validados;
  usuário confirmou que a seleção funciona; a legenda sobreposta foi separada da lista com linhas de dimensão fixa.
  Ordem: Kazooie → Dragon Kazooie → Mumbo → Snowball. Golden Goliath recebe 120 s na entrada pelo menu, corrigindo
  a duração zero relatada; contagem/expiração continuam nativas. Usuário aprovou a revisão no Windows (2026-10-31).
  Persistência/alcance e expiração completa ainda aguardam teste específico; dois Goliaths compartilham o timer nativo.
  RE: [[reverse-engineering/systems/player-transformations]].
- Dragão no J2: usuário relatou que Banjo/Dragon Kazooie mantém aparência normal com dois jogadores. Investigação
  estática encontrou retorno antecipado na recarga quando o modelo base não muda; o teste host anterior cobria o getter
  isolado, substituindo a recarga nativa. Usuário confirmou que Bee → Banjo/Dragon Kazooie funciona no J2: trocar o modelo base
  permite exibir Dragão. Correção implementada: quando a seleção mantém modelo base `0x607`, atualiza o auxiliar
  pelo seletor nativo no escopo do próprio jogador. Regressão com cache/seletor nativos reais falha sem a correção e
  passa com ela; assets e instalação são simulados. Build Windows e 18/18 testes passaram; usuário aprovou a correção
  no Windows (2026-10-31). Isso não substitui a bateria exaustiva de formas/cenários.
  Ver [[reverse-engineering/systems/player-transformations]].
- Atalhos Fly/Blaster no J2 validados pelo usuário no Windows (2026-10-31): entrada, histórico, pedidos e recarga do Blaster separados por
  jogador; opções/desbloqueio compartilhados. R3 físico ou Debug Button + direção configurada atua no próprio personagem.
  Mantém regras originais de forma/estado e prioridade do seletor; mapa/sessão descartam ações e recargas antigas.
  Executor real com memória guest e funções nativas simuladas passou; build Windows e 18/18 testes passaram.
  Aprovação em jogo não substitui a bateria exaustiva de combinações/cenários; Linux `NOT_RUN`.
  Ver [[reverse-engineering/systems/warp-and-player-position]].
- Cogitações não implementadas centralizadas em [[future/implementation-ideas]]: cooperativo com dois processos
  e extensão da suspensão para a corrida adiada. Testes manuais restantes não são propostas de implementação.
- Manutenção (2026-11-02): planos antigos retirados após consolidação de fatos/contratos/aceite; removidos wrapper
  sem chamadores, três constantes sem consumidores, seis contadores órfãos e cálculo local sem leitura.
  Auditoria do compilador em 45 unidades C++ de produção; dependências/gerados preservados. Diagnósticos com
  consumidores mantidos. Não há alteração de comportamento pretendida ou novo teste/log vinculado ao jogo.
  Suíte Windows 30/30 e build aprovados; executável atualizado em `build-win/TooieRecompiled.exe`.
  Comandos, hashes, auditoria de 45 unidades C++ e limites da validação ficam na mensagem do commit.
  Jogo não iniciado/controlado; Linux `NOT_RUN`. Publicação posterior registrada na seção Release.

## Empacotamento Linux x64

- Entrada `./build-linux.sh`: seleção tar/deb/AppImage/todos, análise de dependências e build pelo preset `linux-x64`.
  Scripts, licença do runtime AppImage e instruções de instalação/provisionamento versionados; launcher usa `build-linux`.
- `PROVEN` em `313e2871c71b8a496a7806dbb0c7bf80cffab663` (tag `v0.1.5`, 2026-11-06): build Release com
  `TOOIE_RELEASE_VERSION=0.1.5 ./build-linux.sh all`, exit `0`, geração de 420 arquivos conferida e 19/19 suítes PASS.
  Tar/deb/AppImage têm `BUILD-INFO.txt` da tag, versão `0.1.5` e zero arquivos sujos; manifests, licença e checksums
  aprovados. Binário comum: `8d02707238e5af6486e8431e5dfebeddc8125cf411dc9c8019c025e6149bda51` (SHA-256).
- Três pacotes `BanjoTooieRecompiled-v0.1.5-Linux-x64.*` e três `.sha256` publicados na release `v0.1.5`.
  Os seis hashes SHA-256 remotos coincidem com os locais; anexos Windows/PortMaster, título e estado de pré-release
  preservados. Notas em inglês/português sincronizadas em [[releases/v0.1.5]]. Artefatos: `dist/release-v0.1.5-linux/`.
- Execução isolada pelas entradas dos três pacotes: launcher 15 s e ROM 25 s em cada um, com 2.236 assets pt-BR e
  quadro interpolado registrados. `.deb` apenas extraído; AppImage testado com `--appimage-extract-and-run`.
  HOME/configs/caches/ROMs temporários removidos; estado do dpkg e dados reais do usuário permaneceram iguais.
  Não substitui gameplay completo por API, instalação Debian real nem montagem FUSE padrão.

## Notas operacionais

- Windows: `tools\windows\Build-Tooie.ps1 -Stage Test` **não** atualiza `build-win\TooieRecompiled.exe`; recompile
  `build-win` com ninja antes de medir ou entregar build para teste.
- Pacote de release do Windows: `Build-Tooie.ps1 -Stage Package -ReleaseVersion <ver>` a partir de um worktree limpo
  (o `BUILD-INFO.txt` registra commit e arquivos sujos).
- Testes no jogo: nunca escolha "Save and Quit"/"SALVAR E SAIR" (grava o save antes de perguntar); faça backup de saves e
  configs antes.
- Medições úteis (default OFF): `TOOIE_DIAG_P09_ROD_CLASSIFY=<arquivo>`, `TOOIE_DIAG_WS_CULL_CENSUS=<arquivo>`,
  `TOOIE_DIAG_PRESENT_LOG=1`. Attract automático: `TOOIE_AUTO_START=1`; interpolação forçada:
  `TOOIE_TEST_INTERPOLATION_FPS=60`; pacote de idioma forçado: `TOOIE_TEST_LANGUAGE_PACK=<arquivo>`.
- Mudanças em código vendorizado (`lib/`, `decomp/`) viram `vendor/patches-local/*.patch` + linhas em
  `vendor/patches-applied/ORDER.tsv`, `vendor/UPSTREAMS.tsv` e `vendor/MANIFEST.sha256`; conferir com
  `LC_ALL=C tools/vendor_verify.sh --reverse-check` (no Windows, não rodar `tools/vendor_manifest.sh`).
- Texto pt-BR que não cabe no layout USA: corrigir em `PTBR_TEXT_FIXES` (`tools/build_language_packs.py`), regenerar o
  bundle e atualizar a linha dele em `vendor/MANIFEST.sha256`.

## Continuidade da release v0.1.4

Build e publicação autorizados pelo usuário concluídos; notas finais em [[releases/v0.1.4]]. O rascunho
[RELEASE-NOTES-PENDENTES.md](../RELEASE-NOTES-PENDENTES.md) fica arquivado na raiz, com aviso de consolidação.
O mod de músicas da branch de áudio continua fora desta entrega. O CI Windows da PR falhou na escrita em `/tmp`
do teste `p09_local_pair_snapshot_test`; esse teste passou na suíte local, onde o diretório existe. A causa ambiental
é `STRONG_INFERENCE`; não houve alteração desse teste ou do mecanismo de publicação de snapshots nesta release.
A corrida de trenzinho com o pássaro permanece adiada; não retomar a implementação sem pedido do usuário.

`NEXT_SINGLE_ACTION`: analisar o patch italiano de Rulesless e integrar os recursos compatíveis em bundle próprio; testes no jogo ficam com o usuário.
