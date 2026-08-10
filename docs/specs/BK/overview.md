---
title: "BK — Static Recompilation Overview"
status: current
area: "specs/BK"
updated: 2026-08-08
tags:
  - banjo-recomp
  - recomp
---

# BK — Static Recompilation Overview

`BanjoRecomp` recompiles Banjo-Kazooie (NTSC-U 1.0) into a native PC port.
It does **not** require decompiled source code. Instead it translates the raw
MIPS binary into C via `N64Recomp`, then compiles that C with a modern
compiler (clang/MSVC/GCC) into native x86-64/ARM64 code.

## High level pipeline

```text
Banjo-Kazooie.z64 (US 1.0)
  -> decompress
  -> banjo.us.v10.decompressed.z64
  -> N64Recomp -> C (RecompiledFuncs/)
  -> RSPRecomp -> n_aspMain.cpp (ASP microcode)
  -> clang / MSVC / GCC
  -> native code
  -> reuse N64ModernRuntime + RT64 + RecompFrontend
  -> BanjoRecompiled[.exe]
```

The game's own assets are **not** shipped. The executable grabs them from a
user-provided ROM at runtime. The repository and releases contain no game
assets.

## Key property: no source required

The project does not depend on the decompiled Banjo-Kazooie source to produce
the port. The reverse engineering work of the decomp team is still valuable:

- headers and some function definitions are reused
- names help patch authoring

But the bulk of code is converted directly from the MIPS binary. The recompiler
receives an address-to-function map and turns instructions one by one:

```asm
addiu $r4, $r4, 0x20   ->   ctx->r4 = ADD32(ctx->r4, 0x20);
```

## Libraries / projects used

| Project            | Role                                            | Submodule path        |
| ------------------ | ----------------------------------------------- | --------------------- |
| N64Recomp          | static recompiler (MIPS `->` C)                | external bin          |
| RSPRecomp          | RSP (reality signal processor) microcode recompiler | external bin  |
| RT64               | rendering engine (D3D12/Vulkan/Metal)         | `lib/rt64` |
| N64ModernRuntime   | replaces original N64 runtime; mod support    | `lib/N64ModernRuntime` |
| RecompFrontend     | menus and input handling                        | `lib/RecompFrontend` |
| Banjo-Kazooie decomp | headers + some function defs for patches     | `lib/bk-decomp` |
| BanjoRecompSyms    | symbol maps (`syms.toml` / `datasyms.toml`)       | `BanjoRecompSyms` |

## Game ROM requirement

Only one ROM is accepted: NTSC-U 1.0 Banjo-Kazooie.

| Item     | Value                                                 |
| ---------- | ---------------------------------------------------------- |
| SHA1      | `1fe1632098865f639e22c11b9a81ee8f29c75d7a`                  |
| decompressed ROM | `banjo.us.v10.decompressed.z64`                          |
| entrypoint | `0x80000400` (see `banjo.us.rev0.toml`) |