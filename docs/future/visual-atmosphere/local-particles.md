---
title: "Partículas locais para ambientes vivos"
description: "Neve, cinzas, vapor, goteiras e partículas submersas com fonte, meio e orçamento próprios."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, atmosphere, particles, environments]
---

# Partículas locais para ambientes vivos

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Viabilidade alta para variantes simples após integrar emissores e
profundidade; cobertura de cada cenário ainda é `UNKNOWN`. Ver [guia](README.md) e [perfis](scenario-profiles.md).

## Variantes propostas

| Variante | Região candidata | Regra de compatibilidade |
| --- | --- | --- |
| Neve esparsa | Exteriores aprovados do lado gelado de Hailfire Peaks | Teto bloqueia; não acumula neve física nem torna piso escorregadio |
| Cinzas/pequenas brasas visuais | Regiões de lava aprovadas de Hailfire Peaks | Sem dano; aparência distinta de projéteis perigosos e brilho contido |
| Vapor | Válvulas/saídas identificadas de Grunty Industries | Fonte espacial definida; não adicionar a toda máquina nem duplicar vapor nativo |
| Goteiras | Teto úmido cadastrado em minas/cavernas | Gota termina no piso/água identificados; não equivale a chuva atravessando teto |
| Poeira suspensa | Feixes de luz e salas secas selecionadas | Densidade baixa; não criar névoa branca permanente |
| Partículas submersas/bolhas | Águas e fontes específicas | Só no meio aquático elegível; bolhas precisam de origem justificável |
| Insetos ou pontos luminosos esparsos | Vegetação e horários aprovados | Não confundir com itens; controlar brilho e evitar depender de hora em interiores |
| Distorção térmica | Próximo à lava ou saída quente identificada | Estudo posterior: exige fundo e máscara corretos, sem distorcer HUD/perigos |

Essa lista amplia possibilidades artísticas; não autoriza ativar todas as variantes ao mesmo tempo. Chuva,
[terra caindo da mina](mine-dust.md) e [folhas](airborne-leaves.md) têm planos próprios e compartilham infraestrutura.

## Emissores e composição

Cada variante define fonte, região, meio, direção, densidade, vida, limite e política de término. Não preencher a
câmera com partículas sem origem. Consultar efeitos nativos por região antes de adicionar outro; substituir,
complementar ou manter o original são decisões explícitas, com comparação de referência.

Profundidade e clipping respeitam paredes, tetos e interfaces de água. Transparências exigem ordenação/blending
adequados e orçamento de sobreposição. Soft particles, quando viáveis, precisam de profundidade coerente; não são
uma correção universal para atravessar paredes ou faltar geometria. Sem recurso opcional, oferecer variante simples
validada ou desligar o efeito.

Emissores usam pools limitados e descarte por geração. O limite total por cena/viewports tem prioridade sobre a soma
das intensidades de cada efeito. Controle cosmético de vento, clima e hora vem da [arquitetura](architecture.md).
O primeiro marco não adiciona acúmulo persistente, colisão física, dano ou mudança de puzzle.

## Requisitos

| ID | Aceite proposto |
| --- | --- |
| PAR-01 | Variante adequada à região, com fonte e meio cadastrados; nenhuma emissão automática pelo nome da fase. |
| PAR-02 | Sem duplicar efeitos nativos, confundir perigos/coletáveis ou bloquear gameplay. |
| PAR-03 | Profundidade, alfa, término, split-screen e transições corretos, com pools limitados e RNG cosmético. |
| PAR-04 | Qualidade/intensidade por plataforma e conforto ajustáveis; nenhuma mudança de simulação nativa. |

Primeiro recorte posterior à poeira/folhas: um emissor de vapor ou uma região de neve, escolhendo apenas uma variante
por checkpoint. Pré-requisitos: contexto, emissores e limites; vento simples quando a variante precisar dele.
Deformação de [árvores](wind-and-vegetation.md) e [áudio](ambient-audio.md) novo não são obrigatórios para validar
o efeito visual. [Aceite](validation.md) e [roteiro](implementation-roadmap.md).
