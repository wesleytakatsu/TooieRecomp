---
title: "Etapas de implementação — TooieRecomp"
description: "Índice ordenado das etapas necessárias para entregar Banjo-Tooie nativo no Linux."
status: active
type: implementation-index
project: TooieRecomp
updated: 2026-08-15
source_path: banjo-tooie-Recomp
source_repo: wesleytakatsu/banjo-tooie-Recomp
upstream_repo: Mr-Wiseguy/banjo-tooie
tags:
  - banjo-tooie-Recomp
  - implementation
  - linux
  - index
---

# Etapas de implementação — TooieRecomp

Execute as notas na ordem. Cada uma define entradas, tarefas, artefatos,
verificação e gate de saída; checkboxes são deliberadamente usados para que
`markdown_vault_extract_tasks` produza a fila de trabalho.

Sob a arquitetura standalone da P10, todas as fontes necessárias de decompilação, runtimes e assets estão vendorizadas diretamente em `decomp/`, `lib/` e `assets/`. O build normal é 100% kit-free (`tools/build_linux.sh preflight verify extract recomp configure build test package`), não usando checkouts externos.

| Ordem | Etapa | Resultado | Status |
| ---: | --- | --- | --- |
| 00 | [[00-baseline-and-decisions]] | baseline reproduzível | done |
| 01 | [[01-linux-toolchain]] | toolchain validada | done |
| 02 | [[02-rom-decompression-and-elf]] | ROM decomposta e ELF matching | done |
| 03 | [[03-recompiler-metadata]] | metadados consumíveis pelo N64Recomp | done |
| 04 | [[04-static-recompilation]] | código recompilado compila | done |
| 05 | [[05-native-runtime-and-boot]] | executável chega ao primeiro frame | done |
| 06 | [[06-overlays-and-syscalls]] | código dinâmico funciona | in_progress |
| 07 | [[07-rsp-and-audio]] | tarefas RSP e áudio corretos | active (música inicial OK) |
| 08 | [[08-rt64-input-and-frontend]] | renderização, entrada e UI integradas | active |
| 09 | [[09-gameplay-parity-and-enhancements]] | campanha e timing aprovados | pending |
| 10 | [[10-linux-packaging-and-release]] | release Linux reproduzível | pending |

## Convenções

- `Pxx-Tyy` é um identificador de tarefa estável.
- `status` aceita `pending`, `active`, `blocked` ou `done`.
- “Gate de saída” é obrigatório; sem ele, a próxima etapa não começa.
- Evidências devem conter comando, resultado, commit e hash quando aplicável.
- Decisões arquiteturais atualizam também [[../../plan/index|o plano mestre]].

## Contexto

- Plano mestre: [[../../plan/index]]
- Critérios finais: [[../../plan/definition-of-done]]
- Referência BK: [[../../specs/BK/index]]
- Insumos BT: [[../../specs/BT/index]]

### Nota P06 (2026-08-09)

Dispatch de overlay syscalls implementado e validado em runtime (decode ROM +
tabela gerada + `load_overlays` + call/return). Boot host carrega core1/core2
sem inflate na imagem descomprimida. `ovl_load` no heap, scheduler/ACK e restore
aninhado estão implementados; lifecycle e relocations receberam correções
adicionais durante P08. Detalhe: [[06-overlays-and-syscalls]].

### Nota P07 (2026-08-09)

`n_aspMain` Tooie (RSPRecomp `0x17D0`), dispatch `M_AUDTASK`, SI+CIC-6105
(host) para destravar `gldb`/`gsmaker`/`gsworld`. Música **inicial validada
por ouvido** no sink default. Residual: SFX/pause/hotplug/soak (T06 resto +
T07). Detalhe: [[07-rsp-and-audio]].

### Nota P08 (2026-08-14)

RT64/Vulkan: intro/Jiggy OK; type-4 completa e warp consumido; **visible
attract/gameplay demo reproduced** (mode-5, map 335). PATCH 4 behavior is
independently **CLASS A**. Verdict C preserved; the tracked-state finding
is resolved by independent verdict B, and PATCH 4 is accepted.
`func_800BCF28` binding fix **ACCEPTED (verdict B)**. `func_80013A7C`
lookup-zero **RESOLVED**. Render-submission matrix split **RESOLVED**.
`func_80013B70` stick-angle override is **ACCEPTED at function level** after
focused independent re-review verdict A
([[../../plan/evidence/p08-stick-angle-thunk-fix-rereview]]). Manual WASD
runtime is **RESOLVED**. Gameplay-demo residual is characterized in
[[../../plan/evidence/p08-demo-movement-precision-diagnosis]]; the
post-physics velocity writer discriminator is **COMPLETE** (Grok 4.6 HIGH):
first-straight 510 is original slope-scaled physics (`func_800EF04C` from
`func_8009AE08`); the post-physics-writer hypothesis is **REFUTED**. The historical
verdict C is preserved in
[[../../plan/evidence/p08-stick-angle-thunk-fix-independent-review]].
Current P08-T10
synthetic confirm → mode=1 is **FAIL** (separate). Attract/cinematic facing
is **OPEN_SEPARATE**. The historical one-actor Level B facing claim is
**CORRECTED/SUPERSEDED** by
[[../../plan/evidence/p08-forced-cinematic-facing-independent-review]]:
the original return-facing contract and selected node discriminator remain
**UNKNOWN**; no fix is implemented. P08-T03 original timing/4:3 baseline is
**VALIDATED**.
**P08 USER_DEMO_4 (2026-08-21):** MIPS `round.w.s` nearest-even is
checkpointed (`5854872c79a7cab1c9395ddcd173cece449ffe11`;
[[../../plan/evidence/p08-user-demo4-sincos-semantics-fix]]). Bind-tick and
idle follow-cam glidx 1..38 match ANGRYLION_CONTROL.
`CAMERA_COMPLETE_FIX: NO`. `CAMERA_FEEDBACK_ACCUMULATION: STILL_PRESENT`.
Remaining first difference is dump glidx 39 walking-player `ncbapivot`
jumptable `80112C3C` on `*(node+0x60)+8` (0 vs 5;
[[../../plan/evidence/p08-user-demo4-walkstart-ncbapivot-discriminator]]).
P08 `WIDESCREEN_EDGE_GEOMETRY` is **CLOSED FOR CURRENT SCOPE**: gameplay
visibility and tested 16:9 T_INTRO projection are production-closed; the
ultrawide authored-extent residual is deferred. The next tracked work is the
open graphical/audio bug queue in [[../../HANDOFF]]. P09 remains **PENDING /
NOT_FORMALLY_OPEN**.
WIDESCREEN_EDGE_GEOMETRY (2026-08-31): T_INTRO 16:9 first RT64
divergence is CLIP_X (`K=4/3` AUTO on cinematic viewProj). Guest
presentation-envelope Original/Expand owner diff is `0` (`REFUTED`).
Canonical: [[../../plan/evidence/p08-widescreen-edge-geometry]].
P08 closure: the centered max-16:9 pillarbox candidate for T_INTRO ultrawide
was tested acceptable but is `DEFERRED_AND_REMOVED_FROM_WORKTREE`; the next
single action is animated/swaying model-part graphical-glitch investigation.
The local `0x800979D8` mapping passes its focused contract test. The corrected
BCE84 ring-1 raw join content matches the bounded held-input/P60/ncbapivot
chain (`0x115` → `1` → `2`, `p60.C/E/+8=1/2/5`) on both runtimes; the
the final paired capture at `/tmp/p08-979d8-final-20260822T070735Z` verifies
twice, so the bounded immutable checkpoint is accepted. See
[[../../plan/evidence/p08-user-demo4-800979d8-independent-review]].
`ANGRYLION_CONTROL` lives in `tools/oracle/angrylion-control/`
([[../../specs/BT/angrylion-control-oracle]]); TooieRecomp stays RT64.
Evidência:
[[../../plan/evidence/p08-user-demo1-movement-and-fire-egg-differential]],
[[../../plan/evidence/p08-user-demo7-cutscene-drift-investigation]],
[[../../plan/evidence/p08-user-demo4-sincos-semantics-fix]],
[[../../plan/evidence/p08-user-demo4-post-round-full-state-discriminator]],
[[../../plan/evidence/p08-user-demo4-walkstart-ncbapivot-discriminator]],
[[../../plan/evidence/p08-demo-movement-precision-diagnosis]],
[[../../plan/evidence/p08-stick-angle-thunk-fix]],
[[../../plan/evidence/p08-stick-angle-thunk-fix-rereview]],
[[../../plan/evidence/p08-stick-angle-thunk-fix-independent-review]],
[[../../plan/evidence/p08-player-direction-facing-diagnosis]],
[[../../plan/evidence/p08-directional-input-movement-diagnosis]],
[[../../plan/evidence/p08-visible-demo-movement-diagnosis]],
[[../../plan/evidence/p08-core1-ra-wrapper-fix]],
[[../../plan/evidence/p08-80013a7c-lookup-zero-diagnosis]],
[[../../plan/evidence/p08-bcf28-binding-fix-independent-review]],
[[../../plan/evidence/p08-title-file-select-validation]],
[[../../plan/evidence/p08-original-timing-4x3-baseline]],
[[../../plan/evidence/p08-bcf28-crash-diagnosis]],
[[../../plan/evidence/p08-preemption-patch4-clean-build-independent-review]],
[[../../plan/evidence/p08-preemption-patch4-clean-build-correction]],
[[../../plan/evidence/p08-preemption-patch4-independent-review]],
[[../../plan/evidence/p08-preemption-patch4-validation]],
[[../../plan/evidence/p08-preemption-patch35-rcp-independent-review]],
[[../../plan/evidence/p08-preemption-patch35-rcp-correction]],
[[../../plan/evidence/p08-preemption-patch35-independent-review]],
[[../../plan/evidence/p08-preemption-patch35-design]],
[[../../plan/evidence/p08-preemption-patch3-independent-review]],
[[../../plan/evidence/p08-preemption-patch3-validation]],
[[../../plan/evidence/p08-preemption-patch2-independent-review]],
[[../../plan/evidence/p08-preemption-patch2-validation]],
[[../../plan/evidence/p08-preemption-patch1-independent-review]],
[[../../plan/evidence/p08-preemption-implementation-plan]],
[[../../plan/evidence/p08-preemption-design-adversarial-review]],
[[../../plan/evidence/p08-guest-preemption-causal-validation]],
[[../../plan/evidence/p08-guest-preemption-runtime-audit]],
[[../../plan/evidence/p08-slot7-scheduled-event-validation]],
[[../../plan/evidence/p08-slot7-scheduled-event-driver]],
[[../../plan/evidence/p08-slot7-object-lifecycle-arbitration]],
[[../../plan/evidence/p08-demo-zbuffer-oracle-sol-review]],
[[../../plan/evidence/p08-session-handoff]]. Detalhe:
[[08-rt64-input-and-frontend]].

### Gate P08/P09 — evidence-scope correction (2026-08-20)

- **P08**: `ACTIVE / MANUAL_VALIDATION_PENDING`. The user controller observation
  is retained as `USER_REPORTED`; formal 15/15 gamepad and 14-item display
  checklists are not proven.
- **P09**: `PENDING / NOT_FORMALLY_OPEN`.
- **Open attract residuals**: USER_DEMO_1 and USER_DEMO_5 are user-reproduced
  movement failures; see [[../../plan/evidence/p08-attract-movement-residuals]].
- The Demo 0 parity result is `PROVISIONAL_PRE_P09_PARITY_EVIDENCE`; Demo 1,
  Demo 2, and Demo 3 remain `UNTESTED`.

### Nota P09 (2026-10-20/21)

Investigação causal de interpolação articulada (iteração 3): Mecanismo A (amplificação angular por cosseno antes da tolerância de velocidade) provado no fonte (`rt64_rigid_body.cpp:30-34`); correção no owning layer via `lerpTranslationSpeedOnly`. Gate independente (`resumed-gate.json`): `passed: false` (bloqueadores: compilação sem log salvo, crash `ong2` com SIGSEGV -11 não explicado, árvore RT64 com diagnósticos preexistentes, ausência de teste em objetos não-jogadores e sem gameplay a 60 FPS). P09 formal permanece **`PENDING / NOT_FORMALLY_OPEN`**; caminho estável CURRENT/30 mantido estritamente como padrão. Detalhe: [[09-gameplay-parity-and-enhancements]].

### Nota P10 (2026-10-21)

Auditoria de clonagem limpa e empacotamento para Windows 11 x64 e Linux: (**SUPERSEDED 2026-10-21 pelo projeto standalone e pipeline kit-free**: `tools/deps.lock`/`bootstrap_deps.sh` aposentados; dependências vendorizadas em `lib/`/`decomp/`/`assets/`, ver [[plan/evidence/p10-standalone-vendor-20261021]] e [[plan/evidence/p10-kitfree-generation-and-ui-restoration-20261021]]) dependências vendorizadas no repositório como arquivos regulares; pacote Linux final verificado `TooieRecompiled-linux-x86_64-73bfd43-dirty-private-local.tar.gz` (SHA-256 `4f702d2ba0a0472345f6f1317bfe75eb0df462e96b2352d6d875edaa5c4ebcf9`, 104 arquivos regulares, verify 0, UI exata, zero ROM, preflight kit-free exit 0; binário `0cfcd792…` com rebuild da UI restaurada exit 0 e visual do launcher inspecionado com exit 0; supersedendo os interinos `b6475879…`, `64f130aa…` e `a327666f…`). Prova limpa kit-free (neutral 5c2) separada do build da UI restaurada. Windows 11 x64 classificado como `WINDOWS_BUILD_PENDING_NATIVE_HOST` (sem binário PE do jogo; TUs cross-host falharam em limites de cabeçalhos de terceiros; configure WIN32 passou; requer host nativo com VS 2022 Build Tools [clang-cl] e Windows 11 SDK; suporte a transferência direta de código gerado sem WSL2). P10 e P09 seguem **`pending / abertos`**; o gate técnico de interpolação permanece FAILED. Detalhe: [[10-linux-packaging-and-release]] e [[../../plan/evidence/p10-kitfree-generation-and-ui-restoration-20261021]].
