---
title: "TooieRecomp Spec — Audio Subsystem and RSP Microcode"
description: "Comprehensive technical specification for n_aspMain RSP microcode execution, M_AUDTASK audio task dispatch, SoundManager event scheduling, and audio glitch status."
status: current
area: "specs/Tooie"
updated: 2026-10-19
tags: [specs, tooierecomp, audio, rsp, n_aspmain, soundmanager]
---

# Audio Subsystem and RSP Microcode Specification

## 1. Subsystem Architecture

The audio pipeline executes original Banjo-Tooie audio synthesis through static RSP recompilation of Rare's custom audio microcode (`n_aspMain`), streaming 16-bit linear PCM audio buffers to host audio hardware via SDL2 queue-based streaming (`SDL_QueueAudio`).

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
Native RSP Microcode Execution (rsp/n_aspMain.cpp, text_size: 0x17D0)
                        |
                        v
Stereo 16-bit PCM Audio Buffer (Variable Guest Rate via osAiSetFrequency)
                        |
                        v
SDL2 Audio Device Sink (SDL_QueueAudio, src/main/main.cpp (queue_samples/reset_audio), callback=nullptr)
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Implementation Status |
| :--- | :--- | :--- | :--- |
| **REQ-AUD-01** | RSP Microcode Recomp | `n_aspMain` microcode translates to C++ and compiles without errors. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-AUD-02** | M_AUDTASK Execution | `OSTask` of type `M_AUDTASK` dispatches cleanly to recompiled microcode. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-AUD-03** | Audio Thread Priority | `Thread 4` preempts game thread via Hybrid F' cooperative yields. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-AUD-04** | SoundManager Delta Events| SoundManager delta queue processes event dismissals on schedule. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-AUD-05** | Audio Glitch Resolution | Eliminate intermittent "tic tic" popping artifact. | `PARTIALLY VALIDATED` (Intro fix confirmed; global campaign UNTESTED) |
| **REQ-AUD-06** | Soak & Buffer Stability | P07-T07 soak test detects underrun, drift, and buffer leak. | `PENDING` / `UNTESTED` |
| **REQ-AUD-07** | BGM Volume Control | Scale sequence player volume linearly (0–100%) via single-scale wrapper and live fade updates without mutating authored fades in RDRAM. | `IMPLEMENTED` / `VALIDATED (SCOPED)` |

---

## 3. Interface & Calling Contracts

### 3.1 RSP Microcode Contract (`n_aspMain`)
- **Microcode Text Size**: `0x17D0` bytes (Rare proprietary ASP microcode; `0x17D0` represents microcode `text_size`, not a hash).
- **Configuration**: Defined in `n_aspMain.us.rev0.toml`.
- **Target Implementation**: `rsp/n_aspMain.cpp`.
- **Execution Hook**:
  - Guest game code issues `osSpTaskStart(&audtask)`.
  - When `audtask.t.type == M_AUDTASK`, `src/game/recomp_api.cpp` invokes the recompiled RSP entry point:
    ```cpp
    extern "C" void n_aspMain_entry(uint8_t* rdram, recomp_context* ctx);
    ```
  - Recompiled microcode reads commands from DMEM, processes voice waveforms, applies ADSR envelopes, filters samples, and writes finished PCM data back to RDRAM.

### 3.2 Host Audio Streaming Interface
- **Audio Device Sinking**: Handled directly in `src/main/main.cpp (queue_samples/reset_audio)` via `SDL_QueueAudio` (line 438) with `callback = nullptr` (line 517), rather than an SDL audio callback in `RecompFrontend`.
- **Stream Formats & Conversion**:
  - Guest output: 16-bit signed stereo PCM at dynamic guest sample rate configured via `osAiSetFrequency` (e.g. `func_8001268C.s:48` requests `0x5622` = 22,050 Hz STATIC_MIPS; rate is dynamically controlled and not universally fixed).
  - Host output: Requested as 32-bit float stereo at 48,000 Hz (`obtained` in runtime `NOT_CAPTURED`). Converted and enqueued into SDL2.
- **Queue Timing Output-Parameter Formula (Default Fix)**:
  - In `src/main/main.cpp`, queue duration calculation divides enqueued host output bytes by output format parameters:
    ```cpp
    uint64_t cur_queued_microseconds = uint64_t(sdl_queued_bytes_before) / (output_channels * sizeof(float)) * 1000000 / output_sample_rate;
    ```
  - Corrects previous calculation that erroneously divided host float32 48 kHz bytes by guest input rate/channels, which caused spurious skip decimation when queue reached 46 ms.
- **Diagnostic Infrastructure (Default OFF)**:
  - Controlled via `TOOIE_AUDIO_CAPTURE_DIR` (absolute directory outside repo) and `TOOIE_AUDIO_CAPTURE_SECONDS` (10..20s, default 12s; 2400 chunks / 24 MiB cap).
  - Thread-safe ring buffer with post-window asynchronous flush worker; hot path uses a single atomic load when inactive.
- **Telemetry Hook**:
  ```cpp
  void tooie_diag::note_queue_samples(size_t sample_count, uint32_t queued_bytes_after,
                                      int16_t peak_abs = 0, size_t nonzero = 0);
  void tooie_diag::note_audio_device(uint32_t device_id, const char* driver, const char* device_name);
  ```

### 3.3 SoundManager Thread Interaction (Thread 4)
- **Role**: Dispatches voice parameters, sequence updates, and envelope transitions.
- **Preemption Invariant**:
  - In original hardware, Thread 4 interrupts the main game loop (`Thread 5`).
  - In TooieRecomp, `func_800C2AB8` yields CPU execution via `ultramodern::yield_self_1ms()`.
  - Without this yield, `Thread 4` starves, `table[2].word0` is never cleared, `func_8001EE34` stays pending, and Slot 7 events dead-end (`p08-slot7-scheduled-event-validation.md`).

### 3.4 BGM Sequence Player Volume Contract (`include/tooie_bgm_volume.h`, `include/tooie_bgm_hooks.h`)
- **Memory Layout & Game Structures**:
  - Six sequence players located at `0x80079FA0` (stride `0x274`, `seqp = player + 0x1E0`).
  - Authored master volume (`int16_t`) stored at `player + 0x0`.
  - Six music tracks located at `0x801357D0` (stride `0x50`). Active track id (`int16_t`) at `track + 0x28` (`< 0` indicates inactive/cleared track).
- **Event Dispatch Paths**:
  - Music sequence player: `func_80025ED0` (`AL_SEQP_VOL_EVT`, event type 10).
  - Sound effect player: `func_80020BE0` (event type 3) — left untouched.
- **Lookup Overrides (`src/game/boot_hw_stubs.cpp`)**:
  - `0x80025ED0` -> `tooie_bgm_on_csp_set_vol`: Scales incoming `r5` once if `r4` matches one of the 6 known sequence player pointers (`tooie_bgm::slot_for_seqp`), replacing with `apply_unscaled(authored, percent)`. Unknown `seqp` passes through unmodified.
  - `0x800FB968` -> `tooie_bgm_on_update`: Runs after original `func_800FB968`. For active tracks (`track + 0x28 >= 0`), reads unscaled volume from `player + 0x0` and reposts scaled volume via original `func_80025ED0` using a local copy of `recomp_context` strictly when live target gain changed. Steady 100% emits zero extra events. Authored fades are never mutated in RDRAM.
- **Launcher Preview IPC**:
  - Linux launcher preview child process connects via datagram `socketpair` (`CLOEXEC`). Child dups read fd to `stdin` (fd 0) with `SO_TYPE` validation (`SOCK_DGRAM`). Parent 5ms monitor loop sends 1 byte only on percent change (`MSG_NOSIGNAL`). Child `update_gfx` drains atomic gain.
- **Verification**:
  - Validated by unit tests (`tooie_bgm_volume_test`, `tooie_bgm_hooks_test`) and acoustic captures via owned null sink `tooie_bgm_menu_qa` (preview 100/0/50/100 restored) and live in-game dynamic restoration (0 to 100 without restart).

---

## 4. Operational Defect Status: Audio Deferral and Testing Scope

1. **Intermittent "Tic Tic" Artifact & Resolution Status**:
   - **Symptom**: Intermittent clicking/popping artifact ("tic tic") audible during intro/demos (USER_OBSERVED).
   - **Current Baseline**: Git HEAD `5af4510744ddc90d25f9223ab48e9cff76f5baca` (2026-10-19).
   - **Resolution Status**: `USER_CONFIRMED_FOR_TESTED_INTRO_RECORDING`.
   - **Production Fix**: Corrected queue-time formula active by default in `src/main/main.cpp`. Runtime capture confirmed 0/579 chunks skipped (down from 47 spurious skips).
   - **User Verification**: User audition of `intro-queue-time-fix.wav` confirmed the artifact disappeared for the tested intro recording.
   - **Scope Limitation**: No global audio campaign fix is claimed. Residual decimation for genuine queue overflows (>100 ms) remains unchanged.

2. **Validation Scope & Untested Soak Testing**:
   - Initial playback of music, ambient sounds, and jingles is confirmed functional in Spiral Mountain and early areas as prior unrepeated QA baseline (`ANTERIOR NÃO RERODADA`).
   - REQ-AUD-05 is partially validated for intro playback; full-game audio campaign parity remains UNTESTED.
   - P07-T06 remains partially validated.
   - Task `P07-T07` soak testing (measuring buffer drift, underrun frequency, and long-term queue stability during sustained execution) remains **PENDING / UNTESTED**.

3. **BGM Volume Control Validation Scope**:
   - Sequence player volume attenuation (0–100%) is verified in unit tests, launcher preview IPC, and live in-game restoration.
   - Scope boundaries: Static sequence players 0..5, tracks 0..5, and CSP event 10 are PROVEN. Whether all character voice clips route through sequence players or SoundPlayer remains `UNKNOWN`; SoundPlayer path (`80020BE0`) is left untouched. On 2026-10-20 the user confirmed only effects/voices in the tested final demo clip with BGM at 0% (`USER_OBSERVED / USER_CONFIRMED_FOR_TESTED_DEMO_CLIP`); global audio parity is NOT claimed.
