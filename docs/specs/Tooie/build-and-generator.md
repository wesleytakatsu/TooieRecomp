---
title: "TooieRecomp Spec — Build System and Recompiler Generator"
description: "Technical specification for the CMake build pipeline, Clang compiler toolchains, static recompilation generator, metadata inputs, and dependency linking in TooieRecomp."
status: current
area: "specs/Tooie"
updated: 2026-10-23
tags: [specs, tooierecomp, build, cmake, n64recomp, rsprecomp]
---

# Build System and Recompiler Generator Specification

## 1. System Requirements & Architecture

The build system coordinates the compilation of statically recompiled C/C++ guest code together with host virtualization runtimes and native platform libraries into standalone executables: `TooieRecompiled` on Linux x86-64 and `TooieRecompiled.exe` on Windows x64.

```text
+---------------------------------------------------------------------------------+
|                        Decompilation Sources (decomp/)                          |
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
|  - N64Recomp tooie.us.rev0.recomp.toml -> RecompiledFuncs/*.c                  |
|  - RSPRecomp n_aspMain.us.rev0.toml -> rsp/n_aspMain.cpp                        |
|  - tools/fix_rsprecomp_labels.py -> C++ label normalization                     |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                             Native Host Compilation                             |
|  - Clang mandatory (GCC rejected via CMake FATAL_ERROR)                         |
|  - ModernRuntime + RT64 + RecompFrontend + PatchesLib                           |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|         TooieRecompiled / TooieRecompiled.exe (Native Platform Binary)          |
+---------------------------------------------------------------------------------+
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Status |
| :--- | :--- | :--- | :--- |
| **REQ-BLD-01** | Toolchain Requirement | Compiles with Clang (C++20). GCC is explicitly rejected with FATAL_ERROR. | `IMPLEMENTED` |
| **REQ-BLD-02** | Metadata Determinism | `generate_metadata.py` produces identical tables across clean runs. | `IMPLEMENTED` |
| **REQ-BLD-03** | Zero Undefined Symbols | Missing symbols or unresolved relocations halt the build immediately. | `IMPLEMENTED` |
| **REQ-BLD-04** | RSP Microcode Parity | `n_aspMain.cpp` compiles cleanly and executes audio tasks at `0x17D0`. | `IMPLEMENTED` |
| **REQ-BLD-05** | Test Suite Targets | CMake provides isolated unit test targets runnable without game launch. | `IMPLEMENTED` |
| **REQ-BLD-06** | Asset Quarantine | No ROM binaries, copyrighted audio, or textures bundled in binary output. | `IMPLEMENTED` |

---

## 3. Interface & Configuration Contracts

### 3.1 CMake Configuration & Compiler Constraints
- **Compiler Requirement**:
  - Clang is strictly required (`CMakeLists.txt:26-33`). On Linux: `clang` / `clang++`. On Windows: `clang-cl` with `lld-link`.
  - GCC is rejected at configure time with a `FATAL_ERROR`.
  - C++ standard: C++20 (`std::span`, `<chrono>`, modern templates).
- **Architecture Options**:
  - Linux x86-64 adds `-march=nehalem`, requires FreeType and system SDL2.
  - Windows x64 uses `/OPT:NOICF`, subsystem WINDOWS in Release, links vendored SDL2 2.30.3 VC development files, and bundles `SDL2.dll`, `dxil.dll`, `dxcompiler.dll`.
  - In-source builds are strictly forbidden (`FATAL_ERROR`).
  - Target `TooieUIAssets` wipes and synchronizes assets to `<build>/assets`.
- **ROM Environment**:
  - `TOOIE_CART_Z64`: Path to the 32 MiB retail cartridge ROM (`Banjo-Tooie (USA).z64`). Required by oracle runner (`tools/oracle/.../run.sh`) without implicit parent lookups.

### 3.2 Dependency Management: Vendored Standalone Closure
- **Vendored Trees (`lib/`, `assets/`, `decomp/`)**:
  - All dependencies are vendored directly as regular files (no submodules, gitlinks, or network downloads during build).
  - Provenance tracked in `vendor/UPSTREAMS.tsv`, `vendor/patches-applied/ORDER.tsv`, and `vendor/MANIFEST.sha256`.
  - Verified offline via `tools/vendor_verify.sh [--reverse-check]`.
- **Maintainer Toolchain Kit**:
  - Normal compilation is kit-free. The legacy IDO/gcc compiler kit (`TOOIE_TOOLCHAIN_KIT`, 34 files verified via `tools/toolchain.sha256`) is strictly an optional prerequisite for decomp and metadata regeneration.

### 3.3 Build Pipelines & Execution Stages
- **Linux Pipeline (`tools/build_linux.sh`)**:
  - Stages: `preflight`, `verify`, `extract`, `recomp`, `configure`, `build`, `test`, `package`.
  - `decomp` stage is maintainer-only.
  - Standard test suite runs 8 core unit tests.
- **Windows Pipeline (`tools/windows/Build-Tooie.ps1`)**:
  - Stages: `Check`, `Extract`, `Recomp`, `Verify`, `ImportGenerated`, `Configure`, `Build`, `Test`, `Package`, `All`.
  - Options: `-AllTests` runs all non-argument tests; `-Jobs` specifies concurrency.
  - Standard test suite runs 9 core unit tests (including `rom_cart_import_test`).

### 3.4 Core CMake Targets & Libraries

| Target | Artifact | Linkage / Purpose |
| :--- | :--- | :--- |
| `TooieRecompiled` | `build/TooieRecompiled[.exe]` | Executable linking `PatchesLib`, `RecompiledFuncs`, `tooie_cart_decompress`, `recompui`, `recompinput`, `librecomp`, `ultramodern`, `rt64`, `nfd`, `SDL2`, `Threads` (+ `FREETYPE`, `atomic`, `dl` on Linux). |
| `RecompiledFuncs` | `build/libRecompiledFuncs.a` | Static archive of generated guest functions. Flags: `-fno-strict-aliasing -Wno-unused-variable -Wno-implicit-function-declaration -falign-functions=16 -DRECOMP_MIPS_ROUND_COMPAT=1`. |
| `PatchesLib` | `build/libPatchesLib.a` | Scaffold library (`patches.c`, `patches_bin.c`, `recomp_overlays.inl`). Native overrides live in `src/game/boot_hw_stubs.cpp` as strong symbols. |
| `tooie_cart_decompress` | `build/libtooie_cart_decompress.a` | Static library wrapping miniz for in-memory cartridge decompression. |

### 3.5 Test Targets (`EXCLUDE_FROM_ALL`)
CMake defines isolated verification targets:
- **Core host tests**: `tooie_menu_policy_test`, `tooie_menu_config_test`, `rt64_host_endpoint_current_test`, `p09_interp_burst_test`, `p09_world_interpolation_guard_test`, `p09_local_pair_snapshot_test`, `p09_exact_use_correlation_test`, `rt64_present_decimator_test`, `rom_cart_import_test` (Windows).
- **Additional domain tests**: `preemption_t15`, `bcf28_helpers_test`, `core1_ra_wrappers_test`, `stick_angle_thunk_test`, `matrix_submission_test`, `mips_rounding_test`, `input_direction_test`, `window_display_test`, `tintro_projection_policy_test`, `tooie_demo7_drift_test`, `tooie_fps_meter_test`, `p09_cpu_shader_clip_semantics_test`, `p09_ordered_match_test`, `p08_ws_frustum_test`, `graphics_failure_test`, `tooie_audio_capture_test`, `tooie_bgm_volume_test`, `tooie_bgm_hooks_test`.
- **Argument-driven tests** (SKIP when unparameterized): `tooie_demo1_parity_test`, `tooie_demo1_projectile_test`, `tooie_demo1_target_selector_test`, `tooie_demo1_upd_layout_test`, `p09_local_pair_snapshot_io_test`.

### 3.6 Symbol Wrapping (`TOOIE_WRAPPED_SYMBOLS`)
`CMakeLists.txt` defines 18 intercepted symbols wrapped via `-Wl,--wrap=<sym>` (Linux) and `LINKER:/wrap:<sym>` (Windows `lld-link`):
- Guarantees identical symbol interposition semantics across both platforms.
- Overrides are implemented in `src/game/boot_hw_stubs.cpp`.

### 3.7 Patch Management
- Upstream patches are permanently integrated into `lib/` and documented in `vendor/patches-applied/ORDER.tsv`.
- Project-local runtime modifications are recorded in `vendor/patches-local/` (e.g., `n64modernruntime-overlay-section-dedupe.patch`, `rt64-p09-ordered-transform-match.patch`, `n64recomp-binary-output.patch`, `n64recomp-coff-weak-recomp-func.patch`, `decomp-rom-decompressor-in-memory.patch`).

---

## 4. Invariants & Verification Protocols

1. **Generated Code Immutability**:
   - Files within `RecompiledFuncs/`, `generated/`, and `rsp/n_aspMain.cpp` are purely machine-generated.
   - Hand-editing generated code is strictly prohibited by project policy (`AGENTS.md`). Defects must be fixed in the code generator or through strong native symbol overrides in `boot_hw_stubs.cpp`.
2. **Freshness Invariant**:
   - `python3 tools/check_generated_fresh.py` verifies that generated sources correspond exactly to metadata tables and recompiler configuration.
3. **Static Function Sizing Invariant**:
   - Static function emitters in `N64Recomp` must not truncate function bodies when zero-sized symbols or non-code labels share the entry address (`n64recomp-zero-size-static.patch`).