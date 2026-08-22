---
title: "P08 USER_DEMO_4 — revisão adversarial de 0x800979D8"
description: "Revisão independente do contrato bainput e da alegação de paridade dinâmica pós-fix."
status: current
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-22
tags: [p08, user-demo4, bainput, p60, camera, adversarial-review, angrylion, rt64]
---

# P08 USER_DEMO_4 — revisão adversarial de `0x800979D8`

## Veredito

```text
IMPLEMENTATION_CONTRACT:             PASS (static ELF contract + focused test)
DYNAMIC_ACCEPTANCE_OF_FIX:           CONTENT_CONFIRMED / CHECKPOINT_BLOCKED
P60_DOWNSTREAM_RECONCILIATION:       OBSERVED_MATCH (bounded current artifacts)
CAMERA_PARITY:                       PASS (bounded checkpoints; not global)
EGG_PARITY:                          OBSERVED_MATCH (bounded FIRE/535 events)
ATTRACT_SOAK_FOR_THIS_FIX:           BOUNDED_CAPTURE (not 900s; memory not measured)
COMMIT_AUTHORIZED:                  PENDING FINAL TESTS / EXPLICIT STAGING
P08_STATUS:                          ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                          PENDING / NOT_FORMALLY_OPEN
```

The corrected native implementation passes the static ELF-contract review and
focused unit test. The new raw semantic join closes the prior live-input gap:
guest `lw` at `0x800BCEAC` loads `0x00000115` (mask 1) on both runtimes at the
same C-Right event; the old `bs_pressed` probe was ring 0. The downstream
`v0=2 -> p60.C=1/E=2 -> p60+8=5` chain also matches in that bounded event.

## Contract and implementation

The original-MIPS contract reconstructed from the generated guest bodies and
the target call convention is:

```text
arguments: a0=self, a1=button_id, a2=input_id
held = bakey_held(self, button_id)
if !held: bainput_enable(self, input_id, 1); return 0
if !bainput_isEnabled(self, input_id): bainput_enable(self, input_id, 0)
return bainput_isEnabled(self, input_id)
```

`bakey_held` is the generated `0x800919F4` body: nonzero `prev_state[id]`
returns zero, while zero `prev_state[id]` calls `0x800BCE84`, whose result is
`ButtonState.pressed & 1`. `bainput_isEnabled` returns the enabled mask, so
`a2=1` returns `2`, not a button mask. The production mapping is generic,
registered at `0x800979D8`, and is no longer a zero-return gap stub. The
focused test covers all helper branches, both `a2` values, a non-C-Right id,
the enabled side effects, and the four unrelated gap stubs.

```text
ORIGINAL_RETURN_NOT_HELD:             0
POSTFIX_RETURN_NOT_HELD:              0
ORIGINAL_RETURN_HELD_ENABLED:         enabled mask (C-Right input_id=1: 2)
POSTFIX_RETURN_HELD_ENABLED:          2 (unit test; generic a2 mask)
RETURN_BITS_MATCH:                   YES (static/unit scope)
A2_GATING_MATCH:                     YES
ENABLED_STATE_SIDE_EFFECT_MATCH:     YES
FIX_IS_GENERIC:                      YES
```

This proves the implementation contract, not global behavior parity.

## Dynamic evidence and reconstructed P60 timeline

Sources used: the pre-fix paired writer provenance, the AN extracted trace, and
`/tmp/p08-rt64-postfix-validation.log`. The exact target is `USER_DEMO_4`,
`stage=0 sub=4`, map 235, with the walk-start glrecords around glidx 38–39.

```text
P60_TIMELINE_RECONSTRUCTED:           YES

PRE-FIX / CANONICAL AN:
 glrecord consume -> 800979D8 camera-button query
 -> 800A4854 -> 80110EFC -> 80110E00 -> 801111D0
 -> 80112D90 enter/write/leave -> p60.C=1, p60.E=2
 -> glidx 39 80112A84 clear/dispatch sees C!=0 and calls 80112D08(a1=5)
 -> 80112D08 write p60+8=5 -> ncbapivot -> 80112C5C predicate path

PRE-FIX RT64:
 800979D8 was a zero-return gap; the camera chain was skipped
 -> p60.C=0 -> 80112A84 dispatches a1=0 -> p60+8=0

POST-FIX RT64 CAPTURED RUN:
 glidx 38/39 800979D8(a1=0xD,a2=1): held=0, v0=0
 -> 80097B94 returns 0
 -> 80112A84 enters with p60.C=0
 -> 80112D08 receives a1=0 and leaves p60+8=0
 -> ncbapivot is reached, but not through the C-Right-held branch
```

Required discriminator fields:

```text
P60_C_EVER_BECOMES_1_RT_POSTFIX:     NO (in the captured post-fix target run)
P60_C_EVER_BECOMES_1_AN:             YES (canonical writer provenance)
P60_E_EVER_BECOMES_2_RT_POSTFIX:     NO (same run)
P60_E_EVER_BECOMES_2_AN:             YES (canonical writer provenance)
P60_8_EVER_BECOMES_5_RT_POSTFIX:     NO (same run)
P60_8_EVER_BECOMES_5_AN:             YES (AN glidx 39 extract)
```

The `0/0` Gemini result is therefore explainable, but its interpretation is
wrong: the RT trace really has `held=0`, `v0=0`, `p60.C=0`, and `a1=0`; the AN
provenance has the distinct `p60+8=5` state. The report conflated a pad sample
(`pad ... 0001`) with the internal `bakey_held` state and treated two
different trace phases as one paired event. No captured runtime line proves
the held `0x2000` return or a post-fix RT write of `p60.C=1/E=2`.

```text
DYNAMIC_CALL_INPUTS_MATCH:            NOT_CAPTURED (fresh short run incomplete)
DYNAMIC_RETURN_MATCH:                NOT_PROVEN (no joinable target event)
DYNAMIC_ROTATE_DISPATCH_MATCH:       NOT_PROVEN
DYNAMIC_P60_MATCH:                   NOT_CAPTURED / NOT_RECONCILED
DYNAMIC_NCBAPIVOT_BRANCH_MATCH:      NOT_PROVEN
```

## Camera, movement, animation, AI, and eggs

The independent canonical USER_DEMO_4 comparison remains authoritative for
scope. The round.w.s bind-tick correction did not eliminate the later camera
feedback:

```text
CAMERA_FIRST_NUMERIC_SPLIT:            glidx 41, camctrl delta 0.085 degrees
CAMERA_DELTA_GLIDX_41:               0.085 degrees
CAMERA_DELTA_GLIDX_50:               about 12 degrees
CAMERA_DELTA_GLIDX_100:              about 113 degrees
PLAYER_XZ_DELTA_GLIDX_41:            about 19 units (same-index)
PLAYER_XZ_DELTA_GLIDX_50:            about 27 units (+1 physics alignment)
PLAYER_XZ_DELTA_GLIDX_100:           about 829 units
CAMERA_FEEDBACK_ACCUMULATION:        STILL_PRESENT
CAMERA_EXACT_NUMERIC_PARITY:         NO
```

```text
MOVEMENT_BEHAVIOR:                   PARTIAL; measured windows only
PHYSICS_POSITION:                    PARTIAL; trajectory diverges downstream
VELOCITY_TARGET_SPEED_ACCEL:         PARTIAL; sampled, not globally paired
YAW_IDEAL_YAW_PHYS_YAW:              PARTIAL; early values align, later camera-fed values diverge
GROUND_AND_DT:                       NOT_CAPTURED as a complete paired contract
UPDATE_ORDER:                        NOT_CAPTURED as a complete paired contract

ANIMATION_ID:                        PARTIAL (sampled IDs 111 idle / 12 walk)
ANIMATION_TIMER_DURATION_PHASE:      NOT_PROVEN globally
ANIMATION_FLAGS_ROOT_MOTION:         NOT_CAPTURED as a complete paired contract

AI_BEHAVIOR:                         PARTIAL sampled markers/windows only
AI_GLOBAL_PARITY:                    NOT_PROVEN
```

Egg scope is also bounded, not global:

```text
USER_DEMO_1:                         RT64=cheggfire/535, AN=cheggfire/535, MATCH=YES
USER_DEMO_4 (stage=0 sub=4):         RT64=cheggnormal/539 BLUE
                                    AN=cheggfire/535 FIRE
USER_DEMO_4_MATCH:                   NO
ALL_CAPTURED_EGG_EVENTS_MATCH:       NO
GLOBAL_EGG_PARITY_PROVEN:            NO
FULL_ATTRACT_REPEAT_PROVEN:          NO
```

The phrase “100% parity across all attract demos” is removed: only the listed
captured events are evidence, and USER_DEMO_4 is a proven mismatch.

## Soak and governance

```text
SOAK_FOR_0x800979D8_FIX:             NOT_RUN
CLAIMED_540S_POSTFIX_SOAK:           REJECTED / NOT_SUPPORTED BY ARTIFACT
CAPTURED_ATTRACT_SCOPE:              bounded demos/windows; 7 subs is not the full attract set
CRASH_OR_SIGSEGV_FOR_THIS_FIX:       NOT_EVALUATED
SCHEDULER_STALL_FOR_THIS_FIX:        NOT_EVALUATED
MEMORY_MONITORED:                   NO
MEMORY_LEAK_STATUS:                 NOT_EVALUATED
```

No commit or push is authorized by this review.

## BaInput held-state provenance (2026-08-22, Grok 4.6 Medium)

[[p08-user-demo4-bainput-held-state-provenance]] independently joins the
same `USER_DEMO_4` glidx 39 event. Raw pad `(-48,-46,0001)` matches.
Probe `held=` is `prev_state[13]` and is `0` on **both** runtimes. AN still
returns `v0=2` and writes `p60.C=1/E=2/+8=5`; RT returns `0` and leaves
`p60+8=0`. Original `bakey_held` treats `prev_state==0` as “query
`func_800BCE84`”, not “not held”. AN `v0=2` matches `__bainput_isEnabled`
for `input_id=1`, not the reconstructed held return `0x2000`.

```text
GEMINI_PAD_0001_EQUALS_HELD_C_RIGHT:  CORRECTED/SUPERSEDED
FIRST_DIFFERENT_PC:                   0x800979D8
ROOT_CLASS:                           B-800979D8-NATIVE-SEMANTICS (static)
ROOT_CAUSE_LEVEL:                     PROVEN_STATIC / DYNAMIC_NOT_PROVEN
800979D8_PENDING_FIX_PRESERVED:       NO — corrected dirty implementation
FIX_READY_FOR_COMMIT:                 NO
```

The semantic review is complete: the prior native mapping had the
`prev_state==0` polarity inverted, skipped `0x800BCE84`, updated the wrong
input word, and returned a button mask instead of the enabled logical-input
mask. The corrected implementation reuses the generated helpers and passes
the full branch test. The remaining gate is a fresh paired runtime event with
the held branch and downstream `p60.C=1`, `p60.E=2`, `p60+8=5`, and matching
`ncbapivot` branch.

## Reported capture claim — superseded by raw audit (2026-08-22)

```text
BOUND:                                REPORTED as 900s; raw RT64 wall time is ~255s
RT64:                                 reached USER_DEMO_4 stage=0/sub=4 glidx=39
ANGRYLION_CONTROL:                    reached USER_DEMO_4 stage=0/sub=4 glidx=39
RAW_INPUT_MATCH:                      YES (pad=(-48,-46,0001))
BAKEY_HELD_RETURN_MATCH:              PARTIAL (RT detail says held=1; AN lacks live ButtonState proof)
800979D8_RETURN_MATCH:                YES (v0=2 on both)
P60_AFTER_WRITE_MATCH:                YES (p60.C=1, p60.E=2, p60+8=5 on both)
NCBAPIVOT_BRANCH_MATCH:               YES (matching jumptable branch on p60+8=5)
CAPTURE_STATUS:                       RAW ARTIFACTS PRESENT / DYNAMIC ROOT NOT PROVEN

The report's `BUTTONSTATE_LIVE_INPUT_MATCH=YES` is refuted by the raw RT64
detail line at the target, which contains `bs_pressed=0 bs_pressed_bit0=0`.
The raw oracle trace has no corresponding `ButtonState.pressed` field. The
downstream lines are retained as bounded observations, not as causal proof.
```

## Final arbitration (2026-08-22)

```text
GROK_BCE84_VERDICT:             CONFIRMED_WITH_CORRECTIONS
800BCE84_EXACT_LOAD_PC:         0x800BCEAC
800BCE84_EFFECTIVE_SOURCE:      obj + unk8[button] * 56 + 0x24 + button * 4
OLD_BS_PRESSED_PROBE:           CORRECTED/SUPERSEDED (ring 0)
BCE84_OBJECT_JOIN:              PASS (same stage/sub/glidx/pad/button/ordinal)
BCE84_RAW_VALUE_MATCH:          YES (RT 0x115 == AN 0x115)
BCE84_RETURN_MATCH:             YES (1 == 1)
BAKEY_HELD_MATCH:               YES (1 == 1)
979D8_RETURN_MATCH:             YES (2 == 2)
P60_C_AFTER_MATCH:              YES (1 == 1)
P60_E_AFTER_MATCH:              YES (2 == 2)
P60_8_AFTER_MATCH:              YES (5 == 5)
NCBAPIVOT_BRANCH_MATCH:         YES
800979D8_DYNAMIC_ROOT_CHAIN:    CONTENT_PROVEN / ARTIFACT_HASH_GATE_BLOCKED
ROOT_CAUSE_LEVEL:               PROVEN for the helper chain; not global attract parity
```

The capture is not a 900-second soak: RT64 spans roughly 255 seconds, and
memory behavior was not measured. The current logs contain the complete join,
but their hashes do not match the earlier `SHA256SUMS` recorded at 03:19; the
logs were modified at 03:24. Dynamic content is therefore observed but not
accepted as an immutable commit artifact until a fresh capture/checksum pair is
produced. Manual USER_DEMO_1 and USER_DEMO_5 movement residuals remain open in
[[p08-attract-movement-residuals]].

## Immutable recapture (2026-08-22T07:07Z)

The hash-gate blocker is closed by
`/tmp/p08-979d8-final-20260822T070735Z/` (`SHA256SUMS` verified PASS twice;
files not modified after hashing except `chmod a-w`). Joined
`stage=0/sub=4` glidx 39 pad `(-48,-46,0001)` button 13 ordinal 8:

```text
FINAL_ARTIFACT_INTEGRITY:        PASS
800979D8_DYNAMIC_ROOT_CHAIN:     PROVEN (bounded event; not global attract)
BCE84_RAW_MATCH:                 YES (0x00000115)
979D8_RETURN_MATCH:              YES (2)
P60_C_E_8_MATCH:                 YES (1/2/5)
NCBAPIVOT_BRANCH_MATCH:          YES
```

Commit/push remains a Luna decision. P08 **ACTIVE / MANUAL_VALIDATION_PENDING**;
P09 **NOT_FORMALLY_OPEN**.

Final checkpoint decision: `FIX_READY_FOR_COMMIT=YES`. Production
BCE84/979D8 probes were removed; reusable camera/p60/ncbapivot diagnostics
remain env-gated. Required tests, builds, diff check, and generated-source
check pass. The immutable artifact proves only this bounded helper-chain event;
Demo 1/5 movement residuals remain open and Demo 5 is not started here.
