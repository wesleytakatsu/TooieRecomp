---
title: "Transform interpolation"
description: "Como o RT64 interpola as transforms AUTO do Banjo-Tooie a 60/120 FPS hoje: pareamento por ordem de desenho, gate de translação, guardas P09 e o caminho endpoint-CURRENT com Interpolation OFF."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-10-23
tags: [reverse-engineering, interpolation, rt64]
---

# Transform interpolation

Modelo atual (2026-10-23). Contrato das variáveis de ambiente: [[../../specs/Tooie/graphics-and-presentation]].

## Entrada do RT64 (`PROVEN`)

- O Tooie não marca matrizes com gEX: toda matriz WORLD, VIEW e PROJ chega como `G_EX_ID_AUTO`.
- Cada esqueleto é composto na CPU do jogo (pai × filho) e enviado como uma matriz WORLD por osso já composta. O RT64 não
  conhece a hierarquia.
- Muitos modelos (Banjo, por exemplo) são **segmentados**: cada membro é uma peça rígida sem triângulo atravessando a
  junta. Outros são skinned por vértice (triângulos com vértices em WORLDs diferentes).

## Pareamento quadro atual ↔ anterior (`GameFrame::matchScene`)

1. **Cena e câmera.** Cenas são agrupadas por view/proj. Antes, a câmera AUTO era sempre considerada a mesma entre quadros
   (`PROVEN`). Agora uma câmera AUTO cuja 3×3 é rotação × escala uniforme e que gira > 30° ou salta > 250 unidades entre dois
   quadros do jogo é um **corte**: a cena inteira daquele quadro não é interpolada (`rt64_p09_camera_cut.h`).
2. **Pareamento ordenado.** Para cada hash de draw call com a mesma quantidade de calls nos dois quadros, a k-ésima call atual
   pareia com a k-ésima anterior (o display list desenha um objeto sempre na mesma ordem). Esses pares têm prioridade.
   Candidatos cujo |det| muda 8× ou mais são rejeitados (`rt64_p09_ordered_match.h`).
3. **Alinhamento com lacunas.** Grupos com contagem diferente (objeto ou parte apareceu/sumiu, troca de LOD) são alinhados
   em ordem por programação dinâmica, maximizando pares com origem a ≤ 96 unidades.
4. **Fallback.** O que sobra usa o custo guloso original do RT64 (origem com predição de velocidade, orientação, tela).

## Interpolação e guardas (`TransformProcessor`, ordem de aplicação)

- `RigidBody::lerp`: translação em linha reta, rotação por slerp, cada WORLD sobre a própria origem.
- **Gate AUTO de translação** (`RigidBody::updateLinear`): segura a translação na pose atual quando a velocidade ≥ 5 e ≥ 10×
  a do quadro anterior (suspeita de teletransporte).
- **Translation coherence** (P09): libera holds em conjuntos articulados ligados por triângulos.
- **Segment coherence**: libera o hold de um WORLD cujos vizinhos em movimento (≤ 80 u) são interpolados e cujo salto é
  < 150 u (membros de modelos segmentados).
- **Joint coherence**: infere a hierarquia (a origem do filho fica fixa, drift < 0,5, no sistema de um vizinho ≤ 25 u) e
  prende a origem do filho ao pai interpolado, eliminando a abertura corda × arco nas juntas.
- **Spike guard**: se uma aresta interpolada ficar > 2× a atual e > 4 u maior, o objeto (união por triângulos e juntas) é
  exibido primeiro com interpolação elementwise (`lerpMatrix(prev, cur, t)`: cada vértice sobre a corda reta entre seus
  endpoints, sem arestas excedendo os comprimentos originais e mantendo o tempo interpolado). Se alguma aresta ainda
  ficar esticada sob matrizes elementwise (pareamento incorreto), o objeto é desenhado na pose atual (CURRENT). Switch:
  `TOOIE_TEST_P09_SPIKE_ELEMENTWISE=0` restaura a exibição direta em CURRENT (default ON).
- **Closure guard** (interpolação elementwise): default OFF — encolhia peças e abria juntas ao lado de vizinhos rígidos.

## Interpolation OFF

Com a opção desligada, a política fixa Manual 30 e o host liga o caminho **endpoint-CURRENT** do `RSPProcessCS`
(`requestHostEndpointCurrent`): a posição dos vértices vem dos buffers de endpoint na pose atual. O RT64 ainda faz o
matching de quadros nesse modo.

## Hipóteses refutadas (`REFUTED`)

- Custo de pareamento por geometria de vértices (pior que o original) e identidade por endereço do buffer de vértices
  (instâncias compartilham o buffer).
- Limiar de plausibilidade nos pares ordenados (sem efeito mensurável).
- Auditoria de pares por distância como métrica de pareamento errado (confundida por movimento rápido).
- Forçar desenho via `800DE498` para widescreen (não relacionado à interpolação; ver [[view-volume]]).
- Pular o matching do RT64 com Interpolation OFF mantendo os processadores de endpoint: o processo encerrou sem log numa
  troca de cena do attract (causa `UNKNOWN`).
- Timing de quadros do jogo como causa de tremores: o passo de VI por quadro (`0x80078F64`, contado pela thread de
  VI em `0x80079138`, gravado por `func_800151BC`) foi 2 VIs em todos os 660 quadros da demo 3, somando 1318 VIs iguais
  aos VIs do host; o runtime entrega VIs corretamente.
- Câmera AUTO sofrendo gate de translação: VIEW usa `G_EX_COMPONENT_INTERPOLATE` (elementwise) e nunca sofre gate.
- Variação do passo de locomoção originada no renderizador: após o fix o osso raiz de Mumbo é desenhado exatamente em
  t = 0,5 de cada par; a variação de passo de Mumbo e Banjo decorre da própria locomoção/animação do jogo.

## Ferramentas (default OFF)

- `TOOIE_DIAG_P09_ROD_CLASSIFY=<arquivo>`: triângulos esticados (linhas `TRI`/`WL`), aberturas de junta (`JG`/`JGW`) e
  objetos conectados (linhas `OBJ`, objetos ≥ 20 WORLDs: movimento do centroide `mot`, fração na corda `along`, desvio
  `perp`, erro `rms`, contagens R/C/E/O/H, e movimento/fração de origem do primeiro WORLD `rmot`/`rlerp`).
- `TOOIE_DIAG_P09_PAIR_AUDIT=<arquivo>`, `TOOIE_DIAG_P09_CAMERA_CUT=1`, `TOOIE_DIAG_P09_JOINT=1`.
- Attract automático com interpolação: `TOOIE_AUTO_START=1 TOOIE_TEST_INTERPOLATION_FPS=60`. Muitos glitches só aparecem
  jogando (transições de animação); o attract cobre pouco.
