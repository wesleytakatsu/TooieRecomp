---
title: "TooieRecomp Roadmap"
status: superseded
area: "roadmap"
updated: 2026-08-08
tags:
  - roadmap
  - tooie-recomp
---

# TooieRecomp Roadmap

> [!NOTE]
> Este roadmap histórico foi substituído pelo [[../plan/index|Plano mestre]] e
> pelas [[../plans/BT/index|etapas de implementação]]. Ele permanece como
> registro do raciocínio inicial.

Goal: build a static recomp of **Banjo-Tooie** for PC by adapting the
infrastructure used by **Banjo: Recompiled**, feeding it the reverse
engineering data from the **banjo-tooie-Recomp** decomp.

This is neither a rewrite nor a "swap the ROM" fork: Tooie's dynamic overlay
system and bigger memory map require real adaptation work.

## Why it is feasible

- Toolchain to adopt: `N64Recomp` + `RSPRecomp`.
- Rendering: `RT64`; runtime: `N64ModernRuntime`; menu/input:
  `RecompFrontend`.
- The Banjo Tooie decomp already produces an ELF
  (`build/us/banjotooie_decompressed.elf`), relocations, symbol maps and a
  full overlay table — the three inputs a recompiler historically needs.
- BanjoRecomp is a working reference implementation to copy and adapt.

## Pipeline to build

```text
Banjo-Tooie ROM
  -> decompression
  -> Tooie ELF (from banjotooie)
  -> symbols + relocs + overlay metadata
  -> tooie.us.syms.toml / tooie.us.datasyms.toml (converted)
  -> N64Recomp (main) + RSPRecomp (n_aspMain)
  -> RecompiledFuncs/*.c
  -> native compile (clang/MSVC/GC)
  -> Tooie-as-runtime (copy BK runtime + RT64 + RecompFrontend)
  -> TooieRecomp[.exe]
```

## What the Tooie decomp already contributes

| Need | Source in banjo-tooie-Recomp |
| ---------- | --------------------- |
| ELF | `build/us/banjotooie_decompressed.elf` |
| Function symbols | `symbol_addrs.us.txt`, `undefined_syms.us.txt` |
| Overlay symbols | `ovl_symbol_addrs.us.txt` |
| Syscall table | `syscall_symbol_addrs.us.txt` |
| Relocations   | `reloc_addrs.us.txt`, `ovl_reloc_addrs.us.txt` |
| Overlay metadata | `overlays.us.toml`, `overlay_table.s`, overlay headers |
| Headers / funcs | `include/` + `src/` C |

## The four hard parts

1. **Overlays** — 850+ dynamic banks sharing the 0x80800000 VRAM region.
   Recompile each overlay with its own address space; runtime must map
   original VRAM→native func pointer and call via syscall trampolines
   (see [BT overlays](../specs/BT/overlays.md)).
2. **RSP microcode** — Tooie presumably uses the `n_aspMain` family like BK.
   RSP reinterpretation config must be built and tested.
3. **Frame-rate independence** — the gameplay must not be coupled to
   30/20 FPS half-frames. Patch layer must split update vs. render.
4. **matching vs. respecing** — matched C is nicer but the rapid path is
   auto-translated binary; both are correct but different codebases.

## Recommended milestones

1. **Compile-only** — generate `RecompiledFuncs/*.c` and compile them with
   no crash: prove the toolchain + symbols are consistent.
2. **Boot** — exe reaches RDRAM init → boot → libultra → RSP → RT64 → Rare
   logo at 4:3. This validates architecture.
3. **Overlay loader** — register overlays / syscalls so a level can
   initialize.
4. **Gameplay** — drive a core loop; then features (widescreen, framerate,
   mods) incrementally.

## Golden criterion

First milestone target: Rare's logo rendering. That proves the whole
architecture chain works end-to-end before investing in per-system fixes.

## Files to copy & adapt from BanjoRecomp

- `banjo.us.rev0.toml` → `tooie.us.rev0.toml` (entrypoint, symbols path,
  overlays)
- `n_aspMain.us.rev0.toml` → Tooie ucode offset
- `patches.toml` + `patches/` → game-specific patches
- `src/main/*`, `src/game/*` — copy as starting runtime
- `CMakeLists.txt` — replace recompiled funcs/patches source references

## Docs

- BK references: [BK specs](../specs/BK/index.md)
- BT references: [BT specs](../specs/BT/index.md)
- Origin analysis: [`instruções.md`](../../../instruções.md) in repo root

## Status

Planning / research only. No TooieRecomp code started. This vault documents
the two upstream projects; it does not modify their code.

> Note for agents: `BanjoRecomp/CONTRIBUTING.md` states upstream rejects
> AI-generated code. Keep this project as documentation and reverse-engineering
> notes only.
