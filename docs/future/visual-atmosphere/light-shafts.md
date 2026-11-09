---
title: "Feixes de luz localizados na atmosfera"
description: "Luz visível na poeira de minas e aberturas selecionadas, com técnicas raster e oclusão delimitada."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, light-shafts, atmosphere, rasterization]
---

# Feixes de luz localizados na atmosfera

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Extensão opcional de prioridade baixa. Viabilidade média para feixe autoral
em região delimitada; integração e custo ainda são `UNKNOWN`. Ray tracing permanece fora do escopo.
Ver [guia](README.md), [roteiro](implementation-roadmap.md) e [poeira da mina](mine-dust.md).

## Direção artística

Tornar luz visível em poucos pontos: lâmpada atravessando poeira numa galeria, luz entrando por uma abertura real,
ou luz subaquática onde fizer sentido. Preservar escuridão e profundidade do ambiente sem transformar toda sala
em um conjunto de fachos. A fonte e o espaço de propagação precisam existir e estar cadastrados.

## Alternativas raster e limites

| Caminho candidato | Uso inicial | Limite |
| --- | --- | --- |
| Volume/plano autoral com profundidade | Feixe curto, estável e delimitado numa mina | Precisa de formato, término e oclusão coerentes; evitar plano aparente em giro de câmera |
| Dispersão em tela com máscara de fonte/oclusores | Fonte e aberturas visíveis num recorte aprovado | Informação fora da tela e bordas podem causar desaparecimento/halos |
| Volume de iluminação mais geral | Estudo somente se versões simples forem insuficientes | Exige buffers, oclusão e orçamento maiores; não é pré-requisito do programa |

Uma técnica de dispersão por pós-processamento raster é descrita na
[referência NVIDIA](https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-13-volumetric-light-scattering-post-process).
Ela não comprova que dados e integração necessários já estejam disponíveis aqui. Escolher caminho pelo recorte,
não pela promessa de “luz volumétrica em todas as fases”.

Teto, parede e porta bloqueiam o feixe conforme a representação validada. Profundidade de câmera sozinha não resolve
oclusores fora da tela. Não projetar luz através de uma parede só porque o efeito é transparente; quando a máscara
for insuficiente, limitar volume ou desativar o recorte. O feixe não lança luz em objetos automaticamente:
essa resposta pertence ao [plano de iluminação](lighting.md), e o halo pertence ao [bloom](bloom.md).

Poeira, feixe e névoa dividem orçamento de transparência e contraste. Uma galeria pode ter feixe sem grãos caindo
continuamente. Evitar cintilação, faixas de mistura e ocultação de perigos. Cada viewport tem composição própria,
mas fonte, hora e contexto de mundo são únicos. Histórico, se utilizado, é descartado nos cortes/transições.

## Requisitos

| ID | Aceite proposto |
| --- | --- |
| LFX-01 | Fonte/abertura e volume cadastrados; efeito só em pontos artisticamente aprovados, sem atravessar paredes. |
| LFX-02 | Método declara limites de tela/volume, com fallback estável em giro, bordas, oclusão e duas viewports. |
| LFX-03 | Intensidade e densidade contidas, preservando gameplay e HUD, sem duplicar bloom/luz/névoa nativos. |
| LFX-04 | Custo combinado com partículas medido; opção independente e desligamento sem afetar iluminação existente. |

Primeiro recorte: uma lâmpada identificada e um feixe curto em Glitter Gulch Mine, após a base de poeira e a prova
raster. Comparar com/sem poeira e com uma parede bloqueando a região. [Validação](validation.md).
