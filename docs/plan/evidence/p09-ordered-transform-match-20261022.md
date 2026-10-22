---
title: "P09 — Causa raiz dos glitches de interpolação e correção ordered match"
description: "Causa raiz dominante dos glitches de interpolação identificada no pareamento WORLD guloso do RT64 (94% dos triângulos esticados); correção ordered match por identidade estrutural de chamadas do display list (-94% de estiramento); guardas mantidas; testes host PASS; validação de gameplay pendente."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-10-22
tags: [p09, interpolation, rt64, patches, ordered-match, transform-matching, rod-classify, root-cause, technical-gate]
---

# Evidência P09: Causa raiz dos glitches de interpolação e correção ordered match (2026-10-22)

**Data de medição e ambiente:** 2026-10-22, host nativo Windows 11 x64, HEAD `18bcd12` com alterações locais de trabalho.

---

## 1. Objetivo

Investigar e provar a causa raiz definitiva dos glitches geométricos de interpolação em alta taxa de quadros ("rods", hastes esticadas e partes destacadas de modelos articulados, como as penas da crista da Kazooie e a aba da mochila do Banjo) sob `Interpolation` ON (60 FPS), implementar ferramenta diagnóstica de discriminação causal, avaliar e arbitrar hipóteses concorrentes, desenvolver a correção arquitetural no owning layer (`lib/rt64`), mensurar a redução objetiva de defeitos, auditar a necessidade das guardas preexistentes e documentar o estado de prontidão e resíduos para o gate técnico P09.

---

## 2. Causa raiz (`PROVEN` por medição em runtime + leitura de fonte)

A causa raiz dominante dos glitches de interpolação foi comprovada no algoritmo de casamento temporal de transformações do RT64:

1. **Ausência de anotação de matrizes (gEX):** O Banjo-Tooie não marca matrizes com gEX IDs (todas as transformações WORLD são submetidas como `G_EX_ID_AUTO`).
2. **Custo guloso dominado pela origem:** O RT64 realiza o pareamento temporal entre as matrizes WORLD do frame atual e do frame anterior em `GameFrame::matchScene` (`lib/rt64/src/hle/rt64_game_frame.cpp`) de forma puramente gulosa, ordenando candidatos pela função `computeTransformMatch`:
   $$\text{custo} = |\Delta \text{origem predita}| + \text{orientação (máx 6)} + \text{distância em tela}$$
   onde a origem predita é $\text{prevPos} + \text{linearVelocity}$. A diferença de origem domina completamente o valor do custo.
3. **Indistinção de ossos com pivô compartilhado:** Ossos que compartilham o mesmo pivô anatômico (por exemplo, as diversas penas da crista da cabeça da Kazooie) ou instâncias múltiplas do mesmo pedaço de malha possuem origens espaciais coincidentes ou quase idênticas. Por conseguinte, apresentam custos quase idênticos, tornando-se indistinguíveis perante a heurística gulosa.
4. **Retroalimentação em cascata por velocidade linear herdada:** A velocidade linear (`linearVelocity`) herdada de um pareamento incorreto contamina a predição espacial no frame seguinte, invertendo a decisão de custo em cascata: em 794 casos fortes de pareamento incorreto medidos, 355 seriam escolhidos corretamente se a predição de velocidade estivesse desligada.
5. **Manifestação em "rods" e espigões:** Um par temporal errado força um osso a ser interpolado geometricamente entre as posições de dois ossos anatomicamente diferentes, esticando as arestas dos triângulos conectores em hastes pontiagudas gigantescas ("rods") e deslocando partes rígidas.
6. **Ponto cego das iterações anteriores:** As abordagens anteriores (guardas de coerência de translação, guarda de fechamento articular e o auditor `TOOIE_DIAG_P09_WORLD_LERP_AUDIT`) não convergiram porque adotavam $\max(\text{aresta PREV}, \text{aresta CURRENT})$ como comprimento de referência. Quando o endpoint `PREV` proveio de um par errado, a aresta `PREV` já se encontrava esticada no espaço, mascarando o defeito perante a verificação da guarda.

---

## 3. Ferramenta de medição (`TOOIE_DIAG_P09_ROD_CLASSIFY`)

Para discriminar as causas com precisão métrica sem depender de arestas `PREV` contaminadas, foi introduzida e mantida a ferramenta diagnóstica default-OFF:
- **Variável de controle:** `TOOIE_DIAG_P09_ROD_CLASSIFY=<arquivo>`
- **Arquivo no owning layer:** `lib/rt64/src/render/rt64_p09_rod_classify.h`, invocado em `TransformProcessor::process` logo após a execução das guardas.
- **Critério de classificação:** Inspeciona todos os triângulos de workloads interpolados cuja aresta renderizada final exceda $2\times$ o comprimento da aresta em `CURRENT` e supere $4$ unidades no espaço de mundo. A aresta em `CURRENT` é a única referência confiável de tamanho anatômico. É uma ferramenta estritamente read-only (nunca altera a renderização).
- **Categorias por triângulo esticado:**
  - `I`: Um só WORLD envolvido (defeito intrínseco à interpolação da própria matriz).
  - `U`: Mistura WORLD não mapeado com WORLD mapeado.
  - `P`: Aresta PREV discorda da aresta CURRENT (inconsistência de pareamento temporal ou geometria entre frames).
  - `C`: Mistura WORLD retido em CURRENT com WORLD interpolado.
  - `N`: Resto (lei de interpolação matemática pura).
- **Formato de saída:** Emite uma linha `WL` para cada workload interpolado e até 8 linhas `TRI` com as propriedades dos triângulos mais severos.

---

## 4. Medições base

A medição base foi realizada na cena de attract demo, com inicialização automática e interpolação ativa a 60 FPS no intervalo de workloads 1000–2400 (~1320 frames interpolados):

**Comando de execução (base / pareamento original):**
```powershell
$env:TOOIE_AUTO_START='1'; $env:TOOIE_TEST_INTERPOLATION_FPS='60'; $env:TOOIE_TEST_P09_ORDERED_MATCH='0'; $env:TOOIE_DIAG_P09_ROD_CLASSIFY='rods_base.txt'; build-win\TooieRecompiled.exe
```

**Resultado base:**
- Total de triângulos esticados detectados: **6640**
- Distribuição por categoria:
  - **P = 6268 (94%)** — confirmação categórica de que a inconsistência de pareamento temporal PREV↔CURRENT é a causa amplamente dominante dos artefatos;
  - U = 178;
  - I = 167;
  - N = 27.

---

## 5. Alternativas testadas e refutadas (`REFUTED`)

Duas hipóteses alternativas de pareamento foram testadas experimentalmente nesta sessão e categoricamente refutadas:

1. **Custo por geometria de vértices (`REFUTED`):**
   - *Conceito:* Avaliar a compatibilidade de transformação calculando o deslocamento geométrico médio dos vértices do osso transformados pela matriz candidata anterior.
   - *Resultado:* Piorou o estiramento em cerca de $4\times$, elevando os triângulos esticados para **24215**. Com modelos de N64 contendo apenas 3 a 4 vértices por osso, ossos anatomicamente distintos "encaixam" geometricamente no espaço anterior por pura coincidência dimensional.
2. **Identidade pelo endereço RDRAM do buffer de vértices (`REFUTED`):**
   - *Conceito:* Utilizar o ponteiro de memória RDRAM do buffer de vértices submetido como identificador estável de osso.
   - *Resultado:* O buffer RDRAM não é exclusivo de um osso; o mesmo buffer de vértices é frequentemente reaproveitado sob matrizes distintas para instanciar partes de malha repetidas. Na medição, **720 de 1073 casos fortes de pareamento errado compartilhavam exatamente o mesmo endereço RDRAM**.

---

## 6. Correção no owning layer (ordered match)

A solução arquitetural definitiva explora a estrutura temporal determinística do motor gráfico:
- **Módulos:** Novo cabeçalho puro `lib/rt64/src/hle/rt64_p09_ordered_match.h` e integração em `GameFrame::matchScene` (`lib/rt64/src/hle/rt64_game_frame.cpp`), incorporados como patch local vendorizado no RT64.
- **Identidade estrutural de chamadas:** O display list emite as draw calls de um modelo na mesma ordem sequencial a cada frame. Para cada hash de chamada (`Call::hash`) que apresente exatamente o mesmo número de ocorrências nos dois frames, a $k$-ésima chamada atual pareia deterministicamente com a $k$-ésima chamada anterior ("ordered pairs"). Isso se aplica tanto a chamadas de matriz única quanto a chamadas multi-WORLD (deslocamento a deslocamento: `minWorld + w`).
- **Particionamento em dois níveis:** Implementado com `std::stable_partition`: candidatos pertencentes a pares ordenados recebem prioridade de atribuição primeiro; em seguida, os candidatos restantes recaem no fallback de custo original. Ambos os grupos mantêm internamente a ordenação pelo custo de transformação.
- **Filtro de compatibilidade de escala (`scaleCompatible`):** Rejeita imediatamente candidatos cujo determinante do bloco 3x3 ($|\det 3\times 3|$) varie por um fator de $8\times$ ou mais (correspondendo a uma variação de escala linear $\ge 2\times$ em um único frame), identificando troca abrupta de objeto.
- **Controle de ativação:** Default **ON**. A variável de ambiente `TOOIE_TEST_P09_ORDERED_MATCH=0` desativa a correspondência ordenada e restaura o algoritmo guloso original para testes comparativos A/B.

---

## 7. Resultados e teste de sensibilidade

**Comando de execução (com a correção ordered match):**
```powershell
$env:TOOIE_AUTO_START='1'; $env:TOOIE_TEST_INTERPOLATION_FPS='60'; $env:TOOIE_DIAG_P09_ROD_CLASSIFY='rods_fix.txt'; build-win\TooieRecompiled.exe
```

**Métricas na mesma janela temporal (workloads 1000–2400, ~1320 frames interpolados):**
- Total de triângulos esticados: **378** (contra 6640 na base), representando uma redução de **−94%** (`PROVEN` métrico).
- Distribuição categórica na correção:
  - P = 338;
  - U = 28;
  - N = 12;
  - I = 0.
- **Teste de sensibilidade de limiar:** Foi avaliado um limiar de plausibilidade com diferentes margens de folga (slack 1, 4 e $\infty$). Os resultados de classificação de triângulos foram estritamente idênticos em todas as configurações, motivo pelo qual a lógica de slack foi removida para manter a simplicidade da implementação.

---

## 8. Necessidade das guardas existentes

A introdução do ordered match não elimina a necessidade das guardas de interpolação desenvolvidas nas iterações anteriores:
- Ao executar a correção com as guardas desligadas:
  ```powershell
  $env:TOOIE_TEST_P09_TRANSLATION_COHERENCE='0'
  $env:TOOIE_TEST_P09_CLOSURE_GUARD='0'
  $env:TOOIE_TEST_P09_GATE_SPEED_ONLY='0'
  ```
  o número de triângulos esticados sobe imediatamente para **N = 7939**!
- Causa: O gate AUTO original de translação ainda marca cerca de ~17% das matrizes WORLD como teleporte devido à amplificação de cosseno em reversões de trajetória $\ge 90^\circ$, congelando a translação enquanto a rotação continua interpolando.
- O Mecanismo A continua **`PROVEN`** e essencial: o ordered match corrige o pareamento de juntas esqueléticas entre frames, enquanto as guardas de coerência de translação e velocidade garantem a integridade cinemática das transformações corretamente pareadas.

---

## 9. Auditoria visual cega (`STRONG_INFERENCE`)

Para verificar o impacto visual sem viés de confirmação, foi conduzida uma auditoria cega:
- **Amostragem:** 240 capturas de tela da janela (120 da base sem ordered match + 120 da correção com ordered match), cobrindo os mesmos intervalos do attract demo (workloads ~1590, ~1880, ~2050 e ~2250).
- **Procedimento:** As 240 imagens foram renomeadas com hashes aleatórios, embaralhadas e avaliadas de forma independente por 4 agentes Claude Sonnet 5.5.
- **Resultados:**
  - Base (OFF): Detectado 1 glitch evidente de alta severidade (severidade 3: espigão alaranjado grande projetando-se da crista da Kazooie no wl 1698) + 3 casos incertos.
  - Correção (ON): 0 glitches claros de alta severidade detectados + 6 casos incertos de severidade 1 (a maioria classificada pelos auditores como "provavelmente normal").
  - Inspeção direta no mesmo instante temporal: No workload correspondente da correção (wl 1699), o espigão alaranjado está completamente ausente.
- **Limitação estatística:** Em virtude do tamanho reduzido dos personagens no enquadramento das capturas e do baixo poder estatístico amostral, o suporte visual classifica-se formalmente como **`STRONG_INFERENCE`**, enquanto a redução quantitativa métrica (−94%) é **`PROVEN`**.

---

## 10. Testes de validação

A integridade do código e dos testes foi validada nas seguintes frentes:
1. **Novo teste unitário de host:** `tests/p09_ordered_match_test.cpp` (escrito por Claude Haiku 5.5) cobrindo 26 cenários de pareamento estrutural: **26/26 PASS**.
2. **Teste unitário de guardas:** `p09_world_interpolation_guard_test`: **35/35 PASS**.
3. **Execução completa de testes no Windows:**
   ```powershell
   tools\windows\Build-Tooie.ps1 -Stage Test -AllTests
   ```
   Resultado: `tests: required 9/9 PASS; extra 16/16 PASS; skipped 5` (alvos de driver de vídeo com skip documentado).
4. **Verificação de vendorização:**
   ```bash
   tools/vendor_verify.sh --reverse-check
   ```
   (executado com `LC_ALL=C` no Git Bash): **`VENDOR VERIFY OK`** (27 patches integrados verificados com sucesso).
5. **Binário Windows:** `build-win\TooieRecompiled.exe` com SHA256 `6F39CC23FA3DBC8969BC77F0917823F765063495197C593ED62DE32132855DE4`.

---

## 11. Patches e integridade de vendor

As alterações foram formalizadas como patches locais sob governança de vendor:
- `vendor/patches-local/rt64-p09-ordered-transform-match.patch` (SHA256: `e17ec7d6a71bd2108a41a77d68740ca868818ba703173a3fe667f8a89d52b147`)
- `vendor/patches-local/rt64-p09-rod-classify-diagnostic.patch` (SHA256: `30ae18adc948c74e5406faf1cc701b6a3b2a2947833f8b4148933d43936ad3cf`)
- Registros atualizados: `vendor/patches-applied/ORDER.tsv`, `vendor/UPSTREAMS.tsv` (`LOCAL:`) e `vendor/MANIFEST.sha256`.

---

## 12. Resíduos e pendências

1. **Triângulos esticados residuais (P = 338):** Diversos casos apresentam um deslocamento consistente de índice contíguo (ex.: chamada 39→40, 40→41). Levanta-se a hipótese (`HYPOTHESIS`) de que isso decorra de um "pop" de osso real ou ativação de nó esquelético do próprio motor do jogo entre dois frames consecutivos.
2. **Grupos de hash com contagem divergente:** Quando a quantidade de chamadas de um dado hash varia entre frames, o pareamento ordenado não pode ser aplicado para aquele hash e as chamadas recaem no fallback de custo original. Um próximo passo possível para refinamento é a implementação de alinhamento de sequência.
3. **Comportamento de burst sob D3D12:** O uso de `TOOIE_TEST_INTERP_BURST_AT_WORKLOAD` no Windows com backend D3D12 provocou o encerramento imediato do processo logo após a requisição (observado 1×, classificação **`UNKNOWN`**, não investigado nesta sessão).
4. **Validação de gameplay pelo usuário:** Teste interativo em gameplay real conduzido pelo usuário (`USER_OBSERVED`) segue **PENDENTE**.
5. **Validação no Linux:** Alterações compartilhadas ainda NÃO foram validadas em ambiente Linux (o usuário executará a validação no PC Linux).
6. **Estado de versionamento:** Nenhuma alteração foi commitada no git nesta sessão (worktree sujo intencionalmente mantido).
7. **Gate técnico formal:** O gate técnico formal P09 permanece **não fechado** (`passed: false` historicamente) até a homologação de gameplay e validação multiplataforma.

---

## 13. Teste de gameplay pelo usuário

`USER_OBSERVED` (2026-10-22): jogando com Interpolation ON neste build (exe SHA256 `6F39CC23...`), o usuário relatou que os
rods/espigões "sumiram ou quase sumiram". Relato qualitativo; gate técnico formal P09 continua não fechado.

## 14. Commit, push e Linux

**Commit/push e Linux (2026-10-22)**: correção commitada e enviada (`6685b48` em `origin/main`). Linux: análise de código apenas (sem build/teste) — a causa e a correção estão em código RT64 comum (`GameFrame::matchScene`, header `rt64_p09_ordered_match.h`), C++ padrão sem `#ifdef` de plataforma e independente da API gráfica; o efeito deve valer no Linux (`STRONG_INFERENCE`). Compilação GCC/Clang, `p09_ordered_match_test` e efeito visual no Linux: `NOT_RUN` (usuário valida no PC Linux).

## 15. NEXT_SINGLE_ACTION

retomar o resíduo P08 `WIDESCREEN_EDGE_GEOMETRY` — relato do usuário (`USER_OBSERVED`, 2026-10-22): em widescreen, itens somem nas bordas da tela e algumas paredes também, raro, visto no Gameplay Demo 2 e principalmente na fase dos dinossauros (Terrydactyland); ver fase 08. A validação Linux da P09/P11 segue com o usuário em paralelo.
