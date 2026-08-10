---
title: "BK — BanjoRecompSyms"
status: current
area: "specs/BK"
updated: 2026-08-08
tags:
  - banjo-recomp
  - symbols
  - n64recomp
---

# BK — BanjoRecompSyms

The `BanjoRecompSyms` submodule provides the symbol maps that let `N64Recomp`
know which bytes map to which function. It is the BK analog of what
`banjo-tooie-Recomp` produces via `symbol_addrs.us.txt` + ELF.

## Layout

`BanjoRecompSyms/bk.us.rev0.syms.toml` and
`BanjoRecompSyms/bk.us.rev0.datasyms.toml`.

Both are TOML files in the format used by `N64Recomp`:

- `syms.toml` — function symbols (`.text`)
- `datasyms.toml` — data symbols (`.rodata`, `.data`, `.bss`, overlays ROM data)

## Usage

| Consumer | File used |
| ------------------------ | ------------------------------------------------ |
| game recomp (`banjo.us.rev0.toml`) | `symbols_file_path = BanjoRecompSyms/bk.us.rev0.syms.toml` |
| patches (`patches.toml`) | `func_reference_syms_file = .../bk.us.rev0.syms.toml` |
| patches | `data_reference_syms_files = [ .../bk.us.rev0.datasyms.toml ]` |

## Why it matters for Tooie

For a `TooieRecomp` the equivalent files must be generated. Banjo-tooie already
contains:

- `symbol_addrs.us.txt`
- `undefined_syms.us.txt`
- `reloc_addrs.us.txt`
- `ovl_symbol_addrs.us.txt`, `ovl_reloc_addrs.us.txt`
- `syscall_symbol_addrs.us.txt`

which are the raw material to produce a `tooie.us.syms.toml` +
`tooie.us.datasyms.toml`.

See [BT Symbols](../BT/symbols.md) and
[TooieRecomp roadmap](../../roadmap/tooie-recomp.md).