---
title: "P09 session handoff — live Banjo model + RE docs"
description: "USER_DEMO7 drift investigated (stage 0 sub 8 map 282 pinned, owner UNKNOWN); P09 FPS counter and settings toggle verified; RT64 present success counter and RmlUi hook documented."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-10-17
tags: [tooie-recomp, p09, attract, demo7, drift, fps, counter, settings, overlay, rt64, recompfrontend, present-cadence]
---

# P09 session handoff — live Banjo model + RE docs

Registro operacional vivo: [[p09-current-work]]. Atualizar junto deste handoff
quando houver mudança de estado; confirmar build atual antes da captura V2.

Canonical RE: [[../../reverse-engineering/README]]. Do not use
SUPERSEDED `NEXT_SINGLE_ACTION` (the old presentation/compositing
line, and the older “do not add `TOOIE_DIAG_SECONDARY_SWAY`” line).

## CURRENT (2026-10-17) — USER_DEMO7_CUTSCENE_DRIFT_INVESTIGATION_BOUNDED

Read [[p08-user-demo7-cutscene-drift-investigation]], [[p08-attract-movement-residuals]], [[p08-provisional-follow-camera-and-egg-differential]], and [[p09-fps-counter-and-settings-toggle]].
Read-only bounded investigation of user-observed Demo 7 player drift conducted (provenance: Grok 4.7 Medium analysis in `/home/wesley/.cache/tooie-fps-investigation/cutscene-grok.json`). No code, build, or runtime execution this task (`NOT_RUN`; zero tests reported `PASS`). Prior FPS counter and settings toggle completion, Phase 1 default CURRENT `tfPos` workaround completion, below-native decimator completion, and audio glitch deferral remain strictly preserved.

- Status & flags: `USER_DEMO7_MAPPING_PROVEN: YES`, `USER_DEMO7_MOTION_OWNER: UNKNOWN`, `USER_DEMO7_ORACLE_STATIONARITY: UNKNOWN`, `USER_DEMO5_RESIDUALS_PRESERVED: YES`, `DEFAULT_WORKAROUND_IMPLEMENTED: YES`, `BELOW_NATIVE_DECIMATOR_IMPLEMENTED: YES`, `FPS_COUNTER_AND_TOGGLE_VERIFIED: YES`, `PERMANENT_INTERPOLATION_FIX_NOT_IMPLEMENTED: YES`, `FPS_PRESETS_UI_PLANNED_NOT_IMPLEMENTED: YES`, `P09_FORMAL_PHASE: PENDING / NOT_FORMALLY_OPEN`, `NO_COMMIT: YES`, `NO_PUSH: YES`.
- Scene Mapping & Counter Logic (PROVEN static):
  - Spoken user sequence: `intro -> Demo 1, 2, 3 -> intro -> Demo 4, 5, 6 -> intro -> Demo 7, 8, 9` (hands off keyboard/controller).
  - USER_DEMO7: `stage = 0`, `sub = 8`, internal table index 8 of `D_808003D0` in `.gsattract` (731) at VRAM `0x80800000`. Map 282 (`0x011A`, TDL Stomping Plains), entrance 8, flags `0x01`, mode 5, `glrecord` overlay 698 (1244 samples / count 1245). Preceding intro occurrence 3.
  - Sub 7 contrast: `stage = 0`, `sub = 7` is map 395 (`0x018B`, CS Startup Intro then T_INTRO), mode 5 then 3. That is the intro between Demo 6 and Demo 7, not USER_DEMO7.
  - Counter implementation: `s_user_demo` in `src/game/syscall_handler.cpp:393-428` increments only when `mode == 5`, `map != 0`, and `map != 395`. Low byte `demo_key` (`0x1A`) has no collision within the 16 stage-0 records.
- Disambiguation from Superseded Catalog (PROVEN static):
  - Zero-based "Demo 2" in `p09-user-observed-parity-residuals:190` (`MAP_11A`) is the same map, but evaluated only a partial 520-frame window (vs 1244 samples). `Max err < 0.79u` (`STABLE_NOISE`) was position error versus Angrylion oracle, NOT stationary proof. Header is `CORRECTED/SUPERSEDED` (2026-08-21).
- Motion Owner & Oracle State (UNKNOWN):
  - User observation: player drift observed with hands off controls; deliberate physical input ruled out.
  - Motion owner: UNKNOWN (unseparated between guest world motion, animation root motion, and render pose).
  - Oracle stationarity: UNKNOWN (no still-window oracle baseline for map 282 exists in cache or logs).
  - Existing diagnostic limitation: `tooie_demo_path_tick` (`src/game/boot_hw_stubs.cpp:1638-1644`) early-returns on idle (`return; // pre-walk idle: skip`), dropping the pre-walk still window.
- Demo 5 Status Preservation:
  - Bounded helper `tooie_ud5_bainput_window` is strictly gated to `sub == 5`. Demo 5 is not globally resolved from the bounded helper; open Demo 5 residuals in `p08-attract-movement-residuals` remain preserved.
- Preserved Prior State:
  - Present FPS counter and Graphics options toggle remain verified and ready for gameplay QA.
  - Phase 1 default CURRENT `tfPos` workaround remains default-ON; continuous interpolation remains not implemented.
  - Phase 2 below-native decimator in RT64 remains operational.
  - Audio glitch remains explicitly DEFERRED per user directive.

`NEXT_SINGLE_ACTION`: execute bounded diagnostic capture for stage 0 sub 8 map 282 logging raw glrecord index/sample, OSContPad, remap floats, bastick, bs_id, velocity/accel/gamespeed, world delta, and render pose on each guest tick of the pre-walk idle window, paired with Angrylion oracle check for scene stationarity; or launch `./build/TooieRecompiled` for authorized FPS counter visual QA.

## Prior (2026-10-17) — P09_FPS_COUNTER_AND_SETTINGS_TOGGLE_VERIFIED

Read [[p09-fps-counter-and-settings-toggle]], [[p09-current-work]], [[p09-default-current-workaround-and-fps-plan]], [[../../specs/Tooie/graphics-and-presentation]], and [[../../HANDOFF]].
Under explicit user authorization, the top-left visible present FPS counter and extra settings toggle have been verified across codebase implementation, standalone versioned patch generation, host unit tests, and documentation. The counter measures actual successful swapchain present cadence rather than selected target or simulation rate. The toggle defaults to OFF (`false`), persists in `graphics.json`, and reuses the existing frontend RmlUi render-thread hook without parallel custom UI. In-game runtime QA is pending.

- Status & flags: `DEFAULT_WORKAROUND_IMPLEMENTED: YES`, `BELOW_NATIVE_DECIMATOR_IMPLEMENTED: YES`, `FPS_COUNTER_AND_TOGGLE_VERIFIED: YES`, `PERMANENT_INTERPOLATION_FIX_NOT_IMPLEMENTED: YES`, `FPS_PRESETS_UI_PLANNED_NOT_IMPLEMENTED: YES`, `P09_FORMAL_PHASE: PENDING / NOT_FORMALLY_OPEN`, `NO_COMMIT: YES`, `NO_PUSH: YES`.
- Settings & Persistence:
  - Option: "Show Present FPS" added to `add_graphics_options` in `src/game/config.cpp:186-198` and `include/banjo_config.h` (`banjo::configkeys::graphics::show_fps_counter`).
  - Default: `false` (OFF). Persists automatically through `graphics.json`.
  - User directions: In-game or launcher configuration menu -> Graphics tab -> check "Show Present FPS" to enable. Setting updates live via change callback `tooie::set_fps_overlay_enabled()`.
- Counter & Cadence Architecture:
  - Measurement: `RT64::presentSuccessCount` atomic in `src/hle/rt64_present_stats.h` incremented strictly upon successful `ext.swapChain->present(...)` in `rt64_present_queue.cpp:513`. Decimator skips and failed presents are not counted.
  - Pacing separation: `presentTimestamp` in `rt64_present_queue.cpp:508` remains a pacing clock for `preciseSleepUntil` (above-native target); it is not an FPS display metric. Developer ImPlot present-time graph (`rt64_state.cpp:2465`) remains dev-mode only.
  - Meter: `tooie::PresentRateMeter` (`include/tooie_fps_meter.h`) computes rounded integer rate over a 1 s monotonic window. Stalls > 3 s reset window and display `"-- FPS"`.
  - Overlay: `recompui::set_overlay_update_callback` added to RecompFrontend (`recompui/src/base/ui_state.cpp`), called on render thread in `draw_hook()`. Displays `"%u FPS"` at 16 dp from top-left with dark backing; not a shown context (unaffected by `hide_all_contexts()`, zero input capture).
- Versioned Patches:
  - RecompFrontend: `patches/frontend/recompfrontend-overlay-update-callback.patch` (SHA256: `37266d0bd13d42c0b8f174c0118b6a9b2490784c5880f05e6ee37ab811cd48a7`) on clean `d0d90ba`.
  - RT64: `patches/rt64/rt64-present-success-counter.patch` (SHA256: `173c9b7cafc4b9750fb610b56faca8125399679a874d1c159f6c53e420c31c68`) on dirty `6f1c2d99`.
  - Reverse apply check: `git apply --reverse --check` exits 0 for both patches.
- Build & Verification Gates:
  - Host test: `./build/tooie_fps_meter_test` exits 0 (`tooie_fps_meter_test PASS`). Covers 1 s window, midpoint rounding, zero presents, stalls > 3 s, backwards clock, and resets.
  - Diff check: `git diff --check` passes exit 0 across TooieRecomp, RT64, and RecompFrontend.
- Epistemic Boundaries & Critical Distinctions:
  - Present Cadence vs. Motion Rate: With default-ON CURRENT `tfPos` workaround active, presentation rates above native repeat the current vertex pose. The counter reading near 60 FPS while visible motion updates at ~30 Hz reflects actual swapchain presents, not 60 Hz simulation/motion.
  - Runtime QA NOT performed: No game process was launched during this verification; on-screen visual presentation in live gameplay is pending root/user review.
  - Eight-preset UI widget: Remains `PLANNED` / `NOT_IMPLEMENTED` (`REQ-GFX-06`). The FPS counter is an independent settings toggle.
  - Native guest logic Hz: `viOriginalRate` remains formally `UNKNOWN` (unmeasured).

`NEXT_SINGLE_ACTION`: with user authorization, launch `./build/TooieRecompiled`, enable "Show Present FPS" in Graphics options, and visually verify top-left overlay presentation and pose repeat behavior in gameplay.

## Prior (2026-10-17) — P09_RT64_PRESENT_DECIMATOR_IMPLEMENTED_AND_BUILD_GATES_PASSED

Read [[p09-current-work]], [[p09-default-current-workaround-and-fps-plan]], [[../../specs/Tooie/graphics-and-presentation]], and [[../../HANDOFF]].
Following audit of pacing limitations in RT64 `WorkloadQueue`, Phase 2 below-native presentation decimation is implemented in RT64, verified by host unit test `rt64_present_decimator_test` PASS exit 0, and validated by clean root compilation gates. P09 Phase 1 default CURRENT `tfPos` workaround remains active; 8-preset UI widget and in-game runtime QA remain pending.

- Status & flags: `DEFAULT_WORKAROUND_IMPLEMENTED: YES`, `BELOW_NATIVE_DECIMATOR_IMPLEMENTED: YES`, `PERMANENT_INTERPOLATION_FIX_NOT_IMPLEMENTED: YES`, `FPS_PRESETS_UI_PLANNED_NOT_IMPLEMENTED: YES`, `P09_FORMAL_PHASE: PENDING / NOT_FORMALLY_OPEN`, `NO_COMMIT: YES`, `NO_PUSH: YES`.
- Pacing fix mechanism:
  - Header `src/hle/rt64_present_decimator.h` adds integer accumulator `PresentDecimator` (`ticks += targetRate`, `ticks >= originalRate`), yielding exact average presentation rates for divisors and non-divisors (20, 30, 45, 60) without float drift or integer overflow (`uint32_t`).
  - Active condition: `manualRate && !present.paused && (targetRate > 0) && (viOriginalRate > 0) && (targetRate < viOriginalRate)`.
  - Present queue integration (`src/hle/rt64_present_queue.cpp`): skipped presents take existing catch-up skip path (`skipInterpolation()`, `notifyPresentId(present)`, framebuffer-operation release, and `threadAdvanceBarrier`).
  - Workload ACK path static proof: all guest workloads are consumed and acknowledged in order; guest logic, audio tasks, and VI interrupts are never delayed, slept, or stalled.
- Concurrency & cadence fixes (Sonnet review followup):
  - Proven data race eliminated: `rt64_workload_queue.cpp:1021` previously wrote `ext.sharedResources->viOriginalRate` without holding a lock while the present thread read it under `configurationMutex`. Write is now wrapped under `const std::scoped_lock configurationLock(ext.sharedResources->configurationMutex)`. Lock ordering is verified safe (leaf lock; no lock inversion).
  - Catch-up skip explicitly resets the decimator (`presentDecimator = PresentDecimator()`).
  - Paused presents pass `paused == true` into `shouldPresent()`, resetting cadence so the first unpaused frame displays immediately without dropping.
- Build & test gates:
  - Host test: `tests/rt64_present_decimator_test.cpp` added to `CMakeLists.txt` (`rt64_present_decimator_test`). Tests 20/60, 30/60, 45/60, 20/30, initial workload display, pass-through on non-manual/original/equal-above-native/unknown/paused, rate change resync, and pause-reset. Test exits 0 (`rt64_present_decimator_test PASS`).
  - Companion test: `tooie_rsp_endpoint_current_test` exits 0 (20/20 assertions PASS).
  - Root full build: `cmake --build build --target TooieRecompiled rt64_present_decimator_test tooie_rsp_endpoint_current_test -j2` exit 0 (logged in `/home/wesley/.cache/tooie-fps-investigation/root-build.log`, `[362/362] Linking CXX executable TooieRecompiled`).
  - Final incremental gate: logged in `/home/wesley/.cache/tooie-fps-investigation/root-build-final.log` completed exit 0 (`[6/6] Linking CXX executable TooieRecompiled`).
  - Patch: `patches/rt64/rt64-present-rate-decimator.patch` applies cleanly over RT64 dirty baseline.
  - Diff check: `git diff --check` passes exit 0 across Tooie and RT64 trees.
- Epistemic boundaries & explicit limits:
  - Runtime QA NOT performed: no game process was launched for this verification; no actual in-game FPS measurements or speed guarantees are claimed.
  - Workload ACK path is a static proof only.
  - Above-native rates: extra presents occur, but because default-ON CURRENT `tfPos` workaround is active, intermediate frames repeat the current vertex pose. Smooth motion at rates above native remains impossible until permanent continuous interpolation is implemented.
  - Dynamic guest logic Hz: numeric `viOriginalRate` remains formally `UNKNOWN` (unmeasured).
  - Eight-preset UI widget: remains `PLANNED` / `NOT_IMPLEMENTED`.
  - P09 formal status: remains `PENDING / NOT_FORMALLY_OPEN`.

`NEXT_SINGLE_ACTION`: implement the 8-preset UI selector widget in RecompFrontend and perform runtime QA with FPS measurements.

## Prior (2026-10-16) — P09_DEFAULT_WORKAROUND_IMPLEMENTED_AND_FPS_PRESETS_PLANNED

Read [[p09-current-work]], [[p09-default-current-workaround-and-fps-plan]], and [[../../HANDOFF]].
Following user directive to install the palliative workaround as default run before addressing FPS selection, Phase 1 default workaround is implemented, verified by unit test (20/20 PASS) and independent Haiku review, and verified in root normal gameplay QA. Phase 2 FPS presets {20, 30, 45, 60, 70, 100, 120, 144} are planned but NOT implemented, blocked by actual below-native presentation pacing constraints in RT64 `WorkloadQueue`.

- Status & flags: `DEFAULT_WORKAROUND_IMPLEMENTED: YES`, `PERMANENT_INTERPOLATION_FIX_NOT_IMPLEMENTED: YES`, `FPS_PRESETS_PLANNED_NOT_IMPLEMENTED: YES`, `P09_FORMAL_PHASE: PENDING / NOT_FORMALLY_OPEN`, `NO_COMMIT: YES`, `NO_PUSH: YES`.
- Workaround mechanism: `apply_default_before_renderer_init()` (`src/game/tooie_rsp_endpoint_current.cpp:14-38`) runs in `src/main/main.cpp:1153` before `activate_tooie_guest_preemption()` and `recomp::start()`. When `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` is UNSET, sets `"1"`. Explicit values (`"0"`, `""`, `"1"`, `"keep"`) are preserved. Explicit `0` restores experimental original interpolation. Normal `./build/TooieRecompiled` requires no activation flag.
- Scope & presentation behavior: Workaround is strictly scoped to Tooie integration; RT64 default remains OFF. Uses shader-local `tfPos` CURRENT bypass (`RSPProcessCS.hlsl:78-84`), NOT global weights (`TOOIE_DIAG_FORCE_CURRENT_FRAME` unchanged). Vertex positions are CURRENT; other weights, presentation cadence, and guest logic unchanged. No continuous position interpolation claim at 60+ FPS.
- Future diagnostic rule: Future interpolation diagnostics MUST explicitly set `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0` to prevent workaround from masking visual glitches while CPU witness assumes interpolation. Heavy collectors remain OFF.
- Build & test evidence: `cmake --build build --target tooie_rsp_endpoint_current_test` exit 0; `./build/tooie_rsp_endpoint_current_test` exit 0 (20/20 assertions PASS, saved in `/tmp/p09-default-workaround/policy-test-root.log`). Executable `TooieRecompiled` linked exit 0 (binary SHA256 `0d2e4ba5fa7e4ed0347d83180ff4c831c54b8f47567fa012de15dac3dc736b84`). RT64 HEAD `6f1c2d99` untouched (31 dirty files +13850/-44 preserved). Ordering check is static source order, not runtime/GPU trace. Untouched tests (38, 7, 1354) NOT RUN this round.
- Run provenance & QA: Writer report "Game was not launched" is writer-scoped sandbox provenance. Root actually launched `./build/TooieRecompiled` without flags (`/tmp/p09-default-workaround/normal-launch.log`, size ~852KB). User observed: "Está rodando bem. O áudio tem o mesmo glitch de sempre que já tinha desde quase o início do projeto e pretendo resolver o áudio só mais para frente." Existing audio glitch explicitly deferred; no audio changes.
- Phase 2 pacing facts: UI already accepts manual integer 20..240 step 1 (`ui_config_tab_graphics.cpp:311-313`); runtime accepts 10..1000. These generic values are NOT the requested 8-preset selector. In RT64 (`rt64_workload_queue.cpp:1175`), pacing/interpolation runs only when `targetRate > viOriginalRate`. For below-native targets (`targetRate <= viOriginalRate`), `generateInterpolated=false`, `displayFrames=1`, sending one present per native workload without lower-rate pacing or sleep. Hence 20/30 honesty is not guaranteed (native guest Hz currently UNKNOWN). Preserving ALL guest updates/VI/audio without skipping guest work requires a true presentation decimation hook; backpressure `waitForWorkloadId` is UNVERIFIED. User `graphics.json` unchanged (`Display`, manual 60).
- Deferred research queue: Prior research on first-20 tie groups, nearest-join NDC ties, and Sonnet analysis is preserved as historical and deferred queue; root is not reopened and aliases are not implemented.

`NEXT_SINGLE_ACTION`: singular focused audit/measurement of native cadence and present-queue backpressure to choose a below-native presentation cap without clock changes; then implement presets with actual tests all 8 and timing QA.

## Prior (2026-10-16) — P09_PATHOLOGICAL_TIE_GROUPS_CAPTURED_AND_ARBITRATED

Read [[p09-current-work]], [[p09-pathological-tie-groups]], [[p09-exact-use-pathology-correlation]], and [[p09-causal-baseline-and-fix-direction]].
Bounded first-20 pathological tie-group diagnostic implemented, validated by host tests (1354/1354 exit 0), compiled in TooieRecompiled (rebuilt binary SHA256 `84d31479...`, exit 0), and captured in gameplay runtime (`EXHAUSTED` at 24 attempts, 6 plans, 162 pathological pair occurrences, all ambiguous TIE, 179 unique keys).

- Captures & hashes: `/tmp/p09-tie-discriminator/capture-gameplay/tooie-p09-seam-edge-pathology-correlation.txt` (SHA256 `74b0b2e5f219522c42b041a6a5cea9005bea8f57ddd0fc595df8f69024528c7d`) and sibling `/tmp/p09-tie-discriminator/capture-gameplay/tooie-p09-seam-edge-pathology-ties.txt` (SHA256 `5b1fc21a2af77dbab9abc42919465e45a65a13014321cee405468cc6fd59b2a8`). Versioned patch `patches/rt64/p09-exact-use-pathology-correlation.patch` SHA256 `7b93258cecadcb0b18b1f4a22f0a278c70a1cc20f3e1f0ed8fbb112b0bd7f38f`.
- Group structure: 20 groups stored (`kP09TieGroupCap=20`), 142 omitted (`skippedGroups=142`), 0 member truncation. 12 mult=2, 8 mult=4 = 56 parent face-corner occurrences across distinct 9-field keys. All 20 groups have exactly 2 distinct `storedVertex` records per group at identical parent `WORLD` and printed coordinates (not bitwise identity). All 20 child keys and 56 members returned `MEMBERSHIP=NOT_SELECTED` (`REASON=NONE`) against local active exhaustive plan.
- Independent review & arbitration: Haiku static review PASS (noted unchecked `fclose` before rename, non-blocking); Haiku capture analysis clarified `selected=YES` (nearest search choice) vs `MEMBERSHIP=NOT_SELECTED` (plan membership) as separate non-contradictory fields. Sonnet solution escalation executed; Root arbitration confirmed Sonnet withdrew P1 point-cert (`k`-points, reciprocity, 0.08), global plan miss claim, and raw ID persistence. All three Sonnet options (P1, P2, P3) remain `HYPOTHESIS`.
- Scope & boundaries: `PRODUCTION_FIX_NOT_IMPLEMENTED: YES`, `P09_FORMAL_PHASE: PENDING / NOT_FORMALLY_OPEN`, `NO_COMMIT: YES`, `NO_PUSH: YES`.

`NEXT_SINGLE_ACTION`: a READ-ONLY focused audit of existing paired-boundary-edge/provenance data and its correlation to the sampled pathological uses, starting with producer/topology semantics and what current artifacts actually contain; no new lerp/alias or telemetry implementation automatically. Root stops at newly identified evidence blocker after Sonnet analysis as authorized, not claiming fix.

## Prior (2026-10-15) — P09_EXACT_USE_GAMEPLAY_CAPTURED_ALL_AMBIGUOUS_TIE

Read [[p09-current-work]], [[p09-exact-use-pathology-correlation]], and [[p09-causal-baseline-and-fix-direction]].
Causal baseline is established (`CAUSE_ALREADY_ESTABLISHED: YES`, root class `NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS` proven decisively via `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1` and `TOOIE_DIAG_FORCE_CURRENT_FRAME=1`), while production fix is not implemented (`PRODUCTION_FIX_NOT_IMPLEMENTED: YES`).
Armed gameplay capture executed; process reached limit (`EXHAUSTED` at 24 attempts). Meaningful runtime data captured, but all 194 pathological pair occurrences are ambiguous TIE; coverage rate is undecidable. This existing 194-TIE capture remains an exact-use diagnostic blocker for nearest-NDC join authorization onto alias keys, NOT an unknown root cause. No production fix is implemented.

- Capture file & hash: `/tmp/p09-correlation-implementation/capture-gameplay-20261015.txt` (SHA256 `b6be018a3619a78a7c05ba7d0b8fa6c64e27d22e5cde6bedbebeebdf771203e7`). Initial empty capture retained as history. Game left running in `EXHAUSTED` state (no new searches without rearm).
- Accounting & plans: 2135 walks, 93 eligible dispatches, 24 attempts (4 with path pair, 20 with no path pair), 4 seam searches, 4 local plans built (`PLAN_BUILT=4`). No plan failure, capacity cap, or truncation (`PLAN_INACTIVE=0`, `PLAN_NOT_EXHAUSTIVE=0`, `PAIRS_DROPPED_PENDING_CAP=0`, `VERTS_DROPPED_BONE_CAP=0`, `UNIQUE_TABLE_OVERFLOW=0`, `RECORDED_COHORT_TRUNCATED=NO`).
- Pathology correlation: `PATH_PAIR_TOTAL=194`, `PATH_PAIR_FULLY_COVERED=0`, `PATH_PAIR_ONE_SIDED=0`, `PATH_PAIR_NOT_COVERED=0`, `PATH_PAIR_AMBIGUOUS=194`, `PATH_PAIR_AMBIGUOUS_TIE=194`. Saturated by 194 ties (`PATH_PAIR_FULL_OF_ALL_OBSERVED=0.000000`, definitive rate `NA`). Cumulative occurrences across 4 plans, not 194 unique certified edges.
- Keys & nonpathological universe: 214 unique path keys (1 selected, 213 not selected). Nonpathological: 454 pairs, 481 unique keys (4 selected, 477 not selected). 25 keys on both path and nonpath.
- CPU timings (runtime diagnostic only, NOT game FPS or visual proof): attempt peak 202.762 ms (total 590.915 ms), plan peak 200.776 ms (total 551.464 ms), gate total 10.382 ms.
- Haiku source review: proven that `p09EmitNearestJoinPairs` sets `bestTie = true` on exact float equality `gap == bestGap` in NDC XY distance, and classifier prioritizes TIE over key checks. Coincident parent hypothesis is UNKNOWN; identical coordinates do not prove ambiguity is false because sourcekeys/WORLD policy matter. No tie suppression or dedup production fix authorized.
- Arbitration & fix direction: Haiku proposal for new CLIP_LERP/NDC_LERP visual discriminator is SUPERSEDED by 2026-09-14 closure in `p09-articulated-temporal-coherence-root.md`. Next discriminator serves selection for the known cause, not a fresh search for root.

`NEXT_SINGLE_ACTION` (historical): one bounded diagnostic discriminator logging first ~20 tie groups, exact full sourcekeys/originalWORLD and prev/cur positions, gap/multiplicity and candidate memberships to distinguish repeated same-use from different legitimate-use; then review, before alias/interpolation edits (currently NOT_IMPLEMENTED; task stops at newly observed blocker per scope rule).

## Prior (2026-10-15) — P09_EXACT_USE_PATHOLOGY_CORRELATION_CAPTURED_EMPTY_AND_RELAUNCH_WAITING

Read [[p09-current-work]] and [[p09-exact-use-pathology-correlation]].
First actual capture executed; output is empty (`CAPTURED_EMPTY / NO_ELIGIBLE_SAMPLES`). Root relaunched with dedicated flag and confirmed waiting; user positioning character gameplay before root arming.

- First capture observation (`capture-empty-20261015.txt`): `ARM_COUNT=1`, `REARM_COUNT=0`, `MARKER_CONSUME_FAILURES=0`. `ELIGIBILITY_WALKS=181`, `ELIGIBLE_DISPATCHES=0`, `ATTEMPTS=0`, `PATH_PAIR_TOTAL=0`. Ineligible skip breakdown (first-failure order): `SKIPPED_INELIGIBLE_NO_WORKLOAD=61`, `SKIPPED_INELIGIBLE_NOT_INTERMEDIATE=59`, `SKIPPED_INELIGIBLE_NO_TARGET_CALL=61`; `NO_GEOMETRY=0`, `NO_FRAME_MATCH=0`, `NO_MODEL_BIN=0`. User estimated run at ~30 seconds; scene was `UNKNOWN`.
- Classification: `CAPTURED_EMPTY / NO_ELIGIBLE_SAMPLES`. This is NOT evidence of zero coverage, false positives, or correctness. Former runtime-not-captured state preserved as historical; meaningful pathology sample, visual confirmation, and real CPU execution timings remain `NOT_CAPTURED`.
- Haiku gate review (`haiku-empty-capture-output.json`): read-only check confirmed first-failure gate ordering (`NO_WORKLOAD` -> `NO_GEOMETRY` -> `NO_FRAME_MATCH` -> `NOT_INTERMEDIATE` -> `NO_MODEL_BIN` -> `NO_TARGET_CALL`) and 0x62B target display lists (`p09V4VisibleTargetIndex`). Do NOT promote its inference that movement is strictly required or that modelbin is resolved globally as proven: counters record first failure; zero `no_model_bin` applies only to the 61 dispatches reaching that gate.
- Relaunch status: root relaunched with dedicated flag (`TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION=1 ./build/TooieRecompiled`), confirmed status `STATE=WAITING_FOR_REQUEST`, zero walks. User positioning character gameplay before root arming.

`NEXT_SINGLE_ACTION` (historical): wait for user ready in character scene then arm and inspect eligible/attempt counters; still no aliases or production fix.

## Prior (2026-10-15) — P09_EXACT_USE_PATHOLOGY_CORRELATION_IMPLEMENTED_AND_REVIEWED

Read [[p09-current-work]] and [[p09-exact-use-pathology-correlation]].
Exact-use pathology correlation diagnostic implemented, validated by host tests, compiled in TooieRecompiled, and approved by independent Haiku review.

- Diagnostics & flags: `TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION` (default-OFF, CPU-only, standalone). Request marker `/tmp/tooie-p09-seam-edge-pathology-correlation.arm` with transactional `::remove == 0` consume, startup stale-marker deletion/ignore, pre-walk 2000 ms cadence gate (`p09CorrelationPreWalk`), and finite safety limits (max 24 heavy attempts, 8 seam searches).
- Methodology: 9-field keys on both pair members; same-call local plan avoids global transform-all-before-RSP overwrite; strict all-observed metric `PATH_PAIR_FULL_OF_ALL_OBSERVED` includes ambiguous cases in denominator.
- Gates & review: `p09_exact_use_correlation_test` 1231/1231 passed exit 0; `TooieRecompiled` linked 35/35 exit 0; companion tests 38/38 and 7/7 exit 0; git diff --check exit 0; patch `patches/rt64/p09-exact-use-pathology-correlation.patch` (SHA256 `5108ad3f...`) validated apply/reverse RC:0 on dirty baseline over RT64 `6f1c2d99`. Haiku final review PASS (static/helper/build scope); root arbitration confirmed hash list integrity and exit 0 logs.
- Runtime boundary: runtime pathology coverage, character scene capture, visual proof, CPU timings remain `NOT_CAPTURED`. No aliases, no production fix, formal P09 pending, cleanup/commit not ready (`NO_COMMIT=YES`, `NO_PUSH=YES`).

`NEXT_SINGLE_ACTION` (historical): user runs one scene capture with `TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION=1 ./build/TooieRecompiled`, arming via `/tmp/tooie-p09-seam-edge-pathology-correlation.arm` in character scene, and inspects correlation output; still no aliases or production fix.

## Prior (2026-10-15) — P09_V2_SEAM_EDGE_CAPTURE_REVIEW_CASE2

Read [[p09-current-work]] and [[p09-seam-edge-v2-capture-review]].
Review of preserved snapshot `/tmp/tooie-p09-seam-edge-dry-run-v2-20261015-030500.txt` completed.
Operational decision: `CASE2`. Gate 3 `PASS` scoped to 12 same-call detail records and latest-workload accounting only.

- 12 certified same-call detail records verified: 12/12 `REVERSED`, residuals 0.0 in PREV/CUR (<= tol), disjoint vertices and distinct original worlds, 24/24 competitor margins pass (`nextBest - best > marginNeed`). 135 cross-call certified pairs uninspected (detail output capped at 12); competitor choices not independently auditable.
- Counter semantics arbitrated via `rt64_transform_processor.cpp`: initial Haiku CASE3 suspicion resolved and `CORRECTED/SUPERSEDED` (151 tolpass = 135 cert + 16 ambig; 2063 hash gap is repeated edge inserts; 18 cross boundary gap is isolated projection groups).
- Visual & perf: user qualitative OFF normal / V2 slow observed; no measured FPS or exclusive search causality. Visual correction pending; no production fix, formal P09 pending, cleanup/commit not ready.
- Scope: advances exclusively to default-off CPU exact-use pathology correlation; not production seam identity approval or alias implementation.

`NEXT_SINGLE_ACTION` (historical): implement ONLY default-off CPU exact-use pathology correlation checking both pathological uses against full key (workloadId, fbPair, projection, gameCall, triangleLocal, corner, faceCorner, storedVertex, originalWorld); report total, covered, uncovered, ambiguous, and nonpathological selected; no aliases or performance patch now; correlation remains NOT_IMPLEMENTED/NOT_CAPTURED.

## Prior (2026-10-15) — P09_V2_RUNTIME_EXHAUSTIVE_SEARCH_CAPTURED

Read [[p09-current-work]], [[p09-seam-edge-dry-run-v2]], and [[../../HANDOFF]].
User ran V2 (`TOOIE_DIAG_P09_SEAM_EDGE_DRY_RUN_V2=1 ./build/TooieRecompiled`).
Process finished; no active game process or live flags. Preserved snapshot:
`/tmp/tooie-p09-seam-edge-dry-run-v2-20261015-030500.txt` (SHA256 `256d5fdd935c7d105810d885f4bbdb4047453606cdc5544bc56ef1e81bbbe6e5`).
Full counter/timing details documented once in [[p09-current-work]].

- Search accounting (latest workload): boundaries 4627, same-call evaluated 66588, cross-call raw 664920 (duplicates 554413, unique evaluated 110507), certified 55 same / 135 cross, exact uses 732. Session 2200 workloads cumulative (not latest workload count).
- Search exhaustiveness: `SAME_CALL_SEARCH_EXHAUSTIVE=YES`, `CROSS_CALL_BUCKET_SEARCH_EXHAUSTIVE=YES`, `CROSS_CALL_UNIQUE_PAIR_GENERATION_EXHAUSTIVE=YES`, all search dropped 0, `EMERGENCY_LIMIT_HIT=NO`, `RETRIEVAL_CELL_SMALLER_THAN_EDGE_SCALE=NO` (correcting fictitious `RETRIEVAL_RADIUS_RISK`), `LATEST_WORKLOAD_COUNTS_EXHAUSTIVE=YES`, `ALIAS_PLAN_EXHAUSTIVE=YES`. Text detail cap (12/190 accepted, 178 dropped) does not invalidate exhaustive search (`DETAIL_OUTPUT_ONLY_SATURATION=YES`).
- Acceptance scope: search accounting accepted ONLY FOR LATEST WORKLOAD. Structural/coherence review pending (`CERTIFIED` alone insufficient to prove edge identity). Pathology coverage remains `NOT_CAPTURED`.
- Visual observation: `INCONCLUSIVE` due to extreme slowdown reported by user. Diagnostic changes no rendering, no production fix; persistent glitches do not refute diagnostic. Strong CPU-overhead hypothesis supported by self-times (search 64ms, session peak 439ms), NOT full frame profiler, NOT measured FPS, NOT controlled A-B. Haiku review (`/tmp/p09-haiku-performance.json`) noted callsite cpp:3259 (not unconditional every-frame). A-B qualitativo (`USER_OBSERVED`): volta ao normal sem a flag e desaceleração severa associada ao V2 ativo; confirma associação relatada, mas não é FPS medido, benchmark controlado, prova de custo exclusivo de busca versus outra telemetria nem causalidade de thread/frequência; self-times existentes inalterados (64ms latest, 439ms pico).
- Build & phase status: Retain build `PASS` from 14/10 with freshness caveat (`BYTE_IDENTITY=UNKNOWN_BUILD_DID_NOT_RECOMPILE_SOURCE`). `P09_FORMAL_PHASE=PENDING / NOT_CLAIMED`. `PRODUCTION_FIX_UPDATED=NO`. `CLEANUP_READY=NO`. `COMMIT_READY=NO`.

`NEXT_SINGLE_ACTION`: inspect certified paired-edge A/B provenance and residual/tolerance/margin details from preserved snapshot; only after coherent certification proceed exact-use pathology correlation. Do not implement aliases yet.

## Prior (2026-10-14) — P09_V2_SEARCH_EXHAUSTIVENESS_VALIDATION_GATE

Read [[p09-current-work]], [[p09-seam-edge-dry-run-v2]], and [[../../HANDOFF]].
Actual validation gate verified by orchestrator on 2026-10-14 23:51 -0300:
- Build: `cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test` exit 0 (`[3/3] Linking TooieRecompiled`, log `/tmp/p09-grok-gate-20261014/build.log`).
- Unit tests: `./build/window_display_test` exit 0 (`PASS: 38/38 assertions passed`, log `/tmp/p09-grok-gate-20261014/window_display_test.log`), `./build/tintro_projection_policy_test` exit 0 (`PASS: 7/7 assertions passed`, log `/tmp/p09-grok-gate-20261014/tintro_projection_policy_test.log`).
- Trees & diff checks: `git -C TooieRecomp diff --check` exit 0; `git -C BanjoRecomp/lib/rt64 diff --check` exit 0.
- Source edits: `NONE`. Game launch: `NOT_RUN`. No runtime capture, no visual validation, no production fix.
- Git HEADs: Tooie `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf`, BanjoRecomp `ec859632cfa584e2272d32e46051543429a2baf4`, RT64 `6f1c2d99a4ea571c139f449c326fd176ba8f3496`.
- Freshness caveat (mandatory): incremental build reused the RT64 object compiled on 2026-09-24 (`rt64_transform_processor.cpp.o`) and relinked `TooieRecompiled` on 2026-10-14 23:51. `source mtime <= object mtime <= binary mtime`. V2 symbols (`RT64::p09SeamEdgeDryRunV2WouldAliasExactUse`, `RT64::p09SeamEdgeDryRunV2AliasPlanExhaustive`) are confirmed present via `nm` in both object and executable. This establishes an `INFERENCE` of freshness, NOT byte-for-byte proof of compiling this exact source; `BYTE_IDENTITY=UNKNOWN_BUILD_DID_NOT_RECOMPILE_SOURCE`. Do not claim RT64 source recompiled or that full P09 is proven.
- Artifact hashes: Source SHA256 `941ae09e8891889c2cf3057d06aaa5a413f2efdf6b4be7fe1a020a786c221a07`, Object SHA256 `2f9887fe227c6aff0bf63d018f9610aca8d62e1784be5a010eeb00b310e1b932`, Executable SHA256 `17873fec359484654d8deeb7ae40302f4a4a44b7a40c6ea0e792c63e91152953`.
- Multi-model routing: Grok real model `grok-4.7` medium (not fast, `context_window=200000`), Gemini documentation (Gemini 3.8 Flash High), Haiku `claude-haiku-5-5` medium independent read-only review; Sonnet/Opus not used. Initial CLI permission attempts failed/cancelled before gate; final gate completed.
- Haiku review summary (`/tmp/p09-haiku-review.json`): static check confirmed repair present in `rt64_transform_processor.cpp` for same-call (:2270-2302), cross-call (:2326-2425), and fail-closed safety stops (:2700-2712), but did not follow `gP09V2Plan.exhaustive` assignment to final refusal path (do not claim full review).
- Status: Captures and pathology coverage remain `NOT_CAPTURED`; no aliases or production fix; cleanup/commit not ready; P09 formal remains pending (`PENDING / NOT_FORMALLY_OPEN`).
- Vault audit: marked pending execution by orchestrator after edits.

`NEXT_SINGLE_ACTION`: user performs isolated V2 capture in character scene, first preserving existing output, enabling ONLY `TOOIE_DIAG_P09_SEAM_EDGE_DRY_RUN_V2=1 ./build/TooieRecompiled`; inspect all exhaustive fields incl `RETRIEVAL_RADIUS_RISK=NO`, certification and exact-use provenance. Captures and pathology coverage remain `NOT_CAPTURED`; no aliases or production fix; cleanup/commit not ready; P09 formal remains pending.

## Prior (2026-10-14) — P09_PAIRED_EDGE_V2_SEARCH_EXHAUSTIVENESS_REPAIR

Read [[p09-seam-edge-dry-run-v2]] and [[../../HANDOFF]]. A static re-read
of `rt64_transform_processor.cpp` found the repair already present. This
pass did not edit that file, did not change identity or `1e-4` tolerance,
and did not launch the game. The pre-repair file is not a cross-call
negative: same-call evaluation stopped at 16384, the 48-entry bucket walk
dropped 180588 entries, and the dedup probe failed.

Same-call search evaluates every unordered pair in the call and keeps a
streaming best and next-best partner. Cross-call retrieval is an exact
32-unit cell plus its 26 neighbors. Each retrieved unordered pair is
evaluated once from the lower edge id. `SAME_CALL_PAIRS_DROPPED_FOR_CAP`,
`CROSS_CALL_BUCKET_ENTRIES_DROPPED`, and
`CROSS_CALL_UNIQUE_PAIRS_DROPPED_FOR_CAP` are 0.
`CROSS_CALL_PAIR_DEDUP_PROBE_FAILED=NO`.
`CROSS_CALL_DUPLICATE_PAIR_RETRIEVAL_COUNT` is raw neighbor hits before
per-query unique, minus lower-id-owned pairs. That includes multi-cell
repeats and the reverse lookup. It is not a failed probe.

Safety stops (1e6 edges, 32e6 same-call evaluations, 4e6 unique cross-call
pairs, 1e6 exact uses, 2e6 vertex-audit vertices) set
`EMERGENCY_LIMIT_HIT=YES` and clear
`LATEST_WORKLOAD_COUNTS_EXHAUSTIVE` / `ALIAS_PLAN_EXHAUSTIVE`. A cell
smaller than `1e-4 * current edge length` does the same. Those flags after
the repair are `NOT_CAPTURED`. Pathology coverage stays `NOT_CAPTURED`.
Certified detail names both edges. Rendering is unchanged.

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_P09_SEAM_EDGE_DRY_RUN_V2=1 ./build/TooieRecompiled`
on the character scene, then confirm search, certification, and the
exact-use plan are exhaustive. Do not correlate pathology until that
capture is in. Do not materialize aliases.

## Prior (2026-09-24) — P09_SEAM_ALIAS_DRY_RUN_VALIDITY_REVIEW

Read [[p09-seam-alias-dry-run-validity-review]]. The user capture reports
29 cross-call and zero same-call certified pairs in the latest workload.
The current rule certifies single point pairs at two endpoints, so it is
not enough to identify an articulated seam. Bounds saturation may truncate
search or the alias plan. No exact-use join connects the corrected V2
pathological pairs to this plan. Decision `CASE_B`; B+C remains conditional.

`NEXT_SINGLE_ACTION`: implement only a versioned default-off CPU dry-run
with paired-edge/provenance identity, per-limit saturation counters, and
exact-use plan keys for subsequent corrected V2 coverage correlation.
No production aliases, remap, shader change, commit or push.

## Prior (2026-09-24) — P09_CALL_LOCAL_WORLD_INTERPOLATION_ARCHITECTURE_REVIEW

Read [[p09-call-local-world-interpolation-architecture-review]]. User visual
result for multi-only selector: Kazooie feathers regress, Banjo backpack/top
and Mumbo feathers appear corrected, Klungo stripes and Mumbo flicker persist,
and Banjo snout remains deformed. Both index-level policies are insufficient.
GameCall face ownership exists on CPU; shared vertices require a clone for
different per-call policies. `State::fullSync` draw-data upload precedes
render-time transform interpolation. Recommendation: bounded, default-off
candidate aliases and vertex remap before existing upload, with two-endpoint
seam certification after original frame matching. No production code change.

`NEXT_SINGLE_ACTION`: implement the CPU alias/remap prototype, starting with
dry-run alias/shared-vertex/capacity and false-positive counts. No game was
launched in this review; no commit or push.

## Prior (2026-09-17) — P09_GENERIC_SELECTIVE_WORLD_INTERPOLATION_PRODUCTION_DESIGN

Read [[p09-generic-selective-world-interpolation-production-design]].
Default-off `TOOIE_TEST_P09_SELECTIVE_ELEMENTWISE_WORLD=1` in
`TransformProcessor`. Classifier: GameCall vertex `worldIndices` with
more than one distinct WORLD. Exact 16-entry lerp for those slots only.
Failed W-sign production snap is off. Heavy RSP diagnostic must stay off
for the visual.

`NEXT_SINGLE_ACTION`: `TOOIE_TEST_P09_SELECTIVE_ELEMENTWISE_WORLD=1
./build/TooieRecompiled` and report streaks / smoothness / shear / rigid
camera-object behavior.

## Prior (2026-09-16) — P09_STANDARD_SLERP_WORLD_PRODUCTION_CANDIDATE_DISCRIMINATOR

Read [[p09-standard-slerp-world-production-candidate-discriminator]].
CPU arm `STANDARD_SLERP_WORLD` is implemented, default-off as diagnostic
only, on repaired V2 pairing. Rotation uses explicit shortest-arc SLERP;
other decompose factors match production. Schema
`P09_RSP_ENDPOINT_BINDING_V2_SHADER_EQUIVALENT`.
`ROOT_CAUSE_CLASS=NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`
from the ELEMENTWISE recapture. Do not ship elementwise or standard SLERP
yet. Offline `PASS` (synth 14/14, TooieRecompiled, window 38/38, tintro 7/7).

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled` and paste `STANDARD_SLERP_WORLD_*`.

## Prior (2026-09-16) — P09_REPAIR_CPU_DIAGNOSTIC_SHADER_SEMANTICS

Read [[p09-cpu-diagnostic-shader-semantics-repair]].
Canonical helper is `mul(pos4,mul(WORLD,VIEWPROJ))`. All 14 active CPU
tfPos consumers were repaired; MODELVIEW/hierarchy remain retired.

## Prior (2026-09-15) — P09_CPU_SHADER_MATRIX_SEMANTICS_AND_HOMOGENEOUS_AUDIT

Read [[p09-cpu-shader-matrix-semantics-and-homogeneous-audit]].
`CPU_SHADER_MATRIX_ACTION_EQUIVALENT=NO` was `PROVEN` for the pre-repair
helpers. That audit authorized this mechanical diagnostic repair.

## Prior (2026-09-15) — P09_IMPLEMENT_EXACT_ORDER_VALIDATED_CAMERA_POSE_VIEW

Read [[p09-exact-order-validated-camera-pose-view]]. CPU arm is
implemented on the LLLL samples with validated mathematical SLERP and
exact `((POSE_DERIVED_LIVE_VIEW*LIVE_PROJ)*LIVE_WORLD)*LIVE_POS4`.
New keys `NOT_CAPTURED`. Shader unchanged. Do not ship production.

```text
NEXT_SINGLE_ACTION: TOOIE_DIAG_RSP_ENDPOINT_BINDING=1 ./build/TooieRecompiled
POSE_DERIVED_VIEW_MATCHES_SAFE_CLASS: UNKNOWN_NOT_CAPTURED
PRODUCTION_POLICY_CHANGED: NO
GAME_LAUNCHED: NO
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

## Prior (2026-09-15) — P09_INDEPENDENT_ARCHITECTURE_ROOT_CAUSE_REVIEW

Read [[p09-independent-architecture-root-cause-review]] before acting.
Existing status matches the user's capture: 249 class-A pathological JOINs,
zero misaligned pathological JOINs; mapping-coherent fallback fails.
Root `UNRESOLVED`. The architecture review specified this pose arm; it
did not implement it.

VIEW is direct matrix interpolation. WORLD's quaternion helper has correct
endpoint direction with `1-weight`, but a nonstandard interior rotation law.
WORLD JOIN xyz omits homogeneous w, limiting its coherence proof. CLCL's
endpoint-reference metrics do not prove a unique root. The older pose arm
preserved order but used the same quaternion helper and did not eliminate
its 199 triangle explosions; it is not today's validated pose JOIN test.

## Prior (2026-09-15) — P09_MAPPING_COHERENCE_FALLBACK_COUNTERFACTUAL

Read [[p09-mapping-coherence-fallback-counterfactual]]. Previous
capture: 12 VIEW-mapped/WORLD-unmapped calls. Static: WORLD
unmapped is CURRENT→CURRENT while VIEW interpolates. CPU keeps
A–E / COHORT_ALIGNED_LIVE and adds MAPPING_COHERENT_LIVE.
Keys `NOT_CAPTURED`. Shader unchanged. Do not ship CLCL.

```text
PRODUCTION_POLICY_CHANGED: NO
GPU_READBACK_USED: NO
NEXT_SINGLE_ACTION: TOOIE_DIAG_RSP_ENDPOINT_BINDING=1 ./build/TooieRecompiled
```

## Prior (2026-09-15) — P09_TEMPORAL_COHORT_MISMATCH_CAUSAL_PROOF

Read [[p09-temporal-cohort-mismatch-causal-proof]]. Previous
capture: 12 VIEW-mapped/WORLD-unmapped calls; CLCL fixes 288
pathological JOINs; do not ship CLCL. CPU correlates pathology
vs cohort class and runs COHORT_ALIGNED_LIVE.

```text
PRODUCTION_POLICY_CHANGED: NO
GPU_READBACK_USED: NO
NEXT_SINGLE_ACTION: TOOIE_DIAG_RSP_ENDPOINT_BINDING=1 ./build/TooieRecompiled
```

## Prior (2026-09-15) — P09_VIEW_WORLD_INTERACTION_ROOT_DISCRIMINATOR

Read [[p09-view-world-interaction-root-discriminator]]. Factorial
CLCL is the VIEW×WORLD safe class. Do not ship CURRENT VIEW+WORLD.
CPU world-space JOIN, HIERARCHICAL_WORLD, and MODELVIEW_RIGID wait
recapture. Status write throttled ~1 Hz. Shader unchanged.

```text
PRODUCTION_POLICY_CHANGED: NO
GPU_READBACK_USED: NO
NEXT_SINGLE_ACTION: TOOIE_DIAG_RSP_ENDPOINT_BINDING=1 ./build/TooieRecompiled
```

## Prior (2026-09-14) — P09_TFPOS_FACTORIAL_TERM_ATTRIBUTION

Read [[p09-tfpos-factorial-term-attribution]]. World non-rigidity
`REFUTED`. User recapture: CLCL minimum CURRENT set matching
ALL_CURRENT. POSITION velocity 0. PROJECTION not decisive.

## Prior (2026-09-14) — P09_WORLD_MATRIX_INTERPOLATION_RIGIDITY_FORENSIC

Read [[p09-world-matrix-interpolation-rigidity-forensic]]. User
recapture: `WORLD_NONRIGIDITY_ROOT_CAUSE=REFUTED`.

## Prior (2026-09-14) — P09_DIAGNOSTIC_HEAP_CRASH_FORENSIC

Read [[p09-diagnostic-heap-crash-forensic]]. JOIN builder works
(`53946` pairs). Abort was a proven diagnostic container race
between `gfx_thread_func` and `WorkloadQueue::renderThreadLoop`.
CPU mutex + atomic status write. Do not infer
`BEST_COHERENT_SCOPE`. Do not implement W-crossing / coherent
scope as production. Recapture a complete status file.

```text
CRASH_CAUSED_BY_P09_DIAGNOSTIC: PROVEN
PRODUCTION_POLICY_CHANGED: NO
GPU_READBACK_USED: NO
NEXT_SINGLE_ACTION: TOOIE_DIAG_RSP_ENDPOINT_BINDING=1 ./build/TooieRecompiled
```

## Prior (2026-09-14) — P09_JOIN_DIAGNOSTIC_REPAIR_AND_PER_DRAW_AUDIT

Read [[p09-articulated-temporal-coherence-root]]. JOIN_PAIR_COUNT=0
was a diagnostic identity/topology miss, not a proof that joins are
absent. PER_DRAW envelope numbers ran without JOIN pairs and
without CURRENT-snap percent. CPU JOIN pairing now uses V7
world→bone plus articulated nearest verts. Production unchanged.

```text
PRODUCTION_POLICY_CHANGED: NO
GPU_READBACK_USED: NO
NEXT_SINGLE_ACTION: TOOIE_DIAG_RSP_ENDPOINT_BINDING=1 ./build/TooieRecompiled
```

## Prior (2026-09-13) — P09_SAFE_ENDPOINT_SCREENSPACE_INTERPOLATION_FORENSIC

Read [[p09-projective-w-production-fix]]. Valid residual `33020`.
Zero-cross dominates; TRIANGLE_CURRENT does not solve it. CPU
CLIP_LERP / NDC_LERP counterfactuals instrumented. Production
unchanged.

```text
PRODUCTION_POLICY_CHANGED: NO
GPU_READBACK_USED: NO
NEXT_SINGLE_ACTION: TOOIE_DIAG_RSP_ENDPOINT_BINDING=1 ./build/TooieRecompiled
```

## Prior (2026-09-13) — P09_RESIDUAL_TRIANGLE_DIAGNOSTIC_REPAIR

Read [[p09-projective-w-production-fix]]. CPU residual triangle
walk now matches V4 visible 0x62B faces. Prior zero triangle count
is `INVALID`. Production unchanged.

```text
RESIDUAL_TRIANGLE_SOURCE: V4_REUSED
PRODUCTION_POLICY_CHANGED: NO
GPU_READBACK_USED: NO
NEXT_SINGLE_ACTION: TOOIE_DIAG_RSP_ENDPOINT_BINDING=1 ./build/TooieRecompiled
```

## Prior (2026-09-12) — P09_HUMAN_OBVIOUS_GPU_BRANCH_MARKER

Read [[p09-projective-w-production-fix]]. Same default-off
`TOOIE_DIAG_RSP_MARK_W_CROSSING=1`; marker is now a bounded
horizontal NDC jump (`clip.x += 0.70 * clip.w`, ~35% of screen
width) only on GPU W-sign branch vertices. Production unchanged.

```text
HUMAN_OBVIOUS_BRANCH_MARKER_IMPLEMENTED: YES
PRODUCTION_POLICY_CHANGED: NO
GPU_READBACK_USED: NO
NEXT_SINGLE_ACTION: TOOIE_DIAG_RSP_MARK_W_CROSSING=1 ./build/TooieRecompiled
```

## Prior (2026-09-12) — P09_SELECTIVE_GPU_BRANCH_EXECUTION_PROOF

Read [[p09-projective-w-production-fix]]. Selective W-sign is
default-on. CPU POLICY_A has zero explosions. First branch marker
(`+0.20,-0.12` NDC) was too subtle for binary visual.

```text
CPU_POLICY_A_STATUS: ZERO_EXPLOSIONS
SELECTIVE_POLICY_DEFAULT_ON: YES
SELECTIVE_EXECUTION_BUG_FOUND: NO
BRANCH_MARKER_DIAGNOSTIC_IMPLEMENTED: YES
PRODUCTION_POLICY_CHANGED: NO
NEXT_SINGLE_ACTION: SUPERSEDED (human-obvious marker)
```

## Prior (2026-09-12) — P09_RSP_SHADER_EXECUTION_AND_BUFFER_VISIBILITY_PROOF

Read [[p09-projective-w-production-fix]]. Sidecar content matches
the CPU witness (`PROVEN`). SPIR-V branch and upload-before-dispatch
are present. Intermediate-only counts not yet recaptured. Shader-local
force CURRENT diagnostic is implemented, default-off.

```text
SIDECAR_CONTENT_MATCHES_WITNESS: PROVEN
SHADER_LOCAL_FORCE_CURRENT_DIAGNOSTIC_IMPLEMENTED: YES
PRODUCTION_POLICY_CHANGED: NO
NEXT_SINGLE_ACTION: TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1 ./build/TooieRecompiled
```

## Prior (2026-09-12) — P09_FAILED_PRODUCTION_CANDIDATE_AUDIT

Read [[p09-projective-w-production-fix]]. User visual FAIL for
per-vertex CURRENT-on-W-sign. Offline the policy removes the dumped
explosions (max ratio 1). Do not triangle/draw-widen. Next is GPU
endpoint-buffer consumption, not a new root-cause hunt.

```text
NORMAL_HIGH_RATE_RESULT: GLITCH (pre-candidate)
ORIGINAL_FRAMERATE_RESULT: FIXED
GLOBAL_CURRENT_FRAME_RESULT: FIXED
CPU_TEMPORAL_POSITION_MATH_EXPLAINS_VISUAL_GLITCH: YES
PROJECTIVE_W_SIGN_CROSSING_ROOT_STATUS: PROVEN
PRODUCTION_FIX_IMPLEMENTED: YES
PRODUCTION_FIX_CLASS: PER_VERTEX_CURRENT_ENDPOINT_ON_W_SIGN_CROSSING
GPU_READBACK_USED: NO
NEXT_SINGLE_ACTION: USER VISUAL VALIDATION
```

## Prior (2026-09-10) — P09_PROJECTIVE_W_AND_VIEWPROJ_REPRESENTATION_ROOT_CAUSE

Read [[p09-projective-w-and-viewproj-representation-root-cause]].
VERSION=3: 199 live / 199 SIMPLE. PR23 `REFUTED`. Combined relative
`INVALID`. Recapture witness `VERSION=4` **done**.

```text
NORMAL_HIGH_RATE_RESULT: GLITCH
ORIGINAL_FRAMERATE_RESULT: FIXED
V5C_RESULT: SAME
GLOBAL_CURRENT_FRAME_RESULT: FIXED
CPU_TEMPORAL_POSITION_MATH_EXPLAINS_VISUAL_GLITCH: YES
DECOMPOSE_VS_SIMPLE_CAUSAL_STATUS: REFUTED
PR23_SIMPLE_FIX_STATUS: REFUTED
VERSION3_RELATIVE_COUNTERFACTUAL_VALID: NO
CURRENT_ROOT_CLASS: PROJECTIVE_W_AND_VIEWPROJ_REPRESENTATION
PRODUCTION_FIX_CANDIDATE_FOUND: NO
NEW_RUNTIME_CAPTURE_REQUIRED: YES
GPU_READBACK_USED: NO
NEXT_SINGLE_ACTION: SUPERSEDED (VERSION=4 captured; production candidate implemented)
```

## Prior (2026-09-09) — P09_RELATIVE_VIEW_WORLD_INTERPOLATION_ROOT_CAUSE

Read [[p09-relative-view-world-interpolation-root-cause]]. VERSION=2
SIMPLE=51/51 with live. PR23 `REFUTED`. Dual B/C signatures. Do not
implement tagging. Recapture witness `VERSION=3` **done**; relative
experiment invalid.

```text
NORMAL_HIGH_RATE_RESULT: GLITCH
ORIGINAL_FRAMERATE_RESULT: FIXED
V5C_RESULT: SAME
GLOBAL_CURRENT_FRAME_RESULT: FIXED
CPU_TEMPORAL_POSITION_MATH_EXPLAINS_VISUAL_GLITCH: YES
DECOMPOSE_VS_SIMPLE_CAUSAL_STATUS: REFUTED
PR23_SIMPLE_FIX_STATUS: REFUTED
CURRENT_ROOT_CLASS: INDEPENDENT_VIEW_AND_WORLD_INTERPOLATION_OF_COUPLED_RELATIVE_MOTION
PRODUCTION_FIX_CANDIDATE_FOUND: NO
NEW_RUNTIME_CAPTURE_REQUIRED: YES
GPU_READBACK_USED: NO
NEXT_SINGLE_ACTION: SUPERSEDED (VERSION=3 captured; relative INVALID)
```

## Prior (2026-09-09) — P09_BONED_INTERPOLATION_COUNTERFACTUAL

Read [[p09-boned-interpolation-counterfactual]]. Live AUTO world
lerp is DECOMPOSE of composed bones. SIMPLE is reconstructed
analysis-only in witness schema v2. Do not implement tagging yet.
Do not add a CURRENT visual override.

```text
NORMAL_HIGH_RATE_RESULT: GLITCH
ORIGINAL_FRAMERATE_RESULT: FIXED
V5C_RESULT: SAME
GLOBAL_CURRENT_FRAME_RESULT: FIXED
CPU_TEMPORAL_POSITION_MATH_EXPLAINS_VISUAL_GLITCH: YES
CURRENT_RT64_WORLD_INTERPOLATION_MODE: DECOMPOSE
WRONG_SAME_BONE_OCCURRENCE_PAIRING_OBSERVED: NO
WORLD_MATRIX_INTERPOLATION_CAUSAL_STATUS: SUPPORTED
PRODUCTION_FIX_CANDIDATE_FOUND: YES
PRODUCTION_IMPLEMENTATION_READY: NO
NEW_RUNTIME_CAPTURE_REQUIRED: YES
GPU_READBACK_USED: NO
VISUAL_RECORDING_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION: recapture TOOIE_DIAG_FEATHER_TEMPORAL_COHERENCY=1 (VERSION=2); do not implement tagging until SIMPLE counts land
```

## Prior (2026-09-09) — P09_WORLD_OCCURRENCE_IDENTITY_ROOT_CAUSE_AUDIT

Read [[p09-world-occurrence-identity-root-cause-audit]]. Session-union
39 vs 9 bones; V5C missed JOIN-POP slots; pairing not proven.

## Prior (2026-09-09) — P09_PASSIVE_HIGH_RATE_TEMPORAL_COHERENCY_WITNESS

Read [[p09-passive-high-rate-temporal-coherency-witness]]. Capture
landed; interpretation moved to the occurrence-identity audit.

## Prior (2026-09-09) — P09_TEMPORAL_COHERENCY_ROOT_CAUSE

Read [[p09-temporal-coherency-root-cause]]. Projection-only CURRENT
is `INVALID_TEMPORALLY_INCOHERENT` and its code is removed. Global
current-frame remains `FIXED`. V5C remains `SAME`. Default high-rate
behavior is still `GLITCH` (no diagnostic leak). Do not add
per-component CURRENT overrides. Do not productionize
`TOOIE_DIAG_FORCE_CURRENT_FRAME`.

```text
NORMAL_HIGH_RATE_RESULT: GLITCH
ORIGINAL_FRAMERATE_RESULT: FIXED
V5C_RESULT: SAME
GLOBAL_CURRENT_FRAME_RESULT: FIXED
PROJECTION_ONLY_RESULT: INVALID_TEMPORALLY_INCOHERENT
PROJECTION_DIAG_REMOVED: YES
PROJECTION_DIAG_LEAKED_INTO_DEFAULT: NO
PRODUCTION_FIX_CANDIDATE_FOUND: NO
NEW_CAPTURE_REQUIRED: NO
GPU_READBACK_USED: NO
VISUAL_RECORDING_REQUIRED_NOW: NO
USER_VISUAL_TEST_REQUIRED: NO
```

## Prior (2026-09-09) — P09_HIGH_RATE_FORCE_CURRENT_PROJECTION

Read [[p09-high-rate-force-current-projection]]. User visual:
`INVALID_TEMPORALLY_INCOHERENT`. Diagnostic removed. Isolation of a
single processor with CURRENT is forbidden.

## Prior (2026-09-09) — P09_HIGH_RATE_FORCE_CURRENT_FRAME

Read [[p09-high-rate-force-current-frame]]. V5C visual `SAME`. Original
framerate `FIXED`. User visual of extra frames with CURRENT weights:
`FIXED`. Frame-weight interpolation is `CAUSAL`. Isolation continues
in [[p09-high-rate-force-current-projection]].

```text
V5C_VISUAL_RESULT: SAME
ORIGINAL_FRAMERATE_RESULT: FIXED
GLOBAL_CURRENT_FRAME_RESULT: FIXED
V5C_WAS_EQUIVALENT_TO_GLOBAL_CURRENT_FRAME: NO
PRODUCTION_FIX_CANDIDATE_FOUND: NO
```

## Prior (2026-09-09) — P09_FEATHER_NO_INTERP_V5C

Read [[p09-feather-no-interp-v5c]]. V5B did not freeze 107/108.
V5C covered all nine visible-triangle transforms. User visual: `SAME`.

```text
V5B_WAS_COMPLETE_FOR_EVERY_TRANSFORM_USED_BY_VISIBLE_TRIANGLE_VERTICES: NO
V5C_IMPLEMENTED: YES
V5C_VISUAL_RESULT: SAME
PRODUCTION_FIX_CANDIDATE_FOUND: NO
```

## Prior (2026-09-09) — P09_BANJORECOMP_TRANSFORM_TAGGING_PORTABILITY_AUDIT

Read [[p09-banjorecomp-transform-tagging-portability-audit]]. Tooie
lacks BK `set_model_matrix_group` / PR #22 sort-stable IDs. Live 0x62B
is AUTO. Tagging is still not a ready production port. The V5B-based
claim that interpolation was refuted for the visible feather chain is
`CORRECTED` by V5C coverage of 107/108.

```text
BANJORECOMP_READY_FIX_FOR_P09: NO
PR22_EQUIVALENT_IN_TOOIE: ABSENT
EXPLICIT_MATRIX_GROUP_GAP: YES
SORT_IDENTITY_GAP: NO
CPU_SKINNING_APPLICABLE_TO_FEATHER_DLS: NO
PRODUCTION_FIX_CANDIDATE_FOUND: NO
```

## Prior (2026-09-08) — P09_ANIMATION_CLIP_TRANSITION_AND_AUTHORED_KEY_FORENSIC

Read [[p09-animation-clip-transition-and-authored-key-forensic]].
Intra-clip time is continuous. Clip-change new times are start-0 plus
one ADE4 step. Clip 22 bone 107/108 raw TRS matches ROM cubic keys.
Prior 30/120° 107 steps are authored walk↔idle poses. Sampler status
C. Downstream renderer and guest compose stay closed.

```text
CURRENT_MATRIX_GENERATION_STATUS: CLOSED
SECONDARY_SWAY_STATUS: CLOSED
ANIMATION_TIME_SOURCE_STATUS: CLOSED
ANIMATION_SAMPLER_STATUS: C
PRODUCTION_FIX_CANDIDATE_FOUND: NO
PRESENTATION_AUDIT_NOW_JUSTIFIED: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEW_CAPTURE_REQUIRED: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NEXT_SINGLE_ACTION: user visual A/B vs N64/Angrylion of 0x62B Kazooie walk for authored 107 scale + 108 nod vs remaining visible wobble; no new witness; no GPU readback; no production fix
```

## Prior (2026-09-08) — P09_FEATHER_ANIM_SAMPLE_TIME_V1

Read [[p09-feather-anim-sample-time-v1]]. Default-off CPU witness
captured clip/time after `baanim_update` and raw 0x28 TRS before
AE8A8 (`RAW_BAANIM_UPDATES=1451`). Luna's `ANIMATION_SAMPLE_TIME_BUG`
from negative deltas is `CORRECTED`: those deltas are clip changes.
Classification continued in
[[p09-animation-clip-transition-and-authored-key-forensic]].

```text
CURRENT_MATRIX_GENERATION_STATUS: CLOSED
SECONDARY_SWAY_STATUS: CLOSED
ANIMATION_TIME_SOURCE_STATUS: CAPTURED
PRODUCTION_FIX_CANDIDATE_FOUND: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEW_CAPTURE_REQUIRED: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NEXT_SINGLE_ACTION: done; see CURRENT authored-key forensic
```

## Prior (2026-09-08) — P09_UPSTREAM_CURRENT_BONE_TRANSFORM_SEMANTIC_FORENSIC

Read [[p09-upstream-current-bone-transform-semantic-forensic]].
AE8A8 is Hamilton `parent ⊗ child` on XYZW records. Additive gates
are off. 107 scale/rotation is the movement-coupled input; 103/100
local rotations stay small. Clip id and sample time were missing
from that dump. Downstream renderer stages stay closed. Do not call
GPU bug.

```text
CURRENT_MATRIX_GENERATION_STATUS: CLOSED
SECONDARY_SWAY_STATUS: CLOSED
ANIMATION_TIME_SOURCE_STATUS: RUNTIME_VALUE_REQUIRED
PRODUCTION_FIX_CANDIDATE_FOUND: NO
PRESENTATION_AUDIT_NOW_JUSTIFIED: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEW_CAPTURE_REQUIRED: YES (witness now implemented)
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NEXT_SINGLE_ACTION: bounded CPU witness at baanim_update / 0x8008ADE4 (done: p09-feather-anim-sample-time-v1)
```

## Prior (2026-09-08) — P09_VISIBLE_FEATHER_ALPHA_COVERAGE_TEXTURE_FORENSIC

Read [[p09-visible-feather-alpha-coverage-texture-forensic]].
Twelve plume DLs: local CI4 64x32 + pal16, `G_TT_RGBA16` +
`G_AC_THRESHOLD` during tris, then restore. Crest/head:
`G_TEXTURE on=0`, SHADE alpha, no AC/TT. ST static; no `G_MODIFYVTX`
ST; texgen off. Texture state is owned by the same GameCall.
Position, culling, draw-temporal, and depth stay closed. Do not
call GPU bug.

```text
ALPHA_COVERAGE_STATUS: CLOSED
TEXTURE_COORDINATE_STATUS: CLOSED
TEXTURE_RESOURCE_ASSOCIATION_STATUS: CLOSED
PRODUCTION_FIX_CANDIDATE_FOUND: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEW_CAPTURE_REQUIRED: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NEXT_SINGLE_ACTION: bounded static/command-stream audit of frame presentation/compositing/final-image association for the 14 visible feather DLs; no alpha/ST/texture witness; no GPU readback
```

## Prior (2026-09-07) — P09_POST_POSITION_DRAW_TEMPORAL_AND_DEPTH_STATE_FORENSIC

Read [[p09-post-position-draw-temporal-and-depth-state-forensic]].
V7 16-gen window: 0 generation/range/frame mismatches; each of the 14
DLs has one exclusive GameCall on `fb_pair=3`. Ring reuse cannot
precede GPU completion. Depth z bits are inherited, not set by the 14
DLs, and shared with `0x48F`/`0xBC`. Position and culling stay closed.
Do not call GPU bug.

```text
DRAW_TEMPORAL_ASSOCIATION_STATUS: CLOSED
DEPTH_OCCLUSION_STATE_STATUS: CLOSED
GENERATION_MISMATCH_COUNT: 0
PRODUCTION_FIX_CANDIDATE_FOUND: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEW_CAPTURE_REQUIRED: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NEXT_SINGLE_ACTION: bounded static/command-stream audit of color/alpha/coverage and texture-coordinate paths on the 14 visible feather DLs; no new position/cull/temporal witness; no GPU readback
```

## Prior (2026-09-07) — P09_VISIBLE_FEATHER_CULLING_WINDING_FACE_VISIBILITY_FORENSIC

Read [[p09-visible-feather-culling-winding-face-visibility-forensic]].
Every issued G_TRI on the 14 DLs reached V7 APPEND (1258 invocations).
Cull-both is the only pre-append reject and did not fire. The 14 DLs
preserve inherited cull bits; they do not set FRONT/BACK. `G_CULLDL` is
absent. Position path stays `CLOSED`. Do not call GPU bug.

```text
CULLING_VISIBILITY_STATUS: A
MISSING_TRIANGLES_OBSERVED: NO
CULL_MODE_SAME_ACROSS_ALL_14: YES
CURRENT_BEST_ROOT_CLASS: VISIBLE_FEATHER_POSITION_PIPELINE_CLOSED_ROOT_ELSEWHERE
PRODUCTION_FIX_CANDIDATE_FOUND: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEW_CAPTURE_REQUIRED: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NEXT_SINGLE_ACTION: bounded static/command-stream audit of depth/occlusion/draw temporal association on the 14 visible feather DLs; no new position witness; no GPU readback
```

## Prior (2026-09-07) — P09_BOUNDED_CREST_POST_HANDOFF_TRANSFERABILITY_AUDIT

Read [[p09-bounded-crest-post-handoff-transferability-audit]]. Crest
uses `viewProjIndex=4`, V=0, the same RSPProcess/RasterVS/W' path as
the plumes, and has no reachable `G_MODIFYVTX`. Join 0–4/27–31 is
erased after V3 global-vertex handoff. Position pipeline for all 14
visible feather DLs is closed. Do not call GPU bug.

```text
RIGHT_TO_CREST_Q_RESULT_TRANSFERABLE: YES
RIGHT_TO_CREST_RSPPROCESS_RESULT_TRANSFERABLE: YES
RIGHT_TO_CREST_RSPMODIFY_RESULT_TRANSFERABLE: YES
RIGHT_TO_CREST_RASTERVS_RESULT_TRANSFERABLE: YES
FULL_VISIBLE_FEATHER_POST_HANDOFF_POSITION_PATH_STATUS: CLOSED
CURRENT_BEST_ROOT_CLASS: VISIBLE_FEATHER_POSITION_PIPELINE_CLOSED_ROOT_ELSEWHERE
PRODUCTION_FIX_CANDIDATE_FOUND: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEW_CAPTURE_REQUIRED: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NEXT_SINGLE_ACTION: bounded static/command-stream audit of triangle culling/winding/face visibility on the 14 visible feather DLs; no new position witness; no GPU readback
```

## Prior (2026-09-07) — P09_FEATHER_HANDOFF_V7_FULL_VISIBLE_EXTENSION

Read [[p09-feather-v7-full-visible-extension]]. The diagnostic V7 witness is
extended from the closed right plume to all fourteen statically proven visible
feather DLs: right six, left six, and crest pair. Existing right-target index
and comparator semantics remain unchanged; schema VERSION=3 is required for
the expanded universe. No Q/RSPProcess extension or production fix is included.

```text
ALL_14_EXACT_TARGETS_STATICALLY_REACHABLE: YES
EXISTING_RIGHT_TARGET_SEMANTICS_UNCHANGED: YES
SCHEMA: P09_FEATHER_HANDOFF_V7
VERSION: 3
SELF_DIAGNOSING_STATUS_IMPLEMENTED: YES
USER_MANUAL_TEST_REQUIRED_NOW: YES
USER_TEST_COMMAND: rm -f /tmp/tooie-kazooie-feather-v7-cpu-handoff-status.txt /tmp/tooie-kazooie-feather-v7-cpu-handoff.csv && TOOIE_DIAG_FEATHER_HANDOFF_V7=1 ./build/TooieRecompiled
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NEXT_SINGLE_ACTION: run one bounded VERSION=3 capture and inspect all fourteen target masks and CPU handoff counters
```

## CURRENT (2026-09-07) — P09_VISUAL_GEOMETRY_IDENTITY_AND_SCOPE_FORENSIC

Read [[p09-visual-geometry-identity-and-scope-forensic]]. The six
right-plume DLs are only the right subtree. Left plume
`0x5C0/0x5D8/0x5F0/0x608/0x61E/0x636` and crest `0x4A6/0x4B0` already
draw in V6B. V7/Q'/RSPProcess closures do not cover them. GeoCmd parent
of the plumes is 107. No production fix. No new witness. No GPU
readback.

```text
VISUAL_TARGET_IDENTITY_STATUS: B
CURRENT_WITNESS_IS_INCOMPLETE: YES
ADDITIONAL_TARGET_DLS: 0x5C0,0x5D8,0x5F0,0x608,0x61E,0x636,0x4A6,0x4B0
CURRENT_BEST_ROOT_CLASS: POST_HANDOFF_RENDER_PATH_BUG_UNRESOLVED
PRODUCTION_FIX_CANDIDATE_FOUND: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NEXT_SINGLE_ACTION: extend V7 exact-admission/CPU-handoff to the eight additional DLs; no RSPProcess witness; no GPU readback
```

## Prior (2026-09-07) — P09_NARROW_RSPPROCESS_STRUCTURAL_FORENSIC

Read [[p09-narrow-rspprocess-structural-forensic]]. `RSPProcessCS` has
no remaining credible target-local mechanism after V7, RSPModify
absence, V=0, V5B world-lerp refute, and Q' shared/negligible. Do not
instrument RSPProcess further. Do not use Q-witness class C as
`ROOT_CLASS`. No production fix. No GPU readback.

```text
RSPPROCESS_STATUS: A
RSPPROCESS_STATICALLY_CLOSED: YES
TOP_RSPPROCESS_MECHANISM: NONE
CURRENT_BEST_ROOT_CLASS: POST_HANDOFF_RENDER_PATH_BUG_UNRESOLVED
PROJECTIONPROCESSOR_PRIMARY_CAUSE: REFUTED_FOR_CURRENT_WITNESS
PRODUCTION_FIX_CANDIDATE_FOUND: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NEXT_SINGLE_ACTION: test whether the visible wobble is these six plume DLs versus other 0x62B geometry; no RSPProcess witness; no GPU readback
```

## Prior (2026-09-07) — P09_FEATHER_POSTPROCESS_Q_WITNESS_V1

Read [[p09-feather-postprocess-q-witness-v1]]. CPU-only Q' witness is
implemented. V7 CPU equality stays closed. No production fix. No V8.
No GPU readback.

```text
DIAGNOSTIC_SCHEMA: P09_FEATHER_POSTPROCESS_Q_WITNESS_V1
CPU_HANDOFF_STATUS: CORRECT
PRODUCTION_FIX_CANDIDATE_FOUND: NO
USER_MANUAL_TEST_REQUIRED_NOW: YES
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NEXT_SINGLE_ACTION: user runs TOOIE_DIAG_FEATHER_POSTPROCESS_Q=1 ./build/TooieRecompiled and inspects PRELIMINARY_CLASS
```

## Prior (2026-09-07) — P09_POST_HANDOFF_STRUCTURAL_FORENSIC

Read [[p09-post-handoff-structural-forensic]]. V7 CPU equality is
closed. RSPModify is statically absent on the six right-plume DLs.
First remaining raster-position writer is `RSPProcessCS`. World lerp
remains refuted as primary. ProjectionProcessor Q' is the only new
post-handoff matrix reconstruction not covered by V5B; it is a weak
localized fit. No production fix. No V8 implemented.

```text
CPU_HANDOFF_STATUS: CORRECT
TARGET_RSP_MODIFY_COMMANDS_STATICALLY_POSSIBLE: NO
FIRST_POST_HANDOFF_POSITION_WRITER: RSPProcessCS
CURRENT_BEST_ROOT_CLASS: MORE_EVIDENCE_REQUIRED
PRODUCTION_FIX_CANDIDATE_FOUND: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NEXT_SINGLE_ACTION: implement one bounded CPU reconstruction of post-processor RSPProcess inputs versus V7 handoff
```

## Prior (2026-09-07) — P09_V7_HANDOFF_COMPARATOR_AND_JOIN_ROLE_REPAIR

Exact admission is closed. `byte_mismatch=3` is a comparator artifact
(stride 4 vs production stride 3). Join expected roles are child 0–3 /
parent 28–31 for `0x54C` (105→104), `0x564` (104→103), `0x592`
(103→parent of `+0x124A0`). Schema VERSION=2. Causal payload/render
paths unchanged.

```text
HANDOFF_BYTE_MISMATCH_CLASS: COMPARATOR_ARTIFACT
JOIN_592_PROVENANCE_STATUS: CORRECT
JOIN_54C_PROVENANCE_STATUS: CORRECT
JOIN_564_PROVENANCE_STATUS: CORRECT
USER_MANUAL_TEST_REQUIRED_NOW: YES
USER_TEST_COMMAND: TOOIE_DIAG_FEATHER_HANDOFF_V7=1 ./build/TooieRecompiled
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NEXT_SINGLE_ACTION: user runs one cleaned VERSION=2 V7 capture and inspects APPEND_POSITION_MATCH plus JOIN_*_ROLE_RESOLVED
```

## Prior (2026-09-07) — P09_V7_EXACT_TARGET_ADMISSION_REPAIR

Read [[p09-feather-v7-cpu-handoff-implementation]] for the exact-admission
repair. The last capture published live `0x62B` context (`0x229070`) but
validated 0 targets because the low-page prefilter ignored
`physical(model_bin)&0xFFF`. Admission is now
`SINGLE_EXACT_CONTEXT_DIRECT_MATCH` (V6C full-address equality; no `0xFFF`
gate). One unique published bin; 0 clears. Causal V7 payload/render paths
unchanged.

```text
V7_EXACT_TARGET_ADMISSION_REPAIRED: YES
JOIN_UNRESOLVED_IS_BLOCKER: YES
USER_MANUAL_TEST_REQUIRED_NOW: YES
USER_TEST_COMMAND: TOOIE_DIAG_FEATHER_HANDOFF_V7=1 ./build/TooieRecompiled
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NEXT_SINGLE_ACTION: user runs one bounded V7 capture and inspects APPEND_EXACT_TARGET_MATCH
```

## Prior (2026-09-05) — P09_V7_CPU_HANDOFF_IMPLEMENTATION

Read [[p09-feather-v7-cpu-handoff-implementation]] for the implementation
checkpoint and validation record. `P09_FEATHER_HANDOFF_V7 / VERSION=1` is
implemented default-off in RT64 owning source. Workload generation, post-append
records, fullSync face-slice/source checks, and render-selection records are
bounded to the six right-plume DLs with independent per-target quotas.

```text
V7_IMPLEMENTED: YES
JOIN_UNRESOLVED_IS_BLOCKER: YES
CAPTURE_COMPLETE_FOR_CAUSAL_DECISION: NO (until a capture observes all required join roles)
USER_MANUAL_TEST_REQUIRED_NOW: YES
USER_TEST_COMMAND: TOOIE_DIAG_FEATHER_HANDOFF_V7=1 ./build/TooieRecompiled
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NEXT_SINGLE_ACTION: user runs one bounded V7 capture and inspects CSV/status
```

Build and both requested test targets pass; Tooie and RT64 `git diff --check`
pass. No game was run, and no production behavior changed.

The requested fresh-capture analysis is recorded in
[[p09-v7-cpu-handoff-capture-analysis]]. Both V7 output files were absent, so
the older V6/V6C artifacts were excluded and no causal root was selected. The
follow-up lifecycle repair now creates the status heartbeat at enable and adds
raw hook counters; the next capture must use the cleaned direct-binary command
in that evidence note.

## Prior (2026-09-05) — P09_POST_TRIANGLE_ARCHITECTURAL_REVIEW

Read [[p09-post-triangle-architectural-review]] for all required review fields,
source anchors, hypothesis ranking and the bounded V7 implementation contract.
This review ran no game and changed documentation only. No commit/push.

- V6C aggregate own provenance accepted: 14,880 own matches, zero stale.
- Root join `0x592`: 468 expected, 3,252 unresolved / 3,720. Distal joins
  `0x54C/0x564` are not included by V6C's JOIN filter.
- CSV has 240 samples; zero unresolved target joins and no global vertex IDs
  or final upload payload. `IDX_*` is source-local; all `WORKLOAD=0`.
- Current V6C hook is before real face append. Global CPU correctness and
  exclusive post-submission root are **CORRECTED**, not established.
- `JOIN_UNRESOLVED_IS_BLOCKER=YES` for exclusive localization;
  `JOIN_CAN_EXPLAIN_PRIMARY_VISUAL_ARTIFACT=UNKNOWN`.
- Normal raster path has no world-index compaction/rebase. `RSPProcessCS`
  directly selects world matrices; `RSPWorldCS` is a separate world/motion path.
- The matrix process/upload predicate mismatch is statically real but weakened
  by the current one-workload-per-frame caller. No causal renderer bug found.
- Preserve V5B matrix/SIMPLE exclusions and zero-triangle `0x03000040` refutation.

```text
ROOT_CLASS_AFTER_REVIEW: UNKNOWN / INCOMPLETE_JOIN_AND_CPU_HANDOFF_EVIDENCE
ROOT_CONFIDENCE: LOW
V7_IMPLEMENTED: NO at review time; superseded by the implementation section above
GPU_READBACK_USED: NO
NEXT_IMPLEMENTATION_MODEL: GPT-5.6 Luna / Grok 4.6 Medium
NEXT_SINGLE_ACTION: superseded — see current implementation section above
```

Retain both dirty worktrees and the dirty Plume submodule. V7 needs a
Workload-owned diagnostic generation, post-append baseline, fullSync slice/upload
comparison and referenced-matrix upload-selection records. Do not use an old
capture's frame number or a zero workload ID as an end-to-end join. A correct
CPU upload source alone cannot force a shader/backend root.

## Prior (2026-09-04) — P09_REPAIR_V6_TRIANGLE_WITNESS_AFTER_ZERO_TRI

V5B full-chain no-interpolation remains `SAME` and closed. A real V6
triangle capture then recorded **zero** TRI rows despite V5B G_VTX
coverage `0x7F`. F3DEX2 triangles do call `RSP::drawIndexedTri`. V6's
`FeatherWorldHit` key used matrix/match workload; the G_MTX token is
cleared at G_VTX; `FEATHER_TRI_ROWS` was post-filter. V6B repairs
observability only.

```text
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V6B
TEMPORAL_MATRIX_INTERPOLATION_CAUSAL_STATUS: REFUTED_FOR_VISIBLE_FEATHER_CHAIN
SIMPLE_INTERPOLATION_CAUSAL_STATUS: REFUTED_AS_PRIMARY_FIX
V6_HOOK_IS_ON_ACTUAL_RUNTIME_PATH: YES
V6_DESIGN_BUG: WORLDHIT_KEYED_BY_MATRIX_WORKLOAD_NOT_DRAW_WORKLOAD_AND_TOKEN_CLEARED_BEFORE_TRIANGLE
V6B_IMPLEMENTED: YES
USER_MANUAL_TEST_REQUIRED_NOW: YES
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
NEXT_SINGLE_ACTION: user runs one V6B capture TOOIE_DIAG_SECONDARY_SWAY=1; inspect RAW_TRI_HANDLER_COUNT / RAW_TRI1_COUNT / RAW_TRI2_COUNT then TRI_CLASSIFIED_COUNT and /tmp/tooie-kazooie-feather-v6b-tri.csv
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
```

Evidence: [[p09-kazooie-feather-v6b-triangle-witness-repair]].

## Prior (2026-09-04) — P09_KAZOOIE_FEATHER_V5B_DIAGNOSTIC_GATE_REPAIR

The prior V5 no-interpolation capture was incomplete: only boneAnimID 103
reached the token/match/post-match path because
`secondary_sway_note_dcf48()` rejected `Sibling62B`. This repair changes only
the diagnostic gate. A targeted sibling is accepted only for live resource
`0x62B`, one of the proven IDs `79, 100, 101, 102, 103, 104, 105`, and the
existing proven type-2 node/index mapping. Unrelated `0x62B` siblings remain
rejected.

The resource ID is now carried into each V4 token. The existing modelview
parallel token stack, per-depth token lifetime, child `G_MTX` behavior,
`G_POPMTX` restore, bounded type-0x18 restore alias, and full-width GPR
preservation are unchanged. Matrix, G_MTX, vertex, CALL, matchTransform,
post-match, and no-interpolation evidence is counted per tracked bone. The
status file emits one `BONE<anim>_STATUS` line and one bitmask per seam; all
seven tracked bones correspond to mask `0x7F`.

```text
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V5B
SIBLING62B_TARGETED_ACCEPT_IMPLEMENTED: YES
UNRELATED_62B_SIBLINGS_STILL_REJECTED: YES
FULL_CHAIN_OWN_TOKEN_SUPPORT: YES
FULL_GPR_POINTER_PRESERVATION_RETAINED: YES
MODELVIEW_STACK_TOKEN_LOGIC_RETAINED: YES
FULL_CHAIN_NO_INTERP_OVERRIDE_SUPPORTED: YES
IDENTITY_MATCHING_UNCHANGED: YES
GUEST_MATRICES_UNCHANGED: YES
FULL_CHAIN_COVERAGE_STATUS_IMPLEMENTED: YES
CAPTURE_CAN_SELF_DECLARE_INCOMPLETE: YES
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
DIFF_CHECK: PASS
USER_MANUAL_TEST_REQUIRED_NOW: YES
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
NEXT_SINGLE_ACTION: user runs exactly one V5B no-interpolation capture; inspect masks before interpreting its visual result
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
```

No game was run during this repair.

## CURRENT (2026-09-04) — P09_KAZOOIE_FEATHER_GMTX_TO_DL_V4

Banjo snout stays closed as the clean control. Guest matrix/rotation are
valid; do not reopen AE8A8/AE820 producer RE. Static reconstruction of
`0x62B+0x124A0` proves the visible DLs and why V3 never bound VERTEX:
nested bone 104/105 `G_MTX` expired a scalar token before any `G_VTX`.
V4 stacks the dest-keyed token by modelview depth and aliases type-0x18
same-depth restore `G_MTX`. The fresh V4 capture reaches real RT64 matching
and shows no semantic previous-frame failure. The seven unmatched decisions
are five `CURRENT_ALREADY_MAPPED` and two `PREVIOUS_ALREADY_USED`; no one is
proven causal for the visible `0x5AA` because V4 has no final DL/triangle
association. No production fix.

```text
TOOIE_HEAD: dirty worktree; no commit
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V4
PRIMARY_WITNESS: KAZOOIE_CARRY_BANJO_HEAD_FEATHER
PRIMARY_FEATHER: 0x62B+0x124A0 idx30 anim103
PRIMARY_FEATHER_VISIBLE_GEOCMD: 0x12580 dl=0x5AA and 0x12550 dl=0x592
DOES_PRIMARY_DL_CONTAIN_VERTEX_DIRECTLY: YES
FEATHER_COMMAND_ORDER: MIXED
WHY_V3_SET_VERTEX_WITNESS_SAW_ZERO_ROWS: TOKEN_EXPIRES_BEFORE_CHILD_DL
CORRECT_TOKEN_BINDING_SEAM: setVertexCommon (stacked token + restore alias)
FEATHER_V4_IMPLEMENTED: YES
V4_EXPECTED_TO_REACH_GAMEFRAME_MATCH: PROVEN (133 bound tokens)
V4_EXPECTED_TO_REACH_MATCHTRANSFORM: PROVEN (126 matched)
GUEST_MATRIX_VALID: YES
GUEST_ROTATION_VALID: YES
FIRST_BAD_GUEST_STAGE: NONE_OBSERVED
P09_KNOWLEDGE_PROMOTED_TO_RE_DOCS: YES
FEATHER_RT64_MATCH_STATUS: AUTO_IDENTITY_CORRECT_OBSERVED (126 matched / 7 allocator drops)
V4_DROP_AUDIT: NO_CLEARLY_CAUSAL_DROP; 5 CURRENT_ALREADY_MAPPED, 2 PREVIOUS_ALREADY_USED
MISSING_GROUP_CAUSAL_STATUS: DIAGNOSTIC_A_B_READY (not a production conclusion)
BANJO_SNOUT_ROOT: NO_P09_CAUSAL_FAILURE_OBSERVED
PRODUCTION_FIX: NO
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: HIGH observability / LOW graphical root
AB_SWITCH_IMPLEMENTED: YES (default-off; primary 0x62B/+0x124A0 bone30 anim103)
AB_IDENTITY_MATCHING_UNCHANGED: YES
INTERP_MODE_COUNTERS: IMPLEMENTED
BUILD_RESULT: PASS (TooieRecompiled)
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
DIFF_CHECK: PASS
NEXT_SINGLE_ACTION: user runs one B capture with SIMPLE override and reports visual classification only
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
USER_MANUAL_TEST_REQUIRED_NOW: YES (one B run; no control rerun)
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

## Current full-chain feather witness (2026-09-04)

The previous `TOOIE_DIAG_FEATHER_INTERP_SIMPLE=1` experiment was root-only
(bone 103) and the user observed `SAME`. V5 now retains the V4 identity
witness for all seven proven live `0x62B` feather IDs `79,100–105`, records
post-match matrices and `prevFrameWeight`/`curFrameWeight`, and implements the
default-off full-chain A/B `TOOIE_DIAG_FEATHER_NO_INTERP=1`.

```text
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V5
FULL_CHAIN_V4_TOKEN_SUPPORT: YES
POST_MATCH_MATRIX_WITNESS_IMPLEMENTED: YES
FULL_CHAIN_NO_INTERP_AB_IMPLEMENTED: YES
IDENTITY_MATCHING_UNCHANGED: YES
GUEST_MATRICES_UNCHANGED: YES
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
DIFF_CHECK: PASS
NEXT_SINGLE_ACTION: one no-interpolation capture with the same locomotion
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

Evidence: [[../../reverse-engineering/systems/transform-interpolation]],
[[p09-kazooie-feather-v5-full-chain]].

Evidence: [[p09-kazooie-feather-gmtx-to-dl-trace]],
[[p09-kazooie-feather-v4-ab-static-audit]],
[[p09-kazooie-feather-v3-capture-analysis]],
[[p09-kazooie-feather-v2-producer-witness]],
[[p09-kazooie-feather-v1-capture]],
[[p09-kazooie-carry-banjo-head-feathers]].

## Prior (2026-09-03) — P09_IMPLEMENT_REAL_RT64_MATCH_WITNESS_V4

Preserve uncommitted P09 diagnostics. V3 proves the guest matrix and G_MTX
operand path. V4 now observes real RT64 call pairing, transform candidates,
one-to-one selection, and the actual `matchTransform` result. No production
fix is justified while the root remains low confidence.
The first V4 capture corrected the witness diagnosis: `MATRIX` events carry
real workload IDs, while `VERTEX` events carry workload `0`; the vertex event
overwrites token identity before `matchScene`, so zero CALL/candidate rows do
not prove an RT64 rejection.

```text
TOOIE_HEAD: ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf (worktree dirty; no commit)
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_SINGLE_MTX_IDENTITY_V4
FUNC_80019AA0_CURSOR: D_8007BCB0
G_MTX_OPERAND: gfx_ctx+4
RT64_MATCH_WITNESS: REAL_MATCHTRANSFORM
RT64_CALL_MATCH_WITNESS: ENABLED
FLUSH_SENTINEL_ROWS: SECONDARY_ONLY
MISSING_GROUP_CAUSAL_STATUS: HYPOTHESIS
ROOT_CLASS: RT64_MATCH_WITNESS_UNRESOLVED
ROOT_CONFIDENCE: LOW
V4_CAPTURE_ROOT: DIAGNOSTIC_ARGUMENT_MISMATCH_MATRIX_VS_VERTEX_WORKLOAD
V4_CAPTURE_CONFIDENCE: HIGH (witness failure only)
NEXT_SINGLE_ACTION: preserve matrix workload identity across the vertex callback, rebuild, then capture once
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
USER_MANUAL_TEST_REQUIRED_NOW: NO
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

Evidence: [[p09-v4-real-match-witness]], [[p09-v3-real-guest-matrix-match]],
[[p09-19aa0-gmtx-rt64-match-stage]], [[p09-v2-snout-runtime-correlation]].

### Prior — P09_PROVE_LIVE_0X62B_SNOUT_DCF48_AND_ADE2C

Preserve uncommitted backpack diagnostics. Live type-2 slot is DCF48.
The sibling-aware capture proves snout `+0x11CB8` reaches DCF48 and ADE2C;
the current-slot matrix witness is present, but previous-matrix and RT64
identity data are not. See [[p09-live-0x62b-tree-dcf48-capture]].

```text
TOOIE_HEAD: ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf (worktree dirty; no commit)
LIVE_BANJO_BODY_RESOURCE: 0x62B
X62B_SNOUT_NODE_OFFSET: 0x11CB8
TRUE_0X62B_ASSET_SIZE: 0x13178
E2AA4_PRIMARY_SNOUT_WITNESS: DEMOTED
X62B_SNOUT_USES_E2AA4: NO
FILTER_IDENTITY: DE498 0x62B token; sibling scan + DCF48 a1 == bin+0x11CB8
FILTER_CONFIDENCE: HIGH
NEW_SNOUT_WITNESS_IMPLEMENTED: YES
CANDIDATE_IMPLEMENTED: NO
ROOT_CLASS: SINGLE_G_MTX_IDENTITY_UNRESOLVED
ROOT_CONFIDENCE: LOW
NEXT_SINGLE_ACTION: trace DCF48 func_80018C28/mlMtxApply into the RT64 single-G_MTX identity seam
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
USER_MANUAL_TEST_REQUIRED_NOW: NO
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

Evidence: [[p09-live-0x62b-tree-dcf48-capture]],
[[p09-live-0x62b-tree-reachability]].

### Prior — P09_FIX_MODEL_BOUNDARIES_AND_MOVE_SNOUT_WITNESS_TO_GEOCMD02_E2AA4_SCOPE

Preserve uncommitted backpack diagnostics. `0x20000` as 0x62B ownership is
`CORRECTED`. Generic E2180 snout discovery is `DEMOTED`. See
[[p09-e2aa4-snout-witness]].

```text
TOOIE_HEAD: ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf (worktree dirty; no commit)
LIVE_BANJO_BODY_RESOURCE: 0x62B
X62B_SNOUT_NODE_OFFSET: 0x11CB8
TRUE_0X62B_ASSET_SIZE: 0x13178
OLD_0X14960_ASSUMPTION: RETIRED
OLD_ALLOCATION_SPAN_0X20000: RETIRED
GENERIC_E2180_DISCOVERY_FILTER: DEMOTED
FILTER_IDENTITY: E2AA4 a1 == registered 0x62B bin + 0x11CB8 inside [bin, bin+0x13178) type2 idx11 bone42
FILTER_CONFIDENCE: HIGH
NEW_SNOUT_WITNESS_IMPLEMENTED: YES
CANDIDATE_IMPLEMENTED: NO
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
NEXT_SINGLE_ACTION: one TOOIE_DIAG_SECONDARY_SWAY=1 capture; analyze E2AA4 rejects + e2aa4 tuples
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

### Prior — P09_PROVE_LIVE_0X62B_SNOUT_NODE_LAYOUT_AND_E2180_ARGUMENT_SEMANTICS


Preserve uncommitted backpack diagnostics. `0x14960` as the live `0x62B`
snout offset is `CORRECTED`: it is `0x62A` only. See
[[p09-62b-snout-e2180-layout]].

```text
TOOIE_HEAD: ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf (worktree dirty; no commit)
LIVE_BANJO_BODY_RESOURCE: 0x62B
X62B_SNOUT_NODE_OFFSET: 0x11CB8
OLD_0X14960_ASSUMPTION: RETIRED
OLD_R31_FILTER: RETIRED
OLD_80092444_TLS_FOR_E2180: RETIRED
FILTER_IDENTITY: durable 0x62B model_bin + (rel 0x11CB8 OR type2+bone42 OR type2+idx11)
FILTER_CONFIDENCE: HIGH static node / MEDIUM runtime a1 still that node
NEW_SNOUT_WITNESS_IMPLEMENTED: YES
CANDIDATE_IMPLEMENTED: NO
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
NEXT_SINGLE_ACTION: one TOOIE_DIAG_SECONDARY_SWAY=1 capture; analyze split rejects + e2180 tuples
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

### Prior — P09_RECONCILE_DEFERRED_MODEL_RENDER_CONTEXT_AND_LIVE_SECONDARY_ANIMATION_PRODUCER

Preserve uncommitted backpack diagnostics. `0x56` as body model remains
`CORRECTED/SUPERSEDED`. The r31 whitelist is `RETIRED`. `80092444` TLS is
`RETIRED` for E2180. See [[p09-deferred-render-context-e0b8c]].

```text
TOOIE_HEAD: ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf (worktree dirty; no commit)
LIVE_BANJO_BODY_RESOURCE: 0x62B
OLD_R31_FILTER: RETIRED
OLD_80092444_TLS_FOR_E2180: RETIRED
FILTER_IDENTITY: durable 0x62B model_bin + GeoCmd02 + boneAnimID 42
FILTER_CONFIDENCE: HIGH
NEW_PERSISTENT_MODEL_WITNESS_IMPLEMENTED: YES
CANDIDATE_IMPLEMENTED: NO
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
NEXT_SINGLE_ACTION: reverse-engineer the live E2180 argument/node layout; split WRONG_NODE predicates
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

## CURRENT (2026-09-03) — P09_V5_REAL_CALL_MATCH_RESULT

The V4 capture proved a diagnostic argument mismatch, not an RT64 matching
failure. `matrixCommon` receives `state->workloadId` while
`setVertexCommon` reports `workload.workloadId == 0` at the correlated seam.
V5 preserves the matrix workload as the temporal-match identity, stores vertex
workload context separately, and separates synthetic finalization from real
match decisions.

```text
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_SINGLE_MTX_IDENTITY_V5
WORKLOAD_BINDING: MATRIX_WORKLOAD_PRESERVED
TOKEN_WORKLOAD_PRECEDENCE: MATRIX_NONZERO > VERTEX_ZERO
REAL_UNMATCHED_REQUIRES_REAL_MATCH_DECISION: YES
REAL_MATCHTRANSFORM_WITNESS_RETAINED: YES
PRODUCTION_FIX: NO
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
GIT_DIFF_CHECK: PASS
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION: identify the live Kazooie feather model/bone/transform-owner/G_MTX and reuse the generic V5 match witness
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION: NO
NO_COMMIT: YES
NO_PUSH: YES
```

The fresh V5 capture is usable and postdates the current executable. It
reports 2,379 non-zero matrix workloads preserved over 2,379 zero vertex
workloads, 34 real `matchTransform` matches, and 5 real unmatched decisions.
The latter are all one-to-one previous-transform conflicts; 2,340
`SYNTHETIC_FINALIZATION_ONLY` rows are diagnostic cleanup. All 5 call rows
matched, all 5 transform prerequisites were eligible, and all 77 candidates
were valid. No RT64 AUTO identity or interpolation failure is proven.

The two strict diagnostic-context mismatches are explained by RT64's
`hashFromCall` call pairing and transform-pair keyed observation map; differing
call/range metadata is not sufficient evidence of a wrong previous transform.
The snout path remains guest-valid, but no production fix is authorized. The
next investigation target is Kazooie's feathers while carrying Banjo.

## Fresh persistent-filter capture (2026-09-02)

The current executable was built at `2026-09-02T08:47:18.309Z`; the capture
artifacts were written at `2026-09-02T14:29:09.301Z`, so this is a current
runtime result. It is usable for filter-seam reachability, but not as a
transform witness. Full evidence: [[p09-persistent-snout-filter-zero]].

```text
DE448_0X62B_REGISTRATION_COUNT: 6054
REGISTERED_0X62B_BIN_COUNT: 1 (0x8020E340; derived from model.csv)
E2180_RAW_ENTRY_COUNT: 1614
SNOUT_FILTER_ACCEPT_COUNT: 0
SNOUT_FILTER_REJECTION_BREAKDOWN: NO_DURABLE_MODEL=496, WRONG_NODE=1118, NO_GETTER=0, WRONG_RESOURCE=0
SNOUT_CSV_ROWS: 0 (missing)
BONES_CSV_ROWS: 0 (missing)
HIP_CSV_ROWS: 0 (missing)
E2180_RUNTIME_BINDING_WORKING: YES
PERSISTENT_MODEL_REGISTRATION_WORKING: YES
SNOUT_NODE_FILTER_WORKING: UNKNOWN (WRONG_NODE is not predicate-specific)
E2180_IN_REGISTERED_0X62B_SPAN_COUNT: UNKNOWN
E2180_TYPE2_COUNT: UNKNOWN
E2180_BONE42_COUNT: UNKNOWN
GUEST_ANIMATION_STATE_VALID: UNKNOWN
SNOUT_CURRENT_GUEST_MATRIX_VALID: UNKNOWN
SNOUT_PREVIOUS_MATRIX_VALID: UNKNOWN
PREVIOUS_CURRENT_LOGICAL_PAIRING: UNKNOWN
FIRST_BAD_LAYER: UNKNOWN
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
CANDIDATE_IMPLEMENTED: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO (do not repeat identical capture)
NEXT_SINGLE_ACTION: targeted E2180 argument/node-layout reverse engineering
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
GPU_READBACK_USED: NO
```

The `WRONG_NODE` bucket is emitted after the durable-list check and combines
`type != 2`, `boneAnimID != 42`, and registered-span failure. No exact node or
matrix claim may be promoted from this artifact. The zero E0B8C count remains
non-diagnostic because bawobble's record writer is conditional.

## Repaired runtime-map witness (2026-09-02)

The new capture was inspected directly after the E0B8C/E2180
`add_loaded_function` bindings were compiled. Player identity remains proven,
but the transform witness is not captured. E2180 entered 3,491 times and all
3,491 calls were rejected at `NO_SCOPE`; E0B8C entered zero times. This is a
hard stop against another identical manual run.

```text
PLAYER_MODEL_ROWS: 2126
PLAYER_MODEL_0x62A: 142 rows, frames 495–636, model_bin 0x801D5010
PLAYER_MODEL_0x62B: 1984 rows, frames 637–2620, model_bin 0x80224B40
OUTER_PLAYER_SCOPE_HITS: 2126
DE448_IN_PLAYER_SCOPE_HITS: 2126
E0B8C_RAW_ENTRY_COUNT: 0
E0B8C_FILTER_ACCEPT_COUNT: 0
E0B8C_REJECT_COUNT: 0 (no entry)
E2180_RAW_ENTRY_COUNT: 3491
E2180_FILTER_ACCEPT_COUNT: 0
E2180_REJECT_COUNT: 3491
E2180_REJECTION_BREAKDOWN: NO_SCOPE=3491, NO_GETTER=0, WRONG_RESOURCE=0, WRONG_NODE=0
BONES_CSV_ROWS: 0 (missing)
SNOUT_CSV_ROWS: 0 (missing)
CAPTURE_USABLE: NO (transform witness; player identity usable)
E0B8C_RUNTIME_BINDING_WORKING: NO (not demonstrated)
E2180_RUNTIME_BINDING_WORKING: YES
FILTERS_WORKING: PARTIAL
HIP3_GUEST_STATE_VALID: UNKNOWN
SNOUT_CURRENT_GUEST_MATRIX_VALID: UNKNOWN
PREVIOUS_CURRENT_LOGICAL_PAIRING: UNKNOWN
FIRST_BAD_LAYER: UNKNOWN
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
CANDIDATE_IMPLEMENTED: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION: reconcile actual E0B8C runtime dispatch/call path
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

The asymmetry is meaningful: E2180’s raw entry proves the runtime-map route
works for that target. E0B8C has no observed entry, but the capture alone does
not distinguish “not on this live walk path” from a target-specific mapping
problem. Static player rendering still shows `GeoCmd02 → E2AA4 → E2180`; no
E0B8C call is implied by that render path. Do not classify a guest-state or
interpolation root from this capture.

## Authoritative snout witness attempt (2026-09-02)

The next runtime capture was inspected directly. It proved the repaired
player-model identity again, but the submitted binary predated the later
runtime-map hook repair and emitted neither transform CSV:

```text
PLAYER_MODEL_ROWS: 1934
PLAYER_MODEL_RESOURCE_0x62A: 135 rows, frames 503–637, model_bin 0x801D5020
PLAYER_MODEL_RESOURCE_0x62B: 1799 rows, frames 638–2436, model_bin 0x80224AA0
OUTER_PLAYER_SCOPE_HITS: 1934
DE448_IN_PLAYER_SCOPE_HITS: 1934
DE448_CALLSITE_800926D8_HITS: 1934
PLAYER_MODEL_SAMPLE_WRITES: 1934
BONES_CSV_ROWS: 0 (missing)
SNOUT_CSV_ROWS: 0 (missing)
BONE_ANIM_HITS: 0
SNOUT_MATRIX_HITS: 0
SNOUT_TRANSFORM_WITNESS_CAPTURE_USABLE: NO
```

The zero accepted counters do not identify the failing stage: the old
instrumentation counted only after the E0B8C bone-ID 3/42 filter and the E2180
player/resource/node filters. Static generated code confirms the calls are
address-taken `LOOKUP_FUNC` calls to core2 targets. The linker `--wrap` entries
were therefore insufficient as an authoritative runtime-map binding, even
though the repaired `func_80092444` player scope was live.

After this analysis, the diagnostic was repaired without changing production
behavior. `register_boot_lookup_stubs()` now explicitly binds the E0B8C and
E2180 wrappers through `add_loaded_function`; status also records raw wrapper
entry counts and each snout rejection stage. The next run is a new
instrumentation capture, not a repeat of the identical binary.

```text
HIP3_SAMPLE_COUNT: 0
SNOUT_E2180_SAMPLE_COUNT: 0
HIP3_GUEST_STATE_VALID: UNKNOWN
SNOUT42_CURRENT_GUEST_TRANSFORM_VALID: UNKNOWN
PREVIOUS_CURRENT_LOGICAL_PAIRING: UNKNOWN
PREVIOUS_EQUALS_PRIOR_FRAME_CURRENT: UNKNOWN
MATRIX_GROUP_PRESENT: NO (static Tooie GeoCmd2 operation absent; live witness not reached)
WRONG_PREVIOUS_MATRIX_ASSOCIATION_COUNT: 0 observed / not testable
FIRST_BAD_LAYER: UNKNOWN
BANJO_GEOCMD2_ANALOG: func_803387F8
BANJO_SET_MODEL_MATRIX_GROUP_CALL: set_model_matrix_group -> gEXMatrixGroup (or skip/no-interpolate variant)
BANJO_GROUP_ID_SOURCE: cur_drawn_model_transform_id + cur_model_transform_id_offset for non-map bone nodes
BANJO_GROUP_SCOPE: one GeoCmd2 bone matrix push plus its child traversal
BANJO_PREVIOUS_CURRENT_SEMANTICS: renderer matrix-group identity across frames; not directly captured here
TOOIE_EQUIVALENT_NODE_CLASS: GeoCmd02 via func_800E2AA4 -> func_800E2180
TOOIE_MISSING_OPERATION: explicit per-node gEX matrix-group/tag identity
STRUCTURAL_EQUIVALENCE_CONFIDENCE: MEDIUM
WOULD_BANJO_TAGGING_DISTINGUISH_THE_CAPTURED_SNOUT_MATRIX_IDENTITY: UNKNOWN (no snout matrix row)
BANJO_BACKPACK_USES_SAME_DUAL_MTX_CLASS: YES (static class; live backpack instance unproven)
MUMBO_FEATHERS_USE_SAME_DUAL_MTX_CLASS: YES (static 0x665 GeoCmd02; live feather witness unrun)
KAZOOIE_FEATHERS_USE_SAME_DUAL_MTX_CLASS: UNKNOWN
SHARED_FOUR_CASE_ROOT: UNKNOWN
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
CANDIDATE_IMPLEMENTED: NO
USER_MANUAL_TEST_REQUIRED_NOW: YES (new diagnostic binary)
NEXT_SINGLE_ACTION: run one fresh secondary-sway capture with the repaired runtime-map hooks
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

## SUPERSEDED filter (2026-09-02) — r31 whitelist capture

Instrumentation repaired in [[p09-player-model-witness-r31-repair]]. Do not
re-enable the r31 whitelist. Historical failure:

The capture from `TOOIE_DIAG_SECONDARY_SWAY=1` while walking Banjo+Kazooie was
present but unusable:

```text
DE448_TOTAL_SEEN=31020
DE448_FILTERED_HITS=0
DE448_REJECTED_RA=31020
FIRST_FILTERED=NO
CSV_DATA_ROWS=0
```

The status and header-only CSV are the complete artifact; there are no live
resource IDs, model-bin pointers, walking rows, snout rows, or transform
samples to analyze. The hard-coded filter compares `ctx->r31` against
`0x808172A0`, `0x808172F4`, and `0x8081A4D4`, but the rejected branch records
only a count, so the exact runtime rejected values are unavailable.

Static source establishes the relevant player-render chain as
`func_800E9F20 -> func_800F5008 -> func_80098BA8 -> func_80092444`, with the
DE448 call at `0x800926D8` (`0x800926E0` would be its ordinary MIPS return
address). Generated `LOOKUP_FUNC` calls do not visibly write `ctx->r31`, so
that value cannot be assumed to be the direct DE448 callsite until dispatcher
semantics are proven. The three filter constants therefore do not establish a
live player identity and must not be replaced blindly.

```text
CAPTURE_USABLE: NO
LIVE_BANJO_BODY_RESOURCE: UNKNOWN
LIVE_BANJO_MODEL_BIN: UNKNOWN
BK_WALK_EXPECTED_0x61F: UNKNOWN
BANJO_SNOUT_RENDER_CLASS: UNKNOWN
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
NEXT_SINGLE_ACTION: SUPERSEDED — r31 filter retired; see CURRENT
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

## Repaired player model witness and snout class (2026-09-02)

The repaired `TOOIE_DIAG_SECONDARY_SWAY=1` run is usable. It recorded 2,067
accepted player samples at `func_80092444` / guest callsite `0x800926D8`.
Scope depth peaked at 1; overflow, wrong-callsite, and other rejects were
zero. There were 27,686 DE448 calls total, of which 25,619 were outside the
player scope and 2,067 were in scope.

Accepted resources were `0x62A` (`0x801BEBE0`, 68 bones, 124 rows, frames
486–609) and `0x62B` (`0x8020E340`, 66 bones, 1,943 rows, frames 610–2552).
The sustained normal-walk body is `0x62B`; this is HIGH-confidence live
identity evidence. The `0x61F` static walk expectation is REFUTED for this
capture. The earlier `0x62A` phase is retained as a real player-model phase,
not reclassified by frequency alone.

For live `0x62B`, the snout is `GEOCMD_02` and uses the `E2AA4 → E2180`
dual-matrix path. The static snout node is at model-relative offset `0x14960`
and bone 42 is reached indirectly from bawobble's hip bone 3 through parents
3 → 1 → 18 → 42. ExtraSegments is therefore not the direct normal-walk
snout path. BanjoRecomp's `func_802E6BD0` plus CPU skinning and transform-ID
grouping remains a structural analog only; it does not establish a Tooie
root.

Because the next filter is now HIGH confidence, a default-off witness was
added without changing production behavior. The E0B8C wrapper records only
bone IDs 3 and 42; the E2180 wrapper records only player-scoped `0x62B` node
`0x14960`, including guest previous/current matrices. Runtime interpolation
identity and transform tags remain UNKNOWN until this new witness is run.

```text
CAPTURE_USABLE: YES
OUTER_PLAYER_SCOPE_HITS: 2067
FUNC_80092444_IN_PLAYER_SCOPE_HITS: 2067
DE448_IN_PLAYER_SCOPE_HITS: 2067
DE448_CALLSITE_800926D8_HITS: 2067
PLAYER_MODEL_SAMPLE_WRITES: 2067
LIVE_BANJO_BODY_RESOURCE: 0x62B
LIVE_BANJO_MODEL_BIN: 0x8020E340
LIVE_MODEL_CONFIDENCE: HIGH
BK_WALK_EXPECTED_0X61F: REFUTED_FOR_THIS_CAPTURE
BANJO_SNOUT_RENDER_CLASS: GEOCMD_02
BANJO_SNOUT_USES_E2AA4_E2180: YES
EXTRASEGMENTS_BONE_TRANSFORM_FUNCTION: NOT_APPLICABLE_TO_LIVE_0x62B
EXTRASEGMENTS_CURRENT_MATRIX_SOURCE: UNKNOWN
EXTRASEGMENTS_PREVIOUS_MATRIX_SOURCE: UNKNOWN
EXTRASEGMENTS_INTERPOLATION_IDENTITY: UNKNOWN
BANJORECOMP_EXTRASEGMENTS_ANALOG: func_802E6BD0 + recomp_apply_cpu_skinning + transform-ID matrix group
TOOIE_EXTRA_DIFFERENCE: live normal-walk body is GeoCmd-02, not ExtraSegments
MOST_LIKELY_FIRST_BAD_LAYER: UNKNOWN_PENDING_TRANSFORM_WITNESS
CURRENT_ROOT_CANDIDATE: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
NEXT_RUNTIME_WITNESS_SEAM: E0B8C bone IDs 3/42 plus E2180 player-scoped node 0x14960
NEXT_FILTER_IDENTITY: player scope + resource 0x62B + node offset 0x14960
NEXT_FILTER_CONFIDENCE: HIGH
WITNESS_CAN_DISTINGUISH_GUEST_VS_INTERPOLATION: YES_PENDING_RUNTIME_VALUES
SNOUT_TRANSFORM_WITNESS_IMPLEMENTED: YES
P09_KNOWLEDGE_PROMOTED_TO_RE_DOCS: YES
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
DIFF_CHECK: PASS
USER_MANUAL_TEST_REQUIRED_NOW: YES
NEXT_SINGLE_ACTION: run the new default-off bone/matrix witness once and inspect both generated CSVs
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

Repair writeup: [[p09-player-model-witness-r31-repair]]. Fresh agents:
[[p09-secondary-sway-static-root]], [[p09-backpack-flap-witness]],
[[../../HANDOFF]].

## CURRENT (2026-09-05) — P09 V7 fresh capture state C

The fresh V7 runtime launch produced a status heartbeat with
`ENABLED=1`, raw APPEND/fullSync/render-selection counts of
`3104629/1799/3929`, and no CSV. All tracked counters and all six target
record counters are zero (`OBSERVED_TARGET_MASK=0x00`). This is V7 state C:
activation is valid, but target/filter correlation is the earliest failed gate.
Stop before causal analysis; do not combine V7 with V6/V6B/V6C rows.

```text
V7_RUNTIME_STATE: C
V7_ACTIVATION_VALID: YES
CAPTURE_USABLE: NO
CAPTURE_COMPLETE_FOR_CAUSAL_DECISION: NO
MANDATORY_TARGET_COVERAGE_COMPLETE: NO
CPU_HANDOFF_STATUS: UNKNOWN
ROOT_CLASS: UNKNOWN (target/filter correlation gate)
NEXT_SINGLE_ACTION: resolve why all six target DLs are filtered out, then rerun one bounded V7 capture retaining status and CSV
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```
Do not reopen `func_80092444`, D674C 0x621-first, overlay 456, `0x80800320`, syscall 113/3.
Do not treat INTERPOLATION_TAGGING_BUG as proven.

## SUPERSEDED (2026-09-02) — P09_MAP_BANJO_WOBBLE_PART3_TO_EXACT_GEOMETRY_AND_SNOUT

That round had no `TOOIE_DIAG_SECONDARY_SWAY`. Static findings remain valid.

```text
BANJO_LIVE_WALK_MODEL: 0x61F STRONG_HYPOTHESIS
SNOUT: boneAnimID 42
WOBBLE_PART3: boneAnimID 3 HIP; type-3 consumer 800E0E94 via 800AF04C
PART3_TAG5_RELATION: REFUTED (identity)
BANJO_SNOUT_USES_E2AA4: NO if 0x61F; YES if 0x62A
NEXT_SINGLE_ACTION: SUPERSEDED — do not use
ARTIFACT: /tmp/p09-banjo-wobble-part3-20260902T060241Z/
```

## SUPERSEDED (2026-09-01) — diagnostic propagation repaired

The first dual-matrix witness was zero-sample. The previous single TLS
boolean could be erased by nested DE448 model draws; this is now fixed with a
bounded 32-entry per-thread draw-scope stack. Stage wrappers/counters cover
`babackpack_entrypoint_3`, asset resolution, DE448, DE498, E2AA4, and E2180,
including E2180 rejection reasons. The stable inner identity is the resolved
DE448 `model_bin` pointer; the resource ID is retained from D674C only at the
outer draw boundary.

```text
FILTER_PROPAGATION_BUG_FOUND: YES (fixed)
FILTER_CONTEXT_NEST_SAFE: YES
E2180_SAMPLE_WRITTEN_COUNT: implemented
ROOT_CLASS: INTERPOLATION_TAGGING_ROOT_CANDIDATE / MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
NO_COMMIT: YES
NO_PUSH: YES
NEXT_SINGLE_ACTION: one short stage-counter runtime capture
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
```

Fresh agents should read this plus [[p09-backpack-flap-witness]] and
[[../../HANDOFF]]. Do not reopen `func_80092444` branch hunting.

```text
TOOIE_HEAD: ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf (worktree dirty; no commit)
RT64: 6f1c2d99a4ea571c139f449c326fd176ba8f3496
BANJORECOMP: ec859632cfa584e2272d32e46051543429a2baf4
OLD_BABACKPACK_LOOKUP_HYPOTHESIS: REFUTED_FOR_THIS_RUNTIME_PATH
DRAW_ENTRY: babackpack_entrypoint_3
MODEL_ID: 0x621
RENDER: func_800DE448 → func_800DE498
DUAL_MTX: func_800E2180
TAGGING: missing vs Banjo GeoCmd2
WITNESS: TOOIE_DIAG_BACKPACK_FLAP=1 default OFF
CANDIDATE_IMPLEMENTED: NO
NEXT_SINGLE_ACTION: one short user capture
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
ARTIFACT: /tmp/p09-backpack-real-render-path-20260901T231354Z/
```

## Latest stage-counter capture (2026-09-01)

The repaired nested-draw witness ran, but the presumed backpack identity was
not reached:

```text
BABACKPACK_ENTRY3_HITS=0
ASSET_RESOLVE_HITS=28639
ASSET_FIXED_ID_HITS=0 (0x621/0x625/0x60A/0x61B/0x831)
ASSET_OTHER_HITS=28639
DE448_TOTAL_HITS=17139
DE448_BACKPACK_FILTER_HITS=0
DE498_TOTAL_HITS=17955
DE498_BACKPACK_FILTER_HITS=0
E2AA4_TOTAL_HITS=0
E2AA4_BACKPACK_FILTER_HITS=0
E2180_TOTAL_HITS=1103
E2180_BACKPACK_FILTER_HITS=0
E2180_SAMPLE_WRITTEN_COUNT=0
E2180_REJECT_NO_ACTIVE_MODEL_CONTEXT=1103
FILTER_SCOPE_OVERFLOW_EVENTS=0
```

The pipeline therefore stops at the existing D674C asset-ID/model-bin
association, before DE448 filtering. This is not evidence against the
backpack model or interpolation. The existing asset probe now records a
bounded top-frequency table of the actual nonmatching IDs and model pointers;
it also reports maximum draw-context depth and an explicit overflow boolean.
This is an instrumentation repair, not a new hook or production change.

```text
RUNTIME_WITNESS_CAPTURED: NO
LAST_PROVEN_BACKPACK_STAGE: NONE
FIRST_MISSING_BACKPACK_STAGE: D674C_ASSET_ID_MODEL_BIN_ASSOCIATION
FILTER_PROPAGATION_NOW_WORKING: NOT_RUNTIME_PROVEN
CANDIDATE_IMPLEMENTED: NO
NEXT_SINGLE_ACTION: rerun the same short capture with the repaired existing asset observation table
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

## Latest D674C nonmatching-resource capture (2026-09-02)

The repaired observer recorded 44,620 D674C calls, all outside the five
presumed IDs. It retained 32 nonmatching resource/pointer pairs and reported
18,095 bounded-table evictions. DE448/DE498 remained unfiltered, E2AA4 stayed
at zero, and all 1,167 E2180 calls were rejected for no active model context.
Draw context reached depth 1 with no overflow; the witness CSV and anomaly
file remained empty.

D674C semantics are resolved statically: its argument is a resource-table
key, not a model pointer; its return is the loaded resource/model pointer used
by `babackpack_entrypoint_1` as DE448's fifth argument. The static `0x621` and
`0x625` setup values are valid candidates, but do not explain this runtime
population. The capture lacks callsite/owner and per-frame fields, so no
affected-part identity or interpolation/tagging linkage is proven.

```text
RUNTIME_WITNESS_CAPTURED: NO
FIRST_MISSING_BACKPACK_STAGE: D674C_CALLSITE_OWNER_ASSOCIATION
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
NEXT_SINGLE_ACTION: static MIPS/GeoCmd reverse-engineer Banjo body/snout and attached-model ownership before adding another runtime hook
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

## P09 V7 bridge repair state F (2026-09-07)

Model context now publishes from `secondary_sway_register_durable_bin()` after
the proven normalized `0x62B` metadata check. It is the sole V7 identity source.
Guest model pointers are sign-extended `uint64_t` values with a separate
physical base; replacement and durable-slot eviction clear matching context.
The getter wrapper remains a reason-counted, non-authoritative entry path. RT64 status exposes
seam registration/normalization/publication/clear/generation and getter reject
counters. Exact full-address validation remains unchanged and candidate-only
low-page matching cannot reach causal analysis.

Validation passed: TooieRecompiled build, 38/38 window tests, 7/7 projection
tests, Tooie/RT64 diff checks, and graft build. No game run or capture.

## P09 V7 fresh capture gate result — state G (2026-09-07)

The post-repair user capture is fresh and schema-valid (`P09_FEATHER_HANDOFF_V7`, version 1, enabled). The durable model context works at runtime: 1,449 durable registrations, 1,297 normalized `0x62B`, 1,297 publications, zero clears; last guest bin `0xFFFFFFFF80229070`, physical base `0x229070`. Getter reason counters are emitted; the first reason is `GETTER_REJECT_ARG_FILTER`.

Exact admission still blocks the witness. The CSV has 5,296 candidate records and no validated records: 2,526 reject for zero model context and 2,770 reject for full-address DL mismatch. Candidates are present for `OWN_105_0x534` (1,879), `JOIN_DISTAL_0x54C` (1,906), `OWN_104_0x57C` (1,216), and `OWN_103_0x5AA` (295); `JOIN_DISTAL_0x564` and `JOIN_ROOT_0x592` have none. `CAPTURE_COMPLETE_FOR_CAUSAL_DECISION=NO`, `OBSERVED_TARGET_MASK=0x00`, and tracked append/handoff/render counts remain zero. Per the V7 stop rule, no face/upload/join/render analysis or root promotion is valid. Next single action: repair the candidate-to-full-address admission mismatch and capture again. No code, GPU path, commit, or push changed in this analysis turn.
## P09 V7 fresh capture after exact direct-DL admission repair — state H (2026-09-07)

Fresh status/CSV artifacts are schema-valid and postdate the current executable. Direct full-address admission now works: durable normalized 0x62B publications=1485, exact target matches=40124, per-target exact counts are 0x534=11464, 0x54C=5732, 0x564=5732, 0x57C=5732, 0x592=5732, and 0x5AA=5732; OBSERVED_TARGET_MASK=0x3F.

Bounded CSV coverage is 448 APPEND samples and 96 INITIAL_HANDOFF_OK samples (16 per target), with 32 global RENDER_SELECTION samples. Face range/content checks pass for all sampled handoffs, velocity remains zero, and sampled initial matrices match. Every handoff reports byte_mismatch=3, but the combined counter has no component breakdown, so full append-to-handoff payload equality and the first divergence remain UNKNOWN.

All sampled JOIN APPEND records have unresolved expected roles and all sampled JOIN handoffs set unknown_join=1. CAPTURE_COMPLETE_FOR_CAUSAL_DECISION=NO, so CPU_HANDOFF_STATUS remains UNKNOWN and no later-renderer root is promoted. Next single action: isolate the three byte mismatches by component and resolve JOIN expected-role provenance, then rerun one bounded V7 capture. Analysis only; no production fix, GPU readback, synchronization, commit, or push.
