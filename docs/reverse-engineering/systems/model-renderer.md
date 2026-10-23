---
title: "Model renderer"
description: "func_800DE448 / DE498 and the player vs backpack callsites."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-10-22
tags: [reverse-engineering, model, renderer]
---

# Model renderer

## Draw entry (`PROVEN`)

`func_800DE448(position, arg2, scale, arg4, model_bin)` then
`func_800DE498` geolist / render-mode dispatch.

`func_800DE448` immediately calls `LOOKUP_FUNC(0x800DE498)`. `func_800DE498`
walks GeoCmd via `func_800DE2A4` jalr through `*(D_8012CF8C)`. That
submit is **immediate**. Wrapped `func_800E2180` is **not** dynamically
nested under `func_80092444` (`PROVEN` capture `NO_SCOPE`). Durable
model identity is the `model_bin` pointer (5th DE448 argument), not
PlayerState.

`TOOIE_DIAG_SECONDARY_SWAY=1` player-model CSV still uses the player
table + `func_80092444` for DE448 rows. Live `0x62B` type-2 identity is
taken at `func_800DCF48`. A wrapped `func_800DE2A4` is **list-head only**;
the snout node `model_bin+0x11CB8` is a sibling in the neck child list, so
it will not appear as DE2A4 `a1`. The 0x62B tree CSV scans that sibling
list under a DE498 token. `func_800E2AA4` is `DEMOTED` (capture raw 0).
Do not reuse `0x62A` offset `0x14960`, allocator span `0x20000`, or fat
dual-mtx field overlays on the compact 0x10-byte type-2 header.

`func_800DE498` starts the walker at `model_bin + *(u32*)(model_bin+4)`
(`PROVEN`). That `+4` field is the GeoCmd list offset (`0x119F0` on
`0x62B`, `0x146C8` on `0x62A`), not ExtraSegments. E2180 `a0` is the
Gfx packet pointer; `a1` is the GeoCmd node the type-2 handler received;
`a2` is forwarded into `func_800AF5FC`.

## Visibility gating & registered pre-draw callback chain (`PROVEN`)

Inside `func_800DE498` (guest `0x800DE498`, `RecompiledFuncs/funcs_326.c:4146..6782`),
model rendering is gated by a bounding-sphere frustum visibility check:

1. **Frustum Culling Test (`0x800DE6EC`)**:
   Calls `func_800E3E8C` (`funcs_326.c:4620`), evaluating bounding sphere visibility
   against view frustum side planes via `func_800CACEC`.
   - Result is stored to `0x8012C940` (`MEM_W(-0x36C0, ctx->r1) = ctx->r2`).
2. **Branch on Visibility (`funcs_326.c:4644`)**:
   - **When Culled (`v0 == 0`)**: Branches to `0x800DE718`, invokes reset helper
     `func_800DE318`, and exits at `0x800DF310` returning 0. All downstream callbacks,
     renderer context writes, and display list generation are bypassed.
   - **When Accepted (`v0 != 0`)**: Branches to `L_800DE728` (`0x800DE728`).
3. **Renderer Processing Context Writes**:
   On visibility accept, `func_800DE498` performs context stores to `s0+0x14`, `s0+0x18`,
   and `s0+0x1C`.
   - **Epistemic boundary**: `s0` is the **renderer processing context**, NOT an actor
     state structure (`PROVEN` distinction; do not conflate renderer context with actor state).
4. **Pre-Draw Callback Execution (`0x800DE738: jalr $v0`)**:
   - Callback pointer is loaded from `0x8012C9A8` (`MEM_W(ctx->r2, -0x3658)`).
   - Context argument `$a0` is loaded from `0x8012C9AC` (`MEM_W(ctx->r4, -0x3654)`).
   - Callback registration occurs prior to draw in `func_80101808` (`src/core2/1EDAEA0.c:23-32`):
     `func_800DF47C(callback, actor)` stores the function pointer at `0x8012C9A8` and
     the `Actor*` pointer at `0x8012C9AC`.
5. **Actor Pre-Draw Dispatch Chain (`func_80101870` path)**:
   - For standard actors drawn via `func_80101870` / `func_80101808`, registered callback
     is `func_801015D0`.
   - `func_801015D0` (`funcs_30.c:2772`) at `0x80101784` calls `func_80101934(Actor* arg0)`.
   - `func_80101934` (`funcs_214.c:1217`, `func_80101934.s`) masks `(arg0->unk74 & 0xF)` and
     indexes jump table `D_801243E8`. Table slot `+0x04` dispatches to `func_80101A50(arg0)`.
6. **RNG Stepping and Actor Mutation (`func_80101A50`)**:
   - `func_80101A50` (`funcs_238.c:2571`, `func_80101A50.s`) calls `func_800DC0C0`
     at `0x80101AA0` (RA `0x80101AA8`), advancing global LCG RNG state `D_8012C7F0`.
   - Compares random float `$f0 < D_80126434` (`0.03f`, 3% probability check).
   - On match arm: directly mutates guest actor memory via `sb $t2, 0x7A($a2)` (setting bit `0x10` or `0x20` on `Actor->unk7A`).
   - Both arms call `func_800DF744(1, ...)` and `func_800DF744(2, ...)`, mutating global
     renderer selector array `D_8012C850`.
   - **Epistemic Classification**: The 3% probability threshold check and dual selector
     updates strongly suggest eye-blinking or facial texture animation (`STRONG_INFERENCE`),
     but without explicit asset/animation binding proof in source it remains `STRONG_INFERENCE`.
     Control flow gating, global LCG RNG consumption, actor byte `0x7A` mutation, and
     renderer array writes are strictly **`PROVEN`**.

### Widescreen frustum gating boundary (`PROVEN`)

Because `func_800DE498` gates pre-draw callbacks that mutate actor state (`0x7A`) and advance global LCG RNG (`func_800DC0C0`), opening an aspect-expanded culling scope around `func_800DE498` or its shared frustum helper `func_800E3E8C` desynchronizes gameplay simulation. The recompiler runtime separates pure draw culling from simulation by:
1. Dropping generic `TooieCacecDrawScope` from `func_800E3E8C`, so unclassified callers and mixed renderer `func_800DE498` receive original unexpanded `v0`.
2. Allowlisting pure model display-list/matrix draw callers such as `func_800B97A4` (scope kind 3), which emits G_MTX / G_DL / G_MOVEMEM without RNG or actor callbacks, alongside preserved object-pop scope `func_8011447C` (scope kind 2).

### 0x8012C940, callbacks de ator e reset em 800DE318 (`PROVEN`)

A rotina de renderização `func_800DE498` possui múltiplos acoplamentos entre culling visual e lógica de simulação:

- **Teste condicional e gravação em `0x8012C940`**:
  O teste de esfera em `0x800DE6EC` (via `func_800E3E8C`) só é avaliado quando `D_8012C828 != 0`. O resultado da visibilidade é armazenado na variável global `0x8012C940`. Esse valor é posteriormente lido pela lógica do jogo através de `func_800DF350` por diversos módulos e atores (`sukickball`, `chpiranha`, `chcablespark`, `fxrain`, `fxleaves`).
- **Valor de retorno**:
  Retorna o ponteiro de `model_bin` em caso de visibilidade aceita ou `0` em caso de culling.
- **Callbacks de ator antes e depois do desenho**:
  Invoca os callbacks registrados do ator antes da emissão do desenho (`D_8012C9A8`, ver cadeia acima) e após a conclusão da emissão (`D_8012C9B0`).
- **Reset de configuração em `func_800DE318`**:
  A cada chamada, a função `func_800DE318` reinicializa o contexto do renderizador, incluindo a limpeza de `D_8012C844` (tabela de tags do GeoCmd tipo 10) e `D_8012C848` (tabela de zonas do GeoCmd tipo 15; ver [[geocmd]]).
- **Tentativa de desenho forçado (`REFUTED`)**:
  A tentativa de forçar o desenho em `func_800DE498` para widescreen restaurando `0x8012C940`/`v0` divergia a simulação com os callbacks ativos; com os callbacks zerados (para não divergir) os atores ficavam congelados na borda (a lógica ainda os trata como fora da tela) e algumas entidades eram desenhadas através de paredes/chão ou com estado errado (`USER_OBSERVED`; atribuição aos callbacks zerados = `STRONG_INFERENCE`). A correção adotada foi via planos nativos no volume de visão (ver [[view-volume]] e [[../../plan/evidence/p08-widescreen-native-planes-20261022]]).


## Player-table callsite (`PROVEN` `STATIC_MIPS` / generated C)

Core chain (not overlay-118 RA):

```text
func_800E9F20 → func_800F5008 / func_800F50D0
  → func_80098BA8 → func_80092444
    → func_800DE448 at guest 0x800926D8
```

`func_800E9F20` is a broader frame pipeline (`PARTIAL` player).
`func_800F5008` iterates `D_80135490[0..7]` (`PROVEN` player table).
`func_80092444` has a unique generated caller (`func_80098BA8`).
`a0` is the `PlayerState*`. Model 5th arg comes from
`func_80092B04(player, 1)`.

Generated statement: `LOOKUP_FUNC(0x800DE448)(rdram, ctx);` with
`a0=sp+0x60`, `a1=sp+0x6C`, `a2=0x50(sp)`, `a3=sp+0x54`,
`16(sp)=model_bin`. No `ctx->r31` write.

Overlay 118 `func_80817180` / `func_8081A474` remain additional DE448
sites. Their guest RAs (`0x808172A0` / `0x808172F4` / `0x8081A4D4`) are
**not** a valid generated-host filter. `TOOIE_DIAG_SECONDARY_SWAY=1`
player CSV uses a TLS player-table + `func_80092444` scope. Snout
sampling does **not**.

## Backpack callsite (`PROVEN`)

`babackpack_entrypoint_3` → D674C → DE448. Gameplay backpack `0x621`.

## Dual previous/current matrix (`PROVEN` on GeoCmd `0x02` path)

See [[geocmd]]. ExtraSegments models such as `0x61F` do not take that
path for the snout.

## BanjoRecomp analog (`PROVEN` absence; tagging not a ready port)

BK `func_803387F8` / `set_model_matrix_group(...)` tags GeoCmd2 / LOADDL
with explicit `gEXMatrixGroup`. Tooie `func_800DCF48` has the bone
push/pop and **no** group command (`PROVEN` absence). That missing
equivalent is **not** a ready tagging port. V5B NO_INTERP on 79, 100–105
was visually `SAME` but omitted JOIN parents 107/108 (`CORRECTED`).
`CORRECTED` vs earlier tagging-as-`ROOT_CANDIDATE`. The later boned SIMPLE
candidate is `REFUTED` as a sufficient fix; it is not the current production
recommendation. Provenance:
[[../../plan/evidence/p09-banjorecomp-transform-tagging-portability-audit]],
[[../../plan/evidence/p09-boned-interpolation-counterfactual]].

Current RT64 temporal semantics and proof boundaries are maintained in
[[transform-interpolation]]; visual root remains `UNKNOWN`.

## Modding relevance

**PROVEN ROLE:** submit a `model_bin` with pose.

**POTENTIAL MODDING USE:** model replacement at DE448; child geometry
edits depend on ExtraSegments vs GeoCmd class of the live ID.
