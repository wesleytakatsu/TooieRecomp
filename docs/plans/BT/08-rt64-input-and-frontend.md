---
title: "BT P08 — RT64, entrada e frontend"
description: "Completa renderização, janela, controles e experiência de inicialização no Linux."
status: active
type: implementation-stage
project: TooieRecomp
phase: P08
order: 8
depends_on: P07
updated: 2026-08-09
tags: [banjo-tooie-Recomp, rt64, input, frontend, linux]
---

# BT P08 — RT64, entrada e frontend

## Resultado esperado

O jogo renderiza corretamente pelo RT64, aceita teclado/controle e oferece um
launcher Linux suficiente para selecionar ROM e configurações essenciais.

## Tarefas

- [x] `P08-T01` Integrar RT64 e validar Vulkan nesta máquina (parcial: Haswell Vulkan incompleto no MESA, mas RT64 apresenta frames).
- [~] `P08-T02` Catalogar display lists ou efeitos que exigem patch específico de Tooie (scheduler DP/VI corrigido; catálogo amplo ainda pendente).
- [ ] `P08-T03` Reproduzir primeiro 4:3 e resolução/timing originais sem melhoria ativa.
- [x] `P08-T04` Integrar o mesmo path SDL/RecompFrontend do BanjoRecomp (`poll_inputs`, perfis N64, eventos, device info e rumble por VI).
- [x] `P08-T05` Adotar os defaults e a persistência do BanjoRecomp; `controls.json` é criado de forma lazy pelo RecompFrontend quando o usuário altera um binding.
- [x] `P08-T06` Adaptar auto-start/frontend: esconder launcher ao iniciar jogo; título da janela Tooie.
- [ ] `P08-T07` Testar janela, fullscreen, foco, multi-monitor e escalas.
- [ ] `P08-T08` Fallback e mensagem acionável para driver/API gráfica incompatível.
- [~] `P08-T09` Evidência visual: transição jiggy + terreno/água da abertura; a câmera permanece quase no chão e a sequência não troca de tomada.
- [ ] `P08-T10` Input no title/file select do jogo (launcher OK); diferido até a sequência automática da abertura progredir.
- [ ] `P08-T11` Restaurar a máquina de estados da abertura da ROM: logos, descida de câmera, término da música e demos.

## Implementado (2026-08-09)

### Já existia (P05–P07)

- Link RT64 + Vulkan (`SDL_WINDOW_VULKAN`), `create_render_context` via RecompFrontend.
- Launcher RmlUi, config tabs, input callbacks SDL.
- ROM store em `~/.config/TooieRecompiled/`.

### Fixes desta sessão (e sessões P08 anteriores)

1. **`TOOIE_AUTO_START` não escondia o launcher**  
   Chamava `recomp::start_game` sem `recompui::hide_all_contexts()`.  
   Resultado: música rodava por baixo do SVG do launcher (falso “tela parada”).  
   **Fix:** após `start_game`, `hide_all_contexts()` (igual botão Start do launcher).

2. **Título da janela**  
   `"Banjo: Recompiled"` → `banjo::program_name` (`"Banjo-Tooie: Recompiled"`).

3. **Versão** `0.1.0-p08`.

4. **Deadlock da fila de threads do runtime**  
   `thread_queue_insert` aceitava inserir a mesma `OSThread` duas vezes e criava
   um elo para ela própria (JOY id 7), prendendo o VIMGR. O runtime agora rejeita
   duplicatas e detecta filas malformadas.

5. **Handshake TIDY no host**  
   Os envios bloqueantes para as filas de uma mensagem `0x800769E0` e
   `0x80076A00` podiam parar a MAIN antes que a thread TIDY de prioridade menor
   drenasse a solicitação anterior. Os overrides host coalescem trabalho
   duplicado com `OS_MESG_NOBLOCK`.

6. **Transição jiggy / scheduler DP–VI**  
   O terceiro display list concluía no RT64 enquanto o bit sintético
   `DPC_STATUS_FREEZE` ainda estava ativo. No hardware, uma RDP congelada não
   concluiria essa lista antes do VI seguinte; o scheduler então perdia o sinal
   de fim de frame e a MAIN ficava em `func_800151BC(1)`. O hook Tooie de
   `osDpGetStatus` mascara apenas esse bit contábil, coerente com o renderer host
   que continua consumindo display lists. O jogo passou de `~17.2k` syscalls
   congeladas para `>32k`…`~98k`, overlays e cenas 3D em movimento.

7. **Música no launcher com reinício no Start Game**  
   No Linux, o launcher cria um processo auxiliar oculto que faz o boot usando a
   ROM armazenada e entrega o áudio inicial, sem renderizar o jogo na janela
   visível. Quando `recomp::start_game` é acionado, o auxiliar recebe `SIGTERM`,
   é coletado pelo pai e o processo principal inicia do zero. Em crash do
   launcher, `PR_SET_PDEATHSIG/SIGKILL` impede órfão. Smoke de escuta com
   `TOOIE_DISABLE_LAUNCHER_MUSIC=1` (um processo só) validado pelo utilizador:
   **áudio OK**.

8. **Paridade de controles com BanjoRecomp (parcial)**  
   Defaults, perfis, device info, rumble e `osCont*`/`osMotor*` no path host.
   SI bruto restrito ao CIC-6105. `recompcontrollerdb.txt` copiado do BanjoRecomp.
   Breakpoints: `osContStartReadData` em SCHEDULE, `osContGetReadData` em JOY.
   Evidência: [[../../plan/evidence/p08-controller-host-path]].
   **Title/file select do jogo ainda sem input validado** — ver residual.

9. **Cena 3D após a transição (M3 visual parcial)**  
   Smoke capturou terreno/água pertencente à abertura, sem personagem, HUD,
   logos ou demos. A câmera fica quase no chão e não troca de tomada. Não há
   base para identificar a cena como Spiral Mountain nem para afirmar
   jogabilidade. Estado recorrente `last_ovl=677:3` (`gcsectionDll`), syscalls e
   áudio contínuos.
   Evidência: [[../../plan/evidence/p08-first-area-playable]].

10. **Tentativa EEPROM (FAIL)**  
    Hooks LOOKUP `osEepromProbe` + read/write em `boot_hw_stubs.cpp` (hipótese
    JOY força `errno=8` se probe < 16K). Build OK; **utilizador confirmou que
    title/save ainda não responde**. Handoff: [[../../plan/evidence/p08-title-input-eeprom]].

11. **Overlays e matemática da abertura (não commitado)**  
    O ciclo dinâmico passou a manter alias canônico `0x80800000`, mapeamento do
    pacote no heap, restore aninhado, reuse do overlay ativo/residente e
    relocations curtas/longas validadas. Três gaps reais do core1 deixaram de ser
    no-op: normalização de graus (`0x800136E4`), cosseno em graus
    (`0x800138D0`) e seno em graus (`0x80013970`). Build e smoke de 28 s passam,
    mas a abertura continua na mesma tomada. Handoff completo:
    [[../../plan/evidence/p08-session-handoff]].

### Evidência RT64

| captura | conteúdo |
| --- | --- |
| `p08-window-capture.png` | launcher SVG (antes do fix hide) |
| `p08-game-view.png` | transição jiggy (sessão anterior) |
| `p08-jiggy-transition.png` | wipe jiggy (~10 s) |
| `p08-first-area-*.png` / `p08-spiral-mountain.png` | capturas antigas da tomada de terreno/água; o nome “spiral-mountain” não é identificação confirmada |
| `p08-opening-math-hooks-8s.png` / `p08-opening-math-hooks-23s.png` | câmera baixa antes/depois dos hooks trigonométricos; sem troca de tomada |
| [[../../plan/evidence/p08-first-area-playable]] | nota revisada do smoke visual |
| [[../../plan/evidence/p08-runtime-frontend-smoke]] | smoke frontend/preview |
| [[../../plan/evidence/p08-title-input-eeprom]] | **blocked** title/save input handoff |
| [[../../plan/evidence/p08-session-handoff]] | estado técnico atual e próximos breakpoints |

### Residual

- **PRIORIDADE — abertura automática:** restaurar logos, descida de câmera,
  mudança de tomadas e demos. O timing e o áudio avançam, mas `gsattract`
  aparece apenas uma vez e `chintrocam` não carrega no smoke instrumentado.
  Detalhe: [[../../plan/evidence/p08-session-handoff]].
- Capturar isoladamente logo Rare e menu de título depois de corrigir o fluxo.
- **Diferido — title/file-select input:** launcher UI OK; tentativa EEPROM
  falhou validação humana. Retomar após a abertura automática; ver
  [[../../plan/evidence/p08-title-input-eeprom]].
- Controles físicos (rumble/hotplug) após o fluxo inicial e title/save.
- Paridade 4:3 original vs `Expand` atual em `graphics.json`.
- Validar foco/fullscreen/multi-monitor e mensagem de fallback Vulkan.

## Gate de saída

- [ ] Abertura da ROM progride por logos, descida de câmera e demos sem ficar presa na tomada de terreno/água.
- [~] RT64 ultrapassa a jiggy e renderiza geometria 3D; personagem/HUD, menu e gameplay ainda não comprovados.
- [ ] Entrada no title/file select do jogo e rumble/hotplug — diferidos até a abertura progredir.
- [x] Persistência segue o BanjoRecomp; `controls.json` é lazy e não precisa existir antes de uma alteração.
- [x] Melhorias não forçam alteração da referência de paridade (sem patches Tooie de gameplay ainda).

## Próxima etapa

1. **Agente seguinte:** spawn dos actors intro em `MAP_18B` — `chintroticker`
   (419), `chintrocam` (441), `chintrochar` (440). Mode já fica em 3; mapa
   carrega. Ver [[../../plan/evidence/p08-session-handoff]].
2. Com actors vivos: validar logos, descida de câmera e demos.
3. Depois input title/file-select, rumble/hotplug e polish 4:3/fullscreen.
4. Só então [[09-gameplay-parity-and-enhancements]].
