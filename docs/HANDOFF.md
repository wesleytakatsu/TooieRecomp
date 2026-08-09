# Handoff — TooieRecomp (2026-08-09)

**Repo git:** `TooieRecomp/` apenas (branch `main`).  
**Workspace monorepo:** `/home/wesley/projetos/N64-Recomp` (sem `.git` na raiz).  
**Docs de plano/specs/evidência:** `../docs/` (fora do git Tooie; manter sincronizados no disco).

## Commits recentes

| hash | mensagem |
| --- | --- |
| `e2cdb30` | **P08:** show RT64 game view after auto-start; Tooie window title |
| `73529d5` | **P07:** RSP n_aspMain, SI/CIC-6105, and working intro audio |
| `d2d6426` | Initial commit P00–P06 |

## Estado do produto (o que o usuário já validou)

1. **Música inicial** toca corretamente no sink **default** do SO (FELITRON USB).
2. Com o fix P08, a janela deixa de ficar no SVG do launcher e mostra **framebuffer RT64**:
   transição **jiggy** com pedaço de mundo/grama no recorte.
3. Ainda **não** chega a logo Rare completo / file select / área jogável estável — trava na
   animação de transição (`gctransitionDll`).

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

Docs P07 no repo: `docs/P07-rsp-and-audio.md`, `docs/P07-boot-hang-audio-diag.md`.

## P08 (esta sessão / commit `e2cdb30`) — o que foi feito

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

### Evidência visual (monorepo `../docs/plan/evidence/`)

| ficheiro | conteúdo |
| --- | --- |
| `p08-window-capture.png` | launcher SVG (antes do hide) |
| `p08-game-view.png` | **jiggy transition** + mundo no recorte (após hide) |
| `p08-game-view-later.png` | mesma jiggy ~20s depois (transição não completa) |

### Já existia (não reinventar)
- RT64 + Vulkan linkados no CMake; `create_render_context` em `main.cpp`.
- RecompFrontend (launcher, config, input callbacks).
- Config: `~/.config/TooieRecompiled/{general,graphics,sound}.json` + ROM stored.
- **Não há** `controls.json` ainda até o user abrir o tab de controles.

## Residual prioritário (próximo agente)

### 1. Destravar `gctransitionDll` (bloqueia menu / Rare logo) — **prioridade**
- Após boot: `last_ovl=674` (`gctransitionDll`), syscall count congela (~17262),
  áudio pode continuar um tempo e depois estabilizar; CPU alta possível.
- Tela: jiggy no centro, fundo preto, animação de wipe **não termina**.
- Decomp: `banjo-tooie-Recomp/src/overlays/gc/transitionDll.c` + asm
  `asm/nonmatchings/overlays/gc/transitionDll/`.
- Hipótese: timing VI / mesg / flag de frame / RDP freeze residual (ver P06
  scheduler `func_80013D10` event 3 e hooks DP/SR).
- Ferramenta: `TOOIE_DIAG=1`, eventualmente log em VI / gfx task submit.

### 2. Controles (P08-T05)
- Gerar/validar `~/.config/TooieRecompiled/controls.json`.
- Teclado + gamepad, rumble, hotplug.

### 3. Paridade visual / UI (P08-T02/T03/T07/T08)
- 4:3 original vs `ar_option: Expand` atual.
- Fullscreen / multi-monitor.
- Mensagem se Vulkan falhar (já há message_box em vários caminhos; revisar).

### 4. P07 residual (não bloqueia P08)
- Soak underrun, SFX/fala/pause/hotplug de áudio.

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
| `docs/P07-*.md`, `docs/P08-*.md` | notas no repo |
| `../docs/plans/BT/0{6,7,8}-*.md` | planos detalhados monorepo |

## Não fazer

- Commits na raiz `N64-Recomp` (sem git).
- Reutilizar `n_aspMain` de Banjo-Kazooie.
- Instrumentar `RecompiledFuncs` gerados com fprintf permanente.
- Assumir que silêncio = device errado sem checar `TOOIE_DIAG` peak e PipeWire sink.
