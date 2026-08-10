---
title: "BK — Native Patches"
status: current
area: "specs/BK"
updated: 2026-08-08
tags:
  - banjo-recomp
  - patches
---

# BK — Native Patches

BanjoRecomp changes game behavior by compiling **native C patches** into a
MIPS ELF, then recompiling that ELF with `N64Recomp` and registering the
result as overlays. This is the mechanism that powers features like
widescreen, note saving, and camera changes.

## Patch build flow

```text
patches/*.c (C, mips target)
  -> clang -> patches/*.o
  -> ld.lld -T patches.ld -T syms.ld --emit-relocs
  -> patches/patches.elf
  -> ./N64Recomp patches.toml
  -> RecompiledPatches/ + patches/patches.bin
  -> registered at runtime
```

`patches/Makefile` compiles the C sources. Flags are designed to mirror the
original IDO toolchain output as closely as possible:

- `-target mips -mips2 -mabi=32 -O2 -G0 -mno-abicalls`
- `-fomit-frame-pointer`, `-ffast-math`, `-fno-builtin-*`
- `-nostdinc` with includes from `lib/bk-decomp/include`, `lib/rt64/include`,
  and `N64ModernRuntime/ultramodern/include`

## The patch ELF

`patches/patches.elf` is the linker output. It keeps relocations because the
patch C code needs to reference symbols that only exist inside the recompiled
game.

`syms.ld` maps **dummy addresses** to every function the patches may call. The
recompiler turns those references into native function calls:

```ld
recomp_puts = 0x8F000000;
recomp_exit = 0x8F000004;
recomp_load_overlays = 0x8F000008;
osInvalDCache_recomp = 0x8F00000C;
...
memcpy_recomp = 0x8F000064;
__divdi3_recomp = 0x8F000068;
...
recomp_xxh3 = 0x8F00008C;
recomp_get_bgm_volume = 0x8F000090;
```

Dummy addresses start at `0x8F000000` to avoid collision with any real N64
address (`0x80000000`-`0x80800000` region).

## Patches folder

- `patches.h` — aggregate header
- `patches.ld` — linker script for the ELF
- `syms.ld` — dummy symbol addresses (see above)
- `address_patches.c`, `init_patches.c`, `memory_patches.c`,
  `overlay_loading.c`, `scheduler_patches.c`, `timing_patches.c` — low-level
  hooks
- `sound_patches.c`, `title_screen_patches.c`, `pause_menu_patches.c`,
  `camera_and_axis_inversion_patches.c` — specific feature hooks
- `jinjo_saving.c`, `note_saving.c` — feature implementations
- many `*_transform_tagging.c` files — render-time per-object transforms
  (projection, HUD, map, particle, sky, text, camera)
- `required_patches.c` — patches that must always be applied

## patches.toml

| Key | Value |
| -------- | ------- |
| `elf_path` | `patches/patches.elf` |
| `output_func_path` | `RecompiledPatches` |
| `single_file_output` | `true` |
| `use_absolute_symbols` | `true` |
| `func_reference_syms_file` | `BanjoRecompSyms/bk.us.rev0.syms.toml` |
| `data_reference_syms_files` | `[BanjoRecompSyms/bk.us.rev0.datasyms.toml]` |
| `output_binary_path` | `patches/patches.bin` |
| `unpaired_lo16_warnings` | `false` |
| `allow_exports` | `true` |
| `strict_patch_mode` | `true` |

`output_binary_path` writes a single binary blob (`patches.bin`) instead of
using `objcopy`, allowing `N64Recomp` to patch MIPS32 relocations during
output.

## Registration at runtime

See [Runtime](runtime.md):

- `register_patches.cpp` reads `bk_patches_bin`
- `recomp::overlays::register_patches(...)` wires it in
- `register_base_exports`, `register_base_events`,
  `register_manual_patch_symbols`

## Modding

Patches are how the official modding API surfaces features. See
[Features](features.md).