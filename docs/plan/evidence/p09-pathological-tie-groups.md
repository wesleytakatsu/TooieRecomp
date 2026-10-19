---
title: "P09 — Grupos de empate patológicos e arbitragem de soluções"
description: "Captura dos primeiros 20 grupos de empate, análise estrutural, arbitragem multi-agente e auditoria somente-leitura."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-10-16
tags: [p09, interpolation, diagnostic, ties, arbitration, sonnet, haiku, grok, rt64]
---

# P09 — Grupos de empate patológicos e arbitragem de soluções

## 1. Objetivo delimitado e roteamento de modelos

- **Objetivo**: registrar e analisar os primeiros 20 grupos de empate (`TIE`) na busca de junção mais próxima do diagnóstico de uso exato (`TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION`), identificando chaves de 9 campos, multiplicidade, coordenadas e pertinência ao plano local certificado, sem alterar renderização nem implementar correções de produção (`PRODUCTION_FIX_NOT_IMPLEMENTED: YES`).
- **Roteamento de modelos**:
  - **Grok 4.7 medium** (não fast, `context_window=200000` via CLI `grok`): implementação do discriminador de empates no RT64 (`rt64_rsp_processor.cpp`, `rt64_p09_exact_use_correlation.h`), testes de host (`p09_exact_use_correlation_test.cpp`), e prova de aplicação/reversão do patch.
  - **Claude Haiku 5.5 medium** (via Claude Code CLI): revisão estática independente de código/patch e análise detalhada dos artefatos de captura runtime.
  - **Claude Sonnet 5.5 medium**: escalonamento explícito pelo usuário após evidência demonstrar ausência de solução estabelecida; executou análise inicial de soluções e subsequente arbitragem revisada retirando alegações preliminares.
  - **Gemini 3.8 Flash high** (via Antigravity CLI `agy`): consolidação documental integral e sincronização de handoffs.
  - **Root**: armação da captura via marcador, execução das CLIs e arbitragem final de encerramento no bloqueador de evidência.

## 2. Gates, binários e patch versionado

- **Host test**: `./build/p09_exact_use_correlation_test` exit 0 (`1354/1354 passed`, log `/tmp/p09-tie-discriminator/run-p09-exact-use-correlation-test.log`).
- **Build normal**: `cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test` exit 0 (`[35/35] Linking CXX executable TooieRecompiled`, log `/tmp/p09-tie-discriminator/build-gate.log`).
- **Binários**: `TooieRecompiled` SHA256 `84d31479e6919d999b2e6d49d972d7fd49c06f4faf56807f2850b27a1df86426` (recompilado); `window_display_test` 38/38 exit 0; `tintro_projection_policy_test` 7/7 exit 0.
- **Diff checks**: `git -C TooieRecomp diff --check` exit 0; `git -C BanjoRecomp/lib/rt64 diff --check` exit 0.
- **Git HEADs**: Tooie `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf`; RT64 `6f1c2d99a4ea571c139f449c326fd176ba8f3496` (baseline dirty intencional documentada).
- **Patch versionado**: `TooieRecomp/patches/rt64/p09-exact-use-pathology-correlation.patch` SHA256 `7b93258cecadcb0b18b1f4a22f0a278c70a1cc20f3e1f0ed8fbb112b0bd7f38f` (86535 bytes). Prova apply/reverse em diretório isolado exit 0.
- **Hashes de fontes**: `rt64_rsp_processor.cpp` (`466d2fb8...`, modificado), `rt64_p09_exact_use_correlation.h` (`793dc75e...`, modificado), `p09_exact_use_correlation_test.cpp` (`9d93384a...`, modificado); `rt64_transform_processor.cpp` (`bff56641...`), `rt64_transform_processor.h` (`14e1cbf5...`) e `CMakeLists.txt` (`73cc7b49...`) inalterados nesta rodada.

## 3. Amostra de captura em gameplay e limites

- **Artefatos imutáveis de runtime**:
  - Correlação: `/tmp/p09-tie-discriminator/capture-gameplay/tooie-p09-seam-edge-pathology-correlation.txt` (SHA256 `74b0b2e5f219522c42b041a6a5cea9005bea8f57ddd0fc595df8f69024528c7d`).
  - Grupos de empate: `/tmp/p09-tie-discriminator/capture-gameplay/tooie-p09-seam-edge-pathology-ties.txt` (SHA256 `5b1fc21a2af77dbab9abc42919465e45a65a13014321cee405468cc6fd59b2a8`).
- **Execução e armação**: flag existente `TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION=1`, armação transacional pelo root via `/tmp/tooie-p09-seam-edge-pathology-correlation.arm` após posicionamento pelo usuário.
- **Contadores de execução**: processo atingiu `EXHAUSTED` em 24 tentativas, 6 planos, 162 ocorrências de pares patológicos (`PATH_PAIR_TOTAL=162`, `PATH_OCCURRENCE_TOTAL=324`), todas classificadas como `TIE`, correspondendo a 179 chaves únicas (`PATH_UNIQUE_KEYS=179`). Não há contradição com os 194 da captura anterior (sessões distintas).
- **Amostragem dos 20 grupos**: 20 grupos armazenados (`kP09TieGroupCap=20`), 142 grupos omitidos (`skippedGroups=142`), truncamento de membros zero (`MEMBER_TRUNCATION_EVENTS=0`). Sem supressão de empates (`TIE_SUPPRESSION=NO`).
- **Estrutura dos 20 grupos**:
  - Multiplicidade: 12 grupos com mult=2 (24 membros) e 8 grupos com mult=4 (32 membros), totalizando 56 ocorrências de cantos de face pais (não 56 vértices únicos).
  - Todas as 9 chaves completas diferem dentro de cada grupo.
  - Exatamente 2 `storedVertex` distintos por grupo (não 40 únicos globais).
  - Mesmo `WORLD` de pai dentro de cada grupo; coordenadas impressas `PREV/CUR_NDC` e `PREV/CUR_WORLD` idênticas entre candidatos a pai (não extrapolar identidade binária exata a partir de texto).
  - Os 8 grupos de mult=4 repetem o uso de cantos de face sobre os dois vértices armazenados (16 reusos adicionais).
  - Todos os 20 vértices filhos e 56 membros foram consultados contra o plano local ativo e exaustivo (com workload/geração compatíveis) e retornaram `MEMBERSHIP=NOT_SELECTED` (`REASON=NONE`).
- **Performance de CPU**: pico de tentativa 188.402 ms, pico de plano 186.753 ms (custo de diagnóstico, sem medição de FPS de gameplay nem prova visual). Jogo encerrou no limite, sem novas buscas.

## 4. Revisões independentes e correções conceituais

- **Revisão de código por Haiku**: aprovou a lógica de amostragem (`kP09TieGroupCap=20`, `kP09TieMemberCap=96`), honestidade de identidade (classifica `UNKNOWN` se truncado/incompleto, nunca assume identidade por igualdade de coordenadas), escopo local de plano e design fail-closed. Notou ausência de checagem do retorno de `fclose` antes de `rename` (não bloqueante; `fflush`+`ferror` cobrem erros de escrita) e premissa de serialização de gerações.
- **Correção da análise de artefatos por Haiku**:
  - `selected=YES` indica apenas o candidato mais próximo escolhido pela busca (desempate por índice 0).
  - `MEMBERSHIP=NOT_SELECTED` refere-se à pertinência no plano certificado de costura V2; são campos independentes e sua coexistência não é contradição.
  - `REASON=NONE` em `NOT_SELECTED` significa consulta válida sem falha desconhecida, **NÃO** prova de que a costura é legítima nem falha do classificador.
  - Chaves exatas válidas fora do plano certificado nesta amostra: `PROVEN`; cobertura global de omissões ou correções: `UNKNOWN`.
  - Empates permanecem ambíguos; proibida a supressão por coordenadas compartilhadas.

## 5. Propostas do Sonnet e arbitragem final do Root

- **Propostas preliminares do Sonnet**:
  - P1: Seletor de slots `WORLD` por aresta certificada `(call, world)`.
  - P2: Alias local por chamada e clone condicional de vértices (B+C).
  - P3: Snap para endpoint corrente nos vértices de costura.
- **Arbitragem final do Root (supersede de recomendações preliminares)**:
  - **Retirada formal pelo Sonnet**: o Sonnet retirou expressamente a certificação pontual de P1 (k-pontos, reciprocidade, tolerância `0.08`), a alegação de "omissão global do plano", e a persistência de IDs brutos de `WORLD`/chamada entre workloads sem mapeamento por GameFrame.
  - A certificação pontual por 2 endpoints continua insuficiente (`CASE_B` em `p09-seam-alias-dry-run-validity-review.md`).
  - Todas as 3 opções permanecem como **`HYPOTHESIS`** não implementadas e não aprovadas. P1 arrisca deformar todos os usuários rígidos dos slots selecionados; P3 causa judder temporal; P2 continua condicional com pendências de headroom e ganchos de ciclo de vida antes de `State::fullSync`.
  - **Limitações da proposta revisada de laços de contorno**: igualdade de cardinalidade e ordem cíclica de contorno são testes estruturais heurísticos (`HYPOTHESIS`), e a ausência de aresta pareada não refuta a causa conhecida de `WORLD` composto. Malhas com diferentes triangulações ou vértices duplicados podem falsear a adjacência bruta.
  - Não é possível cruzar IDs brutos da captura global V2 antiga com a captura local nova sem proveniência exata de workload.
  - Ausência do conjunto certificado atual não refuta a causa conhecida nem comprova ausência de costura.

## 6. Próxima ação única e fronteira de implementação

- `NEXT_SINGLE_ACTION`: auditoria focada e somente-leitura dos dados existentes de proveniência e arestas de contorno pareadas em relação aos usos patológicos amostrados, iniciando pela semântica de produtor/topologia e pelo conteúdo real dos artefatos atuais; sem implementação automática de novos lerps, aliases ou telemetria. O root interrompe a execução no bloqueador de evidências identificado após a análise de soluções, sem reivindicar correção de produção.
- `PRODUCTION_FIX_NOT_IMPLEMENTED: YES`.
- `P09_FORMAL_PHASE: PENDING / NOT_FORMALLY_OPEN`.
- `NO_COMMIT: YES`, `NO_PUSH: YES`.
