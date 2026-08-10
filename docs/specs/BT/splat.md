---
title: "BT — Splat configuration"
status: current
area: "specs/BT"
updated: 2026-08-08
tags:
  - banjo-tooie-Recomp
  - splat
---

# BT — Splat configuration

`baserom.us.yaml` drives **Splat**, which splits the decompressed ROM into
`src/`, `asm/`, and `assets/`.

## Header

```yaml
name: Banjo Tooie (North America)
sha1: af479771ff5e91ce3732f3d84146614ab71d8d79
options:
  basename: banjotooie
  target_path: decompressed.us.z64
  base_path: .
  compiler: IDO
  header_encoding: ASCII
  create_cfg_funcs_auto: False
  create_under_score_funcs_auto: False
  section_order: [".text", ".rodata", ".data", ".bss"]
```

- Compiler: `IDO`.
- Build path: `build/us`.

## Segments

Segments follow the ROM memory ordering:

```yaml
segments:
  - name: header      # 0x0
    type: header
  - name: ipl3        # 0x40
    type: bin
  - name: boot        # 0x1000, vram 0x80000400
    type: code
    subsegments:
      - [0x1000, hasm]
      - [0x1050, c, 1050]
      ...
      - [0x22F0, lib, libultra_rom_boot, initialize]
      ...
```

- boot subsegments `[offset, lib, libultra_rom_boot, funcname]` pull library
  functions from `libultra_rom_boot` to build the boot segment byte-exactly.
- `core1` and `core2` (vram ~0x80000000 family) make up the main game logic.
- Sizes for bss come from `# bss_size:` overriding Splat's boundary scan.

## Symbol / relocation inputs

```yaml
symbol_addrs_path: [ovl_symbol_addrs.us.txt, syscall_symbol_addrs.us.txt, symbol_addrs.us.txt]
reloc_addrs_path: [ovl_reloc_addrs.us.txt, reloc_addrs.us.txt]
```

These are exactly the maps a `N64Recomp` run needs. See [Symbols](symbols.md).

## Output

Splat generates:

- `src/` C files (nonmatching functions with `#pragma GLOBAL_ASM`),
- `asm/` assembly for unmatched segments,
- `assets/` binary blobs (and per-overlay `relocs.bin`).

Assembly is later linked into the ELF by the Makefile (see Build.md).