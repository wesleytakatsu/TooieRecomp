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
COMMIT:                 pending checkpoint
PUSHED:                 pending checkpoint
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
