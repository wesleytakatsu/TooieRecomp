---
title: "P10 Evidence — Windows 11 x64 and Linux Clean-Clone Readiness and Packaging Verification"
description: "Forensic evidence, independent audit results, and reproducible build proof for standalone clean-clone readiness on Linux and native Windows 11 x64 preparation."
status: active
type: evidence
project: TooieRecomp
phase: P10
date: 2026-10-21
tags: [tooie-recomp, evidence, p10, packaging, windows, linux, clean-clone, bootstrap, haiku-review, gate]
---

> **SUPERSEDED (2026-10-21, mandato standalone):** o desenho de bootstrap externo descrito abaixo (`tools/deps.lock`, `bootstrap_deps.sh`, `apply_runtime_patches.sh`, `third_party/`) foi aposentado; as dependências foram vendorizadas. Estado atual: [[plan/evidence/p10-standalone-vendor-20261021]]. Este documento permanece como histórico/proveniência.

# P10 Evidence — Windows 11 x64 and Linux Clean-Clone Readiness

## 1. Context and Problem Statement

Attempting to clone `TooieRecomp` on a separate Windows 11 x64 machine was reported as incomplete and failing to build. A multi-agent forensic investigation was conducted to determine all missing prerequisites, establish dependency manifests, eliminate implicit sibling layout assumptions, and prepare verifiable build artifacts for both Linux and Windows 11 x64 as far as the Linux authoring host permits.

### Status and Gate Classifications

- **Linux Packaging:** `LINUX_CANDIDATE_PACKAGE_VERIFIED` (`accepted: true`, local dirty candidate only).
- **Windows Port:** `WINDOWS_BUILD_PENDING_NATIVE_HOST` (`accepted: false`, **no Windows executable exists**).
- **Repository Completeness:** `INCOMPLETE` (`accepted: false`, plain clone of remote `HEAD` does not build).
- **Glitch Technical Gate:** `FAILED` (P09 articulated transform interpolation gate blockers B1–B7 remain open).
- **Phase Lifecycle:** P09 and P10 remain **NOT closed**.

---

## 2. Root Cause Analysis of Clone Incompleteness

Prior to this work, cloning the repository on another system failed due to six prioritized architectural root causes:

1. **Implicit Sibling Directory Dependencies (P0):**
   `CMakeLists.txt` hardcoded `BK_ROOT` to `../BanjoRecomp` (supplying RT64, N64ModernRuntime, RecompFrontend, SlotMap, assets, and controller database) and assumed `../banjo-tooie-Recomp` decompilation was present in the parent directory. A plain `git clone` outside this exact sibling folder tree failed immediately during CMake configuration (`logs/headA-configure.log`, exit code 1).
2. **Uncommitted Working Tree Features (P0):**
   Remote `HEAD` (`73bfd43`) lacked recent gameplay features implemented locally, notably the Graphics menu **Interpolation** toggle (persisted in `graphics.json`, default OFF / 30 FPS / `CURRENT` endpoint), removal of the F8 burst capture hotkey, 5 new unit tests, and 5 SHA-pinned RT64 patches. A plain clone compiled an obsolete revision.
3. **ROM-Derived Generated Inputs Omitted from Git (P0):**
   The static recompilation pipeline requires `RecompiledFuncs/` (~90 MB across 420 translation units and headers), which requires a legitimate user ROM, MIPS GCC decompilation, and patched N64Recomp generation. No documented single entry point or standalone bootstrap existed.
4. **Linux-Specific CMake Build Rules and Syntax (P1):**
   `CMakeLists.txt` configured Linux-only shader pipelines (`dxc-linux` and `LD_LIBRARY_PATH`), dynamically linked `atomic` and system `SDL2`, lacked Windows subsystem definitions and DLL bundling rules (`SDL2.dll`, `dxil.dll`, `dxcompiler.dll`), and several diagnostic headers unguardedly included `<unistd.h>`, `<sys/stat.h>`, or called `getpid()`.
5. **GNU ld Symbol Wrapping Shims Missing on Windows (P1):**
   `src/game/boot_hw_stubs.cpp` registered guest wrapper functions via GNU ld `--wrap` (`__wrap_func_*`). On Windows (using `lld-link` or MSVC `link.exe`), `--wrap` is unsupported, causing 18 undefined `__real_*` symbols during link.
6. **Absence of Licensing, Packaging, and Distribution Manifests (P2):**
   The repository contained no root `README.md`, no license files, no asset ownership manifest, and tracked a symlink `assets -> ../BanjoRecomp/assets` that resolves as a dangling text file on Windows when developer mode / symlinks are disabled.

---

## 3. Pre-Modification Protected Inventory

Baselines were captured prior to editing:
- **Repository HEAD:** `73bfd4303b319ddb98fd42ddb6f6ccf8025794c2`
- **Dirty State Baseline:** 33 tracked modified files, 3 tracked deleted files (`tooie_rsp_endpoint_current.{h,cpp,_test.cpp}`), 25 untracked files. Full unfiltered patch preserved in `baseline-diff.patch` (SHA-256 `adf0df4f...`).
- **Sibling Heads & Submodules:**
  - `BanjoRecomp`: `ec859632...`
  - `rt64`: `6f1c2d99...` (preserved ~14,270 lines of unversioned diagnostic code; **not** folded into any patch)
  - `N64ModernRuntime`: `ca568b6a...`
  - `N64Recomp`: `2b6f0568...`
  - `RecompFrontend`: `d0d90ba4...`
  - `plume`: `d890ac89...`
  - `banjo-tooie-Recomp`: `25033be6...`

---

## 4. Architectural Solutions Implemented

### 4.1 Pinned Bootstrap Manifest (`tools/deps.lock` & `tools/bootstrap_deps.sh`)
Rather than bloating git history with 717 MB of nested submodules, dependencies are pinned by exact commit SHA in `tools/deps.lock`. Running `tools/bootstrap_deps.sh` performs shallow fetch-by-SHA into `third_party/` and validates every nested submodule commit before continuing.

### 4.2 Windows Compatibility & Linker Shims
- Added `_WIN32` shims in `src/game/backpack_lookup_diag.cpp` routing `__real_func_*` calls directly to `func_*` on non-GNU platforms.
- Guarded `<unistd.h>`, `<sys/stat.h>`, and `getpid()` in `src/game/tooie_diag.cpp` and `src/main/tooie_audio_capture.cpp`.
- Extended `CMakeLists.txt` with a `WIN32` branch that fetches SDL2 2.30.3 VC development headers, configures DirectX Shader Compiler (`dxc.exe`), and copies required runtime DLLs.

### 4.3 Git Line Endings Policy
SHA-256 pinned patches require strict LF line endings. To avoid global environment mutation, clone instructions mandate **per-clone configuration**:
```bash
git -c core.autocrlf=false clone <repo-url> TooieRecomp
cd TooieRecomp
git config core.autocrlf false
```

---

## 5. Linux Clean-Clone Materialization Proof

A clean materialization tree was created at `/home/wesley/tooie-proof/tooie` (path strictly verified to contain no `.c` characters to avoid upstream `compile_sjis.py` bugs). The tree was populated with a fresh `git clone --no-hardlinks` of `HEAD` plus the working tree delta (38 tracked and untracked files).

### Verification Results Table

| Stage | Command Executed | Exit Code | Elapsed | Observed Result |
| :--- | :--- | :---: | :---: | :--- |
| **Preflight** | `tools/preflight.sh generate` | `0` | 1s | Prerequisites, tools, path, and ROM verified. |
| **Bootstrap** | `tools/bootstrap_deps.sh --decomp` | `0` | 2m35s | Pinned dependencies fetched into `third_party/`. |
| **Patching** | `tools/apply_runtime_patches.sh` | `0` | 3s | 16 versioned patches applied cleanly; marker verified. |
| **Decomp** | `tools/build_linux.sh decomp` | `0` | 2m16s | Decompressed ROM SHA-1 `af479771...`; ELF SHA-1 `2b2d04c5...`. |
| **Recomp** | `tools/build_linux.sh recomp` | `0` | 1m09s | Generated 420 `RecompiledFuncs` files, byte-identical to developer tree. |
| **Build** | `tools/build_linux.sh configure build` | `0` | 11m04s | Clang 18.1.3 + Ninja, 1158 steps, produced `TooieRecompiled`. |
| **Tests** | Host unit tests (8 targets) | `0` | 2s | **8/8 PASS**: `tooie_menu_policy_test`, `tooie_menu_config_test`, `rt64_host_endpoint_current_test`, `p09_interp_burst_test`, `p09_world_interpolation_guard_test`, `p09_local_pair_snapshot_test`, `p09_exact_use_correlation_test`, `tooie_decimator_test`. |
| **Package** | `tools/package_linux.sh` | `0` | 4s | Built `TooieRecompiled-linux-x86_64-73bfd43-dirty-private-local.tar.gz` (Final SHA-256 `a327666f8baff1ce1134803563d5eb4d5ad1c50edf3359e0a419de5a22a6996f`, 105 files; packaged binary `24e47b02fdf2288d4f59f6bff9f6bda43422b4810b6220a9aee0a4f648a9c148`). Audited: zero ROM or ELF files; 0 `/home/` strings. |
| **Smoke** | `tools/smoke_package.sh <package-dir>` | `0` | 22s | Extracted to isolated sandbox; launched on X11/RADV; sustained 20s liveness under both default loader and system libz override; clean exit; verified `graphics.json` initialized with `interpolation: false`. |

---

## 6. Windows 11 x64 Preparation & Root Supplement

### Compile-Level Verification on Host
- 25/25 Tooie C++ translation units passed `-fsyntax-only` against Windows headers and SDL2 2.30.3 VC definitions (`inventory/winprobe-syntax.txt`).
- CMake configured the `WIN32` target cleanly using Ninja.
- Cross-compilation of the 90 MB generated C code under `llvm-mingw` was attempted but failed on 44 third-party dependency header/linking edges (`inventory/winprobe-failed-edges.txt`). Native compilation under `clang-cl` and Windows 11 SDK remains strictly required.
- Source candidate archive: `TooieRecomp-source-candidate-73bfd43-plus-delta.tar.gz` (SHA-256 `ea0606998192b17a4696ba80546436c002948b48e938e16eaf9a85772d617582`, 512 regular files, source only, zero `.exe` or ROM).

### Root Supplement: Direct Private Transfer (No WSL2 Needed)
The root coordinator generated `TooieRecomp-generated-private-local.tar.gz` (11,157,073 bytes, SHA-256 `e7bb66d40d6f6c499f6d5e5567bdd9cbad2bd3fffb9483900a5fc2d11e379680`), containing the 420 verified `RecompiledFuncs` files (417 C translation units + 1 C++ lookup unit + 2 headers) from the clean Linux proof tree.

On Windows, the user can bypass WSL2 and ROM decompression by importing this bundle directly:
```powershell
.\tools\windows\Build-Tooie.ps1 -Stage Check
.\tools\windows\Build-Tooie.ps1 -Stage Bootstrap
.\tools\windows\Build-Tooie.ps1 -Stage Patch
.\tools\windows\Build-Tooie.ps1 -Stage ImportGenerated -GeneratedFrom C:\path\RecompiledFuncs
.\tools\windows\Build-Tooie.ps1 -Stage Build
.\tools\windows\Build-Tooie.ps1 -Stage Package
```

All Windows instructions remain explicitly classified as **unverified** until executed on a physical Windows 11 host.

---

## 7. Independent Review Audit Findings and Follow-up Resolutions

The independent review (`independent-review.md`, `gate.json`, `followup-report.md`, and `final-gate.json`) evaluated and arbitrated all findings:
1. **Linux Package Integrity & Dependency Mapping (B3):**
   - Verified integrity: `tools/verify_package.sh` passes with exit 0 (`MANIFEST OK (106 files)`, zero ROM/ELF files).
   - In `DEPENDENCIES.txt`, `libz.so.1` mapping was corrected to distro package `zlib1g 1:1.3.dfsg-3.1ubuntu2.2` with a `HOST-SHADOW` note regarding host `/usr/local/lib/libz.so.1`.
   - Verified launch with system zlib: `TOOIE_SMOKE_LD_LIBRARY_PATH=/lib/x86_64-linux-gnu` maps `/usr/lib/x86_64-linux-gnu/libz.so.1.3` and passes 20s liveness (`fu-smoke-systemlibz.log`).
2. **Windows Binary Absence (B2):** Confirmed zero `.exe` or `.dll` files in any artifact. Verified `WINDOWS_BUILD_PENDING_NATIVE_HOST` status.
3. **Privacy Hygiene / Path Leakage (B4):**
   - Resolved in binary: follow-on patch `patches/rt64/rt64-p09-local-pair-snapshot-portable-paths.patch` (SHA-256 `d7081b7d...`) redirects output to `$TOOIE_DIAG_P09_LOCAL_PAIR_DIR` / `$TMPDIR` / `/tmp`. Audited binary `24e47b02...` contains 0 `/home/` strings.
   - Source candidate caveat: `.mcp.json`, `opencode.json`, `.vscode/mcp.json` from git HEAD contain local developer paths; disclosed and documented.
4. **Licensing & Asset Manifest (B5):**
   - Implemented automated manifest generation: `tools/package_manifest.sh` + `tools/package_licenses.tsv` generates `THIRD-PARTY-MANIFEST.tsv` (106 rows, 41 UNKNOWN/UNVERIFIED).
   - Specification `docs/specs/Tooie/asset-and-licensing-manifest.md` updated with exact font and dependency metadata.
5. **File Count Reconciliations (B7 / C01 / C13):**
   - Dirty file counts reconciled: 38 files at materialization (`delta-paths.txt`), 42 at first packaging (`BUILD-INFO.txt`), 46 in current proof tree, 85 in working checkout.
   - Generated code count reconciled: 420 total files in `RecompiledFuncs/` = 417 `funcs_*.c` + `lookup.cpp` + `funcs.h` + `recomp_overlays.inl`.
6. **Gate Verdict:** `linux.accepted: true (local candidate only)`, `windows.accepted: false (WINDOWS_BUILD_PENDING_NATIVE_HOST)`, `repositoryCompleteness: false (REPOSITORY_NOT_COMPLETE)`. Glitch technical gate remains `FAILED`.

---

## 8. Preserved Invariants and Next Single Action

- **Graphics Menu Interpolation:** Toggle default remains strictly OFF (`interpolation: false`, CURRENT 30 FPS).
- **F8 Key Removed:** Hotkey and burst capture modules remain disabled.
- **P08 Widescreen & P09 Glitches:** Prior technical gates stay FAILED; no glitch fix is claimed.
- **No Git Commit / Push:** All modifications remain uncommitted in the local tree.

```text
NEXT_SINGLE_ACTION: On a native Windows 11 x64 host with Visual Studio 2022 Build Tools (clang-cl, Windows 11 SDK) and Git for Windows installed, clone the repository using per-clone CRLF ('git -c core.autocrlf=false clone ...'), import the generated input bundle ('TooieRecomp-generated-private-local.tar.gz') via '.\tools\windows\Build-Tooie.ps1 -Stage ImportGenerated', execute '-Stage Build' and '-Stage Package', and record the exit codes, TooieRecompiled.exe SHA-256, and runtime launch smoke.
```
