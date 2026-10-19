---
title: "P09 — Revisão da captura V2 de arestas pareadas e arbitragem de contadores"
description: "Revisão dos 12 registros de detalhe do snapshot V2, arbitragem dos contadores via código-fonte do RT64 e decisão operacional CASE2 para correlação de patologia."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-10-15
tags: [p09, dry-run, v2, paired-edges, review, arbitration, case2, rt64]
---

# P09 — Revisão da captura V2 de arestas pareadas e arbitragem de contadores

## 1. Proveniência e fontes de evidência

Revisão técnica baseada no snapshot preservado da captura V2 e nos relatórios de auditoria independente:

- **Snapshot de captura analisado:** `/tmp/tooie-p09-seam-edge-dry-run-v2-20261015-030500.txt`
  - SHA256: `256d5fdd935c7d105810d885f4bbdb4047453606cdc5544bc56ef1e81bbbe6e5` (302 linhas, 21.663 bytes).
  - Versão: `P09_SEAM_DRY_RUN_VERSION=2_PAIRED_EDGE_PROVENANCE`.
  - Workload mais recente: `LAST_WORKLOAD_ID=1267`, `LAST_CALLS_SCANNED=598`.
- **Relatórios de revisão independente:**
  - Grok foreground (`grok-4.7`, medium 200k, exit 0, `stopReason: end_turn`): `/tmp/p09-grok-edge-review/report.txt` e `/tmp/p09-grok-edge-review/grok-output.json`. Decisão recomendada: `CASE2`.
  - Claude Haiku revisão inicial (`claude-haiku-5-5`, medium, exit 0): `/tmp/p09-haiku-edge-review.json`. Proposta inicial: `CASE3` por suspeita em contadores de semântica desconhecida.
  - Claude Haiku arbitragem de contadores (`claude-haiku-5-5`, medium, exit 0, `subtype: success`): `/tmp/p09-haiku-counter-semantics-retry.json`. Inspeção direta no repositório `rt64/rt64` (`rt64_transform_processor.cpp`, HEAD `6f1c2d99`, worktree com alterações locais).

## 2. Fatos do snapshot e inspeção dos 12 detalhes certificados

### 2.1 Accounting e exaustividade do último workload
- Busca exaustiva aceita para o último workload: `LATEST_WORKLOAD_COUNTS_EXHAUSTIVE=YES`, `ALIAS_PLAN_EXHAUSTIVE=YES`, `EMERGENCY_LIMIT_HIT=NO`, todos os descartes de segurança `dropped=0`.
- Total de pares certificados: **190** (`SAME_CALL_CERTIFIED=55`, `CROSS_CALL_CERTIFIED=135`).
- Limite de detalhe impresso: `CAP_DETAIL_OUTPUT_ATTEMPTED=190`, `ACCEPTED=12`, `DROPPED=178`, `AFFECTS_EXHAUSTIVENESS=NO`.
- População inspecionada em detalhes: **12 pares, todos same-call (`cross=NO`)**.
- Detalhes cross-call inspecionados: **0 de 135**. Os 12 impressos são apenas o prefixo da saída bounded.

### 2.2 Tabela dos 12 registros de detalhe certificados

| idx | gc A/B | tri A/B | eIF A/B | corners A | corners B | faceCorners A | faceCorners B | verts A | verts B | WORLD A/B | Orient | PREV / CUR res | Margens |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | 0/0 | 6/9 | 2/0 | 2-0 | 0-1 | 6503,6501 | 6510,6511 | 3656,3655 | 3666,3667 | 12/14 | REVERSED | 0.0 / 0.0 | PASS (12.25/12.46 > 0.00227) |
| 1 | 3/3 | 0/7 | 2/2 | 2-0 | 2-0 | 6671,6669 | 6692,6690 | 3730,3731 | 3736,3735 | 17/19 | REVERSED | 0.0 / 0.0 | PASS (12.53/12.53 > 0.00139) |
| 2 | 3/3 | 1/6 | 0/2 | 0-1 | 2-0 | 6672,6673 | 6689,6687 | 3727,3728 | 3734,3732 | 16/18 | REVERSED | 0.0 / 0.0 | PASS (22.23/22.23 > 0.00212) |
| 3 | 3/3 | 2/10 | 1/1 | 1-2 | 1-2 | 6676,6677 | 6700,6701 | 3731,3729 | 3737,3736 | 17/19 | REVERSED | 0.0 / 0.0 | PASS (13.00/13.00 > 0.00134) |
| 4 | 3/3 | 3/6 | 1/1 | 1-2 | 1-2 | 6679,6680 | 6688,6689 | 3728,3726 | 3733,3734 | 16/18 | REVERSED | 0.0 / 0.0 | PASS (20.15/20.15 > 0.00239) |
| 5 | 3/3 | 4/6 | 2/0 | 2-0 | 0-1 | 6683,6681 | 6687,6688 | 3726,3727 | 3732,3733 | 16/18 | REVERSED | 0.0 / 0.0 | PASS (18.53/18.53 > 0.00240) |
| 6 | 3/3 | 5/12 | 1/1 | 1-2 | 1-2 | 6685,6686 | 6706,6707 | 3729,3730 | 3735,3737 | 17/19 | REVERSED | 0.0 / 0.0 | PASS (12.53/12.53 > 0.00139) |
| 7 | 3/3 | 20/27 | 2/2 | 2-0 | 2-0 | 6731,6729 | 6752,6750 | 3752,3753 | 3758,3757 | 23/25 | REVERSED | 0.0 / 0.0 | PASS (12.53/12.53 > 0.00140) |
| 8 | 3/3 | 21/26 | 1/2 | 1-2 | 2-0 | 6733,6734 | 6749,6747 | 3749,3750 | 3756,3754 | 22/24 | REVERSED | 0.0 / 0.0 | PASS (22.32/22.32 > 0.00213) |
| 9 | 3/3 | 22/30 | 0/0 | 0-1 | 0-1 | 6735,6736 | 6759,6760 | 3751,3752 | 3757,3759 | 23/25 | REVERSED | 0.0 / 0.0 | PASS (13.00/13.00 > 0.00135) |
| 10 | 3/3 | 23/26 | 0/0 | 0-1 | 0-1 | 6738,6739 | 6747,6748 | 3748,3749 | 3754,3755 | 22/24 | REVERSED | 0.0 / 0.0 | PASS (20.15/20.15 > 0.00239) |
| 11 | 3/3 | 24/26 | 2/1 | 2-0 | 1-2 | 6743,6741 | 6748,6749 | 3750,3748 | 3755,3756 | 22/24 | REVERSED | 0.0 / 0.0 | PASS (20.15/20.15 > 0.00240) |

Todos os 12 pares possuem `fb=2` e `proj=1`.

### 2.3 Coerência interna verificada
- **Vértices e mundos disjuntos:** 12/12 pares possuem vértices armazenados disjuntos entre A e B (`verts A ∩ verts B = ∅`). Em cada endpoint, o mundo original difere entre A e B (`distinctWorlds=YES`). Relações `(worldA, worldB)` observadas: `(12,14)` [call 0], `(16,18)`, `(17,19)`, `(22,24)`, `(23,25)` [call 3].
- **Resíduos nulos:** 48/48 endpoints (2 endpoints × 2 frames PREV/CUR × 12 pares) reportam resíduo exatamente `0` (token literal `0`), estritamente `<= tolerance`.
- **Margens de competidor:** 24/24 checagens de margem passam (`nextBestA - best > marginNeed` e `nextBestB - best > marginNeed`). Menor folga observada ~12.24 vs `marginNeed` ~0.0024 (folga superior a 5000×).
- **Consistência topológica:** cantos `corner0/1` coincidem com a ordem canônica de `edgeInFace` (0: 0-1; 1: 1-2; 2: 2-0). Vértices formam ciclos e cadeias fechadas coerentes com malhas adjacentes (ex.: triângulos 1, 3, 4 compartilhando arestas da face do triângulo 6).

### 2.4 Limites estritos da amostra
- **Não generalização:** a coerência interna dos 12 registros de detalhe não pode ser generalizada para os 190 certificados nem para os 135 pares cross-call não impressos.
- **Identidade de concorrentes ausente:** os IDs dos competidores e flags de mutual-best por par não são impressos individualmente; não é possível auditar a seleção de concorrentes de forma independente apenas pelo texto.
- **Geometria primária ausente:** o snapshot não inclui posições locais nem linhas da matriz WORLD; o cálculo `dist(mul(localPos4, WORLD))` não pode ser recomputado geometricamente a partir do texto.

## 3. Arbitragem dos contadores e resolução da suspeita CASE3

A proposta inicial de `CASE3` pelo Claude Haiku fundamentava-se na aparente inconsistência aritmética de três contadores cujo significado era desconhecido no dump. A inspeção direta do código-fonte em `BanjoRecomp/lib/rt64/src/render/rt64_transform_processor.cpp` (HEAD `6f1c2d99`, worktree com alterações locais) resolveu a semântica:

### 3.1 Funil cross-call: `tolerancePass 151` vs `uniquePartner 143` / `certified 135` / `ambiguous 16`
- **Código-fonte:**
  - `sameUniquePartner` / `crossUniquePartner` (cpp:2450-2462): incrementado quando há reciprocidade mútua (`mutual`), antes do teste de margem.
  - `sameCertified` / `crossCertified` (cpp:2464-2468): incrementado quando `complete && marginOk`.
  - `sameAmbiguous` / `crossAmbiguous` (cpp:2471): incrementado para qualquer candidato com passe de tolerância que não atinja certificação completa com margem.
- **Arbitragem:** `135 certified + 16 ambiguous = 151 tolerancePass`. Dos 143 com parceiro mútuo, 135 foram certificados e 8 não foram certificados (`143 - 135 = 8`). Dos 151 com passe de tolerância, 8 não tinham parceiro mútuo (`151 - 143 = 8`). A soma `8 + 8 = 16 ambiguous`. As contagens fecham sem contradição aritmética. Isso é compatibilidade contábil e não prova direta de que todos os caminhos de tolerância foram inspecionados individualmente.

### 3.2 Hash de classificação: `attempted 8751`, `accepted 6688`, `dropped 0`
- **Código-fonte:** `classInsert` (cpp:1812-1826): `classAttempted` é incrementado em toda inserção de ocorrência de aresta; `classAccepted` somente quando uma chave nova é criada no hash; `classDropped` somente se o tamanho exceder `kSafetyMaxEdges`.
- **Arbitragem:** A diferença `8751 - 6688 = 2063` representa inserções subsequentes em chaves existentes (aresta já vista no hash). `dropped=0` confirma que o limite de segurança nunca foi atingido.

### 3.3 Arestas boundary: `4627 total` vs `4609 cross-call`
- **Código-fonte:** `crossBoundary` (cpp:1958-1976): soma `boundaryFound` somente para grupos de projeção (`fbPair` + `proj`) que possuam `>= 2` chamadas (`gameCalls`) com arestas de contorno.
- **Arbitragem:** A diferença de 18 arestas corresponde a chamadas de projeção isoladas (sem parceiro potencial na mesma projeção), excluídas da busca cross-call por critério de elegibilidade estrutural, e não a 18 arestas descartadas por erro ou limite de capacidade.

### 3.4 Conclusão da arbitragem
A suspeita inicial de quebra de contadores (`CASE3`) foi **CORRECTED/SUPERSEDED** pela evidência primária do código. O registro histórico da alegação inicial e sua demonstração permanecem preservados. A ausência de detalhes cross-call no log impresso é um **limite de amostragem**, não uma falha estrutural do algoritmo de busca.

## 4. Decisão de arquitetura e próximo passo operacional

- **Decisão adotada:** **`CASE2` operacional**. O accounting exaustivo do último workload e a coerência interna estrita dos 12 registros de detalhe são suficientes para prosseguir para o próximo portão exclusivamente: **correlação de patologia por uso exato (CPU, default-off)**.
- **Ressalvas de governança:** Esta decisão **NÃO** constitui aprovação de identidade de junção de produção, nem autorização para materializar aliases ou clones em shaders/runtime. Os 135 pares cross-call permanecem não inspecionados em detalhe, e as exclusões de concorrentes não puderam ser auditadas de forma independente.
- **Status do Gate 3:** `PASS` escopado exclusivamente aos 12 registros same-call e ao accounting do último workload.
- **Próxima ação técnica única (`NEXT_SINGLE_ACTION`):** Implementar **SOMENTE** a correlação de patologia por uso exato (CPU, default-off). Ambos os usos patológicos conhecidos (testemunhas) devem ser checados contra a chave exata completa: `(workloadId, fbPair, projection, gameCall, triangleLocal, corner, faceCorner, storedVertex, originalWorld)`. Atalhos baseados apenas no slot WORLD original são proibidos. O código deve reportar o total patológico, cobertos, não cobertos, ambíguos e não patológicos selecionados, sem implementar aliases. O design deve ser de baixo custo e evitar repetir buscas exaustivas contínuas a cada frame; nenhum patch de desempenho deve ser aplicado neste momento. Correlação permanece `NOT_IMPLEMENTED / NOT_CAPTURED` no estado real.
