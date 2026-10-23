---
title: "View volume & frustum culling"
description: "View volume layout 0x8012D500, vector generators, plane tests, native widescreen mode and native plane widening hook."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-10-22
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
