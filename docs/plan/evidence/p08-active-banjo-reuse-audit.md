---
title: "P08 — Auditoria de reuso ativo do BanjoRecomp (runtime)"
description: "Auditoria diagnóstico-only: risco de código/estado Banjo-Kazooie ativo no executável TooieRecompiled. Verdict: nenhum behavior BK alcançável (code recompilado Tooie só chama recomp_syscall_handler/func_*; exports recomp_get_* BK não registrados/chamados → DORMANT). N64ModernRuntime DIRTY (threadqueue.cpp + N64Recomp use_lookup_for_all_function_calls). RecompiledPatches = scaffold vazio. Build não reproduzível (5 itens). Scheduler bug = LOCAL TOOIE PATCH IN SHARED RUNTIME. Recomendação: CONTINUE P08."
status: evidence
phase: P08
updated: 2026-08-10
tags: [banjo-tooie-recomp, p08, audit, banjorecomp, reuse, threadqueue, n64modernruntime, build-reproducibility, dormant]
---

# P08 — Active BanjoRecomp reuse audit

## Verdict

| Pergunta | Resposta |
|---|---|
| TOOIE EXECUTING BK GAME LOGIC | **NO** |
| SHARED RUNTIME DIRTY | **YES** |
| RECOMPILED PATCHES SAFE | **YES** (vazio) |
| BUILD REPRODUCIBLE | **NO** |

## Active external dependencies

Deps entram no executável via `CMakeLists.txt:8-11, 27-49` (path relativo `../BanjoRecomp`).

| ITEM | PATH | CLEAN? | GENERIC? | LOCAL PATCH? | RISK |
|---|---|---|---|---|---|
| N64ModernRuntime | `BanjoRecomp/lib/N64ModernRuntime` @ `ca568b6a` | **DIRTY** | sim | **SIM** — `ultramodern/src/threadqueue.cpp` (+27/−1) | **HIGH** |
| N64Recomp (submodule do N64MR) | `…/N64ModernRuntime/N64Recomp` @ `2b6f056` | **DIRTY** | tool build | **SIM** — `src/main.cpp`: `use_lookup_for_all_function_calls = true` | MEDIUM (geração) |
| rt64 | `BanjoRecomp/lib/rt64` @ `6f1c2d9` | CLEAN | sim | não | LOW |
| RecompFrontend | `BanjoRecomp/lib/RecompFrontend` @ `d0d90ba` | CLEAN | sim | não | LOW |
| SlotMap | `BanjoRecomp/lib/SlotMap` | CLEAN | sim | não | LOW |

N64Recomp é **tool de geração** (produz RecompiledFuncs), não linkada no executável; o dirty afeta reprodutibilidade do gerado, não o runtime direto.

## RecompiledPatches

ORIGIN:
**TOOIE** (scaffold P05, gitignored)

ACTIVE CONTENT:
Nenhum. `patches.c` = `tooie_patches_placeholder()`, `patches_bin.c` = `bk_patches_bin[] = {0}` (`size=0`), `recomp_overlays.inl` = seções vazias. `register_tooie_patches()` (`src/main/register_patches.cpp:5`) é vazio e nenhum símbolo do diretório é referenciado em `src/`.

BK CONTAMINATION:
Não. O nome `bk_patches_bin` é apenas legacy de nome (cópia do padrão BK); conteúdo nulo.

## BK-specific runtime candidates

Arquivos em `src/game/` e `src/main/` **byte-idênticos** ao BanjoRecomp (diff confirmado): `config.cpp`, `recomp_api.cpp`, `recomp_data_api.cpp`, `recomp_extension_api.cpp`. `patches/` (headers) idênticos ao BK; `RecompiledPatches` = scaffold.

Cadeia de prova: o código recompilado do Tooie (`RecompiledFuncs/`, 417 .c) chama **somente** `recomp_syscall_handler` e `func_*`/`LOOKUP_FUNC` (mecanismo: JAL → `LOOKUP_FUNC(vram)` → `get_function` → `func_map` dos overlays Tooie, ver `N64MR/librecomp/src/overlays.cpp`). Exports host `recomp_get_*` só são alcançados por nome (patches) — e os patches do Tooie são vazios.

| ITEM | COMPILED | REFERENCED | REACHABLE | CLASSIFICATION | RISK |
|---|---|---|---|---|---|
| `recomp_get_note_saving_enabled` / `recomp_get_jinjo_saving_enabled` (recomp_api.cpp) | sim | não (fora do REGISTER_FUNC, zero refs no gerado) | **não** | **DORMANT BK** | LOW |
| `recomp_get_bgm_volume`, `recomp_get_cutscene_aspect_ratio`, `recomp_get_analog_cam_enabled`, inverted axes (recomp_api.cpp) | sim | registrados (`main.cpp:956-966`) | **não** (jogo nunca os chama) | **DORMANT BK** | LOW |
| `banjo::register_bk_overlays/register_bk_patches` (`include/ovl_patches.hpp:11-12`) | sim | sim | sim (mas delega a `tooie::*`) | **LEGACY NAME** | NONE |
| `banjo::decompress_bk` / `banjo::bk_on_init` (rom_decompression.cpp:26-42) | sim | sim (`main.cpp:576,580`) | sim (wrapper de `tooie::*`) | **LEGACY NAME** | NONE |
| `banjo::init_config` + opções note/jinjo/analog-cam/camera-invert (config.cpp) | sim | sim (`main.cpp:979`) | parcial (config/UI apenas; jogo não lê via exports) | **DORMANT BK** (config-only) | LOW |
| `recomputil::init_extended_object_data` (recomp_extension_api.cpp) | sim | sim (`main.cpp:975`) | genérico (mods/object data) | **SAFE GENERIC** | NONE |
| `banjo::get_game_thread_name` (main.cpp:585-640) | sim | sim | sim — mapeia IDs **Tooie** | **LEGACY NAME** | NONE |

## Non-reproducible state

`HEAD = 74259ff` **não** descreve sozinho o executável.

| ITEM | Efeito |
|---|---|
| N64MR submodule dirty (threadqueue.cpp) fora do git Tooie | runtime muda sem commit |
| N64Recomp submodule dirty (`use_lookup_for_all_function_calls=true`) | geração do RecompiledFuncs muda |
| `RecompiledPatches/` gitignored mas necessário ao CMake (`CMakeLists.txt:72-74`) | build falha sem ele |
| `RecompiledFuncs/` gitignored (417 .c → `.a` 14.8M) | artefato gerado não versionado |
| deps por path relativo `../BanjoRecomp` + symlink `assets →` | revisão não pinada |

## Scheduler bug relation

**LOCAL TOOIE PATCH IN SHARED RUNTIME.**

O patch em `N64ModernRuntime/ultramodern/src/threadqueue.cpp` (guard anti-duplicata) foi aplicado localmente no runtime compartilhado genérico para o Tooie. A interação é: `osSetThreadPri` → `thread_queue_remove` não limpa `t->queue` → o guard rejeita re-inserção → running queue esvazia → `run_next_thread` lança. Não é lógica BK-specific (não existe código de jogo BK envolvido) nem bug genérico upstream conhecido — é consequência do patch local conviver com a semântica de `thread_queue_remove` do runtime compartilhado. Detalhe: [[p08-no-threads-left-diagnosis]].

## Top risks

**P0:** N64MR dirty `threadqueue.cpp` — scheduler wedged, trava o jogo (bug ativo, HIGH).

**P1:** N64Recomp dirty + submodules não pinados — geração do executável não reproduzível; upstream pode evoluir e mudar o binário.

**P2:** `config.cpp`/`recomp_api.cpp` BK byte-idênticos com exports BK nunca alcançados — DORMANT hoje, mas fonte de confusão e risco futuro se alguém "ativar" patches BK por engano.

## Recommendation

**CONTINUE P08**

1. Registrar os dois diffs locais (threadqueue.cpp, N64Recomp main.cpp) com commit do N64MR pinado + diff referenciado, para tornar o estado auditável.
2. Ao aprovar o fix do scheduler, resolver via fix externo estreito no runtime (clear `t->queue` em `thread_queue_remove`) e remover o guard local, mantendo diff mínimo.
3. Deferir renomeação cosmética `banjo::*`/`bk_*` (sem impacto runtime); não registrar exports BK note/jinjo em `main.cpp`.

---

=== ACTIVE REUSE AUDIT ===

TOOIE EXECUTING BK GAME LOGIC:
NO

ACTIVE BK ITEMS:
Nenhum

DORMANT BK ITEMS:
`recomp_get_note_saving_enabled`, `recomp_get_jinjo_saving_enabled` (não registrados/chamados); `recomp_get_bgm_volume`, `recomp_get_cutscene_aspect_ratio`, `recomp_get_analog_cam_enabled`, inverted-axes (registrados, jogo não chama); opções config note/jinjo/analog-cam/camera-invert (config-only)

DIRTY SHARED DEPENDENCIES:
N64ModernRuntime @ `ca568b6a` — `ultramodern/src/threadqueue.cpp` (+27/−1); submodule N64Recomp @ `2b6f056` — `src/main.cpp` (`use_lookup_for_all_function_calls = true`)

RECOMPILED PATCHES:
TOOIE (scaffold vazio)

BUILD REPRODUCIBLE:
NO

CURRENT SCHEDULER BUG SOURCE:
LOCAL TOOIE PATCH IN SHARED RUNTIME (`threadqueue.cpp` guard anti-duplicata × `thread_queue_remove` sem clear `t->queue`)

TOP RISK:
P0 — N64MR dirty threadqueue.cpp: scheduler wedged, trava o jogo

SECOND RISK:
P1 — N64Recomp dirty + deps não pinadas: build não reproduzível

THIRD RISK:
P2 — exports/config BK DORMANT: confusão de manutenção, risco futuro

RECOMMENDATION:
CONTINUE P08

NEXT SINGLE ACTION:
Registrar diffs locais (threadqueue.cpp + N64Recomp main.cpp) com commit N64MR pinado para tornar o estado auditável

=== END AUDIT ===
