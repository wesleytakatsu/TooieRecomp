---
title: "P08 — PATCH 4 Hybrid F' activation / regeneration / validation"
description: "First production integration of Hybrid F' for Tooie: startup activation, generator regen, T13/T14/rollback/T15. C2AB8 freeze resolved. New blocker func_800BCF28."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, patch4, hybrid-f-prime, c2ab8, t13, t15]
---

# P08 — PREEMPTION PATCH 4 VALIDATION

```text
MODE:                 IMPLEMENTATION / VALIDATION
MODEL:                Grok 4.6 HIGH
TOOIE_BASE_HEAD:      33d23ad8a2d9792bf8aff833af3782b597216283
RUNTIME_BASE_HEAD:    ca568b6ad79b9029d14077f0c3ffa757727c5559
N64RECOMP_BASE_HEAD:  2b6f05688de2abc7d86da5b4a89b84c2c6acbabe
PRODUCTION_PREEMPTION_ACTIVE: YES
PERMANENT_PREEMPTION_DELIVERED: YES
TOOIE_REGEN_RUN:      YES
MANUAL_GENERATED_C_EDIT: NO
```

This is the first production integration of the already-reviewed Hybrid F'
implementation. Architecture was not reopened. PATCH 1/2/3/3.5 were not
modified. `func_800BCF28` was identified only, not diagnosed.

Tracked-state delivery correction (scaffolds + clean rebuild) is recorded
in [[p08-preemption-patch4-clean-build-correction]]. The original local
behavioral evidence below is **preserved**.

## 0. Canonical state before writes

```text
TOOIE_HEAD:                 33d23ad8a2d9792bf8aff833af3782b597216283
TOOIE_BRANCH:               main
TOOIE_WORKTREE:             clean
RUNTIME_HEAD:               ca568b6ad79b9029d14077f0c3ffa757727c5559
N64RECOMP_HEAD:             2b6f05688de2abc7d86da5b4a89b84c2c6acbabe
RUNTIME_DIRTY_PROTECTED:    YES (threadqueue Fix B, threads NULLPTR,
                            PATCH3.5 resume hook, preemption impl)
N64RECOMP_DIRTY_PROTECTED:  YES (same-section JAL + PATCH3 safepoints + T11)
NO destructive git cleanup.
```

Patch hashes before any write (and after PATCH 4 — unchanged):

```text
scheduler:   1b3d3d4666e02f748c44d19d867d02669b83753d85fe7eb62d746a5a8cf38309
preemption:  8959055e6c51767fcd7c58b403a2c6ce8bc7aa0ab6c23ffab9dc181905eacd6b
n64recomp:   2045cd90cfae70c881a9504146bac5174ca7a53ba1f5e6200a6a3e4adb5a044c
```

## 1. Baseline before activation / regen

```text
PATCH35 harness:
  cmake -S BanjoRecomp/lib/N64ModernRuntime/ultramodern \
        -B /tmp/p08-p4-baseline-harness \
        -DULTRAMODERN_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
  cmake --build ... --target preemption_harness
  /tmp/p08-p4-baseline-harness/preemption_harness
  PASS (pass=40 fail=0 skipped=0) exit 0

T11:
  cmake --build N64Recomp/build --target PreemptionSafepointTest
  ./PreemptionSafepointTest
  PASS (pass=61 fail=0) exit 0

Tooie current build, no regen:
  ninja -C build TooieRecompiled
  PASS (no work) exit 0
```

## 2. Activation

```text
ACTIVATION_SITE:
  src/main/main.cpp
  activate_tooie_guest_preemption()
  immediately before recomp::start()
  (before ultramodern::preinit / any guest OSThread)

ACTIVATION_BEFORE_GUEST_THREADS: YES
TOOIE_SCOPED_ACTIVATION:         YES
DEFAULT_PREEMPTION:              ON
ROLLBACK_ENV:                    ULTRAMODERN_GUEST_PREEMPTION=0
ROLLBACK_STARTUP_ONLY:           YES
HOT_TOGGLE:                      NO
```

Env parsing (Tooie-only, not a general config parser):

```text
unset / empty:   ON
"0":             OFF
"false"/"off"/"no" (ASCII case-folded): OFF
any other value: ON
```

N64ModernRuntime default remains OFF. Tooie startup chooses ON unless the env
is an explicit false. The public setter is still startup/quiescent-only.

Observed at run start:

```text
ON : [tooie] guest preemption: ON (ULTRAMODERN_GUEST_PREEMPTION=<unset>)
OFF: [tooie] guest preemption: OFF (ULTRAMODERN_GUEST_PREEMPTION=0)
```

## 3. Generator rebuild + regen

```text
N64RECOMP_BINARY:
  BanjoRecomp/lib/N64ModernRuntime/N64Recomp/build/N64Recomp
N64RECOMP_BUILD_HEAD:
  2b6f05688de2abc7d86da5b4a89b84c2c6acbabe + cumulative n64recomp-tooie.patch
N64RECOMP_PATCH_HASH:
  2045cd90cfae70c881a9504146bac5174ca7a53ba1f5e6200a6a3e4adb5a044c
N64RECOMP_BUILD: PASS (ninja, no work; existing patched binary)
TOY T11 BEFORE REGEN: PASS (61/0)

REGEN_COMMAND: ./tools/run_recomp.sh
REGEN_EXIT:    0
REGEN_TIME:    1.57s
FUNCTION_COUNT: 46472
GENERATED_FILES: 420 (90M)
MANUAL_GENERATED_C_EDIT: NO

REGEN_REPRODUCIBLE: YES
  second ./tools/run_recomp.sh: 420/420 files identical, C2AB8 identical,
  STATIC_HINT_CALLS still 4298
```

## 4. Generated output

```text
STATIC_SAFEPOINT_SITE_COUNT: 4298
  (ultramodern_preemption_hint() / ultramodern_service_preemption pairs)
  PATCH 3 estimate was ~4300; actual is in range.

C2AB8_SAFEPOINT_PRESENT:          YES
C2AB8_SAFEPOINT_AFTER_DELAY_SLOT: YES
C2AB8_FILE: RecompiledFuncs/funcs_327.c
```

Inner 60-iter walk (`bne $s2,$s5,L_800C2AFC` at 0x800C2B48) and outer pending
retry (`bnel $s3,$zero,L_800C2AF4` at 0x800C2B50) both emit:

```c
/* delay-slot materialization */
if (ultramodern_preemption_hint()) {
    ultramodern_service_preemption(rdram, (void*)&ctx->status_reg);
}
goto <backward label>;
```

No manual patches. 416/417 hashed `.c` files changed vs pre-regen snapshot
(expected: first safepoint emission). Second regen vs first: 0 changes.

Leftover scan of Tooie src/tests:

```text
C2AB8_OVERRIDE_PRESENT: NO
yield_self_1ms in Tooie diagnostic source: NO
table2 forced clear: NO
manual scheduler injection: NO
```

## 5. Tooie rebuild

```text
compiler: clang / clang++ 18.1.3
config:   Release -O3 -DNDEBUG -march=nehalem
command:  ninja -C build TooieRecompiled
exit:     0
binary:   build/TooieRecompiled
mtime:    2026-08-13 14:05
sha256:   384c8261cd3327ef9441e7b547649d6ddd16c71627b06b5da39bc82af9fe5801
size:     28333640
```

## 6. T13 — PATCH4 ON

Canonical env, preemption unset (default ON). No debugger before the former
freeze.

```text
TOOIE_AUTO_START=1
TOOIE_DIAG=1
TOOIE_DISABLE_LAUNCHER_MUSIC=1
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json
./build/TooieRecompiled
```

```text
START: 2026-08-13T14:05:54-03:00
END:   2026-08-13T14:06:51-03:00
EXIT:  139
SIGNAL: 11 SIGSEGV
DURATION: 57s
```

```text
T13_PATCH4_ON: PASS
C2AB8_ENTERED: YES
C2AB8_RETURNED: YES
TABLE2_BEFORE: NOT_EXPOSED_BY_EXISTING_DIAG
TABLE2_AFTER:  NOT_EXPOSED_BY_EXISTING_DIAG
  INFERENCE: cleared naturally. C2AB8 returned and world-init ran.
  Same permanent outcome as the 2026-08-12 yield experiment, without
  a C2AB8-specific yield or forced clear.
AUDIO_PROGRESS_DURING_FORMER_FREEZE: YES
RSP_AUD_PROGRESS: YES   (21 → 1498 across ticks; still advancing at crash)
QUEUE_SAMP_PROGRESS: YES (30912 → 2259520)
ATTRACT_PROGRESS_AFTER_C2AB8: YES
  1283 ovl_load events; last overlays include world-init set
  (gsworld / basetup path). Crash stack includes func_800A76F4
  then gsworldDll_entrypoint_2 — past A7380/C2AB8.
DOWNSTREAM_SIGSEGV: YES
DOWNSTREAM_FUNCTION: func_800BCF28
DOWNSTREAM_DIAGNOSIS_PERFORMED: NO
```

Post-crash gdb (core of pid 2570407 only):

```text
#0  func_800BCF28 + 225
#1  bakey_heldCount
#2  bainput_func_80097B4C
#3  func_800A4878
#4  func_800A4A74
#5  func_800A4B08
#6  func_800F452C
#7  basetup_entrypoint_2
#8  recomp_syscall_handler
#10 gsworldDll_entrypoint_2
#14 func_800A76F4
```

Matches the prior temporary-yield observation. Not investigated further.

Opening-mode log budget (400 lines) expired while still printing mode=3, so
type-4 / warp=3 / mode=5 were not printed. The crash stack in `func_800A76F4`
plus `gsworldDll_entrypoint_2` is the mode-5/world-init path after C2AB8.

## 7. T14 — intro / audio regression (same ON run)

```text
T14_INTRO_VISUAL:   PASS  (opening overlays 731/677 ran; RT64 presented;
                           no early hang; 57s natural progress)
T14_INTRO_AUDIO:    PASS  (pulseaudio default; queue_samp and rsp_aud
                           advanced from first ticks)
T14_JIGGY:          PASS  (intro path completed far enough to reach
                           A76F4 / world init)
T14_TYPE4:          PASS  (reached A76F4/gsworld; that is after type-4
                           complete + warp consume)
T14_AUDIO_COUNTERS: PASS  (d, rsp_aud, queue_samp strictly increasing
                           until the downstream SIGSEGV)
```

No new early hang or crash before the former C2AB8 window.

## 8. Rollback A/B — same binary, env OFF

```text
SAME_BINARY_ON_OFF: YES
  sha256 384c8261cd3327ef9441e7b547649d6ddd16c71627b06b5da39bc82af9fe5801
START: 2026-08-13T14:08:09-03:00
ENV:   ULTRAMODERN_GUEST_PREEMPTION=0
LOG:   [tooie] guest preemption: OFF (ULTRAMODERN_GUEST_PREEMPTION=0)
```

Generated safepoints remain in the binary. Hint is false; service is not
entered.

Freeze confirmation (no debugger before freeze):

```text
d/syscall:     179205 stable
rsp_aud:       1530 stable
queue_samp:    2302576 stable
last_ovl:      635:2 stable
trailing_stable_ticks: 29 (~58s)  >= 6s required
process:       still alive (not SIGSEGV)
ptrace attach after freeze: DENIED (yama ptrace_scope)
```

```text
ROLLBACK_OFF_REPRODUCES_OLD_FREEZE: YES
CAUSAL_CLASS: A
```

Same generated binary:

- ON: C2AB8 crossed naturally; AUDIO progressed; SIGSEGV in `func_800BCF28`
- OFF: old cooperative freeze returned (d/audio/ovl frozen ≥58s)

No diagnostic yield or table[2] clear.

## 9. T15 microbenchmark

Standalone Tooie target `preemption_t15` (`tests/preemption_t15.cpp`),
`EXCLUDE_FROM_ALL`. No guest ROM. Public APIs only.

```text
CPU:       Intel(R) Core(TM) i5-4690K CPU @ 3.50GHz
OS:        Linux 6.8.0-137-generic x86_64 GNU/Linux
compiler:  Ubuntu clang version 18.1.3
opt:       Release -O3 -DNDEBUG
N:         10,000,020 backedges (inner=60, outer=166667)
reps:      3
```

| mode | median ns/backedge | range | hint_true | service | publish |
| --- | ---: | --- | ---: | ---: | ---: |
| A flag OFF | 1.480 | 1.339–1.525 | 0 | 0 | 0 |
| B flag ON, no publish | 2.326 | 2.232–2.515 | 0 | 0 | 0 |
| C flag ON, ~60 Hz publish | 2.380 | 2.279–2.670 | 2 | 2 | 2 |

```text
T15_A_NS: 1.480
T15_B_NS: 2.326
T15_C_NS: 2.380
T15_B_OVER_A: 1.572
T15_PUBLISH_COUNT: 2
T15_SERVICE_COUNT: 2
SERVICE_PER_BACKEDGE_B: NO
SERVICE_PER_BACKEDGE_C: NO
T15_FUNCTIONAL: PASS
```

No performance percentage gate. C services equal sparse publishes, not
one per backedge. Loop wall time is ~23 ms, so only two 16 ms publishes
fit; that is enough to prove sparsity.

## 10. Actual Tooie performance smoke

Qualitative only, ON run to the downstream crash:

```text
startup → crash: ~57s (intro + attract/world init)
intro obvious slowdown: not observed
audio underruns: not observed (queue_samp/rsp_aud advanced steadily)
CPU saturation: not profiled
```

T15 remains the quantitative microbenchmark.

## 11. Post-activation lower-level tests

```text
POST_ACTIVATION_PATCH35_HARNESS: PASS (40/0/0)
POST_ACTIVATION_T11:             PASS (61/0)
```

Activation is Tooie `main.cpp` only. It does not change the harness or
generator tests.

## 12. Artifacts / reconstruction

```text
ARTIFACTS_CHANGED_IN_PATCH4: NONE
  Tooie-only tracked source + docs. Runtime/N64Recomp patches untouched.
CLEAN_RECONSTRUCTION: NOT_REQUIRED
  reason: no versioned runtime or N64Recomp patch content changed
PATCH_SCRIPT_IDEMPOTENT: YES
  apply_runtime_patches.sh: all three already applied
TOOIE_GAME_SOURCE_CHANGED:
  src/main/main.cpp (activation)
  tests/preemption_t15.cpp (new)
  CMakeLists.txt (preemption_t15 EXCLUDE_FROM_ALL)
RECOMPILEDFUNCS_TRACKED_CHANGED: NO (gitignored; generator-only)
```

## 13. Residuals (not PATCH 4)

```text
GENERIC_JR_RESIDUAL: DEFERRED
TIMER_PI_APPROXIMATIONS: DEFERRED
RCP-X2/MTC0 helper-level: DEFERRED
FLAG_SETTER hot-mutation: DEFERRED (startup-only, as designed)
T03 historical flake: class E, not reproduced
```

## 14. Verdict

```text
PRODUCTION_PREEMPTION_ACTIVE: YES
PERMANENT_PREEMPTION_DELIVERED: YES
C2AB8_BLOCKER_STATUS: RESOLVED
ROOT_CAUSE_STATUS:
  PROVEN + PERMANENTLY FIXED for the observed C2AB8 path
  (Hybrid F' safepoints + Tooie startup ON; CLASS A ON/OFF)
NEW_CURRENT_BLOCKER: func_800BCF28 SIGSEGV
DO_NOT_CLAIM: gameplay / demo complete / input complete
```

## 15. Next

Independent review of this PATCH 4 evidence, then investigate
`func_800BCF28` only. Do not reopen Hybrid F' or PATCH 1–3.5.

=== END PATCH 4 VALIDATION ===
