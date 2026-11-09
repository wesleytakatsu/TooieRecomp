---
title: "Vento em árvores, vegetação e objetos leves"
description: "Movimento visual seletivo por instância, com bases ancoradas, rajadas e preservação de colisão e animação."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, wind, vegetation, animation]
---

# Vento em árvores, vegetação e objetos leves

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Viabilidade média; identificar modelos, partes e representação de cada
vegetal ainda é `UNKNOWN`. Ver [guia](README.md) e [arquitetura](architecture.md).

## Efeito proposto

Copas, folhas, grama e plantas recebem movimento suave e pequenas rajadas. Tecidos, bandeirolas e placas leves
podem reagir quando cadastrados individualmente. Árvores grandes têm base/tronco quase fixos e amplitude maior
nas extremidades; objetos rígidos, edifícios e partes de puzzles não balançam automaticamente.

Um campo de vento do mundo fornece direção, intensidade e variação espacial a vegetação, chuva, folhas e áudio.
Usar fases por instância para evitar movimento sincronizado de todas as árvores, preservando correlação de uma
mesma rajada entre plantas próximas. Interiores sem corrente de ar têm amplitude nula ou perfil específico.

## Estratégias a comparar por recurso

| Representação observada | Técnica candidata | Informação necessária |
| --- | --- | --- |
| Modelo com esqueleto apropriado | Pose adicional em partes identificadas | Ossos, composição e animação nativa existentes |
| Vegetação estática com vértices adequados | Deformação raster limitada por máscara de rigidez e altura local | Pivô, limites, parte elegível e base ancorada |
| Sprite ou plano de folhagem | Movimento do conjunto ou shader específico | Orientação, transparência e ancoragem |
| Árvore integrada à geometria do mapa | Classificação e máscara por região/material, se viável | Separação confiável de solo, tronco, copa e objetos vizinhos |

Nenhuma estratégia está comprovada como universal. Um hash de textura de folhas não garante que toda geometria que
o usa é uma copa. O sistema `bawobble` documentado no [RE](../../reverse-engineering/systems/secondary-animation.md)
não é evidência de um sistema de vento para árvores; não reutilizá-lo por semelhança do efeito visual.

Deformar sempre a partir da pose/base original do frame, sem acumular deslocamentos. Preservar normais, pivôs,
histórico e buffers por instância. Atualizar limites visuais de maneira controlada onde necessário; não ampliar
ou desabilitar culling global. Animação nativa e vento devem compor sem tremor ou dobra duplicada.

Definir em qual espaço a deformação ocorre e qual pose apresentada recebe a amostra de vento. Não aplicar vento
duas vezes entre update guest e interpolação host, nem reutilizar deslocamento do quadro anterior como base.
Validar diferença entre 30/60/120 FPS com mesma sequência temporal; um relógio independente sozinho não prova que
geometria, normais e histórico usam a mesma amostra. Se não houver esse contrato, manter vegetação do recorte nativa.

Árvores escaláveis, plataformas e objetos usados em contato físico exigem amplitudes muito pequenas ou exclusão.
Colisão continua nativa: não mostrar tronco longe de onde o jogador colide. Nenhuma mudança de física de vento ou
empurrão no jogador faz parte desta proposta.

## Requisitos

| ID | Aceite proposto |
| --- | --- |
| WND-01 | Elegibilidade por instância/parte comprovada; solo e objetos rígidos nunca se deformam por engano. |
| WND-02 | Base ancorada, amplitude contida e rajadas coerentes; duas árvores não são cópias perfeitamente sincronizadas. |
| WND-03 | Sem deformação acumulada, sobrescrita entre instâncias, tremor com animação nativa ou corte nas bordas. |
| WND-04 | Colisão, plataformas e árvores de interação continuam coerentes; recursos incompatíveis são excluídos. |
| WND-05 | Frequência independente de FPS, estado compartilhado com partículas e controle de intensidade/desligamento. |
| WND-06 | Pose, espaço de deformação, normais e tempo do vento coerentes com apresentação interpolada, sem aplicação dupla. |

Primeiro recorte: uma árvore identificada, uma cópia do mesmo recurso e solo adjacente. Provar máscara e propriedade
antes de expandir para floresta. Pré-requisitos: campo cosmético de vento, identidade/partes e apresentação temporal
comprovados. [Folhas](airborne-leaves.md) e [chuva](rain.md) são consumidores relacionados; não precisam estar prontas
para testar a árvore. [Validação](validation.md) e [roteiro](implementation-roadmap.md).
