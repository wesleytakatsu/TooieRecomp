---
title: "Chuva contextual e superfícies molhadas"
description: "Precipitação, respingos, abrigo, molhado e áudio por região exposta, com transições suaves."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, weather, rain, atmosphere]
---

# Chuva contextual e superfícies molhadas

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Viabilidade média/alta para chuva localizada; exposição ao céu e integração
de partículas ainda precisam de prova. Ver [guia](README.md), [perfis](scenario-profiles.md) e [arquitetura](architecture.md).

## Comportamento proposto

Chuva ocasional nos exteriores cujo perfil permita, com entrada/saída gradual, inclinação conforme vento, respingos
discretos e som correspondente. A intensidade é compartilhada pelo mundo; cada câmera observa sua própria região.
Chuva não é obrigatória em todas as fases e não deve atingir interiores, superfícies submersas ou regiões incompatíveis.

Criar inicialmente estados seco, transição e chuva moderada. Tempestades e relâmpagos não são requisito do piloto.
Se flashes forem acrescentados no futuro, precisam de opção independente para redução/desativação e intensidade
limitada. Clima não muda aderência, vida, fogo, puzzles, visibilidade de NPCs ou regras de nado nesta proposta.

## Exposição e técnica candidata

Usar volumes delimitados e exclusões de abrigo, com superfícies de término cadastradas. Comparar máscara de exposição
autoral, representação de tetos e consultas de colisão nativas quando seguras. Escolher após inventário; não assumir
que profundidade da câmera identifica telhados fora da imagem. Consultas de colisão do jogo não dependem de ray tracing.

Partículas próximas à câmera são candidatas para reduzir custo, mas o volume deve estar ancorado ao mundo para não
acompanhar cada rotação de maneira artificial. Respeitar profundidade, portas, aberturas e salas adjacentes.
Gotas terminam em chão, teto inferior ou superfície de água correta; respingos não devem surgir no ar ou no piso
de outro andar. Interseção dos volumes de duas câmeras não pode dobrar densidade nem áudio.

Molhado é evolução adicional de material: escurecimento moderado e brilho controlado em superfícies elegíveis,
com tempo próprio de secagem. Área coberta não fica molhada só porque chove no mapa. Guardar estado por região/material,
limitar memória e definir política na troca de sala. Poças geométricas e acúmulo físico estão fora do primeiro marco.

## Requisitos

| ID | Aceite proposto |
| --- | --- |
| RAI-01 | Chuva somente em perfis e regiões aprovados, com intensidade suave e relógio independente de FPS. |
| RAI-02 | Teto, cobertura parcial e submersão bloqueiam precipitação corretamente, inclusive com J1/J2 em meios diferentes. |
| RAI-03 | Gotas e respingos respeitam profundidade e superfície de término; sem duplicação entre câmeras. |
| RAI-04 | Molhado/secagem afetam só materiais elegíveis e expostos; áudio acompanha abrigo sem substituir BGM. |
| RAI-05 | Sem efeito sobre gameplay; densidade ajustável, limpeza e fallback por plataforma demonstrados. |

Primeiro recorte: exterior com um abrigo, uma porta e água ou piso identificável. Provar chuva sob céu e ausência sob
teto antes de adicionar molhado. Pré-requisitos: emissores, profundidade, exposição e campo de vento simples, que pode
ser constante no piloto. Deformação de [árvores](wind-and-vegetation.md), [materiais avançados](surface-materials.md),
[água](water.md) e [áudio](ambient-audio.md) novos não bloqueiam chuva visual inicial. Molhado e respingos adicionais
exigem seus próprios receptores/contratos antes de integrar. Ver [roteiro](implementation-roadmap.md).
