---
title: "BT — ROM Compression & Recompression"
status: current
area: "specs/BT"
updated: 2026-08-08
tags:
  - banjo-tooie-Recomp
  - rom
  - compression
---

# BT — ROM Compression & Recompression

Banjo-Tooie ships a **compressed cartridge**: boot is flat, but `core1`,
`core2` and every overlay are deflated. The decomp repo must read the
compressed baserom (decompress) and later rebuild the compressed ROM from the
built ELF byte-for-byte (recompress).

Formats and offsets used by `tools/rom_decompressor` /
`tools/rom_compressor` (address constants from `tools/tooie_utils.h`,
NTSC-U v1.0).

## Layout & offsets (`RomOffsets us_v10`)

```
core1_start          = 0x1E29B60
core1_text_end       = 0x1E3F718
core1_end            = 0x1E42550
core2_start          = 0x1E42550
core2_text_end       = 0x1E86C76
core2_end            = 0x1E899B0
overlay_table_offset = 0x1E899B0
```

`core1`/`core2` are each stored as **two concatenated streams** (text, then
`rodata+data`): a big-endian 16-bit `decompressed_size >> 4` marker followed
by raw **deflate** data (read via `inflateInit2(., -MAX_WBITS)`), so every
unit descends to `size * 16` decompressed bytes. `rom_decompressor` inflates
and concatenates both halves to build `decompressed.us.z64` (boot verbatim);
`rom_compressor` re-emits them with `bt_zip`.

## Overlay table

At `overlay_table_offset`: first u32 = total table size `(overlay_count + 1)
* 4` (big-endian); then `overlay_count` u32 entries that are **offsets
relative to the table start** bounding each overlay's data (entry `0` after
the size word = start of overlay 1 data region, entry `N` = end of the last
overlay). 1-indexed in the tools: overlay `i` occupies
`[table_entry[i-1], table_entry[i]]`. Byte-exact recompression keeps the
table identical.

## Overlay header (`ovl_header_offsets`)

```
0x0 u16 text_size    (in 16 units)   0x8 u16 entrypoint_count
0x2 u16 rodata_size                  0xA u16 reloc_count
0x4 u16 data_size                    0xC u16 flags
0x6 u16 bss_size                     0xE u16 name_length
             0x10 u16 decompressed_size (>>4)
```

`flags & 0x80` (`OverlayFlags::Compressed`) marks a deflated overlay. When
set, the serialized form is: `[0x10 header][u16 decompressed_size>>4 at
0x10][deflate payload from 0x12]`. The decompressor reads the size at
`ovl_header_offsets::decompressed_size` and inflates the payload (`header_size
= 0x12`). Uncompressed overlays only keep the `0x10` header and lean on the
contents directly.

Entrypoints live after the header; `get_overlay_reloc_ptr` walks past
entrypoint table + rounded name to the reloc array. When read, section sizes
are multiplied by 16; code starts 16-aligned after relocs.

## The xors scaffold

BT xors header words and relocs so a corrupted overlay fails loudly. When a
compressed overlay is built, `apply_xors` does:

1. `bk_crc` over the decompressed overlay content → `crc1`, `crc2`,
2. `header+0 ^= byteswap32(crc1)`, `header+8 ^= byteswap32(crc2)`,
3. each reloc u16 `^= rom_xor_swapped`, where
   `rom_xor = byteswap32(rom_start + 0x40 + overlay_index * 4) & 0xFFFF`
   (a per-overlay key near boot),
4. strips the VRAM base addend (`0x80800000`) out of each MIPS_32/26/HI16
   reloc word.

Decompressor reverses all of it in `undo_xors`. `bk_crc` is a simple additive
checksum.

## Compression (`rom_compressor`)

Rebuilds the running compressed ROM from the ELF:

1. copy boot verbatim up to `core1_start`,
2. `compress_and_write` (via `bt_zip` from `librarezip`) core1/core2 as the
   two-stream + marker layout, aligning to 2/16,
3. overlay loop: if the overlay bytes equal the decompressed baserom contents
   at the same spot, **copy the original compressed bytes verbatim** from the
   baserom (keeps compression byte-exact for unchanged overlays); otherwise
   set `flags |= 0x80`, `apply_xors(i+1, ...)`, write 0x10 header + deflate
   of the remaining content (`compress_and_write`), align to 4,
4. re-emit overlay table then patch boot:
   - `symbol_patch_mapping {core1_compressed_ROM_START/END, core2_compressed_ROM_START/END}` — `R_MIPS_32` subtract `old.+addend`, add `new`; `HI16/LO16` immediates rewritten (addends unsupported),
   - patch `table_addr_ROM_START` symbol → new overlay table address,
   - `patch_crc_values`: recompute the 8-entry `crc_entries` / `build_data` array core1 checksums at boot.

`make check` requires `build/us/banjotooie_decompressed.z64` == `decompressed.us.z64`; `make` output `banjotooie.z64` must byte-match the baserom.

## Why recomp cares

For a static recomp you don't need the compressed ROM — you need the
decompressed byte image for `N64Recomp`, plus overlay reloc/VRAM bookkeeping.
The xors/CRC dance matters if Tooie loads modules from the ROM at runtime,
and the `core*_compressed_ROM_*` symbol backpatching is exactly what a recomp
must emulate.

See [Build](build.md), [Overlays](overlays.md), [Symbols](symbols.md),
[Syscalls](syscalls.md).