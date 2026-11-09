---
title: "Bloom seletivo para fontes luminosas"
description: "Halos contidos em fogo, magia, lava e lâmpadas, com máscara explícita e preservação de HUD."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, bloom, shaders, lighting]
---

# Bloom seletivo para fontes luminosas

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Viabilidade alta após identificação das fontes e integração da composição.
Ver [guia](README.md), [arquitetura](architecture.md) e [iluminação](lighting.md).

## Efeito e técnica candidata

Criar halo suave em emissores escolhidos: lâmpadas, magia, lava e fogo. Gerar máscara de emissão por material/desenho,
reduzir resolução, desfocar e compor com intensidade limitada. Manter núcleo, cor e contraste da fonte.
Limiar de brilho sozinho é insuficiente: neve, texto e superfícies claras também seriam selecionados.

Identificar usos de sprites e texturas compartilhadas antes de associar emissividade. A máscara deve respeitar
oclusão e transparência; um emissor atrás de parede não pode produzir halo indevido. Noite pode ajustar intensidade
suavemente, sem fazer a mesma lâmpada estourar quando a câmera entra numa sala escura.

Bloom não ilumina paredes por si só. A eventual luz lançada pela fonte pertence ao [plano de luz](lighting.md).
Não duplicar efeito de brilho nativo sem comparar o resultado composto.

## Requisitos

| ID | Aceite proposto |
| --- | --- |
| BLM-01 | Só fontes autorizadas emitem; neve, HUD, legendas e texto branco não ganham halo involuntário. |
| BLM-02 | Máscara respeita profundidade e transparência; sem brilho atravessando parede ou aparecendo entre viewports. |
| BLM-03 | Intensidade ajustável/desligável, estável entre horas, resolução, resize e corte; fallback sem bloom. |

Primeiro recorte: uma lâmpada e um elemento branco não emissivo na mesma cena. Comparar máscara isolada e composição
com o efeito nativo, inclusive em movimento. [Critérios compartilhados](validation.md).
