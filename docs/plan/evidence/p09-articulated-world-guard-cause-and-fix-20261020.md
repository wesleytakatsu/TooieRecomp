---
title: "P09 — Investigação causal e guardas de interpolação WORLD articulada"
description: "Avaliação causal de glitches na interpolação F8 (Kazooie crest rods e Banjo backpack strip); guardas de coerência de translação e fechamento articular; iteração 3 causa raiz A comprovada na fonte; continuação pós-cota Grok com patch plume versionado e diagnóstico ong2; gate técnico Haiku 5.5 passed=false."
status: in_progress
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-10-21
tags: [p09, interpolation, rt64, patches, gate, causal-investigation, f8, haiku-review, grok-quota, plume-patch, technical-gate]
---

# Evidência P09: Investigação causal e guardas de interpolação WORLD articulada (2026-10-20/21)

**Escopo:** Investigação causal dos glitches geométricos apontados pelo usuário no burst F8 (`interpolated-burst-20261020-175030/`), implementação de guardas genéricas no RT64 (coerência de translação e fechamento articular), aprofundamento na iteração 3 (causa raiz A na fonte e correção no owning layer), continuação técnica pós-cota do Grok (patch do plume, diagnóstico de encerramento ong2 e rebuild de pin limpo), auditoria de integridade e gates independentes de aceitação (iterações 1/2, 3 e gate técnico pós-Sonnet).

Estado do gate técnico independente pós-Sonnet (`technical-gate.json` / `technical-haiku-review.md`): **`passed: false`** (7 bloqueadores B1–B7).
O paliativo de produção **CURRENT/30 permanece ativo por padrão** (`TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1`, `tooie_menu::kForcedRefreshRate = 30`). A interpolação contínua de alta taxa **não** está liberada para produção.

---

## 1. Inspeção visual primária das testemunhas (Burst F8)

Diretório do usuário: `/home/wesley/Imagens/TooieRecomp/interpolated-burst-20261020-175030/` (20 PNGs, resolução 1622×912, estágio `PRE_VI_RESOLVED_COLOR_TARGET`, todos com peso de frame `curW = 0.500000`, `kind = INTERPOLATED`, `targetRate = 60`, `viOriginalRate = 30`).

O usuário marcou **14 imagens únicas com glitch**: `001`, `002`, `005`, `006`, `007`, `008`, `009`, `010`, `012`, `013`, `014`, `015`, `016`, `018`. As demais permanecem `UNCLASSIFIED` (não classificadas como limpas).

A inspeção direta com leitor de imagem confirmou:
1. **Banjo backpack blue strip em `image_018.png`**:
   - Geometria visível: faixa/prisma triangular pontiagudo em azul vivo projetando-se da borda superior esquerda da mochila azul do Banjo em direção ao cenário.
   - Status: `PROVEN_RENDERED_DEFECT` na imagem de captura; causa específica a nível de transform / osso examinada na iteração 3 (ver seção 6).
2. **Kazooie head crest stretched rods (e.g. `image_010.png`, `image_015.png`, `image_008.png`)**:
   - Geometria visível: hastes finas esticadas (agulhas/rods) de cor laranja/amarela disparando da crista da cabeça da Kazooie para cima/trás durante movimento de giro/balanço.
   - Em `image_015.png` (workload 1619): haste laranja fina terminando em amarelo acima da crista da Kazooie.
   - Em `image_010.png` (workload 1599): espícula amarela nítida estendendo-se verticalmente da cabeça da ave.
   - Em `image_008.png` (workload 1595): haste fina alaranjada evidente.

---

## 2. Hipóteses e discriminação causal

### Mecanismo A: Portão de translação AUTO em malha articulada (`PROVEN`)
- **Fenômeno:** Em `RigidBody::updateLinear` (modo AUTO do RT64), se a velocidade de uma transformação inverte ou salta de razão, o portão de translação desarma (`lerpTranslation = false`), segurando a translação no valor `CURRENT`, enquanto rotação, escala e skew continuam interpolando via decomposição polar / slerp.
- **Efeito em malha articulada:** Em objetos esqueléticos onde cada osso recebe uma matriz WORLD composta separada, um osso que sofre desaceleração/inversão tem sua translação travada em `CURRENT`, mas seus vértices giram em torno da origem de `CURRENT`. Os ossos adjacentes continuam interpolando suavemente translação e rotação entre `PREV` e `CURRENT`. Triângulos que cruzam a junta entre esses ossos conectam vértices com bases espaciais incompatíveis, gerando arestas esticadas gigantescas ("rods").
- **Evidência:** Workload 1594 e 1619 na title scene. Reconstrução CPU demonstra que ~79% dos triângulos esticados são eliminados ao forçar a coerência de translação (se ao menos um osso do grupo conexo interpola translação, todos os ossos articulados interpolam). Classificação: **`PROVEN`** para a classe de juntas testada na title scene.

### Mecanismo B: Fechamento articular sob interpolação não-linear de matrizes compostas (`HYPOTHESIS / CLASS-LEVEL HEURISTIC`)
- **Fenômeno:** Mesmo com translação coerente, matrizes WORLD já compostas interpoladas independentemente por `RigidBody::lerp` sofrem discrepâncias angulares em rotações rápidas entre frames (ex.: ~88° em um único frame de 30 Hz). A interpolação rígida independente de dois ossos vizinhos não preserva a invariância de comprimento da aresta que conecta vértices dos dois ossos.
- **Tratamento:** `p09ClosureGuard` detecta triângulos跨-WORLD onde a aresta interpolada excede `1.25 * refLength + 1.0` unidades e chaveia as matrizes envolvidas para interpolação linear elemento a elemento (`lerpMatrix`), que garante que nenhum ponto exceda a combinação convexa dos endpoints.
- **Limitação:** É uma cura heurística a nível de classe, não uma prova anatômica exata por osso. A interpolação elemento a elemento não é rígida (encolhe levemente o osso durante rotações grandes).

---

## 3. Implementação das guardas no RT64

As guardas foram introduzidas em cabeçalhos inline no namespace `RT64`:
- `src/render/rt64_p09_translation_coherence.h`: detecta grupos conexos via union-find sobre `faceIndices` e `worldIndices`. Se algum membro mapeado do conjunto conexo interpola translação, força os demais (`forceLerp[t] = 1`). Conjuntos onde todos os ossos estão travados (teleporte do objeto inteiro) permanecem travados.
- `src/render/rt64_p09_closure_guard.h`: avalia arestas de triângulos跨-WORLD contra o limite `EdgeRatio = 1.25f` e `EdgeSlack = 1.0f`. Chaveia para `elementwise` caso detecte estiramento.
- `src/render/rt64_p09_world_interpolation_guard.h`: pós-passo em `Workload` após `TransformProcessor::process()`. Mantém-se estritamente **inerte** quando `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` está ativo (padrão Tooie) e quando o opt-in seletivo legado está presente.
- `src/render/rt64_p09_world_lerp_audit.h`: infraestrutura de auditoria default-OFF controlada por `TOOIE_DIAG_P09_WORLD_LERP_AUDIT`.

Controles de teste independentes:
- `TOOIE_TEST_P09_TRANSLATION_COHERENCE=0`: desativa guarda de coerência.
- `TOOIE_TEST_P09_CLOSURE_GUARD=0`: desativa guarda de fechamento articular.
- `TOOIE_DIAG_P09_WORLD_LERP_SHADOW=1`: computa as guardas e audita, mas entrega ao renderizador a interpolação original desprotegida para teste contrafactual idêntico.

---

## 4. Estado da validação e revisão independente da iteração 1/2 (Haiku Gate Histórico)

Revisão formal independente por Claude Haiku 5.5 registrada em `/home/wesley/.cache/tooie-glitch-cause-and-fix/haiku-review.md`.

### Veredito histórico: `gate.json` -> `{"passed": false}`

| Item de Gate | Status | Descrição e Avaliação |
|---|---|---|
| **F1: Prova GPU em par casado** | **ABERTO (HIGH - Bloqueia Gate)** | O par casado existe apenas para ORIGINAL (renderizado) vs GUARDED (reprojeção CPU do mesmo workload 1619). Nenhum frame GPU foi renderizado com guardas ligadas para o mesmo workload. |
| **F2: Métrica de estiramento zero** | **ABERTO (HIGH - Bloqueia Gate)** | A contagem `crossStretched = 0` avalia o próprio critério de convergência do guard (1.25× / +3 unidades). Não constitui métrica independente de erro ou validação visual GPU. |
| **F3: Articulação held presa em CURRENT** | **ABERTO (HIGH)** | Conjuntos inteiros articulados onde todos os membros têm `lerpTranslation == 0` (34.998 transformações no run) são exibidos integralmente em CURRENT (`heldMatrix`). Congelamento de rotação não medido quanto a falso-positivo vs teleporte real. |
| **F4: Cobertura de gameplay** | **ABERTO (MEDIUM)** | Testado apenas no Title Attract. Gameplay real (Talon Trot, Mumbo, Klungo, caminhada) não testado com guardas ativas. |
| **F5: Replay de patches em pin limpo** | **FECHADO (PASS)** | Replay verificado em pin RT64 `6f1c2d99` via `apply_runtime_patches.sh`; todos os stanzas aplicam com exit 0. |
| **F6: Códigos de saída de build** | **FECHADO (PASS)** | Builds com `EXIT=0`, binário `TooieRecompiled` SHA256 `ebf5924cfd1dfd1471e44a0ae438e022a779ac3e32f6e9c8211bbb18da140b76`. |
| **F7: Mecanismo B é heurística** | **CLASSIFICADO (MEDIUM)** | Registrado formalmente como cura heurística não-rígida a nível de classe, não mecanismo anatômico provado. |
| **F8: Causa raiz exata / Mochila** | **CLASSIFICADO (INFO)** | Mecanismo A provado; hastes individuais sem mapeamento de osso; faixa azul da mochila (`in018`) não demonstrada corrigida por par casado. |

### Testes executados (iteração 1/2):
- `./build/p09_world_interpolation_guard_test`: `PASS 27/27`, exit 0.
- `./build/p09_interp_burst_test`: `PASS`, exit 0.
- `git diff --check`: exit 0.
- `markdown_vault_audit`: 0 erros (297 arquivos escaneados).

---

## 5. Patches versionados (iteração 1/2 históricos)

Cadeia de patches aplicável em pin RT64 `6f1c2d99a4ea571c139f449c326fd176ba8f3496` via `tools/apply_runtime_patches.sh`:
- `patches/rt64/rt64-p09-world-interpolation-guard.patch`: SHA256 `1b771a1ab9205f6ffba0c1f8085e53979f38fed031319aedfe04d8cf8bb520d6`
- `patches/rt64/rt64-p09-interp-burst-capture.patch`: SHA256 `af9cd9d9f94718f8ca76f9606f50093317d9ac676ddaca07e44c5bc3bfd4767b`
- `patches/rt64/rt64-p09-local-pair-snapshot.patch`: SHA256 `f3aa59ba93e590416135d40ae5c9b21cb684acdc352e91366cbdff658f4bb57a`
- `patches/rt64/rt64-p09-exact-use-correlation-header.patch`: SHA256 `0d4c250a5c4950dcdb1d0835ea7347014c90893e93332caa5576c67e03459561`

---

## 6. Iteração 3: Retomada pós-limite de sessão e Avaliação Causal Aprofundada

Após a interrupção por cota da sessão anterior, a investigação foi retomada em 2026-10-20/21 (`/home/wesley/.cache/tooie-glitch-cause-and-fix/resumed-cause-and-fix.md`).

### 6.1 Causa Raiz A na Fonte RT64 (`PROVEN`)
- Em `RT64/src/hle/rt64_rigid_body.cpp:30-34`:
  ```cpp
  dotCurVel = std::max(dotCurVel, Epsilon);
  curVelMag /= dotCurVel;
  lerpTranslation = (curVelMag < VelocityTolerance) || (curVelRatio < VelocityRatioTolerance);
  ```
  `dotCurVel` é o cosseno do ângulo entre a velocidade anterior e a atual. Se o ângulo for $\ge 90^\circ$ (reversão de movimento num balanço de membro/osso), `dotCurVel` é limitado a $10^{-6}$.
  A divisão `curVelMag /= dotCurVel` inflaciona a magnitude para milhões de unidades **antes** de comparar com `VelocityTolerance = 5`.
  Consequentemente, **qualquer reversão de direção desarma a translação (`lerpTranslation = false`)**, mesmo que o osso esteja se movendo a velocidades diminutas (ex.: 0.1 unidade/frame no ápice de um balanço).
  O osso travado em `CURRENT` tem seus vértices rotacionando em torno da origem de `CURRENT`, enquanto os ossos adjacentes interpolam a translação PREV-CUR normalmente.
  - No run OFF, 25.899 de 67.045 linhas retidas (38.6%) possuíam velocidade bruta $< 5$ (dentro da tolerância).
  - No workload 1604, 62 de 208 matrizes mapeadas estavam retidas pela amplificação de cosseno; com a liberação correta, apenas 1 permanece retida.

### 6.2 Correção no Owning Layer
- `RT64/src/hle/rt64_rigid_body.{h,cpp}`: introduzido `lerpTranslationSpeedOnly`, avaliando o mesmo portão de tolerância sobre as velocidades brutas (sem amplificação por cosseno). As variáveis existentes `lerpTranslation` e o método `RigidBody::lerp` permanecem intactos, assegurando que outros consumidores (câmera, projeção, caminho padrão CURRENT/30) permaneçam inalterados.
- `RT64/src/render/rt64_p09_world_interpolation_guard.h` + `rt64_p09_translation_coherence.h::gateSpeedOnlyEnabled`: para transformações WORLD, a guarda libera translações que somente a amplificação por cosseno desativou (`!lerpTranslation && lerpTranslationSpeedOnly`). Spikes reais de velocidade ($\ge 10\times$ e $\ge 5$ unidades) continuam retidos.
- Variável de ambiente `TOOIE_TEST_P09_GATE_SPEED_ONLY=0`: restaura o comportamento antigo para comparações A/B.
- Gancho `TOOIE_TEST_INTERP_BURST_AT_WORKLOAD=<id>` no burst controller: permite armar capturas F8 sincronizadas diretamente na render thread para comparar workloads equivalentes.
- Testes unitários expandidos: `tests/p09_world_interpolation_guard_test.cpp` recebeu 8 novas asserções (total 35 asserções), cobrindo reversão lenta, spike real de velocidade, movimento estável, INTERPOLATE e SKIP.
- Guarda experimental de corda de translação (isolated held transforms cujos vértices saem da corda) foi testada e **REMOVIDA** por ablação em 11+ pares de pixels não apresentar ganho perceptível (YAGNI).

### 6.3 Provas da Iteração 3
1. **Pares casados GPU (OFF vs ON) na cena de título (endereça F1)**:
   - Capturas com `tools/run_ab.py` e `tools/ab.sh` geraram pares renderizados pela GPU (`runs/off1..off3` vs `runs/on1..on8`).
   - Alinhamento de conteúdo por offset determinístico via `tools/align.py` (concordância de 99.2% a 99.9% em faces e cross-stretched).
   - Controle de controle: pares OFF vs OFF alinhados resultaram em diferença zero de pixels (`diffpix>8 = 0` em 6/6 pares), provando que diferenças entre OFF e ON decorrem exclusivamente das guardas.
   - Inspeção visual de GPU (`evidence/sheet_on3_a.png`, `backpack_wl1639.png`, `final_backpack_1889_1890.png`):
     - Crista da Kazooie: hastes alaranjadas eliminadas em wl 1604, 1621, 1625, 1628, 1635.
     - Mochila do Banjo (`image_018.png` / wl 1889 e 1890): no wl 1889 OFF a faixa azul flutuava desacoplada; no ON ela aparece recolocada nas costas. No wl 1890 OFF a aba estava deslocada acima do ombro; no ON está recolocada, restando apenas pequena lasca azul/laranja pontual.
2. **Métricas independentes de overshoot de vértice (endereça F2)**:
   - Ferramenta `tools/vover.py` avalia vértices fora da corda PREV-CUR via teste de corda $>0.5\times\text{chord}+3$ (independente do critério $1.25\times/+3$ da guarda):
     | Execução | Vértices fora da corda | Triângulos esticados cruzando WORLD | Vértices congelados em CURRENT | Vértices perto do ponto médio (30-70%) |
     | :--- | :--- | :--- | :--- | :--- |
     | `off1` (original desprotegido) | 26.185 | 14.328 | 0.8% | 89.5% |
     | `on1` (guarda iteração 2) | 7.621 | 0 | 4.6% | 90.8% |
     | `on7` (guarda final iteração 3) | 2.673 | 0 | 0.6% | 97.6% |
   - No attract demo de gameplay (wl 3000-9000): métricas estáticas do dump indicaram que vértices fora da corda caíram de 15.235 para 2.688 e triângulos esticados caíram de 3.952 para 0. **Ressalva crítica**: a execução `runs/ong2` sofreu crash fatal com SIGSEGV (`rc=-11`), e 6 execuções terminaram não-recolhidas (`rc=None`), de modo que a cena de gameplay permanece não-validada e bloqueada por crash fatal.
3. **Risco de snapping F3 (endereça F3)**:
   - Na iteração 2, 33 de 208 matrizes do Banjo/Kazooie no wl1604 e 4.6% dos vértices móveis foram congelados em CURRENT (as hastes sumiram em parte porque o modelo deixou de interpolar).
   - Com o fix do portão de velocidade linear, matrizes presas em CURRENT no wl1604 caíram para 0; vértices presos em CURRENT caíram para 0.6% (inferior ao baseline OFF de 0.8%); vértices próximos ao ponto médio subiram para 97.6%.

---

## 7. Revisão Independente da Iteração 3 (Haiku 5.5 Gate)

Auditoria independente por Claude Haiku 5.5 registrada em `/home/wesley/.cache/tooie-glitch-cause-and-fix/resumed-haiku-review.md` e `resumed-gate.json`.

### Veredito: `resumed-gate.json` -> `{"passed": false}`

| Item de Revisão | Veredito Haiku | Detalhamento e Limitações |
| :--- | :--- | :--- |
| **Causa Raiz A na fonte** | **PROVEN** | Verificado em `RT64/src/hle/rt64_rigid_body.cpp:30-34`. `curVelMag /= dotCurVel` antes de tolerância confirmado. |
| **Escopo do Baseline OFF** | **OK** | `gateSpeedOnly` requer `coherenceOn`; OFF desativa coerência e guarda de fechamento, restaurando o motor original fielmente. |
| **Testes unitários** | **PASS** | Re-executados sem compilação: `./build/p09_world_interpolation_guard_test` (PASS 35/35), `./build/p09_interp_burst_test` (PASS), `./build/p09_local_pair_snapshot_test` (123/123). SHA256 do binário `d31d2c6f...b871` e teste `26b7202b...14a0`. |
| **Pins de patches** | **PASS** | Hashes dos 6 patches coincidem com `tools/apply_runtime_patches.sh`. |
| **Replay em pin limpo** | **PASS** | Replay limpo em checkout novo de RT64 pin `6f1c2d99` aplica os 6 patches em ordem com exit 0; os 4 cabeçalhos `rt64_p09_*.h` são idênticos byte a byte. |
| **Replay vs Árvore de trabalho** | **FAIL** | A árvore de trabalho carrega código de diagnóstico RT64 unversioned (14.263 linhas em 32 arquivos eram preexistentes de fases anteriores como P08 sob preservação de worktree, enquanto a iteração 3 introduziu apenas 5 inserções em rt64_rigid_body.{h,cpp} e rt64_workload_queue.cpp); o binário validado deriva desse estado de árvore suja. |
| **Exits de build** | **NOT VERIFIED** | Sem logs salvos de compilação em disco (`build-iter3*` ausente); dry-run do ninja foi inconclusivo. |
| **Códigos de saída de jogo** | **CONTRADIÇÃO** | `runs/ong2/meta.json` encerrou com `rc=-11` (SIGSEGV) não mencionado nem explicado no doc; 6 runs terminaram com `rc=None` (processos não recolhidos após SIGTERM). |
| **F1 (Par GPU casado)** | **PARCIAL-CLOSED** | Pares GPU renderizados confirmam title scene; falta validação fora da família da cena de título. |
| **F2 (Métrica estiramento)** | **PARCIAL** | Métrica independente de corda (>0.5*chord+3) apoia, mas `crossStretched=0` segue critério próprio da guarda. |
| **F3 (Snapping articulações)** | **ABERTO** | Avaliado apenas em Banjo/Kazooie no wl1604; sem medição em objetos não-jogadores; corte de cena wl4226 ainda retém 56 matrizes. |
| **F4 (Gameplay)** | **ABERTO** | Apenas attract demo avaliado; ausência de gameplay dirigido pelo usuário a 60 FPS. |
| **F7 (Fechamento articular)** | **HEURÍSTICA** | Inalterado; mecanismo B é cura heurística a nível de classe. |
| **F8 (Mochila Banjo)** | **PARCIAL** | Defeito e correção visíveis em 2 frames GPU alinhados (wl1889/1890); resíduo visível em 1890; não medido ao longo de um intervalo contínuo. |

### Bloqueadores Concretos Identificados:
1. Códigos de saída de build da iteração 3 não verificados por log em disco.
2. Contradição de código de saída de jogo: crash `ong2` com `rc=-11` (SIGSEGV) inexplicado, e 6 runs com `rc=None`.
3. Árvore de trabalho com código de diagnóstico não versionado impedindo que a cadeia limpa reproduza o binário de teste.
4. Ausência de teste em objetos articulados não-jogadores (F3).
5. Ausência de gameplay real pelo usuário a 60 FPS (F4).

---

## 8. Patches Versionados Atualizados (Iteração 3)

Cadeia de patches aplicável em pin limpo RT64 `6f1c2d99a4ea571c139f449c326fd176ba8f3496` via `tools/apply_runtime_patches.sh`:
1. `patches/rt64/rt64-rsp-endpoint-current.patch`: SHA256 `49925480c2d1736c790ddf96152844951f6fe27a150ab1cc110347d557c0d813`
2. `patches/rt64/rt64-present-fps-decimator-and-counter.patch`: SHA256 `6d746f27ed1bd3132dcea8464adeb25dcfac4ad38502bf92f402ea6e3a19836c`
3. `patches/rt64/rt64-p09-exact-use-correlation-header.patch`: SHA256 `0d4c250a5c4950dcdb1d0835ea7347014c90893e93332caa5576c67e03459561`
4. `patches/rt64/rt64-p09-local-pair-snapshot.patch`: SHA256 `f3aa59ba93e590416135d40ae5c9b21cb684acdc352e91366cbdff658f4bb57a`
5. `patches/rt64/rt64-p09-interp-burst-capture.patch`: SHA256 `971b7caf1d1d9f8a6cd529792b057d3b83a76cffdbcf504e808d9de90520184c`
6. `patches/rt64/rt64-p09-world-interpolation-guard.patch`: SHA256 `0527497cc43fd858f0f7ebeb425de1329d1df7ebee2cbe02ba6d632944490092`

Artefatos binários sob validação:
- `build/TooieRecompiled`: SHA256 `d31d2c6f444e0314db6b753e084326a926400c7073f4b9b241159a314a63b871`
- `build/p09_world_interpolation_guard_test`: SHA256 `26b7202b37497c4583190be9b31952eec89e9f3ecbeffdb59cb4a148a04214a0`

## 9. Continuação Técnica pós-cota do Grok (2026-10-21)

Após interrupção por cota do Grok Build 4.7 (`status 402 Payment Required: Grok Build usage balance exhausted`) em 2026-10-21 03:55 UTC, os artefatos de build foram inspecionados e a execução técnica continuou via Sonnet e Haiku (`/home/wesley/.cache/tooie-glitch-doc-audit-and-fix/`):

### 9.1 Prova de Build Retida do Grok e Correção do Plume
- O build serial do Grok em árvore limpa `cleanpin-build` registrou exit 0 (`cleanpin-build.exit` = 0, log de 136 KB, binário SHA256 `43e070abb80c5a9272d2945354c652578a09366d0ca7fbf9ec5c9b322ee4942f`).
- Ao executar a primeira captura de burst nesse binário (`cpon1`), ocorreu SIGSEGV em `plume::VulkanCommandList::copyTextureRegion` por dereferência de `dstTexture` nulo.
- Investigação na fonte (`PROVEN_SOURCE`): o código upstream do `plume` suportava apenas cópia buffer->texture (`dst=SUBRESOURCE, src=PLACED_FOOTPRINT`); a captura de burst requer a direção inversa (texture->buffer via `vkCmdCopyImageToBuffer`). Na árvore de trabalho suja, essa ramificação existia como uma edição local desversionada de 31 linhas em `plume_vulkan.cpp` (mtime 2026-08-27).
- Sonnet extraiu e versionou o patch `patches/rt64/rt64-plume-texture-to-buffer-copy.patch` (SHA256 `49c218efa703a8946d65652474b209934cd8ec96a55c2053b93826cf8d070852`, base `d890ac899e505fb30040e037a4037cdeca68f033`), registrando-o em `tools/apply_runtime_patches.sh`.
- Recompilação do pin limpo (`cleanpin-build2.log`, exit 0) gerou o binário auditado final SHA256 `965bbabbc3938ac833fd11b0ccb8b198f27e2108f95ebb51c405f4b4924aec05`. Todas as capturas de burst posteriores (`cpon2`, `cpoff1`, etc.) salvaram 20/20 sem crash.

### 9.2 Diagnóstico do Crash `ong2` (SIGSEGV rc=-11)
- Análise de coredump do PID 1659795 (`gdb -batch`):
  - Thread hóspede 1659838 falhou em `unload_overlays ← ovl_unload ← func_8008160C ← func_800123F4 ← func_800124EC ← run_thread_function` tentando ler nó de `unordered_map` já desalocado.
  - Thread principal no mesmo instante: `__libc_start_main → exit → __run_exit_handlers → ~unordered_map<int, void(*)(uint8_t*, recomp_context*)>` destruindo a tabela estática de funções.
- Causa (`PROVEN_SOURCE`): em `librecomp/src/recomp.cpp` (`recomp::start`), o encerramento do processo destrói tabelas estáticas enquanto threads OSThread do hóspede ainda estão em execução. Condição de corrida no encerramento (teardown race), alheia à interpolação.
- Mitigação de teste: `run_ab.py` e `play.py` foram ajustados para envio de SIGTERM com espera de 30s seguido de SIGABRT/SIGKILL com `wait()` explícito. O produto em si mantém o teardown race em aberto (pertence ao N64ModernRuntime).

### 9.3 Cadeia de Patches Atualizada (7 Patches em Pin Limpo)
Cadeia aplicável em pin limpo RT64 `6f1c2d99a4ea571c139f449c326fd176ba8f3496` via `tools/apply_runtime_patches.sh`:
1. `patches/rt64/rt64-rsp-endpoint-current.patch`: SHA256 `49925480c2d1736c790ddf96152844951f6fe27a150ab1cc110347d557c0d813`
2. `patches/rt64/rt64-present-fps-decimator-and-counter.patch`: SHA256 `6d746f27ed1bd3132dcea8464adeb25dcfac4ad38502bf92f402ea6e3a19836c`
3. `patches/rt64/rt64-p09-exact-use-correlation-header.patch`: SHA256 `0d4c250a5c4950dcdb1d0835ea7347014c90893e93332caa5576c67e03459561`
4. `patches/rt64/rt64-p09-local-pair-snapshot.patch`: SHA256 `f3aa59ba93e590416135d40ae5c9b21cb684acdc352e91366cbdff658f4bb57a`
5. `patches/rt64/rt64-plume-texture-to-buffer-copy.patch`: SHA256 `49c218efa703a8946d65652474b209934cd8ec96a55c2053b93826cf8d070852` (submódulo `src/contrib/plume` em `d890ac89`)
6. `patches/rt64/rt64-p09-interp-burst-capture.patch`: SHA256 `971b7caf1d1d9f8a6cd529792b057d3b83a76cffdbcf504e808d9de90520184c`
7. `patches/rt64/rt64-p09-world-interpolation-guard.patch`: SHA256 `0527497cc43fd858f0f7ebeb425de1329d1df7ebee2cbe02ba6d632944490092`

Artefato binário auditado pós-plume:
- `cleanpin-build/TooieRecompiled`: SHA256 `965bbabbc3938ac833fd11b0ccb8b198f27e2108f95ebb51c405f4b4924aec05`

---

## 10. Veredito do Gate Técnico Independente (Haiku 5.5 pós-Sonnet)

Auditoria registrada em `/home/wesley/.cache/tooie-glitch-doc-audit-and-fix/technical-haiku-review.md` e `technical-gate.json`:
**Veredito: `passed: false`** (7 bloqueadores concretos):
1. **B1 (Critério circular)**: `crossStretchedTris = 0` avalia o próprio critério de recuo elementwise da guarda; como o lerp elementwise é convexo, matrizes chaveadas não esticam por construção. A métrica não pode ser citada como prova independente da correção.
2. **B2 (Mislabeled default path / escopo do fix)**: os scripts de teste (`run_ab.py:31` e `play.py:26`) configuram `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0`. No Tooie de produção, o padrão é definir `"1"` (`include/tooie_diag.h:102-104`). No caminho padrão de produção, a guarda de interpolação permanece **inerte**. Logo, o fix afeta unicamente o modo opt-in 60 FPS com FORCE=0; o run `nat1` foi incorretamente rotulado como "default path" porque também usou FORCE=0.
3. **B3 (Prova visual não-jogador ausente)**: sem par GPU casado com fase alinhada para atores não-jogadores (Klungo, Mumbo/Bottles). Linhas de auditoria não carregam identidade de objeto. O streak alaranjado de penas do Mumbo em `hA_3593.png` aparece em OFF, ON e `nat1`, sendo geometria de fonte da pose (`HYPOTHESIS`), sem prova visual de correção para não-jogadores.
4. **B4 (Vínculo de binário do crash `ong2`)**: o core do `ong2` proveio do build de árvore suja (`build/TooieRecompiled`), de modo que o hash exato daquele binário permanece `UNKNOWN`. A explicação de hang em SIGTERM é `STRONG_INFERENCE` (caminho de eventos SDL2 não inspecionado).
5. **B5 (Testes unitários pós-patch plume)**: os testes unitários (`cleanpin-tests.exits`) precedem a aplicação do patch do plume e não foram re-executados contra o binário final `965bbabb`.
6. **B6 (Distinção da árvore suja do RT64)**: as ~14k linhas não versionadas no RT64 continuam existindo no checkout de trabalho (`BanjoRecomp/lib/rt64`), pré-existentes de fases anteriores (P08/diagnósticos); a documentação deve registrar claramente que o binário validado deriva estritamente da cadeia de pin limpo com patches.
7. **B7 (Validação de gameplay pelo usuário)**: ausência de teste visual pelo usuário (`USER_OBSERVED`) a 60 FPS com o binário reproduzível.

---

## 11. Próximo passo singular (NEXT_SINGLE_ACTION)

`NEXT_SINGLE_ACTION`: Resolver os bloqueadores B2 e B5 do gate técnico validando o build de pin limpo (binário `965bbabb`) sob o escopo explícito de opt-in (`TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0`) e registrando o log limpo de execução dos testes unitários pós-plume, antes de qualquer teste de gameplay dirigido pelo usuário a 60 FPS.
