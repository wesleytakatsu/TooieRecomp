---
title: "BK — Build"
status: current
area: "specs/BK"
updated: 2026-10-21
tags:
  - banjo-recomp
  - build
  - reference
---

# BK — Build (Historical Reference Specification)

> [!WARNING]
> **Reference Specification Only:** This document describes the historical Banjo-Kazooie (`BanjoRecomp`) reference port build pipeline for architectural context. **Do NOT execute these commands for TooieRecomp.** TooieRecomp is fully standalone; its build pipeline lives in [`README.md`](../../../README.md) and [`tools/BUILDING.md`](../../../tools/BUILDING.md) and requires no external repository checkouts or submodules.

---

## 1. Upstream BanjoRecomp Pipeline (Reference Architecture)

In the upstream reference project (Banjo-Kazooie), the build proceeded from a clean clone with submodules:

1. **Submodules:** Upstream cloned submodules (`N64ModernRuntime`, `RT64`, `RecompFrontend`). In TooieRecomp, all shared sources are vendored as regular files under `lib/` and `decomp/`.
2. **Decompression:** Banjo-Kazooie used `bk_rom_compressor` to decompress the ROM for offline static recompilation. In TooieRecomp, `tools/extract_rom.sh` decompresses `Banjo-Tooie (USA).z64` into `decompressed.us.z64`.
3. **Static Generation:** Upstream ran `N64Recomp banjo.us.rev0.toml` and `RSPRecomp n_aspMain.us.rev0.toml`. TooieRecomp runs `tools/run_recomp.sh` and `tools/run_rsprecomp.sh` over Tooie-specific configurations.
4. **Host Compilation:** Upstream built the `BanjoRecompiled` executable. TooieRecomp builds `TooieRecompiled` via CMake and `tools/build_linux.sh` (or `tools/windows/Build-Tooie.ps1`).

---

## 2. Supporting Pieces & Provenance

- `patches/` — upstream native C patch overrides.
- `CMakeLists.txt` — links vendored `lib/rt64`, `lib/N64ModernRuntime`, `lib/RecompFrontend`, plus recompiled C code and native shims.
- Provenance and license tracking for all borrowed runtime elements are maintained in [`vendor/`](../../../vendor/README.md).

---

## 3. Tooie Relevance

Tooie uses its own `tooie.us.rev0.toml`, `tooie.us.rev0.recomp.toml`, decompressed ROM image (`decompressed.us.z64`), and RSP microcode config. While architectural patterns (static table generation, overlay registration, RT64 integration) were borrowed from BanjoRecomp, TooieRecomp does not depend on or build any other checkout.
