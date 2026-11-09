---
title: "Iluminação mais expressiva sem ray tracing"
description: "Plano de luz nativa, fontes locais e extensão raster para integrar personagens e cenário."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, lighting, rasterization, art-direction]
---

# Iluminação mais expressiva sem ray tracing

Estado das melhorias: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Base `PROVEN`: iluminação de modelos de jogador inclui
escrita de cores nos vértices, conforme [RE](../../reverse-engineering/systems/player-lighting.md). Isso não demonstra
um sistema completo de reluzir o mundo por pixel. Ver [guia](README.md).

## Direção e níveis de trabalho

Primeiro estudar ajustes de luz ambiente, direção e fontes nativas por região. Preservar personalidade dos mapas,
melhorar contraste junto a lâmpadas e integrar personagens às cores do ambiente. Luz noturna deve manter percurso,
perigos e texto legíveis. Não clarear todas as cavernas com a mesma luz solar.

Segundo nível candidato: iluminação complementar raster por pixel em materiais selecionados, com normais e posições
comprovadas. Luzes de tochas, lâmpadas industriais e lava são candidatas a fontes limitadas por alcance e por sala.
Sombras projetadas por mapa de sombras seriam estudo posterior dentro desta melhoria, sem dependência de ray tracing.

## Dificuldades e integração

- Inventariar o que vem de textura pintada, cor de vértice, fonte nativa e neblina. Não aplicar a mesma luz duas vezes.
- Confirmar normais utilizáveis: a cor de vértice usada para iluminação não deve ser reinterpretada como normal.
- Geometria pouco subdividida limita qualidade de luz por vértice. Luz por pixel exige novos dados/contratos no shader.
- Separar luz cosmética do estado de jogo; não alterar IA, ativadores ou cor de pistas necessárias a puzzles.
- Restringir fontes por ambiente para não iluminar através de paredes. Alcance radial sozinho não resolve oclusão.
- Preservar buffers por instância e por jogador, incluindo modelos auxiliares e LODs.

Mapas de sombras e reflexos por outra câmera podem precisar de geometria não submetida pela câmera principal.
Não desligar culling global nem reutilizar soluções refutadas para forçar essa geometria. Isso exige uma passagem
explicitamente estudada e orçamento separado. O primeiro piloto não depende de sombras projetadas modernas.

## Requisitos

| ID | Aceite proposto |
| --- | --- |
| LIT-01 | Distinguir componentes nativos e adicionais; A/B demonstra ausência de dupla iluminação. |
| LIT-02 | Fontes locais limitadas por sala, com personagens e materiais coerentes e sem vazamento através de paredes. |
| LIT-03 | Dois jogadores/instâncias preservam suas cores e recursos; sem sobrescrita ou alteração de foco global. |
| LIT-04 | Noite, fogo e interior mantêm gameplay legível, desligamento nativo e fallback por backend. |

Primeiro recorte: uma lâmpada em Glitter Gulch Mine, um personagem próximo e uma sala adjacente que não deve receber
a luz. Documentar primeiro as fontes nativas. Relacionados: [materiais](surface-materials.md),
[bloom](bloom.md), [sombras de contato](contact-shadows.md) e [dia/noite](day-night-cycle.md).
