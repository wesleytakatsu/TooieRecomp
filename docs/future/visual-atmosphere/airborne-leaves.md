---
title: "Folhas ao vento em regiões arborizadas"
description: "Folhas esparsas com origem na vegetação, movimento coerente e exclusões de interiores e gameplay."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, leaves, forests, particles]
---

# Folhas ao vento em regiões arborizadas

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Viabilidade alta para emissores locais; origem e direção artística precisam
ser cadastradas. Ver [guia](README.md), [vento](wind-and-vegetation.md) e [perfis](scenario-profiles.md).

## Efeito proposto

Poucas folhas atravessam regiões arborizadas, com queda lenta, rotação variada e resposta às rajadas. Cores e formas
acompanham vegetação local. Rajadas podem desprender pequenos grupos sem manter um redemoinho permanente ao redor
do jogador. Evitar folhas de uma árvore inexistente no cenário e materiais que pareçam coletáveis ou projéteis.

## Caminho técnico candidato

Partículas com profundidade e alfa adequado, emitidas em copas ou volumes próximos de vegetação cadastrada. Usar
velocidade do campo de vento mais queda e oscilação leves, com limites de vida/distância. Finalizar no solo quando
a superfície for conhecida ou dissolver antes de um contato não validado. Não simular pilhas com colisão no piloto.

Sair do volume, entrar em casa ou mergulhar deve cessar novas emissões e tratar folhas existentes conforme o meio,
sem atravessar teto/paredes. Água pode receber um término discreto se cadastrado; folha flutuante persistente seria
estudo adicional. Profundidade, bordas de alfa e mipmaps precisam evitar halo e cintilação.

Emissores são de mundo e não duplicam por viewport. Uma árvore vista por J1 e J2 mantém evento coerente; limites
de apresentação podem reduzir quantidade por câmera sem mudar o vento do mundo. Partículas não alteram colisão,
inventário, alvos ou RNG nativo. Não mascarar plataformas distantes com excesso de folhas.

## Requisitos

| ID | Aceite proposto |
| --- | --- |
| LEF-01 | Origem, forma e paleta compatíveis com vegetação local; sem emissão genérica em minas/interiores. |
| LEF-02 | Trajetória reage ao mesmo vento das árvores/chuva e respeita profundidade, término e meio. |
| LEF-03 | Emissão não duplica em split-screen, não depende de FPS e limpa recursos nas transições. |
| LEF-04 | Densidade baixa, nenhuma aparência de coletável, alfa estável e controle de intensidade/desligamento. |

Primeiro recorte: uma copa e a passagem para um interior numa região arborizada da Spiral Mountain ou Isle O' Hags,
após confirmar IDs e superfícies. Relacionados: [partículas locais](local-particles.md) e [validação](validation.md).
