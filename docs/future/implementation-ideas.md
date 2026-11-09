---
title: "Cogitações de implementações futuras"
description: "Propostas de cooperativo, suspensão adiada, mods de personagens, atmosfera visual e teste automatizado de mapas."
status: proposed
type: future-ideas
project: TooieRecomp
updated: 2026-11-08
tags: [future, proposals, coop, split-screen, characters, mods, atmosphere]
---

# Cogitações de implementações futuras

Esta nota reúne as partes não implementadas dos antigos planos. Não autoriza implementar nenhuma delas.
Contratos atuais ficam nas specs; limitações observadas e conhecimento nativo ficam no RE. Validação pendente
de código já implementado fica em [[specs/Tooie/testing-and-oracle]], não nesta lista de propostas.

| Cogitação | Estado | Decisão necessária antes de executar |
| --- | --- | --- |
| Cooperativo com dois processos | PROPOSTO / NÃO IMPLEMENTADO | Definir escopo inicial, isolamento de saves e transporte; validar arquitetura antes de tratar viabilidade como comprovada |
| Suspensão cooperativa da corrida de trenzinho com o pássaro | ADIADA / NÃO IMPLEMENTADA | Usuário retoma o ajuste; identificar evento e ciclo nativo antes de projetar suspensão em tempo real |
| Mods de personagens | ANÁLISE / NÃO IMPLEMENTADO | Definir contratos comuns; estudar Mario, cinco Kongs, animais do DK64 e demais candidatos antes de autorizar protótipo |
| Melhorias visuais e atmosfera | PLANEJAMENTO / NÃO IMPLEMENTADO | Seguir o [guia dedicado](visual-atmosphere/README.md); começar pelo catálogo de chão/paredes; ray tracing excluído |
| Varredura automatizada de mapas | PLANEJAMENTO / NÃO IMPLEMENTADO | [Plano de implementação](#5-varredura-automatizada-de-mapas); começar com Witchyworld e uma sala interna |

Os planos de texturas, luz, água, chuva, dia/noite, vento e efeitos ambientais ficam na pasta dedicada
[visual-atmosphere](visual-atmosphere/README.md), com um documento por melhoria, perfis por cenário e matriz de aceite.
Seu estado é proposto: esta documentação não registra implementação nem validação dos efeitos novos.

## 1. Cooperativo com dois processos

Estado: **PROPOSTO / NÃO IMPLEMENTADO**. Isolamento de instâncias, protocolo de rede, fantasma remoto e sincronização
de progresso ainda não existem como modo cooperativo no código. As primitivas do modo local citadas abaixo já existem,
mas não implementam esta proposta. Registrado a pedido do usuário em 2026-10-31; revisão de estado em 2026-11-02.

### Ideia

Rodar **dois processos** do TooieRecomp ao mesmo tempo, um por jogador, cada um com o modo história normal de um
jogador. Os dois conversam pela rede local para:

- cada jogador andar livre por qualquer mapa, ou seguir o outro (opção no Dev Mode);
- quando estiverem no mesmo mapa, cada um ver o Banjo-Kazooie do outro na posição certa;
- compartilhar o progresso (Jiggies, notas, favos, habilidades, portas abertas) como se fosse um save só.

### Relação com o modo de dois jogadores atual

O Dev Mode já tem um modo experimental de **dois jogadores num processo só** (tela dividida; ver
[[specs/Tooie/window-ui-and-config]], seção Dev Mode). Os dois modos se complementam:

| | Um processo (tela dividida, existente) | Dois processos (esta proposta) |
| --- | --- | --- |
| Inimigos e itens em comum | Mesmos objetos; coleta do J2 e alvo de oito famílias de inimigos integrados, sem cobertura universal | Replicação autoritativa de objetos fora do escopo (nível 3) |
| Cada jogador em um mapa diferente | Impossível: o jogo carrega um mapa por vez | Natural |
| Ver o outro jogador | Natural | Fantasma sincronizado (nível 1) |
| Progresso | Um save, automático | Junção de flags, com atraso (nível 2) |

Conclusão (`INFERENCE`): o modo de um processo é o caminho para jogar junto no mesmo mapa; este plano é o caminho para
explorar separado.

### Níveis de compartilhamento

#### Nível 1 — ver o outro (viabilidade proposta)

No mesmo mapa, cada processo mostra um **fantasma** do jogador do outro processo: posição, direção, transformação e
animação. O fantasma não recebe controle e não interage com nada.

Primitivas do modo local (`PROVEN` apenas nos caminhos auditados, sem provar criação de fantasma remoto; ver
[[reverse-engineering/systems/warp-and-player-position]]):

- `func_800F73C4(n)` inicia a criação dos jogadores. O pedido 2 sozinho não prova uma dupla: caminhos de descritor/fallback podem substituir o laço normal. Reutilizar a proveniência documentada em [[reverse-engineering/systems/minigames]].
- Fora dos minijogos o jogo deixa esse jogador extra no estado de espera `0x74`, sem controle e com a física parada:
  pode ser uma base para um fantasma (`HYPOTHESIS`), mas não comprova isolamento de colisão, animação ou atividade.
- `func_8009C0F8(PlayerState*, f32[3])` posiciona; `yaw_set`/`yaw_setIdeal` giram; `func_800A3410` muda a
  transformação e `func_80092778` recarrega o modelo.
- Sem viewport extra é o resultado desejado; criar um segundo modelo sem alterar topologia/atividade ainda exige investigação (`UNKNOWN`).

Pendente para o nível 1:

- **Animação** (`UNKNOWN`): descobrir o identificador e o quadro da animação atual de um `PlayerState` e como forçá-los
  no fantasma (provavelmente pelo `AnimCtrl` de `baanim_getAnimCtrlPtr`).
- **Iluminação do fantasma** (`HYPOTHESIS`, não implementada): a disputa no modo de um processo foi corrigida com
  vértices de saída próprios por modelo; cálculo e luzes já eram individuais (`PROVEN` no Windows, ver
  [[reverse-engineering/systems/player-lighting]]). O fantasma também precisará dessa saída própria; a correção atual
  está limitada ao modo história com dois jogadores, não implementa o modo de dois processos.
- **Interações indesejadas** (`HYPOTHESIS`): inimigos podem perseguir o fantasma se o jogo os mandar para o jogador mais
  próximo; o modo de dois processos não deve ligar o foco por proximidade do modo de um processo.

#### Nível 2 — progresso em comum (viabilidade proposta, com limitações)

- O progresso do jogo são **flags** no save (`decomp/include/gameflags.h`; save em EEPROM de 2 KiB, ver
  [[specs/Tooie/save-system]]).
- Cada processo mantém **seu próprio arquivo de save**. Os dois nunca gravam o mesmo arquivo: um apagaria o outro.
- Periodicamente (ao trocar de mapa e a cada poucos segundos), cada processo envia ao outro as flags que mudaram; quem
  recebe liga as flags que ainda não tem (união).
- Limitação: um item coletado por um jogador só some para o outro quando o mapa recarregar, porque os itens consultam a
  flag ao nascer (`STRONG_INFERENCE`). Sumir na hora exige tratar cada tipo de item.
- Risco: flags de história (cenas, chefes, conversas) aplicadas no meio de uma sequência podem deixar estados
  inconsistentes. Começar sincronizando só coletáveis e habilidades, com uma lista de flags permitidas.
- Pendente: localizar as funções de ler/gravar flags (`UNKNOWN` nesta base) e o formato dos contadores (notas por mapa,
  ovos, penas) que não são flags simples.

#### Nível 3 — mundo em comum (fora do escopo proposto)

Inimigos nas mesmas posições, apanhando dos dois, objetos móveis iguais. Os dois processos não rodam em sincronia: cada
um simula seus objetos com seu próprio tempo e aleatoriedade. Exigiria um processo "dono" por objeto, desligar a
inteligência da cópia no outro processo, identificar o mesmo objeto nos dois lados e replicar estado objeto por
objeto. **Fora do escopo.** Quem quiser mundo em comum usa o modo de um processo.

### Arquitetura proposta

#### Processos e janelas

- Dois executáveis iguais, cada um com seu diretório de configuração (saves, `controls.json`, `dev_cheats.json`) para
  não disputar arquivos. Opção: variável de ambiente ou argumento `--instance N` que muda o diretório.
- Janelas lado a lado (o usuário arruma ou o processo se posiciona na metade da tela). Compor as duas imagens numa
  janela só fica fora do escopo inicial.
- Entrada: `background_input_mode` ligado nos dois; cada processo usa o perfil de um controle só, para um não receber
  os comandos do outro.
- Desempenho: duas instâncias tendem a aumentar CPU/GPU; custo real ainda não medido. Não pressupor fator exato de 2.

#### Comunicação

- UDP em `127.0.0.1` (mesmo PC) com porta configurável; opcionalmente outro PC na rede local.
- Papéis: `host` (escuta) e `guest` (conecta), escolhidos no Dev Mode.
- Mensagens:
  - `hello`: versão do protocolo, hash da ROM, versão do jogo (recusar se diferentes);
  - `player` (por frame ou a 30 Hz): mapa, posição, direção, transformação, estado, animação e quadro;
  - `flags` (por evento e periódico): lista de flags ligadas desde o último envio;
  - `bye`: ao fechar ou sair do modo.
- Tolerância: pacotes perdidos são substituídos pelo próximo; o fantasma interpola entre as duas últimas posições.

#### Lado do jogo

- Thread de rede no host do processo; a thread do jogo só lê o último estado recebido (fila sem bloqueio).
- Criação do fantasma ao entrar num mapa onde o outro está, e remoção ao sair; reaproveita o gancho de
  `func_800F73C4` e o gancho por jogador `func_800991B0` já usados pelos cheats.
- Aplicação de flags na thread do jogo, num ponto seguro (troca de mapa ou fora de cena).

#### Dev Mode

Seção nova "Two processes (experimental)":

- papel (`Off`, `Host`, `Guest`), endereço e porta;
- "Follow the other player" (troca de mapa junto) ou livre;
- "Share progress" (nível 2) liga/desliga;
- estado da conexão.

### Etapas sugeridas

1. **Instâncias separadas:** `--instance N`, diretórios de config independentes, um controle por instância. Aceite: dois
   processos abertos jogando cada um com seu controle, sem disputar arquivos.
2. **Canal de rede:** `hello`/`bye` e estado de conexão no Dev Mode. Aceite: conectar, desconectar e recusar versões
   diferentes.
3. **Fantasma estático:** posição, direção e transformação no mesmo mapa, com interpolação. Aceite: ver o outro andar
   sem tremer, aparecer e sumir ao trocar de mapa.
4. **Animação do fantasma** (exige RE). Aceite: o fantasma anda, pula e voa com a animação certa.
5. **Seguir o outro:** troca de mapa junto pelo mecanismo de warp (`_gcgoto_entrypoint_1`) do Dev Mode.
6. **Progresso em comum:** união de flags permitidas. Aceite: Jiggy coletado por um aparece coletado para o outro após
   recarregar o mapa; nenhum save corrompido.

### Riscos e perguntas em aberto

- Animação do fantasma: `UNKNOWN`, exige RE. Iluminação: preservar saída de vértices própria por instância, ainda não
  implementada/validada para o fantasma deste plano.
- Flags de história fora de ordem podem travar sequências: começar com lista restrita.
- Duas instâncias no mesmo PC podem disputar áudio e foco de janela; validar `background_input_mode` com dois
  processos.
- Linux: mesmas peças, validação separada no PC Linux do usuário.

## 2. Suspensão da corrida de trenzinho com o pássaro

Estado: **ADIADA / NÃO IMPLEMENTADA**. Usuário decidiu manter o comportamento atual. O modo por sala dos
minigames já está implementado; esta cogitação trata apenas da extensão para a corrida relatada, onde J2 continua
acessível. Identidade formal, mapa e controlador exatos são `UNKNOWN`; não atribuir o relato automaticamente a
Canary Mary ou ao mapa `0xC7`.

Antes de desenhar o ajuste, identificar o controlador e as fronteiras de início, vitória, derrota, repetição,
aborto e retorno. `INFERENCE`: uma atividade iniciada num mapa já povoado pode conservar a dupla porque o host
decide na criação de jogadores, sem detector universal de evento ativo. Essa explicação ainda não é um rastreamento
da corrida real. Não bloquear um mundo inteiro, alterar flags de replay, congelar ou apagar J2 como atalho.

Uma implementação futura precisaria preservar J1 e controles nativos, inventário compartilhado, identidade dos
owners, serialização de formas, viewports e rearme de entrada; restaurar J2 somente numa fronteira comprovadamente
segura. Aceite: corrida sem contribuição do J2, repetição/saídas corretas e retorno cooperativo estável, incluindo
regressão com Dev Mode desligado. Desenho concreto e implementação dependem da investigação e de nova solicitação.

Referências atuais: [[reverse-engineering/systems/minigames]], [[specs/Tooie/window-ui-and-config]] e
[[specs/Tooie/testing-and-oracle#5. Experimental split-screen manual coverage]].

## 3. Mods de personagens

Estado: **ANÁLISE / HYPOTHESIS / NÃO IMPLEMENTADO**. Escopo ampliado a pedido do usuário em 2026-11-07. Esta seção estuda suporte a vários mods de personagens; não autoriza iniciar as fases de implementação do plano antigo do Mario.

Branch local de análise: `codex/character-mods-analysis`, criada da `main` em `fdfbce1ef356bfa13859b7cd25fc9a18d52e28f0`. Comparação inicial: `3912fc24b1d0765c592adb410ec696b2ddb74dc2`, usado no estudo de 2026-10-31. O diff contém 318 arquivos alterados, incluindo grandes dependências de renderização; esse número não mede o tamanho da API de mods.

### Requisitos definidos pelo usuário

Decisão de escopo em 2026-11-07: fidelidade ao jogo de origem e integração aditiva passam a ser requisitos. A exceção explícita são as armas dos Kongs disparando ovos do BT, com estoque e seleção de munição do BT. Estes requisitos descrevem o resultado desejado; a solução continua em análise.

| ID | Requisito obrigatório |
| --- | --- |
| CHAR01 | Adicionar personagens ao catálogo, preservando personagens, formas, entradas e dados originais do BT. Nenhum pacote pode ocupar o ID de uma forma nativa como se fosse sua substituição. |
| CHAR02 | O acesso à seleção de personagens externos deve partir de ESC → aba Debug → ativar seleção de personagens. A nomenclatura Dev Mode usada no código se refere à área existente; não criar um segundo fluxo independente. |
| CHAR03 | Mod instalado, habilitado, válido e com dependências disponíveis acrescenta suas entradas à mesma listagem, junto das originais. Instalar não ativa automaticamente um personagem. |
| CHAR04 | Cada personagem conserva habilidades, máquina de ações e movimento de sua versão de origem: salto, altura, tempo no ar, velocidade, aceleração, controle aéreo, frenagem, virada, escalada, natação, combos e recuperações aplicáveis. |
| CHAR05 | Uma aparência nova sobre movimentos genéricos do Banjo ou de outro personagem não atende à fidelidade exigida. Compartilhar código só é aceitável se o comportamento de cada personagem continuar equivalente ao original. |
| CHAR06 | Os cinco Kongs mantêm suas armas distintas, incluindo modelo, animações, empunhadura, mira e características de disparo originais. A integração precisa estudar cada arma separadamente. |
| CHAR07 | Os tiros dessas armas são ovos do BT. Estoque, tipos disponíveis, seleção/ciclagem de munição e efeitos dos ovos seguem o padrão do BT; não criar um inventário paralelo de munições do DK64. |
| CHAR08 | Trocar de personagem não recria ovos, não duplica recursos e não altera a coleção de personagens nativos. Preservar a semântica de inventário compartilhado ou individual que o modo atual do BT realmente usa. |
| CHAR09 | Seleção e instância ativa são por jogador. Um pacote pode acrescentar vários personagens; dois pacotes ou duas instâncias não podem compartilhar estado mutável por acidente. |

#### Fluxo de acesso e identidade

O fluxo pretendido é: carregar catálogo dos mods instalados → abrir ESC → aba Debug → habilitar seleção → abrir a listagem existente para o jogador → escolher uma entrada nativa ou adicionada → confirmar → aplicar troca em fronteira segura do game thread.

Os controles de navegação, confirmação e cancelamento da lista podem conservar o padrão existente. R3/direção, restauração de configuração ou um atalho novo não devem se tornar uma entrada independente para ativar um personagem externo fora do fluxo autorizado pelo ESC. A restauração de instância após mapa/morte precisa derivar de uma seleção já autorizada, sem autoativar um mod apenas porque foi instalado.

A política ao desligar a opção enquanto um personagem externo está ativo ainda precisa de desenho: encerrar ou suspender a instância e retornar a uma forma nativa válida em fronteira segura, inclusive em água ou espaços estreitos. Não decidir isso apagando o jogador ou trocando forma no meio de uma atualização.

A extensão futura deve conservar as entradas nativas e acrescentar entradas identificadas por pacote/personagem, com tipo distinto do identificador nativo de transformação. Um eventual PlayerState/proxy interno serve à participação no mundo do BT; não redefine os personagens e assets originais do jogo.

#### Fidelidade mensurável

A altura do salto e os demais parâmetros não serão ajustados para se parecerem com o Banjo. Primeiro registrar jogo, região/versão, estado de habilidades/upgrades e taxas de atualização usados como referência para cada personagem.

Comparar em geometria e entradas equivalentes: sequência de ações/substados, impulso inicial, trajetória por tick, ápice, tempo até o ápice, duração no ar, deslocamento horizontal, direção, velocidade, aceleração, frenagem e janelas de encadeamento. Testar entrada positiva, entrada fora da janela e cancelamento.

Conversão de coordenadas pode ser necessária entre motores. Com uma escala espacial fixa s do jogo de origem para o BT, posição relativa, velocidade e aceleração espaciais precisam da conversão correspondente; o tempo real e as janelas de ação permanecem os originais. A convenção e a escala ainda precisam ser definidas. Não escolher uma escala diferente para cada salto ou reduzir alcance por causa de um puzzle do BT.

Tolerâncias numéricas serão justificadas por comparação do controlador com a referência, sem autorizar mudança de parâmetros para balanceamento. Obstáculos de um mapa diferente podem interromper a trajetória; isso não autoriza mudar o salto. Uma animação visual correta, um build ou uma skin não comprovam fidelidade.

#### Armas DK64 com ovos BT

O contrato é dividido em arma e projétil. O Kong conserva sua arma original e sua forma de usá-la; o disparo emite o ovo selecionado pelo sistema do BT, com a identidade e o efeito daquele ovo sobre os alvos do BT. Trajetória, detonação e ciclo de vida específicos de cada tipo de ovo devem respeitar o comportamento do BT quando fizerem parte desse projétil. Essas diferenças em relação à munição DK64 são a adaptação solicitada.

A seleção de ovos deve utilizar o padrão de entrada, tipos, elegibilidade e feedback já adotados pelo BT, respeitando os bindings atuais. Não criar um seletor de munição DK64 concorrente. Desbloqueios e disponibilidade dos tipos precisam ser consultados no jogo hospedeiro; selecionar um Kong não concede munição nem habilidades automaticamente.

O consumo precisa ocorrer uma vez na confirmação do disparo válido no game thread. Não descontar por frame de renderização nem deixar a arma estrangeira e a lógica nativa gerarem dois tiros para a mesma solicitação. Conferir estoque e consumir segundo a mesma autoridade que contabiliza coleta e disparo no BT.

Custo por projétil ou rajada, correspondência das melhorias das armas DK64 e integração de tipos de ovos com mecânica especial ainda precisam ser definidos. O custo não deve ser inventado como requisito do usuário. Ovos com ciclo de vida especial exigem estudo próprio de owner, controle, explosão e teardown, em vez de serem tratados como uma esfera comum.

Os símbolos, contadores e caminhos exatos de troca/consumo de ovos não foram estabelecidos nesta revisão. A flag de tutorial de ovo encontrada no grafo não é um contador de munição. Essa localização é uma pendência de RE, não uma API pronta.

#### Pontos do seletor que precisam evoluir

A inspeção estática delimita quatro dependências do catálogo fixo:

- [Definição de formas](../../include/tooie_dev_forms.h#L9): array com 17 entradas e ID nativo de oito bits.
- [Publicação e entrada](../../src/game/tooie_dev_forms.cpp#L24): seleção derivada da forma nativa, navegação pelo tamanho do array e pedido guardado como índice.
- [Desenho da lista](../../src/game/tooie_dev_forms_ui.cpp#L75): nomes, total e janela de sete linhas leem o mesmo array.
- [Confirmação](../../src/game/tooie_dev_forms_hook.cpp#L142): índice resolve diretamente uma forma nativa e executa o fluxo de transformação existente. A entrada pelo ESC está em [build_character_selection](../../src/game/tooie_dev_mode_ui.cpp#L573).

A proposta é um catálogo combinado, mantendo as entradas nativas e acrescentando as externas. A instância selecionada precisa de identidade própria para que um proxy nativo não faça a UI voltar a mostrar Banjo. Pedidos devem guardar identidade estável e geração do catálogo, não depender de um índice que pode mudar entre a abertura e a confirmação.

Confirmar entrada nativa deve continuar pelo caminho nativo. Confirmar entrada externa deverá encaminhar ao provedor correspondente e ao seu ciclo de ativação. Um personagem externo nunca deve ser enviado como um ID arbitrário para func_800A3410.

#### Aceite adicional previsto

| Caso | Resultado exigido em futura validação |
| --- | --- |
| ADD01 | Sem mods, catálogo e comportamento nativos continuam disponíveis. |
| ADD02 | Instalar um pacote acrescenta somente seus personagens; um pacote com vários e vários pacotes coexistem. |
| ADD03 | Remover/desabilitar pacote ou faltar dependência não remove entradas nativas nem aplica outro personagem por índice deslocado. |
| MENU01 | Fora do fluxo ESC/Debug e com seleção desligada, nenhuma entrada externa pode ser ativada. |
| MENU02 | ESC/Debug, opção ligada, lista e confirmação permitem selecionar o personagem externo para o jogador correto. |
| MENU03 | Abrir a lista, alterar disponibilidade do catálogo e depois confirmar não aplica personagem errado nem reutiliza recursos removidos. |
| FID01 | Traço de salto de cada personagem equivale à referência de origem em entradas/geometria equivalentes e escala documentada. |
| FID02 | Andar, virar, frear, controlar no ar, escalar, nadar e encadear ações preservam regras e diferenças próprias de cada personagem. |
| FID03 | Repetir a simulação a diferentes FPS de apresentação não altera traço, altura nem janelas de ação. |
| AMMO01 | Cada Kong conserva sua arma, mas o tiro é o ovo selecionado pelo padrão BT. |
| AMMO02 | Tipo, efeito e estoque do ovo correspondem aos do BT; estoque vazio, coleta e limites são respeitados. |
| AMMO03 | Disparo confirmado desconta uma vez segundo custo definido; entrada rejeitada não desconta nem gera projétil. |
| AMMO04 | Trocar de Kong, voltar ao nativo, morrer e mudar mapa não reabastecem nem duplicam munição. |
| AMMO05 | No cooperativo, tiros de ambos os jogadores e coleta usam a mesma autoridade de estoque do modo BT, sem contadores DK64 paralelos. |
| AMMO06 | Tipos de ovos especiais respeitam owner, fim de vida e retorno seguro de controle. |

Todos os casos adicionais: NOT_IMPLEMENTED. Os 112 casos antigos continuam sendo a matriz do Mario; esta tabela acrescenta requisitos comuns, sem afirmar execução ou aprovação.

### Resultado da revisão do projeto

O projeto atual oferece mais pontos de integração que a base anterior, especialmente propriedade por jogador e carregamento de conteúdo. Ainda é necessário desenhar e provar uma API para personagens externos.

| Evidência atual | O que permite estudar | Limite da evidência |
| --- | --- | --- |
| [Registro de conteúdo](../../src/main/main.cpp#L1208): textura e áudio têm tipo, callbacks e containers próprios | Registro e ciclo de vida de futuros pacotes de personagens podem seguir uma separação semelhante | O ZIP atual permite conteúdo de textura/áudio; não existe nesse registro um tipo de personagem |
| [Contrato de modding](../specs/Tooie/modding-and-extensions.md#L99) | Identidade de mods, raízes internas e integração com o instalador existente | A whitelist do ZIP exclui código; colocar uma DLL ou um novo JSON dentro dele não habilita um controlador |
| [Seletor de formas](../../include/tooie_dev_forms.h#L9) | Seleção por jogador, fila de pedidos e publicação para UI | Catálogo fixo de 17 entradas nativas e dois jogadores; não é registro extensível de personagens |
| [Troca de forma](../../src/game/tooie_dev_forms_hook.cpp#L129) | Exemplo de encerrar comportamento anterior, trocar modelo/forma e escolher estado inicial nativo | Trabalha com IDs e comportamentos existentes do Tooie; não recebe máquina de ações de outro jogo |
| [Identidade cooperativa](../../src/game/tooie_story_coop.cpp#L48) e [RE de minigames](../reverse-engineering/systems/minigames.md) | Associar owner, porta, câmera e época da sessão; evitar confundir auxiliares com J2 | Slots, câmeras, controles e ponteiros não são a mesma identidade; a implementação atual não é uma API pública |
| [Transição de formas](../reverse-engineering/systems/player-transformations.md#L16) | Identificar serialização e restauração por jogador entre mapas | Os bancos de transição nativos continuam compartilhados; não cabe colocar estado estrangeiro neles sem contrato |
| [Iluminação por instância](../reverse-engineering/systems/player-lighting.md#L15) | Recursos de saída mutáveis precisam de propriedade por instância | Compartilhar o modelo não permite compartilhar buffers de vértices iluminados; modelo primário, LOD e auxiliar importam |
| [Câmera e controles](../specs/Tooie/input-and-controllers.md) | Reutilizar resolução de jogador, controle e câmera no desenho da ponte | Modos especiais, mira e minigames têm reservas próprias; não trocar foco global para resolver um personagem |
| [Backends e portátil](../specs/Tooie/startup-and-backends.md), [PortMaster](../specs/Tooie/handheld-portmaster.md) | Avaliar renderização e código nativo por plataforma desde a arquitetura | Um plugin Windows x64/DX12 não demonstra suporte a Linux, aarch64 ou OpenGL ES |
| [Estado do SDK](../specs/Tooie/modding-and-extensions.md#L207) | A infraestrutura herdada de code mods é uma opção a investigar | A spec ainda registra ausência de SDK público e substituição geral de arquivos; suporte pronto a personagem externo permanece UNKNOWN |

`PROVEN` nesta revisão significa código ou documentação atual inspecionados estaticamente. Resultados de builds e testes já registrados no HANDOFF são históricos; não foram repetidos nesta análise. Portabilidade, jogabilidade e compatibilidade da proposta permanecem `UNKNOWN`.

Correções em relação às premissas de 2026-10-31:

- A afirmação geral de ausência de áudio externo foi superada pelo tipo nativo `audio.json`. Esse tipo substitui músicas por sequência; não resolve automaticamente vozes e efeitos de um controlador estrangeiro.
- O ponto de partida agora é um clone unificado com seleção de formas, cooperativo, câmera analógica e vários backends. Não planejar trabalho num antigo checkout Windows que deixou de existir.
- O catálogo de 233 ações e os 112 casos do Mario continuam referências do estudo de SM64, não requisitos suficientes para a plataforma inteira. O HEAD local do SM64 continua `9921382a68bb0c865e5e45eb594d9c64db59b1af` nesta revisão.
- A ordem imediata passa a ser análise de contratos compartilhados. O antigo pedido de iniciar um harness de Mario/árvore deixa de ser a próxima ação desta etapa.

### Pontos de integração já delimitados

O [hook por jogador](../../src/game/tooie_dev_warp_hook.cpp#L284) executa: escopo do jogador → atualização da elegibilidade da câmera → `func_800991B0` nativa → nova leitura da câmera → publicação da porta de controle → seletor de formas → liberação adiada do J2 e cheats. Isso é a ordem do wrapper inspecionado, não o mapa completo de todos os subsistemas do frame.

Consequência: inserir um tick do Mario somente depois desse wrapper não impede a física nativa de já ter executado. Antes de escolher esse ponto, será necessário decompor o update nativo e identificar o que deve continuar para contatos, atores, animação, câmera e progresso. Não desligar o update inteiro por conveniência.

O [hook de yaw](../../src/game/tooie_dev_warp_hook.cpp#L243) chama a rotina nativa antes do Warp e filtra J1. O comentário imediatamente anterior ao update por jogador documenta formas que não passam por yaw, como Blaster. Logo, yaw não deve ser escolhido como relógio universal de personagens.

As [ligações dos hooks](../../src/game/boot_hw_stubs.cpp#L6163) já registram wrappers para yaw, update e criação. Um provedor não deve sobrescrever esses endereços independentemente e perder funcionalidades existentes; será necessário um ponto de composição e arbitragem explícito.

A [publicação de identidade](../../src/game/tooie_story_coop.cpp#L30) admite troca de endereço do owner preservando proveniência e câmera esperada. Esse mecanismo é referência para estudar handles estáveis, não permissão para adotar qualquer ponteiro no mesmo slot.

Pendências delimitadas: ordem interna de input/física/colisão em `func_800991B0`; consumidores da posição antes/depois; exportação ou adaptação de colisão; submissão de geometria estrangeira com todas as APIs. Nenhum desses contratos está comprovado apenas pela presença dos wrappers.

### Modelo proposto para diversos mods

Três responsabilidades devem ser independentes:

| Responsabilidade | Conteúdo proposto | Exemplo de decisão |
| --- | --- | --- |
| Pacote de personagem | Identidade, nome, versão, origem dos dados, modelo, animações, áudio, perfil físico e capacidades | Dois pacotes podem compartilhar um controlador sem compartilhar estado mutável |
| Provedor de comportamento | Máquina de ações, entrada, passo de simulação, pose e requisitos de consultas do mundo | Mario poderia usar um provedor baseado em libsm64; isso não o torna motor obrigatório de DK64 ou Zelda |
| Ponte com o Tooie | Jogador reconhecido pelo jogo, consultas de mundo, eventos de ataque, progresso, ciclo de vida e renderização | O mundo e os alvos respondem por regras do Tooie; o provedor informa movimento e intenções |

Os nomes acima são conceitos de análise. Nenhum schema, extensão de arquivo, símbolo de ABI ou interface já está implementado por esta proposta.

O requisito atual é conservar o comportamento original de cada personagem, com a adaptação de munição explicitamente definida acima. Aparência sobre controle nativo ou movimentos apenas inspirados ficam fora dos critérios de aceite deste estudo. Reutilizar um provedor é aceitável somente quando ele reproduzir o comportamento exigido da versão escolhida.

Vários pacotes instalados devem coexistir. Cada jogador tem uma seleção de personagem e exatamente um controlador de locomoção ativo, salvo modos compostos explicitamente desenhados. J1 Mario/J2 Banjo e J1 Banjo/J2 Mario são cenários diferentes. Dois Marios exigem duas instâncias independentes do controlador.

Não estender automaticamente o limite atual de dois jogadores de história. O registro pode prever instâncias sem embutir esse número na identidade dos pacotes; a disponibilidade de slots continua obedecendo ao modo nativo e à integração existente.

### Contratos a investigar antes de código

| Contrato | Decisão a esclarecer |
| --- | --- |
| Identidade | Separar package ID, character ID, provider ID e instância por jogador. Relacionar owner guest à identidade atual sem congelar endereço bruto: o cooperativo já precisou lidar com troca de ponteiro na tabela sem teardown. |
| Capacidades | Descrever caminhar, saltar, nadar, voar, escalar eixo, agarrar beirada, atravessar teto, investir, morder, golpear e usar projéteis. Capacidade não implica autorização para cumprir qualquer puzzle. |
| Forma física | Dimensões, origem, apoio, alcance, passo, teto e volumes dinâmicos por ação. Rambi, Enguarde e uma forma pequena não cabem num único perfil humanoide. |
| Movimento | Um dono por atualização. Publicar posição, velocidade, orientação e contatos necessários ao Tooie, confirmando ordem real do frame e consultas de atores/gatilhos. |
| Mundo | Oferecer colisão, materiais, plataformas, águas e escaláveis. Comparar exportar geometria com adaptar consultas nativas, conforme necessidades de cada provedor. Não impor triângulos SM64 como única API universal. |
| Tempo | Mario pode exigir passo próximo a 30 Hz. Taxa, ordem e subpassos dos demais jogos precisam de análise própria; FPS de apresentação nunca escolhe física de todos os personagens. |
| Ataques | Normalizar intenção, contato, janela e resposta por alvo. Ground pound, investimento de Rambi, mordida e espada não se tornam o mesmo golpe. |
| Saúde e progresso | Tooie mantém autoridade de save, inventário e objetivos. Saúde/recursos internos do controlador precisam de conversão explícita e aplicação única. |
| Câmera e entrada | Resolver ação, analógico e câmera por jogador; reservar mira, jetpack, voo, nado ou controle externo quando necessários. Referências estrangeiras à câmera não podem assumir foco global. |
| Recursos | Compartilhar dados imutáveis quando seguro e manter pose, buffers mutáveis, luz, sombra, efeitos e contatos por instância. Sem ponteiros persistentes em snapshots de troca de mapa. |
| Transições | Criar, ativar, suspender, morrer, respawnar, teleportar, trocar mapa e remover instância. Revalidar posição de saída para personagens que não podem ocupar o meio atual. |
| Conflitos | Detectar IDs repetidos, provedores incompatíveis, versões de ABI e dois mods tentando substituir o mesmo dono de movimento. Ordem visual de mods não decide implicitamente o controlador ativo. |
| Ativação | Separar instalação, catálogo disponível e personagem em uso. Permitir instalar vários não significa descarregar código nativo com callbacks ainda ativos durante gameplay. Estudar fronteiras seguras e possibilidade de reinício obrigatório. |
| Persistência | Guardar escolha por perfil/jogador fora do save nativo; definir fallback se pacote faltar. Troca precisa preservar progresso e impedir duplicação de coletáveis/recursos. |
| Plataforma | Versionar dependências e arquiteturas suportadas; renderização integrada à cena, com profundidade e interpolação, e orçamento adequado ao portátil. |
| Extração privada | Dados de cada jogo vêm do arquivo privado correto e validado. Ausência de um jogo deve afetar só os pacotes que dele dependem. |

Uma futura API deve oferecer extensões versionadas ou recursos opcionais em vez de um único estado universal com todos os campos de todos os jogos. O mínimo comum precisa ser demonstrado por personagens com exigências diferentes.

### Caminhos técnicos em comparação

| Caminho | Vantagem esperada | Custo ou limitação |
| --- | --- | --- |
| Usar primitivas nativas como suporte interno | Pode ajudar na participação do personagem no mundo do BT | Não autoriza trocar o moveset original por um controle genérico; adaptação apenas visual não atende ao requisito atual |
| Extrair controlador do jogo doador | Permite estudar preservação de ações, substados e física próprios | Exige isolar dependências de colisão, ator, câmera, recursos, áudio e eventos; recompilar o jogo inteiro não faz esse isolamento |
| Usar biblioteca já extraída | Mario possui candidato concreto em libsm64 | Ações dependentes de objetos, como árvores, ainda exigem adaptação; isso não demonstra biblioteca equivalente para DK64/BK/Link |
| Executar contexto guest do doador isoladamente | Alternativa especulativa se a extração não for viável | Muito maior: memória, dispatch, scheduler, gráficos e efeitos precisam de isolamento. Não chamar funções recompiladas do DK64 usando RDRAM/PlayerState do Tooie. Viabilidade e custo UNKNOWN. |

Mesmo sendo jogos N64 e usando ferramentas da mesma família, structs, assets, overlays e globais não possuem compatibilidade automática. A ponte deve trocar dados definidos pelo contrato, não identidades ou ponteiros internos de outro jogo.

### Ordem de estudo dos candidatos

A ordem respeita Mario, os cinco Kongs, animais/outros do DK64 e demais personagens. As capacidades abaixo são perguntas de extração e integração; não representam suporte existente.

| Etapa de análise | Candidato | Questões que precisam ser estudadas |
| --- | --- | --- |
| 1 | Mario do SM64 | Ações e cambalhotas já catalogadas; árvore/poste com proxy; colisão real, água, ataques e integração do provedor. Revisão candidata da libsm64 continua referência fixa a reconferir antes de qualquer integração. |
| 2 | Donkey Kong | Movimentos básicos, ataques e habilidades específicas; dependência de animação, arma, recurso e ativadores do cenário. |
| 2 | Diddy Kong | Diferenças do controlador, cauda/saltos, arma e eventual jetpack; controle e câmera de voo não são inferidos do Mario. |
| 2 | Lanky Kong | Posturas e locomoção especial, dimensões de contato, habilidades e dependências dos ativadores. |
| 2 | Tiny Kong | Tamanho/escala, movimentos aéreos e habilidades; espaço para crescer novamente sem atravessar cenário. |
| 2 | Chunky Kong | Porte, ataques e mudanças de tamanho/força; colisão e interação com objetos pesados. |
| 3 | Rambi | Arquétipo terrestre de porte grande: investimento, alvo, frenagem, curvas, rampas e saída da transformação. |
| 3 | Enguarde | Candidato aquático correspondente ao peixe citado: locomoção e ataque submersos, volume d'água e saída segura quando não houver água. |
| 3 | Outros do DK64 | Inventariar cada candidato e distinguir jogável, transformação, montaria e NPC. Krusha aparece na tabela de atores consultada, mas moveset e escopo de portabilidade não foram analisados. |
| 4 | Formas do BK | Cupim, crocodilo, morsa, abóbora e abelha; comparar movimento/colisão, habilidades e dados com formas realmente existentes no Tooie, preservando diferenças. |
| 5 | Yoshi | Jogo e versão ainda UNKNOWN; movimento e habilidades originais são obrigatórios. Estudar língua, engolir/ovos, salto e natação conforme a versão escolhida. |
| 5 | Luigi | Jogo e versão ainda UNKNOWN; comportamento original é obrigatório. Compartilhar provedor com Mario depende de equivalência demonstrada, não basta mudar perfil/modelo. |
| 5 | Link | Zelda e versão ainda UNKNOWN. Ocarina of Time e Majora's Mask são candidatos distintos; estudar combate, mira, câmera, itens, escalada, água e idade/forma quando aplicáveis. |
| Posterior | Outros | Aplicar o mesmo inventário de capacidades e dependências antes de prometer compatibilidade. |

O catálogo deve conservar as habilidades e movimentos próprios do personagem. A ponte ainda precisa mapear seus efeitos sobre o mundo, os poderes e os gatilhos de acesso do Tooie. Para os Kongs, armas originais disparam ovos, com estoque e seleção do BT, conforme requisito definido. Completar campanha somente com um desses personagens continua sendo investigação adicional.

### O que há nas fontes dos doadores

- **SM64:** checkout local continua na revisão do catálogo anterior. Os 233 estados e 112 casos permanecem como estudo de Mario, com cobertura estática; nenhum personagem foi integrado.
- **DK64:** checkout local `E:\projetos\Donkey-Kong-64-Recompiled` em `3b0c36d3fbfeb0b5d2c47923f35cf84b31edcbf7`, remoto Rainchus/Donkey-Kong-64-Recompiled. `patches/patches_draw.c:17` lista DK, Diddy, Lanky, Tiny, Chunky, Krusha, Rambi e Enguarde. `patches/patches_ui.c:1129` identifica o contador da arena Rambi/Enguarde.
- **Limite do DK64 local:** os cinco submódulos estão não inicializados, incluindo `lib/dk64_decomp`, fixado em `cf8b767257f1e4067fe1bad9e8c3b9db51a9db3f`. Inspeção de patches e identidade de atores não prova extração da física/controladores. Nenhum submódulo foi baixado ou alterado nesta análise.
- **BK:** o [decomp primário](https://github.com/n64decomp/banjo-kazooie) e seu [cabeçalho de enums](https://github.com/n64decomp/banjo-kazooie/blob/master/include/enums.h) fornecem referência inicial. As flags de pagamento nomeiam TERMITE, PUMPKIN, WALRUS, CROC e BEE. A presença das cinco formas não prova formato de assets ou controlador intercambiável com o Tooie.
- **Link:** os projetos primários [Ocarina of Time](https://github.com/zeldaret/oot) e [Majora's Mask](https://github.com/zeldaret/mm) estão disponíveis como fontes de estudo. A existência de decomp não é evidência de biblioteca de personagem isolada. A escolha do jogo ainda não foi feita.

As fontes remotas acima foram consultadas em 2026-11-07. Revisões de BK e Zelda deverão ser fixadas quando o estudo desses controladores começar; não há extração detalhada deles nesta revisão.

### Matriz de análise comum

Esta matriz especifica perguntas e discriminadores para uma eventual implementação. Nenhum experimento de runtime desta proposta foi executado.

| Estudo | Comparação ou cenário mínimo | Resultado necessário da análise |
| --- | --- | --- |
| A1 Registro e disponibilidade | Vários pacotes instalados, IDs repetidos e dependência ausente | Definir descoberta, rejeição, compatibilidade e fallback sem confundir pacote instalado com seleção ativa |
| A2 Propriedade de instância | Dois personagens diferentes, dois iguais e troca de owner sem teardown | Mapear dono de movimento, estado, pose, input, câmera e geração de mapa |
| A3 Mundo e física | Mario, Rambi, Enguarde e uma forma pequena/voadora como arquétipos de contrato | Documentar necessidades distintas e o mínimo compartilhado, sem exigir portar todos para concluir o desenho |
| A4 Ações e eventos | Ground pound, golpe, investimento, mordida e espada contra o mesmo alvo | Definir categorias, janelas, resposta do alvo e limites de equivalência |
| A5 Transições | Água para terra, forma grande para sala pequena, morte, mapa e pacote ausente | Escolher política de recusa/saída/retorno coerente com o estado e com progresso |
| A6 Cooperativo e modos especiais | J1/J2 com provedores diferentes; minigame que suspende J2 | Mapear fronteira de suspensão e retomada sem criar jogador nem trocar forma no meio de evento incompatível |
| A7 Render e áudio | Modelo primário/auxiliar, buffers mutáveis, câmeras distintas e backends | Identificar caminho de submissão e propriedade de recursos, sem assumir suporte apenas pelo desenho do Banjo |
| A8 Empacotamento | Conteúdo declarativo separado de código nativo por plataforma | Comparar alternativas de container e ABI, sem publicar schema como contrato existente |
| A9 Fidelidade e campanha | Movimento original, armas DK64 com ovos BT, adaptação de puzzles e retorno nativo | Definir referência original por personagem, validar a exceção de munição solicitada e identificar lacunas de progressão |

A análise estará pronta para uma decisão de implementação quando houver: contratos mínimos escritos, pontos exatos do frame identificados, inventário de dependências do Mario e do primeiro controlador DK64, política de seleção/retorno e limites por plataforma. Só então propor um protótipo delimitado; nenhum código está autorizado por esta seção.

`NEXT_SINGLE_ACTION` desta análise: mapear estaticamente a ordem de entrada, update do jogador, colisão, consultas de atores e desenho na base atual, produzindo uma tabela de quais dados um provedor Mario precisaria ler/publicar e confrontando-a com os requisitos dos arquétipos terrestre grande e aquático.

## 4. Proveniência e separação de estados

O antigo `docs/future/two-process-coop.md` foi incorporado aqui. O plano
`PLANO-SPLITSCREEN-MINIGAMES.md` foi retirado: sua política implementada está nas specs/RE; critérios manuais estão
na spec de testes; detalhes estáticos por controlador estão em [[reverse-engineering/systems/minigame-controllers]].
Ambos permanecem recuperáveis no histórico do Git. Nenhum teste pendente foi promovido a PASS nesta consolidação.

## 5. Varredura automatizada de mapas

Estado **PROPOSTO / NÃO IMPLEMENTADO**. Objetivo: executar o jogo real por comando, teleportar para cada ambiente
válido da ROM USA e observar cinco segundos de execução depois que cada carregamento terminar. O teste roda por
código, sem IA, cliques ou simulação de jogador. A cobertura mede carga e estabilidade inicial, sem exigir ações
de combate, progressão ou coleta.

### 5.1. Inventário e casos de teste

1. Partir dos 228 IDs nomeados de [maps.h](../../decomp/include/maps.h) e cruzá-los com a tabela nativa de mapas e
   as entradas da ROM USA verificada. Os 37 IDs sem nome entre `0xA0` e `0x1A8` também precisam ser classificados.
   A presença de um nome, como `NULL` ou `UNUSED_JINJO_HOUSE`, não comprova que o destino é carregável.
2. Manter um manifesto versionado com `case_id`, `map_id`, nome, entrada, perfil inicial e classe do caso. Incluir
   áreas principais, interiores, arenas, minigames, mapas multiplayer e cenas com o contexto nativo adequado.
   Usar IDs distintos para variantes que carregam recursos ou modos diferentes. Uma entrada validada por mapa
   basta para a primeira cobertura de ambientes. Várias entradas no mesmo mapa são casos de regressão adicionais.
3. Distinguir mapa válido, reservado, alias e caso pendente. Registrar a justificativa e a evidência de cada exclusão.
   Mapas válidos sem entrada confirmada ficam `NOT_RUN` e impedem declarar cobertura completa. Para cenas que
   saem por conta própria antes de cinco segundos, registrar `EARLY_EXIT` com o destino observado. Só admitir
   uma regra de sucesso específica depois de validar o término nativo esperado; ela fica distinta de `PASS_5S`.

### 5.2. Executor dentro do jogo

1. Criar um modo opt-in, por exemplo `TOOIE_TEST_MAP_SWEEP=<manifesto>`, no mesmo executável que será validado
   para a release. Exigir um perfil de teste isolado e compatível antes de consumir o manifesto. Com a opção
   ausente, o fluxo normal do jogo não deve executar a varredura.
2. Inicializar o renderer real e abrir um save de teste pela lógica nativa. Avaliar `TOOIE_AUTO_START` para o boot,
   verificando no piloto como selecionar o save automaticamente. O ponto de controle roda no thread guest, num
   ponto seguro do loop nativo. Reusar `_gcgoto_entrypoint_1(map, entrance)`, usado pelo Warp, e verificar a rota
   de inicialização para modos sem jogador. O hook atual do Warp depende de J1 e não pode ser a única fonte de
   progresso. Referência em [[reverse-engineering/systems/warp-and-player-position]].
3. Implementar a máquina de estados `BOOT → PEDIDO → TRANSIÇÃO → PRONTO → OBSERVAÇÃO → RESULTADO`. Antes do
   teleporte, gravar o destino, a entrada, o mapa anterior e o identificador do caso. Confirmar a execução de uma
   nova transição, o mapa efetivo, o fim do carregamento e do fade, e a retomada de quadros apresentados. Usar
   `0x80132DC2` e `0x80127642` como sinais já conhecidos, validar o sinal de conclusão do fade no piloto e incluir
   um identificador de geração para distinguir recarga do mesmo mapa de estado anterior ainda ativo.
4. Contar cinco segundos com relógio monotônico a partir de `PRONTO`. Exigir progresso de ticks do jogo e novos
   presents durante a observação. `RT64::presentSuccessCount` indica presents bem-sucedidos; a conclusão da
   transição e, nos casos visíveis, uma amostra de quadro após o fade ajudam a detectar tela preta com presents
   ativos. Validar o critério visual em salas escuras antes de usá-lo como falha. Ver [[specs/Tooie/graphics-and-presentation]].

### 5.3. Supervisor e isolamento

1. Implementar um supervisor Windows em `tools/windows/`, inicialmente como script PowerShell. Receber o diretório
   do build, manifesto e diretório de resultados. Preparar uma cópia executável de teste com `portable.txt`, ROM USA
   fornecida localmente, config explícita e save de teste reproduzível. Confirmar o caminho efetivo de dados no
   launcher e no processo do jogo antes de iniciar. Conferir hashes dos saves reais antes e depois do piloto.
2. Iniciar o jogo sem interação e acompanhar o PID que executa o jogo, inclusive quando houver supervisor do
   launcher. Fixar o backend do perfil e confirmar o backend efetivo. Usar deadlines separados para boot,
   carregamento e ausência de progresso. Consumir eventos JSONL com flush em pontos de transição e batimento
   periódico. Gravar em arquivo próprio, independente do redirecionamento de `TOOIE_GRAPHICS_LOG`.
3. Percorrer os mapas na mesma sessão, na ordem do manifesto, para conservar o histórico do heap. Quando um
   caso falhar, guardar a última transição, PID, código de saída, logs e dump, se disponível. Em timeout, coletar
   o diagnóstico antes de encerrar os processos pertencentes à rodada. Se houver continuação após crash, iniciar
   outra sessão com o mesmo perfil para os casos restantes. Manter a falha original e registrar a divisão em
   sessões. Um novo teste do par anterior → destino pode ajudar a reproduzir, sem apagar a falha da varredura.

### 5.4. Relatório e critério de release

- Registrar por caso o destino pedido e efetivo, entrada, anterior, tempos de carga e observação, progresso de
  ticks e presents, sessão, resultado e caminho dos artefatos. Usar resultados distintos para `PASS_5S`, mapa
  incorreto, crash, timeout de carga, perda de progresso, tela preta persistente, saída antecipada e `NOT_RUN`.
- Identificar a rodada pelo commit, SHA-256 do executável e do manifesto, hashes dos insumos privados e versão do
  perfil, plataforma, GPU e backend. Escrever resumo legível e JSON consumível pelo script de empacotamento.
- Primeiro rodar o piloto local. Só habilitar a trava de release depois que os casos obrigatórios cobrirem o
  inventário validado. A trava aceita apenas um relatório completo para o mesmo executável e perfil exigidos.
  Crash, timeout, mapa errado ou caso obrigatório sem resultado impedem o pacote. Manter os artefatos privados
  localmente. A suíte host atual continua servindo como validação rápida e independente.

### 5.5. Ordem de implementação e aceite

| Etapa | Entrega | Aceite |
| --- | --- | --- |
| 1. Piloto | Manifesto curto com mapa inicial, Witchyworld, sala interna e retorno | Executar a sequência por comando com um save isolado e zero interação manual. Medir cinco segundos após o fim de cada carga. |
| 2. Falhas controladas | Exercitar a máquina de estados e o supervisor com relógio e processos de teste | Falha de boot, mapa incorreto, processo encerrado e travamento geram códigos e relatórios distintos. Reinício preserva a falha e os casos pendentes. |
| 3. Inventário completo | Cruzar nomes, tabela nativa e entradas; preparar perfis de modos especiais | Todo ID tem uma classificação justificada. Cada ambiente válido tem uma entrada testável ou aparece como pendência explícita. |
| 4. Varredura de release | Executar o catálogo válido com captura no build Windows final | Relatório completo, sem falhas ou casos obrigatórios pendentes. Verificar que o empacotamento rejeita relatório incompleto ou de outro binário. |

Primeira ação de implementação: validar em Windows o boot isolado e uma transição por código para Witchyworld,
identificando o ponto seguro no thread guest e o sinal de fim de carregamento antes de ampliar a lista.
