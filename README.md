# TooieRecomp

TooieRecomp is a native static recompilation project for **Banjo-Tooie** (N64, NTSC-U Rev. 0) running natively on PC via static recompiler translation (`N64Recomp` + `RSPRecomp`), `N64ModernRuntime`, `ultramodern`, and `RT64`.

The project is **standalone**: runtime, graphics, frontend, and decompilation sources are vendored directly in this repository (`lib/`, `decomp/`, `assets/`, `tools/wheelhouse/`). No sibling checkouts, git submodules, or build-time downloads are used.

---

## 1. Quickstart & Build Instructions

### 1.1 ROM Prerequisite & Strict Verification

A legally acquired Banjo-Tooie NTSC-U Rev. 0 cartridge ROM is required. **No ROM or game asset data is bundled or hosted in this repository.**

- **Canonical File Name:** `Banjo-Tooie (USA).z64`
- **Location:** Place directly in the repository root directory (or set `TOOIE_ROM=/path/to/Banjo-Tooie (USA).z64`).
- **Git Ignore Status:** Strictly ignored via `.gitignore` (`*.z64` and `/Banjo-Tooie (USA).z64`); never tracked or committed.
- **Expected Hashes:**
  - **File Size:** `33554432` bytes (exactly 32 MiB)
  - **MD5:** `40e98faa24ac3ebe1d25cb5e5ddf49e4`
  - **SHA-1:** `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`

#### Verification Commands

**Linux (Bash):**
```bash
# Verify with dedicated project validator:
tools/rom_check.sh

# Or verify manually:
md5sum "Banjo-Tooie (USA).z64"
# Expected: 40e98faa24ac3ebe1d25cb5e5ddf49e4
sha1sum "Banjo-Tooie (USA).z64"
# Expected: af1a89e12b638b8d82cc4c085c8e01d4cba03fb3
stat -c %s "Banjo-Tooie (USA).z64"
# Expected: 33554432
```

**Windows (PowerShell):**
```powershell
Get-FileHash -Algorithm MD5 ".\Banjo-Tooie (USA).z64"
# Hash: 40E98FAA24AC3EBE1D25CB5E5DDF49E4
Get-FileHash -Algorithm SHA1 ".\Banjo-Tooie (USA).z64"
# Hash: AF1A89E12B638B8D82CC4C085C8E01D4CBA03FB3
(Get-Item ".\Banjo-Tooie (USA).z64").Length
# Length: 33554432
```

---

### 1.2 Game Data vs. Native UI Resources

- **Game Data (100% ROM-Derived):** All textures, audio banks, music sequences, 3D world geometry, collision surfaces, and dialog scripts are extracted and decompressed from your verified ROM. They are never versioned in Git.
- **Native UI Resources (`assets/` and `ui_assets/`):** Per user clarification, the own/free constraint applies strictly to ROM game data. All previously completed menu assets, animations, icons, character vector art, fonts, and the Tooie logo are vendored locally in the repository:
  - Restored two-line Tooie launcher logo: `ui_assets/TooieLogo.svg` (viewBox `0 0 7300 2450`), rendered via `src/main/launcher_animation.cpp`. The interim neutral UI (774x272) is superseded.
  - Character and launcher vector artwork: `Banjo.svg`, `Kazooie.svg`, `Cloud1.svg`–`Cloud4.svg`, `JiggyColor.svg`, `JiggyHole.svg`, `JiggyShine.svg`, and `Logo.svg`.
  - Typography fonts: `Suplexmentary Comic NC.ttf` (origin SMWCNC / Jayvee D. Enaguas; license unverified/unknown preserved honestly without blocking restoration), `InterVariable.ttf` ([SIL OFL 1.1](vendor/licenses/LICENSE-Inter-OFL-1.1.txt)), `NotoEmoji-Regular.ttf` ([SIL OFL 1.1](vendor/licenses/LICENSE-NotoEmoji-OFL-1.1.txt)), and `promptfont.ttf` ([SIL OFL 1.1](assets/promptfont/LICENSE.txt)).
  - Controller database: `recompcontrollerdb.txt` (derived from [SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB), zlib license).
  - Native UI styling: `recomp.rcss` (RmlUi stylesheet).
  - Navigation and mapping icons: `assets/icons/` (including VizMap icons).
  - Modern PC vector fonts and UI resources are **never** extracted from the N64 ROM. CMake target `TooieUIAssets` refreshes `<build>/assets` on every build.

---

### 1.3 ROM Decompression & Extraction

The runtime launcher natively accepts both the original cartridge ROM (`Banjo-Tooie (USA).z64` or byteswapped `.n64`, decompressing in memory transparently) and the uncompressed ROM image (`decompressed.us.z64`).

For recompiler code generation (`recomp` stage) and standalone asset extraction, the decompressed ROM image (`decompressed.us.z64`) is extracted without requiring the legacy MIPS compiler kit:

**Linux (Bash):**
```bash
tools/extract_rom.sh
# Or via the pipeline:
tools/build_linux.sh extract
```

**Windows (PowerShell):**
```powershell
.\tools\windows\Build-Tooie.ps1 -Stage Extract
# Or standalone Python:
python tools/extract_rom.py
```

- **Output:** `build-linux/extract/decompressed.us.z64` (Linux) or `build-win/extract/decompressed.us.z64` (Windows) (git-ignored).
- **Decompressed Size:** `35098224` bytes.
- **Decompressed SHA-1:** `af479771ff5e91ce3732f3d84146614ab71d8d79`.
- **Runtime Usage:** Launch `TooieRecompiled` and select either `Banjo-Tooie (USA).z64` (retail cartridge) or `decompressed.us.z64` in the ROM picker.

---

### 1.4 Building on Linux (Ubuntu 24.04 / Linux Mint 22)

#### Step 1: Install OS Packages

```bash
sudo apt update
sudo apt install -y \
  git cmake ninja-build clang lld pkg-config make g++ python3 \
  libsdl2-dev libfreetype-dev libgtk-3-dev libfmt-dev zlib1g-dev
```

*Note: The normal build is completely kit-free and has no path restrictions. Only the maintainer-only decomp stage requires a workspace path without `.c` (upstream `ultralib` defect).*

#### Step 2: Clone Repository (Enforce LF)

Vendored files must remain byte-exact to match `vendor/MANIFEST.sha256`. Clone with per-clone LF configuration:

```bash
git -c core.autocrlf=false clone https://github.com/wesleytakatsu/TooieRecomp.git
cd TooieRecomp
git config core.autocrlf false
```

#### Step 3: Place Cartridge ROM

Place your verified `Banjo-Tooie (USA).z64` at the repo root (git-ignored) and verify:

```bash
tools/rom_check.sh
```

#### Step 4: Execute Automated Kit-Free Linux Build

The normal build is **100% kit-free**. It requires **no legacy MIPS compilers, no IDO kit, no ELF binaries, and no network downloads**:

```bash
tools/preflight.sh       # Validates environment and ROM (kit-free)
tools/build_linux.sh     # Default stages: preflight verify extract recomp configure build test package
```

The pipeline automatically executes:
1. `preflight`: Verifies offline tools, host libraries, and root ROM.
2. `verify`: Audits integrity of vendored files against `vendor/MANIFEST.sha256`.
3. `extract`: Extracts and decompresses ROM to `build-linux/extract/decompressed.us.z64` (the image the runtime launcher requires).
4. `recomp`: Compiles vendored `N64Recomp` and generates `RecompiledFuncs/` (420 files) directly from the decompressed image and committed `metadata/tooie.us.syms.toml`, verifying output against `metadata/recomp-output.sha256`.
5. `configure` + `build`: Compiles `TooieRecompiled` (1159 build steps) using Clang and Ninja.
6. `test`: Executes 8 host unit test suites.
7. `package`: Creates portable Linux distribution package.

- **Output Executable:** `build-linux/TooieRecompiled`
- **Unit Tests:** `tools/build_linux.sh test` (runs 8 host unit test suites).
- **Packaging:** `tools/package_linux.sh` (produces `dist/TooieRecompiled-linux-x86_64-*-private-local.tar.gz`).
- **Smoke Test:** `tools/smoke_package.sh dist/TooieRecompiled-linux-x86_64-*-private-local` (validates process startup and clean exit in an isolated sandbox).

#### Optional Maintainer-Only: Decompilation & Metadata Regeneration (Legacy Kit)

Normal builds do **not** use or require this stage. Only maintainers who modify decompilation sources or regenerate recompiler metadata (`metadata/tooie.us.syms.toml` via `tools/generate_metadata.py`) need the legacy compiler kit:

- Additional packages: `sudo apt install gcc-mips-linux-gnu binutils-mips-linux-gnu libtoml11-dev python3-venv`
- Set `TOOIE_TOOLCHAIN_KIT=/path/to/toolchain-kit` (verified against `tools/toolchain.sha256` via `tools/toolchain_kit.sh verify`).
- Run: `tools/preflight.sh decomp` and `tools/build_linux.sh decomp`.

---

### 1.5 Building on Windows 11 x64

The native Windows 11 x64 build pipeline is fully proven on host (`Build-Tooie.ps1 -Stage All`). The complete pipeline (Check → Extract → Recomp → Configure → Build → Test → Package) runs natively without WSL, producing a byte-identical `RecompiledFuncs/` generation (420/420 files matching `metadata/recomp-output.sha256`), passing host tests (9/9 required PASS, 15/15 extra PASS), and creating the distribution package.

#### Step 1: Install Official Tools on Windows

Install the prerequisites via official packages or PowerShell with `winget`:

1. **Visual Studio 2022 or newer:**
   Install using `winget` in an **elevated** PowerShell terminal (`--passive` requires administrative privileges):
   ```powershell
   winget install --id Microsoft.VisualStudio.2022.BuildTools --source winget --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.Llvm.Clang --add Microsoft.VisualStudio.Component.VC.Llvm.ClangToolset --add Microsoft.VisualStudio.Component.VC.CMake.Project --add Microsoft.VisualStudio.Component.Windows11SDK.26100"
   ```
   *(Verified on Visual Studio Community 2026 / VS 18 and VS 2022. You can also use the Visual Studio Installer GUI and select "Desktop development with C++", "C++ Clang Compiler for Windows", "MSVC Clang toolset", "C++ CMake tools for Windows", and the Windows 11 SDK).*
2. **Git for Windows:**
   ```powershell
   winget install --id Git.Git -e --source winget
   ```
   *(Git Bash is used only by `tools/vendor_verify.sh` and `tools/package_manifest.sh`).*
3. **Python 3.8+:**
   ```powershell
   winget install --id Python.Python.3.12 -e --source winget
   ```
   *(Build scripts resolve `py -3` or `python` and automatically ignore the Microsoft Store `python3` stub).*

#### Step 2: Clone Repository (Enforce LF)

Line endings must strictly remain LF to match vendored file manifests and recompiler outputs:

```powershell
git -c core.autocrlf=false clone https://github.com/wesleytakatsu/TooieRecomp.git
cd TooieRecomp
git config core.autocrlf false
```

*(Note: `.gitattributes` enforces `* text=auto eol=lf` across the repository).*

#### Step 3: Place Cartridge ROM

Place your verified `Banjo-Tooie (USA).z64` at the repo root (git-ignored) or set `TOOIE_ROM`:

```powershell
python tools/rom_check.py
```

#### Step 4: Execute Automated Native Windows Build

Run the complete native pipeline:

```powershell
.\tools\windows\Build-Tooie.ps1 -Stage All
```

The pipeline automatically executes:
1. `Check`: Validates toolchain (git, cmake, ninja, clang-cl, python, bash) and vendored file integrity.
2. `Extract`: Decompresses retail ROM to `build-win\extract\decompressed.us.z64`.
3. `Recomp`: Builds `N64Recomp.exe` and generates `RecompiledFuncs/` (420 files byte-identical to Linux baseline).
4. `Configure` + `Build`: Compiles `TooieRecompiled.exe` using `clang-cl` and `lld-link` via Ninja (copies `SDL2.dll`, `dxil.dll`, `dxcompiler.dll`).
5. `Test`: Runs required host test suites (9/9 PASS, 15/15 extra PASS with `-AllTests`).
6. `Package`: Creates distribution archive `dist\TooieRecompiled-windows-x64-<HEAD>[-dirty]-private-local.zip` (+ `.sha256`).

Individual stages can also be executed independently:

```powershell
.\tools\windows\Build-Tooie.ps1 -Stage Check
.\tools\windows\Build-Tooie.ps1 -Stage Extract [-Rom <path>]
.\tools\windows\Build-Tooie.ps1 -Stage Recomp [-Jobs N]
.\tools\windows\Build-Tooie.ps1 -Stage Verify
.\tools\windows\Build-Tooie.ps1 -Stage Configure [-BuildDir <path>]
.\tools\windows\Build-Tooie.ps1 -Stage Build [-Jobs N]
.\tools\windows\Build-Tooie.ps1 -Stage Test [-AllTests]
.\tools\windows\Build-Tooie.ps1 -Stage Package
```

- **Jobs:** Defaults to the host's logical CPU count (`$env:NUMBER_OF_PROCESSORS`), or override with `-Jobs N`.
- **Output Executable:** `build-win\TooieRecompiled.exe` (with bundled `SDL2.dll`, `dxil.dll`, and `dxcompiler.dll`).
- **Package Verification:** `python tools/verify_package.py dist\TooieRecompiled-windows-x64-*.zip`.
- **Running the Game:** Execute `.\run-tooie.ps1` (supports the same options as `run-tooie.sh`) or double-click `build-win\TooieRecompiled.exe`. The launcher accepts both `Banjo-Tooie (USA).z64` and `decompressed.us.z64`.

#### Legacy Fallback: `ImportGenerated`

Importing pre-generated `RecompiledFuncs/` from a Linux/WSL2 build remains available as a legacy fallback:

```powershell
.\tools\windows\Build-Tooie.ps1 -Stage ImportGenerated -GeneratedFrom C:\path\to\RecompiledFuncs
```

*(This is preserved solely as a fallback; native recompiler generation via `-Stage Recomp` is now fully supported and recommended).*

---

## 2. Project Status & Known Limitations

- **Linux x86-64:** Kit-free generation fully proven and validated (`KITFREE_VALIDATED`). In an isolated bubblewrap sandbox (`tools/offline_sandbox.sh`, no network, no compiler kit, no ELF), `tools/build_linux.sh preflight verify extract recomp` generated all 420 `RecompiledFuncs/` files **byte-identical** to the ELF-route baseline. Full build completed cleanly (1159/1159 steps, 8 host tests PASS). Note: interim candidate binary `5c2c8df2...` was generated during the clean kit-free proof and is superseded by the restored UI rebuild. Root build of the restored UI completed cleanly (exit 0): 43/43 UI source/resources and 40 UI resource files match candidate-backup, producing binary SHA-256 `0cfcd79206efc74b74e26836ce468c69bcadaf2c3950b40eb31971946026892b` (equals original pre-redesign binary; `launcher-restored-menu.png` visually inspected exit 0). Final Linux package `TooieRecompiled-linux-x86_64-73bfd43-dirty-private-local.tar.gz` generated and verified (SHA-256 `4f702d2ba0a0472345f6f1317bfe75eb0df462e96b2352d6d875edaa5c4ebcf9`, 104 regular files, `verify_package.sh` exit 0, UI exact / contains no ROM; root kit-free preflight exit 0). Source export will be generated after final documentation with checksum in an adjoining external sidecar. Clean kit-free proof (neutral 5c2) is separate from the restored UI build proof.
- **Epistemic Clarification on Recompilation Equivalence:** Producing byte-identical C output proves metadata fidelity and generator equivalence across the symbol and ELF routes; it does **not** by itself prove that the ELF `.text` payload equals ROM slices (as the generator reads ROM function words in both routes). Furthermore, initial symbols-route comparison failed due to missing relocatable flags and dropped relocations, not stale metadata age.
- **Windows 11 x64:** Status is `WINDOWS_NATIVE_BUILD_PROVEN_DEV_HOST`. The native build pipeline (`Build-Tooie.ps1 -Stage All`), test suite (9/9 required PASS, 15/15 extra PASS, 5 driver targets SKIP), packaging, and launcher startup have been proven natively on host (commits `6862e29`..`f22e867`, pushed to `origin/main`). Recompiler output generation is byte-identical to the Linux baseline (420/420 files match `metadata/recomp-output.sha256`). The runtime launcher accepts both retail cartridge ROMs (`.z64`/`.n64`, decompressing in memory) and decompressed images. Pending items: validation on Linux of shared changes (user testing on physical Linux PC); audible confirmation of launcher music on Windows; W5 full manual validation (D3D12/Vulkan, audio, controller, graphics.json persistence); CI not triggered; package smoke test on Windows is non-isolated (writes configuration to `%LOCALAPPDATA%\TooieRecompiled`).
- **Transform Interpolation & Glitches:** The in-game Graphics menu includes an **Interpolation** toggle (persisted in `graphics.json`, defaulting strictly to `false` / 30 FPS / `CURRENT` endpoint). The F8 screenshot hotkey remains disabled; diagnostic RT64 burst and snapshot mechanisms remain available through explicit configuration flags. The P09 articulated transform glitch technical gate remains **FAILED** (`passed: false`); P09 and P10 remain formally open.
- **Licensing & Legal Boundaries:**
  - Machine-translated C code (`RecompiledFuncs/`) and translated microcode (`rsp/n_aspMain.cpp`) embed ROM-derived logic; candidate packages are designated strictly as `private-local`.
  - All native UI assets, typography fonts, animations, and vector art are vendored locally in `assets/` and `ui_assets/`. Provenance and license notices are tracked honestly (OFL 1.1 for Inter, Noto Emoji, and PromptFont; zlib for `recompcontrollerdb.txt`; origin and unverified license status of `Suplexmentary Comic NC.ttf` preserved without blocking restoration).
  - The formal project license remains open / pending owner decision. No blanket legal clearance or public redistribution license is claimed.

---

## 3. Documentation & Specifications

- [Master Implementation Plan](docs/plan/index.md)
- [Operational Handoff State](docs/HANDOFF.md)
- [Agent Navigation & Rules](AGENTS.md) / [Portuguese Agent Rules](docs/AGENTS.md)
- [Detailed Build System Guide](tools/BUILDING.md)
- [Asset Ownership & Licensing Manifest](docs/specs/Tooie/asset-and-licensing-manifest.md)
- [Build & Code Generator Specification](docs/specs/Tooie/build-and-generator.md)
- [Packaging & Dependency Specification](docs/specs/Tooie/packaging-and-dependencies.md)
- [Implementation Phases Index](docs/plans/BT/index.md)
- [Vendored Dependency Provenance](vendor/README.md)
