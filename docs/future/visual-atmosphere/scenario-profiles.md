---
title: "Perfis propostos de atmosfera por cenário"
description: "Direção artística e compatibilidade local de clima, tempo, vento, partículas e água."
status: proposed
type: design-catalog
project: TooieRecomp
updated: 2026-11-07
tags: [future, atmosphere, maps, art-direction]
---

# Perfis propostos de atmosfera por cenário

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. Esta é uma matriz de intenção artística. Não comprova cadastro de mapas,
classificação de interiores ou compatibilidade dos efeitos. Ver [guia](README.md) e [arquitetura](architecture.md).

## Compatibilidade artística inicial

| Ambiente candidato | Atmosfera proposta | Hora e clima | Restrições de compatibilidade |
| --- | --- | --- | --- |
| Spiral Mountain e áreas arborizadas da Isle O' Hags | Grama/paredes restauradas, copas em movimento, folhas esparsas, água suave | Ciclo solar e chuva ocasionais somente em exteriores aprovados | Casas, cavernas, regiões subterrâneas e cenas roteirizadas têm perfil próprio |
| Mayahem Temple | Pedra com desenho preservado, vegetação discreta, luz quente e insetos pontuais | Variação de luz e chuva moderada nos exteriores elegíveis | Câmaras e inscrições precisam continuar legíveis; não aplicar ciclo solar dentro do templo |
| Glitter Gulch Mine | Madeira/rocha restauradas, luzes quentes, terra fina desprendendo de tetos, poeira e goteiras localizadas | Interior com iluminação própria; sem chuva atmosférica ou sol atravessando teto | Queda de terra exige teto cadastrado; poeira não deve parecer desabamento ou perigo |
| Witchyworld | Néons contidos, lâmpadas com brilho localizado, tecidos e placas leves ao vento | Noite artística fixa candidata; transição solar somente se a identidade da sala comportar | Barracas e atrações cobertas não recebem chuva; evitar flashes em jogos e mira |
| Jolly Roger's Lagoon | Água turquesa, ondulações, reflexos suaves, cáusticas e partículas submersas | Ciclo e chuva apenas na superfície exterior elegível | Ambientes submersos e interiores mantêm perfis separados; chuva termina na superfície da água |
| Terrydactyland | Rocha/terra mais definidas, poeira seca, vegetação e folhas em pontos apropriados | Vento e ciclo solar candidatos em exteriores; chuva depende de direção artística local | Cavernas sem céu têm iluminação própria; não mover árvores usadas como apoio além do limite visual aprovado |
| Grunty Industries | Luz industrial, metal pintado, vapor, pingos e sujeira coerente | Exteriores podem ter tempo nublado; interiores não seguem o sol diretamente | Chaminés, máquinas e saídas de vapor exigem emissores identificados; não duplicar efeitos nativos |
| Hailfire Peaks, lado de fogo | Lava com brilho localizado, ar quente e cinzas esparsas | Identidade quente preservada; sem chuva genérica no piloto | Evitar chuva extinguindo fogo apenas visualmente; cinzas não confundem ataques ou brasas perigosas |
| Hailfire Peaks, lado de gelo | Neve leve, vento frio, gelo com brilho contido | Nevasca candidata; céu frio próprio | Não converter automaticamente precipitação em chuva; iglus/interiores sem neve atravessando teto |
| Cloud Cuckooland | Céu estilizado, nuvens e correntes de vento, partículas muito esparsas | Variação solar somente onde a composição do céu permitir | Evitar partículas que escondam bordas e plataformas; chuva não é obrigatória |
| Cauldron Keep | Pedra e materiais restaurados, atmosfera sombria, névoa localizada | Perfil dramático próprio; hora fixa candidata | Cenas finais, desafios e sequências roteirizadas preservam sua composição original |
| Casas, templos fechados, cavernas e salas especiais | Luz e partículas locais somente quando justificadas | Hora exterior pode continuar, mas não recolore globalmente o interior | Janelas, portas e claraboias exigem tratamento explícito; não aplicar perfil exterior por herança cega |
| Bosses, minigames, demos e cutscenes | Aparência nativa inicialmente | Efeitos novos desligados até aceite dedicado | Câmera, visibilidade, contraste de perigos e sequência de eventos têm prioridade |

Todos os nomes são referências humanas. IDs reais, submapas e regiões permanecem `UNKNOWN` nesta proposta.
Nenhuma fase está aprovada integralmente só por constar na tabela.

## Cadastro necessário por sala ou região

| Campo conceitual | Informação necessária |
| --- | --- |
| Identidade | ID real do mapa, sala/região, condições nativas e origem da identificação |
| Ambiente | Exterior, interior, cobertura parcial, submerso ou misto |
| Exposição | Volumes expostos, tetos, aberturas, limites, água e zonas de exclusão |
| Arte | Paleta base, contraste mínimo, materiais e referências originais |
| Tempo | Ciclo permitido, hora fixa ou controle nativo/roteirizado prioritário |
| Clima | Estados permitidos, duração proposta, transições e intensidades máximas |
| Fontes | Emissores de poeira, folhas, neve, vapor e áudio com posição/volume e superfície de término |
| Movimento | Árvores, copas, grama, tecidos e objetos explicitamente elegíveis ao vento |
| Regressão | Plataformas, puzzle, perigos, mira, textos, NPCs e efeitos nativos a preservar |
| Desempenho | Perfil por plataforma, limite de recursos e fallback |
| Estado | Responsável pela revisão, evidência disponível e critérios ainda pendentes |
| Precedência nativa | Efeitos, sons e estados já existentes que serão preservados, complementados ou visualmente substituídos |
| Prova técnica | Origem da identidade/máscara e limites do recorte; técnica conhecida não equivale a integração provada |

Esse cadastro é um modelo de trabalho, não um novo formato já aceito pelo instalador de mods.

## Regras de composição

- Verificar efeitos nativos antes de adicionar novos. Uma fogueira não deve ganhar duas camadas idênticas de fumaça.
- Clima exterior, partículas locais e luzes locais são canais independentes, com limites compartilhados.
- Terra no teto da mina, folhas junto de árvores e vapor numa válvula precisam de origem espacial coerente.
- Uma sala mista pode ter chuva do lado de fora e poeira do lado de dentro, com transição suave no limiar.
- Neve, cinzas e gotas são efeitos cosméticos; não acumulam colisão, dano ou gelo escorregadio nesta proposta.
- Conflitos são resolvidos pelo perfil específico e pelo estado nativo. Se não for possível identificar a região
  com confiança, desativar o efeito contextual até completar o cadastro.
- [Reações ao contato](surface-interactions.md) são candidatas somente onde ação, forma e material forem identificados.
  Poeira de passos combina com terra seca; metal e personagens sem pés precisam de resposta diferente ou nenhuma.
- [Feixes de luz](light-shafts.md) são candidatos em lâmpadas/aberturas reais, especialmente galerias com poeira;
  não devem aparecer em todo teto da mina ou depender de chuva na região exterior.
- Reavaliar cadastro quando estado nativo muda geometria, água, luz ou exposição. Perfil não pode sobreviver
  indefinidamente só porque o ID do mapa permaneceu igual. Até reconhecer a variante, preservar apresentação nativa.

## Primeiros recortes

O piloto de texturas usa Mayahem Temple, começando com 3–5 amostras antes do lote de 20–30. O primeiro emissor de terra usa um teto comprovado em Glitter Gulch Mine.
Folhas e vento usam uma região arborizada e uma árvore identificada, sem assumir elegibilidade de toda a floresta.
Água usa uma superfície delimitada de Jolly Roger's Lagoon. Cada recorte tem aceite próprio em
[validação](validation.md), antes de expansão para o restante da fase. O [roteiro](implementation-roadmap.md) separa
pré-requisitos técnicos de integrações opcionais e os [perfis de qualidade](quality-presets.md) limitam custo combinado.
