---
title: "BT P08 — RT64, entrada e frontend"
description: "Completa renderização, janela, controles e experiência de inicialização no Linux."
status: active
type: implementation-stage
project: TooieRecomp
phase: P08
order: 8
depends_on: P07
updated: 2026-10-22
tags: [banjo-tooie-Recomp, rt64, input, frontend, linux, attract, demo1, demo7, drift, fire-egg, movement-parity, menu, settings, config, forced-refresh, resolution, aspect-ratio]
---

# BT P08 — RT64, entrada e frontend

## CURRENT — WIDESCREEN_EDGE_GEOMETRY: planos nativos 16:9 (2026-10-22)

Ver [[../../plan/evidence/p08-widescreen-native-planes-20261022]].

Resíduo de geometria de bordas em widescreen (`WIDESCREEN_EDGE_GEOMETRY`) corrigido no gameplay testado pelo usuário (`USER_OBSERVED`; censo `PROVEN` métrico no attract). Paredes do intro `MAP_18B` continuam escopo separado, não revalidadas aqui:

- **Relato e sintoma (`USER_OBSERVED`)**: Em 16:9 itens, portas, Jinjos e algumas paredes sumiam nas bordas da tela (visível no Gameplay Demo 2 e Terrydactyland).
- **Diagnóstico e causa raiz (`PROVEN`)**: O ponteiro global do volume de visão `0x8012D500` tem seus vetores horizontais calculados a partir de `+0xE8` (aspecto 4:3) em `0x800CA1B0`/`0x800CA218`/`0x800CA2A8`, e `0x800CAF34` constrói os 4 planos $\{n, d\}$ a cada frame. O renderizador de modelos `0x800DE498` gerava 1323 rejeições indevidas de widescreen (`ws`) em dezenas de modelos distintos.
- **Tentativa 1 refutada (`REFUTED`)**: Forçar desenho em `0x800DE498` restaurando `0x8012C940`/v0 divergia simulação com callbacks ativos; com callbacks nulos congelava atores na borda e tornava Jinjos/portas visíveis através de paredes (`USER_OBSERVED`). Convertido no experimento default-OFF `TOOIE_TEST_WS_MODEL_DRAW_ASPECT=1`.
- **Solução por planos nativos (`PROVEN` de produção, default ON)**: Durante `0x800CA1B0`/`0x800CA218`/`0x800CA2A8`, o aspecto `+0xE8` é temporariamente substituído por `aspecto * (alvo/origem) * margem_lateral` (default 1.15) e restaurado imediatamente. A matriz de projeção `0x800CB124` permanece em 4:3 para o RT64. Todos os 41 acessores e testes nativos (`0x800CAA24`, `0x800CAB70`, `0x800CACEC`) passam a operar sobre planos 16:9 nativos sem distorção anamórfica; expansões por teste passam a $k=1$. O censo ws caiu para 0 em todos os tipos. Desativável por `TOOIE_TEST_WS_NATIVE_PLANES=0`.
- **Proteção do attract mode (`PROVEN` métrico)**: Sob `0x80127632 == 5`, os planos permanecem em 4:3 para preservar os replays gravados; caminho de 1603 posições do jogador na sonda é 100% idêntico ao original.
- **Validação pelo usuário e testes (`USER_OBSERVED` / `PROVEN`)**: Usuário confirmou em gameplay: "Parece tudo correto agora". Testes `Build-Tooie.ps1 -Stage Test -AllTests`: 9/9 obrigatórios + 17/17 extras PASS (incluindo `p08_ws_frustum_test` 68/68 PASS; 5 skipped por desenho); executável SHA256 `0D44B01E52223B9579C19852CEA06EEEC7457913F14F33DA1A5DC63292C72B02`.
- `NEXT_SINGLE_ACTION`: investigar o glitch de interpolação em cutscene relatado pelo usuário (close de Banjo/Kazooie, escopo P09), após commit autorizado desta correção.

## Histórico — CURRENT — `WAITING_USER_SCENE`

Ver [[../../plan/evidence/p09-visual-test-mode-20261020]]. Gates host do snapshot aprovados (build/revisões, unidade 123/123, I/O separado e negativos semânticos); captura real `NOT_EXECUTED`. Novo jogo isolado PID inicial `1188686`, configurado 60 + novo snapshot sem heavy; FPS efetivos `UNKNOWN`, marcador inexistente. `NEXT_SINGLE_ACTION`: aguardar usuário avisar cena pronta e só então armar uma vez, não por cronômetro. Não é aceite de gameplay ou correção permanente; P08 não encerrado/P09 não aberto formalmente. CURRENT/30 normal preservado; sem commit/push; estados abaixo históricos.

## CURRENT (2026-10-20) — VISUAL_TEST_MODE_AUTHORIZED_AND_LAUNCHED

Ver [[../../plan/evidence/p09-visual-test-mode-20261020]]. Nova autorização do usuário permite
retirar o paliativo **nos testes**. Opt-in `TOOIE_TEST_INTERPOLATION_FPS=60`
(ou 120) desativa CURRENT quando não explicitamente definido e força a taxa
manual escolhida; sem opt-in, caminho estável CURRENT/30 preservado.
Build e testes de configuração/endpoint passaram; revisões Sonnet/Haiku sem
bloqueios para este modo. Jogo aberto em teste 60 FPS sem coletores pesados;
resultado visual e taxa efetiva **PENDING / UNKNOWN**.
Reprodução: Demo 1 pós-intro; gameplay lateral para focinho; Talon Trot
`Z + C-Esquerda` lateral para penas da **cabeça** da Kazooie.
`NEXT_SINGLE_ACTION`: obter relato visual desta execução. Snapshot CPU segue
parcial/não aceito para captura. Esta entrada prevalece sobre próximos passos
históricos abaixo; P09 não abre formalmente e nenhum commit/push foi feito.

## Continuação delegada — snapshot parcial, gate final pendente

Ver [[../../plan/evidence/p09-interpolation-resumption-20261020]]. Último build Grok reportado exit 0, snapshot 97/97; diagnóstico default-OFF **`NOT_ACCEPTED_FOR_GAME_CAPTURE`**, revisão final e novo teste I/O ainda pendentes. CURRENT/Manual 30 preservados, nenhuma captura/aceite 60+ FPS. Focinho Banjo e penas **da cabeça** Kazooie priorizados (`USER_OBSERVED`), demais glitches incluídos. Root/builder/reviewer Codex atingiram cota; sem encerramento P08 ou abertura formal P09, sem commit/push. `NEXT_SINGLE_ACTION`: retomar gate independente final de snapshot-only, alcance de hooks, persistência e corrupção, incluindo novo teste I/O em processos separados, antes de captura. Execução sempre delegada; estados abaixo preservam histórico.

## Investigação retomada de interpolação (2026-10-20)

Ver [[../../plan/evidence/p09-interpolation-resumption-20261020]]. Objetivo
delimitado: investigar P09-RES-05 e decidir o discriminador seguinte. CURRENT
`tfPos` padrão e Manual 30 preservados; quatro testes de apoio passaram.
Sem código/patch alterado, jogo, captura nova ou validação visual nesta etapa.
`NEXT_SINGLE_ACTION`: implementar snapshot/replay CPU limitado e default-OFF
de proveniência de arestas e gates de iluminação/texgen; não implementado ainda.
Esta investigação não fecha os gates formais P08 nem abre formalmente P09.

## Menu settings functionality, policy enforcement, window management & BGM volume (2026-10-19)

Evidência canónica: [[../../plan/evidence/p08-settings-functionality-20261019]].
Tarefa delimitada de configurações e volume BGM concluída com sucesso (`BOUNDED_TASK_COMPLETED`). Ajustes de política de menu, proteção de opções desabilitadas com correção de estilo numérico, resoluções 3x/4x, redimensionamento automático de janela, canal IPC de preview e controle de volume BGM integrados e validados. O portão de controles de P08 permanece pendente; sem encerramento global da fase.

- **Canonical Tested Base & Escopo (PROVEN)**:
  - HEAD base testada: `d4d1074016a819d7150923c2b19a91e69889b7a9`. Commit e push deste checkpoint explicitamente autorizados pelo usuário em 2026-10-20.
  - Submódulos e tooling locais protegidos dirty (`RecompFrontend` `d0d90ba`, `N64ModernRuntime` `ca568b6`). Sem edições manuais em arquivos gerados ou recompilados.
- **Ajustes de Política de Menu Autorizados pelo Usuário (PROVEN)**:
  - *Note & Jinjo Saving*: Removido da interface gráfica de forma segura (`hidden = true`, default `Off`, schema de configuração mantido para compatibilidade).
  - *Opções Desabilitadas (Integração Não-Trivial BT)*: Opções sem suporte downstream direto no guest engine Tooie foram desabilitadas na UI com a descrição `"Currently unavailable in Banjo-Tooie."` (`UI_DISABLED / NOT_IMPLEMENTED_BT_CONSUMER`): Câmera Analógica, Sensibilidade, 3 Inversões de Eixo (Terceira Pessoa, Primeira Pessoa, Nado/Voo), Proporção de Cutscenes e Posicionamento do HUD (`hr_option`, HUD Placement). Testes de arrasto real mantêm Sensibilidade 3, Framerate 30 e Câmera Analógica Desligada.
  - *Trava de 30 FPS*: Taxa de quadros forçada para `Manual` 30 FPS (`tooie_menu::kForcedRefreshRate = 30`). Opção na UI desabilitada com aviso de cap temporário. Classificação: cap decorrente de política do usuário pelo uso do paliativo `tfPos` CURRENT (que repete poses de vértices acima de 30 Hz); o RT64 possui capacidade técnica de apresentar a 60+ Hz, não se tratando de limitação de falta de suporte a 60 Hz.
  - *Resoluções 3x e 4x com Downsampling Multiplicativo*: `Original3x` (base 720p) e `Original4x` (base 960p) adicionadas ao runtime e frontend. Fatores de downsampling multiplicam a base selecionada (ex: 3x com DS 2x renderiza a 1440p e reduz para 720p). Resolução `Auto` utiliza escalonamento inteiro de VI de janela (não adaptativo de framerate) e desativa downsampling.
  - *Redimensionamento de Janela por Proporção (Preferência Confirmada do Usuário)*: Selecionar `Original` (4:3) ou `Expand` (16:9) redimensiona a janela em modo janela mantendo a altura atual (1600x900 -> 1200x900 -> 1600x900 mantendo 900) e limitando a largura às dimensões úteis do display (subtraindo bordas/decorações via `SDL_GetWindowBordersSize`). Em modo tela cheia, a solicitação de redimensionamento é ignorada.
- **Diagnóstico e Resolução de Defeito Real no App (CORRECTED / PROVEN)**:
  - *Sintoma*: Configurações antigas salvas com `rr_manual_value: 240` permaneciam a 240 FPS; opções numéricas desabilitadas podiam ser incrementadas (ex: sensibilidade 3 -> 9).
  - *Refutação da Hipótese de Haiku*: Hipótese de risco de ordem de carga REFUTADA como incorreta/incompleta.
  - *Causa Raiz Comprovada*: Realocação de vetor em `recompui`. O frontend armazena as configurações em um `std::vector<std::pair<std::string, Config>>`. Ao registrar abas subsequentes, o vetor realocou memória, transformando a referência `Config&` capturada em um ponteiro dangling!
  - *Correção*: `tooie_menu::install_forced_refresh` resolve `recompui::config::get_graphics_config()` dinamicamente a cada execução. Patch `recompfrontend-disabled-option-guard.patch` bloqueia alterações via `Slider::do_step` e `ConfigPageOptionsMenu::on_set_option_value`. Adicionalmente, `ConfigOptionNumber::update_disabled` agora chama `slider->set_enabled(!disabled)` corrigindo o estilo numérico desabilitado (sessão final Haiku `a7c1b458-e32c-4f51-9d80-eaf5f6b57aef` exit 0 VISUALPASS em `general-disabled-final.png` e `graphics-locked-final.png`; interação por root).
  - *Teste de Regressão*: `tests/tooie_menu_config_test.cpp` simula realocações com 16 configurações fictícias e comprova leitura de 240 forçada para 30 em temp, permanent e save.
- **Thread Safety de Janela SDL (PROVEN)**:
  - Callbacks da interface gráfica não chamam funções SDL diretamente; apenas enfileiram a proporção em um slot atômico (`tooie_menu::request_window_aspect`).
  - A thread de eventos SDL consome a requisição dentro de `update_gfx` (`src/main/main.cpp`), garantindo ausência de concorrência ou deadlocks.
- **Build e Testes Automatizados (PROVEN)**:
  - Compilação completa: `cmake --build build --target TooieRecompiled tooie_bgm_volume_test tooie_bgm_hooks_test tooie_menu_policy_test tooie_menu_config_test -j2` exit 0 (13.1s).
  - Binário SHA256: `e2059b516a90f09e50e77f0622218014798f6a155f6ffe0d2b6a66735427f9f9`.
  - Execução de testes: todos os 4 testes PASS exit 0 (`tooie_menu_policy_test`, `tooie_menu_config_test`, `tooie_bgm_volume_test`, `tooie_bgm_hooks_test`).
- **Evidência Visual, Launcher Preview IPC e Áudio em Runtime (PROVEN)**:
  - Execução isolada fora do repositório com `APP_FOLDER_PATH=/home/wesley/.cache/tooie-menu-settings-20261019/config` (hashes originais preservados).
  - Processo filho de preview do launcher conectado via `socketpair` datagrama nonblocking `CLOEXEC` (dup stdin 0 com validação `SO_TYPE`, monitor pai 5ms com envio de 1 byte via `MSG_NOSIGNAL`).
  - Áudio prático validado com null sink `tooie_bgm_menu_qa` em `/home/wesley/.cache/tooie-menu-settings-20261019/`:
    - Preview de menu (mesmo processo filho ativo): `preview-100.wav` (RMS .0522), `preview-0-real.wav` (RMS .0000017, peak 1 LSB), `preview-50-real.wav` (RMS .0167), `preview-100-restored.wav` (RMS .0373).
    - Restauração ao vivo em gameplay (sem reiniciar): `game-0.wav` (RMS 0, peak 0) -> `game-100-restored.wav` (RMS .0528, peak .3998).
    - Disciplina epistêmica: Músicas variam entre passagens; não alegar proporção exata de 50% em PCM; atenuador linear verificado nos testes unitários. Capturas iniciais com clique renomeadas para `preview-*-ui-not-applied.wav` (inválidas). Vozes em sequence player permanecem UNKNOWN; SoundPlayer intocado; sem alegação de paridade global de áudio. Módulo null sink 536870913 descarregado.
- **Patches Versionados e Aplicador**:
  - Três patches registrados em `tools/apply_runtime_patches.sh`: `n64modernruntime-resolution-3x-4x.patch` (`6955ca32...`), `recompfrontend-resolution-3x-4x.patch` (`60eb3140...`) e `recompfrontend-disabled-option-guard.patch` (`9298ec60...` superando hash 867 anterior).
  - Replay em fixture de base limpa verificado exit 0 em apply e reverse. Script completo live NOT_RUN. Revisão estática Sonnet `c4500e0c-e5c9-4b1b-bcf4-5a054a8fe733` exit 0 (sem bloqueadores no escopo; processo 143 descartado, não cota).
- **Integração de BGM no Guest de BT (PROVEN)**:
  - Contrato BT: 6 players em `0x80079FA0` (stride `0x274`, `seqp+0x1E0`), volume s16 em `player+0`, 6 tracks em `0x801357D0` (stride `0x50`, id s16 em `+0x28`). Despacho CSP `80025ED0` (tipo 10, música) vs SoundPlayer `80020BE0` (tipo 3, SFX).
  - Overrides registrados em `boot_hw_stubs.cpp`: `tooie_bgm_on_csp_set_vol` (`80025ED0`) escala uma única vez os 6 seqps conhecidos; `tooie_bgm_on_update` (`800FB968`) atualiza live gain pós-fade usando cópia local de contexto apenas quando alterado (zero eventos em 100% constante; fades originais intactos).
- **User confirmation (2026-10-20, USER_OBSERVED / USER_CONFIRMED_FOR_TESTED_DEMO_CLIP)**: with BGM at 0% in `final-game-0.wav`, user reported “Apenas efeitos/vozes”. Music removal and preserved effects/voices confirmed for this clip; campaign-wide voice classification remains UNKNOWN.
- **Next Single Action**: `verify published HEAD against origin/main after authorized push`.

## Host launcher Banjo-Tooie wordmark & asset isolation (2026-10-19)

> [!NOTE]
> **Histórico / Reconciliação com a restauração de UI (2026-10-21):** O texto abaixo documenta a criação do asset de launcher em 2026-10-19. Em 2026-10-21, o usuário esclareceu formalmente que a restrição de recursos livres/próprios se aplica estritamente aos dados do jogo extraídos da ROM (texturas, áudio, geometria, scripts). Todos os assets de menu, artes, fontes tipográficas (incluindo `Suplexmentary Comic NC.ttf`), animações completas e o logo Tooie original (`ui_assets/TooieLogo.svg` 7300x2450) foram preservados e restaurados localmente pelo root a partir do backup de candidatos (`candidate-backup`), superando completamente a substituição interina neutra (774x272). Sob a arquitetura standalone da P10, esses assets vivem localmente em `assets/` e `ui_assets/`, gerenciados a cada compilação pelo alvo CMake `TooieUIAssets`, sem qualquer dependência de checkouts externos.

Evidência canónica: [[../../plan/evidence/menu-title-tooie-wordmark-20261019]].
Conclusão da substituição do wordmark do launcher host por asset SVG próprio do Tooie, desacoplado dos assets compartilhados do Banjo-Kazooie:
- **Identidade do Wordmark & Isolamento de Assets (PROVEN)**:
  - SVG dedicado em duas linhas: "BANJO-TOOIE" (azul com contorno amarelo) e "RECOMPILED" (vermelho com contorno amarelo) gerado por `tools/make_tooie_launcher_logo.py` (SHA256 `d13cb8d9...`) lendo `BanjoRecomp/assets/Logo.svg` em modo somente-leitura; `BanjoRecomp/assets/Logo.svg` permanece inalterado e rastreado (limpo, não untracked).
  - Asset Tooie `ui_assets/TooieLogo.svg` (viewBox `0 0 7300 2450`, SHA256 `036d3cb20c97b2b37a427e16c2fa4190b6bc676aae1655a037d7bf4785d5c5b7`) copiado via CMake para `${CMAKE_CURRENT_BINARY_DIR}/ui_assets/TooieLogo.svg` (alvo `TooieLauncherLogo`, dependência de `TooieRecompiled`), preservando os assets compartilhados de `BanjoRecomp/assets/` intactos.
  - `src/main/launcher_animation.cpp` referencia `"../ui_assets/TooieLogo.svg"` com dimensões escaladas `7300.0f * 0.106f` x `2450.0f * 0.106f` (~774x260 dp).
- **Build & Verificação Visual (PROVEN / VISUALLY_CONFIRMED)**:
  - Compilação incremental via `cmake --build build --target TooieRecompiled -j2` exit 0 (não compilação limpa), gerando executável `build/TooieRecompiled` (SHA256 `ffe3d7a52e2e4b97d159e79fb156ab27e3a909dfa73a44633f7ff15b8bfb46d4`).
  - Verificação visual independente realizada por Claude Haiku 5.5 medium (exit 0; capturas 1600x900 `frame_1.png`..`frame_6.png` e 1024x768 `resize_1024x768_4x3.png`); confirmado título em duas linhas legível, ausência de clipping, espaçamento entre letras sem colisões, arte de personagens e botões de menu íntegros. Root inspecionou visualmente apenas `frame_1.png` (1600x900); Haiku inspecionou 4:3 (`resize_1024x768_4x3.png`), sem alegação de inspeção 4:3 por root.
  - Animação de entrada do launcher: follow-up de Claude Haiku 5.5 medium (exit 0; `menu-title-haiku-animation-addendum.txt`) inspecionou 5 imagens discretas (0.28s, 0.79s, 1.42s, 2.57s, 5.87s); composição de entrada do logo PASS, fluidez/interpolação NOT_TESTED (frames discretos). Layouts estáveis em 1600x900 e 1024x768 preservados PASS. Sem QA de gameplay.
  - Modificação não commitada no repositório local; estado pré-existente de ferramentas/configurações locais preservado. Checkpoint anterior publicado com sucesso como commit `2c52af0c7d8287d9003db839ffbe636d585e900a` em `origin/main`.
- **Next Single Action**: `extend oracle comparison to another attract demo when user requests next game adjustment`.

## Checkpoint publicado (2c52af0c): RT64 clean-pin patch replay & getter gate fix (2026-10-19)

Evidência canónica: [[../../plan/evidence/checkpoint-20261019-review-and-patch-replay]].
Checkpoint publicado com sucesso em `origin/main` (commit `2c52af0c7d8287d9003db839ffbe636d585e900a`, 199 caminhos, verificado por `checkpoint-publication-proof.json`).
Revisão pré-commit do manifesto de 196 caminhos concluída com Grok 4.7 Medium terminando em `end_turn`.
- **P1 RT64 Clean-Pin Replay (RESOLVIDO)**: 3 patches limpos (`rt64-rsp-endpoint-current.patch`, `rt64-present-fps-decimator-and-counter.patch`, `rt64-p09-exact-use-correlation-header.patch`) e patch de RecompFrontend registrados em `tools/apply_runtime_patches.sh` (+49 linhas; `bash -n` exit 0, `git diff --check` exit 0); replay validado em clone isolado; testes de host compilam e passam (`rt64_present_decimator_test` PASS, `p09_exact_use_correlation_test` 1354/1354, `p09_cpu_shader_clip_semantics` 14/14). Patches incrementais antigos permanecem no repositório como proveniência histórica, porém são SUPERSEDED para aplicação em pin limpo. Hooks C++ de patologia não reproduzidos do pin.
- **P2 Getter Diagnostic Gate (RESOLVIDO)**: `v7_note_model_getter` chaveado por `g_getter_durable_needed` em `include/tooie_diag.h` e `src/game/tooie_diag.cpp`. No gameplay padrão retorna antes de contadores, leituras de guest, mutex e escritas em host. Validado por `tests/getter_diag_gate_test.cpp` PASS exit 0 e build `TooieRecompiled` exit 0 (SHA256 `74a0cea04a2bc3704860388673c40b2e73ee34d284f3a2afa0faebe91a9c05f5`, distinto do binário candidato A6A2 `a6a2c023...` testado pelo usuário).
- **Confirmação visual**: Mantida a confirmação visual direta do usuário para a área testada ("Sim, continua funcionando" no binário A6A2). Não alegar confirmação visual para o novo hash `74a0cea0...`. Paridade global e mapas não testados permanecem residual.
- **Next Single Action**: `extend oracle comparison to another attract demo when user requests next game adjustment`.

## USER_DEMO1 frustum parity narrow allowlist landed & user visually confirmed for tested area (2026-10-19)

Read [[../../plan/evidence/p08-user-demo1-movement-and-fire-egg-differential]], [[../../plan/evidence/p08-user-demo7-cutscene-drift-investigation]], [[../../plan/evidence/p08-demo-movement-precision-diagnosis]], and [[../../plan/evidence/p08-attract-movement-residuals]].
Grok 4.7 Medium implementation session `01a15118-c738-7dd3-98ed-cc5db5021bb5` completed `end_turn` (38 turns; root observed CLI exit 0). The narrow fix landed in candidate binary A6A2 (SHA256 `a6a2c023a9e3b0688cac15249cda61b5c8b8ccdf68c7c40f91d8c512356bef5e`).
Source edits owned in `src/game/boot_hw_stubs.cpp`, `src/game/tooie_demo1_projectile.h`, and `tests/tooie_demo1_upd_layout_test.cpp`; `RecompiledFuncs` untouched, no generated hand-edits.

- **Scope Boundary & Allowlist Architecture (PROVEN)**:
  - `tooie_wrap_func_800E3E8C` no longer opens a `TooieCacecDrawScope` (shared broad DRAW scope dropped; nested CACEC only expands if an outer scope is active).
  - Pure draw caller `func_800B97A4` registered via `tooie_wrap_func_800B97A4` with scope kind 3 when `tooie_cacec_pure_draw_vram(kCacecPureDrawB97A4)` is true (guest VRAM `0x800B97A4`; accept path emits G_MTX / G_DL / G_MOVEMEM and matrix calls without RNG/simulation side effects).
  - Existing `tooie_wrap_func_8011447C` scope kind 2 preserved for object visual pop culling.
  - Mixed renderer `func_800DE498` (which stores E3E8C `v0` to `0x8012C940` and on accept executes callbacks and writes renderer context) stays on original `v0` because no scope wraps it.
  - Unclassified callers stay on original `v0`. No default flag OFF, no RNG save/restore, no host-PC/RA production gates.
- **Production Default-ON Parity Verification (PROVEN)**:
  - Feature remains production default ON (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1`). Both fresh default-ON captures completed exit 0 with matching verification.
  - **TARGET Capture** (`demo1-sep-target-current/target.csv`, SHA256 `016a102c92f279aef896bcab0802be27b1671adab06f8bc374bf227573b398ec`, owned PID 1776728, 66.084s, exit 0):
    - All 355 paired trajectory samples `w0..w11` (`tgtrec`) match original flag-OFF / oracle2 bit-exact (0 mismatches; old buggy default-ON diverged post 83).
    - All 546 ActorRecord words (`updrec` 39 words x 14 pre/post snapshots across posts 80..86) match original bit-exact.
    - All 14 raw LCG RNG snapshots (`a2`) match original bit-exact (resolving PRE82 RNG split).
    - All 14 `+0x60` words match original `0003006c` (was `0004006c` under buggy default-ON).
  - **PROJECTILE Capture** (`demo1-sep-projectile-current/projectile.csv`, SHA256 `570f42e0ad8d31a883c2345afc5e0b000a3fde22baa7edc7722ab35b031d1531`, owned PID 1774174, 67.274s, exit 0):
    - Post 355 sweep `func_800CD820` leave returns `v0 = 0x80132f60` (hit) and narrowphase `func_800CD6B4` leave returns `1`, exactly matching original hit (old buggy default-ON missed with `v0 = 0`).
    - 22 sweep ENTER pairs and 21 `sueggmove9` ENTER pairs match original in 6 geometry words and gamespeed `gs`.
  - **Build & Tests**: Ninja exit 0 (up-to-date); all unit test binaries (`upd_layout`, `target_selector`, `projectile`, `parity`, `demo7_drift`, `mips_rounding`, `coverage`) exit 0. Test `tooie_demo1_upd_layout_test` verifies `pure_draw_accepts=1` in harness. Unit test models old policy vs new policy; no actual old binary unit test execution exit 1 is claimed.
- **Epistemic Boundaries & Status**:
  - Positive draw accept exists in unit harness; runtime TARGET window recorded `cacecx = 0` (expected when mixed DE498 scope is dropped; runtime cacecx is NOT promoted from 0).
  - Graphical QA of widescreen object-edge pop previously `NOT_TESTED` is now superseded by direct user visual confirmation: user launched `./build/TooieRecompiled` normally without flags in a previously affected gameplay area and confirmed: *"Sim, continua funcionando"*.
  - Classification: **`VISUALLY_CONFIRMED_FOR_TESTED_AREA`**, **`USER_REPORTED`**.
  - Epistemic scope: This confirms preservation of the observed visual expansion in the gameplay area tested by the user. Do NOT claim all maps, all objects, or global movement parity proven. Broader game parity remains an untested residual.
- **User Direction & Next Step**:
  - User explicitly chose KEEP VISUAL EXPANSION ACTIVE (feature stays default ON; candidate binary A6A2).
  - User visual test closes QA for the tested area. No new action needed on the bounded Demo 1 fix. User may have the game open; do not touch running processes.
  - Invariants maintained: P09 Phase 1 default CURRENT `tfPos` workaround remains default-ON (palliative); FPS counter/toggle and RT64 decimator unchanged; audio glitch deferral maintained.

```text
SCENE_IDENTITY: PROVEN (stage 0, sub 0, map 335 entrance 8 mode 5, glrecord ovl 698 cnt 766)
SCOPE_BOUNDARY_FIX: PROVEN (shared E3E8C broad scope dropped; allowlist pure-draw B97A4 kind 3 + preserved 8011447C kind 2; DE498 original v0)
DEFAULT_ON_TARGET_PARITY: PROVEN (355x12 trajectory words, 546 record words, 14 RNG snapshots, +0x60=0003006c exact original under default-ON binary A6A2)
DEFAULT_ON_PROJECTILE_PARITY: PROVEN (post 355 Fire Egg sweep hit v0=0x80132f60, CD6B4=1, 22 sweep + 21 move pairs exact under default-ON binary A6A2)
BUILD_AND_TEST_STATUS: PASS (ninja exit 0; layout/selector/projectile/parity/demo7/mips/coverage exit 0; pure_draw_accepts=1 in harness)
WIDESCREEN_EDGE_QA_STATUS: VISUALLY_CONFIRMED_FOR_TESTED_AREA (USER_REPORTED: "Sim, continua funcionando" on normal noflags launch; cacecx=0 in target window unpromoted; broader game/objects untested residual)
USER_CHOICE: RESOLVED (KEEP EXPANSION VISUAL ACTIVE; feature stays default ON; user visual QA completed for tested area)
PRESERVED_STATE: Object visual pop fix default ON, Demo 7 generator fix, CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: extend oracle comparison to another attract demo when continuing broader fidelity
```

## Prior: USER_DEMO1 source-proven call chain & Grok round 2 implementation (2026-10-19; SUPERSEDED BY LANDED FIX)

Read [[../../plan/evidence/p08-user-demo1-movement-and-fire-egg-differential]], [[../../plan/evidence/p08-user-demo7-cutscene-drift-investigation]], [[../../plan/evidence/p08-demo-movement-precision-diagnosis]], and [[../../plan/evidence/p08-attract-movement-residuals]].
User confirmed that Demo 7 cutscene stopping is **VISUALLY FIXED** and that the Fire Egg miss persists on current corrected binary (SHA256 `87c5a08b96b6fbb9c29c0079ac6b7b6060af8f5da6b92147f60e5dacce85bbe5`). The primary objective is **general subtle gameplay/action parity** across movement, kinematics, and state transitions, not an artificial aim or hit steering adjustment; no global clock hack.
**User Choice Resolved**: KEEP EXPANSION VISUAL ACTIVE while proper fix; **NO temporary OFF default**. Setting flag OFF (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0`) is strictly an experimental per-process counterfactual diagnostic; NO production source or config change has been made.
Production invariants preserved: shader CURRENT `tfPos` workaround remains default-ON (palliative); FPS counter/toggle and RT64 decimator unchanged; audio glitch deferral maintained.

- **Single-Feature Counterfactual Causality (PROVEN)**:
  - On the unchanged binary (`c56ce4e17b554020c36eec14a9e48861693679e9426161031e10aef4527a67e2`) with the TARGET logger (`TOOIE_DIAG_DEMO1_TARGET=1`), setting ONLY `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0` (explicit-off getter at `boot_hw_stubs.cpp:4264..4267`, wrapper 4451 calling original `800CACEC` and returning before aspect-aware expansion; no other `TOOIE_TEST_*` flag combined; CSV `/home/wesley/.cache/tooie-fps-investigation/demo1-first-writer-current-object-original/target.csv`, SHA256 `1f43e7c570f21ec082bc7afac88cf01789cebbd8c2ec4e142f0aee9eb791f928`, 415 rows, 66.217s, exit 0/0, FINISH 357) restored:
    - All 355 `tgtrec` posts 2..356 x 12 raw words bit-exact with the original Angrylion oracle reference (`demo1-first-writer-oracle-2`).
    - All 14 pre/post snapshots across frames 80..86 x 39 ActorRecord words (546 words total) bit-exact with Oracle (`demo1-object-frustum-record39-comparison.json` is `[]` empty list).
    - Record `+0x60` high 16 bits: was `0004` (curr `0004006c`) under default aspect-aware ON vs original `0003` (`0003006c`); with flag OFF, it is now `0003` (`0003006c`), matching Oracle exactly.
    - All 14 raw RNG `D_8012C7F0` snapshots bit-exact with Oracle, completely resolving the PRE82 divergence ($k=4$ `036f8437` vs $k=5$ `c125942a`) and POST83 geometry divergence.
    - All 14 gamespeed `gs` words exact `0x3d4cccce`.
  - In comparison, default Current with aspect-aware ON had 274 posts (83..356) differing in position.
- **Actual Fire Egg Collision Restored (PROVEN)**:
  - In a dedicated PROJECTILE-only capture with the same flag OFF (`TOOIE_DIAG_DEMO1_PROJECTILE=1` and `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0`, controller 1600930 / game 1600931, exit 0/0, 66.824s, 313 rows, FINISH 391, CSV `/home/wesley/.cache/tooie-fps-investigation/demo1-projectile-current-object-original/projectile.csv`, SHA256 `3d54fe91ac1bc570a95a87241322f7d58593575aac951c4b4250d397a3344258`):
    - Actual Fire Egg sweep at post 355: `func_800CD820` sweep leave returns `v0 = 0x80132f60` (nonzero hit, matching Oracle `89a6bad1...`), whereas default Current returns `v0 = 0` (miss).
    - Narrowphase `func_800CD6B4` LEAVE = 1 on both off-current and Oracle; default Current has no such callback at 355.
    - All 22 sweep ENTER input pairs and 21 `sueggmove` ENTER input pairs are exact in `w0..w5` geometry and `w11` `gs`.
    - Sweep/move counts: 44 sweep / 42 move off/oracle vs default 64 / 62 (projectile stops at hit).
    - Bound centers at posts 354 and 355: all 6 selected `0x8008A018` LEAVE center triples exact with Oracle, including actual target post 355 `w0=c5af7c8d, w1=c401d0fa, w2=44a89e51` = `(-5615.56885, -519.265259, 1348.94739)`, completely restoring the prior ~427-unit bound center difference.
- **Causal Verdict & Epistemic Boundaries**:
  - `OBJECT_FRUSTUM_ASPECT_AWARE=1` is **CAUSAL** for observed RNG divergence, NPC trajectory divergence, and Fire Egg collision miss in USER_DEMO1.
  - The underlying call consumer and side-effects remain **UNKNOWN**.
  - Do NOT claim all NPCs globally fixed or physical fix done.
  - Do NOT globally REFUTE widescreen capability; update prior UNTESTED statement to PROVEN at feature level with mechanism UNKNOWN.
  - Source ABI and canonical getter roles from prior investigations are strictly preserved.
- **Source Mechanism Discovered (`PROVEN` Call Chain & Gating)**:
  - Broadly treating `func_800E3E8C` (`E3E8C`) as DRAW scope allows aspect-expanded accept (`v0 = 1`) inside mixed renderer routine `func_800DE498` (`0x800DE6EC`).
  - `func_800DE498` stores `v0` to `0x8012C940`. If nonzero (accepted), it takes the visible path: dispatches registered pre-draw callback `0x800DE738: jalr $v0` with `$a0` loaded from `0x8012C9AC`, calls asset resolver `800D674C`, stores renderer processing context `s0+0x14/0x18/0x1C` (renderer processing context writes, NOT actor state writes; do not conflate), and emits display lists. If culled (`v0 == 0`), it branches to `0x800DE718`, calls cleanup `func_800DE318`, and returns 0.
  - Standard actor draw entry `func_80101870` -> `func_80101808` calls `func_800DF47C(func_801015D0, arg0)`, registering callback `func_801015D0` at `0x8012C9A8` and `Actor*` at `0x8012C9AC`.
  - Callback `func_801015D0` calls `func_80101934(Actor*)` at `0x80101784`, which indexes jump table `D_801243E8` via `(actor->unk74 & 0xF)`. Slot `+0x04` invokes `func_80101A50(Actor*)`.
  - `func_80101A50` calls `func_800DC0C0` (advancing global LCG RNG seed `D_8012C7F0`, RA `0x80101AA8`), tests random float against `0.03f`, mutates guest actor state byte `sb $t2, 0x7A($a2)`, and calls `func_800DF744` with selectors 1 and 2 writing to `D_8012C850`.
  - **Epistemic Classification**: Semantic identity as facial/eye-blink animation is **`STRONG_INFERENCE`** (suggested by 3% probability check and selectors 1/2, but without explicit asset/animation binding proof in source). Control flow gating, global LCG RNG stepping, actor byte `0x7A` mutation, and renderer array writes are strictly **`PROVEN`**.
- **Paired Native Telemetry Arbitration (`PROVEN`)**:
  - Telemetry (`demo1-rng-paired-native-chain-gate.json`): Current has 32 RNG steps vs Oracle 27 across posts 79..84.
  - Sequence step gap grows monotonically across posts: Post 79: 0 (6 vs 6); Post 80: 0 (4 vs 4); Post 81: +1 (7 vs 6; **first extra step**); Post 82: +1 (5 vs 4); Post 83: +2 (6 vs 4); Post 84: +1 (4 vs 3). Cumulative delta = +5 steps across window.
  - LCG chain exhibits **ZERO breaks** across all observed steps and snapshots (`breaks: []`). Every transition satisfies $s' = (1664525 \times s + 1013904223) \pmod{2^{32}}$.
  - Expected oracle after-states, once calculated, never match actual observed stores once desynchronized.
  - First extra RNG call is definitively the **SECOND `func_80101A50` call in post 81 (seq 146 in Current, before `6239b638` -> after `036f8437`)**, NOT global `func_8001608C` (which executes in both runtimes).
  - Preceded directly by `cacecx` extra accept owner `func_800DE498` at seq 142 on position `(-2818, 300, -2237)`.
- **Native PC Mapper & Symbol Resolution (`PROVEN`)**:
  - Old `T/t`-only cache (`demo1-rng-ee14-nm.json`) was **INVALID and incomplete** (omitted recompiled weak `W/w` functions).
  - Completed `nm T t W w` (`demo1-rng-ee14-origin-resolution.json`) validated binary `EE14` (`ee14d6cb1c144a09ccec7f393dc2e986ac26fb0826893d2e0d490df9d19a683c`) with positive function sizes.
  - Return address is guest original `0x80101AA8` (`jal func_800DC0C0` at `0x80101AA0`).
- **Background Workloads & Active Agents (2026-10-19)**:
  - **NO SONNET**: Sonnet quota expired (no more calls); last job `DIAG_READY` (binary `ee14d6cb...` / oracle core `1172a1e1...`), no prod FIX.
  - **Haiku**: Approaching quota limit; reserved/unused now.
  - **Grok 4.7 Medium**: Designated **SOLE** code writer and test executor. Resumed implementation session `01a15118-c738-7dd3-98ed-cc5db5021bb5`: round 1 cap 50 exit 1 with source proof and no edit (`demo1-frustum-parity-grok-implementation-report.txt`); round 2 active budget 70 (`demo1-frustum-parity-grok-implementation-round2.json`, completed `end_turn` exit 0, landing binary A6A2).
  - **Gemini**: Designated for **ALL** Markdown and documentation maintenance.
  - **Native Agent `demo7_capture`**: Read-only Grok review only; NO other capture authority.
  - **Opus**: DONE (read-only existing report `demo1-rng-consumer-opus.json`).
  - Current code EE14 diagnostics superseded by landed fix binary A6A2; original default vis ON CURRENT palliative / FPS / audio unchanged.
- **Main Implementation Objective**:
  - Correct shared `func_800E3E8C` consumer selection to separate genuine visual DRAW passes from logic/simulation callbacks. (RESOLVED in candidate binary A6A2).

```text
SCENE_IDENTITY: PROVEN (stage 0, sub 0, map 335 entrance 8 mode 5, glrecord ovl 698 cnt 766)
SINGLE_FEATURE_CAUSALITY: PROVEN (TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0 restores 355x12 prefix, 39x14 record words, 14 RNG snapshots, and post 355 Fire Egg hit)
SOURCE_MECHANISM: PROVEN (shared E3E8C broad DRAW scope enables mixed DE498 gate; registered callback 801015D0 -> 80101934 -> D_801243E8 -> 80101A50 steps global LCG RNG D_8012C7F0, mutates actor byte 0x7A, writes D_8012C850 selectors via DF744)
SEMANTIC_CLASSIFICATION: facial/eye-blink animation is STRONG_INFERENCE; control flow, RNG step, byte 0x7A mutation, renderer writes are PROVEN
FIRST_EXTRA_RNG_CALL: PROVEN (second func_80101A50 in post 81 seq 146 curr, preceded by cacecx extra accept in func_800DE498 seq 142; NOT global 8001608C)
PAIRED_RNG_GAP: PROVEN (grows 1..4 across posts, 32 curr vs 27 oracle in posts 79..84; zero breaks in chain; oracle after calculated never matches observed store)
NATIVE_PC_MAPPER: PROVEN (old T/t cache INVALID; completed nm T t W w validated binary EE14 with positive sizes; RA guest 80101aa8)
USER_CHOICE: RESOLVED (KEEP EXPANSION VISUAL ACTIVE while proper fix; NO temp OFF default; env flag 0 is EXPERIMENT ONLY)
GROK_STATUS: COMPLETED (session 01a15118-c738-7dd3-98ed-cc5db5021bb5 completed end_turn exit 0, binary A6A2)
AGENT_ROLE_ROUTING: Grok PRIMARY code/tests; Gemini ALL Markdown; Haiku minimal/unused; Native demo7_capture read-only; Sonnet NO calls; Opus DONE
PRESERVED_STATE: Object visual pop fix default ON, Demo 7 generator fix, CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: user visual check of widescreen edges in known affected gameplay area, feature ON; if fails narrower remaining mixed-render separation requires next task
```

## USER_DEMO1 projectile sweep branch, bound center divergence, and target prefix tracking (2026-10-18)

Read [[../../plan/evidence/p08-user-demo1-movement-and-fire-egg-differential]], [[../../plan/evidence/p08-user-demo7-cutscene-drift-investigation]], and [[../../plan/evidence/p08-demo-movement-precision-diagnosis]].
User confirmed that Demo 7 cutscene stopping is **VISUALLY FIXED** and that the Fire Egg miss persists on current corrected binary (SHA256 `87c5a08b96b6fbb9c29c0079ac6b7b6060af8f5da6b92147f60e5dacce85bbe5`). The latest objective is general subtle gameplay/action parity across movement, physics, and state, not an artificial aim/hit correction; no global clock hack. While historical geometric miss was cataloged in earlier recomp studies (BASE `876d299f`, §16/17, margin +36.85), fresh causal root on the current binary is strictly **UNKNOWN**.

Prior prefix parity (consumed 1..420 exact non-clock match at `0x800151BC` ENTRY) was independently confirmed by Haiku (`demo1-actual-pair-haiku-report.txt`). VI handler cadence difference was observed (100/419 transitions, +40 oracle entries); causal role in NPC/mob timing is UNKNOWN / OPEN HYPOTHESIS, player 420 exact does NOT refute NPC timing; `--nospeedlimit` is logical only, not real-time proof. No global clock fix claim.

Pair 2 owner-aware liveness tracking (`demo1-projectile-current-liveness` 189 rows 66.019s vs `demo1-projectile-oracle-liveness` 151 rows 94.215s) proved that at `post 341` Oracle aliased the same owner (`0x80133530`) at new heap address `0x801d085c`: the previous pointer freeze at 341 is **CORRECTED/SUPERSEDED** as a diagnostic stale pointer gap, not a gameplay first divergence. Return retry dedup / unmatched 0 validated. Same query vectors, radius 38, and callback `gs`. At `post 355`: Oracle `func_800CD820` sweep leave returns nonzero hit (`v0 = 0x80132f60`), whereas Current returns `v0 = 0`.

Pair 3 sweep branch diagnostic (`demo1-projectile-current-sweep` 321 rows 65.994s vs `demo1-projectile-oracle-sweep` 315 rows 94.712s) proved identical query 354/355 geometry, radius 38, `gs`, and identical cell-list candidate counts (2, 4, 2, 1, 3 across 5 lists, 24 candidates total, no cap hit). First QUALIFYING candidate bound center returned by `_subaddiecustomhits_entrypoint_5` (`0x8008A018`) differs already at post 354 (delta 431.29) and post 355 (delta 427.23), while radius 279.45 matches and the other 2 qualifying centers match. Oracle proceeds through narrow-phase checks `func_800EC3C4` and `func_800CD6B4` to hit `0x80132f60` at post 355; Current fails the earlier distance bound. First list candidate is NOT first qualifying candidate! Hit marker `0x80132f60` (`lhu+14=0x0671`, `lhu+18=0x0E3F`) has component record returned by `func_80106790` (`0x801c6434` vs `0x801cf614`), which MIPS proves is `vector_at(*(0x80136EE0), lhu+1A>>5)` with stride 0x9C (actor record in global actor vector, not model/bone). Bound center world relation is UNKNOWN pending descriptor bit 1 branch proof.

New target prefix diagnostic (`TOOIE_DIAG_DEMO1_TARGET=1` / `P08_DEMO1_TARGET=1` cap 500 post 1..356) implemented by Sonnet (binary `18fbfa73...`, oracle core `25c46a52...`), current run completed exit 0 (PID 1316481/1316484 HISTORICAL), oracle pending/later. Demo 7 visual PASS, generator fix, default CURRENT `tfPos`, FPS counter/toggle, and audio deferral preserved.

```text
SCENE_IDENTITY: PROVEN (stage 0, sub 0, map 335 entrance 8 mode 5, glrecord ovl 698 cnt 766)
CURRENT_TARGET_DIAG_BINARY_SHA256: 18fbfa735f3c11d60d989f2236f9c016eb51a6dbfbc5f7eebc98c3c6ec68f024
ORACLE_TARGET_CORE_SHA256: 25c46a523803ad879415c55cc6dd0512aebab5947a7f2fef02b1e13c22d6e953
CURRENT_SWEEP_CSV_SHA256: 343ba0401cd45bedcef7e84b2b90781f856038d558c8200395a0b0f5d3aa480e (321 rows)
ORACLE_SWEEP_CSV_SHA256: 89a6bad151beed9b0d3d852f468767e1a5dae75272248ed95ce47da3d7776660 (315 rows)
USER_DEMO7_STATUS: VISUALLY_FIXED (generator patch preserved)
USER_DEMO1_EGG_RESULT: USER_REPORTED_MISS (persists on user binary 87c5...)
USER_DEMO1_PREFIX_PARITY: 420/420_EXACT_MATCH (consumed 1..420 independently confirmed by Haiku)
VI_CADENCE_DELTA_ROLE: UNKNOWN/OPEN_HYPOTHESIS (cadence delta 100/419; player match does not refute NPC timing; logical only)
PAIR2_ORACLE_ALIAS_341: PROVEN (same owner 0x80133530; freeze hypothesis CORRECTED/SUPERSEDED as stale pointer)
SWEEP_QUERY_INPUTS: BIT_EXACT_MATCH (query 354/355 geometry/radius 38/gs exact bit-for-bit)
FIRST_QUALIFYING_BOUND_CENTER_DELTA: 431.29 at post 354, 427.23 at post 355 (radius 279.45 matches; other 2 centers match)
MARKER_CANDIDATE_IDENTITY: lhu14=0x0671, lhu18=0x0E3F (actor record via func_80106790 -> vector_at(*0x80136EE0, lhu1A>>5), stride 0x9C)
BOUND_CENTER_TO_WORLD_RELATION: UNKNOWN (pending descriptor bit 1 branch proof; not proven enemy world position)
TARGET_PREFIX_CAPTURE_STATUS: CURRENT completed exit 0 (PID 1316481/1316484 HISTORICAL); ORACLE pending/later
PRESERVED_STATE: Demo 7 generator fix, CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: finish paired target prefix / first state producer or branch relation before code fix; no target fake hit / global time scale
```

## USER_DEMO7 cutscene drift bounded investigation (2026-10-17)

Read [[../../plan/evidence/p08-user-demo7-cutscene-drift-investigation]], [[../../plan/evidence/p08-attract-movement-residuals]], and [[../../plan/evidence/p08-provisional-follow-camera-and-egg-differential]].
User diagnostic capture analyzed via offline streaming parser (provenance: user capture `demo7-user-capture.log` SHA256 `74f859128f586306626df1eb964e161983c6c916c9509c14b04ff8b577d99467`, 407,919 lines; analysis dump `demo7-user-capture-analysis.txt`; verdict `demo7-user-capture-verdict.txt`; Grok CLI session `01a14b7c-c6ef-7bd3-9a0b-3fef4d99acf4`, exit 0). No code, build, or runtime execution this task (`PARSED_NOT_A_GAME_PASS`; zero tests reported `PASS`). Scene identity is PROVEN as `stage = 0`, `sub = 8`, `cnt = 1245` (1,244 consecutive rows `glidx 1..1244`, `tick_delta == 2`). Logger map field (`0x80127640`) clears to 0 during mode-5 body after warp consumption (filtering `map == 282` drops 100% of body). Viewport camera (`0x8012D500`) cuts overhead at `glidx = 38` (dY 345 -> 755; `cammode` contaminated). Stick released at `glidx = 50`, but `bs = 4`, `anim = 111`, `tspd = 500`, `velxz ~ 514..520` remain. Displacement `glidx 50..90` matches `velxz * gs` (stream gate median ratio 1.000354). Motion owner is narrowed from UNKNOWN mechanism to physical integration of retained target speed in guest world displacement; root cause and interpreted guest writer of `tspd`/`vel` remain STILL UNKNOWN. Collision remains `USER_OBSERVED`; Demo 5 open residuals preserved; CURRENT/FPS/audio unchanged.

```text
USER_DEMO7_SCENE_MAPPING: PROVEN (stage 0, sub 8, glrecord 698 cnt 1245, map clears to 0 in body)
USER_DEMO7_MOTION_OWNER: GUEST_PHYSICAL_INTEGRATION_RETAINED_TSPD (ratio 1.00035; root writer UNKNOWN)
USER_DEMO7_ORACLE_STATIONARITY: UNKNOWN (oracle not yet run)
USER_DEMO5_RESIDUALS: OPEN_PRESERVED
RUNTIME_EXECUTION_THIS_TASK: NOT_RUN
NEXT_SINGLE_ACTION: See [[../../plan/evidence/p08-user-demo7-cutscene-drift-investigation]] (locate guest state bs=4 init/update/end and physics mode/target-speed writer at bs=2->bs=4 glidx 38, trace bounded stage 0 sub 8 glidx 35..60 without map==282 filter, compare Angrylion oracle where necessary before faithful fix; avoid full-run captures)
```

## T_INTRO production projection policy (2026-08-31)

The user visually confirmed the true-source-aspect policy on the tested 16:9
Scene A and Scene B paths. Tooie now applies it automatically at
`func_800E44FC`: source aspect is derived from the actual cinematic matrix as
`abs(P[1][1]/P[0][0])`, while RT64 global code remains unchanged. The policy
is T_INTRO-only, render-only, dynamic, and leaves 21:9+ authored extent open
for revalidation. Candidate A and the Candidate B environment flag were
removed after promotion.

```text
PRODUCTION_POLICY: true cinematic source-aspect compensation
SOURCE_ASPECT_HARDCODED: NO
TINTRO_TRUE_SOURCE_ASPECT_VISUAL_RESULT: VISUALLY_CONFIRMED_FOR_TESTED_16X9_SCENES
TINTRO_21X9_PLUS_FINAL_STATUS: OPEN_REVALIDATION_REQUIRED
PROJECTION_POLICY_REGRESSION_TEST: ADDED / PASS
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
```

## SUPERSEDED — T_INTRO projection source-aspect candidates (2026-08-31)

The prior RT64 investigation's high-confidence root is retained: the generic
workload path derives source aspect from VI `304/228`, then scales the
cinematic projection by `1/(4/3)` at a 16:9 target. This round implements two
default-off diagnostic policies in Tooie, at the native projection owner:

- `TOOIE_TEST_TINTRO_PROJECTION_ORIGINAL=1`: pre-cancel `K_auto`, preserving
  the guest projection.
- `TOOIE_TEST_TINTRO_TRUE_SOURCE_ASPECT=1`: derive source as
  `abs(P[1][1]/P[0][0])` and use `min(target, source)/(4/3)`, preserving the
  cinematic aspect until the target exceeds it.

The scope is mode 3 or MAP 18B only; gameplay is excluded. These are
render-only diagnostic candidates. RT64, `800CAA24`, `800CACEC`, `CAB70`,
geometry, and production defaults are unchanged. CPU witness and aspect-matrix
validation passed; the build and `window_display_test` passed (38/38).

```text
EXISTING_ASPECT_EXTENSION_API: PARTIAL
TOOIE_CAN_OVERRIDE_SOURCE_ASPECT_WITHOUT_RT64_CORE_CHANGE: YES
CANDIDATE_A_IMPLEMENTED: YES / OFF
CANDIDATE_B_IMPLEMENTED: YES / OFF
RENDER_ONLY_CHANGE: YES
DEMO_SEMANTIC_SIDE_EFFECT_RISK: LOW
USER_MANUAL_TEST_REQUIRED_NOW: YES
PREFERRED_USER_TEST_CANDIDATE: B
NEXT_SINGLE_ACTION: TOOIE_TEST_TINTRO_TRUE_SOURCE_ASPECT=1 ./build/TooieRecompiled
ARTIFACT_DIRECTORY: /tmp/p08-tintro-projection-candidate-20260831T034438Z/
ARTIFACT_CHECKSUM_MANIFEST: 665ae932cfe019b1cebd04fa6dc1f78c6b70f8d3388faee6c5522b1e0f7474ed
GPU_READBACK_USED: NO
```

## T_INTRO RT64 triangle translation (2026-08-31)

First numerical RT64 divergence for T_INTRO Scene-A frame-5 covering walls
is CLIP_X: Expand `adjustProjectionMatrix` scales cinematic
`viewProj[0][0]` `0.905148` → `0.678861` with `K=4/3` from VI `304/228`
versus window `16/9`. Guest RSP clip MATCHES golden. Guest presentation
envelope Original/Expand difference remains `0` / `REFUTED`. `±0.754` is
vertical letterbox, not horizontal crop.

```text
TINTRO_16X9_RT64_ROOT_CLASS: RT64_PROJECTION_TRANSLATION
ROOT_CONFIDENCE: HIGH
RT64_EXPAND_EQUATION: first column *= 1/K; K=4/3 AUTO
RT64_ORIGINAL_EXPAND_TRIANGLE_DIFFERENCE_COUNT: 5
RT64_TRANSFORM_CAN_EXPLAIN_LARGE_VISUAL_FAILURE: YES
CLIP_CLASSIFICATION_MATCH: NO
WITNESS_WINDING_MATCH: YES
EXPAND_FLIPS_ANY_WITNESS_WINDING: NO
GUEST_RT64_RASTER_STATE_MATCH: YES
SCENE_B_SHOWS_SAME_RT64_DIVERGENCE: YES
TINTRO_16X9_AUTHORED_EXTENT: STRONG_HYPOTHESIS
TINTRO_21X9_PLUS_AUTHORED_EXTENT: PROVEN
FIX_LAYER: TOOIE
FIX_SCOPE: TINTRO
CANDIDATE_IMPLEMENTED: diagnostic-only (TOOIE_DIAG_RT64_TINTRO_TRI=1)
NEXT_SINGLE_ACTION: Tooie-side default-off T_INTRO/cutscene candidate that prevents AUTO K=4/3 on cinematic viewProj; do not change RT64 globally
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
ARTIFACT_DIRECTORY: /tmp/p08-tintro-rt64-triangle-translation-20260831T032400Z/
ARTIFACT_CHECKSUM_MANIFEST: 4971dfafeea24b2c44d365c9df11350dbbcc4bb7f172991550de54731ff3f07a
GPU_READBACK_USED: NO
```

## T_INTRO presentation envelope reconciliation (2026-08-31)

The prior `16872` Original-vs-Expand owner count is
`REFUTED/SUPERSEDED`: it applied the diagnostic `vis_half≈0.754` to guest X.
The source of that value was the removed historical intro-wall probe, where it was derived
from presentation/projection aspect values. The actual T_INTRO scissor is
`(0,28)-(304,200)`, so guest X is full `[-1,+1]`; the `±0.754` interval is
the vertical/Y letterbox. RT64 Original and Expand both present the full
guest X range, with Original contain-fit pillarbox and Expand full-width.

```text
CORRECTED_ORIGINAL_EXPAND_OWNER_DIFFERENCE_COUNT: 0
OWNER_78_SURFACE_CLASS: UNKNOWN
B1_TRUE_NONWALL_MISMATCH_PIXEL_COUNT: 1764
B1_WALL_TO_WALL_REPLACEMENT_PIXEL_COUNT: 4224
SOURCE_TARGET_ASPECT_DELTA_PERCENT: 0.584795%
CAMERA_FRAMING_EXPOSES_AUTHORED_EXTENT: not promoted for 16:9 from invalid ±0.754
TINTRO_16X9_AUTHORED_EXTENT: STRONG_HYPOTHESIS
GUEST_CPU_RENDER_PATH_AS_ROOT: STRONGLY_WEAKENED
RT64_PRESENTATION_TRANSFORM_AS_ROOT: REFUTED
GPU_READBACK_USED: NO
```

Artifact: `/tmp/p08-tintro-presentation-reconciliation-20260831T025942Z/`
(manifest SHA `c08eeab9d38cf89be1c79b8184bb95e67b53ab3e703ad118901d856cb4dd6dc3`).

## T_INTRO trusted CPU RDP owner reference (2026-08-31)

The pinned local Angrylion RDP Plus CPU implementation now supplies an
offline owner-only replay. Its fixed-point edgewalker, subpixel coverage,
span eligibility, blender, and framebuffer callback are used directly; no
generic float rasterizer and no GPU readback.

Scene A frame 5: 156 environment triangles, 0 unresolved owner pixels,
60.82% wall ownership in the left 20%, and 19.31% in the right 20%. B-1 has
5988 actual wall/alternate overlap samples. The expected wall and alternate
are both write-eligible; the exact wall ID is overwritten by later eligible
draws, yielding final owners of wall-class draw 67 on 4224 samples and unknown
draw 78 on 1764. The exact-ID mismatch cause is `LATER_DRAW_WINS`, not Z,
alpha, or render-state rejection.

`GUEST_CPU_RENDER_PATH_AS_ROOT: STRONGLY_WEAKENED`. Original/Expand has
16872 presented-envelope differences in this replay (Original ±0.754 versus
Expand ±1.000) with identical guest clip input, so guest RDP ownership is
`REFUTED` as the widescreen root. Artifact:
`/tmp/p08-tintro-rdp-owner-reference-20260831T011600Z/` (manifest SHA
`dbf77934a31b2244a5b5af1568413a03c88a0ce823f3886f4b102a4fe9cbf83e`).

## T_INTRO visible surface ownership: Scene A vs B (2026-08-30)

The corrected default-off CPU diagnostic captured Scene-A frame 5 (156
environment triangles) and Scene-B equivalent frame 2409 (80). It now walks
task roots `0x80160cd0` and `0x80159600`, follows nested G_DL inheritance, and
decodes RDP E2/E3 SETOTHERMODE plus FC combiner state. B-1's wall and nearer
alternate share Z compare/update OFF and the same 2-cycle/bilinear/
texture-perspective/force-blend/alpha-none class; wall order is 48 versus
alternate order 13. No state-caused mismatch or exact RDP pixel owner is
proven because blender/coverage cannot be safely resolved CPU-side. No
candidate, geometry, or production fix. Artifact:
`/tmp/p08-tintro-render-state-owner-20260830T180800Z/` (manifest SHA
`3d342e999430f817d0333a539968f87935c5b66a848e29864e857edd6a1c3b14`).

## T_INTRO per-draw transform ownership (2026-08-30)

`TOOIE_DIAG_TINTRO_DRAW_PROVENANCE=1` (default OFF) walks `func_80013EF0`
GraphicsTask DLs with per-draw modelview/projection/segment state.

Scene-A first environment frame is **5**, not 3. G_DL `0x00123340` is
setup-only (`G_ENDDL`, no triangles). First env triangles: `0x0031f6d8`
(main volume, 20 in-frustum, L4/R10) and `0x0031f8f8` (materials) with
MV `0x8017b0b0` translation = camera `(529.3794, 6943.8896, -1904.1279)`.
`OPA7417_ZERO_TRIANGLE_ROOT: WRONG_RESOURCE + WRONG_MODEL_MATRIX`.
F3DEX2 `G_DL_PUSH=0` is a call. `REAL_HOLE_FIRST_DROP_STAGE: NO_CPU_DROP`.
Type-13 rejected leaves still do not own the visible hole. Scene B
replays the same camera/MV class but **different heap G_DLs**
(`0x0031f058`/`0x0031fa38`/`0x00234790`); AABB is not the compact ±2793
room. Shared cinematic, not shared draw identity. Next: dense CPU NDC
occupancy grid + content-stable ID. Do **not** widen CAB70, force
WALL7799, or add geometry. Artifact:
`/tmp/p08-tintro-per-draw-owner-20260830T193644Z/` (manifest
`88a9885d0abc13fb0e0bf3d12008058ee8c89e00404598876148644bb5a66fa9`).

## T_INTRO screen-edge owner audit (2026-08-30)

At the Scene-A opening (timeline frame 3; stable view-projection sample frame
315; camera `(529.3794, 6943.8896, -1904.1279)`), CPU extraction/clipping of
all 5254 OPA 7417 triangles leaves zero triangles inside the guest clip.
The six type-13 lateral leaves are LEFT rejects and remain offscreen (best
NDC xmax `-1.13195`). Therefore `800CAB70`/`800E3DC0` is not proven to own
the user-visible boundary, and no target-only OPA hole is proven at this
exact pose. Scene B has the same candidate boundary and reject result.

Decision: `MORE_EVIDENCE_REQUIRED`; Scene A and B visual root classes remain
open, with shared root only `STRONG_HYPOTHESIS`. Complexity is MEDIUM, so
route the next transform/model-traversal capture to Grok 4.6 Medium. The
single next action is CPU-only per-draw model-transform/display-list ownership
capture at the first user-visible hole frame, replayed in Scene B. Artifact:
`/tmp/p08-tintro-screen-edge-owner-20260830T190608Z/` (manifest
`1d9fad539b87a0c7ffd4fdeffa7840c1a97bf8f8ab148596d0a1660ce7f1f21b`).

## T_INTRO opening type-13 LEFT reject (2026-08-30)

SCENE_A/B opening camera (529.38, 6943.89, -1904.13): `func_800CAB70`
plane0 LEFT is the first reject for six lateral type-13 AABBs. Closest
leaf `0x8037beb8` dist +34.2265015. Guest aspect already 1.767.
`SOURCE_REJECT_TARGET_ACCEPT: NO`. Do **not** add
`TOOIE_TEST_INTRO_TYPE13_ASPECT`. Banjo GeoCmdD analog YES; do not copy
`TRUE`. Artifact `/tmp/p08-tintro-type13-reject-plane-20260830T184303Z/`.

## Two intro wall scenes separated (2026-08-30)

Do not call every cinematic “the intro”.

- `SCENE_A_FIRST_VISIBLE_AFTER_LAUNCH`: T_INTRO mode=3 map=0, first 3D
  after launch (epoch 1, frame 3), G_DL `0x00123340`, camera
  (529.38, 6943.89, -1904.13). HIGH severity. Map-struct is **0** (do
  not assign MAP_18B).
- `SCENE_B_NEW_GAME_INTRO`: post-gameselect replay of the same T_INTRO
  (frontend epoch 5, frame 2454). Same G_DL. Opens at the SCENE_A camera.

Prior WALL7759/7799 work belongs to **SCENE_A** (late first-play kOpenCam
~6946), not SCENE_B. Shared renderer/visibility: YES. Failure class both
`VISIBILITY_REJECT` (`func_800E3DC0`/`func_800CAB70`). DK64 internal-X
bound class: NO. No production fix. Artifact
`/tmp/p08-two-intro-scenes-20260830T181514Z/`.

## WALL7799 type-13 AABB TOP cull (2026-08-30)

Type-13 is `AABB_FRUSTUM_GATE` (`func_800E3DC0` @ `0x800E3DC0` →
`func_800CAB70`). WALL7799 (`0x819A8`, gfx 7783–7807) is original-rejected
at opening ~6954 by the 4-plane AABB (clip reconstruction first=TOP, not
X). `ORIGINAL_N64_WOULD_REJECT_7799: YES`. DK64 internal-X-bound class:
`NO`. `SOURCE_REJECT_TARGET_NEEDS: NO`. Do **not** add
`TOOIE_TEST_INTRO_7799_VIS`. Residual: sibling `0x819F8` AABB-accepts
with no G_DL until `first_rsp=11484`. Artifact
`/tmp/p08-intro-wall7799-gate-20260830T173445Z/`.
`INTRO_16X9_ROOT: UNKNOWN`. 21:9+ authored-extent stays separate.
Gameplay `800CAA24`/`800CACEC` stay production.

## Original-presentation envelope / triangle clip (2026-08-30)

CPU-clip of submitted WALL7759/7779/7644/SKY18 through live guest VP
(opening frame 6954). Guest projection is already 1.767:1. Guest scissor
is full-width, so N64 cinematic X is NDC ±1. The host Original vis_half
±0.754 X-crop is `INFERENCE` and not the N64 intro composition. Lower
left wall continues to x=-1. Upper left (y≥+0.15) is EMPTY without
WALL7799 (never G_DL, both aspects). Do **not** pillarbox ±0.754. Do
**not** extend geometry for 16:9. 21:9+ authored-extent stays separate.
Artifact `/tmp/p08-intro-original-presentation-envelope-20260830T163614Z/`.
Clamp16x9 and groups 81/84 stay `REFUTED`. Gameplay `800CAA24`/`800CACEC`
stay production.

## Prior — 16:9 intro wall DL submission / first-drop gate (2026-08-30)

`INTRO_16X9_AUTHORED_EXTENT_AS_ROOT` is `REFUTED/SUPERSEDED` for the
current user-visible 16:9 hole. Isolated `APP_FOLDER_PATH` 1600x900
Expand/Full vs Original shows the left-wall DLs (gfx 7759/7779/7644)
**submitted in both**. `DL_PRESENT_4_3_MISSING_16_9: NO`.
`INTRO_16X9_FIRST_DROP_STAGE: NO_CPU_DROP`. Opening-pose CPU clip of
those verts is the same cinematic ~1.767:1 matrix.
Host presented vis NDC is `[-1,1]` Expand vs `[-0.754,0.754]` Original.
`INTRO_16X9_ROOT: STRONG_HYPOTHESIS` (Expand presents that clip edge).
21:9+ authored-extent remains `PROVEN` and separate. Clamp16x9 and
groups 81/84 stay `REFUTED`. Artifact
`/tmp/p08-intro-16x9-dl-drop-20260830T160704Z/`. No geometry candidate.
Gameplay `800CAA24`/`800CACEC` stay production.

## Opening-edge real triangle binding (2026-08-30)

Groups 81/84 stay `REFUTED` as the opening-hole root. Live left edge is
OPA Gfx **7759-family WALL** (nested type-13 `dl_start=7721` under
parent 196, WRAP CI `0x020120E0`). Right edge is Gfx **18 sky/horizon
card** / clear; 4:3 right already has no near wall. XLU 7418 is a ROM
**alias** of OPA 7417. 16:9 still hits the left wall (one miss in 297
poses: frame 1255 top-left). `GEOMETRY_EXTENSION_NOW_PROVEN: NO`.
Artifact `/tmp/p08-intro-real-edge-owner-20260830T145138Z/`. Clamp16x9
stays removed. Gameplay `800CAA24`/`800CACEC` stay production.

## Prior — superseding group 81/84 ownership (2026-08-30)

CPU parse of OPA 7417 binds groups 81/84 to Gfx 2561–2585 (7 verts, 3
tris) and 2643–2691 (21 verts, 9 tris), CLAMP materials, reusable
combine state. Those leaves are **not** the live frustum-edge owners:
frame 6955 right horizon misses OPA even at 4:3. No
`TOOIE_TEST_INTRO_GEOMETRY_EXTEND` candidate. Class remains
`MORE_EVIDENCE_REQUIRED`. Artifact
`/tmp/p08-intro-geometry-extension-candidate-20260830T143035Z/`.
Clamp16x9 stays removed. Gameplay `800CAA24`/`800CACEC` stay production.

## Prior — live camera envelope (2026-08-30)

The historical `TOOIE_DIAG_INTRO_CAMERA=1` probe captured the T_INTRO cinematic on an isolated
1600x900 Expand/Clamp16x9 run. Camera owner is `D_8012D500` + `800CB124` +
host `func_800E44FC`. Live VFOV is **30.7071°** from viewport+0xF4.
`D_80127630` never showed map 395 (`mode=3 map=0`). The constructed alcove
pose was not occupied.

Constraining live descent frame 6955 at
`(-1243.81, 3682.93, -1254.03)` looking `(0.749, 0.007, -0.662)` is 4:3-safe
and fails 16:10/16:9/21:9/32:9 on the **right** against OPA type-3 groups
84/81. Live max safe aspect is **1.3363:1** / HFOV **40.30°**. Clamp16x9
remains `REFUTED_FOR_CURRENT_16X9` (cinematic `vp+0xE8` is already ~16:9).
Dynamic full-width camera adaptation is **NO**. Geometry extension is
feasible at MEDIUM complexity (Banjo structural analog HIGH) but vertex /
material / DL indices are still unbound. Class:
`MORE_EVIDENCE_REQUIRED`. Artifact:
`/tmp/p08-intro-live-camera-envelope-20260830T133604Z/`.

The previous native `func_800E44FC` Clamp16x9 candidate remains visually
**NO_CHANGE** and removed. Do not reimplement it. Gameplay `800CAA24` /
`800CACEC` stay production.

## Resultado esperado

O jogo renderiza corretamente pelo RT64, aceita teclado/controle e oferece um
launcher Linux suficiente para selecionar ROM e configurações essenciais.

## Estado corrente

**ATTRACT DEMO2 CRASH RESOLVED (2026-08-16):** deterministic SIGSEGV after DEMO2
([[../../plan/evidence/p08-attract-loop-after-demo2-crash]]) is RESOLVED and VALIDATED.
`INTRO → 335 → 278 → 314 → 395 → INTRO → 235 → 296` succeed; `bsrest_entrypoint_11`
calls `static_344_808003FC` directly without lookup collision. Level A fix validated.

**P08 ACTIVE INTEGRATION STATE:** intro/Jiggy/type-4/warp OK;
**visible attract/gameplay demo reproduced** (mode-5, map 335 Wooded
Hollow). Hybrid F' PATCH 4 is independently CLASS A. PATCH 4 is accepted.
The `func_800BCF28` binding fix is **ACCEPTED (verdict B)** by
[[../../plan/evidence/p08-bcf28-binding-fix-independent-review]]. The
`func_80013A7C` lookup-zero is **RESOLVED**
([[../../plan/evidence/p08-core1-ra-wrapper-fix]]).
**Render-submission matrix split RESOLVED and VALIDATED**
([[../../plan/evidence/p08-render-submission-matrix-fix]]).
The user right-only live-player report is owned by the empty
`func_80013B70` secondary entry. The Tooie-native implementation is accepted
at function level after focused independent re-review verdict A
([[../../plan/evidence/p08-stick-angle-thunk-fix-rereview]]). Manual WASD
runtime is **RESOLVED**. Full analog/gamepad remains **PENDING**. The
gameplay-demo player uses `glrecord` recorded pad through bastick; the
first-straight ~510 is **original slope-scaled physics**
([[../../plan/evidence/p08-demo-movement-precision-diagnosis]] §14) and
the first later analog transition (`glidx = 74..91`) is **internally faithful**
([[../../plan/evidence/p08-demo-movement-precision-diagnosis]] §15).
**MEASURED_ORIGINAL vs MEASURED_RECOMP (2026-08-16 / 2026-08-20):**
Hardware-faithful oracle validation (`ANGRYLION_CONTROL` = Mupen64Plus Pure
Interpreter + Angrylion RDP Plus + cxd4 LLE) proves: pad 100% on consumed
1..765, pre-egg control PASS, shot `glidx=322` position MATCH, camera angle
and ideal_yaw MATCH within sub-pixel float precision (delta +0.0030°),
maximum position XZ error across all 765 frames is < 0.89 units, and behavior
states are 100% bit-identical (0 state mismatches). The historical Rice
camctrl divergence at 385, ideal_yaw divergence, and post-egg motion drift
are **CORRECTED/SUPERSEDED** as **`R-MOTION-ORACLE`** (reference artifacts of
Rice Video HLE). See [[../../plan/evidence/p08-demo-post-egg-angrylion-motion-differential]].
Zero gameplay-meaningful motion divergence remains. Level R; no production fix.
The first user-confirmed demo shot (`glidx = 322` fire-egg) remains a
**CLASS G** collision-space miss versus the downhill digger in the earlier
recomp-only study
([[../../plan/evidence/p08-demo-movement-precision-diagnosis]] §16);
this task did not re-diagnose egg collision.
No fix is implemented. `USER_BUG_B_CINEMATIC_FACING` is
**CURRENTLY_NOT_REPRODUCIBLE_BY_USER**.
The earlier `USER_BUG_B` one-actor Level B classification is
**CORRECTED/SUPERSEDED** by an independent review: original-MIPS case 11
matches generated instruction-for-instruction, advances the return position,
and preserves yaw/`unk54`. Wooded Hollow sucoaster `chdiggerground` therefore
has original actor/state semantics, not a recompilation defect. See
[[../../plan/evidence/p08-forced-cinematic-facing-independent-review]].
`USER_BUG_B_GLOBAL` remains **OPEN**; no fix is implemented.
The selected node discriminator is **RESOLVED** (2026-08-16, DeepSeek V4
Flash Free): `func_80802760` returns node 0 (`B_LIVE_NODE`) on all 3438
captured calls and never `-1` (`A_KEEP_OLD` REFUTED); the 180° freeze is
produced by the return-leg dispatch case 11 (`u72=0x3008` -> `L_80800A3C`)
which moves the digger back without writing `unk54`/`rotation.y`. Original
return-leg MIPS is PROVEN for this case; `func_80802760` equivalence remains
UNKNOWN because that separate function was not compared.
The historical verdict C and its narrow missing `$at` side-effect finding
are preserved in
[[../../plan/evidence/p08-stick-angle-thunk-fix-independent-review]];
host→N64 pad conversion
remains bidirectional and is not reopened. Attract/cinematic facing is a
separate bug. **P08-T10 frontend input is VALIDATED / PASS**. The later
"direct Spiral Mountain / synthetic confirm → mode=1 FAIL" wording is
**CORRECTED/SUPERSEDED** / **REFUTED**
([[../../plan/evidence/p08-title-file-select-validation]]): confirm
warps to `MAP_A1_CS_INTRO` (161) / `mode=3`; the sequencer does not
inject the later START presses that skip the opening story cutscenes.
**P08-T03 is VALIDATED** ([[../../plan/evidence/p08-original-timing-4x3-baseline]]).

Reconstrução do Scheduler/Driver e Preempção: A cadeia completa de despacho
indireto de áudio e o gap estrutural de preempção foram provados:
- Hardware N64: Hardware interrupt preempta `OSThread 6` MAIN (Pri 20) imediatamente para `OSThread 4` AUDIO (Pri 50).
- `N64ModernRuntime`: Agendamento puramente cooperativo; o busy-wait puro em `func_800C2AB8` retém o token de execução e impede que mensagens externas acordem a thread AUDIO.
- **Causa validada (experimento limpo 2026-08-12):** strong override temporário
  de `func_800C2AB8` com `yield_self_1ms(rdram)` → AUDIO roda, `func_800205D0`
  limpa `table[2].word0`, `func_800C269C(7)==1`, `func_800C16A4(7)` dispensa o
  slot 7, `func_800C2AB8` retorna, attract progride. Patch revertido; resolvido
  permanentemente por PATCH 4.

Evidência corrente:

- [[../../plan/evidence/p08-attract-loop-after-demo2-crash]] (**RESOLVED and ACCEPTED** — DEMO2 attract SIGSEGV; Level A, verdict B)
- [[../../plan/evidence/p08-forced-cinematic-facing-diagnosis]] (historical USER_BUG_B one-actor diagnosis; corrected/superseded)
- [[../../plan/evidence/p08-forced-cinematic-facing-independent-review]] (**CURRENT:** original-MIPS case-11 contract; actor refuted, global open)
- [[../../plan/evidence/p08-demo-full-motion-sync-diagnosis]] (**CURRENT:** MEASURED_ORIGINAL vs MEASURED_RECOMP; capture-phase + later run split)
- [[../../plan/evidence/p08-demo-post-egg-motion-divergence]] (**CURRENT:** post-egg run/jump split; writer walkback now resolved)
- [[../../plan/evidence/p08-demo-ideal-yaw-writer-trace]] (**CURRENT:** exact ideal_yaw writer + input provenance; first divergent input = player-control camera)
- [[../../plan/evidence/p08-demo-camera-control-divergence]] (**CURRENT:** player-control camera writer/selector closed)
- [[../../plan/evidence/p08-demo-camera-e1a58-differential]] (E1A58 REFUTED)
- [[../../plan/evidence/p08-demo-camera-mode-producer-differential]] (**CURRENT:** mode 2 / func_80110588 PROVEN; 80110790 a1 differs at 384)
- [[../../plan/evidence/p08-demo-movement-precision-diagnosis]] (gameplay-demo residual; first straight + first analog PASS; original numeric now measured)
- [[../../plan/evidence/p08-stick-angle-thunk-fix]] (implementation evidence; accepted by focused re-review)
- [[../../plan/evidence/p08-stick-angle-thunk-fix-rereview]] (**CURRENT REVIEW** — verdict A; `$r1` delay-slot blocker resolved)
- [[../../plan/evidence/p08-stick-angle-thunk-fix-independent-review]] (historical verdict C; blocker resolved)
- [[../../plan/evidence/p08-player-direction-facing-diagnosis]] (Level A stick-angle diagnosis)
- [[../../plan/evidence/p08-directional-input-movement-diagnosis]] (host→pad bidirectional; not reopened)
- [[../../plan/evidence/p08-render-submission-matrix-fix]] (**CURRENT** — render matrix fix)
- [[../../plan/evidence/p08-render-submission-matrix-diagnosis]] (render matrix diagnosis)
- [[../../plan/evidence/p08-actor-camera-transform-discriminator]] (transform discriminator)
- [[../../plan/evidence/p08-visible-demo-movement-diagnosis]] (visible demo + movement diagnosis)
- [[../../plan/evidence/p08-core1-ra-wrapper-fix]] (core1 RA-wrapper fix; resolved)
- [[../../plan/evidence/p08-80013a7c-lookup-zero-diagnosis]] (Level A diagnosis; resolved)
- [[../../plan/evidence/p08-bcf28-binding-fix-independent-review]] (verdict B; BCF28 fix accepted)
- [[../../plan/evidence/p08-bcf28-binding-fix]] (implementer binding-fix evidence; preserved)
- [[../../plan/evidence/p08-bcf28-crash-diagnosis]] (root cause A diagnosis; preserved)
- [[../../plan/evidence/p08-preemption-patch4-clean-build-independent-review]] (verdict B; PATCH 4 accepted; tracked-state blocker resolved)
- [[../../plan/evidence/p08-preemption-patch4-clean-build-correction]] (implementer correction)
- [[../../plan/evidence/p08-preemption-patch4-independent-review]] (historical verdict C preserved; behavioral CLASS A; finding resolved)
- [[../../plan/evidence/p08-preemption-patch4-validation]] (implementer evidence; preserved)
- [[../../plan/evidence/p08-preemption-patch35-rcp-independent-review]] (verdict B; PATCH35 accepted)
- [[../../plan/evidence/p08-preemption-patch35-rcp-correction]] (implementer TLS RCP correction evidence)
- [[../../plan/evidence/p08-preemption-patch35-independent-review]] (pre-correction verdict C; finding resolved, history preserved)
- [[../../plan/evidence/p08-preemption-patch35-validation]] (implementer evidence; RCP/P3-X6 claims superseded)
- [[../../plan/evidence/p08-preemption-patch35-design]] (PATCH 3.5 design contract; implemented)
- [[../../plan/evidence/p08-preemption-patch35-thread-switch-review]] (thread-dispatch resume hook)
- [[../../plan/evidence/p08-preemption-patch3-independent-review]] (PATCH 3 review B)
- [[../../plan/evidence/p08-preemption-patch3-validation]] (PATCH 3 generator; T11 PASS; flag OFF)
- [[../../plan/evidence/p08-preemption-patch2-independent-review]] (PATCH 2 review B; inherited pre-PATCH4 blockers)
- [[../../plan/evidence/p08-preemption-patch2-validation]] (PATCH 2 runtime service)
- [[../../plan/evidence/p08-preemption-patch1-independent-review]] (PATCH 1 review B)
- [[../../plan/evidence/p08-preemption-patch1-validation]] (PATCH 1 implementer evidence)
- [[../../plan/evidence/p08-preemption-implementation-plan]] (spec Hybrid F'; PATCH 2 landed, PATCH 3+ pending)
- [[../../plan/evidence/p08-preemption-design-adversarial-review]] (arquitetura F' aprovada)
- [[../../plan/evidence/p08-permanent-preemption-design]] (DeepSeek Hybrid F, `CORRECTED/SUPERSEDED`)
- [[../../plan/evidence/p08-guest-preemption-causal-validation]]
- [[../../plan/evidence/p08-guest-preemption-runtime-audit]]
- [[../../plan/evidence/p08-slot7-scheduled-event-validation]]
- [[../../plan/evidence/p08-slot7-scheduled-event-driver]]
- [[../../plan/evidence/p08-slot7-object-lifecycle-arbitration]]
- [[../../plan/evidence/p08-session-handoff]]
- [[../../plan/evidence/p08-original-timing-4x3-baseline]] (**CURRENT:** P08-T03 baseline; blocked pending clean live measurements)
- [[../../plan/evidence/p08-attract-actor-loop-diagnosis]]
- [[../../plan/evidence/p08-type4-transition-semantics-audit]]

> **VALIDAÇÃO DINÂMICA CLEAN FREEZE (2026-08-12):** 3 clean freezes
> determinísticos confirmam **CLASS A**: o objeto de `table[2]` está agendado na
> delta-queue do SoundManager (head, delta 0, payload.p4 == table[2].word0),
> handler `mgr+0x08 = func_8001ED68` (MATCH), `func_8001EE34` pending; a thread
> `[Game] AUDIO` está estacionada em `LightweightSemaphore::waitWithPartialSpinning`
> e os contadores de áudio ficam congelados no freeze → o dispatch não pode rodar
> enquanto MAIN gira em `func_800C2AB8`. Ver
> [[../../plan/evidence/p08-slot7-scheduled-event-validation]].


## Tarefas

- [x] `P08-T01` Integrar RT64 e validar Vulkan nesta máquina.
- [x] `P08-T02` Catalogar display lists/efeitos Tooie; scheduler DP/VI OK ([[../../plan/evidence/p08-display-effect-catalog]]).
- [x] `P08-T03` Reproduzir 4:3 e timing originais sem melhorias ativas
  ([[../../plan/evidence/p08-original-timing-4x3-baseline]]). 60Hz VI, 20/30Hz logic,
  30Hz animação, desacoplamento do host gfx e aspecto 4:3 validados ao vivo.
- [x] `P08-T04` Integrar path SDL/RecompFrontend do BanjoRecomp.
- [x] `P08-T05` Defaults/persistência BanjoRecomp; `controls.json` lazy.
- [x] `P08-T06` Esconder launcher no auto-start e título Tooie.
- [x] `P08-T07` Janela, fullscreen, foco, multi-monitor e escalas (`tooie_window` helpers + SDL/RT64 path; manual multi-monitor/fullscreen validation pending).
- [x] `P08-T08` Fallback/mensagem para driver/API incompatível (`tooie_error` reporting & negative tests PASS).
- [x] `P08-T09` Evidência visual: intro/Jiggy OK; **first demo visible & upright**
  (render submission matrix split resolved via `transform_hooks.cpp`).
- [x] `P08-T10` Input title/file select VALIDATED
  ([[../../plan/evidence/p08-title-file-select-validation]]).
  `P08_T10_FRONTEND_INPUT=PASS`. Direct Spiral Mountain claim
  **CORRECTED/SUPERSEDED**. Current synthetic `mode=1` FAIL
  **REFUTED**.
- [x] `P08-T11` Máquina de estados da abertura: type-4+warp OK; **C2AB8
  RESOLVED by PATCH 4** (CLASS A ON/OFF). Hybrid F' production ON for
  Tooie. **`func_800BCF28` RESOLVED and accepted** (verdict B).
  **`func_80013A7C` core1 RA wrappers RESOLVED**.
  **Render-submission matrix split RESOLVED and VALIDATED** ([[../../plan/evidence/p08-render-submission-matrix-fix]]).


## Implementado / validado

### Frontend, render e áudio

- RT64 + Vulkan, auto-start, janela “Banjo-Tooie: Recompiled”.
- Música inicial validada.
- Input host equivalente ao BanjoRecomp. **P08-T10 VALIDATED / COMPLETE**
  for frontend input: intro -> file select -> slot navigation -> `CONT_A`
  confirm PASS. Post-confirm destination is `MAP_A1_CS_INTRO` (161) /
  `mode=3`. Direct Spiral Mountain and later synthetic `mode=1` FAIL
  claims are **CORRECTED/SUPERSEDED** / **REFUTED**
  ([[../../plan/evidence/p08-title-file-select-validation]]).
- **P08-T03 VALIDATED / COMPLETE**: Source-level NTSC timing contract and
  4:3 guest dimensions are established and validated in
  [[../../plan/evidence/p08-original-timing-4x3-baseline]]. 60Hz VI, 20/30Hz logic,
  30Hz animation, and host presentation decoupling are confirmed.
- **Top-left present FPS counter & settings toggle**: "Show Present FPS" added to `add_graphics_options` (`src/game/config.cpp`), default false, persisted in `graphics.json`. Reuses RecompFrontend render-thread overlay update hook `recompui::set_overlay_update_callback` without parallel UI. Counts actual successful `swapChain->present()` calls (`RT64::presentSuccessCount`). Verified by host unit test `tooie_fps_meter_test` PASS exit 0; patches `patches/frontend/` and `patches/rt64/` reverse-checked; runtime QA pending ([[../../plan/evidence/p09-fps-counter-and-settings-toggle]]).

### Blockers históricos fechados / DO-NOT-REOPEN

| Item / Tópico | Classificação Canônica | Evidência de Fechamento |
|:---|:---|:---|
| **Attract DEMO2 crash / same-section .bsrest** | **RESOLVED (Level A, verdict B)** | [[../../plan/evidence/p08-attract-loop-after-demo2-crash]] |
| **N64Recomp CreateStatic generator fix** | **RESOLVED (Level A)** | `patches/runtime/n64recomp-tooie.patch` |
| **Render-submission matrix split** | **RESOLVED (Level A)** | [[../../plan/evidence/p08-render-submission-matrix-fix]] |
| **BCF28 helper binding / null dereference** | **RESOLVED (Level A, verdict B)** | [[../../plan/evidence/p08-bcf28-binding-fix-independent-review]] |
| **Core1 RA wrappers / LOOKUP_FUNC(0)** | **RESOLVED (Level A)** | [[../../plan/evidence/p08-core1-ra-wrapper-fix]] |
| **PATCH 4 / guest preemption** | **RESOLVED (CLASS A / verdict B)** | [[../../plan/evidence/p08-preemption-patch4-clean-build-independent-review]] |
| **P08-T03 original timing / 4:3 baseline** | **RESOLVED (Level A)** | [[../../plan/evidence/p08-original-timing-4x3-baseline]] |
| **P08-T10 title & file-select input** | **RESOLVED (Level A)** | [[../../plan/evidence/p08-title-file-select-validation]] |
| **func_80013B70 stick angle thunk** | **RESOLVED (verdict A)** | [[../../plan/evidence/p08-stick-angle-thunk-fix-rereview]] |
| **First-straight ~510 slope-scaled physics** | **RESOLVED (Level A)** | [[../../plan/evidence/p08-demo-movement-precision-diagnosis]] §14 |
| **First analog transition (glidx 74..91)** | **RESOLVED (Level A)** | [[../../plan/evidence/p08-demo-movement-precision-diagnosis]] §15 |
| **Rice Z-buffer differential** | **REFUTED / REFERENCE_ARTIFACT (R-ORACLE)** | [[../../plan/evidence/p08-demo-zbuffer-angrylion-oracle]] |
| **Projection record +0x34 Rice differential** | **REFUTED / REFERENCE_ARTIFACT (R-ORACLE)** | [[../../plan/evidence/p08-demo-zbuffer-angrylion-oracle]] |
| **Post-egg gl385/gl392 motion drift** | **REFUTED / REFERENCE_ARTIFACT (R-MOTION-ORACLE)** | [[../../plan/evidence/p08-demo-post-egg-angrylion-motion-differential]] |
| **Collision/raycast interpretation of func_800C4C34** | **REFUTED (viewport/depth clipping helper)** | [[../../plan/evidence/p08-demo-camera-collision-sol-review]] |
| **func_800E1A58 camera differential** | **REFUTED (identity)** | [[../../plan/evidence/p08-demo-camera-e1a58-differential]] |
| **Z-buffer camera classifier attribution** | **RESOLVED (R-ORACLE)** | [[../../plan/evidence/p08-demo-zbuffer-angrylion-oracle]] |
| **Historical Rice camctrl -> ideal_yaw -> motion chain** | **REFUTED / REFERENCE_ARTIFACT (R-MOTION-ORACLE)** | [[../../plan/evidence/p08-demo-post-egg-angrylion-motion-differential]] |
| **USER_BUG_B `chdiggerground` actor-specific defect** | **REFUTED (original MIPS case 11 contract)** | [[../../plan/evidence/p08-forced-cinematic-facing-independent-review]] |
| **USER_BUG_B_GLOBAL (cinematic facing)** | **OPEN / CURRENTLY_NOT_REPRODUCIBLE_BY_USER** | [[../../plan/evidence/p08-forced-cinematic-facing-independent-review]] |

## Evidências P08

### Revisão independente 0x800979D8 (2026-08-22)

[[../../plan/evidence/p08-user-demo4-800979d8-independent-review]] confirma
por disassembly do ELF original que o helper `0x800979D8` é lógica real de
`bainput`. O mapeamento Tooie está registrado e o teste focalizado passa.
[[../../plan/evidence/p08-user-demo4-bainput-held-state-provenance]] junta o
evento C-Right glidx 39: pad `0001` e `prev_state[13]=0` **MATCH**; AN
retorna `v0=2` e grava `p60.C=1/E=2/+8=5`; RT retorna `0`. Probe `held=` é
`prev_state`, não `bakey_held`. Gemini “pad 0001 == held C-Right” é
**CORRECTED/SUPERSEDED**. `ROOT_CLASS=A-CANDIDATE-HELPER`. Sem edição de
produção; mapping pendente preservado.

| evidência | estado/finalidade |
| --- | --- |
| [[../../plan/evidence/p08-user-demo1-movement-and-fire-egg-differential]] | **CURRENT:** USER_DEMO1 movement/action parity & Fire Egg discriminator; paired first-writer capture valid 415 rows / 56 updrec (exit 0/0); record +0x60 differs PRE80 (0004006c vs 0003006c; role UNKNOWN); RNG first diff PRE82; geometry first diff POST83 (norm 0.208); tracked other actor 80132f30, shot candidate 80132f60 separate; no Demo 1 fix |
| [[../../plan/evidence/p08-user-demo4-bainput-held-state-provenance]] | **CURRENT:** USER_DEMO_4 C-Right pad MATCH; first split is native `0x800979D8` return 0 vs 2 (`A-CANDIDATE-HELPER`) |
| [[../../plan/evidence/p08-attract-loop-after-demo2-crash]] | **RESOLVED and ACCEPTED:** DEMO2 attract SIGSEGV; bsrest 0x808003FC resolved via CreateStatic (Level A, verdict B) |
| [[../../plan/evidence/p08-forced-cinematic-facing-diagnosis]] | historical USER_BUG_B one-actor diagnosis; corrected/superseded |
| [[../../plan/evidence/p08-forced-cinematic-facing-independent-review]] | **CURRENT:** original-MIPS case-11 contract; chdiggerground actor REFUTED; global USER_BUG_B OPEN |
| [[../../plan/evidence/p08-demo-movement-precision-diagnosis]] | first straight + first analog PASS; first demo fire-egg glidx 322 target collision volume resolved (TRUE_MISS_MARGIN = +36.85f, PROVEN NO HIT) |
| [[../../plan/evidence/p08-demo-ideal-yaw-writer-trace]] | **CURRENT:** exact ideal_yaw writer (0x8009C930) + input provenance; first divergent input = player-control camera |
| [[../../plan/evidence/p08-demo-camera-control-divergence]] | player-control camera writer/selector closed (Level B) |
| [[../../plan/evidence/p08-demo-camera-e1a58-differential]] | E1A58 REFUTED (identity) |
| [[../../plan/evidence/p08-demo-camera-mode-producer-differential]] | **CURRENT:** mode 2 / func_80110588 PROVEN; 80110790 a1 already differs at consumed 384 |
| [[../../plan/evidence/p08-demo-db-target-writer]] | **CURRENT:** D_8007BCC4 304x228 Z-buffer target writer identification; RT64 copyNativeToRAM depth writeback (B-WRITER) |
| [[../../plan/evidence/p08-demo-zbuffer-oracle-sol-review]] | independent Z-buffer oracle-validity review (C-ORACLE) |
| [[../../plan/evidence/p08-demo-zbuffer-angrylion-oracle]] | **RESOLVED:** Angrylion RDP Plus + CXD4 LLE hardware oracle validation (R-ORACLE) |
| [[../../plan/evidence/p08-demo-post-egg-angrylion-motion-differential]] | **RESOLVED:** post-egg motion revalidation under Angrylion oracle (R-MOTION-ORACLE) |
| [[../../plan/evidence/p08-stick-angle-thunk-fix]] | implementation evidence; accepted by focused re-review |
| [[../../plan/evidence/p08-stick-angle-thunk-fix-independent-review]] | **CURRENT REVIEW:** verdict C; missing `$r1` delay-slot side effect |
| [[../../plan/evidence/p08-player-direction-facing-diagnosis]] | Level A empty 13B70 diagnosis; fix implemented |
| [[../../plan/evidence/p08-directional-input-movement-diagnosis]] | host→pad bidirectional; not reopened |
| [[../../plan/evidence/p08-render-submission-matrix-fix]] | **CURRENT:** Tooie render-submission matrix split validated |
| [[../../plan/evidence/p08-render-submission-matrix-diagnosis]] | RT64 LOAD-path viewproj decompose (resolved by the fix) |
| [[../../plan/evidence/p08-visible-demo-movement-diagnosis]] | visible demo reproduced; movement diagnosis |
| [[../../plan/evidence/p08-core1-ra-wrapper-fix]] | core1 RA-wrapper native fix (resolved) |
| [[../../plan/evidence/p08-80013a7c-lookup-zero-diagnosis]] | Level A diagnosis; resolved by the fix |
| [[../../plan/evidence/p08-bcf28-binding-fix-independent-review]] | verdict B; BCF28 helper binding fix accepted |
| [[../../plan/evidence/p08-bcf28-binding-fix]] | implementer binding-fix evidence (preserved) |
| [[../../plan/evidence/p08-bcf28-crash-diagnosis]] | root cause A diagnosis (preserved) |
| [[../../plan/evidence/p08-preemption-patch4-clean-build-independent-review]] | verdict B; PATCH 4 accepted; tracked-state blocker resolved |
| [[../../plan/evidence/p08-preemption-patch4-independent-review]] | historical verdict C preserved; CLASS A behavior; finding resolved by clean-build review |
| [[../../plan/evidence/p08-preemption-patch4-validation]] | implementer evidence; history preserved |
| [[../../plan/evidence/p08-preemption-patch35-independent-review]] | historical verdict C; RCP finding resolved |
| [[../../plan/evidence/p08-preemption-patch35-validation]] | implementer evidence; PATCH4 gate superseded |
| [[../../plan/evidence/p08-preemption-patch35-design]] | PATCH 3.5 design contract (implemented) |
| [[../../plan/evidence/p08-preemption-patch35-thread-switch-review]] | thread-switch eligibility; verdict B; resume hook |
| [[../../plan/evidence/p08-preemption-patch3-independent-review]] | PATCH 3 independent review B |
| [[../../plan/evidence/p08-preemption-patch3-validation]] | PATCH 3 generator; T11 PASS; flag OFF |
| [[../../plan/evidence/p08-preemption-patch2-independent-review]] | PATCH 2 independent review B; pre-PATCH4 blockers |
| [[../../plan/evidence/p08-preemption-patch2-validation]] | PATCH 2 runtime service |
| [[../../plan/evidence/p08-preemption-patch1-independent-review]] | PATCH 1 independent review B |
| [[../../plan/evidence/p08-preemption-patch1-validation]] | PATCH 1 scaffold + T01 PASS |
| [[../../plan/evidence/p08-preemption-implementation-plan]] | spec file/function + T01–T15; PATCH 2 landed |
| [[../../plan/evidence/p08-preemption-design-adversarial-review]] | arquitetura F' aprovada (veredito C) |
| [[../../plan/evidence/p08-permanent-preemption-design]] | DeepSeek Hybrid F; `CORRECTED/SUPERSEDED` |
| [[../../plan/evidence/p08-guest-preemption-causal-validation]] | experimento limpo class A — gap de preempção provado causalmente |
| [[../../plan/evidence/p08-guest-preemption-runtime-audit]] | auditoria estrutural do gap (Gemini) |
| [[../../plan/evidence/p08-slot7-scheduled-event-validation]] | validação dinâmica class A (evento agendado+due, dispatch AUDIO estacionado) |
| [[../../plan/evidence/p08-slot7-scheduled-event-driver]] | reconstrução estática do scheduler/driver |
| [[../../plan/evidence/p08-slot7-object-lifecycle-arbitration]] | arbitragem Gemini×DeepSeek; struct/driver/order |
| [[../../plan/evidence/p08-session-handoff]] | **CURRENT:** handoff para chat zerado |
| [[../../plan/evidence/p08-original-timing-4x3-baseline]] | **CURRENT:** P08-T03 original timing/4:3 baseline; BLOCKED pending clean live measurements |
| [[../../plan/evidence/p08-display-effect-catalog]] | **CURRENT:** P08-T02 catálogo de display lists e efeitos gráficos |
| [[../../plan/evidence/p08-demo-full-motion-sync-diagnosis]] | **CURRENT:** diagnóstico diferencial de sincronização e movimento da demo de gameplay |
| [[../../plan/evidence/p08-attract-actor-loop-diagnosis]] | freeze mechanism slot7 / C2AB8 |
| [[../../plan/evidence/p08-visual-validation/README]] | frames humanos |
| [[../../plan/evidence/p08-widescreen-edge-geometry]] | **CURRENT:** first Scene-A env owner 0x0031f6d8/0x0031f8f8 frame 5; OPA7417/0x00123340 REFUTED as visible owner; NO_CPU_DROP; 800CAA24/800CACEC production ON |
| demais p08-* | resolved/deferred (ver handoff) |

## Fluxo esperado

```text
intro -> Jiggy -> gsattract -> map 335/entrance 8 -> type-4 complete
 -> warp 3 -> A9924 -> A76F4(5) -> mode 5 -> A9A14 -> gsattract ep1
 -> gameplay demo
```

## P08 unnamed same-section JAL fix — independent acceptance (2026-08-16)

Independent review result: `FIX_VERDICT=B`, `FIX_ACCEPTED=YES`, and the old
DEMO2/CCL SIGSEGV blocker is closed. The B classification records the test-only
hardening and the fact that this host could not reproduce the transition runtime
under Xvfb/lavapipe; it does not identify a generator blocker.

Verified:

- exact base `1f2aba14665c064ce5af829955f1ac089792de77`, patch base/hash contract,
  and active durable ownership in `patches/runtime/n64recomp-tooie.patch`;
- `resolve_jal` policy cases preserved: named same-section direct, unnamed
  same-section `CreateStatic`, cross-section lookup semantics, and non-relocatable
  host-hook lookup semantics;
- all seven section-344 CreateStatic boundaries, original-MIPS helper semantics,
  17/17 direct call-site mappings, 7/7 generated bodies, and zero vulnerable
  lookups after clean regeneration;
- live generator unit test 4/4 cases, all five native tests, and `TooieRecompiled`
  build: PASS;
- regression test strengthened only: exact 17-call mapping, exact section-344
  helper definitions, and zero wrong/missing/lookup calls are now required.

The independent 30-second software-Vulkan/Xvfb run reached `T_INTRO`, emitted no
SIGSEGV, and timed out before CCL. The Radeon/Xvfb path was unavailable because
of missing DRI3 presentation. The documented 300-second Vulkan soak remains
preserved as historical implementation evidence.

```text
ROOT_CAUSE_LEVEL:                           A
FIX_OWNER:                                  N64RECOMP
FIX_IMPLEMENTED:                            YES
FIX_ACCEPTED:                               YES
ATTRACT_DEMO2_CRASH:                        RESOLVED
CREATESTATIC_BOUNDARIES:                    7/7 PASS
AUDITED_DIRECT_CALLS:                       17/17
VULNERABLE_LOOKUPS:                         0
MARKDOWN_VAULT_AUDIT:                       PASS (0 errors; pre-existing warnings/tasks)
NEXT_SINGLE_ACTION:                         return to the next P08 residual
```

## Estado observado (clean)

```text
... warp consumido, A76F4(5) ...
 -> C2AB8 returns (PATCH 4 Hybrid F')
 -> gsworldDll / basetup
 -> func_800BCF28 completes cleanly
 -> func_80013A7C native body entered; old LOOKUP_FUNC(0) absent
 -> 150s continuation at opening mode=3 / map=0 (not diagnosed)
```

## Residual prioritário

1. **USER_BUG_B `chdiggerground` actor — REFUTED (original-MIPS review,
   2026-08-16)** ([[../../plan/evidence/p08-forced-cinematic-facing-independent-review]]).
   The original case-11 return path advances position while preserving
   `Actor.rotation.y` / `unk54`; no fix is authorized. `USER_BUG_B_GLOBAL`
   remains OPEN for another user-confirmed cinematic actor.
2. **Demo-path attract demo movement & camera — RESOLVED (2026-08-20, R-MOTION-ORACLE)**
   ([[../../plan/evidence/p08-demo-post-egg-angrylion-motion-differential]]).
   Hardware oracle (Angrylion RDP Plus + CXD4 LLE) confirms complete parity
   across all 765 demo frames (XZ max error < 0.89 units, 0 behavior-state
   mismatches). Historical Rice camctrl/motion drift is REFUTED as an emulator
   artifact.
3. **P08 acceptance gates** — Physical display validation is **PASS** (`FORMAL_DISPLAY_CHECKLIST_PROVEN: YES`, 14/14 mandatory items passed). Physical gamepad checklist is **PENDING_HARDWARE_UNAVAILABLE** due to temporary hardware unavailability (`PHYSICAL_GAMEPAD_RESULT: PENDING_HARDWARE_UNAVAILABLE`, `USER_CONFIRMED_CONTROLLER_GAMEPLAY: YES` retained). The `0x800979D8` held-input/P60 downstream gate is also **NOT_PROVEN**; see the adversarial review.
4. **File-select confirm transition — RESOLVED / REFUTED**
   ([[../../plan/evidence/p08-title-file-select-validation]]). Post-confirm destination
   is `MAP_A1_CS_INTRO` (161) / `mode=3` (opening story cutscenes). The synthetic
   `mode=1` regression claim was refuted as the sequencer did not inject the START
   skips that bypass opening story cutscenes.
5. Generic non-RA JR safepoints, Timer/PI approximations — deferred.
6. Origem do objeto `table[2]` (intro/attract/handoff) — permanece UNKNOWN;
   PATCH 4 direct word-zero label is INFERENCE.

## Gate de saída

- [x] Abertura até Jiggy/type-4/warp; C2AB8 behavior CLASS A; clean tracked
  Clang build PASS; PATCH 4 distribution independently accepted (verdict B).
- [x] `func_800BCF28` binding fix implementado, validado e aceito (verdict B).
- [x] `func_80013A7C` lookup-zero resolvido (Tooie-native RA wrappers).
- [x] RT64 intro OK; first demo visible; Wooded Hollow render-submission fix validated.
- [x] mode 5 / gsattract ep1 / gameplay demo renderizam (upright after matrix split).
- [x] Entrada title/file select validada; rumble/hotplug remains separate.
- [x] P08-T03 original timing/4:3 baseline validado e desacoplado da janela.
- [x] P08-T07 window, fullscreen, focus, multi-monitor, scale/DPI helpers implemented (`tests/window_display_test.cpp`, 38/38 PASS; physical display validation PASS by user confirmation; physical gamepad validation pending hardware availability).
- [x] P08-T08 graphics driver / Vulkan fallback error handling validado (`tests/graphics_failure_test.cpp`, 25/25 PASS; negative env test PASS).
- [x] Persistência BanjoRecomp; `controls.json` lazy.
- [x] Sem melhorias que alterem timing/paridade de referência.

## Próxima etapa

**P08 remains ACTIVE (MANUAL_VALIDATION_PENDING under Verdict B).**
- **Implemented Scope**: RT64 presentation baseline, input mapping, window/scale integration, graphics/Vulkan error handling, launcher music preview, static call correctness, attract demo hardware-accurate motion sync.
- **Physical Validation Status**: Physical display checklist is **PASS** (`FORMAL_DISPLAY_CHECKLIST_PROVEN: YES`, 14/14 mandatory items passed). Physical gamepad checklist is **PENDING_HARDWARE_UNAVAILABLE** (`USER_CONFIRMED_CONTROLLER_GAMEPLAY: YES` retained). The held-input/P60 dynamic gate for `0x800979D8` is also **NOT_PROVEN**.
- **Provisional Pre-P09 Evidence**: **CORRECTED/SUPERSEDED**. User-continuous attract numbering, `EGG_PARITY_DEFECT: PROVEN`. Independent of ROUND.W.S: **YES**. Independent of remaining camera/demo split: **UNKNOWN**. USER_DEMO_4 follow-camera **REPRODUCED**. Bind-tick yaw `round.w.s` is POST-FIX MATCH vs ANGRYLION_CONTROL (`5854872c79a7cab1c9395ddcd173cece449ffe11`; [[../../plan/evidence/p08-user-demo4-sincos-semantics-fix]]). Idle follow-cam glidx 1..38 is bit-identical. `CAMERA_COMPLETE_FIX: NO`. `CAMERA_FEEDBACK_ACCUMULATION: STILL_PRESENT`. Remaining first difference is dump glidx 39 walking-player `ncbapivot` jumptable `80112C3C` on `*(node+0x60)+8` (0 vs 5 at enter; [[../../plan/evidence/p08-user-demo4-walkstart-ncbapivot-discriminator]]). `HAUNTED_CAVERN_NO_FIRE_EVENT` is **REFUTED**. P09 remains **PENDING / NOT_FORMALLY_OPEN**.
- **Next Stage**: [[09-gameplay-parity-and-enhancements]] remains **PENDING / NOT_FORMALLY_OPEN** until the P08 manual gamepad gate is complete.
- **P08 / P09 Invariants (Protected)**: attract DEMO2 crash, bsrest 0x808003FC, N64Recomp CreateStatic fix, first-straight ~510, fire-egg miss, cinematic facing, stick-angle 13B70, render-submission matrix, BCF28, core1 RA wrappers, PATCH 4 / preemption, P08-T10, original timing/4:3 baseline, Angrylion Z-buffer oracle (R-ORACLE), Angrylion attract demo motion sync (R-MOTION-ORACLE). Oracle checkout/run: [[../../specs/BT/angrylion-control-oracle]] (`tools/oracle/angrylion-control/run.sh`).

## P08 Implementation Status (2026-08-21, Gemini 3.7 Flash HIGH)

- **P08-T07**: Window sanitization, aspect ratio contracts, and display validation helpers implemented via `tooie_window` module and verified via `tests/window_display_test.cpp` (38/38 PASS). Fullscreen/focus/event handling verified on reference SDL/RT64 path. Physical display validation is **PASS** by explicit user confirmation. Physical gamepad validation remains **PENDING_HARDWARE_UNAVAILABLE**.
- **P08-T08**: Graphics driver, Vulkan, and window initialization error handling validated via `tooie_error` module, `tests/graphics_failure_test.cpp` (25/25 PASS), and negative environment tests (`VK_ICD_FILENAMES=/nonexistent.json` exits code 1 with clean diagnostics and no crash).
- **Canonical Tasks**: All 11 canonical tasks audited:
  - `P08-T01`: **DONE**
  - `P08-T02`: **DONE** ([[../../plan/evidence/p08-display-effect-catalog]])
  - `P08-T03`: **DONE** ([[../../plan/evidence/p08-original-timing-4x3-baseline]])
  - `P08-T04`: **DONE**
  - `P08-T05`: **DONE**
  - `P08-T06`: **DONE**
  - `P08-T07`: **IMPLEMENTED_MANUAL_VALIDATION_PENDING** (Display PASS; Gamepad PENDING_HARDWARE_UNAVAILABLE; [[../../plan/evidence/p08-final-physical-validation]])
  - `P08-T08`: **DONE** ([[../../plan/evidence/p08-t07-t08-frontend-system-validation]])
  - `P08-T09`: **DONE** ([[../../plan/evidence/p08-render-submission-matrix-fix]])
  - `P08-T10`: **DONE** ([[../../plan/evidence/p08-title-file-select-validation]])
  - `P08-T11`: **DONE** ([[../../plan/evidence/p08-attract-loop-after-demo2-crash]])
- **Evidence Reference**: [[../../plan/evidence/p08-t07-t08-frontend-system-validation]], [[../../plan/evidence/p08-final-physical-validation]].

## P08 measured post-egg movement discriminator (2026-08-17)

The current measured-original/recomp differential isolates a Level C residual,
without production changes: `ideal_yaw` first differs at glidx 385 and position
first exceeds 2 units at glidx 392 under `ORIGINAL[N] vs RECOMP[N+1]`. Pads are
`765/765`, `bs=21`, `tspd=700`, and slope multiplier `1.0000` match at the
persistent split. The exact writer PC is UNKNOWN; P08 stays active and P09 is
not started. See [[../../plan/evidence/p08-demo-post-egg-motion-divergence]].

**Ideal-yaw writer differential RESOLVED (2026-08-17):** a bounded original
(rebuild of the local official Pure Interpreter with a store tracer) / Recomp
(GDB writer capture) store trace proves the exact writer is identical and the
writer input already differs at glidx 385.

```text
ORIGINAL_IDEAL_WRITER_PCS:        {0x8009C930}  (yaw_setIdeal store)
RECOMP_IDEAL_WRITER_PCS:          {0x8009C930}  (same generated function)
WRITER_PC_MATCH:                  YES
IDEAL_YAW_WRITER_FUNCTION:        yaw_setIdeal (0x8009C914)
CALLER_FUNCTION:                  func_8009D088 (case 1; jal at 0x8009D0EC)
WRITER_INPUT_ALREADY_DIFFERS:     YES — glidx 385 a1 = 99.3756 vs 99.3081
RAW_PAD_MATCH:                    YES
REMAP_MATCH:                      YES
STICK_MAG_MATCH:                  YES
STICK_ANGLE_MATCH:                YES (bang identical at every glidx)
PLAYER_CONTROL_CAMERA_MATCH:      NO (first differs glidx 385)
PLAYER_CONTROL_CAMERA_SOURCE:     D_80127708[*(ps+0x184)*4] + 0x1C
SAME_AS_PREVIOUS_DISPLAY_CAMERA:  NO (display D_8012D500 departs at 386)
CAMERA_DIVERGENCE_REL_TO_PLAYER:  UPSTREAM_OF_PLAYER (CORRECTED/SUPERSEDED)
PRIMARY_CLASS:                    SAME_WRITER_DIFFERENT_INPUT
                                  (first divergent input = CONTROL_CAMERA_INPUT)
ROOT_CAUSE_LEVEL:                 B
FIX_IMPLEMENTED:                  NO
```

The stick path (`bastick_update → func_80013B70`) is REFUTED as the first
divergent input. The remaining discriminator is the camera-control computation
that first writes the divergent player-control camera angle. See
[[../../plan/evidence/p08-demo-ideal-yaw-writer-trace]].

Independent GPT-5.6 Luna review accepts this provenance **WITH CORRECTIONS**:
the writer and formula are verified, but `WRITER_SET_COMPLETE` and
`ORIGINAL_INDEX_MATCH_RECOMP` remain `UNKNOWN`, and no direct dependency
between the player-control source and display camera is proven. The next
single discriminator is an aligned `(ps, ps+0x184, D_80127708[index],
selected+0x1C)` trace with all store widths/PCs to the selected `+0x1C` field.
No production fix is authorized.

## P08 player-control camera divergence — RESOLVED to Level B (2026-08-17)

The aligned selector/store-set/internal-state trace
([[../../plan/evidence/p08-demo-camera-control-divergence]]) proves:
`ps+0x184` (camera-control slot selector) is `0` in both runtimes at every
sample; the selected `D_80127708[0]` record is the same logical camera-control
object; the camctrl store set is complete (`{0x800CA65C}` =
`func_800CA628`, no alias/block write); the camctrl writer input first
diverges at glidx 385 (`-0.0676` deg); and the camera delta maps 1:1 onto the
ideal_yaw delta.

```text
SELECTOR_MATCH:                 YES  (ps+0x184 == 0, both runtimes)
TABLE_SLOT_MATCH:               YES  (D_80127708[0])
CAMCTRL_WRITER_MATCH:           YES  (func_800CA628 / store 0x800CA65C)
CAMCTRL_STORE_SET_COMPLETE:     YES
CAMCTRL_PRIMARY_WRITER_FUNCTION: func_800CA628 (caller func_8010FC38 /
                                 func_8010FE14)
FIRST_INTERNAL_DIVERGENT_FIELD: camctrl +0x1C at glidx 385 (-0.0676 deg)
CAMCTRL_WRITER_INPUT_DIFFERS:   YES  (glidx 385)
GLIDX385_CLASS:                 FIRST_CAUSAL_CAMERA_STATE
PLAYER_DRIFT_CHAIN_CONFIRMED:   YES
PRIMARY_CLASS:                  SAME_WRITER_DIFFERENT_INPUT
                                (first divergent input = CONTROL_CAMERA_INPUT)
ROOT_CAUSE_LEVEL:               R  (func_800E1A58 REFUTED;
                                   [[../../plan/evidence/p08-demo-camera-e1a58-differential]])
FIX_IMPLEMENTED:                NO
```

**CORRECTED/SUPERSEDED:** Mode 2 / `func_80110588` PROVEN. `func_80110790`
is a copier whose `a1[1]` already differs at 384. Overlay caller is
**PROVEN** `ncbapivot_entrypoint_3` (idx 781, RA offset `0x24C`).
dest[1] = `func_80013B70(eye.x-tgt.x, eye.z-tgt.z)`; look-at inputs
already differ at 384. Exact 13B70 operands:
`EYE_INPUT_MATCH=NO` (grows at 384), `TARGET_INPUT_MATCH=NO` (stable
background). Causal component is eye; last writer `func_80112E98`.
See [[../../plan/evidence/p08-demo-camera-13b70-inputs]].

## P08 camera collision attribution review (2026-08-18)

**CORRECTED/SUPERSEDED:** independent original-MIPS review
([[../../plan/evidence/p08-demo-camera-collision-sol-review]]) refutes the
Level A-candidate collision attribution. `func_800C4C34` is a camera-space
projection and viewport/depth clipping helper with no collision candidate or
triangle traversal. `func_800E8918` returns a selected persistent projection
record's `+0x34` halfword, not the direct helper `v0`; historical CLEAR/HIT
labels were therefore incorrect. The downstream counter / `half_0C` /
`func_801138A0` / third-`SET74` chain remains **PROVEN**. Q1 and Q2 differ at
the first measured relevant invocation (glidx 70); glidx 384 is the first
visible effect. Root level `R`; no production fix.

Next single action: trace the selected `func_800E8918` record identity and the
first writer/input divergence at `record+0x34`, beginning with
`func_800E82EC`.

**CORRECTED/SUPERSEDED (2026-08-19 B-INPUT `a3=v1[2]`):** that inequality is a
join artifact ([[../../plan/evidence/p08-demo-projection-v1-direction-producer]]).

**CANONICAL (2026-08-20, B-STATE):** correctly joined Q1 `+0x34` 0 vs 1 is
**real**. Explicit classifier args match; first difference is
`func_8001A0C0` `v0` inside `func_800E81E0`
([[../../plan/evidence/p08-demo-projection-classifier-rejoin]]).
NO production fix.

**func_8001A0C0 STATE (2026-08-20, B-STATE → B-WRITER):**
[[../../plan/evidence/p08-demo-1a0c0-state-differential]],
[[../../plan/evidence/p08-demo-db-8007bcc4-lifecycle]]. `func_8001A0C0`
is a pure leaf `*(u16*)(D_8007BCC4 + 608*a1 + 2*a0)`. Original base
`D_8007BCC4 = 0x80137840` (constant), Q1 cell `0` (25/25). The DB lifecycle
differential shows allocation chain, base, effective address (0x8014F800), and
allocation-time cell content all **match** both controls. The Rice control
boot-bzeros the DB but never copies rendered depth to it; RT64 writes 0xFFFE
then 0x8660..0x8BFC. B-ALLOC/B-ALIAS/B-INIT remain refuted, while historical
B-WRITER is **CORRECTED/SUPERSEDED by C-ORACLE**. NO production fix.

Independent arbitration: [[../../plan/evidence/p08-demo-zbuffer-oracle-sol-review]].
D_8007BCC4 and func_8001A0C0 roles are proven. Rice is not a valid
CPU-visible-Z oracle.

**Z-buffer hardware oracle validation (2026-08-20, R-ORACLE):**
[[../../plan/evidence/p08-demo-zbuffer-angrylion-oracle]]. The hardware-accurate
Angrylion RDP Plus + CXD4 LLE oracle proved that real N64 RDP populates live Z in RDRAM,
the clear value is `0xFFFC`, and `record+0x34` evaluates to `1` across all 25/25
aligned Q1 samples. The historical Rice-based camera divergence was an emulator artifact.
Classified as **R-ORACLE**. No production fix is required.

**Post-egg motion revalidation (2026-08-20, R-MOTION-ORACLE):**
[[../../plan/evidence/p08-demo-post-egg-angrylion-motion-differential]]. Revalidation
of the entire 765-frame Wooded Hollow demo against Angrylion RDP Plus + CXD4 LLE proved
that the historical motion divergence disappeared completely. `camctrl` at glidx 385
matches within `+0.0030°`, `ideal_yaw` matches identically (`102.59°` vs `102.59°`),
max XZ position error across all 765 frames is `< 0.89 units`, and behavior states are
100% bit-identical. Classified as **R-MOTION-ORACLE**. No production fix is required.

NEXT_SINGLE_ACTION: Conclude P08 attract demo motion sync validation as fully proven and proceed to P08 frontend/input milestone sign-off.

**USER_DEMO_5 angular-input evidence (2026-08-23):**
[[../../plan/evidence/p08-user-demo5-locomotion-first-divergence]] records the
new immutable capture (SHA256 PASS twice). `800EE780` is downstream vec3 add;
`800EEBF0`/`800EF214` reconstruct a Cartesian vector from degree-based
pitch/yaw and radius. The first glidx-24 difference is yaw input
`4391e610` RT vs `43927d8b` AN at caller `sp+0x38`, written by the
`0x80800194` add immediately before the call. The two upstream operands
(`80012C3C`, `800125E8`) remain unjoined. Boundary:
`B-ANGLE-SOURCE-PRECALL-COMBINATOR`. No production change or stable-spec
change. The canonical capture label is Grok 4.6 High; immutable stale metadata
was not rewritten.

## `0x800979D8` semantic correction (2026-08-22)

The native helper is now generic and reuses the generated `bakey_held`,
`bainput_isEnabled`, and `bainput_enable` implementations. Its proven static
contract is `prev_state` suppressor/live `800BCE84` query, logical-input
`__enabled` gating via `a2`, and enabled-mask return (`a2=1` → `2`). The full
branch test passes. Instruction-level USER_DEMO_4 `0x800BCE84` live-input join
is **PROVEN** ([[../../plan/evidence/p08-user-demo4-bce84-live-input-join]]);
the bounded `0x800979D8 → P60 → ncbapivot` chain matches in current log
content, and the final paired capture at `/tmp/p08-979d8-final-20260822T070735Z`
passes its SHA256 manifest twice. Global attract parity remains open because USER_DEMO_1
and USER_DEMO_5 have user-reproduced movement residuals. P08 stays
`ACTIVE / MANUAL_VALIDATION_PENDING` and P09 stays
`PENDING / NOT_FORMALLY_OPEN`.

**USER_DEMO_5 p60 mode-state arbitration (2026-08-23):** independent static
reconstruction confirms `func_80112C3C` mode 0 pass-through and modes 1/2/5
step through `800F1738`. The existing paired trace shows p60+8 first differs
at glidx 23 (`RT=5`, `AN=0`), with yaw output first differing at glidx 24.
The static writer boundary is `func_80112D08` store `0x80112D14`, reached by
`func_80112A84`; dynamic first-write provenance remains pending after a
headless RT64 Vulkan startup failure. No production change.

**USER_DEMO_5 `func_801111D0` call-presence arbitration (2026-08-23):**
`80112D90` is called at `0x80111294` only when
`(80112E84(a0)==1) || (8011349C(a0,&sp+0x40)!=0)`; the controlling branch PCs
are `0x80111230` and `0x80111270`. At glidx 23 `p60+0xC` is zero in both
runtimes, while the paired trace shows RT calling D90 and AN skipping it. The
raw record `(2,1,2002)` matches; no direct C_RIGHT predicate exists in
`801111D0`. The exact second-gate return is not captured, so classify as
`B-801111D0-PREDICATE-INPUT` (call presence proven, helper provenance pending).
No production or stable-spec change.

**USER_DEMO_5 `0x80097B94` semantic-action arbitration (2026-08-23):** static
control flow confirms a wrapper for action `0xD`/input `1` over `0x800979D8`.
Oracle matches Gemini's `pad=(2,1,2002)` and records BCE84 ring 0/raw 0, final
result 0. The focused RT64 window has a different pad
`(-13,-15,0xF1F1)` and result 0, so Gemini's claimed `2/0` split is not a
joined event. Implementation coverage is yes; semantic acceptance remains
unproven. No production change.

**USER_DEMO_5 `0x80097B70` / action-`0xA` arbitration (2026-08-24):** the
wrapper passes `C_LEFT`/action `0xA` and input id `0` to `0x800979D8`; caller
`0x800A4928` branches on the result and calls `0x80110EFC(node, 0)` at
`0x800A4930`. The visible `(2,1,2002)` record is joined at glidx 22/23/24.
The prior paired run has RT64 `0/1/1` versus Oracle `0/0/0`; Oracle record 23
has `prev=0`, BCE84 `ring=1/raw=0xF3/masked=1`, `bakey=1`, and input 0
disabled before the helper, while prior RT64 had `bakey=1` and input 0 enabled.
A fresh RT64 run has `ring=0/raw=0x72C/masked=0/bakey=0` and returns 0.
Classify as `B-ACTION-0A-HISTORY-STATE`: the pad is not sufficient to identify
the helper result, and the upstream state writer remains unproven. The
published helper fix covers the shared contract. No production change.

**Corrected USER_DEMO_5 camera re-anchor (2026-08-24):** the correctly paired
replay has records 22-24 MATCH and record 25 as the first observed camera-control
DIFF. Trot-exit input enable is a match in both runtimes (`bainput_enable(0,1)`,
`0xFFFFFFBE -> 0xFFFFFFBF`, bit 0 set), and action `0xA` returns 1 in both.
This supersedes the old `0x80097B94 RT=2/AN=0`, `RT97B70=1/AN=0`, RT-only
`0x80110EFC`/`0x801111D0`/
`0x800979D0`, and action-`0xA` causal claims; `B-TROT-ENABLE-RECOVERY` is not
the divergence/root.

The authoritative camera sample is selected record `+0x1C`, from
`0x80127708[ps+0x184]`; static `func_800CA628` identifies writer PC
`0x800CA65C`, source2[1]. Record 24 is `293.5294` / `0x4392C3C2`; record 25
is RT64 `293.0478` versus Oracle `293.4005` / `0x4392B342`. Oracle's direct
writer event is present, RT64's raw direct-writer event is not; classify as
`B-CAMERA-WRITER-OR-SOURCE-INCOMPLETE-JOIN`. The previous record-23 p60 `5/0`
split and D90/yaw/camera-eye causal claims are superseded and must not be
promoted. Recheck p60 only on record 24->25. Artifact:
`/tmp/p08-demo5-record25-camera-firstdiff-20260824T024500Z/`.
Status: `P08 ACTIVE / MANUAL_VALIDATION_PENDING`; `P09 PENDING / NOT_FORMALLY
OPEN`. No production change.

**First-writer correction:** the newer direct CA628 capture includes RT64. Its
first record-25 event is ordinal `10329` (RA `0x8010FE24`) with old field
`0x4392861E`, source2[1] `0x43922BE8`, and new field `0x43922BE8`; Oracle's
paired event is ordinal `103`, `0x4392B342 -> 0x4392A91A`. CA628 is therefore
the first observed downstream Case-B writer, not the root. CA8B4 copies the
field and E1A58 preserves it later. The actual pre-CA628 writer remains
uncaptured; no old p60/input chain is promoted.

**Camera heading field boundary (corrected):** the capture's `0x800EFA5C`
store writes temporary `stack_angles+0x4`; it does not write
`active_node+0x34`. The input is `active_node+0x3C`, loaded at
`func_8010FE34:0x8010FEA8`, and `selected_record+0x1C` is the later CA628
destination. `active_node+0x34` is not read in this target path.

The direct static writer candidate for `active_node+0x3C` is
`func_8010FDEC:0x8010FDFC`, called by `plcamera_entrypoint_13:0x808004A0`.
Because the artifact lacks a semantic node+0x3C effective-address event, the
current boundary is `B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED`. Do not promote EFA4C,
FE34, FE14, CA8B4, or E1A58 as the node+0x3C producer.

The subsequent node3C checkpoint artifact supersedes its temporal labels:
Oracle's raw probe read `0x8031CF80`/mode `0` during records 22–26, later read
`0x801CACB0` with the non-heading value `0x01015590`, and RT64 supplied no
paired raw checkpoint lines. The previously reported post-player MATCH and
pre-FE34 DIFF were not direct semantic node reads. The corrected boundary is
`B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED`.

**EFA4C heading provenance (2026-08-24):** recapture
`/tmp/p08-demo5-efa4c-heading-provenance-20260824T061436Z/` failed to rejoin
the Luna `10970→EFA4C` heading event (`R24_EFA4C_EVENT_JOIN: FAIL`). Classify
`CASE E - CORRECTED/SUPERSEDED`. EFA4C remains a downstream stack writer; FE34
a load/consumer only when executed; FDEC `REFUTED_FOR_TARGET`. Current live
boundary stays `B-EFA4C-HEADING-SOURCE-PROVENANCE-UNRESOLVED` until a map=296
join actually observes 10970.

**Record24 camera-path arbitration (2026-08-24):** the Grok recapture does
not establish a semantic record24 join. `sub=5` collides across attract
slices; RT has two remapped-`glidx=24` series, while Oracle has no paired
FC38/FE14/CA628 events. The artifact also lacks record ordinal/sequence/pad
context and effective-CA628 state tables. Classify the current camera
boundary as `JOIN-UNRESOLVED`; `CONFLICT_WINNER: NEITHER — NEW CAPTURE
REQUIRED`. The old FE34/10970/EFA4C chain is `CORRECTED/SUPERSEDED`, and
`B-EFFECTIVE-CA628-SOURCE` must not be promoted.

**Independent map296 join and camera boundary correction (2026-08-24):**
The new capture is now JOIN-RESOLVED: 28/28 raw records match bit-perfectly,
with one epoch, stage=0/sub=5/map=296/mode=5, and no duplicate record keys.
D_8012D5B0+0x0 is the demo stream pointer; D_8012D5B8 is the post-consume
counter over six-byte samples. func_800151BC is the proven logic boundary,
but camera tags can be post-consume and must not be called logical ordinals.

The claimed camera entry/exit identity is refuted: A9318 is a separate FE34
pass, not a proven exit of A8E9C. R24 has EFA4C 66 RT vs 73 AN, FE14 1/1,
CA628 2/2, and no A9318 event; R24_CALL_SEQUENCE_MATCH=NO.

The earliest comparable camera event is logical record 0, camera ordinal 1,
CA628 call 1. Its old destination matches (0x4324BF6C), while the source
differs (RT=0x4324B542, AN=0x4324B132), so the current boundary is
B-FIRST-CA628-SOURCE. R24 CA628 is downstream because old/source already
diverge (0x4324A910 vs 0x4324A7B0); output remains unknown. The old
FE34/10970/EFA4C chain is CORRECTED/SUPERSEDED; do not promote
B-EFFECTIVE-CA628-SOURCE or any deeper writer.

Derived artifact:
/tmp/p08-demo5-first-camera-diff-from-joined-run-20260824T074500Z/;
manifest SHA256 d1139defdfa6903041cef31fd2e4db98ca328bd964e702a4e1dca6683a7cff3f.

**USER_DEMO_5 Record 0 CA628 source provenance & writer trace (2026-08-24):**
The new artifact `/tmp/p08-demo5-r0-ca628-source-firstdiff-20260824T144859Z/`
(immutable `0444`, SHA256 verified twice) dynamically traces the first camera
divergence on the collision-free 28/28 joined replay (`stage=0, sub=5, map=296,
mode=5`).

1. Map 296 Entry State: Bit-identical in both runtimes (`cam_sub+0x84 = 0x43240367`,
   `node+0x50 = 0x3f3c04a3`, `node+0x58 = 0x3f800000`). Refutes Case E (Map Init).
2. First Divergent Writer: `func_80116218` at `0x80116380` (`swc1 $f18, 0x4($s1)`).
3. Target Address: `$sp + 0x2C` (`0x8004567C`, caller stack frame of `func_80110588`).
4. Values Stored: RT64 stores `0x4324b542` (164.708038) vs Angrylion stores
   `0x4324b132` (164.692169), driven by upstream angular rate evaluation in
   `func_80116120` / `tooie_wrap_degrees`.
5. Downstream Consumer: `func_800CA628` Call 1 at `0x8010FE1C / 0x8010FE24`
     copies `src2[1]` into `dst+0x1C` (old value matched `0x4324bf6c` in both).
     6. Boundary: `B-R0-CA628-SOURCE-WRITER-PROVEN` (Case A). No production change.

### Independent pre-0x80116380 arbitration: first cam_sub+0x84 transition (2026-08-24)

This stable plan boundary supersedes the preceding root claim: the first observed transition is `func_800EE7F8:0x800EE804`, which writes persistent `cam_sub+0x84` from `0x80045638` as RT `0x4324036A` versus AN `0x4324036D`. The immediate caller remains unknown.

1. **Current status:** `P08 ACTIVE / MANUAL_VALIDATION_PENDING`; `P09 NOT FORMALLY OPEN`.
2. **Root boundary:** `B-R0-FIRST-CAMSUB84-WRITER-SOURCE`; `80116380` and `CA628` are downstream.
3. **Proven continuity:** 28/28 joined records; `80110588` entry base `0x43240367` and node+`0x50` `0x3F3C04A3` match; `80110634` already carries the differing heading; node+`0x50` first differs at `801161E0`.
4. **Superseded:** Gemini's `80116380` Case A and prior `B-R0-CA628-SOURCE-WRITER-PROVEN` as root; older FE34/10970/EFA4C/node+3C roots. Map 296 is Hailfire Peaks — Icy Side / `MAP_128_HP_ICY_SIDE`; current TOML has no human-readable map table.
5. **Pending:** exact nested caller, conditional `800893E0` execution/value, and complete `80116120` input parity.
6. **Artifact:** `/tmp/p08-demo5-r0-pre-80116380-firstdiff-20260824T082500Z/`; manifest `6c793196c0457e162bcf6ebd1ff8ed2f4e764f1a72d544cf09232edd0148c29b`.
7. **Run state:** intentionally dirty preserved; docs only; no production fix, build, or test. Next single action is a paired Record 0 capture with RA/callsite, `f71`, and post-`EE7F8` stack checkpoint. Recommended model: GPT-5.6 Luna High / Terra equivalent.

## Current USER_DEMO_5 angular boundary — 2026-08-25 (func_80097B70 & func_800979D8 Direct Measurement glidx 22->23)

1. **Phase/status:** P08 `ACTIVE / MANUAL_VALIDATION_PENDING`; P09 `PENDING / NOT FORMALLY OPEN`.
2. **Investigation:** Direct dual measurement of `func_80097B70`, `func_800979D8`, branch `0x800A4928`, and resolution of the prior observational gap.
3. **Findings:**
   - glidx 22: `func_800979D8` v0=0/0 MATCH; `func_80097B70` v0=0/0 MATCH; branch 0x800A4928 taken=1/1 MATCH (branches over rotation left in both runtimes); `func_80112D90` NOT called in either runtime (`p60+0xC = 0`).
   - glidx 23: `func_800979D8` v0=1 (RT) vs v0=0 (AN) DIFF; `func_80097B70` v0=1 (RT) vs v0=0 (AN) DIFF; branch 0x800A4928 taken=0 (RT, fallthrough -> calls `func_80110EFC` -> `80110E00` -> `801111D0` -> `80112D90`, setting `p60+0xC = 1`) vs taken=1 (AN, branches over rotation left; `p60+0xC` remains `0`).
4. **Observational gap:** **RESOLVED**. Both runtimes directly measure `func_80097B70` and `func_800979D8` leave return values.
5. **Artifact:** `/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/` (read-only `0444`/`0555`, SHA256 verified twice).
6. **Next single action:** Perform static and mechanical dynamic boundary analysis of `bakey_held` (`0x800919F4`) and `func_800BCE84` for button 10 (C-Left) at `glidx 23`.
7. **Recommended model:** Gemini 3.7 Flash High / GPT-5.6 Luna High. No production fix, commit, or push.

## Latest independent arbitration

- **Artifact:** `/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/`; `sha256sum -c SHA256SUMS` passes; **MANIFEST_SHA:** `650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`.
- **Semantic join:** normalized join `(stage=0, sub=5, map=296, mode=5, glidx, calling_region=func_800A4878, controlling_branch=0x800A4928, bainput_helper=func_800979D8)` is **PROVEN**.
- **Causal result:** `func_80097B70` directly returns the output of `func_800979D8(self, 10, 0)`. Inside `func_800979D8`, divergence is produced by `bakey_held(self, 10)` returning `held = 1` in RT64 vs `held = 0` in ANGRYLION_CONTROL at `glidx 23`.
- **Current root boundary:** `bakey_held(self, 10)` (`0x800919F4`) / `func_800BCE84` button 10 (C-Left) query at `glidx 23`.
- **Next single action:** Perform static and mechanical dynamic boundary analysis of `bakey_held` (`0x800919F4`) and `func_800BCE84` for button 10 (C-Left) at `glidx 23`.
- **Next recommended model:** Gemini 3.7 Flash High / GPT-5.6 Luna High. No fix, commit, or push.
- **Optional mechanical model:** DeepSeek V4 Flash.
- **Optional secondary model:** Grok 4.6 Medium for narrow timing/register ambiguity.

## Latest superseding input-helper arbitration (2026-08-25)

Artifact `/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/` is a valid
`NEW_RAW_CAPTURE`; manifest SHA is
`650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`.

The preceding held-state claim is **CORRECTED/SUPERSEDED**. At the joined
target (`stage=0/sub=5/map=296/mode=5`, glidx 23), both runtimes show
`bakey_held=1`, enabled word `0xffffffbf`, and enabled bit 0 `1`. The first
proven difference is `800979D8` return `RT=1` versus `AN=0`; `80097B70`
faithfully forwards it.

Static read-set: `bakey_held` reads `self+0x40`, `key+0x18+button_id`, and
`key+0`; zero previous state calls `800BCE84(state, button_id)` at
`0x80091A18`. `800BCE84` reads `state+0x8+button_id`, calls `800BCCC0`,
indexes by button, and returns bit 0. Logged map/button-word fields are
trace-only. Since AN lacks direct capture of the complete `800BCE84`
mapped-byte/table inputs, classify this as **Case B**:
`B-800979D8-COMPLETE-READSET-UNKNOWN`; keep `800BCE84` as
`NEXT_UPSTREAM_CANDIDATE`.

**NEXT_SINGLE_ACTION:** dual-runtime glidx 22/23 capture of complete
`bakey_held` state, nested `800BCE84` call/args/return, and `800979D8`
held/return; stop at `800BCE84`. **NEXT_RECOMMENDED_MODEL:** Gemini 3.7
Flash High. No production fix.

## Superseding narrow arbitration — original `800979D8` post-`isEnabled` body (2026-08-25)

The valid artifact is `/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/`,
`NEW_RAW_CAPTURE`, manifest
`650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`, with
all checksum entries passing. At glidx 23, `bakey_held=1/1` and enabled bit
0 `1/1` are proven; host replacement returns `1` with no enable write, while
original AN returns `0`.

## Canonical Checkpoint — Exact MIPS Recovery & Static Implementation Mismatch in `func_800979D8` (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-post-enabled-20260825T184847Z/` (manifest SHA `4c75f328cfaa978174f373884472395cf708eeaf413f096f1808af8c913d705c`, dual `sha256sum -c` PASS).

1. **Exact MIPS Disassembly of `func_800979D8` (`0x800979D8 .. 0x80097A4C`):**
   - `0x800979E8`: calls `bakey_held(self, button_id)` (`0x800919F4`).
   - `0x800979F0`: `beqz $v0, 0x80097A2C` (if unheld -> jumps to `0x80097A2C`).
   - `0x800979FC`: calls `bainput_isEnabled(self, input_id)` (`0x80098024`), reading `*(input + 0x0) & (1 << input_id)`.
   - `0x80097A04`: `bnez $v0, 0x80097A18` (if enabled -> skips `0x80097990(self, input_id, 0)`).
   - `0x80097A10`: calls `func_80097990(self, input_id, 0)` (when enabled == 0, clears bit in `*(input + 0x4)`).
   - `0x80097A1C`: calls `func_80097978(self, input_id)` -> returns `*(input + 0x4) & (1 << input_id)`!
   - `0x80097A24`: jumps to epilogue `0x80097A40` and returns `$v0` from `func_80097978`.
   - Unheld path (`0x80097A2C`): calls `func_80097990(self, input_id, 1)` (sets bit in `*(input + 0x4)`) and returns `$v0 = 0`.

2. **Divergence Mechanism:**
   - At `glidx 23`, `bakey_held` returns `1` and `bainput_isEnabled` returns `1` in both runtimes.
   - Original MIPS: `func_80097978` reads `*(input + 0x4) = 0xFFFFFFFE` -> bit 0 is 0 -> returns `$v0 = 0`.
   - RT64 host replacement (`tooie_bainput_func_800979D8` in `src/game/boot_hw_stubs.cpp`): incorrectly calls `bainput_isEnabled` (reading `*(input + 0x0) = 0xFFFFFFBF` -> bit 0 is 1) a second time -> returns `ctx->r2 = 1`.

3. **Classification:**
   - `STATIC_IMPLEMENTATION_MISMATCH_800979D8`: **PROVEN**.
   - `CURRENT_ROOT_CLASS`: `B-800979D8-STATIC-HOST-IMPLEMENTATION-MISMATCH`.
   - `CURRENT_ROOT_BOUNDARY`: Callee selection in `func_800979D8` at `0x80097A1C` (`func_80097978` on `input+0x4` vs `bainput_isEnabled` on `input+0x0`).
   - `NEXT_SINGLE_ACTION`: GPT-5.6 Luna High independently arbitrate the exact original-vs-host semantic mismatch before any production fix.
   - `NEXT_RECOMMENDED_MODEL`: GPT-5.6 Luna High. No fix, commit, or push.

### Corrected/Superseded — `800979D8` production bug versus current parity root (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-post-enabled-20260825T184847Z/` is valid,
manifest `4c75f328cfaa978174f373884472395cf708eeaf413f096f1808af8c913d705c`,
with all checksum entries passing. The original MIPS body through
`0x80097A4C`, including the post-`isEnabled` path, is complete and proven.

`STATIC_IMPLEMENTATION_MISMATCH_800979D8=PROVEN`: `80097990` writes
`input+0x4` while host `bainput_enable` writes `input+0x0`; `80097978` reads
`input+0x4` at `80097984` while the host's second query uses
`bainput_isEnabled`/`input+0x0`. Keep this as a production correctness bug,
separate from the current parity root.

At glidx23, `input+0=ffffffbf` is a RT64/Angrylion **MATCH**. Functional
`input+0x4` is RT64 `ffffffff` (bit0=1) versus Angrylion `fffffffe` (bit0=0),
a **DIFF**. The earlier complete-input match assertion is
**CORRECTED/SUPERSEDED** to **NO**. glidx22 `ffffffff`/`ffffffff` is the last
proven match, and glidx23 is the first proven difference.

If only the host final query were corrected, RT64 would return `1` and the
original Angrylion path returns `0`; `COUNTERFACTUAL_RETURN_MATCH=NO`.
Therefore `WOULD_FIXING_800979D8_ALONE_RESTORE_GLIDX23_PARITY=NO`.

`PRODUCTION_CORRECTNESS_BUG`: proven static host mismatch.
`CURRENT_PARITY_CAUSAL_ROOT`: input+4 active/triggered state divergence.
`CURRENT_ROOT_CLASS`: `B-BAINPUT-ACTIVE-MASK-INPUT-PLUS4-STATE`.
`CURRENT_ROOT_BOUNDARY`: input+4 bit0 immediately before
`80097A1C -> 80097978` / `80097984`.

`FIX_800979D8_NOW=NO`. `NEXT_SINGLE_ACTION`: statically enumerate immediate
writers/callsites of the bit, starting with known `80097990`, then define one
dual-runtime capture of old/new word, source, writer PC, callsite, call
presence, and semantic occurrence. No capture, instrumentation, or upstream
walk in this round. Next model: **Gemini 3.7 Flash High**; DeepSeek V4 Flash is
optional for candidate enumeration, Grok 4.6 Medium only for ordering
ambiguity.

## Canonical Checkpoint — `input+0x4` First Temporal Writer & Host `800979D8` Causal Proof (2026-08-25)

Artifact `/tmp/p08-demo5-inputplus4-first-writer-20260825T220500Z/` is valid;
`NEW_RAW_CAPTURE`, manifest SHA is `3acb82b7a27b9dfa90da7a782e0618caebd6ef9daa574340261007703417de46`,
all 10 `SHA256SUMS` entries pass (dual verification).

1. **First Temporal Writer Event Identified:**
   - At `glidx 22`, occurrence `func_800979D8(self, button_id=10, input_id=0)`:
     - `held == 1` and `enabled == 0` in BOTH runtimes.
     - `*(input + 0x4)` is `0xFFFFFFFF` (bit 0 = 1) in BOTH runtimes before call.
     - **AN (Original MIPS):** Follows `0x80097A04` branch fallthrough and executes `0x80097A10: jal func_80097990(self, 0, 0)`. `func_80097990` executes CLEAR store at `0x800979CC`, transforming `*(input + 0x4)` from `0xFFFFFFFF` (bit 0 = 1) to `0xFFFFFFFE` (bit 0 = 0).
     - **RT (Host C++ Replacement):** `tooie_bainput_func_800979D8` calls `bainput_enable(self, 0, 0)` at `0x8009817C` (modifying `input + 0x0`) instead of `func_80097990` (modifying `input + 0x4`). Therefore, `*(input + 0x4)` in RT64 is never modified and remains `0xFFFFFFFF` (bit 0 = 1).
   - This single host defect at `glidx 22` is the exact causal producer of the `input+0x4` state divergence.

2. **Downstream Target Divergence at `glidx 23`:**
   - At `glidx 23`, `bainput_enable(self, 0, 1)` updates `*(input + 0x0)` to `0xFFFFFFBF` in both runtimes.
   - When `func_800979D8(self, button_id=10, input_id=0)` is called:
     - `held == 1`, `enabled == 1`.
     - **AN:** Executes `0x80097A1C: jal func_80097978(self, 0)`, reads `*(input + 0x4) = 0xFFFFFFFE` (bit 0 = 0) -> returns `$v0 = 0`. Controlling branch `0x800A4928` branches over camera rotation left (`p60+0xC = 0`).
     - **RT:** Host calls `bainput_isEnabled(self, 0)` on `*(input + 0x0) = 0xFFFFFFBF` (bit 0 = 1) -> returns `ctx->r2 = 1`. Controlling branch `0x800A4928` falls through to `func_80112D90`, setting `p60+0xC = 1` (unwanted camera rotation left).

3. **Classification & Verification:**
   - `FIRST_INPUT_PLUS4_DIFFERING_EVENT`: `glidx 22`, `func_800979D8(self, btn=10, in_id=0)`.
   - `FIRST_INPUT_PLUS4_WRITER_RT`: `NO_WRITE_OBSERVED` (host called `bainput_enable` on `input+0`, omitting write to `input+4`).
   - `FIRST_INPUT_PLUS4_WRITER_AN`: `func_80097990` at `0x800979CC` (CLEAR bit 0).
   - `FIRST_INPUT_PLUS4_WRITER_EVENT_MATCH`: `ABSENT_IN_ONE_RUNTIME`.
   - `FIRST_INPUT_PLUS4_OLD_MATCH`: `YES` (`0xFFFFFFFF`, bit0=1).
   - `FIRST_INPUT_PLUS4_SOURCE_OPERATION_MATCH`: `NO`.
   - `FIRST_INPUT_PLUS4_NEW_MATCH`: `NO` (`0xFFFFFFFE` [bit0=0] vs `0xFFFFFFFF` [bit0=1]).
   - `HOST_800979D8_MISMATCH_CAUSES_INPUT_PLUS4_FIRST_DIFF`: **`PROVEN`**.
   - `CURRENT_ROOT_CLASS`: `B-800979D8-HOST-REPLACEMENT-WRONG-SIDE-EFFECT`.
   - `BAINPUT_INIT_EXECUTES_IN_WINDOW_RT`: `NO`.
   - `BAINPUT_INIT_EXECUTES_IN_WINDOW_AN`: `NO`.
   - `NEXT_SINGLE_ACTION`: GPT-5.6 Luna High arbitrate the first input+4 divergence and the causal proof that the host 800979D8 mismatch produces it.
   - `NEXT_RECOMMENDED_MODEL`: GPT-5.6 Luna High. No fix, commit, or push.

## Final root checkpoint — `800979D8` replacement (2026-08-25)

**CORRECTED/SUPERSEDES** the prior pending arbitration. The valid raw
artifact is `/tmp/p08-demo5-inputplus4-first-writer-20260825T220500Z/`,
manifest SHA
`3acb82b7a27b9dfa90da7a782e0618caebd6ef9daa574340261007703417de46`.

The glidx22 semantic join is **PROVEN** for USER_DEMO_5
`stage=0/sub=5/map=296/mode=5`, player self, `button_id=10`, `input_id=0`,
with the helper call present in both runtimes. Held/enabled are `1/1` and
`0/0`; old `input+4` is `ffffffff/ffffffff`. AN's
`func_80097990(self,0,0)` performs the CLEAR and produces `fffffffe`; RT's
`bainput_enable(self,0,0)` targets `input+0` and leaves `input+4=ffffffff`.
The pre-write state matches and the post-write state is the first diff:
`HOST_800979D8_MISMATCH_CAUSES_INPUT_PLUS4_FIRST_DIFF=PROVEN`.

Use these corrected MIPS locations: SET store `0x800979B0` (`ac590004`),
CLEAR store `0x800979CC` (`ac4c0004`); `0x80097A10` is the callsite,
`0x80097A14` is its delay slot, and `0x80097A18` is the return address.
At glidx23 the original read helper returns 0 from `input+4`, while the
host's wrong second `bainput_isEnabled` returns 1 from `input+0`; the
`80097B70`/`0x800A4928`/camera results are downstream.

`HOST_MISMATCH_WRITE_HELPER=PROVEN` and `HOST_MISMATCH_READ_HELPER=PROVEN`.
They are two wrong substitutions in the same replacement and must be fixed
together. `ROOT_CAUSE_FOUND=YES`; `FIX_IMPLEMENTED=NO`.
A glidx23-only change is insufficient; a complete faithful replacement is
**EXPECTED** to restore the chain, but no runtime PASS is claimed. Next:
implement the minimal host fix and execute the required dual-runtime A/B
validation, then broader attract regression validation. Recommended model:
Gemini 3.7 Flash High.

## Post-Fix Validation Checkpoint — `tooie_bainput_func_800979D8` Faithful Fix (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-fix-validation-20260825T235800Z/` is valid;
`NEW_RAW_CAPTURE_POST_FIX`, manifest SHA is `9a977b6e1c4c276dd2b6616008723d4b3c7139b9d1c4bd6c519c2f23ae3373da`,
all 16 `SHA256SUMS` entries pass (dual verification).

- **`ROOT_CAUSE_FOUND:`** `YES`
- **`ROOT_CAUSE_FIX_VALIDATED:`** `YES`
- **`FIX_IMPLEMENTED:`** `YES`
- **`FIX_SCOPE:`** `src/game/boot_hw_stubs.cpp` (`tooie_bainput_func_80097990` on `input+4`, `tooie_bainput_func_80097978` on `input+4`, and corrected `tooie_bainput_func_800979D8`)
- **`HOST_WRITE_HELPER_FIXED:`** `YES`
- **`HOST_READ_HELPER_FIXED:`** `YES`

**Validation Results (100% Parity with ANGRYLION_CONTROL):**
- `GLIDX21_CONTROL_PARITY:` `PASS` (held=0, enabled=1, input+4=0xFFFFFFFF, v0=0)
- `GLIDX22_HELD_PARITY:` `PASS` (held=1/1)
- `GLIDX22_ENABLED_PARITY:` `PASS` (enabled=0/0)
- `GLIDX22_INPUT4_OLD_PARITY:` `PASS` (0xFFFFFFFF/0xFFFFFFFF)
- `GLIDX22_OPERATION_PARITY:` `PASS` (func_80097990 CLEAR store at 0x800979CC)
- `GLIDX22_INPUT4_NEW_PARITY:` `PASS` (0xFFFFFFFE/0xFFFFFFFE)
- `GLIDX23_INPUT4_PARITY:` `PASS` (0xFFFFFFFE/0xFFFFFFFE)
- `GLIDX23_800979D8_RETURN_PARITY:` `PASS` (v0=0/0)
- `GLIDX23_80097B70_RETURN_PARITY:` `PASS` (v0=0/0)
- `GLIDX23_800A4928_PARITY:` `PASS` (TAKEN/TAKEN, skipping camera rotation left)
- `GLIDX23_80112D90_CALL_PARITY:` `PASS` (NO/NO, camera rotation left completely collapsed)
- `GLIDX23_P60C_PARITY:` `PASS` (p60+0xC = 0/0)
- `POST_FIX_EEBF0_PARITY:` `PASS` (yaw=270.0 in both runtimes)
- `POST_FIX_EE780_PARITY:` `PASS` (a3=0 in glidx 21..23, 3fa21681 in glidx 24, 3fe2ec4e in glidx 25)
- `BTN13_NEARBY_REGRESSION:` `PASS` (input+4 bit1 SET, v0=0 in both runtimes)

**Status & Next Steps:**
- The entire USER_DEMO_5 locomotion/camera divergence chain is **RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX**.
- P08 remains **ACTIVE / MANUAL_VALIDATION_PENDING** pending broader physical validation gates.
- `NEXT_SINGLE_ACTION:` GPT-5.6 Luna High independently arbitrate the post-fix validation artifact and determine the remaining P08 work.
- `NEXT_RECOMMENDED_MODEL:` GPT-5.6 Luna High. No commit/push.

## Arbitragem final pós-fix P08 / USER_DEMO_5 — 2026-08-26 (GPT-5.6 Luna High)

Este bloco **CORRIGE/SUPERSEDE** o checkpoint anterior quanto a escopo,
proveniência e pendências; o histórico permanece preservado.

Artifact: `/tmp/p08-demo5-800979d8-fix-validation-20260825T235800Z/`;
`MANIFEST_SHA=9a977b6e1c4c276dd2b6616008723d4b3c7139b9d1c4bd6c519c2f23ae3373da`;
`SHA256SUMS` confere 16/16 arquivos.

`RT_RAW_PROVENANCE: NEW_RAW_CAPTURE_POST_FIX`;
`AN_RAW_PROVENANCE: REFERENCE_ORACLE_CAPTURE_REUSED`;
`REFERENCE_ORACLE_REUSE_VALID: YES`: o `oracle.log` é byte-identical à
captura Oracle de referência, gerada após a instrumentação e antes do fix.

Veredicto: a implementação de `800979D8` é semanticamente MIPS-fiel
(SET/CLEAR/query em `input+4`, enabled em `input+0`, low-5-bit shift e retorno
sign-extended). `USER_DEMO5_TARGET_CHAIN_PARITY: PROVEN` para
`800979D8 → 80097B70 → 800A4928 → câmera/80112D90 → p60+0xC → yaw/EEBF0/EE780`;
`GLOBAL_GAME_PARITY: NOT_PROVEN`.

`DIRECTLY_SUPPORTED`: held/enabled, input+4 old/new/operação, retornos,
branch, p60+0xC, yaw, EEBF0/EE780 e btn13. `DERIVED_BUT_PROVEN`: junção
causal normalizada e ausência do dispatch de rotação esquerda.
`UNSUPPORTED`: parity global, long playthrough e Windows/controle físico.

`P08_STATUS: ACTIVE / MANUAL_VALIDATION_PENDING`;
`P09_STATUS: PENDING / NOT_FORMALLY_OPEN`;
`USER_DEMO5_STATUS: RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`.
`AUDIO_STARTUP_TICK: P08_RESIDUAL`; `WIDESCREEN_EDGE_GEOMETRY: 800CAA24 PRODUCTION ON; 800CACEC object frustum PRODUCTION ON (user visual VISUALLY_CONFIRMED; DRAW-only H-expand; TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0 disables); attract prod new_accept=279; T_INTRO 16:9 first RT64 divergence CLIP_X (K=4/3 AUTO, HIGH); guest presentation-envelope Original/Expand owner diff=0 REFUTED; ±0.754 is vertical letterbox not X-crop; TINTRO_16X9_AUTHORED_EXTENT STRONG_HYPOTHESIS; 21:9+ authored extent PROVEN/separate; CAMERA_FRAMING_EXPOSES_OUTSIDE_MODEL as current-16:9 root CORRECTED/SUPERSEDED; WALL7799 skip original AABB TOP cull; 819F8 post-AABB skip OPEN; no TOOIE_TEST_INTRO_7799_VIS; no production intro fix this round; next Tooie T_INTRO/cutscene skip of AUTO K=4/3; do not change RT64 globally; ACTOR_CUBE_AABB_SELECTION_AS_EXPAND_DRAW_ROOT REFUTED; TYPE6_AS_INTRO_RESIDUAL_ROOT STRONGLY_WEAKENED`; `FOCAL_00349D38_AS_GLOBAL_ROOT: REFUTED`; `FOCAL_00349D38_AS_WITNESS: YES`; `FOCAL_TEXTURE_TMEM_ANOMALY: STRONGLY_WEAKENED / STATIC_AUDIT_REFUTED`; `FOCAL_TEXTURE_RUNTIME_REVALIDATED_WITH_NEW_AUTO_CAPTURE: NO`; `SAME_RT64_REVISION: YES` (`6f1c2d99`); `CAB70/E3DC0/CACEC: USER NO_CHANGE`; `MAP_VIS_PRODUCTION: ON unless TOOIE_TEST_MAP_VIS_ASPECT_AWARE=0`; `MAP_VIS_CF: TOOIE_TEST_DISABLE_MAP_VIS_CULL` default OFF not promoted; `OBJECT_POP_DIAG: TOOIE_DIAG_WS_OBJECT_POP` default OFF; `MODEL_GEO_CF: 801143A4` USER visual NO_CHANGE / intro 0 calls; `PROP_CELL_CF: TOOIE_TEST_DISABLE_PROP_CELL_VIS_CULL` listed 153→325 intro `dl_rsp` identical (INDEPENDENT of gsprops); `GSPROPS_VIS_CF: TOOIE_TEST_DISABLE_GSPROPS_VIS_CULL` 60s attract inserts 90→114 PARTIAL_HIT, REFUTED as Expand pop; `TYPE6_VIS_CF: TOOIE_TEST_DISABLE_TYPE6_STATIC_VIS_CULL` vis rej=0 / draw 84 identical / RSP NO_CHANGE; `ACTOR_CUBE_AABB_CF: TOOIE_TEST_EXPAND_ACTOR_CUBE_AABB` ea628 22722→257504 PARTIAL_HIT visitation NO_CHANGE draw-DL; `STATIC_DECORATIVE_VIEW_CF: NOT_IMPLEMENTED`; `LOD_CF: NOT_IMPLEMENTED`; `CULL_WINDING_ANOMALY: REFUTED_FOR_FOCAL`
([[../../plan/evidence/p08-widescreen-edge-geometry]]; pixel probe `ABANDONED_FOR_SAFETY`; `PERFORMANCE_GATING_CLEANUP_PRESERVED: YES`; artifact `/tmp/p08-tintro-rt64-triangle-translation-20260831T032400Z/`; next Tooie T_INTRO/cutscene skip of AUTO K=4/3, not `0x819F8`; do **not** wrap `80108DC0` as vis; do **not** reopen `800F0064` as FOV; do **not** reopen bound-sprite draw; do **not** production-bypass `800CAB70`/`800E3DC0`/`800CACEC`/`801143A4`/`800BE09C`/`800BD708`/`80108C14`; `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE` production ON unless `=0`; do **not** combine vis flags; do **not** enter `TARGETED_FOCAL_DEPTH_CAUSAL_TEST_DESIGN` this residual).
`CHARACTER_EDGE_GRAPHICS_GLITCH: P08_RESIDUAL`;
`LONG_PLAYTHROUGH_VALIDATION: MANUAL_VALIDATION_PENDING`;
`WINDOWS_GAMEPAD_VALIDATION: BLOCKING_P08 / MANUAL_VALIDATION_PENDING`.

Remaining P08: `800CAA24` and `800CACEC` aspect-aware H-expand are
**production ON** (disable with `=0`). Object-pop user-confirmed.
`CLEAN_PINNED_BUILD_VALIDATION: PASS` at Tooie `23c070a9` + BanjoRecomp
`ec859632` + RT64 `6f1c2d99` (`TooieRecompiled` isolated Clang/Ninja;
artifact `/tmp/p08-clean-milestone-validation-20260830T035059Z/`
`SHA256SUMS` `af72f29f96026b3d678d0f6914ea21b6aeaae648ea6c9fe25a4e5e1ea7976228`).
Capture ABI no-ops are unused by clean RT64. Intro walls
`STILL_PRESENT` (`CAMERA_FRAMING_EXPOSES_OUTSIDE_MODEL` `PROVEN`;
OPA `0x1CF9` / XLU `0x1CFA`; artifact
`/tmp/p08-intro-source-geometry-framing-20260830T115524Z/`
`SHA256SUMS` `28c90810dcd26bfd9e7015417089cbd14425d0dc271514558727a372db63a867`).
Next: `INTRO_ASPECT_CLAMP` — wire `MAP_18B` to existing
`CutsceneAspectRatioMode` (do not combine vis flags; do not return
to GPU; do not port Banjo verts; no production intro fix this round).
Validação formal Windows/controle físico; long playthrough; triagem
separada de áudio/character-edge. `CURRENT_FIX_READY_FOR_COMMIT: YES`
para o fix focal `800979D8`. Widescreen map+object vis:
`FIX_IMPLEMENTED: YES` (production); intro: `FIX_IMPLEMENTED: NO`.
Do not push.

`USER_OBSERVED_LATE_POP_IN: YES`.
`USER_OBSERVED_EARLY_POP_OUT: YES`.
`BIDIRECTIONAL_EDGE_VISIBILITY_ERROR: USER_OBSERVED`.
`USER_MANUAL_CAB70_BYPASS_VISUAL_RESULT: NO_CHANGE`.
`USER_MANUAL_E3DC0_BYPASS_VISUAL_RESULT: NO_CHANGE`.
`USER_MANUAL_CACEC_BYPASS_VISUAL_RESULT: NO_CHANGE`.
`CAB70_DIRECT_VISUAL_ROOT: REFUTED_FOR_OBSERVED_SYMPTOM`.
`E3DC0_DIRECT_VISUAL_ROOT: REFUTED_FOR_OBSERVED_SYMPTOM`.
`CACEC_DIRECT_VISUAL_ROOT: REFUTED_FOR_OBSERVED_SYMPTOM`.
`CURRENT_ROOT_CLASS: UNKNOWN / RE-ANCHOR_REQUIRED` (historical for the unjoined 1481..2200 window).
`CURRENT_ROOT_CLASS_STATUS: DL_IDENTITY_CAPTURE_INCONCLUSIVE_FOR_VISUAL_WINDOW`.
`CAPTURE_VALID: YES` (1481..2200 unjoined; superseded by 1425..2144 joined capture).
`DL_IDENTITY_JOINED_TO_VISUAL_WINDOW: NO`.
`LATE_POP_IN_SIGNATURE_FOUND: NO`.
`VISIBILITY_THRESHOLD_ASPECT_MISMATCH: NOT_PROVEN_FOR_CAPTURED_EVENT`.
`LATEST_ARTIFACT: /tmp/p08-widescreen-dl-identity-arbitration-20260826T072436Z/`
(`SHA256SUMS` `f4b2c20086ced79a3d728174464a2dc673a6fff201d0c7edfe1f014739ddccff`).

`NEXT_SINGLE_ACTION: GUEST_FRAME_JOINED_DISPLAY_LIST_IDENTITY — dedicated identity log + RT64 guest_frame field so the next USER_SELECTED window contains [ws-dl-emit] and [ws-dl-rsp]. No production frustum/LOD/room/RT64 patch. No new bypass. Model: Gemini 3.7 Flash High.`

### P08 current setup checkpoint — GUEST_DISPLAY_LIST_IDENTITY_CAPTURE (2026-08-26)

Implemented and smoke-tested as diagnostic-only. The guest emission boundary is
GraphicsTask wrapper `0x80014148`; root DL source is `GraphicsTask+0x8`, nested
DL source is direct `Gfx.w1`, and the monotonic emission sequence is active only
inside `TOOIE_DIAG_WS_CONTINUOUS`. RT64 `processDisplayLists` records the
resolved physical identity and workload/counter before HLE dispatch. No
production fix, bypass, culling/projection/LOD/room/section/near-clipping
change, or `RecompiledFuncs/**` edit was made.

```text
BUILD_RESULT: PASS
DL_IDENTITY_CAPTURE_READY: YES
DL_ADDRESS_SEMANTICS: PROVEN
NESTED_DL_TRACKING: YES
NODE_TO_DL_JOIN: NO
DL_TO_RSP_JOIN: YES
RSP_TO_RT64_WORKLOAD_JOIN: PARTIAL
DL_TO_RT64_DRAW_JOIN: NO
END_TO_END_JOIN: PARTIAL
PER_FRAME_ACTIVE_DL_SET_READY: YES
DL_FIRST_LAST_SEEN_READY: YES
DL_SCREEN_BOUNDS_JOIN: NO
EXTRA_BAND_DL_CLASSIFICATION: NO
CURRENT_ROOT_CLASS: UNKNOWN / RE-ANCHOR_REQUIRED
CURRENT_ROOT_BOUNDARY: NOT_PROMOTED
FIX_IMPLEMENTED: NO
TEMP_DIAGNOSTIC_CHANGE_IMPLEMENTED: YES
DEFAULT_RUNTIME_BEHAVIOR_CHANGED: NO
```

Reference artifact: `/tmp/p08-widescreen-dl-identity-setup-20260826T070058Z/`;
manifest SHA `3b9e63bcc0954a9b451045c7d78508176a893c10386ab1984f3ac399febf3390`.
Smoke: 3,892 guest events, 169 roots, 3,723 nested events, monotonic sequence,
and complete 720-frame window.

### P08 current arbitration — DL identity vs USER_SELECTED window (2026-08-26)

User-selected capture `/tmp/tooie-ws-continuous.log` is structurally valid
(`TRIGGER_ACTIVATED` start_frame=1481, `CAPTURE_COMPLETE` after 720 frames,
camera moving). It does **not** join display-list identity onto that window:
0 `[ws-dl-emit]` anywhere, 0 `[ws-dl-rsp]` between trigger and complete.
The RSP/RT64 identity dump after complete is clocks 1..169 and matches the
start_frame=1 intro smoke nested timeline. Neither
`GUEST_DISPLAY_LIST_SELECTION_OR_EMISSION_BOUNDARY` nor
`DOWNSTREAM_RSP_RT64_BOUNDARY` is promoted.

Arbitration artifact: `/tmp/p08-widescreen-dl-identity-arbitration-20260826T072436Z/`;
manifest SHA `f4b2c20086ced79a3d728174464a2dc673a6fff201d0c7edfe1f014739ddccff`.

### P08 DL identity & frame-join fix checkpoint (2026-08-26)

Diagnosed and corrected the previous unjoined capture defect. Hooked master Gfx
task emission at `func_80013EF0` (`0x80013EF0`) reading `task+0x08` and `task+0x0C`,
unified logging into `/tmp/tooie-ws-dl-identity.log` with mutex protection and monotonic
clocks, exported canonical `tooie_get_current_guest_frame()` / `tooie_get_current_capture_id()`,
and linked RSP HLE via `tooie_ws_log_dl_rsp()`.

```text
BUILD_RESULT: PASS
OLD_DL_EVENT_TIMING_BUG_ROOT: PROVEN
DL_IDENTITY_FRAME_JOIN_FIX_STATUS: VERIFIED
GUEST_FRAME_JOIN_ACCURACY: 100.0%
UNJOINED_DL_EVENT_COUNT: 0
DL_EVENTS_WITH_GUEST_FRAME_UNKNOWN: 0
DL_EMIT_INSIDE_CAPTURE_COUNT: 4779
DL_RSP_INSIDE_CAPTURE_COUNT: 8908
DL_RT64_INSIDE_CAPTURE_COUNT: 8908
RT64_DRAW_INSIDE_CAPTURE_COUNT: 47761
ROOT_COMMAND_BUFFER_IDENTITY: DOUBLE_BUFFER (80159600, 80160cd0)
CURRENT_ROOT_CLASS: UNKNOWN / RE-ANCHOR_REQUIRED
CURRENT_ROOT_BOUNDARY: NOT_PROMOTED
FIX_IMPLEMENTED: NO
TEMP_DIAGNOSTIC_CHANGE_IMPLEMENTED: YES
DEFAULT_RUNTIME_BEHAVIOR_CHANGED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-dl-framejoin-fix-20260826T130423Z/
LATEST_SHA256SUMS_SHA: 4c650961c173fb7ad7a924fbaaf90abe1393cc3a9d1a53adfe41807d7442cb32
```

NEXT_SINGLE_ACTION: User triggers one live capture during active gameplay exhibiting the edge geometry glitch (touch /tmp/tooie-ws-capture) to generate authoritative trace in /tmp/tooie-ws-dl-identity.log.

### P08 current arbitration — frame-joined USER_SELECTED window (2026-08-26)

Complete capture `capture_id=1` guest frames **1425..2144** (720,
`[ws-capture-complete]` present). Nested depth-1 unique count 65 equals RSP
depth-1. Consecutive nested Jaccard mean 0.997 (0 intermittent). 22 nested DLs
first_seen after trigger, 9 start-set DLs later absent, concentrated at phys
`0x0034xxxx`/`0x0035xxxx` with **no** static map/terrain bind. Persistent 49
nested DLs are guest-emitted and RSP-received. Extra-band draws occur; DL→draw
join remains NO. Identity last preserved at RSP G_DL visit.

```text
CAPTURE_VALID: YES
GUEST_NESTED_DL_SET_BEHAVIOR: CHANGING
GUEST_DL_TO_RSP_CONTINUITY: PROVEN
GUEST_DL_ABSENCE_EVIDENCE: PARTIAL
GUEST_SELECTION_FOR_PERSISTENT_DLS: REFUTED
RSP_RECEIVES_PERSISTENT_GEOMETRY_EVIDENCE: PROVEN
RSP_TO_RT64_WORKLOAD_JOIN: PARTIAL
DL_TO_RT64_DRAW_JOIN: NO
LAST_IDENTITY_PRESERVING_BOUNDARY: RSP_HLE_G_DL_VISIT
IDENTITY_LOST_AT: RSP_G_DL_TO_RT64_DRAW
RT64_IS_RENDERING_EXTRA_BAND_GEOMETRY: YES
CURRENT_ROOT_CLASS: GUEST_DISPLAY_LIST_SELECTION_OR_EMISSION_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS (later REFINED)
CURRENT_ROOT_BOUNDARY: nested depth-1 G_DL membership (focal 0035dd20)
NEXT_DISCRIMINATOR: DL_PRODUCER_BRANCH_TRACE
FIX_IMPLEMENTED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-dl-framejoined-user-arbitration-20260826T143436Z/
LATEST_SHA256SUMS_SHA: f0f9b493cf4bdac6ebe2378f8ce5b04ce09f8f4eab34058cd58ff7fed6d80f54
```

`NEXT_SINGLE_ACTION:` Trace producer of nested DL `0035dd20` — **DONE**; see producer-trace checkpoint below.

### P08 current producer trace — nested DL 0x8035dd20 (2026-08-26)

Read-only static/dynamic join of focal nested leaf `0x0035dd20` / `0x8035dd20`
on the VALID USER_SELECTED window. Storage class is relocated loaded
map-model payload. First membership branch is `func_80801174_gcmapsects`
(`0x808011CC` sphere `800E3E8C`, `0x808011F8` AABB `800E3D0C`→`func_800CAA24`).
G_DL emitter store is UNJOINED. `DYNAMIC_BRANCH_JOIN: NO`. Visual root
**NOT PROVEN**. No bypass. No production fix.

```text
FOCAL_DL_PHYSICAL: 0x0035dd20
FOCAL_DL_KSEG0: 0x8035dd20
FOCAL_DL_STORAGE_CLASS: LOADED_MAP_MODEL_PAYLOAD
FOCAL_DL_STATIC_OR_DYNAMIC: DYNAMIC_RELOCATED
FOCAL_DL_SEMANTIC_CLASS: LEAF_DISPLAY_LIST / MAP_MODEL_PAYLOAD
FOCAL_DL_MAP_TERRAIN_BIND: UNKNOWN
FOCAL_DL_FROM_GCMAPSECTS: UNKNOWN (pointer) / YES (class selector)
FOCAL_DL_FROM_GCCUBEDLL: NO
FOCAL_DL_FROM_GCSECTIONDLL: NO
FOCAL_DL_REACHES_800CAA24: YES (static section path)
FOCAL_DL_LOD_CONTROLLED: NO
DYNAMIC_BRANCH_JOIN: NO
CURRENT_ROOT_CLASS: GUEST_MAP_SECTION_DL_SELECTION_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY: func_80801174_gcmapsects (800E3E8C ∧ 800E3D0C/CAA24)
NEXT_DISCRIMINATOR: FOCAL_BRANCH_TIMELINE
FIX_IMPLEMENTED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-dl-producer-trace-20260826T150351Z/
LATEST_SHA256SUMS_SHA: 40d591c62899a56b60f73f4e990f019b1b1890b22b4e4d6a33b41b6eba48f5ee
```

`NEXT_SINGLE_ACTION:` Instrument `func_80801174_gcmapsects` + `func_800E3D0C`/`func_800CAA24` vs G_DL `0x8035dd20` membership. No bypass. No production patch.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## P08 ultrawide revalidation (2026-08-31)

The committed T_INTRO projection fix remains valid at tested 16:9. CPU/static
revalidation confirms residual authored-extent wall terminations in Scene A
at 21:9 and 32:9; Scene B shares the opening camera and OPA root. Keep
21:9+/32:9 as an open geometry/presentation decision. No production change
was made in this validation round.

Artifact: `/tmp/p08-tintro-ultrawide-revalidation-20260831T042437Z/`.

### P08 FOCAL_BRANCH_TIMELINE setup (2026-08-26)

Implemented diagnostic-only wrappers for `func_80801174_gcmapsects`'s real
helper path (`800E3E8C → 800E3D0C → 800CAA24`), using caller-frame
reconstruction. Original sphere and CAA24
returns are recorded with the existing DL `capture_id`/`guest_frame`; no
bypass, fix, generated-file edit, or RT64 edit was made in this round.

`CAA24` is a two-group separating-plane predicate over the camera/viewport
volume from `D_8012D500` and two AABB corner vectors (`1=pass`, `0=reject`).
The stable section identity is `section_ptr + section_index + entry_ptr`.
Build PASS. Launcher smoke completed 720 frames and logged 3,881 DL events but
did not reach map traversal, so branch runtime evidence remains `UNKNOWN`.

Artifact: `/tmp/p08-widescreen-focal-branch-timeline-setup-20260826T164317Z/`;
manifest SHA `f010ac8a1a33e97494a0d6d5dcb90a237bc8814399c4a01ba1328f21c9de81e3`.
Next single action: one live user capture; recommended arbitration model:
Grok 4.6 Medium.

### P08 FOCAL_BRANCH_TIMELINE user arbitration (2026-08-26)

USER_SELECTED `touch /tmp/tooie-ws-capture` produced VALID capture_id=1 frames
**1717..2436** (720/720, complete present). Focal nested DL `0x8035dd20` is
present on all 179 graphics-emit frames **1720..2432**. Neighbor `0x8035d268`
drops at 1856; `0x8035e2d8` persists. `[ws-focal-branch]` events = 0 while
`[ws-geo-e]`/`[ws-geo-d]` still fire on stack nodes. CAA24/sphere membership
control is **UNKNOWN**. Focal DL as causal anchor is **WEAKENED** for this
window (Signature D: already present). Setup artifact
`/tmp/p08-widescreen-focal-branch-timeline-setup-20260826T164317Z/` is missing
from `/tmp`. No bypass. No production fix.

```text
CAPTURE_VALID: YES
FOCAL_BRANCH_EVENTS_IN_WINDOW: 0
CAA24_CONTROLS_FOCAL_MEMBERSHIP: UNKNOWN
SPHERE_CONTROLS_FOCAL_MEMBERSHIP: UNKNOWN
FOCAL_DL_8035DD20_AS_CAUSAL_ANCHOR: WEAKENED
CURRENT_ROOT_CLASS: GUEST_MAP_SECTION_DL_SELECTION_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS
NEXT_DISCRIMINATOR: FOCAL_BRANCH_JOIN_REPAIR
FIX_IMPLEMENTED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-focal-branch-user-arbitration-20260826T233105Z/
LATEST_SHA256SUMS_SHA: 4b1867cb2145af8dc5c33b6916943ce335edf6295a886eaef9d37bddf87a7a65
```

`NEXT_SINGLE_ACTION:` Repair observation of `func_80801174_gcmapsects` so `[ws-focal-branch]` emits on a live `0x8035dd20` frame; then recapture at pop-in. No bypass. No production patch.

### P08 FOCAL_BRANCH_JOIN_REPAIR (2026-08-27)

Old RA/LOOKUP gate root cause is proven (jal PC+8, unmaterialized `r31`,
helper SP, CreateStatic, address-taken EP table). Direct hook is `ld --wrap`
of `func_80801174_gcmapsects` plus unlimited ovl=662 `[ws-gcmapsects-syscall]`
and LOOKUP rebind after `load_overlays`. Build PASS. 45s attract smoke: one
syscall `ep=15`, `GC_MAPSECTS_ENTRY_COUNT=0`,
`GC_MAPSECTS_HOOK_VERIFIED=NO`. Focal `0x8035dd20` remains WEAKENED.
`FIX_IMPLEMENTED: NO`. Artifact
`/tmp/p08-widescreen-focal-branch-join-repair-20260827T004617Z/`
(manifest `2c84d1739211aff278e4e469f3af40e816640cc2f0304ff6493d62c9d1f19cfc`).

`NEXT_SINGLE_ACTION:` Get `[ws-gcmapsects-entry] > 0` on the glitch map, then one USER_SELECTED capture. No bypass. No production patch.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

### P08 POST_GCMAPSECTS_REANCHOR (2026-08-27)

Manual gameplay showed the widescreen wall-hole while overlay 662 logged
39 syscalls (`ep4=27`, `ep15=12`, `ep9/13/17=0`) and `func_80801174` ran
**0** times. `GUEST_MAP_SECTION_DL_SELECTION_BOUNDARY` is
`CORRECTED/SUPERSEDED` for this run (`REFUTED_FOR_THIS_RUN` as a necessary
cause). ep4 is a point-in-section AABB query (`func_800EFED0`); a0 stride
`0x9C` matches `Actor`. Static world DLs do not require 80801174.
`CURRENT_ROOT_CLASS:` `RSP_TO_RT64_GEOMETRY_BOUNDARY` / `STRONG_HYPOTHESIS`.
Banjo `gEXSetNearClipping` relevance `MEDIUM`; do not implement. Artifact
`/tmp/p08-widescreen-post-gcmapsects-reanchor-20260827T012256Z/`
(manifest `3c626fd6b1b1d44520550066ef7c34d6d0c2077bb26b0b8b29325623b33e0e37`).
`FIX_IMPLEMENTED: NO`.

`NEXT_SINGLE_ACTION:` `RSP_PRIMITIVE_IDENTITY_PROPAGATION`. No bypass. No production patch. No new user capture this round.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

### P08 RSP_PRIMITIVE_IDENTITY_PROPAGATION (2026-08-27)

End-to-end identity chain from nested guest `G_DL` -> RSP HLE DL context stack ->
primitive sequence generation (`TRI1`, `TRI2`, `QUAD`, `RECT`) -> `DrawCall` batching
-> `FramebufferRenderer` host widescreen NDC extra-band classification (`minNdcX`,
`maxNdcX`, `extra_left_intersect`, `extra_right_intersect`) is implemented, built,
and verified via automated smoke test.

```text
RSP_PRIMITIVE_PROPAGATION:    IMPLEMENTED_AND_VERIFIED
DL_CONTEXT_STACK_ACTIVE:      YES
DL_CONTEXT_EVENTS:            8597
PRIMITIVES_WITH_DL_IDENTITY:  76229
RT64_DRAWS_WITH_PRIM_ID:      45506
JOIN_RATE_DL_TO_PRIMITIVE:    100.0%
JOIN_RATE_PRIMITIVE_TO_DRAW:  100.0%
CURRENT_ROOT_CLASS:           RSP_TO_RT64_GEOMETRY_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY:        RSP_G_DL_TO_RT64_DRAW
LAST_IDENTITY_PRESERVING_BOUNDARY: RT64_HOST_DRAW_EXTRA_BAND_CLASSIFICATION
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-rsp-primitive-identity-setup-20260827T015059Z/
ARTIFACT_CHECKSUM_MANIFEST:   bb7ea94f1a8f71cb101c0cc4cb38832ccd5aa219240f2526669537eadcdd88e0
FIX_IMPLEMENTED:              NO
```

Artifacts dual-verified in `/tmp/p08-widescreen-rsp-primitive-identity-setup-20260827T015059Z/`.
Single unified log `/tmp/tooie-ws-dl-identity.log`.

`NEXT_SINGLE_ACTION:` USER_SELECTED_AUTHORITATIVE_CAPTURE (user plays to visual anomaly with `TOOIE_DIAG_WS_CONTINUOUS=1` and triggers `touch /tmp/tooie-ws-capture`).
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

### P08 RSP_DRAW_USER_ARBITRATION (2026-08-27)

USER_SELECTED capture_id=1 frames **245023..245742** during a visible
widescreen wall-hole. RSP TRI* primitive→draw join **99.94%**. Every proj=1
world frame has extra-left and extra-right classified DrawCalls; the proj=1
extra-band DL set is **STABLE**. `RSP_TO_RT64_DRAW_CREATION_FAILURE` is
`REFUTED`. `CURRENT_ROOT_CLASS:` `POST_RT64_DRAW_VISIBILITY_BOUNDARY` /
`STRONG_HYPOTHESIS`. Clip/depth reject telemetry was not in this log.
Banjo `gEXSetNearClipping` stays `MEDIUM`. Artifact
`/tmp/p08-widescreen-rsp-draw-user-arbitration-20260827T030225Z/`
(manifest `2084f416c77a64082a459bf58244cc49a715972efefb454d131a441c13b914ed`).
`FIX_IMPLEMENTED: NO`.

`NEXT_SINGLE_ACTION:` `EXTRA_BAND_FRAGMENT_DEPTH_VISIBILITY`. No bypass. No production patch.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

### P08 EXTRA_BAND_FRAGMENT_DEPTH_VISIBILITY (2026-08-27)

End-to-end depth visibility and GPU submission instrumentation is implemented,
compiled, and verified via automated smoke test.

```text
POST_DRAW_CPU_REJECTION_FOUND: NO
DRAW_SURVIVES_TO_GPU_SUBMISSION: YES (100.0%)
EXTRA_BAND_DRAWS_CLASSIFIED:  29498
EXTRA_BAND_DRAWS_GPU_SUBMITTED: 29502
EXTRA_BAND_SCISSOR_EMPTY_COUNT: 0
EXTRA_BAND_DRAW_SUBMISSION_RATE: 100.0%
DEPTH_TEST_STATE_CAPTURE_READY: YES
COLOR_DEPTH_EXTENT_MATCH:     YES (304x228 / 1622x912)
DEPTH_COORDINATE_SPACE:       HOST_WIDESCREEN_PIXELS / NDC
EXTRA_BAND_DEPTH_COVERAGE_MISMATCH: REFUTED
CLIP_STATE_CAPTURE_READY:     YES
BANJO_NEAR_CLIPPING_EFFECT_STAGE: RASTERIZER_DEPTH_CLIP_ENABLE
BANJO_NEAR_CLIPPING_CAN_EXPLAIN_LATERAL_HOLES: NO
FRAGMENT_VISIBILITY_DIRECTLY_OBSERVABLE: NO (proxy active)
PASS_IDENTITY_CAPTURE_READY:  YES
CURRENT_ROOT_CLASS:           POST_RT64_DRAW_VISIBILITY_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
USER_CAPTURE_READY:           YES
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-extra-band-depth-visibility-setup-20260827T032333Z/
ARTIFACT_CHECKSUM_MANIFEST:   386063613e37c2b2272957489a0f0b740c05d803b8745503137dc126ab44d712
```

Artifacts dual-verified in `/tmp/p08-widescreen-extra-band-depth-visibility-setup-20260827T032333Z/`.
Single unified log `/tmp/tooie-ws-dl-identity.log` with `[ws-gpu-draw-submit]`.

`NEXT_SINGLE_ACTION:` USER_SELECTED_AUTHORITATIVE_CAPTURE (user plays to visual anomaly with `TOOIE_DIAG_WS_CONTINUOUS=1` and triggers `touch /tmp/tooie-ws-capture`).
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

### P08 GPU_DEPTH_USER_ARBITRATION (2026-08-27)

USER_SELECTED missing-wall capture_id=1 frames **6885..7604**, camera
**720/720 static**. Proj=1 extra-band DrawCalls **3288/3288** reach GPU
submit by primitive identity. Color/depth extents **2164x1140** match.
Near intersections **0**; far `maxNdcZ>1` is epsilon with `depth_clip=0`
(`FAR_DEPTH_CLIP_ANOMALY: REFUTED`). Smoke 29498 vs 29502 is accounting.
`CURRENT_ROOT_CLASS:` `POST_GPU_SUBMISSION_FRAGMENT_OR_COMPOSITE_BOUNDARY`
/ `STRONG_HYPOTHESIS`. Artifact
`/tmp/p08-widescreen-gpu-depth-user-arbitration-20260827T033147Z/`
(manifest `6996bd3bcf9974c86d95fc4650f08bc8b061faf407b5fa80ed39288aa94c2eca`).
`FIX_IMPLEMENTED: NO`.

`NEXT_SINGLE_ACTION:` `SEMANTIC_WALL_DRAW_LOCALIZATION`. No bypass. No production patch. No GPU readback.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

### P08 POST_SUBMISSION_COMPOSITE_STATE_TRACE (2026-08-27)

Static + CPU present telemetry. Pixel probe `NOT_COMPLETED / ABANDONED_FOR_SAFETY`
(three Vulkan incidents). Extra-band proj=1 GPU submit remains proven;
missing-wall-specific draw remains `NOT_PROVEN`. VI `viewRectangle`/`cropRectangle`
are `{0,0,1,1}`. Smoke: `uvMax.x=1.0` on 38 presents; gameplay VI `304x228`
allocates wide host target `1622x912` and samples all of it. Decision **B**
`GPU_FRAGMENT_RASTER_OR_SEMANTIC_BOUNDARY`. Artifact
`/tmp/p08-widescreen-post-submission-composite-audit-20260827T052315Z/`
(manifest `444e33af976f32ba2c4f94409a2210bf80871346281554e9891c66291bb2a2d2`).
`FIX_IMPLEMENTED: NO`.

### P08 SEMANTIC_WALL_DRAW_LOCALIZATION (2026-08-27)

CPU-only. USER GPU artifact missing from `/tmp`. Persistent USER DLs
`0031e***` have 0 hits in remaining logs. Smoke `[ws-wall-bounds]` ranks
map-RAM extra-band DLs (`0031f6d8` vertical subset is scene-local only).
`MISSING_WALL_DRAW_SEMANTIC_JOIN: NO`. Artifact
`/tmp/p08-widescreen-semantic-wall-localization-20260827T063054Z/`
(manifest `2bed363d288b066a76cb656047e2a15c5e988684ad4442ed67aafb67d62e7d51`).
`FIX_IMPLEMENTED: NO`.

### P08 CPU_SCREEN_BOUNDS_WALL_CANDIDATE_ARBITRATION (2026-08-27)

USER_SELECTED capture_id=1 frames **13309..14028**, camera **720/720 static**
`(3467.4,105.0,4735.7)`. Screenshot hole NDC `x[-1.00,-0.76] y[-0.30,1.00]`.
Top cluster: MAP_RAM `00349d38` extra-left TRI1, world
`(2245,-153,3680)-(2497,400,4263)`, `vert_frac=1`, `gpu_submitted=1` 318/318.
`MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL`.
`MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED`.
`CURRENT_ROOT_CLASS:` `WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY`.
`CULL_WINDING_ANOMALY: UNKNOWN`. Artifact
`/tmp/p08-widescreen-user-wall-bounds-arbitration-20260827T151459Z/`
(manifest `cd50d98a55c0ab24bb1ed3d982ae4b648ba148b3bdc2ed0182f31189dd6018b4`).
`FIX_IMPLEMENTED: NO`.

`NEXT_SINGLE_ACTION:` `TARGETED_CULL_WINDING_TRACE`. No bypass. No production patch. No GPU readback.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

### P08 TARGETED_CULL_WINDING_TRACE (2026-08-27)

No new capture. Screenshot mapping recorded as **APPROXIMATE**. Semantic join
stays **PARTIAL**. Focal `00349d38` extra-left TRI1 GBI indices `0,1,2`; object
xyz **not in capture**. Positive X aspect scale **cannot** flip winding.
`SHADER_CULL` = specialized PSO **CullMode FRONT**. `CULL_WINDING_ANOMALY:
UNKNOWN`. Artifact
`/tmp/p08-widescreen-targeted-cull-winding-20260827T152926Z/`
(manifest `c260d0fd338f4b14039e65bc70a33601318682f7686ba5c72441cc18e0d2eb56`).
`FIX_IMPLEMENTED: NO`.

`NEXT_SINGLE_ACTION:` `TARGETED_FOCAL_VERTEX_WINDING_DUMP`. No bypass. No production patch. No GPU readback.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

### P08 TARGETED_FOCAL_VERTEX_WINDING_USER_ARBITRATION (2026-08-27)

USER capture `capture_id=1` frames **8278..8997**, camera static
`(3477.8,113.9,4682.8)`. Real V0/V1/V2 for `00349d38` TRI1 (same world AABB as
the prior extra-left candidate). Geometric **FRONT**, source/target NDC **CCW
MATCH**, expected raster survival **YES**. Same-DL FRONT TRI2 cluster matches.
`CULL_WINDING_ANOMALY: REFUTED_FOR_FOCAL`. Semantic join stays **PARTIAL**.
Artifact `/tmp/p08-widescreen-focal-winding-user-arbitration-20260827T193119Z/`
(manifest `f1499333a0fcdb1395f550848e8539026419058f8935c4c8c6b3a9092943667c`).
`FIX_IMPLEMENTED: NO`.

`NEXT_SINGLE_ACTION:` `TARGETED_RASTER_STATE_TRACE` on `00349d38` TRI1. No bypass. No production patch. No GPU readback.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

### P08 TARGETED_FOCAL_VERTEX_WINDING_USER_ARBITRATION (2026-08-29)

New USER capture `capture_id=1` frames **3754..4473**, camera static
`(3598.4,57.8,4908.3)` at the same missing-wall location. Real V0/V1/V2
identical to the extra-left TRI1 AABB. Geometric **FRONT** (`dot=-0.5196`),
source/target NDC **CCW MATCH** (`Sx=0.702785` for this ~1920x1012 window vs
4:3), expected raster survival **YES**. Same-DL FRONT TRI2 cluster matches;
BACK TRI2 and `0034b800` are culled as expected.
`CULL_WINDING_ANOMALY: REFUTED_FOR_FOCAL`. Semantic join stays **PARTIAL**.
Artifact `/tmp/p08-widescreen-focal-winding-user-arbitration-20260829T164524Z/`
(manifest `d0d993e6e015d482e6807350b8d65de993d6348b7f8d96287972e9d1ee13e489`).
`FIX_IMPLEMENTED: NO`.

### TARGETED_RASTER_STATE_SETUP (2026-08-29)

Targeted raster state telemetry implemented in RT64 `FramebufferRenderer::addDrawCall`
under `TOOIE_DIAG_WS_FOCAL_RASTER=1` logging to `/tmp/tooie-ws-focal-raster-state.log`
with event tag `[ws-focal-raster-state]`.
Audited all 7 possible fragment discard sites in `RasterPS.hlsl` (0 active for focal `00349d38` TRI1).
`POST_PROJECTION_NATIVE_COORDINATE_MISMATCH` is **REFUTED**.
`FIRST_FOCAL_VS_CENTRAL_STATE_DIFFERENCE` is **NONE_OBSERVED**.
Artifact `/tmp/p08-widescreen-targeted-raster-state-setup-20260829T173856Z/`
(manifest `fbd3366d1a666196a77c536779cd3b087a0957382ac60bee2cbffe334c81fc14`).
`FIX_IMPLEMENTED: NO`.

`NEXT_SINGLE_ACTION:` User performs ONE capture at same wall, camera static, with `TOOIE_DIAG_WS_CONTINUOUS=1 TOOIE_DIAG_WS_WALL_BOUNDS=1 TOOIE_DIAG_WS_FOCAL_WINDING=1 TOOIE_DIAG_WS_FOCAL_RASTER=1`. Then Grok 4.6 Medium arbitrates.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

### P08 TARGETED_RASTER_STATE_USER_ARBITRATION (2026-08-29)

USER capture `capture_id=1` frames **2926..3645**, camera static
`(3584.1,50.7,4893.8)` on host viewport **1622x912** (`Sx=0.750`). Focal
`00349d38` extra-left TRI1 (exact world verts) is GPU-submitted 286/286 on
`proj=2` with depth test/write **LESS**. `PRIOR_PROJ1_DEPTH_RESULT_APPLIES_TO_FOCAL: NO`.
Explicit fragment discard **0/7**. Alpha/coverage/color-write/blend-zero
**REFUTED_FOR_FOCAL**. Scissor intersects (**PARTIAL** at top clip). First
runtime state difference vs same-DL FRONT TRI2 is **tex_id**. CPU AABB finds
no strictly closer visible prior depth writer; depth-test result is **not
observable** (`DEPTH_OCCLUSION_RESULT: UNKNOWN`). Semantic join stays
**PARTIAL**. Artifact
`/tmp/p08-widescreen-focal-raster-user-arbitration-20260829T181535Z/`
(manifest `a8ce5b47e43ca92986ce19628cc1c1084e6ff2fa22079dc4f17197bcf8636190`).
`FIX_IMPLEMENTED: NO`.

`NEXT_SINGLE_ACTION:` `TARGETED_FOCAL_TEXTURE_TMEM_TRACE` on `00349d38` TRI1 vs same-DL FRONT TRI2. No GPU readback. No depth bypass. No production patch.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

### P08 T_INTRO presentation policy (2026-08-31; deferred)

The centered 16:9 max-aspect presentation candidate was tested acceptable,
then deferred and removed from the worktree. Its RT64 seam, Tooie hook, and
candidate test are not current behavior. The committed dynamic T_INTRO
projection compensation remains unchanged. Static validation of the removed
candidate had passed before deferral.

`PILLARBOX_CANDIDATE_FINAL_STATUS:` `DEFERRED_AND_REMOVED_FROM_WORKTREE`.

### P08 Attract Demo 7 stationary cinema drift resolution (2026-10-17)

User-observed player drift during Demo 7 in the attract sequence (TDL Stomping Plains, stage 0 sub 8) was forensically isolated, root-caused, and resolved at the owning generator layer without hand-editing generated code:
1. **Kinematics & State Struct**: Disassembly of `bs_setState` (`0x8009E5C8`) proved layout `*(player + 0x120)` maps `+0: prev`, `+4: cur`, `+8: next`. During overhead slide, Banjo executes State 116 (`0x74`, `_bsrest_entrypoint_12`) with `anim = 111` while `bs_prev = 4`.
2. **Root Cause**: In N64 hardware, `bs_setState(116)` calls INIT ep9 (`0x808005B8`) -> `0x8080020C` -> `0x80800308` -> helper `0x80800000` (`static_344_80800000`), which sets target horizontal speed to 0 (`baphysics_set_target_horizontal_velocity`) and clears velocity vectors to (0, 0, 0). In recompiled code, `N64Recomp` static function end-detection logic truncated `static_344_80800000` to an empty function because the decompilation ELF contained zero-size NOTYPE markers (`bsrest_VRAM`, `bsrest_TEXT_START`) at `0x80800000`.
3. **Fix & Regeneration**:
   - Upstream patch `patches/runtime/n64recomp-zero-size-static.patch` applied to `N64Recomp/src/main.cpp` via `tools/apply_runtime_patches.sh` (skips zero-sized symbols sharing the static address).
   - Validated via regression test `tests/n64recomp_zero_size_static_test.sh` (fixed generator PASS, old generator FAIL).
   - Decompilation inputs restored via pinned fork `25033be6` (ROM SHA1 `af479771ff5e91ce3732f3d84146614ab71d8d79`, ELF SHA1 `2b2d04c5b6484594f750b097daf670f7a8a24167`, size 91855676).
   - Full regeneration executed via official tooling (`tools/run_recomp.sh` / N64Recomp); 420 files regenerated; exactly 5 files changed (2 restored helpers in `funcs_416.c`, 4 rounding aliases semantically identical). Full build exit 0; `tools/check_generated_fresh.py` exit 0.
4. **Verification**:
   - Independent Grok final gate on fixed runtime capture (`demo7-fixed-root.log`, 121 rows, controller 396.13s): Criterion 1 (Bounded Demo 7 drift) PASS. 119 pose pairs match N64 reference within quantized rounding tolerances; pad inputs match 120/120; glidx 38..120 is stationary at `(-80.46, 0.00, 4496.25)`, `velxz = 0.0`, `tspd = 0.0`.
   - Criterion 2 (global real-time cadence): 90.032s serial timing test (`timing-fixed-pid-serial-90s.log`) completed exit 0 with PID-tagged serialization fix in `src/game/tooie_diag.cpp`. Guest stream (PID 938341) over 59.038s delta interval (wall 20.004s..79.042s) yielded 60.0121955 VI Hz (3543 VIs) and 24.3063789 logic Hz (1435 logic updates) across changing scenes (~20 or ~30 Hz per scene). Stream PID 938331 had 0 VIs. Bounded conclusion: NO evidence of a 2x VI clock in the measured window. This does NOT constitute universal camera/mob/campaign timing proof; user-specific non-Demo7 reproduction question remains open.

`DEMO7_BOUNDED_PARITY_STATUS:` `PASS` (Criterion 1; source bug CLOSED for tested window).
`GLOBAL_VI_CADENCE_STATUS:` `NO_2X_VI_IN_MEASURED_WINDOW` (~60.012 VI Hz; universal mob/campaign parity unproven).

### P08 Attract Demo 1 subtle movement and action parity investigation (2026-10-18)

Following visual resolution of Demo 7, user reported that Fire Egg still misses on the current build (`87c5...`) and directed focus to general subtle gameplay/action parity across movement, physics, and state:
1. **Diagnostic & Build**: Claude Sonnet implemented isolated default-OFF flags `TOOIE_DIAG_DEMO1_PARITY=1` and `P08_DEMO1_PARITY=1` at PC `0x800151BC` ENTRY (start of logic update, 62 columns, unit test `tooie_demo1_parity_test` PASS exit 0, ninja build exit 0, oracle rebuild exit 0).
2. **Paired Prefix Captures**: Current Tooie (PID 1073249/1073258, 67.68s, exit 0) and pinned Angrylion oracle (PID 1074236/1074257, 98.71s, exit 0) executed cleanly to `window_consumed_420`.
3. **Comparator Results**: 420 exact non-clock matched pairs (`consumed = 1..420`). 0 mismatches across pad inputs `r0..r5`, gamespeed `gs`, facing and ideal yaw, position, velocity, `velxz`, `tspd`, `ptype`, `bs_prev/cur/next`, animation, and viewport camera. Both sides skip only consumed 0.
4. **First In-Window Divergence**: VI-handler entry cadence (`0x8001443C` delta-VI mismatches on 100/419 transitions; Tooie +847 vs oracle +887 entries). Counter difference under `--nospeedlimit`; did not perturb sampled guest fields.
5. **Pair 2 Projectile Liveness & Hit Divergence (PROVEN)**:
   - Oracle ALIAS at post 341 recovered same owner (`0x80133530`) at new heap address `0x801d085c`, restoring trajectory. Old freeze hypothesis `CORRECTED/SUPERSEDED` as a stale pointer diagnostic artifact.
   - 21 movement pairs (`sueggmove9`), 22 sweep input pairs (`sweep800CD820`) including gamespeed `gs` (w11) bit-exact.
   - First different return at post 355: `func_800CD820` original `v0 = 0x80132f60` (hit) vs port `v0 = 0` (miss).
6. **Pair 3 Sweep Branch & Qualifying Center Divergence (PROVEN)**:
   - Query radius 38 (`0x42180000`), sweep geometry and `gs` identical bit-for-bit at posts 354 and 355 across 5 spatial lists (counts 2, 4, 2, 1, 3; 24 candidates total).
   - First candidate center differs at both 354 and 355: post 354 delta length 431.2900595 (curr `(-5239.28, -497.95, 1552.27)` vs orig `(-5617.35, -519.25, 1345.82)`); post 355 delta length 427.2311169 (curr `(-5235.09, -496.97, 1541.99)` vs orig `(-5615.57, -519.27, 1348.95)`). Radius 279.449982 is equal on both runtimes; other 2 qualifying centers and radii match identically.
   - Parser note: PAR3 Haiku flaw raw pointer candidate comparison marked `INVALID_PARSER`; 10 lists `NOT_TESTED` in Haiku but native check (`demo1-sweep-par3-native-check.json`) proves counts; do not call game test FAIL from parser.
7. **Target Helper Structure & Halfword Reader Bug (PROVEN MIPS)**:
   - `func_80106790(marker)` resolves `vector_at(*D_80136EE0, lhu(marker+0x1A) >> 5)` with stride 0x9C (actor record in global actor vector).
   - `_subaddiecustomhits_entrypoint_5` (`0x8008A018`): if descriptor word bit 1 is set, copies actor record `+4/+8/+0xC` position floats; else branches to `func_800EC75C` custom offset. Target class identity and branch unknown until source proof.
   - Old `tooie_d1p_hu` halfword reader had a 4-alignment bug, returning 0 for halfwords at `+2` within an aligned word (including `+0x66`, `+0x6A`, `+0x1A`). Old zero was UNTRUSTWORTHY on Tooie; fixed in diagnostic helper.
8. **Target Selector Fix & Paired Target-2 Telemetry (PROVEN)**:
   - Initial target selector attempt produced 0 rows (FAIL) because it required mutable `0x0E3F` in `lhu(marker+0x18)`, whereas enter rows show `0x0E21` on both runtimes.
   - Selector fixed to `lhu(actor+0x14) == 0x0671 && (lhu(actor+0x18) & 1) != 0` (ID 0671 + active bit 0); verified on real fixtures (old FAIL / new PASS exit 0; Haiku gate PASS exit 0; E3F semantic caveat noted from source `v0+14/18` in sweep leave).
   - Paired capture `demo1-target-{current,oracle}-2` completed exit 0, FINISH 357, 359 rows each.
   - Capture revealed 3 handles sharing ID `0x0671`: bound first `0x80132f30` (post 2), ambiguous `0x80132f00` (post 28), actual hit actor `0x80132f60` (post 140). Thus sampler bound `0x80132f30`, not actual hit actor trajectory.
   - First actor `0x80132f30` positions 355 pairs BIT-EXACT posts 2..82; FIRST raw position divergence at post 83 (curr `44e16cc0 c309103c c30eda4e` vs orig `44e16e09 c3090ffe c30ea630`); `gs` (w11) exact across all 355 posts; other record words first differ at post 121 (meaning unknown); first actor callback record `0x80085c98` (getter class descriptor, not update entry).
9. **Active Background Agents & Audit Status**:
   - Sonnet quota expired; Grok 4.7 Medium implementation session 01a15118-c738-7dd3-98ed-cc5db5021bb5 completed end_turn exit 0, landing binary A6A2.
10. **Preserved Invariants & Non-Goals**:
    - Demo 7 cutscene fix visually confirmed. No default flag OFF, no speed, aim, or audio modifications, no manual generated changes. P09 Phase 1 default CURRENT tfPos workaround remains default-ON (palliative); FPS counter/toggle, RT64 decimator, and audio deferral unchanged.
11. **Frustum Parity Narrow Allowlist Landed (PROVEN)**:
    - Shared `func_800E3E8C` broad DRAW scope dropped; allowlisted pure-draw `func_800B97A4` (scope kind 3) + preserved `func_8011447C` (kind 2). Mixed renderer `func_800DE498` stays original `v0`.
    - Under actual default-ON binary A6A2: 355 paired trajectory samples x 12 words, 546 ActorRecord words, and 14 RNG snapshots match original bit-exact (`+0x60 = 0003006c`). Post 355 sweep returns `v0 = 0x80132f60` (hit) and `CD6B4 = 1` matching original.
    - Test harness reports `pure_draw_accepts=1`. TARGET window recorded `cacecx = 0` (not promoted). Widescreen edge visual QA for tested area confirmed by direct user test: *"Sim, continua funcionando"* (`VISUALLY_CONFIRMED_FOR_TESTED_AREA`, `USER_REPORTED`; broader game/objects untested residual).

`DEMO1_PAIRED_PARITY_STATUS:` `EXACT_355_MATCH_UNDER_DEFAULT_ON` (355x12 trajectory words, 546 record words, 14 RNG snapshots bit-exact with original under default-ON binary A6A2).
`DEMO1_COLLISION_HIT_STATUS:` `HIT_RESTORED_UNDER_DEFAULT_ON` (oracle hit v0=0x80132f60, CD6B4=1, 22 sweep + 21 move pairs exact under default-ON binary A6A2).
`NEXT_SINGLE_ACTION:` extend oracle comparison to another attract demo when continuing broader fidelity.
