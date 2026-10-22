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
- `TOOIE_LIB` (internal, fixed to `<repo>/lib`): vendored `N64ModernRuntime`(+`N64Recomp`), `rt64`, `RecompFrontend`, `SlotMap`, `SDL2-windows`. There is no `BK_ROOT`, no sibling checkout and no `third_party/` cache; an incomplete tree is a `FATAL_ERROR` pointing at `tools/vendor_verify.sh`.
- `CMAKE_BUILD_TYPE`: Standard CMake build configuration (`Release`, `RelWithDebInfo`, `Debug`).
- `CMAKE_CXX_STANDARD`: 20. Required for `std::span`, `<chrono>`, and modern template facilities.
- `TOOIE_CART_Z64`: Path to the 32 MiB NTSC-U Rev. 0 cartridge ROM used by oracle verification scripts.
- `WIN32` Target Support: On Windows, CMake uses the vendored SDL2 2.30.3 VC development files (`lib/SDL2-windows`, imported targets `SDL2::SDL2`/`SDL2::SDL2main`, shared with rt64 via the `vendor/patches-local/rt64-use-host-sdl2-target` seam; no `FetchContent`), uses `clang-cl`, the vendored DirectX Shader Compiler (`lib/rt64/src/contrib/dxc`, x86_64 only) and bundles runtime DLLs (`SDL2.dll`, `dxil.dll`, `dxcompiler.dll`). Cross-compilation translation unit (TU) syntax probes on Linux failed on third-party header boundaries (`plume_d3d12.cpp`, `plume_vulkan.cpp`), so there is no all-TUs-pass claim; CMake WIN32 cross-configure passed; status is strictly `WINDOWS_BUILD_PENDING_NATIVE_HOST` until an actual PE game executable is compiled and verified on a physical Windows 11 host. Supporting SDK binaries (`dxc.exe`, DLLs) intentionally vendored under `lib/` do not count as a game build.

### 3.2 Dependency Management: Vendored Standalone Closure
Status: `IMPLEMENTED` (authoring host Linux); offline Linux proof recorded in `docs/plan/evidence/p10-standalone-vendor-20261021.md` (classification there is authoritative).
- **Category A, vendored as regular files** (no submodule, gitlink, sibling or download): `lib/{N64ModernRuntime(+N64Recomp,thirdparty),rt64,RecompFrontend,SlotMap,SDL2-windows}`, `assets/` (real directory), `recompcontrollerdb.txt`, `decomp/` (sources only), `tools/wheelhouse/` (hash-pinned wheels, `tools/requirements.lock`). Provenance/pins/licence status/integrated patches: `vendor/` (`UPSTREAMS.tsv`, `patches-applied/ORDER.tsv`, `DERIVED-FILES.tsv`, `BINARIES.tsv`, `EXCLUDES.txt`, `MANIFEST.sha256`).
- **Category B, explicit hash-verified prerequisites** (never in git, never downloaded by a build): the canonical ROM `Banjo-Tooie (USA).z64` (size `33554432` bytes, MD5 `40e98faa24ac3ebe1d25cb5e5ddf49e4`, SHA-1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`, validated by `tools/rom_check.sh`), and the extracted decompressed image required by the launcher (`decompressed.us.z64`, size `35098224` bytes, SHA-1 `af479771ff5e91ce3732f3d84146614ab71d8d79`, XXH3 `9F843AD3455EA395`, extracted via `tools/extract_rom.sh`). The normal build is completely kit-free; the legacy IDO/gcc/binutils compiler kit (`TOOIE_TOOLCHAIN_KIT`, 34 files in `tools/toolchain.sha256`, verified via `tools/toolchain_kit.sh verify`) is strictly a maintainer-only prerequisite for `decomp` and metadata regeneration.
- **Category C, host packages**: Clang/CMake/Ninja/SDL2/GTK/MIPS binutils (Linux), Visual Studio Build Tools (Windows).
- `tools/vendor_verify.sh [--reverse-check]`: offline integrity (manifest sha256, exec bits, no symlink/nested .git, no unlisted ELF/PE/ar, patch chain reverse-applies). `tools/vendor_manifest.sh` regenerates the manifest after an intentional change.
- The former `tools/deps.lock`, `tools/bootstrap_deps.sh` and `tools/apply_runtime_patches.sh` are **retired** (history: git + `docs/plan/evidence/p10-windows-linux-clean-clone-readiness-20261021.md`). The 17 patches are integrated; nothing is applied at build time.
- Paths: `tools/paths.sh` (`DECOMP_ROOT` = work copy `$BUILD/decomp-work`, so `decomp/` stays pristine; `TOOIE_DECOMP_ROOT` is an explicit override only); N64Recomp is built out of tree in `$BUILD/n64recomp`.
- Direct Private Input Transfer: `RecompiledFuncs/` can be populated on Windows without WSL2 by extracting `TooieRecomp-generated-private-local.tar.gz` and executing `Build-Tooie.ps1 -Stage ImportGenerated`.
- Line endings: vendored trees are `-text` (byte-exact, `.gitattributes`); clone with `git -c core.autocrlf=false clone ...`; global git mutation is prohibited.

### 3.2 Core Targets

| Target | Artifact | Linkage / Dependencies |
| :--- | :--- | :--- |
| `TooieRecompiled` | `build/TooieRecompiled` | `RecompiledFuncs`, `RecompiledPatches`, `rt64`, `N64ModernRuntime`, `ultramodern`, `RecompFrontend`, `SDL2`, `Threads::Threads`. |
| `RecompiledFuncs` | `build/libRecompiledFuncs.a` | Static archive of ~21,000 recompiled guest functions. Built with Clang/GCC using `-O3` or `-O2`. |
| `RecompiledPatches`| `build/libRecompiledPatches.a`| Native function overrides (`RecompiledPatches/patches.c`). Replaces weak guest symbols. |
| `rt64_host_endpoint_current_test` | `build/rt64_host_endpoint_current_test` | Standalone test for RT64 host endpoint-current latch and override semantics. Supersedes removed `tooie_rsp_endpoint_current_test`. |
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
Historical inventory. These patches are **already integrated** in `lib/` (provenance in `vendor/patches-applied/`, order and sha256 in `vendor/patches-applied/ORDER.tsv`; `patches/…` paths below now read `vendor/patches-applied/…`). New local fixes go to `vendor/patches-local/` (`vendor/UPDATING.md`). Nothing is applied by build scripts:
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
