---
title: "Guest heap system and memory layout"
description: "Native and expanded guest heap layout, allocation free-list structure, pointer validation, and heap exhaustion mechanisms."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-11-09
tags: [reverse-engineering, heap, memory, overlays]
---

# Guest heap system and memory layout

## Native heap layout (`PROVEN` static code)

The native *Banjo-Tooie* guest heap is initialized during early boot in core1 (`decomp/src/core1/heap.c`, `RecompiledFuncs` `funcs_164.c`):

- `func_8001E7E8` initializes the heap boundaries:
  - `D_8007E998 = func_8001E7E0()`: Heap end address, hardcoded constant `0x80400000`.
  - `D_8007E990 = D_8007E994 = end - func_8001E830()`: heap base; `func_8001E830` returns the constant size `0x2C8800` (2,918,400 bytes, ~2.78 MiB).
- The native heap spans `0x80137800..0x80400000`, residing strictly inside the first 4 MiB of RDRAM.
- `heap_setup` (`funcs_164.c`) builds the initial free block list from `D_8007E990` (heap base) and `D_8007BF00 = func_8001E830()` (heap size).
- `D_8003F750` tracks total bytes currently allocated / in use.
- `func_800121D0` (`funcs_351.c`) combines an offset returned by `func_80012194` with the heap size.

## Free-list structure and traversal (`PROVEN` static code)

The free-list layout traversed by `heap_largest_free` and the runtime low-memory guard operates as follows:

- The first block header resides at the heap base (`D_8007E990`).
- The pointer to the next free block is stored at offset `+0x14` within each block structure.
- The block size is calculated as `header.next (+0x04) - block`.
- The final dummy sentinel block is located at `base + size - 0x20`.

## Pointer validation and audited constants

- **`gztrace` pointer validator (`PROVEN` static code)**:
  `func_80800000_gztrace` (`funcs_278.c:2367`) validates guest pointers as 4-byte aligned and within the range `0x80000000..0x80800000` inclusive. It unconditionally rejects any pointer above 8 MiB.
- **Audited `0x8040` and `0x8080` constants (`PROVEN` static code)**:
  Occurrences in generated code (e.g. `funcs_173.c:1483`, `funcs_192.c:5570/5593`) represent integer bitflags, status masks and scalar flags, rather than pointer address boundary checks.
- **Audited `0x7FFFFF` masks (`PROVEN` static code)**:
  Bitmask operations using `0x7FFFFF` (found in `funcs_144`, `funcs_150`, `funcs_153`, `funcs_193`, `funcs_197`, `funcs_363`) are scalar bitfield manipulations, trigonometric indices, or floating-point mantissa extractions, and do not represent 8 MiB RDRAM truncation masks.

## Witchyworld heap exhaustion mechanism

During testing with enhanced draw distances (2026-11-08/09):

- **Observed defect (`USER_REPORTED`)**:
  Setting Graphics > Object Distance to `Unlimited` caused severe guest heap exhaustion in Witchyworld on Windows: crashes during overlay reclamation, loading freezes of ~5 seconds, missing transition Jiggy, and an enemy spawning without body geometry or collision (character model `0x6B1` failed to allocate). With Object Distance set to `Original`, room entry and gameplay proceeded normally.
- **Root cause mechanism (`STRONG_INFERENCE`)**:
  The `Unlimited` setting forces squared distance to `0` at the retention gate `func_80100230` (disabling resource eviction) and sets scheduling radius to `100000000` at `func_8010038C`. Consequently, every actor's dynamic overlay and model payload across the entire level remains resident concurrently. In large maps such as Witchyworld, actor memory requirements exceed the ~2.78 MiB native heap capacity. Earlier dispatcher-side workarounds attempting to handle overlay reclamation reentrancy were reverted.

## Expanded memory (project integration)

Project-side relocation, not original game behaviour. Windows: Witchyworld with Unlimited and story co-op user-validated (`USER_REPORTED`, 2026-11-09); Linux and handheld `NOT_RUN`.

- **Relocation**: Function hooks on `0x8001E7E0` (heap end) and `0x8001E830` (heap size) place the guest heap at `0x8080C000..0x81000000` (`0x7F4000` bytes, ~7.95 MiB, ~2.9x native). The native region `0x80137800..0x80400000` is left unused by the heap.
- **Address space layout**:
  - `0x80000000..0x80800000`: Base 8 MiB RDRAM.
  - `0x80800000..0x80801000`: PI handles.
  - `0x80801000..`: `patch_rdram_start`.
  - `0x80800000..0x8080B5E0`: static overlay link VRAM (canonical mappings registered by the runtime; high-water mark `0x8080B5E0`).
  - `0x8080C000..0x81000000`: Expanded guest heap (first page after static overlays, within 16 MiB physical).
  - `0x81000000..`: Mod space.
- **Latch safety rule**: Expansion is latched once per process at config initialization (`tooie_memory::finish_config`). Expansion is refused (falling back to native heap) if static overlays or patch RDRAM reach or exceed `0x8080C000`.
- **`osMemSize`**: Continues to report 8 MiB (`0x00800000`) so guest software sizing expectations remain satisfied.
- **`gztrace` wrapper**: Linker-wrapped `func_80800000_gztrace` accepts both base RDRAM and the expanded range `0x8080C000..0x81000000`.
- **Low-memory guard**: When Object Distance is not `Original`, extended retention and scheduling temporarily revert to native distances unless the heap has at least 768 KiB total free and a contiguous block of at least 256 KiB.
- **Hardware & RT64 limits**: RDP/RT64 and RSP DMA operate with 24-bit physical addresses (16 MiB). RT64 depth readback mask (`RDRAMSize`) is widened to 16 MiB when expanded memory is active.

See [[specs/Tooie/runtime-and-overlays#3.4 Guest heap layout and expansion]] and [[specs/Tooie/graphics-and-presentation#3.1.1 Object distance]].
