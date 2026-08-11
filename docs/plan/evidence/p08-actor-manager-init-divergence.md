---
title: "P08 — actor manager init: divergence ausente no HEAD (premissa desatualizada)"
description: "Diagnóstico: a cadeia legítima chbaddieDll ep1 → func_800FF390 → func_801065E0 → D_80136EE0 executa NATURALMENTE no HEAD c43e89b. A evidência anterior de 0 hits (a6053a0) foi coletada antes dos fixes de relocations/overlay-cache/scheduler. Não existe primeira divergência neste caminho no HEAD."
status: evidence
phase: P08
updated: 2026-08-10
tags: [banjo-tooie-recomp, p08, actors, chbaddieDll, actor-manager, D_80136EE0, func_801065E0]
---

# P08 — actor manager init: premissa desatualizada (cadeia executa no HEAD)

## Objetivo

Determinar quem provoca `chbaddieDll_entrypoint_1` (overlay 181 ep1) no fluxo
natural da abertura e qual é a primeira divergência que impede a cadeia

```
chbaddieDll ep1 → func_800FF390 → func_801065E0 → D_80136EE0 = vector_new(0x9C, 0x1E)
```

de acontecer no TooieRecomp.

## Resultado

**A cadeia EXECUTA naturalmente no HEAD `c43e89b`. Não há divergência neste
caminho.** A premissa fechada ("func_801065E0 nunca rodou / D_80136EE0 NULL")
é **desatualizada**: foi medida em `a6053a0`, antes dos fixes posteriores já
contidos neste HEAD (`5ac74c0` overlay-cache, `f9d1c24` scheduler).

## Baseline natural

```
TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1
TOOIE_DIAG_SPAWN_MARKER419: OFF (unset)
```

Estável 45 s, sem crash, ~59k syscalls, audio/RSP ativos, stack idle.

## EXPECTED CALL CHAIN (estática)

```
func_800A76F4 (set game mode) / func_800A7BB8
  → func_800A72A4
  → func_800EA0CC
  → _gsworldDll_entrypoint_2   (stub 0x80088B68, syscall 726 ep2)
  → gsworldDll_entrypoint_2    (0x808002A4)
  → func_800A5C28              (0x80800470)
  → _chbaddieDll_entrypoint_1  (stub 0x80085928, syscall 181 ep1)
  → chbaddieDll_entrypoint_1   (0x80800010)
  → func_800FF390
  → func_801065E0
  → vector_new(0x9C, 0x1E)     (0x800B2D70)
  → D_80136EE0
```

- **Quem provoca ep1:** `gsworldDll ep2`, que roda no fluxo de inicialização de
  mundo/startup (chamado por `func_800A72A4` ← máquina de modo de jogo).
- **Fase esperada:** map init / game mode (mode 0→2→3), durante a seção
  `MAP_18B_CS_STARTUP_INTRO`.
- **Mecanismo real:** `func_800A5C28` chama o stub `0x80085928` (syscall 181
  ep1) — não é "load de overlay 181" que chama ep1; é um dispatch explícito.

## OVERLAY 181 LIFECYCLE

| item | valor |
| --- | --- |
| overlay 181 solicitado | YES (boot `ep=0`, e dentro de gsworldDll ep2 `ep=1`) |
| carregado | YES (`ovl_load idx=181` em boot e em ~line 1559) |
| mapeado | YES |
| resident | não persistente (recarregado por dispatch) |
| entrypoint 0 | 1 hit (boot, line 27) |
| **entrypoint 1** | **1 hit (line 1572, dentro de gsworldDll ep2)** |
| entrypoint 2 | 0 |
| unload | normal |

## EXPECTED vs ACTUAL (por nó)

| nó | esperado | real |
| --- | --- | --- |
| gsworldDll ep2 (726:2) | YES | 1 hit (dispatch line 276, 39-59 ms) |
| func_800A5C28 | YES | roda (dentro de 726:2) |
| _chbaddieDll ep1 (181:1) | YES | 1 hit (dispatch line 1572) |
| chbaddieDll ep1 | YES | 1 hit (GDB confirma) |
| func_800FF390 | YES | ENTER/LEAVE (trace) |
| func_801065E0 | YES | ENTER/LEAVE, r2=0x801df080 |
| vector_new(0x9C,0x1E) | YES | ENTER/LEAVE r2=0x801df080 |
| D_80136EE0 | != 0 | **0x801df080** (persiste) |

GDB (binário sem instrumentação, sem env de trace):
```
CHBAD_EP1_HIT #1
INIT_0FF390_HIT #1
INIT_1065E0_HIT #1
```

## FIRST DIVERGENCE

**Inexistente neste caminho no HEAD.** Nenhum nó da cadeia falta; todos os
transitions esperados acontecem. A divergência que a P08 procurava (init do
actor manager nunca ocorria) foi **resolvida pelos fixes já contidos neste
HEAD**:

- `5ac74c0` — invalidação de overlay-cache stale em `ovl_unload`;
- `f9d1c24` — estabilização do scheduler.

A evidência de "0 hits" em `p08-actor-init-segv-diagnosis.md` foi coletada em
`a6053a0`, **antes** de `5ac74c0`/`f9d1c24` (relocations `36a4005` já estavam
presentes na janela antiga). Após esses fixes, `gsworldDll ep2`
roda até o fim e alcança `func_800A5C28 → chbaddieDll ep1`.

## EXPECTED STATE / ACTUAL STATE

- Modo de jogo: mode 0 (boot) → 2 (startup) → 3 (gcsection/game mode).
- `gcsectionDll ep4` chamado com `a0=0x18b` (MAP_18B startup intro): YES.
- `D_80136EE0`: esperado != 0; real = `0x801df080`.

## ROOT CAUSE CLASS

**I — unknown/inconclusive para "primeira divergência", porque não há
divergência restante neste caminho.** A classe aplicável no momento da
evidência antiga era A/E (overlay lifecycle/registration), mas foi corrigida
pelos commits já presentes no HEAD. Nenhuma ação de fix é necessária para que
a cadeia legítima aconteça.

## CONFIDENCE

HIGH — evidência dupla (trace instrumentado env-gated + GDB em fluxo
inalterado, com leituras de endereço corrigidas).

## EVIDENCE

- `src/game/syscall_handler.cpp` trace env-gated `TOOIE_DIAG_TRACE_181`
  (aplicado e revertido; não comprometido) — capturou dispatch 181:1 e
  ENTER/LEAVE de ff390/1065e0/vector_new.
- GDB `chbaddieDll ep1 / func_800FF390 / func_801065E0` em fluxo natural:
  1 hit cada, sem SIGSEGV, processo estável.
- D_80136EE0: `00000000` antes, `801df080` após ep1, persistente.

## NÃO FEITO

- Não chamei ep1/ff390/1065e0 manualmente.
- Não escrevi D_80136EE0; não spawn de marker 419/569; nenhum actor fake.
- Nenhuma alteração de mode/map, overlay lifecycle ou host syscall.
- Nenhum fix implementado; alterações de diagnóstico revertidas.
