---
title: "BK — Recompiler Config (banjo.us.rev0.toml)"
status: current
area: "specs/BK"
updated: 2026-08-08
tags:
  - banjo-recomp
  - config
  - n64recomp
---

# BK — Recompiler Config

`banjo.us.rev0.toml` configures `N64Recomp` for the Banjo-Kazooie NTSC 1.0
ROM. All paths are relative to the TOML location.

## Section `[input]`

| Key                     | Value                                              |
| ----------------------- | -------------------------------------------------- |
| `entrypoint`            | `0x80000400`                                        |
| `output_func_path`      | `RecompiledFuncs`                                  |
| `rom_file_path`         | `banjo.us.v10.decompressed.z64`                     |
| `symbols_file_path`     | `BanjoRecompSyms/bk.us.rev0.syms.toml`             |
| `elf_path`              | `banjo.us.v10.elf` (commented out, used by patches) |
| `bss_section_suffix`    | `_bss`                                              |

## `manual_funcs`

Hand-written function addresses not discoverable from symbols. Used for the
two Leo (64DD) exception functions which are defined manually:

```toml
manual_funcs = [
    { name = "boot___osLeoAbnormalResume", section = ".boot_bk_boot", vram = 0x800037E4, size = 0xE8 },
    { name = "boot___osLeoResume", section = ".boot_bk_boot", vram = 0x800038CC, size = 0xEC },
]
```

## `mdebug_file_mappings`

Maps `.text` sections to a `.output_section` (e.g. `.TTC`, `.SM`) based on the
mdebug filename, for sections that had no proper symbols.

## `[patches]`

Controls what `N64Recomp` does while translating:

| Key              | Meaning                                                              |
| --------------------- | ------------------------------------------------------------------ |
| `ignored`        | functions excluded entirely (e.g. `boot_n_aspMainTextStart`, `boot_gSPF3DEX_fifoTextStart`) |
| `renamed`        | symbols renamed (e.g. `wmemcpy`)                                    |
| `stubs`          | functions not translated; calls to them become no-ops or runtime hooks (the entire `boot_*` libultra surface) |
| `instruction`    | one-instruction MIPS patches (see next section)                     |

### The `stubs` list

The `stubs` entry is large and essentially re-stubs the whole `boot` segment:

- `boot___ll_div`, `boot___ll_mod`, `boot___ll_mul`, `boot___ull_*`, etc.
- all `boot_os*` (DMA, interrupt, os-system, layout, etc.)
- `osWriteBackDCacheAll` (misspelled upstream)
- unnamed functions `func_8026A2E0`, `func_8026A824`

Stubs make the game build and let the runtime libraries handle those ops.

### Instruction patches

All of the instruction patches change one immediate. The comment lines in the
original file document the bug being fixed:

```toml
value = 0x3C0E8000   # address 0x8024C9A8 in func viewport_setRenderPerspectiveMatrix
                     # lui $rE, 0xA000 -> lui $rE, 0x8000
```

They fix a hardware address bug (0xA000... to 0x8000...) repeated in 6
functions:
`viewport_setRenderPerspectiveMatrix`, `anctrl_setDuration`,
`func_802D3DA4`, `func_8038F4C0`, `func_8038AC04`, `func_80388D48`.

## Controller input

- `recompcontrollerdb.txt` - gamepad data for `RecompFrontend` input.

## Related notes

- [Symbols](symbols.md) — where the symbol maps come from
- [Patches](patches.md) — same mechanics for the native patch ELF
- [Build](build.md) — full pipeline
- [Overview](overview.md)