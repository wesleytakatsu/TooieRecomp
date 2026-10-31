---
title: "Handoff — TooieRecomp"
description: "Estado atual do TooieRecomp por área: build Linux/Windows, runtime, render, idiomas, release, pendências e próxima ação."
status: current
type: handoff
project: TooieRecomp
updated: 2026-10-31
tags: [tooie-recomp, handoff, status]
---
# Handoff — TooieRecomp

Estado corrente para um agente ou chat novo. Investigações antigas, planos concluídos (multijogador, idiomas) e
evidências ficam no histórico do git (`git log -- <caminho>`); propostas futuras e o checkpoint experimental de tela
dividida estão em `docs/future/`. Contratos estáveis:
[[specs/index]]. Conhecimento do jogo: [[reverse-engineering/README]].

## Estado por área

| Área | Estado | Referência |
| --- | --- | --- |
| Build standalone (vendorizado, kit-free) | Linux e Windows 11 x64 nativo; `VENDOR VERIFY OK` (42 patches) | [[specs/Tooie/build-and-generator]], [[specs/Tooie/packaging-and-dependencies]] |
| ROM / extração | ROM canônica `Banjo-Tooie (USA).z64` (privada, git-ignored) → imagem descomprimida exigida pelo launcher | [[specs/Tooie/asset-and-licensing-manifest]] |
| Runtime e overlays | dispatch de syscalls de overlay, `ovl_load`/`ovl_unload` host-aware; dedupe de `loaded_sections`; callback `on_rom_loaded` no librecomp | [[specs/Tooie/runtime-and-overlays]] |
| Áudio | `n_aspMain` + fila de saída corrigida; BGM controlável | [[specs/Tooie/audio-system]] |
| Render 4:3 / widescreen | 16:9 por planos de culling nativos (produção) | [[specs/Tooie/graphics-and-presentation]], [[reverse-engineering/systems/view-volume]] |
| Interpolação (60/120 FPS) | pareamento ordenado + guardas P09 + spike guard elementwise (produção) | [[specs/Tooie/graphics-and-presentation]], [[reverse-engineering/systems/transform-interpolation]] |
| Desempenho RT64 (cenas com ~1000 desenhos) | check sets do pareamento por chaves distintas e cache do hash de TMEM (patches `rt64-perf-*`, resultado idêntico); ponto da Glitter Gulch Mine de 9-12 para 60 FPS no Windows D3D12; o usuário jogou e validou no Windows (2026-10-30); Linux `NOT_RUN` | [[specs/Tooie/graphics-and-presentation]] |
| Entrada / UI / config | SDL2, multijogador local até 4 (opção default Off), menu ESC, FPS overlay, botão GitHub | [[specs/Tooie/input-and-controllers]], [[specs/Tooie/window-ui-and-config]] |
| Idiomas do jogo | EN, pt-BR (embutido, `assets/languages/pt-BR.tlpb`), FR/DE/ES (importados no jogo da ROM europeia do jogador); validado no Windows | [[specs/Tooie/language-packs]], [[reverse-engineering/systems/text-and-fonts]] |
| Modding | `.rtz` e ZIP original com raiz interna RT64; cópia manual, instalação pela interface e ativação/desativação de BT Reloaded 0.1.1 validadas pelo usuário no Windows (2026-10-30); Linux e reordenação `NOT_RUN`; resto da API herdado e não validado | [[specs/Tooie/modding-and-extensions]] |
| Dev Mode (menu ESC) | aba com aviso a aceitar; `Warp` salva, renomeia, exclui e teleporta para posições (`dev_warps.json`), inclusive entre mapas pelo `gcgoto` do jogo (posiciona 2 s depois de a fase nova carregar); validado nos demos e com save real no Windows pelo usuário; salvar com o jogo pausado (Start) `NOT_RUN`. `Cheats`: Debug Button (entrada nova dos controles, padrão R3/T) + direção do D-pad escolhida por cheat (`dev_cheats.json`); cheats `Fly` (estado de decolagem `0x23` do pad de voo) e `Breegull Blaster` (liga com `0xC8`, desliga com `0xC9` e recarrega o modelo), validados pelo usuário no Windows (2026-10-30); o Warp não anda com o Blaster ligado | [[specs/Tooie/window-ui-and-config]], [[reverse-engineering/systems/warp-and-player-position]] |
| Testes | Windows `Build-Tooie.ps1 -Stage Test`: 19 obrigatórios (inclui 5 de idioma, `tooie_dev_forms_test` e regressão Win32 `rt64_hidden_window_test`); Linux `build_linux.sh test`: 10 configurados; execução Linux desta mudança `NOT_RUN` | [[specs/Tooie/testing-and-oracle]] |

## Release

- Versão fonte `0.1.3`; pré-release `v0.1.3` para Windows x64 com o cheat Breegull Blaster do Dev Mode, a partir da
  branch `feat/dev-mode`. `v0.1.2` (Debug Button e cheat Fly) e `v0.1.1` (Dev Mode/Warp, otimizações RT64 e ZIPs de
  texturas) continuam publicadas. O pacote de texturas do usuário não integra a distribuição.
- `v0.1.0-p08` permanece como release anterior, sem substituição dos seus arquivos. As notas publicadas dessa release
  registram pacotes Windows e Linux com idiomas; validação completa de gameplay no Linux continua pendente.
- Nomes de pacote: `BanjoTooieRecompiled-v<ver>-<OS>-x64`; cada arquivo tem `.sha256`.

## Pendências conhecidas

- Persistência do modo de janela: `PROVEN` que F11/Alt+Enter aplicava o modo apenas em memória, sem gravar
  `graphics.json`. Atalho agora salva valores confirmados e aplica pelo callback, mantendo outras edições pendentes.
  Regressão com o frontend/librecomp reais falhou antes e passou depois: ambos os modos sobrevivem à recriação da
  config; menu Apply e isolamento de edições pendentes cobertos. Windows 19/19 e build passaram; Linux `NOT_RUN`.
  Patch: `vendor/patches-local/recompfrontend-persist-window-mode.patch`. Reabertura manual: `PENDING_USER`.

- Inicialização no Windows: prévia de música do launcher usa processo com janela oculta; RT64 acrescentava
  `WS_VISIBLE` ao aplicar fullscreen e a expunha como uma segunda janela de jogo. Correção mantém a visibilidade
  anterior e evita ativação de janelas ocultas, sem forçar um modo. Regressão executa a
  rotina RT64/Win32 real num desktop isolado: falha antes (`fullscreen changed window visibility`), passa depois
  para janela oculta e visível, inclusive restauração dos limites. Nova abertura completa: `PENDING_USER`.
  Patch: `vendor/patches-local/rt64-hidden-window-visibility.patch`; nenhuma partida iniciada pelo agente nesta correção.

- Validação no Linux (feita pelo usuário no PC Linux; nunca WSL) de widescreen, interpolação, desempenho, multijogador e
  idiomas: `NOT_RUN`. Depois dela, gerar e publicar pacotes Linux novos na release.
- `tools/build_linux.sh test` ainda não roda os 5 testes de idioma, o teste ZIP nem o de cheats (só o Windows os executa).
- `.deb` e AppImage foram gerados à mão no Linux; não há script versionado para eles.
- Paredes do intro em 16:9 (`T_INTRO`) e ultrawide (> 16:9): resíduo separado, não tratado.
- Pular o matching de frames do RT64 com Interpolation OFF: `REFUTED` (processo encerrou sem log na troca de cena do
  attract); causa `UNKNOWN`.
- Captura burst `TOOIE_TEST_INTERP_BURST_AT_WORKLOAD` encerrou o processo no Windows/D3D12 (1×, `UNKNOWN`).
- Wrappers de diagnóstico `ud4`/`ud5` não-macro ainda registrados (custo baixo; logs desligados por padrão).
- **Checkpoint experimental**, branch `codex/two-player-split-screen`: dois jogadores em tela dividida no Dev Mode
  (J2 controlável, pega itens e ataca; uso pai/filho aprovado pelo usuário). Faltam inimigos mirarem o J2 e portas.
  Iluminação: vértices de saída próprios por modelo de jogador; A/B instrumentado no attract Windows eliminou alias
  simultâneo e sobrescrita RGB. `PROVEN` no teste visual do usuário no Windows (2026-10-31): problema gráfico resolvido;
  captura manual terminou com exit 0. Linux e playtest específico de transformações/modelo secundário `NOT_RUN`.
  Diagnósticos temporários por desenho removidos; captura leve opt-in disponível em
  `tools/windows/Test-SplitScreenLighting.ps1` (backup + logs). Handoff: [[future/two-player-split-screen]];
  RE: [[reverse-engineering/systems/player-lighting]].
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
- Plano futuro (proposto, não implementado): cooperativo com dois processos, cada um com um jogador, vendo-se no mesmo
  mapa e compartilhando progresso. Ver [[future/two-process-coop]].

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

`NEXT_SINGLE_ACTION`: usuário abrir o build Windows, alternar modo com F11, fechar e reabrir para confirmar a persistência.
