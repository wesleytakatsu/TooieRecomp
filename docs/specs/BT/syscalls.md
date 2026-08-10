---
title: "BT — Overlay Syscall Dispatch"
status: current
area: "specs/BT"
updated: 2026-08-09
tags:
  - banjo-tooie-Recomp
  - syscalls
  - overlays
  - TooieRecomp
---

# BT — Overlay Syscall Dispatch

Tooie calls overlay functions through a **syscall table**: each entry is a
code stub `syscall <overlay>` + an immediate holding the entrypoint index.
The runtime (core2) intercepts the syscall, loads/reads the right overlay and
jumps to the entrypoint. This is the recomp-critical bridge between core2 and
the 850+ dynamic overlays.

## `tools/syscall_builder` (decomp)

Generates `overlay_syscalls.s` from `overlays.us.toml` (banjo-tooie-Recomp):

```asm
dlabel overlay_syscalls
    syscall 0
    addi $t0, $zero, 0x0

glabel _gsmaker_entrypoint_0
    syscall 2
    addi $t0, $zero, 0x0
glabel _gsmaker_entrypoint_1
    syscall 2
    addi $t0, $zero, 0x4
...
```

### Encoding (NTSC-U, verified on `decompressed.us.z64`)

| Field | Encoding | Meaning |
| --- | --- | --- |
| Syscall code | MIPS `SPECIAL`/`SYSCALL`, bits `[25:6]` | **Overlay index** from `overlays.us.toml` (game slot; gsmaker=`2`, bainvisible=`164`) |
| Next instruction | `addi $t0,$zero,imm` **or** `xori $t0,$zero,imm` | `imm = entry_index * 4` |
| Entry index | `imm >> 2` | Index into that overlay’s `entrypoints` list — **not** the function’s `.text` offset |

The older note “syscall id = overlay_index + 1” is **wrong** for US 1.0 ROM
samples checked against the decomp symbols.

- **xori quirk:** `tools/syscall_builder.cpp` keeps `xori_entrypoints` (e.g.
  `bainvisible_entrypoint_3`) because those stubs encode `xori` instead of `addi`.
- Table lives in **core2**; exported labels are `_<ovl>_entrypoint_<n>`
  (`syscall_symbol_addrs.us.txt` / Tooie `metadata/syscalls.table.toml`).

### Core2 placement

| | Value |
| --- | --- |
| Core2 VRAM base | `0x800815C0` |
| Core2 ROM base (decompressed image) | `0x01E5AEB0` |
| Stub → ROM | `rom_off = 0x01E5AEB0 + (vram - 0x800815C0)` |

## How core2 calls an overlay (original game)

Core2 calls `_bapackctrl_entrypoint_1()` etc. Labels resolve to syscall stubs.
At runtime the exception/syscall path:

1. reads syscall code → overlay index;
2. reads `$t0` → entry index;
3. ensures the overlay is loaded (heap, relocs, BSS, optional inflate/XOR);
4. jumps to entrypoint in overlay VRAM (`0x80800000` + … when banked there).

## TooieRecomp — static recomp path

### What N64Recomp emits

Each stub becomes a native function that **only** calls the handler and returns:

```c
RECOMP_FUNC void _bainvisible_entrypoint_3(uint8_t* rdram, recomp_context* ctx) {
    recomp_syscall_handler(rdram, ctx, 0x800854C0);
    return;
    // dead code: xori $t0 never runs
}
```

So `recomp_syscall_handler` must decode both instruction words from the ROM.

### Dispatch implementation (P06)

| Piece | Location |
| --- | --- |
| Handler | `TooieRecomp/src/game/syscall_handler.cpp` |
| Generated table | `TooieRecomp/src/game/generated/overlay_dispatch.gen.cpp` |
| Generator | `TooieRecomp/tools/generate_overlay_dispatch.py` |
| Decode tests | `TooieRecomp/tools/test_syscall_decode.py` |

Algorithm:

1. Read big-endian words at core2 ROM offset for `instruction_vram`.
2. Decode overlay index + entry index; set `ctx->r8 = entry_index * 4`.
3. Look up `g_tooie_overlay_dispatch[overlay_index]` (built from
   `overlays.table.toml` + `recomp_overlays.inl` section rom/size, **not**
   N64Recomp’s alphabetical `overlay_sections_by_index`).
4. If overlay changed: unload bank at `0x80800000`, `do_rom_read` image into
   RDRAM, `load_overlays` for native func_map + reloc bases.
5. Call native `entrypoints[entry_index]` (names **without** leading `_`).

Native overlay functions still use `LOOKUP_FUNC(0x80800xxx)` for intra-overlay
calls; those addresses must be registered by `load_overlays`.

### Boot bridges (why they exist)

Runtime ROM is the **decompressed** recomp image. Original boot DMA+inflates
**compressed** banks. Host overrides in
`TooieRecomp/src/game/boot_hw_stubs.cpp` skip inflate for fixed banks
(`func_80000450` → core1, `func_80019EC0` → core2) and implement
`rom_read_word` / `rom_dma_read` via `recomp::do_rom_*`.

See plan stage [[../../plans/BT/06-overlays-and-syscalls]].

## Related

- [Overlays](overlays.md) — VRAM/header/compressor
- [Symbols](symbols.md) — `syscall_symbol_addrs.us.txt`
- Evidence: `docs/plan/evidence/p06-*.log` / `p06-build-summary.txt`
