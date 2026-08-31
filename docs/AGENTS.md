---
title: "Regras para agentes — TooieRecomp"
description: "Regras de navegação, implementação e evidência para agentes que trabalham no plano TooieRecomp."
status: current
type: agent-rules
project: TooieRecomp
tags: [agents, workflow, tooierecomp, rules]
---

# Regras para agentes — TooieRecomp

## Workspace

Abra o CLI/IDE em `TooieRecomp/` (este repositório git). Não use
`N64-Recomp/` como raiz do agente: as configs de MCP/vault agora vivem
aqui (`docs/MARKDOWN_VAULT_TOOLS.md`).

Irmãos no disco, fora deste git, que o **build** ainda usa:

- `../BanjoRecomp` — RT64, N64ModernRuntime, assets (`CMakeLists.txt` `BK_ROOT`)
- `../Banjo-Tooie (USA).z64` — cartucho do oráculo Angrylion (`TOOIE_CART_Z64`)
- `../banjo-tooie-Recomp` — decomp; não é o executável Tooie

Não copie `.venv` do pai. Não trate `BanjoRecomp` como o jogo.

## Ordem de leitura

1. Leia [[plan/index|o plano mestre]].
2. Localize a fase ativa em [[plans/BT/index]].
3. Leia somente as specs BK/BT relacionadas pela nota da fase.
4. Selecione um ID de tarefa aberto antes de alterar código.

## Regras de execução

- Não pule o gate de saída da fase anterior.
- Não trate o BanjoRecomp como emulador nem troque apenas sua ROM.
- Use BK como contrato de runtime e BT como fonte dos dados do jogo.
- Corrija geradores e metadados; nunca edite código recompilado gerado.
- Faça o build falhar para símbolo, relocation, syscall ou tarefa RSP desconhecida.
- Preserve comportamento, timing e 4:3 originais antes de adicionar melhorias.
- Não copie, distribua ou versione ROM e assets proprietários.
- Não altere um hash esperado sem evidência e revisão da fonte de verdade.

## Evidência obrigatória

Para fechar uma tarefa, registre na nota da fase:

- comando reproduzível;
- exit code e resumo do resultado;
- commit dos repositórios envolvidos;
- hashes dos artefatos binários relevantes;
- teste ou reprodução que prova o critério de aceite.

Depois, marque o checkbox, atualize o `status` da fase quando aplicável e rode
`markdown_vault_audit`.

## Fontes de verdade

- Critérios finais: [[plan/definition-of-done]]
- Reutilização BK: [[specs/BK/reuse-contract]]
- Prontidão BT: [[specs/BT/recomp-readiness]]
- ROM de entrada: SHA-1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`
- Oráculo de medição `ANGRYLION_CONTROL`: [[specs/BT/angrylion-control-oracle]] (`tools/oracle/angrylion-control/run.sh`). Angrylion não substitui o RT64.

## Contrato de continuidade entre agentes

Cada tarefa técnica deve terminar revisando:

- evidence específica;
- `docs/plan/evidence/p08-session-handoff.md` quando o estado operacional mudou;
- `docs/HANDOFF.md` quando o estado global mudou;
- phase plan ativo;
- specs relacionadas;
- indexes/vault quando necessário.

Specs = contratos estáveis.
Evidence = investigação e hipóteses.
Handoff = estado corrente para novo chat.

P08 `WIDESCREEN_EDGE_GEOMETRY` current: `800CAA24` production ON;
`800CACEC` object frustum production ON (user visual confirmed). Two
cinematic wall defects are now named separately:

- `SCENE_A_FIRST_VISIBLE_AFTER_LAUNCH` — T_INTRO mode=3 map=0, first 3D
  after launch. Task head G_DL `0x00123340` is **setup-only**. First env
  triangles at frame 5: heap `0x0031f6d8`/`0x0031f8f8`, MV `0x8017b0b0`
  T(camera) (529.38, 6943.89, -1904.13).
- `SCENE_B_NEW_GAME_INTRO` — post-gameselect replay of the same T_INTRO /
  same setup head. Heap env G_DL addresses are **not** content-stable.
  Do **not** assign MAP_18B from the mode-struct (map=0).

T_INTRO 16:9 first RT64 divergence is `CLIP_X` (`RT64_PROJECTION_TRANSLATION`,
HIGH): Expand AUTO scales cinematic `viewProj[0][0]` `0.905148` → `0.678861`
with `K=4/3` from VI `304/228` vs window `16/9`. Guest RSP clip MATCHES.
Guest presentation-envelope Original/Expand owner diff is `0` (`REFUTED`).
`±0.754` is vertical cinematic letterbox, **not** horizontal crop. Do not
promote `CAMERA_FRAMING_EXPOSES_AUTHORED_EXTENT` for current 16:9 from that
invalid axis. `TINTRO_16X9_AUTHORED_EXTENT` is `STRONG_HYPOTHESIS` (extra
FOV contents; not independent of the 4/3 scale). 21:9+ authored extent
stays `PROVEN` and separate.

F3DEX2 `G_DL_PUSH=0` is a call (`0xDE00`); `0xDE01` is branch. Do not
treat `0x00123340` or static OPA 7417 as the first-scene wall owner.
Prior WALL7759/7799 work belongs to **SCENE_A** (late first-play pose
kOpenCam), not SCENE_B. Type-13 remains `AABB_FRUSTUM_GATE`
(`func_800E3DC0`/`func_800CAB70`). Opening-camera first reject for the
six lateral leaves is **LEFT** (`CAB70`, dist +34.23…+576.19 vs guest
1.767), but those leaves are **not** the visible hole owner.
`SOURCE_REJECT_TARGET_ACCEPT: NO`. `REAL_HOLE_FIRST_DROP_STAGE: NO_CPU_DROP`.
Do **not** add `TOOIE_TEST_INTRO_TYPE13_ASPECT` or `TOOIE_TEST_INTRO_7799_VIS`.
Do not widen CAB70. Clamp16x9 `REFUTED`. Do not pillarbox ±0.754. Do not
force WALL7799. Do not copy Banjo GeoCmdD `TRUE`. Do not change RT64 AUTO
globally. The centered black-pillarbox candidate for T_INTRO wider than 16:9
was tested acceptable experimentally, then deferred and removed from both
worktrees; it is not current production behavior. Next queue: animated/swaying
model-part glitches, then intermittent audio “tic tic”; both roots remain
`UNKNOWN`. Do not mix the deferred ultrawide issue into that investigation.

Toda conclusão deve ser classificada:
- `PROVEN`
- `REFUTED`
- `CORRECTED/SUPERSEDED`
- `INFERENCE`
- `DISPUTED`
- `UNKNOWN`

Não promover hipótese para spec.

Quando modelos discordarem:
registrar a divergência explicitamente.

Antes de encerrar:
- `markdown_vault_audit`
- `git diff --check`
- status do worktree

Residual P08 ativo (`WIDESCREEN_EDGE_GEOMETRY`): estado canônico em
[[HANDOFF]] e [[plan/evidence/p08-session-handoff]].
`gspropsDll_entrypoint_1` é load one-shot type-6 de todos os gccubes, não
visibilidade por frame. Consumidor per-frame type-6: `800FF62C` sobre
`D_80136EE0`; vis euclidiana `800EFFB4` (não FOV/aspect); TYPE6 vis CF
`NO_CHANGE`. Sprites 7/9/C (`800CBB18` = type 0xC, não 0xA) disparam no
load do mapa via `gccube_ep6`; `800F0064` é círculo XZ mundo, não
câmera/FOV/aspect; GSPROPS vis CF `PARTIAL_HIT` em inserts e `REFUTED`
como pop Expand. Instâncias `800B3CDC` em `D_8012AE80+0xE0` são occupancy
query (`800CB8F0` LIVE 5777; `draw_fn=NONE`);
`BOUND_SPRITE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED`. Não envolver
`80108DC0` como vis. Não tratar `800F0064` como gate FOV. Não reabrir
draw de bound-sprite. Não combinar flags MAP_VIS / MODEL_GEO / PROP_CELL /
GSPROPS_VIS / TYPE6_VIS. `8011458C` é occupancy/near via `801143A4`,
não dispatch de actor. Cadeia actor
`80114184`→`800BD708`→`800CD23C`→`800EA628`→`80103508`→`8010108C`.
CF `TOOIE_TEST_EXPAND_ACTOR_CUBE_AABB`: `PARTIAL_HIT` visitation /
`NO_CHANGE` draw-DL. `ACTOR_PICKUP_VIEW_CULL_AS_EXPAND_ROOT: REFUTED`.
`80108C14` é submit raro da mesma lista type-6 `D_80136EE0` (`LIVE` 2 vs
`800FF62C=1337`); jalr GETTER, não DRAW; gates RESOURCE only.
`80108C14_AS_REMAINING_DRAW_ROOT: REFUTED`.
`REMAINING_STATIC_DECORATIVE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED`.
`800CAA24` aspect-aware (H `n_cam_x /= k`) é **produção ON** (visual
gameplay confirmado; `TOOIE_TEST_MAP_VIS_ASPECT_AWARE=0` restaura original).
`800CACEC` object frustum H-expand is **produção ON** (user visual
confirmed; `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0` restaura original;
DRAW-only). `ACTOR_CUBE_AABB_SELECTION_AS_EXPAND_DRAW_ROOT: REFUTED`.
`TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED`. Intro walls
`STILL_PRESENT` as two named scenes (SCENE_A first-visible T_INTRO,
SCENE_B new-game replay). Class `CAMERA_FRAMING_EXPOSES_OUTSIDE_MODEL`
as current 16:9 root is `CORRECTED/SUPERSEDED`. Next is SCENE_A opening
type-13 reject plane, not 819F8. `CLEAN_PINNED_BUILD_VALIDATION:
PASS` (Tooie `23c070a9`, RT64 `6f1c2d99`; artifact
`/tmp/p08-clean-milestone-validation-20260830T035059Z/`). Wording
“GeoCmd intra-model untested” is `CORRECTED/SUPERSEDED`: era
`800E3DC0`/`800CAB70`. Não voltar à GPU. Não combinar flags de vis.
