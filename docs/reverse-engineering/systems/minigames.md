---
title: "Minigames e catálogo de replay"
description: "Identificação dos 23 sinalizadores de replay, salas relacionadas e fronteiras para suspensão do cooperativo experimental."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-11-02
tags: [minigames, replay, multiplayer, split-screen]
---

# Minigames e catálogo de replay

## Escopo e estados de verdade

`PROVEN` (`STATIC_SOURCE`): `decomp/include/gameflags.h` enumera exatamente 23 entradas
`GFLAG_*_MINIGAME_REPLAY_*`, consecutivas de `0xBE3` a `0xBF9`. Correspondem aos 15 grupos solicitados pelo
usuário. Os números 3 nos dois Kickballs, Dodgems e Tower of Tragedy representam etapas distintas.

`STRONG_INFERENCE`: esses sinalizadores descrevem a disponibilidade de replay, conforme seus nomes e sua
posição entre os catálogos globais de boss replay e cinema replay. Não constituem um detector de partida ativa.
O ciclo exato de leitura/gravação desses bits não foi rastreado nesta investigação. Não usar nem modificar esses
bits para suspender o cooperativo durante uma partida.

Este é o catálogo de replay, não uma definição exaustiva de qualquer atividade que possa ser chamada de
minigame. Corridas e outros desafios têm identidades próprias no jogo; sua inclusão na política de suspensão
não decorre automaticamente destes 23 sinalizadores.

## Catálogo identificado

Identidades de flags e mapas: `decomp/include/gameflags.h` e `decomp/include/maps.h` (`PROVEN`,
`STATIC_SOURCE`). A associação entre nome do desafio e sala com nome correspondente é `STRONG_INFERENCE`,
sem rastreamento completo das transições de todos os eventos. O teste do usuário é parcial e não identifica
individualmente os eventos aprovados (detalhes da limitação abaixo).

| Grupo | Etapas / sinalizadores de replay | Salas relacionadas no modo história |
| --- | --- | --- |
| Mayan Kickball | Quartas `0xBE3`; semifinal `0xBE4`; final `0xBE5` | `0xC6`, `0xC8`, `0xC9` |
| Ordnance Storage | `0xBE6` | `0xDC` |
| Dodgem Challenge | 1v1 `0xBE7`; 1v2 `0xBE8`; 1v3 `0xBE9` | `0xDE`, `0xDF`, `0xE0` |
| Hoop Hurry | `0xBEA` | `0xE5` |
| Balloon Burst | `0xBEB` | `0xE4` |
| Saucer of Peril | `0xBEC` | `0x124`; variantes de percurso nomeadas `0x13B`, `0x13C` |
| Mini-Sub Challenge | `0xBED` | `0x181` (Sea Bottom Cavern) |
| Chompa's Belly | `0xBEE` | `0x123` |
| Clinker's Cavern | `0xBEF` | `0x162` |
| Twinkly Packing | `0xBF0` | `0x10A` |
| Colosseum Kickball | Quartas `0xBF1`; semifinal `0xBF2`; final `0xBF3` | `0x12E`, `0x12F`, `0x130` |
| Pot O' Gold | `0xBF4` | `0x13D` |
| Trash Can Germs | `0xBF5` | `0x137` |
| Zubbas' Hive | `0xBF6` | `0x139` |
| Tower of Tragedy | Rodadas 1 `0xBF7`, 2 `0xBF8`, 3 `0xBF9` | `0x15F` |
| **Total** | **23 sinalizadores, 15 grupos** | |

Notas de identidade:

- Dodgems usa nomes `2v1` e `3v1` nos identificadores de flags, mas `1V2` e `1V3` nos mapas. São as mesmas
  quantidades de oponentes das etapas enumeradas, não eventos adicionais.
- `0xCA..0xCE` também recebem nomes `MT_KICKBALL_3` no enum de mapas. Seu uso efetivo e a relação com as três
  etapas são `UNKNOWN`; não contar essas cinco entradas como cinco novos minigames.
- `0xBB`, `0xDD`, `0x12D` e `0x163` são estádio/lobbies/entrada, distintos das arenas listadas. Não bloquear
  automaticamente lobbies por semelhança do nome.
- Mini-Sub: `func_80800BA8_chminisubgame` possui caminhos de transição para `0x181`, entrada 1, e `0x1A9`,
  entrada `0x1B` (`PROVEN`, `STATIC_MIPS`, `RecompiledFuncs/funcs_158.c`, instruções `0x80800E38..0x80800E8C`).
  `0x1A9` é Sea Bottom, uma área maior de Jolly Roger's Lagoon; não deve ser excluída inteira com base nesse retorno.
  `0x181` é a sala associada ao desafio; o ciclo exato de início/repetição/saída ainda requer validação.
  No mesmo helper, tag de evento/escolha `0xCEB` chega a `0x80800DFC`: com `func_800D395C()` não zero,
  argumento original `a2 == 1` chama `_gcfrontend_entrypoint_11`; demais valores chamam
  `_gcfrontend_entrypoint_10`. Esses caminhos pulam ambos os gotos locais. Com resultado zero, `a2 == 1`
  escolhe `(0x181,1)`; demais valores escolhem `(0x1A9,0x1B)`. Distinção local `PROVEN` (`STATIC_MIPS`);
  semântica completa dos helpers frontend e destinos posteriores de replay: `UNKNOWN`.
- Saucer of Peril: `0x13B`/`0x13C` são `WW_CRAZY_CASTLE_SOP`/`WW_STAR_SPINNER_SOP`. O helper nativo
  `func_80800514_chflysaucer` resolve seletores 1..6 em `(0x124,8)`, `(0x13C,1)`, `(0x124,8)`, `(0x13B,1)`,
  `(0x124,1)` e `(0xD6,8)` (`PROVEN`, `STATIC_MIPS`/`STATIC_ROM`, tabela da seção 1692 `+0x3E48`).
  O retorno com seletor 6 possui lógica de resultado/diálogo antes de zerar o percurso (detalhes abaixo).
  Sequência completa ao vivo e todos os abortos continuam `UNKNOWN`.

## Controladores nativos identificados

`PROVEN` (`STATIC_MIPS`/`STATIC_ROM`): os getters abaixo retornam descritores das respectivas seções; seus ponteiros
de callback foram resolvidos com `metadata/tooie.us.syms.toml` e a ROM descomprimida canônica verificada.
Números de seção não são IDs de overlay. Os papéis semânticos completos de todos os atores secundários permanecem
`UNKNOWN`; não apagar participantes/alvos nativos para remover J2.

| Família | Overlay | Getter do descritor principal | Seção / offset |
| --- | --- | --- | --- |
| Mayan Kickball | `chstoneballcontrol` | `chstoneballcontrol_entrypoint_0` | 1608 / `0x50D4` |
| Ordnance | `chtntmineshoot` | `chtntmineshoot_entrypoint_1` | 2204 / `0x26D8` |
| Dodgems | `chdodgemcontrol` | `chdodgemcontrol_entrypoint_0` | 1752 / `0x4DB4` |
| Hoop Hurry | `chhoopswoop` | `chhoopswoop_entrypoint_0` | 1016 / `0x1EC0` |
| Balloon Burst | `chballoonburst` | `chballoonburst_entrypoint_0` | 1008 / `0x1C40` |
| Saucer | `chflysaucer` | `chflysaucer_entrypoint_1` | 1692 / `0x4668` |
| Mini-Sub | `chminisubgame` | `chminisubgame_entrypoint_0` | 792 / `0x1424` |
| Chompa | `chmonsterinterior` | `chmonsterinterior_entrypoint_0` | 1320 / `0x1E80` |
| Clinker | `chclinker` | `chclinker_entrypoint_1` | 2208 / `0x1550` |
| Packing | `chfactorytwinklies` | `chfactorytwinklies_entrypoint_0` | 1308 / `0x2218` |
| Colosseum | `chcollballcontrol` | `chcollballcontrol_entrypoint_0` | 1840 / `0x4DA8` |
| Pot O' Gold | `chpotogoldgame` | `chpotogoldgame_entrypoint_4` | 2336 / `0x2578` |
| Trash Can | `chtrashcangame` | `chtrashcangame_entrypoint_0` | 2384 / `0x31B4` |
| Zubbas | `chwaspnestgame` | `chwaspnestgame_entrypoint_0` | 776 / `0x1F80` |
| Tower of Tragedy | `chquiz` | `chquiz_entrypoint_1` | 2484 / `0x7334` |

Discriminadores de identidade (`PROVEN`, `STATIC_MIPS`):

- `func_80800C38_chmonsterinterior` consulta `(0x2E,1)` em `func_800D0A9C`, coerente com
  `TDL_CHOMPAS_BELLY` em `jiggies.h`; `func_8080078C_chmonsterinterior` tem retry para `(0x123,1)`.
- `chfishshootgame` consulta `(0x21,1)` na inicialização, coerente com `JRL_CHRIS_P_BACON`, e não deve ser
  confundido com Chompa por envolver tiro. `chchompaslunch` é outro ator, com máquina de estados própria.
- `chpotogoldgame_entrypoint_0` executa lógica; o getter é entrypoint 4. Índice zero não significa descritor universal.
- Em Saucer, `func_80800000_chflysaucer` lê o campo nativo de três bits `(0xA34,3)` via `func_800DA564` e o
  espelha no bloco de extensão `+0x26`; `func_8080003C_chflysaucer` o escreve por `func_800DA7A8`.
  Não confundir esse seletor com a flag global de replay `0xBEC`; vida útil/reset completo do campo é `UNKNOWN`.
- `chfpsgame` contém infraestrutura de sessão/atualização FPS compartilhada. Sua presença não identifica
  exclusivamente Ordnance ou Clinker; a contagem nativa vem de `gcstatusDll_entrypoint_11`.

## Versões de multiplayer nativo

`PROVEN` (`STATIC_SOURCE`): existem mapas separados `0x164`/`0x165` (Ordnance), `0x17B` (Balloon),
`0x17C` (Hoop), `0x17D` (Packing), `0x17F`/`0x180` (Kickball), `0x182` (Submarine),
`0x183` (Chompa), `0x185` (Trashcan), `0x186` (Dodgems), `0x188` (Bee Hive) e `0x189` (Quiz).
Isso não prova que todos os replays individuais usem esses mapas.

`PROVEN` (`STATIC_MIPS`): `gcstatusDll_entrypoint_11` retorna 1 fora do subtipo nativo `0x1B` e retorna a
contagem nativa em `0x8012B404` dentro dele (`RecompiledFuncs/funcs_238.c`, instruções
`0x80800118..0x80800140`). O multiplayer original deve manter sua própria contagem, câmeras e controles.
O pedido do usuário é suspender a adição experimental de J2 na história, não eliminar o multiplayer original.

## Estado atual da integração host

`PROVEN` (`STATIC_SOURCE` e harness de controle nativo; teste de gameplay parcial pelo usuário, sem aceite universal):

- `tooie_story_coop::decide_creation` conserva o pedido nativo nas salas catalogadas, aliases Mayan conservadores,
  salas MP relacionadas e retorno `0xD6`/Saucer 6. Subtipo nativo `0x1B` tem precedência, inclusive com pedido 1.
- `two_players_active()` continua significando opção ligada + Dev Mode desbloqueado. `created_pair` reconhece a
  dupla realmente promovida; `running` acrescenta a preferência atual. `owned_slot` conserva a identidade de saída
  enquanto o outro owner já pode ter sido retirado. Slot de câmera `+0x184` não serve como identidade de jogador.
- Wrapper de `func_800F61E4` registra as alocações normais da tentativa. Duas publicações válidas nesses slots,
  confirmadas após criação, autorizam a dupla; registros nativos e fallback isolado não bastam.
- `PROVEN` (log Windows, mapa `0xAF`, 2026-11-06): com a dupla viva, o slot 0 de `0x80135490` mudou de `801F5DA0`
  para `801F55C0` sem teardown `80098E64`. Causa da mudança (compactação de heap): `STRONG_INFERENCE`. A dupla
  confirmada segue o slot quando o objeto novo mantém o `+0x184` gravado na criação; o campo é só guarda dessa
  mudança, não identidade, que continua vindo da alocação normal.
- IA, Glowbo, menu de viagem, retenção por distância e Warp exigem participação efetiva; formas e iluminação
  preservam identidade dos owners já criados. O escopo de atividade provisória/serialização não habilita gameplay.
- J1 mantém a política anterior de disponibilidade do seletor. Sua inicialização usa o banco nativo de transição;
  cache J2 só é instalado na construção própria. Goliath continua passando pela reversão nativa antes da captura.
- Entrada host usa publicação atômica por porta/ciclo de owner, com mutex entre admissão e cancelamento. Novo owner
  exige Debug/R3, D-pad, A/B e navegação neutros antes do próximo acorde. Menu indisponível não drena entrada nativa.
- Cada recriação/retirada limpa amostras/reservas analógicas. Mini-Sub `0x181` e segmentos SOP `0x13B`/`0x13C`
  também usam controles C nativos; auxiliares não recebem a câmera analógica de J2.

Pontos de integração: `src/game/tooie_story_coop.cpp`, `include/tooie_story_coop.h`, `tooie_dev_warp_hook.cpp`,
`tooie_dev_forms_hook.cpp`, `tooie_dev_cheats_hook.cpp`, `tooie_dev_enemy_ai.cpp`, `tooie_dev_collectibles_hook.cpp`,
`tooie_dev_travel_menu.cpp`, `tooie_object_distance.cpp`, `tooie_analog_camera_hook.cpp` e `src/main/main.cpp`.

## Criação nativa e fronteiras de identidade

`PROVEN` (`STATIC_MIPS`): o pedido de contagem em `func_800F73C4` não garante alocação dessa quantidade.
Antes do laço de `0x800F7464`, três predicados podem desviá-lo: `func_800DB9B0`,
`_glcutDll_entrypoint_1` (`0x800887B0`) e `func_800F99E8`. Depois, `func_800F8B94` pode criar jogadores
por registros nativos, via `func_800F6204(0)`. Existe ainda criação de fallback em `0x800F74C0`.
Portanto, dois ponteiros distintos na tabela não provam dois jogadores cooperativos promovidos pelo host.
Papéis semânticos completos dos registros permanecem `UNKNOWN`.

O laço solicitado usa `func_800F61E4` -> `func_800F6140(-1)`; o caminho de registros usa
`func_800F6204` -> `func_800F6140` com o argumento recebido. Em `func_800F6140`, câmera/controle são
resolvidos antes de `func_800F43A8(slot)`; o ponteiro retornado só é publicado depois dessa alocação.
`func_800F73C4` também consulta `func_800F65D0` em `0x800F7510` e `0x800F7528`, antes de retornar ao
wrapper host. Um guard baseado exclusivamente em identidade confirmada após esse retorno mudaria esse trecho.

`PROVEN` (`STATIC_MIPS`): no caminho ordinário de carga, `func_800A72A4` chama `func_800A8D70` antes
de `func_800EA0CC`. Este chama `gsworldDll_entrypoint_2` com o estado `0x80132DC0`; o destino é escrito
em `+2` antes da consulta da contagem e da criação. O estado desse bloco não deve ser confundido com
o modo em `0x80127632`. Restauração ao vivo de todos os layouts/canais: `NOT_RUN` para a suspensão implementada.

`PROVEN` (`STATIC_MIPS`): no caminho normal de `func_800F759C`, `func_800F3880` executa o teardown
`func_80098E64` antes de liberar o PlayerState; a associação de câmera e o slot são limpos depois pelo chamador.
A identidade host precisa expirar no ciclo de destruição, preservando o handoff de valores durante teardown.
Igualdade de mapa/endereço não prova continuidade quando uma alocação é reutilizada.

Âncoras: `RecompiledFuncs/funcs_153.c:3305`, `funcs_188.c:4589`, `funcs_113.c:9673`,
`funcs_160.c:444`, `funcs_134.c:2943`, `funcs_292.c:2787`, `funcs_252.c:3753`,
`funcs_52.c:1950`, `funcs_246.c:1461`, `funcs_303.c:204`. Código gerado usado somente para leitura.

## Entrada host nas transições

No baseline investigado (`PROVEN`, `STATIC_SOURCE`), `Menus::reset` apagava também histórico de botões e drain;
`reset_requests` dos cheats limpava apenas pedidos atômicos, preservando histórico de acordes. Em `Menus::input`,
drain era processado antes do retorno por indisponibilidade e podia consumir entrada nativa. Logo, cancelamento de pedidos,
rearme de atalhos e consumo de controles são contratos diferentes.

No baseline investigado, publicação de Analog Camera era por controle. Ausência de J2 não produz nova publicação para seu controle;
`reset_motion` limpa amostras, reservas e movimento, mas então rodava no reset global de formas, não em
toda criação por mapa. Conclusão estática; falha visual/de entrada específica em minigames: `UNKNOWN`.

Fontes: `src/game/tooie_dev_forms.cpp:7`, `:41`; `src/game/tooie_dev_cheats.cpp:133`, `:155`;
`src/game/tooie_analog_camera_hook.cpp:74`, `:113`; `src/game/tooie_dev_forms_hook.cpp:115`, `:247`;
`src/main/main.cpp:1064`. A suspensão implementada preserva controles nativos e rearma somente os atalhos host,
sem alterar perfis ou remover portas do multiplayer original.

## Saucer: retorno compartilhado e resultado

`PROVEN` (`STATIC_ROM`/`STATIC_MIPS`): na seção 1692, tabela `+0x3E60` do helper
`func_808005FC_chflysaucer` mapeia valores 0..6, respectivamente, aos blocos
`0x80800638`, `0x8080067C`, `0x80800698`, `0x808006B4`, `0x808006D0`, `0x808006EC`, `0x80800708`.
Esses blocos gravam o próximo seletor 1..6 e depois 0. Para 1..5, avançar chama o helper de goto; para 0,
isso depende de `func_800D395C()==0`. Avançar a partir de 6 grava 0 sem goto nesse bloco.

A tabela de inicialização `+0x4214` mapeia o seletor 6 para `0x80803020`: exige mapa `0xD6`, inicializa posição
e chama `func_80801130_chflysaucer`. Esse helper executa classificação de resultado e caminhos de pontuação/
recompensa/diálogo. Portanto, carregar `0xD6` com seletor 6 não significa que todo processamento do evento terminou.
Um caminho do callback de evento em `0x80801A38` chama o avanço e muda a subfase sem exigir nova carga de mapa.

O seletor vem do campo nativo `(0xA34,3)` e é espelhado na extensão `+0x26`. O getter multibit
`func_800DA564` é somente leitura; esse ID acessa o banco `D_8012C770[2]`, distinto da disponibilidade de replay.
Não é necessário aguardar a criação do ator para consultar o valor na fronteira de criação de jogadores.
Vida útil completa do banco e todos os caminhos externos de reset: `UNKNOWN`.

Há três dimensões independentes: estado do ator (`Actor+0x72 >> 10`), subfase do controlador (extensão `+0x25`,
alterada por `func_808012A4_chflysaucer`) e seletor de percurso (`+0x26`). Não identificar um pelo número do outro.

Fontes: `RecompiledFuncs/funcs_20.c:2015`, `funcs_121.c:371`, `funcs_195.c:1539`, `:2720`,
`funcs_38.c:1482`, `funcs_68.c:4835`; getter em `decomp/src/core2/1EB3750.c:219`.
Tabelas escalares conferidas na ROM descomprimida canônica (SHA-1 em [[specs/Tooie/asset-and-licensing-manifest]]); nenhuma reprodução
ao vivo executada nesta revisão.

## Teardown após retirada do primeiro jogador

`PROVEN` (`STATIC_MIPS`): `func_800F482C(0)` destrói vagas ocupadas em ordem crescente; o slot de J1 pode estar
limpo quando o teardown de J2 começa. `func_80098E64`, em `0x80098EBC..0x80098EC8`, testa
`func_8008DAA8(self)` e pula o bloco de serialização se falso. Esse helper chama `func_800F65D0` com o campo
`PlayerState+0x184`. Logo, exigir ambos os jogadores vivos em todo uso do gate impediria a serialização do
segundo após a retirada do primeiro.

Fontes: `RecompiledFuncs/funcs_27.c:4`, `funcs_309.c:312`, `funcs_4.c:3373`;
wrapper `src/game/tooie_dev_forms_hook.cpp`. A integração implementa capacidade de serialização limitada ao
owner de saída reconhecido. Harness com destruição ascendente e teardown/atividade nativos preserva o handoff J2
com J1 ausente, sem reativar gameplay. Serviços de mundo/modelo são sintéticos; jogo real continua `NOT_RUN`.

## Viewports e leitura de controles têm ciclos distintos

`PROVEN` (`STATIC_MIPS`): `func_800A8D70` testa a flag de seção `0x20`. Sem ela, grava uma viewport em
`0x801276D2`; com ela, usa a contagem de `gcstatusDll_entrypoint_11`. Inicializa as viewports e limpa associações
de câmera. Esse corpo não chama o setter de canais (`RecompiledFuncs/funcs_269.c:45..263`).

`PROVEN` (`STATIC_MIPS`): os quatro JAL diretos a `func_80016BDC` nesta saída gerada pertencem a
`chgameselect`: inicialização pede 4; `func_80803888_chgameselect` pede 1 ou 2 conforme `a2`;
`func_808038EC_chgameselect` pede 1; `func_80803B3C_chgameselect` usa `gcstatusDll_entrypoint_10`.
Não é uma prova sobre todos os chamadores indiretos. O helper chama `osContSetCh` com o byte baixo do argumento.
Âncoras: `RecompiledFuncs/funcs_103.c:3157`, `funcs_197.c:1617`, `funcs_243.c:403`, `funcs_286.c:950`,
`funcs_282.c:401..428`.

`PROVEN` (`STATIC_SOURCE`): ultramodern mantém a contagem em `max_controllers` até novo setter/init; não a
reinicializa por mapa. O adaptador librecomp preserva bytes guest das portas não lidas. Fontes:
`lib/N64ModernRuntime/ultramodern/src/input.cpp:86..94`, `:118..122`, `:153..173` e
`lib/N64ModernRuntime/librecomp/src/cont.cpp:45..67`.

Consequência para a suspensão: duas portas sendo lidas não provam dois jogadores/viewports ativos. Não diminuir
canais nem limpar buffers guest para suspender o J2 experimental; preservar admissão/rearme dos atalhos host
por owner. Consumo direto de portas pelos 23 eventos ao vivo e restauração visual: `NOT_RUN` nesta revisão.

## Política implementada e validação parcial

**IMPLEMENTADA; ACEITE PARCIAL COM LIMITAÇÃO CONHECIDA:** suspender o cooperativo experimental por sala de desafio. Ao carregar uma das
salas reconhecidas, conservar a criação nativa de um jogador e uma viewport; J1 joga em tela inteira. Ao sair para
um mapa permitido, o carregamento normal volta a criar J2. A preferência `two_players` permanece ligada.

Essa política também suspende o cooperativo durante exploração, instruções, preparação e resultado dentro da
própria sala. Não equivale a detectar apenas os segundos de partida ativa. Tem a vantagem de usar o ciclo nativo
de criação/destruição por mapa, sem apagar nem congelar um jogador no meio da simulação.

Se for necessário preservar J2 dentro dessas salas antes e depois da partida, os detectores individuais de
início/fim e o mecanismo seguro de suspensão em tempo real são `UNKNOWN`. O helper
`susubgame_entrypoint_2` apenas lê o byte em `a1+0`; a existência desse helper não prova um estado global comum
aos 15 grupos. Não usar um cronômetro ou uma flag de replay como substituto universal.

A implementação deve preservar o subtipo nativo `0x1B`, os lobbies e o jogo com Dev Mode desligado; também deve
evitar que as extensões cooperativas tratem qualquer segundo jogador nativo/auxiliar como J2 da história.
Exceção compartilhada exigida pelo fluxo estático: `0xD6` com seletor Saucer 6 também deve recusar a promoção.
Não bloquear Witchyworld inteira. Se o resultado zerar o seletor sem recriar jogadores, a dupla retorna somente
na próxima criação nativa elegível; não adicionar J2 no meio da simulação.
Validar as 23 etapas, repetição, vitória, derrota, saída e retorno ao cooperativo; incluir as variantes de percurso
do Saucer e as identidades de Kickball ainda não esclarecidas. A investigação original era apenas documental. Implementação host e regressões nativas/sintéticas agora existem;
resumo do estado atual está no [[HANDOFF]]; comandos/resultados detalhados permanecem nas mensagens de commit.
Matriz manual e critérios por evento:
[[specs/Tooie/testing-and-oracle#5.1 Minigame acceptance matrix]]. Matriz completa dos 23 eventos e Linux: `NOT_RUN`.

## Limitação conhecida: corrida de trenzinho com o pássaro

- `PROVEN / USER_REPORT` (Windows, 2026-11-02): o usuário relatou que a suspensão funciona em alguns eventos,
  mas J2 continua acessível durante a corrida de trenzinho com o pássaro. Não houve captura de mapa, controlador,
  subtipo ou fronteira de criação nesse teste; identidade formal e caminho nativo exatos permanecem `UNKNOWN`.
- `PROVEN / STATIC_SOURCE`: a política atual decide na criação nativa de jogadores por mapa, com a exceção
  contextual Saucer já descrita. Não instala detector global de início/fim de corridas, nem remove J2 ao iniciar
  uma atividade no mesmo mapa. `0xC7` (Glitter Gulch Mine principal) permanece elegível e é controle negativo
  explícito no harness; isso não confirma que o relato ocorreu nesse mapa.
- `INFERENCE`: um evento iniciado num mapa de exploração já povoado pode conservar a dupla. Essa explicação
  é compatível com a arquitetura, mas o percurso exato da corrida relatada não foi rastreado neste checkpoint.
- Decisão de escopo do usuário: manter como está por enquanto, documentar e commitar. A correção dessa corrida
  está adiada. Não bloquear um mundo inteiro, alterar flags de replay ou apagar J2 no meio do evento como atalho.
- Se retomado: identificar primeiro o controlador e as fronteiras nativas de início/fim, incluindo repetição,
  vitória/derrota e retorno. A matriz dos 23 eventos continua sem aceite individual; não inventar nomes de casos PASS.

Plano técnico antigo retirado após consolidação: fatos estáticos por família em [[minigame-controllers]],
critérios manuais em [[specs/Tooie/testing-and-oracle]] e extensão adiada em [[future/implementation-ideas]].
Pseudocódigo e registros históricos permanecem no Git; a remoção do plano não representa aceite universal.
Relacionados: [[camera-controls]], [[warp-and-player-position]], [[specs/Tooie/testing-and-oracle]].
