---
title: "BT P09 — Paridade de gameplay e melhorias"
description: "P09 top-left present FPS counter and Graphics settings toggle verified; RT64 present success counter and RmlUi overlay hook documented; unit tests PASS; runtime QA pending."
status: pending
type: implementation-stage
project: TooieRecomp
phase: P09
order: 9
depends_on: P08
updated: 2026-10-17
tags: [banjo-tooie-Recomp, parity, gameplay, framerate, widescreen, fps, overlay]
---

# BT P09 — Paridade de gameplay e melhorias

Registro operacional vivo: [[../../plan/evidence/p09-current-work]]. Manter
sincronizado com os handoffs a cada mudança; confirmar build atual antes da
captura V2. O acompanhamento do residual não abre formalmente a fase P09.

## Resultado esperado

Comportamento funcional equivalente ao original em timing padrão, seguido por
melhorias opcionais que não alteram gameplay.

## Estado corrente (P09-RES-05 — Top-left present FPS counter & settings toggle verified, 2026-10-17)

Evidências: [[../../plan/evidence/p09-fps-counter-and-settings-toggle]], [[../../plan/evidence/p09-current-work]], [[../../plan/evidence/p09-default-current-workaround-and-fps-plan]], [[../../plan/evidence/p09-session-handoff]], [[../../specs/Tooie/graphics-and-presentation]] e [[../../HANDOFF]].
Sob autorização explícita do usuário, o contador de FPS no canto superior esquerdo e o toggle de configurações ("Show Present FPS") foram verificados na implementação, patches versionados gerados para os submódulos, testes unitários de host executados com sucesso e documentados. O contador mede a cadência real de swapchain present, e não a taxa alvo selecionada nem a taxa de simulação/movimento. O toggle possui padrão OFF (`false`), persiste em `graphics.json` e reutiliza o hook de render-thread RmlUi do RecompFrontend sem criar UIs paralelas. A validação de runtime em gameplay com observação visual permanece pendente.

- Status & flags: `DEFAULT_WORKAROUND_IMPLEMENTED: YES`, `BELOW_NATIVE_DECIMATOR_IMPLEMENTED: YES`, `FPS_COUNTER_AND_TOGGLE_VERIFIED: YES`, `PERMANENT_INTERPOLATION_FIX_NOT_IMPLEMENTED: YES`, `FPS_PRESETS_UI_PLANNED_NOT_IMPLEMENTED: YES`, `P09_FORMAL_PHASE: PENDING / NOT_FORMALLY_OPEN`, `NO_COMMIT: YES`, `NO_PUSH: YES`.
- Configuração e persistência:
  - Opção booleana "Show Present FPS" em `add_graphics_options` (`src/game/config.cpp:186-198`), chave `banjo::configkeys::graphics::show_fps_counter` (`include/banjo_config.h`).
  - Padrão: `false` (desmarcado). Persistência automática em `graphics.json`.
  - Instruções de uso: Menu de configurações -> aba Graphics -> marcar "Show Present FPS". Ativação imediata via callback `tooie::set_fps_overlay_enabled()`.
- Mecanismo do contador e overlay:
  - Medição: `RT64::presentSuccessCount` incrementado estritamente em `ext.swapChain->present() == true` (`rt64_present_queue.cpp:513`). Quadros pulados pelo decimador e falhas de present não incrementam.
  - Pacing desacoplado: `presentTimestamp` em `rt64_present_queue.cpp:508` é relógio de pacing para `preciseSleepUntil` (alvo acima da nativa), não métrica de FPS. Gráfico ImPlot (`rt64_state.cpp:2465`) permanece exclusivo do modo desenvolvedor.
  - Medidor de cadência: `tooie::PresentRateMeter` (`include/tooie_fps_meter.h`) calcula taxa inteira arredondada sobre janela deslizante de 1 s. Pausas/gaps > 3 s resetam a janela e exibem `"-- FPS"`.
  - Overlay: hook `recompui::set_overlay_update_callback` em `RecompFrontend` (`ui_state.cpp`), chamado na render thread em `draw_hook()`. Renders a 16 dp do topo-esquerdo com fundo escuro; não é um contexto exibido (não é fechado por `hide_all_contexts()`, zero captura de input).
- Patches versionados & cadeia de aplicação limpa:
  - `tools/apply_runtime_patches.sh` aplica em pin limpo RT64 (`6f1c2d99`) e RecompFrontend (`d0d90ba`):
    - `patches/rt64/rt64-rsp-endpoint-current.patch` (SHA256 `49925480c2d1736c790ddf96152844951f6fe27a150ab1cc110347d557c0d813`)
    - `patches/rt64/rt64-present-fps-decimator-and-counter.patch` (SHA256 `6d746f27ed1bd3132dcea8464adeb25dcfac4ad38502bf92f402ea6e3a19836c`) — substitui de forma cumulativa os patches separados de decimador e contador.
    - `patches/rt64/rt64-p09-exact-use-correlation-header.patch` (SHA256 `0d4c250a5c4950dcdb1d0835ea7347014c90893e93332caa5576c67e03459561`)
    - `patches/frontend/recompfrontend-overlay-update-callback.patch` (SHA256 `37266d0bd13d42c0b8f174c0118b6a9b2490784c5880f05e6ee37ab811cd48a7`)
  - Patches incrementais históricos mantidos para proveniência, porém SUPERSEDED para aplicação em pin limpo: `patches/rt64/rt64-present-success-counter.patch` e `patches/rt64/rt64-present-rate-decimator.patch` (ambos falham com exit 1 em pin limpo). Veja [[../../plan/evidence/checkpoint-20261019-review-and-patch-replay]].
- Evidências de testes e build:
  - Teste unitário de host: `./build/tooie_fps_meter_test` exit 0 (`tooie_fps_meter_test PASS`). Testa janela de 1 s, arredondamento, zero presents, stalls > 3 s, relógio invertido e reset.
  - Diff check: `git diff --check` exit 0 em Tooie, RT64 e RecompFrontend.
- Limites epistêmicos:
  - QA em runtime NÃO executada: nenhum processo de jogo lançado; sem observação visual in-game do overlay neste checkpoint.
  - Cadência de apresentação vs taxa de simulação: sob o contorno paliativo padrão CURRENT `tfPos`, apresentações acima da taxa nativa repetem a pose de vértice; contador próximo a 60 FPS com movimento aparente a ~30 Hz reflete a frequência real de swapchain present, não simulação a 60 Hz.
  - Frequência nativa do guest: `viOriginalRate` permanece formalmente `UNKNOWN` (não medido).
  - Seletor de 8 presets na UI: permanece `PLANNED` / `NOT_IMPLEMENTED` (`REQ-GFX-06`).
  - Paridade de gameplay global e cadência em tempo real: o escopo estendido do usuário (paridade global de comportamento e velocidade para jogador, mobs e câmera) **NÃO ESTÁ COMPROVADO COMO RESOLVIDO** universalmente. A paridade de pose/câmera/animação foi aprovada na cena delimitada do Demo 7 (Critério 1: PASS). O teste de cadência serial de 90 s (`timing-fixed-pid-serial-90s.log`) com fix de mutex/PID no logger (`src/game/tooie_diag.cpp`) confirmou cadência de VI em ~60.012 Hz (PID 938341: 3543 VIs em 59.038 s; 24.306 logic Hz) sem evidência de relógio de VI 2x na janela medida. Isso NÃO prova timing idêntico para todos os mobs/câmeras/campanha nem resolve questionamentos específicos de reprodução fora do Demo 7. Não promover paridade geral de mobs ou campanha.
  - P09 formal permanece: `PENDING / NOT_FORMALLY_OPEN`.

Próxima ação única: **normal clean user visual QA Demo 7, and concrete non-Demo 7 speed reproduction if persists.**


## Estado anterior (P09-RES-05 — RT64 present decimator implemented & build gates passed, 2026-10-17)

Evidências: [[../../plan/evidence/p09-current-work]], [[../../plan/evidence/p09-default-current-workaround-and-fps-plan]], [[../../plan/evidence/p09-session-handoff]], [[../../specs/Tooie/graphics-and-presentation]] e [[../../HANDOFF]].
A decimação de apresentação para taxas abaixo da nativa foi implementada na fila de apresentação do RT64 (`PresentQueue`), eliminando o comportamento anterior em que taxas <= nativa não realizavam decimação ou repouso. O contorno padrão CURRENT `tfPos` (Fase 1) permanece ativo. O seletor de 8 presets na UI do frontend e a validação de runtime em gameplay com medição de FPS permanecem pendentes.

- Status & flags: `DEFAULT_WORKAROUND_IMPLEMENTED: YES`, `BELOW_NATIVE_DECIMATOR_IMPLEMENTED: YES`, `PERMANENT_INTERPOLATION_FIX_NOT_IMPLEMENTED: YES`, `FPS_PRESETS_UI_PLANNED_NOT_IMPLEMENTED: YES`, `P09_FORMAL_PHASE: PENDING / NOT_FORMALLY_OPEN`, `NO_COMMIT: YES`, `NO_PUSH: YES`.
- Mecanismo do decimador: `PresentDecimator` (`src/hle/rt64_present_decimator.h`) com acumulador inteiro de ticks (`ticks += targetRate`, `ticks >= originalRate`), garantindo taxa média exata para divisores (20, 30) e não divisores (45, 70, ...) sem desvio de ponto flutuante ou overflow de inteiros (`uint32_t`). Atua sob `manualRate && !present.paused && targetRate > 0 && viOriginalRate > 0 && targetRate < viOriginalRate`.
- Prova estática do fluxo de ACKs: quadros pulados reutilizam o caminho existente de catch-up skip em `PresentQueue::threadLoop()` (`skipInterpolation()`, `notifyPresentId(present)`, liberação de framebuffer e `threadAdvanceBarrier`). Todos os workloads e ACKs são consumidos na ordem original; threads de guest, VI e áudio nunca dormem nem pausam.
- Correção de data race e resets (Sonnet review followup):
  - Data race eliminada: `rt64_workload_queue.cpp:1021` escrevia `viOriginalRate` sem lock enquanto `PresentQueue` lia sob `configurationMutex`. A escrita agora ocorre sob `std::scoped_lock(ext.sharedResources->configurationMutex)`. Ordem de locks verificada: `configurationMutex` é leaf lock (sem inversão).
  - Resets de cadência: catch-up skip reseta explicitamente o decimador (`presentDecimator = PresentDecimator()`); quadros pausados chamam `shouldPresent()` com `paused == true`, resetando a cadência para que o primeiro quadro pós-pausa seja apresentado imediatamente.
- Evidências de testes e build:
  - Teste unitário de host: `tests/rt64_present_decimator_test.cpp` adicionado ao `CMakeLists.txt` (`rt64_present_decimator_test`), exit 0 (`PASS`).
  - Teste de contorno: `tooie_rsp_endpoint_current_test` exit 0 (20/20 PASS).
  - Build do Root: `cmake --build build --target TooieRecompiled rt64_present_decimator_test tooie_rsp_endpoint_current_test -j2` exit 0 (`root-build.log`, `[362/362] Linking CXX executable TooieRecompiled`).
  - Gate incremental final: `root-build-final.log` concluído com exit 0 (`[6/6] Linking CXX executable TooieRecompiled`).
  - Patch versionado: `patches/rt64/rt64-present-rate-decimator.patch` (histórico; substituído para clean pin por `patches/rt64/rt64-present-fps-decimator-and-counter.patch`).
  - Diff check: `git diff --check` exit 0 em Tooie e RT64.
- Limites epistêmicos:
  - QA em runtime NÃO executada: nenhum processo de jogo foi lançado; sem medições reais de FPS ou garantias de velocidade em gameplay.
  - Prova do caminho de ACK é estática via fluxo de controle no código.
  - Taxas acima da nativa: geram apresentações extras, mas com o contorno CURRENT `tfPos` ativo, repetem a mesma pose; movimento suave acima da taxa nativa requer a correção permanente de interpolação.
  - Frequência nativa do guest: `viOriginalRate` permanece formalmente `UNKNOWN` (não medido).
  - Seletor de 8 presets na UI: permanece `PLANNED` / `NOT_IMPLEMENTED`.

Próxima ação única: **implement the 8-preset UI selector widget in RecompFrontend and perform runtime QA with FPS measurements.**

## Estado anterior (P09-RES-05 — default workaround implemented & FPS presets planned, 2026-10-16)

Evidências: [[../../plan/evidence/p09-current-work]], [[../../plan/evidence/p09-default-current-workaround-and-fps-plan]], [[../../plan/evidence/p09-session-handoff]] e [[../../HANDOFF]].
Conforme nova diretriz explícita do usuário, a Fase 1 implementou o contorno paliativo (shader-local `tfPos` CURRENT) como padrão operacional de TooieRecomp, dispensando flags manuais. A Fase 2 (seletor com 8 presets de taxa de quadros: 20, 30, 45, 60, 70, 100, 120, 144 sem alteração de velocidade de simulação) foi auditada e planejada, mas NÃO foi implementada neste momento, permanecendo bloqueada por restrições reais de ritmo de apresentação (pacing) abaixo da taxa nativa na fila de trabalho do RT64 (`WorkloadQueue`).

- Status & flags: `DEFAULT_WORKAROUND_IMPLEMENTED: YES`, `PERMANENT_INTERPOLATION_FIX_NOT_IMPLEMENTED: YES`, `FPS_PRESETS_PLANNED_NOT_IMPLEMENTED: YES`, `P09_FORMAL_PHASE: PENDING / NOT_FORMALLY_OPEN`, `NO_COMMIT: YES`, `NO_PUSH: YES`.
- Mecanismo do contorno: `apply_default_before_renderer_init()` (`src/game/tooie_rsp_endpoint_current.cpp:14-38`) é executado em `src/main/main.cpp:1153` antes de `activate_tooie_guest_preemption()` e `recomp::start()`. Caso `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` não esteja definida, define `"1"`. Valores explícitos (`"0"`, `""`, `"1"`, `"keep"`) são preservados. O valor explícito `0` restaura a interpolação original experimental. O executável normal `./build/TooieRecompiled` não requer flags.
- Escopo e comportamento de renderização: Escopado estritamente à integração Tooie; RT64 mantém padrão OFF. Utiliza bypass local no shader `RSPProcessCS.hlsl:78-84` (`tfPos`), NÃO altera pesos globais (`TOOIE_DIAG_FORCE_CURRENT_FRAME` inalterado). Posições de vértices avaliam em CURRENT; pesos de projeção/view/lookat/texgen/fog e lógica de guest inalterados. Sem alegação de interpolação contínua a 60+ FPS.
- Regra para diagnósticos futuros: Capturas futuras de interpolação DEVEM definir explicitamente `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0` para evitar mascarar anomalias visuais enquanto testemunhas em CPU supõem interpolação ativa. Coletores pesados permanecem OFF.
- Evidência de build e testes: `cmake --build build --target tooie_rsp_endpoint_current_test` exit 0; `./build/tooie_rsp_endpoint_current_test` exit 0 (20/20 asserções PASS, log persistido em `/tmp/p09-default-workaround/policy-test-root.log`). Executável `TooieRecompiled` relincado exit 0 (binário SHA256 `0d2e4ba5fa7e4ed0347d83180ff4c831c54b8f47567fa012de15dac3dc736b84`). RT64 HEAD `6f1c2d99` intocado (31 arquivos dirty preservados). Checagem de ordem estática no código-fonte, não traço GPU/runtime. Testes não tocados (38, 7, 1354) NOT RUN nesta rodada.
- Proveniência de execução e QA: Relato do escritor indicando "Game was not launched" reflete proveniência do sandbox de escrita. O Root lançou efetivamente `./build/TooieRecompiled` sem flags (`/tmp/p09-default-workaround/normal-launch.log`, ~852KB). Usuário observou em gameplay: "Está rodando bem. O áudio tem o mesmo glitch de sempre que já tinha desde quase o início do projeto e pretendo resolver o áudio só mais para frente." Glitch de áudio preexistente explicitamente postergado; sem alterações em áudio.
- Fatos de pacing da Fase 2: Interface já aceita valores manuais inteiros de 20 a 240 (`ui_config_tab_graphics.cpp:311-313`); runtime aceita de 10 a 1000. Esses valores genéricos NÃO correspondem ao seletor exato de 8 presets solicitado. No RT64 (`rt64_workload_queue.cpp:1175`), a interpolação/pacing só atua se `targetRate > viOriginalRate`. Abaixo da taxa nativa (`targetRate <= viOriginalRate`), `generateInterpolated=false` e `displayFrames=1`, enviando uma apresentação por carga nativa sem limite ou pausa. Logo, a honestidade em 20/30 FPS não é garantida (Hz nativo do guest é UNKNOWN). Preservar todos os updates de guest/VI/áudio sem pular trabalho de guest exige decimação real de apresentação; backpressure `waitForWorkloadId` é UNVERIFIED. Arquivo `graphics.json` inalterado (`Display`, manual 60).
- Fila de pesquisa preservada: Pesquisas anteriores de grupos de empate (primeiros 20), empates de nearest-join NDC e análise de soluções do Sonnet são preservadas como histórico e fila diferida; causa raiz não é reaberta e aliases não são implementados.

Próxima ação única: **singular focused audit/measurement of native cadence and present-queue backpressure to choose a below-native presentation cap without clock changes; then implement presets with actual tests all 8 and timing QA.**

## Estado anterior (P09-RES-05 — pathological tie groups captured & arbitrated, 2026-10-16)

Evidências: [[../../plan/evidence/p09-current-work]], [[../../plan/evidence/p09-pathological-tie-groups]], [[../../plan/evidence/p09-exact-use-pathology-correlation]], [[../../plan/evidence/p09-causal-baseline-and-fix-direction]], [[../../plan/evidence/p09-session-handoff]] e [[../../HANDOFF]].
Diagnóstico dos primeiros 20 grupos de empate patológicos implementado, validado por testes de host (1354/1354 exit 0), compilado no TooieRecompiled (binário SHA256 `84d31479...`, exit 0) e capturado em gameplay (`EXHAUSTED` em 24 tentativas, 6 planos, 162 ocorrências de pares patológicos, todos `TIE`, 179 chaves únicas).

- Capturas & hashes: `/tmp/p09-tie-discriminator/capture-gameplay/tooie-p09-seam-edge-pathology-correlation.txt` (SHA256 `74b0b2e5...`) e `/tmp/p09-tie-discriminator/capture-gameplay/tooie-p09-seam-edge-pathology-ties.txt` (SHA256 `5b1fc21a...`). Patch versionado `patches/rt64/p09-exact-use-pathology-correlation.patch` SHA256 `7b93258c...`.
- Estrutura dos grupos: 20 grupos armazenados (`kP09TieGroupCap=20`), 142 grupos omitidos, 0 truncamento. 12 mult=2, 8 mult=4 = 56 ocorrências de cantos de face de pais com chaves de 9 campos distintas. Todos os 20 grupos têm exatamente 2 `storedVertex` distintos por grupo no mesmo `WORLD` pai e coordenadas impressas idênticas (não identidade binária). Todos os 20 vértices filhos e 56 membros retornaram `MEMBERSHIP=NOT_SELECTED` (`REASON=NONE`) contra o plano local ativo e exaustivo.
- Revisão independente e arbitragem: revisão estática Haiku PASS (notada ausência de checagem do retorno de `fclose`, não bloqueante); análise Haiku corrigiu distinção entre `selected=YES` (escolha argmin-0) e `MEMBERSHIP=NOT_SELECTED` (pertinência no plano de costura). Escalonamento Sonnet executado; arbitragem do Root confirmou que o Sonnet retirou a certificação pontual de P1 (k-pontos, reciprocidade, 0.08), alegações de omissão global do plano e persistência de IDs brutos. As três opções do Sonnet (P1, P2, P3) permanecem como `HYPOTHESIS`.
- Escopo e limites: `PRODUCTION_FIX_NOT_IMPLEMENTED: YES`, `P09_FORMAL_PHASE: PENDING / NOT_FORMALLY_OPEN`, `NO_COMMIT: YES`, `NO_PUSH: YES`.

Próxima ação única: **a READ-ONLY focused audit of existing paired-boundary-edge/provenance data and its correlation to the sampled pathological uses, starting with producer/topology semantics and what current artifacts actually contain; no new lerp/alias or telemetry implementation automatically. Root stops at newly identified evidence blocker after Sonnet analysis as authorized, not claiming fix.**

## Estado anterior (P09-RES-05 — gameplay captured, 194/194 ambiguous TIE & tie discriminator needed, 2026-10-15)

Evidências: [[../../plan/evidence/p09-current-work]], [[../../plan/evidence/p09-exact-use-pathology-correlation]], [[../../plan/evidence/p09-causal-baseline-and-fix-direction]], [[../../plan/evidence/p09-session-handoff]] e [[../../HANDOFF]].
A base causal do residual está estabelecida (`CAUSE_ALREADY_ESTABLISHED: YES`, classe `NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS` provada decisivamente via `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1` e `TOOIE_DIAG_FORCE_CURRENT_FRAME=1`), enquanto a correção de produção não está implementada (`PRODUCTION_FIX_NOT_IMPLEMENTED: YES`).
Captura em gameplay executada com sucesso; limite atingido (`EXHAUSTED` em 24 tentativas). Dados de runtime significativos foram capturados, mas todas as 194 ocorrências de pares patológicos são ambíguas por empate (`TIE`); taxa de cobertura é indecidível. A captura existente de 194 TIEs permanece como bloqueio diagnóstico de uso exato para autorização de junção NDC mais próxima sobre chaves de alias, e NÃO como causa raiz desconhecida. Nenhuma correção de produção foi feita.

- Captura & hash: `/tmp/p09-correlation-implementation/capture-gameplay-20261015.txt` (SHA256 `b6be018a3619a78a7c05ba7d0b8fa6c64e27d22e5cde6bedbebeebdf771203e7`). Captura vazia inicial mantida como histórico. Jogo mantido em execução no estado `EXHAUSTED` (sem novas buscas sem novo rearm).
- Accounting & planos: 2135 walks, 93 dispatches elegíveis, 24 tentativas (4 com par patológico, 20 sem par patológico), 4 buscas de costura, 4 planos locais construídos (`PLAN_BUILT=4`). Sem falhas de plano, limites de capacidade atingidos ou truncamento (`PLAN_INACTIVE=0`, `PLAN_NOT_EXHAUSTIVE=0`, descartes 0).
- Correlação de patologias: `PATH_PAIR_TOTAL=194`, `PATH_PAIR_FULLY_COVERED=0`, `PATH_PAIR_ONE_SIDED=0`, `PATH_PAIR_NOT_COVERED=0`, `PATH_PAIR_AMBIGUOUS=194`, `PATH_PAIR_AMBIGUOUS_TIE=194`. Saturação por 194 empates (`PATH_PAIR_FULL_OF_ALL_OBSERVED=0.000000`, taxa definitiva `NA`). Tratam-se de ocorrências cumulativas ao longo de 4 planos, não 194 arestas certificadas únicas.
- Chaves & universo não patológico: 214 chaves de caminho únicas (1 selecionada, 213 não selecionadas). Não patológico: 454 pares, 481 chaves únicas (4 selecionadas, 477 não selecionadas). 25 chaves em ambos.
- Tempos de CPU (telemetria diagnóstica em runtime, não FPS do jogo nem prova visual): pico de tentativa 202.762 ms (total 590.915 ms), pico de plano 200.776 ms (total 551.464 ms), portão total 10.382 ms.
- Revisão Haiku da causa: provado que `p09EmitNearestJoinPairs` ativa `bestTie = true` em igualdade float exata `gap == bestGap` em distância NDC XY, e o classificador prioriza TIE antes de checagens de chave. Hipótese de pais coincidentes é UNKNOWN; coordenadas idênticas não provam que a ambiguidade é falsa porque chaves de origem e política de WORLD importam. Nenhuma supressão automática de empate autorizada.
- Arbitragem e direção de correção: recomendação Haiku de novos experimentos visuais com CLIP_LERP/NDC_LERP é SUPERSEDED pelo encerramento de 2026-09-14 em `p09-articulated-temporal-coherence-root.md`. O próximo discriminador atende à seleção para a causa conhecida, não a uma nova busca por causa raiz.

Próxima ação única (histórica): **one bounded diagnostic discriminator logging first ~20 tie groups, exact full sourcekeys/originalWORLD and prev/cur positions, gap/multiplicity and candidate memberships to distinguish repeated same-use from different legitimate-use; then review, before alias/interpolation edits (currently NOT_IMPLEMENTED; task stops at newly observed blocker per scope rule).**

## Estado anterior (P09-RES-05 — exact-use correlation empty capture & relaunch waiting, 2026-10-15)

Evidências: [[../../plan/evidence/p09-current-work]], [[../../plan/evidence/p09-exact-use-pathology-correlation]], [[../../plan/evidence/p09-session-handoff]] e [[../../HANDOFF]].
Primeira captura real executada com o diagnóstico de correlação por uso exato; resultado vazio (`CAPTURED_EMPTY / NO_ELIGIBLE_SAMPLES`). Root relançou o executável com a flag dedicada e confirmou espera; usuário posicionando o personagem em gameplay antes da armação.

- Primeira captura (`capture-empty-20261015.txt`): `ARM_COUNT=1`, `REARM_COUNT=0`, `MARKER_CONSUME_FAILURES=0`. `ELIGIBILITY_WALKS=181`, `ELIGIBLE_DISPATCHES=0`, `ATTEMPTS=0`, `PATH_PAIR_TOTAL=0`. Motivos de descarte por ineligibilidade (ordem de primeira falha): `SKIPPED_INELIGIBLE_NO_WORKLOAD=61`, `SKIPPED_INELIGIBLE_NOT_INTERMEDIATE=59`, `SKIPPED_INELIGIBLE_NO_TARGET_CALL=61`; `NO_GEOMETRY=0`, `NO_FRAME_MATCH=0`, `NO_MODEL_BIN=0`. Usuário estimou o tempo em ~30 segundos; cena: `UNKNOWN`.
- Classificação: `CAPTURED_EMPTY / NO_ELIGIBLE_SAMPLES`. NÃO constitui evidência de cobertura zero, ausência de patologias nem de correção. Estado anterior preservado como histórico; amostra patológica significativa, prova visual e tempos reais de CPU continuam `NOT_CAPTURED`.
- Revisão Haiku do gate (`haiku-empty-capture-output.json`): confirmou a cadeia sequencial de primeira falha (`NO_WORKLOAD` -> `NO_GEOMETRY` -> `NO_FRAME_MATCH` -> `NOT_INTERMEDIATE` -> `NO_MODEL_BIN` -> `NO_TARGET_CALL`) e os alvos de DL de penas 0x62B (`p09V4VisibleTargetIndex`). Não promover a inferência de que movimento é estritamente obrigatório ou de que o model bin está resolvido globalmente como comprovado: contadores registram primeira falha; zero `no_model_bin` ocorreu apenas para os 61 dispatches que chegaram a esse estágio.
- Relançamento pelo root: processo iniciado com flag dedicada (`TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION=1 ./build/TooieRecompiled`), verificado `STATE=WAITING_FOR_REQUEST`, 0 walks. Usuário posicionando gameplay antes de o root armar.

Próxima ação única (histórica): **wait for user ready in character scene then arm and inspect eligible/attempt counters; still no aliases or production fix.**

## Estado anterior (P09-RES-05 — exact-use pathology correlation implemented & reviewed, 2026-10-15)

Evidências: [[../../plan/evidence/p09-current-work]], [[../../plan/evidence/p09-exact-use-pathology-correlation]], [[../../plan/evidence/p09-session-handoff]] e [[../../HANDOFF]].
Diagnóstico de correlação de patologia por uso exato implementado, validado por 1231 asserções de host, relincado no TooieRecompiled e aprovado em revisão independente final Haiku.

- Diagnóstico & flags: `TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION` (default-OFF, CPU-only, standalone). Marcador de requisição `/tmp/tooie-p09-seam-edge-pathology-correlation.arm` com consumo transacional `::remove == 0`, descarte de marcador stale na inicialização, intervalo de 2000 ms verificado antes do walk (`p09CorrelationPreWalk`), e limites de segurança (máximo 24 tentativas pesadas, 8 buscas de costura).
- Metodologia: chaves de 9 campos transportadas para ambos os membros do par; plano local construído na mesma chamada evita sobreposição global; métrica estrita `PATH_PAIR_FULL_OF_ALL_OBSERVED` inclui ambíguos no denominador.
- Gates & revisão: teste de host `p09_exact_use_correlation_test` 1231/1231 exit 0; `TooieRecompiled` ligado (35/35) exit 0; unitários de regressão 38/38 e 7/7 exit 0; `git diff --check` exit 0; patch `patches/rt64/p09-exact-use-pathology-correlation.patch` (SHA256 `5108ad3f...`) validado em aplicação/reversão sobre a linha de base dirty intencional de RT64. Revisão final Haiku PASS (estático/helpers/build); arbitragem de integridade de hashes e logs confirmada.
- Fronteira de runtime: cobertura de patologia em jogo, cena de personagens, prova visual e tempos de CPU permanecem `NOT_CAPTURED`. Sem aliases, sem correção de produção; fase formal P09 permanece pendente (`PENDING / NOT_FORMALLY_OPEN`); limpeza/commit `NOT_READY` (`NO_COMMIT=YES`, `NO_PUSH=YES`).

Próxima ação única (histórica): **user runs one scene capture with `TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION=1 ./build/TooieRecompiled`, arming via `/tmp/tooie-p09-seam-edge-pathology-correlation.arm` in character scene, and inspects correlation output; still no aliases or production fix.**

## Estado anterior (P09-RES-05 — V2 paired-edge capture review & CASE2 decision, 2026-10-15)

Evidências: [[../../plan/evidence/p09-current-work]], [[../../plan/evidence/p09-seam-edge-dry-run-v2]], [[../../plan/evidence/p09-session-handoff]] e [[../../HANDOFF]].
Captura V2 executada pelo usuário (`TOOIE_DIAG_P09_SEAM_EDGE_DRY_RUN_V2=1 ./build/TooieRecompiled`).
Processo encerrado; sem processo ativo ou flags ao vivo no momento. Snapshot preservado:
`/tmp/tooie-p09-seam-edge-dry-run-v2-20261015-030500.txt` (SHA256 `256d5fdd935c7d105810d885f4bbdb4047453606cdc5544bc56ef1e81bbbe6e5`).
Detalhes completos de contadores e temporização registrados em [[../../plan/evidence/p09-current-work]].

- Accounting de busca (último workload): 4627 boundaries, same-call avaliados 66588, cross-call brutos 664920 (duplicatas 554413, únicos avaliados 110507), certificados 55 same / 135 cross, exatos 732. Sessão cumulativa: 2200 workloads.
- Exaustividade da busca: `SAME_CALL_SEARCH_EXHAUSTIVE=YES`, `CROSS_CALL_BUCKET_SEARCH_EXHAUSTIVE=YES`, `CROSS_CALL_UNIQUE_PAIR_GENERATION_EXHAUSTIVE=YES`, descartes de busca 0, `EMERGENCY_LIMIT_HIT=NO`, `RETRIEVAL_CELL_SMALLER_THAN_EDGE_SCALE=NO` (correção terminológica do fictício `RETRIEVAL_RADIUS_RISK`), `LATEST_WORKLOAD_COUNTS_EXHAUSTIVE=YES`, `ALIAS_PLAN_EXHAUSTIVE=YES`. Saturação textual (`DETAIL_OUTPUT_ONLY_SATURATION=YES`, 12/190 aceitos) não invalida busca exaustiva.
- Escopo de aceitação: accounting de busca aceito SOMENTE para o último workload. Portão de coerência estrutural/proveniência pendente (`CERTIFIED` sozinho não prova identidade de aresta). Cobertura de patologias: `NOT_CAPTURED`.
- Observação visual: `INCONCLUSIVE` devido à desaceleração extrema reportada. O diagnóstico não altera renderização; falhas visuais persistentes não refutam o diagnóstico. Hipótese de sobrecarga de CPU (`HYPOTHESIS` forte) sustentada por self-times medidos (busca 64ms, pico de sessão 439ms), não é profiler nem FPS medido nem A-B controlado. Revisão Haiku (`/tmp/p09-haiku-performance.json`) apontou callsite cpp:3259 (não incondicional a cada frame). A-B qualitativo (`USER_OBSERVED`): volta ao normal sem a flag e desaceleração severa associada ao V2 ativo; confirma associação relatada, mas não é FPS medido, benchmark controlado, prova de custo exclusivo de busca versus outra telemetria nem causalidade de thread/frequência; self-times existentes inalterados (64ms latest, 439ms pico).
- Build & fase: mantido `PASS` de 14/10 com ressalva de frescura (`BYTE_IDENTITY=UNKNOWN_BUILD_DID_NOT_RECOMPILE_SOURCE`). `P09_FORMAL_PHASE=PENDING / NOT_CLAIMED`. `PRODUCTION_FIX_UPDATED=NO`. `CLEANUP_READY=NO`. `COMMIT_READY=NO`.

Próxima ação única: **inspect certified paired-edge A/B provenance and residual/tolerance/margin details from preserved snapshot; only after coherent certification proceed exact-use pathology correlation. Do not implement aliases yet.**

## Estado anterior (P09-RES-05 — V2 search exhaustiveness validation gate, 2026-10-14)

Evidências: [[../../plan/evidence/p09-current-work]], [[../../plan/evidence/p09-session-handoff]] e [[../../HANDOFF]].
Gate real de validação verificado pelo orquestrador em 14/10 às 23:51 -0300:
- Build: `cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test` exit 0 (`[3/3] Linking TooieRecompiled`, log `/tmp/p09-grok-gate-20261014/build.log`).
- Testes unitários: `./build/window_display_test` exit 0 (`PASS: 38/38 assertions passed`, log `/tmp/p09-grok-gate-20261014/window_display_test.log`); `./build/tintro_projection_policy_test` exit 0 (`PASS: 7/7 assertions passed`, log `/tmp/p09-grok-gate-20261014/tintro_projection_policy_test.log`).
- Diff checks: `git -C TooieRecomp diff --check` exit 0; `git -C BanjoRecomp/lib/rt64 diff --check` exit 0.
- Edições em código-fonte: `NONE`. Lançamento do jogo: `NOT_RUN`. Nenhuma captura runtime ou validação visual; sem correção de produção.
- Git HEADs: Tooie `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf`, BanjoRecomp `ec859632cfa584e2272d32e46051543429a2baf4`, RT64 `6f1c2d99a4ea571c139f449c326fd176ba8f3496`.
- Ressalva mandatória de frescura (freshness caveat): build incremental reutilizou o objeto RT64 compilado em 24/09 (`rt64_transform_processor.cpp.o`) e relincou o executável `TooieRecompiled` em 14/10 às 23:51. Relação temporal: `source mtime <= object mtime <= binary mtime`. Símbolos V2 (`RT64::p09SeamEdgeDryRunV2WouldAliasExactUse`, `RT64::p09SeamEdgeDryRunV2AliasPlanExhaustive`) confirmados no objeto e no binário via `nm`. Classificação: `INFERENCE` de frescura, NÃO prova byte a byte de compilação deste código-fonte exato; `BYTE_IDENTITY=UNKNOWN_BUILD_DID_NOT_RECOMPILE_SOURCE`. Não afirmar que o fonte RT64 foi recompilado nem que P09 está comprovado.
- Hashes SHA256: Fonte `941ae09e8891889c2cf3057d06aaa5a413f2efdf6b4be7fe1a020a786c221a07`, Objeto `2f9887fe227c6aff0bf63d018f9610aca8d62e1784be5a010eeb00b310e1b932`, Executável `17873fec359484654d8deeb7ae40302f4a4a44b7a40c6ea0e792c63e91152953`.
- Roteamento de modelos: Grok modelo real `grok-4.7` medium (não fast, `context_window=200000`), Gemini na documentação (Gemini 3.8 Flash High), Haiku `claude-haiku-5-5` medium em revisão independente somente-leitura; Sonnet e Opus não utilizados. Tentativas iniciais de CLI com falha/cancelamento de permissão ocorreram antes do gate; gate final efetivo concluído com sucesso.
- Resumo da revisão Haiku (`/tmp/p09-haiku-review.json`): confirmou estaticamente o código de reparo presente para mesmo-call (:2270-2302), cross-call (:2326-2425) e safety stops fail-closed (:2700-2712), mas não traçou a atribuição de `gP09V2Plan.exhaustive` até o caminho de recusa final do plano (não reivindicar revisão completa).
- Estado: Capturas e cobertura de patologia permanecem `NOT_CAPTURED`; sem aliases ou correção de produção; limpeza/commit `NOT_READY`; P09 formal permanece pendente (`PENDING / NOT_FORMALLY_OPEN`).
- Auditoria markdown-vault: pendente de execução pelo orquestrador após as edições.

Próxima ação única: **user performs isolated V2 capture in character scene, first preserving existing output, enabling ONLY `TOOIE_DIAG_P09_SEAM_EDGE_DRY_RUN_V2=1 ./build/TooieRecompiled`; inspect all exhaustive fields incl `RETRIEVAL_RADIUS_RISK=NO`, certification and exact-use provenance**. Captures/pathology coverage remain `NOT_CAPTURED`; no aliases or production fix; cleanup/commit not ready; P09 formal remains pending.

## Estado anterior (P09-RES-05 — V2 search exhaustiveness repair, 2026-09-24)

A captura anterior não é um negativo cross-call: a busca same-call parou em
16384, o passe de bucket de 48 descartou 180588 entradas, e o dedup
cross-call falhou. A identidade da aresta, a tolerância `1e-4` e o render
não mudaram. A busca agora é por workload: todos os pares same-call, célula
exata com vizinhança 26, e cada par único avaliado uma vez pelo menor id.
Limites que restam são de segurança e marcam o workload como não exaustivo.
Correlação patológica continua `NOT_CAPTURED`. Jogo não lançado. Evidência:
[[../../plan/evidence/p09-seam-edge-dry-run-v2]].

Próxima ação única: **`TOOIE_DIAG_P09_SEAM_EDGE_DRY_RUN_V2=1 ./build/TooieRecompiled`
na cena dos personagens e confirmar que busca, certificação e plano de uso
exato não estão truncados** antes de correlacionar patologia. Não
materializar aliases.

## Estado anterior (P09-RES-05 — seam-alias validity review, 2026-09-24)

Captura do usuário: 29 seams cross-call e zero same-call no workload mais
recente; 9857/11903 workloads da sessão tiveram ao menos um par certificado.
Revisão estática: coincidência de um ponto em PREV/CURRENT não certifica
identidade da junção; `BOUNDS_SATURATED=YES` pode limitar busca e plano de
aliases; cobertura dos usos patológicos V2 ainda não foi correlacionada.
Decisão `CASE_B`: manter B+C como arquitetura condicional, sem aliases de
produção. Evidência: [[../../plan/evidence/p09-seam-alias-dry-run-validity-review]].

Próxima ação única: **implementar somente novo dry-run CPU, default-off,
com identidade de aresta pareada/proveniência, contadores por limite e chave
de uso exato para futura correlação V2**. Não remapear `worldIndices`.

## Estado anterior (P09-RES-05 — seam-alias dry-run, 2026-09-24)

`TOOIE_DIAG_P09_SEAM_ALIAS_DRY_RUN=1` grava
`/tmp/tooie-p09-seam-alias-dry-run.txt`. O passe é só CPU, default-off, e
não altera interpolação, buffers, `worldIndices` nem shader. Identidade de
GameCall e o pareamento PREV do slot original estão disponíveis nessa
camada. Aliases futuros herdariam o match do slot original e entrariam
antes do upload em `State::fullSync`. Parceiros ambíguos e geometria não
certificada no mesmo GameCall ficam em `RigidBody::lerp`. Contagens de
seams, aliases e clones: `NOT_CAPTURED` (jogo não lançado).
`SNOUT_SPECIFIC_ROOT=UNPROVEN`. Evidência:
[[../../plan/evidence/p09-seam-alias-dry-run]].

Próxima ação única: **`TOOIE_DIAG_P09_SEAM_ALIAS_DRY_RUN=1 ./build/TooieRecompiled`
na cena dos personagens e ler o arquivo de dry-run**. Não duplicar matrizes
nem remapear `worldIndices` ainda. Seletores por índice WORLD continuam
default-off.

## Estado anterior (P09-RES-05 — call-local WORLD architecture review, 2026-09-24)

Os dois seletores por índice WORLD continuam default-off e são insuficientes.
O teste visual multi-only trouxe de volta as penas de Kazooie; listras de
Klungo, flicker de Mumbo e focinho de Banjo persistem. A revisão estática
localizou a identidade do GameCall na faixa de faces CPU, o colapso da
política no slot WORLD e a ordem upload-before-interpolation. Protótipo
recomendado: aliases CPU por uso candidato, clone de vértices partilhados e
certificação geométrica nos dois endpoints antes de aplicar elementwise.
Evidência:
[[../../plan/evidence/p09-call-local-world-interpolation-architecture-review]].

## Estado corrente (P09-RES-05 — selective elementwise WORLD candidate, 2026-09-17)

Candidato CPU default-off `TOOIE_TEST_P09_SELECTIVE_ELEMENTWISE_WORLD=1`
em `TransformProcessor`: lerp 16 entradas só para WORLD usados por
GameCalls com mais de um `worldIndices` distinto. Restantes WORLD
continuam `RigidBody::lerp`. VIEW/PROJECTION inalterados. Snap W-sign de
produção desligado. Sem lançar o jogo nesta tarefa. TooieRecompiled,
window 38/38, tintro 7/7. Evidência:
[[../../plan/evidence/p09-generic-selective-world-interpolation-production-design]].

Próxima ação única: **TOOIE_TEST_P09_SELECTIVE_ELEMENTWISE_WORLD=1
./build/TooieRecompiled** (high-rate, sem
`TOOIE_DIAG_RSP_ENDPOINT_BINDING`) e responder as quatro perguntas
visuais.

## Estado anterior (P09-RES-05 — standard SLERP WORLD discriminator, 2026-09-16)

Braço CPU `STANDARD_SLERP_WORLD` no schema
`P09_RSP_ENDPOINT_BINDING_V2_SHADER_EQUIVALENT`. Clip:
`mul(LIVE_POS4,mul(STANDARD_SLERP_WORLD,mul(LIVE_VIEW,LIVE_PROJ)))`.
Só a rotação muda (SLERP matemático); T/S/skew/persp iguais à produção.
Sem mudança de produção. Recapture ELEMENTWISE já classificou
`NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`
(`PROVEN`); não promover elementwise. TooieRecompiled, window 38/38,
tintro 7/7, synth 14/14. Evidência:
[[../../plan/evidence/p09-standard-slerp-world-production-candidate-discriminator]].

Próxima ação única: **TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled** e colar `STANDARD_SLERP_WORLD_*`.

## Estado anterior (P09-RES-05 — elementwise WORLD lerp discriminator, 2026-09-16)

Recapture V2: linearidade `YES`, 156/156 patologias corrigidas, gap
articulado reduzido, não-rigidez material `YES`. Root
`NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`.
Não promover elementwise. Evidência:
[[../../plan/evidence/p09-elementwise-world-interpolation-causal-discriminator]].

## Estado anterior (P09-RES-05 — CPU diagnostic shader-semantics repair, 2026-09-16)

Diagnósticos CPU usam `mul(pos4,mul(WORLD,VIEWPROJ))`. 14 consumidores
ativos reparados; braços MODELVIEW/hierarchy `SUPERSEDED`. Schema
`P09_RSP_ENDPOINT_BINDING_V2_SHADER_EQUIVALENT`. Pose: motivos de rejeição
separados e endpoints reais `w=0/1` checados. Root do jogo `UNRESOLVED`.
Sem HLSL/upload de produção, sem captura. Validação offline `PASS`
(synth 9/9, SPIR-V estático, TooieRecompiled, window 38/38, tintro 7/7).
Evidência:
[[../../plan/evidence/p09-cpu-diagnostic-shader-semantics-repair]].

## Estado anterior (P09-RES-05 — validated camera-pose VIEW arm, 2026-09-15)

Braço CPU `EXACT_ORDER_VALIDATED_CAMERA_POSE_VIEW` implementado
(default-off) sobre as mesmas amostras LLLL, com SLERP matemático e
ordem `((POSE_DERIVED_LIVE_VIEW*LIVE_PROJ)*LIVE_WORLD)*LIVE_POS4`.
Chaves novas `NOT_CAPTURED`. Root `UNRESOLVED`. Sem produção.
Evidência: [[../../plan/evidence/p09-exact-order-validated-camera-pose-view]].

Próxima ação única: **TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled** e colar `POSE_VIEW_*` /
`POSE_DERIVED_VIEW_MATCHES_SAFE_CLASS` / cobertura patológica.

## Estado anterior (P09-RES-05 — independent architecture review, 2026-09-15)

Mapping-coherent fallback `REFUTED` pela captura existente: 249 JOINs
patológicos alinhados, zero desalinhados. Root `UNRESOLVED`.
VIEW interpola entradas de matriz inversa; quaternion WORLD tem endpoints
corretos, mas lei interior diferente de SLERP matemático. JOIN WORLD xyz
descarta w e não prova coerência homogênea. Não reabrir hierarchy/W fixes.
Evidência: [[../../plan/evidence/p09-independent-architecture-root-cause-review]].

## Estado anterior (P09-RES-05 — mapping-coherent fallback, 2026-09-15)

Os 12 mismatches classe B são VIEW interpolado + WORLD CURRENT,
não dois PREV workloads distintos (`STRONG_INFERENCE` static).
Diagnóstico CPU preserva A–E / `COHORT_ALIGNED_LIVE` e adiciona
`MAPPING_COHERENT_LIVE` (CURRENT VIEW quando WORLD unmapped).
Chaves novas `NOT_CAPTURED`. Não implementar produção.
Evidência:
[[../../plan/evidence/p09-mapping-coherence-fallback-counterfactual]].

Próxima ação única: **TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled** e colar `MAPPING_COHERENT_*` /
`CLASS_B_MAPPING_ASYMMETRY_EXPLAINS_CATASTROPHIC_GLITCH` /
`CLASS_B_WORLD_PREV_RECOVERABLE` / `COHORT_ALIGNED_*`.

## Estado anterior (P09-RES-05 — temporal cohort mismatch causal proof, 2026-09-15)

Capture anterior: 12 VIEW-mapped/WORLD-unmapped; CLCL safe;
SAME_COHORT_COMPOSITE não é safe. Diagnóstico CPU classifica
patologia por classe A–E e corre `COHORT_ALIGNED_LIVE` sem snap
CURRENT. Evidência:
[[../../plan/evidence/p09-temporal-cohort-mismatch-causal-proof]].

## Estado anterior (P09-RES-05 — exact-order VIEW×WORLD cohort forensic, 2026-09-15)

World-space JOIN recapture: LIVE preserves attachments
(`MAX=1.006`); hierarchy `REFUTED`. Previous MODELVIEW
counterfactual reordered PROJECTION (`INVALID` for root).
Exact-order factorial: `CLCL` remains the safe class. CPU now
traces VIEW/WORLD temporal cohorts, LLLL pathological
attribution, homogeneous W, and SAME_COHORT_COMPOSITE without
reordering PROJECTION. Keys `NOT_CAPTURED`. Shader inalterado.
Evidência:
[[../../plan/evidence/p09-exact-order-view-world-temporal-cohort-forensic]].

Próxima ação única: **TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled** e colar `VIEW_WORLD_*_TEMPORAL_COHORT_ALIGNED`
/ `LLLL_PATHOLOGICAL_*` / `SAME_COHORT_COMPOSITE_*` /
`CLCL_COUNTERFACTUAL_VALID` / `VIEW_WORLD_ROOT_CLASS`.

## Estado anterior (P09-RES-05 — VIEW×WORLD interaction discriminator, 2026-09-15)

Factorial recapture: `CLCL` is the safe class. World-space JOIN
instrumented; later recapture closed hierarchy. Previous
MODELVIEW order is invalid. Evidência:
[[../../plan/evidence/p09-view-world-interaction-root-discriminator]].

## Estado anterior (P09-RES-05 — tfPos factorial term attribution, 2026-09-14)

World non-rigidity `REFUTED`. 16-way factorial captured: CLCL is
the minimum CURRENT set matching ALL_CURRENT. Evidência:
[[../../plan/evidence/p09-tfpos-factorial-term-attribution]].

## Estado anterior (P09-RES-05 — world matrix interpolation rigidity, 2026-09-14)

JOIN recapture VALID. Production `worldMat` is `RigidBody::lerp`
TRS+SLERP when decompose is valid. User recapture closed
`WORLD_NONRIGIDITY_ROOT_CAUSE=REFUTED`. Evidência:
[[../../plan/evidence/p09-world-matrix-interpolation-rigidity-forensic]].

## Estado anterior (P09-RES-05 — diagnostic heap crash forensic, 2026-09-14)

JOIN recapture VALID (`JOIN_PAIR_COUNT=53946`) until native
`double free`. Core: gfx thread + RT64 render worker both in
`p09PercentileCopy` on global residual vectors (`PROVEN`
diagnostic race). Overlay unload is not the free site.
`WORLD_TO_BONE_BOUND_COUNT=0` is last-dispatch. CPU mutex +
atomic status write. Do not infer `BEST_COHERENT_SCOPE`. POLICY_A
kills JOIN catastrophes but does not solve the visual glitch.
Shader de produção inalterado. Evidência:
[[../../plan/evidence/p09-diagnostic-heap-crash-forensic]].

Próxima ação única: **TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled** e colar o status completo incluindo
`PER_*_CURRENT_*`, `PER_*_TRIGGER_COUNT`, `PER_*_JOIN`,
`BEST_COHERENT_SCOPE`, `CPU_DIAG_SHARED_STATE_LOCKED`,
`STATUS_WRITE_ATOMIC`.

## Estado anterior (P09-RES-05 — JOIN diagnostic repair, 2026-09-14)

Capture anterior: residual VALID (`34060`), JOIN INVALID
(`JOIN_PAIR_COUNT=0`). O builder exigia aresta mista no mesmo
triângulo e comparava bone IDs a `worldIndices` concretos.
PER_DRAW `MAX=1.026` é independente de JOIN e ainda não confiável
sem percentagem CURRENT. Diagnóstico CPU repara world→bone V7 e
pares articulados nearest; shader de produção inalterado.
Evidência:
[[../../plan/evidence/p09-articulated-temporal-coherence-root]].

Próxima ação única: **TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled** e colar `JOIN_DIAGNOSTIC_VALID`,
`JOIN_PAIR_COUNT`, `LIVE_JOIN_*` / `POLICY_A_JOIN_*` /
`*_JOIN_PHASE_DELTA`, `PER_DRAW_CURRENT_VERTEX_PERCENT`,
`PER_DRAW_IS_EFFECTIVELY_ALL_CURRENT`, `PER_*_TRIGGER_COUNT`,
`BEST_COHERENT_SCOPE`.

## Estado anterior (P09-RES-05 — screenspace interpolation domain, 2026-09-13)

Capture residual **VALID** (`33020`). Classe dominante = zero-cross
(`32561`); TRIANGLE_CURRENT não resolve. POLICY_A reduz extremos
mas o glitch visual permanece. Diagnóstico CPU agora mede
`POLICY_CLIP_LERP` e `POLICY_NDC_LERP` (crossing → CURRENT),
overshoot NDC do LIVE, e envelope dos endpoints. Shader de
produção inalterado. Evidência:
[[../../plan/evidence/p09-projective-w-production-fix]].

Próxima ação única: **TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled** e colar `CLIP_LERP_*`, `NDC_LERP_*`,
`ZERO_CROSS_LIVE_NDC_OVERSHOOT_*`, `BEST_TEMPORAL_POSITION_POLICY`.

## Estado anterior (P09-RES-05 — residual triangle diagnostic repair, 2026-09-13)

Capture anterior: verts intermédios 63805, máscara DL `0x3fff`,
`RESIDUAL_TRIANGLE_COUNT=0` (`INVALID`). O acumulador exigia
`anyFeather` e `curOk`+edge floor; V4 conta todas as faces dos DLs
alvo 0x62B. Diagnóstico CPU reutiliza essa seleção. Shader de
produção inalterado. Evidência:
[[../../plan/evidence/p09-projective-w-production-fix]].

Próxima ação única: **TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled** e colar `RESIDUAL_DIAGNOSTIC_VALID`,
`RESIDUAL_TRIANGLE_COUNT` e POLICY_A_*/LIVE_*/CROSS_*/TRIANGLE_CURRENT_*.

## Estado anterior (P09-RES-05 — residual after POLICY_A, 2026-09-12)

Visual: high-rate `GLITCH`; POLICY_A `FAIL`; all-CURRENT `PASS`;
marcador W-crossing `A/PASS`. `GPU_W_SIGN_BRANCH_EXECUTES: PROVEN`.
`POLICY_A_MAX_EXPANSION_RATIO=0` só conta explosões `ratio>=5`
(`OLD_POLICY_A_MAX_METRIC_WAS_FILTERED: YES`). Diagnóstico CPU
passa a medir distribuição residual vs ALL_CURRENT (não só
explosões). Shader de produção inalterado. Números residuais
`NOT_CAPTURED`. Evidência:
[[../../plan/evidence/p09-projective-w-production-fix]].

Próxima ação única: **TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled** (high-rate normal) e colar as chaves
POLICY_A_*/LIVE_*/CROSS_*/TRIANGLE_CURRENT_*/ZERO_CROSS_* de
`/tmp/tooie-rsp-endpoint-binding-status.txt`.

## Estado anterior (P09-RES-05 — human-obvious GPU W-crossing marker, 2026-09-12)

O marcador NDC `(+0.20,-0.12)` era subtil demais. Com
`TOOIE_DIAG_RSP_MARK_W_CROSSING=1`, vértices no branch
`prevW*curW <= 0` recebem `clip.x += 0.70 * clip.w` (~35% da
largura do ecrã), depois da escolha CURRENT. Política de produção
inalterada. Evidência:
[[../../plan/evidence/p09-projective-w-production-fix]].

Próxima ação única: **SUPERSEDED**.

## Estado anterior (P09-RES-05 — selective GPU branch execution, 2026-09-12)

CPU `POLICY_A_EXPLOSION_COUNT=0` (926 crossings, sidecar=witness).
A política seletiva `prevW*curW <= 0` está **default-on** no run
normal (sem flag). Tasks 1–3 não acharam disable acidental nem
falso-negativo aritmético. Primeiro marcador shader-only
`TOOIE_DIAG_RSP_MARK_W_CROSSING=1` (`+0.20,-0.12` NDC) era subtil.
Evidência: [[../../plan/evidence/p09-projective-w-production-fix]].

Próxima ação única: **SUPERSEDED**.

## Estado anterior (P09-RES-05 — intermediate projective collapse, 2026-09-12)

Force-CURRENT em `RSPProcessCS` `PASS` (glitch some). Política
`prevW*curW <= 0` `FAIL`. Sidecar=witness nas penas intermédias
(768/768, depois 926/926). Replay CPU live agora tem
`POLICY_A_EXPLOSION_COUNT=0`. Shader de produção inalterado.
Evidência: [[../../plan/evidence/p09-projective-w-production-fix]].

Próxima ação única: **SUPERSEDED**.

## Estado anterior (P09-RES-05 — RSP shader execution / buffer visibility, 2026-09-12)

Sidecar CPU = witness: `PROVEN` (108569/108569, feather 1698/1698,
diffs 0). Upload estático antes do dispatch; SPIR-V tem o branch
W-sign e `tfPos` CURRENT chega a `dstPos`. Contagens
intermédias-only ainda **não** capturadas ao vivo. Diagnóstico
shader-local `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1` implementado
(default-off). Política de produção inalterada. Evidência:
[[../../plan/evidence/p09-projective-w-production-fix]].

Próxima ação única: **TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1
./build/TooieRecompiled** e reportar se o glitch das penas muda.

## Estado anterior (P09-RES-05 — sidecar vs VERSION=4 witness, 2026-09-12)

Live bind: t22/t23 object identity MATCH. Session A/B later proved
sidecar=witness. Last-dispatch `0` vs VERSION=4 `1292` was
overwrite/population, not construction. Evidência:
[[../../plan/evidence/p09-projective-w-production-fix]].

Próxima ação única: **SUPERSEDED**.

## Estado anterior (P09-RES-05 — RSP endpoint sidecar binding proof, 2026-09-12)

Visual **FAIL** não prova que o branch GPU executou.
`FAILED_VISUAL_IMPL_CLASS: UNKNOWN`. Dispatch estático: feathers
0x62B indexadas usam `RSPProcessCS` → `screenPosBuffer`. t22/t23
mapeiam para os sidecars; layout/writes/ordem PREV/CURRENT estão
consistentes; SPIR-V Binding 22/23. Sem bug de plumbing **PROVEN**.
Diagnóstico default-off `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1`.
Evidência: [[../../plan/evidence/p09-projective-w-production-fix]].

Próxima ação única: **correr high-rate com
`TOOIE_DIAG_RSP_ENDPOINT_BINDING=1` e ler
`/tmp/tooie-rsp-endpoint-binding-status.txt`**.

## Estado anterior (P09-RES-05 — failed per-vertex W-sign candidate, 2026-09-12)

Visual do utilizador: candidato per-vértice **FAIL** (stretch
original). `TOOIE_DIAG_FORCE_CURRENT_FRAME=1` continua bom.
Offline: 48/75 explosões dumpadas, 3/3 vértices cruzam; per-vértice
e triângulo CURRENT → 0 explosões. Não alargar granularidade.
**CORRECTED/SUPERSEDED** quanto a “o branch GPU falhou ao substituir
`tfPos`”: isso não está `PROVEN`. Evidência:
[[../../plan/evidence/p09-projective-w-production-fix]].

Próxima ação única: **SUPERSEDED**.

## Estado anterior (P09-RES-05 — projective-W production candidate, 2026-09-12)

Candidato per-vértice implementado; visual do utilizador **FAIL**.
Evidência: [[../../plan/evidence/p09-projective-w-production-fix]].

Próxima ação única: **SUPERSEDED**.

## Estado anterior (P09-RES-05 — projective W / viewProj representation, 2026-09-10)

VERSION=3: 199 explosões live e 199 SIMPLE. PR23 `REFUTED`.
`COMBINED_RELATIVE_*` é `INVALID`. Recapture VERSION=4 **feito**.
Evidência:
[[../../plan/evidence/p09-projective-w-and-viewproj-representation-root-cause]].

Próxima ação única: **SUPERSEDED**.

## Estado anterior (P09-RES-05 — relative V×W interpolation, 2026-09-09)

VERSION=2: 51 explosões live e 51 SIMPLE. PR23 `REFUTED`. Classe:
interpolação independente de VIEW e WORLD. Witness v3 (análise)
espera recapture. Não portar PR22/23. Evidência:
[[../../plan/evidence/p09-relative-view-world-interpolation-root-cause]].

Próxima ação única: **SUPERSEDED** (VERSION=3 capturado; relative INVALID).

## Estado anterior (P09-RES-05 — boned SIMPLE counterfactual, 2026-09-09)

Live AUTO interpola WORLD com **DECOMPOSE** de matrizes já compostas.
Emparelhamento errado da mesma bone **não** está provado nas 44
explosões. Não fazer override visual CURRENT. O mesmo witness
passivo reconstrói SIMPLE offline (`lerpMatrixComponents`); o
status antigo não tinha matrizes, falta recapture `VERSION=2`.
Candidato de produção: grupos GeoCmd Tooie + SIMPLE em modelos
com ossos. **Não** implementar tagging nesta tarefa.
Evidência:
[[../../plan/evidence/p09-boned-interpolation-counterfactual]].

Próxima ação única: recapture
`TOOIE_DIAG_FEATHER_TEMPORAL_COHERENCY=1` e ler
`EXPLOSION_COUNT_SIMPLE_COUNTERFACTUAL`.

## Estado anterior (P09-RES-05 — temporal coherency, 2026-09-09)

FPS alto normal `GLITCH` (sem leak do diagnóstico de projeção).
Framerate original `FIXED`. V5C `SAME`. Global CURRENT `FIXED`.
Projection-only CURRENT é `INVALID_TEMPORALLY_INCOHERENT` (ghosting);
código removido. Não isolar mais um processador com CURRENT.
`FORCE_CURRENT_FRAME` continua só diagnóstico. Evidência:
[[../../plan/evidence/p09-temporal-coherency-root-cause]].

## Estado anterior (P09-RES-05 — projection-only INVALID, 2026-09-09)

`TOOIE_DIAG_FORCE_CURRENT_PROJECTION` misturou Projection=CURRENT com
outros interpolados. Visual: glitch + ghosting.
`INVALID_TEMPORALLY_INCOHERENT`. Código removido. Evidência:
[[../../plan/evidence/p09-high-rate-force-current-projection]],
[[../../plan/evidence/p09-temporal-coherency-root-cause]].

## Estado anterior (P09-RES-05 — high-rate force-current, 2026-09-09)

V5C visual `SAME` nos nove transforms das JOIN visíveis. Framerate
original: `FIXED` (`USER_OBSERVED`). Diagnóstico global CURRENT:
`FIXED`. Evidência:
[[../../plan/evidence/p09-high-rate-force-current-frame]].

## Estado anterior (P09-RES-05 — V5C NO_INTERP 107/108, 2026-09-09)

V5B não cobriu os pais de JOIN 107 e 108. V5C estendeu o mesmo
`NO_INTERP` default-off aos nove ossos. Visual do utilizador: `SAME`.
Evidência: [[../../plan/evidence/p09-feather-no-interp-v5c]].

## Estado anterior (P09-RES-05 — BanjoRecomp tagging não é fix pronto, 2026-09-09)

PR #22/#23 existe no BK e está **ABSENT** no Tooie (AUTO em 0x62B).
Não é solução pronta de tagging. O fechamento V5B de interpolação
foi `CORRECTED` por V5C (pais 107/108). Evidência:
[[../../plan/evidence/p09-banjorecomp-transform-tagging-portability-audit]],
[[../../plan/evidence/p09-feather-no-interp-v5c]].

## Estado anterior (P09-RES-05 — sampler das penas é animação authored, 2026-09-08)

Capture V1 + ROM: tempo intra-clip contínuo; resets negativos são
mudanças de clip com t=0 + um passo ADE4. `func_800E0170` /
`func_800E04C0` batem nas keys do clip 22 (osso 107/108). Os 30/120°
antigos são interpolação authored idle↔walk, não bug de sampler.
Status C. Sem fix, sem novo capture, sem jogo. Evidência:
[[../../plan/evidence/p09-animation-clip-transition-and-authored-key-forensic]].

Próxima ação única: A/B visual vs N64/Angrylion do walk 0x62B
Kazooie (rampa de escala 107 + nod 108 authored vs wobble restante).
Não adicionar witness. Não GPU readback. Não production-fix.

## Estado anterior (P09-RES-05 — witness de tempo de sample V1 capturado, 2026-09-08)

Witness `P09_FEATHER_ANIM_SAMPLE_TIME_V1` capturou 1451 updates.
Classificação Luna de bug de tempo foi `CORRECTED`: deltas negativos
são mudanças de clip. Evidência:
[[../../plan/evidence/p09-feather-anim-sample-time-v1]].

## Estado anterior (P09-RES-05 — compose convidado das penas fechado, tempo do clip aberto, 2026-09-08)

`func_800AE8A8` é Hamilton `parent ⊗ child` em XYZW. Produto offline
≤ 1.6e-7 rad em 991 frames 0x62B. Additive `a3+8/9` = 0. Os saltos
grandes são o osso 107 (30°/60°/120° + escala uniforme). Clip id e
tempo de sample `baanim` não estão no dump. Sem fix, sem witness
novo, sem jogo. Evidência:
[[../../plan/evidence/p09-upstream-current-bone-transform-semantic-forensic]].

Próxima ação única: um witness CPU em `baanim_update` / `0x8008ADE4`
(clip id, tempo/frame, TRS 0x28 de 79, 100–105, 107, 108 e um osso
de corpo). Não GPU readback. Não começar apresentação/compositing.

## Estado anterior (P09-RES-05 — alpha/coverage/textura das penas visíveis fechados, 2026-09-08)

As 12 DLs de pluma carregam CI4 64x32 + pal16, ligam `G_TT_RGBA16` e
`G_AC_THRESHOLD` só durante os triângulos, e restauram. Crest/cabeça
usam `G_TEXTURE on=0` e combine SHADE, sem AC/TT. ST é ROM estático;
sem `G_MODIFYVTX` ST. Textura vive no mesmo GameCall. Não cobre o
crest; sem driver de material só-em-movimento. Sem fix, sem novo
capture, sem jogo. Evidência:
[[../../plan/evidence/p09-visible-feather-alpha-coverage-texture-forensic]].

Próxima ação única: auditoria estática / command-stream de
apresentação / compositing / associação da imagem final nos 14 DLs.
Não adicionar witness de alpha, ST ou textura. Não fazer GPU readback.

## Estado anterior (P09-RES-05 — associação temporal de draw e profundidade fechadas, 2026-09-07)

Face/GameCall/GameFrame ficam no workload produtor (16 gerações V7, 0
mismatch, um `game_call` por DL, `fb_pair=3`). Reuso do anel espera GPU.
Os 14 DLs não setam zCmp/zUpd/ZMODE/primDepth; profundidade raster RT64
é otherMode herdado, igual ao corpo. Sem fix, sem novo capture, sem
jogo. Evidência:
[[../../plan/evidence/p09-post-position-draw-temporal-and-depth-state-forensic]].

Próxima ação única: auditoria estática / command-stream de
cor/alpha/coverage e coordenadas de textura nos 14 DLs. Não adicionar
witness de posição, cull ou temporal. Não fazer GPU readback.

## Estado anterior (P09-RES-05 — culling/winding das penas visíveis fechado, 2026-09-07)

Os 14 DLs visíveis submetem todos os G_TRI ao APPEND V7 (`1258`
invocações, resto 0). O único reject pré-append é cull-both e não
disparou. Os DLs preservam `G_CULL_*` herdado (iguais ao corpo
`0x48F`/`0xBC`); não há `G_CULLDL`. Caminho de posição permanece
`CLOSED`. Sem fix, sem novo capture, sem jogo. Evidência:
[[../../plan/evidence/p09-visible-feather-culling-winding-face-visibility-forensic]].

Próxima ação única: auditoria estática / command-stream de profundidade
/ oclusão / associação temporal de draw nos 14 DLs. Não adicionar
witness de posição. Não fazer GPU readback.

## Estado anterior (P09-RES-05 — caminho de posição das penas visíveis fechado, 2026-09-07)

Q / RSPProcess / RSPModify / RasterVS da pluma direita transferem para
o crest (`0x4A6`/`0x4B0`). Handoff CPU V3 permanece `CORRECT` nos 14
DLs. Caminho de posição pós-handoff das penas visíveis: `CLOSED`.
Não chamar bug de GPU. Sem fix, sem novo capture, sem jogo. Evidência:
[[../../plan/evidence/p09-bounded-crest-post-handoff-transferability-audit]].

Próxima ação única: auditoria estática / command-stream de culling /
winding / visibilidade de faces nos 14 DLs. Não adicionar witness de
posição. Não fazer GPU readback.

## Estado anterior (P09-RES-05 — identidade visual incompleta, 2026-09-07)

Os seis DLs fechados cobrem só a pluma direita. A pluma esquerda
(`0x5C0/0x5D8/0x5F0/0x608/0x61E/0x636`) e o crest (`0x4A6/0x4B0`) já
desenham no V6B. Pai GeoCmd das plumas é 107, não a cabeça 108.
Sem fix, sem novo witness, sem jogo. Evidência:
[[../../plan/evidence/p09-visual-geometry-identity-and-scope-forensic]].

Próxima ação única: estender a admissão/handoff V7 aos oito DLs
adicionais. Não instrumentar RSPProcess. Não fazer GPU readback.

## Estado anterior (P09-RES-05 — RSPProcessCS fechado estaticamente, 2026-09-07)

`RSPProcessCS` não tem mecanismo alvo-local credível após V7, ausência
de `G_MODIFYVTX`, V=0, V5B e Q' compartilhado/negligível. Classe C do
testemunho Q não é `ROOT_CLASS`. Sem fix, sem novo witness, sem jogo.
Evidência:
[[../../plan/evidence/p09-narrow-rspprocess-structural-forensic]].

Próxima ação única: verificar se o wobble visível é realmente os seis
DLs da pluma direita versus outra geometria `0x62B`. Não instrumentar
RSPProcess. Não fazer GPU readback.

## Estado anterior (P09-RES-05 — testemunho CPU Q' pós-processador, 2026-09-07)

Witness `P09_FEATHER_POSTPROCESS_Q_WITNESS_V1` implementado (default-off,
`TOOIE_DIAG_FEATHER_POSTPROCESS_Q=1`). Reconstrói RSPProcess com Q
current/previous/pre-aspect/Q' e W' fixo. Controlo de corpo estático:
Banjo torso `dl=0xBC` e cabeça Kazooie 108 `dl=0x48F`. Sem fix de
produção. Sem V8. Sem jogo. Evidência:
[[../../plan/evidence/p09-feather-postprocess-q-witness-v1]].

Próxima ação única: usuário executar
`TOOIE_DIAG_FEATHER_POSTPROCESS_Q=1 ./build/TooieRecompiled` e inspecionar
`PRELIMINARY_CLASS` no status.

## Estado anterior (P09-RES-05 — forense estrutural pós-handoff, 2026-09-07)

O handoff CPU V7 VERSION=2 permanece `CORRECT`. A auditoria estática
pós-handoff ([[../../plan/evidence/p09-post-handoff-structural-forensic]])
mostra zero `G_MODIFYVTX` nos seis DLs-alvo. O primeiro escritor de
posição restante é `RSPProcessCS`. Interpolação de matriz mundo continua
refutada (V5B). Sem fix de produção. Sem V8. Sem jogo.

Próxima ação única: implementar uma reconstrução CPU limitada das
entradas de RSPProcess após TransformProcessor+ProjectionProcessor e
compará-las ao handoff V7.

## Estado anterior (P09-RES-04 — comparador V7 stride-3 e papéis de join, 2026-09-07)

A admissão exata está comprovada (`APPEND_EXACT_TARGET_MATCH=40124`,
máscara `0x3F`). `byte_mismatch=3` é artefato do comparador (stride 4 vs
stride 3 de `posFloats`). Os joins `0x54C`/`0x564`/`0x592` usam o mapa
estático child 0–3 / parent 28–31. Schema `VERSION=2`. Sem jogo nesta sessão.

Próxima ação única: usuário executar `TOOIE_DIAG_FEATHER_HANDOFF_V7=1
./build/TooieRecompiled` e inspecionar `APPEND_POSITION_MATCH` e
`JOIN_*_ROLE_RESOLVED`.

## Estado anterior (P09-RES-04 — admissão V7 por endereço exato, 2026-09-07)

O contexto de modelo `0x62B` durável publicou 1297 vezes (`base=0x229070`,
0 clears), mas a validação ficou em 0 porque o prefiltro de página baixa
ignorava `physical(model_bin)&0xFFF`. A admissão agora compara os seis
endereços V6C completos. Sem prefiltro `0xFFF`. Sem jogo nesta sessão.

Próxima ação única: usuário executar `TOOIE_DIAG_FEATHER_HANDOFF_V7=1
./build/TooieRecompiled` e inspecionar `APPEND_EXACT_TARGET_MATCH`.

## Estado anterior (P09-RES-04 — implementação V7 CPU handoff, 2026-09-05)

[[../../plan/evidence/p09-feather-v7-cpu-handoff-implementation]] registra a
implementação do testemunho `P09_FEATHER_HANDOFF_V7 / VERSION=1`, default-off,
nos seams de append, `State::fullSync` e seleção de matrizes no render. Os seis
DLs da pluma direita têm quotas independentes; joins permanecem gate explícito.
Build e testes estáticos solicitados passam. Nenhum jogo, readback, sync, fix,
commit ou push foi executado.

Próxima ação única: usuário executar `TOOIE_DIAG_FEATHER_HANDOFF_V7=1
./build/TooieRecompiled` uma vez e inspecionar o par CSV/status.

## Estado anterior (P09-RES-03 — revisão estática pós-triângulo, 2026-09-05)

[[../../plan/evidence/p09-post-triangle-architectural-review]] contém a revisão
arquitetural e a especificação V7, ainda **NOT_IMPLEMENTED**. V6C aceita os
matches dos três meshes próprios, mas deixa `3252/3720` junções de `0x592`
não resolvidas e não cobre as junções distais `0x54C/0x564`. A localização
exclusiva após submissão RSP é **CORRECTED**; raiz UNKNOWN, confiança LOW.
NO_INTERP/SIMPLE e leftover `0x03000040` permanecem refutados em seus escopos.

Implementação V7 acima supersede a ação de implementação. Esta revisão foi
somente documental e não altera o estado formal pending da fase.

## Estado anterior (P09 residual gráfico — pena Kazooie V6B, 2026-09-04)

- primary token `0x62B+0x124A0` idx 30 anim 103 still `PROVEN_FOR_CAPTURE`
- guest matrix/rotation valid; do not reopen AE8A8/AE820 producer RE
- AUTO identity `CORRECT` for bones `79, 100–105`; `VISIBLE_REQUIRED_MATCH_DROP_COUNT=0`
- V5B full-chain `NO_INTERP` masks GMTX/POSTMATCH/NO_INTERP `0x7F`;
  `RESULT==CURRENT` 211/211; user visual `SAME`
- `TEMPORAL_MATRIX_INTERPOLATION_CAUSAL_STATUS: REFUTED_FOR_VISIBLE_FEATHER_CHAIN`
- `SIMPLE_INTERPOLATION_CAUSAL_STATUS: REFUTED_AS_PRIMARY_FIX`
- V6 runtime TRI=0 is a witness-key failure, not missing geometry
- diagnostic schema `P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V6B` (default-off;
  raw GBI/RSP counters; classify from per-vertex `worldIndices`)
- Banjo snout remains the clean control; Banjo matrix-group/SIMPLE
  relevance to the current root is `LOW`
- next: one V6B capture (`TOOIE_DIAG_SECONDARY_SWAY=1`)

Canonical writeup: [[../../plan/evidence/p09-kazooie-feather-v6b-triangle-witness-repair]],
[[../../plan/evidence/p09-kazooie-feather-v6-post-matrix-vertex-path]],
[[../../plan/evidence/p09-kazooie-feather-gmtx-to-dl-trace]],
[[../../plan/evidence/p09-kazooie-feather-v5-full-chain]],
[[../../plan/evidence/p09-session-handoff]], [[../../HANDOFF]],
[[../../reverse-engineering/systems/geocmd]],
[[../../reverse-engineering/characters/kazooie]],
[[../../reverse-engineering/systems/transform-interpolation]].

## Estado corrente (P09 residual gráfico — focinho DCF48/ADE2C, 2026-09-03)

P09 remains formally `PENDING / NOT_FORMALLY_OPEN`. Canonical RE knowledge
base: [[../../reverse-engineering/README]].

- live walk body `0x62B` `PROVEN_FOR_CAPTURE`; snout compact GeoCmd02 @ `0x11CB8` INDEX 11
- `E2AA4_RAW_ENTRY_COUNT=0`; E2AA4 is `STATIC_PATH_CANDIDATE` / `DEMOTED`
- live type-2 handler `func_800DCF48` `PROVEN_RUNTIME` (30 bounded type-2 tuples)
- `+0x11CB8` missing from the old DE2A4 census was a list-head wrap; snout is sibling
- tree CSV witness records snout→DCF48 (`PROVEN_RUNTIME`, 2,243 accepts)
- ADE2C current-slot samples exist (`2,243`); `ADE2C` is the local bone source
- static trace proves `func_80019AA0` composes into a temporary 0x40-byte matrix arena slot before `mlMtxApply`/`guMtxF2L`
- RT64 plain G_MTX uses AUTO transform matching; rotating guest addresses alone do not prove identity churn
- Banjo has an explicit group around the equivalent single-G_MTX path; Tooie's observed path does not
- bounded CPU-only guest/RT64 witness for **Kazooie head feathers** is
  `P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V2` (AE8A8/AE820 producer dump;
  V1 token/G_MTX identity retained; RT64 match deferred)
- V1 frames 1420–1424 are uniform scale, not a singular matrix (`TOO_STRONG`)
- Banjo snout V5: no proven AUTO identity failure; snout closed as clean control
- primary token `0x62B+0x124A0` idx 30 anim 103 (combined plume) with `0x61C`/`0x83A` crest fallback
- local 4x4 writer `func_800AE820`; scale legality `YES`; bug-vs-intent `INSUFFICIENT_EVIDENCE`
- first V2 enabled boot crashed in `func_800AEBA8` from AE8A8 wrap zero-extending KSEG0 GPRs (`PROVEN`); wrap now restores full `gpr`
- root remains `MORE_EVIDENCE_REQUIRED` (`MEDIUM` mechanism / `LOW` causal); no production fix

Canonical writeup: [[../../plan/evidence/p09-session-handoff]],
[[../../plan/evidence/p09-live-geocmd-type2-dispatch]],
[[../../plan/evidence/p09-e2aa4-snout-witness]],
[[../../plan/evidence/p09-62b-snout-e2180-layout]],
[[../../plan/evidence/p09-persistent-snout-filter-zero]],
[[../../plan/evidence/p09-deferred-render-context-e0b8c]],
[[../../plan/evidence/p09-player-model-witness-r31-repair]],
[[../../plan/evidence/p09-secondary-sway-static-root]], [[../../HANDOFF]],
[[../../plan/evidence/p09-de2a4-dcf48-runtime-capture]],
[[../../plan/evidence/p09-live-0x62b-tree-dcf48-capture]],
[[../../plan/evidence/p09-live-0x62b-tree-reachability]],
[[../../plan/evidence/p09-single-gmtx-identity-static-trace]],
[[../../plan/evidence/p09-single-gmtx-identity-witness]],
[[../../plan/evidence/p09-19aa0-gmtx-rt64-match-stage]],
[[../../plan/evidence/p09-v4-real-match-witness]], [[../../plan/evidence/p09-v5-real-call-match]],
[[../../plan/evidence/p09-kazooie-carry-banjo-head-feathers]],
[[../../plan/evidence/p09-kazooie-feather-v1-capture]],
[[../../plan/evidence/p09-kazooie-feather-v2-producer-witness]],
[[../../plan/evidence/p09-kazooie-feather-v3-implementation]],
[[../../plan/evidence/p09-kazooie-feather-v4-capture-analysis]],
[[../../reverse-engineering/systems/transform-interpolation]],
[[../../reverse-engineering/systems/animation]].
Prior wobble artifact: `/tmp/p09-banjo-wobble-part3-20260902T060241Z/`.

## Estado corrente (P09 Pendente — evidência pré-abertura, 2026-08-20)

- **P08 Formal**: `ACTIVE / MANUAL_VALIDATION_PENDING`; a observação positiva dos controles durante gameplay não prova o checklist físico completo, e não há confirmação física de display.
- **P09 Formal**: `PENDING / NOT_FORMALLY_OPEN`.
- **Evidência provisória**: [[../../plan/evidence/p09-user-observed-parity-residuals]] cobre apenas Demo 0 (Wooded Hollow) contra o oracle hardware-faithful `ANGRYLION_CONTROL` (Mupen64Plus Pure Interpreter + Angrylion RDP Plus + CXD4 LLE).
- **Demo 0**: pad 765/765, trajetória limitada, paridade de behavior-state, Blue Egg no evento glidx 322 e resultado de Z-buffer/classifier são preservados. Atributos diretos de animação e render-state completo permanecem não medidos/parciais.
- **Relatos globais**: Fire Egg permanece `USER_REPORTED_UNLOCATED`; drift visual permanece `USER_REPORTED / OPEN`. Demo 1, Demo 2 e Demo 3 estão `UNTESTED`.
- **Backlog de Resíduos de Fidelidade**: `P09-RES-03` glitches gráficos (`USER_REPORTED / UNCATALOGUED`), `P09-RES-04` tic/tic de áudio (`USER_REPORTED / UNDIAGNOSED`) e `P09-RES-05` culling de borda widescreen (`USER_REPORTED`).

## Tarefas de paridade

- [ ] `P09-T01` Criar matriz por boot, áreas, chefes, minigames, transformações, cutscenes e final.
- [ ] `P09-T02` Cobrir criação, carregamento, sobrescrita e migração de saves.
- [ ] `P09-T03` Comparar checkpoints de estado entre execução de referência e nativa.
- [ ] `P09-T04` Corrigir timers, RNG, física, animação e sincronização no timing original.
- [ ] `P09-T05` Executar campanha completa e registrar toda divergência reproduzível.
- [ ] `P09-T06` Rodar sanitizers e soak tests em trocas repetidas de área.

## Tarefas de melhoria

- [ ] `P09-T07` Desacoplar update de gameplay da apresentação para FPS arbitrário.
- [ ] `P09-T08` Adicionar widescreen/ultrawide com correções de câmera, HUD e cutscenes.
- [ ] `P09-T09` Adicionar opções de câmera/entrada sem modificar o preset fiel.
- [ ] `P09-T10` Definir API de mods somente após estabilizar layout e eventos do runtime.
- [ ] `P09-T11` Reexecutar a matriz completa para cada melhoria habilitada e desabilitada.

## Gate de saída

- Campanha completa e saves aprovados no preset fiel.
- FPS alto não muda velocidade, física, animações, timers, áudio ou cutscenes.
- Melhorias podem ser desligadas para restaurar comportamento de referência.
- Critérios funcionais e audiovisuais de [[../../plan/definition-of-done]] atendidos.

## Próxima etapa

1. Completar a validação física formal de P08.
2. Depois de abrir P09 formalmente, alinhar Demo 1, Demo 2 e Demo 3 contra `ANGRYLION_CONTROL`, identificar eventos de ovos/projéteis, medir animação/render-state direto e localizar o drift visual.
3. Só então selecionar o próximo defeito entre áudio e gráficos.

## Estado corrente (P09-RES-04 — V7 durable-bin bridge repair, 2026-09-07)

A publicação de identidade do testemunho V7 foi movida para o seam já provado
`secondary_sway_register_durable_bin()`, após normalização por metadados do
modelo para `0x62B`. O getter `80092B04` permanece somente diagnóstico, com
contadores de rejeição por motivo. O ponteiro guest é preservado em largura
completa e a base física é registrada separadamente; validação continua por
endereço completo e candidatos de página baixa não entram na análise causal.
Build/tests/diff checks/graft passaram. A próxima ação única autorizada é uma
captura V7 bounded com os dois artefatos, sem executar outra análise downstream
até `TARGET_VALIDATED_COUNT` ser não-zero.

## Estado corrente (P09-RES-04 — V7 capture gate state G, 2026-09-07)

A captura V7 pós-reparo confirmou publicação runtime no seam durável: 1.449 registros, 1.297 normalizações `0x62B`, 1.297 publicações e zero limpezas. Porém a admissão exata ainda falha: 5.296 candidatos, zero validados; 2.526 rejeições por model bin zero e 2.770 por divergência de endereço completo. O gate causal permanece fechado (`CAPTURE_COMPLETE_FOR_CAUSAL_DECISION=NO`, máscara `0x00`), portanto face/upload/join/render não devem ser analisados. A próxima ação única é reparar a divergência candidato→endereço completo e repetir uma captura bounded.
## Estado corrente (P09-RES-04 — V7 exact direct-admission capture state H, 2026-09-07)

Capture fresh and schema-valid. The durable model seam publishes 1485 normalized 0x62B contexts. Direct physical(model_bin)+0x2670+logical*8 admission yields 40124 exact matches with OBSERVED_TARGET_MASK=0x3F; per-target exact counts are 11464 for 0x534 and 5732 for each of 0x54C, 0x564, 0x57C, 0x592, and 0x5AA.

Pipeline samples include 448 APPEND and 96 INITIAL_HANDOFF_OK records across all six targets, plus 32 RENDER_SELECTION records. Face slice range/content and velocity-zero checks pass in the sampled handoffs. Every handoff has byte_mismatch=3, and this aggregate counter does not identify the component, so CPU payload equality and first divergence remain UNKNOWN. JOIN expected-role provenance is unresolved (192 JOIN APPEND samples and 48 JOIN handoffs), which keeps CAPTURE_COMPLETE_FOR_CAUSAL_DECISION=NO.

Next single action: split the handoff byte-mismatch counter by component and resolve JOIN expected-role provenance, then rerun one bounded V7 capture. No production change, GPU readback, synchronization, commit, or push in this analysis state.
