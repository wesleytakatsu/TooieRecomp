---
title: "Audio sequence volume and BGM fade architecture"
description: "Reverse engineering knowledge base for Banjo-Tooie sequence player memory layout, fade updates, volume setters, CSP event dispatch, and BGM attenuation contracts."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-10-31
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

### 2.3 Low-level sequence identity and channel selection

- **PROVEN (STATIC_MIPS)**: `func_80016ED0(a0, a1)` uses the low byte of `a0` as slot index (stride `0x274`) and stores
  the sequence ID from `a1` at `player + 0x26C` (`int16_t`). `a1 == -1` stops the player through `func_80024E90`;
  starting another sequence recursively stops the previous one. This low-level identity also covers short jingles.
- **PROVEN (STATIC_MIPS)**: a sequence asset is requested as `sequence_id + 0x1DDD` through `func_800D674C`
  (`80016FCC..80016FD4`). The CSP state word at `player + 0x20C` is waited on until zero before replacing sequence data
  (`80016F84..80016FA4`); it can still be zero during asynchronous startup.
- **PROVEN (STATIC_MIPS)**: `func_800178C4(slot, channel_mask, transition)` selects channel bits and stores the requested
  mask in the six-word table at `0x8007AED0 + slot * 4` (`80017C88..80017D2C`). It iterates 32 channel bits. A change
  in mask can change instrumentation without restarting the sequence; map/position-dependent variants must account for it.
- **PROVEN (RUNTIME_CPU)**: sequence `0x67` is requested when the native frontend enters Game Select. The external
  ROM-map listing also labels `0x32` as Spiral Mountain and `0x04` as Got Egg; these names alone do not classify a
  particular channel variant. [ROM-map research](https://hack64.net/wiki/doku.php?id=banjo_tooie:rom_map).

- **PROVEN (RUNTIME_CPU, user test)**: native sequence `0x32` remains in slot 1 while the requested channel mask
  alternates between `0x000063FF` and `0x00001C00`. The external MP3 stays selected without a new sequence start.
  Those masks have disjoint bits and together cover `0x00007FFF`.
- **STRONG_INFERENCE**: in the user's Spiral Mountain water-entry/exit test, `0x000063FF` corresponds to surface
  instrumentation and `0x00001C00` to underwater instrumentation. Explicitly annotated transition timing is still needed
  for final variant/fade QA; do not generalize these masks to other songs.
- **PROVEN (STATIC_MIPS)**: `func_800178C4` changes channel gains without restarting the sequence. Its third argument
  participates in calculating the transition (`80017934..80017B38`); the native fade's exact time and curve have not
  been measured for the prototype. A fixed host fade is not evidence of native equivalence.

### 2.4 Native channel gain envelopes

- **PROVEN (STATIC_MIPS)**: `func_80017810(slot)` returns the embedded `seqp`. Its `+0x60` word points to channel
  records of stride `0x38`. The record's `+9` byte is current gain, `+0xA` target, `+0xB` transition control and
  `+0xC` a floating-point step (`80023EC4..80023F44`). These are Rare's fields, not the stock libultra channel layout.
- **PROVEN (STATIC_MIPS)**: `func_800262DC` posts custom controller `0xFC`; its handler `8002418C..800241D0`
  immediately writes both current and target gain. `func_80026238` posts controller `0xFF`; `80023CE4..80023EB4`
  computes the step from current/target and the low seven transition-control bits, then schedules controller `0xFE`.
- **PROVEN (STATIC_MIPS)**: `0xFE` handler `80023EB8..80024184` moves current gain toward target, clamps the final
  step and reschedules while unequal (`800240D0..80024128`). Reversals use the current gain. Its scheduling depends
  on CSP timing and the step; a constant host crossfade is not an equivalent implementation.
- **PROVEN (STATIC_MIPS)**: voice gain calculation `80029DCC..80029E24` reads channel `+9`; `255` bypasses scaling,
  otherwise it applies `(base_volume * q + 1) >> 8`. Host normalization `q / 256` follows this channel multiplier,
  while not attempting to duplicate integer rounding for each native voice.
- **PROVEN (HOST_TEST)**: shared-clock MP3 layers seek to the decoded phase at 1:20 through repeated loops and
  retain it through native gain changes and rapid reversals. This proves the host mechanism, not native acoustic
  parity or the surface/underwater interpretation.
- **PROVEN (USER_REPORT, Windows)**: the user reported that version `0.2.0` appears to work correctly after testing
  the synchronized transition. This is positive perceptual QA, not a measured native fade/phase comparison; no
  additional log from that run was analyzed. Campaign-wide acoustic parity remains **UNKNOWN**.

### 2.5 Area channel presets and the jukebox

- **PROVEN (RUNTIME_CPU, renderer)**: Rare's player drives **32 channels** per sequence. The channel records at
  `seqp + 0x60` (stride `0x38`, gain `+9`) exist for all 32: requesting mask `0x41C00000` for sequence `0x39` sets
  exactly channels 22, 23, 24 and 30 to gain `0xFF`, and `0x01FE0000` for `0x32` sets channels 17-24. Area masks use
  bits up to 31, so a 16-channel model misses most area variants.
- **PROVEN (STATIC data)**: the area channel presets live in core2 data at ROM `0x1EFD774` (VRAM `0x80123E84`,
  decompressed USA image), 72 entries of 12 bytes `{u32 mask_a, u32 mask_b, u16 bpm_a, u16 bpm_b}` (`0x00` and `0x31`/`0x45` are all-channels entries;
  the table ends at `0x47`, floats follow). Neighbouring presets belong to one sequence, for example `0x01..0x02`
  Spiral Mountain (`0x32`), `0x03..0x0A` Mayahem Temple (`0x39`), `0x3B..0x44` Isle o' Hags (`0x58`).
- **PROVEN (STATIC data)**: Jolly's jukebox (overlay `chmusicmenu`, ROM data `0x2081C70`) lists 48 sequence IDs, then
  95 entries `{s16 sequence, u8 flag, u8 preset}` (`preset 0xFF` = whole sequence), in the same order as the jukebox
  track names in text assets `0x1851..0x1859`, `0x1869`, `0x186A`, `0x186B` (worlds in `0x1850`). Example:
  `(0x32, 0, 1)` SPIRAL MOUNTAIN, `(0x32, 0xFF, 1)` MOUNTAIN AQUATIC, `(0x32, 0, 2)` MOUNTAIN CAVES.
- **PROVEN (STATIC_MIPS)**: `func_800FCFB0(track, sequence, preset, underwater)` applies a preset: with `underwater`
  nonzero it uses `mask_b`/`bpm_b` (`+0x4`, `+0xA`), otherwise `mask_a`/`bpm_a` (`+0x0`, `+0x8`). A tempo of `0xFFFF`
  is skipped; otherwise it calls `func_800FCED0(sequence, 60000000 / bpm, transition)` (tempo) and
  `func_800FCDE0(sequence, mask, transition)` (mask), which reach the slot's `func_80017D74` and `func_800178C4`.
- **PROVEN (STATIC_MIPS)**: `func_80017D74(slot, microseconds_per_beat, transition)` keeps the request at
  `0x8007AEA0 + slot * 4` (ignoring a repeated value) and the per-frame step at `0x8007AEB8 + slot * 4`; a transition
  of `0.0` posts the tempo at once through `func_80026330` (CSP meta event `FF 51`), otherwise it ramps at 30 steps
  per second of transition. `func_800260B0(seqp)` returns the current tempo as `seqp+0x24` divided by the float at
  `(*(seqp+0x18))+8`. The sequence's own tempo event at its start overrides a request made before it plays.
- **PROVEN (RUNTIME_CPU, renderer)**: with the preset tempo applied, the native renders match the album's area tracks
  (for example Mayahem Temple underwater at 105 BPM, Glitter Gulch Mine underwater at 100 BPM, Witchyworld Haunted
  Zone at 170 BPM, all 0.9-0.99), and the album's loop lengths scale by the tempo ratio (Mayahem Temple 122 s at
  120 BPM, underwater 139.43 s at 105 BPM). Loops of the 13 layered sequences are whole numbers of beats (160-378).
- **STRONG_INFERENCE**: channel groups loop with periods of their own (Glitter Gulch Mine near the Space Zone repeats
  every 228 beats while the main arrangement repeats every 256; Stomping Plains 304 against 296), consistent with
  Rare's per-channel loop markers. Preset `0x46` (`0xB000`, 75 BPM) is Jinjo Village drained (sequence `0x59`).
- **PROVEN (RUNTIME_CPU, renderer)**: the maintainer renderer (`TOOIE_TEST_AUDIO_RENDER`, see
  [[../../specs/Tooie/modding-and-extensions]]) plays a sequence with `func_80016ED0(slot, id)`, its volume with
  `func_80017404(slot, 0x7FFF)` and a mask with `func_800178C4(slot, mask, 0)` in the Game Select slot, and the
  recorded native mix matches the album's Spiral Mountain, Banjo's House Blues and Game Select at chroma correlation
  0.98-0.997.

The external-audio integration wraps `80016ED0` in addition to the two existing volume/update hooks. It observes IDs,
channel masks and restart events while retaining the original guest sequence execution. Polling a high-level track ID
alone cannot distinguish repeated starts of the same jingle or multiple instrument variants of one sequence.

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
  5. **Launcher Audio Boundary**:
      - The host launcher now plays an independent imported MP3, without invoking the guest sequence player or its IPC preview.
        This does not change the native volume model above; host behavior is specified in
        [Launcher Startup and Backends](../../specs/Tooie/startup-and-backends.md).

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
- **Specification**: [[../../specs/Tooie/audio-system]] (subsystem specification and requirements traceability).
