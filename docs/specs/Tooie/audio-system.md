---
title: "TooieRecomp Spec — Audio Subsystem and RSP Microcode"
description: "Comprehensive technical specification for n_aspMain RSP microcode execution, M_AUDTASK audio task dispatch, SoundManager event scheduling, and audio glitch status."
status: current
area: "specs/Tooie"
updated: 2026-10-16
tags: [specs, tooierecomp, audio, rsp, n_aspmain, soundmanager]
---

# Audio Subsystem and RSP Microcode Specification

## 1. Subsystem Architecture

The audio pipeline executes original Banjo-Tooie audio synthesis through static RSP recompilation of Rare's custom audio microcode (`n_aspMain`), streaming 16-bit linear PCM audio buffers to host audio hardware via SDL2.

```text
Game Logic Audio Requests (Music, SFX, Jingles)
                        |
                        v
SoundManager Delta Queue (Thread 4 [AUDIO], Priority 12)
                        |
                        v
OSTask Construction (Type: M_AUDTASK, Flags: 0)
                        |
                        v
Native RSP Microcode Execution (rsp/n_aspMain.cpp, Hash: 0x17D0)
                        |
                        v
Stereo 16-bit PCM Audio Buffer (32000 Hz)
                        |
                        v
SDL2 Audio Device Sink (tooie_diag::note_queue_samples)
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Implementation Status |
| :--- | :--- | :--- | :--- |
| **REQ-AUD-01** | RSP Microcode Recomp | `n_aspMain` microcode translates to C++ and compiles without errors. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-AUD-02** | M_AUDTASK Execution | `OSTask` of type `M_AUDTASK` dispatches cleanly to recompiled microcode. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-AUD-03** | Audio Thread Priority | `Thread 4` preempts game thread via Hybrid F' cooperative yields. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-AUD-04** | SoundManager Delta Events| SoundManager delta queue processes event dismissals on schedule. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-AUD-05** | Audio Glitch Resolution | Eliminate intermittent "tic tic" popping artifact. | `PLANNED` / `DEFERRED` |
| **REQ-AUD-06** | Soak & Buffer Stability | P07-T07 soak test detects underrun, drift, and buffer leak. | `UNTESTED` / `PENDING` |

---

## 3. Interface & Calling Contracts

### 3.1 RSP Microcode Contract (`n_aspMain`)
- **Microcode Hash**: `0x17D0` (Rare proprietary ASP microcode).
- **Configuration**: Defined in `n_aspMain.us.rev0.toml`.
- **Target Implementation**: `rsp/n_aspMain.cpp`.
- **Execution Hook**:
  - Guest game code issues `osSpTaskStart(&audtask)`.
  - When `audtask.t.type == M_AUDTASK`, `src/game/recomp_api.cpp` invokes the recompiled RSP entry point:
    ```cpp
    extern "C" void n_aspMain_entry(uint8_t* rdram, recomp_context* ctx);
    ```
  - The recompiled microcode reads commands from DMEM, processes voice waveforms, applies ADSR envelopes, filters samples, and writes finished PCM data back to RDRAM.

### 3.2 Host Audio Streaming Interface
- **Audio Device Sink**: Handled by SDL2 audio callback in `RecompFrontend`.
- **Telemetry Hook**:
  ```cpp
  void tooie_diag::note_queue_samples(size_t sample_count, uint32_t queued_bytes_after,
                                      int16_t peak_abs = 0, size_t nonzero = 0);
  void tooie_diag::note_audio_device(uint32_t device_id, const char* driver, const char* device_name);
  ```
- **Buffer Parameters**: 16-bit signed stereo samples at 32,000 Hz.

### 3.3 SoundManager Thread Interaction (Thread 4)
- **Role**: Dispatches voice parameters, sequence updates, and envelope transitions.
- **Preemption Invariant**:
  - In original hardware, Thread 4 interrupts the main game loop (`Thread 5`).
  - In TooieRecomp, `func_800C2AB8` yields CPU execution via `ultramodern::yield_self_1ms()`.
  - Without this yield, `Thread 4` starves, `table[2].word0` is never cleared, `func_8001EE34` stays pending, and Slot 7 events dead-end (`p08-slot7-scheduled-event-validation.md`).

---

## 4. Operational Defect Status: Audio Deferral and Testing Scope

1. **Intermittent "Tic Tic" Artifact**:
   - **Symptom**: An intermittent clicking/popping artifact ("tic tic") is audible during gameplay and cutscenes.
   - **Proven Facts**:
     - Present from the very first audio boot in P07.
     - Not caused by the P08 preemption patch or the P09 interpolation workaround.
   - **Operational Directive**:
     - The user has explicitly directed that this audio artifact is **DEFERRED** to a future dedicated audio phase.
     - **Invariant**: No speculative audio adjustments, buffer modifications, or thread priority hacks are permitted during current phases.

2. **Validation Scope & Untested Soak Testing**:
   - Initial playback of music, ambient sounds, and jingles is confirmed functional in Spiral Mountain and early areas.
   - However, **full-game audio campaign parity is UNTESTED**.
   - `P07-T07` soak testing (measuring buffer drift, underrun, and long-term sample queue leaks over extended play sessions) remains **UNEXECUTED / PENDING**.
