---
title: "Céu, neblina e profundidade atmosférica"
description: "Horizonte coerente com hora e clima, nuvens estilizadas e preservação dos limites visuais do cenário."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, sky, fog, atmosphere]
---

# Céu, neblina e profundidade atmosférica

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Viabilidade média/alta, dependente da representação de céu/neblina e
materiais já substituídos. Ver [guia](README.md), [dia/noite](day-night-cycle.md) e [perfis](scenario-profiles.md).

## Melhoria proposta

Preservar nuvens e desenho estilizado, oferecendo horizonte e iluminação ambiental coerentes com hora/clima.
Névoa suave pode reforçar profundidade em exteriores, umidade numa lagoa e distância numa caverna, com parâmetros
específicos. Não usar a mesma névoa cinza em todos os mundos.

Comparar restauração de texturas de céu existentes com novas variantes artísticas controladas. Se o céu usa faces
ou recortes, reconstruir o conjunto com correspondência comprovada para não criar emendas. Inventariar BT Reloaded
antes de produzir outro céu: o pacote local já contém arquivos na categoria `Skybox`.

## Caminho técnico candidato

Identificar céu, horizonte, neblina nativa e composição final separadamente. Definir se a variação acontece em textura,
parâmetros de luz, shader ou combinação. Nuvens animadas precisam preservar continuidade e não revelar paredes do
céu. Cor do horizonte deve combinar com névoa e reflexo da [água](water.md).

Não remover neblina indiscriminadamente. Ela pode ajudar a compor distâncias e esconder limites originalmente não
destinados à observação. Uma atmosfera mais clara não autoriza desenhar geometria ausente nem mudar regras de
culling. A opção de distância de objetos tem efeitos próprios e não é uma chave de “qualidade atmosférica”.

Interiores e regiões submersas têm perfil distinto; céu exterior não deve atravessar paredes. J1/J2 podem observar
meios diferentes com a mesma hora global. [Feixes de luz localizados](light-shafts.md) possuem plano separado,
de prioridade posterior e com limites de oclusão explícitos. Névoa volumétrica geral não é requisito do piloto.
Dois estados de céu em hora fixa podem ser validados antes do ciclo dia/noite contínuo.

## Requisitos

| ID | Aceite proposto |
| --- | --- |
| SKY-01 | Céu, horizonte e transições sem emendas, acompanhando hora/clima somente nas regiões permitidas. |
| SKY-02 | Névoa preserva identidade e legibilidade, sem dupla aplicação ou exposição de limites do mapa. |
| SKY-03 | Interiores, submersão, duas câmeras e cortes tratados corretamente; HUD e culling nativo preservados. |
| SKY-04 | Convivência com pacotes existentes, qualidade por backend e desligamento demonstrados. |

Primeiro recorte: exterior com horizonte visível, interior adjacente e uma vista de água quando disponível.
Comparar nativo e dois horários fixos antes de aceitar ciclo contínuo. Relacionados: [cores](color-grading.md),
[texturas](texture-restoration.md) e [validação](validation.md).
