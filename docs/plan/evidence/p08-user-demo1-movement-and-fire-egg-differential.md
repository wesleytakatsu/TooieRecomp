---
title: "P08 — USER_DEMO1 movement parity and Fire Egg diagnostic discriminator"
description: "USER_DEMO1 frustum parity narrow fix LANDED & VISUALLY CONFIRMED FOR TESTED AREA (candidate binary A6A2 sha256 a6a2c023...). Shared func_800E3E8C generic draw scope dropped; allowlisted pure-draw func_800B97A4 (kind 3) + preserved func_8011447C (kind 2). Mixed renderer func_800DE498 stays original v0. Feature remains default-ON (no flag OFF, no RNG save/restore). Under default-ON: both PROJECTILE (exit 0) and TARGET (exit 0) match original bit-exact: 355x12 trajectory words, 546 ActorRecord words, 14 RNG snapshots, post 355 Fire Egg hit v0=0x80132f60, 22 sweep + 21 move pairs exact. Harness unit tests exit 0 with pure_draw_accepts=1; runtime window had cacecx=0 (runtime cacecx not promoted; user direct visual check closes area QA). User confirmed normal launch: \"Sim, continua funcionando\" (USER_REPORTED, VISUALLY_CONFIRMED_FOR_TESTED_AREA; broader game/objects/movement parity untested residual). NEXT_SINGLE_ACTION: extend oracle comparison to another attract demo when continuing broader fidelity."
status: evidence
phase: P08
date: 2026-10-19
tags: [p08, attract, demo1, wooded-hollow, map-335, fire-egg, projectile, sweep-branch, target-prefix, first-writer, updrec, custom-hits, liveness, movement-parity, collision, diagnostic, counterfactual, object-frustum, aspect-aware, causality, rng-consumers, grok-implementation]
---

# USER_DEMO1 movement parity and Fire Egg diagnostic discriminator

## 1. Executive summary & epistemic status

This document records the operational state, diagnostic validation, paired telemetry, single-feature counterfactual causality proof, and source-level call chain proof for the investigation of **USER_DEMO1** subtle movement and action parity, using the first Fire Egg shot as a sensitive discriminator.

- **User Observations & Directives (2026-10-19)**:
  - User confirmed that the Demo 7 cutscene stopping failure is **VISUALLY FIXED** by the generator-level static sizing patch (`patches/runtime/n64recomp-zero-size-static.patch`).
  - User confirmed that the Fire Egg miss persists on the current corrected binary (`87c5a08b96b6fbb9c29c0079ac6b7b6060af8f5da6b92147f60e5dacce85bbe5`).
  - User clarified that the primary objective is **general subtle gameplay/action parity** across movement, kinematics, and state transitions, not an artificial aim or hit steering patch. Fire Egg is a sensitive discriminator, not the sole acceptance criterion; no global clock hack.
  - **User Choice Resolved**: KEEP EXPANSION VISUAL ACTIVE while proper fix; **NO temporary OFF default**. Setting flag OFF (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0`) is strictly an experimental per-process counterfactual diagnostic; it is NOT a production mitigation.
  - Production invariants preserved: shader CURRENT `tfPos` workaround remains default-ON (palliative); FPS counter/toggle and RT64 decimator unchanged; audio glitch deferral maintained.
- **Single-Feature Counterfactual Causality (PROVEN)**:
  - **Single Feature Isolation**: On the identical binary (`c56ce4e17b554020c36eec14a9e48861693679e9426161031e10aef4527a67e2`), with identical TARGET logger (`TOOIE_DIAG_DEMO1_TARGET=1`), setting ONLY `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0` (explicit-off getter at `boot_hw_stubs.cpp:4264..4267`, wrapper 4451 calling original `800CACEC` and returning before aspect-aware expansion; no other `TOOIE_TEST_*` flag combined) restored:
    - All 355 `tgtrec` posts 2..356 x 12 raw words bit-exact with the original Angrylion oracle reference (`demo1-first-writer-oracle-2`).
    - All 14 pre/post snapshots across frames 80..86 x 39 ActorRecord words (546 words total) bit-exact with Oracle (`demo1-object-frustum-record39-comparison.json` is `[]` empty list).
    - Record `+0x60` high 16 bits: was `0004` (curr `0004006c`) under default aspect-aware ON vs original `0003` (`0003006c`); with flag OFF, it is now `0003` (`0003006c`), matching Oracle exactly.
    - All 14 raw RNG `D_8012C7F0` snapshots bit-exact with Oracle, completely resolving the PRE82 divergence ($k=4$ `036f8437` vs $k=5$ `c125942a`) and POST83 geometry divergence.
    - All 14 gamespeed `gs` words exact `0x3d4cccce`.
  - **Actual Fire Egg Collision Restored**: In a dedicated PROJECTILE-only capture with the same flag OFF (`TOOIE_DIAG_DEMO1_PROJECTILE=1` and `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0`, controller 1600930 / game 1600931, exit 0/0, 66.824s, 313 rows, FINISH 391, CSV prefix `3d54fe91`):
    - Actual Fire Egg sweep at post 355: `func_800CD820` sweep leave returns `0x80132f60` (nonzero hit, matching Oracle `89a6bad1...`), whereas default production ON returns `0` (miss).
    - Narrowphase `func_800CD6B4` LEAVE = 1 on both off-current and Oracle; default production ON has no such callback at 355.
    - All 22 sweep ENTER input pairs and 21 `sueggmove` ENTER input pairs are exact in `w0..w5` geometry and `w11` `gs`.
    - Sweep/move counts: 44 sweep / 42 move off/oracle vs default 64 / 62 (projectile stops at hit).
    - Bound centers at posts 354 and 355: all 6 selected `0x8008A018` LEAVE center triples exact with Oracle, including actual target post 355 `w0=c5af7c8d, w1=c401d0fa, w2=44a89e51` = `(-5615.56885, -519.265259, 1348.94739)`, completely restoring the prior ~427-unit bound center difference.
  - **Causal Verdict**: `OBJECT_FRUSTUM_ASPECT_AWARE=1` is **CAUSAL** for the observed RNG divergence, NPC trajectory divergence, and Fire Egg collision miss in USER_DEMO1.
- **Source Mechanism Discovered (`PROVEN` Call Chain & Gating)**:
  - Broadly treating `func_800E3E8C` (`E3E8C`) as DRAW scope allows aspect-expanded accept (`v0 = 1`) inside mixed renderer routine `func_800DE498` (`0x800DE6EC`).
  - `func_800DE498` stores `v0` to `0x8012C940`. If nonzero, it executes registered pre-draw callback `0x800DE738: jalr $v0` with `$a0` loaded from `0x8012C9AC`, plus `800D674C` and writes renderer processing context `s0+0x14/0x18/0x1C` (which are renderer context writes, NOT actor state writes; do not conflate), then emits display lists. If culled (`v0 == 0`), it branches to `0x800DE718`, calls reset helper `func_800DE318` and returns 0.
  - Standard actor draw entry `func_80101870` -> `func_80101808` calls `func_800DF47C(func_801015D0, arg0)`, registering callback `func_801015D0` at `0x8012C9A8` and actor pointer at `0x8012C9AC`.
  - Callback `func_801015D0` calls `func_80101934(Actor*)` at `0x80101784`, which indexes jump table `D_801243E8` via `(actor->unk74 & 0xF)`. Slot `+0x04` invokes `func_80101A50(Actor*)`.
  - `func_80101A50` calls `func_800DC0C0` (advancing global LCG RNG seed `D_8012C7F0`, RA `0x80101AA8`), tests random float against `0.03f`, mutates guest actor state byte `sb $t2, 0x7A($a2)`, and calls `func_800DF744` with selectors 1 and 2 writing to `D_8012C850`.
  - **Epistemic Classification**: Semantic identity as facial/eye-blink animation is **`STRONG_INFERENCE`** (3% check and selectors 1/2 strongly suggest blinking, but without explicit asset/animation binding proof in source). Control flow gating, RNG step, actor byte `0x7A` mutation, and renderer array writes are strictly **`PROVEN`**.
- **Paired Native Chain Verification (`PROVEN`)**:
  - Telemetry (`demo1-rng-paired-native-chain-gate.json`): Current has 32 RNG steps vs Oracle 27 across posts 79..84.
  - Sequence gap grows 1..4 across posts (Post 81: +1; Post 82: +1; Post 83: +2; Post 84: +1).
  - LCG chain exhibits **ZERO breaks** across all observed steps and snapshots (`breaks: []`).
  - Expected oracle after-states, once calculated, never match actual observed stores once desynchronized.
  - First extra RNG call is definitively the **SECOND `func_80101A50` call in post 81 (seq 146 in Current, before `6239b638` -> after `036f8437`)**, NOT global `func_8001608C` (which runs in both runtimes).
  - Preceded directly by `cacecx` extra accept owner `func_800DE498` at seq 142.
- **Native PC Mapper & Symbol Resolution**:
  - Old `T/t`-only cache (`demo1-rng-ee14-nm.json`) was **INVALID and incomplete** (omitted recompiled `W/w` weak functions).
  - Completed `nm T t W w` (`demo1-rng-ee14-origin-resolution.json`) validated binary `EE14` (`ee14d6cb1c144a09ccec7f393dc2e986ac26fb0826893d2e0d490df9d19a683c`) with positive function sizes.
  - Return address is guest original `0x80101AA8` (`jal func_800DC0C0` at `0x80101AA0`).
- **Critical Agent Allocation & Roles (2026-10-19)**:
  - **NO SONNET**: Sonnet quota expired. Any further Sonnet calls are strictly forbidden.
  - **Haiku**: Approaching quota limit; minimal/unused now.
  - **Grok 4.7 Medium**: Designated **SOLE** code writer and test executor. Resumed implementation session `01a15118-c738-7dd3-98ed-cc5db5021bb5`: round 1 cap 50 exit 1 with source proof and no edit (`demo1-frustum-parity-grok-implementation-report.txt`); round 2 active budget 70 (`demo1-frustum-parity-grok-implementation-round2.json`, no outcome yet).
  - **Gemini**: Designated for **ALL** Markdown and documentation maintenance.
  - **Native Agent `demo7_capture`**: Read-only Grok review only; NO other capture authority.
  - **Opus**: DONE (read-only existing report `demo1-rng-consumer-opus.json`, not active job).
  - Current code EE14 diagnostics is NOT a fix; original default vis ON CURRENT palliative / FPS / audio unchanged.
- **Main Objective & Scope**:
  - Correct shared `func_800E3E8C` consumer selection to separate genuine visual DRAW passes from logic/simulation callbacks.
  - Preserve existing `8011447C` unless primary simulation harm proof is established (guest outparam write alone is NOT sim evidence).
  - Allowed pure DRAW writes: Gfx buffers, render-only temps.
  - `+0x60` semantic remains strictly UNKNOWN and is not necessary for resolving this cause.

```text
SCENE_IDENTITY: PROVEN (stage 0, sub 0, map 335 entrance 8 mode 5, glrecord ovl 698 cnt 766)
SINGLE_FEATURE_CAUSALITY: PROVEN (TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0 restores 355x12 prefix, 39x14 record words, 14 RNG snapshots, and post 355 Fire Egg hit)
SOURCE_MECHANISM: PROVEN (shared E3E8C broad DRAW scope enables mixed DE498 gate; registered callback 801015D0 -> 80101934 -> D_801243E8 -> 80101A50 steps global LCG RNG D_8012C7F0, mutates actor byte 0x7A, writes D_8012C850 selectors via DF744)
SEMANTIC_CLASSIFICATION: facial/eye-blink animation is STRONG_INFERENCE; control flow, RNG step, byte 0x7A mutation, renderer writes are PROVEN
FIRST_EXTRA_RNG_CALL: PROVEN (second func_80101A50 in post 81 seq 146 curr, preceded by cacecx extra accept in func_800DE498 seq 142; NOT global 8001608C)
PAIRED_RNG_GAP: PROVEN (grows 1..4 across posts, 32 curr vs 27 oracle in posts 79..84; zero breaks in chain; oracle after calculated never matches observed store)
NATIVE_PC_MAPPER: PROVEN (old T/t cache INVALID; completed nm T t W w validated binary EE14 with positive sizes; RA guest 80101aa8)
USER_CHOICE: RESOLVED (KEEP EXPANSION VISUAL ACTIVE while proper fix; NO temp OFF default; env flag 0 is EXPERIMENT ONLY)
GROK_STATUS: COMPLETED (end_turn, root observed CLI exit 0; landed candidate binary A6A2 default ON; bounded parity + user area visual confirmed, broader parity untested)
AGENT_ROLE_ROUTING: Grok PRIMARY code/tests; Gemini ALL Markdown; Haiku minimal/unused; Native demo7_capture read-only; Sonnet NO calls; Opus DONE
PRESERVED_STATE: Object visual pop fix default ON, Demo 7 generator fix, CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
NEXT_SINGLE_ACTION: extend oracle comparison to another attract demo when continuing broader fidelity
```

---


## 2. Scene identity & historical context

USER_DEMO1 is the first playable attract sequence demo:
- **Internal Mapping**: `stage = 0`, `sub = 0`, map 335 (`0x14F`, Wooded Hollow entrance 8), mode 5.
- **Recording Stream**: `glrecord` overlay 698, entrypoint 0, sample count 766 (765 recorded pad samples).
- **Map Variable Behavior**: Logger map field `MEM_H(0x10, 0x80127630)` at `0x80127640` frequently clears to 0 during the mode-5 body after warp consumption.
- **Egg / Projectile Type**:
  - `cheggfire` (overlay 535), `baeggsetup a1=2` FIRE in both Tooie and Angrylion oracle streams.
  - Distinct from USER_DEMO4 (sub 4, historical FIRE vs BLUE mismatch). Egg type is NOT the open question.

### Historical comparison (BASE 876d299f)
In `docs/plan/evidence/p08-demo-movement-precision-diagnosis.md` (§16 and §17):
- Recomp result was recorded as a geometric miss with margin `+36.85` (sweep radius 38, closest approach `glidx 365..366`, target sys `0x80085C98`).
- First C-up input edge occurred at `glidx 322` (stick `(-9, 84)`, buttons `0x2008`), spawn at `glidx 334`.
- **Epistemic boundary**: That historical measurement was taken at BASE `876d299f`. It is **NOT** proof of the exact kinematics, timing, or margin on current binary `87c5a08b...`.

### Historical oracle stream nuance
- `/tmp/p08-angrylion-attract-eggs.log` (append log from earlier phases) contains sub 0 `cheggfire` (overlay 535) with raw post word `333`.
- Raw post word `333` does not contain projectile XYZ, velocity, or collision result.
- Index discrepancy: post word `333` vs first C-up edge `322` vs spawn `334` reflect different hook sites and conventions (cursor post vs consumed index). They must not be collapsed into a single index.

---

## 3. Current bounded SHOT capture audit (NOT collision PASS)

An orchestrator-owned bounded capture was executed on the current binary:
- **Binary**: SHA256 `87c5a08b96b6fbb9c29c0079ac6b7b6060af8f5da6b92147f60e5dacce85bbe5`.
- **Execution**: Owned PID 1059296, command line `./build/TooieRecompiled` with `TOOIE_DIAG_SHOT=1`, terminated cleanly after 66.71 s (`WINDOW_DONE`, exit 0).
- **Artifacts**:
  - `demo1-current-controller.log`: 66.71 s wall time, clean shutdown.
  - `demo1-fireegg-current.log`: 97 `[shot-tick]` rows and 2738 `[shot-act]` rows.
- **First C-Up Rising Edge Telemetry**:
  - POST-labeled cursor: `glidx = 322`.
  - Recorded pad sample: stick `(-9, 84)`, buttons `0x2008`.
  - Player kinematic state:
    - Position: `(-4655.37, -422.81, 1113.69)`
    - Yaw: `296.76°`, Ideal yaw: `299.55°`
    - Target speed: `500.0`, Horizontal velocity: `velxz = 505.64`
- **Flag Isolation Audit**:
  - `TOOIE_DIAG_SHOT=1` activates `g_shot_enabled` only; it does not force `TOOIE_DIAG_ATTRACT`, `TOOIE_DIAG_DEMO_SYNC`, `TOOIE_DIAG_DEMO_PATH`, or `TOOIE_DIAG_MOVEMENT`.
  - It does not cause diagnostic cascades or massive 400k-line log inflation.
- **Deficiencies of SHOT-Only Telemetry**:
  - Lacks `0..305` prefix: no baseline trajectory before the shot is recorded.
  - Lacks current state word: `bs` logged at `*(player + 0x120) + 0` is `bs_prev`, not current state `bs_cur` at `+4`.
  - Lacks muzzle calculation, aim vector, projectile velocity vector, swept collision parameters, and target collider geometry.
  - Absence of a `HIT` suffix does NOT prove a miss; a `HIT` suffix on an unverified target does NOT prove an enemy hit.
  - **Verdict**: The SHOT capture is bounded telemetry, **NOT a collision PASS**.

---

## 4. Target & actor identity invalidation

Analysis of `demo1-fireegg-current.log` resolved a critical actor identification pitfall:
- **Mask `+0x6C & 0x1FF` is INVALID**:
  - The legacy logger extracted actor identity as `*(actor + 0x6C) & 0x1FF`.
  - Successive frames for the same heap actor pointer (`0x801CE954`, sys pointer `0x80085C98`) showed values `mid = 9`, `8`, `7`, etc.
  - Offset `+0x6C` functions as a frame or state countdown, not a static type or actor ID.
  - Filtering or naming actors via `+0x6C & 0x1FF` leads to false classification.
- **Vector `D_80136EE0`**:
  - Scanned elements of size `0x9C` (up to 64 slots) contain various active actors and props.
  - Position within `D_80136EE0` is dynamic and does not guarantee target identity.
- **Required Selection Policy**:
  - The target Wooded Hollow digger must be identified by validated system metadata (`sys == 0x80085C98`), verified via source disassembly, not by low-bit masks or slot indices.
  - Target XYZ from original N64 hardware has not been captured for this binary and remains to be measured.

---

## 5. Diagnostic implementation & build verification (Session 22383)

To isolate the first divergence between TooieRecompiled and original N64 hardware without altering gameplay, Claude Sonnet CLI session 22383 implemented and validated a mirrored diagnostic:
- **Environment Flags**: Isolated, default-OFF flags `TOOIE_DIAG_DEMO1_PARITY=1` (Tooie) and `P08_DEMO1_PARITY=1` (Mupen/Angrylion oracle).
- **Execution Site**: Mirrored at EXACT PC `0x800151BC` ENTRY (start of logic update). Pure memory reads, zero guest state mutation.
- **Scope**: Mode 5, stage 0, sub 0, `glidx` cursor `0..420` (max 421 rows), player 0 only.
- **Schema (62 Columns)**:
  1. `phase` ("entry_800151BC"), `pc` (800151bc), `seq`, `elapsed_ns`, cumulative guest VI handler entries (`0x8001443C`).
  2. Gamespeed word `gs_bits` (%08x) and float `gs` (%.9g) at `0x8012C760`.
  3. `mode`, `stage`, `sub`, `map`, `post`, `consumed`, `count`.
  4. Raw 6-byte pad: `r0..r5` (including stick `x,y` and buttons).
  5. Remap/bstick: `rmx,rmy,bx,by,bang,bdist`.
  6. Facing and kinematics: `yaw,ideal,phys_yaw,yaw_b`, player position `px,py,pz` and bit words `px_b,py_b,pz_b`, previous position `ppx,ppy,ppz`, velocity `vx,vy,vz` and bit words `vx_b,vz_b`, `velxz,tspd,tspd_b,accel,mult,ptype`.
  7. State struct words: `bs_prev` (`+0`), `bs_cur` (`+4`), `bs_next` (`+8`) from `*(player + 0x120)`.
  8. Animation: `anim_id,anim_timer,anim_dur,anim_phase`.
  9. Viewport camera: `camx,camy,camz,camctrl,sel`.
- **Files**:
  - New: `src/game/tooie_demo1_parity.h`, `tests/tooie_demo1_parity_test.cpp`, CMake test target `tooie_demo1_parity_test` (EXCLUDE_FROM_ALL).
  - Modified: `src/game/boot_hw_stubs.cpp`, `src/game/tooie_diag.cpp`, `include/tooie_diag.h`, `tools/p08_attract_egg_hook.h`, `tools/oracle/angrylion-control/patches/mupen64plus-core-p08-instrumentation.patch`.
- **Build & Test Exits**:
  - Unit test `tests/tooie_demo1_parity_test`: PASS exit 0 (header text, 62 conversions, schema layout verified).
  - Ninja build: exit 0 (`TooieRecompiled` binary SHA256 `5b6fc6d069f00411cfd2ade4b94d7fa15055a44206bc100ffbd5ba569af31bc6`).
  - Oracle build: `tools/oracle/angrylion-control/scripts/build.sh --force` exit 0 (lib SHA256 `c2a9e7fa7a3476c4a6eacb35bedc30c3d8128c16a28e190fbf0c0e96c05d9c04`).
  - `git diff --check`: exit 0.

---

## 6. Controlled paired captures (Tooie vs Pinned Angrylion Oracle)

With the diagnostic validated, orchestrator-controlled captures were executed:
- **TooieRecompiled Capture**:
  - Script: `/home/wesley/.cache/tooie-fps-investigation/demo1-parity-current-capture.py`
  - Controller PID 1073249, game PID/pgid 1073258. Command: `stdbuf -eL -oL build/TooieRecompiled`.
  - Duration: 67.679 s. Termination reason: `window_consumed_420`. Exit code 0.
  - Output log: `/home/wesley/.cache/tooie-fps-investigation/demo1-parity-current/console.log` (501,453 bytes, SHA256 `57920a649162c7bec4b17dd3d0e86ef41eed0fb813c98008a6323e5fe83492aa`).
  - Content: 1 header row, 420 data rows, 0 bad rows.
- **Pinned Angrylion Oracle Capture**:
  - Script: `/home/wesley/.cache/tooie-fps-investigation/demo1-parity-oracle-capture.py`
  - Controller PID 1074236, oracle PID/pgid 1074257. Command: `tools/oracle/angrylion-control/run.sh`.
  - Flags: `P08_DEMO1_PARITY=1`, fresh CSV path, `TOOIE_ANGRYLION_AUDIO=dummy`, `--nospeedlimit`.
  - Duration: 98.713 s. Termination reason: `window_consumed_420`. Exit code 0.
  - Output CSV: `/home/wesley/.cache/tooie-fps-investigation/demo1-parity-oracle/demo1.csv` (179,297 bytes, SHA256 `fa45b8d202a791eba76b6ef0f2b20265ed2297cddc25eb0c90b11811f55736e3`).
  - Content: 1 header row, 420 data rows, 0 bad rows.
  - **Wall-time status**: PERTURBED under `--nospeedlimit`. Logical parity only; not real-time speed proof.
- **Process Safety**:
  - All owned capture processes exited cleanly. No processes were relaunched for analysis.
  - Pre-launch scan and post-exit scan verified no user game process was signaled or killed.

---

## 7. Paired comparator verdict & divergence analysis (Grok report & compare JSON)

The two 420-row datasets were independently compared using `demo1-parity-compare.py` (JSON output: `/home/wesley/.cache/tooie-fps-investigation/demo1-parity-compare.json`) and arbitrated by Grok (`demo1-paired-main-grok-report.txt`):
- **Key & Boundary Alignment**:
  - Joined strictly on `consumed` index (`consumed = post - 1`).
  - Exactly 420 paired rows (`consumed = 1..420`).
  - `tooie_only`: empty (0). `oracle_only`: empty (0). Gaps: none (0).
  - Both sides miss **ONLY consumed 0** (`post = 1` never logged at this hook). Identical skip proves no port split or dropped ticks.
- **Prefix Invariance (PROVEN MATCH)**:
  - **Zero 1-ULP differences, zero geometric or action differences** across all sampled player and camera fields from consumed 1 through 420.
  - Identical text / bit-level match on:
    - Raw input pad bytes `r0..r5`
    - Gamespeed word `gs_bits`: both `0x3D4CCCCE` (0.05) and `0x3D088889` (0.03333) occur and match at every post, including C-up row 322 (`0x3D4CCCCE`).
    - Facing yaw, ideal yaw, physical yaw (`yaw, ideal, phys_yaw, yaw_b`)
    - World position `px,py,pz` and bit words `px_b,py_b,pz_b`
    - Velocity `vx,vy,vz`, bit words `vx_b,vz_b`, horizontal velocity `velxz`, target speed `tspd, tspd_b`
    - State words `bs_prev, bs_cur, bs_next`
    - Animation `anim_id, anim_timer, anim_dur, anim_phase`
    - Viewport camera `camx, camy, camz, camctrl, sel`
- **First In-Window Divergence: Guest VI-Handler Entry Cadence**:
  - `vi` measures cumulative guest entries of `0x8001443C`.
  - Absolute `vi` at consumed 1 differs due to pre-window boot differences (Tooie 3134, oracle 3414).
  - In-window adjacent `dvi` mismatches: **100 of 419 transitions**.
  - First divergence at step consumed 1->2: Tooie `dvi = 0` (remains 3134), oracle `dvi = 6` (3414 -> 3420).
  - In-window cumulative growth: Tooie +847 entries, oracle +887 entries (+40 oracle entries).
  - **Epistemic Classification**: PROVEN as an emulator/runtime cadence difference between RT64/recomp and mupen pure interpreter under `--nospeedlimit`. It did NOT perturb any sampled guest input, pose, velocity, yaw, animation, or camera field. It does not justify any clock multiplier or game-speed hack.
- **Independent Gate Confirmation (Haiku `demo1-actual-pair-haiku-report.txt`)**:
  - Independent read-only parser (`gate.py`, stdlib only) re-evaluated actual CSV and console log files without using `compare.json`.
  - Confirmed 420 unique contiguous consumed indices (1..420) in order with 0 field mismatches across all non-clock columns.
  - Confirmed keyed shift test on pad bytes `r0..r5`: shift 0 = 0 mismatch rows; shift ±1/±2 = 286..295 mismatch rows, proving input rows are strictly aligned and not blanket-shifted.
  - Assessed VI cadence delta: 100/419 step mismatches; reads as physical frame scheduling / VI-delivery cadence difference under `--nospeedlimit` (INFERENCE); NOT proven root cause. Sampled game-state fields remain exact on every row while VI cadence differs. Hit/miss remains strictly UNKNOWN.
- **Final Paired Gate Verdict**:
  - `player/input/camera/gs prefix MATCH through consumed 420. HIT UNKNOWN. No code fix. Not PASS. Not a visual-miss proof. Not a realtime proof. No aim, hit, or clock change.`

---

## 8. Fire-edge sample & projectile dispatch telemetry

- **First C-Up Rising Edge (Consumed 322 / Post 323)**:
  - Recorded pad sample: `r0..r5 = 247, 84, 32, 8, 3, 0` (stick `x = -9`, `y = 84`, buttons `0x2008` = C-up with Z).
  - Exact match between Tooie and oracle at PC `0x800151BC` ENTRY:
    - `px,py,pz = -4692.11475, -426.484711, 1129.06921` (`px_b = c592a0eb`, `py_b = c3d53e0b`, `pz_b = 448d2237`)
    - `yaw = 291.797241°` (`yaw_b = 4391e60c`), `ideal = 288.833771°`, `phys_yaw = 292.704498°`
    - `vx,vy,vz = -265.320801, -1.0, 111.010895`, `velxz = 287.608307`
    - `bs_prev = 7`, `bs_cur = 9`, `bs_next = 0`
    - `anim_id = 42`, `anim_timer = 0.0500000045`, `anim_dur = 1.0`, `anim_phase = 0.0500000045`
    - `cam = -4086.1438, -160.789032, 950.552246`, `camctrl = 106.851303`
    - `gs_bits = 3d4cccce` (0.05)
  - **Phase Distinction**: This entry pose is distinct from the later SHOT leave pose on `87c5...` (`-4655.37, -422.81, 1113.69`). Both facts stand; they represent different phases and binaries.
- **Oracle Egg Append Tail Telemetry (`/tmp/p08-angrylion-attract-eggs.log`)**:
  - File size 56,989 bytes, 426 lines (preserved historical log).
  - Dispatch occurs at **raw post 333** (consumed 332):
    - `[egg-dispatch] pc=80086D30 a0=80084a58 a1=00000000 ra=801e5e0c ... post=333`
    - `[egg-sys] pc=800871f0 ovl=535 ep=2 a0=801d6638 a1=801d6644 ra=801e5c38 ... post=333 cheggfire obj64=c5932bf8 obj6c=448e0aee`
  - `obj64 = c5932bf8` decodes to f32 `-4709.49609375`. `obj6c = 448e0aee` decodes to f32 `1136.341552734375`.
  - These words do not equal player position bits (`px_b = c592a0eb` = -4692.11).
  - Callee `800871f0` overlay 535 ep 2 is the actual `cheggfire` sys dispatch at raw post 333, distinct from the C-up input at consumed 322.

---

## 9. Independent contract & capture audits

Two independent audits were executed in parallel under user authorization:
1. **Grok ABI / Numeric / Timing Contract Audit (`workload-grok-parity-audit-report.txt`)**:
   - Process PID 1065434: **ACTIVE** in background.
   - Verified compiler flags: `RecompiledFuncs` uses `-fno-strict-aliasing` and `RECOMP_MIPS_ROUND_COMPAT=1` (nearest-even rounding); `TooieRecompiled` does not set `RECOMP_MIPS_ROUND_COMPAT` (native C++ math helpers return float in `f0` and are not ROUND instructions).
   - Override mechanism: installed via `register_boot_lookup_stubs` -> `add_loaded_function`.
   - Math leaf inventory: `800136E4` (`tooie_wrap_degrees` via `std::fmod`), `800138D0` (`tooie_cos_degrees` via `cos`), `80013970` (`tooie_sin_degrees` via `sin`). Degree constant `pi/180 = 0.01745329251994329577f`.
   - Open hypotheses: H1 (libm sin/cos/fmod vs ROM N64 sine table), H2 (`800BCCC0/800BCCD8` delay slot folding / sign extension), H3 (`800979D8` unheld write), H4 (cadence wrapper guest state perturbation).
   - Differentiated diagnostic binary `5b6fc6d0` from user-bug baseline `87c5a08b`.
2. **Haiku Capture & Comparator Validity Audit (`workload-haiku-capture-audit-report.txt`)**:
   - Evaluated 20 static gates G1-G20.
   - Verified mechanical 62-column schema parity between Tooie and oracle hooks (G20 PASS).
   - Flagged G15: Oracle CSV default path append mode (`"a"`) risk of duplicating headers if run repeatedly on the default file. Grok's capture script avoided this by supplying a fresh per-run CSV path (`demo1-parity-oracle/demo1.csv`).
   - Flagged G14: Absence of per-row PID/run ID tokens.
   - Temporal note: Haiku evaluated static code before captures ran; Grok ran captures subsequently. Preserved without contradiction.

---

## 10. Prior background audits & process tracking

- **Active Background Agent (Historical Phase)**:
  - `PID 1065434`: `/home/wesley/.local/bin/grok` running `workload-grok-parity-audit`.
- **Historical Exited Processes**:
  - Background Driver: `PID 1067821` (`demo1-background-driver.py`).
  - Grok Paired Runner: `PID 1067822` (exited 0).
  - Tooie Parity Capture: Controller `PID 1073249`, Game `PID 1073258` (exited 0).
  - Oracle Parity Capture: Controller `PID 1074236`, Oracle `PID 1074257` (exited 0).
  - Previous Bounded SHOT Capture: `PID 1059296` (exited 0).
  - User Game: `PID 1050817` (closed by user).

---

## 11. Projectile diagnostic design & implementation (Sonnet session)

To investigate projectile spawn and propagation downstream of logic entry, Claude Sonnet CLI session (report `demo1-projectile-diagnostic-sonnet-report.txt`, JSON `demo1-projectile-diagnostic-sonnet.json`) implemented an isolated, default-OFF projectile diagnostic:
- **Status & Execution**: Completed with `is_error = false`. Ninja build exit 0; unit and regression tests reported 0 failures.
- **Environment Flags**:
  - Tooie: `TOOIE_DIAG_DEMO1_PROJECTILE=1` (default OFF).
  - Oracle: `P08_DEMO1_PROJECTILE=1` with `P08_DEMO1_PROJECTILE_CSV=<fresh_path>` (default OFF, file opened `"w"`).
- **Five Instrumented Entry/Return Points**:
  1. `_baeggsetup_entrypoint_5` @ `0x80084A60` (orig `0x8080042C`): player ps in `a0`, writes three vec3s at `*(ps + 0x70)`.
  2. `_cheggfire_entrypoint_2` @ `0x800871F0` (orig `0x80800304`): egg spawn dispatch, returns spawned object in `v0`.
  3. `_sueggmove_entrypoint_9` @ `0x8008A480` (orig `0x80800360`): movement update.
  4. `_suegg_entrypoint_9` @ `0x8008A3C0` (orig `0x80800558`): suegg collision handling.
  5. `func_800CD820` (core2): spherical sweep testing, returns `v0` (hit object or 0).
- **Tracked Object Polling (`objpoll`)**:
  - Because actors and projectiles dispatch per-frame updates via overlay-relative callback tables (`0x808xxxxx`), direct wrappers alone may not intercept all frame callbacks.
  - `objpoll` samples all tracked projectile objects per logic frame at `0x800151BC` ENTRY across 37 columns.
- **Binary Artifacts**:
  - Tooie executable: SHA256 `af0723b4b7f802446b455401b1c9e83a50aa61772ec37a598aa451df7f666705`.
  - Pinned Oracle core: SHA256 `41b59e2802be5782a2f3ab4f685bbad2fee062bd4da72e8ee01c6d6c4b7ebe88`.
- **Epistemic Classification**: Diagnostic ONLY; zero production behavior modification.

---

## 12. Haiku independent static gate on projectile diagnostic

Haiku evaluated the projectile diagnostic implementation (`demo1-projectile-haiku-gate-report.txt`) prior to runtime execution:
- **Evaluated Gates (PASS)**:
  - G1/G2: Exactly-once call to original code on both sides (Tooie calls orig once; Oracle traces PC).
  - G3/G4: Context / register preservation (reads `r4..r7` before call, untouched context passed, reads `r2` after; no guest writes).
  - G5/G6: Calling conventions and pointer/float-bit interpretation (radius filter `0x42180000` = 38.0f raw bits).
  - G7: Nested return handling (stack depth 8; delay-slot returns handled).
  - G9: 37-column CSV schema parity between Tooie and Oracle formats.
  - G10/G11/G12/G13/G14: Default-OFF gate, read-only memory access, CSV file semantics, stderr prefix, window marker semantics.
- **Coverage Caveat (G8)**:
  - The static gate validates direct calls and `LOOKUP_FUNC` sites.
  - Indirect calls dispatched through overlay-relative descriptor or callback pointers (`0x808xxxxx`) are NOT guaranteed to be intercepted by Tooie wrappers.
  - Missing callback rows alone are NOT proof of game divergence; `objpoll` provides the necessary fallback telemetry.

---

## 13. Controlled projectile captures (Current Tooie vs Pinned Oracle)

Serial controlled captures were executed using `demo1-projectile-owned-capture.py` without touching user game processes:
- **Current Tooie Capture**:
  - Controller PID 1282788, Game PID/pgid 1282791.
  - Binary: SHA256 `af0723b4b7f802446b455401b1c9e83a50aa61772ec37a598aa451df7f666705`.
  - Duration: 66.243 s. Exit codes: 0 / 0.
  - Reason: `FINISH_MARKER` (post 391).
  - CSV: `/home/wesley/.cache/tooie-fps-investigation/demo1-projectile-current/projectile.csv` (SHA256 `6b5686b77b723180f08954d3573be31d73294140b22815ae39f90725432775c7`).
  - Rows: 1 header row + 189 data rows.
  - Event Breakdown: setup 1 pair, fire 1 pair (post 333), movement 31 pairs, sweep 32 pairs, objpoll 57.
- **Pinned Angrylion Oracle Capture**:
  - Controller PID 1283584, Game PID/pgid 1283587.
  - Core: SHA256 `41b59e2802be5782a2f3ab4f685bbad2fee062bd4da72e8ee01c6d6c4b7ebe88`.
  - Settings: Fresh isolated config/states/saves, `--nospeedlimit` (PERTURBED_LOGICAL_ONLY).
  - Duration: 94.041 s. Exit codes: 0 / 0.
  - Reason: `FINISH_MARKER` (post 391).
  - CSV: `/home/wesley/.cache/tooie-fps-investigation/demo1-projectile-oracle/projectile.csv` (SHA256 `8bd18fd1444d6f8f6305b200f0c5afc68d0401739b6c4f10af29d42261568aab`).
  - Rows: 1 header row + 92 data rows.
  - Event Breakdown: setup 1 pair, fire 2 enter / 1 leave, movement 7 pairs, sweep 7 pairs, objpoll 57.
- **Process Accounting**: Both runs exited 0 cleanly. All command lines, environments, and hashes are preserved in controller JSONL logs.

---

## 14. Projectile telemetry comparison, old-pointer freeze, and defrag hypothesis

Comparison of the two projectile CSV captures reveals bounded parity and a crucial pointer-lifecycle divergence:
- **Spawn & Early Propagation Parity (PROVEN)**:
  - `baeggsetup5` enter and leave: raw payload bit-exact between Tooie and Oracle.
  - `cheggfire2` spawn enter and leave: raw payload bit-exact at post 333.
  - Matched movement and sweep pairs (`post 333..340`): all 7 pairs have bit-exact raw payloads.
  - All 7 oracle sweep leave rows record `v0 = 0` (same as Current: no collision hit detected). A baseline of no hit is proven for this early window.
- **Oracle Pointer Freeze at Post 341**:
  - First raw `objpoll` XYZ difference occurs at `post 341` (~40 unit XYZ difference).
  - Inspection of the Oracle telemetry shows that the tracked pointer `0x801d0a2c` freezes at `post 341..342`.
  - At `post 343`, the memory at `0x801d0a2c` exhibits heap link words.
  - At `post 388`, `0x801d0a2c` contains MIPS instruction words.
  - **Epistemic Classification**: Object liveness, relocation, or freeing is strictly **UNKNOWN** (possible DEFRAG HYPOTHESIS). This pointer freeze is **NOT proven as a gameplay divergence**; the physical egg object may have been relocated on Oracle while the hook continued tracking the stale pointer. Current tracked pointer `0x801c767c` continues normal movement.
- **Unmeasured Snapshot Parameters**:
  - Target/muzzle final normals and per-callback delta-time are NOT captured (`w11` unused).
  - VI cadence cannot be inferred as the root cause of downstream trajectory differences without liveness-tracked data.

---

## 15. Active background workloads & process tracking

- **Interrupted Redundant Audit**:
  - Prior Grok main 40-turn audit PID 1274832 was interrupted via SIGINT after 13 read-only turns (redundant gates before runtime); supervisor PID 1270040 was terminated via SIGTERM.
  - Classification: `INTERRUPTED_REDUNDANT_AUDIT_DIRECT_CAPTURE` (not PASS). Raw JSON may be empty, source transcript preserved.
- **Auditor & Job Status**:
  - `Grok PID 1284587` (max 12 turns) and `PID 1295061` (max 5 turns): **INCOMPLETE**. Parser/cache exists for old 341 raw diff, but no formal new claim has completed. Mark explicitly incomplete; no generic 'Grok PASS'.
  - `Sonnet PID 1286150`: **COMPLETED** (`demo1-projectile-liveness-sonnet-report.txt`). Proved owner-aware alias at post 341.
  - `Sonnet PID 1303866`: **COMPLETED** (`demo1-sweep-first-branch-sonnet-report.txt`). Proved post 354/355 qualifying candidate bound center divergence.
  - `Sonnet PID 1314417`: **COMPLETED** (`demo1-target-first-producer-sonnet-report.txt`). Proved MIPS helper chain (`80106790`, `vector_at`, `8008A018`, descriptor bit 1 branch).
  - `Haiku Static Gates`: **COMPLETED exit 0** for actual pair 2 and pair 3 (raw claims confirmed).
  - `Native Checker`: `demo1-sweep-par3-native-check.py` exit 0 (`demo1-sweep-par3-native-check.json`), validates raw bits, counts, centers, radius, and return class without depending on LLM output.

---

## 16. Telemetry comparison, alias resolution, and hit divergence (Pair 2)

Pair 2 owner-aware telemetry resolved the pointer lifecycle question:
- **Oracle Pointer Freeze Superseded (PROVEN)**:
  - Oracle ALIAS at `post 341` recovered the same owner object (`0x80133530`) at a new heap address (`0x801d085c`).
  - The previous pointer freeze at post 341 is **CORRECTED/SUPERSEDED** as a diagnostic stale pointer artifact, NOT gameplay divergence.
  - Return retry dedup / unmatched 0 validated. Query vectors, radius 38, and callback `gs` match.
- **Collision Hit Divergence at Post 355 (PROVEN)**:
  - At `post 355`: Oracle `func_800CD820` sweep leave returns nonzero hit (`v0 = 0x80132f60`).
  - Current Tooie returns `v0 = 0` (no hit detected).
  - First proven collision outcome divergence occurs at `post 355`.

---

## 17. Pair 3 sweep branch & qualifying candidate bound center divergence (post 354..355)

Pair 3 diagnostic isolated the spatial sweep candidate evaluation inside `func_800CD820`:
- **Capture Metrics**:
  - Current Tooie: 321 rows, 65.994s, exit 0, CSV SHA256 `343ba0401cd45bedcef7e84b2b90781f856038d558c8200395a0b0f5d3aa480e`.
  - Oracle: 315 rows, 94.712s, exit 0, CSV SHA256 `89a6bad151beed9b0d3d852f468767e1a5dae75272248ed95ce47da3d7776660`.
  - Controllers PID 1306868/1307713 are HISTORICAL exited.
- **Candidate List Traversal**:
  - Query 354 and 355 geometry, radius 38, and `gs` are EXACT bit-for-bit.
  - 5 spatial cell lists tested (counts 2, 4, 2, 1, 3 on both; 24 candidates total, no cap hit).
- **First Qualifying Candidate Center Divergence**:
  - First QUALIFYING candidate custom bound center returned by `_subaddiecustomhits_entrypoint_5` (`0x8008A018`) differs already at post 354:
    - Current: `(-5239.2832, -497.945862, 1552.26599)`
    - Oracle: `(-5617.354, -519.254761, 1345.82104)`
    - Raw Euclidean distance delta: `431.2900595`.
  - Post 355:
    - Current: `(-5235.08984, -496.966675, 1541.98865)`
    - Oracle: `(-5615.56885, -519.265259, 1348.94739)`
    - Raw Euclidean distance delta: `427.2311169`.
  - Bound radius returned is IDENTICAL: `279.449982`.
  - Other 2 qualifying candidate centers match identically between Current and Oracle.
  - Sweep execution: Oracle advances through `EC3C4` / `CD6B4` to hit at 355; Current fails an earlier distance branch.
  - **Critical Distinction**: The first candidate in the candidate list is **NOT** the first qualifying candidate.

---

## 18. Target candidate structure, vector_at(*D_80136EE0), and descriptor branch (Source MIPS)

Source MIPS decompilation of the hit target producer chain:
- **Marker Candidate Identity**:
  - Hit marker `0x80132f60` present in both captures.
  - Header IDs: `lhu(marker + 0x14) = 0x0671`, `lhu(marker + 0x18) = 0x0E3F`.
  - Target class name remains **UNKNOWN** pending descriptor branch resolution.
- **Component Record Lookup (`func_80106790`)**:
  - `func_80106790(marker)` computes index `a1 = lhu(marker + 0x1A) >> 5`.
  - Calls `vector_at(*D_80136EE0, a1)`.
  - Implementation: `begin = *(vec + 4) + idx * *(vec)` with record stride `0x9C`.
  - Returns component record address (`0x801c6434` Current vs `0x801cf614` Oracle).
- **Bound Center Calculation (`_subaddiecustomhits_entrypoint_5` @ `0x8008A018`)**:
  - Checks descriptor word `t7 = *(*(*a0) + 8)`.
  - If bit 1 is SET: copies record `+4/+8/+0xC` center floats directly.
  - ELSE: branches to `func_800EC75C` custom offset calculation.
  - **Epistemic Boundary**: Relation between bound center and world/base position is UNKNOWN pending descriptor bit 1 branch proof; NEVER call the 427-431 delta an enemy/world position difference yet.
- **Native Implementation Review**:
  - No exact native override bug found in `8008A018`, `80106790`, `800B28CC`, `80103328`, or `800EC75C`.

---

## 19. Diagnostic halfword reader correction

- Prior host reader `tooie_d1p_hu` required 4-byte aligned addresses.
- Halfwords located at `+2` within an aligned word (including `+0x66`, `+0x6A`, `+0x1A`) falsely returned 0 on Tooie.
- Native helper was corrected to inspect the containing aligned word.
- Oracle correct reader 0 still observed. Does not alter guest production logic.

---

## 20. Target prefix selector fix & real fixtures validation

- **Root of the Initial 0-Row Failure**:
  - The initial predicate required `lhu(marker + 0x18) == 0x0E3F` based on the oracle hit-leave identity words.
  - Inspection of real `s_106790` enter rows demonstrated that `lhu(marker + 0x18) == 0x0E21` for marker `0x80132f60` on BOTH runtimes (Current: 2 rows; Oracle: 4 rows; posts 354/355). Only the later hit-leave read has `0x0E3F`. The upper bits of `+0x18` and `+0x28` are mutable state, not static identity.
- **Fixed Selector Rule**:
  - `lhu(actor + 0x14) == 0x0671 && (lhu(actor + 0x18) & 1) != 0` (class ID 0671 + bit 0 active flag).
  - The first matching handle binds; any different handle matching the rule is logged as an ambiguity event (`tgtambig`, max 8 rows) and does not rebind.
- **Fixture Regression Test & Haiku Gate**:
  - Validated via `tooie_demo1_target_selector_test` on fixtures extracted directly from real PAR3 sweep CSVs (`tests/fixtures/demo1_s106790_enter_{current,oracle}.csv`):
    - Current: 6 enter rows, 2 matches, 0 old matches, bound `0x80132f60`, exit 0.
    - Oracle: 8 enter rows, 4 matches, 0 old matches, bound `0x80132f60`, exit 0.
    - Old predicate compiled into test failed immediately (`exit 1`); new header passed cleanly (`exit 0`).
  - Independent Haiku gate (`demo1-target-selector-haiku.json`) verified test exit 0 on both fixtures and verified rule logic. Haiku noted the semantic caveat: sweep CSVs do not log `lhu14/lhu18` on LEAVE rows; `0x0E3F` should resolve from source `v0+14/18` in sweep leave rather than assuming a0.

---

## 21. Paired target-2 capture telemetry & first actor divergence (post 83)

A fresh paired target capture was completed for both runtimes under `TOOIE_DIAG_DEMO1_TARGET=1` and `P08_DEMO1_TARGET=1`:
- **Execution & Capture Metrics**:
  - Current: `/home/wesley/.cache/tooie-fps-investigation/demo1-target-current-2/target.csv` (359 rows, FINISH 357, exit 0).
  - Oracle: `/home/wesley/.cache/tooie-fps-investigation/demo1-target-oracle-2/target.csv` (359 rows, FINISH 357, exit 0).
  - Both controllers exited cleanly with code 0.
- **Handle Multiplicity (3 Handles with ID 0x0671)**:
  - `tgtfound` / `tgtambig` telemetry revealed three distinct handles sharing class ID `0x0671`:
    1. `0x80132f30`: First seen at `post 2`, idx = 2 (`lhu(+0x1A) >> 5`), `*(+0x10) = 0x0d72`, `*actor = 0x801d1818`, `+0x28 = 0x00000202`. Bound first by the selector.
    2. `0x80132f00`: First seen at `post 28`, idx = 1, `*(+0x10) = 0x0d70`, `*actor = 0x802193dc`, `+0x28 = 0x00000202`. Logged as `tgtambig`.
    3. `0x80132f60`: First seen at `post 140`, idx = 3, `*(+0x10) = 0x0d74`, `*actor = 0x802193f4`, `+0x28 = 0x70000202`. Logged as `tgtambig`.
  - Because the selector bound the first matching handle (`0x80132f30`), the 355 `tgtrec` rows tracked `0x80132f30`, NOT the actual hit marker `0x80132f60`.
  - Target trajectory of the actual hit actor `0x80132f60` is therefore NOT captured in this dataset.
- **First Actor (`0x80132f30`) Telemetry Analysis**:
  - Across the 355 paired `tgtrec` rows:
    - Positions (`+4/+8/+0xC` floats) are **BIT-EXACT** for all posts `2..82`!
    - **FIRST Raw Position Divergence occurs at Post 83**:
      - Current raw words: `44e16cc0  c309103c  c30eda4e`
      - Oracle raw words:  `44e16e09  c3090ffe  c30ea630`
    - Gamespeed `gs` (w11) is **BIT-EXACT** across all 355 posts (`w11` matches bit-for-bit).
    - Other actor record words first diverge at post 121 (field semantic meaning UNKNOWN).
    - First actor callback record is `0x80085C98` (getter class descriptor, not update entry).

---

## 22. Grok first NPC prefix audit completion (DONE)

- **Job Status & Replacement**:
  - The previous pending Grok data-only prefix audit completed cleanly (`stopReason: end_turn`, 7 turns, CLI exit 0, replacing earlier pending status).
  - Artifacts: `/home/wesley/.cache/tooie-fps-investigation/demo1-first-npc-prefix-grok-report.txt` and `demo1-first-npc-prefix-grok.json` (summary at `demo1-first-npc-prefix-grok-summary.json`, comparator script `demo1-first-npc-prefix-grok-compare.py`).
  - Inputs verified: Current CSV SHA256 `46c6ef9d5055be0b0700030645fe03cd78b9af1f998823f4777e00dfc79d23e2`; Oracle CSV SHA256 `d8631d86085dd723f02e914fc4ddba9f615e9d153ac495c687bdc73624319f84` (359 data rows, 37 columns each, controller return code 0).
- **Trajectory Join & Marker Separation**:
  - Join consists of 355 `tgtrec` pairs, posts 2 through 356 inclusive, with no gaps or duplicate posts.
  - Three distinct handles identified:
    1. `0x80132f30`: bound at `tgtfound` post 2 (`a1 = 0x00000002`). This is the continuously recorded stream (`v0 = 0x80132f30`, callback word `w3 = 0x80085C98`).
    2. `0x80132f00`: marker row at post 28 (`a1 = 0x00000001`). Ambiguous other handle.
    3. `0x80132f60`: marker row at post 140 (`a1 = 0x00000003`). Projectile target candidate, NOT the sampled trajectory.
  - Post 357 is `tgtfinish` on both runtimes (`w0 = 358`, `w1 = 2`, `w2 = 2`, `w3 = 3`, then zeros); does not name an actual hit actor.
- **Position & Word Analysis**:
  - Position words `w0`, `w1`, `w2` (raw float32 hex words) match bit-for-bit across posts 2 through 82 (81 pairs).
  - First raw position difference occurs at post 83: Current `44e16cc0 c309103c c30eda4e` vs Oracle `44e16e09 c3090ffe c30ea630` (Euclidean distance delta `0.207508441`).
  - All posts 83 through 356 differ on all three axes; divergence grows monotonically (post 90 delta 3.006, post 356 delta 637.92).
  - All 2130 position words across both runtimes are finite and nonzero (no NaN, Inf, +0, -0).
  - Gamespeed `w11` is raw-exact across all 355 pairs (`0x3d4cccce` and `0x3d088889`).
  - Other record words: `w3`, `w4`, `w8` have no difference. `w10` first differs at post 121 (`04311038` vs `04311048`, XOR `0x70`), `w9` first differs at 171, and `w5..w7` first differ at 215. Mismatches in non-position words are preserved as raw words without inferring physical deltas.
  - Self-check: in-memory mutation flipping bit 0 of `w0` at post 40 moved the first position difference from 83 to 40, verifying detector sensitivity.

---

## 23. Preceding first-writer capture diagnostic failure & Sonnet fix

- **Observed Failure in First Writer Capture**:
  - Paired run `demo1-first-writer-current` (controller 1574609 / game 1574612, exit 0/0, 65.870s) yielded 359 prefix rows and 0 `updrec` rows.
  - Classified as a **diagnostic FAIL**, not a gameplay divergence or parity pass.
- **Root Cause (Type Confusion & Host RA Unreliability)**:
  1. **Host RA Unreliability in Recomp**: In generated code (`funcs_338.c`), `func_800FF62C` performs `jal 0x800FF6D4` (LOOKUP_FUNC for `func_80100368`) without setting `ctx->r31` immediately before the call (only prologue save and epilogue restore exist in the host C function). Consequently, guest RA in `ctx->r31` at `func_80100368` entry is stale and caller-dependent. Any range guard on RA in the recomp host is unsound and rejects real direct calls.
  2. **MARKER vs RECORD Arg Confusion**: `func_80100368(a0)` executes `jalr *(a0+0x10)`. Loop function `func_800FF62C` iterates the actor record vector and passes the actor RECORD (`s0` returned by `func_801067C4`) as `a0`. The earlier wrapper compared `a0` with `s_d1t_marker` (the MarkerHandle), which was never true (`accepted_real_call_arg = 0`).
  3. **Post Dump Mistake**: Post flush attempted to dump 39 words from the MarkerHandle address rather than resolving the active ActorRecord pointer.
- **Sonnet Correction (Default-OFF)**:
  - Header `src/game/tooie_demo1_projectile.h`: `upd_expected_record(lhu14, begin, stride, idx)` resolves the expected record pointer dynamically; `upd_accept` compares `record_arg` with `expected_record` within call-local SP scope; `kUpdChunkWords = 10` (`w10 = MarkerHandle`, `w11 = gs`).
  - Host `src/game/boot_hw_stubs.cpp`: `tooie_d1t_w_100368` passes `record_arg` (`ctx->r4`) and the resolved record to `upd_accept`; PRE dumps from the record; `tooie_d1t_upd_flush` re-resolves the record from `s_d1t_marker` for POST dump (emitting `postmiss` if unresolvable). Call-local SP scope (`ctx->r29 == scope_entry_r29 - 0x58`) isolates direct calls from `800FF62C`.
  - Oracle `tools/p08_attract_egg_hook.h`: mirrored comparison using exact MIPS return addresses `{0x800FF6DC, 0x800FF718}` (valid on MIPS interpreter).
- **Regression Fixtures (355x2 Rows)**:
  - Validated via `tooie_demo1_upd_layout_test` using real fixtures `tests/fixtures/demo1_tgtrec_ident_{current,oracle}.csv` (355 rows each):
    - Old-wrong build (`-DTOOIE_UPD_TEST_OLD_MARKER_COMPARE`): exited code 1 (FAIL: `accepted_real_call_arg = 0`).
    - Fixed build: exited code 0 (PASS: `accepted_real_call_arg = 355/355` on both runtimes).
    - Correctly handles dynamic actor vector relocations across frames: 5 relocations on Current fixture, 7 relocations on Oracle fixture.

---

## 24. Independent Haiku ABI analysis (`func_80100368` and SP scope)

- **Independent Inspection (`demo1-writer-record-abi-haiku-report.txt`)**:
  - `func_80100368` ABI (`RecompiledFuncs/funcs_137.c`):
    - Stack adjustment: `sp -= 0x18`; loads `ctx->r25 = MEM_W(ctx->r4, 0x10)`; dispatches `LOOKUP_FUNC(ctx->r25)(rdram, ctx)`; restores ra; returns.
    - `ctx->r4` (`a0`) is unmodified before the call. Callee receives `a0 = record` unchanged.
    - Argument `a0` **must** be the actor RECORD. Passing MarkerHandle `0x80132f30` would dispatch `*(0x80132f30 + 0x10) = 0x00000d72` as code (invalid dereference).
  - Caller structure (`RecompiledFuncs/funcs_338.c`, `func_800FF62C`):
    - Adjusts `ctx->r29 -= 0x58` at entry (line 2367).
    - Direct calls at lines 2515 and 2570 pass `ctx->r4 = ctx->r16` (`s0` = record returned by `801067C4`).
    - Confirmed no `ctx->r31 =` assignment at the jal sites in host C.
    - Direct-call discriminator is strictly call-local SP: `ctx->r29 == scope_entry_r29 - 0x58`. Nested calls from actor callbacks run at deeper stack frames (`r29 < scope_entry_r29 - 0x58`).
  - Correction of Haiku Resolver Criticism:
    - Haiku noted that `func_800FF62C` does not read `marker+0x1A`.
    - Haiku's initial criticism was corrected to **`NOT_TESTED`**: absence of `marker+0x1A` within the loop body does not refute external mapper `func_80106790`.
    - Independent source disassembly of `func_80106790` confirms it explicitly computes `index = lhu(marker+0x1A) >> 5` and calls `vector_at(*D_80136EE0, index)`.

---

## 25. Actual FIRSTWRITER 2 paired capture telemetry

A fresh paired first-writer capture (`-writer2`) was completed for both runtimes under default-OFF flags `TOOIE_DIAG_DEMO1_TARGET=1` and `P08_DEMO1_TARGET=1`:

- **Execution & Artifact Metrics**:
  - Current:
    - Path: `/home/wesley/.cache/tooie-fps-investigation/demo1-first-writer-current-2/target.csv`
    - SHA256: `fbb2770bc1270762849aaf25f5dacefa4b18d04976ae8c3900618a6da652c75f`
    - Controller PID: 1584474, Game PID: 1584475, elapsed: 66.459s, exit: 0/0.
    - Binary SHA256: `c56ce4e17b554020c36eec14a9e48861693679e9426161031e10aef4527a67e2`.
  - Oracle:
    - Path: `/home/wesley/.cache/tooie-fps-investigation/demo1-first-writer-oracle-2/target.csv`
    - SHA256: `73c1faaace84937324c565889580c620513498971495a663ad356da70f06313c`
    - Controller PID: 1585301, Instance PID: 1585302, elapsed: 101.350s, exit: 0/0.
    - Core SHA256: `4b782df4545ecb84a1aeee69c8159f893cfb503c5b38da78efb630d4baf8e199`.
  - Both captures produced exactly 415 rows (1 header, 355 `tgtrec` pairs, 3 marker rows, 1 finish row, 56 `updrec` rows).
- **Diagnostics in `tgtfinish` (Posts 80..86 Window)**:
  - `w4`: Direct getter calls during window (Current: `0x0f15` / 3861; Oracle: `0x0ed1` / 3793).
  - `w5`: Invalid-binding count = **`0`** on both runtimes (bound marker always resolved to an active record).
  - `w6`: Calls with `a0 == resolved record` (Current: `0xd5` / 213; Oracle: `0xd6` / 214).
  - `w7`: Pre dumps emitted = **`7`** on both runtimes (exact posts 80..86).
  - `w8`: Postmiss rows = **`0`** on both runtimes.
- **Baseline Invariance**:
  - All 355 `tgtrec` posts 2..356 across all 12 words match the pre-writer TARGET2 baseline bit-for-bit on both runtimes. The writer instrumentation introduced zero side-effects.

---

## 26. Proven comparisons: record +0x60, RNG transition, and geometry

Direct word-by-word comparison of all 39 words across the 56 `updrec` rows (`demo1-first-writer-comparison.json`) proves:

1. **Record +0x60 Differs Already at First Available Sample (PRE80)**:
   - At post 80 PRE: Current `0004006c` vs Oracle `0003006c` (difference in upper halfword: `0x0004` vs `0x0003`).
   - This difference is constant across all 56 rows (posts 80..86 pre/post).
   - **Crucial Epistemic Boundary**: Semantic meaning of `+0x60` is strictly **UNKNOWN**. Do NOT label this field as a state, timer, pointer, or physics fault without owning source code verification.
   - All other 38 words (words 0..23, 25..38) are **BIT-EXACT** between Current and Oracle through PRE83 inclusive!
2. **RNG `D_8012C7F0` State Diverges Between POST81 and PRE82 (LCG Recurrence & Step Distances)**:
   - **LCG Recurrence (PROVEN from generated source `funcs_103.c:3470` / `func_800DC0C0`)**:
     - Recurrence formula: $s' = (1664525 \times s + 1013904223) \pmod{2^{32}}$ ($a = \text{0x0019660D}$, $c = \text{0x3C6EF35F}$, $m = 2^{32}$).
   - **Sampled States & Step Distances from shared POST81 (`28f1cd53`)**:
     - Post 80 PRE/POST and Post 81 PRE/POST are bit-exact between runtimes; POST81 state is identical: `28f1cd53`.
     - Post 82 PRE: Oracle reaches `036f8437` at $k=4$ forward LCG steps from POST81. Current reaches `c125942a` at $k=5$ forward LCG steps. (Oracle reaches `c125942a` at POST82, $k=5$).
     - Relative sequence step distance offset (Oracle $\to$ Current forward steps):
       - PRE82: $+1$ step (Oracle $k=4$, Current $k=5$)
       - POST82: $+1$ step (Oracle $k=5$, Current $k=6$ `7c7a3581`)
       - PRE83: $+2$ steps (Oracle $k=8$ `aa6fd75b`, Current $k=10$ `45720845`)
       - POST83: $+3$ steps (Oracle $k=9$ `7bce24fe`, Current $k=12$ `1ca86abf`)
       - PRE84: $+4$ steps (Oracle $k=12$ `1ca86abf`, Current $k=16$ `09f72263`)
       - POST84: $+4$ steps
   - **Epistemic Qualification**: These offsets are mathematical **LCG sequence step distances**, NOT proven actual call counts or causal proof until other potential writes to `D_8012C7F0` are audited and excluded.
3. **Gamespeed (`gs`, w11) is Exact**:
   - All 14 sampled `gs` w11 bits match `0x3d4cccce` identically across both runtimes.
4. **Geometry First Differs at POST83**:
   - Position words (`+0x04`, `+0x08`, `+0x0C`):
     - PRE83: Current `[1821.17737, -135.26294, -146.24696]` == Oracle `[1821.17737, -135.26294, -146.24696]` (BIT-EXACT).
     - POST83: Current `[1803.39844, -137.06342, -142.85275]` vs Oracle `[1803.43860, -137.06247, -142.64917]`.
     - Raw float Euclidean delta = `0.207508441` (exact match with Grok prefix report).
   - Additional raw word diffs at POST83:
     - `+0x48`: Current `438c677a` (280.808) vs Oracle `438cbb8e` (281.465).
     - `+0x54`: Current `4389fee4` (275.991) vs Oracle `438cbb8e` (281.465).
5. **Entity Scope & Bracket Nuance**:
   - Tracked actor is **FIRST BOUND OTHER ACTOR `0x80132f30`** (getter callback `0x80085C98`).
   - Projectile target marker candidate `0x80132f60` is a separate entity and was not tracked in this run.
   - The POST bracket spans from the tracked record's getter call to the next loop getter call or logic exit; it can include downstream processing or subsequent actor updates. Therefore, POST83 divergence is NOT isolated-update proof of actor `0x80132f30`'s internal callback.

---

## 27. Epistemic corrections & widescreen status

- **Widescreen Feedback Status (PROVEN at Feature Level, Mechanism UNKNOWN)**:
  - In a prior diagnostic note, Sonnet inferred that because divergence occurred in pre-draw simulation state, widescreen changes were refuted. That inference was corrected as invalid because prior-frame draw/visibility feedback was untested.
  - With the completion of the single-feature counterfactual below, `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1` is **PROVEN** to be the causal feature for the observed pre-draw simulation divergence (RNG, record `+0x60`, and motion).
  - Crucially: this does **NOT** globally refute widescreen presentation capability. Widescreen viewport rendering remains fully supported; the defect is an unintended coupling where the aspect-aware object frustum culling expansion (`800CACEC`) leaks into simulation/AI logic or consumes/advances simulation state (such as RNG or actor record state) rather than affecting purely visual display-list emission.
  - The exact underlying call consumer and side-effects remain **UNKNOWN**.
- **Suspect `C401C` Audio**:
  - The hypothesis regarding `C401C` audio remains unproven and requires dedicated validation before any canonical promotion.
- **Preserved Status**:
  - Demo 7 cutscene stopping remains visually fixed via the generator static patch.
  - No production fix or gameplay parity claim is made for Demo 1.
  - P09 Phase 1 default CURRENT `tfPos` workaround remains default-ON.
  - Present FPS counter, toggle, and RT64 decimator remain operational.

---

## 28. Single-feature counterfactual telemetry (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0`)

A single-feature counterfactual run was executed on the unchanged C56 binary (`c56ce4e17b554020c36eec14a9e48861693679e9426161031e10aef4527a67e2`) using the TARGET diagnostic logger (`TOOIE_DIAG_DEMO1_TARGET=1`), setting ONLY `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0` (per-process environment flag; getter at `boot_hw_stubs.cpp:4264..4267`, wrapper 4451 calling original `800CACEC` and returning before aspect-aware expansion; all other `TOOIE_TEST_*` flags cleared):

- **Capture Metrics**:
  - Script: `/home/wesley/.cache/tooie-fps-investigation/demo1-projectile-owned-capture.py current -writer-object-original`
  - Controller PID 1599028, owned game PID 1599030, duration 66.217s, exit 0/0, FINISH post 357, 415 rows + 1 header.
  - CSV: `/home/wesley/.cache/tooie-fps-investigation/demo1-first-writer-current-object-original/target.csv` (SHA256 `1f43e7c570f21ec082bc7afac88cf01789cebbd8c2ec4e142f0aee9eb791f928`).
  - Diagnostics: `invalidresolve = 0`, `predumps = 7`, `postmiss = 0`, 56 `updrec` rows (posts 80..86 x pre/post x 4 chunks).
- **Comparison Against Preserved Oracle Baseline (`demo1-first-writer-oracle-2`)**:
  - **355 `tgtrec` Pairs (Posts 2..356)**: All 12 raw words are **BIT-EXACT** (`demo1-object-frustum-counterfactual-comparison.json`). Zero position, velocity, callback, or gamespeed differences across all 355 posts.
  - **546 ActorRecord Words (14 Snapshots x 39 Words)**: Evaluated in `demo1-object-frustum-record39-comparison.json`, which produced `[]` (empty list = **ZERO differences**).
  - **Record `+0x60` High 16 Bits Restored**: Under default aspect-aware ON, Current showed `0004006c` vs Oracle `0003006c` (difference in upper halfword `0004` vs `0003`). Under `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0`, Current matches Oracle at `0003006c` across all 14 snapshots!
  - **Raw RNG `D_8012C7F0` Bit-Exact**: All 14 sampled RNG states match Oracle identically. The PRE82 split (where default ON diverged from Oracle $k=4$ `036f8437` to Current $k=5$ `c125942a`) is completely absent.
  - **Gamespeed `gs` Exact**: All 14 sampled `gs` w11 bits match `0x3d4cccce`.
- **Comparison Against Default Current (`demo1-first-writer-current-2`)**:
  - Confirmed 274 posts (83..356) differ in position between default ON and flag OFF.
  - Because binary, logger, toolchain, and operating environment were identical, the behavioral restoration is strictly isolated to `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0`.

---

## 29. Actual Fire Egg projectile counterfactual telemetry (Hit Restored at Post 355)

To determine whether this single feature restores actual projectile-target interaction, an orchestrator-controlled PROJECTILE-only capture was executed with `TOOIE_DIAG_DEMO1_PROJECTILE=1` and `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0` (no TARGET logger, no other `TOOIE_TEST_*` flag):

- **Capture Metrics**:
  - Script: `/home/wesley/.cache/tooie-fps-investigation/demo1-projectile-owned-capture.py current -object-original`
  - Controller PID 1600930, game PID 1600931, duration 66.824s, exit 0/0, FINISH post 391, 313 rows + 1 header.
  - CSV: `/home/wesley/.cache/tooie-fps-investigation/demo1-projectile-current-object-original/projectile.csv` (SHA256 `3d54fe91ac1bc570a95a87241322f7d58593575aac951c4b4250d397a3344258`).
  - Evaluated via `demo1-fire-egg-object-frustum-counterfactual.json` against preserved Oracle sweep CSV (`89a6bad1...`) and default Current sweep CSV (`343ba040...`).
- **Telemetry Findings**:
  - **Post 355 Sweep Hit Restored**: At post 355, `func_800CD820` sweep leave returns `v0 = 0x80132f60` (nonzero hit), exactly matching Oracle. Default Current with aspect-aware ON returns `v0 = 0` (miss).
  - **Target Candidate Metadata**: Candidate linked by returned handle `0x80132f60` has matching raw words: `w0 = 05440293`, `w1 = 0e800202`, `w2 = 00000671`, `w3 = 00000e21`, `w6 = 8008a328`, `w7 = 80089ff8`. (`0671` is class/resource identity).
  - **Narrowphase `func_800CD6B4`**: LEAVE returns `1` on both off-current and Oracle; default Current has no such callback at post 355.
  - **Input Pairs Parity**: All 22 sweep ENTER input pairs and 21 `sueggmove` ENTER input pairs match Oracle bit-exact in `w0..w5` geometry and `w11` `gs`.
  - **Row Counts Reflect Hit**: 44 sweep and 42 move rows on off-current and Oracle vs 64 sweep and 62 move rows on default Current (because the projectile stops advancing upon hit).
  - **Bound Center Restoration**: All 6 selected candidate bound centers returned by `_subaddiecustomhits_entrypoint_5` (`0x8008A018`) at posts 354 and 355 match Oracle bit-exact. The actual target center at post 355 (`w0 = c5af7c8d`, `w1 = c401d0fa`, `w2 = 44a89e51` = `[-5615.56885, -519.265259, 1348.94739]`) is identical to Oracle, resolving the previous ~427-unit bound center divergence.

---

## 30. Causal attribution & mechanism boundaries

- **Causal Conclusion**:
  - `OBJECT_FRUSTUM_ASPECT_AWARE=1` is **PROVEN CAUSAL** for the observed RNG divergence, NPC trajectory divergence, and Fire Egg collision miss in USER_DEMO1.
- **Epistemic Boundaries**:
  - The exact underlying call consumer and side-effects remain **UNKNOWN**.
  - Do not claim that all NPCs globally are fixed or that a physical gameplay fix is completed.
  - Do not disable aspect-aware object culling globally in production: user directive requires preserving the object visual pop fix in production default (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1`).
  - Production solution requires an **owning-layer architectural separation** that provides aspect-expanded frustum culling for visual DRAW passes while ensuring simulation and logic evaluation paths use the original 4:3 frustum without side effects.

---

## 31. Auditor arbitration & background workloads

- **Completed Background Jobs**:
  - **Opus 5.5 Medium Consumer Design (`demo1-rng-consumer-opus.json`)**: COMPLETED (exit 0, `is_error = false`, 14 turns, 242.8s wall time). Identified two RNG step entries (`func_800DC0C0` and `func_800DC090`) and four non-stepping writers (`800DC310` reseed constant, `800DC324` seed a0, `800DC330`/`800DC354` save/restore in `func_800CFA90`). Static call-site counts identified via generated sources; noted procedural limit (used raw grep in generated code in violation of navigation rules). Proposed wide RNG trace is now conditional and not automatically needed since the single-feature counterfactual cleanly isolated the root.
  - **Grok 4.7 Medium Field 60 Audit (`demo1-field60-and-rng-grok.json`)**: Exited at turn limit (30 turns, exit 1 INCOMPLETE prelim). Noted typo in preliminary text (written Cpost82=Opre82, corrected to Cpre82=Opost82); found no specific field role for `+0x60`. Census stopped as feature-level causality is now established.
  - **Sonnet Final Job (`DIAG_READY`)**: Delivered diagnostic implementation for RNG / visibility caller tracking (`ee14d6cb...` / oracle core `1172a1e1...`). No production fix landed. Identified that `func_800E3E8C` (`E3E8C`) shared visibility predicate has logic/FX consumers while treated as DRAW scope broadly. Sonnet quota is now expired; no further calls.
  - **Grok Prior 24-Turn Owner Pass**: Exited at stop cap INCOMPLETE (no report written); current resume session reuses context.
- **Active Background Jobs (2026-10-19)**:
  - **Grok 4.7 Medium Implementation & Tests (`demo1-frustum-parity-grok-implementation.json` / `report.txt`)**: Resumed session `01a15118-c738-7dd3-98ed-cc5db5021bb5`, max 50 turns, RUNNING. Sole active code writer and test executor. Tasked to close first extra RNG consumer and implement faithful caller allowlist fix, validating actual default PROJECTILE and TARGET tests while preserving expanded DRAW ON.

---

## 32. Paired RNG consumer diagnostic telemetry (2026-10-19)

A paired RNG diagnostic capture was completed across Current and Oracle to evaluate caller paths and consumer behavior around the `func_800CACEC` and `func_800E3E8C` visibility predicates:

- **Current RNG Diagnostic Run (`demo1-rng-current`)**:
  - Binary: SHA256 `ee14d6cb1c144a09ccec7f393dc2e986ac26fb0826893d2e0d490df9d19a683c` (`ee14`).
  - Controller PID: 1610710, Game PID: 1610711, elapsed: 65.553s, reason: `FINISH_MARKER`, exit 0.
  - CSV: `/home/wesley/.cache/tooie-fps-investigation/demo1-rng-current/target.csv` (SHA256 `d3588ec347ff547e1bb116e3712ddad91b83fb85d218a0c89477d39ea3c9caa2`).
  - Metrics: 516 rows, 56 `updrec` rows, 32 `rngcall` events (7 `func_800CACEC` $\times$ 62 `func_800E3E8C`), baseline 355x12 prefix unchanged.
- **Oracle RNG Diagnostic Run (`demo1-rng-oracle`)**:
  - Core: SHA256 `1172a1e164d2e8c54116a94e08f53bf624328ef4b4296c7b962e0f42b5a8d747` (`core1172`).
  - Root controller PID: 1753858, Instance PID: 1753861, elapsed: 122.589s, reason: `FINISH_MARKER`, exit 0.
  - CSV: `/home/wesley/.cache/tooie-fps-investigation/demo1-rng-oracle/target.csv` (SHA256 `6a4c7534f9c7108fb0cdda35ddbc17fd643838dced75973b06bbaf17957a0b2f`).
  - Rows: 503 data rows (504 total lines).
- **Caller Architecture Finding**:
  - Discovered that `func_800E3E8C` (`E3E8C`) functions as a shared visibility predicate serving both visual culling and logic/particle/FX consumers.
  - Broadly treating all `func_800E3E8C` callers as visual DRAW caused simulation/FX paths to evaluate with aspect-expanded bounds, altering downstream RNG and state.
  - A faithful caller allowlist separating genuine DRAW paths from simulation/FX paths is required.

---

## 33. Source proof: func_800DE498 mixed renderer gate and registered callback chain (`PROVEN`)

A rigorous source review of original MIPS disassembly, core2 decompilation sources (`src/core2/1EDAEA0.c`, `asm/nonmatchings/core2/1EDAEA0/func_80101934.s`, `func_80101A50.s`), and recompiled C output (`RecompiledFuncs/funcs_326.c`, `funcs_30.c`, `funcs_214.c`, `funcs_238.c`) definitively isolated how widening `func_800E3E8C` visibility drives guest simulation side effects:

1. **Pre-Draw Callback Registration (`func_80101808`)**:
   Standard actor drawing dispatches through `func_80101870(Actor* arg0, void* arg1)` -> `func_80101808(arg0, func_801015D0)`.
   `func_80101808` invokes `func_800DF47C(func_801015D0, arg0)`, which writes:
   - `MEM_W(0x8012C9A8) = func_801015D0` (registered callback function pointer)
   - `MEM_W(0x8012C9AC) = arg0` (registered callback context argument / `Actor*`)
   It then invokes `func_800DE448`, immediately entering `func_800DE498`.
2. **Frustum Gating Inside `func_800DE498` (`funcs_326.c:4613..4687`)**:
   At guest `0x800DE6EC`, `func_800DE498` calls `func_800E3E8C` (frustum bounding sphere check) and stores `v0` to `0x8012C940`:
   - **Culled (`v0 == 0`)**: Branches to `0x800DE718`, calls cleanup `func_800DE318`, and exits at `0x800DF310` returning 0. The registered callback is completely bypassed; no RNG is stepped and no memory is mutated.
   - **Accepted (`v0 != 0`)**: Branches to `0x800DE728`.
3. **Renderer Processing Context Writes (`s0+0x14/0x18/0x1C`)**:
   On accept, `func_800DE498` stores context values into `s0+0x14`, `s0+0x18`, and `s0+0x1C`.
   - **Critical Distinction**: `s0` is the **renderer processing context**, NOT an actor state structure. These stores are renderer context writes, NOT actor state mutations (`PROVEN` distinction; do not conflate renderer context with actor state).
4. **Callback Execution (`0x800DE738: jalr $v0`)**:
   Loads `$v0` from `0x8012C9A8` (`func_801015D0`) and `$a0` from `0x8012C9AC` (`Actor*`), and executes `jalr $v0`:
   - `func_801015D0` (`funcs_30.c:2772`) at `0x80101784` calls `func_80101934(Actor* arg0)`.
   - `func_80101934` (`funcs_214.c:1217`, `func_80101934.s`) masks `(arg0->unk74 & 0xF)` and indexes jump table `D_801243E8`. Table entry `+0x04` invokes `func_80101A50(arg0)`.
5. **RNG Stepping and Actor Mutation in `func_80101A50`**:
   `func_80101A50` (`funcs_238.c:2571`, `func_80101A50.s`):
   - At `0x80101AA0` (guest RA `0x80101AA8`), calls `func_800DC0C0`, stepping the global LCG random seed `D_8012C7F0`.
   - Tests random float `$f0 < D_80126434` (`0.03f`, 3% check).
   - On match arm: directly mutates guest actor state byte `sb $t2, 0x7A($a2)` (setting bit `0x10` or `0x20` on `Actor->unk7A`).
   - Both arms call `func_800DF744(1, ...)` and `func_800DF744(2, ...)`, writing to global renderer selector array `D_8012C850`.
   - **Epistemic Classification**: Semantic identity as facial/eye-blink animation is **`STRONG_INFERENCE`** (suggested by 3% probability check and dual model selector indices 1 and 2, but without explicit asset/animation binding proof in source). Control flow gating, global LCG RNG stepping, actor byte `0x7A` mutation, and renderer array writes are strictly **`PROVEN`**.

---

## 34. Paired native telemetry arbitration: monotonic gap growth, zero breaks, and first extra RNG isolation (`PROVEN`)

Cross-validation of the paired native telemetry datasets (`demo1-rng-paired-native-chain-gate.json`, `demo1-rng-ee14-origin-resolution.json`, and `target.csv`):

1. **RNG Sequence Step Counts (Posts 79..84)**:
   - Oracle (4:3): 27 total RNG steps (Post 79: 6; Post 80: 4; Post 81: 6; Post 82: 4; Post 83: 4; Post 84: 3).
   - Current (Aspect-Aware Default ON): 32 total RNG steps (Post 79: 6; Post 80: 4; Post 81: 7; Post 82: 5; Post 83: 6; Post 84: 4).
   - Monotonic step gap growth across posts:
     - Post 79: gap = 0 (6 vs 6, caller identities bit-exact)
     - Post 80: gap = 0 (4 vs 4, caller identities bit-exact)
     - Post 81: gap = +1 (7 vs 6; **first extra RNG step**)
     - Post 82: gap = +1 (5 vs 4)
     - Post 83: gap = +2 (6 vs 4)
     - Post 84: gap = +1 (4 vs 3)
     - Cumulative delta = +5 steps across window (32 vs 27).
2. **LCG Chain Invariance (`breaks: []`)**:
   - The chain exhibits **ZERO breaks** across all observed LCG recurrence steps and snapshots (`breaks: []`).
   - Every transition exactly satisfies $s' = (1664525 \times s + 1013904223) \pmod{2^{32}}$.
   - Expected oracle after-states, once calculated, never match actual observed stores once desynchronized.
3. **Exact Isolation of the First Extra RNG Call**:
   - In Post 81, Oracle and Current share an identical prefix through `func_800DC178` (before `b5a07296` -> after `93e288fd`), followed by the first `func_80101A50` call (before `93e288fd` -> after `6239b638`).
   - At Current seq 142, `cacecx` extra accept occurs under owner `func_800DE498` on position `(-2818, 300, -2237)`.
   - At Current seq 146, the **SECOND `func_80101A50` call occurs** (before `6239b638` -> after `036f8437`, origin `func_80101A50`, guest RA `0x80101AA8`).
   - Oracle has no second `func_80101A50` call that frame; it consumes seed `6239b638` later via `func_8001608C` and produces after-state `036f8437`.
   - The first extra RNG step is definitively the **SECOND `func_80101A50` call in post 81 (seq 146 curr)**, NOT global `func_8001608C` (which executes in both runtimes).
   - Later post-81 differences are a deterministic chain shift, not independent new consumers.

---

## 35. Native PC mapper and symbol resolution validation (`PROVEN`)

- **Resolution of Invalid Cache**:
  - The previous symbol lookup cache `demo1-rng-ee14-nm.json` was **INVALID and incomplete** because it extracted only `T/t` symbols and omitted recompiled `W/w` weak functions.
  - The corrected, completed extraction `demo1-rng-ee14-origin-resolution.json` (`nm -C` / `nm T t W w`) validated binary `EE14` (`ee14d6cb1c144a09ccec7f393dc2e986ac26fb0826893d2e0d490df9d19a683c`) with exact positive function sizes:
    - `func_8001608C`: address 8701920, size 3506, type `W`
    - `func_800DC298`: address 4600128, size 174, type `W`
    - `func_8010297C`: address 6063104, size 464, type `W`
    - `func_80101A50`: address 5014576, size 899, type `W`
    - `func_800DC128`: address 10329888, size 261, type `W`
    - `func_800DC178`: address 10872384, size 235, type `W`
    - `func_800E3E8C`: address 2540848, size 423, type `W`
    - `func_800DE498`: address 7226240, size 9835, type `W`
- **MIPS Guest Return Address**:
  - Original guest source return address for the RNG call is `0x80101AA8` (`jal func_800DC0C0` at `0x80101AA0`).

---

## 36. Operational status, agent roles, and next single action

- **Operational Status & Quotas (2026-10-19)**:
  - **User Choice Resolved**: KEEP EXPANSION VISUAL ACTIVE while proper fix; NO temporary OFF default.
  - **NO Sonnet**: Quota expired; all calls forbidden.
  - **Haiku**: Approaching quota limit; reserved/unused.
  - **Grok 4.7 Medium**: SOLE code writer and test executor. Resumed implementation session `01a15118-c738-7dd3-98ed-cc5db5021bb5`: round 1 cap 50 exit 1 with source proof and no edit (`demo1-frustum-parity-grok-implementation-report.txt`); round 2 completed `end_turn` (num_turns 38) and root observed CLI exit 0, landing binary A6A2.
  - **Gemini**: Designated for ALL Markdown and documentation maintenance.
  - **Native Agent `demo7_capture`**: Read-only Grok review only; NO other capture authority.
  - Current code EE14 diagnostics superseded by landed fix binary A6A2; original default vis ON CURRENT palliative / FPS / audio unchanged.
- **Main Implementation Objective**:
  - Correct shared `func_800E3E8C` consumer selection to separate genuine visual DRAW passes from logic/simulation callbacks. (RESOLVED in candidate binary A6A2).

---

## 37. Frustum parity narrow draw allowlist landed and default-ON verification (2026-10-19)

The narrow draw allowlist fix was implemented and verified under candidate binary A6A2 (SHA256 `a6a2c023a9e3b0688cac15249cda61b5c8b8ccdf68c7c40f91d8c512356bef5e`, commit HEAD `ff5b6d99...`):
- **Source Changes (`PROVEN`)**:
  - `src/game/boot_hw_stubs.cpp`: `tooie_wrap_func_800E3E8C` dropped `TooieCacecDrawScope` (no longer opens scope; nested CACEC widens only if outer scope is active). Pure draw caller `func_800B97A4` registered via `tooie_wrap_func_800B97A4` with scope kind 3 when `tooie_cacec_pure_draw_vram(kCacecPureDrawB97A4)` is true (guest VRAM `0x800B97A4`). Preserved `tooie_wrap_func_8011447C` scope kind 2. Mixed caller `func_800DE498` (site `0x800DE6EC`) stays on original `v0`.
  - `src/game/tooie_demo1_projectile.h`: Pure draw constants and helper updates.
  - `tests/tooie_demo1_upd_layout_test.cpp`: Unit tests modeling old wide-scope policy vs new narrow-allowlist policy (`pure_draw_accepts=1`).
  - No generated code hand-edited; no RNG save/restore; no default flag OFF.
- **Default-ON Runtime Verification (`PROVEN`)**:
  - **TARGET Capture** (`demo1-sep-target-current/target.csv`, SHA256 `016a102c92f279aef896bcab0802be27b1671adab06f8bc374bf227573b398ec`, owned PID 1776728, 66.084s, exit 0):
    - All 355 `tgtrec` paired trajectory samples x 12 words match original bit-exact (0 mismatches; old buggy default-ON split at post 83).
    - All 546 `updrec` ActorRecord words (39 words x 14 pre/post snapshots posts 80..86) match original bit-exact.
    - All 14 raw LCG RNG snapshots (`a2`) match original bit-exact (0 mismatches; PRE82 split resolved).
    - All 14 `+0x60` words match original `0003006c` (was `0004006c` under buggy default-ON).
  - **PROJECTILE Capture** (`demo1-sep-projectile-current/projectile.csv`, SHA256 `570f42e0ad8d31a883c2345afc5e0b000a3fde22baa7edc7722ab35b031d1531`, owned PID 1774174, 67.274s, exit 0):
    - Post 355 sweep `func_800CD820` leave returns `v0 = 0x80132f60` (hit) and narrowphase `func_800CD6B4` leave returns `1` (matches original; old buggy default-ON missed with `v0 = 0`).
    - 22 sweep ENTER pairs and 21 `sueggmove9` ENTER pairs match original in 6 geometry words and gamespeed `gs`.
  - **Build & Tests**: Ninja exit 0; tests `tooie_demo1_upd_layout_test`, `tooie_demo1_target_selector_test`, `tooie_demo1_projectile_test`, `tooie_demo1_parity_test`, `tooie_demo7_drift_test`, `mips_rounding_test`, and `demo1_projectile_coverage_test.sh` all exit 0. Test `upd_layout_test` reports `pure_draw_accepts=1` in harness. Unit test models old policy vs new policy; no actual old binary unit test execution exit 1 is claimed.
- **Epistemic Boundaries**:
  - Positive draw accept exists in unit harness; runtime TARGET window recorded `cacecx = 0` (expected when mixed DE498 scope is dropped; runtime cacecx is NOT promoted from 0).
  - Graphical QA of widescreen object-edge pop was previously `NOT_TESTED`; now superseded by direct user visual confirmation (see §38).
  - Do NOT claim global movement parity, all glitches fixed, full visual preserved, or permanent one RNG offset.
- **User Communication & Outcome**:
  - User was asked via async to run normal `./build/TooieRecompiled` without flags in a previously affected gameplay area and confirm object edge pop. User response: *"Sim, continua funcionando"* (see §38).
  - Invariants maintained: P09 Phase 1 default CURRENT `tfPos` workaround remains default-ON (palliative); FPS counter/toggle and RT64 decimator unchanged; audio glitch deferral maintained.

---

## 38. Direct user visual confirmation for tested area (2026-10-19)

User was asked to launch `./build/TooieRecompiled` normally without flags and observe a gameplay area where objects previously popped at the widescreen edges.
- **User Response**: *"Sim, continua funcionando"*.
- **Classification**: `VISUALLY_CONFIRMED_FOR_TESTED_AREA`, `USER_REPORTED`.
- **Epistemic Scope**: Direct user feedback confirms preservation of the observed visual expansion in the gameplay area tested by the user, superseding the previous pending visual QA state. This does not prove all maps, all objects, or global gameplay parity; broader game parity remains an untested residual.
- **Runtime Draw Metrics**: Runtime TARGET window had `cacecx = 0` (not promoted to positive; only user's visual confirmation closes visual QA for the tested area).
- **Preserved State**: Bounded Demo 1 fix complete on candidate binary A6A2 under production default ON (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1`); 355x12 trajectory words, 546 ActorRecord words, 14 RNG snapshots, post 355 Fire Egg sweep hit `v0 = 0x80132f60` (`CD6B4 = 1`), 22 sweep + 21 move pairs exact. Pure-draw allowlist (`func_800B97A4` kind 3, `func_8011447C` kind 2) preserved; mixed renderer `func_800DE498` original `v0`; shader CURRENT `tfPos` palliative default-ON; FPS counter/toggle, RT64 decimator, and audio deferral unchanged. No new action needed on the bounded Demo 1 fix.

`NEXT_SINGLE_ACTION`: extend oracle comparison to another attract demo when continuing broader fidelity.
