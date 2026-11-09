---
title: "Terra e poeira desprendendo dos tetos das minas"
description: "Emissores locais em Glitter Gulch Mine, com queda fina, dissolução e término no piso correto."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, mine, dust, particles]
---

# Terra e poeira desprendendo dos tetos das minas

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Viabilidade inicial alta para partículas locais, com posição e limites
autorais. Ver [guia](README.md), [perfis](scenario-profiles.md) e [arquitetura](architecture.md).

## Atmosfera proposta

Pequenos desprendimentos ocasionais de terra fina do teto e de vigas de Glitter Gulch Mine, acompanhados de poeira
suave. Dar sensação de espaço antigo e vivo sem sugerir ataque, desabamento iminente ou nova mecânica de perigo.
Partículas esparsas podem ficar mais visíveis junto às luzes; não criar uma cortina constante de fumaça.

## Implementação candidata

Cadastrar emissores em tetos/vigas comprovados, com área de origem, direção, altura de término, intervalo e limite
de partículas. Cada pequeno evento libera grãos e poeira que desacelera/desaparece. Contato com o chão pode gerar
uma expansão discreta, somente quando a superfície de término estiver identificada.

Usar volumes locais ou consultas nativas de superfície após validar custo e segurança. Não emitir da posição
arbitrária acima da câmera: isso cria terra saindo de túneis vazios, atravessando pontes ou vindo do lado errado do
teto. Em regiões próximas a água, definir se o grão termina nela; nunca criar impacto no piso submerso por padrão.

Intervalos usam aleatoriedade cosmética própria, sem RNG do jogo. Geração de mapa/sala controla descarte e reentrada;
duas câmeras não disparam o evento duas vezes. Um som muito discreto de grãos é candidato, sem disparar por partícula
e sem mascarar cues de objetos/inimigos. Não adicionar fragmentos com colisão ou dano.

## Requisitos

| ID | Aceite proposto |
| --- | --- |
| DST-01 | Terra parte de teto/viga cadastrado e termina na superfície correta; nada atravessa salas ou pisos. |
| DST-02 | Evento raro, leve e legível como ambiência; sem esconder inimigos, caminhos e coletáveis. |
| DST-03 | Emissão única por evento de mundo, RNG próprio, pausas sem acúmulo e descarte ao sair/trocar mapa. |
| DST-04 | Sem colisão/dano ou custo ilimitado; densidade, áudio e fallback controláveis. |

Primeiro recorte: um teto, piso e túnel vizinho identificados em Glitter Gulch Mine. Validar parado, caminhando,
com as duas câmeras e saindo da região. Relacionados: [partículas locais](local-particles.md),
[luz](lighting.md), [áudio](ambient-audio.md) e [aceite](validation.md).
