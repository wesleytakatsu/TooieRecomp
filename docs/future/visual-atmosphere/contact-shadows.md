---
title: "Sombras de contato e oclusão ambiente raster"
description: "Volume discreto nos encontros entre superfícies, com limites explícitos de técnicas em tela."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, shadows, ambient-occlusion, rasterization]
---

# Sombras de contato e oclusão ambiente raster

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Viabilidade média; profundidade, máscaras e reconstrução ainda precisam
de prova. Ver [guia](README.md) e [arquitetura](architecture.md).

## Melhoria proposta

Dar volume discreto a encontros parede/chão, bases de objetos e reentrâncias, preservando sombras nativas.
Uma técnica de oclusão ambiente em tela é candidata. Ela aproxima bloqueio local da luz; não substitui uma sombra
solar completa nem possui informação de tudo o que está fora da imagem.

Confirmar profundidade linearizada e projeção corretas por viewport e caminho gráfico. Reconstruir normais ou
fornecê-las onde possível, definir raio em unidades coerentes e limitar intensidade. Excluir HUD, céu, partículas,
sprites inadequados e água transparente para evitar manchas e contornos falsos.

## Limites e cuidados

Geometria de baixo detalhe pode produzir faixas e halos. Profundidade de superfícies transparentes pode não
representar sua superfície visível. Ruído e filtragem temporal precisam de histórico próprio por câmera e descarte
em cortes, Warp e transições. Sombras nativas já pintadas nas texturas ou vértices não devem ser escurecidas em excesso.
O piloto pode usar filtragem espacial antes de assumir custo e dependência de histórico temporal.

## Requisitos

| ID | Aceite proposto |
| --- | --- |
| SHD-01 | Contato ganha volume moderado sem faixa preta em bordas, dupla sombra ou perda de pistas visuais. |
| SHD-02 | HUD, céu, sprites e transparências tratados explicitamente; sem oclusão falsa sobre água ou partículas. |
| SHD-03 | Resultado estável em movimento, split-screen, corte e resize; custo medido e opção de desligar. |

Primeiro recorte: canto parede/chão, objeto móvel e sprite na mesma cena, com visualização de profundidade/máscara.
Sombras projetadas por fontes pertencem ao [plano de iluminação](lighting.md), não são aceitas por este teste.
