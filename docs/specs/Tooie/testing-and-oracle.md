---
title: "TooieRecomp Spec — Testing Framework and Measurement Oracle"
description: "Technical specification for the host unit test harness, regression test suite, and the ANGRYLION_CONTROL software-rendered ground truth oracle."
status: current
area: "specs/Tooie"
updated: 2026-10-23
tags: [specs, tooierecomp, tests, oracle, angrylion, regression]
---

# Testing Framework and Measurement Oracle Specification

## 1. Testing Framework Architecture

TooieRecomp enforces a strict verification hierarchy designed to validate guest behavior at the narrowest useful level first (unit tests), then integration/runtime, before conducting end-to-end game reproductions.

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
|  - BCF28 helper bindings, Core1 RA overrides, stick-angle thunk verification    |
|  - rt64_host_endpoint_current_test (host latch & environment override)          |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|               ANGRYLION_CONTROL Ground Truth Oracle (tools/oracle/)             |
|  - Mupen64Plus Pure Interpreter + Angrylion RDP Plus + CXD4 LLE RSP (Linux)     |
|  - Bit-accurate software rasterization; golden frame & depth comparisons        |
+---------------------------------------------------------------------------------+
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Status |
| :--- | :--- | :--- | :--- |
| **REQ-TST-01** | Standalone Unit Tests | Tests execute directly on host without requiring game boot or graphics presentation. | `IMPLEMENTED` |
| **REQ-TST-02** | FPU Semantic Testing | `mips_rounding_test` validates IEEE 754 roundTiesToEven behavior. | `IMPLEMENTED` |
| **REQ-TST-03** | Host Endpoint Test | `rt64_host_endpoint_current_test` validates latching and environment overrides. | `IMPLEMENTED` |
| **REQ-TST-04** | Oracle Bit-Accuracy | `ANGRYLION_CONTROL` captures golden Z-buffer and pad state on Linux. | `IMPLEMENTED` |
| **REQ-TST-05** | Zero Oracle in Release | Angrylion and Mupen binaries must never be packaged into release client. | `IMPLEMENTED` |

---

## 3. Host Automated Test Suite Registry

All CMake test executables are marked `EXCLUDE_FROM_ALL` and link against `boot_hw_stubs.cpp` using the unified `TOOIE_WRAPPED_SYMBOLS` wrapping contract.

### 3.1 Test Execution Commands
- **Linux (`tools/build_linux.sh test`)**: Executes 8 core verification tests:
  - `tooie_menu_policy_test`
  - `tooie_menu_config_test`
  - `rt64_host_endpoint_current_test`
  - `p09_interp_burst_test`
  - `p09_world_interpolation_guard_test`
  - `p09_local_pair_snapshot_test`
  - `p09_exact_use_correlation_test`
  - `rt64_present_decimator_test`
- **Windows (`tools\windows\Build-Tooie.ps1 -Stage Test [-AllTests]`)**:
  - Default runs 9 required verification tests (the 8 above plus `rom_cart_import_test`).
  - Flag `-AllTests` runs all non-argument tests. Argument-driven tests report `SKIP` when invoked without input data.

### 3.2 Catalogue of Automated Test Targets

| Target | Source File | Scope / Purpose |
| :--- | :--- | :--- |
| `rt64_host_endpoint_current_test` | `tests/rt64_host_endpoint_current_test.cpp` | Validates host endpoint-current latch semantics and environment overrides. |
| `window_display_test` | `tests/window_display_test.cpp` | Tests display index clamping, dimension sanitizing, and aspect ratio math. |
| `tintro_projection_policy_test` | `tests/tintro_projection_policy_test.cpp` | Tests cutscene projection source aspect ratio and compensation factor. |
| `p09_exact_use_correlation_test` | `tests/p09_exact_use_correlation_test.cpp` | Tests seam-edge diagnostic keys, plan building, and memberships. |
| `p09_ordered_match_test` | `tests/p09_ordered_match_test.cpp` | Tests draw-call hash-ordered transform pairing. |
| `p09_world_interpolation_guard_test`| `tests/p09_world_interpolation_guard_test.cpp` | Tests P09 world transform interpolation guards. |
| `p09_local_pair_snapshot_test` | `tests/p09_local_pair_snapshot_test.cpp` | Tests transform pairing snapshot capture. |
| `p09_interp_burst_test` | `tests/p09_interp_burst_test.cpp` | Tests frame capture buffer formatting. |
| `rt64_present_decimator_test` | `tests/rt64_present_decimator_test.cpp` | Tests integer accumulator presentation rate decimation. |
| `tooie_fps_meter_test` | `tests/tooie_fps_meter_test.cpp` | Tests sliding-window presentation rate metering. |
| `tooie_menu_policy_test` | `tests/tooie_menu_policy_test.cpp` | Tests menu config coercion and framerate option interactions. |
| `tooie_menu_config_test` | `tests/tooie_menu_config_test.cpp` | Tests serialization and config sink callbacks. |
| `rom_cart_import_test` | `tests/rom_cart_import_test.cpp` | Tests retail cartridge ROM loading and in-memory decompression. |
| `stick_angle_thunk_test` | `tests/stick_angle_thunk_test.cpp` | Tests `func_80013B70` normalization across cardinals and diagonals. |
| `bcf28_helpers_test` | `tests/bcf28_helpers_test.cpp` | Verifies `func_800BCF28` binding fix and gap stub replacements. |
| `core1_ra_wrappers_test` | `tests/core1_ra_wrappers_test.cpp` | Verifies return address preservation across core1 overlay transitions. |
| `graphics_failure_test` | `tests/graphics_failure_test.cpp` | Verifies structured error reporting and exit on graphics initialization failure. |
| `input_direction_test` | `tests/input_direction_test.cpp` | Verifies bidirectional stick input mapping and cardinal quadrant alignment. |
| `matrix_submission_test` | `tests/matrix_submission_test.cpp` | Verifies display list matrix push/pop semantics and fixed-point conversions. |
| `mips_rounding_test` | `tests/mips_rounding_test.cpp` | Verifies IEEE 754 nearest-even rounding in floating-point to integer conversions. |
| `preemption_t15` | `tests/preemption_t15.cpp` | Verifies Hybrid F' preemption thread scheduling. |
| `p08_ws_frustum_test` | `tests/p08_ws_frustum_test.cpp` | Tests 16:9 widescreen view volume plane adjustments. |
| `p09_cpu_shader_clip_semantics_test`| `tests/p09_cpu_shader_clip_semantics.cpp` | Verifies CPU diagnostic shader matrix multiplication order. |
| `tooie_audio_capture_test` | `tests/tooie_audio_capture_test.cpp` | Tests audio stream buffer capture. |
| `tooie_bgm_volume_test` | `tests/tooie_bgm_volume_test.cpp` | Tests BGM sequence player volume scaling. |
| `tooie_bgm_hooks_test` | `tests/tooie_bgm_hooks_test.cpp` | Tests audio driver sequence update intercept hooks. |
| `tooie_demo7_drift_test` | `tests/tooie_demo7_drift_test.cpp` | Tests attract demo kinematic monitoring. |

### 3.3 Auxiliary and Python Test Scripts
- `tests/attract_demo2_transition_jal_test.py`: Validates disassembly JAL targets in attract transitions.
- `tests/n64recomp_zero_size_static_test.sh`: Validates static function end-boundary sizing in N64Recomp.
- `tools/test_rom_check.py`: Validates ROM checksum algorithms.
- `tools/test_syscall_decode.py`: Validates core2 syscall decode tables.
- `tools/test_metadata_invariants.py`: Verifies metadata consistency across table definitions.

---

## 4. ANGRYLION_CONTROL Measurement Oracle

To establish bit-accurate ground truth independent of host graphics drivers, TooieRecomp uses `ANGRYLION_CONTROL` (detailed in [Angrylion Control Oracle](angrylion-control-oracle.md)):
- **Location**: `tools/oracle/angrylion-control/`.
- **Scripts**:
  - `run.sh`: Main invocation wrapper running Mupen64Plus with Angrylion RDP Plus and CXD4 LLE RSP. Requires `TOOIE_CART_Z64`.
  - `scripts/build.sh`: Builds local Mupen64Plus core and plugins on Linux.
  - `scripts/fetch.sh`: Fetches pinned repository dependencies.
  - `scripts/status.sh`: Verifies oracle build freshness and binary hashes.
- **Contract & Scope**:
  - Acts exclusively as an offline measurement oracle and regression detector.
  - Captures golden Z-buffer depth arrays, player coordinates, and pad state.
  - **Invariant**: TooieRecomp renders strictly via RT64; Angrylion is never bundled into the production game client.