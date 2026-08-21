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

The `800137AC` a2/f14 observation is **CORRECTED/SUPERSEDED** by the glidx 39
call-tree join below. It is not a live-input discriminator.

## glidx 39 800137AC provenance join (2026-08-21)

Joined from the existing paired capture (`/tmp/p08-rt64-display-glidx39-r2.log`,
`/tmp/p08-angrylion-ud4-cam.log`) plus generated bodies (read-only). No
production fix and no generated C edit.

Call names match through `800137AC` enter (compact event 057). First structural
mismatch is event 060: RT64 leaves `800137AC` then enters `800137C4`; the oracle
enters `800137C4` while `800137AC` is still the parent. That is **not** the first
semantic difference.

```text
CALL_TREE_STRUCTURALLY_ALIGNED:        YES through 800137AC enter
FIRST_CALL_COUNT_OR_ORDER_DIFFERENCE:  compact event 060 (RT L 800137AC vs AN E 800137C4)
EXTRA_80013818_ORACLE_CALL:            ALIGNMENT_ARTIFACT
```

The extra oracle `80013818` is nested under `800137F4` (`jal 0x80013818`,
`ra=80013810`). RT64's host interposition `func_800137F4` in
`src/game/boot_hw_stubs.cpp` calls `func_80013818` directly (not
`LOOKUP_FUNC`), so the wrap does not emit a nested np-call. The helper still
runs.

### 800137AC executed read set

Generated `func_800137AC` (`RecompiledFuncs/funcs_134.c`):

1. `lwc1 $f14, 0x16C8($at)` from `0x800416C8` (overwrites entry `f14`)
2. `mtc1` 0x43B40000 into `$f16` (360.0)
3. tail `b 0x80013818` via `LOOKUP_FUNC`

```text
800137AC_A2_USED_BY_CHILD:             NO
A2_SET_PC:                             not consumed; 800EEBF0 lw a2, 4(saved a1) then 800EF214 sw a2, 0x30(sp)
A2_SOURCE:                             dead at 800137AC; live later as 800137F4 f12
A2_SOURCE_BITS_RT:                     439a589e
A2_SOURCE_BITS_AN:                     439a7fd5
FIRST_A2_PROVENANCE_DIFFERENCE_PC:     not a 800137AC discriminator
800137AC_F14_USED_BY_CHILD:            NO (entry value overwritten at 0x800137B0)
F14_LAST_WRITER_PC:                    0x800137B0 lwc1 from 0x800416C8
F14_SOURCE_BITS_RT:                    enter 41aa94a2 (stale); 80013818 enter 3b360b61
F14_SOURCE_BITS_AN:                    enter 439a054e (stale); 80013818 enter 3b360b61
FIRST_F14_PROVENANCE_DIFFERENCE_PC:    entry f14 is ABI-stale; post-overwrite MATCH
800137AC_EXECUTED_READ_SET:            MEM_W(0x800416C8); then 80013818 uses f12 and the new f14
TRUE_LIVE_INPUTS:                      f12 from 800EF214 mtc1 a1 (41aa94a2 MATCH); f14 from 0x800416C8 (MATCH at 80013818 enter)
DEAD_ENTRY_VALUES:                     a2, entry f14
```

`80013818` enter after that stub: `f12=41aa94a2` and `f14=3b360b61` on **both**
sides. `a2` is used by `800EF214` only after `800137AC`/`800137C4`, as `f12` of
`800137F4`.

### First control-flow difference: 80112C3C jumptable

First non-pointer child-output difference is `80112C3C` leave. Enter values
match (`a1=4399de17`, `a2=9`, `f12=411ed8c8`, `f14=41aa94a2`). Leave:

| field | RT64 | ANGRYLION |
| --- | --- | --- |
| f0 | 4399de17 (=a1, unchanged path) | 439a054e |
| f12 | 411ed8c8 (unchanged) | 00000000 |
| f14 | 41aa94a2 (unchanged) | 439a054e |
| a2 | 00000009 (unchanged) | 43b0854e |

Generated `func_80112C3C`: `v0 = *(a0+0x60)`; `t7 = *(v0+8) - 1`; `sltiu $at, t7, 5`;
`beq $at, $zero, L_80112CF4` (return `f0 = a1`). Oracle FPU inside this wrap is
`80013734` (helper `800F1738`, jumptable case 4). RT64 never enters that helper:
leave GPRs/FPRs equal enter.

The selector is already different at **ncbapivot enter**, before any child:

| glidx | RT p60_0,p60_4,p60_8 | AN p60_0,p60_4,p60_8 |
| --- | --- | --- |
| 36..38 enter/leave | 0,0,0 | 0,0,0 |
| 39 enter | 0,0,0 | 439a054e, 43b0854e, 5 |

`p60_8=0` → unsigned `t7=0xffffffff`, `sltiu 5` fails, skip jumptable.
`p60_8=5` → `t7=4`, case 4 → `L_80112C78` → `jal 0x800F1738`.

Player pos/yaw/ideal at glidx 39 enter MATCH
(`pos_bits=c2bd3182,36800000,c4e82e39`, `yaw=43265555`, `ideal=43801ef0`).
`n74`/`n80`/`tgt` still idle-match at enter. Animation ID/phase/frame/rate are
**not** in this dump.

```text
LAST_MATCHING_EXECUTED_PC:             80112C58 sltiu $at, $t7, 5
FIRST_DIFFERENT_CONTROL_FLOW_PC:       80112C5C beq $at, $zero, L_80112CF4
GUEST_OPCODE:                          10200025 (beq)
BRANCH_INPUT_BITS_RT:                  p60_8=00000000 t7=ffffffff sltiu5=0
BRANCH_INPUT_BITS_AN:                  p60_8=00000005 t7=00000004 sltiu5=1
BRANCH_RESULT_RT:                      taken → L_80112CF4, f0=a1
BRANCH_RESULT_AN:                      not taken → jr table case 4 → 800F1738
PLAYER_PRECAMERA_STATE_MATCH:          PARTIAL (pos/yaw/ideal MATCH; node+0x60 object DIFF)
PLAYER_ANIMATION_FULL_STATE_CAPTURED:  NO
```

### FCSR

`recomp_context` has no guest FCR31 (only `mips3_float_mode` / host
`fegetround()`). Env-gated RT64 dumps now print `fcr31=UNEMULATED` plus RM /
mips3 / CP0 status. Oracle already dumps full `fcr31`.

```text
RT64_FULL_FCSR_CAPTURED:               NO (guest FCR31 not stored; dump is RM/mips3/status only)
FCSR_RT_ENTRY:                         rm=0 mips3=0 (UNEMULATED C/FS/Cause/Enable)
FCSR_AN_ENTRY:                         01000800  RM=0 FS=1 EnableV=1 C=0
FCSR_AN_80112C3C_LEAVE:                01800800  RM=0 FS=1 EnableV=1 C=1
FCSR_DIFFERENCE:                       oracle C bit set on the taken 800F1738 path
FCSR_DIFFERENCE_CAUSALLY_READ:         NO
```

`80112C5C` is integer `beq` on `sltiu`. It does not read FCSR. Oracle C is a
**consequence** of the jumptable helper (`c.lt.s` / COP1 compare), not the
cause of the split.

### Classification

```text
FIRST_PROVEN_DISCRIMINATOR:            node+0x60 object word +8 at glidx 39 ncbapivot enter (0 vs 5); 80112C5C uses it
ROOT_CLASS:                            B-NCBAPIVOT-CONTROL-INPUT
A_CANDIDATE:                           NOT_PROVEN
ROOT_CAUSE_LEVEL:                      PROVEN for the first split inside this ncbapivot; p60 writer is outside it
FIX_IMPLEMENTED:                       NO
ROUND_W_S:                             CLOSED
CAMERA_FEEDBACK_ACCUMULATION:          STILL_PRESENT
EGG_PARITY_DEFECT:                     PROVEN
EGG_INDEPENDENT_FROM_REMAINING_CAMERA_STATE: UNKNOWN
P08_STATUS:                            ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                            PENDING / NOT_FORMALLY_OPEN
```

The producer of `*(node+0x60)+8` is **not** inside this glidx 39 ncbapivot:
glidx 38 leave is `p60_8=0` on both; glidx 39 enter already differs. Next
bounded action is that writer between those two ncbapivot calls. No semantic
fix.

For a clean rerun by another agent, use the operational checklist in
[[p08-walkstart-reproduction-runbook]].
