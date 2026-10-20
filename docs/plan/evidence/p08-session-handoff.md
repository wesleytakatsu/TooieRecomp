---
title: "P08 — session handoff: attract demo e render submission"
description: "CHECKPOINT PUBLISHED & HOST LAUNCHER WORDMARK COMPLETE: Prior checkpoint published on origin/main as 2c52af0c (199 paths). Host launcher wordmark replaced by Tooie-owned two-line SVG logo (ui_assets/TooieLogo.svg, viewBox 0 0 7300 2450) and CMake isolated copy rule to build/ui_assets preserving shared BanjoRecomp assets (tracked, clean). Incremental build ffe3d7a5 exit 0. Claude Haiku 5.5 medium exit 0 visual QA confirmed 1600x900 and 1024x768 settled layouts; root visually inspected ONLY frame_1.png (1600x900). Animation addendum bounded PASS on entry composition across 5 discrete frames (smoothness/easing NOT_TESTED). Pre-existing local tool/config state preserved. Ongoing parity NEXT_SINGLE_ACTION: extend oracle comparison to another attract demo when user requests next game adjustment."
status: evidence
phase: P08
updated: 2026-10-19
tags: [tooie-recomp, p08, p09, checkpoint-published, launcher, wordmark, logo, ui_assets, visual-verification, rt64, patches, replay, clean-pin, getter-gate, handoff, zbuffer, oracle, angrylion, cxd4, lle, r-oracle, r-motion-oracle, rt64, intro, attract, demo1, demo2, visible-demo, demo7, drift, fire-egg, projectile, sweep-branch, target-prefix, first-writer, updrec, custom-hits, liveness, movement-parity, generator-fix, counterfactual, object-frustum, aspect-aware, causality, rng-consumers, grok-implementation]
---
# P08 — session handoff: ATTRACT DEMO & RENDER SUBMISSION

## Áudio tic tic — fix de fila e validação de intro (2026-10-19)

- **Contexto**: Correção do tempo de fila SDL na intro ([[plan/evidence/p07-audio-tic-tic-investigation-20261019]]).
- **Commit HEAD**: `5af4510744ddc90d25f9223ab48e9cff76f5baca` | Status: `USER_CONFIRMED_FOR_TESTED_INTRO_RECORDING`.
- **Fix de Produção (Default)**: `main.cpp` divide bytes SDL por taxa e canais host (48 kHz float32). Sem flag de teste.
- **Diagnóstico (Default OFF)**: `TOOIE_AUDIO_CAPTURE_DIR` com worker pós-janela e caps rigorosos (24 MiB / 2400 chunks).
- **Evidência Runtime**: Pré-fix apresentava 47 skips espúrios (filas reais de 46–59 ms). Pós-fix obteve 0/579 skips e pre/post idênticos.
- **Validação do Usuário**: Usuário escutou `intro-queue-time-fix.wav` (33s Pulse) e atestou que o ruído "Desapareceu".
- **Rastreabilidade**: REQ-AUD-05 parcialmente validado para intro; P07-T06 parcial; P07-T07 PENDING/UNTESTED.
- **NEXT_SINGLE_ACTION**: QA focado de áudio em gameplay real (SFX/voz/pausa) via `./build/TooieRecompiled` padrão.

## CURRENT (2026-10-19) — HOST_LAUNCHER_TOOIE_WORDMARK_AND_CHECKPOINT_PUBLICATION_CONFIRMED

Read [[menu-title-tooie-wordmark-20261019]], [[checkpoint-20261019-review-and-patch-replay]], and [[p08-attract-movement-residuals]]. Details in canonical evidence.

- **Checkpoint 2c52af0c Published (PROVEN)**: Published exact on `origin/main` (199 paths, exit 0, `checkpoint-publication-proof.json`).
- **Launcher Wordmark & Asset Isolation (PROVEN)**: Two-line vector logo ("BANJO-TOOIE" blue/yellow, "RECOMPILED" red/yellow) via `tools/make_tooie_launcher_logo.py` (`d13cb8d9...`) into `ui_assets/TooieLogo.svg` (`036d3cb2...`). CMake target `TooieLauncherLogo` copies to `build/ui_assets/`; `BanjoRecomp/assets/Logo.svg` remains unchanged and tracked (clean).
- **Build (PROVEN)**: Incremental build `cmake --build build --target TooieRecompiled -j2` exit 0 (NOT clean build), binary `build/TooieRecompiled` (`ffe3d7a5...`).
- **Settled Visual QA (VISUALLY_CONFIRMED)**: Claude Haiku 5.5 medium (exit 0) confirmed 1600x900 and 1024x768 4:3 settled layouts (legible, no clipping). Root visually inspected ONLY `frame_1.png` (1600x900); no root 4:3 claim.
- **Entry Animation Follow-up (BOUNDED_PASS)**: Claude Haiku 5.5 medium (exit 0, `menu-title-haiku-animation-addendum.txt`) viewed 5 discrete frames at 0.28/0.79/1.42/2.57/5.87s (`t_0_2s.png`..`t_5s.png`). Logo entry composition PASS; smoothness/easing NOT_TESTED (discrete frames); no gameplay QA.
- **Repository & Runtime State**: Wordmark task uncommitted; pre-existing local tool/config state preserved; gameplay invariants intact.
- **Next Single Action**: `extend oracle comparison to another attract demo when user requests next game adjustment`.

## Checkpoint publicado (2c52af0c) (2026-10-19) — RT64_CLEAN_PIN_REPLAY_AND_GETTER_GATE_RESOLVED

Read [[checkpoint-20261019-review-and-patch-replay]], [[p08-user-demo1-movement-and-fire-egg-differential]], [[p08-user-demo7-cutscene-drift-investigation]], [[p08-demo-movement-precision-diagnosis]], and [[p08-attract-movement-residuals]].
Checkpoint published to origin/main as commit 2c52af0c7d8287d9003db839ffbe636d585e900a (199 paths, verified by checkpoint-publication-proof.json). Pre-commit review completed with both Grok 4.7 Medium sessions ending with `end_turn`; findings P1 and P2 resolved.

- **P1 Resolution — RT64 Clean-Pin Patch Replay & Applicator Chain (PROVEN)**:
  - Initial review identified that older incremental RT64 patches (`rt64-present-rate-decimator.patch`, `rt64-present-success-counter.patch`, `p09-exact-use-pathology-correlation.patch`) were based on the dirty worktree and failed `git apply --check` (exit 1) on clean RT64 pin `6f1c2d99a4ea571c139f449c326fd176ba8f3496`.
  - Replaced for clean pin apply by 3 new non-overlapping patches in `patches/rt64/` against base `6f1c2d99`:
    - `patches/rt64/rt64-rsp-endpoint-current.patch` (SHA256 `49925480...`, 16,629 bytes): production CURRENT `tfPos` snap and post-snap fog in `RSPProcessCS.hlsl`, push constant expansion, and CPU data path.
    - `patches/rt64/rt64-present-fps-decimator-and-counter.patch` (SHA256 `6d746f27...`, 5,415 bytes): cumulative presentation decimator (`rt64_present_decimator.h`, `rt64_present_stats.h`), `viOriginalRate` write protection under `configurationMutex`, and `presentSuccessCount` increment.
    - `patches/rt64/rt64-p09-exact-use-correlation-header.patch` (SHA256 `0d4c250a...`, 43,083 bytes): diagnostic correlation header required by host unit test.
    - `patches/frontend/recompfrontend-overlay-update-callback.patch` (SHA256 `37266d0b...`, 2,732 bytes, pin `d0d90ba`): RecompFrontend overlay update hook.
  - `tools/apply_runtime_patches.sh` registered with the 3 RT64 clean-pin patches and frontend patch (+49 insertions, SHA256 `87a6455f...`; `bash -n` exit 0, `git diff --check` exit 0). Applicator script was NOT run against live checkouts to protect uncommitted diagnostic state.
  - Scratch replay on isolated clone `rt64-scratch-apply`: reverse `--check` exit 0; 14 covered paths verified (6 byte-identical to live, 8 differ strictly due to excluded diagnostics).
  - Standalone tests compiled and passed against scratch: `rt64_present_decimator_test` PASS exit 0; `p09_exact_use_correlation_test` 1354/1354 PASS exit 0; `p09_cpu_shader_clip_semantics` 14/14 PASS exit 0.
  - Old incremental patches remain in tree for historical provenance, but are SUPERSEDED for clean-pin apply. Pathology C++ hook integration was explicitly NOT reproduced from clean pin; not all diagnostics are claimed reproducible from pin.
- **P2 Resolution — Model-Getter Diagnostic Gate Fix (PROVEN)**:
  - In `src/game/backpack_lookup_diag.cpp`, `__wrap_func_80092B04` unconditionally called `v7_note_model_getter`, which did guest memory reads, atomic counter increments, and acquired `g_secondary_sway_mu` to write `g_durable_bins` on default gameplay.
  - Gated in `include/tooie_diag.h` and `src/game/tooie_diag.cpp`: `v7_note_model_getter` returns early when `!g_getter_durable_needed` (evaluated in `ensure_init()` across consumers requesting durable metadata: `TOOIE_DIAG_SECONDARY_SWAY`, `TOOIE_DIAG_FEATHER_HANDOFF_V7`, `TOOIE_DIAG_FEATHER_POSTPROCESS_Q`, temporal coherency, `TOOIE_DIAG_RSP_ENDPOINT_BINDING`, and strict `TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION`).
  - Validated by host test `tests/getter_diag_gate_test.cpp` PASS exit 0.
  - Rebuilt binary `build/TooieRecompiled` SHA256 `74a0cea04a2bc3704860388673c40b2e73ee34d284f3a2afa0faebe91a9c05f5` (size: 29,418,200 bytes, exit 0).
  - **Binary hash distinction**: Rebuilt binary `74a0cea0...` is distinct from prior user-tested candidate binary A6A2 (SHA256 `a6a2c023...`, size: 29,413,928 bytes). User visual confirmation ("Sim, continua funcionando") was conducted on A6A2; do NOT claim user visual confirmation for `74a0cea0...`.
- **Preserved State & Invariants**:
  - Aspect-aware widescreen culling remains default ON (`TOOIE_TEST_MAP_VIS_ASPECT_AWARE=1`, `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1`). Narrow allowlist: pure draw `func_800B97A4` (kind 3) + preserved `func_8011447C` (kind 2); mixed renderer `func_800DE498` stays on original unexpanded `v0`.
  - Widescreen object edge pop fix is visually confirmed for the tested area on candidate binary A6A2 (`VISUALLY_CONFIRMED_FOR_TESTED_AREA`, `USER_REPORTED`). Broader maps/objects/movement parity remains an untested residual.
  - USER_DEMO1 trajectory (355 samples), actor records (546 words), LCG RNG snapshots (14), and Fire Egg sweep hit (`v0=0x80132f60`, `func_800CD6B4=1`) match original bit-exact.
  - Demo 7 cutscene stop fix preserved (`n64recomp-zero-size-static.patch`).
  - CURRENT `tfPos` workaround default ON (palliative); intermediate frames repeat vertex poses above native; continuous interpolation remains `NOT_IMPLEMENTED`.
  - Audio glitch remains deferred; 8-preset UI selector remains `PLANNED` / `NOT_IMPLEMENTED`; formal P09 phase remains `PENDING / NOT_FORMALLY_OPEN`.
- **Operational Boundaries / What Was Not Run**:
  - No game launch or termination during review; user may have game running.
  - Native A6A2 gate not repeated; cited from cache.
  - Applicator script not executed against live trees.
  - Root owns Git stage, commit, and push; Git commit will establish publication truth.

```text
PRECOMMIT_REVIEW_STATUS: PASSED_WITH_FINDINGS_RESOLVED (P1 and P2 resolved; clean-pin replay verified; automated gates exit 0)
P1_RT64_CLEAN_PIN_CHAIN: RESOLVED (3 new patches + frontend patch in tools/apply_runtime_patches.sh; old incremental patches SUPERSEDED)
P2_GETTER_DIAG_GATE: RESOLVED (v7_note_model_getter gated by g_getter_durable_needed; getter_diag_gate_test PASS; binary 74a0cea0 exit 0)
BINARY_HASH_DISTINCTION: PROVEN (latest build 74a0cea0 distinct from user-tested candidate A6A2 a6a2c023; user visual confirmation applies to A6A2 only)
WIDESCREEN_EDGE_QA_STATUS: VISUALLY_CONFIRMED_FOR_TESTED_AREA (USER_REPORTED on candidate A6A2; broader game parity untested residual)
PRESERVED_STATE: Object visual pop fix default ON, Demo 7 generator fix, CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: extend oracle comparison to another attract demo when user requests next game adjustment
```

## Prior (2026-10-19) — USER_DEMO1_FRUSTUM_PARITY_LANDED_AND_USER_VISUALLY_CONFIRMED_FOR_TESTED_AREA

Read [[p08-user-demo1-movement-and-fire-egg-differential]], [[p08-user-demo7-cutscene-drift-investigation]], [[p08-demo-movement-precision-diagnosis]], and [[p08-attract-movement-residuals]].
Grok 4.7 Medium implementation session `01a15118-c738-7dd3-98ed-cc5db5021bb5` completed `end_turn` (38 turns; root observed CLI exit 0). The narrow fix landed in candidate binary A6A2 (SHA256 `a6a2c023a9e3b0688cac15249cda61b5c8b8ccdf68c7c40f91d8c512356bef5e`).
Source edits owned in `src/game/boot_hw_stubs.cpp`, `src/game/tooie_demo1_projectile.h`, and `tests/tooie_demo1_upd_layout_test.cpp`; `RecompiledFuncs` untouched, no generated hand-edits.

- **Scope Boundary & Allowlist Architecture (PROVEN)**:
  - `tooie_wrap_func_800E3E8C` no longer opens a `TooieCacecDrawScope` (shared broad DRAW scope dropped; nested CACEC only expands if an outer scope is active).
  - Pure draw caller `func_800B97A4` registered via `tooie_wrap_func_800B97A4` with scope kind 3 when `tooie_cacec_pure_draw_vram(kCacecPureDrawB97A4)` is true (guest VRAM `0x800B97A4`; accept path emits G_MTX / G_DL / G_MOVEMEM and matrix calls without RNG/simulation side effects).
  - Existing `tooie_wrap_func_8011447C` scope kind 2 preserved for object visual pop culling.
  - Mixed renderer `func_800DE498` (which stores E3E8C `v0` to `0x8012C940` and on accept executes callbacks and writes renderer context) stays on original `v0` because no scope wraps it.
  - Unclassified callers stay on original `v0`. No default flag OFF, no RNG save/restore, no host-PC/RA production gates.
- **Production Default-ON Parity Verification (PROVEN)**:
  - Feature remains production default ON (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1`). Both fresh default-ON captures completed exit 0 with matching verification.
  - **TARGET Capture** (`demo1-sep-target-current/target.csv`, SHA256 `016a102c92f279aef896bcab0802be27b1671adab06f8bc374bf227573b398ec`, owned PID 1776728, 66.084s, exit 0):
    - All 355 paired trajectory samples `w0..w11` (`tgtrec`) match original flag-OFF / oracle2 bit-exact (0 mismatches; old buggy default-ON diverged post 83).
    - All 546 ActorRecord words (`updrec` 39 words x 14 pre/post snapshots across posts 80..86) match original bit-exact.
    - All 14 raw LCG RNG snapshots (`a2`) match original bit-exact (resolving PRE82 RNG split).
    - All 14 `+0x60` words match original `0003006c` (was `0004006c` under buggy default-ON).
  - **PROJECTILE Capture** (`demo1-sep-projectile-current/projectile.csv`, SHA256 `570f42e0ad8d31a883c2345afc5e0b000a3fde22baa7edc7722ab35b031d1531`, owned PID 1774174, 67.274s, exit 0):
    - Post 355 sweep `func_800CD820` leave returns `v0 = 0x80132f60` (hit) and narrowphase `func_800CD6B4` leave returns `1`, exactly matching original hit (old buggy default-ON missed with `v0 = 0`).
    - 22 sweep ENTER pairs and 21 `sueggmove9` ENTER pairs match original in 6 geometry words and gamespeed `gs`.
  - **Build & Tests**: Ninja exit 0 (up-to-date); all unit test binaries (`upd_layout`, `target_selector`, `projectile`, `parity`, `demo7_drift`, `mips_rounding`, `coverage`) exit 0. Test `tooie_demo1_upd_layout_test` verifies `pure_draw_accepts=1` in harness. Unit test models old policy vs new policy; no actual old binary unit test execution exit 1 is claimed.
- **Epistemic Boundaries & Status**:
  - Positive draw accept exists in unit harness; runtime TARGET window recorded `cacecx = 0` (expected when mixed DE498 scope is dropped; runtime cacecx is NOT promoted from 0).
  - Graphical QA of widescreen object-edge pop previously `NOT_TESTED` is now superseded by direct user visual confirmation: user launched `./build/TooieRecompiled` normally without flags in a previously affected gameplay area and confirmed: *"Sim, continua funcionando"*.
  - Classification: **`VISUALLY_CONFIRMED_FOR_TESTED_AREA`**, **`USER_REPORTED`**.
  - Epistemic scope: This confirms preservation of the observed visual expansion in the gameplay area tested by the user. Do NOT claim all maps, all objects, or global movement parity proven. Broader game parity remains an untested residual.
- **User Direction & Next Step**:
  - User explicitly chose KEEP VISUAL EXPANSION ACTIVE (feature stays default ON; candidate binary A6A2).
  - User visual test closes QA for the tested area. No new action needed on the bounded Demo 1 fix. User may have the game open; do not touch running processes.
  - Invariants maintained: P09 Phase 1 default CURRENT `tfPos` workaround remains default-ON (palliative); FPS counter/toggle and RT64 decimator unchanged; audio glitch deferral maintained.

```text
SCENE_IDENTITY: PROVEN (stage 0, sub 0, map 335 entrance 8 mode 5, glrecord ovl 698 cnt 766)
SCOPE_BOUNDARY_FIX: PROVEN (shared E3E8C broad scope dropped; allowlist pure-draw B97A4 kind 3 + preserved 8011447C kind 2; DE498 original v0)
DEFAULT_ON_TARGET_PARITY: PROVEN (355x12 trajectory words, 546 record words, 14 RNG snapshots, +0x60=0003006c exact original under default-ON binary A6A2)
DEFAULT_ON_PROJECTILE_PARITY: PROVEN (post 355 Fire Egg sweep hit v0=0x80132f60, CD6B4=1, 22 sweep + 21 move pairs exact under default-ON binary A6A2)
BUILD_AND_TEST_STATUS: PASS (ninja exit 0; layout/selector/projectile/parity/demo7/mips/coverage exit 0; pure_draw_accepts=1 in harness)
WIDESCREEN_EDGE_QA_STATUS: VISUALLY_CONFIRMED_FOR_TESTED_AREA (USER_REPORTED: "Sim, continua funcionando" on normal noflags launch; cacecx=0 in target window unpromoted; broader game/objects untested residual)
USER_CHOICE: RESOLVED (KEEP EXPANSION VISUAL ACTIVE; feature stays default ON; user visual QA completed for tested area)
PRESERVED_STATE: Object visual pop fix default ON, Demo 7 generator fix, CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: extend oracle comparison to another attract demo when continuing broader fidelity
```

## Prior (2026-10-19) — USER_DEMO1_SOURCE_PROVEN_CALL_CHAIN_AND_GROK_ROUND2 (SUPERSEDED BY LANDED FIX)

Read [[p08-user-demo1-movement-and-fire-egg-differential]], [[p08-user-demo7-cutscene-drift-investigation]], [[p08-demo-movement-precision-diagnosis]], and [[p08-attract-movement-residuals]].
Prior single-feature counterfactual causality documentation remains SUCCESS and canonical (`demo1-frustum-causality-checkpoint-gemini.json`): setting `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0` as an experimental per-process flag restored 355 NPC prefix words, all 546 ActorRecord words, 14 RNG snapshots, and post 355 Fire Egg sweep hit (`v0 = 0x80132f60`, narrowphase `func_800CD6B4` LEAVE = 1).
User confirmed Demo 7 cutscene stopping is **VISUALLY FIXED** and Fire Egg miss persists on current default production binary (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1`). User reaffirmed objective is general subtle gameplay/action parity across movement, kinematics, and state transitions, not an artificial aim or steering patch; no global clock hack.
**User Choice Resolved**: KEEP EXPANSION VISUAL ACTIVE while proper fix; **NO temporary OFF default**. Setting flag OFF (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0`) is strictly an experimental per-process counterfactual diagnostic; it is NOT a production mitigation. Production invariants preserved: shader CURRENT `tfPos` workaround remains default-ON (palliative); FPS counter/toggle and RT64 decimator unchanged; audio glitch deferral maintained.

- **Source Findings & Gating Architecture (`PROVEN`)**:
  - Broadly treating `func_800E3E8C` (`E3E8C`) as DRAW scope allows aspect-expanded accept (`v0 = 1`) inside mixed renderer routine `func_800DE498` (`0x800DE6EC`).
  - `func_800DE498` stores `v0` to `0x8012C940`. If nonzero (accepted), it takes the visible path: executes registered pre-draw callback `0x800DE738: jalr $v0` with `$a0` loaded from `0x8012C9AC`, calls asset resolver `800D674C`, stores renderer processing context `s0+0x14/0x18/0x1C`, and emits display lists. If culled (`v0 == 0`), it branches to `0x800DE718`, calls cleanup helper `func_800DE318`, and returns 0.
  - **Renderer Context vs. Actor State**: Stores to `s0+0x14`, `s0+0x18`, and `s0+0x1C` are writes to the **renderer processing context** (`s0`), NOT actor state writes (`PROVEN` distinction; do not conflate).
  - **Registration Chain**: Standard actor draw entry `func_80101870` -> `func_80101808` calls `func_800DF47C(func_801015D0, arg0)`, registering callback `func_801015D0` at `0x8012C9A8` and `Actor*` at `0x8012C9AC`, then enters `func_800DE448` -> `func_800DE498`.
  - **Pre-Draw Dispatch**: Callback `func_801015D0` calls `func_80101934(Actor*)` at `0x80101784`, which indexes jump table `D_801243E8` via `(actor->unk74 & 0xF)`. Table entry `+0x04` invokes `func_80101A50(Actor*)`.
  - **RNG Stepping and Actor Mutation**: `func_80101A50` calls `func_800DC0C0` at `0x80101AA0` (RA `0x80101AA8`), advancing global LCG RNG seed `D_8012C7F0`. It tests random float `$f0 < D_80126434` (`0.03f`). On match, it directly mutates guest actor state byte `sb $t2, 0x7A($a2)` (setting bit `0x10` or `0x20` on `Actor->unk7A`). Both arms call `func_800DF744(1, ...)` and `func_800DF744(2, ...)`, writing to global renderer selector array `D_8012C850`.
  - **Epistemic Classification**: Semantic identity as facial/eye-blink animation is **`STRONG_INFERENCE`** (suggested by 3% probability check and selectors 1/2, but without explicit asset/animation binding proof in source). Control flow gating, global LCG RNG stepping, actor byte `0x7A` mutation, and renderer array writes are strictly **`PROVEN`**.
- **Paired Native Telemetry Arbitration (`PROVEN`)**:
  - Telemetry (`demo1-rng-paired-native-chain-gate.json`): Current has 32 RNG steps vs Oracle 27 across posts 79..84.
  - Sequence step gap grows monotonically across posts: Post 79: 0 (6 vs 6); Post 80: 0 (4 vs 4); Post 81: +1 (7 vs 6; **first extra step**); Post 82: +1 (5 vs 4); Post 83: +2 (6 vs 4); Post 84: +1 (4 vs 3). Cumulative delta = +5 steps across window.
  - LCG chain exhibits **ZERO breaks** across all observed steps and snapshots (`breaks: []`). Every transition satisfies $s' = (1664525 \times s + 1013904223) \pmod{2^{32}}$.
  - Expected oracle after-states, once calculated, never match actual observed stores once desynchronized.
  - First extra RNG call is definitively the **SECOND `func_80101A50` call in post 81 (seq 146 in Current, before `6239b638` -> after `036f8437`)**, NOT global `func_8001608C` (which executes in both runtimes).
  - Preceded directly by `cacecx` extra accept owner `func_800DE498` at seq 142 on position `(-2818, 300, -2237)`.
  - Later differences are a deterministic chain shift, not independent new consumers.
- **Native PC Mapper & Symbol Resolution (`PROVEN`)**:
  - Old `T/t`-only cache (`demo1-rng-ee14-nm.json`) was **INVALID and incomplete** (omitted recompiled weak `W/w` functions).
  - Completed `nm T t W w` (`demo1-rng-ee14-origin-resolution.json`) validated binary `EE14` (`ee14d6cb1c144a09ccec7f393dc2e986ac26fb0826893d2e0d490df9d19a683c`) with positive function sizes.
  - Return address is guest original `0x80101AA8` (`jal func_800DC0C0` at `0x80101AA0`).
- **Critical Operational & Quota Update**:
  - **NO SONNET**: Sonnet quota is expired. All further Sonnet calls are strictly forbidden.
  - **Haiku**: Approaching quota limit; reserved/unused now.
  - **Grok 4.7 Medium**: Designated **SOLE** code writer and test executor. Resumed implementation session `01a15118-c738-7dd3-98ed-cc5db5021bb5`: round 1 cap 50 exit 1 with source proof and no edit (`demo1-frustum-parity-grok-implementation-report.txt`); round 2 active budget 70 (`demo1-frustum-parity-grok-implementation-round2.json`, completed `end_turn` exit 0).
  - **Gemini**: Designated for **ALL** Markdown and documentation maintenance.
  - **Native Agent `demo7_capture`**: Read-only Grok review only; NO other capture authority.
  - **Opus**: DONE (read-only existing report `demo1-rng-consumer-opus.json`, not active job).
  - Current code EE14 diagnostics is NOT a fix; original default vis ON CURRENT palliative / FPS / audio unchanged.
- **Main Implementation Objective**:
  - Correct shared `func_800E3E8C` consumer selection to separate genuine visual DRAW passes from logic/simulation callbacks.
  - Preserve existing `func_8011447C` unless primary simulation harm proof is established (guest outparam write alone is NOT sim evidence).
  - Allowed pure DRAW writes: Gfx buffers, render-only temps.
  - `+0x60` semantic remains strictly UNKNOWN and is not necessary for this cause.

```text
SCENE_IDENTITY: PROVEN (stage 0, sub 0, map 335 entrance 8 mode 5, glrecord ovl 698 cnt 766)
SINGLE_FEATURE_CAUSALITY: PROVEN (TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0 restores 355x12 prefix, 39x14 record words, 14 RNG snapshots, and post 355 Fire Egg hit)
SOURCE_MECHANISM: PROVEN (shared E3E8C broad DRAW scope enables mixed DE498 gate; registered callback 801015D0 -> 80101934 -> D_801243E8 -> 80101A50 steps global LCG RNG D_8012C7F0, mutates actor byte 0x7A, writes D_8012C850 selectors via DF744)
SEMANTIC_CLASSIFICATION: facial/eye-blink animation is STRONG_INFERENCE; control flow, RNG step, byte 0x7A mutation, renderer writes are PROVEN
FIRST_EXTRA_RNG_CALL: PROVEN (second func_80101A50 in post 81 seq 146 curr, preceded by cacecx extra accept in func_800DE498 seq 142; NOT global 8001608C)
PAIRED_RNG_GAP: PROVEN (grows 1..4 across posts, 32 curr vs 27 oracle in posts 79..84; zero breaks in chain; oracle after calculated never matches observed store)
NATIVE_PC_MAPPER: PROVEN (old T/t cache INVALID; completed nm T t W w validated binary EE14 with positive sizes; RA guest 80101aa8)
USER_CHOICE: RESOLVED (KEEP EXPANSION VISUAL ACTIVE while proper fix; NO temp OFF default; env flag 0 is EXPERIMENT ONLY)
GROK_STATUS: COMPLETED (session 01a15118-c738-7dd3-98ed-cc5db5021bb5 completed end_turn exit 0, binary A6A2)
AGENT_ROLE_ROUTING: Grok PRIMARY code/tests; Gemini ALL Markdown; Haiku minimal/unused; Native demo7_capture read-only; Sonnet NO calls; Opus DONE
PRESERVED_STATE: Object visual pop fix default ON, Demo 7 generator fix, CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: user visual check of widescreen edges in known affected gameplay area, feature ON; if fails narrower remaining mixed-render separation requires next task
```


## Prior (2026-10-18) — USER_DEMO1_OBJECT_FRUSTUM_COUNTERFACTUAL_CAUSALITY_PROVEN

Read [[p08-user-demo1-movement-and-fire-egg-differential]], [[p08-user-demo7-cutscene-drift-investigation]], [[p08-demo-movement-precision-diagnosis]], and [[p08-attract-movement-residuals]].
User confirmed that Demo 7 cutscene stopping is **VISUALLY FIXED** and that the Fire Egg miss persists on the current corrected binary (SHA256 `87c5a08b96b6fbb9c29c0079ac6b7b6060af8f5da6b92147f60e5dacce85bbe5`). User reaffirmed the objective is **general subtle gameplay/action parity** across movement, physics, and state, not an artificial aim or hit steering adjustment; no global clock hack.
User requires keeping the object visual pop fix production default ON (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1`); shader CURRENT `tfPos` workaround remains default-ON (palliative); FPS counter/toggle and RT64 decimator unchanged; audio deferral maintained. Setting flag OFF (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0`) is strictly an experimental per-process counterfactual diagnostic; NO production source or config change has been made.

- **Single-Feature Counterfactual Causality (PROVEN)**:
  - On the unchanged binary (`c56ce4e17b554020c36eec14a9e48861693679e9426161031e10aef4527a67e2`) with the TARGET logger (`TOOIE_DIAG_DEMO1_TARGET=1`), setting ONLY `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0` (explicit-off getter at `boot_hw_stubs.cpp:4264..4267`, wrapper 4451 calling original `800CACEC` and returning before aspect-aware expansion; no other `TOOIE_TEST_*` flag combined; CSV `/home/wesley/.cache/tooie-fps-investigation/demo1-first-writer-current-object-original/target.csv`, SHA256 `1f43e7c570f21ec082bc7afac88cf01789cebbd8c2ec4e142f0aee9eb791f928`, 415 rows, 66.217s, exit 0/0, FINISH 357) restored:
    - All 355 `tgtrec` posts 2..356 x 12 raw words bit-exact with the original Angrylion oracle reference (`demo1-first-writer-oracle-2`).
    - All 14 pre/post snapshots across frames 80..86 x 39 ActorRecord words (546 words total) bit-exact with Oracle (`demo1-object-frustum-record39-comparison.json` is `[]` empty list).
    - Record `+0x60` high 16 bits: was `0004` (curr `0004006c`) under default aspect-aware ON vs original `0003` (`0003006c`); with flag OFF, it is now `0003` (`0003006c`), matching Oracle exactly.
    - All 14 raw RNG `D_8012C7F0` snapshots bit-exact with Oracle, completely resolving the PRE82 divergence ($k=4$ `036f8437` vs $k=5$ `c125942a`) and POST83 geometry divergence.
    - All 14 gamespeed `gs` words exact `0x3d4cccce`.
  - In comparison, default Current with aspect-aware ON had 274 posts (83..356) differing in position.
- **Actual Fire Egg Collision Restored (PROVEN)**:
  - In a dedicated PROJECTILE-only capture with the same flag OFF (`TOOIE_DIAG_DEMO1_PROJECTILE=1` and `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0`, controller 1600930 / game 1600931, exit 0/0, 66.824s, 313 rows, FINISH 391, CSV `/home/wesley/.cache/tooie-fps-investigation/demo1-projectile-current-object-original/projectile.csv`, SHA256 `3d54fe91ac1bc570a95a87241322f7d58593575aac951c4b4250d397a3344258`):
    - Actual Fire Egg sweep at post 355: `func_800CD820` sweep leave returns `v0 = 0x80132f60` (nonzero hit, matching Oracle `89a6bad1...`), whereas default Current returns `v0 = 0` (miss).
    - Narrowphase `func_800CD6B4` LEAVE = 1 on both off-current and Oracle; default Current has no such callback at 355.
    - All 22 sweep ENTER input pairs and 21 `sueggmove` ENTER input pairs are exact in `w0..w5` geometry and `w11` `gs`.
    - Sweep/move counts: 44 sweep / 42 move off/oracle vs default 64 / 62 (projectile stops at hit).
    - Bound centers at posts 354 and 355: all 6 selected `0x8008A018` LEAVE center triples exact with Oracle, including actual target post 355 `w0=c5af7c8d, w1=c401d0fa, w2=44a89e51` = `(-5615.56885, -519.265259, 1348.94739)`, completely restoring the prior ~427-unit bound center difference.
- **Causal Verdict & Epistemic Boundaries**:
  - `OBJECT_FRUSTUM_ASPECT_AWARE=1` is **CAUSAL** for observed RNG divergence, NPC trajectory divergence, and Fire Egg collision miss in USER_DEMO1.
  - The underlying call consumer and side-effects remain **UNKNOWN**.
  - Do NOT claim all NPCs globally fixed or physical fix done.
  - Do NOT globally REFUTE widescreen capability; update prior UNTESTED statement to PROVEN at feature level with mechanism UNKNOWN.
  - Source ABI and canonical getter roles from prior investigations are strictly preserved.
- **Background Workloads & Active Agents**:
  - Sonnet medium code job (`demo1-frustum-parity-fix-sonnet.json` / `report.txt`): RUNNING, code writer only, default acceptance capture ONLY after READY.
  - Grok field 60 old 30-turn audit exited with code 1 INCOMPLETE (`demo1-field60-and-rng-grok.json` and `.report.txt`), noted typo (Cpre82=Opost82) and found no field role; census stopped as feature-level causality is now established.
  - New main Grok READONLY job (`demo1-frustum-simulation-owner-grok.json` / `report.txt`): RUNNING bounded CACEC feedback; immutable baseline copy `demo1-frustum-default-baseline.cpp`.
  - Opus 5.5 medium READ-ONLY consumer design job (`demo1-rng-consumer-opus.json`): COMPLETED (exit 0, `is_error=false`, 14 turns, 242.8s). Proposed two entries (`func_800DC090` and `func_800DC0C0`) + writers (`800DC310`, `800DC324`, `800DC330`, `800DC354`), static coverage NOT gate tested. Opus used raw grep in violation of navigation policy, noted procedurally. Wide RNG trace proposal is now conditional, not automatically next because minimal counterfactual already isolated the feature.
- **Preserved Invariants**:
  - Demo 7 generator fix (`patches/runtime/n64recomp-zero-size-static.patch` and official 420-file regeneration) strictly preserved; user visual PASS maintained.
  - P09 Phase 1 default CURRENT `tfPos` workaround remains default-ON.
  - Present FPS counter and Graphics options toggle remain verified.
  - RT64 below-native present decimator remains operational.
  - Audio glitch remains explicitly deferred per user directive.

```text
SCENE_IDENTITY: PROVEN (stage 0, sub 0, map 335 entrance 8 mode 5, glrecord ovl 698 cnt 766)
SINGLE_FEATURE_CAUSALITY: PROVEN (TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0 restores 355x12 prefix, 39x14 record words, 14 RNG snapshots, and post 355 Fire Egg hit)
UNDERLYING_MECHANISM: UNKNOWN (exact call consumer / side-effects of 800CACEC aspect-aware expand in simulation still unisolated)
CURRENT_CF_TARGET_CSV: /home/wesley/.cache/tooie-fps-investigation/demo1-first-writer-current-object-original/target.csv (SHA256 1f43e7c5..., 415 rows, 56 updrec, FINISH 357)
CURRENT_CF_PROJECTILE_CSV: /home/wesley/.cache/tooie-fps-investigation/demo1-projectile-current-object-original/projectile.csv (SHA256 3d54fe91..., 313 rows, FINISH 391)
CF_TARGET_PREFIX_PARITY: 355/355_EXACT_MATCH (all 12 words bit-exact with Oracle 73c1faa...)
CF_RECORD_WORDS_PARITY: 546/546_EXACT_MATCH (14 snapshots x 39 words bit-exact; demo1-object-frustum-record39-comparison.json is [])
CF_RECORD_PLUS_60_STATUS: RESTORED_TO_ORACLE_0003006c (was 0004006c under default aspect-aware ON)
CF_RNG_D8012C7F0_STATUS: 14/14_EXACT_MATCH (all 14 snapshots bit-exact with Oracle; PRE82 split resolved)
CF_FIRE_EGG_HIT_STATUS: HIT_RESTORED_AT_POST355 (v0=0x80132f60, CD6B4=1, 22/21 sweep/move pairs exact, actual target center restored)
USER_DEMO7_STATUS: VISUALLY_FIXED (generator patch preserved)
USER_DEMO1_PRODUCTION_STATUS: USER_REPORTED_MISS (persists on production binary with default ON; OFF is experimental CF only)
WIDESCREEN_CAPABILITY_STATUS: NOT_GLOBALLY_REFUTED (prior UNTESTED statement updated to PROVEN at feature level with mechanism UNKNOWN)
ACTIVE_BACKGROUND_AGENTS: Sonnet job demo1-frustum-parity-fix-sonnet RUNNING; Grok job demo1-frustum-simulation-owner-grok RUNNING
COMPLETED_BACKGROUND_AGENTS: Opus job demo1-rng-consumer-opus DONE (exit 0, 14 turns; noted raw grep limit); Grok field60 exit 1 INCOMPLETE
PRESERVED_STATE: Object visual pop fix default ON, Demo 7 generator fix, CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: owning-layer separation of expanded DRAW from original simulation, preserving visible object fix (awaiting Sonnet code writer / Grok simulation owner analysis)
```

## Prior (2026-10-18) — USER_DEMO1_FIRST_WRITER_PAIRED_CAPTURE_AND_RECORD_FIELD60_RNG_ANALYSIS (SUPERSEDED BY COUNTERFACTUAL CAUSAL PROOF)

Read [[p08-user-demo1-movement-and-fire-egg-differential]], [[p08-user-demo7-cutscene-drift-investigation]], [[p08-demo-movement-precision-diagnosis]], and [[p08-attract-movement-residuals]].
User confirmed that Demo 7 cutscene stopping is **VISUALLY FIXED** and that the Fire Egg miss persists on the current corrected binary (SHA256 `87c5a08b96b6fbb9c29c0079ac6b7b6060af8f5da6b92147f60e5dacce85bbe5`). User reaffirmed the objective is **general subtle gameplay/action parity** across movement, physics, and state, not an artificial aim or hit steering adjustment; no global clock hack. While the historical geometric miss was cataloged in earlier recomp studies (BASE `876d299f`, §16/17, margin +36.85), the fresh causal root on the current binary is strictly **UNKNOWN**.

- Prior Prefix Parity & Critical VI Cadence Wording (PROVEN & INDEPENDENTLY CONFIRMED):
  - Prior 420 exact non-clock player/input/camera/gs/anim/phys match at EXACT PC `0x800151BC` ENTRY (consumed 1..420) independently confirmed by Haiku (`demo1-actual-pair-haiku-report.txt`, 0 field mismatches across 420 paired rows).
  - VI handler cadence difference observed (100/419 transitions, +40 oracle entries); causal role in NPC/mob timing UNKNOWN/OPEN HYPOTHESIS, player 420 exact does NOT refute NPC timing; `--nospeedlimit` is logical only, not real-time proof. No global clock fix claim.
- Grok First NPC Prefix Audit Completed (DONE):
  - Grok prefix audit finished cleanly (`demo1-first-npc-prefix-grok-report.txt` and `.json`, 7 turns, stopReason `end_turn`, CLI exit 0, replacing earlier pending status).
  - 355 `tgtrec` pairs (posts 2..356) joined on post. Trajectory handle `0x80132f30` (post 2) with callback word `w3 = 0x80085C98`.
  - Ambiguous markers `0x80132f00` (post 28) and `0x80132f60` (post 140 projectile target) separated; `0x80132f60` is NOT the sampled trajectory.
  - Position words `w0..w2` match posts 2..82 (81 pairs). First position diff occurs at post 83 (delta `0.207508441`).
  - Word `w10` first differs at post 121 (`04311038` vs `04311048`), `w9` at 171, `w5..w7` at 215. `w11` (`gs`) raw exact on all 355 pairs.
  - In-memory mutation test verified: flipping bit 0 of `w0` at post 40 shifted the first diff to post 40.
- Preceding First-Writer Diagnostic Failure & Fix (PROVEN):
  - Initial first-writer capture yielded 0 `updrec` rows (diagnostic FAIL).
  - Causes:
    1. Host RA unreliability in recomp: generated code for `func_800FF62C` calls `LOOKUP_FUNC(0x80100368)` without setting `ctx->r31` immediately before jal; guest RA is stale/caller-dependent on host; RA range guard unsound in recomp.
    2. MARKER vs RECORD arg confusion: `func_80100368(a0)` receives actor RECORD (`s0` from `801067C4`), but wrapper checked `a0 == s_d1t_marker` (MarkerHandle), which never matched (`accepted_real_call_arg = 0`).
    3. Post dump mistake: flush dumped 39 words at the MarkerHandle address instead of resolving the actor record.
  - Corrected default-OFF via Sonnet: resolved record dynamically via `upd_expected_record`, direct-call SP scope (`ctx->r29 == scope_entry_r29 - 0x58`), post flush re-resolves record from marker (or emits `postmiss`).
  - Regression fixture `tests/tooie_demo1_upd_layout_test.cpp` on `demo1_tgtrec_ident_{current,oracle}.csv` (355x2 rows): old-wrong build exits 1 (FAIL: 0/355 accepted); new build exits 0 (PASS: 355/355 accepted), covering 5 relocations on Current and 7 on Oracle.
- Independent Haiku ABI Gate (PROVEN):
  - `demo1-writer-record-abi-haiku-report.txt` confirmed `func_80100368` receives actor RECORD in a0 and calls `*(record+0x10)` unchanged; marker `80132f30` is invalid as a0.
  - Direct-call SP scope (`ctx->r29 == scope_entry_r29 - 0x58`) correctly isolates direct calls from `800FF62C` while excluding nested callee calls.
  - Haiku mapper criticism corrected to `NOT_TESTED`; independent generated source `80106790` maps `marker+0x1A >> 5` through `vector_at(*D_80136EE0)`.
- NEW Actual FIRSTWRITER 2 Paired Capture Telemetry (PROVEN):
  - Current: `/home/wesley/.cache/tooie-fps-investigation/demo1-first-writer-current-2/target.csv` (SHA256 `fbb2770bc1270762849aaf25f5dacefa4b18d04976ae8c3900618a6da652c75f`, controller 1584474, game 1584475, exit 0/0, 66.459s). Binary SHA256 `c56ce4e17b554020c36eec14a9e48861693679e9426161031e10aef4527a67e2`.
  - Oracle: `/home/wesley/.cache/tooie-fps-investigation/demo1-first-writer-oracle-2/target.csv` (SHA256 `73c1faaace84937324c565889580c620513498971495a663ad356da70f06313c`, controller 1585301, instance 1585302, exit 0/0, 101.350s). Core SHA256 `4b782df4545ecb84a1aeee69c8159f893cfb503c5b38da78efb630d4baf8e199`.
  - Both captures produced exactly 415 valid rows (exit 0/0), 56 `updrec` rows (posts 80..86 x pre/post x 4 chunks of 10/10/10/9 words; 39 words total).
  - Finish diagnostics: `w5 = 0` (invalid bindings), `w7 = 7` (predumps emitted), `w8 = 0` (postmiss rows).
  - Baseline invariance: all 355 `tgtrec` posts 2..356 x 12 words match pre-writer TARGET2 baseline bit-for-bit.
- Proven Comparisons & Geometry Findings:
  - **Record +0x60 First Diff**: Already differs at first available sample PRE80: Current `0004006c` vs Oracle `0003006c` (difference in upper halfword `0004` vs `0003`). Semantic meaning is strictly **UNKNOWN**; do not promote field role to state/timer/pointer/fault without owning source. All other 38 words are **BIT-EXACT** through PRE83 inclusive!
  - **RNG `D_8012C7F0` First Diff & Step Distances**: Matches PRE/POST 80 and 81 (`1cf1d2b5`, `c91d98c4`, `28f1cd53`). Recurrence confirmed from generated source: $s' = (1664525 \times s + 1013904223) \pmod{2^{32}}$. Shared POST81 = `28f1cd53`. First sampled diff PRE82: Oracle $k=4$ (`036f8437`) vs Current $k=5$ (`c125942a`). Step distance offset (Oracle $\to$ Current) grows: PRE82 $+1$, POST82 $+1$, PRE83 $+2$, POST83 $+3$, PRE84 $+4$. These are LCG sequence step distances, NOT proven actual call counts until other writes to `D_8012C7F0` are excluded.
  - **Gamespeed `gs`**: All 14 sampled `gs` w11 bits match `0x3d4cccce` identically across both runtimes.
  - **Geometry First Diff**: POST83: +4/+8/+C (norm `0.207508441`), plus RAW +48/+54 (+48 cur `438c677a` vs ora `438cbb8e`, +54 cur `4389fee4` vs ora `438cbb8e`), cause UNKNOWN. Pre83 positions are exact.
  - **Entity Identity**: Tracked stream is FIRST BOUND OTHER ACTOR `0x80132f30` (callback `0x80085C98`); actual projectile target `0x80132f60` remains separate.
  - **Post Bracket Nuance**: Post bracket spans to the next loop getter call or logic exit; it can contain downstream actor prep/work; NOT an isolated-update proof.
- Epistemic Corrections & Active Workload:
  - **Sonnet Widescreen Inference Correction**: Sonnet's report asserted that because divergence occurs in pre-draw simulation state, widescreen changes are refuted. That inference is **INVALID**: prior-frame draw/visibility feedback has not been excluded, therefore a pre-draw difference alone cannot refute the hypothesis; no assertion is made that feedback actually occurs here. Widescreen feedback remains an **UNTESTED HYPOTHESIS**, NOT REFUTED.
  - Suspect `C401C` audio still needs proof; no canonical promotion.
  - Completed and active background jobs: Grok field 60 30-turn audit exited with code 1 INCOMPLETE; Opus 5.5 consumer design completed exit 0; Sonnet medium code job running; new main Grok simulation owner job running.
- Preserved Invariants:
  - Demo 7 generator fix (`patches/runtime/n64recomp-zero-size-static.patch` and official 420-file regeneration) strictly preserved; user visual PASS maintained.
  - No production fix or parity claim for Demo 1.
  - P09 Phase 1 default CURRENT `tfPos` workaround remains default-ON.
  - Present FPS counter and Graphics options toggle remain verified.
  - RT64 below-native present decimator remains operational.
  - Audio glitch remains explicitly deferred per user directive.

```text
SCENE_IDENTITY: PROVEN (stage 0, sub 0, map 335 entrance 8 mode 5, glrecord ovl 698 cnt 766)
CURRENT_WRITER2_CSV_PATH: /home/wesley/.cache/tooie-fps-investigation/demo1-first-writer-current-2/target.csv (415 rows, 56 updrec, FINISH 357)
ORACLE_WRITER2_CSV_PATH: /home/wesley/.cache/tooie-fps-investigation/demo1-first-writer-oracle-2/target.csv (415 rows, 56 updrec, FINISH 357)
USER_DEMO7_STATUS: VISUALLY_FIXED (generator patch preserved)
USER_DEMO1_EGG_RESULT: USER_REPORTED_MISS (persists on user binary 87c5...)
USER_DEMO1_PREFIX_PARITY: 420/420_EXACT_MATCH (consumed 1..420 independently confirmed by Haiku)
FIRST_WRITER_DIAG_FIX: PROVEN (Sonnet host-RA / record-arg fix; 355x2 fixture old FAIL 1 / new PASS 0; covers relocations 5/7)
FIRST_WRITER_ABI_GATE: PROVEN (Haiku 80100368 record arg confirmed, direct SP scope confirmed; mapper criticism marked NOT_TESTED)
GROK_PREFIX_REPORT: DONE (end_turn 7 turns, CLI exit 0; replaces earlier pending; 355 tgtrec pairs, post 83 first pos diff)
WRITER2_CAPTURE_STATUS: PROVEN (valid 415/415 rows, exit 0/0, 56 updrec, invalid_binding=0, predumps=7, postmiss=0)
RECORD_PLUS_60_DIFF: PROVEN (current 0004006c vs orig 0003006c already PRE80; meaning UNKNOWN; other 38 words exact thru PRE83)
RNG_D8012C7F0_DIFF: PROVEN (POST81 same 28f1cd53; LCG 1664525x+1013904223u32; orig PRE82 k=4 / curr k=5; step offset PRE83 +2, POST83 +3, PRE84 +4; sequence distances NOT proven call counts)
GS_W11_STATUS: BIT_EXACT (all 14 sampled gs w11 bits match 3d4cccce)
GEOMETRY_FIRST_DIFF: POST83 (+4/+8/+C pos norm 0.207508441, RAW +48/+54; pre83 positions exact; cause UNKNOWN)
TRACKED_ACTOR_IDENTITY: FIRST BOUND OTHER ACTOR 80132f30 (callback 80085C98; actual shot candidate 80132f60 remains separate)
SONNET_WIDESCREEN_CLAIM: CORRECTED (predraw diff refuting widescreen was INVALID INFERENCE; prior-frame draw feedback UNTESTED not REFUTED)
ACTIVE_BACKGROUND_AGENTS: Grok job demo1-field60-and-rng-grok RUNNING; Opus job demo1-rng-consumer-opus RUNNING (both NOTPASS; no cause/fix)
PRESERVED_STATE: Demo 7 generator fix, CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: trace RNG consumers between POST81 and PRE82 (read-only design, no cause/fix)
```

## Prior (2026-10-18) — USER_DEMO1_PROJECTILE_DIAGNOSTIC_AND_LIVENESS_TRACKING (SUPERSEDED BY PAIR 2 & 3 SWEEP/TARGET DIAGNOSTICS)

Read [[p08-user-demo1-movement-and-fire-egg-differential]], [[p08-user-demo7-cutscene-drift-investigation]], [[p08-demo-movement-precision-diagnosis]], and [[p08-attract-movement-residuals]].
User confirmed that Demo 7 cutscene stopping is **VISUALLY FIXED** and that the Fire Egg miss persists on the current corrected binary (SHA256 `87c5a08b96b6fbb9c29c0079ac6b7b6060af8f5da6b92147f60e5dacce85bbe5`). User reaffirmed the objective is **general subtle gameplay/action parity** across movement, physics, and state, not an artificial aim or hit steering adjustment; no global clock hack. While the historical geometric miss was cataloged in earlier recomp studies (BASE `876d299f`, §16/17, margin +36.85), the fresh causal root on the current binary is strictly **UNKNOWN**.

- Prior Prefix Parity (PROVEN & INDEPENDENTLY CONFIRMED):
  - Prior 420 exact non-clock player/input/camera/gs/anim/phys match at EXACT PC `0x800151BC` ENTRY (consumed 1..420) independently confirmed by Haiku (`demo1-actual-pair-haiku-report.txt`, 0 field mismatches across 420 paired rows).
  - VI cadence delta (100/419 transitions) and +40 oracle handler entries are NOT proven root cause; possible NPC/timer cadence remains an OPEN HYPOTHESIS; `--nospeedlimit` is logical only, not wall time.
- Sonnet Projectile Diagnostic Implementation (PROVEN):
  - Sonnet session completed with `is_error=false` (report `demo1-projectile-diagnostic-sonnet-report.txt`, JSON `demo1-projectile-diagnostic-sonnet.json`). Build exit 0, all unit/regression tests reported 0.
  - Isolated default-OFF flags: `TOOIE_DIAG_DEMO1_PROJECTILE=1` (Tooie) and `P08_DEMO1_PROJECTILE=1` with `P08_DEMO1_PROJECTILE_CSV` (Angrylion oracle). No typos (exact flag names verified).
  - Five entry/return hook points: `_baeggsetup_entrypoint_5` @ `0x80084A60`, `_cheggfire_entrypoint_2` @ `0x800871F0`, `_sueggmove_entrypoint_9` @ `0x8008A480`, `_suegg_entrypoint_9` @ `0x8008A3C0`, and core2 sweep `func_800CD820`. Plus per-frame `objpoll` (37 columns) for overlay-relative callback tracking.
  - Binary SHA256: `af0723b4b7f802446b455401b1c9e83a50aa61772ec37a598aa451df7f666705`. Oracle core SHA256: `41b59e2802be5782a2f3ab4f685bbad2fee062bd4da72e8ee01c6d6c4b7ebe88`. Diagnostic ONLY; no production behavior fix.
- Haiku Static Gate Report (`demo1-projectile-haiku-gate-report.txt`):
  - Evaluated static code: ABI, exactly-once original call, ctx/reg preservation, 37-col schema, and default-OFF gate: PASS.
  - Coverage caveat: gate validates direct and `LOOKUP_FUNC` sites; indirect calls via overlay-relative descriptor/callback pointers (`0x808xxxxx`) are NOT guaranteed to be intercepted by Tooie wrappers (which is why `objpoll` exists). Missing callback alone is NOT proof of divergence.
- Owned Controlled Projectile Captures (PROVEN):
  - Current Tooie: Controller PID 1282788, game PID/pgid 1282791, elapsed 66.243s, exit 0/0, FINISH post 391, 189 data rows + 1 header. CSV `/home/wesley/.cache/tooie-fps-investigation/demo1-projectile-current/projectile.csv` (SHA256 `6b5686b77b723180f08954d3573be31d73294140b22815ae39f90725432775c7`). Flag ONLY `TOOIE_DIAG_DEMO1_PROJECTILE=1`; user game check passed. Counts: setup 1 pair, fire 1 pair (post 333), movement 31 pairs, sweep 32 pairs, objpoll 57.
  - Pinned Angrylion Oracle: Controller PID 1283584, game PID/pgid 1283587, elapsed 94.041s, exit 0/0, FINISH post 391, fresh isolated config/states/saves, `--nospeedlimit` (PERTURBED_LOGICAL_ONLY). 92 data rows + 1 header. CSV `/home/wesley/.cache/tooie-fps-investigation/demo1-projectile-oracle/projectile.csv` (SHA256 `8bd18fd1444d6f8f6305b200f0c5afc68d0401739b6c4f10af29d42261568aab`). Counts: setup 1 pair, fire 2 enter / 1 leave, movement 7 pairs, sweep 7 pairs, objpoll 57. Controller JSONLs record exact cmd/env/hashes.
- Bounded Telemetry Findings & Epistemic Boundaries (PROVEN):
  - `setup_enter`/`leave` and `fire spawn_enter`/`leave` raw payloads are bit-exact between Tooie and Oracle.
  - All 7 actually matched movement and sweep pairs (`post 333..340`) have bit-exact raw payloads.
  - All 7 oracle sweep leave rows show `v0 = 0` (same as Current: no hit detected in this window). Baseline no-hit proven for these frames.
  - First raw `objpoll` difference occurs at `post 341` (~40 XYZ difference), BUT on Oracle the old tracked pointer `0x801d0a2c` freezes at `341..342`, exhibits a heap link at `343`, and then MIPS code words at `388`.
  - Object liveness, relocation, or free status is UNKNOWN (possible DEFRAG HYPOTHESIS); this is NOT proven as a first gameplay divergence! Current tracked pointer `0x801c767c` continues movement normally.
  - Snapshot UNKNOWN: target/muzzle final normals and callback delta-time are not captured (`w11` unused). Do not infer VI root.
- Background Workloads & Active Agents:
  - Prior Grok main 40-turn audit PID 1274832 was interrupted via SIGINT after 13 read-only turns (redundant gates before runtime); supervisor PID 1270040 was terminated via SIGTERM. Classification: `INTERRUPTED_REDUNDANT_AUDIT_DIRECT_CAPTURE` (not PASS). Raw JSON may be empty, source transcript preserved. Not active.
  - Grok data-only job PID 1284587 (max 12 turns, outputs `demo1-projectile-compare-grok.json` and `report.txt`), INCOMPLETE.
  - Sonnet liveness + callback DT diagnostic job PID 1286150 (outputs `demo1-projectile-liveness-sonnet.json` and `report.txt`), COMPLETED.
- Preserved Invariants:
  - Demo 7 generator fix (`patches/runtime/n64recomp-zero-size-static.patch` and official 420-file regeneration) strictly preserved; user visual PASS maintained.
  - User observation that FireEgg still MISSES on user build `87c5...` preserved.
  - P09 Phase 1 default CURRENT `tfPos` workaround remains default-ON.
  - Present FPS counter and Graphics options toggle remain verified.
  - RT64 below-native present decimator remains operational.
  - Audio glitch remains explicitly deferred per user directive.
  - No global clock, aim, or collision hypotheses promoted to specs.

```text
SCENE_IDENTITY: PROVEN (stage 0, sub 0, map 335 entrance 8 mode 5, glrecord ovl 698 cnt 766)
CURRENT_PROJECTILE_BINARY_SHA256: af0723b4b7f802446b455401b1c9e83a50aa61772ec37a598aa451df7f666705
ORACLE_CORE_SHA256: 41b59e2802be5782a2f3ab4f685bbad2fee062bd4da72e8ee01c6d6c4b7ebe88
CURRENT_PROJECTILE_CSV_SHA256: 6b5686b77b723180f08954d3573be31d73294140b22815ae39f90725432775c7 (189 rows)
ORACLE_PROJECTILE_CSV_SHA256: 8bd18fd1444d6f8f6305b200f0c5afc68d0401739b6c4f10af29d42261568aab (92 rows)
USER_DEMO7_STATUS: VISUALLY_FIXED (generator patch preserved)
USER_DEMO1_EGG_RESULT: USER_REPORTED_MISS (persists on user binary 87c5...)
USER_DEMO1_PREFIX_PARITY: 420/420_EXACT_MATCH (consumed 1..420 independently confirmed by Haiku)
PROJECTILE_SPAWN_AND_EARLY_SWEEPS: BIT_EXACT_MATCH (setup/fire/sweeps 333..340 matched; sweep leave v0=0)
ORACLE_TRACKED_PTR_FREEZE_AT_341: PROVEN (0x801d0a2c freezes 341..342, heap link 343; liveness/reloc UNKNOWN)
COLLISION_HIT_STATUS: UNKNOWN (baseline no-hit proven for early window; overall hit UNKNOWN)
ACTIVE_BACKGROUND_AGENTS: PID 1284587 (Grok compare), PID 1286150 (Sonnet liveness+DT diag)
PRESERVED_STATE: Demo 7 generator fix, CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: await liveness diagnostic gates and fresh bounded paired capture using stable identity; not global aim/clock fix
```

## Prior (2026-10-18) — USER_DEMO1_PAIRED_PARITY_PREFIX_MATCH_AND_HIT_UNKNOWN (SUPERSEDED BY PROJECTILE CAPTURE)

Read [[p08-user-demo1-movement-and-fire-egg-differential]], [[p08-user-demo7-cutscene-drift-investigation]], [[p08-demo-movement-precision-diagnosis]], and [[p08-attract-movement-residuals]].
User confirmed that Demo 7 cutscene stopping is **VISUALLY FIXED** and that the Fire Egg miss persists on the current corrected binary (SHA256 `87c5a08b96b6fbb9c29c0079ac6b7b6060af8f5da6b92147f60e5dacce85bbe5`). User clarified the latest objective is **general subtle gameplay/action parity** across movement, physics, and state, not an artificial aim or hit steering adjustment; no global clock hack. While the historical geometric miss was cataloged in earlier recomp studies (BASE `876d299f`, §16/17, margin +36.85), the fresh causal root on the current binary is strictly **UNKNOWN**.

- Diagnostic Implementation & Build Verification (PROVEN):
  - Sonnet CLI session 22383 implemented isolated, default-OFF flags `TOOIE_DIAG_DEMO1_PARITY=1` and `P08_DEMO1_PARITY=1` mirrored at EXACT PC `0x800151BC` ENTRY, stage 0 sub 0, glidx `0..420` (max 421 rows), player 0 only.
  - Schema covers 62 columns: cumulative guest VI handler entries `0x8001443C` and logic counter, raw 6-byte pad (`r0..r5`), precise 9-group fields (`px,py,pz`, `vx,vy,vz`, `ptype`, `velxz`, `tspd`, `yaw`, `ideal`, `phys_yaw`, `bs_prev`/`cur`/`next`, `anim`, camera `camx,camy,camz,camctrl,sel`, gamespeed `gs`).
  - Unit test `tests/tooie_demo1_parity_test` passed exit 0; ninja build exit 0 (diagnostic binary SHA256 `5b6fc6d069f00411cfd2ade4b94d7fa15055a44206bc100ffbd5ba569af31bc6`); oracle core rebuild exit 0 (lib SHA256 `c2a9e7fa7a3476c4a6eacb35bedc30c3d8128c16a28e190fbf0c0e96c05d9c04`).
- Controlled Paired Captures (PROVEN):
  - Current Tooie: Controller PID 1073249, game PID/pgid 1073258, elapsed 67.679s, exit 0, reason `window_consumed_420`. Console log `/home/wesley/.cache/tooie-fps-investigation/demo1-parity-current/console.log` (501,453 bytes, SHA256 `57920a649162c7bec4b17dd3d0e86ef41eed0fb813c98008a6323e5fe83492aa`). Header + 420 data rows.
  - Pinned Angrylion Oracle: Controller PID 1074236, oracle PID/pgid 1074257, elapsed 98.713s, exit 0, reason `window_consumed_420`. CSV `/home/wesley/.cache/tooie-fps-investigation/demo1-parity-oracle/demo1.csv` (179,297 bytes, SHA256 `fa45b8d202a791eba76b6ef0f2b20265ed2297cddc25eb0c90b11811f55736e3`). Header + 420 data rows. Wall time under `--nospeedlimit` is perturbed; logical parity only.
  - Owned capture processes exited cleanly; no processes relaunched; no user processes touched.
- Paired Comparator Verdict & Divergence Analysis (PROVEN):
  - Evaluated via `demo1-parity-compare.py` -> `demo1-parity-compare.json` and Grok final paired report `demo1-paired-main-grok-report.txt`.
  - **420 exact non-clock matched pairs** (consumed 1..420). Both sides skip only consumed 0 (post 1), confirming identical hook boundary without port split.
  - Zero 1-ULP differences, zero persistent geometric or action differences across sampled player and camera fields from consumed 1 through 420: raw `r0..r5`, `gs_bits` (matches at every consumed: `0x3D4CCCCE` = 0.05 and `0x3D088889` = 0.03333, including C-up row 322), `yaw`, `ideal`, `phys_yaw`, `px,py,pz` and bit words, `vx,vy,vz` and bit words, `velxz`, `tspd`, `accel`, `mult`, `ptype`, `bs_prev/cur/next`, `anim_id/timer/dur/phase`, `camx/camy/camz, camctrl, sel`.
  - **First in-window divergence**: VI-handler entry cadence (`0x8001443C`). In-window delta-VI mismatches on 100 of 419 transitions. First step consumed 1->2: Tooie dvi 0 (vi 3134), oracle dvi 6 (vi 3414->3420). Cumulative VI growth consumed 1->420: Tooie +847, oracle +887 (+40 entries). This is an emulator/runtime entry cadence difference under `--nospeedlimit`; it did NOT move any sampled guest fields.
  - Verdict: `player/input/camera/gs prefix MATCH through consumed 420. HIT UNKNOWN. No code fix. Not PASS. Not a visual-miss proof. Not a realtime proof. No aim, hit, or clock change.`
- Fire-Edge & Projectile Dispatch Telemetry:
  - Consumed 322 / post 323 (buttons `0x2008` = C-up with Z, stick `(-9, 84)`): player entry pose matches bit-exact between Tooie and oracle (`px,py,pz = -4692.11475, -426.484711, 1129.06921`, `yaw = 291.797241°`, `velxz = 287.608307`, `bs_cur = 9`, `anim_id = 42`, `cam = -4086.1438, -160.789032, 950.552246`). This entry pose differs from the later SHOT leave pose on `87c5...` due to sample phase and binary.
  - Oracle append tail (`/tmp/p08-angrylion-attract-eggs.log`, 56,989 bytes, 426 lines) reveals `cheggfire` sys dispatch occurs at raw post 333 (consumed 332): `800871f0` overlay 535 ep 2 (`obj64 = c5932bf8` [-4709.5], `obj6c = 448e0aee` [1136.34]).
  - Actor low-9 masking (`+0x6C & 0x1FF`) remains strictly INVALID.
- Independent Audits & Preserved Disagreements:
  - Grok Contract Audit (`workload-grok-parity-audit-report.txt`): PID 1065434 remains ACTIVE in background (user explicitly requested keeping agents running across quota resets). Audited compiler flags (`RecompiledFuncs` with `RECOMP_MIPS_ROUND_COMPAT=1`; `TooieRecompiled` without it), lookup table binding mechanism, native libm math overrides (`800136E4`, `800138D0`, `80013970`), stick helpers (`800BCCC0`, `800BCCD8`), and hypotheses H1-H4. Differentiated diagnostic binary `5b6fc6d0` from user-reported miss binary `87c5a08b`.
  - Haiku Capture Audit (`workload-haiku-capture-audit-report.txt`): 20 static gates G1-G20 evaluated. Confirmed 62-column schema parity. Flagged G15 oracle CSV append mode risk (successfully averted by Grok's fresh path) and G14 run-tagging gap.
- Active & Historical Process Tracking:
  - Active background agent: PID 1065434 (Grok independent contract audit).
  - Historical exited PIDs: Driver PID 1067821, Grok paired runner PID 1067822, Tooie controller PID 1073249 / game PID 1073258, Oracle controller PID 1074236 / oracle PID 1074257, earlier SHOT capture PID 1059296, user game PID 1050817.
- Preserved Prior State:
  - Demo 7 generator fix (`patches/runtime/n64recomp-zero-size-static.patch` and official 420-file regeneration) strictly preserved; user visual PASS maintained.
  - User observation that FireEgg still MISSES on current build `87c5...` preserved.
  - P09 Phase 1 default CURRENT `tfPos` workaround remains default-ON.
  - Present FPS counter and Graphics options toggle remain verified.
  - RT64 below-native present decimator remains operational.
  - Audio glitch remains explicitly deferred per user directive.
  - No global clock, aim, or collision hypotheses promoted to specs.

```text
SCENE_IDENTITY: PROVEN (stage 0, sub 0, map 335 entrance 8 mode 5, glrecord ovl 698 cnt 766)
CURRENT_DIAGNOSTIC_BINARY_SHA256: 5b6fc6d069f00411cfd2ade4b94d7fa15055a44206bc100ffbd5ba569af31bc6
USER_BUG_BASELINE_BINARY_SHA256: 87c5a08b96b6fbb9c29c0079ac6b7b6060af8f5da6b92147f60e5dacce85bbe5
ORACLE_CORE_SHA256: c2a9e7fa7a3476c4a6eacb35bedc30c3d8128c16a28e190fbf0c0e96c05d9c04
USER_DEMO7_STATUS: VISUALLY_FIXED (generator patch preserved)
USER_DEMO1_EGG_RESULT: USER_REPORTED_MISS (persists on user binary 87c5...)
USER_DEMO1_PAIRED_PARITY_PREFIX: 420/420_MATCH (consumed 1..420 non-clock columns bit-exact)
VI_CADENCE_CADENCE_MISMATCHES: 100/419_STEPS (runtime/emulator delta under --nospeedlimit; 0 guest field movement)
COLLISION_HIT_STATUS: UNKNOWN (no hit/miss event in prefix sampling)
FRESH_CAUSAL_ROOT: UNKNOWN (downstream of logic-entry player prefix)
ACTIVE_BACKGROUND_AGENT: PID 1065434 (Grok independent contract audit)
PRESERVED_STATE: Demo 7 generator fix, CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: same-phase bounded diagnostic on both sides around post 333 (cheggfire 800871f0 ovl 535 ep 2 + sweep callee), default OFF
```

## Prior (2026-10-17) — USER_DEMO7_FIX_PROVEN_AND_VERIFIED

Read [[p08-user-demo7-cutscene-drift-investigation]], [[p08-attract-movement-residuals]], and [[p08-provisional-follow-camera-and-egg-differential]].
The user-observed player drift during Demo 7 in the attract sequence (TDL Stomping Plains, stage 0 sub 8) has been forensically isolated, root-caused, and resolved at the owning generator layer without hand-editing generated code. Disassembly of `bs_setState` (`0x8009E5C8`) proved layout `*(player + 0x120)` maps `+0: prev`, `+4: cur`, `+8: next`, identifying the sliding state as State 116 (`0x74`, `_bsrest_entrypoint_12`). In N64 hardware, State 116 INIT calls static helper `0x80800000` (`static_344_80800000`) to reset `tspd = 0` and clear velocity vectors. In recompiled code, `static_344_80800000` was emitted with an empty body `;}` due to zero-size ELF markers (`bsrest_VRAM`, `bsrest_TEXT_START`) at `0x80800000` truncating the static in `N64Recomp/src/main.cpp`. Fix applied via versioned patch `patches/runtime/n64recomp-zero-size-static.patch` (registered in `tools/apply_runtime_patches.sh`), verified by `tests/n64recomp_zero_size_static_test.sh` (PASS), and official tooling full regeneration executed cleanly (ROM SHA1 `af479771ff5e91ce3732f3d84146614ab71d8d79`, ELF SHA1 `2b2d04c5b6484594f750b097daf670f7a8a24167`, size 91855676). Full build and fresh check exit 0. Independent final gate evaluation by Grok on fixed runtime capture `demo7-fixed-root.log` (121 rows, controller 396.13s) confirms **Criterion 1 (Bounded Demo 7 Drift) PASS**: 119 pose pairs match N64 reference within quantized tolerances, 120/120 pads match, glidx 38..120 is fully stationary at `(-80.46, 0.00, 4496.25)`, `velxz = 0.0`, `tspd = 0.0`. Serial 90.032s timing test (`timing-fixed-pid-serial-90s.log`) with PID-tagged serialization fix in `src/game/tooie_diag.cpp` verified guest VI cadence at ~60.012 Hz (PID 938341: 3543 VIs in 59.038 s; 24.306 logic Hz) with NO evidence of a 2x VI clock in the measured window. This does NOT prove universal camera/mob/campaign timing; user-specific non-Demo7 reproduction question remains open. Production bug is CLOSED for the tested window.

```text
USER_DEMO7_SCENE_MAPPING: PROVEN (stage 0, sub 8, glrecord 698 cnt 1245)
USER_DEMO7_ROOT_CAUSE: PROVEN (N64Recomp zero-size ELF marker bug truncated bsrest static_344_80800000)
USER_DEMO7_FIX_LAYER: GENERATOR_SOURCE (patches/runtime/n64recomp-zero-size-static.patch)
USER_DEMO7_CRITERION_1: PASS (119/119 pose pairs within tol; gl38..120 stationary pos/vel/tspd=0)
USER_DEMO7_CRITERION_2: NO_2X_VI_IN_MEASURED_WINDOW (~60.012 VI Hz; universal mobs/campaign unproven)
USER_DEMO5_RESIDUALS: OPEN_PRESERVED
PRESERVED_STATE: CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: normal clean user visual QA Demo 7, and concrete non-Demo 7 speed reproduction if persists.
```

## Prior (2026-10-17) — USER_DEMO7_CUTSCENE_DRIFT_INVESTIGATION_BOUNDED (SUPERSEDED BY DEMO 7 FIX)


## Prior (2026-08-31) — T_INTRO_TRUE_SOURCE_ASPECT_PRODUCTION

The user visually confirmed the true-source-aspect policy on the tested 16:9
Scene A and Scene B paths: no disappearing wall has been observed. It is now
production-default in Tooie at `func_800E44FC`, remains T_INTRO-only and
render-only, and derives source aspect from `abs(P[1][1]/P[0][0])` in the
actual cinematic projection. The old VI `304/228` source remains an RT64 root
finding, not the production cinematic source.

```text
PRODUCTION_POLICY: true cinematic source-aspect compensation
SOURCE_ASPECT_HARDCODED: NO
CANDIDATE_A_FINAL_STATUS: REMOVED
CANDIDATE_B_ENV_FLAG_FINAL_STATUS: REMOVED; production default
TINTRO_TRUE_SOURCE_ASPECT_VISUAL_RESULT: VISUALLY_CONFIRMED_FOR_TESTED_16X9_SCENES
TINTRO_21X9_PLUS_FINAL_STATUS: OPEN_REVALIDATION_REQUIRED
PROJECTION_POLICY_REGRESSION_TEST: ADDED / PASS
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
NO_PUSH: YES
```

## SUPERSEDED (2026-08-31) — T_INTRO_PROJECTION_SOURCE_ASPECT_CANDIDATES

The RT64 `K=4/3` projection root is confirmed. Two diagnostic-only Tooie-side
policies are now implemented, default OFF, and restricted to T_INTRO mode 3
or MAP 18B (395): `TOOIE_TEST_TINTRO_PROJECTION_ORIGINAL` and
`TOOIE_TEST_TINTRO_TRUE_SOURCE_ASPECT`. The latter derives the cinematic
source aspect from `abs(P[1][1]/P[0][0])`, which is approximately
`1.767441860`, and uses `min(target, source)/(4/3)` so no `K<1` is applied.
The code changes only the temporary projection matrix; no RT64 source setter,
global RT64 edit, geometry edit, or production promotion was made.

```text
EXISTING_ASPECT_EXTENSION_API: PARTIAL
TOOIE_CAN_OVERRIDE_SOURCE_ASPECT_WITHOUT_RT64_CORE_CHANGE: YES
CANDIDATE_A_IMPLEMENTED: YES; CANDIDATE_A_DEFAULT: OFF
CANDIDATE_B_IMPLEMENTED: YES; CANDIDATE_B_DEFAULT: OFF
CANDIDATE_A_MATCHES_GUEST_GOLDEN: YES
CANDIDATE_B_MATCHES_EXPECTED_TRUE_ASPECT: YES
SCENE_A_CANDIDATE_ACTIVE: YES
SCENE_B_CANDIDATE_ACTIVE: YES
GAMEPLAY_CANDIDATE_ACTIVE: NO
RENDER_ONLY_CHANGE: YES
DEMO_SEMANTIC_SIDE_EFFECT_RISK: LOW
TINTRO_21X9_PLUS_AUTHORED_EXTENT: PROVEN / SEPARATE
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS
USER_MANUAL_TEST_REQUIRED_NOW: YES
NEXT_SINGLE_ACTION: TOOIE_TEST_TINTRO_TRUE_SOURCE_ASPECT=1 ./build/TooieRecompiled
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-tintro-projection-candidate-20260831T034438Z/
ARTIFACT_CHECKSUM_MANIFEST: 665ae932cfe019b1cebd04fa6dc1f78c6b70f8d3388faee6c5522b1e0f7474ed
GPU_READBACK_USED: NO
```

## CURRENT (2026-08-31) — T_INTRO_RT64_TRIANGLE_TRANSLATION_DIVERGENCE

Guest golden CLIP/NDC from Scene-A frame 5 MATCH RT64 RSP CPU clip. The
first RT64 divergence is Expand `adjustProjectionMatrix`: cinematic
`viewProj[0][0]` `0.905148` → `0.678861` (`K=4/3` from VI 304/228 vs
window 16:9). This is **not** the 0.58% guest-projection-aspect delta and
is **not** the already-refuted presentation-envelope remap.

```text
TINTRO_16X9_RT64_ROOT_CLASS: RT64_PROJECTION_TRANSLATION
ROOT_CONFIDENCE: HIGH
FIRST_RT64_DIVERGENCE_STAGE: CLIP_X
RT64_EXPAND_EQUATION: matrix[row][0] *= 1/K; K=target/source=4/3; AUTO when origin NONE && scissor covers FB width && width>height
RT64_ORIGINAL_EXPAND_TRIANGLE_DIFFERENCE_COUNT: 5
RT64_TRANSFORM_CAN_EXPLAIN_LARGE_VISUAL_FAILURE: YES
CLIP_CLASSIFICATION_MATCH: NO
WITNESS_WINDING_MATCH: YES
EXPAND_FLIPS_ANY_WITNESS_WINDING: NO
GUEST_RT64_RASTER_STATE_MATCH: YES
SCENE_B_SHOWS_SAME_RT64_DIVERGENCE: YES
TINTRO_16X9_AUTHORED_EXTENT: STRONG_HYPOTHESIS
TINTRO_21X9_PLUS_AUTHORED_EXTENT: PROVEN
FIX_LAYER: TOOIE
FIX_SCOPE: TINTRO
CANDIDATE_IMPLEMENTED: diagnostic-only
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION: implement a Tooie-side default-off T_INTRO/cutscene candidate that prevents RT64 AUTO from applying K=4/3 to the cinematic viewProj; re-check Scene-A frame 5 walls; do not change RT64 globally
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
ARTIFACT_DIRECTORY: /tmp/p08-tintro-rt64-triangle-translation-20260831T032400Z/
ARTIFACT_CHECKSUM_MANIFEST: 4971dfafeea24b2c44d365c9df11350dbbcc4bb7f172991550de54731ff3f07a
GPU_READBACK_USED: NO
```

Witnesses (leaf `0x0031f6d8`): A idx 77 WALL `f5281fc8`; B idx 81 WALL
`61f2aa16`; C idx 47 B1_EXPECTED_WALL `6a470e60`; D idx 78 B1_LATER
`75749e8e`; E idx 67 UNKNOWN `f0dd1ea1`. RSP CPU clip MATCH; GPU
`modViewProj` uses the scaled first column. Isolation:
`APP_FOLDER_PATH=/tmp/tooie-intro-16x9-xdg/TooieRecompiled`.

## CURRENT (2026-08-31) — T_INTRO_PRESENTATION_ENVELOPE_RECONCILIATION

The prior Original `[-0.754,+0.754]` versus Expand `[-1,+1]` owner delta is
`REFUTED/SUPERSEDED`. `vis_half` originated in
the removed historical intro-wall probe as `presented/src_a`; it is an
aspect-derived host scalar, not guest-NDC X. The live T_INTRO scissor is
`(0,28)-(304,200)`: X is full `[-1,+1]`, while Y is letterboxed to
`[-0.754385965,+0.754385965]`.

RT64 Original and Expand both present all guest X values; they differ only in
host contain-fit mapping (Original pillarboxes at 16:9, Expand fills the
window). Corrected presented-owner difference is `0` (old `16872` was the
intentional `vis_half` X test). B-1 remains `1764` conservative UNKNOWN-owner
pixels and `4224` WALL-to-WALL replacements; the exact-ID mismatch is not a
proven visual wall defect. `RT64_PRESENTATION_TRANSFORM_AS_ROOT: REFUTED`.

Artifact: `/tmp/p08-tintro-presentation-reconciliation-20260831T025942Z/`
(manifest SHA `c08eeab9d38cf89be1c79b8184bb95e67b53ab3e703ad118901d856cb4dd6dc3`).

## CURRENT (2026-08-31) — TRUSTED_CPU_RDP_PIXEL_OWNER_REFERENCE

The local pinned Angrylion RDP Plus CPU core (`9c8b9ed3e7d7f00dff8bc872ccdd3fba1a3673fc`)
now supplies the offline owner replay. It uses the reference edgewalker,
subpixel setup, coverage-mask generation, z/alpha eligibility, blender path,
and framebuffer write callback; no generic float rasterizer or GPU readback.

```text
SCENE_A_FRAME:                     5
SCENE_A_ENV_TRIANGLES:             156
SCENE_A_OWNER_UNRESOLVED_PIXELS:   0
SCENE_A_WALL_OWNER_LEFT_20:        60.82%
SCENE_A_WALL_OWNER_RIGHT_20:       19.31%
B1_ACTUAL_OVERLAP_PIXELS:          5988
B1_EXACT_WALL_ID_FINAL:             NO; later draw owners replace it
B1_FINAL_CLASS:                     WALL 4224 / UNKNOWN 1764
B1_OWNER_CAUSE:                     LATER_DRAW_WINS
ORIGINAL_EXPAND_DIFFERENCE:         16872 presented-envelope pixels
GUEST_CPU_RENDER_PATH_AS_ROOT:      STRONGLY_WEAKENED
GPU_READBACK_USED:                  NO
```

The expected B-1 wall and CPU-nearest alternate both write through the
reference path (wall coverage 8/8 everywhere; alternate 8/8 except one 7/8
sample). The exact expected wall ID is not the final owner because later
draws 67 and 78 write the region; this is not a Z/alpha/state rejection.
Artifact: `/tmp/p08-tintro-rdp-owner-reference-20260831T011600Z/` (manifest
`dbf77934a31b2244a5b5af1568413a03c88a0ce823f3886f4b102a4fe9cbf83e`).

## CURRENT (2026-08-30) — T_INTRO_VISIBLE_SURFACE_OWNERSHIP_SCENE_A_VS_B

Scene-A frame 5 remains 156 submitted/render-eligible triangles; fresh
Scene-B equivalent capture frame 2409 has 80. The complete task-root walk
observed roots `0x80160cd0` and `0x80159600` (first-env A/B roots respectively
`0x80160cd0`/`0x80159600`) and decodes E2/E3 RDP SETOTHERMODE through nested
G_DL calls. B-1 wall and nearer alternate share Z compare/update OFF and the
same 2-cycle/bilinear/force-blend/alpha-none class; wall draw order is 48,
alternate 13. Exact RDP blender/coverage ownership remains UNKNOWN, so no
state-caused mismatch or candidate is proven. Artifact:
`/tmp/p08-tintro-render-state-owner-20260830T180800Z/` (manifest SHA
`3d342e999430f817d0333a539968f87935c5b66a848e29864e857edd6a1c3b14`).

## Prior (2026-08-30) — T_INTRO_PER_DRAW_TRANSFORM_OWNERSHIP

```text
SCENE_A_FIRST_ENV_FRAME:      5
SCENE_A_FIRST_HOLE_WITNESS_FRAME: 5
OPA7417_ZERO_TRIANGLE_ROOT:   WRONG_RESOURCE + WRONG_MODEL_MATRIX
SCENE_A_LEFT_EDGE_GDL:        0x0031f6d8 / 0x0031f8f8
SCENE_A_LEFT_EDGE_MODEL_MATRIX: 0x8017b0b0 T(camera)
PRIMARY_HOLE_FRAME:           5
PRIMARY_HOLE_SIDE:            LEFT
REAL_HOLE_FIRST_DROP_STAGE:   NO_CPU_DROP
REAL_HOLE_FIRST_DROP_FUNCTION: func_80013EF0
A_B_EQUIVALENT_DRAW_OWNER:    NO
A_B_EQUIVALENT_FIRST_DROP:    YES
SCENE_A_VISUAL_ROOT_CLASS:    MORE_EVIDENCE_REQUIRED
SHARED_VISUAL_ROOT:           STRONG_HYPOTHESIS
IMPLEMENTATION_COMPLEXITY:    LOW_MEDIUM
IMPLEMENTED_FINAL_FIX:        NO
NEXT_SINGLE_ACTION:           Dense CPU NDC occupancy grid of 0x0031f6d8/0x0031f8f8 at Scene-A frame 5; content-stable ID for Scene B
NEXT_RECOMMENDED_MODEL:       GPT-5.6 Luna
ARTIFACT_DIRECTORY:           /tmp/p08-tintro-per-draw-owner-20260830T193644Z/
ARTIFACT_CHECKSUM_MANIFEST:   88a9885d0abc13fb0e0bf3d12008058ee8c89e00404598876148644bb5a66fa9
GPU_READBACK_USED:            NO
```

`TOOIE_DIAG_TINTRO_DRAW_PROVENANCE=1` (default OFF). F3DEX2 `G_DL_PUSH=0`
is a call; `0x00123340` is setup-only. Frame-5 env uses local verts + MV
translation = camera. Do not widen CAB70. Do not force WALL7799.

## Prior session (2026-08-30) — T_INTRO_SCREEN_EDGE_OWNER_AUDIT

```text
SCENE_A_OPENING_FRAME:        3 (stable VP sample frame 315)
OPA_7417_TRIANGLES_IN_CLIP:   0
SCREEN_EDGE_TRIANGLE_OWNER:   NONE_PROVEN
ARTIFACT_DIRECTORY:           /tmp/p08-tintro-screen-edge-owner-20260830T190608Z/
ARTIFACT_CHECKSUM_MANIFEST:   1d9fad539b87a0c7ffd4fdeffa7840c1a97bf8f8ab148596d0a1660ce7f1f21b
```

Static OPA-7417 ownership of the opening hole is REFUTED/INSUFFICIENT.

## Prior session (2026-08-30) — T_INTRO_TYPE13_OPENING_LEFT_REJECT

```text
CURRENT_HEAD:                 17d49afe33871ba03e49c3f922658673ba59c659
TYPE13_REJECT_OWNER:          800CAB70
LIVE_PLANE_MAPPING_PROVEN:    YES (0=LEFT 2=TOP 3=BOTTOM; 1=RIGHT/FAR)
SCENE_A_FIRST_REJECT_PLANE:   LEFT
SCENE_A_FIRST_REJECT_VALUE:   +34.2265015
SOURCE_REJECT_TARGET_ACCEPT:  NO
FOUR_THREE_IDENTITY:          PROVEN
A_B_EQUIVALENT_CAMERA_FOUND:  YES
A_B_SAME_REJECT_RESULT:       YES
TOOIE_TYPE13_MATCHES_BANJO_GEOCMDD_CLASS: YES
CANDIDATE_IMPLEMENTED:        NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
SHARED_ROOT:                  PROVEN
NEXT_SINGLE_ACTION:           Identify SUBMITTED on-screen L/R 20% geometry at opening; do not add TOOIE_TEST_INTRO_TYPE13_ASPECT
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-tintro-type13-reject-plane-20260830T184303Z/
ARTIFACT_CHECKSUM_MANIFEST:   93b735b8ad433c291ea32340fae6627b268047636987a553f504f3909cfe967a
BUILD_RESULT:                 PASS
WINDOW_DISPLAY_TEST:          PASS 38/38
GPU_READBACK_USED:            NO
```

`TOOIE_DIAG_TINTRO_TYPE13=1` (default OFF) dumps live CAB70 planes from
the viewport pointer (`*0x8012D500`), not the pointer slot. Guest aspect
1.767 already. Six left-edge type-13 AABBs original-LEFT-reject; widen
does not accept. Scene B replay matches exactly. WALL7799 remains late
SCENE_A TOP, kept.

## Prior (2026-08-30) — TWO_INTRO_SCENES_SEPARATED

```text
CURRENT_HEAD:                 17d49afe33871ba03e49c3f922658673ba59c659
SCENE_A:                      SCENE_A_FIRST_VISIBLE_AFTER_LAUNCH
SCENE_A_EPOCH:                1
SCENE_A_MAP:                  0
SCENE_A_OVERLAY:              674
SCENE_A_OPA_MODEL:            G_DL 0x00123340
SCENE_A_CAMERA_OWNER:         T_INTRO cam (529.38, 6943.89, -1904.13)
SCENE_A_ROOT_DRAW:            func_80013EF0 + func_800E3DC0
SCENE_B:                      SCENE_B_NEW_GAME_INTRO
SCENE_B_EPOCH:                5 (frontend)
SCENE_B_MAP:                  0
SCENE_B_OVERLAY:              440/638 after gameselect 205
SCENE_B_OPA_MODEL:            G_DL 0x00123340
SCENE_B_CAMERA_OWNER:         T_INTRO replay; same opening cam as SCENE_A
SCENE_B_ROOT_DRAW:            func_80013EF0 + func_800E3DC0
PRIOR_WALL7759_INVESTIGATION_BELONGS_TO: SCENE_A
SCENE_A_B_SHARED_MAP_RENDERER: YES
SCENE_A_B_SHARED_VISIBILITY_STACK: YES
SCENE_A_FAILURE_CLASS:        VISIBILITY_REJECT
SCENE_B_FAILURE_CLASS:        VISIBILITY_REJECT
SCENE_A_WALL_OWNER:           G_DL 0x00123340
SCENE_A_FIRST_DROP_STAGE:     VISIBILITY_REJECT
SCENE_A_FIRST_DROP_FUNCTION:  func_800E3DC0 / func_800CAB70
SCENE_A_WALL_DL_SUBMISSION:   PROVEN_PRESENT
SCENE_A_MATCHES_DK64_INTERNAL_BOUND_PATTERN: NO
SCENE_B_MATCHES_DK64_INTERNAL_BOUND_PATTERN: NO
SHARED_ROOT_PLAUSIBLE:        HIGH
SHARED_ROOT_CANDIDATE:        same T_INTRO model + type-13 AABB; two plays
IMPLEMENTED_FINAL_FIX:        NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION:           At SCENE_A opening cam prove type-13 first reject plane (X vs TOP) for G_DL 0x00123340 lateral walls; CPU-only; do not force WALL7799
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-two-intro-scenes-20260830T181514Z/
ARTIFACT_CHECKSUM_MANIFEST:   4aae5e68e62eb61f5fc7dba8e6b9007d06ba70f61e5da1e762f680bde56b048a
BUILD_RESULT:                 PASS
WINDOW_DISPLAY_TEST:          PASS 38/38
GPU_READBACK_USED:            NO
NO_COMMIT:                    YES
NO_PUSH:                      YES
```

`TOOIE_DIAG_STARTUP_SCENES=1` (default OFF) is the process-start timeline.
It does not enable frontend. WALL7799 kOpenCam evidence remains valid for
**late SCENE_A** only; do not treat it as SCENE_B. Do not implement
`TOOIE_TEST_INTRO_7799_VIS`.

## Prior (2026-08-30) — MAP18B_ORIGINAL_PRESENTATION_ENVELOPE

```text
CURRENT_HEAD:                 17d49afe33871ba03e49c3f922658673ba59c659
INTRO_16X9_AUTHORED_EXTENT_AS_ROOT: REFUTED/SUPERSEDED
INTRO_21X9_PLUS_AUTHORED_EXTENT: PROVEN
INTRO_16X9_CPU_CULL_ROOT:     REFUTED
INTRO_16X9_MISSING_DL_ROOT:   REFUTED (7759/7779/7644/SKY18); OPEN (WALL7799)
INTRO_16X9_PRESENTATION_REVEALS_ORIGINALLY_CROPPED_NDC: REFUTED (X-crop)
INTRO_GUEST_PROJECTION_ALREADY_WIDE: PROVEN
INTRO_16X9_NEEDS_NEW_GEOMETRY: NO
INTRO_WALL_DL_SUBMISSION:     PROVEN_PRESENT
DL_PRESENT_4_3_MISSING_16_9:  NO
INTRO_16X9_FIRST_DROP_STAGE:  NO_CPU_DROP
INTRO_16X9_ROOT:              STRONG_HYPOTHESIS (upper-left empty w/o 7799)
Clamp16x9:                    REFUTED
GROUP81_AS_OPENING_HOLE_ROOT: REFUTED
GROUP84_AS_OPENING_HOLE_ROOT: REFUTED
INTRO_21X9_PLUS_GEOMETRY_EXTENSION_NEED: OPEN / LIKELY
IMPLEMENTED_CANDIDATE:        NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION:           Prove why nested leaf 0x819A8 (WALL7799 gfx 7783-7807) is never G_DL-appended at the opening pose; CPU-only; do not extend geometry; do not pillarbox ±0.754
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-intro-original-presentation-envelope-20260830T163614Z/
ARTIFACT_CHECKSUM_MANIFEST:   2c2ffed179c34aafc57a3040f83ffc175b4c24257abc2761be7ef895884a8fea
BUILD_RESULT:                 PASS
GPU_READBACK_USED:            NO
NO_COMMIT:                    YES
NO_PUSH:                      YES
```

Live VP dumps (71) + ROM triangles clipped in guest clip-space. Opening
frame 6954: submitted 7759/7779 cover left to ndc x=-1 for y≤+0.05;
y≥+0.15 empty at both ±0.754 and ±1 without 7799. Guest scissor is
full width, so original cinematic X is ±1. Isolation uses
`APP_FOLDER_PATH=/tmp/tooie-intro-16x9-xdg/TooieRecompiled`.
`INTRO_16X9_MISSING_DL_ROOT OPEN (WALL7799)` is `CORRECTED/SUPERSEDED`
by the type-13 AABB TOP-cull proof (current). The prior
`INTRO_SPECIAL_GEO_GATE: nested type-13 DRAW_DISTANCE` label is
`CORRECTED/SUPERSEDED` → `AABB_FRUSTUM_GATE`.

## Prior (2026-08-30) — MAP18B_16X9_EDGE_DRAW_SUBMISSION_AND_FIRST_DROP_GATE

```text
CURRENT_HEAD:                 17d49afe33871ba03e49c3f922658673ba59c659
INTRO_16X9_AUTHORED_EXTENT_AS_ROOT: REFUTED/SUPERSEDED
INTRO_21X9_PLUS_AUTHORED_EXTENT: PROVEN
INTRO_WALL_DL_SUBMISSION:     PROVEN_PRESENT
DL_PRESENT_4_3_MISSING_16_9:  NO
INTRO_16X9_FIRST_DROP_STAGE:  NO_CPU_DROP
INTRO_16X9_ROOT:              STRONG_HYPOTHESIS
CPU_CLIP_SPACE_DIFFERENCE:    NONE
HOST_VISIBLE_NDC_DIFFERENCE:  YES
DROP_ASPECT_DEPENDENT:        NO
INTRO_SPECIAL_GEO_GATE:       nested type-13 DRAW_DISTANCE 0x81980/0x81940/0x819A8
PRIOR_TEST_COVERED_THIS_EXACT_PATH: NO
Clamp16x9:                    REFUTED
GROUP81_AS_OPENING_HOLE_ROOT: REFUTED
GROUP84_AS_OPENING_HOLE_ROOT: REFUTED
INTRO_21X9_PLUS_GEOMETRY_EXTENSION_NEED: OPEN / LIKELY
IMPLEMENTED_FINAL_FIX:        NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION:           CPU-clip WALL7759 against ndc x=-1 vs x=-0.754; no geometry extension
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-intro-16x9-dl-drop-20260830T160704Z/
ARTIFACT_CHECKSUM_MANIFEST:   9ca8fa3c374c75b81594fa5f535633ff977f2a542bbb11389646d6ccbdca553a
BUILD_RESULT:                 PASS
GPU_READBACK_USED:            NO
NO_COMMIT:                    YES
NO_PUSH:                      YES
```

the historical `TOOIE_DIAG_INTRO_WALL_DL=1` probe (removed). Isolation must use
`APP_FOLDER_PATH` (XDG_CONFIG_HOME is ignored). Unique G_DL matching:
7759/7779/7644/SKY18 present at 16:9 Expand and 4:3 Original. No 16:9-only
CPU drop. Expand presented vis NDC is ±1.000; Original is ±0.754.
Cinematic projection stays ~1.767:1.

## Prior (2026-08-30) — MAP18B_OPENING_WALL_REAL_TRIANGLE_BINDING

```text
CURRENT_HEAD:                 17d49afe33871ba03e49c3f922658673ba59c659
GFX7759_GROUP:                nested type-13 196/7721 AABB (-894,2635,-4305)-(767,3780,-2810)
GFX7759_VISUAL_ROLE:          WALL
GFX18_GROUP:                  0
GFX18_VISUAL_ROLE:            BACKGROUND
XLU_EDGE_COVERAGE_PRESENT:    NO (7418 ROM-identical to 7417)
SOURCE_4_3_EDGE_COVERAGE_COMPLETE: NO (right sky/clear)
LEFT_VISIBLE_EDGE_OWNER:      gfx 7779/7644/7759/7760/7799
RIGHT_VISIBLE_EDGE_OWNER:     gfx 18 / NO_MODEL_GEOMETRY
TARGET_ONLY_HOLE_COUNT:       0 at 16:9/6955; 13 at 21:9+32:9 left
HIGH_Y_HOLE_CONFIRMED_BY_LIVE_RAYS: NO
GEOMETRY_EXTENSION_NOW_PROVEN: NO
TOOIE_MATCHES_BANJO_PATTERN:  LOW
IMPLEMENTED_FINAL_FIX:        NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION:           Bind gfx 10993/3214/8336 and frame-1255 LEFT y=+1 7799 miss; do not extend 81/84 or 7759
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-intro-real-edge-owner-20260830T145138Z/
ARTIFACT_CHECKSUM_MANIFEST:   380e7c2aec4d849f6fb3d7197173adfd2f533066890e3d497ec74c8fdcd4560f
BUILD_RESULT:                 SKIP
GPU_READBACK_USED:            NO
NO_COMMIT:                    YES
NO_PUSH:                      YES
```

Left opening edge is the WRAP wall at gfx 7759-family. Right is sky.
XLU 7418 is not a second mesh. 16:9 still hits the left wall. No
candidate. Clamp16x9 stays refuted. Do not push.

## Prior (2026-08-30) — MAP18B_INTRO_BOUNDARY_OWNERSHIP_AND_GEOMETRY_EXTENSION_CANDIDATE

```text
CURRENT_HEAD:                 17d49afe33871ba03e49c3f922658673ba59c659
GROUP81_VERTEX_COUNT:         7
GROUP81_TRIANGLE_COUNT:       3
GROUP84_VERTEX_COUNT:         21
GROUP84_TRIANGLE_COUNT:       9
RIGHT_BOUNDARY_SURFACE:       OPA group 81 gfx 2583
LEFT_BOUNDARY_SURFACE:        OPA group 84 DL 2643-2691
MATERIAL_STATE_REUSABLE:      YES
TEXTURE_CAN_REPEAT:           NO
STATIC_EXTENSION_FEASIBLE:    YES
STATIC_EXTENSION_SAFE:        NO
DYNAMIC_EXTENSION_FEASIBLE:   YES
TOOIE_CAN_USE_SAME_ARCHITECTURAL_PATTERN: PARTIAL
HIGH_Y_SEPARATE_HOLE_EXISTS:  YES
CHOSEN_ARCHITECTURE:          MORE_EVIDENCE_REQUIRED
CANDIDATE_IMPLEMENTED:        NO
IMPLEMENTED_FINAL_FIX:        NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION:           Bind opening-shot wall to live frustum-edge triangle hits (gfx 7759 / gfx 18) plus XLU 7418
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-intro-geometry-extension-candidate-20260830T143035Z/
ARTIFACT_CHECKSUM_MANIFEST:   6000f3f889a2693191175de272570e69ae73d93cd4be8eead9580c7adaa213e4
BUILD_RESULT:                 SKIP
GPU_READBACK_USED:            NO
NO_COMMIT:                    YES
NO_PUSH:                      YES
```

OPA 7417 type-13 leaves 81/84 have exact DL/vertex/material bind. Live
frustum-edge rays do not hit those leaves. No geometry candidate. Clamp16x9
stays refuted. Do not push.

## Prior (2026-08-30) — MAP18B_LIVE_CAMERA_AND_WALL_ENVELOPE_CAPTURE

```text
CURRENT_HEAD:                 17d49afe33871ba03e49c3f922658673ba59c659
PRODUCTION_MILESTONE_HEAD:    23c070a9a2e00c691c04e543e0d8fc2aafebcd41
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP18B_CAMERA_OWNER:          D_8012D500 + func_800CB124 + host func_800E44FC
INTRO_REAL_CAMERA_POSITION:   (-1243.8077, 3682.9285, -1254.0330) frame 6955
INTRO_REAL_CAMERA_FORWARD:    (0.74934, 0.00698, -0.66215)
INTRO_REAL_VFOV:              30.7071 deg
INTRO_REAL_TARGET_ASPECT:     1.777778
INTRO_REAL_TARGET_HFOV:       52.0369 deg
INTRO_WALL_GEO_GROUP:         81 right / 84 left
INTRO_WALL_LOCAL_AABB:        group 81 (2336,1258,-4308)-(4460,2563,-3486)
BOUNDARY_PLANAR:              NO
LIVE_MAX_SAFE_HORIZONTAL_FOV: 40.2985 deg
LIVE_MAX_SAFE_ASPECT:         1.3363:1
SAFE_4_3:                     YES
SAFE_16_10:                   NO
SAFE_16_9:                    NO
SAFE_21_9:                    NO
SAFE_32_9:                    NO
DYNAMIC_CAMERA_FULL_WIDTH_FEASIBLE: NO
GEOMETRY_EXTENSION_FEASIBLE:  YES
GEOMETRY_EXTENSION_COMPLEXITY: MEDIUM
BANJO_TOOIE_STRUCTURAL_SIMILARITY: HIGH
INTRO_FINAL_SOLUTION_CLASS:   MORE_EVIDENCE_REQUIRED
INTRO_CLAMP16X9_VISUAL_RESULT: NO_CHANGE
FAILED_CLAMP_CHANGE_REMOVED:    YES
IMPLEMENTED_FINAL_FIX:        NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION:           CPU-parse OPA 7417 vertex/material/DL ownership for live groups 81/84
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-intro-live-camera-envelope-20260830T133604Z/
ARTIFACT_CHECKSUM_MANIFEST:   3c4b0229c98c233364abc69def03f61f23749e21f2c11dac5216b3c913dbf8d7
GPU_READBACK_USED:            NO
BUILD_RESULT:                 PASS
NO_COMMIT:                    YES
NO_PUSH:                      YES
```

Live CPU T_INTRO capture. Constraining descent is 4:3-safe and fails
wider aspects on the right (group 81). Camera full-width is not
viable. Geometry extension needs vertex/DL bind. No production fix.
Do not push.

## Prior (2026-08-30) — INTRO_ARBITRARY_ASPECT_AUDIT

Representative alcove inference only; live pose UNKNOWN. Artifact
`/tmp/p08-intro-arbitrary-aspect-fix-design-20260830T122919Z/`.

## Prior (2026-08-30) — CLEAN_PINNED_BUILD_VALIDATION

```text
CURRENT_HEAD:                 23c070a9a2e00c691c04e543e0d8fc2aafebcd41
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
GAMEPLAY_MAP_ROOT:            resolved / production fix active
GAMEPLAY_OBJECT_ROOT:         resolved / production fix active
CLEAN_PINNED_BUILD_VALIDATION: PASS
CLEAN_CONFIGURE_RESULT:       PASS
CLEAN_BUILD_RESULT:           PASS
CLEAN_SMOKE_RESULT:           SKIP
CAPTURE_ABI_NEEDED_BY_CLEAN_BUILD: NO
CAPTURE_ABI_REFERENCED_ONLY_BY_DIRTY_DIAGNOSTICS: YES
MILESTONE_REPRODUCIBLE_FROM_CLEAN_PINS: YES
INTRO_WALL_RESIDUAL:          STILL_PRESENT
INTRO_MISSING_WALL_CLASS:     SOURCE_GEOMETRY / CAMERA_FRAMING
INTRO_ROOT_STATUS:            STRONG_HYPOTHESIS
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_DISCRIMINATOR:           INTRO_MAP_18B_SOURCE_EXTENT_VS_EXPAND_FRUSTUM
NEXT_SINGLE_ACTION:           CPU dump MAP_18B OPA/XLU vertex XZ vs 4:3 and Expand
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-clean-milestone-validation-20260830T035059Z/
ARTIFACT_CHECKSUM_MANIFEST:   af72f29f96026b3d678d0f6914ea21b6aeaae648ea6c9fe25a4e5e1ea7976228
```

Isolated clean pins built `TooieRecompiled`. Capture ABI is unused by
clean RT64. Do not push.

## Prior (2026-08-30) — OBJECT_FRUSTUM_PRODUCTION + INTRO_WALL_ROOT

```text
CURRENT_HEAD:                 23c070a9a2e00c691c04e543e0d8fc2aafebcd41
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
800CACEC_AS_OBJECT_POP_GATE:  PROVEN
OBJECT_FRUSTUM_ASPECT_AWARE_VISUAL_RESULT: VISUALLY_CONFIRMED
OBJECT_FRUSTUM_PRODUCTION_PROMOTED: YES
PRODUCTION_OBJECT_FIX_MATCHES_VALIDATED_DIAGNOSTIC: YES (attract new_accept 279 vs diagnostic 244)
DEMO_ORIGINAL_CULL_RESULT_PRESERVED: YES (DRAW-only expand)
MAP_VIS_PRODUCTION_FIX:       ACTIVE
INTRO_WALL_RESIDUAL:          STILL_PRESENT
INTRO_MISSING_WALL_CLASS:     SOURCE_GEOMETRY / CAMERA_FRAMING
INTRO_ROOT_STATUS:            STRONG_HYPOTHESIS
INTRO_STATE_FUNCTION:         T_INTRO mode=3 map=0; MAP_18B_CS_STARTUP_INTRO 395
INTRO_CAMERA_PATH:            chintrocam
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_DISCRIMINATOR:           INTRO_MAP_18B_SOURCE_EXTENT_VS_EXPAND_FRUSTUM
NEXT_SINGLE_ACTION:           CPU dump MAP_18B OPA/XLU vertex XZ vs 4:3 and Expand
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-object-production-intro-root-20260830T031102Z/
ARTIFACT_CHECKSUM_MANIFEST:   3d69e9a80426875a31e3c65c1c07e538c61a93d9b49d9d54df2c39a1a2ba0fe8
```

User confirmed object-frustum aspect-aware. Production ON unless
`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0`. Intro walls remain after
both gameplay fixes. Banjo intro needed extra OPA triangles. No intro
production fix. This commit is the gameplay `800CAA24` + `800CACEC`
production milestone. Do not push.

## Prior (2026-08-30) — OBJECT_MODEL_FRUSTUM_BANJO_PORT

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_VIS_PRODUCTION_PROMOTED:  YES
GAMEPLAY_INSTANCES_CAPTURED:  YES
OBJECT_A_RUNTIME_IDENTITY:    chsilo 0x801ad888 MEDIUM
OBJECT_B_RUNTIME_IDENTITY:    chnests 0x801adf3c MEDIUM
CONTROL_C_RUNTIME_IDENTITY:   chnests 0x801adc30 LOW
OBJECT_A_TRANSITION_CLASS:    OBJECT_MODEL_FRUSTUM_SIDE_PLANE
OBJECT_B_TRANSITION_CLASS:    OBJECT_MODEL_FRUSTUM_SIDE_PLANE
SOURCE_REJECT_TARGET_ACCEPT_COUNT: 89/96 logs; attract 244 applied
800CACEC_AS_OBJECT_POP_GATE:  PROVEN
CACEC_SEES_EXPANDED_PLANES:   NO
OBJECT_FRUSTUM_COUNTERFACTUAL_FLAG: TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE
OBJECT_FRUSTUM_COUNTERFACTUAL_RESULT: STRONG_HIT
USER_MANUAL_TEST_REQUIRED_NOW: YES
NEXT_DISCRIMINATOR:           USER_VISUAL_OBJECT_FRUSTUM_ASPECT
NEXT_SINGLE_ACTION:           TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1 ./build/TooieRecompiled
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-object-model-frustum-banjo-port-20260830T024845Z/
ARTIFACT_CHECKSUM_MANIFEST:   ab92e43b8cfe48e81ae44b85beed830d8751f621d54aeadc7bd0b8ae55f9e126
```

User rerun summary `/tmp/tooie-ws-object-pop-summary.txt` SHA256
`6eaffa472c24e6a9556d9a0197205a7aaacac21436f265c743d400b54a089bab`.
frames=8861 logs=96 dropped=902. Prior CACEC bypass NO_CHANGE was
intro/map walls. Diagnostic aspect-aware wrap default OFF. Do not
promote. Do not combine vis flags.

## Prior (2026-08-30) — OBJECT_POP_WITNESS_ANALYSIS

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_VIS_PRODUCTION_PROMOTED:  YES
OBJECT_POP_SCREENSHOT_SEQUENCE: PROVEN
OBJECT_POP_CLASSIFICATION:    INCONCLUSIVE
OBJECT_A_CLASS:               UNKNOWN
OBJECT_B_CLASS:               UNKNOWN
CONTROL_OBJECT_C_CLASS:       UNKNOWN
LOCKED_THIS_RUN:              chintrochar 0x80086C98
TYPE6_GAMEPLAY_OBJECT_POP_STATUS: REOPENED_TARGETED
800CACEC_STRUCTURAL_ANALOG_OF_8024DB50: PROVEN
800CACEC_AS_OBJECT_POP_GATE:  STRONG_HYPOTHESIS
USER_MANUAL_TEST_REQUIRED_NOW: YES
NEXT_DISCRIMINATOR:           USER_VISUAL_OBJECT_POP_DIAG_CACEC
NEXT_SINGLE_ACTION:           TOOIE_DIAG_WS_OBJECT_POP=1 ./build/TooieRecompiled (do not combine vis flags)
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-object-pop-witness-analysis-20260830T022303Z/
ARTIFACT_CHECKSUM_MANIFEST:   471db3eaf6f138143cfcf7443adbeb6eeef183aa40d38c9a31ef54610b98dbef
```

Summary `/tmp/tooie-ws-object-pop-summary.txt` SHA256
`2c9a7b53b0761257d77c662526b5dbf45a6c098ed135d508c7bdd761208ee595`.
frames=8315 logs=0. Diagnostic retargeted onto `800CACEC`. No object
production fix. Banjo demo pattern: keep orig cull for callbacks; do not
blanket TRUE.

## Prior (2026-08-30) — MAP_VIS_PRODUCTION + OBJECT_POP_WITNESS

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_VIS_ASPECT_AWARE_VISUAL_RESULT: VISUALLY_CONFIRMED_FOR_TESTED_GAMEPLAY_MAP_GEOMETRY
MAP_VIS_PRODUCTION_PROMOTED:  YES
PRODUCTION_MAP_FIX_MATCHES_VALIDATED_DIAGNOSTIC: YES
FOUR_THREE_IDENTITY:          PROVEN
OBJECT_POP_SCREENSHOT_SEQUENCE: PROVEN
OBJECT_A_CLASS:               UNKNOWN
OBJECT_B_CLASS:               UNKNOWN
CONTROL_OBJECT_C_CLASS:       UNKNOWN
TYPE6_GAMEPLAY_OBJECT_POP_STATUS: REOPENED_TARGETED
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
ACTOR_CUBE_AABB_SELECTION_AS_EXPAND_DRAW_ROOT: REFUTED
INTRO_WALL_RESIDUAL:          OPEN_MIXED
OBJECT_POP_DIAGNOSTIC_FLAG:   TOOIE_DIAG_WS_OBJECT_POP
USER_MANUAL_TEST_REQUIRED_NOW: YES
NEXT_DISCRIMINATOR:           USER_VISUAL_OBJECT_POP_DIAG
NEXT_SINGLE_ACTION:           TOOIE_DIAG_WS_OBJECT_POP=1 ./build/TooieRecompiled (do not combine vis flags)
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-map-production-and-object-witness-20260830T020240Z/
ARTIFACT_CHECKSUM_MANIFEST:   9c7566804674e3057f71414d1cf659396e47e75fb705dfdf4bd046b0ba5e86d8
```

Production: `tooie_map_vis_aspect_aware_enabled()` ON unless env `=0`.
Blanket not promoted. Attract P@5880 new_accept=3425 vs C 3504. Object
identities UNKNOWN; diagnostic default OFF, summary
`/tmp/tooie-ws-object-pop-summary.txt`.

## Prior (2026-08-30) — WIDESCREEN_EDGE_GEOMETRY 800CAA24_FINAL_SEMANTICS

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MAP_VIS_ASPECT_AWARE_CLASSIFICATION: STRONG_CANDIDATE
FUNC_800CAA24_ROLE: world float AABB vs 4 frustum SIDE planes (no near/far)
FUNC_800E3D0C_ROLE: map-section wrapper; two 800CAA24 on D_8012D500
PLANE_BUILD: 800CA440/800CA218/800CAF34; H uses aspect (vol+0xF8); V does not (vol+0x104)
VOL_PLUS_0xC: world forward vec3 (NOT pitch/yaw); do not reuse tooie_compute_wide_frustum_planes
K: target/source via tooie_window::calculate_target_aspect_ratio; k<=1 original; H n_cam_x /= k
INDEPENDENT_GEOCMD_LIKE_CULL_EXISTS: YES (800E3DC0/800CAB70)
DISTINCT_GEOCMD_LAYER_BEYOND_E3DC0: NO
INTRA_MODEL_GEOCMD_ANALOG_UNTESTED_WORDING: CORRECTED/SUPERSEDED
E3DC0_DIRECT_ROOT: REFUTED
TARGET_ASPECT_AVAILABLE_TO_GUEST_PATCH: YES
RT64_CHANGE_NEEDED: NO
FOUR_THREE_IDENTITY: PROVEN (k<=1 leaves orig)
ALIGNED_FRAMES: 5880
ABC_DL: A=262956 B=1063786 C=283719 (C/A=1.079 B/A=4.045)
C_ASPECT: new_accept=3504 still_rej=329256 skip=0 narrowed=1 k=1.33333
USER_VISUAL_TEST_REQUIRED_NOW: YES
NEXT_DISCRIMINATOR:           USER_VISUAL_MAP_VIS_ASPECT_AWARE
NEXT_SINGLE_ACTION:           TOOIE_TEST_MAP_VIS_ASPECT_AWARE=1 ./build/TooieRecompiled (do not combine with TOOIE_TEST_DISABLE_MAP_VIS_CULL)
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-map-vis-final-semantics-20260830T010730Z/
ARTIFACT_CHECKSUM_MANIFEST:   2c66095cc262b4d6b8fad31a4bc13b0609109db4555e223cb638fce456c3eaca
```

Attract C `[ws-map-vis-aspect] flag=1 k=1.33333 source=1.33333 target=1.77778`.
Prior remaining-static artifact preserved
`/tmp/p08-widescreen-remaining-static-decorative-draw-20260830T002649Z/`
`f7fa715b7bb5888df0222be2ef5a2439243453f67732ad6aafb1414f634c5174`.

## Prior (2026-08-30) — WIDESCREEN_EDGE_GEOMETRY REMAINING_STATIC_DECORATIVE_MODEL_DRAW

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
FUNC_80108C14_ROLE: rare D_80136EE0 walk; overlay-loaded then 800819B4
FUNC_80108C14_IS_PER_FRAME: NO
FUNC_80108C14_IS_DRAW: CONDITIONAL
FUNC_80108C14_EMITS_DL: YES structurally; LIVE 0
MODEL_LIST_GLOBAL: D_80136EE0 (0x9C type-6 objects; same as 800FF62C)
MODEL_LIST_JALR: 80100368 obj+0x10 GETTER (not DRAW)
80108C14_AS_REMAINING_DRAW_ROOT: REFUTED
REMAINING_STATIC_DECORATIVE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
STATIC_DECORATIVE_VIEW_COUNTERFACTUAL: NOT_IMPLEMENTED
ACTOR_PICKUP_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
BOUND_SPRITE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
SPRITE_800F0064_AS_LATERAL_EXPAND_ROOT: REFUTED
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
TYPE6_AS_GLOBAL_GAMEPLAY_RESIDUAL_ROOT: UNKNOWN / LOW_PRIORITY
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: NO
NEXT_DISCRIMINATOR:           GLOBAL_RESIDUAL_RERANK_800CAA24_FINAL_SEMANTICS
NEXT_SINGLE_ACTION:           Re-rank remaining gameplay residual after closed object families: 800CAA24 remaining section semantics versus intra-model GeoCmd analog versus leftover source geometry. Do not return to GPU. Do not combine vis flags.
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-remaining-static-decorative-draw-20260830T002649Z/
ARTIFACT_CHECKSUM_MANIFEST:   f7fa715b7bb5888df0222be2ef5a2439243453f67732ad6aafb1414f634c5174
```

60s attract LIVE: `08c14=2 visits=42 skip_id0=41 da298 fail=1 dl=0`;
type-6 `ff62c=1337 vis rej=0 dl=53`. Intersection of `D_80136EE0` and
`800819B4` is the closed type-6 family only. HARD STOP Parts 14/15.
Prior actor-pickup artifact preserved
`/tmp/p08-widescreen-actor-pickup-premodel-view-20260830T000358Z/`
`4706ad90341a23e6bbf299a53acac7495b32f472387dbcc3d7dc77665fec0d5d`.

## Prior (2026-08-30) — WIDESCREEN_EDGE_GEOMETRY ACTOR_PICKUP_PREMODEL_VIEW_VISIBILITY

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
FUNC_8011458C_ROLE: occupancy/near-fog probe via 801143A4; NOT actor draw
ACTOR_SOURCE_CONTAINER: gccube grid D_80128410 / cube 0xC occupancy
ACTOR_SPATIAL_SELECTION_FUNCTION: 80114184 AABB + 800BD708 cube idx
ACTOR_LIST_CONSUMER: 800CD23C
ACTOR_DRAW_DISPATCH: 800EA628
ACTOR_MODEL_DRAW: 80103508
ACTOR_DL_EMIT: 8010108C
ACTOR_PIPELINE_EXERCISED: YES (60s attract)
ACTORS_CONSIDERED: ea628 A=22722 B=257504
ACTORS_SENT_TO_DRAW: 03508=3 both; draw108c=0 dl=0
ACTOR_VIEW_COUNTERFACTUAL_FLAG: TOOIE_TEST_EXPAND_ACTOR_CUBE_AABB
ACTOR_VIEW_COUNTERFACTUAL_RESULT: PARTIAL_HIT visitation / NO_CHANGE draw-DL
ACTOR_PICKUP_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
BOUND_SPRITE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
SPRITE_800F0064_AS_LATERAL_EXPAND_ROOT: REFUTED
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
TYPE6_AS_GLOBAL_GAMEPLAY_RESIDUAL_ROOT: UNKNOWN / LOW_PRIORITY
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: NO
NEXT_DISCRIMINATOR:           REMAINING_STATIC_DECORATIVE_MODEL_DRAW
NEXT_SINGLE_ACTION:           Start at 80108C14 model-list / overlay jalr DRAW vs UPDATE for leftover items/rocks not 800CAA24, not 800EA628->8010108C (attract 0108c=0), not bound-sprite, not type-6 Euclidean. Do not return to GPU.
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-actor-pickup-premodel-view-20260830T000358Z/
ARTIFACT_CHECKSUM_MANIFEST:   4706ad90341a23e6bbf299a53acac7495b32f472387dbcc3d7dc77665fec0d5d
```

60s attract LIVE A/B: cube expand ±2 XZ raises `ea628` 22722→257504 and
`cube_span` max 8→72; `03508` stays 3; actor `0108c=0`. Extra candidates
fail Euclidean `800EB350` / pass-mode `800CB854`. HARD STOP on actor
view family. Prior bound-sprite artifact preserved
`/tmp/p08-widescreen-bound-sprite-per-frame-draw-20260829T231221Z/`
`fde93ff1c4e3059d186c567f810ad753b7fbe9cdc075ede0b193726d62a98d51`.

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY BOUND_SPRITE_INSTANCE_PER_FRAME_DRAW

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
BOUND_SPRITE_INSERT_FUNCTION: 800B3CDC
BOUND_SPRITE_CONTAINER: D_8012AE80+0xE0
BOUND_SPRITE_RECORD_SIZE: 0x18
BOUND_SPRITE_FIRST_PER_FRAME_CONSUMER: 800CB8F0
BOUND_SPRITE_DRAW_FUNCTION: NONE
BOUND_SPRITE_DL_EMIT_FUNCTION: NONE
BOUND_SPRITE_PATH_EXERCISED: YES (60s attract; cb8f0=5777)
BOUND_SPRITE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
SPRITE_800F0064_AS_LATERAL_EXPAND_ROOT: REFUTED
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
TYPE6_AS_GLOBAL_GAMEPLAY_RESIDUAL_ROOT: UNKNOWN / LOW_PRIORITY
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: NO
NEXT_DISCRIMINATOR:           ACTOR_PICKUP_PREMODEL_VIEW_VISIBILITY
NEXT_SINGLE_ACTION:           Trace actor/pickup pre-draw vis at 800CAB70/800E3DC0/800CACEC and 8011458C; prove SOURCE_OUTSIDE_TARGET_INSIDE before any actor CF. Do not reopen 800F0064 or bound-sprite draw.
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-bound-sprite-per-frame-draw-20260829T231221Z/
ARTIFACT_CHECKSUM_MANIFEST:   fde93ff1c4e3059d186c567f810ad753b7fbe9cdc075ede0b193726d62a98d51
```

60s attract LIVE: inserts=90; query cbc3c=6 cbdd4=334 cc078=1730 cc338=813;
cb8f0 5777/5777; inst_samples=21; draw_fn=NONE. Packed ids seen:
0,59,72,75,85,109,128,143 (unnamed). HARD STOP: occupancy/query family,
not a renderer. Actor next-start: 800CAB70/E3DC0 LIVE 77107/74658/2449,
8011458C=1480, 800BE09C RANGE (not static sprite root).

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY GSPROPS_SPRITE_7_9_A_PER_FRAME_VISIBILITY

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
PROP_TYPE_7_ROLE: static bound sprite
PROP_TYPE_9_ROLE: static bound sprite
PROP_TYPE_A_ROLE: SKIPPED
PROP_TYPE_C_ROLE: static bound sprite (800CBB18)
FUNC_800F0064_CAN_CAUSE_LATERAL_EXPAND_POP: NO
SPRITE_PATH_AUTOMATICALLY_EXERCISED: YES (60s attract)
T7_SEEN: 47  T9_SEEN: 36  TC_SEEN: 1
SPRITE_VISIBILITY_TESTS: 703  ACC: 62  REJ: 641
INSERTS A/B: 90 / 114
SPRITE_VIS_COUNTERFACTUAL_CPU_RESULT: PARTIAL_HIT
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
TYPE6_AS_GLOBAL_GAMEPLAY_RESIDUAL_ROOT: UNKNOWN / LOW_PRIORITY
USER_VISUAL_TEST_REQUIRED_NOW: NO
NEXT_DISCRIMINATOR:           BOUND_SPRITE_INSTANCE_PER_FRAME_DRAW
NEXT_SINGLE_ACTION:           Walk D_8012AE80+0xE0 / 800B3CDC products per frame; find camera/FOV gate if any. Do not reopen 800F0064 as vis.
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-gsprops-sprite-visibility-20260829T224516Z/
ARTIFACT_CHECKSUM_MANIFEST:   40941e011e86c403ec0b9cb0f781a6db3b15ff04cd4d7ea6aae18ce5be604894
```

60s attract LIVE: gccube6=2 (map load). 30s intro still t7=0. Type 0xA is
skipped; prior A→800CBB18 mapping is CORRECTED/SUPERSEDED (type 0xC).

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY LOADED_STATIC_MODEL_PER_FRAME_DRAW

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION:           YES
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
MODEL_GEO_FLAG_CONFIRMED_ACTIVE_DURING_GAMEPLAY: UNKNOWN
GSPROPS_ENTRYPOINT_1_ROLE: PROVEN_ONE_SHOT_TYPE6_LOAD
GSPROPS_ENTRYPOINT_1_SELECTOR_RESULT: REFUTED_AS_PER_FRAME_VISIBILITY_SELECTOR
PROP_CELL_800BE09C_RESULT: REFUTED_FOR_STATIC_TYPE6_ROOT
FUNC_80108DC0_ROLE: model/resource constructor into D_80136EE0 (NOT vis)
FUNC_80108DC0_RETURN_STORAGE_LOCATION: D_80136EE0 via 80106668 (gsprops_1 does not store v0)
TYPE6_PROP_RECORD_SIZE: 0x14 cube-list; loaded object 0x9C
GSPROPMARKER_3_ROLE: scale decode only; no persistent marker
MODEL_PLUS_0X38_SEMANTIC: ctor INIT FUNC then SCALE FLOAT (80102FDC)
TYPE6_FIRST_PER_FRAME_CONSUMER: 800FF62C
TYPE6_FIRST_PER_FRAME_CONSUMER_ADDRESS: 0x800FF62C
TYPE6_PER_FRAME_ITERATION_SOURCE: D_80136EE0
TYPE6_DRAW_FUNCTION: 8010108C (first visit) then vtable+0x10
TYPE6_DL_EMIT_FUNCTION: 800819B4
TYPE6_FIRST_VISIBILITY_GATE: 8010038C later + 800EFFB4 Euclidean sphere
TYPE6_FIRST_VISIBILITY_GATE_ROLE: dist^2 vs radius^2; NOT frustum/FOV/aspect
TYPE6_VISIBILITY_GATE_STATUS: PROVEN exists; LIVE never rejected tracked intro type-6
TYPE6_CONSUMER_EXERCISED: YES (25s auto-start still intro; t7=0)
TRACKED_TYPE6_LOADED: 10
TRACKED_TYPE6_CONSUMER_SEEN: 320
TRACKED_TYPE6_DRAW_ATTEMPTS: 84
TRACKED_TYPE6_VISIBILITY_REJECTS: 0
TRACKED_TYPE6_VISIBILITY_ACCEPTS: 0
TRACKED_TYPE6_DL_EMITS: 53
TYPE6_VIS_COUNTERFACTUAL_IMPLEMENTED: YES (TOOIE_TEST_DISABLE_TYPE6_STATIC_VIS_CULL)
TYPE6_VIS_COUNTERFACTUAL_RESULT: NO_CHANGE
TYPE6_AS_OBSERVED_RESIDUAL_ROOT: STRONGLY_WEAKENED (intro)
RESIDUAL_ROCKS_LIKELY_TYPE6: UNKNOWN
RESIDUAL_ITEMS_LIKELY_TYPE6: UNKNOWN
RESIDUAL_GENERAL_ASSETS_LIKELY_TYPE6: UNKNOWN
STATIC_SPRITE_PROP_PATH: gccube_6 -> gsprops_2 types 7/9/A -> gcbound; NOT_EXERCISED intro; NOW NEXT
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: NO
CURRENT_ROOT_SET: 800CAA24 PARTIAL + type-6 draw path PROVEN but vis not aspect + sprite/gcbound OPEN + intro leftover MIXED
CURRENT_ROOT_SET_STATUS: PARTIAL_PROVEN + OPEN_RESIDUALS
PERFORMANCE_GATING_CLEANUP_PRESERVED: YES
GPU_READBACK_USED:            NO
FIX_IMPLEMENTED:              NO
NEXT_DISCRIMINATOR:           GSPROPS_SPRITE_7_9_A_PER_FRAME_VISIBILITY
NEXT_SINGLE_ACTION:           Exercise types 7/9/A -> gcbound -> 800F0064 on an automated attract/demo window that actually dispatches sprites (intro t7=0). Do not combine vis flags.
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-loaded-static-model-per-frame-draw-20260829T215504Z/
ARTIFACT_CHECKSUM_MANIFEST:   de43c12dfcaccd59e3b4cd4ca2b6aa88a74dd232e89b757c4e6e56393b4c7940
```

Live 25s A/B type-6: loaded=10, consumer_seen=320, draw_attempts=84,
vis rej/acc/forced=0, dl_emits=53, dl_rsp 34472 identical. `800EFFB4`
never entered for tracked type-6. `t7=t9=tA=0`. Do not wrap `80108DC0`
as vis. Do not bypass `+0x64` bit17. Do not combine vis flags.

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY GSPROPS_STATIC_PROP_SELECTOR

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION:           YES
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
GSPROPS_ENTRYPOINT_1_ROLE: PROVEN one-shot type-6 load of ALL cubes; NOT per-frame vis
STATIC_PROP_SELECTOR_STATUS: PROVEN one-shot ALL-cube type-6 load; REFUTED as residual per-frame vis
GSPROPS_VIS_COUNTERFACTUAL_IMPLEMENTED: YES (TOOIE_TEST_DISABLE_GSPROPS_VIS_CULL)
GSPROPS_SELECTOR_COUNTERFACTUAL_CPU_RESULT: NO_CHANGE
NEXT_DISCRIMINATOR:           LOADED_STATIC_MODEL_PER_FRAME_DRAW
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-gsprops-static-prop-selector-20260829T210810Z/
ARTIFACT_CHECKSUM_MANIFEST:   09bb0ef3fc74105e1306577af670ef13d088044768047bad03438b53378e27ce
```

Live intro 12s/25s: `bdcb8=1`, cubes=1274, ep0 appended=11, ep1 count=11,
load ok=10 fail=1, ep2=2548, t7=t9=tA=0, gcbound3=0, xzrad=0, 80108C14=1.
`800F0064` CF never fired (`forced=0`). RSP A/B 9375 vs 9445 is variance
(prior PROP_CELL A was 9445). Do not wrap `80108DC0` as vis.

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY RESIDUAL_PROP_CELL_AND_CUTSCENE_VISIBILITY_AUDIT

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP:             ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION:           YES
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
MODEL_GEO_FLAG_CONFIRMED_ACTIVE_DURING_GAMEPLAY: UNKNOWN
BK_PROP_SPATIAL_CONTAINER: Cube / CubeList / Prop
BK_VIEWPORT_CUBE_PATCHES_CURRENTLY_PRESENT: NO
BK_VIEWPORT_CUBE_PATCHES_HISTORICALLY_PRESENT: YES (8b79f76; removed 1e19dad)
TOOIE_PROP_SPATIAL_CONTAINER: gccube D_80128410
TOOIE_PROP_CELL_VISIBILITY_FUNCTION: 800BE09C (3x3x3 RANGE)
TOOIE_PROP_CELL_VISIBILITY_STATUS: STRONG_HYPOTHESIS actor; REFUTED intro RSP
TOOIE_PROP_DRAW_REACHED_WHEN_CELL_REJECTED: NO
PROP_CELL_COUNTERFACTUAL_IMPLEMENTED: YES
PROP_CELL_COUNTERFACTUAL_CPU_RESULT: listed 153->325; dl_rsp 9445 identical; model_geo 0
USER_VISUAL_TEST_REQUIRED_NOW: NO
INTRO_800CAA24_ACTIVE: YES
INTRO_NORMAL_MAP_RENDER_PATH: YES
INTRO_GEOMETRY_PRESENCE: suppressed set PROVEN; leftover UNKNOWN
INTRO_RESIDUAL_CLASS: MIXED
DEMO_RESIDUAL_CLASS: MIXED / UNKNOWN
BANJORECOMP_CURRENT_WIDESCREEN_CULLING_COMPLETE: NO
CUTSCENE_ASPECT_FALLBACK_FEASIBLE: PARTIAL
PERFORMANCE_GATING_CLEANUP_PRESERVED: YES
GPU_READBACK_USED:            NO
FIX_IMPLEMENTED:              NO
NEXT_DISCRIMINATOR:           GSPROPS_STATIC_PROP_SELECTOR
NEXT_SINGLE_ACTION:           Find selector feeding gspropsDll_entrypoint_1; do not combine MAP_VIS
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-residual-prop-cell-cutscene-audit-20260829T204203Z/
ARTIFACT_CHECKSUM_MANIFEST:   52a0f350cc540708f9a6de44c7e60bc60066c51e6807e71c71395589bee59fbd
```

Gemini TMEM setup artifact
`/tmp/p08-widescreen-focal-texture-tmem-setup-20260829T183238Z/`
(`SHA256SUMS` `d91118158d0753c17e1ad0370e6150834255ffebffc67b02554692f39ad8b782`)
is preserved for PERFORMANCE_GATING_CLEANUP. Epistemic correction: that smoke
never reached `00349d38`, so TMEM text is not a new runtime observation.
Do **not** continue into `TARGETED_FOCAL_DEPTH_CAUSAL_TEST_DESIGN` this round.

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY TARGETED_FOCAL_TEXTURE_TMEM_SETUP

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
PERFORMANCE_GATING_CLEANUP_IMPLEMENTED: YES
DIAGNOSTICS_OFF_OVERHEAD:     NEGLIGIBLE
OLD_P08_LOGS_DEFAULT_OFF:     YES
FOCAL_TEXTURE_FLAG:           TOOIE_DIAG_WS_FOCAL_TEXTURE
FOCAL_AUTO_TRIGGER_IMPLEMENTED: YES
AUTO_CAPTURE_DURATION:        300 focal draws (~150-180 frames)
AUTO_CAPTURE_STOP_CONDITION:  s_auto_draw_count >= 300
MANUAL_TOUCH_REQUIRED:        NO
FOCAL_SOURCE_DL:              00349d38 extra-left TRI1
FOCAL_TEXTURE_TRACE_READY:    YES
FOCAL_TMEM_TRACE_READY:       YES
FOCAL_TILE_TRACE_READY:       YES
FOCAL_LOAD_CHAIN_READY:       YES
FOCAL_UV_TRACE_READY:         YES
FOCAL_VS_CENTRAL_COMPARE_READY: YES
FOCAL_TEXTURE_TMEM_ANOMALY:   REFUTED
GPU_READBACK_USED:            NO
DEFAULT_RUNTIME_BEHAVIOR_CHANGED: NO
DEFAULT_RUNTIME_LOGGING:      NONE (standard boot output only)
USER_CAPTURE_REQUIRED_NOW:    NO
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-focal-texture-tmem-setup-20260829T183238Z/
ARTIFACT_CHECKSUM_MANIFEST:   d91118158d0753c17e1ad0370e6150834255ffebffc67b02554692f39ad8b782
NEXT_SINGLE_ACTION:           Design and implement TARGETED_FOCAL_DEPTH_CAUSAL_TEST_DESIGN to isolate depth occlusion without GPU readback.
NEXT_RECOMMENDED_MODEL:       Gemini 3.7 Flash High
```

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY TARGETED_RASTER_STATE_USER_ARBITRATION

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
CAPTURE_VALID:                YES
CAPTURE_ID:                   1
CAPTURE_START_FRAME:          2926
CAPTURE_END_FRAME:            3645
CAMERA_EFFECTIVELY_STATIC:    YES (3584.1, 50.7, 4893.8)
FOCAL_EVENTS:                 288 TRI1 (world verts exact)
FOCAL_GPU_SUBMITTED:          YES 286/286 call=134 proj=2
FOCAL_PIPELINE:               000075df080aa190 SPECIALIZED RasterVS+RasterPSMS
FOCAL_DEPTH_TEST_ENABLED:     YES
FOCAL_DEPTH_WRITE_ENABLED:    YES
FOCAL_DEPTH_COMPARE:          LESS
FOCAL_DEPTH_RANGE_VALID:      YES
FOCAL_DEPTH_TEST_RESULT_DIRECTLY_OBSERVABLE: NO
FOCAL_DEPTH_TARGET_ID:        00137840
PRIOR_PROJ1_DEPTH_RESULT_APPLIES_TO_FOCAL: NO
DEPTH_CLEAR_BEFORE_FOCAL:     UNKNOWN
DEPTH_OCCLUSION_RESULT:       UNKNOWN
PRIOR_DEPTH_OCCLUDER_CANDIDATES: 1_WEAK_CORNER_SLIVER (00348470)
ACTIVE_FRAGMENT_DISCARD_SITES_FOR_FOCAL: 0
ALPHA_COMPARE_ROOT:           REFUTED_FOR_FOCAL
COVERAGE_ZERO_ROOT:           REFUTED_FOR_FOCAL
COLOR_WRITE_DISABLED_ROOT:    REFUTED_FOR_FOCAL
BLEND_ZERO_OUTPUT_ROOT:       REFUTED_FOR_FOCAL
FOCAL_INTERSECTS_RASTER_SCISSOR: YES
FIRST_FOCAL_VS_CENTRAL_RUNTIME_STATE_DIFFERENCE: tex_id
CENTRAL_CONTROL_IS_DEPTH_EQUIVALENT: PARTIAL
CULL_WINDING_ANOMALY:         REFUTED_FOR_FOCAL
MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED
CURRENT_ROOT_CLASS:           WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-focal-raster-user-arbitration-20260829T181535Z/
ARTIFACT_CHECKSUM_MANIFEST:   a8ce5b47e43ca92986ce19628cc1c1084e6ff2fa22079dc4f17197bcf8636190
```

Canonical: [[p08-widescreen-edge-geometry]]. USER raster-state capture on the
missing-wall pose. Proj=2 focal has depth test/write LESS; do not apply the
proj=1 depth-off result. Explicit fragment discard 0/7. First runtime state
difference vs same-DL FRONT TRI2 is TMEM `tex_id`. Depth occlusion remains
UNKNOWN (no readback; no strictly closer visible prior writer).

`NEXT_SINGLE_ACTION:` `TARGETED_FOCAL_TEXTURE_TMEM_TRACE` on `00349d38` TRI1 vs same-DL FRONT TRI2. No GPU readback. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior session (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY TARGETED_RASTER_STATE_SETUP

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
INSTRUMENTATION_STATUS:       TARGETED_RASTER_STATE_TRACE_IMPLEMENTED
FOCAL_SOURCE_DL:              00349d38 extra-left TRI1
FOCAL_RASTER_FILTER_IMPLEMENTED: YES
PIPELINE_ID_CAPTURE_READY:    YES
SHADER_VARIANT_CAPTURE_READY: YES
ALPHA_COMPARE_CAPTURE_READY:  YES
COVERAGE_CAPTURE_READY:       YES
BLEND_CAPTURE_READY:          YES
COLOR_WRITE_CAPTURE_READY:    YES
DEPTH_CAPTURE_READY:          YES
SCISSOR_CAPTURE_READY:        YES
VIEWPORT_CAPTURE_READY:       YES
POSSCREEN_SEMANTICS_RESOLVED: YES (CPU-only, not in GPU shaders)
SOURCE_TARGET_CLIP_RELATION_CAPTURE_READY: YES
COMBINER_CAPTURE_READY:       YES
TEXTURE_STATE_CAPTURE_READY:  YES
FRAGMENT_DISCARD_SITES_IDENTIFIED: YES (0/7 active for focal)
FOCAL_HOST_SCREEN_BOUNDS_READY: YES
FOCAL_SCISSOR_INTERSECTION_READY: YES
POST_PROJECTION_NATIVE_COORDINATE_MISMATCH: REFUTED
SOURCE_CLIP_FLAGS_USED_AFTER_TARGET_EXPANSION: NO
FIRST_FOCAL_VS_CENTRAL_STATE_DIFFERENCE: NONE_OBSERVED
FOCAL_RASTER_STATE_CAPTURE_READY: YES
USER_CAPTURE_READY:           YES
GPU_READBACK_USED:            NO
FIX_IMPLEMENTED:              NO
DEFAULT_RUNTIME_BEHAVIOR_CHANGED: NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-targeted-raster-state-setup-20260829T173856Z/
ARTIFACT_CHECKSUM_MANIFEST:   fbd3366d1a666196a77c536779cd3b087a0957382ac60bee2cbffe334c81fc14
```

Canonical: [[p08-widescreen-edge-geometry]]. Targeted raster state trace
implemented in RT64 `FramebufferRenderer::addDrawCall` with dedicated log sink
`/tmp/tooie-ws-focal-raster-state.log` and unified tag `[ws-focal-raster-state]`.
Shader/PSO mapping proves specialized `RasterVS`/`RasterPS` pipeline is identical
between focal TRI1 and central same-DL FRONT TRI2. Alpha compare, coverage, blend,
color write mask (0x0F), depth test/write, and scissor intersection ([100..503] px
in [0..1920]) are 100% healthy. `FIRST_FOCAL_VS_CENTRAL_STATE_DIFFERENCE` is
`NONE_OBSERVED`.

`NEXT_SINGLE_ACTION:` User performs ONE capture at same wall, camera static, with `TOOIE_DIAG_WS_CONTINUOUS=1 TOOIE_DIAG_WS_WALL_BOUNDS=1 TOOIE_DIAG_WS_FOCAL_WINDING=1 TOOIE_DIAG_WS_FOCAL_RASTER=1`. Then Grok 4.6 Medium arbitrates.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## Prior session (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY TARGETED_FOCAL_VERTEX_WINDING_USER_ARBITRATION

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
CAPTURE_VALID:                YES
CAPTURE_ID:                   1
CAPTURE_START_FRAME:          3754
CAPTURE_END_FRAME:            4473
CAM_POS:                      (3598.4, 57.8, 4908.3)
FOCAL_00349D38_EVENTS:        1445 (TRI1=289, TRI2_0=578, TRI2_1=578)
FOCAL_V0_WORLD:               (2497.000, 400.000, 4263.000)
FOCAL_V1_WORLD:               (2497.000, -153.000, 4263.000)
FOCAL_V2_WORLD:               (2245.000, -44.000, 3680.000)
GEOMETRIC_EXPECTED_FACING:    FRONT
SOURCE_TARGET_WINDING_MATCH:  YES
RASTER_CLASSIFIED_FACING:     BACK (CLOCKWISE front face; CULL_FRONT keeps it)
EXPECTED_RASTER_SURVIVAL:     YES
CULL_WINDING_ANOMALY:         REFUTED_FOR_FOCAL
MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED
CURRENT_ROOT_CLASS:           WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-focal-winding-user-arbitration-20260829T164524Z/
ARTIFACT_CHECKSUM_MANIFEST:   d0d993e6e015d482e6807350b8d65de993d6348b7f8d96287972e9d1ee13e489
```

Canonical: [[p08-widescreen-edge-geometry]]. New USER capture at the same
missing-wall pose. Cull/winding is not the unique loss of the focal wall
triangle. Next is raster state on the submitted draw.

`NEXT_SINGLE_ACTION:` TARGETED_RASTER_STATE_TRACE on `00349d38` TRI1. No GPU readback. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior session (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY TARGETED_FOCAL_VERTEX_WINDING_USER_ARBITRATION

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
CAPTURE_VALID:                YES
CAPTURE_ID:                   1
CAPTURE_START_FRAME:          8278
CAPTURE_END_FRAME:            8997
CAM_POS:                      (3477.8, 113.9, 4682.8)
FOCAL_00349D38_EVENTS:        1440 (TRI1=288, TRI2_0=576, TRI2_1=576)
FOCAL_V0_WORLD:               (2497.000, 400.000, 4263.000)
FOCAL_V1_WORLD:               (2497.000, -153.000, 4263.000)
FOCAL_V2_WORLD:               (2245.000, -44.000, 3680.000)
GEOMETRIC_EXPECTED_FACING:    FRONT
SOURCE_TARGET_WINDING_MATCH:  YES
RASTER_CLASSIFIED_FACING:     BACK (CLOCKWISE front face; CULL_FRONT keeps it)
EXPECTED_RASTER_SURVIVAL:     YES
CULL_WINDING_ANOMALY:         REFUTED_FOR_FOCAL
MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED
CURRENT_ROOT_CLASS:           WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-focal-winding-user-arbitration-20260827T193119Z/
ARTIFACT_CHECKSUM_MANIFEST:   f1499333a0fcdb1395f550848e8539026419058f8935c4c8c6b3a9092943667c
```

Canonical: [[p08-widescreen-edge-geometry]]. Cull/winding is not the unique
loss of the focal wall triangle. Next is raster state on the submitted draw.

`NEXT_SINGLE_ACTION:` TARGETED_RASTER_STATE_TRACE on `00349d38` TRI1. No GPU readback. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior session (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY TARGETED_FOCAL_VERTEX_WINDING_DUMP

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
INSTRUMENTATION_STATUS:       IMPLEMENTED_CPU_SIDE_SAFE
GPU_READBACKS_USED:           NO (0 buffer readbacks, 0 image transfers, 0 thread race conditions)
PRIMARY_FOCAL_CANDIDATE:      source_dl = 00349d38 extra-left TRI1
SECONDARY_CONTROL:            source_dl = 0034b800
INTERNAL_CONTROL:             source_dl = 00349d38 central TRI2 triangles
LOG_SINKS:                    /tmp/tooie-ws-focal-winding.log + unified [ws-focal-winding]
ASPECT_SCALE_INVARIANCE_THEOREM: PROVEN (Sx > 0 strictly preserves 2D NDC area sign)
SELFTEST_RESULT:              66/66 checks passed (100% sign preservation across 1:1, 4:3, 16:10, 16:9, 21:9, 32:9)
BUILD_STATUS:                 SUCCESS (ninja exit code 0)
CURRENT_ROOT_CLASS:           WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-focal-vertex-winding-setup-20260827T154601Z/
ARTIFACT_CHECKSUM_MANIFEST:   d29cce81fc78995a14de11979ef1933cbcfa8fb0dbf158cbeb1bddf5cdcacc15
```

Canonical: [[p08-widescreen-edge-geometry]]. CPU-side telemetry hooked into `FramebufferRenderer::addDrawCall` extracts real $V_0, V_1, V_2$ object, world, clip, and NDC coordinates, model determinant, signed areas, world normals, camera positions, geometric dot products, and clip masks without any GPU memory readbacks or queue synchronization.

`NEXT_SINGLE_ACTION:` Execute safe focal vertex telemetry capture (or process captured telemetry) to record real $V_0/V_1/V_2$ parameters for `00349d38` extra-left TRI1 vs controls. No GPU readback. No fix. No bypass.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior session (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY TARGETED_CULL_WINDING_TRACE

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
USER_SCREENSHOT_CROP_MAPPING: APPROXIMATE
MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED
FOCAL_SOURCE_DL:              00349d38 extra-left TRI1
POSITIVE_X_ASPECT_SCALE_CAN_FLIP_WINDING: NO
CULL_WINDING_ANOMALY:         UNKNOWN (verts/signed area not in capture)
CURRENT_ROOT_CLASS:           WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY:        after GPU submit of 00349d38 extra-left TRI1
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-targeted-cull-winding-20260827T152926Z/
ARTIFACT_CHECKSUM_MANIFEST:   c260d0fd338f4b14039e65bc70a33601318682f7686ba5c72441cc18e0d2eb56
```

Canonical: [[p08-widescreen-edge-geometry]]. Aspect X-scale cannot invert
winding. SHADER_CULL is fixed-function FRONT cull after VS Y compensation.
Facing mismatch remains unmeasured.

`NEXT_SINGLE_ACTION:` TARGETED_FOCAL_VERTEX_WINDING_DUMP. No GPU readback. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior session (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY CPU_SCREEN_BOUNDS_WALL_CANDIDATE_ARBITRATION

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
FINAL_RENDER_TARGET_PIXEL_PROBE: NOT_COMPLETED / ABANDONED_FOR_SAFETY
GPU_PIXEL_READBACK_CURRENT_APPROACH: UNSAFE
LIVE_GPU_READBACK:            DO_NOT_RETRY_WITH_CURRENT_APPROACH
WIDE_EXTENT_PRESERVED_THROUGH_PRESENT: PROVEN
VIRENDERER_COMPOSITE_ROOT:    REFUTED as extra-band X crop
CAPTURE_VALID:                YES
CAPTURE_ID:                   1
CAPTURE_START_FRAME:          13309
CAPTURE_END_FRAME:            14028
CAMERA_EFFECTIVELY_STATIC:    YES (720/720) cam=(3467.4,105.0,4735.7)
USER_HOLE_NDC_RECT:           x[-1.00,-0.76] y[-0.30,1.00]
TOP_CANDIDATE_SOURCE_DL:      00349d38 extra-left TRI1
TOP_CANDIDATE_GPU_SUBMITTED:  YES
MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED
CULL_WINDING_ANOMALY:         UNKNOWN
CURRENT_ROOT_CLASS:           WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY:        after GPU submit of 00349d38 extra-left TRI1
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-user-wall-bounds-arbitration-20260827T151459Z/
ARTIFACT_CHECKSUM_MANIFEST:   cd50d98a55c0ab24bb1ed3d982ae4b648ba148b3bdc2ed0182f31189dd6018b4
```

Canonical: [[p08-widescreen-edge-geometry]].
Do not treat smoke `0031f6d8` as the user wall. Distant `0031e***` proj=1
planes are demoted. Nearby MAP_RAM `00349d38` extra-left TRI1 is the PARTIAL
semantic join and **does** reach GPU.

`NEXT_SINGLE_ACTION:` TARGETED_CULL_WINDING_TRACE. No GPU readback. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## Prior session (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY POST_SUBMISSION_COMPOSITE_STATE_TRACE

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
FINAL_RENDER_TARGET_PIXEL_PROBE: NOT_COMPLETED / ABANDONED_FOR_SAFETY
GPU_PIXEL_READBACK_CURRENT_APPROACH: UNSAFE
LIVE_GPU_READBACK:            DO_NOT_RETRY_WITH_CURRENT_APPROACH
CAPTURE_VALID:                YES
CAPTURE_ID:                   1
CAPTURE_START_FRAME:          6885
CAPTURE_END_FRAME:            7604
CAMERA_EFFECTIVELY_STATIC:    YES (720/720)
SMOKE_SUBMISSION_COUNT_DISCREPANCY_ROOT: ACCOUNTING_WINDOW_PLUS_ONE_DUPLICATE_GPU_SUBMIT
SMOKE_TRUE_SUBMISSION_JOIN_RATE: 100.0%
USER_EXTRA_BAND_SUBMISSION_JOIN_RATE: 100.0%
DRAW_TO_GPU_SUBMISSION_FAILURE: REFUTED
EXTRA_BAND_PROJ1_DRAWS_REACH_GPU_DURING_VISIBLE_GLITCH: PROVEN
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: NOT_PROVEN
MISSING_WALL_DRAW_SEMANTIC_JOIN: NO
COLOR_DEPTH_EXTENT_MATCH:     YES (2164x1140)
EXTRA_BAND_DEPTH_TARGET_TRUNCATION: REFUTED_FOR_USER_CAPTURE
NEAR_PLANE_INTERSECTION_COUNT: 0
FAR_DEPTH_CLIP_ANOMALY:       REFUTED
BANJO_NEAR_CLIPPING_REFERENCE_RELEVANCE: LOW
VI_UV_COVERS_WIDE_SOURCE:     PROVEN (CPU smoke)
POST_RENDER_4_3_CROP_PATH:    NOT_FOUND
WIDE_EXTENT_PRESERVED_THROUGH_PRESENT: PROVEN (smoke)
VIRENDERER_COMPOSITE_ROOT:    REFUTED as extra-band X crop; NOT_PROVEN as wall root
GPU_FRAGMENT_RASTER_ROOT:     NOT_PROVEN
COMPOSITE_CROP_ROOT:          STRONGLY_WEAKENED
ARBITRATION_CASE:             C (GPU submit healthy); composite crop not the extra-band hole
CURRENT_ROOT_CLASS:           POST_GPU_SUBMISSION_FRAGMENT_OR_COMPOSITE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY:        after GPU drawIndexedInstanced/drawInstanced
LAST_IDENTITY_PRESERVING_BOUNDARY: GPU_DRAW_SUBMISSION
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-post-submission-composite-audit-20260827T052315Z/
ARTIFACT_CHECKSUM_MANIFEST:   444e33af976f32ba2c4f94409a2210bf80871346281554e9891c66291bb2a2d2
```

Canonical: [[p08-widescreen-edge-geometry]].
Three Vulkan incidents (swapchain TRANSFER_SRC missing; MSAA vkCmdCopyImageToBuffer;
threadWorkload vs threadPresent) forced abandoning pixel probe. Composite path
preserves wide X (`uvMax.x=1`). Overclaim "missing wall reaches GPU" remains
forbidden: semantic join is NO. Next: SEMANTIC_WALL_DRAW_LOCALIZATION.

`NEXT_SINGLE_ACTION:` SEMANTIC_WALL_DRAW_LOCALIZATION. No GPU readback. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## Prior session (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY GPU_DEPTH_USER_ARBITRATION

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
USER_SELECTED_WIDESCREEN_MISSING_WALL: YES
CAPTURE_VALID:                YES
CAPTURE_ID:                   1
CAPTURE_START_FRAME:          6885
CAPTURE_END_FRAME:            7604
CAMERA_EFFECTIVELY_STATIC:    YES (720/720)
SMOKE_SUBMISSION_COUNT_DISCREPANCY_ROOT: ACCOUNTING_WINDOW_PLUS_ONE_DUPLICATE_GPU_SUBMIT
SMOKE_TRUE_SUBMISSION_JOIN_RATE: 100.0%
USER_EXTRA_BAND_DRAWS_CLASSIFIED: 56718
USER_EXTRA_BAND_DRAWS_CLASSIFIED_PROJ1: 3288
USER_EXTRA_BAND_SUBMISSION_JOIN_RATE: 100.0% (source_dl+prim_start+prim_end+proj)
DRAW_TO_GPU_SUBMISSION_FAILURE: REFUTED
GPU_SUBMITTED_EXTRA_BAND_SET_BEHAVIOR: STABLE
DEPTH_TEST_STATE_FOR_EXTRA_BAND: OFF (all 3288 proj1)
DEPTH_WRITE_STATE_FOR_EXTRA_BAND: OFF
DEPTH_COMPARE_FOR_EXTRA_BAND: ALWAYS_OR_OFF
EXTRA_BAND_DEPTH_STATE_DIFFERS_FROM_CENTRAL: NO (proxy: extra+central only; no extra-only proj1 GPU rows)
COLOR_TARGET_HOST_EXTENT:     2164x1140
DEPTH_TARGET_HOST_EXTENT:     2164x1140
COLOR_DEPTH_EXTENT_MATCH:     YES
EXTRA_BAND_DEPTH_TARGET_TRUNCATION: REFUTED_FOR_USER_CAPTURE
NEAR_PLANE_INTERSECTION_COUNT: 0
FAR_PLANE_INTERSECTION_COUNT: 411
FAR_DEPTH_CLIP_ANOMALY:       REFUTED
BANJO_NEAR_CLIPPING_REFERENCE_RELEVANCE: LOW
MISSING_WALL_DRAW_SEMANTIC_JOIN: NO
ARBITRATION_CASE:             C
CURRENT_ROOT_CLASS:           POST_GPU_SUBMISSION_FRAGMENT_OR_COMPOSITE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY:        after GPU drawIndexedInstanced/drawInstanced
LAST_IDENTITY_PRESERVING_BOUNDARY: GPU_DRAW_SUBMISSION
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-gpu-depth-user-arbitration-20260827T033147Z/
ARTIFACT_CHECKSUM_MANIFEST:   6996bd3bcf9974c86d95fc4650f08bc8b061faf407b5fa80ed39288aa94c2eca
```

Canonical: [[p08-widescreen-edge-geometry]].
Reference setup remains
`/tmp/p08-widescreen-extra-band-depth-visibility-setup-20260827T032333Z/`
(manifest `386063613e37c2b2272957489a0f0b740c05d803b8745503137dc126ab44d712`)
and is **not** user evidence. The setup 29498 vs 29502 count is accounting
(3 pre-window classified draws + 1 duplicate GPU submit). User proj=1 extra-band
primitive identity join is 100%. Prior `POST_RT64_DRAW_VISIBILITY_BOUNDARY` is
`CORRECTED/SUPERSEDED` to post-GPU-submit fragment/composite.

`NEXT_SINGLE_ACTION:` FINAL_RENDER_TARGET_PIXEL_PROBE (diagnostic-only extra-band color/depth target occupancy; no depth-test disable; no fix; no bypass).
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior session (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY EXTRA_BAND_FRAGMENT_DEPTH_VISIBILITY

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
POST_DRAW_CPU_REJECTION_FOUND: NO
DRAW_SURVIVES_TO_GPU_SUBMISSION: YES (100.0%)
EXTRA_BAND_DRAWS_CLASSIFIED:  29498
EXTRA_BAND_DRAWS_GPU_SUBMITTED: 29502
EXTRA_BAND_DRAW_SUBMISSION_RATE: 100.0%
DEPTH_TEST_STATE_CAPTURE_READY: YES
COLOR_DEPTH_EXTENT_MATCH:     YES
DEPTH_COORDINATE_SPACE:       HOST_WIDESCREEN_PIXELS / NDC
EXTRA_BAND_DEPTH_COVERAGE_MISMATCH: REFUTED
CLIP_STATE_CAPTURE_READY:     YES
BANJO_NEAR_CLIPPING_EFFECT_STAGE: RASTERIZER_DEPTH_CLIP_ENABLE
BANJO_NEAR_CLIPPING_CAN_EXPLAIN_LATERAL_HOLES: NO
FRAGMENT_VISIBILITY_DIRECTLY_OBSERVABLE: NO (proxy active)
PASS_IDENTITY_CAPTURE_READY:  YES
CURRENT_ROOT_CLASS:           POST_RT64_DRAW_VISIBILITY_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
USER_CAPTURE_READY:           YES
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-extra-band-depth-visibility-setup-20260827T032333Z/
ARTIFACT_CHECKSUM_MANIFEST:   386063613e37c2b2272957489a0f0b740c05d803b8745503137dc126ab44d712
```

Canonical: [[p08-widescreen-edge-geometry]].
Setup artifact: `/tmp/p08-widescreen-extra-band-depth-visibility-setup-20260827T032333Z/`
(manifest `386063613e37c2b2272957489a0f0b740c05d803b8745503137dc126ab44d712`).
Dual verification: PASS. All extra-band draws survive 100% to final GPU draw submission.
Depth extent matches color extent (100%). Depth mismatch is REFUTED.
Banjo near clipping affects only near-plane Z and does not explain lateral holes.

`NEXT_SINGLE_ACTION:` USER_SELECTED_AUTHORITATIVE_CAPTURE (user plays to visual anomaly with `TOOIE_DIAG_WS_CONTINUOUS=1` and triggers `touch /tmp/tooie-ws-capture`).
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## Prior session (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY RSP_DRAW_USER_ARBITRATION

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
USER_SELECTED_WIDESCREEN_VISIBILITY_ERROR: YES
CAPTURE_VALID:                YES (id=1 frames 245023..245742)
ARBITRATION_CASE:             B
RSP_TO_RT64_DRAW_CREATION_FAILURE: REFUTED
RT64_DRAW_PRESENT_DURING_VISUAL_ERROR: PROVEN (structural)
PROJ1_EXTRA_BAND_SET:         STABLE (8 persistent DLs; Jaccard 1.0)
CURRENT_ROOT_CLASS:           POST_RT64_DRAW_VISIBILITY_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY:        after RT64 extra-band DrawCall classification
LAST_IDENTITY_PRESERVING_BOUNDARY: RT64_HOST_DRAW_EXTRA_BAND_CLASSIFICATION
BANJO_NEAR_CLIPPING_REFERENCE_RELEVANCE: MEDIUM
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-rsp-draw-user-arbitration-20260827T030225Z/
ARTIFACT_CHECKSUM_MANIFEST:   2084f416c77a64082a459bf58244cc49a715972efefb454d131a441c13b914ed
```

Canonical: [[p08-widescreen-edge-geometry]].
User capture while the wall-hole was visible. RSP TRI* join 99.94%. Extra-band
draws exist on every world (proj=1) frame. Prior draw-creation root is
`CORRECTED/SUPERSEDED`. Do not reopen CAB70/80801174/focal `0x8035dd20`.

`NEXT_SINGLE_ACTION:` EXTRA_BAND_FRAGMENT_DEPTH_VISIBILITY. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior session (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY RSP_PRIMITIVE_IDENTITY_PROPAGATION

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
RSP_PRIMITIVE_PROPAGATION:    IMPLEMENTED_AND_VERIFIED
DL_CONTEXT_STACK_ACTIVE:      YES
DL_CONTEXT_EVENTS:            8597
PRIMITIVES_WITH_DL_IDENTITY:  76229
RT64_DRAWS_WITH_PRIM_ID:      45506
JOIN_RATE_DL_TO_PRIMITIVE:    100.0%
JOIN_RATE_PRIMITIVE_TO_DRAW:  100.0%
CURRENT_ROOT_CLASS:           RSP_TO_RT64_GEOMETRY_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY:        RSP_G_DL_TO_RT64_DRAW
LAST_IDENTITY_PRESERVING_BOUNDARY: RT64_HOST_DRAW_EXTRA_BAND_CLASSIFICATION
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-rsp-primitive-identity-setup-20260827T015059Z/
ARTIFACT_CHECKSUM_MANIFEST:   bb7ea94f1a8f71cb101c0cc4cb38832ccd5aa219240f2526669537eadcdd88e0
FIX_IMPLEMENTED:              NO
```

Canonical: [[p08-widescreen-edge-geometry]].
Setup artifact: `/tmp/p08-widescreen-rsp-primitive-identity-setup-20260827T015059Z/`
(manifest `bb7ea94f1a8f71cb101c0cc4cb38832ccd5aa219240f2526669537eadcdd88e0`).
End-to-end continuous identity propagation is proven working and verified.

`NEXT_SINGLE_ACTION:` USER_SELECTED_AUTHORITATIVE_CAPTURE (user plays to visual anomaly with `TOOIE_DIAG_WS_CONTINUOUS=1` and triggers `touch /tmp/tooie-ws-capture`).
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior session (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY POST_GCMAPSECTS_REANCHOR

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
USER_OBSERVED_WIDESCREEN_GLITCH_IN_RUN: YES
OVERLAY662_TOTAL_SYSCALLS:    39 (ep4=27 ep15=12 ep9/13/17=0)
GCMAPSECTS_ENTRY_COUNT:       0
GCMAPSECTS_80801174_REQUIRED_FOR_OBSERVED_SYMPTOM: REFUTED_FOR_THIS_RUN
EP4_FUNCTION:                 gcmapsects_entrypoint_4 (AABB point query)
FOCAL_DL_8035DD20_AS_CAUSAL_ANCHOR: WEAKENED
CURRENT_ROOT_CLASS:           RSP_TO_RT64_GEOMETRY_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY:        RSP_G_DL_TO_RT64_DRAW
FIX_IMPLEMENTED:              NO
```

Canonical: [[p08-widescreen-edge-geometry]].
Reanchor artifact: `/tmp/p08-widescreen-post-gcmapsects-reanchor-20260827T012256Z/`
(manifest `3c626fd6b1b1d44520550066ef7c34d6d0c2077bb26b0b8b29325623b33e0e37`).
Prior VALID capture 1717..2436 unchanged. No new USER_SELECTED capture.

`NEXT_SINGLE_ACTION:` RSP_PRIMITIVE_IDENTITY_PROPAGATION. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Prior session (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY FOCAL_BRANCH_JOIN_REPAIR

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
CAPTURE_VALID:                YES (1717..2436; no new user capture)
FOCAL_DL_FIRST_SEEN_FRAME:    1720
FOCAL_DL_LAST_SEEN_FRAME:     2432
FOCAL_DL_8035DD20_AS_CAUSAL_ANCHOR: WEAKENED
FOCAL_CLUSTER_TRANSITION:     MIXED_ASYMMETRIC_NEIGHBOR_DROPOUT
SECTION_BLOCK_ACTIVATION:     NOT_SUPPORTED
FOCAL_BRANCH_EVENTS_IN_WINDOW: 0
OLD_RA_GATE_ROOT_CAUSE:       RA is call-site+8 not 0x80801174; r31 not materialized; helper SP; LOOKUP misses CreateStatic; dispatch table address-takes EPs
DIRECT_GCMAPSECTS_HOOK:       YES (ld --wrap 80801174 + unlimited 662 syscall log + LOOKUP rebind)
GC_MAPSECTS_HOOK_VERIFIED:    NO (45s attract: one syscall ep=15 only)
BUILD_RESULT:                 PASS
FIX_IMPLEMENTED:              NO
CURRENT_ROOT_CLASS:           GUEST_MAP_SECTION_DL_SELECTION_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    OBSERVABILITY_REPAIR
CURRENT_ROOT_BOUNDARY:        func_80801174_gcmapsects
USER_CAPTURE_READY:           NO
```

Canonical: [[p08-widescreen-edge-geometry]].
Join-repair artifact: `/tmp/p08-widescreen-focal-branch-join-repair-20260827T004617Z/`
(manifest `2c84d1739211aff278e4e469f3af40e816640cc2f0304ff6493d62c9d1f19cfc`).
Prior VALID capture: `/tmp/p08-widescreen-focal-branch-user-arbitration-20260826T233105Z/`
(manifest `4b1867cb2145af8dc5c33b6916943ce335edf6295a886eaef9d37bddf87a7a65`).

`NEXT_SINGLE_ACTION:` Obtain `[ws-gcmapsects-entry] > 0` on the glitch map (attract does not tick ep 9/17). Then USER_SELECTED capture. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## Prior session (2026-08-27) — first ld --wrap install

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                    6f1c2d99a4ea571c139f449c326fd176ba8f3496
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
CAPTURE_VALID:                YES (1717..2436; no new user capture)
FOCAL_DL_FIRST_SEEN_FRAME:    1720
FOCAL_DL_LAST_SEEN_FRAME:     2432
FOCAL_DL_8035DD20_AS_CAUSAL_ANCHOR: WEAKENED
FOCAL_CLUSTER_TRANSITION:     MIXED_ASYMMETRIC_NEIGHBOR_DROPOUT
SECTION_BLOCK_ACTIVATION:     NOT_SUPPORTED
FOCAL_BRANCH_EVENTS_IN_WINDOW: 0
OLD_RA_GATE_ROOT_CAUSE:       RA is call-site+8 not 0x80801174; r31 not materialized; helper SP reconstruction; LOOKUP 0x80801174 misses CreateStatic and ram+offset
DIRECT_GCMAPSECTS_HOOK:       YES (ld --wrap)
GC_MAPSECTS_HOOK_VERIFIED:    NO (ENTRY_COUNT=0 in attract/TDL/Wooded smoke)
BUILD_RESULT:                 PASS
FIX_IMPLEMENTED:              NO
CURRENT_ROOT_CLASS:           GUEST_MAP_SECTION_DL_SELECTION_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    OBSERVABILITY_REPAIR
CURRENT_ROOT_BOUNDARY:        func_80801174_gcmapsects
USER_CAPTURE_READY:           NO
```

Canonical: [[p08-widescreen-edge-geometry]].
Join-repair artifact: `/tmp/p08-widescreen-focal-branch-join-repair-20260827T002831Z/`
(manifest `0cd05b6bbac330111b9bdd4a8fa86630b0b6dee02f968d9b568722723450b4d1`).
Prior VALID capture: `/tmp/p08-widescreen-focal-branch-user-arbitration-20260826T233105Z/`
(manifest `4b1867cb2145af8dc5c33b6916943ce335edf6295a886eaef9d37bddf87a7a65`).

`NEXT_SINGLE_ACTION:` Obtain `[ws-gcmapsects-entry] > 0` on a map that ticks ep 9/17, then USER_SELECTED capture. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## Prior session (2026-08-26) — WIDESCREEN_EDGE_GEOMETRY FOCAL_BRANCH_TIMELINE user arbitration

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
WORKTREE:                     DIRTY (preserve)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
USER_OBSERVED_LATE_POP_IN:               YES
USER_OBSERVED_EARLY_POP_OUT:             YES
BIDIRECTIONAL_EDGE_VISIBILITY_ERROR:     USER_OBSERVED
USER_MANUAL_CAB70_BYPASS_VISUAL_RESULT:  NO_CHANGE
USER_MANUAL_E3DC0_BYPASS_VISUAL_RESULT:  NO_CHANGE
USER_MANUAL_CACEC_BYPASS_VISUAL_RESULT:  NO_CHANGE
CAB70_DIRECT_VISUAL_ROOT:                REFUTED_FOR_OBSERVED_SYMPTOM
E3DC0_DIRECT_VISUAL_ROOT:                REFUTED_FOR_OBSERVED_SYMPTOM
CACEC_DIRECT_VISUAL_ROOT:                REFUTED_FOR_OBSERVED_SYMPTOM
SPHERE_CULLING_IS_VISUAL_ROOT:           REFUTED_FOR_OBSERVED_SYMPTOM
BBOX_VISUAL_ROOT:                        REFUTED_FOR_OBSERVED_SYMPTOM
FRUSTUM_ASPECT_MISMATCH_FOR_CAPTURED_EVENT: REFUTED
USER_TRIGGER_FOUND:                      YES
CAPTURE_COMPLETE_FOUND:                  YES
CAPTURE_VALID:                           YES
CAPTURE_ID:                              1
CAPTURE_START_FRAME:                     1717
CAPTURE_END_FRAME:                       2436
FOCAL_BRANCH_LOG_PRESENT:                YES (0 bytes)
FOCAL_BRANCH_EVENTS_IN_WINDOW:           0
FOCAL_SECTION_IDENTITY:                  UNKNOWN
FOCAL_SECTION_IDENTITY_STABLE:           UNKNOWN
FOCAL_DL_PHYSICAL:                       0x0035dd20
FOCAL_DL_KSEG0:                          0x8035dd20
FOCAL_DL_STORAGE_CLASS:                  LOADED_MAP_MODEL_PAYLOAD
FOCAL_DL_STATIC_OR_DYNAMIC:              DYNAMIC_RELOCATED
FOCAL_DL_SEMANTIC_CLASS:                 LEAF_DISPLAY_LIST / MAP_MODEL_PAYLOAD
FOCAL_DL_MAP_TERRAIN_BIND:               UNKNOWN
FOCAL_DL_FIRST_SEEN_FRAME:               1720
FOCAL_DL_LAST_SEEN_FRAME:                2432
FOCAL_DL_REAPPEARS:                      NO
LAST_SEEN_IS_CAPTURE_TAIL:               YES
FOCAL_DL_FROM_GCMAPSECTS:                UNKNOWN (pointer) / YES (class selector)
FOCAL_DL_FROM_GCCUBEDLL:                 NO
FOCAL_DL_FROM_GCSECTIONDLL:              NO
FOCAL_DL_CONTROLLED_BY_GCSECTIONSKIP:    UNKNOWN
FOCAL_DL_REACHES_800CAA24:               YES (static section path)
FOCAL_DL_LOD_CONTROLLED:                 NO
FOCAL_DL_EMITTER_FUNCTION:               UNJOINED
DYNAMIC_BRANCH_JOIN:                     NO
CAA24_CONTROLS_FOCAL_MEMBERSHIP:         UNKNOWN
SPHERE_CONTROLS_FOCAL_MEMBERSHIP:        UNKNOWN
POST_WALK_SELECTION_INVOLVED:            UNKNOWN
FOCAL_DL_8035DD20_AS_CAUSAL_ANCHOR:      WEAKENED
FOCAL_CLUSTER_TRANSITION:                MIXED_ASYMMETRIC_NEIGHBOR_DROPOUT
SECTION_BLOCK_ACTIVATION:                NOT_SUPPORTED
GUEST_DL_TO_RSP_CONTINUITY:              PROVEN
LAST_IDENTITY_PRESERVING_BOUNDARY:       RSP_HLE_G_DL_VISIT
IDENTITY_LOST_AT:                        RSP_G_DL_TO_RT64_DRAW
DL_TO_RT64_DRAW_JOIN:                    NO
CURRENT_ROOT_CLASS:                      GUEST_MAP_SECTION_DL_SELECTION_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:               STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY:                   func_80801174_gcmapsects (800E3E8C ∧ 800E3D0C/CAA24)
PRIOR_ROOT_CLASS:                        GUEST_DISPLAY_LIST_SELECTION_OR_EMISSION_BOUNDARY (REFINED, not REFUTED)
NEXT_DISCRIMINATOR:                      FOCAL_BRANCH_JOIN_REPAIR
FIX_IMPLEMENTED:                         NO
TEMP_DIAGNOSTIC_CHANGE_THIS_ROUND:       NO
```

USER_SELECTED **1717..2436** is VALID. Focal `0x8035dd20` is present on every
graphics-emit frame (1720..2432); no branch onset. `[ws-focal-branch]=0` while
geo helpers still log. CAA24/sphere membership control remains UNKNOWN. Do not
bypass CAA24. Do not reopen CAB70/E3DC0/CACEC.

Canonical: [[p08-widescreen-edge-geometry]].
Latest Branch Arbitration: `/tmp/p08-widescreen-focal-branch-user-arbitration-20260826T233105Z/`
(manifest `4b1867cb2145af8dc5c33b6916943ce335edf6295a886eaef9d37bddf87a7a65`).
Latest Producer: `/tmp/p08-widescreen-dl-producer-trace-20260826T150351Z/`
(manifest `40d591c62899a56b60f73f4e990f019b1b1890b22b4e4d6a33b41b6eba48f5ee`).
Latest DL-identity Arbitration: `/tmp/p08-widescreen-dl-framejoined-user-arbitration-20260826T143436Z/`
(manifest `f0f9b493cf4bdac6ebe2378f8ce5b04ce09f8f4eab34058cd58ff7fed6d80f54`).
Frame-join Setup: `/tmp/p08-widescreen-dl-framejoin-fix-20260826T130423Z/`
(manifest `4c650961c173fb7ad7a924fbaaf90abe1393cc3a9d1a53adfe41807d7442cb32`).

`NEXT_SINGLE_ACTION:` Repair observation of `func_80801174_gcmapsects` so `[ws-focal-branch]` emits on a live `0x8035dd20` frame; then recapture at pop-in. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

Prior artifacts (history): CACEC setup `/tmp/p08-widescreen-cacec-counterfactual-setup-20260826T054500Z/`; bidirectional `/tmp/p08-widescreen-bidirectional-visibility-20260826T044851Z/` (`f1b8621a90ef6ea2f25613313732dfe437c55e46fb3b324582b1f199c7f0c6f4`); FAIL `/tmp/p08-widescreen-user-group-join-20260826T042031Z/`; structural `/tmp/p08-widescreen-group-join-20260826T041000Z/`. Do not open P09. Do not reopen USER_DEMO_5. Do not reopen CAB70/E3DC0/CACEC as visual roots. No production frustum/LOD/room/RT64 patch.

## Prior session (2026-08-25) — class C telemetry — CORRECTED/SUPERSEDED as visual root

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
WORKTREE_AT_INVESTIGATION:    CLEAN
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
USER_DEMO5_STATUS:            RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX
P08_STATUS:                   ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                   PENDING / NOT_FORMALLY_OPEN
CURRENT_ROOT_CLASS:           C — CPU_FRUSTUM_VISIBILITY_CULLING_BOUNDARY
CURRENT_ROOT_BOUNDARY:        func_800CAB70
VISUAL_FIX_STATUS:            USER_VALIDATION_PENDING
FIX_IMPLEMENTED:              NO
```

The 2026-08-26 `POST_P08_ENHANCEMENT` widescreen label is
**CORRECTED/SUPERSEDED**.
Controlled counterfactual **telemetry** remains **PROVEN** via artifact
`/tmp/p08-widescreen-cull-counterfactual-20260825T225500Z/`
(manifest `c356a78d52344b43b440f07df4cdb951be6b672c16f485fcc382a9e6990f9057`).

- `LIVE_CAPTURED:` Guest 304x228, scissor (0,0,304,228). Baseline Expand culls 99.7% of bbox checks (36,066/36,172) including lateral terrain chunks like `min=(-1020,1980,-4520)`.
- `CAB70_BYPASS:` Forcing `func_800CAB70` return `1` (VISIBLE) turns 100% of bbox checks visible (2,679/2,679) and automatically propagates to `func_800E3DC0`.
- `CAB70_CAUSES_LATERAL_MISSING_GEOMETRY:` **CORRECTED/SUPERSEDED** — was telemetry-only; user visual `NO_CHANGE`.
- `E3DC0_BYPASS_NEEDED:` **`NO`** for the *dispatch* path; also **not** the visual root.
- Class C as current visual root is **CORRECTED/SUPERSEDED**.
- `FIX_IMPLEMENTED:` `NO` (temporary diagnostic env-gated bypass only; default runtime unchanged).

Draft `ar_option != Original` / copy-Banjo-demo-guard production spec remains
**CORRECTED/SUPERSEDED**. Do not implement that CAB70 patch as the visual fix.

## CURRENT_HEAD

- Branch: `main` @ `f1fdfefb3346e16bc813c106f0947c0d60aa228c` (walkstart capture). ROUND.W.S fix is `5854872c79a7cab1c9395ddcd173cece449ffe11`.
- ANGRYLION_CONTROL tree: `tools/oracle/angrylion-control/` (relocated from `/tmp/p08-m64p*`; [[../../specs/BT/angrylion-control-oracle]]). Playable: `./run-tooie-angrylion-rdp.sh`. RT64: `./run-tooie.sh`. TooieRecomp stays RT64.
- Agent workspace: open CLI/IDE in **TooieRecomp/**. MCP/vault configs are in-repo ([[../../MARKDOWN_VAULT_TOOLS]]). Sibling `BanjoRecomp` + cart ROM remain for CMake/oracle only.
- Provisional Player & AI Speed Parity Baseline: [[p08-provisional-player-ai-speed-parity]] (`PLAYER_SCALAR_SPEED_PARITY: PASS`, `AI_ACTOR_SPEED_PARITY: PASS`, `USER_DEMO4_CLASSIFICATION: CAMERA_DIRECTION_DEFECT_WITH_SPEED_PARITY`, `HISTORICAL_FIRST_STRAIGHT_SPEED: PROVEN_CLEAN`).
- Attract user-numbering / USER_DEMO_4 egg defect: [[p08-provisional-follow-camera-and-egg-differential]] (`EGG_PARITY_DEFECT: PROVEN`).
- USER_DEMO_4 post-ROUND.W.S full-state discriminator: [[p08-user-demo4-post-round-full-state-discriminator]] (idle CA8B4/CA628 glidx 1..38 MATCH; first remaining bit difference = dump glidx 39 `ncbapivot_entrypoint_3` output; player pos/yaw/ideal MATCH).
- USER_DEMO_4 walkstart ncbapivot join: [[p08-user-demo4-walkstart-ncbapivot-discriminator]] (`800137AC` a2/f14 **CORRECTED/SUPERSEDED**; first split is `80112C5C` on `p60_8` 0 vs 5; `ROOT_CLASS=B-NCBAPIVOT-CONTROL-INPUT`).
- USER_DEMO_4 CA8B4 first seed: [[p08-user-demo4-ca8b4-first-seed]] (pre-fix glidx-1 ULP **CORRECTED/SUPERSEDED** by ROUND.W.S).
- USER_DEMO_4 ncbapivot first tick: [[p08-user-demo4-ncbapivot-first-instruction]] (pre-fix n74 DIFF; **POST-FIX bind n74 MATCH**).
- USER_DEMO_4 live-eye writer: [[p08-user-demo4-live-eye-writer]] (node init MATCH; `80110770`/`800EE7F8` faithful; first bit write is ordinal 2 on the mode=2 bind ncbapivot tick).
- USER_DEMO_4 bind-tick follow-eye: [[p08-user-demo4-bind-tick-follow-eye]] (pre-fix A-CANDIDATE `006d5329` vs `006d5328`; **POST-FIX MATCH** `006d5328`). Live replay: [[p08-user-demo4-sincos-semantics-fix]].
- Active Fix Evidence: [[p08-attract-loop-after-demo2-crash]] (**RESOLVED and VALIDATED**).
- Preserved diagnosis: [[p08-demo-movement-precision-diagnosis]] §16 & §17 (first demo shot miss & target collision volume resolved).
- Active Fix Evidence: [[p08-stick-angle-thunk-fix]].
- Independent Review: [[p08-stick-angle-thunk-fix-rereview]] — **VERDICT A**; prior `$r1` delay-slot blocker resolved.
- Stick-angle diagnosis: [[p08-player-direction-facing-diagnosis]].
- Gameplay Demo Motion Sync Diagnosis: [[p08-demo-full-motion-sync-diagnosis]] (**MEASURED_ORIGINAL vs MEASURED_RECOMP**, 2026-08-16).
- Display & Effect Catalog: [[p08-display-effect-catalog]].
- Title & File Select Validation: [[p08-title-file-select-validation]].
- Render-submission fix: [[p08-render-submission-matrix-fix]].
- Directional input diagnosis: [[p08-directional-input-movement-diagnosis]].
- Diagnosis: [[p08-render-submission-matrix-diagnosis]].
- Discriminator: [[p08-actor-camera-transform-discriminator]].
- Movement diagnosis: [[p08-visible-demo-movement-diagnosis]].
- RA-wrapper fix: [[p08-core1-ra-wrapper-fix]].
- Overlay angle producer: [[p08-demo-camera-overlay-angle-producer]]
  (`ncbapivot_entrypoint_3` PROVEN).
- Camera eye producer differential: [[p08-demo-camera-eye-producer-differential]] (PROVEN).
- Bounded branch predicate differential: [[p08-demo-ncbapivot-branch-predicate]]
  (PROVEN Case C / Epistemic correction: branch 0x808001C0 not taken in both;
  gate divergence at 0x808001D0 via `func_801138A0` return value and `func_80113D30` clearance tests).
- Four-query clearance differential: [[p08-demo-camera-clearance-rays]]
  (**CORRECTED/SUPERSEDED** by [[p08-demo-camera-collision-sol-review]]:
  the measured `func_800E8918` return divergence is real, but `func_800C4C34`
  is projection/viewport clipping, not collision, and its direct `v0` was not
  the value captured by the trace).

## CURRENT_PHASE

P08 — RT64, input e frontend is **ACTIVE (MANUAL_VALIDATION_PENDING under Verdict B)**.
Canonical tasks (T01..T11) audited:
- T01..T06, T08..T11: **DONE** (T02 display list and effect catalog completed; [[p08-display-effect-catalog]])
- T07: **IMPLEMENTED_MANUAL_VALIDATION_PENDING** (`tooie_window` helpers verified via unit tests; physical display validation `PASS` by user confirmation; formal physical gamepad validation `PENDING_HARDWARE_UNAVAILABLE`)
- P08-T08 (Graphics driver / Vulkan failure handling): **DONE** (`tooie_error` reporting, `assert(false)` removal, negative env test PASS).
- Attract DEMO2 SIGSEGV (bsrest 0x808003FC): **RESOLVED and VALIDATED**.
- User physical display confirmation: **PASS** — all 14 mandatory display/window tests passed user validation.
- User physical gamepad status: **PENDING_HARDWARE_UNAVAILABLE** — no gamepad hardware currently available for formal 15/15 checklist execution (`USER_CONFIRMED_CONTROLLER_GAMEPLAY: YES` retained).
- Only remaining formal P08 gate: **PHYSICAL_GAMEPAD_VALIDATION**.
- Next phase: **P09 remains PENDING / NOT_FORMALLY_OPEN** until the P08 manual gamepad gate is completed.
- Provisional pre-P09 parity evidence: **CORRECTED/SUPERSEDED** by [[p08-provisional-follow-camera-and-egg-differential]] (`EGG_PARITY_DEFECT: PROVEN` and USER_DEMO_4 follow-camera **REPRODUCED**; Demo 0..3 is not the complete attract cycle).
- USER_DEMO_4 post-ROUND.W.S full-state join: [[p08-user-demo4-post-round-full-state-discriminator]] — idle follow-cam glidx 1..38 bit-identical. Walkstart join [[p08-user-demo4-walkstart-ncbapivot-discriminator]]: first remaining split is glidx 39 `80112C3C` jumptable on `*(node+0x60)+8` (0 vs 5 at ncbapivot enter; player pos/yaw/ideal MATCH). Demo-sync still prints 0.0853° @41 then feedback accumulation. Egg remains BLUE vs FIRE; independence from remaining camera is **UNKNOWN**. ROUND.W.S architectural fix is committed as `5854872c79a7cab1c9395ddcd173cece449ffe11` (`CAMERA_COMPLETE_FIX: NO`, `CAMERA_FEEDBACK_ACCUMULATION: STILL_PRESENT`). `glidx 36` / pre-fix glidx-1 seed are **CORRECTED/SUPERSEDED**. `800137AC` a2/f14 as first live input is **CORRECTED/SUPERSEDED**.
- The glidx-39 p60 producer is now bounded and joined as **B-P60-WRITER-INPUT**: RT64 `80112A84 → 80112D08` supplies `a1=0` from `p60+0xC=0`; the oracle supplies `a1=5` from `p60+0xC=1`. See [[p08-user-demo4-p60-writer-provenance]]. No production fix is authorized.

## CURRENT_PRODUCT_STATE

- **P08-T07 & P08-T08 VALIDATED**:
  - Safe window creation with dimension sanitization (>= 320x240) and high-DPI scaling calculations.
  - Multi-monitor / display index validation with fallback to primary display (0).
  - Aspect ratio calculations decoupled from host window size; 4:3 baseline preserved.
  - Controlled graphics/Vulkan startup failure reporting via user-visible message box and technical stderr diagnostics without crashes (`assert(false)` removed).
  - Dead duplicate state (`tooie_window::WindowState`) pruned to maintain clean single ownership in reference SDL/RT64 frontend.
- **`func_800BCF28` SIGSEGV RESOLVED and independently accepted (verdict B)**: Tooie-native helpers bound at `0x800BCCC0` and `0x800BCCD8`.
- Intro visual completa (céu, logo, Jiggy).
- Attract request correto; type-4 completa; warp consumido; mode-5 alcançado.
- `func_800C2AB8` is behaviorally resolved with PATCH4 default ON.
- **`func_80013A7C` lookup-zero RESOLVED** (Tooie-native RA wrappers; [[p08-core1-ra-wrapper-fix]]).
- **VISIBLE GAMEPLAY DEMO & RENDER SUBMISSION RESOLVED (2026-08-14):**
  - Incremental projection split implemented in `src/game/transform_hooks.cpp`.
  - RT64 avoids the erroneous unscaled `matrixDecomposeViewProj` path.
  - Gameplay demo full motion synchronization now has **MEASURED_ORIGINAL** ([[p08-demo-full-motion-sync-diagnosis]] §9). Official mupen64plus Pure Interpreter + Rice VI hook; ROM SHA-1 match; pad 100% on 765 samples. Same-index ~25u lead is one physics step (recomp dumps at glrecord consume; original at VI). After +1 shift the first walk, analog turn, and shot-stop match. Prior 45.11° camera-lag mismatch **CORRECTED/SUPERSEDED**. Remaining discriminator: post-egg run/jump after ~392. Level C (NO FIX).
  - Characters (`chjinjo` and `Banjo`) and camera are upright and elevated.
  - Banjo walks smoothly along ground path throughout 75s Wooded Hollow demo.
  - Host matrix regression test `matrix_submission_test` PASS (`MAX_MATRIX_ERROR: 0.000000e+00`).
  - Unit tests `bcf28_helpers_test` and `core1_ra_wrappers_test` PASS.
- **TITLE SCREEN / FILE-SELECTION VALIDATED (P08-T10 COMPLETE):**
  - Synthetic input sequencer tests startup intro -> file select
    (`chgameselect`, overlay 205) -> slot navigation -> `CONT_A` confirm.
  - Zero frontend input divergence. Post-confirm destination is
    `MAP_A1_CS_INTRO` (161) / `mode=3`, not live Spiral Mountain.
  - Overlay IDs 128/111/148 are `baeggcursor` / `baattach` / `bashoes`
    (old `chspiralmountain` / `chbanjohouse` / `chjinjo` labels
    **CORRECTED/SUPERSEDED**).
  - Later synthetic `mode=1` FAIL is **REFUTED**: the sequencer does not
    inject the later START presses that skip the opening story cutscenes.
- **ORIGINAL 4:3 & TIMING BASELINE VALIDATED (P08-T03 COMPLETE):**
  - Gated live timing counters prove exact 60.000 Hz VI event rate, 20/30 Hz logic rate, 30 Hz skeletal animation rate, decoupled host presentation (~935 Hz), and 4:3 content aspect.
- **ATTRACT DEMO2 CRASH RESOLVED (2026-08-16):**
  - Unnamed same-section overlay targets resolved via N64Recomp CreateStatic ordering fix.
  - 17 risky lookups -> 0; all 7 static bodies generated and verified.
  - 300s autonomous soak passed all transitions through CCL 314, CS Startup Intro 395, and beyond.
- `RecompiledFuncs` é gerado — não editar.

## CURRENT_STATUS / NEXT PHASE WORK

**P08 Status: ACTIVE (MANUAL_VALIDATION_PENDING under Verdict B).**
- Implementation is substantially complete with all automated tests passing.
- Physical display validation: **PASS** (user confirmed all 14 mandatory display tests).
- Remaining P08 exit gate before formal P09 open:
  1. Manual physical controller testing on real gamepad hardware (`PENDING_HARDWARE_UNAVAILABLE`).
- P08 Invariants (DO NOT REOPEN): attract DEMO2 crash, bsrest 0x808003FC, N64Recomp CreateStatic fix, first-straight ~510, fire-egg miss, cinematic facing, stick-angle 13B70, render-submission matrix, BCF28, core1 RA wrappers, PATCH 4 / preemption, P08-T10, original timing/4:3 baseline. Do not modify shared RT64. PATCH 4 stays ON.

## ROOT_CAUSE_STATUS

```text
C2AB8_ROOT_CAUSE_PROVEN:             YES (PATCH 4, CLASS A)
BCF28_ROOT_CAUSE_PROVEN:             YES (Level A; ACCEPTED verdict B)
80013A7C_ROOT_CAUSE_PROVEN:         YES (Level A)
80013A7C_FIX_STATUS:                 RESOLVED (Tooie-native strong symbols)
ACTOR_ROTATION_EXPLAINS_POSE:        NO (Actor.rotation is upright)
RENDER_SUBMISSION_FIX_STATUS:        RESOLVED (Level A; transform_hooks.cpp)
FRONTEND_INPUT_VALIDATION:           PASS
P08_T10_FRONTEND_INPUT:              PASS
DIRECT_SPIRAL_MOUNTAIN_TRANSITION_CLAIM: CORRECTED/SUPERSEDED
CURRENT_T10_REGRESSION:              REFUTED
MATRIX_EQUIVALENCE_TEST:             PASS
WOODED_HOLLOW_MODE5_VALIDATION:      PASS
STICK_ANGLE_13B70_FIX:               ACCEPTED (WASD runtime RESOLVED; analog PENDING)
DEMO_PATH_RESIDUAL:                  LEVEL D — first straight & first analog PASS
  First demo shot (glidx 322 fire-egg) TRUE_COLLISION_MISS_MARGIN = +36.85f (PROVEN NO HIT)
  Target collision center: (-5890.15, -504.61, 1601.84), radius = 113.00f
  World intercept: (-5912.92, -348.00, 1500.63) terrain triangle 0x8037bf94
  USER_PLAYER_SPEED_VISUAL_RESIDUAL: OPEN_SEPARATE
USER_BUG_B:                          CURRENTLY_NOT_REPRODUCIBLE_BY_USER
  GLOBAL_PROVEN_RESOLVED=NO
  Historical chdiggerground actor remains REFUTED
  ([[p08-forced-cinematic-facing-independent-review]]).
P08_T10_SYNTHETIC_FLOW_CURRENT:      REFUTED as a current-HEAD regression;
                                     sequencer never injects later START skips
ATTRACT_DEMO2_LOOP_CRASH:            PROVEN Level A
  bsrest_entrypoint_11 LOOKUP_FUNC(0x808003FC) = baflameline_entrypoint_5
  T1 335 and T2 278 succeed; T3 314 init SIGSEGV EXIT=139
```

## Independent acceptance review — 2026-08-16 (GPT-5.6 Luna, X-HIGH)

The fix is accepted with a non-blocking runtime-environment caveat. The exact
expected repository head was present, the durable patch base/hash contract was
verified, and a clean regeneration reproduced the intended output. The policy
unit test executed all four requested JAL cases (`T11 PASS`, `pass=75 fail=0`).

```text
BASE_HEAD:                                  1f2aba14665c064ce5af829955f1ac089792de77
PATCH_BASE_CORRECT:                         YES
PATCH_HASH_CORRECT:                         YES
PATCH_APPLICATION_CONTRACT:                 PASS
UNNAMED_SAME_SECTION_CREATESTATIC:          PASS
CROSS_SECTION_POLICY_PRESERVED:             YES
NON_RELOCATABLE_POLICY_PRESERVED:           YES
HOST_HOOK_POLICY_PRESERVED:                 YES
CREATESTATIC_BOUNDARIES:                    7/7 PASS
BSREST_808003FC_VERDICT:                    A
STATIC_HELPER_SPOTCHECK_1:                  8080020C — PASS (72/72, branches/JALs bounded)
STATIC_HELPER_SPOTCHECK_2:                  8080032C — PASS (52/52, branches/JALs bounded)
TOTAL_AUDITED_CALLS:                        17
DIRECT_CORRECT_STATIC:                      17
WRONG_STATIC:                               0
CALL_MISSING:                               0
LOOKUP_REMAINS:                             0
STATIC_TEST_STRENGTH:                       B
STATIC_TEST_FALSE_PASS_RISK:                LOW
TEST_STRENGTHENED:                          YES (test-only)
GENERATOR_POLICY_TEST_EXECUTED:              YES
GENERATOR_POLICY_CASES:                      4/4
CUMULATIVE_PATCH_PRIOR_CONTENT_PRESERVED:    YES
UNRELATED_GENERATOR_SEMANTICS_CHANGED:      NO
REGENERATED_LOCAL_BODIES:                   7/7
REGENERATED_DIRECT_CALLS:                   17/17
REGENERATED_VULNERABLE_LOOKUPS:             0
ATTRACT_STATIC_TEST:                         PASS
STICK_ANGLE_TEST:                            PASS
MATRIX_SUBMISSION_TEST:                     PASS
INPUT_DIRECTION_TEST:                       PASS
BCF28_TEST:                                  PASS
CORE1_RA_TEST:                               PASS
TOOIE_BUILD:                                 PASS
```

The short independent LVP/Xvfb run launched and reached `T_INTRO`, with no
SIGSEGV, but timed out before CCL; the Radeon/Xvfb path lacked DRI3 presentation.
The earlier 300-second Vulkan soak remains implementation evidence. For this
independent run: `CCL_REACHED=NO`, `BSREST_EP11_REACHED=NO`,
`CORRECT_LOCAL_TARGET=UNKNOWN`, `BAFLAMELINE_WRONG_BIND=UNKNOWN`,
`INTRO_395_REACHED=NO`, `EXIT_139=NO`.

```text
FIX_VERDICT:                                B
FIX_ACCEPTED:                               YES
ATTRACT_DEMO2_CRASH:                        RESOLVED
MARKDOWN_VAULT_AUDIT:                       PASS (0 errors; pre-existing warnings/tasks remain)
NEXT_SINGLE_ACTION:                         return to the next P08 residual
```

## RESOLVED_DO_NOT_REOPEN

1. Overlay data relocation / `LOOKUP_FUNC(0)`.
2. Actor manager null from artificial spawn.
3. `func800D6E54` as population pass.
4. Resident overlay / gcsection stale mapping.
5. Scheduler queue insert/remove / `No threads left` (without new proof).
6. IPL3 integrity gate.
7. R_MIPS_26 same-section / `do_dma` SIGSEGV.
8. Type-4 incomplete / warp gated as clean-freeze blocker.
9. Warp not consumed as clean-freeze blocker.
10. `func_80026500` list walk as current hang PC.
11. `func_800A907C -> func_8001EE34` as object driver — REFUTED.
12. "table[2] object NÃO está agendado" — REFUTED.
13. C2AB8 cooperative freeze as current blocker — RESOLVED by PATCH 4.
14. Hybrid F' architecture / PATCH 1 / 2 / 3 / 3.5 internals.
15. `func_800BCF28` SIGSEGV / `0x800BCCC0` lookup_gap_stub — RESOLVED and accepted.
16. `func_80013A7C` LOOKUP_FUNC(0) / core1 RA wrappers — RESOLVED.
17. Actor.rotation as root cause of supine pose — REFUTED (Actor.rotation is upright).
18. Tooie render submission matrix split — RESOLVED.
19. Title screen / file-selection input flow — VALIDATED (No divergence).
20. P08-T03 original timing/4:3 baseline — BLOCKED; no claim of baseline
    correctness until the clean live measurements exist.

Do not reopen these without new evidence.

## NEXT_SINGLE_ACTION
Identify the relocated overlay that calls `func_80110790` (ORIGINAL RA
`0x801D3E7C` at consumed 384) and trace who writes its `a1` stack vector
(`0x80045638`) before the setter. Mode 2 / `func_80110588` is PROVEN.
`func_80110790` is a copier; its `a1[1]` already differs (−0.0675°).
Do not reopen E1A58 or FE14/CA628. See [[p08-demo-camera-mode-producer-differential]].

## P08 FORCED FACING INDEPENDENT REVIEW (2026-08-16)

```text
SELECTED_ACTOR:        Wooded Hollow chdiggerground return
RETURN_STATE:          0x98bb1942
RETURN_ANIMATION:      0xc3aa1065 -> 0xc3ab8000
MOVEMENT_INTENTION:    PROVEN — position integrates backward (294.21°);
                       original case 11 never writes unk54/rotation.y
EXPECTED_FACING:       PRESERVE_PRIOR_YAW (original case-11 MIPS)
NODE_DISCRIMINATOR:    RESOLVED — func_80802760 returns node 0 (B_LIVE_NODE)
                       on all 3438 calls; A_KEEP_OLD (-1) REFUTED (0)
ROOT_CAUSE_LEVEL:      R
USER_BUG_B_THIS_ACTOR: REFUTED
USER_BUG_B_GLOBAL:     OPEN
FIX_IMPLEMENTED:       NO
MARKDOWN_VAULT_AUDIT:  PASS_WITH_PREEXISTING_WARNINGS (errors=0; 50 warnings
                       across 118 files)
```

```text
ORIGINAL_CASE11_WRITES_YAW:       NO
ORIGINAL_CASE11_WRITES_UNK54:     NO
GENERATED_CASE11_MATCHES_ORIGINAL: YES
DOES_ORIGINAL_CASE11_ADVANCE_POSITION_ALONG_RETURN_DIRECTION: YES
DOES_ORIGINAL_CASE11_UPDATE_FACING_TO_RETURN_DIRECTION:       NO
ORIGINAL_REVERSE_WALKS_BACKWARD:  YES
ORIGINAL_RETURN_STATE_DISPATCH_MATCHES_RECOMP: YES
DEEPSEEK_GENERATED_MATCH_CLAIM:  CORRECTED/SUPERSEDED
FUNC_80802760_GENERATED_MATCH_ORIGINAL: UNKNOWN
FIRST_OBSERVED_MECHANISM:         case-11 return movement with preserved yaw
ROOT_CAUSE:                       original actor/state contract; no recomp defect
USER_BUG_B_GLOBAL:                OPEN
PREVIOUS_NEXT_SINGLE_ACTION:      select a different user-confirmed actor
                                  (CORRECTED/SUPERSEDED by the unnamed-JAL review)
```

Evidence: [[p08-forced-cinematic-facing-independent-review]].

```text
REVIEW_CHECKPOINT_COMMIT: 902348f
REVIEW_CHECKPOINT_PUSHED: YES
DEEPSEEK_DISCRIMINATOR_COMMIT: (this checkpoint; docs-only)
DEEPSEEK_DISCRIMINATOR_PUSHED: YES
```

## FILES_TO_READ_NEXT

00. [[p08-t07-t08-frontend-system-validation]] — **P08-T07 & P08-T08 VALIDATED**: window/display, aspect ratio, graphics failure handling
0. [[p08-attract-loop-after-demo2-crash]] — **RESOLVED and ACCEPTED** DEMO2 attract SIGSEGV (Level A, verdict B)
0. [[p08-demo-movement-precision-diagnosis]] — first demo shot miss §16 (preserved)
0. [[p08-forced-cinematic-facing-diagnosis]] — USER_BUG_B historical; currently not reproducible
0a. [[p08-stick-angle-thunk-fix]] — stick-angle secondary-entry fix
0a. [[p08-stick-angle-thunk-fix-rereview]] — verdict A; blocker resolved
0aa. [[p08-stick-angle-thunk-fix-independent-review]] — historical verdict C
0b. [[p08-player-direction-facing-diagnosis]] — Level A diagnosis
0c. [[p08-directional-input-movement-diagnosis]] — host→pad (not reopened)
1. [[p08-render-submission-matrix-fix]] — render matrix submission fix
2. [[p08-render-submission-matrix-diagnosis]] — render matrix diagnosis
3. [[p08-actor-camera-transform-discriminator]] — actor vs camera discriminator
4. [[p08-visible-demo-movement-diagnosis]] — visible demo + movement diagnosis
5. [[p08-core1-ra-wrapper-fix]] — RA-wrapper fix (resolved; do not reopen)
6. [[p08-demo-camera-control-divergence]] — player-control camera writer/selector (Level B closed)
6a. [[p08-demo-camera-e1a58-differential]] — E1A58 REFUTED
6b. [[p08-demo-camera-mode-producer-differential]] — mode 2 / func_80110588 PROVEN; 80110790 a1 already differs at 384
6c. [[p08-demo-projection-record-34-differential]] — +0x34 writer B-INPUT; a3 join later corrected
6d. [[p08-demo-projection-v1-direction-producer]] — v1[2] producer; a3 inequality CORRECTED/SUPERSEDED
6e. [[p08-demo-projection-classifier-rejoin]] — Q1 +0x34 real; B-STATE func_8001A0C0 v0
6f. [[p08-user-demo4-ca8b4-first-seed]] — USER_DEMO_4 glidx-1 CA8B4 identity / rec init MATCH
6g. [[p08-user-demo4-ncbapivot-first-instruction]] — USER_DEMO_4 ncbapivot ep3 enter: node+0x74 already differs
7. `docs/HANDOFF.md`, `docs/plans/BT/08-rt64-input-and-frontend.md`

## CHECKPOINT SCOPE

```text
CODE_CHANGED             = YES (apply_runtime_patches.sh hash, attract static test)
RUNTIME_CODE_CHANGED     = NO (N64ModernRuntime core untouched)
GENERATOR_CODE_CHANGED   = YES (N64Recomp recompilation.cpp CreateStatic ordering fix via patch)
PATCH_CODE_CHANGED       = YES (patches/runtime/n64recomp-tooie.patch)
RECOMPILEDFUNCS          = REGENERATED CLEANLY (not hand-edited)
```

## P08 unnamed overlay JAL fix & validation (2026-08-16, Gemini 3.7 Flash HIGH)

`bsrest_entrypoint_11` at `0x80800668` JALs to unnamed `.bsrest` `0x808003FC`.
The root cause was Level A: `resolve_jal` previously fell back to
`use_lookup_for_all_function_calls` (Ambiguous -> `LOOKUP_FUNC`), binding to
`baflameline_entrypoint_5` due to shared overlay VRAM.

**Fix Implemented:** In N64Recomp `resolve_jal`, for relocatable sections, if
`in_current_section` is true and no existing function symbol matched in this
section, `JalResolutionResult::CreateStatic` is returned before the lookup-all
fallback. All 7 audited `.bsrest` unnamed targets (`0x80800000`, `0x8080003C`,
`0x80800080`, `0x808000E4`, `0x8080020C`, `0x8080032C`, `0x808003FC`) now
generate local static bodies in `funcs_416.c`, and all 17 vulnerable same-section
JAL sites call them directly without `LOOKUP_FUNC`.

**Validation:**
- `tests/attract_demo2_transition_jal_test.py`: **PASS** (17 lookups -> 0, 7/7 bodies present, semantics match original MIPS).
- Generator unit test in `PreemptionSafepointTest` (`test_jal_resolution_policy`): **PASS** (75/75 assertions).
- All native unit tests (`stick_angle_thunk_test`, `matrix_submission_test`, `input_direction_test`, `bcf28_helpers_test`, `core1_ra_wrappers_test`, `mips_rounding_test`): **PASS**.
- Full 300s autonomous attract soak with Vulkan: **PASS** (completed transitions `INTRO → 335 Wooded Hollow → 278 TDL → 314 CCL → 395 CS Intro → INTRO → 235 Mayahem → 296 JRL`, `EXIT_139=NO`, `UNEXPECTED_SIGSEGV=NO`).

```text
ROOT_CAUSE_LEVEL: A
FIX_OWNER: N64RECOMP
CREATESTATIC_ORDERING_FIX: IMPLEMENTED in patches/runtime/n64recomp-tooie.patch
PATCH_SHA256: adf97c9dfe20220143939ca1aa8728afc14a4eff957e35d8d2352ef027b98dc7
VULNERABLE_LOOKUPS_BEFORE: 17
VULNERABLE_LOOKUPS_AFTER: 0
GENERATED_LOCAL_BODIES: 7/7
BSREST_808003FC_GENERATED_MATCH_ORIGINAL: YES
ATTRACT_STATIC_TEST: PASS
TOOIE_BUILD: PASS
WOOD_HOLLOW_REACHED: YES
TDL_REACHED: YES
CCL_REACHED: YES
CCL_MODE2_PLAYER_INIT_COMPLETED: YES
BSREST_EP11_EXECUTED: YES
CORRECT_BSREST_808003FC_TARGET_EXECUTED: YES
WRONG_BAFLAMELINE_BIND_COUNT: 0
STARTUP_INTRO_395_REACHED: YES
SURVIVED_OLD_CRASH_POINT: YES
EXIT_139: NO
ATTRACT_RUNTIME_SECONDS: 300s
ATTRACT_TRANSITIONS_COMPLETED: 7
UNEXPECTED_SIGSEGV: NO
FIX_IMPLEMENTED: YES
FIX_VALIDATED: YES
```

## P08 post-egg movement divergence — independent narrow differential (2026-08-17)

The next discriminator was isolated without a production change. The fresh
Recomp run used the same attract setup and the reused official original trace;
input bytes matched `765/765`.

```text
PHASE_CORRECTED_ALIGNMENT:        ORIGINAL[N] vs RECOMP[N+1]
FIRST_EXACT_DIFFERENCE:           glidx 385 ideal_yaw 99.38 vs 99.31
FIRST_MEANINGFUL_DIFFERENCE:      glidx 392 position XZ error 2.997
FIRST_BAD_FIELD:                  ideal_yaw
STATE_SEQUENCE:                   bs21 and later bs8 match at persistent split
TSPD/MULTIPLIER:                  700/700 and 1.0000/1.0000
WRITER_PC:                        UNKNOWN on original and Recomp
ROOT_CAUSE_LEVEL:                 C
FIX_IMPLEMENTED:                  NO
```

This is a turn drift that grows into a jump-trajectory drift. The camera is
downstream, the actor control scan is UNKNOWN, and the later split occurs after
the already-fired first egg; the first shot miss remains closed. See
[[p08-demo-post-egg-motion-divergence]].

```text
NEXT_SINGLE_ACTION: obtain a bounded original/Recomp guest-store trace for the
                    ideal_yaw writer only; do not change production behavior.
```

## P08 ideal-yaw writer differential — RESOLVED (2026-08-17)

The exact ideal_yaw writer and its input provenance are now PROVEN by a
bounded original/Recomp store trace
([[p08-demo-ideal-yaw-writer-trace]]). The original Pure-Interpreter build was
rediscovered at `/tmp/p08-m64p` and rebuilt locally (no sudo) with a narrow
store tracer; the Recomp writer was captured via a GDB breakpoint on
`yaw_setIdeal`.

```text
MUPEN_SOURCE_FOUND:               YES (/tmp/p08-m64p; core 6dca4c1, rice 038882d)
MUPEN_REBUILT:                    YES (local, no sudo)
ROM_MATCH:                        YES (af1a89e12b638b8d82cc4c085c8e01d4cba03fb3)
ORIGINAL_IDEAL_WRITER_PCS:        {0x8009C930}  (yaw_setIdeal store)
RECOMP_IDEAL_WRITER_PCS:          {0x8009C930}  (same generated function)
WRITER_PC_MATCH:                  YES
IDEAL_YAW_WRITER_FUNCTION:        yaw_setIdeal (0x8009C914)
CALLER_FUNCTION:                  func_8009D088 (case 1; jal at 0x8009D0EC)
WRITER_INPUT_ALREADY_DIFFERS:     YES — glidx 385 a1 = 99.3756 vs 99.3081
RAW_PAD_MATCH:                    YES
REMAP_MATCH:                      YES
STICK_MAG_MATCH:                  YES
STICK_ANGLE_MATCH:                YES (bang identical at every glidx)
PLAYER_CONTROL_CAMERA_MATCH:      NO (first differs glidx 385)
PLAYER_CONTROL_CAMERA_SOURCE:     D_80127708[*(ps+0x184)*4] + 0x1C
SAME_AS_PREVIOUS_DISPLAY_CAMERA:  NO (display D_8012D500 departs at 386)
CAMERA_DIVERGENCE_REL_TO_PLAYER:  UPSTREAM_OF_PLAYER (CORRECTED/SUPERSEDED
                                  from AFTER_PLAYER for the control source)
PRIMARY_CLASS:                    SAME_WRITER_DIFFERENT_INPUT
                                  (first divergent input = CONTROL_CAMERA_INPUT)
INITIAL_0P07_DELTA_CLASS:         PROVEN_UPSTREAM_DIVERGENT_VALUE (writer input
                                   differs; earlier selector/internal camera
                                   state is not excluded)
EARLY_TURN_STORE_TRACE_CONTROL:   PASS (glidx 74..91 delta 0.0000)
ROOT_CAUSE_LEVEL:                 B (identical writer; stick path refuted;
                                  camera-angle writer instruction remains)
FIX_IMPLEMENTED:                  NO
```

The stick path (`bastick_update → func_80013B70`) is REFUTED as the first
divergent input. Independent review accepts the writer provenance with a
bounded correction: `WRITER_SET_COMPLETE=UNKNOWN`,
`ORIGINAL_INDEX_MATCH_RECOMP=UNKNOWN`, and the control/display-camera
dependency is not proven. The next discriminator is the aligned selector and
complete-width store trace for `D_80127708[index]+0x1C`. No production change
is authorized.

## P08 player-control camera divergence — RESOLVED to Level B (2026-08-17)

The selector/store-set/internal-state discriminator was executed
([[p08-demo-camera-control-divergence]]). The selector `ps+0x184` is **0 in
both runtimes at every sample**; the selected `D_80127708[0]` record is the
same logical camera-control object; the camctrl store set is complete
(`{0x800CA65C}` = `func_800CA628` only, no alias/block write); the camctrl
writer input first diverges at glidx 385 (`-0.0676` deg); the camera delta
maps 1:1 onto the ideal_yaw delta.

```text
MUPEN_ENV_REUSED:               YES (/tmp/p08-m64p; core 6dca4c1, rice 038882d)
PS_0X184_FIELD_SIZE:            4 bytes (s32; camera-control slot selector)
PS_0X184_SEMANTIC:              index into D_80127708 (8-entry pointer table of
                                player-control camera records)
ORIGINAL_SELECTOR_SEQUENCE:     0 at every writer call (367..405)
RECOMP_SELECTOR_SEQUENCE:       0 at every demo sample (1..480)
SELECTOR_MATCH:                 YES
D_80127708_KIND:                8-entry pointer table (.space 0x20, stride 4)
TABLE_SLOT_MATCH:               YES (index 0 in both)
SELECTED_RECORD_SEMANTIC_MATCH: YES (camera-control record; +0x1C = player yaw)
CAMCTRL_PLUS_0X1C_SEMANTIC:     player-control camera angle consumed by
                                bastick_getAngleRelativeToBanjo
CAMCTRL_DIRECT_STORE_PCS_ORIGINAL: {0x800CA65C} (func_800CA628, swc1 $f6,0x14($v1))
CAMCTRL_DIRECT_STORE_PCS_RECOMP:   func_800CA628 (same generated MIPS)
CAMCTRL_STORE_SET_COMPLETE:     YES (all widths + overlap detection; only
                                0x800CA65C touches selected+0x1C)
ALIAS_OR_BLOCK_WRITE_RISK:      LOW (aligned8_memcpy overlaps ps+0x184 but never
                                cam+0x1C)
CAMCTRL_WRITER_MATCH:           YES
CAMCTRL_PRIMARY_WRITER_PC:      0x800CA65C
CAMCTRL_PRIMARY_WRITER_FUNCTION: func_800CA628 (0x800CA628)
CAMCTRL_PRIMARY_WRITER_CALLER:  func_8010FC38 (JAL 0x8010FCD8) and
                                func_8010FE14 (JAL 0x8010FE1C)
EARLY_CAMERA_WRITER_CONTROL:    PASS (glidx 74..91; camctrl tracks <0.005 deg)
FIRST_INTERNAL_CAMERA_EXACT_DIFFERENCE:   glidx 385 (camctrl +0x1C -0.0676 deg)
FIRST_INTERNAL_CAMERA_MEANINGFUL_DIFFERENCE: glidx 385
FIRST_INTERNAL_DIVERGENT_FIELD: camctrl (+0x1C); eye +0x00 in same write
FIRST_INTERNAL_DIVERGENT_GLIDX: 385
CAMCTRL_WRITER_INPUT_ALREADY_DIFFERS: YES
FIRST_BAD_CAMERA_INPUT:         camctrl angle (src2[1] -> cam+0x1C)
                                CORRECTED/SUPERSEDED from src2[2]
FIRST_BAD_CAMERA_INPUT_SOURCE:  CORRECTED/SUPERSEDED — FE14/CA628 during
                                func_8010FC38, before CA8B4/E1A58
                                ([[p08-demo-camera-e1a58-differential]])
CONTROL_DISPLAY_CAMERA_RELATION: SHARED_UPSTREAM_STATE (no direct dependency
                                proven; display departs 386)
GLIDX385_CLASS:                 FIRST_CAUSAL_CAMERA_STATE
PLAYER_DRIFT_CHAIN_CONFIRMED:   YES (d_camctrl == d_ideal to 0.0001)
PRIMARY_CLASS:                  SAME_WRITER_DIFFERENT_INPUT
                                (first divergent input = CONTROL_CAMERA_INPUT)
FIRST_BAD_STAGE:                camctrl write at func_800CA628, glidx 385
FIRST_BAD_FUNCTION:             CORRECTED/SUPERSEDED — not E1A58 (identity).
                                FE14/CA628 during FC38 mode update
FIRST_BAD_GUEST_PC:             0x800CA65C SAME in both; src2[1] already differs
ROOT_CAUSE_LEVEL:               R (func_800E1A58 REFUTED;
                                [[p08-demo-camera-e1a58-differential]])
FIX_OWNER_CANDIDATE:            UNKNOWN (FC38 mode-update helpers)
FIX_IMPLEMENTED:                NO
```

## P08 func_800E1A58 differential — REFUTED (2026-08-17)

Function-level original/Recomp pipeline at consumed 70..95 and 370..410
([[p08-demo-camera-e1a58-differential]]). `func_800E1A58` is identity on both
sides (all 10 constraint slots type 1). The first meaningful camctrl is
already `src2[1]` at the FE14/`func_800CA628` write during `func_8010FC38`,
before CA8B4/E1A58 (consumed 384 = prior aligned 385, −0.0675°).
`cam+0x1C` is `out2[1]`, not `out2[2]`. Call alignment 1/1/1/2 PASS. Early
control PASS (`|Δ|≤0.0007°`). Level R. No production change.

## P08 FC38 mode producer — Level B (2026-08-17)

Mode 2 / `func_80110588` is **PROVEN**. `func_80110790` `a1[1]` already
differs at consumed 384 (−0.0675°). Caller is **PROVEN** `ncbapivot`
(781) / `ncbapivot_entrypoint_3` (RA offset `0x24C`). dest[1] =
`func_80013B70(eye.x-tgt.x, eye.z-tgt.z)`; inputs already differ at 384.
See [[p08-demo-camera-overlay-angle-producer]] and
[[p08-demo-camera-13b70-inputs]]: `EYE_INPUT_MATCH=NO` (grows at 384),
`TARGET_INPUT_MATCH=NO` (stable background). Causal component is eye;
last writer `func_80112E98`.

## P08 camera collision attribution — REFUTED (2026-08-18, GPT-5.6 Sol X-HIGH)

Independent original-MIPS and generated-path review
([[p08-demo-camera-collision-sol-review]]) proves that `func_800C4C34` is a
camera-space projection and viewport/depth clipping helper. It has no
broadphase, collision candidates, triangles, masks, or intersection helper.
`func_800E8918` returns `lh selected_record+0x34`, not the direct `v0` from
`func_800C4C34`; historical `q_hit_v0` / CLEAR / HIT labels are
**CORRECTED/SUPERSEDED**. The counter-to-camera causal chain remains
**PROVEN**. Q1 and Q2 already differ at the first measured relevant invocation,
glidx 70; glidx 384 is the first visible effect. Root level `R`; no fix.

`NEXT_SINGLE_ACTION`: trace the selected `func_800E8918` record identity and
the first differing writer/input to its `+0x34` halfword, beginning with the
candidate writer region in `func_800E82EC` (`0x800E851C..0x800E866C`).

## P08 projection record +0x34 differential — B-INPUT PROVEN (2026-08-19)

Full record-identity / store-provenance differential
([[p08-demo-projection-record-34-differential]]). The `func_800E8918` return
(`lh selected record+0x34`) already differs at the first relevant invocation
(Q1 glidx 70: Original 0 vs Recomp 1; Q0 control 0 vs 0 — PASS). The causal
writer is `func_800E82EC` (mode==1) in both runtimes, same logical record and
identical shared inputs (`a0=trunc(rec+0)=176`, `a1=trunc(rec+4)=161`,
`a2=v1[1]=0x3f820408`), but the classifier `func_800E81E0` receives a
causally different `a3 = v1[2]` (147.82677 original vs 147.41731 recomp), so
`f0(E81E0)` = -73.34 (orig) vs -1264.7 (recomp), flipping `f20 = rec+8 + f0`
and hence `+0x34` = 0 vs 1.

```text
B-INPUT (PROVEN)             NO FIX
E82EC same-writer:           YES
E8918 final return:          0x800E8A10 lh v0, 0x34(s0)
Q1 glidx70 rec+0x34:         ORIG 0 / RECOMP 1
Q0 control:                  PASS (0/0)
a3=v1[2] orig/recomp:        147.82677 / 147.41731
f0(E81E0) orig/recomp:       -73.34 / -1264.7
f20=rec+8+f0:                +947 / -244
a3 source:                   D_8012D600+phi*60+sub*12+8 (E82EC first loop,
                             func_800A8984/func_800CA6D4 direction vectors)
MUPEN_REUSED:                YES (core 6dca4c1, rice 038882d; REQUIRED --emumode 0
                             Pure Interpreter; Cached Interpreter does NOT carry the
                             pure_interp hooks)
ROM_MATCH:                   YES (af1a89e12b638b8d82cc4c085c8e01d4cba03fb3)
FIX_IMPLEMENTED:             NO
```

## P08 projection v1[2] direction producer — join CORRECTED (2026-08-20)

[[p08-demo-projection-v1-direction-producer]]. Aligned Q1 `a3` matches 26/26.
The `0x4313D3A7` vs `0x43136AD5` pair is consecutive stream values on both
runtimes. That inequality is **CORRECTED/SUPERSEDED**.

## P08 projection classifier rejoin — B-STATE (2026-08-20)

[[p08-demo-projection-classifier-rejoin]]. Q1 `+0x34` 0 vs 1 **survives**
correct join (25/25). Explicit `a0–a3` match. First difference:
`func_8001A0C0(176,161)` `v0` (Rice control 0 vs RT64 nonzero). E8918 selector 2
returns 0 vs 1 (**PROVEN** continuity). Q0 selector 1 remains 0/0.
**B-STATE + B-WRITER (2026-08-20)** [[p08-demo-1a0c0-state-differential]],
[[p08-demo-db-8007bcc4-lifecycle]]: `func_8001A0C0` is a pure leaf
`v0 = *(u16*)(D_8007BCC4 + 608*a1 + 2*a0)`. Allocation chain, base
(`D_8007BCC4 = 0x80137840` both runtimes), effective address
(`0x8014F800` both), and allocation-time cell (0 in both) all **match**.
ORIGINAL_RICE_CONTROL boot-bzeros the region once and never copies rendered Z
back to the target (store-watch: 1 write, the boot bzero, value 0). RT64's cell
is 0 right after allocation (ALLOC_AFTER), then copyback turns it to 0xFFFE and
live values (0x83A4..0x8BFC) by Q1. This proves a renderer-control difference,
not a real-N64 defect (B-ALLOC/B-ALIAS/B-INIT remain refuted).

```text
NEXT_SINGLE_ACTION: superseded by the hardware-faithful Original oracle run.
```

## Fresh USER_DEMO_4 walkstart ncbapivot join (2026-08-21)

The bounded real-display capture is now complete. RT64 and the Angrylion
oracle ran on `DISPLAY=:0` with RADV RX 580 (720 s timeout), using only the
diagnostic `TOOIE_UD4_WALKSTART=1` gate. RT64 reached `stage=0/sub=4` and
captured seven ncbapivot entries/leaves for glidx 36..42; the oracle reached
the same sub-window and produced the glidx 39 trace plus the motion CSV.
Joining by consumed glrecord and pad bytes gives exact agreement for glidx
38..42 (including glidx 39 `-48,-46,0001`).

At glidx 39, the entry read values match after pointer relocation (node+0x74,
node+0x80, tgt, selected record, vectors, position, yaw, ideal-yaw, f0/f12),
but the output differs. This does **not** close a same-complete-inputs/FPU
join: RT64 exposes only `rm=0`/`mips3=0`, while the oracle exposes FCSR
`01000800` at enter and `01800800` at leave. The earliest observed child-input
divergence is `800137AC` enter (`a2/f14`: RT64 `439a589e/41aa94a2`, oracle
`439a7fd5/439a054e`); call-stack order diverges immediately afterward.

```text
P08_STATUS:                        ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                        PENDING / NOT_FORMALLY_OPEN
NCBAPIVOT_WALK_READ_SET_COMPLETE: NO (RT64 FCSR/FPR provenance incomplete)
FIRST_EXECUTED_READ_DIFFERENCE:    NOT_PROVEN; earliest observed child-input difference 800137AC enter
SAME_INPUTS_DIFFERENT_OUTPUT:      NO / NOT_PROVEN
ROOT_CLASS:                        B-NCBAPIVOT-WALKSTART-FIRST-EXECUTED-INPUT
ROOT_CAUSE_LEVEL:                  NOT_PROVEN
FIX_IMPLEMENTED:                   NO
RT64_ARTIFACT:                     /tmp/p08-rt64-display-glidx39-r2.log
ORACLE_ARTIFACTS:                  /tmp/p08-angrylion-ud4-cam.log, /tmp/p08-angrylion-user_demo4.csv
```

This remains diagnosis-only. Do not reopen `round.w.s`, generated output,
camera feedback, or the egg path. The next bounded discriminator is missing
RT64 FCSR/read provenance around the `800137AC` inputs.

## P08 projection DB / Z-buffer target writer identification — B-WRITER PROVEN (2026-08-20)

[[p08-demo-db-target-writer]]. The exact Recomp writer to `D_8007BCC4 + 0x17FC0`
(`0x8014F800`) is PROVEN:
- `D_8007BCC4` (`0x80137840`, size `0x21D80` = 138,624 bytes) is the **304×228 16-bit N64 Z-Buffer (Depth Buffer)** (`304 * 228 * 2 = 138,624 = 0x21D80`).
- Target `0x8014F800` is the Z-buffer pixel at screen coordinates $(X=176, Y=161)$.
- `func_8001A0C0(176, 161)` is a Z-buffer depth query used by `func_800E81E0` for projection occlusion / visibility testing.
- The Recomp writer is `RT64::Framebuffer::copyNativeToRAM` -> `RT64::NativeTarget::copyToRAM` (`memcpy`), performing GPU depth buffer writeback to RDRAM (`0x80137840..0x801595C0`).
- Display list commands `gDPSetDepthImage(0x00137840)` and `gDPSetColorImage(0x00137840)` are emitted by `func_800E7BC8` and `func_800E7C28`.
- First write value is `0xFFFE` (clear depth); subsequent values `0x83A4..0x8BFC` are rendered geometry depths.
- ORIGINAL_RICE_CONTROL omitted depth writeback and left RDRAM at 0x0000.
  REAL_N64_EXPECTATION is a populated RDRAM Z image after proper RDP and cache
  synchronization. RT64 populates it, but its exact format is not bit-exact.

```text
ROOT_CAUSE_LEVEL:   C-ORACLE
FIX_IMPLEMENTED:    NO
NEXT_SINGLE_ACTION: run the bounded Mupen Pure Interpreter + Angrylion RDP Plus
                    + CXD4 LLE Original oracle experiment.
```

## Z-buffer & Post-Egg Motion Oracle Resolutions (2026-08-20)

Controlling evidence:
- [[p08-demo-zbuffer-oracle-sol-review]] (epistemic arbitration)
- [[p08-demo-zbuffer-angrylion-oracle]] (**RESOLVED as R-ORACLE**)
- [[p08-demo-post-egg-angrylion-motion-differential]] (**RESOLVED as R-MOTION-ORACLE**)
- [[p08-final-physical-validation]] (**PENDING USER EXECUTION**)

Summary of hardware oracle findings:
1. **Z-Buffer Oracle (R-ORACLE):** The hardware-faithful `ANGRYLION_CONTROL` run (Mupen64Plus Pure Interpreter + Angrylion RDP Plus + CXD4 LLE) produced live Z in RDRAM, with clear value `0xFFFC`; `record+0x34` evaluates to `1` across all 25/25 aligned Q1 samples. This is an emulator oracle, not a physical N64 measurement. The prior camera divergence was a Rice HLE reference artifact.
2. **Post-Egg Motion Oracle (R-MOTION-ORACLE):** The historical motion drift (which reached 58 units under Rice) disappeared completely under `ANGRYLION_CONTROL`. `camctrl` at glidx 385 matches within `+0.0030°` (delta `53.5799°` vs `53.5829°`), `ideal_yaw` matches identically (`102.59°` vs `102.59°`), maximum XZ error across the entire 765-frame Demo 0 stream is `< 0.89 units`, and behavior-state progression is 100% bit-identical (`0` state mismatches).
3. **No Production Fix Required for this measured stream:** Demo 0 movement, camera-control, and physics measurements are faithful to the hardware-faithful oracle. This does not close untested attract demos or the formal physical-display gate.
4. **Current Phase & Remaining Gates:**
   - `CURRENT_PHASE`: **P08 ACTIVE / MANUAL_VALIDATION_PENDING**
   - `AUTOMATED_SCOPE`: **COMPLETE**
   - `P09_FORMALLY_OPEN`: **NO**
   - `CURRENT_BLOCKERS`:
     - `PHYSICAL_GAMEPAD_VALIDATION`
     - `PHYSICAL_MULTI_MONITOR_DISPLAY_VALIDATION`

NEXT_SINGLE_ACTION: user executes docs/plan/evidence/p08-final-physical-validation.md checklist on physical desktop hardware.

For repeatable USER_DEMO_4 walkstart captures, use
[[p08-walkstart-reproduction-runbook]] before starting a new diagnostic run.

## Attract user-numbering and USER_DEMO_4 egg defect (2026-08-21)

Canonical: [[p08-provisional-follow-camera-and-egg-differential]].

The user numbers attract gameplay continuously across intro/music blocks.
`USER_DEMO_4` is map 235 Haunted Cavern (`stage=0 sub=4`) after the first
intro return, not zero-based Demo 3. Syscall spawn: Angrylion `cheggfire` 535
vs RT64 `cheggnormal` 539. `EGG_PARITY_DEFECT: PROVEN`. User confirmed this
Angrylion RDP Plus nightly-build run was visually correct for fire egg and
for the previously-broken demo behavior.

b042865 complete-attract claims (`REFERENCE_FIRE_EGG_EVENT_FOUND=NO`,
`GLOBAL_VISUAL_DRIFT_STATUS=NOT_REPRODUCED_IN_ATTRACT_SET`,
`REAL_PARITY_DEFECT_FOUND=NO`) are **DISPUTED** / **CORRECTED/SUPERSEDED**.
`HAUNTED_CAVERN_NO_FIRE_EVENT` is **REFUTED**. Demo 0 Wooded Hollow
R-MOTION-ORACLE measurements are retained for **that clip only**. USER_DEMO_4
follow-camera + player trajectory are **REPRODUCED** (pad 877/877, camctrl
split glidx 41, FEEDBACK_ACCUMULATION). P09 is **NOT_FORMALLY_OPEN**. No
production fix.

```text
EGG_PARITY_DEFECT:              PROVEN
CAMERA_DIVERGENCE_REPRODUCED:   YES
PLAYER_TRAJECTORY_DIVERGENCE_REPRODUCED: YES
CAMCTRL_WRITER:                 func_800CA628 CONFIRMED (USER_DEMO_4); stores src2[1]
CA628_INPUTS_AT_GLIDX_36:       ALREADY_DIFFER (eye ULP + src2[1] 1-2 ULP); producer func_800CA8B4
ANGRYLION_NIGHTLY_VISUAL:       USER_CONFIRMED PASS
FULL_ATTRACT_SEQUENCE_REPEAT_PROVEN: NO
FIX_IMPLEMENTED:                NO
NEXT_SINGLE_ACTION:             dump USER_DEMO_4 CA8B4/camera-record eye+camctrl for glidx 1-35 idle slew; no fix
```

## Provisional Player & AI Movement Speed Parity Baseline (2026-08-21)

Canonical: [[p08-provisional-player-ai-speed-parity]].

Independent parity check of movement speed magnitude, timing, animation rate, and trajectory across `ANGRYLION_CONTROL` (Mupen64Plus Pure Interpreter + Angrylion RDP Plus + CXD4 LLE) and `RT64_CONTROL` (TooieRecomp):

1. **Player Scalar Speed Parity:** **PASS**
   - Window A (`USER_DEMO_1` First Straight glidx 20..70): `tspd = 500.0`, `velxz = 510.2442 u/s` both runtimes (speed error = `0.000018 u/s`, direction error = `0.000204°`). Downhill slope scale `1.019454` confirmed original N64 behavior (`HISTORICAL_FIRST_STRAIGHT_SPEED: PROVEN_CLEAN`).
   - Window C (`USER_DEMO_1` Post-Egg glidx 380..460): `tspd = 620.0`, peak run `velxz = 673.5218 u/s` (speed error = `0.000564 u/s`, direction error = `0.005849°`).
   - Window B (`USER_DEMO_4` Haunted Cavern transition glidx 38..60): speed magnitude `sqrt(vx² + vz²)` matches within `0.0017..2.87 u/s` (sub-0.5% slope variation), while direction diverges due to the proven follow-camera defect (`CLASSIFICATION: CAMERA_DIRECTION_DEFECT_WITH_SPEED_PARITY`).
2. **AI Actor Speed Parity:** **PASS**
   - `chdiggerground` (Marker 337 / ovl 219+220): 100% exact route speed match (`0.0000 u/s` error).
   - Autonomous Patrol Baddie (Marker 2 / `sys 0x80086E60`): constant 15.8114 u/s waypoint patrol (`0.0000 u/s` error).
   - `chjinjo` (Marker 148): stationary idle and vertical hop parity (`0.0000 u/s` horizontal error).
3. **Timing and Physics dt:** **PASS**
   - Logic tick rate: 30.000 Hz (`1:1` advance per consumed glrecord sample).
   - Physics dt: `0.050000 s` during baphysics integration; `0.033333 s` input step.
   - Animation update frequency: 30.0 Hz.
4. **Classification & Status:**
   - `FIRST_PROVEN_SPEED_DEFECT: NONE`
   - `ROOT_CLASS: CAMERA_DIRECTION_DEFECT_WITH_SPEED_PARITY`
   - `FIX_IMPLEMENTED: NO`
   - `P08_STATUS: ACTIVE_MANUAL_VALIDATION_PENDING`
   - `P09_STATUS: PENDING_NOT_FORMALLY_OPEN`

## USER_DEMO_4 p60+0xC first-writer discriminator (2026-08-21)

Canonical: [[p08-user-demo4-p60-control-writer]].

The first reason why `p60+0xC` diverges (`RT64 = 0` vs `Oracle = 1` before glidx 39) is proven:
1. **Root Discriminator**: `0x800979D8` (`__bainput_func_800979D8`), a static internal helper in `bainput.c` that checks `bakey_held(self, button_id)` for C-buttons, was bound in `boot_hw_stubs.cpp` to `lookup_gap_stub<0x800979D8>` returning `v0 = 0`.
2. **Missing Write in RT64**: On `glidx=38`, the demo controller inputs hold C-Right. In Oracle, `__bainput_func_800979D8` returns 1 -> `bainput_should_rotate_camera_right` (0x80097B94) returns 1 -> `func_800A4854` calls `func_80110EFC(node, 1)` -> `func_80110E00` -> `func_801111D0` -> `func_80112D90`, storing `p60+0xC = 1` and `p60+0xE = 2`. In RT64, the gap stub returns 0, so the entire camera rotation write chain is skipped (`MISSING_WRITE`).
3. **Downstream Divergence at glidx 39**: In Oracle, `80112A84` reads `p60+0xC == 1` and sets `a1 = 5` for `80112D08`, storing `p60+8 = 5`, leading to transition pivot in `ncbapivot` (0x80112C3C). In RT64, `p60+0xC == 0`, so `80112A84` sets `a1 = 0` and `80112D08` stores `p60+8 = 0`, skipping transition pivot.
4. **Prior State**: 100% byte-matched player movement, positions, yaw, demo script ticks, and animation states across glidx 0..38.
5. **Status**: No production fix applied (worktree clean, diagnosis complete).

## Independent 0x800979D8 review and dynamic validation (2026-08-22)

The original ELF disassembly independently confirms that `0x800979D8` is a
real static helper. Its contract is stateful: `bakey_held(self, button_id)`,
logical-input enable gating via `a2`, and `BaInput.__enabled` bit updates. A
generic native mapping replaced the gap stub in
[[p08-user-demo4-800979d8-independent-review]].

`BAINPUT_HELPER_TESTS: PASS`; the four unrelated zero-return gaps remain
unchanged.

The post-fix runtime artifact is deliberately bounded; see
[[p08-user-demo4-800979d8-postfix-validation]]:
- Helper runtime: called at glidx 38–39, but `held=0` and return `0`; held-branch parity is **NOT_PROVEN**.
- Downstream `p60`: captured RT64 has `p60.C=0`, `a1=0`, and `p60+8=0`; P60 parity is **NOT_RECONCILED**.
- Movement/animation: sampled and bounded only; complete state parity is **NOT_PROVEN**.
- Egg Event: USER_DEMO_4 remains RT64 BLUE (`cheggnormal`/539) versus AN FIRE (`cheggfire`/535).
- Attract soak and memory behavior: **NOT_RUN / NOT_EVALUATED** for this fix.
- Worktree: implementation and focused test are local and uncommitted. P08 remains `ACTIVE / MANUAL_VALIDATION_PENDING`; P09 remains `PENDING / NOT_FORMALLY_OPEN`.

## USER_DEMO_4 0x800BCE84 live-input join (2026-08-22)

Canonical: [[p08-user-demo4-bce84-live-input-join]].

Exact guest `lw` at `0x800BCEAC` on USER_DEMO_4 glidx 39 C-Right (`ord=8`,
pad `(-48,-46,0001)`): both runtimes load `0x00000115` from
`obj + unk8[13]*56 + 0x24 + 13*4` (`ring=1`, offset `+0x90`), mask/return 1.
Old `bs_pressed=0` was ring 0 (`+0x58`) — **CORRECTED/SUPERSEDED**.
`LIVE_INPUT_EQUIVALENCE: PROVEN`. No commit.

## USER_DEMO_4 BaInput held-state provenance (2026-08-22)

Canonical: [[p08-user-demo4-bainput-held-state-provenance]].

Raw pad `(-48,-46,0001)` at glidx 39 **MATCH**. Diagnostic `held=` is
`prev_state[13]` and is `0` on both runtimes. AN `0x800979D8` returns `2`
and writes `p60.C=1/E=2/+8=5`; RT returns `0`. Gemini “pad 0001 == held
C-Right” is **CORRECTED/SUPERSEDED**. First discriminator is the native
helper (`ROOT_CLASS=A-CANDIDATE-HELPER`). Pending `0x800979D8` mapping is
preserved; no production edit; no commit.

```text
INPUT_DIVERGENCE_LAYER:        LOOKUP_GAP (native 0x800979D8 replacement)
FIRST_DIFFERENT_PC:            0x800979D8
NEXT_SINGLE_ACTION:            GPT-5.6 Luna x-high independent semantic
                               review of bakey_held polarity / v0=2 vs 0x2000
```

## Semantic arbitration update (2026-08-22)

The native mapping is now corrected, dirty, and uncommitted. `bakey_held`
uses the proven `prev_state` polarity and reaches `800BCE84` when that byte is
zero; `0x800979D8` returns the logical-input enabled mask (`a2=1` → `2`). The
full helper branch test passes. A fresh 180-second paired attempt was
incomplete (RT64 reached glidx 39 only in `stage=0/sub=0`; oracle did not reach
the joinable `stage=0/sub=4` target), so after-write p60 and ncbapivot parity
remain **NOT_CAPTURED / NOT_PROVEN**. P08/P09 statuses are unchanged and no
commit/push is authorized.

## Raw audit of reported paired capture (2026-08-22)

The reported `/tmp/p08-walkstart-20260822-021445/` artifacts exist and their
five reported hashes match. The target pad and downstream chain are present,
but the causal gate is not: RT64's raw `ud4-bainput-979d8-detail` target line
records `bs_pressed=0 bs_pressed_bit0=0`, while the oracle log has no equivalent
live `ButtonState.pressed` field. The reported 900-second duration is also not
supported by the RT64 `wall_ms` range (about 255 s); the real environment is
Linux Mint 22.2/kernel `6.8.0-138-generic` with RX 580 RADV. Bounded camera and
trajectory matches remain useful observations, but `800979D8_DYNAMIC_ROOT_CHAIN`
is **NOT_PROVEN**, memory leak status is **NOT_EVALUATED**, and no commit/push is
authorized. Temporary probes remain untouched because the final acceptance gate
did not pass.

## Final BCE84 checkpoint and residual register (2026-08-22)

The prior raw-audit paragraph is superseded by the second BCE84 capture. Its
hashes match the recorded `SHA256SUMS`, including `/tmp/p08-angrylion-ud4-cam.log`.
At stage 0/sub 4/glidx 39, pad `(-48,-46,0001)`, button 13, ordinal 8:

```text
800BCE84:       RT addr=8034c460 ring=1 raw=00000115 mask=1
                AN addr=80342760 ring=1 raw=00000115 mask=1
BAKEY_HELD:     RT=1 AN=1
979D8_RETURN:   RT=2 AN=2
P60_AFTER:      C=1 E=2 +8=5 on both
NCBAPIVOT:      matching branch
```

The old `bs_pressed=0` probe is **CORRECTED/SUPERSEDED** because it read ring
0. `800979D8_DYNAMIC_ROOT_CHAIN` is now **PROVEN for this bounded event**;
global attract parity is not. The exact BCE84/979D8 production probes were
removed; env-gated chain/state diagnostics remain for the next investigation.

Manual residuals are preserved in [[p08-attract-movement-residuals]]:
USER_DEMO_1 and USER_DEMO_5 are reproduced by the user; Demos 2, 3, and 6 only
appear correct manually; Demo 4 is supported by bounded paired evidence. The
Talon-Trot/locomotion-transition explanation remains an **INFERENCE / USER-
OBSERVED CORRELATION**. P08 stays `ACTIVE / MANUAL_VALIDATION_PENDING`; P09 is
`PENDING / NOT_FORMALLY_OPEN`.

## Artifact-integrity correction (2026-08-22)

The preceding “hashes match” statement is **CORRECTED/SUPERSEDED**. The
registered `SHA256SUMS` values versus current files are:

```text
FILE                              REGISTERED   CURRENT
p08-rt64-bce84-join.log          0c11c0f4...  9cdf01ae...
p08-angrylion-ud4-cam.log       47626f98...  ecc74dc1...
p08-angrylion-bce84-join.log    c58c55ff...  bf1af28d...
oracle-target-excerpt.log        3c56f5ee...  3c56f5ee...  MATCH
rt64-target-excerpt.log          0cb43009...  0cb43009...  MATCH
```

The current logs were modified at 03:24, after `SHA256SUMS` and excerpts at
03:19. Their target lines still show the corrected ring-1 join and downstream
values, so semantic content is **OBSERVED**, but the immutable capture gate is
**BLOCKED**. No commit/push is authorized. Next action: produce a fresh
bounded paired capture that writes final logs and `SHA256SUMS` atomically.

## Immutable recapture (2026-08-22T07:07Z)

Fresh directory `/tmp/p08-979d8-final-20260822T070735Z/`. Writers were stopped
before rename; `SHA256SUMS` was generated after final names + excerpts;
`sha256sum -c` **PASS** twice; artifacts then `chmod a-w`. Joined USER_DEMO_4
values match the prior semantic capture (`BCE84=0x115/1`, `bakey=1`,
`979D8=2`, `p60 C/E/+8=1/2/5`). `FINAL_ARTIFACT_INTEGRITY: PASS`.
`800979D8_DYNAMIC_ROOT_CHAIN: PROVEN` for this bounded event. No Demo 5 /
Talon Trot work. No commit/push.

## Final immutable checkpoint (2026-08-22T07:07Z)

`/tmp/p08-979d8-final-20260822T070735Z/SHA256SUMS` passes twice. The paired
target join is stage 0/sub 4, glidx 39, pad `(-48,-46,0001)`, button 13,
ordinal 8; BCE84 ring-1 raw `0x115`/return 1, `bakey_held=1`,
`0x800979D8=2`, and p60 C/E/+8 `1/2/5` match at the bounded ncbapivot event.
Temporary task-specific BCE84/979D8 probes are cleaned; reusable diagnostics
remain. Tests, build, diff check, and generated-source check pass. P08 remains
`ACTIVE / MANUAL_VALIDATION_PENDING`; Demo 1/5 movement residuals remain open.

## USER_DEMO_5 `800EEBF0` angular-input checkpoint — 2026-08-23

New immutable capture: `/tmp/p08-demo5-ee780-source-20260823T053510Z/`;
SHA256 verification passed twice. `800EE780` is a proven downstream vec3 add.
The first causal difference in the paired glidx-24 join is already its
`sp+0x54` input, produced by `800EEBF0`/`800EF214`.

`800EEBF0` takes a two-f32 angle pair plus a radius and emits
`(r*cos(p)*sin(y), r*sin(p), r*cos(p)*cos(y))`. The first and third inputs
match; only the yaw/heading differs (`4391e610` RT vs `43927d8b` AN). The
second angle resides at `sp+0x38` and is written by the caller's
`add.s @ 0x80800194`, stored at `0x808001A0` before the call at `0x8080019C`.
The add's `80012C3C`/`800125E8` operands were not dynamically joined.

Boundary: `B-ANGLE-SOURCE-PRECALL-COMBINATOR`. No production change. The
capture is documented under the actual prior-run label `Grok 4.6 High`; the
stale `Medium` string in immutable metadata was left untouched to preserve
hashes. Next action: capture only the two operands of the `0x80800194` add.

## USER_DEMO_5 p60 mode-state review — 2026-08-23

`func_80112C3C` is independently confirmed: mode `0` returns `in_yaw`, modes
`1/2/5` step through `800F1738`, and modes `3/4` use `800F1DCC`. The existing
paired trace places the first p60+8 value split at glidx 23 (`RT=5`, `AN=0`),
with the first returned-yaw split at glidx 24. Static provenance isolates
`func_80112D08` / store `0x80112D14` via caller `func_80112A84`; dynamic first
writer remains unproven due the headless RT64 Vulkan startup failure. No fix.

## USER_DEMO_5 D90 call-presence review — 2026-08-23

`func_801111D0` has two call-controlling branches: `0x80111230` tests
`func_80112E84(a0)==1` (`p60+0xC`), then `0x80111270` tests
`func_8011349C(a0,&sp+0x40)!=0`; `jal 80112D90` is at `0x80111294`.
At glidx 23 the first input is zero in both runtimes. The paired trace proves
RT reaches D90 and AN skips it, so the first control-flow split is the boolean
second-gate result at `0x80111270`; its raw helper return was not logged. The
same `(2,1,2002)` pad record is present in both runtimes. No direct C_RIGHT
predicate exists in `801111D0`; no production change.

## USER_DEMO_5 `0x80097B94` follow-up — 2026-08-23

The independent arbitration establishes the static contract: `0x80097B94`
passes action `0xD`/input `1` to `0x800979D8` and returns its `v0`. The Oracle
capture matches Gemini's `(2,1,2002)` record at glidx 23 and reaches BCE84 with
`prev_state=0`, ring 0, raw button word 0, hence result 0. The focused RT64
capture's normalized glidx 23 has a different pad record
`(-13,-15,0xF1F1)` and also returns 0. Result split `2/0` remains unjoined and
unproven; published helper implementation coverage is complete, semantic
event acceptance is not. No production change.

## USER_DEMO_5 `0x80097B70` / action `0xA` follow-up — 2026-08-24

The camera-left wrapper passes action `0xA` (`C_LEFT`) and input id `0` to
`0x800979D8`; caller `0x800A4878` tests its return at `0x800A4928` and reaches
`0x80110EFC(node, 0)` at `0x800A4930` when nonzero. The same visible Gemini
record `(2,1,2002)` is joined at glidx 22/23/24.

In the prior paired capture, record 23 is RT64 `v0=1`/consumer reached versus
Oracle `v0=0`/consumer skipped. Oracle's complete BCE84 state is
`prev=0, ring=1, raw=0xF3, masked=1, bakey=1`, with input 0 disabled before
the helper; prior RT64 had `bakey=1` and input 0 enabled. A fresh RT64 capture
of the same visible records instead had `ring=0, raw=0x72C, masked=0,
bakey=0`, returning 0. The first bounded difference is therefore the
history/ring-selected current word and enabled bit, not the pad.

Classification is `B-ACTION-0A-HISTORY-STATE`. The published `0x800979D8`
fix covers the helper contract, but the state writer remains unproven. No
production change.

## Corrected USER_DEMO_5 re-anchor: camera control (2026-08-24)

Use the correctly paired replay's record join: records 22-24 are matches and
record 25 is the first observed camera-control difference. The Trot-exit writer
also matches in both runtimes (`bainput_enable(0,1)`, `0xFFFFFFBE ->
0xFFFFFFBF`, bit 0 set), and action `0xA` returns 1 in both. This supersedes the
old `0x80097B94 RT=2/AN=0`, `RT97B70=1/AN=0`, RT-only
`0x80110EFC`/`0x801111D0`/`0x800979D0`, and
action-`0xA` causal chain claims; `B-TROT-ENABLE-RECOVERY` is not root.

`camctrl` is sampled from `0x80127708[ps+0x184]` record `+0x1C`. Static
`func_800CA628` gives the relevant writer PC `0x800CA65C`, source2[1]. Record
24 is `293.5294` / `0x4392C3C2`; record 25 is RT64 `293.0478` versus Oracle
`293.4005` / `0x4392B342`. Oracle has the direct writer event; RT64 lacks the
corresponding raw direct-writer capture. First-diff status is therefore
`B-CAMERA-WRITER-OR-SOURCE-INCOMPLETE-JOIN`, not a proven upstream input cause.

The old record-23 p60 `5/0` split and camera-chain causal claims are superseded
for this pairing; recheck p60 only on the true record-24->25 path. Next capture
must instrument RT64 `0x800CA65C` with raw old/source/new bits and source2
inputs. Artifact: `/tmp/p08-demo5-record25-camera-firstdiff-20260824T024500Z/`.
Status: `P08 ACTIVE / MANUAL_VALIDATION_PENDING`; `P09 PENDING / NOT_FORMALLY
OPEN`. No production change.

### Direct-writer arbitration correction (2026-08-24)

The newer CA628 artifact contains the RT64 raw event. At record 25 the first
relevant CA628 is RT64 ordinal `10329` (RA `0x8010FE24`), with old field
`0x4392861E`, source2[1] `0x43922BE8`, and new field `0x43922BE8`; Oracle's
paired event is ordinal `103`, `0x4392B342 -> 0x4392A91A`. This is Case B:
CA628 is the first observed writer, but the field and source are already
divergent at entry. `CA8B4` is only a vector copy and E1A58 preserves the
heading in the later call. The actual writer before CA628 remains uncaptured;
E1A58 is not root.

### Active camera-node heading boundary (2026-08-24 correction)

Keep these four storages separate: `active_node+0x34`,
`active_node+0x3C`, temporary `stack_angles+0x4`, and
`selected_record+0x1C`. The latest artifact watches the third and fourth
storages, not `active_node+0x34`. `func_8010FE34:0x8010FEA8` loads the second;
`func_800EFA4C:0x800EFA5C` stores it into the third; CA628 later copies the
third into the fourth.

The only direct static node+0x3C writer identified in the indexed primary code
is `func_8010FDEC:0x8010FDFC`, called by `plcamera_entrypoint_13` at
`0x808004A0`. The artifact did not capture its effective-address mutation, so
the first causal node+0x3C writer remains unresolved:
`B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED`. Latest slot-0 semantic pointers are RT
`0x801C6610` and Oracle `0x801CACB0`; the prior `0x8039D820` pointer belongs to
an older FE14 capture and is not joined to this pair.

### Node+0x3C checkpoint audit correction (2026-08-24)

The later node3C artifact is hash-valid and read-only, but its temporal labels
are not valid semantic node+0x3C evidence. Oracle's raw records 22–26 read
`node=0x8031CF80`, `mode=0`, and `node+0x3C=0x01015590`; later it reads
`0x801CACB0` with the same non-heading field. RT64 has no paired raw node3C
checkpoint lines. The reported post-player MATCH and pre-FE34 DIFF were thus
inferred from the heading pipeline, not direct joined reads. Keep the boundary
as `B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED`.

### USER_DEMO_5 EFA4C heading provenance (2026-08-24)

Current operational boundary **before** the register-provenance pass:
`B-EFA4C-HEADING-SOURCE-PROVENANCE-UNRESOLVED`. That pass recaptured USER_DEMO_5
glidx 22–25 and **failed** to rejoin the Luna EFA4C heading pair
(`0x4392861E` vs `0x4392B342`). `func_80110970` / `func_8010FE34` did not
execute. Observed consumer path is `FC38→FE14→CA628`. Classification:
`CASE E - CORRECTED/SUPERSEDED`. Artifact:
`/tmp/p08-demo5-efa4c-heading-provenance-20260824T061436Z/`. Next single
action: independently arbitrate whether the 10970→EFA4C event exists on a
map=296-only join, then promote at most one source/control producer.

### USER_DEMO_5 record24 camera-path conflict arbitration (2026-08-24)

That arbitration is complete but the event join is not: `ROOT_CLASS:
JOIN-UNRESOLVED`. The Grok artifact is immutable and hash-valid, yet its
`sub=5` window collides across attract slices and has two RT remapped-
`glidx=24` series. It lacks the requested record ordinal, 6-byte sequence,
pad/context join, paired call-order table, effective-CA628 table, and
record24/25 state files. RT has wrapper observations for FC38/FE14/CA628 with
`ra=0`; Oracle has no matching semantic lines. Therefore the replacement path
is not proven, and the old FE34/10970/EFA4C claim is only
`CORRECTED/SUPERSEDED`, not replaced by a causal winner.

`CONFLICT_WINNER: NEITHER — NEW CAPTURE REQUIRED`. Do not promote
`B-EFFECTIVE-CA628-SOURCE`; keep all record/input/state fields from this
capture `UNKNOWN` until a map=296-only paired run joins ordinal, sequence,
counter, pad tuple, stage/sub/map, relative call order, and effective CA628
destination/source/old/new bits in both runtimes. This does not alter the
separate `B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED` result.

### Independent map296 join validation and earliest camera boundary (2026-08-24)

The replacement capture resolves replay identity: raw ud5-record streams are 28/28, raw6 is 28/28 bit-perfect, canonical keys have no duplicates, and the single epoch is gated by stage=0, sub=5, map=296, mode=5. D_8012D5B0+0x0 is the demo pointer, D_8012D5B8 is its post-consume record counter, and camera rec_ord can be one greater than the logical record while sharing the same logic tick. func_800151BC remains the proven tick boundary.

The camera sub-join does not support the previously asserted sequence. R24 EFA4C counts are RT=66/AN=73, FE14=1/1, CA628=2/2, and A9318=0/0. Static 0x800A9318 is a separate 0x8010FE34 pass, not a proven exit of 0x800A8E9C; CAMERA_UPDATE_BOUNDARY_VERDICT=REFUTED and R24_CALL_SEQUENCE_MATCH=NO.

Earliest comparable camera state is logical record 0, camera ordinal 1, CA628 call 1 (capture tag rec_ord=1). Old destination bits match (0x4324BF6C), but source bits differ (RT=0x4324B542, AN=0x4324B132). Current boundary: ROOT_CLASS=B-FIRST-CA628-SOURCE. R24 is downstream: both old and source are already different (0x4324A910 vs 0x4324A7B0); output bits remain unknown. This supersedes JOIN-UNRESOLVED for replay identity while retaining old causal chains as CORRECTED/SUPERSEDED; it does not alter B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED.

Derived artifact: /tmp/p08-demo5-first-camera-diff-from-joined-run-20260824T074500Z/; manifest SHA256 d1139defdfa6903041cef31fd2e4db98ca328bd964e702a4e1dca6683a7cff3f. No production change, commit, or push.

### USER_DEMO_5 Record 0 CA628 source provenance & writer trace (2026-08-24)

- **Artifact Directory:** `/tmp/p08-demo5-r0-ca628-source-firstdiff-20260824T144859Z/`
- **Replay State:** `stage=0, sub=5, map=296, mode=5`, 28/28 bit-perfect record join.
- **Initial Map 296 Checkpoint:** `sub+0x84 = 0x43240367` (164.013290), `node+0x50 = 0x3f3c04a3` (0.734446), `node+0x58 = 0x3f800000` (1.000000) bit-identical in BOTH runtimes. Refutes Case E (Map Init).
- **First Divergent Writer:** `func_80116218` at `0x80116380` (`swc1 $f18, 0x4($s1)`).
- **Target Memory Address:** `$sp + 0x2C` (`0x8004567C`, caller stack frame of `func_80110588`).
- **Target Lifetime:** `STACK` (ephemeral stack-allocated vector `sp+0x28`).
- **Values Stored ($f18):** RT64: `0x4324b542` (164.708038) vs Angrylion: `0x4324b132` (164.692169).
- **Downstream Consumer:** `func_800CA628` Call 1 at `0x8010FE1C / 0x8010FE24` copies `src2[1]` (`*(sp+0x2C)`) into `dst+0x1C` (old value matched `0x4324bf6c`).
- **Classification:** `Case A — Writer input diff / upstream angular math evaluation in func_80116218 / func_80116120`.
- **Operational Boundary:** `B-R0-CA628-SOURCE-WRITER-PROVEN`.

### Independent pre-0x80116380 arbitration: first cam_sub+0x84 transition (2026-08-24)

**CURRENT_STATUS:** `P08 ACTIVE / MANUAL_VALIDATION_PENDING`; `P09 NOT FORMALLY OPEN`.

**CURRENT_ROOT_BOUNDARY:** `B-R0-FIRST-CAMSUB84-WRITER-SOURCE` — `func_800EE7F8:0x800EE804` writes persistent `cam_sub+0x84` from `0x80045638`; immediate caller unknown.

**LATEST_PROVEN_FACTS:** 28/28 joined records; `func_80110588` entry base `0x43240367` and node+`0x50` `0x3F3C04A3` match; the earlier `EE7F8` write is RT `0x4324036A` versus AN `0x4324036D`; the `0x80110634` stack copy already carries the difference; node+`0x50` first differs at `0x801161E0`; `0x80116380`/`CA628` are downstream.

**REFUTED_OR_SUPERSEDED_CLAIMS:** Gemini's `80116380` root; prior `B-R0-CA628-SOURCE-WRITER-PROVEN` as root; all older FE34/10970/EFA4C/node+3C roots. Map 296 is Hailfire Peaks — Icy Side / `MAP_128_HP_ICY_SIDE`, not Mayahem; current TOML has no human-readable map table.

**UNRESOLVED_QUESTIONS:** immediate caller/source producer; conditional `0x800893E0` execution/value; complete `func_80116120` input parity.

**ARTIFACT_PATHS:** `/tmp/p08-demo5-r0-pre-80116380-firstdiff-20260824T082500Z/`, joined `/tmp/p08-demo5-map296-semantic-join-20260824T071156Z/`, source `/tmp/p08-demo5-r0-ca628-source-firstdiff-20260824T144859Z/`.

**ARTIFACT_MANIFEST_SHA256:** `6c793196c0457e162bcf6ebd1ff8ed2f4e764f1a72d544cf09232edd0148c29b`.

**WORKTREE_STATE:** intentionally dirty and preserved; documentation-only changes this run; no `RecompiledFuncs`/`src/overlays` changes.

**CODE/BUILD/TEST_STATUS:** no production fix; no build/test; `git diff --check` pending.

**WHAT_WAS_DONE_THIS_RUN:** independent raw/static arbitration plus immutable artifact and handoff update.

**WHAT_REMAINS_PENDING:** exact caller of the earlier persistent `EE7F8` write.

**NEXT_SINGLE_ACTION:** paired Record 0 capture with RA/callsite around that write, `f71`, and post-`EE7F8` stack checkpoint; stop there.

**NEXT_RECOMMENDED_MODEL:** GPT-5.6 Luna High / Terra equivalent, adversarial float provenance.

## USER_DEMO_5 `800EE780 → 800EEBF0 src_dir[1]` arbitration (2026-08-24)

**PROJECT_PHASE:** P08 active causal arbitration; **P08_STATUS:** `ACTIVE / MANUAL_VALIDATION_PENDING`; **P09_STATUS:** `PENDING / NOT_FORMALLY OPEN`.

**CURRENT_HEAD:** `25595a2eb8241ef7ca4aa135e6681e44abcf8598`. **WORKTREE_STATUS:** intentionally dirty state preserved; documentation-only change this round; no `RecompiledFuncs/**` or `src/overlays` change.

**CURRENT_INVESTIGATION:** `USER_DEMO_5`, `stage=0/sub=5/map=296`, `Hailfire Peaks — Icy Side / MAP_128_HP_ICY_SIDE`.

**CURRENT_ROOT_CLASS:** `B-800EEBF0-INPUT` — `PROVEN`.

**CURRENT_ROOT_BOUNDARY:** `func_800EEBF0` consumes `sp+0x34` (`src_dir[0] = pitch`, `src_dir[1] = yaw`) and radius `a2`, producing `sp+0x54`. At glidx 24, its `src_dir[1]` (yaw) is the first divergent input (RT: `0x4391E610` vs AN: `0x43927D8B`), causing its Cartesian output `sp+0x54` to diverge. `func_800EE780` is an elementwise vector adder (`sp+0x48 = sp+0x28 + sp+0x54`) whose input A is bit-identical and whose output divergence is strictly inherited from `sp+0x54`.

**LAST_PROVEN_MATCH:** glidx 23:
- `func_800EEBF0`: RT64 ord=42 <-> ANGRYLION ord=6 (MATCH, 100% bit-exact).
  - `old = (0xC43BDF50, 0x43BEBF32, 0x439FDC90)`, `src_dir = (0x41C84F23, 0x43928610, 0x446158B2)`, `scale = 0x446158D2`, `out = (0xC43BDF66, 0x43BEBF2F, 0x439FDC60)`.
- `func_800EE780`: RT64 ord=374 <-> ANGRYLION ord=454 (MATCH, 100% bit-exact).
  - `old = (0x45818C16, 0x447DB8E3, 0x458225C9)`, `src_a1 = (0x45990800, 0x441E594A, 0x45705000)`, `src_a2 = (0xC43BDF66, 0x43BEBF2F, 0x439FDC60)`, `out = (0x45818C13, 0x447DB8E2, 0x458225C6)`.

**FIRST_PROVEN_DIFF:** glidx 24:
- `func_800EEBF0`: RT64 ord=43 <-> ANGRYLION ord=7.
  - `old = (0xC43CBBD0, 0x43BEC05C, 0x43A01330)` [MATCH].
  - `src_dir[0]` (pitch) = `0x41C79912` (24.949741°) [MATCH].
  - `src_dir[1]` (yaw) = RT: `0x4391E610` (291.797363°) vs AN: `0x43927D8B` (292.980804°) [DIFF].
  - `scale` = `0x44621A8F` (904.414978) [MATCH].
  - `out` = RT: `(0xC43E589D, 0x43BEC05A, 0x43983EF2)` vs AN: `(0xC43CBBB0, 0x43BEC05A, 0x43A01366)` [DIFF].
- `func_800EE780`: RT64 ord=382 <-> ANGRYLION ord=462.
  - `old = (0x45818C16, 0x447DB8E2, 0x458225C9)` [MATCH].
  - `src_a1` = `(0x45992390, 0x441E58B4, 0x4570492C)` [MATCH].
  - `src_a2` = RT: `(0xC43E589D, 0x43BEC05A, 0x43983EF2)` vs AN: `(0xC43CBBB0, 0x43BEC05A, 0x43A01366)` [DIFF from 800EEBF0].
  - `out` = RT: `(0x4581587C, 0x447DB8E1, 0x4581A885)` vs AN: `(0x45818C1A, 0x447DB8E1, 0x458225CC)` [DIFF].

**LATEST_PROVEN_CAUSAL_CHAIN:** `func_800EEBF0(input_yaw DIFF)` → `sp+0x54 (DIFF)` → `func_800EE780(a1 MATCH, a2 DIFF)` → `sp+0x48 (DIFF)`.

**CURRENTLY_DOWNSTREAM_FUNCTIONS:** `func_800EE780` and `func_800EEBF0` are both faithful downstream helpers with identical logic between runtimes.

**REFUTED_OR_SUPERSEDED_CHAINS:** bounded USER_DEMO_4 `979D8 → BCE84 → P60`; generic movement, CA628, FE14, and prior `80116380` camera-root claims remain superseded.

**LATEST_ARTIFACT / CAPTURE_DIR:** `/tmp/p08-demo5-eebf0-ee780-20260824T182550Z/`.
- **MANIFEST_SHA:** `6d1ec2322e9b9022b5600a3c54e74bc2bb09c6454af2773fe91793ad63bc7610`.
- **SHA256_VERIFY:** PASS (verified twice; read-only `0444`/`0555`).

**CURRENT_UNKNOWNS:** upstream evaluation producing return value `$v0` of `func_80097B70` (`bainput_should_rotate_camera_left` calling `func_800979D8` at `0x80097B7C`) at `glidx 23`.

**PENDING_WORK:** GPT-5.6 Luna High causal arbitration of the `func_80112D90` call-presence boundary artifact before advancing upstream.

**NEXT_SINGLE_ACTION:** GPT-5.6 Luna High independently arbitrates the new immutable paired `func_80112D90` call-boundary capture (`/tmp/p08-demo5-80112d90-call-boundary-20260825T040600Z/`) where the call presence difference (`RT64=YES` vs `ANGRYLION=NO`) is proven to originate upstream at branch `0x800A4928` (`beq $v0, $zero`) due to `func_80097B70` returning `1` in RT64 vs `0` in ANGRYLION.

**NEXT_RECOMMENDED_MODEL:** GPT-5.6 Luna High.

**FIX_IMPLEMENTED:** NO. **COMMIT:** NONE. **PUSHED:** NO.

## Current arbitration checkpoint — 2026-08-25 (func_80097B70 & func_800979D8 Direct Measurement glidx 22->23)

`/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/` is the current immutable paired artifact. `sha256sum -c SHA256SUMS` passes for every file and `SHA256SUMS` hashes to `650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`.

**RAW_LOG_PROVENANCE:** `NEW_RAW_CAPTURE` (Dual live capture of `TooieRecompiled` and Mupen64Plus `ANGRYLION_CONTROL`).

**Observational Gap Resolution:**
The prior gap where ANGRYLION `func_80097B70` return = 0 was an inference is now **RESOLVED**. Both runtimes directly measured `func_80097B70`, `func_800979D8`, branch `0x800A4928`, and the downstream dispatch chain:
- **`glidx 22`:**
  - `func_800979D8` leave: RT `v0=0` vs AN `v0=0` (**MATCH / PROVEN**).
  - `func_80097B70` leave: RT `v0=0` vs AN `v0=0` (**MATCH / PROVEN**).
  - Branch `0x800A4928`: `v0 == 0`, `taken = 1` in both runtimes (**MATCH / PROVEN**).
  - `func_80112D90`: NOT called in either runtime; `p60+0xC = 0` (**MATCH / PROVEN**).
- **`glidx 23`:**
  - `func_800979D8` leave: RT `v0=1` vs AN `v0=0` (**DIVERGENCE / PROVEN**).
  - `func_80097B70` leave: RT `v0=1` vs AN `v0=0` (**DIVERGENCE / PROVEN**).
  - Branch `0x800A4928`:
    - RT64: `v0 == 1`, `taken = 0` (falls through -> calls `func_80110EFC` -> `func_80110E00` -> `func_801111D0` -> `func_80112D90`, setting `p60+0xC = 1`).
    - ANGRYLION: `v0 == 0`, `taken = 1` (branches to `0x800A4940` -> skips camera rotation dispatch; `p60+0xC` remains `0`).

**Semantic join:** `PROVEN` across both runtimes. The normalized tuple `(stage=0, sub=5, map=296, mode=5, glidx, calling_region=func_800A4878, controlling_branch=0x800A4928, bainput_helper=func_800979D8)` matches across the glidx 22->23 transition.

**CURRENT_ROOT_CLASS:** `B-BAINPUT-HELD-STATE` / `B-ROTATE-CAMERA-LEFT-INPUT`.
**CURRENT_ROOT_BOUNDARY:** `bakey_held(self, 10)` (`0x800919F4`) query inside `func_800979D8` evaluating to `held = 1` in RT64 vs `held = 0` in ANGRYLION at `glidx 23`.

**NEXT_SINGLE_ACTION:** Perform static and mechanical dynamic boundary analysis of `bakey_held` (`0x800919F4`) and `func_800BCE84` for button 10 (C-Left) at `glidx 23`.
**NEXT_RECOMMENDED_MODEL:** Gemini 3.7 Flash High for mechanical boundary instrumentation / GPT-5.6 Luna High for causal arbitration. No fix, commit, or push.
**OPTIONAL_MECHANICAL_MODEL:** DeepSeek V4 Flash.
**OPTIONAL_SECONDARY_MODEL:** Grok 4.6 Medium for narrow timing/register ambiguity.

## Superseding checkpoint — `800979D8` primary-log correction (2026-08-25)

Artifact `/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/` is valid;
manifest SHA is `650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`
and all `SHA256SUMS` entries pass. `analysis.md` is
**CORRECTED/SUPERSEDED** where it claims `held RT=1 / AN=0`.

Primary logs show at target glidx 23: `bakey_held=1` in both runtimes,
enabled word `0xffffffbf` with bit 0 set in both, and `800979D8` return
`RT=1` versus `AN=0`. `80097B70` is faithful forwarding; this is the first
proven diff. AN lacks a direct `800BCE84` marker for its mapped-byte/table
inputs, so complete external-state parity is not proven and the current
classification is **Case B**, not a promoted `800BCE84` root.

Static: `bakey_held` reads `self+0x40`, `key+0x18+button_id`, and `key+0`,
then calls `800BCE84(state, button_id)` at `0x80091A18`; `800BCE84` reads
`state+0x8+button_id`, calls `800BCCC0`, indexes the returned table, and
returns bit 0. Map/button-word entry fields are trace-only. **CURRENT_ROOT_CLASS:**
`B-800979D8-COMPLETE-READSET-UNKNOWN`. **NEXT_SINGLE_ACTION:** capture the
complete `bakey_held` state and nested `800BCE84` call/args/return at glidx
22/23 in both runtimes; stop at `800BCE84`. **NEXT_RECOMMENDED_MODEL:**
Gemini 3.7 Flash High. No production fix.

## Superseding narrow arbitration — `800979D8` post-`isEnabled` body (2026-08-25)

The artifact is valid (`NEW_RAW_CAPTURE`, manifest
`650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`, all
checksums **PASS**). At semantic glidx 23, `bakey_held=1/1`, enabled bit
0 `1/1`, and final `800979D8` `RT=1/AN=0` are **PROVEN**. The host path
with held and enabled both true has no enable side effect and returns `1`.

## Canonical Checkpoint — `func_800979D8` Exact MIPS Recovery & Static Implementation Mismatch (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-post-enabled-20260825T184847Z/` is valid;
`NEW_RAW_CAPTURE`, manifest SHA is `4c75f328cfaa978174f373884472395cf708eeaf413f096f1808af8c913d705c`,
and all `SHA256SUMS` entries pass (dual verification).

1. **Exact Original MIPS Body (`0x800979D8 .. 0x80097A4C`) Recovered:**
   - `0x800979E8`: calls `bakey_held(self, button_id)` (`0x800919F4`).
   - `0x800979F0`: `beqz $v0, 0x80097A2C` (if held == 0, jumps to unheld path).
   - `0x800979FC`: calls `bainput_isEnabled(self, input_id)` (`0x80098024`), which reads `*(input + 0x0) & (1 << input_id)`.
   - `0x80097A04`: `bnez $v0, 0x80097A18` (if enabled != 0, skips disable helper).
   - `0x80097A10`: calls `func_80097990(self, input_id, 0)` (when enabled == 0, clears bit in `*(input + 0x4)`).
   - `0x80097A1C`: calls `func_80097978(self, input_id)`, which reads and returns `*(input + 0x4) & (1 << input_id)`!
   - `0x80097A24`: jumps to epilogue `0x80097A40` and returns `$v0` from `func_80097978`.
   - Unheld path (`0x80097A2C`): calls `func_80097990(self, input_id, 1)` (sets bit in `*(input + 0x4)`) and returns `$v0 = 0`.

2. **Root Cause of glidx 23 Divergence:**
   - At `glidx 23`, `bakey_held` returns `1` and `bainput_isEnabled` returns `1` in both runtimes.
   - Original MIPS executes `0x80097A1C: jal 0x80097978`. `func_80097978` queries `*(input + 0x4) = 0xFFFFFFFE` -> bit 0 is 0 -> returns `$v0 = 0`.
   - RT64 host replacement (`tooie_bainput_func_800979D8` in `src/game/boot_hw_stubs.cpp`): incorrectly calls `bainput_isEnabled(self, input_id)` a second time (reading `*(input + 0x0) = 0xFFFFFFBF` -> bit 0 is 1) -> returns `ctx->r2 = 1`.

3. **Classification & Boundaries:**
   - `STATIC_IMPLEMENTATION_MISMATCH_800979D8`: **PROVEN**.
   - `CURRENT_ROOT_CLASS`: `B-800979D8-STATIC-HOST-IMPLEMENTATION-MISMATCH`.
   - `CURRENT_ROOT_BOUNDARY`: Callee selection in `func_800979D8` (`0x80097A1C` calls `func_80097978` on `input+0x4`, not `bainput_isEnabled` on `input+0x0`).
   - `NEXT_SINGLE_ACTION`: GPT-5.6 Luna High independently arbitrate the exact original-vs-host semantic mismatch before any production fix.
   - `NEXT_RECOMMENDED_MODEL`: GPT-5.6 Luna High. No fix, commit, or push.

### Corrected/Superseded — `800979D8` bug versus current `input+0x4` root (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-post-enabled-20260825T184847Z/` is
SHA-valid (manifest `4c75f328cfaa978174f373884472395cf708eeaf413f096f1808af8c913d705c`; all
checks pass). The original body `0x800979D8..0x80097A4C`, including the
post-`isEnabled` path, is complete and statically proven.

`STATIC_IMPLEMENTATION_MISMATCH_800979D8=PROVEN`: original `80097990` writes
`input+0x4` but host `bainput_enable` writes `input+0x0`; original `80097978`
reads `input+0x4` at `80097984` but the host's second query reads `input+0x0`.
This remains a real production bug and should eventually be fixed.

At glidx23, `input+0=ffffffbf` matches in both runtimes, but `input+4` is
RT64 `ffffffff` (bit0=1) versus Angrylion `fffffffe` (bit0=0). The prior
`COMPLETE_EXTERNAL_INPUTS_MATCH=YES` claim is self-contradictory and
**CORRECTED/SUPERSEDED** to **NO**. glidx22 `ffffffff`/`ffffffff` is the
last proven match; glidx23 is the first proven difference.

With no state change, a corrected host would return RT64 `1`, while original
Angrylion returns `0`; counterfactual parity is **NO**. Hence
`WOULD_FIXING_800979D8_ALONE_RESTORE_GLIDX23_PARITY=NO`.

`PRODUCTION_CORRECTNESS_BUG`: static `800979D8` host mismatch.
`CURRENT_PARITY_CAUSAL_ROOT`: divergent active/triggered `input+0x4` bit.
`CURRENT_ROOT_CLASS`: `B-BAINPUT-ACTIVE-MASK-INPUT-PLUS4-STATE`.
`CURRENT_ROOT_BOUNDARY`: bit0 of `input+0x4` for `input_id=0`, immediately
before `80097A1C -> 80097978` / read PC `80097984`.

`FIX_800979D8_NOW=NO`. `NEXT_SINGLE_ACTION`: statically enumerate immediate
writers/callsites of this bit, starting at known writer `80097990`, then use
one future dual-runtime join of old/new words, source, writer PC, callsite,
call presence, and semantic occurrence. No new capture, upstream walk, or
production edit in this checkpoint. Recommended model: **Gemini 3.7 Flash High**
(DeepSeek V4 Flash for mechanical candidate enumeration; Grok 4.6 Medium only
for ordering ambiguity).

## Canonical Checkpoint — `input+0x4` First Temporal Writer & Host `800979D8` Causal Proof (2026-08-25)

Artifact `/tmp/p08-demo5-inputplus4-first-writer-20260825T220500Z/` is valid;
`NEW_RAW_CAPTURE`, manifest SHA is `3acb82b7a27b9dfa90da7a782e0618caebd6ef9daa574340261007703417de46`,
all 10 `SHA256SUMS` entries pass (dual verification).

1. **First Temporal Writer Event Identified:**
   - At `glidx 22`, occurrence `func_800979D8(self, button_id=10, input_id=0)`:
     - `held == 1` and `enabled == 0` in BOTH runtimes.
     - `*(input + 0x4)` is `0xFFFFFFFF` (bit 0 = 1) in BOTH runtimes before call.
     - **AN (Original MIPS):** Follows `0x80097A04` branch fallthrough and executes `0x80097A10: jal func_80097990(self, 0, 0)`. `func_80097990` executes CLEAR store at `0x800979CC`, transforming `*(input + 0x4)` from `0xFFFFFFFF` (bit 0 = 1) to `0xFFFFFFFE` (bit 0 = 0).
     - **RT (Host C++ Replacement):** `tooie_bainput_func_800979D8` calls `bainput_enable(self, 0, 0)` at `0x8009817C` (modifying `input + 0x0`) instead of `func_80097990` (modifying `input + 0x4`). Therefore, `*(input + 0x4)` in RT64 is never modified and remains `0xFFFFFFFF` (bit 0 = 1).
   - This single host defect at `glidx 22` is the exact causal producer of the `input+0x4` state divergence.

2. **Downstream Target Divergence at `glidx 23`:**
   - At `glidx 23`, `bainput_enable(self, 0, 1)` updates `*(input + 0x0)` to `0xFFFFFFBF` in both runtimes.
   - When `func_800979D8(self, button_id=10, input_id=0)` is called:
     - `held == 1`, `enabled == 1`.
     - **AN:** Executes `0x80097A1C: jal func_80097978(self, 0)`, reads `*(input + 0x4) = 0xFFFFFFFE` (bit 0 = 0) -> returns `$v0 = 0`. Controlling branch `0x800A4928` branches over camera rotation left (`p60+0xC = 0`).
     - **RT:** Host calls `bainput_isEnabled(self, 0)` on `*(input + 0x0) = 0xFFFFFFBF` (bit 0 = 1) -> returns `ctx->r2 = 1`. Controlling branch `0x800A4928` falls through to `func_80112D90`, setting `p60+0xC = 1` (unwanted camera rotation left).

3. **Classification & Verification:**
   - `FIRST_INPUT_PLUS4_DIFFERING_EVENT`: `glidx 22`, `func_800979D8(self, btn=10, in_id=0)`.
   - `FIRST_INPUT_PLUS4_WRITER_RT`: `NO_WRITE_OBSERVED` (host called `bainput_enable` on `input+0`, omitting write to `input+4`).
   - `FIRST_INPUT_PLUS4_WRITER_AN`: `func_80097990` at `0x800979CC` (CLEAR bit 0).
   - `FIRST_INPUT_PLUS4_WRITER_EVENT_MATCH`: `ABSENT_IN_ONE_RUNTIME`.
   - `FIRST_INPUT_PLUS4_OLD_MATCH`: `YES` (`0xFFFFFFFF`, bit0=1).
   - `FIRST_INPUT_PLUS4_SOURCE_OPERATION_MATCH`: `NO`.
   - `FIRST_INPUT_PLUS4_NEW_MATCH`: `NO` (`0xFFFFFFFE` [bit0=0] vs `0xFFFFFFFF` [bit0=1]).
   - `HOST_800979D8_MISMATCH_CAUSES_INPUT_PLUS4_FIRST_DIFF`: **`PROVEN`**.
   - `CURRENT_ROOT_CLASS`: `B-800979D8-HOST-REPLACEMENT-WRONG-SIDE-EFFECT`.
   - `BAINPUT_INIT_EXECUTES_IN_WINDOW_RT`: `NO`.
   - `BAINPUT_INIT_EXECUTES_IN_WINDOW_AN`: `NO`.
   - `NEXT_SINGLE_ACTION`: GPT-5.6 Luna High arbitrate the first input+4 divergence and the causal proof that the host 800979D8 mismatch produces it.
   - `NEXT_RECOMMENDED_MODEL`: GPT-5.6 Luna High. No fix, commit, or push.

## Final causal root — raw `input+4` writer arbitration (2026-08-25)

**CORRECTED/SUPERSEDES** the prior pending root. Artifact
`/tmp/p08-demo5-inputplus4-first-writer-20260825T220500Z/` is a valid
`NEW_RAW_CAPTURE`; manifest SHA is
`3acb82b7a27b9dfa90da7a782e0618caebd6ef9daa574340261007703417de46`.

`GLIDX22_800979D8_SEMANTIC_JOIN=PROVEN` for
`stage=0/sub=5/map=296/mode=5`, player semantic self, button 10, input 0;
call presence is YES in both. Held and enabled are `1/1` and `0/0`, and
the old `input+4` is `ffffffff/ffffffff`. The pre-write glidx22 state thus
matches. AN executes `0x80097A10 jal func_80097990` with `a1=0,a2=0` and
changes `input+4` to `fffffffe`; RT calls `bainput_enable` and leaves
`input+4=ffffffff` while writing `input+0`. The post-write state is the
first divergence: `HOST_800979D8_MISMATCH_CAUSES_INPUT_PLUS4_FIRST_DIFF=PROVEN`.

Raw opcode correction: `CLEAR_STORE_PC=0x800979CC` (`ac4c0004`),
`SET_STORE_PC=0x800979B0` (`ac590004`); `80097A10` is the callsite,
`80097A14` its delay slot, and `80097A18` its return address. Any earlier
store-PC or caller-PC labels are **CORRECTED/SUPERSEDED**. At glidx23,
AN reads `input+4` through `func_80097978` and returns 0; RT reads `input+0`
through `bainput_isEnabled` and returns 1, yielding downstream
`80097B70=0/1` and `800A4928=TAKEN/NOT_TAKEN`.

`ROOT_CAUSE_FOUND=YES`, `FIX_IMPLEMENTED=NO`,
`CURRENT_ROOT_CLASS=B-800979D8-HOST-REPLACEMENT-WRONG-SIDE-EFFECT`.
The write-helper and read-helper mismatches are both **PROVEN** and belong
to the same incorrect replacement; fix them together. A query-only glidx23
change is insufficient, while a full faithful replacement is **EXPECTED**
to restore this causal chain (not runtime-tested). Next: production fix plus
dual-runtime A/B validation, followed by broader attract regression checks.

## Post-Fix Validation Checkpoint — `tooie_bainput_func_800979D8` Faithful Fix (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-fix-validation-20260825T235800Z/` is valid;
`NEW_RAW_CAPTURE_POST_FIX`, manifest SHA is `9a977b6e1c4c276dd2b6616008723d4b3c7139b9d1c4bd6c519c2f23ae3373da`,
all 16 `SHA256SUMS` entries pass (dual verification).

- **`ROOT_CAUSE_FOUND:`** `YES`
- **`ROOT_CAUSE_FIX_VALIDATED:`** `YES`
- **`FIX_IMPLEMENTED:`** `YES`
- **`FIX_SCOPE:`** `src/game/boot_hw_stubs.cpp` (`tooie_bainput_func_80097990` on `input+4`, `tooie_bainput_func_80097978` on `input+4`, and corrected `tooie_bainput_func_800979D8`)
- **`HOST_WRITE_HELPER_FIXED:`** `YES`
- **`HOST_READ_HELPER_FIXED:`** `YES`

**Validation Results (100% Parity with ANGRYLION_CONTROL):**
- `GLIDX21_CONTROL_PARITY:` `PASS` (held=0, enabled=1, input+4=0xFFFFFFFF, v0=0)
- `GLIDX22_HELD_PARITY:` `PASS` (held=1/1)
- `GLIDX22_ENABLED_PARITY:` `PASS` (enabled=0/0)
- `GLIDX22_INPUT4_OLD_PARITY:` `PASS` (0xFFFFFFFF/0xFFFFFFFF)
- `GLIDX22_OPERATION_PARITY:` `PASS` (func_80097990 CLEAR store)
- `GLIDX22_INPUT4_NEW_PARITY:` `PASS` (0xFFFFFFFE/0xFFFFFFFE)
- `GLIDX23_INPUT4_PARITY:` `PASS` (0xFFFFFFFE/0xFFFFFFFE)
- `GLIDX23_800979D8_RETURN_PARITY:` `PASS` (v0=0/0)
- `GLIDX23_80097B70_RETURN_PARITY:` `PASS` (v0=0/0)
- `GLIDX23_800A4928_PARITY:` `PASS` (TAKEN/TAKEN, skipping camera rotation left)
- `GLIDX23_80112D90_CALL_PARITY:` `PASS` (NO/NO, camera rotation left completely collapsed)
- `GLIDX23_P60C_PARITY:` `PASS` (p60+0xC = 0/0)
- `POST_FIX_EEBF0_PARITY:` `PASS` (yaw=270.0 in both runtimes)
- `POST_FIX_EE780_PARITY:` `PASS` (a3=0 in glidx 21..23, 3fa21681 in glidx 24, 3fe2ec4e in glidx 25)
- `BTN13_NEARBY_REGRESSION:` `PASS` (input+4 bit1 SET, v0=0 in both runtimes)

**Status & Next Steps:**
- The entire USER_DEMO_5 locomotion/camera divergence chain is **RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX**.
- P08 remains **ACTIVE / MANUAL_VALIDATION_PENDING** pending broader physical validation gates.
- `NEXT_SINGLE_ACTION:` GPT-5.6 Luna High independently arbitrate the post-fix validation artifact and determine the remaining P08 work.
- `NEXT_RECOMMENDED_MODEL:` GPT-5.6 Luna High. No commit/push.

## Arbitragem final pós-fix P08 / USER_DEMO_5 — 2026-08-26 (GPT-5.6 Luna High)

Este bloco **CORRIGE/SUPERSEDE** o checkpoint anterior quanto ao escopo e ao
status; o registro histórico permanece intacto.

Artifact: `/tmp/p08-demo5-800979d8-fix-validation-20260825T235800Z/`;
`MANIFEST_SHA=9a977b6e1c4c276dd2b6616008723d4b3c7139b9d1c4bd6c519c2f23ae3373da`;
checksum integral `SHA256SUMS`: 16/16 `OK`.

`RT_RAW_PROVENANCE: NEW_RAW_CAPTURE_POST_FIX`;
`AN_RAW_PROVENANCE: REFERENCE_ORACLE_CAPTURE_REUSED`;
`REFERENCE_ORACLE_REUSE_VALID: YES`: `oracle.log` coincide byte a byte com
`/tmp/oracle_fresh3.log`, e a linha temporal mostra a Oracle após a
instrumentação e antes do fix. Validade limitada a este artifact.

Veredicto: a substituição host de `800979D8` é semanticamente fiel ao MIPS
(SET/CLEAR em `input+4`, consulta de enabled em `input+0`, query final em
`input+4`, low-5-bit shift e retorno sign-extended). O chain focal
`800979D8 → 80097B70 → 800A4928 → câmera/80112D90 → p60+0xC → yaw/EEBF0/EE780`
tem `USER_DEMO5_TARGET_CHAIN_PARITY: PROVEN`; `GLOBAL_GAME_PARITY: NOT_PROVEN`.

Classificação dos gates: `DIRECTLY_SUPPORTED` para estado held/enabled,
old/new `input+4`, operações, retornos, branch, p60+0xC, yaw, EEBF0/EE780 e
btn13; `DERIVED_BUT_PROVEN` para a junção causal normalizada e ausência do
dispatch de rotação esquerda; `UNSUPPORTED` para parity global, long
playthrough e Windows/controle físico.

`P08_STATUS: ACTIVE / MANUAL_VALIDATION_PENDING`;
`P09_STATUS: PENDING / NOT_FORMALLY_OPEN`;
`USER_DEMO5_STATUS: RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`.
`AUDIO_STARTUP_TICK` e `CHARACTER_EDGE_GRAPHICS_GLITCH` são `P08_RESIDUAL`;
`WIDESCREEN_EDGE_GEOMETRY` era `POST_P08_ENHANCEMENT` por ser conteúdo 4:3;
that causal label is **CORRECTED/SUPERSEDED** by
[[p08-widescreen-edge-geometry]] (`D — HOST_RT64_WIDESCREEN_ADAPTATION_BOUNDARY`);
`LONG_PLAYTHROUGH_VALIDATION` é `MANUAL_VALIDATION_PENDING`;
`WINDOWS_GAMEPAD_VALIDATION` é `BLOCKING_P08 / MANUAL_VALIDATION_PENDING`.

Remaining P08: checklist Windows/controle físico (bloqueador), long
playthrough/regressão ampla e triagem separada de áudio/gráficos. O fix focal
está `CURRENT_FIX_READY_FOR_COMMIT: YES`, mas o worktree não é um commit único
limpo por conter instrumentação/diagnóstico e docs históricos. Artifact `/tmp`
não é commitável; `BUILD_THIS_ROUND: NOT_RUN`; nenhum commit/push foi feito.

`NEXT_SINGLE_ACTION: same-scene Expand vs Original capture of guest scissor,
RT64 useWideViewport/adjustAspectRatio, and side-object submission
(WIDESCREEN_EDGE_GEOMETRY). Formal Windows/gamepad validation remains a
separate P08 gate.`

## Superseding P08 DL identity & frame-join fix (2026-08-26)

The active next action is the verified single-log `GUEST_DISPLAY_LIST_IDENTITY_CAPTURE`.
The timing and unjoined capture defect was diagnosed and resolved: `boot_hw_stubs.cpp`
hooked `0x80014148` and checked `task+0x08` vs `task+0x0C` as `ucode_boot` size,
which dropped all `[ws-dl-emit]` records, while `rt64_interpreter.cpp` buffered
independent log writes and hardcoded `guest_frame=UNKNOWN`.

The fix:
1. Hooked master graphics task submission at `func_80013EF0` (`0x80013EF0`), where
   `$a0` is `GraphicsTask*` (`task+0x08 = start`, `task+0x0C = end`, `size = end - start`).
2. Thread-safe, mutex-protected unified logging directly into `/tmp/tooie-ws-dl-identity.log`.
3. Exported canonical `tooie_get_current_guest_frame()` and `tooie_get_current_capture_id()`
   read by Tooie guest hooks, RSP HLE interpreter, and RT64 rasterizer.
4. Direct C linkage `tooie_ws_log_dl_rsp()` replacing file polling.

`BUILD_RESULT: PASS`; `DL_ADDRESS_SEMANTICS: PROVEN`;
`DL_TO_RSP_JOIN: YES`; `RSP_TO_RT64_WORKLOAD_JOIN: YES`;
`DL_TO_RT64_DRAW_JOIN: YES`; `END_TO_END_JOIN: YES`;
`PER_FRAME_ACTIVE_DL_SET_READY: YES`; `DL_FIRST_LAST_SEEN_READY: YES`;
`OLD_DL_EVENT_TIMING_BUG_ROOT: PROVEN`; `DL_IDENTITY_FRAME_JOIN_FIX_STATUS: VERIFIED`;
`GUEST_FRAME_JOIN_ACCURACY: 100.0%`; `UNJOINED_DL_EVENT_COUNT: 0`;
`DL_EVENTS_WITH_GUEST_FRAME_UNKNOWN: 0`.

Validation smoke test results:
- `[ws-capture-start]`: 1
- `[ws-frame]`: 648
- `[ws-dl-emit]`: 4,779
- `[ws-dl-rsp]`: 8,908
- `[ws-dl-rt64]`: 8,908
- `[ws-rt64-draw]`: 47,761
- Root ping-pong: `80159600` and `80160cd0` (`DOUBLE_BUFFER`).
- Root join match: 100% matched with ~78µs main-to-render thread latency.

Causal state: `CURRENT_ROOT_CLASS: UNKNOWN / RE-ANCHOR_REQUIRED` (preserved);
`CURRENT_ROOT_BOUNDARY: NOT_PROMOTED` (preserved).
Artifact: `/tmp/p08-widescreen-dl-framejoin-fix-20260826T130423Z/`; manifest
SHA256: `4c650961c173fb7ad7a924fbaaf90abe1393cc3a9d1a53adfe41807d7442cb32`.

`NEXT_SINGLE_ACTION: User triggers one live capture during active gameplay exhibiting the edge geometry glitch (touch /tmp/tooie-ws-capture) to generate authoritative trace in /tmp/tooie-ws-dl-identity.log.`

## FOCAL_BRANCH_TIMELINE setup (2026-08-26)

Diagnostic-only wrappers now join `func_80801174_gcmapsects`'s real helper
path (`800E3E8C → 800E3D0C → 800CAA24`) using caller-frame reconstruction and
the existing DL stream by
`capture_id` and `guest_frame`. Identity is
`section_ptr + section_index + section_entry_ptr`; generated code was not
edited. `CAA24` is a separating-plane/AABB predicate over the volume pointer
from `D_8012D500` (`1=pass`, `0=reject`), whose exact writer remains
`UNKNOWN`. Build PASS; launcher smoke completed 720 frames and emitted 3,881
DL events but no map-section calls. Runtime branch evidence is therefore
pending, not refuted. Artifact:
`/tmp/p08-widescreen-focal-branch-timeline-setup-20260826T164317Z/`; SHA
`f010ac8a1a33e97494a0d6d5dcb90a237bc8814399c4a01ba1328f21c9de81e3`.

## 2026-08-31 ultrawide revalidation

Under the committed dynamic cinematic source-aspect policy, CPU homogeneous
clipping of the corrected MAP_18B OPA triangles confirms Scene A
wall-to-empty authored-extent terminations in target-only bands at 21:9 and
32:9. The right-side target-only content is mixed SKY18/background and is not
classified as missing wall geometry. Scene B reaches the same opening camera
and OPA root, so the same semantic boundary applies; no heap-address or leaf
set identity is required. 16:9 remains the previously confirmed production
milestone. No geometry extension was implemented.

Artifact: `/tmp/p08-tintro-ultrawide-revalidation-20260831T042437Z/`.

## P08 closure / deferred ultrawide presentation (2026-08-31)

The committed T_INTRO dynamic source-aspect projection fix is closed and
visually confirmed for the tested 16:9 scenes. The experimentally acceptable
centered max-16:9 pillarbox candidate was deferred and removed from the
Tooie/RT64 worktrees; no RT64 production commit is required. The next single
action is the animated/swaying model-part graphical-glitch investigation.
Open queue after P08: animated model-part glitches (`UNKNOWN`, NEXT), audio
“tic tic” (`UNKNOWN`, after bug 1), and T_INTRO ultrawide above 16:9 (deferred,
authored-extent residual proven).
