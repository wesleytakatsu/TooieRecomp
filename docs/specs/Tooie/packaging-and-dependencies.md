---
title: "TooieRecomp Spec — Packaging, Releases and Dependency Boundaries"
description: "Technical specification for standalone Linux and Windows packaging, clean-room asset separation, legal ROM requirements, and runtime dependencies."
status: current
area: "specs/Tooie"
updated: 2026-10-28
tags: [specs, tooierecomp, packaging, release, linux, windows, dependencies, assets]
---

# Packaging, Releases and Dependency Boundaries Specification

## 1. Release Architecture & Standalone Packaging

TooieRecomp produces self-contained, redistributable application packages for Linux x86-64 and Windows x64:
- Target Platforms: Linux x86-64 and Windows x64.
- Primary Binaries: `TooieRecompiled` (Linux) and `TooieRecompiled.exe` (Windows).
- Release Artifact Formats:
  - Standalone compressed tarball (`.tar.gz`) on Linux (`tools/package_linux.sh`):
    - Release: `TOOIE_RELEASE_VERSION=<ver> tools/package_linux.sh` → `dist/BanjoTooieRecompiled-v<ver>-Linux-x64.tar.gz`.
    - Development: `dist/BanjoTooieRecompiled-dev-<git describe --always --dirty>-Linux-x64.tar.gz`.
  - Standalone compressed zip archive (`.zip`) on Windows (`Build-Tooie.ps1 -Stage Package`):
    - Release: `Build-Tooie.ps1 -Stage Package -ReleaseVersion <ver>` → `dist\BanjoTooieRecompiled-v<ver>-Windows-x64.zip`.
    - Development: `dist\BanjoTooieRecompiled-dev-<git describe --always --dirty>-Windows-x64.zip`.
  - Archive Structure & Metadata:
    - Each archive contains a single top-level `BanjoTooieRecompiled` directory.
    - Checksum sidecar: `<archive>.sha256`.
    - Package metadata: `BUILD-INFO.txt` (recording exact commit/dirty state and `release_version`), `AUDIT.txt`, `DEPENDENCIES.txt`, `MANIFEST.sha256`, and `THIRD-PARTY-MANIFEST.tsv`.
    - End-user documentation: `README-PACKAGE.txt` (explains ROM requirements, SHA-1 verification, controls/saves, legal notices, and repository URL).
  - Standalone source archive (`tools/package_source.sh`).
  - Linux `.deb` and AppImage files were published with `v0.1.0-p08`, but they were built by hand on Linux: there is no
    versioned script for them yet. Flatpak is `NOT_IMPLEMENTED`.
  - The `assets/` tree (UI resources and `assets/languages/pt-BR.tlpb`) is copied next to the executable by the CMake
    target `TooieUIAssets` and shipped in every package. Imported FR/DE/ES packs are written to the user's app folder,
    never to the package.

```text
+---------------------------------------------------------------------------------+
|                        Release Packaging Pipeline                               |
|  - Compiles TooieRecompiled (Clang / clang-cl, Release optimization)            |
|  - Bundles native shaders (RT64 HLSL/SPIR-V), UI assets, controller database     |
|  - Generates AUDIT.txt, BUILD-INFO.txt, DEPENDENCIES.txt, MANIFEST.sha256       |
|  - Validates package contents via tools/verify_package.py                       |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                        Clean-Room Asset Extraction Pipeline                     |
|  - User supplies retail Banjo-Tooie NTSC-U ROM (.z64) or decompressed image    |
|  - Launcher decompresses retail ROM on the fly in memory                        |
|  - Save files persist to <config>/saves/<game_id>.bin                           |
+---------------------------------------------------------------------------------+
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Status |
| :--- | :--- | :--- | :--- |
| **REQ-PKG-01** | Zero Proprietary Assets | Binaries and packages contain zero Nintendo/Rare copyrighted assets. | `IMPLEMENTED` |
| **REQ-PKG-02** | Legal ROM Checksum | Launcher validates ROM hashes (`af1a89e1...` / `af479771...`). | `IMPLEMENTED` |
| **REQ-PKG-03** | On-the-Fly Decompression | Launcher transparently decompresses retail 32 MiB cartridge ROMs in memory. | `IMPLEMENTED` |
| **REQ-PKG-04** | Multiplatform Distribution | Distributes standalone packages for Linux x86-64 and Windows x64. | `IMPLEMENTED` |
| **REQ-PKG-05** | Standalone Portability | Dynamically links only standard platform libraries. | `IMPLEMENTED` |

---

## 3. Clean-Room Asset Separation & Legal ROM Contract

1. **Zero Copyrighted Game Assets**:
   - The compiled executable and repository do not contain copyrighted textures, audio samples, dialog text, model geometry, or level maps.
   - Machine-translated C code (`RecompiledFuncs/`) and translated microcode (`rsp/n_aspMain.cpp`) embed ROM-derived logic; they are compiled directly into the released executable and excluded from source tracking (`RecompiledFuncs/` is never published as source).
   - User-interface resources (`assets/` and `ui_assets/`) contain restored menu assets, character vector art, animations, icons, typography fonts, and UI wordmarks; zero ROM game data is bundled.
2. **User-Provided ROM**:
   - End users provide their own legally acquired cartridge dump or decompressed image:
     - Retail Cartridge: Big-Endian `.z64`, NTSC-U Rev. 0, internal header `NB7E`, size 33,554,432 bytes (32 MiB), MD5 `40e98faa24ac3ebe1d25cb5e5ddf49e4`, SHA-1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`, XXH3 `0x00F70DE5F2D70EA2`.
     - Decompressed Image: Size 35,098,224 bytes, SHA-1 `af479771ff5e91ce3732f3d84146614ab71d8d79`, XXH3 `0x9F843AD3455EA395`.
   - **Dual Acceptance**: The launcher (`rom_decompression.cpp`, `main.cpp`) accepts either format. Retail cartridges are decompressed transparently in memory without writing intermediate files; pre-decompressed images are mapped directly.
3. **Initialization & Persistence**:
   - `bk_on_init` registers boot lookup stubs (`banjo::register_boot_lookup_stubs`).
   - Save files and configuration persist to `<config_path>/saves/<game_id>.bin` (on Windows: `%LOCALAPPDATA%\TooieRecompiled`).

---

## 4. Host Dependency Boundaries

| Dependency | Platform | Purpose | Linkage |
| :--- | :--- | :--- | :--- |
| `glibc` / `libm` / `pthread` | Linux | C standard library and POSIX threads | Dynamic |
| `libvulkan` | Linux | Vulkan graphics loader | Dynamic |
| `Direct3D 12` / `DXGI` | Windows | Graphics API (default backend) | System dynamic |
| `libSDL2` | Linux | Windowing and controller input | System dynamic |
| `SDL2.dll` | Windows | Windowing and controller input | Bundled (vendored VC build) |
| `dxil.dll` / `dxcompiler.dll` | Windows | DirectX Shader Compiler | Bundled (vendored DXC) |
| `FreeType` | Linux | Font rendering | System dynamic |

Static components (`RT64`, `N64ModernRuntime`, `ultramodern`, `fmt`, `toml11`, `xxHash`) are compiled directly into the binary to prevent host dependency conflicts.

---

## 5. Packaging Invariants & Validation Rules

1. **Forbidden Files in Distribution Packages**:
   - Packages must NOT contain `.z64`, `.n64`, `.v64`, `.elf`, `.sav`, `.eep`, or `.log` files.
   - Any `*.json` file at the root of the distribution archive is strictly forbidden.
   - Specific configuration files (`graphics.json`, `general.json`, `sound.json`, `controls.json`) must not be bundled anywhere in the package.
   - No `baserom*` or `decompressed*` ROM images.
2. **License Validation**:
   - Every file bundled in the distribution archive must have an explicit attribution rule mapped in `tools/package_licenses.tsv`.
   - Package integrity is verified via `tools/verify_package.py`.
   - Rows marked UNKNOWN or UNVERIFIED in `THIRD-PARTY-MANIFEST.tsv` remain open owner items to clear.
   - Distribution packages contain no game assets and no ROM; binaries are released publicly (first public-ready release: `v0.1.0-p08`).
3. **Execution Scripts**:
   - Package launchers `run-tooie.sh` and `run-tooie.ps1` manage execution environment variables:
     - `TOOIE_BUILD_DIR`: Path to build directory.
     - `TOOIE_AUTO_START`: Skips launcher GUI directly into gameplay.
     - `TOOIE_DIAG`: Activates telemetry output.
     - `TOOIE_DIAG_ATTRACT`: Enables attract demo monitoring.
     - `TOOIE_DISABLE_LAUNCHER_MUSIC`: Silences launcher audio preview.
     - `TOOIE_DIAG_INTERVAL_MS`: Telemetry sampling interval.
4. **Windows Vendoring Precaution**:
   - `tools/vendor_manifest.sh` must NOT be executed under Windows, as NTFS permission semantics corrupt `vendor/MODES.tsv`. Manifest updates on Windows must be restricted to hash entries in `vendor/MANIFEST.sha256`.

---

## 6. Related Documentation

- **[Asset Ownership & Licensing Manifest](asset-and-licensing-manifest.md)**: Copyright boundaries, legal ROM requirements, third-party component licenses, and package distribution policies.
- **[Operational Handoff](../../HANDOFF.md)**: Current system status, open packaging items, and release readiness.