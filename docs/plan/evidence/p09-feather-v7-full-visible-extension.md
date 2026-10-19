---
title: "P09 V7 full visible feather CPU handoff extension"
description: "Extends the diagnostic V7 exact-admission witness from the right plume to the left plume and crest."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-07
tags: [tooie-recomp, p09, kazooie, feather, handoff, diagnostic]
---

# P09 V7 full visible feather CPU handoff extension

This is a diagnostic-only extension. The original six right-plume target
indices remain unchanged and keep their VERSION=2 semantics. The witness now
uses `P09_FEATHER_HANDOFF_V7 / VERSION=3` and admits fourteen exact addresses:
the six right-plume DLs, six left-plume DLs, and the two crest DLs.

## Static target contract

All targets use `physical(model_bin)+0x2670+logical_index*8` and full-address
equality. The left own meshes are `0x636` (bone 100, n=8,
`0x01003B10`, slots 0–7), `0x608` (bone 101, n=8, `0x01003A10`, slots 0–7),
and `0x5C0` (bone 102, n=12, `0x01003850`, slots 0–11). Their world matrix
expectations are the owning bones 100, 101, and 102 respectively.

The left join maps are independently derived from the ROM display lists:

* `0x61E` (107↔100): child n=4 from `0x01003A90`, slots 0–3, POP, parent
  n=4 from `0x01003AD0`, slots 28–31; triangles mix the two roles.
* `0x5F0` (100↔101): child n=4 from `0x01003990`, slots 0–3, POP, parent
  n=4 from `0x010039D0`, slots 28–31; triangles mix the two roles.
* `0x5D8` (101↔102): child n=4 from `0x01003910`, slots 0–3, POP, parent
  n=4 from `0x01003950`, slots 28–31; triangles mix the two roles.

Thus the left join derivation is **PROVEN** and symmetric with the right
plume in slot and POP ordering; only the source banks and bone relationships
differ.

The crest own `0x4A6` is bone 79, child of head 108, with n=7 from
`0x01003010`, slots 0–6. Crest join `0x4B0` is independently classified:
child n=5 from `0x01003080`, slots 0–4, POP, parent n=5 from `0x010030D0`,
slots 27–31. The child role is bone 79 and the parent role is bone 108;
the static topology is 3 own triangles plus 6 join triangles. Crest join
derivation is **PROVEN** and intentionally does not use plume n=4 semantics.

## Implementation and validation

Changed only the existing RT64 diagnostic witness in
`BanjoRecomp/lib/rt64/src/hle/rt64_workload.h` and
`rt64_workload.cpp`; the existing RSP admission path is reused. The status
heartbeat reports region presence/provenance masks, independent counters for
`0x61E`, `0x5F0`, `0x5D8`, and `0x4B0`, per-region component mismatches, a
self-diagnosing first-divergence region, and bounded row limits
(128 append, 144 handoff, 128 render rows per target; total diagnostic budget
5888 rows).

```text
RIGHT_6_EXACT_IDENTITIES_UNCHANGED: YES
LEFT_6_EXACT_IDENTITIES_PROVEN: YES
CREST_2_EXACT_IDENTITIES_PROVEN: YES
ALL_14_EXACT_TARGETS_STATICALLY_REACHABLE: YES
EXISTING_RIGHT_TARGET_SEMANTICS_UNCHANGED: YES
SCHEMA_CHANGE_REQUIRED: YES
NEW_VERSION: 3
SELF_DIAGNOSING_STATUS_IMPLEMENTED: YES
FULL_VISIBLE_FEATHER_CPU_HANDOFF_DECISION_IMPLEMENTED: YES
BUILD_RESULT: PASS (cmake --build build -j2)
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
TOOIE_DIFF_CHECK: PASS
RT64_DIFF_CHECK: PASS
USER_MANUAL_TEST_REQUIRED_NOW: YES
USER_TEST_COMMAND: rm -f /tmp/tooie-kazooie-feather-v7-cpu-handoff-status.txt /tmp/tooie-kazooie-feather-v7-cpu-handoff.csv && TOOIE_DIAG_FEATHER_HANDOFF_V7=1 ./build/TooieRecompiled
NEXT_SINGLE_ACTION: run one bounded VERSION=3 capture and inspect all fourteen target masks and CPU handoff counters
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna after capture
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```
