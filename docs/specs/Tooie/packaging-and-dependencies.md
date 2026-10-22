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
1. **Zero Copyrighted Game Assets**:
   - The compiled executable and repository do not contain copyrighted textures, audio samples, dialog text, model geometry, or level maps.
   - Machine-translated C code (`RecompiledFuncs/`) and translated microcode (`rsp/n_aspMain.cpp`) embed ROM-derived logic and are strictly designated for private local testing.
   - User-interface resources (`assets/` and `ui_assets/`) contain all restored menu assets, character vector art, animations, icons, typography fonts (including `Suplexmentary Comic NC.ttf`, `InterVariable.ttf`, `NotoEmoji-Regular.ttf`, and `promptfont.ttf`), and the 7300x2450 Tooie logo; zero ROM game data is bundled.
2. **User-Provided ROM**:
   - End users must provide their own legally acquired cartridge dump:
     - Format: Big-Endian `.z64`.
     - Region & Revision: NTSC-U Rev. 0.
     - Internal Header ID: `NB7E`.
     - File Size: Exactly 33,554,432 bytes (32 MiB).
     - Expected MD5: `40e98faa24ac3ebe1d25cb5e5ddf49e4`.
     - Expected SHA-1: `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`.
     - Launcher Decompressed Image Requirement: `tools/extract_rom.sh` decompresses the cartridge into `decompressed.us.z64` (size `35098224` bytes, SHA-1 `af479771ff5e91ce3732f3d84146614ab71d8d79`), which the launcher requires at startup (raw cartridge ROMs are rejected).
3. **Asset Pipeline (`src/game/rom_decompression.cpp`)**:
   - Initial boot identifies user ROM file path.
   - Verifies hash against `GameEntry.rom_hash`.
   - Populates guest cartridge memory buffer and registers boot lookup stubs (`banjo::register_boot_lookup_stubs`).

---

## 4. Host Dependency Boundaries

TooieRecomp links dynamically only against ubiquitous system libraries to ensure maximum portability across Linux distributions:

| Dependency | Minimum Version | Shared Library (`.so`) | Invariant / Boundary |
| :--- | :--- | :--- | :--- |
| `glibc` | 2.31+ | `libc.so.6`, `libm.so.6`, `libpthread.so.0` | Intended reference target (Ubuntu 20.04+, Debian 11+); tested on Linux Mint 22.2 (glibc 2.39). |
| `libvulkan` | 1.2+ | `libvulkan.so.1` | Modern Vulkan loader. Fallback dialog on failure. |
| `libSDL2` | 2.0.12+ | `libSDL2-2.0.so.0` | X11/Wayland windowing and GameController subsystem. |
| `libz` | 1.2.11+ | `libz.so.1` | Compression library. **Portability Blocker B3**: current candidate dynamically bound to `/usr/local/lib/libz.so.1`; must be rebuilt against standard system `zlib1g` before distribution. |
| `libasound / libpulse` | Any | Interfaced via SDL2 | Host sound output; no direct host driver binding. |
| Modern C++ Runtime | GCC 11+ / Clang 14+ | `libstdc++.so.6` or `libc++.so.1` | Requires full C++20 standard library support. |

Static components (`RT64`, `N64ModernRuntime`, `ultramodern`, `fmt`, `toml11`, `xxHash`) are compiled directly into the binary to avoid external dynamic dependency conflicts. Note that while glibc >= 2.31 is the targeted distribution baseline, production validation requires an actual release package produced and tested on a clean glibc 2.31 container or VM during phase P10.

---

## 5. Candidate Packaging Verification (October 2026)

- **Linux Candidate:** Package `TooieRecompiled-linux-x86_64-73bfd43-dirty-private-local.tar.gz` (SHA-256 `4f702d2ba0a0472345f6f1317bfe75eb0df462e96b2352d6d875edaa5c4ebcf9`, 104 regular archive files, binary SHA-256 `0cfcd79206efc74b74e26836ce468c69bcadaf2c3950b40eb31971946026892b` equals original binary, launcher visual verified, zero ROM, UI exact) generated and verified by `tools/package_linux.sh` and `tools/verify_package.sh` (exit 0). Supersedes initial packages `b6475879...`, `64f130aa...` and historical pre-standalone candidates. Status: `LINUX_CANDIDATE_PACKAGE_VERIFIED` (candidate local package; P10 not closed). F1 `ldconfig` resolution fixed (0 warnings). 20-second launch smoke verified process liveness on X11/RADV under both default loader and distro zlib override (`TOOIE_SMOKE_LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu`). Note: smoke verifies process liveness only; gameplay and rendering were not evaluated. Root kit-free preflight exit 0. Clean kit-free proof (neutral 5c2) is separate from restored UI build.
- **Windows Candidate:** Status é `WINDOWS_NATIVE_BUILD_PROVEN_DEV_HOST`. O pipeline nativo no Windows 11 x64 foi comprovado no host de desenvolvimento (`tools\windows\Build-Tooie.ps1 -Stage All`, commits `6862e29`..`f22e867` em `origin/main`). Produziu o executável `build-win/TooieRecompiled.exe` (23.944.704 bytes), DLLs de suporte (`SDL2.dll`, `dxil.dll`, `dxcompiler.dll`) e o pacote de distribuição `dist/TooieRecompiled-windows-x64-<HEAD>[-dirty]-private-local.zip` (+ `.sha256`) auditado via `tools/verify_package.py` (106 arquivos no manifesto, `AUDIT.txt` PASS, sem ROM/ELF). Suíte de testes: 9/9 obrigatórios PASS, 15/15 extras PASS, 5 alvos de driver com SKIP documentado. Launcher comprovado aceitando ROMs de cartucho comercial (`.z64`/`.n64`) e imagens descomprimidas. Smoke test de execução (15s) passou (ressalva: não é isolado no Windows, pois grava configurações em `%LOCALAPPDATA%\TooieRecompiled`). Pendências: validação manual interativa completa W5 e revalidação no Linux das alterações compartilhadas.
- **Source Archive:** Final delivery is `dist/standalone-20261021/TooieRecomp-source-73bfd430-dirty.tar.gz`, exported after documentation finalization; its checksum is supplied in the adjacent `.sha256` sidecar (avoids a self-referential archive hash). Snapshot `4aab8d213dfe77c411db91d68441c6b34dce7b0bc8b3b3cb7528b23c2d1ee16f` is historical pre-doc evidence, not the final delivery.
- **Asset Ownership and Legal Manifest:** Detailed legal boundaries, ROM-derived code classification, and third-party asset licenses are cataloged in [[asset-and-licensing-manifest|Asset and Licensing Manifest]].

## 6. Standalone Source Archive and Vendored Closure Contract (2026-10-21)

Contract (stable parts only; measured results live in `docs/plan/evidence/p10-standalone-vendor-20261021.md`):
1. **One checkout.** All source dependencies are regular files in the repository (`lib/`, `decomp/`, `assets/`, `recompcontrollerdb.txt`, `tools/wheelhouse/`); no submodule, gitlink, symlink, sibling checkout, `third_party/` cache or build-time download. `tools/vendor_verify.sh` proves it offline against `vendor/MANIFEST.sha256` (8266 files, 118 exec bits). Reverse-check verifies all 18 patches (17 prior integrated + 1 local SDL target seam).
2. **Explicit prerequisites** (not in git): the ROM (`TOOIE_ROM`), the legacy IDO/gcc/binutils kit (`TOOIE_TOOLCHAIN_KIT`; hashes in `tools/toolchain.sha256`), OS packages. Python tooling comes from the hash-pinned wheelhouse (`pip --no-index --require-hashes`).
3. **Source archive** = `tools/package_source.sh`: the git-visible tree as regular files, **excluding** personal agent/editor configs (`.claude .cursor .gemini .grok .serena .opencode .agents .codex graft GEMINI.md .ignore opencode* .mcp.json .vscode/mcp.json`), local scratch (`cpu_math_check`, `dist/`, `build*/`), and refusing any ROM-type file, `RecompiledFuncs/`, decomp build outputs or compiler-kit paths. It may include the allowlisted third-party DXC/SDL2/FreeType binaries (`vendor/BINARIES.tsv`), never the game binary, the private generated input or build outputs. `assets/` is a real directory in the archive. Policy note on C2: 10 HEAD-tracked agent/MCP configuration files (`.mcp.json`, etc.) contain machine-local absolute paths (`/home/wesley/...`) and are deliberately excluded from source archives to protect user privacy and portability.
4. **Provenance** lives in `vendor/` (pins, licence status, integrated patch chain, derived-file list, binary allowlist, excludes). The 17 historical patches are integrated; builds never apply patches. F5 resolution: nested `.gitrepo` metadata in `decomp/lib/ultralib/tools/asm_differ/` removed; exclusion rule `(^|/)\.gitrepo$` added to `vendor/EXCLUDES.txt`; origin details preserved in `vendor/provenance/decomp-ultralib-asm_differ-gitrepo.origin.txt`.
5. **Network isolation proof** uses `tools/offline_sandbox.sh` (bubblewrap: no network, no host filesystem except system dirs and the checkout). F1 resolution: `tools/package_linux.sh` uses fallback chain `/sbin/ldconfig` to resolve library mappings cleanly under restricted PATH without warnings.
6. **Packaging metadata C3 (FIXED):** When Git metadata is absent, `BUILD-INFO.txt` reports `tooie_tree_dirty_files: unavailable (source export without Git metadata)`, rather than a false zero. The exported tree and the dirty working checkout have distinct provenance scopes.
7. Public redistribution of the source archive or packages stays blocked while `UNKNOWN`/`UNVERIFIED` licence rows in `vendor/UPSTREAMS.tsv` / `asset-and-licensing-manifest.md` are unresolved (owner decision).

Windows integrity prerequisites: Python 3.8+ disponível (`py -3`, `python`; stub da Microsoft Store ignorado). O script `tools/vendor_verify.sh` roda sob Git Bash (verificação de bits executáveis POSIX é explicitamente pulada no MSYS/MINGW; checagens de SHA-256 e de binários permitidos são estritas). Regra operacional fundamental: `tools/vendor_manifest.sh` NÃO deve ser executado no Windows, pois o sistema de arquivos NTFS regrava permissões que corrompem `vendor/MODES.tsv`; atualizações de patches no Windows devem editar exclusivamente as linhas de hash dos arquivos alterados em `vendor/MANIFEST.sha256`. A cadeia de patches locais foi validada reversamente com `vendor_verify.sh --reverse-check` (25 patches, exit 0).
