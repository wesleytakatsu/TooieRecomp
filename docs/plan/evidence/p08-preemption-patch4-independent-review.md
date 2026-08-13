---
title: "P08 — independent review of preemption PATCH 4"
description: "Independent source, reconstruction, ON/OFF, regression, performance, and distribution review of Hybrid F' PATCH 4."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, patch4, independent-review, reproducibility]
---

# P08 — PREEMPTION PATCH 4 INDEPENDENT REVIEW

## Scope and verdict

Review base: `e2ac9c53426bddc3ed54b89a1f6c2b8f8ca0df98` on `main`.
The Tooie worktree was clean before review. The review was read, reproduce,
test, and documentation only. Runtime/N64Recomp source and generated C were
not edited. `func_800BCF28` was identified only as the downstream crash
function; its cause was not investigated.

```text
VERDICT: C — PATCH 4 correction required
```

The original verdict C is **preserved**. Implementer correction of the
distribution blocker is in
[[p08-preemption-patch4-clean-build-correction]]. That correction is
**not** independently accepted yet.

The behavioral mechanism passes independent review. The same executable,
SHA-256 `384c8261cd3327ef9441e7b547649d6ddd16c71627b06b5da39bc82af9fe5801`,
crossed the former C2AB8 blocker with the default ON and reproduced the old
stable freeze with the startup flag OFF. This is causal class A.

The correction is a tracked-state distribution blocker, not contradictory
evidence against Hybrid F'. A disposable clone regenerated all 4,298
safepoints deterministically from the tracked patch artifacts, but a strict
clean Tooie build stopped because CMake requires ignored, untracked files:

- `RecompiledPatches/patches.c`
- `RecompiledPatches/patches_bin.c`
- later integration also consumes `RecompiledPatches/recomp_overlays.inl`

`tools/run_recomp.sh` regenerates `RecompiledFuncs`, not these patch
scaffolds, and no tracked step recreates them. Copying the existing local
scaffolds into the disposable checkout allowed a supplementary build, but
that is specifically not clean-clone reproducibility. Therefore another
developer cannot obtain the claimed production binary from the stated
tracked inputs alone, and PATCH 4 is not accepted as delivered.

## Diff and activation review

The range `33d23ad8a2d9792bf8aff833af3782b597216283..e2ac9c5` changes
only the expected production surfaces: Tooie startup activation in
`src/main/main.cpp`, the T15 benchmark, an `EXCLUDE_FROM_ALL` CMake target,
and documentation. It does not change the runtime patch, N64Recomp patch, or
generator policy.

`activate_tooie_guest_preemption()` has one call site, immediately before
`recomp::start()`. The runtime default remains OFF; Tooie sets it once before
guest execution. There is no later hot toggle. The parser implements:

```text
unset or empty -> ON
0, false, off, no (case-insensitive) -> OFF
everything else -> ON
```

## Reconstruction and generated output

Disposable reconstruction used these exact dependency bases:

```text
N64ModernRuntime ca568b6ad79b9029d14077f0c3ffa757727c5559
N64Recomp        2b6f05688de2abc7d86da5b4a89b84c2c6acbabe
```

The active artifacts applied in canonical order. A second application
reported every patch already applied and returned success.

```text
scheduler   1b3d3d4666e02f748c44d19d867d02669b83753d85fe7eb62d746a5a8cf38309
preemption 8959055e6c51767fcd7c58b403a2c6ce8bc7aa0ab6c23ffab9dc181905eacd6b
N64Recomp   2045cd90cfae70c881a9504146bac5174ca7a53ba1f5e6200a6a3e4adb5a044c
```

Patched N64Recomp built cleanly. Two canonical Tooie regenerations produced
420 files each, 4,298 hint calls each, and the same aggregate digest
`a54bcbbfa7a5e997084f9b511087169b95734ff68574a6f7cc91d25bd1ae3236`.

In generated `RecompiledFuncs/funcs_327.c`, both relevant backward branches
in `func_800C2AB8` materialize the MIPS delay-slot assignment first, invoke
the safepoint hint/service second, and transfer backward third. No strong
C2AB8 override, diagnostic yield, forced table clear, or Tooie-specific
scheduler call was found.

## Same-binary causal control

ON used the canonical environment with
`ULTRAMODERN_GUEST_PREEMPTION` absent from the process environment. The
startup log reported the feature ON. Execution crossed C2AB8, audio counters
continued, world initialization ran, and the process ended in SIGSEGV at
`func_800BCF28`. Representative independent counters advanced from
`d=72382/rsp_aud=491/queue_samp=742624` through
`d=129778/rsp_aud=1031/queue_samp=1556640` and continued toward the crash.
The implementer log independently spans `rsp_aud 21 -> 1498` and
`queue_samp 30912 -> 2259520` through the former freeze interval.

OFF used the same executable without rebuild or regeneration and added only
`ULTRAMODERN_GUEST_PREEMPTION=0`. At the old boundary, repeated diagnostic
ticks remained exactly:

```text
d/r = 179285/179285
rsp_aud = 1530
queue_calls = 1529
queue_samp = 2304784
last_ovl = 635:2
```

The values were stable comfortably beyond six seconds. This is the old
cooperative C2AB8 freeze, not a slower run.

## Evidence precision

PATCH 4 did not expose `table[2].word0` before or after the ON run. C2AB8's
contract and downstream progress prove that its pending condition ceased and
the function returned. They do not directly observe the exact word as zero in
this run. The prior temporary-yield experiment did observe zero, so zero here
is a well-supported inference, not a PATCH 4 direct measurement.

The lifecycle evidence already proves that `func_800A76F4(5)` is reached only
after type-4 completion opens the gate and `func_800A9924` consumes the warp.
The PATCH 4 world-init stack (`func_800A76F4`, `gsworldDll_entrypoint_2`,
`basetup_entrypoint_2`, `bakey_heldCount`, `func_800BCF28`) is therefore
strictly downstream. The milestones are logically proven even though the
opening diagnostic line budget expired before printing every transition.

## T15 and lower-level regressions

Independent T15 medians on this machine:

```text
A OFF                  1.304 ns/backedge
B ON, no publish       2.223 ns/backedge
C ON, host publishes   2.223 ns/backedge
B/A                    1.704x
B-A                    0.919 ns/backedge
```

All measured C repetitions published and serviced two requests. The loop
result and counters are consumed, so the optimizer cannot erase the relevant
calls. The benchmark is conditional rather than fully self-policing because
its source fails C only when both `service == 0` and `publish_count == 0`; a
publish-without-service result would not itself fail. The observed protocol
passes. A 70.4% relative microbenchmark slowdown is not free, but the absolute
fast-path cost is below 1 ns/backedge and no Tooie runtime regression was
observed. There is no percentage acceptance gate, so performance is not a
PATCH 4 blocker. Dynamic site density remains unprofiled; 4,298 static sites
do not imply 4,298 executions per frame.

Reconstructed lower-level results:

```text
PATCH35 harness: PASS (pass=40 fail=0 skipped=0)
T11:             PASS (pass=61 fail=0)
```

## Independent handoff

```text
=== P08 PREEMPTION PATCH 4 INDEPENDENT REVIEW ===

BASE_HEAD:
e2ac9c53426bddc3ed54b89a1f6c2b8f8ca0df98

MODEL:
GPT-5.6 SOL HIGH

VERDICT:
C

PATCH4_SCOPE_NARROW:
YES

ACTIVATION_TOOIE_SCOPED:
YES

ACTIVATION_BEFORE_GUEST_EXECUTION:
YES

ENV_ROLLBACK_VALID:
YES

CLEAN_TOOIE_REGEN:
PASS

CLEAN_TOOIE_BUILD:
FAIL

PATCH4_DISTRIBUTABLE_FROM_TRACKED_STATE:
NO

PATCH_HASHES_VALID:
YES

PATCH_SCRIPT_IDEMPOTENT:
YES

SAFEPOINT_COUNT:
4298

C2AB8_INNER_SAFEPOINT:
PASS

C2AB8_OUTER_SAFEPOINT:
PASS

DELAY_SLOT_ORDER:
PASS

MANUAL_C2AB8_HELP:
NONE

SAME_BINARY_ON_OFF:
YES

ON_C2AB8_CROSSED:
YES

ON_AUDIO_CONTINUES:
YES

ON_WORLD_INIT_REACHED:
YES

OFF_OLD_FREEZE_REPRODUCED:
YES

CAUSAL_CLASS:
A

TABLE2_WORD0_ZERO:
INFERENCE

C2AB8_PENDING_CONDITION_RESOLVED:
PROVEN

INTRO_VISUAL:
PROVEN

INTRO_AUDIO:
PROVEN

JIGGY:
PROVEN

TYPE4_COMPLETE:
PROVEN

WARP_CONSUMED:
PROVEN

MODE5_WORLD_INIT:
PROVEN

AUDIO_PROGRESS_DURING_OLD_FREEZE_WINDOW:
PROVEN

DOWNSTREAM_SIGSEGV:
YES

DOWNSTREAM_FUNCTION:
func_800BCF28

DOWNSTREAM_MATCHES_PRIOR_OBSERVATION:
YES

DOWNSTREAM_DIAGNOSIS_PERFORMED:
NO

T15_BENCHMARK_VALID:
CONDITIONAL

T15_A_NS:
1.304

T15_B_NS:
2.223

T15_C_NS:
2.223

T15_B_OVER_A:
1.704x

FAST_PATH_ABSOLUTE_OVERHEAD:
0.919 ns/backedge

T15_FUNCTIONAL:
PASS

PERFORMANCE_PATCH4_BLOCKER:
NO

PERFORMANCE_RESIDUAL:
70.4% relative microbenchmark cost; <1 ns absolute; dynamic site density unprofiled

PATCH35_HARNESS:
PASS (pass=40 fail=0 skipped=0)

T11:
PASS (pass=61 fail=0)

ROLLBACK_VALID:
YES

ROLLBACK_REQUIRES_REBUILD:
NO

DEFAULT_ON_PROVEN:
YES

REGEN_DETERMINISTIC:
YES

BLOCKING_ISSUES:
Clean tracked Tooie clone cannot build because required RecompiledPatches scaffolds are ignored and have no tracked regeneration step.

NONBLOCKING_ISSUES:
T15 C assertion is weaker than its evidence claim; dynamic safepoint density is unprofiled; table[2].word0 zero is inferred, not directly observed in PATCH 4.

PATCH4_ACCEPTED:
NO

PERMANENT_PREEMPTION_DELIVERED:
NO

C2AB8_BLOCKER_STATUS:
RESOLVED

ROOT_CAUSE_STATUS:
PROVEN for the observed C2AB8 path; permanent local behavior validated, tracked delivery correction required

CURRENT_BLOCKER:
OTHER — PATCH 4 tracked-state build/distribution correction; func_800BCF28 remains the next product divergence

SOURCE_CODE_CHANGED:
NO

EVIDENCE_CREATED:
docs/plan/evidence/p08-preemption-patch4-independent-review.md

SESSION_HANDOFF_UPDATED:
YES

GLOBAL_HANDOFF_UPDATED:
YES

PHASE_P08_UPDATED:
YES

SPECS_REVIEWED:
recomp-readiness, rsp-and-audio, build, matching, overlays

SPECS_CHANGED:
recomp-readiness and rsp-and-audio truth labels only

MARKDOWN_VAULT_AUDIT:
FAIL repo-wide (0 errors, 49 pre-existing warnings); new evidence note PASS 100/100

DOC_COMMIT:
see the repository commit containing this review

PUSHED:
reported by the final publication handoff

NEW_HEAD:
see the final publication handoff

WORKTREE:
expected clean after the documentation commit; verify in final publication handoff

NEXT_SINGLE_ACTION:
correct PATCH 4 tracked clean-build reproducibility before touching func_800BCF28

=== END HANDOFF ===
```

Do not claim gameplay, frontend, or P08 complete. The behavioral progression
remains intro/Jiggy -> attract -> C2AB8 crossed -> world init ->
`func_800BCF28` SIGSEGV, but downstream investigation is gated on correcting
the Patch 4 distribution contract.
