---
title: "Plano — Multijogador local com até 4 controles"
description: "Plano de trabalho do suporte a 4 jogadores locais: atribuição de controles e teclado por jogador, mapeamento por jogador, teste por vibração e regra das portas N64."
status: completed
type: plan
project: TooieRecomp
updated: 2026-10-23
tags: [tooie-recomp, plan, input, multiplayer, controllers]
---
# Plano — Multijogador local com até 4 controles

Documento de trabalho temporário (pedido do usuário em 2026-10-23). Ao concluir, o contrato final vai para
[[specs/Tooie/input-and-controllers]] e este arquivo é removido. Estado corrente geral: [[HANDOFF]].

## 1. Objetivo

Até 4 pessoas jogam ao mesmo tempo no modo multijogador nativo do Banjo-Tooie, cada uma com seu controle e/ou o teclado,
com mapeamento próprio por jogador, configurável no menu de opções.

## 2. Requisitos

| ID | Requisito | Situação inicial |
|---|---|---|
| R1 | O jogo reconhece os controles conectados e o teclado (até 4 jogadores) | Parcial: no modo atual as 4 portas aparecem conectadas e todas recebem a mesma entrada misturada |
| R2 | Mapeamento independente **por jogador** (P1..P4), separado para controle e teclado | Falta: hoje o mapeamento é por modelo de controle |
| R3 | Mapeamento persiste entre sessões | Existe para perfis; falta para a atribuição de jogadores |
| R4 | Botão "Restaurar padrão" para o jogador/dispositivo selecionado | Existe (`ui_config_page_controls.cpp:488`), precisa apontar para o perfil do jogador |
| R5 | Teclado num jogador e controle em outro; teclado **e** controle no mesmo jogador (como o P1 hoje) | Falta: um jogador é controle **ou** teclado |
| R6 | Trocar, nas opções, qual controle cada jogador usa | Falta: só refazendo a atribuição inteira |
| R7 | Botão **Vibrar** por jogador: envia 1 s de vibração ao controle selecionado para identificá-lo fisicamente | Falta |
| R8 | Indicador de input: ao apertar qualquer botão/tecla, o menu mostra que aquele dispositivo está recebendo input e a qual jogador está ligado | Parcial: o ícone pisca só durante a atribuição |
| R9 | Com o multijogador desligado, o comportamento atual não muda | Precisa ser garantido |

## 3. Decisões tomadas

- **D1 — Mapeamento por jogador** (decisão do usuário, 2026-10-23). Controles iguais sem número de série (XInput) não podem
  ser distinguidos entre boots; o mapeamento vive no jogador. Cada jogador tem um perfil de controle e um de teclado.
  O padrão inicial do perfil de controle segue o tipo do controle (`SDL_GameControllerGetType`: Xbox, PlayStation,
  Nintendo, genérico).
- **D2 — Opção "Multijogador (até 4)"** em Controles, desligada por padrão. Desligada = modo de um jogador atual (R9).
  Mudar exige reiniciar enquanto a fase 0 não provar que o jogo relê a presença das portas.
- **D3 — Regras de atribuição**: um controle pertence a no máximo um jogador; o teclado pertence a no máximo um jogador;
  cada jogador tem no máximo um controle e pode ter o teclado. Escolher para P2 um controle que está no P1 **troca**
  (o P1 recebe o controle antigo do P2, ou fica sem).
- **D4 — Regra das portas N64**: a porta 0 fica sempre conectada (se ninguém foi atribuído, usa a entrada misturada de
  hoje, para o jogo nunca ficar sem controle); as portas 1–3 só aparecem conectadas se o jogador tiver um dispositivo.
  Porta sem dispositivo retorna "sem resposta".
- **D5 — Teste por vibração**: o botão **Vibrar** chama `SDL_GameControllerRumble(controller, 0xFFFF, 0xFFFF, 1000)`
  direto no controle, ignorando a intensidade de vibração configurada. Durante esse 1 s, a vibração do jogo para aquele
  controle é suspensa (senão `update_rumble`, chamado a cada VI, sobrescreve o teste). No Windows com driver RAWINPUT
  (ex.: Xbox One S por Bluetooth), o SDL rejeita vibração com erro de correlação ("Controller isn't correlated yet,
  try hitting a button first") até o controle enviar a primeira entrada; a UI exibe "Press a button on it first" até
  haver correlação. Outros estados na UI: "Rumbling…" (1 s), "No rumble" e "Rumble failed".
- **D6 — Persistência**: a atribuição é salva no JSON de controles e restaurada ao reconectar: primeiro por número de
  série, depois por modelo (hash do GUID) na ordem de conexão. O usuário corrige pelo menu quando dois controles iguais
  trocam de lugar.

## 4. O que já existe (RecompFrontend vendorizado, desligado)

- `recompinput::players` (`lib/RecompFrontend/recompinput/src/players.cpp`): modo multijogador, até 4 jogadores,
  atribuição por "aperte um botão", teclado como jogador separado.
- Tela de atribuição e cartões de jogador (`recompui/src/composites/ui_assign_players_modal.cpp`, `ui_player_card.cpp`);
  botão "Assign players" na página Controles (`recompui/src/config/ui_config_page_controls.cpp:317`).
- Perfis por jogador (`profiles.cpp`, `players_input_profile_indices`), vibração e giroscópio por jogador
  (`input_state.cpp`), JSON de controles (`profiles::save_controls_config`).
- Runtime: `ultramodern/src/input.cpp` consulta 4 portas via `get_connected_device_info` e `get_input`.
- Jogo: lê `OSContPad D_80079B40[4]`, mas só se `D_80079B90[0].errno == 0`, e esse status é preenchido uma vez em
  `osContInit` (`decomp/src/core1/1E2D890.c`).
- DK64Recompiled não tem nada a mais: só destrava o menu do jogo e continua em modo de um jogador.

## 5. Defeitos conhecidos a corrigir

| ID | Defeito | Classificação |
|---|---|---|
| B1 | Jogadores 2–4 começam com o perfil 0 (teclado de um jogador): o teclado controlaria todos em modo multijogador (`profiles.cpp:22`, `:340`) | `INFERENCE` (leitura de código) |
| B2 | Jogador guarda ponteiro `SDL_GameController*` depois da desconexão: risco de uso após liberar | `INFERENCE` |
| B3 | `get_n64_input` sempre retorna `true`: portas sem jogador aparecem conectadas e paradas | `PROVEN` (código) |
| B4 | `ultramodern/src/input.cpp`: `*pattern = 1 << controller;` deveria ser `|=` (bitpattern fica só com a última porta) | `PROVEN` (medido: portas 0+1 davam `pattern=02`); corrigido (`|=`) |
| B5 | Hash do GUID ignora o número de série (`profiles.cpp:290`): controles iguais dividem o perfil | `PROVEN` (código); deixa de importar com D1 |
| B6 | `librecomp/src/cont.cpp` `osContGetReadData_recomp` só grava a porta quando `err_no == 0`: porta que deixa de responder mantém `err_no` 0 e os últimos botões (jogador removido segurando botão fica "apertando") | `PROVEN` (código); correção na fase 2 |
| B8 | Depois de `osContSetCh(2)` (o jogo chama ao entrar no modo multiplayer) o ultramodern lê só 2 portas, mas `osContGetReadData_recomp` copiava as 4 entradas de um array não inicializado: portas 2–3 com lixo (`errno=00`, `buttons=01C1`), jogo via 4 jogadores | `PROVEN` (log do usuário); corrigido (sentinela, portas não lidas ficam intocadas) |
| B7 | `ContextId::get_focused_element` desreferenciava slots vazios durante a construção de elementos (crash ao voltar da edição de mapeamento) | `PROVEN` (código); corrigido no recompui |

## 6. Interface alvo (página Controles, multijogador ligado)

```text
┌ Jogador 1 ───────────────────────────────────────────────┐
│ Controle: [ Controle 2 — Xbox Wireless Controller   ▼ ] [ Vibrar 1 s ] │
│ Teclado:  [x] ligado a este jogador                       │
│ Input:    ● recebendo (Controle 2)                        │
│ [ Editar mapeamento do controle ] [ Editar mapeamento do teclado ]     │
│ [ Restaurar padrão ]                                      │
└───────────────────────────────────────────────────────────┘
  ... Jogador 2, 3, 4 iguais ...

Dispositivos conectados
  Teclado                          → Jogador 1   ● (pisca a cada tecla)
  Controle 1 — DualSense           → Jogador 2   ●
  Controle 2 — Xbox Wireless       → Jogador 1   ●
  Controle 3 — Xbox Wireless       → nenhum      ●
```

- O seletor de controle lista "Nenhum" + todos os controles conectados (nome do SDL + número de ordem).
- O indicador `●` acende por ~250 ms a cada botão, eixo além da zona morta ou tecla, em qualquer tela do menu.
- "Restaurar padrão" restaura o perfil (controle ou teclado) que está sendo editado, do jogador selecionado.
- A tela "Assign players" existente continua como atalho rápido ("cada um aperte um botão").

## 7. Fases

Tamanho: P < 150 linhas, M 300–500, G 600–900. Patches em `lib/` seguem a política de vendoring
(`vendor/patches-local/*.patch` + `ORDER.tsv` + `UPSTREAMS.tsv` + `MANIFEST.sha256`).

### Fase 0 — Medir como o jogo detecta as portas (P, diagnóstico temporário)

- [x] Diagnóstico atrás de variável de ambiente em `src/game/boot_hw_stubs.cpp`: registrar por quadro o `err_no` das
      4 portas lido pelo jogo, o bitpattern `D_80079D68`, `D_80079D6C[0..3]` e chamadas a `osContStartQuery`/`osContGetQuery`.
- [x] Usuário: abrir o menu Multiplayer do jogo com 2 controles (log em jogo: presença por quadro; `osContSetCh(4)` → `(2)`).
- Aceite: saber se a presença das portas 1–3 vem do boot, de cada quadro ou do bitpattern. Decide D2 (reiniciar ou não)
  e a fase 5.
- Remover o diagnóstico antes do commit da fase 1 (ou mantê-lo desligado por padrão, se útil).

### Fase 1 — Opção e regra das portas no Tooie (P)

- [x] Opção "Multijogador (até 4)" em `src/game/config.cpp`, desligada por padrão.
- [x] `src/main/main.cpp`: ligada → `set_single_player_mode(false)` + `set_player_count_range(1, 4)`.
- [x] `get_connected_device_info` e `tooie_get_n64_input` seguem D4 (corrige B3 no lado do Tooie).
- Aceite: com a opção desligada, nada muda (attract e jogo normal); ligada, o jogo vê só as portas com jogador.

### Fase 2 — Modelo de jogador no `recompinput` (M, patch local)

- [x] `Player` passa a ter `controller` (opcional) **e** `keyboard_enabled` ao mesmo tempo (R5).
- [x] API nova: `players::set_player_controller(player, controller | nullptr)` com troca (D3),
      `players::set_player_keyboard(player, bool)`.
- [x] Corrigir B1 (perfis vazios = `-1`) e B2 (limpar o ponteiro na desconexão, guardar a identidade para reconectar).
- [x] Salvar/restaurar a atribuição (D6) no JSON de controles.
- [x] Testes de unidade da troca, da persistência e da reconexão.
- Aceite: atribuições sobrevivem a reiniciar e a desconectar/reconectar; teclado não vaza para outros jogadores.

### Fase 3 — Mapeamento por jogador (M, patch local)

- [x] Perfis `player_N_controller` e `player_N_keyboard` (N = 1..4), padrão do controle pelo tipo SDL (D1).
- [x] "Restaurar padrão" (R4) e a edição de mapeamento usam o perfil do jogador.
- [x] Migração: os perfis atuais de um jogador viram os do P1; o modo de um jogador continua usando os mesmos.
- Aceite: mapeamentos diferentes em P1 e P2 com controles iguais; persistem após reiniciar.

### Fase 4 — Interface (G, patch local no `recompui`)

- [x] Cartão por jogador com seletor de controle, caixa do teclado, botões de mapeamento e "Restaurar padrão" (seção 6).
- [x] Botão **Vibrar 1 s** (R7, D5), desabilitado sem controle ou se o controle não tem vibração
      (`SDL_GameControllerHasRumble`).
- [x] Indicador de input por jogador e lista "Dispositivos conectados" (R8).
- [x] Navegação por controle e teclado no menu, textos em PT/EN conforme o resto da UI.
- Aceite: usuário consegue identificar cada controle pela vibração e pelo indicador, trocar controles de jogador e
  ligar o teclado em qualquer jogador sem refazer a atribuição.

### Fase 5 — Bitpattern no ultramodern (P, patch local; só se a fase 0 mostrar uso)

- [x] `|=` em `__osContGetInitData` (B4).

### Fase 6 — Documentação e validação (P)

- [x] Atualizar [[specs/Tooie/input-and-controllers]] (novo requisito REQ-INP-07) e [[specs/Tooie/window-ui-and-config]].
- [x] Atualizar [[HANDOFF]].
- [ ] Remover este plano.
- [x] Validação Windows pelo usuário (2026-10-23: "Parece tudo estar funcionando perfeitamente").
- [ ] Validação Linux no PC do usuário (nunca WSL).

## 8. Matriz de validação

| Caso | Esperado |
|---|---|
| Multijogador desligado, 1 controle + teclado | Igual a hoje |
| Multijogador desligado, attract demo | Igual a hoje |
| Ligado, P1 = controle + teclado | P1 responde aos dois |
| Ligado, P1 = teclado, P2 = controle | Cada um só controla o seu jogador |
| Ligado, 4 controles iguais | 4 jogadores independentes no menu Multiplayer do jogo |
| Trocar controle do P2 para o P3 no menu | Troca imediata; persistida após reiniciar |
| Desconectar e reconectar o controle do P3 | Volta para o P3 |
| Vibrar 1 s no P2 | Só o controle do P2 vibra, 1 s |
| Apertar botão em qualquer controle no menu | Indicador acende no dispositivo e no jogador certos |
| Restaurar padrão no P2 | Só o perfil do P2 volta ao padrão |

## 9. Riscos e perguntas abertas

- Se o jogo só lê a presença no boot, entrar um jogador com o jogo rodando exige reiniciar ou um hook para refazer a
  consulta (fase 0 decide).
- Patches grandes no RecompFrontend encarecem futuras atualizações dele: manter 3 patches separados
  (`players`, `profiles`, `ui`).
- O menu Multiplayer do jogo pode exigir minigames liberados no save para testar.

## 10. Progresso

| Data | Fase | Resultado |
|---|---|---|
| 2026-10-23 | — | Plano criado; D1 decidido pelo usuário |
| 2026-10-23 | 0+1 | Implementado (Codex), revisado: diagnóstico `TOOIE_DIAG_CONT_PORTS=<arquivo>` (default OFF); opção "Multiplayer (up to 4 players)" na página geral (default Off, vale após reiniciar); regra D4 em `include/tooie_controller_ports.h` + `tooie_controller_ports_test` PASS. Fallback da porta 0 provisório: só teclado (API mesclada vem na fase 2). Medição da fase 0 pendente (precisa do usuário) |
| 2026-10-23 | 2+3 | Implementado (Codex), revisado: 4 slots fixos (`recompinput/player_slots.h`, lógica pura) com controle + teclado por jogador, troca, reconexão por série/modelo, atribuição salva em `players[4]` no JSON de controles; perfis `mp_player_N_controller`/`_keyboard` (P1 copia os de um jogador na criação); `get_merged_n64_input` para o fallback da porta 0; atividade por dispositivo e `test_rumble` (suspende a vibração do jogo durante o teste); B1, B2, B6 corrigidos. `recompinput_player_slots_test` PASS |
| 2026-10-23 | 4 | Implementado (Codex), revisado: cartões 2×2 (Controller `Select` com "None" + controles, "Rumble 1 s"/"No rumble", toggle Keyboard, indicador de input, "Edit mapping"), lista "Connected devices" com jogador e indicador; edição alterna controle/teclado quando o jogador tem os dois; "Reset to defaults" só no perfil editado. `Build-Tooie -Stage Test -AllTests`: required 9/9, extra 19/19, 5 skipped. Pendente: validação visual/física pelo usuário, medição da fase 0, patches de vendoring, docs finais |
| 2026-10-23 | teste visual | Teste visual automatizado (capturas de tela + teclado/mouse sintéticos): layout 2x2, atribuição, troca, mover teclado, indicador de atividade do teclado, alternância na tela de mapeamento, persistência e correção do crash ao voltar verificados; vibração física e atividade de controle pendentes de validação pelo usuário |
| 2026-10-23 | vendoring | 3 patches locais criados (`recompfrontend-mp-player-slots.patch`, `recompfrontend-mp-controls-ui.patch`, `n64modernruntime-cont-port-fidelity.patch`); `VENDOR VERIFY OK` com 33 patches |
| 2026-10-23 | validação | Usuário validou no Windows com 2 controles. Log em jogo revelou B4 (bitpattern) e B8 (portas acima de `osContSetCh` com lixo); ambos corrigidos (Codex) e revisados; smoke de um jogador (boot, Start → seleção de arquivo, `pattern=0F`) OK |
