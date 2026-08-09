# Tooie RSP / OSTask inventory (P07-T01)

Static inventory from `banjo-tooie-Recomp` (scheduler `src/core1/1E2B840.c` and
`baserom.us.yaml`). Runtime confirmation: set `TOOIE_LOG_RSP=1` when SP tasks
run (audio path only; gfx never hits `get_rsp_microcode`).

## Task types observed

| type | name | ucode (game) | host path |
| ---: | --- | --- | --- |
| 1 | `M_GFXTASK` | `gspF3DEX2.NoN.fifo` or `gspL3DEX2.fifo` | RT64 HLE (`ultramodern::submit_rsp_task` → gfx action queue) |
| 2 | `M_AUDTASK` | `n_aspMain` | recompiled `rsp/n_aspMain.cpp` via `get_rsp_microcode` |

No `M_VIDTASK` / JPEG / other types appear in the custom Tooie scheduler.

## Static OSTask slots (RDRAM templates)

- `D_8003F3B0` — audio task template (`M_AUDTASK`), filled in `func_80013E34`:
  - `ucode_boot` = `rspbootTextStart`
  - `ucode` = `n_aspMainTextStart` (VRAM `0x80037880`)
  - `ucode_data` = `n_aspMainDataStart` (VRAM `0x80042010`)
  - `data_ptr` / `data_size` from `AudioTask` ACMD list
- `D_8003F3F0` — graphics task template (`M_GFXTASK`):
  - default path `func_80013EF0` → F3DEX2 NoN fifo
  - alternate path (line ~178) → L3DEX2 fifo

## ROM blobs (decompressed.us.z64)

| name | ROM start | ROM end | size |
| --- | ---: | ---: | ---: |
| `n_aspMain` text | `0x1E4F3B0` | `0x1E50B80` | `0x17D0` |
| `rspboot` text | `0x1E50B80` | `0x1E50C50` | `0xD0` |
| `gspF3DEX2.NoN.fifo` text | `0x1E50C50` | `0x1E51FE0` | `0x1390` |
| `gspF3DEX2.fifo` text | `0x1E51FE0` | `0x1E53370` | `0x1390` |
| `gspL3DEX2.fifo` text | `0x1E53370` | `0x1E54500` | `0x1190` |
| `n_aspMain` data | `0x1E59B40` | `0x1E5A280` | `0x740` |

`gspF3DEX2.fifo` (non-NoN) is present on the ROM but the scheduler paths found
in `1E2B840.c` only assign NoN and L3DEX2.

## Host registration

```text
recomp::rsp::callbacks_t { .get_rsp_microcode = get_rsp_microcode }
ultramodern::audio_callbacks_t { queue_samples, get_frames_remaining, set_frequency }
```

Unknown non-audio types: `get_rsp_microcode` logs full OSTask fields and returns
`nullptr` (librecomp then fails the task with a diagnostic).

## P07 runtime notes (2026-08-09)

- Audio host uses **SDL default device** (not a hard-coded sink).
- Boot requires host **SI completion + CIC-NUS-6105** for `gldbDll` asset decrypt
  (see `docs/P07-boot-hang-audio-diag.md`). Without it, world load spins in
  `gsmaker_entrypoint_16` and PCM is silence.
- User validation: **title/intro music plays correctly** on system default output.
