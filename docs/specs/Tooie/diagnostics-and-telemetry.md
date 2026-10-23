---
title: "TooieRecomp Spec — Diagnostics and Runtime Telemetry"
description: "Technical specification for the tooie_diag diagnostic framework, environment flags, telemetry collectors, and safety invariants."
status: current
area: "specs/Tooie"
updated: 2026-10-23
tags: [specs, tooierecomp, diagnostics, telemetry, flags, safety]
---

# Diagnostics and Runtime Telemetry Specification

## 1. Subsystem Architecture

TooieRecomp embeds a diagnostic framework (`include/tooie_diag.h`, `src/game/tooie_diag.cpp`) used during investigation of runtime crashes, preemption gaps, and graphics transform defects.

```text
Environment Control Flags (TOOIE_DIAG_*)
                 |
                 v
Diagnostic Registry & Safety Gates (src/game/tooie_diag.cpp)
                 |
                 +---> Throttled Memory & State Probes (CPU-only)
                 |
                 +---> RT64 Matrix/Vertex Hooks (tooie_diag_rt64_*)
                 |
                 v
Diagnostic Log Output (stderr or specified file destination)
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Status |
| :--- | :--- | :--- | :--- |
| **REQ-DIA-01** | Default-OFF Invariant | Heavy telemetry and file dumps default to OFF in normal gameplay runs. | `IMPLEMENTED` |
| **REQ-DIA-02** | Diagnostic Endpoint Rule | Measuring interpolation requires Interpolation ON and endpoint CURRENT inactive. | `IMPLEMENTED` |
| **REQ-DIA-03** | Structured Logging | Diagnostic facilities use bounded output rates without hanging execution. | `IMPLEMENTED` |
| **REQ-DIA-04** | Zero GPU Stalls | Probes evaluate on CPU; zero blocking GPU readback stalls in render loop. | `IMPLEMENTED` |

*Note on baseline output*: Normal execution unconditionally emits initial overlay syscall dispatches (first 128) and RT64 state-change lines (`[tooie-interp]`). All heavy file loggers and continuous telemetry streams are strictly gated.

---

## 3. Flag Conventions & Registry

### 3.1 Environment Parsing Conventions
1. **Convention A (Standard Tooie)**:
   - Activated when variable is non-empty and first character is not `'0'`.
   - Examples: `TOOIE_DIAG_OVERLAY_LOG`, `TOOIE_DIAG_UD5`, `TOOIE_TEST_MAP_VIS_ASPECT_AWARE`.
2. **Convention B (Presence Check)**:
   - Activated whenever variable is defined in environment (`getenv != nullptr`), even if set to `"0"`.
   - Examples: RT64 P09 diagnostics (`TOOIE_DIAG_P09_CAMERA_CUT`, `TOOIE_DIAG_P09_JOINT`).

### 3.2 Primary Diagnostic Environment Flags

| Flag Name | Category | Convention | Description |
| :--- | :--- | :--- | :--- |
| `TOOIE_DIAG` | Core | A | Master switch for basic diagnostic logging. |
| `TOOIE_DIAG_INTERVAL_MS` | Core | Value | Sampling interval in milliseconds for periodic telemetry. |
| `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` | Graphics | Value | Explicit `"0"` enables interpolation; non-empty non-zero forces CURRENT. Unset defers to host menu option. |
| `TOOIE_TEST_INTERPOLATION_FPS` | Graphics | Value | Accepts `60` or `120` to force Interpolation ON at that target rate. |
| `TOOIE_DIAG_PRESENT_LOG` | Graphics | A | Emits present rate telemetry every 250 ms. |
| `TOOIE_DIAG_OVERLAY_LOG` | Runtime | A | Logs overlay load and unload events. |
| `TOOIE_DIAG_P09_CAMERA_CUT` | RT64 | B | Logs camera cut detection decisions and matrices. |
| `TOOIE_DIAG_P09_JOINT` | RT64 | B | Logs joint inference statistics every 200 workloads. |
| `TOOIE_DIAG_P09_ROD_CLASSIFY=<path>` | RT64 | Path | Evaluates geometry after guard processing; classifies stretched rods and logs connected object telemetry (`OBJ` rows for objects ≥ 20 WORLDs: centroid motion `mot`, chord fraction `along`, off-chord `perp`, `rms` error, R/C/E/O/H counts, root motion `rmot`/`rlerp`). |
| `TOOIE_DIAG_P09_PAIR_AUDIT=<path>` | RT64 | Path | Outputs pairwise transform correspondence evaluations. |
| `TOOIE_DIAG_P09_LOCAL_PAIR_SNAPSHOT` | RT64 | A | Captures transform snapshot pairs for test harnesses. |
| `TOOIE_DIAG_TIMING` | Telemetry | A | Logs scheduling and frame pacing telemetry. |
| `TOOIE_DIAG_INPUT` / `_INPUT_VECTOR` | Input | A | Logs controller input state and stick vectors. |
| `TOOIE_DIAG_DEMO_PATH` / `_DEMO_SYNC` | Attract | A | Logs attract demo position sync and path tracking. |
| `TOOIE_DIAG_DEMO7_DRIFT` | Attract | A | Monitors kinematic drift in attract Demo 7. |
| `TOOIE_DIAG_DEMO1_PARITY` / `_PROJECTILE` | Attract | A | Monitors fire egg trajectory and target hits in Demo 1. |
| `TOOIE_LOG_RSP` | Audio | A | Emits audio RSP task execution logs. |
| `TOOIE_AUDIO_CAPTURE_DIR` | Audio | Path | Directory destination for captured audio PCM chunks. |
| `TOOIE_AUDIO_CAPTURE_SECONDS` | Audio | Value | Duration for audio capture (10..20 s, default 12 s). |
| `TOOIE_NO_GUI_DIALOG` | Error | A | Suppresses GUI error dialogs on fatal errors (logs to console only). |
| `TOOIE_AUTO_START` | Launcher | A | Skips launcher window directly into game loop. |
| `TOOIE_DISABLE_LAUNCHER_MUSIC` | Launcher | A | Mutes preview BGM in launcher window. |
| `ULTRAMODERN_GUEST_PREEMPTION` | Runtime | Value | Unset/empty = ON. Value `0`, `false`, `off`, `no` = OFF. |

---

## 4. C++ Diagnostic API (`include/tooie_diag.h`)

```cpp
namespace tooie_diag {
    // Feature gates
    bool enabled();
    bool timing_enabled();
    bool movement_enabled();
    bool input_enabled();
    bool frontend_enabled();
    bool demo_path_enabled();
    bool demo_sync_enabled();
    bool direction_enabled();
    bool backpack_flap_enabled();
    bool secondary_sway_enabled();

    // Event hooks
    void note_syscall_dispatch(uint32_t ovl, uint32_t ep);
    void note_syscall_return(uint32_t ovl, uint32_t ep);
    void note_sched_ack();
    void note_rsp_audtask();
    void note_queue_samples(size_t sample_count, uint32_t queued_bytes_after, int16_t peak_abs, size_t nonzero);
    void note_guest_vi();
    void note_guest_input_update();
    void note_13b70(uint32_t ra, float y, float x, float mag, float angle);

    // RT64 telemetry probes & lifecycle
    uint64_t tooie_diag_rt64_matrix(...);
    void tooie_diag_rt64_vertex(...);
    void tooie_diag_rt64_feather_tri(...);
    void tooie_diag_rt64_match(...);
    void tooie_diag_rt64_override(...);
    void tooie_diag_rt64_observe(...);
    void tooie_diag_rt64_finalize(...);
    void tooie_diag_rt64_flush();
}
```

---

## 5. Operational Safety Protocols

1. **Measuring Interpolation**:
   - Because TooieRecomp requests host endpoint CURRENT by default when Interpolation is OFF in the menu, tests and diagnostics evaluating transform interpolation must ensure Interpolation is ON (or set `TOOIE_TEST_INTERPOLATION_FPS=60|120`) and leave `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` unset or set to `"0"`.
2. **Zero GPU Readback in Hot Loops**:
   - Runtime telemetry hooks evaluate CPU-side state only. Synchronous GPU readbacks or pipeline stalls during active render passes are strictly avoided to prevent timing corruption.