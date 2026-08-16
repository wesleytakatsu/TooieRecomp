---
title: "P08 — independent review of chdiggerground reverse facing"
description: "Independent semantic review and one narrow runtime discriminator for the Wooded Hollow chdiggerground return state. No production fix implemented."
status: evidence
phase: P08
updated: 2026-08-16
tags: [tooie-recomp, p08, forced-facing, cinematic, chdiggerground, sucoaster, independent-review]
---

# P08 — FORCED FACING INDEPENDENT REVIEW

## Review boundary

```text
BASE_HEAD:       a68e8a9d047098d51ef8b5bfda875b319c9b5b2f
MODEL:           GPT-5.6 Luna
REASONING:       X-HIGH
SELECTED_ACTOR:  Wooded Hollow gsattract mode 5, overlay 219 chdiggerground
SCOPE:           one actor; return transition; one node-selection discriminator
FIX_IMPLEMENTED: NO
```

The prior Level B / `TOOIE_NATIVE` conclusion is preserved historically in
[[p08-forced-cinematic-facing-diagnosis]], but it is not binding on this
review. The exact 180° visual/logical difference does not establish that the
actor is required to face its return travel.

## Semantic conclusion

```text
RETURN_STATE:                                0x98bb1942
RETURN_ANIMATION:                            0xc3aa1065 at transition,
                                             0xc3ab8000 during sustained return
RETURN_MOVEMENT_INTENTION:                   UNKNOWN
ORIGINAL_EXPECTED_FACING_RELATIVE_TO_TRAVEL: UNKNOWN
USER_BUG_B_THIS_ACTOR:                       UNKNOWN
USER_BUG_B_GLOBAL:                           OPEN
ROOT_CAUSE_LEVEL:                            C
```

The selected actor changes from outbound state `0x98bb1941` to return state
`0x98bb1942`. Its animation also changes (`0xc3a74406`/outbound samples to
`0xc3aa1065`, then `0xc3ab8000`). That proves a real state/animation
transition, but animation identity alone does not prove `FORWARD_WALK` or
`BACKWARD_WALK`; no original state-table contract was found that authorizes
that inference.

The first reverse sample in the current diagnostic run was approximately:

```text
tick:       56050
position:   (-2548.28, -326.75, 980.36)
travel:     301.80 degrees
yaw/unk54:  114.21 degrees
delta:      172.41 degrees
state:      0x98bb1942
animation:  0xc3aa1065
```

The later stable sample reproduces the canonical exact difference:

```text
travel:     294.21 degrees
yaw/unk54:  114.21 degrees
delta:      180.00 degrees
```

Therefore the correct semantic answer is not “the recomp is wrong” and not
“the actor is intentionally backward” yet. The original contract remains
`UNKNOWN`.

## Narrow runtime discriminator

The requested breakpoint was placed at the return from
`func_8080264C_sucoaster` (`+0x52`) and the downstream
`func_8010CD50` path. It did not produce a target-attributed call for the
selected actor. Guest actor addresses are allocation/run dependent; the
hard-coded address from an earlier diagnostic run was not stable, while the
facing probe did reach the same logical scene and transition.

```text
NODE_INDEX_ON_LAST_OUTBOUND_TICK:  UNKNOWN
NODE_INDEX_ON_FIRST_REVERSE_TICK:  UNKNOWN
FUNC_80802760_RETURNS:              UNKNOWN for selected actor
NODE_CASE:                          OTHER (selected call not captured)
NODE_POINT:                         UNKNOWN
ACTOR_POS:                          captured by facing probe; see samples above
NODE_HEADING:                       UNKNOWN
TRAVEL_HEADING:                     301.80 first reverse sample;
                                    294.21 stable reverse sample
```

This means neither Case A (`-1`, keep old yaw) nor Case B (live stale node)
is proven. No node index, path point, or node heading may be used to select a
fix.

## State and node trace

```text
NODE_SELECTION_INPUTS:  sucoaster/path object passed into
                        func_80802760; its current/next node, direction,
                        path parameter, endpoint, and reverse fields were
                        not captured for the selected call
EXPECTED_NODE_SELECTION: UNKNOWN — original return contract not established
ACTUAL_NODE_SELECTION:   UNKNOWN — selected-call v0 was not captured
FIRST_NODE_STATE_DIVERGENCE: UNKNOWN
FIRST_BAD_STAGE:         UNKNOWN
FIRST_BAD_FUNCTION:      UNKNOWN
FIRST_BAD_GUEST_PC:      UNKNOWN
EXPECTED_VALUE:          UNKNOWN
ACTUAL_VALUE:            yaw 114.21 degrees versus travel 294.21 degrees
                         in the stable reverse sample
```

The calibration did establish an important ownership boundary: the helper's
input is the sucoaster/path object, while `func_8010CD50` later receives the
actor pointer and the helper's returned index. That is not enough to claim
that the actor fields or `func_8080264C` are the first bad writer.

## Original-MIPS / generated-body review

```text
FUNC_8080264C_GENERATED_MATCH: YES
FUNC_8010CD50_GENERATED_MATCH:  YES
FUNC_80802760_GENERATED_MATCH:  UNKNOWN
```

`func_8080264C_sucoaster` preserves the helper call, `-1` comparison, live
index handoff, and the delay-slot load that retains the prior yaw. The
generated `func_8010CD50` preserves the node-point lookup followed by
`func_800F1DF4` heading calculation and its delay-slot argument setup.
Those two bodies therefore do not provide evidence for a native override.

A complete original-MIPS side-by-side verification of `func_80802760` was not
available in the indexed workspace during this review, and the selected
runtime call did not yield its return value. It is consequently reported as
`UNKNOWN`, not as a presumed generator defect.

```text
ORIGINAL_REVERSE_FACING_MECHANISM: UNKNOWN
FIX_OWNER_CANDIDATE:               UNKNOWN
MINIMAL_FIX_FILES:                 NONE — review only
```

No +180°, node override, generated-C edit, transform tag, native override, or
P08-T10 work is justified by this evidence.

## Differential and controls

```text
OUTBOUND_STATE:        0x98bb1941
OUTBOUND_ANIMATION:    c3a74406 (sampled)
OUTBOUND_HEADING/YAW:  approximately 110.54 / 110.54 degrees

FIRST_REVERSE_STATE:        0x98bb1942
FIRST_REVERSE_ANIMATION:    c3aa1065
FIRST_REVERSE_HEADING/YAW:  301.80 / 114.21 degrees

REVERSAL_STATE_CHANGE:  0x98bb1941 -> 0x98bb1942
NODE_CHANGED:           UNKNOWN
YAW_TARGET_CHANGED:     YES — observed value changes; node causality unknown
ANIMATION_CHANGED:      YES
```

```text
SIBLING_REVERSE_BEHAVIOR_AVAILABLE: NO
LOGICAL_VS_RENDER_CONFIRMED:        YES
```

The existing diagnostic equality `Actor.unk54 == Actor.rotation.y == visible
model direction` remains valid. Matrix/render submission is therefore closed
for this review. Sibling actors were not used as a reverse-state control;
their outbound movement/facing behavior cannot establish this actor's return
contract.

## Validation and handoff

```text
STICK_ANGLE_TEST:       PASS
MATRIX_SUBMISSION_TEST: PASS
INPUT_DIRECTION_TEST:   PASS
BCF28_TEST:             PASS
CORE1_RA_TEST:          PASS
TOOIE_BUILD:            PASS
MARKDOWN_VAULT_AUDIT:   PASS (repository-local audit; see checkpoint)
COMMIT:                 b19bcf9 (review checkpoint)
PUSHED:                 YES
CHECKPOINT_NEW_HEAD:    b19bcf9
WORKTREE:               documentation-only changes; no production-code fix
```

The runtime discriminator is the remaining evidence gap. If the user later
confirms this exact actor differs from original, the next review must capture
the selected call without relying on a stale heap address. Otherwise, select
a different concrete forced/cinematic actor whose original and recomp views
are side-by-side confirmed.

```text
NEXT_SINGLE_ACTION: run only the selected-call discriminator again with a
                    stable actor identity, or choose a different user-confirmed
                    forced/cinematic mismatch; do not implement a fix here
```

## SELECTED-CALL DISCRIMINATOR — DeepSeek V4 Flash Free / OpenCode Zen

```text
BASE_HEAD:          337f6dd0a7c82a6eadf3a5a162dad6d0c165c877
MODEL:              DeepSeek V4 Flash Free (opencode/deepseek-v4-flash-free)
MODE:               narrow dynamic runtime capture, this-run actor identity
FIX_IMPLEMENTED:    NO
```

### Method

A strong-symbol diagnostic override of `func_80802760_sucoaster` was compiled
into the TooieRecompiled target (`src/game/p08_dig_capture.cpp`, env-gated on
`TOOIE_DIAG_FORCED_FACING=1`). `func_80802760` is the path helper shared by
every sucoaster-overlay state handler; it returns either a live path-node
index (`B_LIVE_NODE`) or `-1` (`A_KEEP_OLD`). The override replicates the
generated body instruction-for-instruction (addiu/sw $ra; lw v0,0x18(a0);
sll t7,v0,12; bgez → 0x8008A240(v0>>28)+0x8010D290(MEM_W(v0+0x3C)), else
0x800F54E4), records `v0` before restoring it, and does not perturb guest
state. All 8 call sites of `func_80802760` (grep-verified) resolve to the
strong symbol, so **every** invocation in the run is captured — no stale heap
address is used; the actor is identified by its sucoaster/path pointer, its
state `0x98bb1941`/`0x98bb1942`, and its position in the known reversal
region `x≈-2530..-2550, z≈972..980`.

```text
CURRENT_RUN_ACTOR_POINTER:     0x801c63ec -> 0x801c615c (dig6; same logical
                               instance, realloc after state flip)
CURRENT_RUN_SUCOASTER_POINTER: path 0x8020fef0 (digger path, outbound AND
                               return leg), also 0x8020fc40 in return window
IDENTITY_METHOD:               dynamic this-run (actor sys 0x80085C98
                               chdiggerground, state 0x98bb1941/42, reversal-
                               region position, digger path pointer); no
                               fixed historical address
```

### Result — all 3438 captured calls across the whole run

```text
TOTAL_FUNC_80802760_CALLS:     3438 (dig4/dig5/dig6 runs, consistent)
FUNC_80802760_RETURNS:         0  (node index 0)  — 3438 / 3438 calls
A_KEEP_OLD (-1) OCCURRENCES:   0  (never)
NODE_CASE:                     B_LIVE_NODE
NODE_INDEX:                    0
```

Every invocation of `func_80802760` in the run — outbound leg, the
`0x98bb1941 -> 0x98bb1942` state flip, and the entire sustained return leg
(state `0x98bb1942`, gfx ticks ≈54356..56150 and beyond, path
`0x8020fef0`) — returns node index **0**, never `-1`. Sample:

```text
[dig02760] tick=54805 path=8020fef0 path18=10c00000 v0=0 node_case=B_LIVE_NODE
[dig02760] tick=55116 path=8020fef0 path18=10c00000 v0=0 node_case=B_LIVE_NODE
... (all subsequent return-leg samples identical: v0=0)
```

### Case split resolution

```text
CASE_A_KEEP_OLD (v0 == -1):    REFUTED — zero occurrences in 3438 calls
CASE_B_LIVE_NODE (v0 != -1):   PROVEN — node index 0 on every call,
                               including the entire return leg
```

Because `func_80802760` always returns node 0 (not `-1`), the `keep old yaw`
branch of `func_8080264C_sucoaster` (funcs_241.c:3772 `if (v0 != -1)`
hand-off to `func_8010CD50`) is **never** the source of the frozen 114.21°:
during the outbound leg `func_8080264C` always hands node 0 to
`func_8010CD50`, and during the return leg `func_8080264C` is not called at
all — the return leg runs a different handler (see below) that moves the
digger back along the path without updating `Actor.rotation.y`/`unk54`.

### Return-leg dispatch identification

```text
STATE_0x98BB1942_DISPATCH_KEY: u72 = 0x3008   (actor+0x72, halfword)
DISPATCH_CASE:                 (0x3008 >> 10) - 1 = 11 -> L_80800A3C
RETURN_LEG_HANDLER:            func_8080027C_sucoaster case 11
                               (funcs_151.c:4683) -> sucoaster_entrypoint_19,
                               func_80101DDC, func_80107070
FUNC_8080264C_CALLED_ON_RETURN: NO (never called after state flip)
```

The facing probe captured `u72=0x3008` exclusively in state `0x98bb1942`
(`[facing] ... u72=3008 ...`), i.e. the return leg is dispatched to case 11,
whose chain does **not** call `func_8080264C`. The digger continues to query
`func_80802760` on its path in the return window and gets node 0 every time,
but no path-yaw writer updates `unk54`/`rotation.y` to the 294.21° travel
heading — hence the exact 180° freeze.

### Node-point note (not asserted)

The diagnostic also dumped `node_pt` from the `func_800F5A00` table
(`0x8013<<16 + 0x5490 + node*4`, matching funcs_393.c:5392). For node 0 the
raw slot was read, but the resolved point goes through
`func_800F3BB0 -> func_8009C128`, which the capture did not call; the
dumped `pt=` bytes are therefore NOT asserted as the node position.

```text
NODE_POINT:        NOT ASSERTED (resolved via func_8009C128; not captured)
NODE_HEADING:      NOT ASSERTED
TRAVEL_HEADING:    294.21° stable return sample (previous review)
```

### Original-MIPS / generated review

```text
FUNC_80802760_GENERATED_MATCH:  YES — funcs_262.c:2679 body is structurally
                                faithful: bgez on bit-19 sign of path+0x18,
                                delay slots present (nop / srl / lw), two
                                sub-branches (0x800F54E4 vs
                                0x8008A240+0x8010D290), return v0. No ROM/
                                original-MIPS disassembly available in the
                                workspace to disassemble 0x80802760 directly
                                (no .z64/.elf present); classification is
                                based on generated-body structure + runtime
                                return value, not an external MIPS dump.
ORIGINAL_REVERSE_FACING_MECHANISM: UNKNOWN — no original dump to prove
                                whether the original keeps yaw on the
                                outbound node during return travel.
RETURN_MOVEMENT_INTENTION:      OTHER — position integrates backward
                                (294.21°) while yaw/unk54 stay frozen at the
                                last outbound value (114.21°); the return-leg
                                handler (case 11) does not write unk54.
ORIGINAL_EXPECTED_FACING_RELATIVE_TO_TRAVEL: UNKNOWN (180° observed, but no
                                original evidence that the intended facing is
                                0° relative to the return travel).
```

### Decision

```text
USER_BUG_B_THIS_ACTOR:  UNKNOWN — not PROVEN, not REFUTED. The recomp runs
                        the original path helper faithfully (node 0 always)
                        and the return-leg case-11 handler moves the digger
                        back without updating unk54/rotation.y. Whether the
                        original BT code does the same (i.e. "back into the
                        burrow" is intended) or updates yaw cannot be decided
                        without an original-MIPS disassembly of the return
                        leg. No ROM/ELF is present in this workspace.
USER_BUG_B_GLOBAL:      OPEN
ROOT_CAUSE_LEVEL:       C — one discriminator remains: original return-leg
                        MIPS (does it write unk54 on state 0x98bb1942?).
                        Case A (func_80802760 returning -1) is now REFUTED.
FIX_OWNER_CANDIDATE:    NONE — no fix justified by this evidence
FIX_IMPLEMENTED:        NO
```

### Tests / validation

```text
STICK_ANGLE_TEST:       PASS
MATRIX_SUBMISSION_TEST: PASS
INPUT_DIRECTION_TEST:   PASS
BCF28_TEST:             PASS
CORE1_RA_TEST:          PASS
TOOIE_BUILD:            PASS
MARKDOWN_VAULT_AUDIT:   PASS
COMMIT:                 (docs-only checkpoint; see handoff)
PUSHED:                 YES
WORKTREE:               documentation-only; diagnostic override reverted
```

The diagnostic override (`src/game/p08_dig_capture.cpp`, CMakeLists entry,
`u72` probe in `syscall_handler.cpp`) was **reverted** before this checkpoint.
Runs consistently end in the pre-existing later-attract SIGSEGV (EXIT=139),
which is not facing evidence.
