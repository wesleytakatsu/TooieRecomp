---
title: "Plano futuro — cooperativo com dois processos"
description: "Proposta de modo de brincadeira no Dev Mode: dois processos do jogo, cada um com um jogador, vendo-se no mesmo mapa e compartilhando progresso."
status: proposed
type: future-plan
project: TooieRecomp
updated: 2026-10-31
tags: [future, multiplayer, dev-mode, coop]
---

# Plano futuro — cooperativo com dois processos

Estado: **PROPOSTO / NÃO IMPLEMENTADO**. Nada desta nota existe no código. É um modo de brincadeira do Dev Mode, não
um recurso de jogo sério. Registrado a pedido do usuário em 2026-10-31.

## 1. Ideia

Rodar **dois processos** do TooieRecomp ao mesmo tempo, um por jogador, cada um com o modo história normal de um
jogador. Os dois conversam pela rede local para:

- cada jogador andar livre por qualquer mapa, ou seguir o outro (opção no Dev Mode);
- quando estiverem no mesmo mapa, cada um ver o Banjo-Kazooie do outro na posição certa;
- compartilhar o progresso (Jiggies, notas, favos, habilidades, portas abertas) como se fosse um save só.

## 2. Relação com o modo de dois jogadores atual

O Dev Mode já tem um modo experimental de **dois jogadores num processo só** (tela dividida; ver
[[specs/Tooie/window-ui-and-config]], seção Dev Mode). Os dois modos se complementam:

| | Um processo (tela dividida, existente) | Dois processos (esta proposta) |
| --- | --- | --- |
| Inimigos e itens em comum | Já são os mesmos objetos; falta os objetos reagirem ao jogador 2 | Praticamente impossível (nível 3) |
| Cada jogador em um mapa diferente | Impossível: o jogo carrega um mapa por vez | Natural |
| Ver o outro jogador | Natural | Fantasma sincronizado (nível 1) |
| Progresso | Um save, automático | Junção de flags, com atraso (nível 2) |

Conclusão (`INFERENCE`): o modo de um processo é o caminho para jogar junto no mesmo mapa; este plano é o caminho para
explorar separado.

## 3. Níveis de compartilhamento

### Nível 1 — ver o outro (viável)

No mesmo mapa, cada processo mostra um **fantasma** do jogador do outro processo: posição, direção, transformação e
animação. O fantasma não recebe controle e não interage com nada.

Base técnica já conhecida (`PROVEN` no modo de um processo, ver
[[reverse-engineering/systems/warp-and-player-position]]):

- `func_800F73C4(n)` cria os jogadores do mapa; pedir 2 cria o segundo `PlayerState` em `D_80135490[1]`.
- Fora dos minijogos o jogo deixa esse jogador extra no estado de espera `0x74`, sem controle e com a física parada:
  é exatamente o que um fantasma precisa.
- `func_8009C0F8(PlayerState*, f32[3])` posiciona; `yaw_set`/`yaw_setIdeal` giram; `func_800A3410` muda a
  transformação e `func_80092778` recarrega o modelo.
- Sem viewport extra: o processo continua em tela cheia, só com um segundo modelo no mundo.

Pendente para o nível 1:

- **Animação** (`UNKNOWN`): descobrir o identificador e o quadro da animação atual de um `PlayerState` e como forçá-los
  no fantasma (provavelmente pelo `AnimCtrl` de `baanim_getAnimCtrlPtr`).
- **Iluminação** (`UNKNOWN`): o modo de um processo mostrou que dois jogadores dividem a mesma iluminação; o fantasma
  herdaria o problema.
- **Interações indesejadas** (`HYPOTHESIS`): inimigos podem perseguir o fantasma se o jogo os mandar para o jogador mais
  próximo; o modo de dois processos não deve ligar o foco por proximidade do modo de um processo.

### Nível 2 — progresso em comum (viável, com limitações)

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

### Nível 3 — mundo em comum (inviável na prática)

Inimigos nas mesmas posições, apanhando dos dois, objetos móveis iguais. Os dois processos não rodam em sincronia: cada
um simula seus objetos com seu próprio tempo e aleatoriedade. Exigiria um processo "dono" por objeto, desligar a
inteligência da cópia no outro processo, identificar o mesmo objeto nos dois lados e replicar estado objeto por
objeto. **Fora do escopo.** Quem quiser mundo em comum usa o modo de um processo.

## 4. Arquitetura proposta

### Processos e janelas

- Dois executáveis iguais, cada um com seu diretório de configuração (saves, `controls.json`, `dev_cheats.json`) para
  não disputar arquivos. Opção: variável de ambiente ou argumento `--instance N` que muda o diretório.
- Janelas lado a lado (o usuário arruma ou o processo se posiciona na metade da tela). Compor as duas imagens numa
  janela só fica fora do escopo inicial.
- Entrada: `background_input_mode` ligado nos dois; cada processo usa o perfil de um controle só, para um não receber
  os comandos do outro.
- Desempenho: duas instâncias dobram CPU e GPU; medir no PC do usuário.

### Comunicação

- UDP em `127.0.0.1` (mesmo PC) com porta configurável; opcionalmente outro PC na rede local.
- Papéis: `host` (escuta) e `guest` (conecta), escolhidos no Dev Mode.
- Mensagens:
  - `hello`: versão do protocolo, hash da ROM, versão do jogo (recusar se diferentes);
  - `player` (por frame ou a 30 Hz): mapa, posição, direção, transformação, estado, animação e quadro;
  - `flags` (por evento e periódico): lista de flags ligadas desde o último envio;
  - `bye`: ao fechar ou sair do modo.
- Tolerância: pacotes perdidos são substituídos pelo próximo; o fantasma interpola entre as duas últimas posições.

### Lado do jogo

- Thread de rede no host do processo; a thread do jogo só lê o último estado recebido (fila sem bloqueio).
- Criação do fantasma ao entrar num mapa onde o outro está, e remoção ao sair; reaproveita o gancho de
  `func_800F73C4` e o gancho por jogador `func_800991B0` já usados pelos cheats.
- Aplicação de flags na thread do jogo, num ponto seguro (troca de mapa ou fora de cena).

### Dev Mode

Seção nova "Two processes (experimental)":

- papel (`Off`, `Host`, `Guest`), endereço e porta;
- "Follow the other player" (troca de mapa junto) ou livre;
- "Share progress" (nível 2) liga/desliga;
- estado da conexão.

## 5. Etapas sugeridas

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

## 6. Riscos e perguntas em aberto

- Animação e iluminação do fantasma: `UNKNOWN`, exigem RE.
- Flags de história fora de ordem podem travar sequências: começar com lista restrita.
- Duas instâncias no mesmo PC podem disputar áudio e foco de janela; validar `background_input_mode` com dois
  processos.
- Linux: mesmas peças, validação separada no PC Linux do usuário.
