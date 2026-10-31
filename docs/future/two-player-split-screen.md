---
title: "Dois jogadores em tela dividida (Dev Mode, experimental) — estado e continuação"
description: "Handoff do modo experimental de dois jogadores no modo história: o que funciona, ganchos, endereços, diagnósticos temporários e próximos passos."
status: in-progress
type: handoff
project: TooieRecomp
updated: 2026-10-31
tags: [dev-mode, multiplayer, split-screen, handoff]
---

# Dois jogadores em tela dividida — estado e continuação

Checkpoint experimental em `E:\projetos\TooieRecomp-win`, branch `codex/two-player-split-screen`, derivada de
`feat/dev-mode` (base `f42fdae10cdb6bde7b5ca2a09ce4b898f4c68e0b`, release v0.1.3). A implementação parcial foi
preservada; os diagnósticos temporários foram removidos antes do commit. Plano relacionado (futuro, não iniciado):
[[future/two-process-coop]]. Contrato atual: [[specs/Tooie/window-ui-and-config]]; RE canônico:
[[reverse-engineering/systems/warp-and-player-position]].

## 1. O que funciona (playtest anterior do usuário no Windows, relatado no handoff de origem)

O usuário confirmou que o estado atual é adequado para pai e filho jogarem juntos. O foco atual é iluminação
independente; IA e portas continuam fora desta correção. O A/B automatizado de iluminação está na seção 8.

- Dev Mode → seção **Experimental** → "Two players (split screen)" (`dev_cheats.json`, chave `two_players`). Só vale
  com a aba desbloqueada na sessão e a partir do próximo carregamento de mapa. Requer Multiplayer ligado e o jogador 2
  atribuído a um controle em Controls.
- Tela dividida com duas viewports; J2 aparece e é controlado pelo controle 2; os dois trocam de mapa juntos.
- J2 aparece ao lado do J1 (spawn imediato ao lado; 2 s depois, com o J1 parado, é recolocado ao lado e liberado).
- J2 pega itens, ataca inimigos e os ovos dele acertam inimigos.
- Iluminação independente: problema gráfico resolvido, confirmado pelo usuário no Windows em 2026-10-31. O cálculo já
  era individual; a correção separa os vértices de saída RGB de cada jogador/modelo (`PROVEN`, seção 8).

## 2. O que falta (prioridade)

1. **Inimigos não perseguem nem atacam o J2.**
2. **J2 não usa portas/trocas de mapa.**
3. Morte do J2 (queda) mata "o jogador" (morte conjunta).
4. Cheats do Debug Button só valem para o J1.
5. Câmera do J2 levemente desorientada (observado; pode ter sido efeito de tentativas já revertidas — reavaliar).
6. Playtest específico de transformações/modelo secundário e Linux `NOT_RUN`.

## 3. Como foi feito (código)

Arquivos: `src/game/tooie_dev_warp_hook.cpp` (ganchos), `include/tooie_dev_warp_hook.h`, `src/game/boot_hw_stubs.cpp`
(registro com `recomp::overlays::add_loaded_function`), `include/tooie_dev_cheats.h` / `src/game/tooie_dev_cheats.cpp`
(`Settings::two_players`, `two_players_active()`, `two_players_map()`), `src/game/tooie_dev_mode_ui.cpp` (seção
Experimental), `tests/tooie_dev_cheats_test.cpp`.

| Gancho (vram) | Função do jogo | O que fazemos |
| --- | --- | --- |
| `0x800F73C4` `tooie_dev_on_create_players` | cria `n` jogadores ao carregar mapa (`n` = `gcstatusDll_entrypoint_11`: 1 fora do modo de jogo `0x1B`) | se ativo e mapa permitido: soma 1 viewport (`func_800A8FDC`, contador s16 `0x801276D2`), lê 2 portas de controle (`func_80016BDC(2)` = `osContSetCh`), pede `n=2`; põe J2 ao lado do J1; agenda o reposicionamento |
| `0x800991B0` `tooie_dev_on_player_update` | atualização por jogador (roda em toda forma) | J2: após 2 s e J1 parado (`0x1`), posiciona J2 ao lado (+60 em Y) **ainda em espera** e no frame seguinte troca `0x74`→`0x1`; também roda os cheats do J1 |
| `0x800F65D0` `tooie_dev_on_player_active` | "jogador índice X está ativo?": 1 para todos no modo `0x1B`, senão só o índice em foco (`D_801354DC`) | responde 1 para o índice 1 → J2 passa a colidir (itens, ataque) |
| `0x8009C88C` `tooie_dev_on_yaw_update` | `yaw_update` (não roda no Breegull Blaster) | Warp; ao aplicar posição do J1 também põe o J2 ao lado |

Fatos de RE descobertos (todos `PROVEN` por log em jogo, salvo indicação):

- Lista de jogadores: `PlayerState* D_80135490[8]`; `func_800F6140` cria em vaga livre; `func_800A907C(slot, ctrl, vp)`
  liga jogador→controle/viewport (tabela `0x801276D0`: `+0x0` contagem, `+0x2` viewports, `+0x4` usos por controle,
  `+0x8` controle por vaga, `+0x18` viewport por vaga, `+0x38` câmera por vaga).
- `func_800A8D70` (carga do mapa) põe viewports = 1 fora dos mapas multijogador.
- A seleção de save chama `func_80016BDC(1)` (`osContSetCh`): só a porta 1 é lida depois disso.
- Jogador extra fica no estado **`0x74`** (espera, sem controle, física parada, só animação base). Ao sair de `0x74`
  num ponto sem chão, cai; posicionar já em queda não segura — posicionar em espera e só depois liberar.
- Leitura de botões: `bakey_update` só repassa com `func_80091E80(self, 0x40)` (pilha de modos em `PlayerState+0x48`,
  topo `0x7F` nos dois) — não era a trava.
- Colisão jogador→objeto é feita pelo **jogador**: `func_800991B0` → `func_8008E6F0` → (gate `func_8008DAA8` →
  `func_800F65D0`) → `func_800CDBA8` (busca) → `func_8008E530` → `func_800EB210` → `func_80109F18` → `func_8010114C` →
  `func_8010108C` → callback do objeto (ex.: `chhoney`) → `func_800FFAB0` (marca ator para apagar).
- Objetos consultam o jogador em foco via `func_800F54E4()` (lê `D_801354DC`) e a API `func_800F4xxx..F8xxx(index,…)`.
  **Trocar `D_801354DC` por ator mais próximo foi testado e REVERTIDO** (`REFUTED` como correção): quebrou o ovo do J2 e
  a câmera sem fazer inimigos perseguirem. Laço de atores: `func_800FF62C`; `func_80100368(actor)` = ActorData.

## 4. Próximos passos sugeridos

1. **Inimigos perseguirem/atacarem o J2:** IA usa `func_800F54E4()` (sempre J1). Rastrear um inimigo perseguindo o J1
   com o método de pilha nativa (seção 5) a partir de uma função de dano ao jogador ou da chamada de posição do
   jogador dentro do update do inimigo; preferir um gate do tipo `func_800F65D0` ou uma escolha de alvo explícita no
   inimigo em vez de trocar o foco global.
2. **Portas:** procurar quem dispara a troca de mapa (`func_800A7990` / `_gcgoto_entrypoint_1`) e se o gatilho testa
   só o jogador em foco; rastrear com pilha nativa uma porta usada pelo J1.
3. **Iluminação resolvida:** usuário confirmou a correção visual no Windows (seção 8). Hipótese de
   valor global único de luz `CORRECTED`: os estados de luz e tint são individuais; o conflito era a saída dos vértices.
4. Morte do J2 sem matar o J1; cheats para o J2 (atalho no controle 2, alvo `D_80135490[1]`).
5. Checkpoint: `TOOIE-DIAG` removido, spec `window-ui-and-config.md` (seção Dev Mode), RE
   `warp-and-player-position.md`, `HANDOFF.md` e registro dos testes atualizados. Validações automatizadas do
   checkpoint estão na seção 7; elas não substituem o playtest.

## 5. Diagnósticos temporários removidos neste checkpoint

Os ganchos, registros, declarações e dependências abaixo foram removidos. A lista descreve o método da investigação
anterior; não indica instrumentação ativa no código atual.

- `src/main/main.cpp`: contagem de leituras por porta (`[tooie-2p-input]`).
- `tooie_dev_warp_hook.cpp`: `[tooie-2p-mode]` (estado/flags por jogador, usa `bakey_getControllerIndex`),
  `[tooie-2p] slot controllers…`, `[tooie-2p-chain]` (pilha nativa em `func_800FFAB0` via `CaptureStackBackTrace`,
  com `#include <windows.h>` no topo), `[tooie-2p-coll]` (contadores em `0x8008E6F0`, `0x8008E530`, `0x800EB210`);
  registros correspondentes em `boot_hw_stubs.cpp` e `tooie_dev_warp_hook.h`.

Método de pilha nativa (funcionou na investigação anterior): gerar cópia com PDB e resolver offsets. Scripts lidos e
mantidos fora do repositório em `F:\Recursos do Tooie\Scripts do Claude Code` (`link_dbg_win.py`, `symchains.py`).
Ambos usam caminhos absolutos para este checkout e requerem o ambiente VS e os artefatos locais correspondentes.
Para reutilizar o método é preciso instrumentar novamente o ponto investigado, com diagnóstico default OFF.

```text
# no VS dev shell (scratch vsdev.ps1): manter rsp e religar com PDB
touch build-win/CMakeFiles/TooieRecompiled.dir/src/game/tooie_dev_warp.cpp.obj ; ninja -C build-win -d keeprsp TooieRecompiled.exe
python "F:\Recursos do Tooie\Scripts do Claude Code\link_dbg_win.py"  # gera build-win/TooieRecompiled_dbg.exe + .pdb
# rodar o _dbg.exe, capturar stderr, depois resolver com dbghelp (llvm-symbolizer não lê só símbolos públicos):
python "F:\Recursos do Tooie\Scripts do Claude Code\symchains.py" chains.txt
```

Os nomes vêm como `weak.func_XXXXXXXX...` = função recompilada do endereço guest.

## 6. Avisos

- Rodar o jogo só com backup de `%LOCALAPPDATA%\TooieRecompiled`; nunca "Save and Quit" em teste.
- Fechar o jogo antes de linkar (exe travado). Build rápido: `ninja -C build-win TooieRecompiled` no VS dev shell.
- Scripts Python de edição: cuidado com `\n` em heredoc (virou quebra de linha real) e com `index()` que pega a
  primeira ocorrência (já duplicou um trecho do arquivo uma vez).

## 7. Validação do checkpoint

Base: `f42fdae10cdb6bde7b5ca2a09ce4b898f4c68e0b`. Resultados e comandos exatos também ficam na mensagem do commit.
Reprodução limpa em jogo e Linux: `NOT_RUN`. `markdown_vault_audit` não está disponível neste ambiente.

| Comando | Exit code | Resultado observado |
| --- | --- | --- |
| `pwsh -NoProfile -File tools/windows/Build-Tooie.ps1 -Stage Test` | 0 | 17/17 testes obrigatórios passaram, incluindo `tooie_dev_cheats_test` |
| `pwsh -NoProfile -File tools/windows/Build-Tooie.ps1 -Stage Build` | 0 | `TooieRecompiled.exe` recompilado e linkado; sete avisos em trechos preexistentes de `boot_hw_stubs.cpp` |
| `LC_ALL=C bash tools/vendor_verify.sh --reverse-check` | 0 | `VENDOR VERIFY OK`; 8274 arquivos, 40 patches; exec bits POSIX não aplicáveis no NTFS/MSYS |

Checagem estática ad hoc: sete documentos UTF-8, 80 links resolvidos, frontmatter e whitespace válidos.

## 8. Iluminação independente — investigação e correção (2026-10-31)

Base: `0464aa6203b940569290de86034ba0b182fa1c1d`. O usuário relatou cores/brilho piscando conforme os jogadores se
moviam. Diagnóstico read-only identificou luzes e tint em endereços distintos para cada jogador, mas saída RGB no mesmo
buffer de vértices. Exemplo inicial, correção desligada: J1 e J2 usavam `0x801E4748`.

Correção em `tooie_dev_warp_hook.cpp`: escopos nos setters primário (`0x80092EC8`), LOD (`0x800941C8`) e secundário
(`0x8009359C`) fazem `func_80100D24` pedir um buffer próprio quando o pedido original é zero e o modelo usa iluminação.
Inclui inicialização antes de publicar os jogadores na tabela; o allocator e a liberação continuam sendo nativos.
O modo é latched apenas quando o pedido original de um jogador vira dois; não muda pedidos nativos de multijogador.
Kill switch: `TOOIE_TEST_2P_PRIVATE_VERTICES=0` (default on).

Discriminador Windows/D3D12, attract mapa 335, mesmo binário instrumentado e chave 0/1. `TOOIE_AUTO_START=1` inicia o
jogo; a flag temporária `TOOIE_TEST_2P_LIGHTING=1` ligava a opção apenas em memória. A flag temporária
`TOOIE_DIAG_2P_LIGHTING=<csv>` registrava os endereços nos ganchos `0x80092444` e `0x8010E4C4`. Também calculava hash
do RGB de J1 imediatamente antes/depois do desenho completo de J2. As duas flags temporárias e esses ganchos foram
removidos do código final; só a chave de desligar a correção permanece.

| Medida | Correção OFF | Correção ON |
| --- | --- | --- |
| Chamadas de iluminação registradas | 6000 (J1: 2980; J2: 3020) | 6000 (J1: 2980; J2: 3020) |
| Alias entre saídas simultaneamente vigentes | 3780/5924 comparações | 0/5924 comparações |
| RGB de J1 alterado pelo desenho de J2 | 7/40 verificações | 0/40 verificações |
| Buffers iniciais J1 / J2 | `0x801E4748` / `0x801E4748` | `0x801F1060` / `0x80208640` |

Houve troca de LOD na amostra. O allocator reutilizou um endereço liberado de J1 mais tarde para J2; isso é esperado:
a comparação considera endereços simultaneamente vigentes, não a união de todos os endereços usados durante a execução.
Saves `bt.n64.us.1.0.bin` e `.bak`: hashes SHA-256 idênticos antes/depois. Saves e configs foram copiados antes do teste;
os processos de teste foram encerrados sem usar "Save and Quit".

Artefatos locais privados em `C:\Users\Wesley\AppData\Local\Temp\tooie-lighting-20261031-000238`:

- `private-0.csv`, SHA-256 `61aa1d5c67875d0cbd3ce752cce7ef9de7901f54c0758c6b26f60b0b91401896`.
- `private-1.csv`, SHA-256 `bc13e441e0b227b1eedd18efff4a646e0b30b3524fe54b41e336a7171dce4c65`.
- `instrumented-final.patch`: diff rastreado com a instrumentação temporária para reproduzir a partir da base,
  SHA-256 `797f2dbe52e94ef29658ad54abef078ca17ac162762ab1113fe863b75819c3d7`.
- `%TEMP%/tooie-lighting-run.ps1 -Variant 0` e `-Variant 1`: comandos de lançamento usados através de `rtk proxy pwsh
  -NoProfile -File`; a flag da correção foi a diferença entre as variantes.

Esse teste prova isolamento da saída e ausência das sobrescritas observadas na amostra. Depois dele, o usuário confirmou
que a correção resolveu o problema gráfico das luzes no modo história Windows (`PROVEN`, 2026-10-31). Playtest específico
de transformações/modelo secundário e Linux: `NOT_RUN`.

Validação final após remover os diagnósticos:

| Comando / checagem | Exit code | Resultado |
| --- | --- | --- |
| `pwsh -NoProfile -File tools/windows/Build-Tooie.ps1 -Stage Build` | 0 | `TooieRecompiled.exe` recompilado/linkado; sete avisos preexistentes em `boot_hw_stubs.cpp` |
| `pwsh -NoProfile -File tools/windows/Build-Tooie.ps1 -Stage Test` | 0 | 17/17 testes obrigatórios passaram |
| Checagem estática ad hoc | 0 | Sete docs UTF-8, 89 links válidos, frontmatter/whitespace válidos, instrumentação temporária ausente |
| `git diff --check` | 0 | Limpo |

Build da validação anterior, antes da captura leve opt-in: `build-win/TooieRecompiled.exe`, SHA-256
`a7f2cd3c81f528a177f393860dd9cbc976c494c2674ff1148e43fb351e34d012`.

## 9. Captura manual leve, a pedido do usuário

```powershell
pwsh -NoProfile -File "E:\projetos\TooieRecomp-win\tools\windows\Test-SplitScreenLighting.ps1"
```

O script imprime uma pasta nova `%TEMP%\TooieRecomp-lighting-<data-hora>` com `lighting.log`, `stdout.log`, `stderr.log`,
`run.json` (HEAD, estado do worktree, SHA-256 do executável, horários e PID) e `backup/` dos saves/configs. Abra o Dev
Mode, aceite o aviso, ligue Two players e carregue o mapa normalmente. Faça a captura visual separadamente e feche a
janela do jogo para terminar; não use "Save and Quit". A captura não restaura nem sobrescreve seus saves/configs.

`-LogDir <pasta-nova>` escolhe outro destino. `-DisablePrivateVertices` desliga apenas a correção para A/B opcional;
o primeiro teste deve usar a correção ligada (comando acima).

Esta captura usa `TOOIE_DIAG_2P_LIGHTING_LOG=<arquivo>`: default OFF, eventos de mapa e alocação de modelos, sem hooks
por desenho e sem hashes por frame. Guarda dono/slot, tipo primário/LOD/secundário, asset, instance, contagem de buffers
pedida/efetiva, saída privada e ponteiros de luz/tint. Ela identifica caminhos de alocação e possíveis aliases;
não repete o discriminador RGB da seção 8. `slot=-1` durante a criação inicial é esperado antes da publicação do jogador
na tabela. Endereços podem ser reutilizados após liberar uma instância.

Validação da captura: `Build-Tooie.ps1 -Stage Build` e `-Stage Test` terminaram com exit 0 (17/17 testes);
parser PowerShell e rejeição de destino já existente passaram, sem abrir o jogo. O cabeçalho de `lighting.log` traz
horário Unix em milissegundos; `t` mede segundos desde a abertura do log para correlacionar eventos com o vídeo.
Checagem estática: oito documentos, 91 links válidos; `git diff --check` limpo.

Captura manual posterior do usuário: `C:\Users\Wesley\AppData\Local\Temp\TooieRecomp-lighting-20261031-003120-888`,
iniciada às 00:31:21 -03:00 e encerrada às 00:39:43 -03:00 em 2026-10-31, exit 0, correção ligada. O log registrou
37 eventos de modelo, 36 alocações privadas e caminhos primário/LOD/secundário. O usuário confirmou o resultado visual;
os eventos de alocação não substituem um playtest específico de cada transformação.

- Executável testado: SHA-256 `7feb57ae7c7400186884f1e5ca180c43b8d971b2100af3c607b71e590530ddd4`.
- `lighting.log`: SHA-256 `510a3c2309b7e6bea9a9ff1db2014e56c0490ef74ef0c85e02bb1a321e732e70`.
- `stderr.log`: SHA-256 `db86a8515b1454bf31acd61d36929bc380c5c0d8ff408bac51a9bb2878cfacbc`.

Revisão do checkpoint: sem achados bloqueantes; código gerado e dependências vendorizadas não foram alterados.

`NEXT_SINGLE_ACTION`: rastrear a seleção de alvo de um inimigo que persegue J1 para permitir que também escolha J2,
sem trocar o foco global `D_801354DC` (tentativa anterior `REFUTED`).
