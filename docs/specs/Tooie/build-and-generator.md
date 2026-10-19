---
title: "TooieRecomp Spec — Build System and Recompiler Generator"
description: "Detailed technical specification for the CMake build pipeline, static recompilation generator, metadata inputs, and dependency linking in TooieRecomp."
status: current
area: "specs/Tooie"
updated: 2026-10-16
tags: [specs, tooierecomp, build, cmake, n64recomp, rsprecomp]
---

# Build System and Recompiler Generator Specification

## 1. System Requirements & Architecture

The build system coordinates the compilation of statically recompiled C/C++ guest code together with host virtualization runtimes and native platform libraries into a standalone Linux x86-64 executable `TooieRecompiled`.

```text
+---------------------------------------------------------------------------------+
|                        Decompilation Assets (banjo-tooie-Recomp)                |
|  - Matching ELF: build/us/banjotooie_decompressed.elf                           |
|  - Splat configuration: baserom.us.yaml                                         |
|  - Address maps: symbol_addrs.us.txt, reloc_addrs.us.txt, overlays.us.toml      |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                        Metadata Extraction & Table Generators                   |
|  - tools/generate_metadata.py -> metadata/*.table.toml, metadata/manifest.json |
|  - tools/generate_overlay_dispatch.py -> generated/overlay_dispatch.inc.h       |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                         Static Recompilation Generators                         |
|  - N64Recomp tooie.us.rev0.recomp.toml -> RecompiledFuncs/*.c (~21,000 funcs)   |
|  - RSPRecomp n_aspMain.us.rev0.toml -> rsp/n_aspMain.cpp                        |
|  - tools/fix_rsprecomp_labels.py -> C++ label normalization                     |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                             Native Host Compilation                             |
|  - Clang / GCC (C++20) via CMake >= 3.20                                        |
|  - ModernRuntime + RT64 + RecompFrontend + RecompiledPatches                    |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|               TooieRecompiled (Standalone Linux x86-64 Executable)              |
+---------------------------------------------------------------------------------+
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Implementation Status |
| :--- | :--- | :--- | :--- |
| **REQ-BLD-01** | Toolchain Compatibility | Compiles with Clang 14+ or GCC 11+ supporting C++20 on Linux x86-64. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-BLD-02** | Metadata Determinism | `generate_metadata.py` produces identical hashes across clean runs. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-BLD-03** | Zero Undefined Symbols | Linker rejects missing symbols, unresolved relocations, or unknown syscalls. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-BLD-04** | RSP Microcode Parity | `n_aspMain.cpp` compiles cleanly and executes audio tasks at `0x17D0`. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-BLD-05** | Test Suite Targets | CMake provides isolated unit test targets runnable without game launch. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-BLD-06** | Asset Quarantine | No ROM binaries, copyrighted audio, or textures bundled in binary output. | `IMPLEMENTED` / `VALIDATED` |

---

## 3. Interface & Configuration Contracts

### 3.1 CMake Build Options
- `BK_ROOT` (Path): Path to sibling `BanjoRecomp` checkout supplying `N64ModernRuntime`, `rt64`, and `RecompFrontend`. Defaults to `../BanjoRecomp`.
- `CMAKE_BUILD_TYPE`: Standard CMake build configuration (`Release`, `RelWithDebInfo`, `Debug`).
- `CMAKE_CXX_STANDARD`: 20. Required for `std::span`, `<chrono>`, and modern template facilities.
- `TOOIE_CART_Z64`: Path to the 32 MiB NTSC-U Rev. 0 cartridge ROM used by oracle verification scripts.

### 3.2 Core Targets

| Target | Artifact | Linkage / Dependencies |
| :--- | :--- | :--- |
| `TooieRecompiled` | `build/TooieRecompiled` | `RecompiledFuncs`, `RecompiledPatches`, `rt64`, `N64ModernRuntime`, `ultramodern`, `RecompFrontend`, `SDL2`, `Threads::Threads`. |
| `RecompiledFuncs` | `build/libRecompiledFuncs.a` | Static archive of ~21,000 recompiled guest functions. Built with Clang/GCC using `-O3` or `-O2`. |
| `RecompiledPatches`| `build/libRecompiledPatches.a`| Native function overrides (`RecompiledPatches/patches.c`). Replaces weak guest symbols. |
| `tooie_rsp_endpoint_current_test` | `build/tooie_rsp_endpoint_current_test` | Standalone test for `apply_default_before_renderer_init()` (20 assertions). |
| `window_display_test` | `build/window_display_test` | Standalone test for display index and aspect ratio sanitizer (38 assertions). |
| `tintro_projection_policy_test` | `build/tintro_projection_policy_test` | Standalone test for cutscene projection compensation (7 assertions). |
| `p09_exact_use_correlation_test` | `build/p09_exact_use_correlation_test` | Standalone test for seam edge diagnostic keys (1231 assertions). |
| `tooie_demo7_drift_test` | `build/tooie_demo7_drift_test` | Standalone host unit test for Demo 7 bounded kinematic monitoring. |
| `mips_rounding_test` | `build/mips_rounding_test` | Standalone test for IEEE 754 roundTiesToEven FPU rounding (21 assertions). |

### 3.3 Metadata Configuration Files
- `metadata/manifest.json`: Defines binary segments, memory layout, entry point (`0x800815C0`), and ROM CRC.
- `metadata/overlays.table.toml`: Contains index, RAM base, ROM offset, code size, data size, and BSS size for all 884 overlays.
- `metadata/syscalls.table.toml`: Mapping of syscall numbers to target overlay indices and function offsets.
- `metadata/section_table.toml`: Memory segments for core1, core2, audio, and boot sections.
- `tooie.us.rev0.toml` & `tooie.us.rev0.recomp.toml`: Recompiler configuration specifying function boundaries, stub replacements, and register tracking.
- `n_aspMain.us.rev0.toml`: RSP recompiler configuration defining DMEM/IMEM layout and command jump tables.

### 3.4 Generator, Runtime & Submodule Versioned Patch Inventory
All external patches applied to submodules/dependencies are registered in `tools/apply_runtime_patches.sh` and applied idempotently:
- **N64ModernRuntime** (`ca568b6ad79b9029d14077f0c3ffa757727c5559`):
  - `patches/runtime/n64modernruntime-scheduler.patch` (SHA256 `1b3d3d46...`): scheduler yield and prioritization.
  - `patches/runtime/n64modernruntime-preemption.patch` (SHA256 `8959055e...`): preemption runtime service, C ABI, cause-aware hold, per-thread RCP mask.
- **N64Recomp** (`2b6f05688de2abc7d86da5b4a89b84c2c6acbabe`):
  - `patches/runtime/n64recomp-tooie.patch` (SHA256 `adf97c9d...`): cumulative generator patch for same-section JAL lookup, ROUND_W_S emission, and loop backedge yield safepoints.
  - `patches/runtime/n64recomp-zero-size-static.patch` (SHA256 `48ca1e7710a7b6ecd2f4cfe3c8ac064c11b2307ad98f35ee53b70b3f1cfebb58`): fixes static function end-boundary detection by skipping zero-sized ELF symbols sharing the static address (e.g. `bsrest_VRAM` / `bsrest_TEXT_START` at `0x80800000`). Validated by `tests/n64recomp_zero_size_static_test.sh` PASS.
- **RT64** (`6f1c2d99a4ea571c139f449c326fd176ba8f3496`):
  - `patches/rt64/rt64-rsp-endpoint-current.patch` (SHA256 `49925480...`): production CURRENT `tfPos` snap, post-snap fog, CPU data path, and push constant expansion.
  - `patches/rt64/rt64-present-fps-decimator-and-counter.patch` (SHA256 `6d746f27...`): cumulative presentation decimator (`rt64_present_decimator.h`, `rt64_present_stats.h`), `viOriginalRate` write protection under `configurationMutex`, and `presentSuccessCount` increment.
  - `patches/rt64/rt64-p09-exact-use-correlation-header.patch` (SHA256 `0d4c250a...`): installs `src/render/rt64_p09_exact_use_correlation.h` for host test compilation.
  - *Historical provenance note*: Older incremental patches `rt64-present-rate-decimator.patch`, `rt64-present-success-counter.patch`, and `p09-exact-use-pathology-correlation.patch` remain for provenance, but are SUPERSEDED for clean-pin apply (failing with exit 1 on clean pin `6f1c2d99`).
- **RecompFrontend** (`d0d90ba49f46f4896aaeda362056c21b1e342561`):
  - `patches/frontend/recompfrontend-overlay-update-callback.patch` (SHA256 `37266d0b...`): registers `recompui::set_overlay_update_callback` for render-thread overlay update.

---

## 4. Invariants & Verification Protocols

1. **Generated Code Immutability**:
   - Files within `RecompiledFuncs/`, `generated/`, and `rsp/n_aspMain.cpp` are purely machine-generated.
   - Hand-editing generated code is strictly prohibited by project policy (`AGENTS.md`). Defects must be resolved in `N64Recomp`, generator scripts, or through strong overrides in `RecompiledPatches`.
2. **Freshness Invariant**:
   - `python3 tools/check_generated_fresh.py` must return exit code 0, verifying that generated sources match current metadata tables and recompiler configuration.
3. **Fail-Fast Linking**:
   - The linker flag `-Wl,--no-undefined` (or platform equivalent) ensures that missing symbols or unmapped guest function pointers immediately halt the build with an actionable error.
4. **Static Function Sizing Invariant**:
   - Static function emitters in `N64Recomp` must not truncate function bodies when zero-sized symbols or non-code labels share the entry address. All emitted statics must retain their full instruction body.
