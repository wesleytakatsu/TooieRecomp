---
title: "P08 — USER_DEMO_4 walkstart ncbapivot discriminator"
description: "Bounded arbitration status for the first walking ncbapivot_entrypoint_3 update at Haunted Cavern glidx 39."
status: active
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-21
tags: [p08, user-demo-4, ncbapivot, walkstart, angrylion, rt64, manual-validation-pending]
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

## Historical blocked run (superseded by fresh capture below)

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
NEW_HEAD:                          origin/main (verified at handoff)
```

The canonical wrapper run still terminated in the runtime scheduler before the
Haunted Cavern window. A direct Lavapipe run progressed farther but timed out
in `stage=0/sub=2`; neither is a semantic result. The local historical files
listed above do show the expected `stage=0/sub=4` sequence (`glidx 36..38`
idle, `glidx 39` first walk), but they do not contain a complete executed
`ncbapivot` read/FPR/stack trace and are not treated as a fresh paired join.

## Historical required next capture (superseded)

Run the existing env-gated diagnostic in a working RT64/Vulkan presentation
environment, and the paired Mupen64Plus Pure Interpreter + Angrylion RDP Plus
nightly-build + CXD4 LLE oracle capture. Join by stage/sub, consumed glrecord
index, and pad bytes. The capture must include the complete executed read set,
child-call inputs/outputs, and all required movement, animation, demo-state,
environment, and relevant AI fields for `glidx 36..42` before any instruction
semantic claim is made.

No production semantic fix or generated file was modified; source edits are
env-gated diagnostics only.

## Fresh paired glidx 39 walkstart capture (2026-08-21)

The previously blocked capture was completed on the physical X11/RADV path.
Both runs used `DISPLAY=:0`, the AMD RX 580 RADV ICD, and a timeout of 720 s;
no Lavapipe or headless presentation was used. The diagnostic gate was enabled
only with `TOOIE_UD4_WALKSTART=1` (without `TOOIE_UD4_FIRST_TICK`); no
production semantic fix or generated code was changed, and source edits are
env-gated diagnostics only.

```text
RT64_LOG:       /tmp/p08-rt64-display-glidx39-r2.log
ORACLE_LOG:     /tmp/p08-angrylion-display-glidx39-r2.log
ORACLE_TRACE:   /tmp/p08-angrylion-ud4-cam.log
ORACLE_CSV:     /tmp/p08-angrylion-user_demo4.csv
RT64_RESULT:    stage=0/sub=4; 7 ncbapivot enters + 7 leaves (glidx 36..42); progressed to sub=5
ORACLE_RESULT:  stage=0/sub=4; glidx 39 TRACE_ON + leave captured; CSV window reaches glidx 638
PAD_JOIN:       glidx 38..42 match exactly by stage/sub, consumed glrecord, and pad bytes
```

The glidx 39 entry data match after pointer relocation: `node+0x74`,
`node+0x80`, `tgt`, selected-record payload, `eye_in`, `src2_in`, position,
yaw, ideal-yaw, `f0`, and `f12` are equal. The ncbapivot output is not equal:
RT64 writes `f0=43a958bd`, `node+0x74.x=c43e67de`, and
`node+0x80.x=43a958bd`; the oracle writes `f0=43a9580d`,
`node+0x74.x=c43e5a36`, and `node+0x80.x=43a9580d`.

This is not yet a complete same-inputs/different-output instruction join.
RT64 exposes only `rm=0`/`mips3=0` in this diagnostic, while the oracle exposes
`fcr31=01000800` at entry and `01800800` at leave. The first observed child
input divergence is at `800137AC` enter: RT64 has `a2=439a589e`,
`f14=41aa94a2`; the oracle has `a2=439a7fd5`, `f14=439a054e`. The call-stack
order also diverges at event 60 (RT64 leaves `800137AC` before `800137C4`;
the oracle keeps it active). Both `800137F4` and `80013818` execute, with one
additional `80013818` invocation in the oracle trace.

```text
NCBAPIVOT_WALK_READ_SET_COMPLETE:  NO (RT64 full FCSR/FPR provenance not exposed)
FIRST_EXECUTED_READ_DIFFERENCE:    NOT_PROVEN; earliest observed child-input difference is 800137AC enter (a2/f14)
SAME_INPUTS_DIFFERENT_NCBAPIVOT_OUTPUT: NO / NOT_PROVEN
ROOT_CLASS:                        B-NCBAPIVOT-WALKSTART-FIRST-EXECUTED-INPUT
ROOT_CAUSE_LEVEL:                  NOT_PROVEN
FIX_IMPLEMENTED:                   NO
P08_STATUS:                        ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                        PENDING
```

This evidence is diagnostic-only. Do not reopen `round.w.s`, camera feedback,
generated recompiler output, or the egg path. The next bounded action is to
capture the missing RT64 FCSR bits and the executed read provenance around the
`800137AC` inputs; no semantic fix is authorized by this result.
