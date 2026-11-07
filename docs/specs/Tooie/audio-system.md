---
title: "TooieRecomp Spec — Audio Subsystem and RSP Microcode"
description: "Technical specification for n_aspMain RSP microcode execution, M_AUDTASK audio task dispatch, SoundManager event scheduling, and audio stream management."
status: current
area: "specs/Tooie"
updated: 2026-10-31
tags: [specs, tooierecomp, audio, rsp, n_aspmain, soundmanager]
---

# Audio Subsystem and RSP Microcode Specification

## 1. Subsystem Architecture

The audio pipeline executes original Banjo-Tooie audio synthesis through static RSP recompilation of Rare's custom audio microcode (`n_aspMain`), streaming 16-bit linear PCM audio buffers converted to 32-bit float stereo at 48,000 Hz to host audio hardware via SDL2 queue streaming (`SDL_QueueAudio`).

```text
Game Logic Audio Requests (Music, SFX, Jingles)
                        |
                        v
SoundManager Delta Queue (Thread 4 [AUDIO])
                        |
                        v
OSTask Construction (Type: M_AUDTASK, Flags: 0)
                        |
                        v
Native RSP Microcode Execution (rsp/n_aspMain.cpp, text_size: 0x17D0)
                        |
                        v
Stereo PCM Conversion & Decimation Threshold Guard (>100 ms)
                        |
                        v
SDL2 Audio Output Sink (SDL_QueueAudio, src/main/main.cpp)
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Status |
| :--- | :--- | :--- | :--- |
| **REQ-AUD-01** | RSP Microcode Recomp | `n_aspMain` microcode translates to C++ and compiles without errors. | `IMPLEMENTED` |
| **REQ-AUD-02** | M_AUDTASK Execution | `OSTask` of type `M_AUDTASK` intercepted in `src/main/main.cpp` and dispatched cleanly. | `IMPLEMENTED` |
| **REQ-AUD-03** | Audio Thread Priority | `Thread 4` executes via generator loop backedge safepoints. | `IMPLEMENTED` |
| **REQ-AUD-04** | SoundManager Delta Events| SoundManager delta queue processes event dismissals on schedule. | `IMPLEMENTED` |
| **REQ-AUD-05** | Audio Stream Stability | Correct host output queue calculation prevents spurious buffer skips. | `IMPLEMENTED` |
| **REQ-AUD-06** | Extended Soak Testing | Formal multi-hour audio buffer drift and soak validation across campaign. | `PARTIAL` |
| **REQ-AUD-07** | BGM Volume Control | Scale sequence player volume linearly (0–100%) via intercept hooks. | `IMPLEMENTED` |

---

## 3. Interface & Calling Contracts

### 3.1 RSP Microcode Contract (`n_aspMain`)
- **Microcode Definition**:
  - Rare custom ASP microcode configured in `n_aspMain.us.rev0.toml` (`text_size = 0x17D0`).
  - Compiled directly into the main executable as `rsp/n_aspMain.cpp`.
- **Execution Hook (`src/main/main.cpp`)**:
  - The runtime intercepts audio tasks when `task->t.type == M_AUDTASK` in `src/main/main.cpp`.
  - Dispatches execution to the recompiled RSP entry point:
    ```cpp
    extern "C" void n_aspMain_entry(uint8_t* rdram, recomp_context* ctx);
    ```
  - When enabled via `TOOIE_LOG_RSP`, outputs audio task parameters.
  - Microcode reads audio commands from DMEM, processes voice waveforms, applies ADSR envelopes, and outputs finished PCM audio buffers to RDRAM.

### 3.2 Host Audio Streaming Interface (`src/main/main.cpp`)
- **Audio Device Sinking**:
  - Managed in `src/main/main.cpp` via `SDL_QueueAudio` without an SDL audio callback (`callback = nullptr`).
  - Dynamic guest rate: Provided by `osAiSetFrequency` (typically 22,050 Hz or 32,000 Hz).
  - Host output format: 32-bit floating-point stereo (`AUDIO_F32`) at 48,000 Hz (`output_sample_rate = 48000`).
- **Queue Timing Calculation**:
  - Queue duration evaluates host output bytes divided by output stream parameters:
    ```cpp
    uint64_t cur_queued_microseconds = uint64_t(sdl_queued_bytes_before) / (output_channels * sizeof(float)) * 1000000 / output_sample_rate;
    ```
  - Safeguard: When queue exceeds 100 ms (`skip_factor = cur_queued_microseconds / 100000`), decimation skips excess audio chunks to prevent runaway latency.
- **Diagnostic Capture**:
  - The old PCM capture module is retained only as standalone development/test tooling. It is not linked into or
    called by the game executable; `TOOIE_AUDIO_CAPTURE_DIR` does not enable capture in the game.
- **Telemetry Hooks**:
  - Runtime audio no longer scans buffers for peak/nonzero measurements or collects extra queue sizes for telemetry.
    Queue-size queries used by latency control and guest audio scheduling remain functional requirements.
  ```cpp
  void tooie_diag::note_audio_device(uint32_t device_id, const char* driver, const char* device_name);
  ```

### 3.3 Thread Interaction & Scheduling
- **Thread Architecture**:
  - Audio processing is driven by `Thread 4` (`[AUDIO]`), interacting with main game logic in `Thread 6` (`[MAIN]`).
  - Preemption between the game loop and the audio synthesis thread is handled cooperatively through code generator backedge safepoints (`ultramodern_preemption_hint()`).

### 3.4 BGM Sequence Player Volume Contract
- **Volume Interception Hooks (`src/game/boot_hw_stubs.cpp`)**:
  - `0x80025ED0` (`tooie_bgm_on_csp_set_vol`): Intercepts CSP sequence volume events, scaling the authored master volume by the configured user percentage (0–100%).
  - `0x800FB968` (`tooie_bgm_on_update`): Updates active music tracks without mutating authored volume fades in RDRAM.
  - Reverse engineering details on player layouts and sequence track structures are maintained in [Audio Sequence Volume System](../../reverse-engineering/systems/audio-sequence-volume.md).
- **Independent Launcher Music**:
  - A host MP3/SDL player owns the launcher's audio; it stops before game start. The old guest preview/IPC was removed.
  - Local import, loop metadata, volumes and failure handling: [Launcher Startup and Backends](startup-and-backends.md).

---

## 4. Verification & Residuals

### External audio packs (prototype)

ZIP mods containing `audio.json` can replace selected CSP sequences with 48 kHz stereo MP3s. The original sequence
continues running; only its CSP volume events are muted while a validated replacement matches. Unmapped sequences,
SoundPlayer effects and guest scheduling retain their original path. Disabling a pack restores the current authored
volume, including when the user BGM percentage has not changed. This native integration requires no code-mod SDK.

`tooie_audio_mod.cpp` loads and indexes compressed files at activation (32 MiB per file, 256 MiB per pack, at most 128
rules and 20 minutes per file). Six audio-consumer slots, each with at most four synchronized layers, decode
incrementally from memory using pinned `dr_mp3`;
the guest producer only publishes slot state. Mixing occurs after conversion to 48 kHz and before the existing latency
decimator, so original and replacement sources advance and decimate together. Master and BGM gain each apply once.
Playback bounds and loops use decoded PCM frame indices; MP3 encoder delay/padding are handled by the decoder.

A sequence restart, including retriggering the same ID, resets its shared 48 kHz clock. The clock advances for every
active native sequence even without a mapped/enabled pack; enabling, disabling, reorder and instrumentation changes
do not reset it. Each selected layer opens at that clock position (in beats for layers that state their tempo),
folding through its own loop. Silent layers are not decoded; they seek back to the clock when they return.

Native layers use the current Rare CSP channel gains (32 channels), sampled after the original BGM update
(`255 -> 1`, otherwise `q / 256`); layer weights are the non-negative least-squares combination of the layer channel
groups closest to those gains (see [[modding-and-extensions]]). Weights and authored master volume use a 5 ms
dezipper; the native envelope determines the transition. This frame-sampled reconstruction is not sample-exact
native synthesis. The final sum is clamped to [-1, 1].

Format, area tempos, layers and the pack builder: [[modding-and-extensions]]. Music packs do not replace SoundPlayer
voices or reproduce per-instrument edits inside a rendered stereo track, and campaign-wide acoustic parity is not
established. The audio mod writes no logs; problems with a pack (a rejected MP3 or a failed enable) are reported on
stderr and, for enable failures, in a message box.

Host verification: `tooie_audio_mod_test <48000-Hz-stereo.mp3>`, `tooie_bgm_hooks_test`, `tooie_bgm_volume_test` and
`tooie_texture_zip_mod_test`. A fixture-free invocation of the first test explicitly skips codec playback checks.

- **Queue Formula Verification**: Host unit tests (`tests/tooie_audio_capture_test.cpp`, `tests/tooie_bgm_volume_test.cpp`, `tests/tooie_bgm_hooks_test.cpp`) validate audio capture, BGM attenuation, and volume hook stability.
- **Residual Scope**: Extended soak testing across lengthy campaign gameplay sessions remains an open validation task. Full campaign audio parity across all sound effects, speech clips, and environmental audio is evaluated progressively against the reference oracle.
