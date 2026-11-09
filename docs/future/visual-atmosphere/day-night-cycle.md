---
title: "Passagem visual do tempo com dia e noite"
description: "Ciclo solar cosmético por cenário, com céu, luz, materiais emissivos e regras para interiores e cenas."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, time, day-night, atmosphere]
---

# Passagem visual do tempo com dia e noite

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Viabilidade média; requer coerência de céu, luz e arte já iluminada.
Ver [guia](README.md), [arquitetura](architecture.md) e [perfis](scenario-profiles.md).

## Ciclo cosmético proposto

Oferecer manhã, dia, entardecer e noite, com transições graduais e duração configurável. Não vincular ao relógio
real do computador. Proposta inicial: sessão começa ao meio-dia, duração experimental de 30 minutos reais por ciclo
completo e possibilidade de fixar hora. Esse valor é ponto de partida artístico, não medição nem contrato existente.

O relógio é único na sessão. Tempo avança pelo contexto cosmético controlado, não por desenho ou número de câmeras.
Pausa, menu e cenas roteirizadas congelam o avanço, sem compensação posterior do tempo decorrido. Entrar em interior
não reinicia o ciclo: hora continua globalmente, mas luz interior segue seu perfil. Reabrir o jogo reinicia a sessão
no piloto; persistência externa às saves nativas é uma decisão posterior.

## Coerência visual

| Componente | Comportamento pretendido |
| --- | --- |
| Céu | Estados ou composição coerentes com horizonte, nuvens e hora; não apenas escurecer o céu diurno |
| Luz ambiente e direção | Cores graduais, noite legível e direção solar somente onde a representação suportar |
| Texturas e cores nativas | Identificar luz/sombra já incorporadas; evitar sol novo contradizendo sombra pintada |
| Fontes locais | Lâmpadas e fogo permanecem coerentes; acendimento cosmético só em objetos identificados e autorizados |
| Água | Reflexo de ambiente e cor acompanham céu/hora do perfil |
| Névoa | Cor e densidade compatíveis com a sala e horizonte, sem revelar limites do mapa |
| Áudio | Ambiência pode variar suavemente; música e cues de gameplay preservados |

Um filtro escuro aplicado sobre toda a tela não atende ao requisito de noite. Sol/lua visíveis são elementos
opcionais que exigem posição, oclusão e integração com o céu; não são necessários para aceitar o primeiro piloto.

Provar primeiro dois estados fixos de céu, paleta e luz ambiente. Só então interpolar o ciclo. Enquanto sombras e
iluminação incorporadas aos assets não tiverem tratamento demonstrado, manter direção de luz compatível com elas;
não simular um sol girando que contradiz sombras estáticas no chão. Uma versão inicial pode variar atmosfera sem
sol/lua visíveis e sem sombras móveis. Rotação solar e novas sombras são extensões separadas, não promessa do piloto.

## Exceções por cenário

Exteriores aprovados podem ciclar. Interiores fechados, ambientes submersos, regiões de lava e mapas com atmosfera
roteirizada possuem hora fixa ou interpretação local. Witchyworld pode manter noite artística; Cauldron Keep pode
preservar composição dramática. Esses são candidatos sujeitos a revisão por sala, não regras provadas do jogo.

Cutscenes, demos, bosses e minigames mantêm aparência nativa inicialmente. Qualquer estado nativo que determine
composição/iluminação tem prioridade. O ciclo não muda NPCs, timers, disponibilidade de itens, portas ou progressão.
Não rotacionar sombras nativas pintadas como se fossem geometria dinâmica.

## Requisitos

| ID | Aceite proposto |
| --- | --- |
| DAY-01 | Duração e fase iguais a 30/60/120 FPS e com duas câmeras; pausa/retomada sem salto. |
| DAY-02 | Céu, luz, fontes locais, água e névoa evoluem de maneira coerente; sem simples filtro noturno global. |
| DAY-03 | Perfis aprovados por sala; interiores e cenas especiais preservados, sem clarear/escurecer por herança indevida. |
| DAY-04 | Noite mantém plataformas, inimigos, perigos e pistas legíveis; hora fixa e desligamento disponíveis. |
| DAY-05 | Sem gravação em save nativo ou efeito sobre gameplay; troca de sala preserva fase da sessão e recursos são descartados corretamente. |
| DAY-06 | Estados em hora fixa aprovados antes do ciclo; direção de luz e céu não contradizem sombras/iluminação incorporadas aos assets. |

Primeiro recorte: um exterior e seu interior adjacente. Inspecionar ciclo acelerado para defeitos e depois duração
normal para ritmo artístico. Pré-requisitos: relógio cosmético e estados fixos de [céu](sky-and-fog.md),
[cores](color-grading.md) e [luz](lighting.md) adequados ao recorte. [Água](water.md) e [áudio](ambient-audio.md)
novos não bloqueiam o ciclo; quando presentes, devem acompanhar seu contexto. [Roteiro](implementation-roadmap.md).
