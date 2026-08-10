# Handoff — TooieRecomp (atualizado 2026-08-10 P08)

**Prioridade atual:** spawn dos actors da abertura em `MAP_18B_CS_STARTUP_INTRO`.
Mapa e mode=3 carregam; `chintroticker`/`chintrocam`/`chintrochar` **nunca**
disparam → câmera default no chão, sem logos/demos. Não chamar Spiral Mountain.

**Scheduler runtime estabilizado (2026-08-10):** decisão **C** implementada e
validada no scheduler do N64ModernRuntime — `thread_queue_remove` com traversal
real, scan de duplicata no insert, e guard `t->queue != NULLPTR` em
`osSetThreadPri`. `No threads left to run!`, SIGSEGV de insert(NULLPTR) e
self-loop = 0 em 14 runs. Patch versionado:
`patches/runtime/n64modernruntime-scheduler.patch` + `tools/apply_runtime_patches.sh`
idempotente. Detalhe: `plan/evidence/p08-no-threads-left-diagnosis.md`
(seção `## Final scheduler fix validation`).

Handoff técnico: `plan/evidence/p08-session-handoff.md`.

**Diagnóstico fechado (2026-08-09):** o abort `Failed to find function at
0x00000000` no spawn forçado é **INVALID** (não é callback opcional). O crash
está em `func_80108C90` (não `func_80107C2C`), lendo `ActorInfo[0x38]` em
endereço link-time `0x80804FA0`, onde o data do overlay 467 não está mapeado no
runtime (ROM tem `func_80107C2C` nesse campo). Adicionalmente, o auto-spawn usa
marker id **419** que resolve para `chcollballcontrol`, não chintroticker
(marker correto **569**). Detalhe: `plan/evidence/p08-actor-null-callback-diagnosis.md`.

**FIX aplicado (2026-08-10, P08):** a causa raiz do 0 era a relocation de data
de overlay **descartada pelo gerador de metadados** — `readelf -S` sem `-W`
truncava nomes de seção longos (`.chcollballcontrol` → `.chcollballc[...]`), a
seção não entrava na lista relocatable e o N64Recomp descartava o `.rel`. A
correção é na camada generator/metadata (`-W` + listas `.txt`/`.recomp.txt`
idênticas, 851 seções). **Confirmado:** `ActorInfo[0x38] == 0x80107C2C` e
`func_80108C90` ultrapassa o `LOOKUP_FUNC(0)`. **Novo ponto de parada (não
diagnosticado):** SIGSEGV em `vector_push_back` (`func_80106668` em
`func_80107C2C`). Detalhe: `plan/evidence/p08-overlay-data-relocation-diagnosis.md`.

**SCHEDULER FIX C (2026-08-10):** estabilizado o contrato do scheduler do
N64ModernRuntime (runtime externo, base `ca568b6`, NÃO commitado no
BanjoRecomp). Alterações confirmadas: (1) `thread_queue_insert` preserva scan
REAL de links contra duplicata/self-loop (field guard removido); (2)
`thread_queue_remove` percorre a fila real (`pred`/`next`) e retorna false para
`queue_ == NULLPTR`, sem zerar `t->queue`; (3) `osSetThreadPri` só faz
remove+reinsert quando `t_ != this_thread() && state != STOPPED && t->queue !=
NULLPTR`, sempre muda `priority` e sempre roda `check_running_queue`.
Validação: harness 13/13 PASS (insert duplicate, stale field, remove
head/non-head/tail/missing/null, pri runqueue/blocked/queue_null/current);
14 runs (12 smokes + 2 GDB) com `No threads left`=0, SIGSEGV=0, self-loop=0,
duplicata=0, LOOKUP_FUNC(0)=0; `last_ovl=677:3` alcançado; caso
queue==NULL do crash original não é mais alcançável (0 ocorrências com
instrumentação temporária, removida no binário final). Repro: patch versionado
`patches/runtime/n64modernruntime-scheduler.patch` (SHA256
`1b3d3d4666e02f748c44d19d867d02669b83753d85fe7eb62d746a5a8cf38309`) +
`tools/apply_runtime_patches.sh` (idempotente). Detalhe:
`plan/evidence/p08-no-threads-left-diagnosis.md`.

**Input no title/file select permanece pendente**, mas foi explicitamente
diferido pelo usuário. A tentativa de hooks `osEepromProbe`+EEPROM falhou na
validação humana: `plan/evidence/p08-title-input-eeprom.md`.

**Repo git:** `TooieRecomp/` apenas (branch `main`).  
**Workspace monorepo:** `/home/wesley/projetos/N64-Recomp` (sem `.git` na raiz).  
**Docs de plano/specs/evidência:** `./` (vault local neste repo).

## Commits recentes

| hash | mensagem |
| --- | --- |
| `3c57905` | fix(p08): complete overlay lifecycle and core1 math hooks |
| `69e12fc` | feat(p08): stabilize frontend and runtime |
| `5bf8f06` | docs: handoff for next agent (P07/P08 state and residual) |
| `e2cdb30` | **P08:** show RT64 game view after auto-start; Tooie window title |
| `73529d5` | **P07:** RSP n_aspMain, SI/CIC-6105, and working intro audio |
| `d2d6426` | Initial commit P00–P06 |

## Working tree não commitada neste handoff

- `src/game/syscall_handler.cpp`: logs `[tooie-opening]` (mode/attr/stage) sob
  `TOOIE_DIAG=1` para ovl 731/419/441/208/695/677/639/778.
- Lifecycle+math já estão em `3c57905`.

Smoke 25 s: mode estabiliza em **3**, map section `a0=0x18B`, `gsattract:4` 1×,
**0×** ovl 419/440/441. Syscalls ~38k, áudio OK.

## Estado do produto (o que o usuário já validou)

1. **Música inicial** toca corretamente no sink **default** do SO (FELITRON USB).
2. O launcher mantém o visual/animação do BanjoRecomp e é a única janela visível
   até **Start Game**.
3. A transição `gctransitionDll` foi destravada; o RT64 chega a uma tomada 3D
   de terreno/água da abertura. A câmera fica quase no chão e não avança.
   Personagem, HUD, menu e gameplay não foram comprovados.
4. **Áudio** validado pelo utilizador no sink default (com ou sem preview do launcher).
5. No Linux, a música do launcher vem de um processo oculto que executa a ROM; ao
   iniciar o jogo ele encerra e o boot/música recomeçam no processo visível.

## P07 (já commitado) — resumo para não reabrir

### Áudio / RSP
- `n_aspMain` Tooie gerado com RSPRecomp (`n_aspMain.us.rev0.toml`):
  - ROM **decompressed**: text `0x1E4F3B0`, size **`0x17D0`** (≠ BK `0xC60`).
  - `tools/run_rsprecomp.sh` + `tools/fix_rsprecomp_labels.py` (ramos `>0x1FFF`).
- Dispatch: `M_AUDTASK` → `n_aspMain`; gfx via RT64.
- SDL: `OpenAudioDevice(nullptr)` = default do sistema.

### Boot blockers resolvidos (host)
| bug | sintoma | fix em `src/game/boot_hw_stubs.cpp` |
| --- | --- | --- |
| `__osSiRawStartDma` no-op | hang em `gldbDll` (`osRecvMesg` BLOCK) | espelho PIF 64 B + `ultramodern::send_si_message()` |
| CIC 6105 LUT errada | stream `gsmaker` infinito, PCM zero | algoritmo **X-Scale/mupen** `n64_cic_nus_6105` (len 30), zera `pif[0x2e/0x2f]` |

Stack de hang antigo: `gsworldDll:2 → gsmaker:2 → gldbDll:0`.  
Depois do fix: `leave ovl=726`, stack `(idle)`.

### Diagnóstico
```sh
TOOIE_AUTO_START=1 TOOIE_DIAG=1 ./TooieRecompiled
# peak/nonzero > 0 com boot saudável; hang stack em [tooie-diag]
```
Arquivos: `include/tooie_diag.h`, `src/game/tooie_diag.cpp`.

Docs P07: `plan/evidence/p07-boot-hang-audio-diag.md`, `plans/BT/07-rsp-and-audio.md`.

## P08 — o que foi feito

### Bug principal
`TOOIE_AUTO_START` em `src/main/main.cpp` fazia:

```cpp
recomp::start_game(game_id, {});
// FALTAVA: recompui::hide_all_contexts();
```

O botão Start do launcher (RecompFrontend) chama **os dois**. Sem hide, o RmlUi do
launcher (SVG Banjo/Kazooie fundo azul, `v0.1.0-p0x`) cobria o RT64 → usuário via
“tela parada” mas ouvia música.

### Fix aplicado
```cpp
recomp::start_game(game_id, {});
recompui::hide_all_contexts();
```

Também:
- Título SDL: `banjo::program_name` → `"Banjo-Tooie: Recompiled"`.
- `version_string = "0.1.0-p08"`.
- `thread_queue_insert` rejeita inserção duplicada, eliminando auto-elo da JOY.
- Filas TIDY de uma mensagem coalescem pedidos duplicados sem bloquear a MAIN.
- O hook Tooie de `osDpGetStatus` mascara `DPC_STATUS_FREEZE`: o RT64 processa
  display lists mesmo durante esse estado sintético, ao contrário da RDP real.
- Preview de música ROM-backed em processo SDL oculto; `SIGTERM` + reap no
  Start Game e `PR_SET_PDEATHSIG/SIGKILL` se o launcher morrer.

### Evidência visual (`plan/evidence/`)

| ficheiro | conteúdo |
| --- | --- |
| `p08-window-capture.png` | launcher SVG (antes do hide) |
| `p08-game-view.png` | **jiggy transition** + mundo no recorte (após hide) |
| `p08-runtime-frontend-smoke.md` | trace DP/VI, `>32k` syscalls e ciclo preview → boot visível |

### Já existia (não reinventar)
- RT64 + Vulkan linkados no CMake; `create_render_context` em `main.cpp`.
- RecompFrontend (launcher, config, input callbacks).
- Config: `~/.config/TooieRecompiled/{general,graphics,sound}.json` + ROM stored.
- `controls.json` é persistido de forma lazy pelo RecompFrontend quando um
  binding é alterado; a ausência anterior a isso é normal e igual ao BanjoRecomp.
- Defaults, descrições, callbacks, single-player, Rumble Pak e exports de
  analógico direito têm implementação idêntica ao BanjoRecomp.
- Os VRAMs Tooie de `osContInit`, polling/leitura, `osContSetCh`, motor e rumble
  apontam para os wrappers host do N64ModernRuntime. GDB confirmou
  `osContStartReadData_recomp` em `[Game] SCHEDULE` e
  `osContGetReadData_recomp` em `[Game] JOY`; o SI bruto continua exclusivo do
  CIC-6105.
- O build copia o `recompcontrollerdb.txt` canônico de `../BanjoRecomp` para a
  pasta do executável; o smoke não mostra mais `Invalid RWops`.
- O desbloqueio do scheduler também depende da alteração ainda não commitada em
  `../BanjoRecomp/lib/N64ModernRuntime/ultramodern/src/threadqueue.cpp`; este
  commit de TooieRecomp não inclui a dependência externa.

## Residual prioritário (próximo agente)

### 0. PRIORIDADE — divergência que impede a init do actor manager

- Relocation de data de overlay: **RESOLVIDO** (P08, camada generator/metadata).
  `LOOKUP_FUNC(0)` não ocorre mais; `ActorInfo[0x38] == 0x80107C2C`.
- SIGSEGV em `vector_push_back` (`func_80108C90 → func_80107C2C →
  func_80106668 → vector_push_back`): **DIAGNOSTICADO como artefato do spawn
  diagnóstico.** `D_80136EE0` (vetor global de Actors) == NULL; initializer
  (`chbaddieDll ep1 → func_800FF390 → func_801065E0`) **nunca executou**;
  args do hook `func_80108C90(419, scratch, 0, 0)` são **VALID** per o
  contrato real. ROOT CAUSE: A primário / C candidata. CONFIDENCE: MEDIUM.
- SIGSEGV natural `glintrosync:6 → gcsection:4`: **DIAGNOSTICADO E CORRIGIDO**
  (marker 419 OFF). Causa: `ovl_unload` não invalidava `s_resident_overlays` —
  o mapping cached de 677 ficava stale e a região de heap liberada era
  reutilizada por outra overlay (709); `func_808000F0` lia tabela corrompida,
  não achava 0x18B e retornava NULL. Fix estreito: `tooie_invalidate_resident_overlay`
  em `ovl_unload`. ROOT CAUSE: E. CONFIDENCE: HIGH.
- O spawn diagnóstico (marker **419** = chcollballcontrol; overlay 419 =
  chintroticker, marker **569** = chintroticker) agora é **opt-in, default OFF**
  via `TOOIE_DIAG_SPAWN_MARKER419=1`.
- Próximo bloqueio do fluxo natural (pós-fix): `std::runtime_error:
  No threads left to run!` = `run_next_thread` encontrou a running queue vazia
  (NÃO é "esgotamento de threads"). DIAGNÓSTICO COMPLETO (ROOT CAUSE E, HIGH):
  patch local em `threadqueue.cpp` rejeita re-inserção após `osSetThreadPri`
  porque `thread_queue_remove` não limpa `t->queue` → fila drena até vazia.
  Detalhes: `plan/evidence/p08-no-threads-left-diagnosis.md`.
- Auditoria de reuso do BanjoRecomp (diagnosis-only): NENHUM behavior BK ativo;
  N64MR DIRTY (threadqueue.cpp + N64Recomp `use_lookup_for_all_function_calls`);
  RecompiledPatches vazio; build não reproduzível. Detalhes:
  `plan/evidence/p08-active-banjo-reuse-audit.md`.
- Detalhes: `plan/evidence/p08-gcsection-glintrosync-segv-diagnosis.md`,
  `plan/evidence/p08-actor-init-segv-diagnosis.md`,
  `plan/evidence/p08-overlay-data-relocation-diagnosis.md`,
  `plan/evidence/p08-session-handoff.md`.

### 1. Diferido — title / file select input

- Launcher RecompFrontend: input OK.
- Title/file select do jogo: pad não entra no save/start (utilizador 2026-08-09).
- Tentativa EEPROM falhou. Retomar somente depois da abertura automática;
  hipóteses em `plan/evidence/p08-title-input-eeprom.md`.

### 2. Captura dirigida de menu e gameplay

- Depois do fix, capturar logos, descida de câmera, menu e demos.
- Só considerar gameplay com personagem/HUD e controle comprovados.

### 3. Controles (P08-T05)

- Path host `osCont*` já ligado; falta fix title/save + teste físico rumble/hotplug.
- `controls.json` lazy (igual BanjoRecomp).

### 4. Paridade visual / UI (P08-T02/T03/T07/T08)
- 4:3, fullscreen/multi-monitor, fallback Vulkan.


## Como buildar e correr

```sh
cd TooieRecomp/build   # ou reconfigure se necessário
cmake --build . --target TooieRecompiled -j$(nproc)

# ROM: já em ~/.config/TooieRecompiled/bt.n64.us.1.0.z64 (layout decompressed)
# Hash esperado no GameEntry: 0x9F843AD3455EA395ULL

TOOIE_AUTO_START=1 ./TooieRecompiled
# Janela: "Banjo-Tooie: Recompiled"
# Escape: menu config RecompFrontend
# TOOIE_DIAG=1  — contadores
# TOOIE_LOG_RSP=1 — cada M_AUDTASK

TOOIE_LAUNCHER_MUSIC_SMOKE=1 TOOIE_DIAG=1 ./TooieRecompiled
# Valida preview oculto, encerramento e segundo boot visível.
# TOOIE_DISABLE_LAUNCHER_MUSIC=1 desativa o preview.
```

Dependências de tree: `../BanjoRecomp` (runtime/RT64/Frontend),  
`../banjo-tooie-Recomp/decompressed.us.z64` (regen ASP).

## Arquivos-chave

| path | papel |
| --- | --- |
| `src/main/main.cpp` | janela, auto-start, RSP dispatch, SDL audio |
| `src/game/boot_hw_stubs.cpp` | SI, CIC 6105, scheduler, ovl_load host |
| `src/game/syscall_handler.cpp` | overlay syscalls + nested bank |
| `src/game/tooie_diag.cpp` | diagnóstico |
| `rsp/n_aspMain.cpp` | microcódigo áudio |
| `n_aspMain.us.rev0.toml` | config RSPRecomp |
| `plan/evidence/p07-*.md`, `plans/BT/07-*.md` | evidências/plano P07 |
| `plans/BT/0{6,7,8}-*.md` | planos detalhados (vault local) |

## Não fazer

- Commits na raiz `N64-Recomp` (sem git).
- Reutilizar `n_aspMain` de Banjo-Kazooie.
- Instrumentar `RecompiledFuncs` gerados com fprintf permanente.
- Assumir que silêncio = device errado sem checar `TOOIE_DIAG` peak e PipeWire sink.
