---
title: "P09 — Default shader-local CURRENT workaround and FPS presets plan"
description: "Phase 1 default workaround implemented; Phase 2 below-native present decimator implemented and unit-tested in RT64; 8-preset UI widget and runtime QA pending."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-10-17
tags: [p09, interpolation, workaround, fps, pacing, rt64]
---

# P09 — Default shader-local CURRENT workaround and FPS presets plan

## User Directive & Operational Scope
- User directive: install palliative workaround as DEFAULT run, document; THEN FPS selector {20, 30, 45, 60, 70, 100, 120, 144} without game speed / simulation changes.
- Checkpoint scope: Phase 1 DELIVERED (Tooie default-ON workaround); Phase 2 PLANNED & BLOCKED by below-native presentation pacing (not fake UI).
- Permanent fix: NOT IMPLEMENTED. Prior research (first-20 tie groups, Sonnet analysis) preserved as deferred history; no alias/snap implemented.
- Presentation behavior: Uses existing shader-local `tfPos` CURRENT bypass (`rt64_rsp_processor.cpp:60-67`, `RSPProcessCS.hlsl:78-84`), NOT global weights (`TOOIE_DIAG_FORCE_CURRENT_FRAME` remains separate). Vertex positions evaluate at CURRENT; other temporal weights (projection/view/lookat/texgen) and guest logic remain unchanged, but fog derives from `tfPos.z/w` and therefore follows CURRENT positions (as established in the causal baseline). No continuous position interpolation claim at 60+ FPS.

## Phase 1 Implementation & Static Verification
- Mechanism: `apply_default_before_renderer_init()` (`tooie_rsp_endpoint_current.cpp:14-38`) called at `main.cpp:1153` before `activate_tooie_guest_preemption()` (1158) and `recomp::start` (1161).
- Scope boundary: strictly Tooie integration; RT64 default remains OFF (`TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` unset in RT64 itself).
- Environment semantics: sets `"1"` ONLY when `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` is UNSET. All explicit values are preserved; `"0"` and empty (`""`) are reliably OFF in RT64, and `"1"` is ON (avoid universal assertions for arbitrary strings since the exact RT64 parser was not newly tested for all inputs; do not recommend strings such as `false` or `off` to disable).
- Rollback: explicit `0` on launch (`TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0 ./build/TooieRecompiled`) restores experimental original interpolation.
- Normal execution: `./build/TooieRecompiled` now requires NO activation flags. `setenv`/`_putenv_s` failure prints warning and startup continues.
- Diagnostic capture rule: future interpolation captures MUST explicitly set `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0` to avoid hiding glitches while CPU witnesses assume interpolation. Heavy collectors remain OFF.
- Proof strength: static source order scan in `main.cpp`; not a new GPU readback or runtime first-`getenv` trace.

## Source & Binary Evidence
- Source hashes:
  - `include/tooie_rsp_endpoint_current.h`: `35016f3ce777ef0c9a9a9cfd0097aab3ab236940fdba3d3f5521be6b1ab234e0`
  - `src/game/tooie_rsp_endpoint_current.cpp`: `fa2f93eb7e5ad9d38bf12e635081fffffe7f41e5641deb4e7d42bd1e938b4a3d`
  - `tests/tooie_rsp_endpoint_current_test.cpp`: `b4592c65bace33503bef6e64cf37de10e6e274f68ac6520d87f85fdb2b8b0937`
  - `src/main/main.cpp`: `60ad3911b62d53c57f40d6094ab14295ded876e10106ab27a5f981b77624177f`
  - `include/tooie_diag.h`: `9de0beac929efaa9974aebf6d3b811b04e17b36c47c70082fc8cb42d28881a6c`
  - `CMakeLists.txt`: `64bada48280f37a22af53d48b0ab6aebce71325e6241727c4109d18e58016650`
- Binary hashes & validation:
  - `build/tooie_rsp_endpoint_current_test`: `19e9d9e233033ca82f282b1649ab17bd3417f2deb335aea853f3577702611708`
  - `build/TooieRecompiled`: `0d2e4ba5fa7e4ed0347d83180ff4c831c54b8f47567fa012de15dac3dc736b84`
  - Test command: `./build/tooie_rsp_endpoint_current_test` exit 0 (20/20 assertions PASS, persisted in `/tmp/p09-default-workaround/policy-test-root.log`).
  - RT64 baseline: HEAD `6f1c2d99`, byte identity unchanged (not rebuilt, 31 dirty files +13850/-44 preserved). Game clock unchanged.
  - Untouched tests: `window_display_test` (38), `tintro_projection_policy_test` (7), `p09_exact_use_correlation_test` (1354) NOT RUN this round.

## Launch Provenance & User Gameplay QA
- Launch provenance: writer report stating "Game was not launched" reflects writer-scoped execution boundary (agents share the filesystem, but writer did not launch the binary). Root actually launched `./build/TooieRecompiled` without flags (`/tmp/p09-default-workaround/normal-launch.log`, size 852KB, mtime 00:41:50).
- User gameplay QA: "Está rodando bem. O áudio tem o mesmo glitch de sempre que já tinha desde quase o início do projeto e pretendo resolver o áudio só mais para frente."
- Parity classification: `USER_OBSERVED` normal run works well. Known historical CURRENT visual PASS retained separate from current root QA. Existing audio glitch is explicitly deferred by user request; no audio changes. No quantitative timing benchmark yet.

## Phase 2 FPS Presets & Below-Native Presentation Decimation
- Exact planned presets: `20, 30, 45, 60, 70, 100, 120, 144`.
- Frontend options: `ui_config_tab_graphics.cpp:311-313` provides generic `Manual` integer option (20..240 step 1, default 60), clamped in runtime to 10..1000. The dedicated 8-preset user selector widget remains `PLANNED` / `NOT_IMPLEMENTED`.
- Presentation & pacing architecture:
  - Previous blocker: Below-native target (`targetRate <= viOriginalRate`) sent 1 present per native workload without decimation or sleeping. Below-native target was an effective no-op.
  - Solution: RT64 Present Decimator (`src/hle/rt64_present_decimator.h` & `src/hle/rt64_present_queue.cpp`).
  - Integer accumulator: `PresentDecimator` computes exact average presentation cadence for non-divisors (45, 70, ...) and divisors (20, 30) using `ticks += targetRate; if (ticks >= originalRate) { ticks -= originalRate; return true; }` without floating-point drift or integer overflow (`uint32_t`).
  - Condition: applies when `manualRate && !present.paused && (targetRate > 0) && (viOriginalRate > 0) && (targetRate < viOriginalRate)`.
  - Workload ACK path (static proof): Skipped presentations take the existing catch-up skip path in `PresentQueue::threadLoop()`: `skipInterpolation()`, `notifyPresentId(present)`, framebuffer-operation release, and `threadAdvanceBarrier`. Every guest workload is consumed and acknowledged. The guest thread, VI timing, and audio processing are never slept or stalled.
  - Concurrency & data race elimination: Sonnet review identified that `rt64_workload_queue.cpp:1021` previously wrote `ext.sharedResources->viOriginalRate` without holding a lock, while the present thread read it under `configurationMutex` (proven data race). The write is now protected by `const std::scoped_lock configurationLock(ext.sharedResources->configurationMutex)`. Lock ordering is verified safe: `configurationMutex` is a leaf lock across both workload and present threads (no lock inversion).
  - Cadence resets: catch-up skips explicitly reset the decimator (`presentDecimator = PresentDecimator()`); paused presents pass `present.paused == true` into `shouldPresent()`, resetting the cadence so the first unpaused frame presents immediately without being dropped.
- Build & test validation:
  - Host test: `tests/rt64_present_decimator_test.cpp` added to `CMakeLists.txt` (`rt64_present_decimator_test`). Checks 20/60, 30/60, 45/60, 20/30, initial workload display, pass-through on non-manual/original/equal-above-native/unknown/paused, rate change resync, and pause-reset. Test exits 0 (`rt64_present_decimator_test PASS`).
  - Companion test: `tooie_rsp_endpoint_current_test` exits 0 (20/20 assertions PASS).
  - Full root build: `cmake --build build --target TooieRecompiled rt64_present_decimator_test tooie_rsp_endpoint_current_test -j2` exit 0 (logged in `/home/wesley/.cache/tooie-fps-investigation/root-build.log`, `[362/362] Linking CXX executable TooieRecompiled`).
  - Final incremental gate: logged in `/home/wesley/.cache/tooie-fps-investigation/root-build-final.log` completed exit 0 (`[6/6] Linking CXX executable TooieRecompiled`).
  - Patch: `patches/rt64/rt64-present-rate-decimator.patch` passes dry-run against HEAD RT64 exit 0.
  - Diff check: `git diff --check` passes exit 0.
- Boundaries, epistemic state & limitations:
  - Runtime QA NOT performed: no game process was launched for this test; no actual in-game FPS measurements or speed guarantees are claimed.
  - Workload ACK path is a static proof only.
  - Above-native rates: extra presents occur, but because the default-ON CURRENT `tfPos` workaround is active, intermediate frames repeat the current vertex pose. Smooth motion at rates above native remains impossible until permanent continuous interpolation is implemented.
  - Dynamic guest logic Hz: numeric `viOriginalRate` remains formally `UNKNOWN` (unmeasured).
  - Eight-preset UI widget: remains `PLANNED` / `NOT_IMPLEMENTED`.

## Top-Left Present FPS Counter & Settings Toggle
As documented in [[p09-fps-counter-and-settings-toggle]], the top-left visible present FPS counter and "Show Present FPS" graphics settings toggle have been verified across codebase implementation, standalone versioned patch generation, host unit tests, and documentation. The counter measures actual successful swapchain present cadence rather than selected target or simulation rate. The toggle defaults to OFF (`false`), persists in `graphics.json`, and reuses the existing frontend RmlUi render-thread hook without parallel custom UI. In-game runtime QA is pending.

## Single Next Action
`NEXT_SINGLE_ACTION`: with user authorization, launch `./build/TooieRecompiled`, enable "Show Present FPS" in Graphics options, and visually verify top-left overlay presentation and pose repeat behavior in gameplay.
