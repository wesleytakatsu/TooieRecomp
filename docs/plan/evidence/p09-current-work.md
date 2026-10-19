---
title: "P09 — Estado atual e diário de trabalho"
description: "Registro operacional vivo do P09-RES-05, pendências, gates e próxima ação."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-10-17
tags: [p09, interpolation, handoff, workflow]
---

# P09 — Estado atual e diário de trabalho

## Como manter este registro sincronizado

Este é o resumo operacional vivo do residual **P09-RES-05**. Atualizar ao
iniciar e encerrar cada sessão e após captura, mudança de decisão ou resultado
visual relevante. Não há sincronização automática com o executável ou o jogo.

Antes de trabalhar, conferir HEAD e alterações locais dos repositórios
envolvidos, ler o handoff e a evidência mais recente. Depois de trabalhar:

1. Atualizar a tabela de estado, o objetivo atual e `NEXT_SINGLE_ACTION`.
2. Acrescentar ao diário o que foi feito, resultado, pendências e evidência.
3. Registrar comandos, exit codes, HEADs e hashes de artefatos quando aplicável.
4. Atualizar também [[../../HANDOFF]], [[p09-session-handoff]] e
   [[../../plans/BT/09-gameplay-parity-and-enhancements]] se o estado técnico mudar.
5. Rodar as verificações documentais e `git diff --check`.

Não sobrescrever experimentos anteriores. Marcar conclusões superadas como
`CORRECTED/SUPERSEDED`. Resultado não executado é `NOT_RUN` ou `NOT_CAPTURED`.
Datas de atualização documental não tornam testes históricos atuais.

Fonte de verdade: repositório/evidências atuais, handoff global, handoff da
sessão e plano da fase. Este resumo deve refletir essas fontes; divergências
devem ser registradas e resolvidas com evidência primária.

## Roteamento de agentes

Instruções correntes do usuário para divisão de trabalho entre modelos e ferramentas:

- **Codex local**: orquestrador principal do fluxo de trabalho.
- **Gemini 3.8 Flash high** (via Antigravity CLI `agy`): responsável por toda a documentação e manutenção de arquivos Markdown.
- **Grok 4.7 medium** (nunca fast; `context_window=200000` via CLI `grok`): modelo principal e mais utilizado para implementação de código e execução de testes em complexidade média.
- **Claude Haiku 5.5 medium** (via Claude Code CLI): análise de código, buscas e tarefas simples.
- **Claude Sonnet 5.5 medium**: escalonamento somente se os outros modelos não conseguirem resolver.
- **Claude Opus 5.5 medium**: reservado exclusivamente para problemas de complexidade extrema.

## Estado em 2026-10-17

| Item | Situação | Base / limite da afirmação |
| --- | --- | --- |
| Fase P09 formal | `PENDING / NOT_FORMALLY_OPEN` | Índices mantêm P08 ativo; residual acompanhado como P09-RES-05 |
| Contorno paliativo padrão (Fase 1) | `IMPLEMENTED` (em 16/10) | `apply_default_before_renderer_init()` instala `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1` quando unset; normal `./build/TooieRecompiled` dispensa flags; rollback via `0`; teste unitário 20/20 PASS (`policy-test-root.log` exit 0); binário TooieRecompiled SHA256 `0d2e4ba5...` exit 0; RT64 HEAD `6f1c2d99` intocado; QA normal do usuário OK ("está rodando bem", áudio mantido deferred) |
| Presets de FPS / Decimação (Fase 2) | `DECIMATOR_IMPLEMENTED_AND_TESTED / UI_PRESETS_PLANNED` | `PresentDecimator` (`rt64_present_decimator.h` e `rt64_present_queue.cpp`) implementado e testado (`rt64_present_decimator_test` PASS exit 0); data race de `viOriginalRate` corrigida sob `configurationMutex`; resets em pause/catch-up corrigidos; patch versionado `patches/rt64/rt64-present-rate-decimator.patch`; builds root `root-build.log` e `root-build-final.log` exit 0; fluxo de ACKs preservado (static proof); QA de runtime NÃO executada, sem medição de FPS ou garantia de velocidade; widget UI de 8 presets pendente |
| Contador FPS top-left e toggle de settings | `IMPLEMENTED_AND_VERIFIED / UI_HOOK_INTEGRATED` | Opção "Show Present FPS" em `add_graphics_options` (`src/game/config.cpp`, default false, `graphics.json`); hook RmlUi `recompui::set_overlay_update_callback`; `RT64::presentSuccessCount` incrementado apenas em `swapChain->present() == true`; patches versionados `patches/frontend/` e `patches/rt64/`; `tooie_fps_meter_test` PASS exit 0; `git diff --check` exit 0; QA de runtime NÃO executada; limitação de pose-repeat sob CURRENT `tfPos` documentada |
| Correção de produção permanente | `NOT_IMPLEMENTED` | Contorno apenas bypassa interpolação de posição (`tfPos` CURRENT); sem interpolação contínua a 60+ FPS; fila de pesquisa anterior (grupos de empate / Sonnet) preservada como histórico diferido |
| Causa principal | `PROVEN` nas evidências anteriores | `NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS` |
| Arquitetura B+C | `CONDITIONAL` | Alias WORLD por uso certificado; clone físico apenas em conflito de política |
| Diagnóstico V2 | Implementado, default-off | Arestas pareadas, proveniência e identidade de uso exato |
| Reparo da busca V2 | `PROVEN` no executável testado | Log runtime confirma execução dos caminhos reparados |
| Gate 1 build e testes | `PASS` (em 17/10) | `root-build.log` exit 0 (`TooieRecompiled` [362/362]), `root-build-final.log` exit 0 (`TooieRecompiled` [6/6]), unitários `rt64_present_decimator_test` e `tooie_rsp_endpoint_current_test` (20/20) PASS, `git diff --check` exit 0 |
| Busca runtime V2 (latest workload) | `CAPTURED / ACCEPTED` | Accounting de busca exaustivo no último workload (dropped=0, `LATEST_WORKLOAD_COUNTS_EXHAUSTIVE=YES`, `ALIAS_PLAN_EXHAUSTIVE=YES`); `DETAIL_OUTPUT_ONLY_SATURATION=YES` em 12/190 não afeta exaustividade; campo real `RETRIEVAL_CELL_SMALLER_THAN_EDGE_SCALE=NO` |
| Coerência da certificação V2 | `PASS (escopado a 12 same-call / accounting)` | Decisão CASE2 em [[p09-seam-edge-v2-capture-review]]: 12/12 resíduos 0, 24/24 margens passam, vértices e mundos disjuntos; arbitragem do RT64 cpp:2450-71/1812-26/1958-76 resolve suspeita CASE3; 135 cross-call não impressos permanecem não inspecionados em detalhe |
| Correlação de patologia por uso exato | `GAMEPLAY_CAPTURED / 20_TIE_GROUPS_SAMPLED / ALL_AMBIGUOUS_TIE (162/162) / NO_PRODUCTION_FIX` | Captura runtime (`tooie-p09-seam-edge-pathology-correlation.txt` SHA256 `74b0b2e5...`, ties SHA256 `5b1fc21a...`, EXHAUSTED em 24 tentativas, 6 planos, 162 pares todos TIE, 179 chaves únicas); 20 grupos armazenados; fila de pesquisa anterior mantida como histórico diferido |
| Avaliação visual | `USER_OBSERVED (workaround)` | Contorno padrão (`tfPos` CURRENT) verificado pelo usuário em gameplay: jogo roda bem; áudio tem o glitch preexistente mantido deferred a pedido do usuário; sem alteração de áudio |
| Sobrecarga CPU do diagnóstico | `HYPOTHESIS` sustentada por tempos medidos e A-B qualitativo | A-B qualitativo (`USER_OBSERVED`): volta ao normal sem flags pesadas; coletores pesados permanecem OFF no modo padrão |
| Causa específica do focinho | `UNKNOWN / UNPROVEN` | Não assumir que a arquitetura proposta o resolverá |
| Limpeza / commit P09 | `NOT_READY` | Dependem dos gates e prova visual |

Distinção operacional: a sessão anterior de 14/10 limitou-se à consolidação
documental (somente leitura, sem build/testes). Neste gate de 14/10 23:51, o
orquestrador verificou a conclusão real da validação (`cmake --build` exit 0 com
`[3/3] Linking TooieRecompiled`, `window_display_test` 38/38 exit 0,
`tintro_projection_policy_test` 7/7 exit 0, e `git diff --check` exit 0 em Tooie
e RT64). Edições de código-fonte: `NONE`. Lançamento do jogo: `NOT_RUN`.
Nenhuma captura runtime ou validação visual, nenhuma correção de produção.
Roteamento: Grok modelo real `grok-4.7` medium (não fast,
`context_window=200000`), Gemini na documentação (Gemini 3.8 Flash High), Haiku
`claude-haiku-5-5` medium em revisão independente somente-leitura; Sonnet e
Opus não foram utilizados. Tentativas iniciais de CLI com falha/cancelamento de
permissão ocorreram antes do gate; o gate final efetivo foi concluído com sucesso.

## Problema e resultados já conhecidos

Alta taxa de quadros: partes articuladas podem esticar/piscar. Testemunhas:
penas da Kazooie, mochila/aba superior e focinho do Banjo, penas do Mumbo.
Framerate original foi observado como correto historicamente; revalidar no final.

- `PROVEN` nas capturas corrigidas: elementwise WORLD preserva encaixes nas
  amostras patológicas. O prompt de continuidade relata 427/427; isso não é
  uma medição nova desta sessão nem aprovação para aplicação global.
- Elementwise global causa não rigidez/shear; não promover globalmente.
- SLERP padrão corrigiu apenas 159/427 no relato de continuidade; insuficiente.
- Seletores por índice WORLD vazam entre usos e são insuficientes. O primeiro
  melhorou penas/mochila, mas introduziu listras em Klungo e flicker de Mumbo.
  O multi-only trouxe de volta o glitch das penas da Kazooie.
- V1 por coincidência de pontos está retirado como identidade de produção.
- Preservar a semântica row-vector: `mul(pos4, mul(WORLD, mul(VIEW, PROJECTION)))`.
  Diagnósticos antigos column-vector estão superados.

## Objetivo atual e próxima ação única

**Objetivo delimitado:** Contorno paliativo padrão da Fase 1 (`tfPos` CURRENT via `apply_default_before_renderer_init()` quando `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` estiver unset) implementado e validado por teste unitário (20/20 PASS), revisão independente Haiku PASS, build exit 0 e QA de execução normal do usuário confirmando que o jogo roda bem (glitch de áudio preexistente mantido deferred a pedido do usuário; sem alterações em áudio). Fase 2 (seletor de 8 presets de taxa de quadros {20, 30, 45, 60, 70, 100, 120, 144} sem alteração na velocidade de simulação) auditada e planejada, porém bloqueada por restrições de pacing abaixo da taxa nativa no RT64 (`WorkloadQueue`). Correção permanente de interpolação não implementada; fila de pesquisa anterior preservada como histórico diferido. Fase formal P09 permanece pendente.

`NEXT_SINGLE_ACTION`: singular focused audit/measurement of native cadence and present-queue backpressure to choose a below-native presentation cap without clock changes; then implement presets with actual tests all 8 and timing QA.

## Fila de gates

| Ordem | Gate | Situação / condição para avançar |
| --- | --- | --- |
| 1 | Build atual e testes | `PASS` em 15/10 (TooieRecompiled relincado 35/35 exit 0, host test 1231/1231 exit 0, unitários 38/38 e 7/7 exit 0, diff --check exit 0) |
| 2 | Captura isolada V2 na cena dos personagens | `PASS / CAPTURED` em 15/10 00:05 (`/tmp/tooie-p09-seam-edge-dry-run-v2.txt`, snapshot SHA256 `256d5fdd...`) |
| 3 | Exaustividade e inspeção de ambos os lados das arestas | `PASS` (escopado aos 12 same-call / accounting do último workload em [[p09-seam-edge-v2-capture-review]]; decisão CASE2 operacional) |
| 4 | Correlação patológica por uso exato | `GAMEPLAY_CAPTURED / ALL_AMBIGUOUS_TIE` (24 attempts, 4 planos, 194 pares todos TIE, cobertura indecidível; discriminador de empates necessário antes de avançar) |
| 5 | Protótipo real B+C default-off | Só com cobertura convincente e exposição não patológica examinada |
| 6 | Prova visual | Corrigir testemunhas; sem listras de Klungo/flicker de Mumbo ou deformações novas (avaliação V2 atual `INCONCLUSIVE` devido à lentidão) |
| 7 | Limpeza e produção | Só após prova visual; política genérica, sem hardcoding de personagem/modelo |
| 8 | Validação final e commit | Build/testes/diffs + high-rate sem diagnósticos + framerate original |

Captura executada em 15/10: habilitado somente `TOOIE_DIAG_P09_SEAM_EDGE_DRY_RUN_V2=1`
ao executar `./build/TooieRecompiled`. Saída: `/tmp/tooie-p09-seam-edge-dry-run-v2.txt`
(preservado snapshot `/tmp/tooie-p09-seam-edge-dry-run-v2-20261015-030500.txt`).
Toggles de render e diagnósticos antigos permaneceram desligados.

Aceitação de exaustividade da busca (confirmada no último workload em 15/10):

```text
SAME_CALL_SEARCH_EXHAUSTIVE=YES
SAME_CALL_PAIRS_DROPPED_FOR_CAP=0
CROSS_CALL_BUCKET_SEARCH_EXHAUSTIVE=YES
CROSS_CALL_BUCKET_ENTRIES_DROPPED=0
CROSS_CALL_UNIQUE_PAIR_GENERATION_EXHAUSTIVE=YES
CROSS_CALL_PAIR_DEDUP_PROBE_FAILED=NO
CROSS_CALL_UNIQUE_PAIRS_DROPPED_FOR_CAP=0
EMERGENCY_LIMIT_HIT=NO
RETRIEVAL_CELL_SMALLER_THAN_EDGE_SCALE=NO
LATEST_WORKLOAD_COUNTS_EXHAUSTIVE=YES
ALIAS_PLAN_EXHAUSTIVE=YES
```

Nota de correção terminológica: o campo real no log/código é `RETRIEVAL_CELL_SMALLER_THAN_EDGE_SCALE=NO`, e não a designação fictícia `RETRIEVAL_RADIUS_RISK` usada em checklists/prompts anteriores. Saturação de detalhe de texto (`DETAIL_OUTPUT_ONLY_SATURATION=YES`, 12 aceitos, 178 descartados no texto) não invalida a busca exaustiva (`AFFECTS_EXHAUSTIVENESS=NO`).

Verificar alguns registros certificados: arestas A/B, GameCalls, triângulos,
vértices, WORLDs, orientação, resíduos PREV/CURRENT, tolerâncias e margem do
segundo candidato. `CERTIFIED` sozinho não prova identidade correta.
Se houver truncamento, corrigir somente busca/accounting; não alterar tolerância
nem correlacionar. Se a certificação for incoerente, revisar arquitetura primeiro.

A tolerância atual (`1e-4` de escala de aresta + erro numérico linear, sem
translação/floor absoluto) é `DIAGNOSTIC_NOT_PROVEN`; não retunar neste gate.

Correlação futura: chave completa workload/fbPair/projection/GameCall/triângulo/
corner/faceCorner/vértice/WORLD original, usando
`p09SeamEdgeDryRunV2WouldAliasExactUse`. WORLD sozinho não vale.
Registrar total patológico, cobertos, não cobertos, ambíguos, taxa de cobertura
e usos não patológicos selecionados. Meta próxima de 100%, com zero descobertos
ou explicação precisa para cada um. Ambiguidade não conta como cobertura.

## Invariantes e limites

- Preservar alterações preexistentes em Tooie, BanjoRecomp, RT64 e Plume.
- Não editar `RecompiledFuncs/**`; não resetar, limpar, stash, pull ou rebase.
- Somente diagnósticos CPU/estáticos/command stream. Proibidos readback, cópia,
  staging, queries, transições ou sincronização GPU para diagnóstico.
- Aliases futuros herdam o pareamento temporal da WORLD original, sem identidade
  própria de frame matching. Usos não certificados ficam rígidos.
- Auditar índices WORLD uint16, buffers associados e ranges de upload antes do
  protótipo real; preferir nenhum branch no shader.
- Não reabrir W-sign, diagnóstico column-vector, SLERP global suficiente ou
  novos seletores apenas por índice WORLD sem nova evidência primária.
- O texto recebido relata autorização histórica para commit após os gates;
  esta sessão não faz commit nem push. Push exige pedido explícito.
- Sincronização das demos/attract é tarefa separada após P09; aceleração dos
  personagens é hipótese não comprovada. Não misturar com este objetivo.

## Diário de sessões

### 2026-10-14 — Consolidação documental

- Pedido atual: criar registro Markdown local para continuidade do trabalho.
- Consultados: handoff global, handoff P09, plano/índices e notas de arquitetura,
  validade V1 e diagnóstico V2. Concordam sobre captura pós-reparo pendente.
- Tooie HEAD observado: `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf`.
- Worktree já continha numerosas alterações e evidências não rastreadas;
  preservadas. HEAD/status dos irmãos não revalidados nesta tarefa documental.
- Fonte complementar: prompt de continuidade fornecido pelo usuário; seus
  números históricos e autorizações relatadas não substituem validação atual.
- Build/testes/jogo: `NOT_RUN`. Nenhuma implementação de render alterada.
- Entrega: este registro, com links nos handoffs e no plano P09.

### 2026-10-14 — Início do gate delegado

- Início do gate delegado com distribuição de tarefas entre modelos:
  - Grok: build e execução de testes em andamento (`RUNNING`).
  - Haiku: revisão independente em andamento (`RUNNING`).
- Nenhuma captura runtime ou validação visual realizada nesta rodada ainda.
- Nenhum `PASS` reivindicado até a verificação dos exit codes reais.
- Testes de fumaça de CLI (smokes) para Gemini e Haiku concluídos com exit code 0.
- Artefato do smoke do Gemini registrado em `/tmp/p09-agy-smoke.json`.

### 2026-10-14 — Conclusão do gate de validação e revisão independente

- Conclusão dos estados `RUNNING` anteriores de build e revisão:
  - Tentativas iniciais de permissão CLI falharam/foram canceladas antes da execução do gate; gate final executado e concluído com sucesso.
  - Orquestrador verificou os comandos e exit codes reais:
    - `cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test` exit 0 (`[3/3] Linking TooieRecompiled`, log `/tmp/p09-grok-gate-20261014/build.log`).
    - `./build/window_display_test` exit 0 (`PASS: 38/38 assertions passed`, log `/tmp/p09-grok-gate-20261014/window_display_test.log`).
    - `./build/tintro_projection_policy_test` exit 0 (`PASS: 7/7 assertions passed`, log `/tmp/p09-grok-gate-20261014/tintro_projection_policy_test.log`).
    - `git -C /home/wesley/projetos/N64-Recomp/TooieRecomp diff --check` exit 0.
    - `git -C /home/wesley/projetos/N64-Recomp/BanjoRecomp/lib/rt64 diff --check` exit 0.
  - HEADs dos repositórios:
    - TooieRecomp: `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf`
    - BanjoRecomp: `ec859632cfa584e2272d32e46051543429a2baf4`
    - RT64: `6f1c2d99a4ea571c139f449c326fd176ba8f3496`
  - Edições em código-fonte: `NONE`.
  - Lançamento do jogo: `NOT_RUN`.
  - Nenhuma captura runtime ou validação visual; sem correção de produção.
  - Modelos utilizados:
    - Grok: modelo real `grok-4.7` medium (não fast, `context_window=200000`).
    - Gemini: documentação via Antigravity CLI (Gemini 3.8 Flash High).
    - Haiku: `claude-haiku-5-5` medium em revisão independente somente-leitura.
    - Sonnet e Opus não foram utilizados.
  - Resumo da revisão Haiku (`/tmp/p09-haiku-review.json`):
    - Identificou estaticamente o reparo em `rt64_transform_processor.cpp`: same-call streaming de todos os pares (`:2270-2302`), cross-call com célula exata 32 + vizinhança 3×3×3 (27 células) e dedup pelo menor edge id (`:2326-2425`), limites de segurança fail-closed (`:2700-2712`, `:1615-1617`).
    - Lacuna registrada: não verificou a atribuição de `gP09V2Plan.exhaustive` nem o caminho exato de recusa final do plano (não reivindicar revisão completa).
  - Ressalva mandatória de frescura (freshness caveat):
    - Build incremental reutilizou o objeto RT64 de 24 de setembro (`2026-09-24 19:41:18 -0300`) e relincou o executável em 14 de outubro às 23:51 -0300.
    - Relação temporal: `source mtime <= object mtime <= binary mtime`.
    - Símbolos V2 presentes via `nm` no objeto e no executável (`RT64::p09SeamEdgeDryRunV2WouldAliasExactUse`, `RT64::p09SeamEdgeDryRunV2AliasPlanExhaustive`).
    - Classificação: `INFERENCE` de frescura, NÃO prova byte a byte de compilação deste código-fonte exato; `BYTE_IDENTITY=UNKNOWN_BUILD_DID_NOT_RECOMPILE_SOURCE`.
    - Não afirmar que o fonte RT64 foi recompilado nem que P09 está comprovado.
    - Hashes SHA256:
      - Fonte (`rt64_transform_processor.cpp`): `941ae09e8891889c2cf3057d06aaa5a413f2efdf6b4be7fe1a020a786c221a07`
      - Header (`rt64_transform_processor.h`): `00a65b5b779e046152f8199af6eaafd060d8f92dfdbc27fc2033d54f3ac6c9b6`
      - Objeto (`rt64_transform_processor.cpp.o`): `2f9887fe227c6aff0bf63d018f9610aca8d62e1784be5a010eeb00b310e1b932`
      - Executável (`TooieRecompiled`): `17873fec359484654d8deeb7ae40302f4a4a44b7a40c6ea0e792c63e91152953`
  - Auditoria markdown-vault: pendente de execução pelo orquestrador após as edições (marcada como pendente, sem afirmação prévia de conclusão).
  - NEXT_SINGLE_ACTION: user performs isolated V2 capture in character scene, first preserving existing output, enabling ONLY `TOOIE_DIAG_P09_SEAM_EDGE_DRY_RUN_V2=1 ./build/TooieRecompiled`; inspect all exhaustive fields incl `RETRIEVAL_RADIUS_RISK=NO`, certification and exact-use provenance.

### 2026-10-15 — Captura runtime V2 e revisão de desempenho

- Execução V2 pelo usuário com `TOOIE_DIAG_P09_SEAM_EDGE_DRY_RUN_V2=1 ./build/TooieRecompiled`:
  - Captura gerada: `/tmp/tooie-p09-seam-edge-dry-run-v2.txt` (mtime 2026-10-15T03:05:00.703Z, local 15/10 00:05, 21.663 bytes).
  - Snapshot preservado: `/tmp/tooie-p09-seam-edge-dry-run-v2-20261015-030500.txt` (SHA256 `256d5fdd935c7d105810d885f4bbdb4047453606cdc5544bc56ef1e81bbbe6e5`).
  - Processo do jogo encerrado (sem processo ativo ou flags ao vivo para verificação no momento).
- Contadores do workload mais recente (`LAST_WORKLOAD_ID=1267`, `LAST_CALLS_SCANNED=598`):
  - `BOUNDARY_EDGE_COUNT=4627` (same-call 4627, cross-call 4609).
  - Same-call: `RAW_RETRIEVED=66588`, `STRUCTURALLY_COMPATIBLE=10828`, `TOLERANCE_PASS=55`, `CERTIFIED=55`, `AMBIGUOUS=0`.
  - Cross-call: `RAW_RETRIEVED=664920`, `DUPLICATE_RETRIEVAL=554413`, `UNIQUE_EVALUATED=110507`, `STRUCTURALLY_COMPATIBLE=86530`, `TOLERANCE_PASS=151`, `CERTIFIED=135`, `AMBIGUOUS=16`.
  - Exact-uses: `CERTIFIED_SEAM_EXACT_USE_COUNT=732`, `CANDIDATE_ALIAS_COUNT=732`, `UNIQUE_ORIGINAL_WORLD_COUNT_WITH_ALIASES=99`, `VERTEX_CLONE_REQUIRED_COUNT=267`.
  - Sessão cumulativa (não confundir com o último workload): `SESSION_WORKLOAD_COUNT=2200`, `SESSION_WORKLOADS_WITH_CERTIFIED_EDGE_SEAMS=922`, `SESSION_PEAK_CERTIFIED_EDGE_SEAMS=372`.
- Accounting de busca e exaustividade (aceitos SOMENTE para o último workload):
  - `SAME_CALL_SEARCH_EXHAUSTIVE=YES`, `SAME_CALL_PAIRS_DROPPED_FOR_CAP=0`, `SAME_CALL_PAIRS_DROPPED_FOR_SAFETY=0`.
  - `CROSS_CALL_BUCKET_SEARCH_EXHAUSTIVE=YES`, `CROSS_CALL_BUCKET_ENTRIES_DROPPED=0`, `CROSS_CALL_UNIQUE_PAIR_GENERATION_EXHAUSTIVE=YES`, `CROSS_CALL_PAIR_DEDUP_PROBE_FAILED=NO`, `CROSS_CALL_UNIQUE_PAIRS_DROPPED_FOR_CAP=0`, `CROSS_CALL_UNIQUE_PAIRS_DROPPED_FOR_SAFETY=0`.
  - `EMERGENCY_LIMIT_HIT=NO`.
  - `RETRIEVAL_CELL_SMALLER_THAN_EDGE_SCALE=NO` (campo real emitido; não existe o fictício `RETRIEVAL_RADIUS_RISK`).
  - `LATEST_WORKLOAD_COUNTS_EXHAUSTIVE=YES`, `ALIAS_PLAN_EXHAUSTIVE=YES`.
  - Limite de detalhe de texto: `CAP_DETAIL_OUTPUT_ACCEPTED=12` tentados 190 descartados 178 (`AFFECTS_EXHAUSTIVENESS=NO`, `DETAIL_OUTPUT_ONLY_SATURATION=YES`); saturação da saída textual não afeta a exaustividade da busca.
  - Coerência estrutural pendente: `CERTIFIED` sozinho não prova identidade de aresta; proveniência A/B e margens ainda não foram revisadas.
  - Correlação de patologia: `PATHOLOGY_CORRELATION_CAPTURED=NO` (`PATHOLOGICAL_USE_*=NOT_CAPTURED`).
- Desempenho e observação visual:
  - Usuário reportou extrema lentidão no jogo com V2 ativo e glitches visuais ainda presentes.
  - Avaliação visual: `INCONCLUSIVE` devido à desaceleração extrema. O dry-run não altera o pipeline de renderização nem implementa correção de produção; falhas visuais persistentes não refutam a validade do diagnóstico.
  - Tempos CPU capturados (self-time do diagnóstico): último workload `LATEST_SEARCH_CPU_MS=64` (same-call 6ms, cross-call 58ms); pico da sessão `SESSION_PEAK_SEARCH_CPU_MS=439`.
  - Hipótese de sobrecarga (`HYPOTHESIS` forte) sustentada pelos self-times medidos; NÃO constitui perfilamento completo de frame, nem FPS medido, nem comparação controlada OFF/ON A-B. Root solicitou relato comparativo OFF/ON opcional e aguarda resposta (não afirmar que A-B ocorreu).
  - Recomendação: usuário deve desativar V2 (`TOOIE_DIAG_P09_SEAM_EDGE_DRY_RUN_V2=0` ou unset) e executar normalmente sem diagnóstico para observação visual e de desempenho limpo.
  - Comparação qualitativa OFF/ON (`USER_OBSERVED`): usuário confirmou ("Sim, volta ao normal") que executar `./build/TooieRecompiled` sem a flag V2 restaura o desempenho normal, associando a lentidão severa ao V2 ativo. Confirma a associação relatada, mas não é FPS medido, benchmark controlado, prova de custo exclusivo de busca versus outra telemetria nem demonstração de causalidade por thread/frequência; self-times de busca existentes inalterados (64ms latest, 439ms pico); sem alegação de cura visual (o dry-run não altera renderização, a correção de produção não está implementada e os glitches visuais continuam presentes).
- Revisão independente Haiku (`/tmp/p09-haiku-performance.json`):
  - Executada via Claude CLI exato `claude-haiku-5-5 --effort medium` (15 turns, 72s, exit 0, `is_error=false`, canonical modelUsage firstParty).
  - Localizou callsite em `rt64_transform_processor.cpp:3259` no loop de workloads, mas não inspecionou todas as condições de guarda/frequência/thread ao redor do callsite; nunca afirmar que é incondicional a cada frame.
  - Nenhum patch de Grok, código ou ajuste de desempenho necessário nesta rodada.
- Status do P09: formal permanece pendente (`PENDING / NOT_FORMALLY_OPEN`), produção `NO`, limpeza/commit `NO`. Build de 14/10 mantido com ressalva de frescura.
- `NEXT_SINGLE_ACTION` (histórico): inspect certified paired-edge A/B provenance and residual/tolerance/margin details from preserved snapshot; only after coherent certification proceed exact-use pathology correlation. Do not implement aliases yet.

### 2026-10-15 — Revisão da captura V2 de arestas pareadas e arbitragem de contadores

- Revisão dos registros certificados do snapshot `/tmp/tooie-p09-seam-edge-dry-run-v2-20261015-030500.txt` (SHA256 `256d5fdd935c7d105810d885f4bbdb4047453606cdc5544bc56ef1e81bbbe6e5`):
  - Evidência dedicada: [[p09-seam-edge-v2-capture-review]].
  - Total de pares certificados no último workload: 190 (55 same-call, 135 cross-call).
  - Detalhes impressos: 12 pares (todos same-call, 0 cross-call, `CAP_DETAIL_OUTPUT_ACCEPTED=12` de 190 tentados, `AFFECTS_EXHAUSTIVENESS=NO`).
  - Coerência interna dos 12 pares: 12/12 `REVERSED`, resíduos 0.0 em ambos os endpoints nos dois frames (<= tolerância), vértices armazenados e mundos originais disjuntos em ambos os endpoints (`verts A ∩ verts B = ∅`, `(12,14)×1`, `(16,18)×3`, `(17,19)×3`, `(22,24)×3`, `(23,25)×2`), 24/24 checagens de margem de competidor aprovadas (`nextBest - best > marginNeed`).
  - Limite estrito de amostragem: 135 pares cross-call não impressos permanecem não inspecionados em detalhe; não generalizar os 12 pares para os 190 certificados nem para cross-call. Competidores e flags mutual-best não impressos por par; geometria local e matrizes não impressas (recomputação de `mul(localPos4, WORLD)` é `UNKNOWN`).
- Arbitragem de contadores e resolução da suspeita CASE3:
  - Claude Haiku revisão inicial (`/tmp/p09-haiku-edge-review.json`, exit 0): propôs `CASE3` por aparente inconsistência aritmética (151 tolpass vs 143 unique / 135 cert / 16 ambig; 8751 attempted vs 6688 accepted em classificação; 4609 cross vs 4627 boundary).
  - Grok revisão foreground (`/tmp/p09-grok-edge-review/report.txt`, `grok-4.7` medium 200k, exit 0, `stopReason: end_turn`): recomendou `CASE2`, verificou coerência das 12 linhas e fechamento das identidades contábeis.
  - Claude Haiku arbitragem de código (`/tmp/p09-haiku-counter-semantics-retry.json`, `claude-haiku-5-5` medium, exit 0, `subtype: success`): inspeção direta em `rt64/rt64` (`rt64_transform_processor.cpp`, HEAD `6f1c2d99`):
    - `uniquePartner` (cpp:2450-2462): incrementa se mútuo antes do teste de margem; `certified` (cpp:2464-2468) requer `complete && marginOk`; `ambiguous` (cpp:2471) para não certificados. Identidade: `135 cert + 16 ambig = 151 tolpass`; `143 mutual - 135 cert = 8` mutual ambíguos; `151 tolpass - 143 mutual = 8` não mútuos ambíguos (`8 + 8 = 16`). Sem contradição aritmética. Diferença de 8+8 é compatibilidade contábil, não prova de inspeção individual de todas as rotas de tolerância.
    - `classInsert` (cpp:1812-1826): `attempted` (8751) conta todas as inserções; `accepted` (6688) apenas chaves novas; diferença de 2063 são inserções repetidas em chaves já existentes; `dropped=0` (segurança nunca atingida).
    - `crossBoundary` (cpp:1958-1976): soma apenas grupos de projeção com `>= 2` chamadas; diferença de 18 arestas são chamadas isoladas na projeção, não falhas de borda ou descartes de segurança.
  - Alegação inicial CASE3 é `CORRECTED/SUPERSEDED` com base na evidência primária do código; histórico preservado. Suspeita resolvida; ausência de detalhes cross-call permanece limite de amostragem, não quebra de evidência.
- Decisão operacional: **`CASE2`**. Accounting exaustivo e 12 registros de detalhe coerentes bastam para o avanço exclusivo ao próximo portão: correlação de patologia por uso exato (CPU, default-off).
- Não há aprovação de identidade de produção, nem autorização para aliases ou clones.
- Observação de desempenho e visual do usuário mantida: sem FPS medido / sem causalidade de busca exclusiva provada; avaliação visual `INCONCLUSIVE`, correções visuais e produção `NOT_IMPLEMENTED`.
- Gate 3: `PASS` escopado aos 12 registros same-call e ao accounting do último workload.
- `NEXT_SINGLE_ACTION`: implement ONLY default-off CPU exact-use pathology correlation checking both pathological uses against full key (workloadId, fbPair, projection, gameCall, triangleLocal, corner, faceCorner, storedVertex, originalWorld); report total, covered, uncovered, ambiguous, and nonpathological selected; no aliases or performance patch now; correlation remains NOT_IMPLEMENTED/NOT_CAPTURED.

### 2026-10-15 — Implementação do fonte de correlação de patologia (escalonamento Sonnet 5.5)

- Escalonamento da integração: após rodadas de planejamento de Grok 4.7 medium sem escrita do código, a implementação complexa do fonte foi escalonada para Claude Sonnet 5.5 medium; continuação via CLI direto após limites de uso do operador no orquestrador nativo.
- Execução do Sonnet CLI: exit code 0 (`is_error=false`), `canonicalModel: claude-sonnet-5-5`. Claude Opus não foi utilizado.
- Fontes implementados por Sonnet:
  - RT64: `src/render/rt64_rsp_processor.cpp`, `src/render/rt64_transform_processor.cpp`, `src/render/rt64_transform_processor.h`, e novo `src/render/rt64_p09_exact_use_correlation.h` (header-only, helpers puros de CPU).
  - Tooie: `CMakeLists.txt` (novo alvo `p09_exact_use_correlation_test`, `EXCLUDE_FROM_ALL`), `tests/p09_exact_use_correlation_test.cpp` (novo teste de host).
  - Cópias de linha de base e hashes preservados em `/tmp/p09-correlation-implementation/baseline-files/` (relatório completo em `/tmp/p09-correlation-implementation/sonnet-implementation-report.txt`).
- Status de validação técnica:
  - Apenas verificação sintática `g++ -fsyntax-only` realizada no novo teste de host (`exit 0`, sem emissão de objeto, teste não executado).
  - Nenhum build de RT64 realizado, nenhum teste de host executado, sem captura de runtime e sem validação visual.
  - Classificação técnica estrita: `SOURCE_IMPLEMENTED / BUILD_PENDING / REVIEW_PENDING / NOT_CAPTURED`; nenhum `PASS` ou correção de glitches reivindicado.
- Nova flag de diagnóstico: `TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION` (pretendida como default-OFF standalone, plano sob demanda local à mesma chamada RSP sob `workloadMutex`; sem aliases, sem remap/clones e sem alteração na interpolação de produção; limites e comportamento de runtime ainda não validados).
- Próximos passos e propriedade: Grok 4.7 medium (`context_window=200000`, não fast) assume compilação/testes e correções pontuais de validação; revisão independente por Claude Haiku 5.5 seguirá.
- `NEXT_SINGLE_ACTION` (histórico): Grok 4.7 medium compiles RT64 and executes host test `p09_exact_use_correlation_test` (verifying all assertions pass, clean tests pass, and git diff --check passes); Haiku independent review follows; no aliases or runtime capture yet.

### 2026-10-15 — Validação, correções de revisão e aprovação final da correlação por uso exato

- **Divisão de trabalho e roteamento:**
  - Grok 4.7 medium (`context_window=200000`, não fast): compilação RT64, testes de host, implementação das correções de revisão final e testes adicionais (1231 asserções).
  - Claude Haiku 5.5 medium: revisões independentes de aceitação (inicial, intermediária de correções e final).
  - Claude Sonnet 5.5 medium: integração inicial prévia de produtor/chave/ciclo de vida.
  - Gemini 3.8 Flash high (Antigravity CLI): documentação técnica dos resultados. Claude Opus não utilizado.
  - Continuação via CLIs dedicadas após limites de sessão nos agentes nativos do operador.
- **Histórico de revisão independente e correções:**
  - *Revisão inicial Haiku (`haiku-review-output.json`):* `FAIL` de aceitação por ausência de métrica estrita considerando ambíguos no denominador (`PATH_PAIR_FULL_OF_ALL_OBSERVED`), flags de prontidão/truncamento incompletas e risco de exaustão prematura do orçamento de dispatch antes da cena de personagens.
  - *Revisão intermediária de correções Haiku (`haiku-fix-review-output.json`):* `FAIL` na aceitação 4 (o cooldown de 2000 ms ainda executava a varredura cara de chamadas `p09CorrGate` antes de descartar) e retorno de `::remove` no consumo do marcador de armação não verificado.
  - *Correções finais por Grok 4.7 medium (`grok-final-review-fixes-output.json`):*
    1. Helper compartilhado `p09CorrelationPreWalk` chamado antes do gate (`rt64_rsp_processor.cpp:4892`); Waiting, Exhausted e Cooldown retornam antes de qualquer varredura de chamadas ou projeções, incrementando `skippedWallClock` sem consumir tentativas ou `eligibleSeen`.
    2. Consumo transacional do marcador `/tmp/tooie-p09-seam-edge-pathology-correlation.arm`: checagem estrita de `::remove == 0`; falhas ativam latch `consumeFailedLatched` e incrementam `consumeFailures` sem comutar fase ou resetar sessão.
    3. Remoção de helpers mortos de reset de teste (`p09CorrelationBudgetReset`, `p09ExactUseCorrelationReset`).
    4. Cabeçalho de pré-requisitos no patch versionado `patches/rt64/p09-exact-use-pathology-correlation.patch` (SHA256 `5108ad3f82423dbc561c5ee2b13cfbf1f88d083140a628c07ed41c7f37d828d0`), explicitando que aplica-se sobre a linha de base dirty intencional em RT64 HEAD `6f1c2d99`, e não sobre HEAD limpo.
  - *Revisão final Haiku (`haiku-final-review-output.json`):* `PASS` em todos os itens remanescentes, restrito a evidências estáticas, helpers e testes de host.
  - *Arbitragem do Root:*
    - Nota LOW sobre lista de hashes: verificada como `CORRECTED`; `final-review-fixes-source-hashes.txt` contém todos os 7 caminhos e hashes idênticos.
    - Nota sobre ausência de linha de exit no log do build-test: leitura derivada confirma `EXIT:0`, corroborada pela execução do teste `1231/1231 passed` com `EXIT:0`.
    - Ramo de cooldown defensivo redundante em `p09CorrelationDecide`: classificado como não bloqueante (defensivo para chamadores diretos de teste).
- **Validação de build e testes executados:**
  - Build focado: `cmake --build build --target p09_exact_use_correlation_test` EXIT:0.
  - Teste de host focado: `./build/p09_exact_use_correlation_test` -> `1231/1231 passed`, EXIT:0.
  - Build principal: `cmake --build build --target TooieRecompiled` (`[35/35] Linking TooieRecompiled`) EXIT:0.
  - Testes de regressão: `window_display_test` 38/38 EXIT:0; `tintro_projection_policy_test` 7/7 EXIT:0.
  - Whitespace / diff check: `git diff --check` RT64 EXIT:0, Tooie EXIT:0.
  - Grafo e indexação: `graft build` 1426 nós EXIT:0; jCodeMunch `index_file` validado.
  - Prova do patch em cópia temporária: apply RC:0, reverse RC:0 com restauração exata dos hashes da linha de base; dry-run em HEAD limpo RC:1 (negativo esperado).
- **Invariantes preservados:**
  - Default OFF; CPU / dados estáticos; sem alteração de renderização, shaders, interpolação de produção, aliases ou clones.
  - Worktree dirty preexistente preservado (`NO_COMMIT=YES`, `NO_PUSH=YES`).
  - Sem alegação de correção visual de glitches ou cobertura de jogo 100%.
- **Evidência dedicada criada:** [[p09-exact-use-pathology-correlation]].
- **Fronteira técnica (histórico):** cobertura real de patologia, cena de personagens, tempos de CPU e validação visual permaneciam `NOT_CAPTURED`.
- **`NEXT_SINGLE_ACTION` (histórico):** user runs one scene capture with `TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION=1 ./build/TooieRecompiled`, arming via `/tmp/tooie-p09-seam-edge-pathology-correlation.arm` in character scene, and inspects correlation output; still no aliases or production fix.

### 2026-10-15 — Primeira captura de correlação (amostra vazia), revisão da cadeia de portão e relançamento aguardando cena

- **Primeira captura runtime realizada:**
  - Arquivo de captura gerado em `/tmp/p09-correlation-implementation/capture-empty-20261015.txt`.
  - Contadores de ciclo de vida e controle: `ARM_COUNT=1`, `REARM_COUNT=0`, `MARKER_CONSUME_FAILURES=0`, `MARKER_CONSUME_LATCHED=NO`.
  - Triagem de elegibilidade: `ELIGIBILITY_WALKS=181`, `ELIGIBLE_DISPATCHES=0`, `ATTEMPTS=0`, `PATH_PAIR_TOTAL=0` (pares=0).
  - Skips por ineligibilidade (ordem sequencial de primeira falha): `SKIPPED_INELIGIBLE_NO_WORKLOAD=61`, `SKIPPED_INELIGIBLE_NOT_INTERMEDIATE=59`, `SKIPPED_INELIGIBLE_NO_TARGET_CALL=61`. Contadores em zero: `SKIPPED_INELIGIBLE_NO_GEOMETRY=0`, `SKIPPED_INELIGIBLE_NO_FRAME_MATCH=0`, `SKIPPED_INELIGIBLE_NO_MODEL_BIN=0`.
  - Duração e contexto: usuário estimou a janela em ~30 segundos; cena em que ocorreu a captura: `UNKNOWN`.
  - Classificação técnica: `CAPTURED_EMPTY` / `NO_ELIGIBLE_SAMPLES`. **NÃO** constitui evidência de cobertura zero, ausência de patologias nem de correção.
  - Histórico preservado: o texto anterior de `RUNTIME_NOT_CAPTURED` permanece como registro histórico pré-captura; esclarece-se que amostra patológica significativa, prova visual e tempos reais de CPU continuam `NOT_CAPTURED`.
- **Revisão somente-leitura por Claude Haiku (`haiku-empty-capture-output.json`):**
  - Rastreou estaticamente a cadeia de elegibilidade no RT64 (`rt64_p09_exact_use_correlation.h:421-431` e `rt64_rsp_processor.cpp`): ordem first-failure `NO_WORKLOAD` -> `NO_GEOMETRY` -> `NO_FRAME_MATCH` -> `NOT_INTERMEDIATE` -> `NO_MODEL_BIN` -> `NO_TARGET_CALL`.
  - Confirmou alvos em `p09V4VisibleTargetIndex` (14 alvos V7 + 8 valores lógicos adicionais das DLs de penas de 0x62B).
  - **Limitação de inferência:** a inferência de que movimento é estritamente obrigatório ou de que o `model_bin` está resolvido globalmente **NÃO** é promovida como comprovada (`PROVEN`). Os contadores refletem a ordem de primeira falha; `NO_MODEL_BIN=0` ocorreu apenas para os 61 dispatches que chegaram a esse teste na cadeia. A dependência de `curFrameWeight` em relação a movimento versus configurações de taxa de quadros/interpolação permanece `INFERENCE` não verificada.
- **Relançamento pelo Root e estado operacional:**
  - O root relançou o executável com a flag dedicada (`TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION=1 ./build/TooieRecompiled`).
  - Confirmado via arquivo de status: `STATE=WAITING_FOR_REQUEST`, zero walks (`ELIGIBILITY_WALKS=0`).
  - Usuário está posicionando o personagem em cena de gameplay antes de o root armar a captura via marcador.
- **`NEXT_SINGLE_ACTION` (histórico):** wait for user ready in character scene then arm and inspect eligible/attempt counters; still no aliases or production fix.

### 2026-10-15 — Segunda captura runtime em gameplay (194/194 TIE) e análise de empates no nearest-join

- **Captura em gameplay realizada:**
  - Usuário confirmou presença na cena de gameplay dos personagens; root armou a captura via marcador após prontidão.
  - Arquivo de captura: `/tmp/p09-correlation-implementation/capture-gameplay-20261015.txt`.
  - SHA256: `b6be018a3619a78a7c05ba7d0b8fa6c64e27d22e5cde6bedbebeebdf771203e7`.
  - Proveniência completa: captura vazia anterior mantida como histórico. Jogo mantido aberto em estado `EXHAUSTED` (sem novas buscas sem novo rearm).
  - Estado e orçamento: `STATE=EXHAUSTED` após 24 tentativas (`ATTEMPTS=24`).
  - Triagem de elegibilidade: `ELIGIBILITY_WALKS=2135`, `ELIGIBLE_DISPATCHES=93`.
  - Execução de buscas e planos: 4 tentativas com par patológico (`ATTEMPTS_WITH_PATH_PAIR=4`, 20 sem par patológico), 4 buscas de costura executadas (`SEAM_SEARCHES=4`), 4 planos locais construídos com sucesso (`PLAN_BUILT=4`).
  - Integridade da amostragem: sem falhas de plano, saturação ou descartes (`PLAN_INACTIVE=0`, `PLAN_NOT_EXHAUSTIVE=0`, `PAIRS_DROPPED_PENDING_CAP=0`, `VERTS_DROPPED_BONE_CAP=0`, `UNIQUE_TABLE_OVERFLOW=0`, `RECORDED_COHORT_TRUNCATED=NO`).
  - Contadores de correlação patológica:
    - `PATH_PAIR_TOTAL=194`, `PATH_PAIR_FULLY_COVERED=0`, `PATH_PAIR_ONE_SIDED=0`, `PATH_PAIR_NOT_COVERED=0`.
    - `PATH_PAIR_AMBIGUOUS=194`, `PATH_PAIR_AMBIGUOUS_TIE=194`. Todos os 194 pares observados são ambíguos por empate (`TIE`).
    - Ocorrências cumulativas: tratam-se de 194 ocorrências cumulativas de pares patológicos ao longo dos 4 planos (`PATH_OCCURRENCE_TOTAL=388`), e **não** de 194 arestas certificadas únicas.
    - Chaves exatas únicas: `PATH_UNIQUE_KEYS=214` (1 selecionada / `PATH_UNIQUE_KEYS_SELECTED=1`, 213 não selecionadas / `PATH_UNIQUE_KEYS_NOT_SELECTED=213`).
  - Universo não patológico: `NONPATH_PAIR_TOTAL=454`, `NONPATH_OCCURRENCE_TOTAL=908` (54 selecionadas, 854 não selecionadas), `NONPATH_UNIQUE_KEYS=481` (4 selecionadas, 477 não selecionadas). `UNIQUE_KEYS_ON_BOTH_PATH_AND_NONPATH=25`.
  - Tempos de CPU (telemetria do diagnóstico em runtime, **não** FPS do jogo nem prova visual):
    - Tentativas pesadas: pico de 202.762 ms (`CPU_ATTEMPT_MS_PEAK=202.762`), total de 590.915 ms (`CPU_ATTEMPT_MS_TOTAL=590.915`).
    - Plano de costura: pico de 200.776 ms (`CPU_SEAM_PLAN_MS_PEAK=200.776`), total de 551.464 ms (`CPU_SEAM_PLAN_MS_TOTAL=551.464`).
    - Triagem de portão: total de 10.382 ms (`CPU_GATE_MS_TOTAL=10.382`).
  - Avaliação de cobertura: dados significativos de runtime capturados (`MEANINGFUL_RUNTIME_DATA_CAPTURED`), porém a taxa de cobertura é indecidível no momento (`PATH_PAIR_FULL_OF_ALL_OBSERVED=0.000000`, taxa definitiva `NA`) devido aos 194 empates. Sem correção de produção.
- **Revisão da causa do empate por Claude Haiku (`haiku-tie-review-output.json`):**
  - Mecanismo do código comprovado (`PROVEN` no fonte): a busca de junção mais próxima (`p09EmitNearestJoinPairs`, `rt64_rsp_processor.cpp:2115-2131`) avalia a soma de distâncias euclidianas em NDC `p09NdcDistXY(child.prev, parent.prev) + p09NdcDistXY(child.cur, parent.cur)`.
  - Uma melhora estrita limpa `bestTie = false` (:2126); a ocorrência de igualdade exata float `gap == bestGap` (:2129, sem epsilon) ativa `bestTie = true`.
  - No classificador (`rt64_p09_exact_use_correlation.h:167`), a checagem `if (tie) return ambiguous(Tie);` executa com precedência sobre a verificação de completude e pertinência das chaves, forçando a classificação como ambíguo.
  - **Limites da análise:** a hipótese de pais duplicados ou vértices coincidentes na lista permanece `UNKNOWN`. **NÃO** aceitar a afirmação de que coordenadas idênticas tornam automaticamente a ambiguidade "falsa", pois diferentes chaves de origem (`sourcekeys`) e diferentes matrizes/políticas de `WORLD` continuam relevantes para a identidade de renderização. Nenhuma supressão automática de empate ou dedup de produção está autorizada.
- **Consolidação da base causal e direção de correção ([[p09-causal-baseline-and-fix-direction]]):**
  - Causa já estabelecida (`CAUSE_ALREADY_ESTABLISHED: YES`): a classe `NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS` foi provada decisivamente através dos controles diagnósticos `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1` e `TOOIE_DIAG_FORCE_CURRENT_FRAME=1`.
  - Correção de produção não implementada (`PRODUCTION_FIX_NOT_IMPLEMENTED: YES`): não há aliases de produção ou remaps em vigor.
  - A captura existente de 194 empates (`capture-gameplay-20261015.txt`, SHA256 `b6be018a...`) permanece como bloqueio diagnóstico de autorização por NDC mais próximo, e **não** como causa raiz desconhecida.
  - Recomendação Haiku de novos discriminadores visuais CLIP_LERP/NDC_LERP arbitradada como `SUPERSEDED` pelo encerramento em `p09-articulated-temporal-coherence-root.md`.
  - O próximo discriminador serve à seleção e autorização para a causa conhecida, não a uma nova busca por causa raiz.
- **`NEXT_SINGLE_ACTION` (histórica):** one bounded diagnostic discriminator logging first ~20 tie groups, exact full sourcekeys/originalWORLD and prev/cur positions, gap/multiplicity and candidate memberships to distinguish repeated same-use from different legitimate-use; then review, before alias/interpolation edits (currently NOT_IMPLEMENTED; task stops at newly observed blocker per scope rule).

### 2026-10-16: Captura de 20 grupos de empate, revisão independente e arbitragem final do Root

- **Data/hora e objetivo**: 2026-10-16; implementar discriminador dos primeiros 20 grupos de empate na busca de junção mais próxima (`TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION`), registrar chaves completas de 9 campos, multiplicidade, coordenadas NDC/WORLD e pertinência ao plano local certificado V2.
- **Roteamento de modelos**:
  - Grok 4.7 medium (não fast, 200k): implementação no RT64 (`rt64_rsp_processor.cpp`, `rt64_p09_exact_use_correlation.h`), testes de host (`p09_exact_use_correlation_test.cpp`, 1354/1354 passed exit 0).
  - Claude Haiku 5.5 medium: revisão estática de código/patch (sem defeitos bloqueantes; identificou ausência de checagem de retorno de `fclose` antes de `rename`) e análise detalhada dos artefatos de captura.
  - Claude Sonnet 5.5 medium: escalonamento explícito após evidências; análise de soluções preliminares e subsequente arbitragem retirando alegações preliminares.
  - Gemini 3.8 Flash high: documentação e sincronização de handoffs.
  - Root: armação da captura e arbitragem final.
- **HEADs e dirty state inicial**:
  - TooieRecomp: `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf`.
  - RT64: `6f1c2d99a4ea571c139f449c326fd176ba8f3496` (baseline dirty intencional documentada).
- **Gates e binários**:
  - Host test: `./build/p09_exact_use_correlation_test` exit 0 (`1354/1354 passed`).
  - Build gate: `cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test` exit 0 (`[35/35] Linking CXX executable TooieRecompiled`).
  - Binários: `TooieRecompiled` SHA256 `84d31479e6919d999b2e6d49d972d7fd49c06f4faf56807f2850b27a1df86426`; unitários 38/38 e 7/7 exit 0.
  - Diff check: `git diff --check` exit 0 em TooieRecomp e RT64.
  - Patch: `patches/rt64/p09-exact-use-pathology-correlation.patch` SHA256 `7b93258cecadcb0b18b1f4a22f0a278c70a1cc20f3e1f0ed8fbb112b0bd7f38f` (86535 bytes).
- **Captura runtime em gameplay**:
  - Correlação: `/tmp/p09-tie-discriminator/capture-gameplay/tooie-p09-seam-edge-pathology-correlation.txt` (SHA256 `74b0b2e5f219522c42b041a6a5cea9005bea8f57ddd0fc595df8f69024528c7d`).
  - Ties: `/tmp/p09-tie-discriminator/capture-gameplay/tooie-p09-seam-edge-pathology-ties.txt` (SHA256 `5b1fc21a2af77dbab9abc42919465e45a65a13014321cee405468cc6fd59b2a8`).
  - Limite atingido: `EXHAUSTED` em 24 tentativas, 6 planos, 162 ocorrências patológicas (todas `TIE`), 179 chaves únicas (`PATH_OCCURRENCE_TOTAL=324`, `PATH_UNIQUE_KEYS=179`).
  - Amostra dos 20 grupos: 20 armazenados (`kP09TieGroupCap=20`), 142 omitidos (`skippedGroups=142`), 0 truncados (`membersTruncated=0`). 12 grupos com mult=2, 8 grupos com mult=4 = 56 membros pai.
  - Estrutura: 2 `storedVertex` distintos por grupo no mesmo `WORLD` pai; coordenadas impressas `PREV/CUR_NDC` e `PREV/CUR_WORLD` idênticas (não assumir identidade binária). 56 membros e 20 filhos retornaram `MEMBERSHIP=NOT_SELECTED` (`REASON=NONE`) no plano local ativo e exaustivo.
- **Análises e arbitragens**:
  - Correção Haiku: `selected=YES` (escolha de busca por índice 0) e `MEMBERSHIP=NOT_SELECTED` (pertinência no plano) são campos independentes. `REASON=NONE` significa consulta válida sem erro de lookup, não legitimidade nem falha de classificador.
  - Arbitragem Root / Sonnet: Sonnet retirou a proposta P1 de certificação pontual (k-pontos, reciprocidade, 0.08; `CASE_B` continua vigente), alegação de omissão global do plano e persistência de IDs brutos. As 3 opções (P1 slot selector, P2 alias local B+C, P3 snap) permanecem `HYPOTHESIS`.
- **Fronteira e status**: `PRODUCTION_FIX_NOT_IMPLEMENTED: YES`, `P09_FORMAL_PHASE: PENDING / NOT_FORMALLY_OPEN`, `NO_COMMIT: YES`, `NO_PUSH: YES`.
- **NEXT_SINGLE_ACTION**: a READ-ONLY focused audit of existing paired-boundary-edge/provenance data and its correlation to the sampled pathological uses, starting with producer/topology semantics and what current artifacts actually contain; no new lerp/alias or telemetry implementation automatically. Root stops at newly identified evidence blocker after Sonnet analysis as authorized, not claiming fix.

### 2026-10-16 — Fase 1 contorno padrão (tfPos CURRENT) implementado e Fase 2 presets de FPS planejados

- **Data/hora e objetivo**: 2026-10-16T03:37:30Z; diretriz explícita do usuário para transformar o contorno paliativo (`tfPos` CURRENT) em padrão operacional de TooieRecomp, documentar, e em seguida planejar o seletor de FPS {20, 30, 45, 60, 70, 100, 120, 144} sem mudança de velocidade de simulação.
- **Roteamento de modelos**:
  - Grok 4.7 medium (não fast, 200k): implementação de código em TooieRecomp (`apply_default_before_renderer_init()`), testes de unidade (`tooie_rsp_endpoint_current_test.cpp`), compilação e verificação de hashes.
  - Claude Haiku 5.5 medium: revisão estática de código (PASS, sem defeitos bloqueantes) e auditoria técnica da arquitetura de ritmo/pacing de apresentação no RT64.
  - Gemini 3.8 Flash high: documentação técnica e sincronização dos handoffs e plano da fase.
  - Root: lançamento de teste em gameplay normal, persistência do log de teste (`policy-test-root.log`) e QA com o usuário.
- **HEADs e dirty state inicial**:
  - TooieRecomp: `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf`.
  - RT64: `6f1c2d99a4ea571c139f449c326fd176ba8f3496` (intocado; dirty baseline de 31 arquivos +13850/-44 preservada).
- **Mudança executada (Fase 1)**:
  - Adicionada rotina `apply_default_before_renderer_init()` em `src/game/tooie_rsp_endpoint_current.cpp` e `include/tooie_rsp_endpoint_current.h`.
  - Chamada em `src/main/main.cpp:1153` antes de `activate_tooie_guest_preemption()` e `recomp::start()`.
  - Política: define `"1"` somente quando `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` estiver unset. Qualquer valor explícito existente (`"0"`, `""`, `"1"`, `"keep"`) é preservado. O valor explícito `0` restaura a interpolação original no próximo lançamento.
  - O executável normal `./build/TooieRecompiled` dispensa flags manuais.
  - Falha de `setenv`/`_putenv_s` imprime aviso e o startup prossegue.
- **Comandos e exit codes**:
  - `cmake --build build --target tooie_rsp_endpoint_current_test`: exit 0.
  - `./build/tooie_rsp_endpoint_current_test`: exit 0 (20/20 asserções PASS, log persistido em `/tmp/p09-default-workaround/policy-test-root.log`).
  - `cmake --build build --target TooieRecompiled`: exit 0 (binário SHA256 `0d2e4ba5fa7e4ed0347d83180ff4c831c54b8f47567fa012de15dac3dc736b84`).
  - `git diff --check`: exit 0.
  - Testes não tocados (38, 7, 1354) NOT RUN nesta rodada.
- **Revisão Haiku e achados documentais**:
  - Código aprovado (PASS). Sem blocker.
  - Esclarecido que string vazia `""` desativa a flag no RT64 (restaurando interpolação original). Apenas `"0"` e `""` desativam; outros valores não nulos ativam.
  - Regra explícita para capturas futuras: ferramentas de diagnóstico DEVEM definir `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0` para evitar mascarar o glitch visual.
  - Validação de ordenação em `main.cpp` é estática no código-fonte, não prova de traço GPU em runtime.
- **Esclarecimento de proveniência de execução**:
  - O relatório do escritor `report.txt` com "Game was not launched" reflete estritamente a fronteira do sandbox de escrita.
  - O Root executou efetivamente `./build/TooieRecompiled` sem flags de ativação (`/tmp/p09-default-workaround/normal-launch.log`, ~852KB).
- **QA de gameplay pelo usuário**:
  - Observação do usuário: "Está rodando bem. O áudio tem o mesmo glitch de sempre que já tinha desde quase o início do projeto e pretendo resolver o áudio só mais para frente."
  - Classificação: `USER_OBSERVED` normal run funciona bem. Glitch de áudio preexistente mantido deferred a pedido do usuário; sem alterações em áudio.
- **Fatos de pacing e bloqueio da Fase 2 (presets de FPS)**:
  - 8 presets solicitados: `20, 30, 45, 60, 70, 100, 120, 144`.
  - Frontend: `ui_config_tab_graphics.cpp:311-313` já aceita opção manual inteira 20..240 step 1; runtime aceita 10..1000. Reusar esta opção na UI futura com os 8 presets, sem duplicar controles.
  - Limitações de pacing do RT64 (`rt64_workload_queue.cpp:1175`): interpolação/ritmo só atua quando `targetRate > viOriginalRate`. Para alvos menores ou iguais à taxa nativa, `generateInterpolated=false` e `displayFrames=1`, apresentando um quadro por workload sem limitação de taxa ou sleep.
  - Logo, a honestidade das taxas 20/30 não é garantida dependendo da cena e do Hz nativo do guest (atualmente UNKNOWN).
  - Taxas fracionárias/não-divisoras (45, 70, 100) são acomodadas por acumulador de ticks inteiros acima da taxa nativa, sem criar novos updates de guest.
  - Taxas elevadas são limitadas pelo swapchain de hardware (ex.: 144 Hz em tela de 60 Hz roda a 60 Hz).
  - Decimação de apresentação real sem pular trabalho de guest ou pausar threads de guest requer hook seguro na fila de apresentação; backpressure em `waitForWorkloadId` permanece UNVERIFIED.
  - Fase 2 permanece planejada e bloqueada por essa restrição de temporização, não implementada por simples UI.
- **Status e limites**: `DEFAULT_WORKAROUND_IMPLEMENTED: YES`, `PERMANENT_INTERPOLATION_FIX_NOT_IMPLEMENTED: YES`, `FPS_PRESETS_PLANNED_NOT_IMPLEMENTED: YES`, `P09_FORMAL_PHASE: PENDING / NOT_FORMALLY_OPEN`, `NO_COMMIT: YES`, `NO_PUSH: YES`. Fila de pesquisa anterior (grupos de empate / Sonnet) preservada como histórico diferido.
- **NEXT_SINGLE_ACTION**: singular focused audit/measurement of native cadence and present-queue backpressure to choose a below-native presentation cap without clock changes; then implement presets with actual tests all 8 and timing QA.

### 2026-10-17 — Implementação e validação do decimador de apresentação RT64 (Phase 2 P09)

- **Objetivo**: Implementar decimação de apresentação para limites de taxa abaixo da taxa nativa do jogo (pacing) sem alterar temporização do guest, tarefas de áudio ou sincronização de workloads, e validar através de testes de host e builds completos.
- **Arquitetura e implementação**:
  - `src/hle/rt64_present_decimator.h`: adicionado struct `PresentDecimator` com acumulador inteiro de ticks (`ticks += targetRate`, `ticks >= originalRate`). Garante cadência média exata para divisores (20, 30) e não divisores (45, 70, ...) sem desvio de ponto flutuante ou overflow (`uint32_t`).
  - `src/hle/rt64_present_queue.cpp`: integrado na fila de apresentação para taxas manuais (`manualRate && !present.paused && targetRate > 0 && viOriginalRate > 0 && targetRate < viOriginalRate`).
  - Prova estática do fluxo de ACKs: quadros pulados reutilizam o caminho de catch-up skip (`skipInterpolation()`, `notifyPresentId(present)`, liberação de operação de framebuffer e sinalização de `threadAdvanceBarrier`). Todos os workloads e sinais de ACK são consumidos na ordem original; threads de guest, VI e áudio nunca dormem nem pausam.
- **Revisão e correções de concorrência (Sonnet followup)**:
  - Data race eliminada: identificado que `rt64_workload_queue.cpp:1021` escrevia `viOriginalRate` sem lock, enquanto `rt64_present_queue.cpp` lia sob `configurationMutex`. A escrita foi protegida com `const std::scoped_lock configurationLock(ext.sharedResources->configurationMutex)`. Ordem de locks verificada: `configurationMutex` é leaf lock em ambas as threads; sem inversão de locks.
  - Resets de cadência: catch-up skip reseta explicitamente o decimador (`presentDecimator = PresentDecimator()`); quadros pausados chamam `shouldPresent()` com `paused == true`, resetando a cadência para que o primeiro quadro pós-pausa seja apresentado imediatamente sem descarte.
- **Testes e artefatos**:
  - Teste unitário de host: `tests/rt64_present_decimator_test.cpp` adicionado ao `CMakeLists.txt` (`rt64_present_decimator_test`). Testa 20/60, 30/60, 45/60, 20/30, exibição do primeiro workload, repasse de Original/não-manual/igual ou superior/desconhecido/pausa, ressincronização de taxa e reset em pausa. Teste executa com exit 0 (`PASS`).
  - Teste de contorno de endpoint: `tooie_rsp_endpoint_current_test` exit 0 (20/20 asserções PASS).
  - Patch versionado: `patches/rt64/rt64-present-rate-decimator.patch`.
- **Validação de build do Root**:
  - Build completo: `cmake --build build --target TooieRecompiled rt64_present_decimator_test tooie_rsp_endpoint_current_test -j2` exit 0 (`root-build.log`, `[362/362] Linking CXX executable TooieRecompiled`).
  - Gate incremental final: `root-build-final.log` concluído com exit 0 (`[6/6] Linking CXX executable TooieRecompiled`).
  - Checagem de diff: `git diff --check` exit 0 em Tooie e RT64.
- **Fronteiras epistêmicas e limitações**:
  - QA em runtime NÃO executada: nenhum processo de jogo foi lançado; sem medições reais de FPS em gameplay ou garantias empíricas de velocidade.
  - Prova do caminho de ACK é estática via fluxo de controle no código.
  - Taxas acima da nativa: quadros intermediários repetem a pose de vértice CURRENT (`tfPos`); interpolação contínua e suave acima da taxa nativa continua impossível sem a correção arquitetural permanente de interpolação.
  - Frequência nativa do guest: `viOriginalRate` formalmente `UNKNOWN` (não medido).
  - Seletor de 8 presets na UI: permanece `PLANNED` / `NOT_IMPLEMENTED`.
- **NEXT_SINGLE_ACTION**: with user authorization, launch `./build/TooieRecompiled`, enable "Show Present FPS" in Graphics options, and visually verify top-left overlay presentation and pose repeat behavior in gameplay.

### Sessão 2026-10-17 — Top-left present FPS counter e settings toggle verificados

- **Data/hora e objetivo**: 2026-10-17 16:10; verificar implementação, patches e testes do contador de FPS no canto superior esquerdo e toggle de settings ("Show Present FPS"), conforme diretiva do usuário e relatórios `counter-grok.json` e `counter-sonnet.json`.
- **HEADs + dirty state inicial**:
  - `TooieRecomp`: `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf` (arquivos preexistentes modificados preservados).
  - `BanjoRecomp/lib/rt64`: `6f1c2d99a4ea571c139f449c326fd176ba8f3496` (31 arquivos dirty preservados).
  - `BanjoRecomp/lib/RecompFrontend`: `d0d90ba49f46f4896aaeda362056c21b1e342561` (limpo antes das alterações).
- **Mudança executada**:
  - Opção booleana "Show Present FPS" adicionada em `add_graphics_options` (`src/game/config.cpp`), chave `banjo::configkeys::graphics::show_fps_counter` (`include/banjo_config.h`), padrão `false`, persistência em `graphics.json`. Callback aciona `tooie::set_fps_overlay_enabled()`.
  - Contador atômico `RT64::presentSuccessCount` incrementado exclusivamente quando `swapChain->present()` retorna `true` (`rt64_present_queue.cpp:513`).
  - Hook RmlUi `recompui::set_overlay_update_callback` em `RecompFrontend` (`ui_state.cpp`), executado na render thread em `draw_hook()`.
  - Medidor de cadência e texto `tooie::PresentRateMeter` (`include/tooie_fps_meter.h`, `src/game/fps_overlay.cpp`), exibindo `"%u FPS"` a 16 dp do topo-esquerdo com fundo escuro. Stalls > 3 s mostram `"-- FPS"`.
- **Comandos + exit codes**:
  - `./build/tooie_fps_meter_test`: exit 0 (`tooie_fps_meter_test PASS`).
  - `git apply --reverse --check patches/frontend/...`: exit 0.
  - `git apply --reverse --check patches/rt64/...`: exit 0.
  - `git diff --check`: exit 0 em Tooie, RT64 e RecompFrontend.
- **Artefatos e patches versionados**:
  - `patches/frontend/recompfrontend-overlay-update-callback.patch`: SHA256 `37266d0bd13d42c0b8f174c0118b6a9b2490784c5880f05e6ee37ab811cd48a7`.
  - `patches/rt64/rt64-present-success-counter.patch`: SHA256 `173c9b7cafc4b9750fb610b56faca8125399679a874d1c159f6c53e420c31c68`.
- **Fronteiras epistêmicas e limites**:
  - QA em runtime NÃO executada: nenhum processo de jogo lançado; sem observação visual in-game do overlay neste checkpoint.
  - Limitação de cadência vs simulação: sob o contorno paliativo CURRENT `tfPos` ativo, taxas acima da nativa repetem a pose de vértice; contador próximo a 60 FPS com movimento aparente a 30 Hz reflete a frequência real de swapchain present, não velocidade de simulação.
  - Hz nativo do guest: `viOriginalRate` permanece formalmente `UNKNOWN`.
  - Seletor de 8 presets na UI: permanece `PLANNED` / `NOT_IMPLEMENTED` (`REQ-GFX-06`).
- **NEXT_SINGLE_ACTION**: with user authorization, launch `./build/TooieRecompiled`, enable "Show Present FPS" in Graphics options, and visually verify top-left overlay presentation and pose repeat behavior in gameplay.

### Modelo para a próxima atualização

```text
Data/hora e objetivo:
HEADs + dirty state inicial:
Mudança executada:
Comandos + exit codes:
Captura/artefato + hash:
Resultado e classificação (PROVEN/HYPOTHESIS/UNKNOWN/...):
Validação visual (quem observou, cena, modo, flags):
Não testado / bloqueio:
Documentos sincronizados:
Status final / commit / push:
NEXT_SINGLE_ACTION:
```

## Referências

- [[../../HANDOFF]]
- [[p09-session-handoff]]
- [[../../plans/BT/09-gameplay-parity-and-enhancements]]
- [[p09-fps-counter-and-settings-toggle]]
- [[p09-default-current-workaround-and-fps-plan]]
- [[p09-pathological-tie-groups]]
- [[p09-causal-baseline-and-fix-direction]]
- [[p09-exact-use-pathology-correlation]]
- [[p09-seam-edge-v2-capture-review]]
- [[p09-seam-edge-dry-run-v2]]
- [[p09-call-local-world-interpolation-architecture-review]]
- [[p09-seam-alias-dry-run-validity-review]]
- [[p09-elementwise-world-interpolation-causal-discriminator]]
- [[p09-cpu-diagnostic-shader-semantics-repair]]
