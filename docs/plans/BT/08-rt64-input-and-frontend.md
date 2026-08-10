---
title: "BT P08 — RT64, entrada e frontend"
description: "Completa renderização, janela, controles e experiência de inicialização no Linux."
status: active
type: implementation-stage
project: TooieRecomp
phase: P08
order: 8
depends_on: P07
updated: 2026-08-10
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

12. **Scheduler do runtime estabilizado (decisão C, 2026-08-10)**  
    O N64ModernRuntime (externo, base `ca568b6`, não commitado) recebeu o patch
    de scheduler da decisão C: `thread_queue_remove` com traversal real da fila
    (pred/next) e `false` para `queue_ == NULLPTR` sem zerar `t->queue`;
    `thread_queue_insert` mantém o scan REAL de links contra duplicata/self-loop;
    `osSetThreadPri` só remove+reinsert com `t_ != this_thread() && state !=
    STOPPED && t->queue != NULLPTR`, sempre muda `priority` e sempre chama
    `check_running_queue`. Harness nativo 13/13 PASS; 14 runs (12 smokes + 2
    GDB) com `No threads left`=0, SIGSEGV=0, self-loop=0, `last_ovl=677:3`
    alcançado. Repro versionado:
    `patches/runtime/n64modernruntime-scheduler.patch` +
    `tools/apply_runtime_patches.sh` (idempotente). Evidência:
    [[../../plan/evidence/p08-no-threads-left-diagnosis]] (seção
    `## Final scheduler fix validation`).

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
| [[../../plan/evidence/p08-actor-null-callback-diagnosis]] | callback nulo em `func_80108C90` provado INVALID; causa = data de overlay não mapeado em link-time; marker 419 ≠ chintroticker (569) |
| [[../../plan/evidence/p08-overlay-data-relocation-diagnosis]] | **FIX aplicado:** relocation de data de overlay perdida — causa raiz = metadata (nomes de seção truncados por `readelf -S` sem `-W`); corrigido no gerador; `ActorInfo[0x38] == 0x80107C2C`; `LOOKUP_FUNC(0)` FIXED |
| [[../../plan/evidence/p08-actor-init-segv-diagnosis]] | **SIGSEGV de actor diagnosticado como artefato do spawn:** `D_80136EE0` NULL (initializer `chbaddieDll ep1 → func_800FF390 → func_801065E0` nunca executou); args do hook VALID; crash some com AUTO-SPAWN OFF; fluxo natural diverge antes em `gcsectionDll:4` (via glintrosync) |
| [[../../plan/evidence/p08-gcsection-glintrosync-segv-diagnosis]] | **FIX aplicado:** SIGSEGV natural `glintrosync:6 → gcsection:4` — `func_808000F0` retornava NULL porque o cache de overlay resident ficava stale (`ovl_unload` não invalidava `s_resident_overlays`) e a região de heap liberada era reutilizada por outra overlay (709), corrompendo a tabela de seções; ROOT CAUSE E, HIGH. Hook marker-419 agora opt-in `TOOIE_DIAG_SPAWN_MARKER419=1` |

### Residual

- **PRIORIDADE — abertura automática:** restaurar logos, descida de câmera,
  mudança de tomadas e demos. O timing e o áudio avançam, mas `gsattract`
  aparece apenas uma vez e `chintrocam` não carrega no smoke instrumentado.
  Detalhe: [[../../plan/evidence/p08-session-handoff]].
- **PRIORIDADE — mapeamento de data de overlays:** **RESOLVIDO.** A causa era a
  relocation de data descartada pelo gerador de metadados (nomes de seção
  truncados por `readelf -S` sem `-W`); corrigido na camada generator/metadata.
  `ActorInfo[0x38]` agora lê `0x80107C2C` e `func_80108C90` ultrapassa o
  `LOOKUP_FUNC(0)`. Detalhe:
  [[../../plan/evidence/p08-overlay-data-relocation-diagnosis]].
- **PRIORIDADE — novo SIGSEGV de inicialização de actor:** **DIAGNOSTICADO como
  artefato do spawn diagnóstico.** Em `vector_push_back` (via
  `func_80106668`/`func_80107C2C`), `D_80136EE0` (vetor global de Actors) está
  NULL porque o initializer (`chbaddieDll ep1 → func_800FF390 →
  func_801065E0`) nunca executou; args do hook `func_80108C90` são VALID; o
  crash some com AUTO-SPAWN OFF (o fluxo natural diverge antes, num segundo
  SIGSEGV em `gcsectionDll:4` via `glintrosync:6`, sem diagnóstico). ROOT CAUSE
  A primário / C candidata, CONFIDENCE MEDIUM. Detalhe:
  [[../../plan/evidence/p08-actor-init-segv-diagnosis]].
- **PRIORIDADE — SIGSEGV natural `glintrosync:6 → gcsection:4`:**
  **RESOLVIDO.** O spawn diagnóstico (marker 419) agora é opt-in default OFF
  (`TOOIE_DIAG_SPAWN_MARKER419=1`). `func_808000F0` retornava NULL porque o
  cache de overlay resident ficava stale: o jogo descarregava 677 via
  `ovl_unload` e o host não invalidava `s_resident_overlays[677]`; a região de
  heap liberada era reutilizada pela overlay 709, corrompendo a tabela de
  seções. Fix estreito: `tooie_invalidate_resident_overlay()` em `ovl_unload`.
  ROOT CAUSE E, CONFIDENCE HIGH. Detalhe:
  [[../../plan/evidence/p08-gcsection-glintrosync-segv-diagnosis]].
  Novo bloqueio natural pós-fix: `std::runtime_error: No threads left to run!`
  = `run_next_thread` com a running queue vazia. DIAGNÓSTICO COMPLETO: ROOT
  CAUSE E (patch local `threadqueue.cpp` rejeita re-inserção após
  `osSetThreadPri`; `thread_queue_remove` não limpa `t->queue` → fila drena),
  CONFIDENCE HIGH. Detalhe:
  [[../../plan/evidence/p08-no-threads-left-diagnosis]]. Fix estreito pendente
  de autorização (arquivo é dependência externa N64ModernRuntime).
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

1. **Agente seguinte:** continuar a busca da divergência que impede a init do
   actor manager (`chbaddieDll ep1 → func_800FF390 → func_801065E0`) sem forçá-la,
   agora que o scheduler do runtime está estabilizado (decisão C —
   `No threads left to run!`, insert(NULLPTR) SIGSEGV e self-loop eliminados;
   ver fix 12 e [[../../plan/evidence/p08-no-threads-left-diagnosis]]).
   Refs: [[../../plan/evidence/p08-gcsection-glintrosync-segv-diagnosis]].
2. Com actors vivos: validar logos, descida de câmera e demos.
3. Depois input title/file-select, rumble/hotplug e polish 4:3/fullscreen.
4. Só então [[09-gameplay-parity-and-enhancements]].
