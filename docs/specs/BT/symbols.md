---
title: "BT — Symbols and Relocations"
status: current
area: "specs/BT"
updated: 2026-08-08
tags:
  - banjo-tooie-Recomp
  - symbols
---

# BT — Symbols and Relocations

The repo ships plain-text symbol and relocation maps used by Splat at build
time. These are the raw material for a static-recomp symbol table.

## Files

| File | Purpose |
| ---- | ------- |
| `symbol_addrs.us.txt` | boot/core1/core2 symbols (functions + data) |
| `undefined_syms.us.txt` | exterior/placeholder addresses (hardware regs, undecompressed offsets) |
| `reloc_addrs.us.txt` | boot/core1/core2 relocation records |
| `ovl_symbol_addrs.us.txt` | overlay symbols (functions + `R_` data into overlay) |
| `ovl_reloc_addrs.us.txt` | overlay relocation records |
| `syscall_symbol_addrs.us.txt` | overlay syscall dispatch table entrypoints |

## Format

`symbol_addrs.us.txt`:

```text
D_80000400 = 0x80000400; // segment: buffers ignore:true type:u8 size:0x400
D_80000400 = 0x80000400; // type:data
func_80002000 = 0x80002000; // type:func
```

Comments carry the metadata Splat uses: `segment:`, `type:`, `size:`,
`rom:`, `ignore:true`.

`ovl_symbol_addrs.us.txt`:

```text
func_80800040_gs = 0x80800040; // segment:gs rom:0x02178C20 type:func
R_80800280_gs = 0x80800280; // segment:gs rom:0x02178E60
gz_entrypoint_0 = 0x80800148; // segment:gz rom:0x02178D28
```

Overlay functions live in the `0x80800000` overlay VRAM space; `rom:` gives
the file offset.

`reloc_addrs.us.txt`:

```text
rom:0x1024 symbol:boot_thread_stack reloc:MIPS_HI16 addend:0x2000
rom:0x1028 symbol:boot_thread_stack reloc:MIPS_LO16 addend:0x2000
```

`syscall_symbol_addrs.us.txt`:

```text
overlay_syscalls = 0x80082540;
_gsmaker_entrypoint_0 = 0x80082548;
```

## What the linker consumes

`undefined_syms.us.txt` is passed to `ld` via `-T undefined_syms.us.txt`:

```text
core1_compressed_ROM_START = 0x1E29B60;
core1_compressed_ROM_END = 0x1E42550;
core2_compressed_ROM_START = 0x1E42550;
core2_compressed_ROM_END = 0x1E899B0;
overlay_table_compressed_ROM_START = 0x1E899B0;
```

Some are replaced during compression (see comment in file).

## Tooie relevance

These maps are the equivalent of `BanjoRecompSyms/bk.us.rev0.syms.toml` for
BK. A recomp pipeline must convert them into `tooie.us.syms.toml` +
`tooie.us.datasyms.toml`.

See [BK Symbols](../BK/symbols.md), [Splat](splat.md),
[TooieRecomp roadmap](../../roadmap/tooie-recomp.md).