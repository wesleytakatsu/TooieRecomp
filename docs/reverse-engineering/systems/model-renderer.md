---
title: "Model renderer"
description: "func_800DE448 / DE498 and the player vs backpack callsites."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-11-01
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
  A tentativa de forçar o desenho em `func_800DE498` para widescreen restaurando `0x8012C940`/`v0` divergia a simulação com os callbacks ativos; com os callbacks zerados (para não divergir) os atores ficavam congelados na borda (a lógica ainda os trata como fora da tela) e algumas entidades eram desenhadas através de paredes/chão ou com estado errado (`USER_OBSERVED`; atribuição aos callbacks zerados = `STRONG_INFERENCE`). A correção adotada foi via planos nativos no volume de visão (ver [[view-volume]] e `p08-widescreen-native-planes-20261022`).


## Model distance rejection and fade (`PROVEN` static code)

Distance rejection is separate from the side-plane sphere test described above. In `func_800DE498`:

- `0x800DE4F8..0x800DE560` computes model position minus camera position, then rejects excessive absolute
  displacement on any axis against the three constants at `0x80125D54..0x80125D5C`. Their numerical values were
  not established in this analysis; these checks must not be assumed safe to remove as ordinary visual distance.
- `0x800DE578..0x800DE60C` obtains two unsigned model measures from offsets `+0x12` and `+0x14` of the model's
  auxiliary header, or fallback globals when no model is supplied. `func_800EEF94` computes Euclidean camera
  distance from the displacement vector.
- `0x800DE610..0x800DE688`: at distance at least `4000`, if the current limit equals the default comparison
  constant at `0x80125D60`, the limit at `0x8012C82C` can be reduced to
  `header_measure_12 * model_scale * value_at_8012C830 * 50`. The precise asset meaning of measure `+0x12`
  remains `UNKNOWN`; `+0x14 * model_scale` supplies the later sphere radius.
- `0x800DE68C..0x800DE6C8` multiplies that limit by `func_800E42C0()` and rejects when distance is greater than
  or equal to the result, before the frustum test and registered callbacks. `func_800E42C0` delegates to
  `func_800CA6E8`, which reads the current view-volume float at `+0x164`; this is not proof of a far-clip role.
- `0x800DE800..0x800DE880` optionally fades alpha over the last `700` units before the resulting limit.
  `func_800DF470` sets the fade enable at `0x8012C834`. This is a model distance fade, not the map's fog contract.

`func_800DE318` resets `0x8012C82C` from `0x80125D50` and `0x8012C830` to `1.0` for subsequent draws.
`func_800DF428(float)` exchanges the distance-limit float and returns its previous value;
`func_800DF464(float)` writes `0x8012C830`. A one-time global write does not implement a persistent menu policy,
and multiplying only the initial limit can change the default-equality branch above.

Primary executable representation: `RecompiledFuncs/funcs_326.c` (`func_800DE498`), `funcs_367.c`
(`func_800DE318`), `funcs_296.c` (`func_800DF428`), `funcs_112.c` (`func_800DF464`), `funcs_353.c`
(`func_800DF470`), `funcs_193.c` (`func_800EEF94`), and `funcs_201.c` (`func_800CA6E8`). Generated files were
read only. No live game capture established universal class/map coverage.

### Boundary with actor scheduling and map visibility

`PROVEN`: increasing the model limit alone does not widen `func_8010038C`'s actor update radius. The actor loop
`func_800FF62C` tests that separate gate before invoking descriptor update callbacks. See
[[warp-and-player-position#Glowbo collection and deferred actor cleanup]] for its flags, focused-player position,
radius and sector limitations. `func_8010A79C` also checks initialization/activity/deletion-related flags before
dispatching descriptor draw callback `+0x14`; receiving a draw request is not unconditional drawing.

`INFERENCE`: a configurable distance policy should preserve frustum/sector/state checks and coordinate any
necessary animation/resource residency changes per actor family. Render-all by disabling all culling is not
equivalent to removing distance rejection. The documented forced-frustum experiment remains `REFUTED`.
`UNKNOWN`: complete coverage of Jinjos, Jiggies, sprites, enemies, map-specific props and effects by the model
limit, the necessary upstream residency changes, and performance at increased distances. A shared renderer path
does not prove every class reaches it at arbitrary distance. No universal safe multiplier has been validated.

### Actor scheduling and resource retention

`PROVEN` static code: `func_8010A79C` requires bit `0x8000` of actor halfword `+0x7E`, clear bit `0x20000`
of word `+0x64`, and set bit 0 of **halfword** `+0x64`, before obtaining the descriptor through `func_80100368`
and invoking its draw callback `+0x14`. Halfword `+0x64` is the high half of the big-endian word.

The resource gate `func_80100230` (`RecompiledFuncs/funcs_411.c`) requires actor bit `0x4000` at `+0x7C`.
For nonzero high-halfword radius, it compares `func_800EEB40(player_point, actor+4)` to the squared radius and
returns 1 outside the radius; radius zero also returns 1. `func_801002C0` uses a positive result before checking
the actor's syscall/overlay reference with `func_80081D34` and calling `func_800819B4`. Classification as a
distance resource-retention boundary is `STRONG_INFERENCE`; the branches/calls are `PROVEN` static code.
The integration scales only this gate's matching squared-distance result, preserving radius storage,
unload APIs and special zero-radius semantics. Existing distant actors can fail later draw selection;
absence from rendering does not establish absence of an actor record.

### Spatial collection and sprite paths (`PROVEN` static code)

The normal collection path is `func_800BD594` -> `func_800D3104` -> `func_800EB5E0`. Map flag `0x200`
selects the alternate all-actor collector `func_8010A874`. In `func_800D3104`, candidate grid bounds are
clipped to `4 * func_800E42C0()` cells from the camera cell (`0x800D3264..0x800D333C`). Each nonempty
candidate passes `func_800D2F44` before its entries reach the queue. That function first compares squared
camera-to-cell-center distance with a radius derived from four cell widths, view scale and the constant
at `0x80125B00`, then calls the native AABB test on cell bounds plus margins. Extending only a model cutoff
or update radius cannot extend these earlier bounds. Primary generated spans: `funcs_160.c`, `funcs_311.c`,
`funcs_87.c` and `funcs_45.c`.

Placed props need not have actor records. `func_800EB51C` dispatches entries with word `+0x08` bit 0 set
through `func_800EC7D4`; other entries go through `_gspropprop_entrypoint_3`. Its model branch calls
`func_800BFAD0` -> `func_800DE448`, installing a grid-derived limit and model fade. Its sprite branch calls
`func_800BFB58` -> `func_800E2AA4`, installing a separate sprite limit/fade. Both depend on cell selection.
Primary generated spans: `funcs_96.c`, `funcs_226.c`, `funcs_77.c` and `funcs_270.c`.

`subaddiedraw25d_entrypoint_1` renders through `func_800E2C54` -> `func_800E1C00`, not `func_800DE498`.
`subaddiedraw25d_entrypoint_3` installs descriptor `+0x1A`'s limit, or a grid-derived default through
`func_800D2F20`/`func_800E249C`. Sprite cutoff is word `0x8012D494`; fade enable is byte `0x8012D492`.
`func_800E1C00` uses view-depth from `func_800E3928`, rejects negative depth or depth beyond the scaled limit,
has a default-limit projected-size shortcut controlled by byte `0x8012D491`, and fades alpha across the
final **500** units when enabled (`0x800E1D68..0x800E1DDC`). Primary generated spans: `funcs_159.c`,
`funcs_211.c`, `funcs_412.c`, `funcs_399.c`, `funcs_373.c` and `funcs_99.c`.

Marker `+0x14` identifies a model/resource, not an actor class. Native descriptors associate resource 1954
with `chsignpost` (radius 450, draw `func_80101870`) and 1649 with `chdiggerground` (radius 3000,
draw `func_808001C8_chdiggerground`, whose normal branch calls `func_80101870`). `chpickups` descriptors
also include model callbacks; not every pickup is a sprite. Current map is halfword `0x80132DC2`, read by
`func_800EA05C`; `0x80127630` is the game-state header.

### Coordinated distance policy (`PROVEN` fixture; Windows user-validated)

The integration scopes `func_800D3104` and matching `func_800D2F44` invocations. Only their exact native-stack
calls to `func_800E42C0` receive an enlarged distance scalar; unrelated queries retain their native value.
Native grid bounds are clipped less for 2x/4x, or not within ordinary map extents for the finite Unlimited
scalar. Native cell eligibility and AABB tests still run; no global FOV/projection change is made.

The model policy preserves physical length and adjusts the finalized limit at its scoped `func_800E42C0`
call. For the default limit, resolving the native 4000-unit optimization gives
`min(default * S, max(4000, size_limit * S))`, where `S` is the viewport scalar. Native 700-unit alpha
operates on physical distance and the enlarged boundary. Player models and native-frustum-disabled draws
retain native cutoff/alpha. Coordinate guards, callbacks and reset remain native.

Sprite preparation receives an extended limit for story world views 0..3 outside player draw scopes.
Input limit and shortcut/fade flags are restored after preparation; native output position and alpha remain
available to geometry. Physical view-depth and the 500-unit fade calculation remain native. The projected-size
shortcut is bypassed within enhanced preparation, including when a multiplied limit equals its sentinel.

`CORRECTED/SUPERSEDED`: dividing physical model distance removed radial fade, and a later temporal appearance
cache could restart transparency after omitted drawing without a new actor lifetime. Both approaches were
removed. The final policy has no temporal cache, frame counter or diagnostics in the game executable.

The regression executes native model cutoff/reset/alpha, update/retention gates, grid collection and radial
cell rejection together, and sprite preparation/fade. It covers policy/zoom, AABB/empty-cell rejection,
unscoped helper isolation, physical distance, scripted transparency, player/UI exclusions, initialization,
zero-radius semantics, living J2, mode guards and real Config Apply/Cancel/persistence/defaults. World/grid,
camera/frustum, player/resource services and sqrt are synthetic; model probes stop before geometry/GPU.
The user accepted the revised Windows gameplay behavior; exhaustive class/map/performance coverage and Linux
remain `UNKNOWN` / `NOT_RUN`.

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
`p09-banjorecomp-transform-tagging-portability-audit`,
`p09-boned-interpolation-counterfactual`.

Current RT64 temporal semantics and proof boundaries are maintained in
[[transform-interpolation]]; visual root remains `UNKNOWN`.

## Player lighting

Player CPU lighting can mutate shared model vertices when instances lack private output vertices. The experimental
story split-screen integration allocates those outputs through the native instance allocator. See
[[player-lighting]] for the proven path, ownership and runtime/visual verification boundary.

## Modding relevance

**PROVEN ROLE:** submit a `model_bin` with pose.

**POTENTIAL MODDING USE:** model replacement at DE448; child geometry
edits depend on ExtraSegments vs GeoCmd class of the live ID.
