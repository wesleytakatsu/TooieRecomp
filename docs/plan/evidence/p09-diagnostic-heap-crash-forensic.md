---
title: "P09 diagnostic heap crash forensic"
description: "Native double-free abort during JOIN recapture is a proven CPU diagnostic data race; overlay unload is not the free site."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-14
tags: [tooie-recomp, p09, interpolation, join, diagnostic, heap, crash]
---

# P09 diagnostic heap crash forensic

## Capture that aborted

JOIN repair recapture was VALID until process abort on feather
collectible pickup. Status file truncated mid-line after
`POLICY_A_P99_JOIN_PHASE_`. Do not infer `BEST_COHERENT_SCOPE`
from that incomplete file.

```text
JOIN_DIAGNOSTIC_VALID=YES
JOIN_PAIR_BUILDER_REACHED=YES
JOIN_PAIR_COUNT=53946
JOIN_MIXED_WORLD_TRIANGLE_COUNT=9990
JOIN_MULTI_WORLD_GAMECALL_COUNT=2331
JOIN_EDGE_SKIP_SAME_WORLD=44955
JOIN_EDGE_SKIP_UNMAPPED_BONE=0
JOIN_ARTICULATED_PAIR_EMIT_COUNT=53946
SESSION_FEATHER_BONE_MASK=0x1ff
LIVE_JOIN_MAX_RELATIVE_ERROR=444117.469
LIVE_JOIN_P95_RELATIVE_ERROR=0.146564901
LIVE_JOIN_P99_RELATIVE_ERROR=0.629098833
POLICY_A_JOIN_MAX_RELATIVE_ERROR=1.00578058
POLICY_A_JOIN_P95_RELATIVE_ERROR=0.157621086
POLICY_A_JOIN_P99_RELATIVE_ERROR=0.734187126
CURRENT_JOIN_MAX_RELATIVE_ERROR=1
PREV_JOIN_MAX_RELATIVE_ERROR=1
LIVE_MAX_JOIN_PHASE_DELTA=915.268188
LIVE_P95_JOIN_PHASE_DELTA=0.39479208
LIVE_P99_JOIN_PHASE_DELTA=56.3828049
POLICY_A_MAX_JOIN_PHASE_DELTA=0.484863997
POLICY_A_P95_JOIN_PHASE_DELTA=0.00559738278
```

POLICY_A eliminates catastrophic JOIN/phase outliers. It does not
clearly improve ordinary P95/P99 JOIN relative error. Visual
POLICY_A is already `FAIL`. Do not treat W-crossing as a P09
solution. Do not implement a production coherent scope until
`PER_*` CURRENT percents, trigger counts, JOIN metrics, and
`BEST_COHERENT_SCOPE` are captured from a complete status file.

## Task 1 — core dump

```text
CORE_DUMP_FOUND: YES
PID: 217569
SIGNAL: SIGABRT (6)
TIMESTAMP: 2026-09-14 19:17:34 -03
GLIBC: double free or corruption (out)
ABORT_THREAD: gfx_thread_func LWP 217589
CONCURRENT_THREAD: WorkloadQueue::renderThreadLoop LWP 217629
```

Abort thread:

```text
malloc_printerr
_int_free_merge_chunk / _int_free / __libc_free
std::vector<float>::operator=
RT64::p09PercentileCopy(P09ResidualAccum const&, float)
RT64::p09WritePolicyAResidualBlock
RT64::p09RspEndpointBindingNote
RT64::RSPProcessor::process
RT64::State::fullSync
RT64::Interpreter::processDisplayLists
RT64::Application::processDisplayLists
gfx_thread_func
```

Concurrent thread in the same diagnostic:

```text
std::__introsort_loop on vector<float>
RT64::p09PercentileCopy(P09ResidualAccum const&, float)
RT64::p09WritePolicyAResidualBlock
RT64::p09RspEndpointBindingNote
RT64::RSPProcessor::process
RT64::WorkloadQueue::threadRenderFrame
RT64::WorkloadQueue::renderThreadLoop
```

```text
MALLOC_ABORT_SITE: vector<float> assignment inside p09PercentileCopy
FIRST_PROJECT_FRAME: p09PercentileCopy / p09WritePolicyAResidualBlock / p09RspEndpointBindingNote
CRASH_ALLOCATOR_CLASS: DOUBLE_FREE
```

## Task 2 — JOIN repair audit

Cross-draw JOIN pairing stores value copies (`P09JoinVert`) in
per-call and per-bone local vectors, not pointers into GameCall
face/vertex storage. World→bone uses a 512-slot stack array with
`world >= 512` rejected. That path is not the abort site.

The new JOIN work made `p09RspEndpointBindingNote` heavier, so
`gfx_thread_func` and the RT64 render worker overlap more often
on the process-global accumulators.

## Task 3 — concurrency

`g_p09RspEndpointBinding` is a process-global singleton. Both
`State::fullSync` and `WorkloadQueue::threadRenderFrame` call
`RSPProcessor::process` → `p09RspEndpointBindingNote`. There was
no CPU mutex. `P09ResidualAccum::sortedSamples()` does
`sorted = samples; std::sort(sorted)` while another thread may be
in the same function. That is a container data race; glibc later
reports double-free.

```text
DIAGNOSTIC_SHARED_STATE_THREAD_SAFE: NO (before fix)
POSSIBLE_CONTAINER_DATA_RACE: YES (PROVEN in core)
```

Fix: `std::mutex` around the whole diagnostic note, including
percentile cache mutation and status write. Production
`RSPProcessCS` unchanged. No GPU sync.

## Task 4 — WORLD_TO_BONE_BOUND_COUNT=0

```text
WORLD_TO_BONE_MAPPING_STATE_VALID: YES
WORLD_TO_BONE_BOUND_COUNT_SCOPE: LAST_DISPATCH
```

The counter is reset at the start of each `process()` and only
counts V7 OWN/JOIN DL world→bone binds on that dispatch.
`LAST_FEATHER_BONE_MASK=0x0` on the truncated file is the same
last-dispatch field. `SESSION_FEATHER_BONE_MASK=0x1ff` and
`JOIN_PAIR_COUNT=53946` are session-accumulated. Zero last-dispatch
binds does not contradict session JOIN pairs and cannot itself
cause the heap abort.

## Task 5 — game vs diagnostic

```text
CRASH_CAUSED_BY_P09_DIAGNOSTIC: PROVEN
CRASH_CAUSED_BY_EXISTING_GAME_OVERLAY_PATH: NO_EVIDENCE
```

The abort thread is inside the diagnostic percentile writer.
`ovl_unload` on stdout is not the free site. Feather pickup likely
only increased overlapping `RSPProcessor::process` calls.

## Task 6/7 — fix

- CPU mutex on `g_p09RspEndpointBinding`
- JOIN triangle records by index, not a pointer into `callJoinVerts`
- status written to `/tmp/tooie-rsp-endpoint-binding-status.txt.tmp`
  then `rename` over the final path only after a complete flush

```text
DIAGNOSTIC_FIX_IMPLEMENTED: YES
STATUS_WRITE_NOW_ATOMIC: YES
PRODUCTION_POLICY_CHANGED: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
```
