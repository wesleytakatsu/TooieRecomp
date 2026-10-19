---
title: "TooieRecomp Spec — Diagnostics and Runtime Telemetry"
description: "Comprehensive technical specification for the tooie_diag diagnostic framework, environment flags, telemetry collectors, and safety invariants."
status: current
area: "specs/Tooie"
updated: 2026-10-16
tags: [specs, tooierecomp, diagnostics, telemetry, flags, safety]
---

# Diagnostics and Runtime Telemetry Specification

## 1. Subsystem Architecture

TooieRecomp embeds a non-intrusive diagnostic framework (`include/tooie_diag.h`, `src/game/tooie_diag.cpp`) used during investigation of runtime crashes, preemption gaps, and graphics interpolation defects.

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
Atomic Status Emitter (tmp file + atomic rename) -> /tmp/*.txt
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Implementation Status |
| :--- | :--- | :--- | :--- |
| **REQ-DIA-01** | Default-OFF Invariant | All diagnostic telemetry defaults to OFF in normal gameplay runs. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-DIA-02** | Diagnostic Rollback Rule | Captures measuring interpolation MUST set `FORCE_ENDPOINT_CURRENT=0`. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-DIA-03** | Atomic File Persistence | Status files write to `.tmp` before atomic rename to prevent partial reads. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-DIA-04** | Circuit Breakers | Diagnostic collectors enforce finite attempt caps (max 24 heavy attempts). | `IMPLEMENTED` / `VALIDATED` |
| **REQ-DIA-05** | Zero GPU Stalls | Probes evaluate on CPU; zero blocking GPU readback stalls in render loop. | `IMPLEMENTED` / `VALIDATED` |

---

## 3. Interface & Flag Catalog

### 3.1 Environment Flags Registry
All diagnostic facilities are **DEFAULT OFF** in normal gameplay runs:

| Flag Name | Subsystem | Default | Behavior & Output File |
| :--- | :--- | :--- | :--- |
| `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` | Graphics/RSP | `"1"` (Tooie) | Controls `tfPos` bypass. When unset in TooieRecomp, sets `"1"`. Explicit `"0"` enables interpolation. |
| `TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION` | Seam Search | `0` (OFF) | Correlates pathological edge joins against local plans. Triggered via `/tmp/*.arm` marker. |
| `TOOIE_DIAG_P09_SEAM_EDGE_DRY_RUN_V2` | Seam Search | `0` (OFF) | Boundary-edge dry-run search. Writes `/tmp/tooie-p09-seam-edge-dry-run-v2.txt`. |
| `TOOIE_DIAG_RSP_ENDPOINT_BINDING` | Graphics/RSP | `0` (OFF) | Matrix decomposition and temporal cohort telemetry. Writes `/tmp/tooie-rsp-endpoint-binding-status.txt`. |
| `TOOIE_DIAG_P09_SELECTOR_AUDIT` | Graphics/RSP | `0` (OFF) | Audits mixed single vs multi-usage of WORLD transform indices per workload. |
| `TOOIE_TEST_MAP_VIS_ASPECT_AWARE` | Culling | `1` (ON) | Aspect-aware map geometry culling (`func_800CAA24`). Set `0` to disable. |
| `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE`| Culling | `1` (ON) | Aspect-aware actor frustum culling (`func_800CACEC`). Set `0` to disable. |

### 3.2 C++ Diagnostic API (`include/tooie_diag.h`)
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

    // RT64 telemetry probes
    uint64_t tooie_diag_rt64_matrix(...);
    void tooie_diag_rt64_vertex(...);
    void tooie_diag_rt64_feather_tri(...);
    void tooie_diag_rt64_match(...);
}
```

---

## 4. Operational Safety Protocols

1. **Mandatory Diagnostic Flag Rule**:
   - Because TooieRecomp defaults `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1` to ensure glitch-free gameplay, any diagnostic or test measuring high-framerate interpolation **MUST explicitly specify `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0`**. Otherwise, the workaround will mask mesh deformations.
2. **Atomic Status Emission**:
   - Status files in `/tmp` must be written to temporary files and atomically renamed upon completion to prevent race conditions or partial reads by monitoring processes.
3. **Finite Execution Caps & Circuit Breakers**:
   - Heavy diagnostic routines must enforce strict maximum attempt counts (e.g. max 24 heavy attempts) and emergency bailouts (`EMERGENCY_LIMIT_HIT=YES`) to prevent hanging or unbounded heap growth.
4. **No GPU Readback**:
   - Runtime diagnostics must evaluate data on CPU before or during command submission; stalls waiting for GPU fences or buffer mapping during active rendering are prohibited.
