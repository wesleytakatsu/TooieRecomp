---
title: "BK — Build"
status: current
area: "specs/BK"
updated: 2026-08-08
tags:
  - banjo-recomp
  - build
---

# BK — Build

Full pipeline to go from the Banjo-Kazooie US ROM to a native executable,
based on `BUILDING.md`.

## 1. Clone

```sh
git clone --recurse-submodules
```

## 2. Dependencies

| OS | Packages |
| ------ | -------------------------------------------------------- |
| Ubuntu (Linux) | `cmake ninja-build libsdl2-dev libgtk-3-dev lld llvm clang` |
| Windows | Visual Studio 2022 with C++, Clang for Windows, CMake tools; `make` via Chocolatey |

## 3. Decompress the ROM

Requires the NTSC-U 1.0 Banjo-Kazooie ROM
(SHA1 `1fe1632098865f639e22c11b9a81ee8f29c75d7a`).

Two ways to decompress:

- Set up the Banjo-Kazooie decompilation (gitlab) — decompresses during its
  build.
- Use `bk_rom_compressor --decompress` (MittenzHugg).

Copy result as `banjo.us.v10.decompressed.z64` in repo root.

## 4. Generate C code

Build `N64Recomp` and `RSPRecomp`, copy the binaries to repo root, then:

```sh
./N64Recomp banjo.us.rev0.toml
./RSPRecomp n_aspMain.us.rev0.toml
```

- `N64Recomp` → `RecompiledFuncs/`
- `RSPRecomp` → `rsp/n_aspMain.cpp`

## 5. Build project

```sh
cmake -S . -B build-cmake \
  -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang \
  -G Ninja -DCMAKE_BUILD_TYPE=Release      # or Debug
cmake --build build-cmake --target BanjoRecompiled -j$(nproc) --config Release
```

Windows: open in VS, build/run/debug from IDE.

## 6. Result

- `build-cmake/BanjoRecompiled` (CMake) or `out/build/x64-<Config>` (VS).
- Run from repo root, or copy `assets/` next to the executable.

> The game itself, in the running app, must use a standard (not decompressed)
> ROM. The decompressed ROM is only for building.

## Supporting pieces

- `patches/Makefile` — builds `patches/patches.elf` (see [Patches](patches.md))
- `CMakeLists.txt` — wires `lib/rt64`, `lib/N64ModernRuntime`,
  `lib/RecompFrontend`, plus the recompiled funcs/patches.
- `banjo.us.rev0.toml` — see [Config](config.md).

## Tooie relevance

Tooie needs its own `tooie.us.rev0.toml`, decompressed ROM, symbol map and
RSP ucode config. The `banjo-tooie-Recomp` repo gives the symbol/overlay raw
material; the pipeline scaffolding is copied from BK.

See [TooieRecomp roadmap](../../roadmap/tooie-recomp.md).