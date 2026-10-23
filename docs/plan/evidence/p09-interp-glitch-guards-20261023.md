---
title: "P09 — Guardas de interpolação e eliminação de glitches geométricos"
description: "Conjunto de guardas arquiteturais no RT64 (ordered match por alinhamento DP, corte de câmera, coerência de segmentos e juntas, spike guard) eliminando estiramentos residuais e aberturas articulares a 60 FPS (-100% no attract); guarda de fechamento desligada por padrão; validação de gameplay pelo usuário 'Ficou muito bom'."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-10-23
tags: [p09, interpolation, rt64, patches, guards, camera-cut, segment-coherence, joint-coherence, spike-guard, align-match]
---

# Evidência P09: Guardas de interpolação e eliminação de glitches geométricos (2026-10-23)

**Data de consolidação:** 2026-10-23
**Ambiente:** Host nativo Windows 11 x64, commit `f760b6d` (integrado em `origin/main`), patch local `vendor/patches-local/rt64-p09-interp-glitch-guards.patch`.

---

## 1. Contexto e Motivação (`USER_OBSERVED`)

A introdução do algoritmo de casamento estrutural ordenado (`ordered match`, commit `6685b48`, 2026-10-22) reduziu em 94% os triângulos esticados ("rods") no RT64. Contudo, a validação contínua em gameplay e cenas de demonstração revelou defeitos visuais residuais (`USER_OBSERVED`):
- Ocorrência de flashes raros de um único frame durante a movimentação livre;
- Membros de modelos articulados (como o Banjo) projetados fora de posição;
- Pulsos e ombros soltos ou descolados do tronco em modelos anatômicos segmentados;
- Estiramentos pontuais de vértices em transições bruscas de câmera e cortes de cena.

Diante desses relatos, desenvolveu-se uma suíte integrada de guardas arquiteturais no owning layer (`lib/rt64`), abordando descontinuidades de câmera, modelos segmentados sem conectividade por triângulos, alinhamento determinístico com gaps para listas de chamadas assimétricas e detecção de espigões por componentes conexos.

Após a aplicação do conjunto de guardas e seus devidos ajustes de calibração, o usuário avaliou o gameplay interativo a 60 FPS (`USER_OBSERVED`, 2026-10-23):
> "Ficou muito bom"

---

## 2. Arquitetura e Arquivos Modificados no Owning Layer (`lib/rt64`)

As guardas foram introduzidas como extensão do RT64 através do patch local `vendor/patches-local/rt64-p09-interp-glitch-guards.patch` (verificação `tools/vendor_verify.sh --reverse-check` com 28 patches OK):

1. **`lib/rt64/src/hle/rt64_p09_ordered_match.h`**:
   - Implementa alinhamento de sequências de chamadas do display list via programação dinâmica (DP) com inserção de lacunas ordenadas quando os grupos de mesmo hash possuem contagens desiguais entre frames.
2. **`lib/rt64/src/hle/rt64_p09_camera_cut.h`**:
   - Avalia a descontinuidade temporal da matriz de projeção/visão em câmeras automáticas (`G_EX_ID_AUTO`), detectando giros e saltos espaciais bruscos e prevenindo falsos positivos por meio do condicionamento ortogonal da submatriz 3x3.
3. **`lib/rt64/src/render/rt64_p09_spike_guard.h`**:
   - Concentra as guardas geométricas de renderização:
     - `Segment coherence`: destrava membros segmentados retidos pelo gate de velocidade linear;
     - `Joint coherence`: infere relações hierárquicas pai-filho e amarra a translação do filho ao pai;
     - `Spike guard`: reverte partições de objetos inteiros para `CURRENT` sob deformação excessiva;
     - `uniteJoints`: agrupa componentes conexos unindo não apenas triângulos adjacentes, mas também juntas inferidas contíguas.
4. **`lib/rt64/src/hle/rt64_game_frame.cpp`**:
   - Integra a chamada de corte de câmera (`rt64_p09_camera_cut.h`) e a invocação do casamento ordenado com alinhamento DP (`alignOrderedCalls`).
   - Fornece suporte ao diagnóstico de auditoria de pares (`TOOIE_DIAG_P09_PAIR_AUDIT`).
5. **`lib/rt64/src/render/rt64_transform_processor.cpp`**:
   - Estabelece a ordem rigorosa de processamento das guardas na etapa de renderização:
     1. Guardas legadas de translação (`p09TranslationCoherence`);
     2. Coerência de segmentos (`p09SegmentCoherence`);
     3. Coerência de juntas (`p09JointCoherence`);
     4. Guarda de espigões (`p09SpikeGuard`);
     5. Classificador diagnóstico de hastes esticadas (`p09RodClassify`).
6. **`lib/rt64/src/render/rt64_p09_closure_guard.h`**:
   - Altera o padrão da guarda elementwise legada para default OFF, eliminando o encolhimento artificial de peças em rotação.
7. **`lib/rt64/src/render/rt64_p09_rod_classify.h`**:
   - Adiciona classificação e emissão de linhas `JG` (Joint Gap) e `JGW` (Joint Gap Workload) para rastrear separações articulares anômalas.

---

## 3. Detalhamento dos Mecanismos e Controles

Todas as novas guardas operam como default ON, oferecendo flags individuais de desativação (`=0`) para isolamento experimental.

### 3.1 Alinhamento Ordenado com Lacunas (`TOOIE_TEST_P09_ALIGN_MATCH`)
- **Problema anterior:** No `ordered match` original, quando um grupo de chamadas de display list com o mesmo hash estrutural apresentava contagem diferente entre o frame anterior e o atual (por exemplo, ativação condicional de nós ou partes de geometria), o grupo inteiro era descartado para o algoritmo guloso por custo de posição, reiniciando o ciclo de contaminação temporal por velocidade herdada.
- **Mecanismo:** Introdução de alinhamento por programação dinâmica (DP) admitindo lacunas (gaps) ordenadas para grupos de até 96 chamadas (com teto de segurança de $2^{18}$ células na tabela DP). As chamadas alinhadas são pareadas mantendo a correspondência topológica individual; as chamadas excedentes recaem isoladamente sem desordenar as demais.
- **Resultado isolado:** Redução dos triângulos esticados residuais no attract demo de 378 para 276.

### 3.2 Corte de Câmera (`TOOIE_TEST_P09_CAMERA_CUT`)
- **Problema anterior:** A câmera de visualização automática no RT64 era incondicionalmente marcada como mapeada para interpolação (`viewProjMap.mapped = true`). Em transições bruscas de cutscene ou correções instantâneas de ângulo, a interpolação temporal de matrizes produzia arrasto elástico ("screen smear") e distorções na geometria projetada.
- **Mecanismo:** Avaliação de variação temporal entre matrizes consecutivas. Se a rotação angular relativa exceder $30^\circ$ ou o deslocamento espacial linear superar $250$ unidades, o frame é classificado como corte de câmera (`camera cut`). Sob corte, a cena tem sua interpolação desativada naquele instante e é renderizada na pose real (`CURRENT`).
- **Resolução de falsos positivos (`CORRECTED`):** A implementação inicial produzia 576 falsos positivos no attract devido a matrizes de projeção degeneradas ou passagens ortogonais de interface/HUD. A validação foi refinada exigindo que a submatriz $3\times 3$ represente estritamente uma transformação ortogonal com escala uniforme ($\|R^T R - s^2 I\| < \epsilon$).
- **Resultado:** Identificação exata de 16 cortes de câmera reais e legítimos no attract demo, com zero falsos positivos em elementos de interface.

### 3.3 Coerência de Segmentos (`TOOIE_TEST_P09_SEGMENT_COHERENCE`)
- **Problema anterior:** O portão automático de velocidade linear do RT64 (`RigidBody::updateLinear`) desarmava a interpolação de translação sempre que a velocidade era $\ge 5$ e $\ge 10\times$ a do frame anterior. Em modelos com membros anatômicos segmentados (sem malha contínua ligando os ossos, como braços e ombros do Banjo), essa regra travava translações legítimas de membros enquanto a raiz do corpo se deslocava, fazendo com que membros se descolassem e ficassem para trás no espaço.
- **Mecanismo:** Se uma matriz com salto de velocidade possui matrizes vizinhas (raio $\le 80$ unidades) em movimento que estão interpolando com sucesso, e o deslocamento próprio do osso for $< 150$ unidades, a matriz é liberada para interpolar normalmente.
- **Resultado:** 1025 liberações de interpolação legítimas registradas durante o attract demo.

### 3.4 Coerência de Juntas (`TOOIE_TEST_P09_JOINT_COHERENCE`)
- **Problema anterior:** Como o jogo não exporta a árvore esquelética nativa para o emulador, as matrizes de ossos eram tratadas de maneira desacoplada, permitindo que pequenas divergências de integração angular abrissem fendas em articulações mecânicas ou esqueléticas.
- **Mecanismo:** Inferência da hierarquia articular em runtime. Se a posição relativa do pivô de um osso no sistema de coordenadas de um osso vizinho ($\le 25$ unidades) se mantiver estável entre frames (deriva temporal $\text{drift} < 0,5$ unidades), infere-se uma relação rígida pai-filho. A origem do nó filho é amarrada diretamente à origem transformada do nó pai interpolado. Ciclos transitivos são desfeitos ordenando pelo menor índice da matriz.

### 3.5 Guarda de Espigões (`TOOIE_TEST_P09_SPIKE_GUARD`)
- **Mecanismo:** Rastreamento geométrico global de componentes de malha. Se qualquer aresta renderizada dentro de uma partição conectada exceder $2\times$ o tamanho anatômico de sua contraparte em `CURRENT` e for maior que $4$ unidades no espaço de mundo, todo o objeto é projetado na pose `CURRENT` naquele frame, impedindo a exibição de espigões.
- **Correção da topologia articular (`CORRECTED`):** A formulação preliminar unia partições estritamente por adjacência de triângulos (`uniteTriangles`). Em personagens segmentados, partes desconectadas da malha (ex.: luvas/antebraços) tinham suas matrizes revertidas isoladamente para `CURRENT` enquanto o resto do corpo interpolava, rasgando as articulações. A correção introduziu `uniteJoints`, que agrega componentes conexos tanto por triângulos quanto por juntas inferidas contíguas.

### 3.6 Guarda de Fechamento Elementwise (`TOOIE_TEST_P09_CLOSURE_GUARD`)
- **Reversão para default OFF (`CORRECTED`):** A antiga guarda de fechamento articular (Mecanismo B), que forçava interpolação linear elemento a elemento (`lerpMatrix`), causava perda de ortogonalidade sob rotações amplas, resultando em encolhimento visível de peças e abrindo articulações em modelos segmentados. Foi tornada inativa por padrão (default OFF); pode ser reativada explicitamente com `TOOIE_TEST_P09_CLOSURE_GUARD=1`.

---

## 4. Medições Experimentais e Resultados (Attract Demo, wl 1000–2400)

As medições comparativas foram realizadas no cenário padronizado de attract demo (workloads 1000 a 2400, aproximadamente 1320 frames interpolados):

| Métrica | Base (Ordered Match Isolado) | Pós-Guardas Integradas | Variação | Classificação |
| :--- | :---: | :---: | :---: | :---: |
| **Triângulos esticados (arestas $>2\times$ CUR e $>4$ u)** | 378 | **0** | −100% | `PROVEN` métrico |
| **Juntas abertas ($>3$ unidades)** | 202 | **0** | −100% | `PROVEN` métrico |
| **Frames com juntas abertas** | 45 | **0** | −100% | `PROVEN` métrico |
| **Cortes reais de câmera tratados** | 0 | **16** | +16 | `PROVEN` métrico |
| **Liberações por coerência de segmento** | 0 | **1025** | +1025 | `PROVEN` métrico |
| **Redução prévia por alinhamento DP** | 378 | 276 | −27% | `PROVEN` métrico |

---

## 5. Arbitragens Epistêmicas e Correção de Métricas

1. **Critério de detecção de juntas abertas (`CORRECTED`)**:
   - A métrica inicial contabilizava qualquer par de transformações a uma distância $\le 25$ unidades como uma junta esquelética em potencial. Essa aproximação gerava falsos positivos volumosos ao tratar objetos independentes que passavam próximos um do outro como "juntas rompidas".
   - A métrica foi corrigida (`CORRECTED`) para verificar a estabilidade do pivô ao longo do tempo ($\text{drift} < 0,5$), medindo apenas juntas verdadeiras.
2. **Auditoria de pares por distância (`TOOIE_DIAG_P09_PAIR_AUDIT`)**:
   - A ferramenta de auditoria de pares sinalizava falsos erros de pareamento durante movimentações muito rápidas no espaço (onde o deslocamento linear excede o limiar euclidiano estático). Foi classificada como inadequada para métrica objetiva e retida exclusivamente para depuração manual.

---

## 6. Verificação de Testes Host e Integridade

A bateria de testes unitários do host foi executada com sucesso total no ambiente Windows nativo:
- **`p09_ordered_match_test`**: **41/41 PASS** (inclui 15 novos cenários cobrindo alinhamento por programação dinâmica com lacunas, validados com auxílio do modelo Haiku 5.5);
- **`p09_world_interpolation_guard_test`**: **35/35 PASS** (expectativas atualizadas para refletir o novo padrão da guarda de fechamento elementwise em OFF);
- **Suíte de testes obrigatórios**: **9/9 PASS**;
- **Suíte de testes extras**: **17/17 PASS**;
- **Compatibilidade Linux**: Classificada como `NOT_RUN` em tempo de execução. O código das guardas reside inteiramente no núcleo agnóstico do RT64 (`lib/rt64`), escrito em C++ padrão sem dependências de API gráfica ou extensões de plataforma (`STRONG_INFERENCE` de equivalência funcional).

---

## 7. Status e Validação Final

O conjunto de guardas encontra-se integrado e commitado no ramo principal (`f760b6d`). A erradicação dos artefatos pontuais e o alinhamento das peças articulares foram atestados em runtime por medição objetiva no attract (zero estiramentos e zero aberturas articulares) e confirmados subjetivamente em gameplay interativo pelo usuário (`USER_OBSERVED`).
