---
title: "View volume & frustum culling"
description: "View volume layout 0x8012D500, vector generators, plane tests, native widescreen mode and native plane widening hook."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-11-01
tags: [reverse-engineering, view-volume, frustum, culling, widescreen]
---

# View volume & frustum culling

## Canonical view volume layout (`PROVEN`)

A engine de Banjo-Tooie centraliza os parâmetros de visualização e planos de corte do frustum de câmera através de uma estrutura de volume de visão:

- **Ponteiro global (`0x8012D500`)**: Mantém o ponteiro para o volume de visão da viewport ativa, atualizado como espelho da tabela de viewports `0x80127680 + 12 * idx`.
- **Layout de memória da estrutura**:
  - `+0x00`: Posição da câmera no mundo (`Vector3f`, coordenadas flutuantes x, y, z).
  - `+0x0C`: Vetor direcional forward da câmera (`Vector3f`).
  - `+0x24 + i * 0x10`: 4 planos de frustum $\{n, d\}$, cada um composto por um vetor normal tridimensional e uma distância escalar ($i = 0, 1$ representam os planos horizontais esquerdo e direito; $i = 2, 3$ representam os planos verticais superior e inferior).
  - `+0xE4`: Campo de visão vertical FOV (`40.0f`).
  - `+0xE8`: Proporção de aspecto da câmera (base original $4/3 \approx 1.3333334$, lido de `D_80125970`).
  - `+0xF8` / `+0x11C`: Vetores delimitadores horizontais.
  - `+0x104`: Vetor delimitador vertical.
- **Acessores estáticos**: Apenas 41 funções acessoras situadas na faixa `0x800E3900`–`0x800E4574` leem diretamente a global `0x8012D500`.

---

## Plane culling tests (`PROVEN`)

Os testes de visibilidade da geometria e dos modelos contra os planos do volume de visão são realizados por um conjunto restrito de funções especializadas:

1. **`0x800CAA24` (AABB float)**:
   Testa uma caixa delimitadora alinhada aos eixos (AABB) com coordenadas em ponto flutuante contra os planos de corte. Chamadores: `0x800E3D0C` (gates GeoCmd tipo 13 `0x800DDC28`/`0x800DDD14`, seções de mapa `gcmapsects`, `0x800D2F44`), segundo passo de `0x800E3DC0` e `0x800BC948` (cena em cache por mapa).
2. **`0x800CAB70` (AABB halfword)**:
   Testa caixas delimitadoras codificadas em inteiros de 16 bits (halfword). Único chamador `0x800E3DC0` (usado por `vpmodule`, `rtlight`, `chpoolkids`, `idtwinklecolour`).
3. **`0x800CACEC` (Bounding sphere)**:
   Testa uma esfera delimitadora tridimensional contra os planos de corte. É o teste fundamental utilizado para culling de modelos de atores, itens e adereços no renderizador [[model-renderer]].
4. **`0x800CAD9C` (Point test)**:
   Testa a inclusão de um ponto único no volume. Identificado como código morto não utilizado em chamadas de gameplay ativo.
5. **`0x800CAEA4` (Single vertical plane test)**:
   Testa objetos exclusivamente contra o plano vertical inferior/superior (plano 2).

---

## Vector and plane constructors (`PROVEN`)

A montagem do frustum é desacoplada em duas etapas temporais:

- **Cálculo dos vetores direcionais**:
  As funções `0x800CA1B0`, `0x800CA218` e `0x800CA2A8` calculam os vetores delimitadores horizontais a partir da proporção de aspecto lida do campo `+0xE8`:
  $$\theta = \text{aspecto} \times \frac{\text{FOV}}{2}$$
- **Montagem dos planos de corte (`0x800CAF34`)**:
  A cada frame, `0x800CAF34` constrói os 4 planos $\{n, d\}$ a partir dos vetores delimitadores previamente gerados, **sem ler diretamente** o campo de aspecto `+0xE8`.
- **Montagem da projeção gráfica (`0x800CB124`)**:
  A função `0x800CB124` constrói a matriz de projeção enviada ao pipeline gráfico utilizando o valor de proporção armazenado em `+0xE8`.

---

## Native widescreen mode (`PROVEN`)

Banjo-Tooie possui uma opção nativa de tela widescreen acessível no menu de opções do jogo:

- **Ponto de escrita**: A rotina `set_widescreen` (`0x800157E0`) armazena a escolha no halfword `0x800799B8`.
- **Consumidores do estado widescreen**:
  - `0x80015D14`: Multiplica a proporção da viewport pela constante de aspecto em `0x80041710` e encaminha ao volume de visão via `0x80015CE8` e `0x800CA510`.
  - `0x800E42F0`: Ajusta os limites da projeção ortográfica do HUD de $\pm 608$ para $\pm 810$.
  - `0x800DF944`: Aplica fator de escala horizontal no eixo X aos modelos dos elementos do HUD.
  - `gsattract_entrypoint_5`.
- **Natureza do modo nativo**:
  Este modo é **anamórfico** (`STRONG_INFERENCE`: projeção e ortho do HUD alargados no mesmo framebuffer 4:3): a imagem do mundo e os elementos do HUD são comprimidos na proporção 4:3 do sinal analógico de vídeo do Nintendo 64, projetados para serem esticados horizontalmente pela televisão receptora no modo 16:9.

---

## Native widescreen planes hook (`PROVEN` runtime)

No TooieRecomp, para evitar os defeitos visuais do modo anamórfico e permitir que o RT64 estenda o campo visual limpo sem distorções, foi implementado o mecanismo de planos nativos de culling (ver `p08-widescreen-native-planes-20261022`):

1. **Ajuste temporário de aspecto em `+0xE8`**:
   Durante a execução dos construtores de vetores `0x800CA1B0`, `0x800CA218` e `0x800CA2A8`, o valor de aspecto em `+0xE8` é temporariamente substituído por:
   $$\text{aspecto}_{\text{temp}} = \text{aspecto} \times \left( \frac{\text{aspecto\_alvo}}{\text{aspecto\_origem}} \right) \times \text{margem\_lateral}$$
   e restaurado ao valor original imediatamente após a saída dessas funções.
2. **Preservação de `0x800CB124`**:
   A montagem da projeção gráfica em `0x800CB124` continua lendo a base 4:3 original, deixando o controle de expansão de projeção exclusivamente a cargo do RT64.
3. **Efeito unificado sobre os testes**:
   Todos os 41 acessores e todas as funções nativas de teste (`0x800CAA24`, `0x800CAB70`, `0x800CACEC`) passam a avaliar objetos contra planos verdadeiramente posicionados em 16:9, eliminando o culling prematuro de itens, portas e paredes nas bordas laterais da tela sem alterar retornos de função $v_0$.
4. **Colapso de redundância**:
   Com os planos nativos ativos, as expansões manuais por teste de AABB e esfera fixam $k = 1.0$, prevenindo ampliações duplas.
5. **Proteção de replay de demos**:
   Quando a engine está reproduzindo um demo gravado do attract mode (`0x80127632 == 5`), a substituição de aspecto é ignorada, mantendo os planos estritamente em 4:3 para evitar dessincronização dos movimentos gravados do jogador.

## Lower-corner pickup visibility in widescreen

`PROVEN` by Windows user acceptance (2026-11-01): preserving lateral framebuffer bounds resolves the
reported angle-dependent cuts of pickups/enemies/shadows in the added widescreen strips of Jinjo Village.
The supplied cases include lower corners and positions near/above mid-height. The defect predates Object
Distance and also occurs with interpolation disabled. A 4:3 view omits the affected lateral strips.
This acceptance covers the tested Windows reproduction, not every map, projection, backend or Linux.

`PROVEN` native visibility boundaries: `func_800CACEC` rejects whole model spheres; GeoCmd AABB gates can
separately reject child subtrees. Sprite preparation `func_800E1C00` uses view depth, distance, size and an
optional callback, without an X/Y screen-edge test. The native-plane integration changes only lateral
aspect during its three builders. The vertical vector at `+0x104` in `func_800CA218` uses its angle
independently of `+0xE8`; `func_800CAF34` rotates it into planes at `+0x44` and `+0x54`.
Widening vertical planes or forcing model draws was not the fix for this defect.

`PROVEN` native render plumbing: `func_800E9F20` obtains the Gfx context through `func_800A7180`;
its entry a0 is not that context. The requested rectangle `{x,y,width,height}` occupies the first four
halfwords at `0x8012D5D0`; `func_800E7FB0` writes it. `func_800E7DAC` forwards it to `func_800E7A90`,
which emits a changed scissor and caches the applied rectangle in the following four halfwords through
`0x8012D5DE`. Viewport payloads are separate `G_MOVEMEM` data.

`CORRECTED`: a 104-byte submission at `[-7127,-920,4412]` must not be identified as a shadow from its
pointer or command count. Its captured vertices form a volumetric mesh (local Y 0..30); the paired
232-byte submission contains three animated quads. Those facts alone do not establish actor/resource roles.

### Native projection reload cache bounds

`PROVEN` static native code and isolated execution: `func_800BC948` uses two banks at
`0x801282E0 + 0xC8 + bank * 0x30`. Each contains a projection reload, `func_800BCC90` matrix command,
ENDDL, another projection reload and ENDDL. Native `func_800E4640` emits one command, consuming 40 of
48 bytes. Two inline P/V matrix commands per reload consumed 56 bytes and overwrote the following bank.

The integration restores split P/V through one G_DL pointing to a frame-local P LOAD / V MUL / ENDDL list
in the matrix arena. Segment 8 and cleared-cache fallback retain native sizes. The real native constructor
regression fails under the former implementation and checks both banks and surrounding bytes after correction.
`REFUTED` as a sufficient visual fix: user testing still showed the corner defect after this independent
overflow correction.

### Lateral framebuffer bounds after triangle submission

`PROVEN` host bookkeeping defect: `RSP::drawIndexedTri` submits indices before intersecting triangle
screen-space bounds with the native scissor. A triangle wholly outside that X range lost all recorded
Y extent even when the widened projection made it visible. `State::submitFramebufferPair` and
`fullSync` derive framebuffer height/write rows from those bounds.

Expand and Manual above 4:3 retain lateral triangles by using the native scissor X span for bounds
metadata, then applying the original Y intersection. Original, Manual at/below 4:3 and orthographic
draws retain native bookkeeping. Submitted indices, actual draw scissors, transforms, depth rules and
native visibility predicates remain unchanged. `TOOIE_TEST_WS_DRAW_BOUNDS=0` restores the previous
metadata policy at startup. See [[../../specs/Tooie/graphics-and-presentation#Widescreen framebuffer bounds]].

The regression executes real RT64 State/WorkloadQueue/RSP with synthetic geometry, without GPU or game:
a submitted right-strip triangle spanning Y 190..210 formerly left height at 180; the corrected path
records 210. It also covers left/right strips, upper write bounds, vertical scissor rejection,
depth-update policy, unchanged face indices, mode guards and the disabled policy.

`PROVEN` visual acceptance is limited to the Windows cases reported by the user. Exhaustive map/backend
coverage is unverified; Linux is `NOT_RUN`. The temporary guest-command observer, capture script, F8
trigger and diagnostic-only fixture were removed after acceptance. No visibility logging or counters
from that investigation remain in the production path.

## Cinematic projection compensation scope

`CORRECTED`: the former `intro_context` accepted **mode 3 OR map 395**, which did not restrict compensation to T_INTRO.
Mode 3 also covers ordinary story gameplay; it is not an intro identity. The implementation now requires
**mode 3 AND map 395**. Ordinary story maps bypass compensation regardless of their projection or window aspect.
See [[warp-and-player-position]] for the independently established game-mode semantics.

The projection hook `func_800E44FC` calls `tooie_tintro_projection_policy::apply` for perspective submissions.
For an accepted context, `apply` multiplies the first projection column when `abs(P11/P00)` lies in `[1.50, 2.10]`
and the target aspect exceeds 4:3. A 4:3 gameplay matrix bypasses this path. The native projection builder
`func_800CB124` already sets `P00 = volume[+0xF4] / volume[+0xE8]` and `P11 = volume[+0xF4]`; the ratio is a
projection parameter, not proof that a particular map or cinematic needs special compensation.

`PROVEN` in the standalone regression compiling production `apply` and `calculate_target_aspect_ratio`, with synthetic
RDRAM contexts/matrices and simulated window/config services: the old gate changes an ordinary story cinematic
projection and fails the unchanged-matrix assertion. The corrected gate passes. The T_INTRO formula remains active
only in its verified context; other modes, including attract mode, pass through. Original and 4:3 targets add no scale.
Expand continues deriving the target from current dimensions, including 16:10, 16:9, ultrawide and irregular windows;
dimensions at/below 4:3 and invalid sizes use the existing 4:3 baseline. The test changes dimensions and maps in one
process, checking both the intro exception and unchanged ordinary projections.

`STRONG_INFERENCE`: this unintended scope explains the user's single-player report that Jamjars's ability
explanation stretches horizontally and normal gameplay immediately restores the aspect. RT64's normal AUTO
projection adjustment remains in place after the added factor. **UNKNOWN**: the exact projection, viewport/scissor
and factor during the user's Jamjars event; no live event was captured. The fixture is not a rendered gameplay test.

The previous formula-only regression could not detect this scope error. It now executes the production policy.
This does not refute the historical T_INTRO-specific visual acceptance or reopen its separate edge-geometry residual.
`PROVEN` by the user's Windows visual acceptance (2026-11-01): the correction resolved the reported single-player
scene symptom. Acceptance does not establish the exact live projection values or an exhaustive visual matrix of
Original/Expand, window aspects and T_INTRO. Those broader visual checks remain unconfirmed; Linux is `NOT_RUN`.
