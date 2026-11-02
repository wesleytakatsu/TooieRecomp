---
title: "Cogitações de implementações futuras"
description: "Propostas ainda não implementadas e extensão adiada da suspensão cooperativa; não é um plano de execução ativo."
status: proposed
type: future-ideas
project: TooieRecomp
updated: 2026-11-02
tags: [future, proposals, coop, split-screen]
---

# Cogitações de implementações futuras

Esta nota reúne as partes não implementadas dos antigos planos. Não autoriza implementar nenhuma delas.
Contratos atuais ficam nas specs; limitações observadas e conhecimento nativo ficam no RE. Validação pendente
de código já implementado fica em [[specs/Tooie/testing-and-oracle]], não nesta lista de propostas.

| Cogitação | Estado | Decisão necessária antes de executar |
| --- | --- | --- |
| Cooperativo com dois processos | PROPOSTO / NÃO IMPLEMENTADO | Definir escopo inicial, isolamento de saves e transporte; validar arquitetura antes de tratar viabilidade como comprovada |
| Suspensão cooperativa da corrida de trenzinho com o pássaro | ADIADA / NÃO IMPLEMENTADA | Usuário retoma o ajuste; identificar evento e ciclo nativo antes de projetar suspensão em tempo real |

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

## 3. Proveniência e separação de estados

O antigo `docs/future/two-process-coop.md` foi incorporado aqui. O plano
`PLANO-SPLITSCREEN-MINIGAMES.md` foi retirado: sua política implementada está nas specs/RE; critérios manuais estão
na spec de testes; detalhes estáticos por controlador estão em [[reverse-engineering/systems/minigame-controllers]].
Ambos permanecem recuperáveis no histórico do Git. Nenhum teste pendente foi promovido a PASS nesta consolidação.
