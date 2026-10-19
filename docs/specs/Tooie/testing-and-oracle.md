---
title: "TooieRecomp Spec — Testing Framework and Measurement Oracle"
description: "Comprehensive technical specification for the host unit test harness, regression test suite, and the ANGRYLION_CONTROL software-rendered ground truth oracle."
status: current
area: "specs/Tooie"
updated: 2026-10-16
tags: [specs, tooierecomp, tests, oracle, angrylion, regression]
---

# Testing Framework and Measurement Oracle Specification

## 1. Testing Framework Architecture

TooieRecomp enforces a strict verification hierarchy designed to validate guest behavior at the narrowest useful level first (unit tests), then integration/runtime, before conducting expensive end-to-end game reproductions.

```text
+---------------------------------------------------------------------------------+
|                       Host Unit & Regression Tests (tests/)                     |
|  - Standalone C++ executables compiling specific subsystems in isolation        |
|  - Arithmetic semantics, window math, projection policy, preemption logic      |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                      Runtime Contract Verification (Recomp)                     |
|  - BCF28 helper bindings, Core1 RA wrappers, stick-angle thunk verification     |
|  - tooie_rsp_endpoint_current_test (20/20 policy validation)                   |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|               ANGRYLION_CONTROL Ground Truth Oracle (tools/oracle/)              |
|  - Mupen64Plus Pure Interpreter + Angrylion RDP Plus + CXD4 LLE RSP             |
|  - Bit-accurate software rasterization; golden frame & depth comparisons        |
+---------------------------------------------------------------------------------+
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Implementation Status |
| :--- | :--- | :--- | :--- |
| **REQ-TST-01** | Standalone Unit Tests | Tests execute directly on host without requiring game boot or Vulkan. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-TST-02** | FPU Semantic Testing | `mips_rounding_test` validates IEEE 754 roundTiesToEven behavior. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-TST-03** | Workaround Policy Test | `tooie_rsp_endpoint_current_test` validates 20/20 environment rules. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-TST-04** | Oracle Bit-Accuracy | `ANGRYLION_CONTROL` captures golden Z-buffer and pad state. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-TST-05** | Zero Oracle in Release | Angrylion and Mupen binaries must never be packaged into release client. | `IMPLEMENTED` / `VALIDATED` |

---

## 3. Host Automated Test Suite Registry

The repository maintains a catalogue of 14 automated verification targets within `tests/` (12 CMake C++ test executables, 1 auxiliary preemption harness, and 1 Python script):

| Test Target | Source File | Assertions / Verification Scope | Historical Milestone Status |
| :--- | :--- | :--- | :--- |
| `tooie_rsp_endpoint_current_test` | `tests/tooie_rsp_endpoint_current_test.cpp` | 20/20 assertions. Tests unset, `"0"`, `""`, `"1"`, `"keep"`, and error rollback paths. | `PASS` (historical 2026-10-16, exit 0) |
| `window_display_test` | `tests/window_display_test.cpp` | 38/38 assertions. Tests display index clamping, dimension sanitizing, aspect ratio math. | `PASS` (historical 2026-10-14, exit 0) |
| `tintro_projection_policy_test` | `tests/tintro_projection_policy_test.cpp` | 7/7 assertions. Tests cutscene projection source aspect ratio and compensation factor. | `PASS` (historical 2026-10-14, exit 0) |
| `p09_exact_use_correlation_test` | `tests/p09_exact_use_correlation_test.cpp` | 1231/1231 assertions. Tests seam-edge diagnostic keys, plan building, and memberships. | `PASS` (historical 2026-10-15, exit 0) |
| `stick_angle_thunk_test` | `tests/stick_angle_thunk_test.cpp` | Tests `func_80013B70` normalization across all cardinals, diagonals, and magnitudes. | `PASS` (historical P08, exit 0) |
| `bcf28_helpers_test` | `tests/bcf28_helpers_test.cpp` | Verifies `func_800BCF28` binding fix and gap stub replacements. | `PASS` (historical P08, exit 0) |
| `core1_ra_wrappers_test` | `tests/core1_ra_wrappers_test.cpp` | Verifies return address preservation across core1 overlay transitions. | `PASS` (historical P08, exit 0) |
| `graphics_failure_test` | `tests/graphics_failure_test.cpp` | Verifies structured error reporting and clean exit on Vulkan initialization failure. | `PASS` (historical P08, exit 0) |
| `input_direction_test` | `tests/input_direction_test.cpp` | Verifies bidirectional stick input mapping and cardinal quadrant alignment. | `PASS` (historical P08, exit 0) |
| `matrix_submission_test` | `tests/matrix_submission_test.cpp` | Verifies display list matrix push/pop semantics and fixed-point conversions. | `PASS` (historical P08, exit 0) |
| `mips_rounding_test` | `tests/mips_rounding_test.cpp` | Verifies IEEE 754 nearest-even rounding in floating-point to integer conversions. | `PASS` (historical P08, exit 0) |
| `preemption_t15` | `tests/preemption_t15.cpp` | Verifies Hybrid F' preemption thread scheduling (CMake `EXCLUDE_FROM_ALL TRUE`). | `PASS` (historical P08, exit 0) |
| `p09_cpu_shader_clip_semantics_test` | `tests/p09_cpu_shader_clip_semantics.cpp` | Verifies CPU diagnostic shader matrix multiplication order and homogeneous clip math. | `PASS` (historical P09, exit 0) |
| `attract_demo2_transition_jal_test.py`| `tests/attract_demo2_transition_jal_test.py` | Python static analysis validating disassembly JAL targets in attract transitions. | `PASS` (historical P08, exit 0) |

> [!IMPORTANT]
> **Audit Constraint & Reality Check**: The catalogue above lists the 14 automated test targets defined in the repository. **Cataloguing these tests is NOT proof that all 14 ran or passed in any single recent session or continuous build.** No unit tests were rerun during this documentation audit task. Each passing status reflects bounded historical evidence documented in specific phase milestones. Whole-suite regression execution requires explicit test suite invocation.

---

## 4. ANGRYLION_CONTROL Measurement Oracle

To establish bit-accurate ground truth independent of host graphics drivers, TooieRecomp relies on `ANGRYLION_CONTROL` ([[../BT/angrylion-control-oracle|Oracle Specification]]):
- **Location**: `tools/oracle/angrylion-control/`.
- **Scripts**:
  - `run.sh`: Main invocation wrapper running Mupen64Plus with Angrylion RDP Plus and CXD4 LLE RSP.
  - `scripts/build.sh`: Builds local Mupen64Plus core and plugins.
  - `scripts/fetch.sh`: Fetches pinned repository dependencies.
  - `scripts/status.sh`: Verifies oracle build freshness and binary hashes.
- **Contract & Scope**:
  - Acts exclusively as an offline measurement oracle and regression detector.
  - Used to capture golden Z-buffer depth arrays, player coordinates, and pad state.
  - **Invariant**: TooieRecomp itself renders strictly via `RT64`; Angrylion is never bundled into the production game client.
