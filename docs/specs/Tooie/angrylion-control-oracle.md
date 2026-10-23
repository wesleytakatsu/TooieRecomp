---
title: "ANGRYLION_CONTROL — Mupen + Angrylion RDP Plus + CXD4 LLE"
description: "Pinned measurement oracle for TooieRecomp visual and behavioral parity. Angrylion is an offline reference; the native port renders strictly via RT64."
status: current
area: "specs/Tooie"
updated: 2026-10-23
tags: [specs, tooierecomp, oracle, angrylion, mupen64plus, cxd4, lle, rt64]
---

# ANGRYLION_CONTROL — Mupen + Angrylion RDP Plus + CXD4 LLE

## 1. System Role & Architecture

`ANGRYLION_CONTROL` is the hardware-faithful software-rendered reference used to verify TooieRecomp rendering and kinematics. It is an offline verification oracle, not a production renderer, and is supported exclusively on Linux.

| Side | Process | Video | RSP | CPU |
| :--- | :--- | :--- | :--- | :--- |
| **Oracle** | `tools/oracle/angrylion-control/run.sh` | Angrylion RDP Plus | CXD4 LLE | Mupen Pure Interpreter |
| **Target** | `TooieRecompiled` | RT64 | Tooie RSP recomp | Native recompiled C |

```text
+---------------------------------------------------------------------------------+
|                       ANGRYLION_CONTROL Linux Oracle Harness                    |
|  - Host Platform: Linux x86-64 (mupen64plus core, cxd4, angrylion-rdp-plus)     |
|  - Inputs: TOOIE_CART_Z64 (Retail 32 MiB ROM, mandatory)                        |
|  - Captures: Golden frame framebuffer, Z-buffer arrays, and pad logs            |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                         Differential Analysis Scripts                           |
|  - tools/oracle/angrylion-control/run.sh                                        |
|  - Frame comparison against RT64 captures (run-tooie.sh -A)                     |
+---------------------------------------------------------------------------------+
```

---

## 2. Directory Layout & Repository Structure

- **Tracked in repository (`tools/oracle/angrylion-control/`)**:
  - `pins.env`: Source of truth for pinned upstream repository commits.
  - `patches/mupen64plus-core-p08-instrumentation.patch`: Intercept patches for Mupen core.
  - `cfg/mupen64plus.cfg`: Configuration template evaluated by `run.sh`.
  - `run.sh`: Main invocation wrapper script.
  - `scripts/{common,fetch,build,status}.sh`: Build and environment verification scripts.
  - `tools/p08_attract_egg_hook.h`: Attract instrumentation hook.
- **Local build cache (`work/`, gitignored)**:
  - `work/mupen/`: Mupen core, audio, and UI libraries.
  - `work/lle/`: Angrylion RDP Plus and CXD4 RSP plugins.

---

## 3. Pinned Components & Configuration

All upstream Git URLs and commit pins are defined authoritatively in `tools/oracle/angrylion-control/pins.env` (including `mupen64plus-core`, `ui-console`, `audio-sdl`, `input-sdl`, `rsp-hle`, `angrylion-rdp-plus`, and `rsp-cxd4`).

### Runtime Configuration
- **CPU Mode**: Pure Interpreter (`--emumode 0`, `NO_ASM=1`).
- **Cartridge ROM Requirement**:
  - Requires the retail 32 MiB cartridge ROM (`Banjo-Tooie (USA).z64`, SHA-1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`).
  - Must be explicitly specified via the `TOOIE_CART_Z64` environment variable.
  - No automatic fallback to sibling or parent directories is performed. Decompressed images are rejected.

---

## 4. Execution Commands

### Building and Managing the Oracle (Linux only)
```sh
tools/oracle/angrylion-control/scripts/fetch.sh
tools/oracle/angrylion-control/scripts/build.sh
tools/oracle/angrylion-control/scripts/status.sh
```

### Running the Oracle
```sh
# Run oracle with dummy audio
TOOIE_CART_Z64=/path/to/cart.z64 tools/oracle/angrylion-control/run.sh

# Run oracle with SDL audio
TOOIE_CART_Z64=/path/to/cart.z64 TOOIE_ANGRYLION_AUDIO=sdl tools/oracle/angrylion-control/run.sh

# Run with custom video settings
TOOIE_CART_Z64=/path/to/cart.z64 tools/oracle/angrylion-control/run.sh --set 'Video-AngrylionPlus[ViVsync]=False'
```

### TooieRecomp Comparison Runs
```sh
# Linux launcher with attract diagnostics
./run-tooie.sh -A

# Windows launcher with attract diagnostics
.\run-tooie.ps1 -Diag -Attract
```

---

## 5. Invariants & Scope

1. **Measurement Only**: Angrylion is strictly an offline validation tool and is never bundled into or used as a runtime renderer for TooieRecomp.
2. **Platform Scope**: The automated oracle harness is implemented and supported on Linux only.
3. **ROM Separation**: Pinned oracle builds and tests never commit or distribute ROM binaries.