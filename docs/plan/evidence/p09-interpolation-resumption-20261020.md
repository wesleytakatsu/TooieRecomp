---
title: "Retomada da investigação de interpolação P09 (2026-10-20)"
description: "Revisão causal/estática, hipóteses de regressão e próximo diagnóstico limitado."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-10-20
tags: [interpolation, p09, rt64, hlsl, diagnostics]
---

# Retomada da investigação de interpolação

## CURRENT — `WAITING_USER_SCENE`, gates host aprovados

Ver [[p09-visual-test-mode-20261020]]. Build e revisão independente final do snapshot passaram no **escopo host**: unidade 123/123, produtor/leitor separados, 13 negativos semânticos com FNV válido rejeitados, Sonnet `NO_BLOCKERS`. Isso substitui os blockers/97 casos históricos abaixo; **não constitui validação de captura runtime**. Novo jogo isolado PID inicial `1188686` aberto, configurado 60 + novo snapshot sem heavy; FPS efetivos `UNKNOWN`. Marcador inexistente; root aguarda usuário avisar cena Demo 1/gameplay lateral pronta. **`NEXT_SINGLE_ACTION`: aguardar esse aviso, depois armar uma vez; não usar cronômetro.** Captura real `NOT_EXECUTED`, identidades anatômicas `UNKNOWN`, correção permanente `NOT_IMPLEMENTED`; caminho normal CURRENT/30 preservado; sem commit/push ou alteração de gates formais das fases. Estados abaixo permanecem históricos.

## CURRENT — reprodução visual confirmada, snapshot ainda não aceito

Ver [[p09-visual-test-mode-20261020]]. Usuário: **“Os glitches reapareceram”**, `USER_OBSERVED / USER_CONFIRMED_FOR_TESTED_RUN`, no modo configurado opt-in 60, CURRENT desligado e sem coletores pesados. Depois: **“Jogo fechado; não conferi contador”**. FPS efetivos `UNKNOWN`; não atribuir esse relato individualmente a focinho/penas ou a uma cena não informada. Não houve captura CPU/GPU correlacionada nem teste visual posterior. Caminho normal CURRENT/30 preservado; correção permanente `NOT_IMPLEMENTED`; snapshot parcial **`NOT_ACCEPTED_FOR_GAME_CAPTURE`**. `NEXT_SINGLE_ACTION`: concluir os gates independentes de snapshot-only/alcance/persistência/corrupção/I/O antes de relançar captura. Checkpoints abaixo preservam estados históricos anteriores ao teste visual.

## CURRENT — checkpoint de implementação parcial, revisão final pendente

**Diagnóstico: `NOT_ACCEPTED_FOR_GAME_CAPTURE`. Correção permanente de interpolação: `NOT_IMPLEMENTED`.** Este checkpoint prevalece sobre os estados anteriores abaixo. CURRENT `tfPos` e Manual 30 preservados; P09 `PENDING / NOT_FORMALLY_OPEN`, gate P08 pendente. Sem commit/push, jogo lançado, captura real ou validação visual de 60+ FPS nesta continuação.

Grok via Cursor implementou um diagnóstico CPU default-OFF de snapshot/replay limitado, novo header em RT64, patch versionado, hooks Tooie e testes. O ciclo inicial compilou e passou 61 casos, mas a revisão Sonnet `db86d180-28cc-49ad-8fd1-592dda2f2d79` encontrou três blockers: snapshot-only não alimentava tokens, concorrência com token writers e persistência somente de resumo, sem arrays autocontidos e com sobrescrita. Após correções e 88 casos passando, Sonnet `61708b40-617e-4c0b-9f64-479adc68eb16` confirmou mutex e writer com cópia própria corrigidos, porém identificou outer hooks ainda dependentes de secondary sway. O loader auxiliar do Haiku também aceitou corrupção de payload que preserva magic/tamanho; ausência de checksum/validação semântica segue pendente.

**Último resultado reportado pelo executor, sem revisão final:** Grok sessão `4f58874e-3091-4eb8-858c-a2e2b15ddfd8`, JSON `is_error: false`, relatou `cmake --build build --target p09_local_pair_snapshot_test TooieRecompiled -j2` exit 0, teste `p09_local_pair_snapshot_test` **97/97**, exit 0, e fixture de aplicação em RT64 `6f1c2d99` mais os três pré-requisitos exit 0. Log `p09-scene-arm.log` e resposta `grok-arm-fix.json` preservados. Patch atual SHA256 conferido pelo agente documental: `34e1cf312106ab240e672aa74e909823303c4db7ca1f9919b78426461c6a2c30`. Isso não encerra os blockers de revisão.

Segundo esse executor, a flag `TOOIE_DIAG_P09_LOCAL_PAIR_SNAPSHOT=1` apenas espera, o marcador manual de cena inicia quatro tentativas, e há claim de slot único por processo e writer fora do render com cópia imutável. **`UNKNOWN` até o gate final:** alcançabilidade de toda a cadeia snapshot-only, validade dos limites e labels persistidos, tratamento de corrupção e comportamento concorrente do último ajuste. O teste versionável de I/O `tests/p09_local_pair_snapshot_io_test.cpp` foi criado pelo Haiku (`8c73bfba-c634-4632-9918-ca5204967acc`), mas sua compilação/execução é **`NOT_RUN`**; o harness auxiliar anterior em cache não o substitui. Não executar os comandos de captura reportados pelo builder antes dessa revisão.

Execução e proveniência:

- `p09-local-pair-snapshot-tests.log`: ciclo anterior build Tooie e narrow tests exit 0; snapshot **88/88**, exact-use **1354/1354**, CPU shader **14/14**; fixture apply exit 0. São gates host daquele ciclo, não nova captura nem aceite do último código.
- A tentativa de retomar sessão de planejamento ASK foi interrompida pelo executor, exit 143, sem edições; nova sessão Cursor em modo agent foi usada. A persistência do modo ASK era um risco operacional, não defeito comprovado da CLI.
- Os agentes Codex root, builder e reviewer chegaram a erro de cota de uso, observado em `list_agents`; por isso não houve gate final. Isso não comprova esgotamento de conta Cursor/Antigravity. Nenhuma hora de reset é assumida neste checkpoint.
- Preferência explícita do usuário: **sempre delegar execução**. Root orquestra e arbitra; Grok implementa código/testes via Cursor; Sonnet e Haiku revisam via Cursor; Gemini mantém Markdown via `agy`. Nenhuma memória Codex foi alterada.
- Artefatos em `/home/wesley/.cache/tooie-interpolation-next-20261020/`; HEAD Tooie permanece `73bfd4303b319ddb98fd42ddb6f6ccf8025794c2`. Dirty local e dependências preexistentes preservados. Refresh final de índices Graft/jCode após este código: **`NOT_CONFIRMED`**.

**`NEXT_SINGLE_ACTION`: retomar o gate independente final de snapshot-only, alcançabilidade de hooks, persistência e rejeição de corrupção, incluindo compilar/executar o novo teste I/O em processos separados, antes de qualquer captura em jogo.**

Síntese de checkpoint por Gemini via `agy --model gemini-3.8-flash-high --effort high --print-timeout 180s --output-format json --print ...`, conversa `3484dac2-6d7a-437a-91e4-f3a1a3927f01`, exit 0, `status: SUCCESS`, 165.3 s; `gemini-checkpoint-pending.json`. Integração documental corrigiu a distinção entre revisões anteriores, claims do último executor e testes ainda não rodados.

Fechamento documental deste checkpoint parcial: `markdown_vault_audit` **295 arquivos, 0 erros, 61 avisos preexistentes** (17 tamanho, 44 lint); `git diff --check` exit 0. Handoffs P08/P09/global, diário, planos P08/P09 e limite de promoção em RE sincronizados. Worktree tem código/patch/testes desta continuação ainda não commitados, além da documentação e das configurações/diretórios locais preexistentes; não houve limpeza, stage, commit ou push. Os parágrafos abaixo sobre etapa somente documental permanecem como histórico da investigação anterior.

## Testemunhas visuais prioritárias — continuação 2026-10-20

Registram-se como observações fornecidas pelo usuário (`USER_OBSERVED`) que os glitches visuais mais frequentes ocorriam no **focinho do Banjo** e nas **penas da cabeça de Kazooie**, diferenciando-se expressamente das asas e da mochila, embora vários outros glitches também estivessem presentes. Essa demarcação não estabelece identidade comprovada de modelo, bone ou resource, servindo como critério de priorização para validações e capturas futuras sem descartar as demais ocorrências.

O diagnóstico permanece inconcluído, sem realização de nova captura ou aceite visual, preservando-se CURRENT `tfPos` default-ON e Manual 30, com P08 mantido aberto e a fase P09 classificada como `PENDING / NOT_FORMALLY_OPEN`. A continuação segue delegada: Grok via Cursor na implementação do snapshot/replay CPU limitado, Sonnet via Cursor na revisão, Haiku via Cursor no gate independente e Gemini via `agy` na manutenção do Markdown.

**`NEXT_SINGLE_ACTION`: implementar o diagnóstico default-OFF limitado e auditá-lo antes de qualquer captura, sem alterar shader, alias ou remap.** Estado desta continuação: `IN_PROGRESS`; a ausência de código/capturas descrita na investigação anterior abaixo refere-se à etapa já encerrada, não antecipa o resultado dos agentes atuais.

Redação inicial por Gemini `gemini-3.8-flash-high --effort high`, via `agy`, conversa `d3d4bc49-ef9c-47a8-99fe-cbd769dab98d`, exit 0, JSON `status: SUCCESS` (56.9 s). Artefato: `/home/wesley/.cache/tooie-interpolation-next-20261020/gemini-witness.json`. Integração mecânica pelo agente documental; não se trata de evidência visual ou de implementação concluída.

### Revisão preliminar do contrato e cobertura

Pareceres auxiliares estáticos, **sem prova runtime nem conformidade integral do workflow**:

- **`INFERENCE` / revisão auxiliar Haiku:** o callback legado `is_full_kazooie_feather_trace` filtra `resource62B`; o caminho historicamente chamado de focinho (`11`/animação `42`) usa hooks separados. Portanto, não presumir que o callback de penas cobre a testemunha do focinho. Cabeça versus asa e identidade anatômica desses filtros continuam **`UNKNOWN`**; constantes não provam essas identidades.
- **`UNKNOWN` / proposta não adotada:** `kP09ArticulatedJoinPairs` com 96 entradas não identifica por si só cabeça/focinho. Não usá-lo como prova de cobertura anatômica.
- **Contrato requerido, não resultado já demonstrado:** `mapped=false` duplicando CURRENT como PREV não representa PREV físico. O snapshot deve possuir cópias antes da reciclagem, registrar geração/IDs, rejeitar truncamento, falta de mapa e valores não finitos, e declarar caps de bytes/tempo/frames e varredura `O(faces)` quando aplicável. Validações e implementação ainda aguardam gates.
- O diagnóstico permanece `IN_PROGRESS`; identificação anatômica, captura real e aceite visual estão `NOT_VALIDATED` nesta revisão. CURRENT/Manual 30 preservados.

Haiku via Cursor: sessão `a5b99662-d593-4c27-8541-c0fe3ab22b7c`, exit 0, `success/is_error=false`, 57.2 s, artefato `haiku-target-audit.json`. Sonnet via Cursor: sessão `69ded17f-7dd7-4e3f-8482-4b05af2bf8e8`, exit 0, `success/is_error=false`, 111.4 s, `sonnet-contract.json`. Ambos relataram jCodeMunch indisponível na CLI e navegação incompleta; resultados são auxiliares, não comprovação de execução das regras do repositório. Os nomes dos artefatos são relativos a `/home/wesley/.cache/tooie-interpolation-next-20261020/`.

Síntese documental por Gemini via `agy`, modelo/effort acima, sessão `d2e0a59f-5e76-499c-a0ea-c7a167613c5a`, exit 0, `status: SUCCESS`, 45.8 s, `gemini-preliminary-audit.json`. Labels normalizados na integração para preservar a distinção entre contrato exigido, proposta rejeitada e resultado executado.

## Estado e causa histórica

- **Escopo:** investigar e decidir o próximo passo, sem implementar correção de produção. P09 `PENDING / NOT_FORMALLY_OPEN`, residual `P09-RES-05`; esta investigação não fecha P08.
- **Baseline:** CURRENT `tfPos` default-ON e Manual 30 preservados. Nenhum código/patch alterado, jogo lançado, captura nova, commit ou push nesta etapa.
- **HEAD publicado anteriormente:** `73bfd4303b319ddb98fd42ddb6f6ccf8025794c2`. RT64 `6f1c2d99a4ea571c139f449c326fd176ba8f3496`, dirty protegido, diff rastreado SHA256 `339f7195ada462ebf24884c5fbd6d6471023f02bcc6da76712c2a377f5545d45` antes/depois. Binário SHA256 `e2059b516a90f09e50e77f0622218014798f6a155f6ffe0d2b6a66735427f9f9`.
- **Causa histórica aceita:** `NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`, `PROVEN` nas capturas CPU corrigidas. CURRENT elimina o sintoma, mas repete poses. Elementwise global fecha junções e cisalha malhas; o seletor WORLD antigo apresentou regressões em Klungo/Mumbo e deixou o focinho afetado. Alias por chamada + clone B+C permanece `CONDITIONAL`, não implementado.
- Fontes: [[p09-causal-baseline-and-fix-direction]], [[p09-pathological-tie-groups]], [[p09-current-work]]. CLIP/NDC/W-sign não foram reabertos. Os contratos gráficos foram revistos e permanecem inalterados.

## Evidências estáticas confirmadas pelo root

Todos os caminhos de fonte abaixo são relativos a `../BanjoRecomp/lib/rt64/`.

- **`PROVEN` / `OTHER` / `STATIC_SOURCE` (shader/seletor):** `src/render/rt64_transform_processor.cpp:3194-3204,3231` troca `curMatrix` por elementwise nos slots selecionados e envia `lerpWorldTransforms`. `src/shaders/RSPProcessCS.hlsl:60,72` usa essa `worldMat` em `tfPos`; a mesma matriz alimenta `computeTextureGen` (:106, com o bit enabled de `lookAtIndex`) e iluminação (:116,122,125, com `lightCount > 0`). As mesmas afirmações aparecem nos pareceres como `PROVEN_STATIC`.
- **`HYPOTHESIS`:** alterar também iluminação/texgen pode ter contribuído para regressões do seletor. Os gates reais de Klungo/Mumbo continuam `UNKNOWN`; nenhum incidente novo foi capturado. Isso não reabre texgen como causa original da geometria das penas, anteriormente refutada. Posição elementwise com iluminação/texgen rígidos é um candidato experimental futuro, não uma solução demonstrada.
- **`PROVEN` / `OTHER` / `STATIC_SOURCE` (topologia):** `src/render/rt64_transform_processor.cpp:1811-1881` conta arestas por par não dirigido de índices armazenados dentro de cada `GameCall`, admitindo contagem 1. V2 já possui pareamento same-call/cross-call; não se conclui ausência de busca entre chamadas. `src/hle/rt64_rsp.cpp:895-916` pode clonar vértices usados; :1310-1328 emite `faceIndices` globais. O impacto desses mecanismos sobre as costuras patológicas é `UNKNOWN`.
- **Identidade:** coincidência de ponto, mesmo WORLD e float impresso não certificam identidade; empates não podem ser suprimidos por esses motivos. A certificação exige os dois endpoints de uma aresta pareada em PREV/CURRENT, semântica física row-vector finita e a tolerância existente `1e-4 * comprimento`, sem piso 0.25 ou epsilon absoluto inventado.
- **Artefatos indisponíveis:** `/tmp/p09-tie-discriminator/capture-gameplay/tooie-p09-seam-edge-pathology-correlation.txt` e `/tmp/p09-tie-discriminator/capture-gameplay/tooie-p09-seam-edge-pathology-ties.txt` estão ausentes. Não houve reanálise desses arrays nem revalidação de seus hashes; os resultados documentados são históricos.

## Validação executada nesta sessão

```sh
cmake --build build --target p09_exact_use_correlation_test p09_cpu_shader_clip_semantics_test tooie_rsp_endpoint_current_test rt64_present_decimator_test -j2
./build/p09_exact_use_correlation_test
./build/p09_cpu_shader_clip_semantics_test
./build/tooie_rsp_endpoint_current_test
./build/rt64_present_decimator_test
```

Todos exit 0. Build: `ninja: no work to do` (checagem de atualidade, não nova recompilação). Resultados dos executáveis, respectivamente: **1354/1354**, **14/14**, **20/20**, **PASS**. Manifesto e hashes individuais: `/home/wesley/.cache/tooie-interpolation-20261020/root-validation.json`. Nenhum teste visual/GPU, medição de taxa nativa ou validação de 60/120 FPS nesta etapa. Diffs rastreados de RT64, runtime e frontend permaneceram idênticos antes/depois; dirty preexistente protegido.

## Pareceres, ferramentas e arbitragem

- Cursor CLI: `cursor-agent --trust --force --print --mode ask --model MODEL --output-format json PROMPT`. Modelos exatos: `grok-4.7-medium` (três rodadas), `claude-sonnet-5-5-medium` e `claude-haiku-5-5-medium`; todos exit 0 e JSON `success/is_error=false`. Não há comprovação de contexto de 200k pelo Cursor.
- Sessões Grok: `c836b991-cd73-4288-9e60-aab892b157d9`, `e4825b94-13ca-4c15-b8e1-5f6c0f5a32ff`, `c3ab71cc-e491-4bca-8779-c502a8fdb26d`. Sonnet: `bb388c15-5a6c-45a5-9193-7bf6079864b0`. Haiku: `0ca2edd6-bf98-4a42-8289-bd8ac8f24a40`.
- Root utilizou Graft e jCodeMunch; os agentes relataram jCodeMunch indisponível em seus catálogos. Sonnet admitiu não ler a navegação obrigatória: seu parecer é auxiliar, não prova de conformidade do workflow. Os mecanismos adotados acima foram conferidos pelo root no fonte.
- **Arbitragem:** Grok retirou o aceite pontual e a tolerância absoluta inventada. Não se adotam as propostas Sonnet de suprimir TIE por mesmo WORLD/coordenadas impressas, garantir ausência de glitches com CURRENT seletivo, ou declarar que persistência após um A/B provaria vazamento de slot. Também não se adota a inferência Haiku `30 < viOriginalRate`: taxa real é `UNKNOWN`. A condição estática acima do nativo é `viOriginalRate > 0 && targetRate > viOriginalRate`, além de não pausado e target válido.
- Gemini via `agy --model gemini-3.8-flash-high --effort high --print-timeout ... --output-format json --print PROMPT`: auditoria `SUCCESS`, conversa `8dedc69c-1fc2-4b3d-9855-bb6a2c9b0c46`. A primeira síntese, conversa `86ef55b0-21b9-4181-8192-57ef296df76c`, retornou **status ERROR/interrupted** e timeout de 4 minutos apesar de exit CLI 0; não é sucesso. Nova redação `SUCCESS`, exit 0, conversa `479c7a95-fdb7-4c74-9924-fb5c262eb222` em `gemini-short.json`, revisada e integrada pelo root.
- Auditoria Gemini corrigida pelo root: P08 não está globalmente fechado; `ninja no work` não é recompilação; os gates de interpolação acima e decimação abaixo do nativo são diferentes. A nova redação teve suas citações expandidas e o efeito da separação de iluminação limitado às regressões hipotéticas.
- Artefatos preservados em `/home/wesley/.cache/tooie-interpolation-20261020/`: `root-validation.json`, `status.json`, `grok.json`, `grok-arbitration.json`, `grok-lighting.json`, `sonnet.json`, `haiku.json`, `gemini-audit.json`, `gemini-synthesis.json`, `gemini-short.json` e scripts reproduzíveis. Não versionar dumps brutos futuros no git.

## Próxima ação única e critérios

**`NEXT_SINGLE_ACTION`: implementar um diagnóstico CPU default-OFF de snapshot/replay limitado e autocontido de um par PREV/CURRENT correspondente, armado na cena, antes da reciclagem do workload.** Ainda `NOT_IMPLEMENTED`; não é autorização técnica para alias, remap ou correção de produção.

1. Reter geração, chaves de nove campos, posições locais, matrizes originais, mapa temporal, ownership, triângulos/arestas incidentes e ambas as pontas; incluir `lightCount`, `lookAtIndex` e máscara candidata sem ativar o seletor.
2. Definir limites explícitos de memória, frames e bytes, registrar contagens/custo e rejeitar sucesso com truncamento ou mapeamento incompleto. Não invocar `p09EmitNearestJoinPairs`/`p09SeamEdgeDryRunV2` pesados no render. Não prometer O(incidentes) se a implementação precisar de uma varredura limitada O(faces).
3. Reproduzir offline o classificador atual e auditar arestas físicas pareadas e gates de iluminação/texgen. Gate aberto demonstra possibilidade, não causalidade visual; gate fechado no incidente limita esse candidato. Um conjunto incompleto continua `UNKNOWN`, sem dedup automática de TIE.
4. Só depois escolher entre A/B de posição com luz rígida e arquitetura local B+C. Nenhum desses caminhos está validado como correção final; preservação de rigidez e junções requer teste próprio.

O snapshot CPU pode manter CURRENT/Manual 30 e avaliar pesos intermediários offline. Aceite visual futuro exige endpoint explicitamente OFF **e** override de FPS controlado no build de diagnóstico; a flag `0` isolada não remove a trava Manual 30. O contador mede PRESENT, não pose nova nem velocidade de simulação. Os presets pedidos continuam `{20, 30, 45, 60, 70, 100, 120, 144}`; sua liberação depende do reparo e de provas de timing/áudio/gameplay preservados.

## Encerramento

Somente documentação alterada nesta tarefa; código gerado, paliativo e dependências protegidas preservados. Sem commit/push. Handoffs e planos P08/P09 sincronizados; conhecimento estático promovido de forma limitada para [[../../reverse-engineering/systems/transform-interpolation]].

`markdown_vault_audit` executado: **295 arquivos, 0 erros, 61 avisos preexistentes** (17 diagnósticos de tamanho, 44 de lint). `git diff --check` exit 0. Worktree: sete documentos rastreados modificados e esta nova evidência não rastreada; configurações locais e diretórios de ferramentas preexistentes continuam fora do escopo. `NEXT_SINGLE_ACTION` é exclusivamente o diagnóstico limitado descrito acima.
