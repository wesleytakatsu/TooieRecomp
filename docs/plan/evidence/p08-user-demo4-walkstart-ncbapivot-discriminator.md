---
title: "P08 — USER_DEMO_4 walkstart ncbapivot discriminator"
description: "Bounded arbitration status for the first walking ncbapivot_entrypoint_3 update at Haunted Cavern glidx 39."
status: blocked
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-21
tags: [p08, user-demo-4, ncbapivot, walkstart, angrylion, rt64, blocked]
---

# USER_DEMO_4 walkstart ncbapivot discriminator

## Scope

Target: Haunted Cavern (`stage=0 sub=4`), consumed `glidx 39`: the first
walking `ncbapivot_entrypoint_3` output after corrected idle parity. `glidx 38`
is the matching predecessor; `glidx 40` is reserved only for temporal
disambiguation.

Angrylion RDP Plus nightly-build is an oracle only. TooieRecomp remains on
RT64.

## Current proven state

- `5854872c79a7cab1c9395ddcd173cece449ffe11` closes the bind-tick
  `round.w.s` semantic defect. It must not be reopened.
- Idle CA8B4/CA628 data through `glidx 38` is bit-identical against the pinned
  `ANGRYLION_CONTROL` reference oracle.
- At `glidx 39`, the established execution order places the first remaining
  output difference at `ncbapivot_entrypoint_3`; CA628, CA8B4, and E1A58 are
  faithful copiers of already-different state.
- The existing postfix capture does not contain every read/FPR/stack value
  consumed before that output. Therefore it cannot establish a
  same-input/different-output instruction join.

## This-run result

```text
BASE_HEAD:                         4f6b897c00c2353d77c409d2b65cec20de0cc4ed
BUILD:                             cmake --build build --target TooieRecompiled -j2
BUILD_EXIT:                        0
RT64_WRAPPER_RUN:                  reached stage=0/sub=1, then No threads left to run!
RT64_DIRECT_RUN:                   reached stage=0/sub=2 (glidx 656), timeout 180s
RT64_GLIDX39_CAPTURE:              NOT_CAPTURED in this fresh bounded run
ANGRYLION_STATUS:                  PASS (pinned nightly/RSP/CXD4, ROM SHA-1 verified)
ANGRYLION_FRESH_RUN:               reached stage=0/sub=2, no input-driven sub=4 window
ANGRYLION_GLIDX39_CAPTURE:         NOT_CAPTURED in this fresh bounded run
HISTORICAL_GLIDX39_ARTIFACTS:      PRESENT (see /tmp/p08-ud4-rt64-full.log and /tmp/p08-angrylion-user_demo4.csv)
TEMP_TARGET_FILTER_PROBE:          BUILT_AND_REVERTED (no tracked source change)
MARKDOWN_VAULT_AUDIT:              UNAVAILABLE (command not installed; local config binding present)
LOCAL_WIKILINK_CHECK:              155 files / 880 links / 3 pre-existing unresolved targets
NCBAPIVOT_WALK_READ_SET_COMPLETE:  NO
FIRST_EXECUTED_READ_DIFFERENCE:    UNKNOWN (writer postfix is insufficient)
CAMERA_ENTRY_STATE_COMPLETE:       NO
ENVIRONMENT_INPUTS_MATCH:          NOT_PROVEN (no fresh paired sub=4 join)
ROOT_CLASS:                        B-NCBAPIVOT-WALKSTART-OUTPUT (A-CANDIDATE only)
ROOT_CAUSE_LEVEL:                  NOT_PROVEN
FIX_IMPLEMENTED:                   NO
CAMERA_FEEDBACK_ACCUMULATION:      STILL_PRESENT
EGG_PARITY_DEFECT:                 PROVEN (inherited P08 evidence)
EGG_INDEPENDENT_FROM_REMAINING_CAMERA_STATE: UNKNOWN
PLAYER_SPEED_EVIDENCE_SCOPE_CORRECTED: YES (inherited P08 evidence)
COMMIT:                            3e9c799 (evidence checkpoint)
PUSHED:                            YES (origin/main)
NEW_HEAD:                          e2761d3
```

The canonical wrapper run still terminated in the runtime scheduler before the
Haunted Cavern window. A direct Lavapipe run progressed farther but timed out
in `stage=0/sub=2`; neither is a semantic result. The local historical files
listed above do show the expected `stage=0/sub=4` sequence (`glidx 36..38`
idle, `glidx 39` first walk), but they do not contain a complete executed
`ncbapivot` read/FPR/stack trace and are not treated as a fresh paired join.

## Required next capture

Run the existing env-gated diagnostic in a working RT64/Vulkan presentation
environment, and the paired Mupen64Plus Pure Interpreter + Angrylion RDP Plus
nightly-build + CXD4 LLE oracle capture. Join by stage/sub, consumed glrecord
index, and pad bytes. The capture must include the complete executed read set,
child-call inputs/outputs, and all required movement, animation, demo-state,
environment, and relevant AI fields for `glidx 36..42` before any instruction
semantic claim is made.

No production or generated file was modified by this investigation.
