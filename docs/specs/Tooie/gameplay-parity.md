---
title: "TooieRecomp Spec — Gameplay Parity and Verification Criteria"
description: "Technical specification for gameplay parity verification, camera mechanics, physics invariants, and attract sequence validation in TooieRecomp."
status: current
area: "specs/Tooie"
updated: 2026-10-23
tags: [specs, tooierecomp, gameplay, parity, camera, physics, attract]
---

# Gameplay Parity and Verification Criteria Specification

## 1. Definition of Gameplay Parity

TooieRecomp targets gameplay parity defined by three core invariants:
1. Game mechanics, player physics, collision checks, object interactions, and AI state machines operate faithfully to original Nintendo 64 hardware execution.
2. The game executes from boot through the final boss and credits sequence without crashes, softlocks, or progression defects.
3. Widescreen and framerate enhancements introduce zero game logic divergence or physics desynchronization.

```text
+---------------------------------------------------------------------------------+
|                         Game Logic Execution Layer                              |
|  - R4300i Integer & FPU Semantics (Nearest-Even Rounding, Truncation)           |
|  - Dynamic Guest Tick Cadence (EXPECTED_LOGIC_HZ: dynamic frame divisor)        |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                        Verification & Differential Oracles                      |
|  - Automated Attract Demos: Regression suite against reference recordings       |
|  - Software Reference Oracle: ANGRYLION_CONTROL (Linux Mupen + Angrylion + CXD4)|
|  - Host Unit Test Harness: tests/mips_rounding_test.cpp, tests/matrix_*.cpp    |
+---------------------------------------------------------------------------------+
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Status |
| :--- | :--- | :--- | :--- |
| **REQ-PAR-01** | FPU Nearest-Even Rounding | `round.w.s` enforces IEEE 754 roundTiesToEven semantics. | `IMPLEMENTED` |
| **REQ-PAR-02** | Static Boundary Preservation | Recompiler emits full instruction bodies for static functions sharing addresses with zero-size ELF symbols. | `IMPLEMENTED` |
| **REQ-PAR-03** | Attract Parity Regression Suite | Recorded attract sequences execute consistently for regression detection. | `PARTIAL` |
| **REQ-PAR-04** | Attract 4:3 Culling Isolation | Attract mode (`0x80127632 == 5`) restores isolated 4:3 culling planes. | `IMPLEMENTED` |
| **REQ-PAR-05** | Campaign Progression | Complete playthrough from boot to credits without progression blockers. | `PARTIAL` |

---

## 3. Mathematical & Arithmetic Precision Invariants

### 3.1 `round.w.s` Nearest-Even Rounding
- On MIPS R4300i, `round.w.s` implements IEEE 754 **roundTiesToEven** (halfway cases round to the nearest even integer).
- Standard C/C++ integer casts `(int)x` perform truncation toward zero (`round_toward_zero`), while `std::round` performs round half away from zero.
- Recompiled trigonometry and angle approximations rely on nearest-even rounding.
- Verification: Validated in host test `tests/mips_rounding_test.cpp`.

### 3.2 Native Guest Timing and Cadence Constraints
- `EXPECTED_VI_HZ`: `60.000 Hz` (NTSC VI interrupt cadence).
- Guest logic frame divisor is dynamic and evaluated at runtime.
- `recomp_get_target_framerate` (`recomp_api.cpp:75-78`) forwards `60 / frame_divisor` to the runtime via `ultramodern::get_target_framerate()`.

---

## 4. Attract Sequence Verification & Culling Protection

Banjo-Tooie contains automated demo sequences stored as compressed controller inputs in ROM:
- **Attract Mode Detection & Plane Isolation**:
  - Detected via guest halfword `0x80127632 == 5`.
  - When active, native 16:9 view volume widening is automatically disabled in `boot_hw_stubs.cpp`, retaining original 4:3 culling behavior to prevent physics or simulation divergence.
- **Culling Allowlist Preservation**:
  - The object culling hook `func_800CACEC` restricts aspect-aware expansion to allowlisted pure draw callers (`func_800B97A4` kind 3, `func_8011447C` kind 2), preventing simulation divergences during recorded attract playback. See [Graphics Specification](graphics-and-presentation.md).
- **Regression Detection Suite**:
  - Attract sequences (Demos 0, 1, 4, 5, 7) are evaluated against the offline reference oracle `ANGRYLION_CONTROL` (see [Testing & Oracle](testing-and-oracle.md)). Full end-to-end campaign parity is evaluated progressively.