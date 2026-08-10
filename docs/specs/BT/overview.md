---
title: "BT — Decomp Overview"
status: current
area: "specs/BT"
updated: 2026-08-08
tags:
  - banjo-tooie-Recomp
  - decomp
---

# BT — Decompilation Overview

WIP decompilation of Banjo-Tooie (NTSC-U) by **Mr-Wiseguy** (author of
`N64Recomp` / `N64ModernRuntime` / `rarezip`). Uses **Splat** to split the
ROM and rebase the game to an ELF.

This is not a recomp port — it rebuilds bytes from C and verifies against the
original ROM (`matching decompilation`).

## Progress

Tracked per segment (decompals progress influx / progress.deco.mp):

- `all` / boot / core1 / core2 / overlays
- Badges update automatically in the repo README.

## What the repo builds

```text
baserom.us.z64 (compressed)
  -> rom_decompressor
  -> decompressed.us.z64
  -> splat split (baserom.us.yaml)
  -> src/ + asm/ + assets/ trees
  -> C compiled with IDO 5.3
  -> link (mips-linux-gnu-ld)
  -> build/us/banjotooie_decompressed.elf
  -> rom_compressor + n64crc
  -> build/us/banjotooie.z64
```

`make check` compares `decompressed.us.z64` (original) with the
`UNCOMPRESSED_ROM` extracted from the ELF: a successful diff means the
decompiled C **matches** the game bytes.

## Key facts

- Compiler: **IDO 5.3** (`tools/` downloads an ido static recomp).
- Assembler/linker: `mips-linux-gnu-*` binutils.
- Core memory: `include/` mirrors the game's C runtime (libultra from
  `Mr-Wiseguy/ultralib` `tooie` branch).
- Audio: Banjo-Tooie also uses the `n_aspMain` ASP ucode via core2.

## Relation to the N64 homework

`instruções.md` describes this repo as the *matching decomp* arm along a
parallel *static recomp* arm. Both arms share the same ELF and symbol data;
the recomp arm uses N64Recomp instead of IDO-rebuilding exactly.

See [Build](build.md), [Symbols](symbols.md), [Overlays](overlays.md),
and [TooieRecomp roadmap](../../roadmap/tooie-recomp.md).