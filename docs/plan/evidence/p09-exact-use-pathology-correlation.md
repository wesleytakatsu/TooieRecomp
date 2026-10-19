---
title: "P09 — Correlação de patologia por uso exato"
description: "Implementação, testes de host, revisão independente e limites do diagnóstico de correlação de patologia por uso exato."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-10-15
tags: [p09, interpolation, diagnostic, correlation, exact-use, review, rt64]
---

# P09 — Correlação de patologia por uso exato

## 1. Objetivo

Implementar a correlação de patologia por uso exato no RT64/TooieRecomp como diagnóstico puramente CPU e default-OFF (`TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION`), verificando se ocorrências de arestas de costura certificadas coincidem com os usos em renderização das junções patológicas conhecidas (penas da Kazooie, mochila/aba superior e focinho do Banjo, penas do Mumbo).

O diagnóstico não altera shaders, não ativa interpolação de produção, não cria aliases de `worldIndices`, não gera remap/clones de vértices e não edita arquivos de `RecompiledFuncs/**`.

## 2. Divisão de trabalho e roteamento de modelos

Conforme diretrizes do projeto e restrições de execução:

- **Grok 4.7 medium** (`context_window=200000`, não fast): modelo principal para código, testes de host e resolução das correções de revisão final.
- **Claude Sonnet 5.5 medium**: escalonamento pontual para a integração complexa de produtor/chave/ciclo de vida após tentativas de planejamento pelo Grok.
- **Claude Haiku 5.5 medium**: análise estática e revisões independentes de aceitação (rodada inicial, rodada intermediária de correções e revisão final).
- **Gemini 3.8 Flash high** (via Antigravity CLI `agy`): responsável exclusivo por toda a documentação Markdown.
- **Claude Opus**: não utilizado.
- **Continuidade operacional**: agentes nativos de operação atingiram limites de uso da plataforma; o root prosseguiu diretamente via CLIs dedicadas com rastreabilidade explícita.
- **Evolução dos testes**: a suíte inicial com Sonnet continha 1078 asserções (`PASS`), mas a aceitação da revisão independente Haiku resultou em `FAIL`; após as correções finais pelo Grok, a suíte expandiu para 1231 asserções (`1231/1231 PASS`).

## 3. Comando de captura e fluxo do marcador de requisição

### 3.1 Execução e flag
- **Comando de execução**:
  ```sh
  TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION=1 ./build/TooieRecompiled
  ```
- **Flag dedicada**: `TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION`.
  - Fail-closed: aceita apenas `1`, `true`, `yes`, `on` (insensível a maiúsculas/minúsculas).
  - Standalone: opera de forma independente, sem exigir as flags pesadas legadas de V2 ou endpoints.

### 3.2 Arquivo marcador e ciclo de vida (`P09CaptureControl`)
- **Caminho do marcador**: `/tmp/tooie-p09-seam-edge-pathology-correlation.arm`.
- **Estado inicial**: inicia em `WaitingForRequest`.
- **Tratamento de marcador obsoleto (stale)**: se o arquivo já existir no momento da inicialização do jogo antes de uma ausência ser observada, ele é removido e ignorado (`StaleIgnored`), impedindo captura prematura antes de o jogador posicionar a cena.
- **Armação transacional**:
  - A armação exige que a ausência do marcador seja observada pelo menos uma vez.
  - Quando o usuário cria o arquivo marcador na cena desejada, o loop de polling (~1 Hz no RSP) tenta a remoção via `::remove(kP09CorrArmPath) == 0`.
  - Somente o retorno zero de `::remove` comuta o estado para `Capturing` (`Armed` event).
- **Falha de consumo**: se `::remove` falhar, o estado latencia `consumeFailedLatched`, incrementa `consumeFailures` uma única vez e mantém o estado anterior sem armar ou rearmar repetidamente enquanto o arquivo persistir.
- **Rearmação e reset de sessão**: a criação de novo marcador durante `Capturing` ou `Exhausted` resulta em `Rearmed`, que executa `p09CorrResetCorrelationOnly` (zerando acumuladores de correlação e geração, preservando os contadores de ciclo de vida do controle).

### 3.3 Cadência de relógio de parede e limites finitos
- **Intervalo entre tentativas pesadas**: `kP09CorrelationAttemptIntervalMs = 2000` (2 segundos).
- **Pre-walk cooldown**: a função pura `p09CorrelationPreWalk` avalia a fase, a exaustão de orçamento e a cadência de 2000 ms *antes* de executar a varredura cara de chamadas (`p09CorrGate`). Durante o intervalo de 2000 ms, retorna `Cooldown` e incrementa `skippedWallClock` sem consumir tentativas nem `eligibleSeen`.
- **Limites de segurança de execução**:
  - Limite máximo de 24 tentativas pesadas (`maxAttempts = 24`).
  - Limite máximo de 8 buscas exaustivas de costura (`kMaxSeamSearches = 8`). Hitting desse teto encerra a captura mesmo com tentativas restantes (`p09CorrelationBudgetExhausted`).
  - A 2 segundos de intervalo, 24 tentativas consom ao menos ~46 segundos de tempo real. Cenas curtas podem permanecer em `Capturing` com `CAPTURE_RUN_INCOMPLETE=YES`.
- **Risco de travamento (hitch)**: a busca exaustiva em CPU durante a tentativa pesada é síncrona na thread do RSP e pode causar hitch perceptível (`EXHAUSTIVE_CPU_SEARCH_CAN_HITCH=YES`). `NO_WALL_TIME_ABORT=YES`. Métricas de FPS e tempo de frame permanecem `NOT_CAPTURED`.

## 4. Métricas e escopo amostrado

### 4.1 Chave de renderização de 9 campos
A correlação transporta chaves completas de 9 campos para ambos os membros do par:
```text
workloadId, fbPair, projection, gameCall, triangleLocal, corner, faceCorner, storedVertex, originalWorld
```
O plano é construído localmente na mesma chamada de coleta sob `g_p09RspEndpointBindingMutex` (`p09BuildLocalAliasPlan`), evitando a sobreposição global insegura anterior de transformações pré-RSP.

### 4.2 Métricas de cobertura estrita vs. secundária
- **Métrica estrita de todos os observados**:
  ```text
  PATH_PAIR_FULL_OF_ALL_OBSERVED = full / (full + one_sided + not_covered + ambiguous)
  ```
  Casos ambíguos permanecem obrigatoriamente no denominador.
- **Taxa definitiva secundária**:
  ```text
  PATH_PAIR_COVERAGE_RATE_DEFINITIVE = full / (full + one_sided + not_covered)
  ```
  Exclui ambíguos do denominador; rotulada expressamente como métrica secundária.
- **Prontidão de coorte observada**:
  `OBSERVED_COHORT_READY_FOR_REVIEW=YES` exige que todos os pares observados sejam `full`, total > 0 e dados não truncados (`RECORDED_COHORT_TRUNCATED=NO`).
  Esta prontidão aplica-se estritamente à coorte inspecionada; **nunca** constitui prontidão global de jogo ou produção (`GLOBAL_READY=NO`, `GLOBAL_GAME_COVERAGE=NO`, `VISUAL_PROOF=NO`).

### 4.3 Limites do escopo amostrado
Filtros preexistentes de modelo, bin e teto de 96 vértices por slot (`kP09ArticulatedJoinPairs`, primeiras 96 posições) limitam a amostragem. Nem todos os modelos do jogo nem todos os 135 pares certificados cross-call foram observados nesta etapa.

## 5. Limites de segurança e desempenho

- **Default OFF**: desativado por padrão; sem impacto em builds padrão.
- **CPU / static data**: sem readbacks de GPU, sem buffers de staging, sem barreiras ou sincronizações adicionadas ao pipeline de renderização.
- **Sem alteração de render**: interpolação original inalterada; sem aliases de produção, sem remapeamento de `worldIndices` e sem clonagem física de vértices.
- **Preservação do worktree**: alterações locais preexistentes em RT64 mantidas intactas.

## 6. Comandos e resultados reais dos testes

| Comando / Teste | Resultado | Exit Code | Observação |
| --- | --- | --- | --- |
| `cmake --build build --target p09_exact_use_correlation_test` | Compilação e link concluídos | EXIT:0 | Binário do teste de host focado atualizado |
| `./build/p09_exact_use_correlation_test` | `1231/1231 passed` | EXIT:0 | Cobre pré-walk, arm transacional, cooldown, exaustão, métricas estritas |
| `cmake --build build --target TooieRecompiled` | `[35/35] Linking TooieRecompiled` | EXIT:0 | Executável principal relincado com RT64 atualizado |
| `./build/window_display_test` | `38/38 assertions passed` | EXIT:0 | Regressão de janela aprovada (binário existente) |
| `./build/tintro_projection_policy_test` | `7/7 assertions passed` | EXIT:0 | Política de projeção aprovada (binário existente) |
| `git diff --check` (TooieRecomp) | Sem avisos de whitespace | TOOIE_EXIT:0 | Worktree limpo de resíduos |
| `git diff --check` (RT64) | Sem avisos de whitespace | RT64_EXIT:0 | Worktree limpo de resíduos |
| `graft build` | 1426 nós, 1298 arestas | GRAFT_EXIT:0 | Grafo de contexto atualizado |

## 7. Histórico e arbitragem da revisão independente

1. **Revisão inicial Haiku** (`haiku-review-output.json`):
   - `FAIL` de aceitação por ausência da métrica estrita considerando ambíguos no denominador (`PATH_PAIR_FULL_OF_ALL_OBSERVED`), ausência de flags explícitas de prontidão/truncamento e consumo de orçamento pré-cena sem reset.
2. **Revisão intermediária de correções Haiku** (`haiku-fix-review-output.json`):
   - `FAIL` remanescente na aceitação 4: o cooldown de relógio de parede ainda executava a varredura cara de chamadas (`p09CorrGate`) antes do descarte; consumo do marcador sem verificação do retorno de `::remove`.
3. **Correções finais pelo Grok** (`grok-final-review-fixes-output.json` e `implementation-report.txt`):
   - Criação do helper puro `p09CorrelationPreWalk` antes da chamada ao gate.
   - Consumo transacional com checagem estrita de `::remove(...) == 0` e latch de falha.
   - Remoção de helpers mortos de reset de teste (`p09CorrelationBudgetReset`, `p09ExactUseCorrelationReset`).
   - Cabeçalho explicativo com pré-requisitos no patch versionado.
4. **Revisão final Haiku** (`haiku-final-review-output.json`):
   - `PASS` em todos os itens remanescentes, restrito a evidências estáticas, helpers e testes de host.
5. **Arbitragem final do Root**:
   - **Nota sobre lista de hashes**: a observação LOW da revisão Haiku de que `final-review-fixes-source-hashes.txt` omitia arquivos RT64 foi verificada diretamente pelo root como `CORRECTED`; o arquivo contém todos os 7 caminhos com hashes idênticos.
   - **Linha de exit do build-test**: a observação sobre ausência de linha explícita no log foi verificada pelo root com leitura derivada do log indicando `EXIT:0`, corroborada pela execução do teste com `1231/1231 passed` e `EXIT:0`.
   - **Ramo defensivo de cooldown em Decide**: a presença redundante da checagem em `p09CorrelationDecide` é inofensiva e defensiva para chamadores diretos de teste; classificada como não bloqueante sem necessidade de refatoração.

## 8. Pré-requisitos e hashes do patch e código-fonte

### 8.1 Patch versionado
- **Arquivo**: `patches/rt64/p09-exact-use-pathology-correlation.patch`
- **SHA256**: `5108ad3f82423dbc561c5ee2b13cfbf1f88d083140a628c07ed41c7f37d828d0`
- **Linha de base**: aplica-se sobre a linha de base dirty intencional sobre RT64 HEAD `6f1c2d99a4ea571c139f449c326fd176ba8f3496`, e **não** sobre HEAD limpo (dry-run em HEAD limpo resulta no RC:1 negativo esperado com 11 de 12 hunks falhando).
- **Prova de aplicação/reversão**: teste em cópia temporária de `/tmp/p09-correlation-implementation/baseline-files` executou aplicação (RC:0) e reversão (RC:0), restaurando exatamente os hashes da linha de base com o novo header ausente.

### 8.2 Hashes dos arquivos-fonte (compilados e validados)
- `rt64/src/render/rt64_rsp_processor.cpp`: `d406915fedb247be6c3c8a6b1f692bc093da6fc90f0340f5d30b5ee08e544117`
- `rt64/src/render/rt64_transform_processor.cpp`: `bff566414a369ab3fe98b8d6d29c400f4b41e7105a0fce3ab9e7e2ab473b3d53`
- `rt64/src/render/rt64_transform_processor.h`: `14e1cbf57bdaa39e91bfa974d2db34c08ddf54abfdd632ec575266260cda4a2c`
- `rt64/src/render/rt64_p09_exact_use_correlation.h`: `388ff4ff7706681db276d2f990b8a60541667173287799f16f52c2beea061ae2`
- `TooieRecomp/tests/p09_exact_use_correlation_test.cpp`: `b58be8bed9c5b8016a02a470e6f758f186072d4bbe4573d87468fa3b7824acf0`
- `TooieRecomp/CMakeLists.txt`: `73cc7b49df0a6fdd36263aa47706875527fd2a678c810859f1da57683dc50d3d`

## 9. Avaliação de atualizações em specs e engenharia reversa

Avaliação concluída:
- A semântica original do jogo, as matrizes e a convenção row-vector (`mul(pos4, mul(WORLD, mul(VIEW, PROJECTION)))`) permanecem inalteradas.
- O código implementado restringe-se a instrumentação diagnóstica em CPU e testes de host.
- Não há novos fatos estáveis de engenharia reversa ou contratos permanentes de engine a serem promovidos para `docs/reverse-engineering/` ou specs definitivas neste momento.
- A evidência permanece integralmente contida nos documentos de evidência e no diário operacional.

## 10. Fronteira de runtime e primeira observação de captura

- **Status anterior (histórico)**: cobertura real de patologia, captura na cena de personagens, prova visual e tempos reais de CPU permaneciam integralmente `NOT_CAPTURED`.
- **Primeira captura runtime (`capture-empty-20261015.txt`)**:
  - Arquivo de captura gerado em `/tmp/p09-correlation-implementation/capture-empty-20261015.txt`.
  - Contadores de ciclo de vida e controle: `ARM_COUNT=1`, `REARM_COUNT=0`, `MARKER_CONSUME_FAILURES=0`, `MARKER_CONSUME_LATCHED=NO`.
  - Contadores de triagem: `ELIGIBILITY_WALKS=181`, `ELIGIBLE_DISPATCHES=0`, `ATTEMPTS=0`, `PATH_PAIR_TOTAL=0` (pares=0).
  - Triagem de ineligibilidade (ordem de primeira falha): `SKIPPED_INELIGIBLE_NO_WORKLOAD=61`, `SKIPPED_INELIGIBLE_NOT_INTERMEDIATE=59`, `SKIPPED_INELIGIBLE_NO_TARGET_CALL=61`. Contadores em zero: `SKIPPED_INELIGIBLE_NO_GEOMETRY=0`, `SKIPPED_INELIGIBLE_NO_FRAME_MATCH=0`, `SKIPPED_INELIGIBLE_NO_MODEL_BIN=0`.
  - Contexto de execução: tempo de captura estimado pelo usuário em ~30 segundos; cena em que ocorreu a captura: `UNKNOWN`.
  - Classificação técnica: `CAPTURED_EMPTY` / `NO_ELIGIBLE_SAMPLES`. **NÃO** constitui evidência de cobertura zero, ausência de patologias nem de correção. Amostra patológica significativa, confirmação visual e tempos reais de CPU continuam `NOT_CAPTURED`.
- **Revisão somente-leitura da cadeia de portão por Claude Haiku (`haiku-empty-capture-output.json`)**:
  - Rastreou estaticamente a cadeia de elegibilidade no RT64 (`rt64_p09_exact_use_correlation.h:421-431` e `rt64_rsp_processor.cpp`), confirmando a ordem sequencial de primeira falha: `NO_WORKLOAD` -> `NO_GEOMETRY` -> `NO_FRAME_MATCH` -> `NOT_INTERMEDIATE` -> `NO_MODEL_BIN` -> `NO_TARGET_CALL`.
  - Identificou os alvos monitorados em `p09V4VisibleTargetIndex` (14 alvos V7 + 8 lógicos adicionais das DLs de penas de 0x62B).
  - **Limitação de inferência**: a inferência de que movimento de gameplay é estritamente obrigatório ou de que o `model_bin` está resolvido globalmente **NÃO** é promovida como comprovada (`PROVEN`). Os contadores registram apenas a primeira falha; `NO_MODEL_BIN=0` ocorreu estritamente nos 61 dispatches que alcançaram esse estágio da checagem. A dependência de `curFrameWeight` quanto a interpolação ou movimento permanece `INFERENCE` não verificada.
- **Relançamento pelo Root e estado pré-captura (histórico)**:
  - O root relançou o executável com a flag dedicada (`TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION=1 ./build/TooieRecompiled`).
  - Confirmado via arquivo de status: `STATE=WAITING_FOR_REQUEST`, zero walks (`ELIGIBILITY_WALKS=0`). O usuário confirmou estar posicionado em gameplay e o root realizou a armação via marcador.
- **Segunda captura runtime em gameplay (`capture-gameplay-20261015.txt`)**:
  - Arquivo de captura gerado em `/tmp/p09-correlation-implementation/capture-gameplay-20261015.txt`.
  - Hash SHA256: `b6be018a3619a78a7c05ba7d0b8fa6c64e27d22e5cde6bedbebeebdf771203e7`.
  - Proveniência completa: captura vazia inicial preservada como histórico; jogo deixado em execução em estado `EXHAUSTED` (sem novas buscas sem novo rearm).
  - Estado e orçamento: `STATE=EXHAUSTED` após esgotar o limite máximo de 24 tentativas (`ATTEMPTS=24`).
  - Triagem de elegibilidade: `ELIGIBILITY_WALKS=2135`, `ELIGIBLE_DISPATCHES=93`.
  - Execução de buscas e planos: 4 tentativas com par patológico (`ATTEMPTS_WITH_PATH_PAIR=4`, 20 sem par patológico), 4 buscas de costura executadas (`SEAM_SEARCHES=4`), 4 planos locais construídos com sucesso (`PLAN_BUILT=4`).
  - Integridade da amostragem: sem falhas de plano, saturação ou truncamento (`PLAN_INACTIVE=0`, `PLAN_NOT_EXHAUSTIVE=0`, `PAIRS_DROPPED_PENDING_CAP=0`, `VERTS_DROPPED_BONE_CAP=0`, `UNIQUE_TABLE_OVERFLOW=0`, `RECORDED_COHORT_TRUNCATED=NO`).
  - Contadores de correlação patológica:
    - `PATH_PAIR_TOTAL=194`, `PATH_PAIR_FULLY_COVERED=0`, `PATH_PAIR_ONE_SIDED=0`, `PATH_PAIR_NOT_COVERED=0`.
    - `PATH_PAIR_AMBIGUOUS=194`, `PATH_PAIR_AMBIGUOUS_TIE=194`. Todos os 194 pares patológicos observados foram classificados como ambíguos devido a empate (`TIE`).
    - Ocorrências cumulativas: tratam-se de 194 ocorrências cumulativas de pares patológicos ao longo dos 4 planos (`PATH_OCCURRENCE_TOTAL=388`), e **não** de 194 arestas certificadas únicas.
    - Chaves exatas únicas: `PATH_UNIQUE_KEYS=214` (1 selecionada / `PATH_UNIQUE_KEYS_SELECTED=1`, 213 não selecionadas / `PATH_UNIQUE_KEYS_NOT_SELECTED=213`).
  - Universo não patológico: `NONPATH_PAIR_TOTAL=454`, `NONPATH_OCCURRENCE_TOTAL=908` (54 selecionadas, 854 não selecionadas), `NONPATH_UNIQUE_KEYS=481` (4 selecionadas, 477 não selecionadas). `UNIQUE_KEYS_ON_BOTH_PATH_AND_NONPATH=25`.
  - Tempos de CPU medidos (telemetria do diagnóstico em runtime, **não** FPS do jogo nem prova visual):
    - Tentativas pesadas: pico de 202.762 ms (`CPU_ATTEMPT_MS_PEAK=202.762`), total de 590.915 ms (`CPU_ATTEMPT_MS_TOTAL=590.915`).
    - Plano de costura: pico de 200.776 ms (`CPU_SEAM_PLAN_MS_PEAK=200.776`), total de 551.464 ms (`CPU_SEAM_PLAN_MS_TOTAL=551.464`).
    - Triagem de portão: total de 10.382 ms (`CPU_GATE_MS_TOTAL=10.382`).
  - Avaliação de cobertura: dados de runtime significativos foram capturados (`MEANINGFUL_RUNTIME_DATA_CAPTURED`), porém a taxa de cobertura é indecidível no momento (`PATH_PAIR_FULL_OF_ALL_OBSERVED=0.000000`, taxa definitiva `NA`) devido à saturação de empates (`TIE=194`). Nenhuma correção de produção implementada.
- **Revisão da causa do empate por Claude Haiku (`haiku-tie-review-output.json`)**:
  - Mecanismo do código comprovado (`PROVEN` no fonte): a busca de junção mais próxima (`p09EmitNearestJoinPairs`, `rt64_rsp_processor.cpp:2115-2131`) avalia a soma de distâncias euclidianas em NDC `p09NdcDistXY(child.prev, parent.prev) + p09NdcDistXY(child.cur, parent.cur)`.
  - Uma melhora estrita limpa `bestTie = false` (:2126); a ocorrência de igualdade exata float `gap == bestGap` (:2129, sem epsilon) ativa `bestTie = true`.
  - No classificador (`rt64_p09_exact_use_correlation.h:167`), a checagem `if (tie) return ambiguous(Tie);` executa com precedência sobre a verificação de completude e pertinência das chaves, forçando a classificação como ambíguo.
  - **Limites da análise**: a hipótese de pais duplicados ou vértices coincidentes na lista permanece `UNKNOWN`. **NÃO** aceitar a afirmação de que coordenadas idênticas tornam automaticamente a ambiguidade "falsa", pois diferentes chaves de origem (`sourcekeys`) e diferentes matrizes/políticas de `WORLD` continuam relevantes para a identidade de renderização. Nenhuma supressão automática de empate ou dedup de produção está autorizada.
- **Compromisso**: nenhum commit ou push nesta etapa (`NO_COMMIT=YES`, `NO_PUSH=YES`).
- **Próxima ação única (`NEXT_SINGLE_ACTION` histórica, concluída)**:
  ```text
  One bounded diagnostic discriminator logging first ~20 tie groups, exact full sourcekeys/originalWORLD and prev/cur positions, gap/multiplicity and candidate memberships to distinguish repeated same-use from different legitimate-use; then review, before alias/interpolation edits (currently NOT_IMPLEMENTED; task stops at newly observed blocker per scope rule).
  ```
- **Atualização subsequente (2026-10-16)**: discriminador dos primeiros 20 grupos de empate executado e analisado em [[p09-pathological-tie-groups]]. Ação seguinte atualizada para auditoria focada e somente-leitura de proveniência de contorno pareado.
