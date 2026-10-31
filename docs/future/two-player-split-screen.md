---
title: "Dois jogadores em tela dividida (Dev Mode, experimental) — estado e continuação"
description: "Handoff do modo experimental de dois jogadores no modo história: o que funciona, ganchos, endereços, diagnósticos temporários e próximos passos."
status: in-progress
type: handoff
project: TooieRecomp
updated: 2026-10-30
tags: [dev-mode, multiplayer, split-screen, handoff]
---

# Dois jogadores em tela dividida — estado e continuação

Checkpoint experimental em `E:\projetos\TooieRecomp-win`, branch `codex/two-player-split-screen`, derivada de
`feat/dev-mode` (base `f42fdae10cdb6bde7b5ca2a09ce4b898f4c68e0b`, release v0.1.3). A implementação parcial foi
preservada; os diagnósticos temporários foram removidos antes do commit. Plano relacionado (futuro, não iniciado):
[[future/two-process-coop]]. Contrato atual: [[specs/Tooie/window-ui-and-config]]; RE canônico:
[[reverse-engineering/systems/warp-and-player-position]].

## 1. O que funciona (playtest anterior do usuário no Windows, relatado no handoff de origem)

Essas observações não foram repetidas após a remoção dos diagnósticos (`NOT_RUN`).

- Dev Mode → seção **Experimental** → "Two players (split screen)" (`dev_cheats.json`, chave `two_players`). Só vale
  com a aba desbloqueada na sessão e a partir do próximo carregamento de mapa. Requer Multiplayer ligado e o jogador 2
  atribuído a um controle em Controls.
- Tela dividida com duas viewports; J2 aparece e é controlado pelo controle 2; os dois trocam de mapa juntos.
- J2 aparece ao lado do J1 (spawn imediato ao lado; 2 s depois, com o J1 parado, é recolocado ao lado e liberado).
- J2 pega itens, ataca inimigos e os ovos dele acertam inimigos.

## 2. O que falta (prioridade)

1. **Inimigos não perseguem nem atacam o J2.**
2. **J2 não usa portas/trocas de mapa.**
3. **Iluminação compartilhada:** os dois modelos disputam a mesma iluminação (cada jogador deveria ter a sua).
4. Morte do J2 (queda) mata "o jogador" (morte conjunta).
5. Cheats do Debug Button só valem para o J1.
6. Câmera do J2 levemente desorientada (observado; pode ter sido efeito de tentativas já revertidas — reavaliar).
7. Repetir a reprodução limpa no Windows após a remoção dos diagnósticos; Linux `NOT_RUN`.

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
3. **Iluminação:** achar onde a luz do personagem é calculada (provavelmente por modelo do jogador, global única);
   separar por `PlayerState`.
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

`NEXT_SINGLE_ACTION`: repetir no Windows o teste dos dois jogadores com backup dos saves/configs: controles
independentes, spawn, coleta, ataques e troca de mapa pelo J1.
