---
title: "BT — Tooling"
status: current
area: "specs/BT"
updated: 2026-08-08
tags:
  - banjo-tooie-Recomp
  - tooling
---

# BT — Tooling

C++ and Python tools under `banjo-tooie-Recomp/tools/`, built by `tools/Makefile`
and used by the main Makefile.

## C++ utilities

### `rom_decompressor`

Decompresses the cartridge ROM into a plain running image:

```sh
tools/rom_decompressor baserom.us.z64 decompressed.us.z64
```

Uses Rare's compression format.

### `rom_compressor`

Rebuilds the compressed ROM from the built ELF, preserving the original
compression and layout so the ROM matches byte-for-byte.

```make
tools/rom_compressor $(UNCOMPRESSED_ROM) $(ELF) $(ROM) $(BASEROM) $(DECOMPRESSED_BASEROM)
```

Requires the iterated original compressed baserom.

### `overlay_processor`

Main overlay metadata generator. Reads the prelim ELF + `overlays.us.toml`,
and for each overlay writes:

- `overlay_table.s`
- `assets/overlays/<name>/<name>_header.s`

Also validates section sizes are merged and divisible by 0x10, and replaces
generated relocs with the original order (from `assets/overlays/<name>/relocs.bin`)
to preserve matching.

### `syscall_builder`

Reads `overlays.us.toml`, emits `overlay_syscalls.s` — the runtime syscall
table. Has a hardcoded list of entrypoints where the trampoline is `xori`
instead of `addi`.

### `n64crc`

Rewrites the N64 header CRC after ROM rebuild:

```sh
tools/n64crc build
```

## Python scripts

- `list_overlays.py` — prints names of non-empty overlays from the TOML.
- `diff.py`, `first_diff.py`, `diff_settings.py` — diff/compare tools for
  matching verification.
- `m2ctx.py` — extracts a translation unit context from the build dir
  (fiddler for `m2ctx` / IDO decompilation workflows).
- `restructure.py`, `progress.py`, `upload_frogress.py` — decompals
  integration (progress metrics).
- `asm-processor/` — Splat's compile-time assembly interleaver.

## Toolchain note

`tools/ido/cc` is downloaded by `make -C tools` (IDO 5.3 recomp from
decompals/ido-static-recomp). `fmtlib`, `toml11`, `ELFIO`, `rarezip`,
`splat` are git submodules.

## RSP

The BT tree does **not** recompile the RSP microcode (that's a BK/recomp
concern). Audio ucode (`n_aspMain`) still lives in the core2 ROM and would be
handled by `RSPRecomp` for a recomp port.

See [RSP (BK)](../BK/rsp.md), [Build](build.md), [Overlays](overlays.md).