---
title: "Reações visuais ao contato com o ambiente"
description: "Poeira de passos, pequenos respingos e ondulações com eventos nativos e materiais identificados."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, immersion, contacts, particles]
---

# Reações visuais ao contato com o ambiente

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Ideia adicional de prioridade posterior à base de partículas. Integração
de contatos/eventos ainda é `UNKNOWN`. Ver [guia](README.md) e [roteiro](implementation-roadmap.md).

## Ideia e limites

Conectar presença do personagem ao material: pequenas nuvens de terra em passos secos, respingos em superfícies
rasas elegíveis e anéis discretos de contato na água. A intensidade acompanha ação/contato comprovados, sem inventar
um passo a cada frame. O objetivo é resposta sutil, não trilha permanente atrás do jogador.

| Resposta candidata | Informação necessária | Limite inicial |
| --- | --- | --- |
| Poeira de passo ou aterrissagem | Evento/contato real, ponto de apoio e material seco | Não emitir em metal, água ou durante voo |
| Respingos locais | Contato de superfície e exposição/molhado válidos | Sem respingo no ar, embaixo do teto por engano ou em água distante |
| Ondulação em água | Ponto e plano/volume de contato apropriado | Não aplicar o mesmo círculo a todo tipo de água ou ao fundo submerso |
| Pegadas efêmeras em areia/neve | Receptor, normal, decal e limite de vida | Extensão posterior; não é requisito nem substitui colisão nativa |

Inventariar primeiro efeitos e sons nativos. Complementar ou preservar o existente, sem duplicar passo, impacto,
som e respingo. Não substituir animação/física só para obter callback. Cada forma nativa tem geometria e contatos
próprios; rodas, nado e voo não devem receber os mesmos efeitos de pés do Banjo.

## Eventos e propriedade

Identificar evento semântico nativo ou mudança de contato confiável, associando jogador/instância, geração,
superfície e ação. Não usar somente amplitude do analógico, posição da câmera ou deslocamento por render frame.
Deduplicar na atualização; compor posição/pose com a apresentação sem emitir novamente nos frames interpolados.

Recursos pertencem ao mundo/ator correto; câmeras apenas os observam. J1/J2 podem criar dois contatos reais distintos,
mas a mesma pegada não nasce duas vezes por haver duas viewports. Teleporte, morte e troca de mapa cancelam eventos
adiados da geração anterior. Ausência de material/contato confiável implica não emitir.

Não adicionar dano, aderência, marcas de progresso, colisão de partículas ou persistência no save. Decals precisam
de limites, profundidade, orientação e receptor apropriados; se isso não estiver demonstrado, ficam adiados.
Os primeiros efeitos podem usar sprites curtos já integrados à base de [partículas](local-particles.md).

## Requisitos

| ID | Aceite proposto |
| --- | --- |
| INT-01 | Contato e material comprovados, com seleção por ação/forma; nenhuma emissão por mera entrada analógica. |
| INT-02 | Complemento não duplica partículas, sons e eventos nativos nem altera física, estado ou progresso. |
| INT-03 | Evento único por contato, independente de FPS/viewports, com identidade de jogador e geração corretas. |
| INT-04 | Efeito nasce na superfície certa, fica coerente com pose apresentada e respeita profundidade/vida/limites. |
| INT-05 | Limpeza em transições, custo combinado e controles de qualidade aprovados; material desconhecido não emite. |

Primeiro recorte: passo ou aterrissagem de Banjo numa superfície seca identificada, contrastando com piso rígido
onde o efeito não deve existir. Só depois estudar respingos, outras formas e pegadas. Relacionados:
[água](water.md), [chuva](rain.md), [áudio](ambient-audio.md) e [validação](validation.md).
