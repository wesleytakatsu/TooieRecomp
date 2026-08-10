# P07 boot hang + audio silence diagnosis (2026-08-09)

## Audio device routing (not a false positive)

- Host: `SDL_OpenAudioDevice(nullptr, …)` → SDL **default** device
- Driver: `pulseaudio` (PipeWire pulse compatibility)
- Pulse sink-input for `TooieRecompiled` → sink **#58** =
  `alsa_output.usb-FELITRON_Stile_Compact_Voip_…` (system default, user headset)
- App volume 100%, not corked, not muted (`sound.json` main_volume=100)

## What runs vs what is silent

With `TOOIE_DIAG=1` (~12–18 s after auto-start):

| counter | observation |
| --- | --- |
| `rsp_aud` | grows (~30/s) — `M_AUDTASK` running |
| `queue_calls` / `queue_samp` | grows — samples **enqueued** to SDL |
| **`nonzero` / `peak` / `maxpeak`** | **always 0** — buffers are **digital silence** |
| `queue_bytes` | non-zero (~12–25 KiB) — empty PCM still occupies queue |

Conclusion: **channel is correct**; silence is **zero samples from the game**, not wrong sink.

## Boot hang (frozen screen)

Syscall stack **frozen** at depth 3:

```text
726:2  gsworldDll_entrypoint_2
  └─ 2:2  gsmaker_entrypoint_2
       └─ 708:0  gldbDll_entrypoint_0   ← never returns
```

- `syscall d=110 r=107 inflight=3` stays constant after early boot
- Nested overlay bank path loads `gldbDll` and never leaves
- Audio **thread path** keeps synthesizing/queueing silence while main world path is stuck

`gldbDll_entrypoint_0` (decomp): size query → `heap_alloc` → asset lookup →
`rom_dma_read` → XOR unpack (`func_808000E8`) → second alloc / `func_8001C2AC`.

## Config

`~/.config/TooieRecompiled/sound.json`: `main_volume=100`, `bgm_volume=100`.

## How to reproduce

```bash
cd TooieRecomp/build
TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DIAG_INTERVAL_MS=2000 ./TooieRecompiled
# watch stderr for [tooie-diag] lines
```

## Fixes applied this session

1. **`TOOIE_DIAG=1`** — counters: syscall d/r, rsp_aud, queue_*, peak/nonzero, hang stack.
2. **`__osSiRawStartDma`**: was a pure no-op → now posts SI mesg (`ultramodern::send_si_message`) **and** maintains a 64-byte PIF-RAM mirror with **CIC-NUS-6105** C/R (needed by `gldbDll`).
3. Result: `gldbDll_entrypoint_0` **returns** (no longer permanent hang on `osRecvMesg`).

## Current residual (after SI + correct CIC 6105)

| observation | meaning |
| --- | --- |
| `leave ovl=726 ep=2` | **gsworld load finishes** (was infinite gsmaker spin) |
| stack `(idle)` | no nested overlay hang |
| `peak` / `maxpeak` **> 0**, `nonzero` large | **real audio PCM** enqueued to default sink |
| `rsp_aud` advances then may plateau | audio path works; may pause depending on game state |
| Visual title/menu | still may not be full file-select (P08 gfx/input) |

CIC algorithm must match X-Scale/mupen (`n64_cic_nus_6105.c`); earlier broken lut-switch left decrypt wrong → gsmaker stream never terminated.

## User validation (2026-08-09)

- Restart with `TOOIE_AUTO_START=1`.
- **Initial music plays correctly** on system default output (FELITRON headset).
