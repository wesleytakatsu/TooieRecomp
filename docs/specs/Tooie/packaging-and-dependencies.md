---
title: "TooieRecomp Spec — Packaging, Releases and Dependency Boundaries"
description: "Comprehensive technical specification for standalone Linux packaging, clean-room asset separation, legal ROM requirements, and runtime dependencies."
status: current
area: "specs/Tooie"
updated: 2026-10-16
tags: [specs, tooierecomp, packaging, release, linux, dependencies, assets]
---

# Packaging, Releases and Dependency Boundaries Specification

## 1. Release Architecture & Standalone Packaging (P10)

Under [[../../plan/definition-of-done|Definition of Done]], TooieRecomp must produce a clean, self-contained, and redistributable Linux x86-64 application:
- Target Platform: Linux x86-64 (glibc >= 2.31).
- Primary Binary: `TooieRecompiled`.
- Release Artifact Formats:
  - Standalone compressed tarball (`.tar.gz`).
  - Linux AppImage (`.AppImage`).
  - Flatpak manifest.

```text
+---------------------------------------------------------------------------------+
|                        Release Packaging Pipeline (P10)                         |
|  - Compiles TooieRecompiled (Clang/GCC, Release optimization)                   |
|  - Bundles native shaders (RT64 HLSL/SPIR-V), UI fonts, controller DB           |
|  - Validates zero proprietary game assets are embedded                          |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                        Clean-Room Asset Extraction Pipeline                     |
|  - User supplies legitimate Banjo-Tooie NTSC-U ROM (.z64)                       |
|  - Validates SHA-1: af1a89e12b638b8d82cc4c085c8e01d4cba03fb3                   |
|  - Decompresses and extracts game assets to user cache (~/.local/share/tooie)    |
+---------------------------------------------------------------------------------+
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Implementation Status |
| :--- | :--- | :--- | :--- |
| **REQ-PKG-01** | Zero Proprietary Assets | Binary and package contain zero Nintendo/Rare copyrighted assets. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-PKG-02** | Legal ROM Checksum | First-launch verifies SHA-1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-PKG-03** | On-the-Fly Decompression | Decompresses 32 MiB cartridge into working memory cache. | `PLANNED` / `IN_PROGRESS` |
| **REQ-PKG-04** | Broad Linux Compatibility| Runs on glibc >= 2.31 distributions without requiring root/sudo. | `INTENDED_REFERENCE_TARGET` / `UNVALIDATED` |
| **REQ-PKG-05** | Standalone Portability | Dynamically links only ubiquitous standard system libraries. | `IMPLEMENTED` / `VALIDATED` |

---

## 3. Clean-Room Asset Separation & Legal ROM Contract

To ensure strict compliance with copyright law and project invariants:
1. **Zero Copyrighted Assets**:
   - The compiled executable and repository do not contain copyrighted textures, audio samples, dialog text, model geometry, or level maps.
2. **User-Provided ROM**:
   - End users must provide their own legally acquired cartridge dump:
     - Format: Big-Endian `.z64`.
     - Region & Revision: NTSC-U Rev. 0.
     - Internal Header ID: `NB7E`.
     - File Size: Exactly 33,554,432 bytes (32 MiB).
     - Expected SHA-1: `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`.
3. **Asset Pipeline (`src/game/rom_decompression.cpp`)**:
   - Initial boot identifies user ROM file path.
   - Verifies hash against `GameEntry.rom_hash`.
   - Populates guest cartridge memory buffer and registers boot lookup stubs (`banjo::register_boot_lookup_stubs`).

---

## 4. Host Dependency Boundaries

TooieRecomp links dynamically only against ubiquitous system libraries to ensure maximum portability across Linux distributions:

| Dependency | Minimum Version | Shared Library (`.so`) | Invariant / Boundary |
| :--- | :--- | :--- | :--- |
| `glibc` | 2.31+ | `libc.so.6`, `libm.so.6`, `libpthread.so.0` | Intended reference target (Ubuntu 20.04+, Debian 11+); unvalidated until P10 artifact build. |
| `libvulkan` | 1.2+ | `libvulkan.so.1` | Modern Vulkan loader. Fallback dialog on failure. |
| `libSDL2` | 2.0.12+ | `libSDL2-2.0.so.0` | X11/Wayland windowing and GameController subsystem. |
| `libasound / libpulse` | Any | Interfaced via SDL2 | Host sound output; no direct host driver binding. |
| Modern C++ Runtime | GCC 11+ / Clang 14+ | `libstdc++.so.6` or `libc++.so.1` | Requires full C++20 standard library support. |

Static components (`RT64`, `N64ModernRuntime`, `ultramodern`, `fmt`, `toml11`, `xxHash`) are compiled directly into the binary to avoid external dynamic dependency conflicts. Note that while glibc >= 2.31 is the targeted distribution baseline, production validation requires an actual release package produced and tested on a clean glibc 2.31 container or VM during phase P10.
