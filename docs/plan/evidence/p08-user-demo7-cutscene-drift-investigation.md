---
title: "P08 — USER_DEMO7 cutscene drift bounded investigation"
description: "Bounded investigation and PROVEN generator fix for Demo 7 attract player drift; cause proven as N64Recomp zero-size ELF marker bug truncating bsrest stop helper (static_344_80800000); versioned generator patch applied, full regeneration via official tooling executed; independent Grok final gate PASS on 119 pose pairs; stationary glidx 38..120 verified."
status: evidence
type: evidence
project: TooieRecomp
phase: P08
date: 2026-10-17
tags: [p08, attract, demo7, cutscene, drift, stomping-plains, map-282, glrecord, generator-fix, n64recomp]
---

# USER_DEMO7 cutscene drift bounded investigation

## 1. Executive summary

This document records the bounded forensic investigation, root-cause isolation, and generator-layer resolution of the user-observed player drift during Demo 7 in the attract sequence (TDL Stomping Plains, map 282, stage 0 sub 8).

The defect was **PROVEN** to be a static function sizing defect in `N64Recomp` (`src/main.cpp`): zero-sized NOTYPE ELF markers (`bsrest_VRAM` and `bsrest_TEXT_START` at `0x80800000` in section 344) caused `N64Recomp` to truncate `static_344_80800000` (and `static_304_80800324` in `.bsledge`) to an empty body. In original N64 hardware, `static_344_80800000` is called during State 116 (`bsrest`) initialization to clear target horizontal velocity (`tspd = 0`) and reset velocity vectors to `(0, 0, 0)`. The empty body caused Banjo to retain pre-cinematic walking momentum (`tspd = 500.0`, `velxz ~ 514..520`) throughout the overhead cinematic camera slide (`glidx 38..120`).

The fix was applied at the generator source layer (`patches/runtime/n64recomp-zero-size-static.patch`), registered in `tools/apply_runtime_patches.sh`, validated via standalone regression test `tests/n64recomp_zero_size_static_test.sh` (new generator PASS, old generator FAIL), and all 420 recompiled files were regenerated via official tooling (`tools/run_recomp.sh` / N64Recomp) using restored decompilation inputs (pinned fork `25033be6`, ROM SHA1 `af479771ff5e91ce3732f3d84146614ab71d8d79`, ELF SHA1 `2b2d04c5b6484594f750b097daf670f7a8a24167`, size 91855676). Zero hand-editing of generated code was performed.

Independent final gate evaluation by Grok on runtime capture `demo7-fixed-root.log` (121 rows, controller 396.13s) confirmed:
- **Criterion 1 (Demo 7 Bounded Stationary Parity)**: **PASS**. All 119 pose pairs ($i \to i+1$) match reference within quantized rounding tolerances; pad inputs match 120/120; glidx 38..120 is fully stationary at pos `(-80.46, 0.00, 4496.25)`, `velxz = 0.0`, `tspd = 0.0`.
- **Criterion 2 (Global Real-Time Speed & Cadence)**: **UNKNOWN / IN_PROGRESS**. Global speed across player, mobs, and camera is not proven resolved by a single bounded scene. A serial 90-second cadence diagnostic (`timing-fixed-serial-90s.log`) is currently executing with read-only audit of logger lines. Do not infer a fixed global clock from partial data; do not promote full campaign parity.

```text
SCENE_IDENTITY: PROVEN (stage 0, sub 8, map 282 entrance 8 mode 5, glrecord ovl 698 ep0)
USER_PRECISE_OBSERVATION: RESOLVED (drift eliminated; Banjo stationary during overhead cinema)
ROOT_CAUSE: PROVEN (N64Recomp zero-size ELF marker bug truncated bsrest helper static_344_80800000)
FIX_LAYER: GENERATOR_SOURCE (patches/runtime/n64recomp-zero-size-static.patch, official tooling regen)
REGRESSION_TEST: PASS (tests/n64recomp_zero_size_static_test.sh)
CRITERION_1_BOUNDED_PARITY: PASS (119/119 pose pairs within tol; gl38..120 stationary pos/vel/tspd=0)
CRITERION_2_GLOBAL_CADENCE: UNKNOWN / IN_PROGRESS (timing-fixed-serial-90s.log under audit)
PRESERVED_STATE: CURRENT tfPos workaround, FPS counter/toggle, RT64 decimator, audio deferral unchanged
```


## 2. Spoken sequence and scene mapping (PROVEN)

The user counts attract gameplay demos continuously across introductory sequences:

```text
intro -> Demo 1 -> Demo 2 -> Demo 3 -> intro -> Demo 4 -> Demo 5 -> Demo 6 -> intro -> Demo 7 -> Demo 8 -> Demo 9
```

The underlying attract registry is documented in [[p08-provisional-follow-camera-and-egg-differential]] (Section 3 static table and Section 4 live slice):

- **Overlay**: `.gsattract` (index 2882, overlay 731), VRAM `0x80800000`.
- **Table**: `D_808003D0` contains 6-byte records (`u16be map`, `u8 entrance`, `u8 flags`, `u16 extra`).
- **Stage pointer**: `D_80800490[0]` points to `0x808003D0` (stage 0). Stage 1 (`0x80800430`) was not reached live.
- **Runtime index**: `D_80135470+1` (`sub`).
- **Counter**: `src/game/syscall_handler.cpp:393-428` (`s_user_demo`) increments only when `mode == 5`, `map != 0`, and `map != 395`.

### Scene identity comparison

- **USER_DEMO6**: `stage = 0`, `sub = 6`, map 185 (Mayahem Temple), entrance 7.
- **Preceding intro**: `stage = 0`, `sub = 7`, map 395 (`0x018B`, CS Startup Intro then T_INTRO), mode 5 then 3. This is the intro *between* Demo 6 and Demo 7. It is **not** USER_DEMO7.
- **USER_DEMO7 (PROVEN)**:
  - User ordinal: 7
  - Internal attract index: `stage = 0`, `sub = 8` (table index `i = 8` of `D_808003D0`)
  - Map: 282 (`0x011A`, TDL Stomping Plains)
  - Entrance: 8
  - Flags: `0x01`
  - Mode: 5
  - Input asset: `glrecord` overlay 698, 1244 samples / count 1245
  - Preceding intro occurrence: 3
- **USER_DEMO8**: `stage = 0`, `sub = 9`, map 264.

Low byte of map 282 is `0x1A`. In the 16-record stage 0 table (maps 335, 278, 314, 395, 235, 296, 185, 395, 282, 264, 230, 395, 244, 276, 423, 395), no other map shares low byte `0x1A`. The `demo_key` does not collide within the live stage 0 table.

## 3. Disambiguation from superseded catalog and 520-frame measurement

The historical parity table in [[p09-user-observed-parity-residuals]] (Section 7, line 190) lists a zero-based "Demo 2" on `MAP_11A` (Stomping Plains, map 282) with:

```text
Demo 2 | MAP_11A | ANGRYLION_CONTROL | PASS | 520/520 (100%) | Max err < 0.79u (STABLE_NOISE) | Bit-identical | Upright & aligned
```

Critical distinctions:

1. **Catalog numbering conflict**: That table indexed only `flags = 0x01` maps (335, 235, 282, 244), skipping others. "Demo 2" was a zero-based filtered catalog row, not user ordinal 2.
2. **Partial window**: The measurement evaluated only 520 frames out of the 1244-sample `glrecord` stream.
3. **`STABLE_NOISE` is not stationary proof**: The metric `Max err < 0.79u` measures tracking position error versus the Angrylion oracle, **not** that player world position was constant or stationary.
4. **Outdated baseline**: The row is dated 2026-08-21, prior to the 2026-10-16/17 default `tfPos` CURRENT workaround and FPS pacing implementation.
5. **Header status**: The header of [[p09-user-observed-parity-residuals]] is explicitly `CORRECTED/SUPERSEDED`. It cannot be used as proof that Demo 7 is stationary or drift-free.

## 4. Source analysis of diagnostic logging and sampling nuances

### 4.1. `TOOIE_DIAG_DEMO_PATH` (`tooie_demo_path_tick`)

Inspection of `tooie_demo_path_tick` in `src/game/boot_hw_stubs.cpp:1638-1648`:

```cpp
    // First-straight window only: emit every guest tick once the recorded-pad
    // walk begins (gl_adv > 0), up to 240 ticks. Do not run into later analog
    // turns in the recorded demo.
    if (gl_adv > 0 || dposxz > 1.0f) {
        s_moving = true;
    }
    if (!s_moving) {
        return; // pre-walk idle: skip
    }
    if (s_emitted >= 240) {
        return;
    }
```

Corrected logging nuance:
- The variables `s_emitted` and `s_moving` are process-lifetime statics.
- In a full attract sequence (`intro -> Demo 1..3 -> intro -> Demo 4..6 -> intro -> Demo 7`), the 240-tick budget is already entirely exhausted by earlier demos long before stage 0, sub 8 (map 282) executes.
- The `return; // pre-walk idle: skip` check was only a tick-0 skip prior to the initial pad advance (`gl_adv > 0`), not an "animation idle filter" that drops mid-demo cutscenes.
- Furthermore, `tooie_demo_path_tick` does not log `cammode` or animation ID, and its initial `dpos` calculation can be against an uninitialized `s_prev_px` if early-returned.

### 4.2. `TOOIE_DIAG_DEMO_SYNC` (`tooie_demo_sync_tick`)

In contrast, `TOOIE_DIAG_DEMO_SYNC` (and `TOOIE_DIAG_ATTRACT` which activates it) logs on every mode-5 tick without a 240-tick cap:
- Emits consumed `glidx`, pad sample, remap, `bastick`, `bs`, world position (`player + 0xE4`), physics position (`phys + 0x04`), velocity (`phys + 0x10`), `tspd` (`phys + 0x3C`), yaw, mult, accel, camera position/yaw, `camctrl` (`cam record + 0x1C`), `cammode` (`cam record + 0x16`), animation ID/timer/duration/type/flags/phase, stage, sub, `gamespeed` (`D_8012C760`), `gfcnt` (`D_8012C764`), and map.
- Diagnostic weight & caveat: `tooie_demo_sync_tick` is not cheap. It fires on every mode-5 tick across the entire attract run and performs an actor scan of up to 64 actors per frame, plus the UD4-specific line.
- Unattended heavy full 9-demo diagnostics should **not** be recommended as clean proof due to execution weight, buffer/log explosion, and diagnostic overhead. A bounded offline-filtered capture focused strictly on map 282 is required.

## 5. Status of Demo 5 and bounded helpers

`USER_DEMO_5` (map 296, `stage = 0`, `sub = 5`) has a bounded helper `tooie_ud5_bainput_window` in `src/game/boot_hw_stubs.cpp:2548-2564`. This helper is strictly gated to `mode == 5 && sub == 5 && glidx >= 0 && glidx <= 35` under diagnostic flags.

Rules and status preservation:

- The bounded helper cannot activate during `sub == 8` (Demo 7).
- `USER_DEMO_5` must **not** be asserted as globally resolved by the bounded helper.
- [[p08-attract-movement-residuals]] explicitly records `USER_DEMO_5_MANUAL_STATUS: RESIDUAL_REPRODUCED_BY_USER` (gets lost early in snowy terrain). Open Demo 5 residuals remain preserved.
- Bounded helper logic from Demo 4 (`tooie_ud4_writer_window`, Haunted Cavern `sub == 4`) and Demo 5 does not generalize to Demo 7.

## 6. Hypotheses, binding status, and classification

### 6.1. Host decoder binding status

Inspection of registered boot stubs in `src/game/boot_hw_stubs.cpp` (`add_loaded_function` block ~4368-4526):
- `80087920` (`tooie_wrap_plcamera13`) calls `_plcamera_entrypoint_13` as a passthrough; it contains no mode decode.
- `8008C894` calls `baanim_update` after diagnostic logging; it contains no root-motion decode.
- `80098140` (`bainput_enable`), `80097B70`/`80097B94` (camera rotate), and `800979D8` are host input bindings scoped to UD4/UD5 windows (`sub == 4` / `sub == 5`) and do not select `sub == 8`.
- `func_800D8FF8` is only a comment next to the gamespeed load in `tooie_demo_path_tick`, not in `add_loaded_function`.
- `glrecord` 698 ep0 is an interpreted overlay; `syscall_handler.cpp` samples after that overlay returns and does not decode control mode.

Crucial distinction:
- While a source-specific host decoder turning an "overhead cinematic" into a locomotion lock is absent from the audited stubs, **do not claim source proves no missing decoder globally**.
- The absence of a host decoder is **not** a reason to freeze physics or zero velocity. Camera mode (`cammode` at record + 0x16) is already readable in guest memory, and player control state (`player + 0x120 bs_id`), physics (`player + 0xC8`), and world position (`player + 0xE4`) are guest structures manipulated by interpreted guest code. The writer function during the overhead shot remains `UNKNOWN`.

### 6.2. Collision observation vs measurement

- User precise observation: Character starts walking, stops ~1 second later. During intended stop under an overhead cinematic camera, player displays idle/standing animation with stationary legs, but slides in one direction and still collides with the environment, hands-off controls, then resumes demonstration gameplay.
- Classification: The observed collision response provides a **`STRONG_INFERENCE`** that guest world/actor motion and guest collision response are actively running (rather than a pure presentation-layer render interpolation artifact).
- Epistemic boundary: This is **not a measurement**. No contact, surface normal, or collider field is logged by `demo-path` or `demo-sync`. It must not be promoted to a proven guest collision trace until instrumented.

### 6.3. Candidate discriminator (`stage = 0`, `sub = 8`, `map = 282`, `glidx 0..90`)

To determine the motion owner without modifying guest behavior, the existing fields in `demo-sync` can be evaluated per tick across the transition from walk to idle/overhead shot (`glidx 0..90`):
1. **World delta** (`player + 0xE4` displacement between successive ticks) vs **`velocity * gamespeed`** (`phys + 0x10 * D_8012C760`).
2. **`tspd`** (`phys + 0x3C`) and **`accel`** (`phys + 0x44`): whether physics registers locomotion speed.
3. **`bs_id`** (`player + 0x120`) and **`anim id` / `phase`**: whether pose and control state transitioned to idle at ~1s.
4. **`cammode`** (`record + 0x16`) and camera Y vs player Y: marking the overhead cinematic shot.
5. **Recorded stick & pad samples**: confirming whether stick input is neutral during the slide.

Discriminator logic:
- If world delta matches `velocity * gamespeed`, the physics integrator explains the displacement (retained velocity/inertia).
- If `velocity` and `tspd` are ~0 while world delta remains non-zero, look for a **different writer** (guest script, external actor translation, etc.), rather than blindly assuming root motion.

### 6.4. Evidence classification

- **`PROVEN` (static/code)**:
  - Spoken Demo 7 maps to `stage = 0`, `sub = 8`, map 282, entrance 8, mode 5, `glrecord` overlay 698 ep0.
  - Sub 7 is map 395 CS Startup Intro (preceding intro 3), not Demo 7.
  - `s_user_demo` increments only on non-intro mode 5 maps; no low-8 collision exists in stage 0.
  - `tooie_demo_path_tick` 240-tick budget is process-lifetime and exhausted by earlier attract demos.
  - `tooie_demo_sync_tick` logs every mode-5 tick without a 240-tick cap, sampling up to 64 actors per frame.
  - Audited boot lookup stubs contain no overhead camera locomotion lock decoder.
  - `tooie_ud5_bainput_window` gates strictly on `sub == 5`.
  - Default `tfPos` CURRENT workaround (`apply_default_before_renderer_init`) repeats current vertex pose when `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` is unset.
- **`USER_OBSERVED`**:
  - Demo 7 player starts walking, stops ~1s later, displays standing/idle animation with stationary legs during overhead cinematic camera, slides and collides hands-off, then resumes demonstration gameplay.
- **`STRONG_INFERENCE`**:
  - Observed collisions indicate guest world/actor motion and guest collision response are running during the slide, rather than pure presentation-layer render interpolation.
- **`UNKNOWN`**:
  - Motion writer (which interpreted guest function or script modifies physics/position during the overhead cinematic).
  - Whether original N64 hardware / Angrylion oracle displays identical slide/displacement during the same still window.
  - True collision measurement (no collider/contact trace exists yet).
  - Whether a missing host decoder exists outside the audited stubs (do not claim source proves no missing decoder globally).

### 6.5. Hypothesis ranking

- **H1 (STRONG_INFERENCE)**: The reported drift occurs during `stage = 0`, `sub = 8`, map 282 entrance 8, during playback of player slot `D_80135490[0]`. Any investigation using `sub = 7`, zero-based Demo 2, map 235, or map 296 targets the wrong clip.
- **H2 (HYPOTHESIS, candidate discriminator)**: Guest physics integrates residual velocity during an animation that appears stationary to the user (evaluable via world delta vs `vel * gamespeed`).
- **H3 (HYPOTHESIS, alternate guest writer)**: An interpreted guest script or camera transition logic writes to player position or physics while velocity/tspd are zero (evaluable if world delta is non-zero with `vel == 0 && tspd == 0`).
- **H4 (HYPOTHESIS)**: Stale host pad state or an input thunk leaks into `OSContPad` (`D_80079B40`) or remap floats (`D_80079AF8+8/+C`) while the consumed `glrecord` sample is neutral, and the actor control state accepts locomotion.
- **H5 (HYPOTHESIS)**: Player control state (`bs_id` at `player + 0x120`) lacks a cutscene locomotion lock, or animation root motion applies translation while the pose appears idle.
- **H6 (HYPOTHESIS)**: Extra guest logic ticks or gamespeed scaling (`D_8012C760`) integrate velocity during the still interval. (Audio preemption is refuted as a cause).
- **H7 (HYPOTHESIS, weaker)**: Render pose moves while guest world position remains stationary. Because the default `tfPos` workaround repeats the current vertex pose above native presentation rates, visual lerp sliding is less likely than guest world displacement or root motion, and observed environmental collision strongly favors guest motion.
- **H8 (WEAK / REFUTED reuse)**: Demo 4 camera chain or Demo 5 `800979D8` helper leaking into Demo 7. Sub gates (`sub == 4`, `sub == 5`) prevent execution in `sub == 8`.

## 7. Bounded minimal next capture (NOT RUN)

To arbitrate motion ownership without modifying production code, a single bounded diagnostic capture is specified:

- **Flag**: `TOOIE_DIAG_DEMO_SYNC=1 ./build/TooieRecompiled` (do **not** enable `TOOIE_DIAG_ATTRACT`, `TOOIE_DIAG_DEMO_PATH`, vis-cull flags, or any `tfPos`/FPS/audio modifications).
- **Target window**: Stage 0, sub 8, map 282, consumed `glidx 0..90`.
- **Filtering**: Post-process / offline-filter the log to retain only `[demo-sync]` lines matching `stage = 0`, `sub = 8`, `map = 282`, `glidx 0..90` to avoid heavy multi-megabyte log bloat from earlier demos. Do not recommend unattended heavy full 9-demo diagnostics as clean proof due to execution weight.
- **Analysis per tick**:
  1. Compare `cammode` transition and camera Y vs player Y.
  2. Inspect `bs_id` and `anim id` / phase for the idle transition at ~1s.
  3. Compare world position delta vs `velocity * gamespeed`.
  4. Inspect `tspd`, `accel`, `bastick`, and recorded `OSContPad` stick.
  5. If `velocity` and `tspd` are ~0 while world delta is non-zero, identify the alternate guest writer (script, etc.) rather than assuming root motion.
- **Oracle comparison**: Run the identical `glidx 0..90` window on the Angrylion oracle at the same guest addresses (`player + 0xE4`, `phys + 0x10`, `phys + 0x3C`, `player + 0x120`) to confirm whether hardware exhibits the same behavior.
- **Safety**: No physics freezing, no velocity zeroing, all code and defaults untouched.

`NEXT_SINGLE_ACTION`: execute one bounded capture with `TOOIE_DIAG_DEMO_SYNC=1 ./build/TooieRecompiled` (preserving CURRENT and avoiding heavy collectors), offline-filter `[demo-sync]` lines for `stage=0 sub=8 map=282 glidx 0..90`, and evaluate world delta vs `vel * gamespeed`, `tspd`, `bs`, `cammode`, stick, and `anim`; compare against Angrylion oracle over the same glidx band if needed; no physics freezing and all defaults untouched.

> [!NOTE]
> **Historical status update (2026-10-17)**: The diagnostic capture specified in this section was executed by the user and analyzed via streaming parser. The historical `NEXT_SINGLE_ACTION` above is superseded by the empirical findings and concrete next action in Section 8 below.

## 8. User capture analysis and motion owner narrowing (2026-10-17)

> [!IMPORTANT]
> **State layout correction note (2026-10-17)**: Claims in this section referring to Banjo executing state `bs = 4` during the overhead cinematic slide are **`CORRECTED/SUPERSEDED`** by Section 9. Disassembly of original Tooie MIPS `bs_setState` (`0x8009E5D8`) proved that `*(player + 0x120)` maps `+0: prev`, `+4: cur`, `+8: next`. The legacy logger sampled offset `+0` (`bs_prev`), which was 4 during the slide while current state was actually **`bs_cur = 116` (`0x74`)**. Do not call state 4 cinematic.

### 8.1. Capture provenance and execution boundaries

The empirical diagnostic capture specified in Section 7 was executed by the user and analyzed via an offline streaming parser without modifying code or configuration:

- **Raw capture log**: `/home/wesley/.cache/tooie-fps-investigation/demo7-user-capture.log` (SHA256: `74f859128f586306626df1eb964e161983c6c916c9509c14b04ff8b577d99467`, 407,919 lines, ~44 MB). Persists in external cache; not copied into the repository.
- **Analysis scripts & dumps**:
  - Analysis dump: `/home/wesley/.cache/tooie-fps-investigation/demo7-user-capture-analysis.txt` (259 lines).
  - Verdict summary: `/home/wesley/.cache/tooie-fps-investigation/demo7-user-capture-verdict.txt` (84 lines).
  - Streaming parser: `/home/wesley/.cache/tooie-fps-investigation/demo7_capture_parse.py`.
- **Session provenance**: Grok CLI exit code 0, session ID `01a14b7c-c6ef-7bd3-9a0b-3fef4d99acf4` (from `/home/wesley/.cache/tooie-fps-investigation/capture-analysis-grok.json` text field only, excluding thought).
- **Runtime execution classification**: `PARSED_NOT_A_GAME_PASS`. The analysis is strictly a post-run parse of user execution artifacts; no build, unit tests, or game binary were launched by the analysis pass.
- **Diagnostic overhead limitation**: `TOOIE_DIAG_DEMO_SYNC=1` also automatically forced movement diagnostics (`tooie_diag ensure_init`), causing the log to expand to 407,919 lines (including 227,223 `tooie-move` lines and 82,720 `tooie-actor` lines). This execution weight confirms that unattended heavy full-attract captures must be avoided in future investigations; future diagnostic passes must remain bounded.

### 8.2. Independent gate and data integrity (PROVEN)

Census across the parsed capture:
- **`[demo-sync]` coverage**: 6,607 lines parsed, 0 unparsed. All lines belong to `stage = 0` and `mode = 5`.
- **Target clip (`stage = 0`, `sub = 8`)**: Exactly 1,244 consecutive lines covering consumed `glidx 1..1244` (`cnt = 1245` on every row).
- **Sample continuity**: 1,243 steps with `dglidx == 1`; 0 duplicate indices, 0 dropped frames, 0 gaps.
- **Tick step**: `tick_delta == 2` on all 1,243 steps (`tick 13393..15879`, span 2486 ticks = 2 * 1243).
- **Delta-time definition**: `gamespeed` (`gs`) is observed as `0.03333` on 1,198 steps (~30 Hz) and `0.05` on 46 steps (~20 Hz). `gs` is already the frame delta-time in seconds; velocity must **not** be multiplied by `tick_delta`.

### 8.3. Logger map-clear behavior and scene identification (PROVEN)

Investigation of the logger writer `tooie_demo_sync_tick` (`src/game/boot_hw_stubs.cpp:1894-1925`):
- The `map` field is read via `MEM_H(0x10, 0x80127630)` (signed 16-bit halfword at `0x80127640`).
- This address is live and reflects map IDs on transition tails: it logs 278, 314, 395, 296, 185, and 264.
- However, during the steady body of mode-5 playback clips, this field is observed as 0 (the exact guest writer that clears `0x80127640` to 0 is untraced; categorical claims of after-warp consumption mechanisms are removed absent primary proof). As a consequence, maps 282, 235, and 335 never appear during body playback.
- **Critical finding**: Any filter requiring `map == 282` discards 100% of the Demo 7 body.
- **Scene identity verification**: The identity of Demo 7 is conclusively proven by `stage = 0`, `sub = 8`, `cnt = 1245`, occurring immediately after intro occurrence 3 (`sub = 7`, map 395, last line at `glidx = 934`) and immediately before Demo 8 (`sub = 9`, map 264, first line at `glidx = 1245` with identical player position `(1652.97, -362.84, -1228.23)` and 0.00 XZ jump).
- **Epistemic boundary**: The exact guest writer that clears `0x80127640` to 0 has not been disassembled or traced; the clearing mechanism must not be claimed as proven beyond the observed runtime zeros.

### 8.4. Viewport camera cut vs contaminated `cammode` (PROVEN)

Evaluation of camera tracking fields:
- **`cammode` contaminated**: The raw halfword at `cam record + 0x16` spans -29573 to +28360, with only 3 of 1,244 samples falling in the expected range `[-1, 64]` (1,166 distinct values). It overlaps the lower half of a 32-bit float at `+0x14`. It is invalid as an enum without proof and must not be used.
- **Coherent viewport cut (`glidx = 38`)**: The viewport camera pointer at `0x8012D500` provides coherent world coordinates at `+0x00`:
  - `glidx 1..37`: Camera follows player with vertical offset `dY` locked at ~345.1 and horizontal distance `h` ~855..887; `camctrl` is ~345.
  - `glidx 38`: Camera viewport jumps overhead to `(168.7, 754.8, 5467.4)` (`dY = 754.8`, `h = 1011.7`), and `camctrl` drops from 340.7 to 3.53.
  - This abrupt jump at `glidx = 38` cleanly marks the overhead cinematic shot observed by the user.

### 8.5. Transition timeline and kinematics analysis (PROVEN / STRONG_INFERENCE)

Tracing the state transitions across `stage = 0, sub = 8`:

1. **Pre-walk idle stand (`glidx 1..33`)**:
   - Player is completely stationary at `(-116.00, 0.00, 4523.00)`.
   - `bs = 0`, `anim = 111` (timer advances `0.000 -> 0.291 / 5.500`), `tspd = 0`, `vel = (0, -1, 0)`, `velxz = 0.00`.
   - Recorded pad and `bastick` are neutral. Follow camera is locked.
   - This represents ~1.1 seconds of guest idle stand **prior** to walking.

2. **Stick-driven walk (`glidx 34..49`)**:
   - `glidx 34`: First movement tick. `bs = 1`, `anim = 3`, pad `(38, 45)`, `bastick = (0.30, 0.40)`. Step into gl 35 is 8.81 while start-of-interval `velxz = 0` and `tspd = 0` (single transition sample).
   - `glidx 35..37`: `bs = 2`, `anim = 12`, `tspd` rises to 500, `velxz` accelerates from 176.29 to 396.52.
   - `glidx 38`: State switch to `bs = 4`, `anim = 111`, `tspd = 500.0`, `velxz = 441.46`. Viewport camera cuts overhead (`dY` jumps to 754.8).
   - `glidx 39..49`: Recorded stick remains deflected (`bastick` Y reaches 1.00), `bs = 4`, `anim = 111`, `tspd = 500`, `velxz` reaches 513.29.
   - The stick-driven walk lasts ~0.5 seconds of guest time (`glidx 34..49`), not 1 full second.

3. **Stick release and steady slide (`glidx 50..90+`)**:
   - `glidx 50`: Recorded stick returns to zero (`remap = (0.00, 0.00)`, `bastick = (0.00, 0.00, 0.00)`, pad `(1, 0, 0000)` where x=1 is a 1-count `glrecord` residue byte; bstick angle 90 persists harmlessly).
   - Player position at gl 50 is `(116.70, -22.24, 4336.06)`.
   - Crucial state: Despite stick release, `tspd` remains `500.0`, `velxz` remains `514.04`, and `anim` remains `111` with `bs = 4`.
   - `glidx 50..90`: Stick and remap remain completely neutral (`0.00`). `tspd` remains pinned at `500.0`. `velxz` slowly plateaus at `519.82`.
   - Player world position steadily translates across the XZ plane, reaching `(650.40, -149.10, 3898.99)` at `glidx = 90`.
   - Elevation Y drops from -22.24 toward -149.10, reaching terrain floor -200.00 at `glidx = 106` (ground-follow physics).

4. **Stream gate and physical integration ratio (PROVEN)**:
   - Root-independent stream evaluation computed across the 41 steps of stick-neutral slide (`glidx 50..90`):
     - Median step XZ: 17.23 units.
     - Median `velxz`: 516.94 units.
     - Median predicted displacement `velxz * gs`: 17.23 units.
     - Median ratio `deltaXZ / (velxz * gs)`: **`1.000354467227695`** (min `0.66643885`, max `1.00159322`).
   - Body-wide ratio evaluation (367 steps with predicted displacement > 5 units and step > 1 unit):
     - Median ratio: **`1.0003838778232614`** (p10 = 0.603, p90 = 1.532).
     - Median residual `|deltaXZ - velxz * gs|`: **`0.000`** (p90 = 11.133, max = 24.937).
   - This tight unity correlation proves that physical integration of the non-zero guest velocity explains the steady slide.
   - **Epistemic boundary**: Do not universally refute another guest writer or declare the exact integrator function from correlation alone.

5. **Later gameplay resumption inside sub 8**:
   - `glidx 38..803`: Continuous run of `bs = 4`, `anim = 111` traversing 2491.38 units of XZ path (`tspd = 500`).
   - `glidx 804`: Transition to `bs = 116`.
   - `glidx 805..809`: `bs = 1`, `anim = 90` with neutral stick and retained/scripted `tspd = 677.4`.
   - `glidx 810..816`: Air/falling states `bs = 152` / `bs = 47`.
   - `glidx 831+`: Recorded pad and `bastick` become active again (`bs = 20`, `22`, `21`).
   - `glidx 1123..1244`: Standing pose `bs = 23`, `anim = 111`, `tspd = 0`, `velxz = 0`, with controller buttons held.
   - `glidx 1245`: Clean, continuous handoff to Demo 8 (`sub = 9`, map 264) with 0.00 XZ jump.

### 8.6. Status of hypotheses and remaining blocker

Evaluation of hypotheses following capture analysis:

- **H1 (SCENE_IDENTITY)**: **`PROVEN`**. The clip is unambiguously `stage = 0`, `sub = 8` (`glrecord` 698, count 1245).
- **H2 (PHYSICAL_INTEGRATION_SLIDE)**: **`PROVEN` (mechanism)**. Displacement during the slide matches `velxz * gs` (median ratio 1.00035). The actor slides because guest physics integrates non-zero velocity and target speed (`tspd = 500`) while displaying an idle-appearing animation (`anim = 111`).
- **H3 (ALTERNATE_GUEST_WRITER_SLIDE)**: **`REFUTED` (for steady slide)**. There is no second position writer displacing the actor with zero velocity during the slide. Single-step ratio outliers (e.g. at `glidx = 34`) reflect interval-boundary acceleration sampling, not sustained external displacement.
- **H4 (HOST_PAD_LEAK)**: **`REFUTED`**. Recorded pad, `bastick`, and `remap` return to 0 at `glidx = 50`. The slide is driven by guest physics retaining target speed, not a stuck host controller or thunk leak.
- **H7 (RENDER_LERP_DRIFT)**: **`REFUTED`**. Displacement is genuine guest world position advancement tracked by the physics position (`|pos - ppos|_xz` median 0.000).

Remaining bounded blocker:
- **Motion owner status**: Motion owner is narrowed from UNKNOWN mechanism to guest-world displacement with nonzero retained target speed/velocity during an idle-looking pose (`anim 111`, `bs 4`).
- **Root cause and function writer**: STILL UNKNOWN. Which interpreted guest routine writes `phys + 0x3C` (`tspd`) and `phys + 0x10` (`velocity`) and keeps `tspd = 500` after the recorded stick is released? No code correction made.
- **Scope limitation**: No claim that all cutscenes share this root.
- **Unverified symbols**: The exact semantic role of `bs = 4` and the formal symbol name of `animation 111` (which matches the ID of the pre-walk stand at `bs = 0`) have not been reproven from symbol tables in this pass.
- **Collision status**: Collision response remains `USER_OBSERVED`; no surface normal or contact record was logged.
- **Oracle comparison**: An Angrylion oracle baseline for `stage = 0, sub = 8` has not yet been executed.
- **Preserved constraints**: No code changes made. No physics freezing or unconditional zeroing of velocity. CURRENT workaround, FPS settings, and audio remain strictly untouched.

### 8.7. Concrete next single action (SUPERSEDED BY SECTION 9)

> [!NOTE]
> The historical next action below has been superseded by the empirical findings, root bounded capture, State 116 source proof, and Phase 2 oracle execution documented in Section 9.

`NEXT_SINGLE_ACTION`: locate the interpreted guest state `bs = 4` init/update/end and physics mode/target-speed writer at the `bs = 2 -> bs = 4` transition (`glidx = 38`), then trace if needed a bounded window of `stage = 0, sub = 8` (`glidx 35..60`) without filtering on `map == 282`; compare against Angrylion oracle where necessary before attempting any faithful fix. Avoid full-run captures due to `DEMO_SYNC` execution weight (407k lines with 227k `tooie-move` entries).

## 9. Bounded diagnostic capture, `bs_setState` layout correction, and State 116 source proof (2026-10-17)

### 9.1. Player state structure layout correction (CORRECTED/SUPERSEDED)

Prior claims in Section 8 that Banjo was executing state `bs = 4` during the overhead cinematic slide are **`CORRECTED/SUPERSEDED`**:
- **Discovery**: An independent oracle hook surfaced the player state triple (`bs_prev`, `bs_cur`, `bs_next`), revealing that the legacy `[demo-sync]` field (`MEM_W(player + 0x120)`) sampled offset `+0`, which is the *previous* state (`bs_prev`), not the *current* state.
- **Source proof (PROVEN from original Tooie MIPS)**:
  Inspection of original MIPS code for `bs_setState` at `0x8009E5D8` (in `RecompiledFuncs/funcs_350.c:1434-1551`, verified against ROM `0x01E5AEB0` core2 loaded at RAM `0x800815C0`):
  ```text
  0x8009E5DC: lw   t6, 0x120(a0) ; 0x8009E5E0: sw   a1, 0x8(t6)   ; next = id
  ...
  0x8009E618: lw   t9, 0x4(v0)   ; 0x8009E61C: sw   t9, 0x0(v0)   ; prev = cur
  0x8009E624: lw   t0, 0x8(v0)   ; 0x8009E628: sw   t0, 0x4(v0)   ; cur = next
  0x8009E630: sw   zero, 0x8(t1)                                  ; next = 0
  ```
  Therefore, `*(player + 0x120)` maps strictly as:
  - `+0x00`: `prev` (`bs_prev`)
  - `+0x04`: `cur` (`bs_cur`)
  - `+0x08`: `next` (`bs_next`)
  This matches Banjo-Kazooie `code_13780.c` and independent oracle inspection.
- **Impact on kinematic timeline**:
  - `glidx 35..37`: `bs_prev = 2`, `bs_cur = 4`.
  - `glidx 38`: `bs_prev = 4`, `bs_cur = 116` (`0x74`), `ptype = 2`, `tspd = 500.0`, `anim = 111` (duration 5.5), `velxz = 441.46`.
  - `glidx 50..120`: `bs_cur = 116` persists, `ptype = 2`, neutral stick / remap (`x=0, y=0`, angle 90, dist 0), `velocity` ~514..520, and player world position continuously translates across the ground floor.
  - State 4 is a transient walk/locomotion state lasting only ~3 ticks (`glidx 35..37`); it is **not** an "overhead cinematic" state. The persistent state during the hands-off slide is **State 116 (`0x74`)**.

### 9.2. Isolated bounded diagnostic implementation (TOOIE_DIAG_DEMO7_DRIFT=1)

To eliminate the massive 407k-line diagnostic cascade caused by `TOOIE_DIAG_DEMO_SYNC` and `TOOIE_DIAG_ATTRACT`, Sonnet (`claude-sonnet-5-5`) implemented an isolated, bounded diagnostic flag:
- **Flag**: `TOOIE_DIAG_DEMO7_DRIFT=1` (default OFF).
- **Behavior**: Strictly isolated; does **not** trigger movement diagnostics (`tooie-move`), actor scanning (`tooie-actor`), or full attract logging.
- **Hook site**: In `src/game/syscall_handler.cpp` at the exit of overlay 698 ep0 (`glrecord` dispatch), after `glrecord` consumes the pad record for `consumed_glidx` (= post-1), before guest player update for that frame. Values represent start-of-interval state (consistent with `demo-sync` conventions).
- **Gate & filtering**:
  - Gated strictly to `stage == 0, sub == 8` (`D_80135470` bytes 0, 1) and consumed `glidx 0..120`.
  - Emits at most 121 rows per cycle (resets when leaving `sub == 8`).
  - Samples player slot `D_80135490[0]` only; zero actor scanning.
  - Omits untrustworthy/contaminated fields: `cammode` (halfword overlaps float), `egg_slot`, and `map`.
- **Logged fields**:
  `[demo7-drift]` stage, sub, glidx, post, pad `(x, y, btn)`, remap `(x, y)`, bstick `(x, y, ang, dist)`, `bs_prev`, `bs_cur`, `bs_next`, `ptype` (`phys + 0x00`), `pos` (`player + 0xE4`), `ppos` (`phys + 0x04/08/0C`), `vel` (`phys + 0x10/14/18`), `velxz`, `tspd` (`phys + 0x3C`), `anim` `(id, timer, dur)`, viewport `cam` xyz (`0x8012D500`), and `gs` (`0x8012C760`).
- **Files touched**: `src/game/tooie_demo7_drift.h`, `src/game/boot_hw_stubs.cpp`, `src/game/syscall_handler.cpp`, `src/game/tooie_diag.cpp`, `include/tooie_diag.h`, `tests/tooie_demo7_drift_test.cpp`, and `CMakeLists.txt`.
- **Verification**: Dedicated host test `tooie_demo7_drift_test` exits 0 (`PASS tooie_demo7_drift_test`).

### 9.3. Root bounded diagnostic execution (PROVEN)

The root user executed the bounded diagnostic without heavy collectors:
- **Command**: Direct `./build/TooieRecompiled` with `TOOIE_DIAG_DEMO7_DRIFT=1`.
- **Artifacts**:
  - Capture log: `/home/wesley/.cache/tooie-fps-investigation/demo7-bounded-root.log` (121 rows `glidx 0..120` parsed).
  - Build log: `/home/wesley/.cache/tooie-fps-investigation/demo7-root-full-build.log` (full build exit 0).
  - Controller duration: 407.21s; game own PID closed normally (SIGINT / clean shutdown).
  - Test validation: `tooie_demo7_drift_test` PASS exit 0. Pre-existing dirty worktree preserved.
- **Empirical validation**:
  - Exactly 121 rows emitted (`glidx 0..120`).
  - Confirms kinematics of earlier heavy capture without log bloat:
    - `glidx 0..33`: `bs_prev = 0, bs_cur = 1, bs_next = 0`, `anim = 111`, `tspd = 0.0`, `vel = (0, -1, 0)`.
    - `glidx 34`: `bs_prev = 1, bs_cur = 2`, `anim = 3`.
    - `glidx 35..37`: `bs_prev = 2, bs_cur = 4`, `anim = 12`, `tspd = 500.0`.
    - `glidx 38`: `bs_prev = 4, bs_cur = 116` (`0x74`), `anim = 111` (dur 5.5), `tspd = 500.0`, `velxz = 441.46`.
    - `glidx 50..120`: `bs_cur = 116`, stick/remap neutral (`(0, 0)`, ang 90, dist 0), `tspd = 500.0`, `velxz ~ 514..520`, world position continuously translates across XZ, Y descends to terrain floor -200.00 at `glidx = 106..120`.
  - Recompiled game position and velocity agree with prior heavy capture; no production fix yet.

### 9.4. State 116 (`0x74`) source proof and method dispatch (PROVEN)

Source analysis of Banjo-Tooie ROM bytes and original MIPS by Sonnet (`claude-sonnet-5-5`):
- **Dispatch table**: `D_80117F38` with stride 12 (`index = state * 12`, computed via `sll 2; subu; sll 2`).
  - Dispatchers: `0x8009CAAC` (END method, `a0 = 0`), `0x8009CAF8` (INIT method, `a0 = 1`), `0x8009CB44` (interrupt slot, `a0 = 3`), UPDATE (`a0 = 2`).
- **Entry 116**:
  - Stride 12 entry at `0x80117F38 + 116 * 12` points to `0x800839A8` (word +0), with flags `0x00750000` (+4) and `0x00000010` (+8).
  - `0x800839A8` is syscall stub `_bsrest_entrypoint_12` (syscall 84, entrypoint 12) in overlay 84 `bsrest` (`ROM 0x01F33AF0`, `RAM 0x80800000`).
- **Method table (`0x80800930`)**:
  `_bsrest_entrypoint_12` at `0x80800688` is a getter returning `*(0x80800930 + 4 * a0)`:
  - `[0] = 0x800839A0`: ep11 = END (`0x80800660`).
  - `[1] = 0x80083990`: ep9 = INIT (`0x808005B8`).
  - `[2] = 0x80083998`: ep10 = UPDATE (`0x808005F8`).
  - `[3] = 0x80099B94`: core2 default interrupt.
- **Method behavior disassembly**:
  - **INIT (ep9)**: Fetches `(anim_id, speed)` via `0x80084698`; calls `0x8008CB3C` (starts animation, selecting anim 111); if `0x8009CC68(player) == 4`, sets flag `0x80095760(ps + 0x80, 0x18) = 1` and passes bone/tag 5 vector via `0x800F3BD0` to object via `0x800893F8`.
  - **UPDATE (ep10)**: Updates bone 5 vector if flag set; then evaluates exit gates:
    1. If `0x8008E35C(player)` is true -> sets next state to 119 (`0x77`);
    2. Else if `0x8080032C(player) != 0` -> selects next state via `0x8080069C` (ep13) and calls `bs_setNextState` (`0x8009E5C8`).
  - **END (ep11)**: Cleans up flags (`0x808003FC`, `0x8080003C`).
  - **Physics invariance**: **NONE** of INIT, UPDATE, or END writes or resets `tspd` (`phys + 0x3C`), `velocity` (`phys + 0x10..0x18`), or `ptype` (`phys + 0x00`). State 116 contains no physics mode reset logic.
- **Exit gate `0x8080032C` analysis**:
  - Reads current anim via `0x8008CABC` / `0x8008B264`. For anim 111 (not `0x50` or `0x5B`), returns 1 (permitting exit) **only if `0x8009CC68(player) == 0`**.
  - `0x8009CC68`: Reads byte at `(*(ps + 0xF0))[ depth * 16 - 16 ]`, where `depth = (*(ps + 0xF0)) + 0x80` (returns 0 if depth == 0).
- **Epistemic boundary**:
  - Persistent State 116 may be original scripted cinema behavior; do not assume a faulty state stack or state exit bug without an original hardware/oracle baseline.
  - None proven as bug yet. The owning layer of the defect is UNKNOWN. Do not implement physics freezing, velocity zeroing, or global clock scaling.

### 9.5. Angrylion oracle capture: Phase 1 incomplete and Phase 2 in progress

- **Phase 1 Oracle Run (INCOMPLETE / NOT PASS)**:
  - Rebuilt with `scripts/build.sh --force` (exit 0) under `P08_DEMO7_SUB8=1`.
  - CSV artifact: `/home/wesley/.cache/tooie-fps-investigation/demo7-oracle-isolated/demo7.csv`.
  - Result: Yielded only 5 rows (`glidx = 0, 33, 34, 35, 36`).
  - Cause: The hook was placed on PC `0x8009C914` (`yaw_setIdeal`), which is not called per-frame during idle/still windows, and output was buffered until `glidx = 120`. Controller process shutdown normally via SIGINT (PID 735836, 802.31s).
  - Empirical alignment: Aligning oracle row `idx + 1` with Tooie shows tight agreement on current position and velocity before State 116 (pos max error 0.0058, vel max error 0.0064 rounded), though anim timers/durations show possible phase difference.
  - Epistemic note: Do NOT claim this was a buffer-only issue or that 5 rows prove full matching.
- **Phase 2 Oracle Instrumentation & Correction (Sonnet)**:
  - Corrected versioned `tools/p08_attract_egg_hook.h` and patch `tools/oracle/angrylion-control/patches/mupen64plus-core-p08-instrumentation.patch` to hook a single per-logic PC `0x800151BC` tick-start sampler with pc/phase/tick/gs, adding `fflush()` per row without yaw mixing.
  - Default OFF under same `P08_DEMO7_SUB8=1` flag.
  - Checked: reverse apply check exit 0, `build.sh --force` exit 0, core dirty baseline preserved.
  - Note: Hash `50abbcbb9215fa780ef03d67573c93a38e79e622f1a602d3a12fbad81adc8c02` is the SHA-256 of the git diff bytes, not a file content hash.
- **Phase 2 Oracle Capture Execution (IN_PROGRESS)**:
  - Running at `/home/wesley/.cache/tooie-fps-investigation/demo7-oracle-phase2/demo7.csv` (PID 750322, controller 52251).
  - Execution mode: `--nospeedlimit` official dummy under isolated config/saves/state (logical parity ONLY; wall timing PERTURBED; no original user game process).
  - Epistemic constraint: Do **NOT** claim results until the capture completes.
  - Source phase row `gs` left from previous update is not necessarily the integration interval; join by `glidx` and pad, and validate phase/shift before drawing causality conclusions.

### 9.6. Expanded user objective and acceptance criteria

The user has expanded the investigation objective beyond Demo 7 player drift to include potential incorrect global behavior or speed (player, mobs, camera):
- **Criterion 1 (Demo 7 cutscene parity)**: Faithful Demo 7 stationary cinema parity versus original N64 hardware/oracle.
- **Criterion 2 (Real-time speed preservation)**: Preserve guest logic, VI cadence, physical motion, animation playback, and camera tracking at faithful real-time speed (not mere FPS pose repetition).
- **Scope rule**: Diagnostics running with `--nospeedlimit` or across two isolated instances do NOT prove wall-clock throughput. Do not apply a global fix based on observations from a single map.

### 9.7. Session provenance, agent handoff, and documentation audit

- **Grok 4.7 Medium**: Executed first capture analysis (exit 0); legacy `bs = 4` source investigation was interrupted (exit 130); fresh State 116 attempt reached `max_turn_requests` (18, exit 1) with no owner report; explicitly `NOT_PASS`.
- **Sonnet 5.5 (`claude-sonnet-5-5`)**: Successfully escalated and delivered primary MIPS disassembly proof for `bs_setState` layout (`+0 prev, +4 cur, +8 next`), State 116 entry/method/exit disassembly, isolated diagnostic flag implementation, and Phase 2 oracle tick-start sampler.
- **Documentation audit**:
  - `markdown_vault_audit`: 288 files scanned; 0 errors, 15 diagnostic issues, 44 lint issues, 59 legacy warnings. Actual counts reported from tool; none invented.
  - Preserved constraints: No code, configuration, or generated files modified. Runtime CURRENT workaround, FPS settings, and audio remain strictly untouched.

### 9.8. Oracle Phase 3 trace and differential findings (PROVEN)

Angrylion oracle Phase 3 trace (`demo7-oracle-phase3-trace/demo7.csv`, 120 data rows `glidx 1..120`; `console.log` 40 lines) captured the exact N64 execution flow during Demo 7:
- In the N64 oracle trace, sequence 11 `tspd_set(ps, a1=0)` at return address `0x8034CBB4` is nested directly inside sequence 10 `bs_setState(116)` (`ra = 0x8009AB60`).
- Before State 116 entry, position and velocity match Tooie within tight tolerances.
- In Tooie pre-fix (`demo7-write-trace-root.log`), sequence 10 entered and returned with zero nested `tspd_set` calls; `tspd` remained locked at `500.0` and velocity was never cleared.

### 9.9. MIPS caller trace and generator defect isolation (PROVEN)

Primary reverse-engineering of original MIPS instructions isolated the missing call:
- **Overlay Base Proof**: The per-frame State 116 UPDATE method calls `bs_setState(ps, 0)` at overlay offset `0x644` (`jal 0x8009E5C8` in `bsrest` ep10, ROM `0x01F33AF0 + 0x644`), return address = base + `0x64C`. The oracle shows return address `0x8034D1EC` for that call, establishing `base(bsrest) = 0x8034CBA0`.
- **Target Helper**: Return address `0x8034CBB4 = base + 0x14` corresponds to the delay slot after `jal 0x8009B9B0` at `bsrest` overlay offset `0x0C` (with `li a1, 0` at `0x10`). This is the static helper at `bsrest` overlay offset `0x0` (ROM `0x01F33AF0`, RAM `0x80800000`):
  ```mips
  0x00 addiu sp, sp, -24
  0x04 sw    ra, 0x14(sp)
  0x08 sw    a0, 0x18(sp)
  0x0C jal   baphysics_set_target_horizontal_velocity
  0x10 li    a1, 0                              ; tspd = 0
  0x14 jal   func_8008E078                      ; gate on player struct
  0x1C beqz  v0, +0x14
  0x24 jal   func_8009BA9C (a1=0)               ; velocity reset to (0, 0, 0)
  ```
- **Caller Chain**: `bs_setState(116)` -> INIT ep9 (`0x808005B8`) -> `0x8080020C` (anim start) -> `0x80800308` calls helper at `0x80800000`. Prior claim that "State 116 init contains no reset logic" was `CORRECTED/SUPERSEDED` (the helper at the tail of `0x8080020C` was previously misread as a jump/data table).
- **Tooie Generator Defect**: In committed generated output `RecompiledFuncs/funcs_416.c:4188`:
  ```c
  RECOMP_FUNC void static_344_80800000(...) { uint64_t hi...; int c1cs = 0; ;}
  ```
  The helper was emitted with an **EMPTY BODY**. Exactly two empty statics existed in the entire codebase: `static_344_80800000` (this stop helper) and `static_304_80800324` (in `.bsledge`).
- **Root Cause in `N64Recomp`**: In `N64Recomp/src/main.cpp` (~L794-815), static function end-boundary detection used the address of the first section function >= static address. Because the decompilation ELF had zero-sized NOTYPE marker symbols at `0x80800000` (`bsrest_VRAM` and `bsrest_TEXT_START` in section 344), `cur_func_end == static_func_addr`, causing `insn_words` to be empty and emitting a hollow function body `;}`.

### 9.10. Generator fix, regression test, and tooling regeneration (PROVEN)

1. **Upstream Fix**:
   - Patched `N64Recomp/src/main.cpp`: when searching for the closest section function to bound a static, skip entries equal to the static's address that are zero-sized in that section (and check index bounds before indexing).
   - Versioned patch: `patches/runtime/n64recomp-zero-size-static.patch` (SHA256 `48ca1e7710a7b6ecd2f4cfe3c8ac064c11b2307ad98f35ee53b70b3f1cfebb58`).
   - Registered in `tools/apply_runtime_patches.sh` directly after `n64recomp-tooie.patch`.
2. **Regression Test**:
   - Implemented `tests/n64recomp_zero_size_static_test.sh`: copies metadata, inserts a zero-sized function symbol at `0x80800000` in `.bsrest`, runs generator, and asserts `static_*_80800000` contains `LOOKUP_FUNC(0x8009B9B0)`.
   - Results: Fixed generator exits 0 (`PASS zero-size static keeps body`); old pre-fix generator backup exits 1 (`FAIL empty/missing static_*_80800000 body`).
3. **Official Tooling Regeneration**:
   - Input recovery: Restored pinned decompilation fork `25033be6ec9e35975483f064d086391d2381ca4c`, ROM SHA1 `af479771ff5e91ce3732f3d84146614ab71d8d79`, matching ELF `banjotooie_decompressed.elf` (SHA1 `2b2d04c5b6484594f750b097daf670f7a8a24167`, size 91855676).
   - Full regeneration executed via `tools/run_recomp.sh` / in-tree fixed `N64Recomp` (46,472 functions across 420 files).
   - Atomic replacement: Exactly 5 of 420 files differed from the prior tree:
     - `funcs_416.c`: `static_344_80800000` gained 53 lines (restoring `LOOKUP_FUNC(0x8009B9B0)` with `a1=0` and velocity clear) and `static_304_80800324` gained 14 lines.
     - `funcs_18.c`, `funcs_216.c`, `funcs_229.c`, `funcs_343.c`: one rounding line each updated from `lroundf` to `ROUND_W_S` (semantically identical via `#define lroundf recomp_round_w_s` in `include/recomp.h`).
   - Zero generated code was edited by hand.
   - Freshness check: `python3 tools/check_generated_fresh.py` exits 0 ("OK: RecompiledFuncs is newer than inputs").
   - Full build: `ninja -C build TooieRecompiled tooie_demo7_drift_test` exits 0 (425 steps, `build/TooieRecompiled` linked).

### 9.11. Independent Grok final gate evaluation (PROVEN)

Independent gate audit executed by Grok on fixed runtime capture `demo7-fixed-root.log` (121 rows `glidx 0..120`, controller 396.13s wall time):
- **Criterion 1 (Bounded Demo 7 Drift)**: **PASS**.
  - 119 pose pairs aligned (original $i \to$ fixed $i+1$, $i=1..119$): **0 tolerance failures**.
    - Position max error 3D: `0.005771` (tolerance $\le 0.009$).
    - Velocity max error 3D: `0.006328` (tolerance $\le 0.009$).
    - Camera max error 3D: `0.008015` (tolerance $\le 0.009$).
    - Anim timer max error: `0.000485` (tolerance $\le 0.00051$).
    - Anim duration max error: `0.000447` (tolerance $\le 0.00051$).
    - Target speed (`tspd`) max error: `0.044263` (tolerance $\le 0.051$).
  - Input alignment ($i \to i$): 120/120 pad inputs match identically.
  - Trace nesting: Sequence 11 `tspd_set a1=0` nested inside sequence 10 `bs_setState(0x74)` verified on fixed log (pre `tspd = 500.0`, post `tspd = 0.0`, leave `vel = (0, 0, 0)`).
  - Stationarity (`glidx 38..120`, 83 rows): `bs_cur = 116`, `tspd = 0.0`, position fixed at `(-80.46, 0.00, 4496.25)`, `velxz = 0.0`.
- **Criterion 2 (Global Real-Time Speed & Cadence)**: **NOT YET PROVEN RESOLVED / BOUNDED CADENCE CONFIRMED**.
  - One matched window does not certify global speed. The `gs` word disagreement (0.05000 vs 0.03333 on 42/119 rows) reflects sample-phase differences (tick entry PC `0x800151BC` vs glrecord overlay 698 ep0 leave), not a per-update integration error in this window. Raw gamespeed snapshots across sample phases cannot equate global speed.
  - A dedicated 90.032-second global serial cadence test (`timing-fixed-pid-serial-90s.log`) was completed exit 0 with a PID-tagged mutex serialization fix (`g_timing_mu` in `src/game/tooie_diag.cpp`).
  - Results for guest execution stream (PID 938341):
    - First sample: `wall = 20.004s, vi = 1173, logic = 377`.
    - Last sample (<80s): `wall = 79.042s, vi = 4716, logic = 1812`.
    - Delta interval: `59.038s`, `3543 VIs` => **`60.0121955 Hz`**; `1435 logic updates` => **`24.3063789 Hz`** across changing scenes (individual scenes run at ~20 or ~30 Hz).
    - Parent/other stream (PID 938331) showed 0 VIs.
  - Bounded conclusion: **NO evidence of a 2x VI clock** in the measured window.
  - Scope limit: This does **NOT** prove universal camera/mob/campaign timing parity with N64, nor resolve potential user-specific velocity reproductions outside Demo 7. Production bug is closed for the tested window.

### 9.12. Concrete next single action (SUPERSEDES PRIOR)

`NEXT_SINGLE_ACTION`: normal clean user visual QA Demo 7, and concrete non-Demo 7 speed reproduction if persists.
