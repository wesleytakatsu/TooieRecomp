---
title: "Audio sequence volume and BGM fade architecture"
description: "Reverse engineering knowledge base for Banjo-Tooie sequence player memory layout, fade updates, volume setters, CSP event dispatch, and BGM attenuation contracts."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-10-20
tags: [reverse-engineering, audio, bgm, sequence-player, csp]
---

# Audio sequence volume and BGM fade architecture

## 1. System Overview

Banjo-Tooie synthesizes music sequences via Rare's Compact Sequence Player (CSP) architecture executed on the audio thread. Unlike the primary sound effect player (`SoundPlayer`), sequence playback uses six dedicated sequence player slots with per-frame software volume fades.

```text
Game Logic / SoundManager
            |
            +---> func_80017404(slot, vol) [Volume Setter: writes player+0, calls CSP]
            +---> func_80016ED0(...)        [Sequence Start: calls CSP directly]
            |
            v
Compact Sequence Player (CSP) Event Dispatch (func_80025ED0, AL_SEQP_VOL_EVT, Event 10)
            |
            v
Audio RSP Task Dispatch (n_aspMain)
```

---

## 2. Memory Layout & Structures (PROVEN / STATIC_MIPS / RUNTIME_CPU)

### 2.1 Sequence Players Array
- **Base Address**: `0x80079FA0` (`kPlayerBase`).
- **Slot Count**: 6 (`kSlotCount = 6`, slots 0 to 5).
- **Stride**: `0x274` bytes (`kPlayerStride`).
- **Address Formula**:
  $$\text{player\_addr}(\text{slot}) = 0x80079FA0 + \text{slot} \times 0x274$$
- **Key Offsets**:
  - `+0x00`: Authored master volume (`int16_t`, unscaled).
  - `+0x1E0`: Embedded Compact Sequence Player state. The `seqp` argument is the address `player + 0x1E0`; this offset is not a stored pointer field and must not be dereferenced to obtain `seqp`.

### 2.2 Music Tracks Array
- **Base Address**: `0x801357D0` (`kTrackBase`).
- **Slot Count**: 6 (slots 0 to 5).
- **Stride**: `0x50` bytes (`kTrackStride`).
- **Address Formula**:
  $$\text{track\_addr}(\text{slot}) = 0x801357D0 + \text{slot} \times 0x50$$
- **Key Offsets**:
  - `+0x28`: Active sequence / track ID (`int16_t`). Values $< 0$ denote an idle or cleared track.

---

## 3. Function Calling Contracts & Dispatch Paths (PROVEN / STATIC_MIPS)

| Symbol | Address | Calling Contract | Role |
| :--- | :--- | :--- | :--- |
| `func_80025ED0` | `0x80025ED0` | `(rdram, ctx)` with `r4=seqp`, `r5=vol` | CSP event dispatch (`AL_SEQP_VOL_EVT`, event type 10). Controls music playback volume. |
| `func_80020BE0` | `0x80020BE0` | `(rdram, ctx)` with `r4=sndp`, `r5=vol` | SoundPlayer event dispatch (event type 3). Controls sound effects (SFX). Untouched by sequence volume scaling. |
| `func_80017404` | `0x80017404` | `(slot, vol)` | Sequence volume setter. Stores unscaled `vol` at `player + 0x0`, then jumps to CSP `func_80025ED0`. |
| `func_80016ED0` | `0x80016ED0` | Original guest ABI; complete argument contract not documented here | Sequence start routine. Directly invokes CSP `func_80025ED0`, bypassing `func_80017404`. |
| `func_800FB968` | `0x800FB968` | `(rdram, ctx)` | Per-frame high-level track update and fade calculator for active sequence tracks. |

---

## 4. Refuted Hypotheses & Architectural Evolution

### 4.1 Refuted Double-Event Proposal
- **Hypothesis**: Intercept `func_80017404` to call the original function, followed by posting a second, scaled event to `func_80025ED0`.
- **Status**: `REFUTED`.
- **Findings**:
  1. The original `17404` always posts an unscaled event; paths that skip a second scaled event (for example, because gain is unchanged) leave authored gain active. The accepted wrapper scales each original CSP event once.
  2. Sequence start in `func_80016ED0` called CSP `func_80025ED0` directly, completely bypassing `func_80017404`.
  3. Active software fades driven by `func_800FB968` continuously overwrote live gains.

### 4.2 Integrated Production Architecture (PROVEN)
- **Status**: `PROVEN` (integrated via `recomp::overlays::add_loaded_function` lookup overrides in `src/game/boot_hw_stubs.cpp`).
- **Core Principles**:
  1. **Single-Scale Wrapper (`tooie_bgm_on_csp_set_vol`)**:
     - Wraps `func_80025ED0`.
     - Maps `r4` (`seqp`) to slot index via `tooie_bgm::slot_for_seqp`.
     - For recognized sequence players (slots 0..5), scales `r5` once via `apply_unscaled(authored, percent)`.
     - Unrecognized `seqp` pointers pass through without modification.
  2. **Post-Update Frame Hook (`tooie_bgm_on_update`)**:
     - Executes after original `func_800FB968`.
     - For active tracks (`track + 0x28 >= 0`), reads unscaled authored volume from `player + 0x0`.
     - Calls `take_reapply` to evaluate if the effective target volume changed since the previous event.
     - Reposts the updated scaled volume to original `func_80025ED0` using a local copy of `recomp_context`.
     - Steady-state 100% volume generates zero additional events.
  3. **Non-Destructive Invariant**:
     - Authored volume values in RDRAM (`player + 0x0`) and active fade envelopes are never modified in place.
  4. **Thread Safety**:
     - Guest thread reads volume strictly from an atomic integer (`g_percent`).
     - Configuration callbacks update the atomic percentage without capturing `Config` references.
  5. **Launcher Preview IPC**:
     - Linux launcher preview child process receives volume updates over a nonblocking datagram `socketpair` (`CLOEXEC`) on `stdin` (fd 0). Parent monitor sends 1 byte only when volume percentage changes.

---

## 5. Epistemic Classification & Boundaries

| Domain | Classification | Notes |
| :--- | :--- | :--- |
| **Sequence Player Layout** | `PROVEN (STATIC_MIPS / RUNTIME)` | 6 slots at `0x80079FA0` (stride `0x274`), `seqp` at `+0x1E0`, authored volume at `+0x0`. |
| **Track Structure Layout** | `PROVEN (STATIC_MIPS / RUNTIME)` | 6 tracks at `0x801357D0` (stride `0x50`), active id at `+0x28`. |
| **CSP Event Dispatch** | `PROVEN (STATIC_MIPS)` | `func_80025ED0` handles event type 10 (`AL_SEQP_VOL_EVT`). |
| **SoundPlayer Dispatch** | `PROVEN (STATIC_MIPS)` | `func_80020BE0` handles event type 3 (SFX). Untouched by BGM attenuation. |
| **Linear Scaling Math** | `PROVEN (UNIT_TEST)` | Evaluated and verified in `tests/tooie_bgm_volume_test.cpp`. |
| **Acoustic Attenuation** | `PROVEN (RUNTIME_QA)` | Validated with PulseAudio null sink `tooie_bgm_menu_qa` (preview stream and in-game live restoration). |
| **Voice ID Classification** | `UNKNOWN` | Whether character voice clips (e.g. mumbling dialogues) route through sequence player slots or SoundPlayer is unclassified. |
| **Global Audio Parity** | `UNKNOWN / PENDING` | Long-term soak testing (`P07-T07`) and global campaign audio parity remain unverified. |

---

## 6. Provenance & Artifacts

- **Unit Tests**:
  - `tests/tooie_bgm_volume_test.cpp`: Validates identity scaling (100%), muting (0%), attenuation (50%), numeric bounds, compounding avoidance, restoration, idle live changes, and unknown pointer classification.
  - `tests/tooie_bgm_hooks_test.cpp`: Validates hook dispatch contracts, unchanged forwarding of unknown pointers, event counts, authored memory preservation, and context copying.
- **Evidence Documents**:
  - `p08-settings-functionality-20261019`: Full test execution and acoustic QA captures.
  - [[../../specs/Tooie/audio-system]]: Subsystem specification and requirements traceability.
- **Audio Captures**:
  - `/home/wesley/.cache/tooie-menu-settings-20261019/`: `preview-100.wav`, `preview-0-real.wav`, `preview-50-real.wav`, `preview-100-restored.wav`, `game-0.wav`, `game-100-restored.wav`.
