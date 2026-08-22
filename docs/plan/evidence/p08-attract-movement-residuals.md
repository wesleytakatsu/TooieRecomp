---
title: "P08 — attract movement residuals after the 0x800979D8 checkpoint"
description: "User-observed residuals in attract scenes preserved separately from the validated USER_DEMO_4 helper fix."
status: active
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-22
tags: [p08, attract, movement, residual, user-observation, demo1, demo5]
---

# Attract movement residuals

This note records manual observations after the `0x800979D8` checkpoint. They
are product/parity evidence, not automated proofs and not a new movement fix.

```text
USER_DEMO_1_MANUAL_STATUS:  RESIDUAL_REPRODUCED_BY_USER
USER_DEMO_2_MANUAL_STATUS:  APPEARS_CORRECT_BY_USER / NOT_AUTOMATED_PROOF
USER_DEMO_3_MANUAL_STATUS:  APPEARS_CORRECT_BY_USER / NOT_AUTOMATED_PROOF
USER_DEMO_4_MANUAL_STATUS:  APPEARS_CORRECT_BY_USER / bounded paired evidence
USER_DEMO_5_MANUAL_STATUS:  RESIDUAL_REPRODUCED_BY_USER
USER_DEMO_6_MANUAL_STATUS:  APPEARS_CORRECT_BY_USER / NOT_AUTOMATED_PROOF
GLOBAL_MOVEMENT_PARITY:     NOT_PROVEN
GLOBAL_ATTRACT_PARITY:      NOT_PROVEN
GLOBAL_ANIMATION_PARITY:    NOT_PROVEN
```

Observed detail:

- Demo 1 is correct for roughly 80% of the scene, then follows the wrong
  trajectory / gets lost.
- Demo 5 starts correctly, then gets lost very early in the icy/snow scene.
- Demo 4 now looks correct in manual play and its bounded paired capture
  validates the `0x800979D8 → P60 → ncbapivot` event; this does not close the
  remaining attract set.

## Hypothesis

```text
TALON_TROT_OR_LOCOMOTION_TRANSITION_HYPOTHESIS:
INFERENCE / USER_OBSERVED_CORRELATION
```

The suspected correlation is a transition between normal Banjo movement and
the Kazooie-carries-Banjo/Talon-Trot state. It is not causal proof. Possible
future discriminators include behavior/substate, locomotion mode, animation
ID/phase/timer/rate/transition, root motion, position, velocity, target speed,
acceleration, yaw, ground state, physics `dt`, and movement-target/script
state.

## Next single action

Trace `USER_DEMO_5` from scene start to its first divergence, recording those
state fields and testing the Talon-Trot correlation without assuming it is
causal. `USER_DEMO_1` is the second target. No implementation change belongs
to this checkpoint.

P08 remains `ACTIVE / MANUAL_VALIDATION_PENDING`; P09 remains
`PENDING / NOT_FORMALLY_OPEN`.
