---
title: "TooieRecomp Spec — Gameplay Parity and Verification Criteria"
description: "Comprehensive technical specification for gameplay parity verification, camera mechanics, physics invariants, and attract sequence validation in TooieRecomp."
status: current
area: "specs/Tooie"
updated: 2026-10-16
tags: [specs, tooierecomp, gameplay, parity, camera, physics, attract]
---

# Gameplay Parity and Verification Criteria Specification

## 1. Definition of Gameplay Parity

Under [[../../plan/definition-of-done|Definition of Done]], TooieRecomp achieves gameplay parity when:
1. All game mechanics, player physics, collision checks, object interactions, and AI state machines behave identically to original N64 hardware.
2. The game can be played from boot through the final boss (HAG 1) and credits without crashes, softlocks, or progression blockers.
3. Enhanced presentation modes (widescreen, high framerates) introduce zero game logic divergence or physics desynchronization.

```text
+---------------------------------------------------------------------------------+
|                         Game Logic Execution Layer                              |
|  - R4300i Integer & FPU Semantics (Nearest-Even Rounding, Truncation)           |
|  - Dynamic Guest Tick Cadence (EXPECTED_LOGIC_HZ: UNKNOWN; frame divisor dynamic) |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                        Verification & Differential Oracles                      |
|  - Automated Attract Demos: Demo 0 (765 frames), USER_DEMO_4 (glidx 1..38)     |
|  - Software Reference Oracle: ANGRYLION_CONTROL (Mupen + Angrylion RDP + CXD4)  |
|  - Host Unit Test Harness: tests/mips_rounding_test.cpp, tests/matrix_*.cpp    |
+---------------------------------------------------------------------------------+
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Implementation Status |
| :--- | :--- | :--- | :--- |
| **REQ-PAR-01** | Attract Demo 0 Parity | 765/765 input frames, physics, and blue egg fire match reference. | `PROVISIONAL_MATCH` |
| **REQ-PAR-02** | USER_DEMO_4 Chain | Follow-cam glidx 1..38 and BCE84/P60/ncbapivot chain match. | `BOUNDED_MATCH_ACCEPTED` |
| **REQ-PAR-03** | Attract Demo 1 Parity | USER_DEMO_1 completes Talon-Trot sequence without desync. | `OPEN_RESIDUAL` |
| **REQ-PAR-04** | Attract Demo 5 Parity | USER_DEMO_5 completes Witchyworld locomotion without desync. | `OPEN_RESIDUAL` |
| **REQ-PAR-05** | FPU Nearest-Even Rounding | `round.w.s` enforces IEEE 754 roundTiesToEven semantics. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-PAR-06** | Camera Raycast Clearance | `func_801138A0` clearance queries evaluate without drift. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-PAR-07** | Campaign Completion | Complete game playthrough from boot to credits without softlock. | `PLANNED` / `NOT_FORMALLY_OPEN` |
| **REQ-PAR-08** | Attract Demo 7 Stationary Parity | Demo 7 glidx 38..120 stationary pose, zero velocity, and zero target speed match reference. | `BOUNDED_MATCH_ACCEPTED` |

---

## 3. Mathematical & Arithmetic Precision Invariants

Subtle differences between modern x86-64 SSE/AVX math and MIPS R4300i FPU instructions can cause physics to diverge over thousands of frames:

### 3.1 `round.w.s` Nearest-Even Rounding
- On MIPS R4300i, `round.w.s` implements IEEE 754 **roundTiesToEven** (halfway cases round to the nearest even integer).
- Standard C/C++ casts `(int)x` perform truncation toward zero (`round_toward_zero`), while `std::round` performs round half away from zero.
- In static recompilation, trigonometric approximations in player facing and camera orientation rely on nearest-even rounding.
- **Verification**: Validated in `tests/mips_rounding_test.cpp` (3/3 assertions PASS). Checkpointed in commit `5854872c79a7cab1c9395ddcd173cece449ffe11` (`p08-user-demo4-sincos-semantics-fix.md`).

### 3.2 Viewport Clipping vs Physical Collision
- Empirical analysis established that `func_800C4C34` is viewport projection clipping, not physical camera collision (`p08-demo-camera-collision-sol-review.md`).
- Viewport clipping bounds must not be conflated with obstacle collision geometry.

### 3.3 Native Guest Timing and Cadence Constraints
Ground truth established in `docs/plan/evidence/p08-original-timing-4x3-baseline.md`:
- `EXPECTED_VI_HZ`: `60.000 Hz` (NTSC VI interrupt cadence).
- `EXPECTED_LOGIC_HZ`: `UNKNOWN` (guest frame divisor is dynamic and not proven).
- `EXPECTED_RENDERED_GAME_FRAMES_HZ`: `UNKNOWN` (display-list submission count unmeasured).
- `EXPECTED_ANIMATION_UPDATE_HZ`: `UNKNOWN` (`baanim_update` rate unmeasured).
- `recomp_get_target_framerate`: returns `60 / frame_divisor` to the runtime, but the active Tooie divisor is dynamic and unproven, so it is strictly classified as `UNKNOWN` rather than assuming 20 or 30 Hz.
- `IS_SIMULATION_TIED_TO_HOST_PRESENT` and `IS_ANIMATION_TIED_TO_HOST_PRESENT`: `UNKNOWN`.

---

## 4. Attract Sequence Status & Differential Register

Banjo-Tooie's recorded attract demos provide an automated regression suite:

1. **Demo 0 (Jinjo Village / Blue Egg)**:
   - 765/765 input frames match reference.
   - Blue egg firing at glidx 322 matches Angrylion reference frame.
   - Status: `PROVISIONAL_PRE_P09_PARITY_EVIDENCE`.
2. **USER_DEMO_4 (Wooded Hollow / Pivot)**:
   - Follow-cam matches Angrylion glidx 1..38.
   - Bounded BCE84 ring-1 raw join, `0x800979D8`, and P60/ncbapivot chain match reference.
   - Status: `BOUNDED_MATCH_ACCEPTED`.
3. **USER_DEMO_1 (Wooded Hollow / Fire Egg & Talon Trot)**:
   - Parity discriminator: Single-feature counterfactual isolated `func_800CACEC` aspect-aware expand in `func_800E3E8C` as causal. Narrow fix dropped generic `func_800E3E8C` scope, allowlisted pure-draw `func_800B97A4` (kind 3), preserved `func_8011447C` (kind 2), and kept mixed renderer `func_800DE498` original `v0`.
   - Under actual default-ON (binary A6A2): restores 355-post trajectory, 546 ActorRecord words, 14 RNG snapshots, and post 355 Fire Egg collision hit (`0x80132f60`, narrowphase `CD6B4 = 1`, 22 sweep + 21 move pairs exact).
   - Status: `BOUNDED_DEMO1_MATCH_ACCEPTED` / `VISUALLY_CONFIRMED_FOR_TESTED_AREA` (user-reported: "Sim, continua funcionando" under noflags launch; broader game parity remains untested residual).
4. **USER_DEMO_5 (Witchyworld / Locomotion)**:
   - First locomotion divergence under active forensic investigation (`p08-user-demo5-locomotion-first-divergence.md`).
   - Status: `USER_REPORTED / OPEN`.
5. **USER_DEMO_7 (TDL Stomping Plains / Stationary Cinema)**:
   - 119 pose pairs ($i \to i+1$) match reference within quantized tolerances (pos/vel/cam $\le 0.009$, anim $\le 0.00051$, tspd $\le 0.051$). Pad inputs match 120/120.
   - Glidx 38..120 is fully stationary at pos `(-80.46, 0.00, 4496.25)`, `velxz = 0.0`, `tspd = 0.0`.
   - Root cause: Static helper `static_344_80800000` (`bsrest` stop helper) truncated to empty body by `N64Recomp` zero-size ELF marker bug; resolved via generator patch `patches/runtime/n64recomp-zero-size-static.patch` and full clean regeneration.
   - Status: `BOUNDED_MATCH_ACCEPTED` (Criterion 1: Demo 7 bounded stationary parity PASS).

*Operational Constraint*: Global attract parity is **NOT CLAIMED**. Criterion 2 (global real-time cadence): 90.032s serial timing test (`timing-fixed-pid-serial-90s.log`) with PID-tagged serialization fix confirmed guest VI cadence in ~60.012 Hz (PID 938341: 3543 VIs in 59.038 s; 24.306 logic Hz) with no evidence of a 2x VI clock in the measured window. This does NOT prove universal camera/mob/campaign timing; user-specific non-Demo7 reproduction question remains open. P09 formal status remains `PENDING / NOT_FORMALLY_OPEN`.
