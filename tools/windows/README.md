# TooieRecomp — Windows 11 x64 Native Build Guide

Status: `WINDOWS_NATIVE_BUILD_PROVEN_DEV_HOST`.
The Windows 11 x64 native build pipeline is verified on one development host (commits `6862e29`..`f22e867` on `origin/main`). The entire build workflow (`tools\windows\Build-Tooie.ps1 -Stage All`: Check → Extract → Recomp → Configure → Build → Test → Package) runs completely natively without WSL or virtual machines.

---

## 1. Prerequisites & Official Component IDs

To build natively on Windows 11 x64, install the following official tools:

### 1.1 Visual Studio 2022 or Later
Install Visual Studio 2022 or later (verified on Visual Studio Community 2026 / VS 18 and VS 2022) with the **Desktop development with C++** workload and the following component IDs:

- `Microsoft.VisualStudio.Component.VC.Llvm.Clang` — Clang compiler for Windows (`clang-cl` 20.1.8 and `lld-link`)
- `Microsoft.VisualStudio.Component.VC.Llvm.ClangToolset` — MSVC Clang toolset integration
- `Microsoft.VisualStudio.Component.VC.CMake.Project` — CMake and Ninja build tools
- `Microsoft.VisualStudio.Component.Windows11SDK.26100` (or newer Windows 11 SDK)

**Installation via winget (Run in an ELEVATED / Administrator terminal):**
```powershell
winget install --id Microsoft.VisualStudio.2022.BuildTools --source winget --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.Llvm.Clang --add Microsoft.VisualStudio.Component.VC.Llvm.ClangToolset --add Microsoft.VisualStudio.Component.VC.CMake.Project --add Microsoft.VisualStudio.Component.Windows11SDK.26100"
```
*(Note: If you run the installer without elevation, `--passive` fails with error code 5007).*

### 1.2 Git for Windows
```powershell
winget install --id Git.Git -e --source winget
```
Git Bash (`bash.exe`) is used only for offline integrity validation scripts (`tools/vendor_verify.sh` and `tools/package_manifest.sh`).

### 1.3 Python 3.8+
```powershell
winget install --id Python.Python.3.12 -e --source winget
```
Build helpers require standard Python 3.8+ accessible as `py -3` or `python`. Note that the Microsoft Store execution alias (`WindowsApps\python3.exe`) is automatically detected and skipped by `Build-Tooie.ps1`.

---

## 2. Repository Clone & Line Endings (LF)

Vendored files and metadata checksums require strict LF line endings. Clone the repository with per-clone CRLF conversion disabled:

```powershell
git -c core.autocrlf=false clone https://github.com/wesleytakatsu/TooieRecomp.git
cd TooieRecomp
git config core.autocrlf false
```

The repository `.gitattributes` enforces `* text=auto eol=lf`.

---

## 3. Cartridge ROM Prerequisite

Place your verified NTSC-U Rev. 0 cartridge ROM `Banjo-Tooie (USA).z64` (33,554,432 bytes, MD5 `40e98faa24ac3ebe1d25cb5e5ddf49e4`, SHA-1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`) in the repository root directory (git-ignored) or set the environment variable `TOOIE_ROM`.

Verify your ROM with:
```powershell
python tools/rom_check.py
```

---

## 4. Build Pipeline (`Build-Tooie.ps1`)

The primary build interface on Windows is `tools\windows\Build-Tooie.ps1`.

### 4.1 Automated End-to-End Build

Run the full pipeline from clean checkout to tested distribution package:

```powershell
.\tools\windows\Build-Tooie.ps1 -Stage All
```

This sequentially executes:
1. `Check`: Validates host toolchain, developer shell environment, LF line endings, and vendored file integrity.
2. `Extract`: Decompresses retail ROM to `<build>\extract\decompressed.us.z64` using `tools/extract_rom.py`.
3. `Recomp`: Builds `N64Recomp.exe` and generates 420 C files in `RecompiledFuncs/` (byte-identical to Linux baseline).
4. `Configure`: Generates CMake build configuration with Ninja and `clang-cl`.
5. `Build`: Compiles `TooieRecompiled.exe` and copies runtime DLLs (`SDL2.dll`, `dxil.dll`, `dxcompiler.dll`).
6. `Test`: Executes host unit test suites (14/14 required PASS).
7. `Package`: Audits and packages release archive into `dist/`.

### 4.2 Individual Pipeline Stages

You can execute any individual stage:

```powershell
# Verify host tools and vendored file integrity
.\tools\windows\Build-Tooie.ps1 -Stage Check

# Decompress ROM to <build>/extract/decompressed.us.z64
.\tools\windows\Build-Tooie.ps1 -Stage Extract [-Rom <path>]

# Compile N64Recomp.exe and generate RecompiledFuncs/
.\tools\windows\Build-Tooie.ps1 -Stage Recomp [-Jobs N]

# Verify generated RecompiledFuncs against metadata/recomp-output.sha256
.\tools\windows\Build-Tooie.ps1 -Stage Verify

# Configure CMake with clang-cl and Ninja
.\tools\windows\Build-Tooie.ps1 -Stage Configure [-BuildDir <path>]

# Build TooieRecompiled.exe
.\tools\windows\Build-Tooie.ps1 -Stage Build [-Jobs N]

# Run required host unit tests (14/14 PASS)
.\tools\windows\Build-Tooie.ps1 -Stage Test

# Build all test targets and run extra unit tests (19/19 extra PASS)
.\tools\windows\Build-Tooie.ps1 -Stage Test -AllTests

# Package release archive into dist/
.\tools\windows\Build-Tooie.ps1 -Stage Package [-ReleaseVersion <ver>]
```

### 4.3 Pipeline Parameters

- `-Stage <name>`: `Check | Extract | Recomp | Verify | Configure | Build | Test | Package | All | ImportGenerated` (default: `Check`).
- `-Jobs N`: Concurrency level for Ninja (default: host logical CPU count `$env:NUMBER_OF_PROCESSORS`).
- `-BuildDir <path>`: Build tree directory (default: `build-win`).
- `-Rom <path>`: Explicit path to cartridge ROM for `Extract`.
- `-AllTests`: With `-Stage Test` or `-Stage All`, builds all `*_test` targets and executes all argument-free tests.

### 4.4 Legacy Fallback: `ImportGenerated`

To bypass local recompiler generation and import pre-generated `RecompiledFuncs/` from an existing Linux or WSL2 tree:

```powershell
.\tools\windows\Build-Tooie.ps1 -Stage ImportGenerated -GeneratedFrom C:\path\to\RecompiledFuncs
```
*(This is preserved strictly as a fallback; native recompiler generation via `-Stage Recomp` is now fully supported).*

---

## 5. Build Artifacts & Running the Game

- **Executable:** `build-win\TooieRecompiled.exe`
- **Companion DLLs:** `build-win\SDL2.dll`, `build-win\dxil.dll`, `build-win\dxcompiler.dll`
- **Launcher Execution:**
  - Launch from PowerShell: `.\run-tooie.ps1` (supports the same command-line flags as `run-tooie.sh`).
  - Or double-click `build-win\TooieRecompiled.exe`.
  - The launcher accepts both the original cartridge ROM (`Banjo-Tooie (USA).z64` / `.n64`, decompressing in memory transparently) and the decompressed image (`build-win\extract\decompressed.us.z64`).
- **Distribution Package:**
  - Archive: `dist\BanjoTooieRecompiled-v<ver>-Windows-x64.zip` (with `-ReleaseVersion <ver>`) or `dist\BanjoTooieRecompiled-dev-<desc>-Windows-x64.zip` (+ `.sha256`)
  - Each archive contains one top-level folder `BanjoTooieRecompiled`, `BUILD-INFO.txt` and the end-user `README-PACKAGE.txt`; the `.sha256` sidecar sits next to the archive.
  - Checksum and content audit:
    ```powershell
    python tools\verify_package.py dist\BanjoTooieRecompiled-*-Windows-x64*.zip
    ```

---

## 6. Host Unit Test Results

The Windows test suite is fully ported and verified:
- **Required Host Tests (14/14 PASS):**
  - `tooie_language_pack_test`, `tooie_text_font_map_test`, `tooie_language_import_test`, `tooie_font_sprite_test`,
    `tooie_language_bundle_test`
  - `tooie_menu_policy_test`
  - `tooie_menu_config_test`
  - `rt64_host_endpoint_current_test`
  - `p09_interp_burst_test`
  - `p09_world_interpolation_guard_test`
  - `p09_local_pair_snapshot_test`
  - `p09_exact_use_correlation_test` (1355/1355 passed; configured with `/STACK:8388608` 8 MiB stack)
  - `rt64_present_decimator_test`
  - `rom_cart_import_test`
- **Extra Unit Tests with `-AllTests` (19/19 PASS):**
  - Includes `window_display_test`, `tintro_projection_policy_test`, `tooie_demo7_drift_test`, `mips_rounding_test`, and Python test ports (`demo1_projectile_coverage_test.py`, `n64recomp_zero_size_static_test.py`, `attract_demo2_transition_jal_test.py`, `tools/test_rom_check.py`).
- **Skipped Driver Targets (5 SKIP):**
  - `tooie_demo1_parity_test`, `tooie_demo1_projectile_test`, `tooie_demo1_target_selector_test`, `tooie_demo1_upd_layout_test`, `p09_local_pair_snapshot_io_test`.
  - These executables compile cleanly (`exit 0`), but are skipped during automated runs because they require command-line arguments or external fixtures (identical to Linux behavior).

---

## 7. Troubleshooting & Important Invariants

1. **Microsoft Store Python Stub:**
   If `python3` resolves to `WindowsApps\python3.exe`, executing it without Store installation prompts the Microsoft Store dialog. `Build-Tooie.ps1` automatically detects and skips `WindowsApps` stubs and uses standard `py -3` or `python`.
2. **DO NOT run `tools/vendor_manifest.sh` on Windows:**
   The NTFS file system does not store POSIX executable permission bits. Running `vendor_manifest.sh` on Windows will rewrite `vendor/MODES.tsv` with zeroed exec bits, corrupting Linux vendor audits. When modifying vendored files or applying patches on Windows, update only the hash lines in `vendor/MANIFEST.sha256` and verify using `bash tools/vendor_verify.sh --reverse-check`.
3. **Visual Studio Installer Elevation:**
   Installing components with `--passive` requires an elevated (Administrator) shell. Running without elevation fails with exit code 5007.
4. **Smoke Test Sandbox Isolation:**
   On Windows, `tools/smoke_package.py` is not strictly sandboxed from user application data because Win32 `SHGetKnownFolderPath` writes settings to `%LOCALAPPDATA%\TooieRecompiled` regardless of environment variable overrides.
