---
title: "ANGRYLION_CONTROL — Mupen + Angrylion RDP Plus + CXD4 LLE"
description: "Pinned measurement oracle for TooieRecomp P08/P09 parity. Angrylion is not the TooieRecomp renderer; the port stays on RT64."
status: current
type: spec
project: TooieRecomp
phase: P08
updated: 2026-08-21
tags: [tooie-recomp, p08, oracle, angrylion, mupen64plus, cxd4, lle, rt64]
---

# ANGRYLION_CONTROL — Mupen + Angrylion RDP Plus + CXD4 LLE

## Role

`ANGRYLION_CONTROL` is the hardware-faithful **measurement/reference** used
against `RT64_CONTROL` (TooieRecomp + RT64). It is **not** a production
renderer and must not replace RT64 in `TooieRecompiled`.

| Side | Process | Video | RSP | CPU |
| --- | --- | --- | --- | --- |
| Oracle | `tools/oracle/angrylion-control/run.sh` | Angrylion RDP Plus | CXD4 LLE | Mupen Pure Interpreter |
| Target | `./build/TooieRecompiled` | RT64 | Tooie RSP recomp | native recompiled C |

The plugin reports itself as **`angrylion's RDP Plus nightly-build`**. That
string is `GIT_TAG` in Angrylion `src/core/version.h` when the tree is built
from `master`, not a separate nightly installer.

A Windows BizHawk copy at `~/.local/opt/BizHawk-2.11.1-win/dll/` is **not**
this oracle (`angrylion-rdp.dll`, different plugin).

## Layout

Tracked (in git):

```text
tools/oracle/angrylion-control/
  pins.env
  patches/mupen64plus-core-p08-instrumentation.patch
  cfg/mupen64plus.cfg          # placeholders; run.sh fills them
  run.sh
  scripts/{common,fetch,build,status}.sh
tools/p08_attract_egg_hook.h   # instrumentation copied into Mupen core
```

Local prefix (gitignored `work/`):

```text
tools/oracle/angrylion-control/work/
  mupen/     # core, ui, audio, input, rsp-hle, PREFIX inst/
  lle/       # angrylion-rdp-plus, mupen64plus-rsp-cxd4, plugins/, cfg/
```

This tree was relocated from `/tmp/p08-m64p` and `/tmp/p08-m64p-lle`.
Compatibility symlinks remain at those `/tmp` paths on this machine so older
commands still resolve. New work must use `run.sh`.

Rice Video (`mupen64plus-video-rice`) may still exist under `work/mupen` as
the historical `ORIGINAL_RICE_CONTROL`. It is **invalid** for CPU-visible
Z-buffer / camera classification ([[../../plan/evidence/p08-demo-zbuffer-oracle-sol-review]]).
Do not select it for new captures.

## Pins

Source of truth: `tools/oracle/angrylion-control/pins.env`.

| Component | Repository | Commit |
| --- | --- | --- |
| mupen64plus-core | https://github.com/mupen64plus/mupen64plus-core | `6dca4c15370ac3e2171ce7b31426695f8f39b460` |
| ui-console | https://github.com/mupen64plus/mupen64plus-ui-console | `c8ac4862a019d7885b24927d9b4db5dd3e42a528` |
| audio-sdl | https://github.com/mupen64plus/mupen64plus-audio-sdl | `2faed1c7e62c5f292948e7cd2398c184970cf794` |
| input-sdl | https://github.com/mupen64plus/mupen64plus-input-sdl | `842c39e89749aa3a8d02202b2afddd20b29cdfdb` |
| rsp-hle (installed, unused in LLE run) | https://github.com/mupen64plus/mupen64plus-rsp-hle | `8a7a472a7172eb2c8725b305eae26818ed7b51a2` |
| Angrylion RDP Plus | https://github.com/ata4/angrylion-rdp-plus | `9c8b9ed3e7d7f00dff8bc872ccdd3fba1a3673fc` |
| rsp-cxd4 | https://github.com/mupen64plus/mupen64plus-rsp-cxd4 | `00906a92641c540a64e4e7505012157315314810` |

```text
CPU_MODE:                PURE_INTERPRETER (--emumode 0, core NO_ASM=1)
VIDEO_PLUGIN:            angrylion's RDP Plus v1.6.0 (GIT_TAG nightly-build)
RSP_PLUGIN:              Static Interpreter (cxd4) v0.1.1
ROM_SHA1:                af1a89e12b638b8d82cc4c085c8e01d4cba03fb3
ROM_FILE:                cartridge NTSC-U Rev. 0 (Banjo-Tooie (USA).z64)
                         NOT TooieRecomp/build/bt.n64.us.1.0.z64 (decompressed)
```

Do not bump a pin without a new evidence note.

## Commands

First time on a new machine (this checkout already has `work/` locally):

```sh
tools/oracle/angrylion-control/scripts/fetch.sh
tools/oracle/angrylion-control/scripts/build.sh
tools/oracle/angrylion-control/scripts/status.sh
```

`build.sh` is a no-op when the binaries already exist. Pass `--force` to rebuild.
Core flags: `OSD=0 VULKAN=0 NO_ASM=1` (OSD=0 avoids a GLU host dependency).

Run the attract / USER_DEMO_4 oracle (needs `DISPLAY`):

```sh
tools/oracle/angrylion-control/run.sh
# extra mupen flags:
tools/oracle/angrylion-control/run.sh --set 'Video-AngrylionPlus[ViVsync]=False'
```

Cartridge path: `TOOIE_CART_Z64`, else
`TooieRecomp/../Banjo-Tooie (USA).z64`. SHA-1 is checked before launch.

TooieRecomp side (unchanged):

```sh
TOOIE_DIAG_ATTRACT=1 TOOIE_AUTO_START=1 ./build/TooieRecompiled
```

Join captures by `stage` / `sub` / consumed glrecord index / pad bytes, not
wall-clock. Angrylion dumps from `tools/p08_attract_egg_hook.h` go to
`/tmp/p08-angrylion-*.csv` / `/tmp/p08-angrylion-ud4-cam.log` (logging only).

## Instrumentation

`patches/mupen64plus-core-p08-instrumentation.patch` inserts PC/syscall probes
into Mupen `pure_interp.c` and includes `p08_attract_egg_hook.h`. The hook is
copied from `tools/p08_attract_egg_hook.h` at build time. It does not change
RDP/RSP behavior.

`p09_egg_hook.h` in the old `/tmp` core tree is leftover and **unused**.

## Config that is in scope

From `cfg/mupen64plus.cfg` after `run.sh` substitution:

```text
R4300Emulator = 0
VideoPlugin   = work/lle/angrylion-rdp-plus/build/mupen64plus-video-angrylion-plus.so
RspPlugin     = work/lle/mupen64plus-rsp-cxd4/projects/unix/mupen64plus-rsp-cxd4-sse2.so
AudioPlugin   = dummy
[rsp-cxd4] DisplayListToGraphicsPlugin = False
[Video-AngrylionPlus] Parallel=True NumWorkers=0 ViMode=0 DpCompat=0
```

`~/.config/mupen64plus/mupen64plus.cfg` still points at Rice under the old
plugin dir. `run.sh` uses `--configdir` + `--nosaveoptions` and does **not**
read that user cfg.

## Invariants

- Angrylion remains measurement-only.
- TooieRecomp remains RT64.
- Do not ship or version the ROM.
- Do not vendor `work/` or `.so` files.
- P08 = ACTIVE / MANUAL_VALIDATION_PENDING; P09 = PENDING / NOT_FORMALLY_OPEN.
