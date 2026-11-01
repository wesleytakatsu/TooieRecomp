---
title: "Handoff — TooieRecomp"
description: "Estado atual do TooieRecomp por área: build Linux/Windows, runtime, render, idiomas, release, pendências e próxima ação."
status: current
type: handoff
project: TooieRecomp
updated: 2026-11-01
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
| Build standalone (vendorizado, kit-free) | Linux e Windows 11 x64 nativo; `VENDOR VERIFY OK` (48 patches) | [[specs/Tooie/build-and-generator]], [[specs/Tooie/packaging-and-dependencies]] |
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
| Testes | Windows `Build-Tooie.ps1 -Stage Test`: 29 obrigatórios (inclui câmera analógica/entrada SDL real, bounds do RSP, bancos de recarga P/V, distância de objetos, 5 de idioma, regressões de projeção/Glowbo/IA/menu de viagem e Win32 `rt64_hidden_window_test`); Linux `build_linux.sh test`: 17 configurados; execução Linux desta mudança `NOT_RUN` | [[specs/Tooie/testing-and-oracle]] |

## Release

- Versão fonte `0.1.3`; pré-release `v0.1.3` para Windows x64 com o cheat Breegull Blaster do Dev Mode, a partir da
  branch `feat/dev-mode`. `v0.1.2` (Debug Button e cheat Fly) e `v0.1.1` (Dev Mode/Warp, otimizações RT64 e ZIPs de
  texturas) continuam publicadas. O pacote de texturas do usuário não integra a distribuição.
- `v0.1.0-p08` permanece como release anterior, sem substituição dos seus arquivos. As notas publicadas dessa release
  registram pacotes Windows e Linux com idiomas; validação completa de gameplay no Linux continua pendente.
- Nomes de pacote: `BanjoTooieRecompiled-v<ver>-<OS>-x64`; cada arquivo tem `.sha256`.

## Pendências conhecidas

- Analog Camera: General disponibiliza On/Off (padrão Off), sensibilidade e inversão da câmera em terceira pessoa.
  Entrada proporcional/radial com suavização pelo tempo nativo; Z lógico restaura os bindings C do analógico
  imediatamente na leitura do controle. C físico/teclado e demais bindings preservados. Amostras por porta,
  movimento por jogador; resolve separadamente jogador, câmera nativa (oito slots) e controle. Não muda foco global.
  Integração nos filtros yaw/pitch dos controladores string/pan/pivot, antes da colisão nativa. Modos especiais,
  minigames em salas dedicadas, Blaster, primeira pessoa, câmeras bloqueadas/não suportadas e attract seguem nativos.
  Guarda atual do guest é refeita antes de mover; entrada host usa a última elegibilidade publicada por jogador.
  Sem novos logs/contadores no jogo. Regressões permanentes fora do executável; cobertura sintética não comprova
  sensação visual, colisão real ou todos os desafios/transformações. Usuário confirmou o funcionamento no Windows
  (2026-11-01); aceite limitado aos casos testados. Linux: `NOT_RUN`.
  Patch `recompfrontend-per-player-camera-input.patch`; verificação vendor: 8335 arquivos, 48 patches.
  Windows: suíte 29/29, regressão de câmera 77 verificações, três helpers de integração e build passaram.
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
  Revisão removeu logger de frames/tempos TK64, print de hover, captura PCM do executável e varredura diagnóstica
  de pico/silêncio por buffer. Contador do teste de áudio fica apenas no harness; log opt-in da API foi preservado.

- Validação no Linux (feita pelo usuário no PC Linux; nunca WSL) de widescreen, interpolação, desempenho, multijogador e
  idiomas: `NOT_RUN`. Depois dela, gerar e publicar pacotes Linux novos na release.
- `tools/build_linux.sh test` ainda não roda os 5 testes de idioma, o teste ZIP nem o de cheats (só o Windows os executa).
- `.deb` e AppImage foram gerados à mão no Linux; não há script versionado para eles.
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
- **Checkpoint experimental**, branch `codex/two-player-split-screen`: dois jogadores em tela dividida no Dev Mode
  (J2 controlável, pega itens e ataca; uso pai/filho aprovado pelo usuário). IA J1/J2 aprovada no caso relatado; portas pendentes.
  Iluminação: vértices de saída próprios por modelo de jogador; A/B instrumentado no attract Windows eliminou alias
  simultâneo e sobrescrita RGB. `PROVEN` no teste visual do usuário no Windows (2026-10-31): problema gráfico resolvido;
  captura manual terminou com exit 0. Linux e playtest específico de transformações/modelo secundário `NOT_RUN`.
  Diagnósticos temporários por desenho removidos; captura leve opt-in disponível em
  `tools/windows/Test-SplitScreenLighting.ps1` (backup + logs). Handoff: [[future/two-player-split-screen]];
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
  RE: [[reverse-engineering/systems/warp-and-player-position]]; roteiro: [[future/two-player-split-screen]] seção 13.
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
  RE: [[reverse-engineering/systems/warp-and-player-position]]; teste/roteiro: [[future/two-player-split-screen]] seção 12.
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

`NEXT_SINGLE_ACTION`: validar Analog Camera no Linux nativo.
