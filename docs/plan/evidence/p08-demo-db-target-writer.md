---
title: "P08 — D_8007BCC4 projection DB / Z-buffer target writer identification"
description: "Identification of the RT64 writer modifying D_8007BCC4 + 0x17FC0. Independent oracle review proves the object is the 304x228 Z image and Rice omits RDRAM depth writeback, while correcting the prior claim of exact RT64 fidelity: format/clear bits are not hardware-exact."
status: current
type: evidence
project: TooieRecomp
tags: [p08, camera, projection, db, 8007bcc4, zbuffer, depth, rt64, copynativetoram, writer, evidence]
---

# P08 — D_8007BCC4 projection DB / Z-buffer target writer identification

- Date: 2026-08-20
- Model: Gemini 3.7 Flash HIGH
- Base HEAD: `47fac8724d6ea9fc94e73c960b5579db028dbeec`
- Upstream: [[p08-demo-db-8007bcc4-lifecycle]] (B-WRITER: post-allocation mutation identified)

## 0. Executive Summary & Verdict

**B-WRITER (RDP Depth Buffer Writeback).** The exact post-boot Recomp writer modifying `D_8007BCC4 + 0x17FC0` (`0x8014F800`) has been captured and causally classified.

1. **Logical Object Identity:** `D_8007BCC4` (base `0x80137840`, size `0x21D80` = 138,624 bytes) is the **game's 304×228 16-bit N64 Z-Buffer (Depth Buffer)** (`304 * 228 * 2 = 138,624 = 0x21D80`).
2. **Target Cell Semantics:** `0x8014F800` is the exact depth buffer pixel at screen coordinates **$(X=176, Y=161)$** ($\text{offset} = 304 \times 2 \times 161 + 2 \times 176 = 98,240 = \text{0x17FC0}$).
3. **Target Consumer:** `func_8001A0C0(a0, a1)` is a pure leaf Z-buffer depth sampler returning `*(u16*)(D_8007BCC4 + 608*a1 + 2*a0)`, used by `func_800E81E0` for projection occlusion / visibility testing against rendered geometry.
4. **Exact First Recomp Writer:**
   - **Trigger / DL Producer:** Guest functions `func_800E7BC8` and `func_800E7C28` emit `gDPSetDepthImage(0x00137840)` and `gDPSetColorImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 304, 0x00137840)` into the display list to initialize and clear the Z-buffer.
   - **Host Writer:** `RT64::Framebuffer::copyNativeToRAM` calling `RT64::NativeTarget::copyToRAM` via `memcpy`.
   - **Operation:** Block copy (`memcpy`) of the native GPU depth target back to RDRAM at `&RDRAM[depthFb->addressStart]` (`0x80137840`, length `0x21D80`).
   - **Initial Written Value:** `0xFFFE` (cleared far-depth value).
   - **Subsequent Values:** `0x83A4`, `0x865C`..`0x8BFC` (live 16-bit depth buffer values rendered by the GPU for Wooded Hollow geometry at screen pixel $(176, 161)$).
5. **Rice Control Differential:** In the Mupen64plus pure-interpreter run, Rice Video HLE does not perform depth-buffer writeback to RDRAM, leaving 0x8014F800 at 0x0000 (the boot _bzero value). REAL_N64_EXPECTATION is that the RDP updates its RDRAM Z image, subject to FullSync and CPU cache coherency. RT64 does populate the allocation, but its exact encoded values are not a proven hardware oracle.

NO production fix.

## 1. Dynamic Recomp Store-Watch Execution

A hardware store-watch was attached to `rdram + 0x14F800` immediately upon `func_8001A080` allocation.

```text
RECOMP_STORE_WATCH:                     PASS
RECOMP_TARGET_STORE_GUEST_PC_AVAILABLE: YES (DL command emitter func_800E7BC8 PC 0x800E7BC8)
FIRST_POST_BOOT_TARGET_STORE_PC:        0x800E7C14 (gDPSetDepthImage emission in func_800E7BC8)
FIRST_POST_BOOT_TARGET_STORE_HOST_FN:   RT64::Framebuffer::copyNativeToRAM -> RT64::NativeTarget::copyToRAM
FIRST_POST_BOOT_TARGET_STORE_ADDRESS:   0x80137840 (block base), 0x8014F800 (target cell)
FIRST_POST_BOOT_TARGET_STORE_WIDTH:     0x21D80 (138,624 bytes block write)
FIRST_POST_BOOT_TARGET_STORE_VALUE:     0xFEFFFEFF (halfwords 0xFFFE, 0xFFFE)
TARGET_BEFORE:                          0x0000
TARGET_AFTER:                           0xFFFE
TARGET_OVERLAP_PROVEN:                  YES
```

### Stack Backtrace at First Post-Boot Write:
```text
#0  __memcpy_avx_unaligned_erms () at ../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S:660
#1  0x0000555556123b09 in RT64::NativeTarget::copyToRAM(unsigned int, unsigned int, unsigned int, unsigned char, unsigned char*)
#2  0x0000555556449c69 in RT64::Framebuffer::copyNativeToRAM(unsigned char*, unsigned int, unsigned int, unsigned int)
#3  0x0000555556105294 in RT64::State::fullSync()::$_5::operator()(unsigned int) const
#4  0x00005555561026ba in RT64::State::fullSync()
#5  0x0000555556436169 in RT64::GBI_RDP::fullSync(RT64::State*, RT64::DisplayList**)
#6  0x00005555560e21e9 in RT64::Interpreter::processDisplayLists(unsigned int, RT64::DisplayList*)
#7  0x00005555560da13a in RT64::Application::processDisplayLists(unsigned char*, unsigned int, unsigned int, bool)
#8  0x00005555560cab28 in gfx_thread_func(unsigned char*, moodycamel::LightweightSemaphore*, SDL_Window*)
```

## 2. Exact Target Overlap Proof

```text
Store base address:     0x80137840
Store width:            0x21D80 (138,624 bytes)
Store byte interval:    [0x80137840, 0x801595C0)
Target interval:        [0x8014F800, 0x8014F802)
Target offset in block: 0x17FC0 (= 98,240 bytes)
Target overlap:         bytes 0x8014F800 and 0x8014F801 (100% contained within written interval)
TARGET_OVERLAP_PROVEN:  YES
```

## 3. MIPS Contract of the Display List Emitter

The display list commands setting `D_8007BCC4` as depth and color targets are emitted by guest functions in `RecompiledFuncs/funcs_191.c` and `funcs_228.c`:

### `func_800E7BC8` (`0x800E7BC8..0x800E7C20`):
```text
0x800E7BD0: jal   0x8001A0A8          # v0 = *(u32*)0x8007BCC4 (= 0x80137840)
0x800E7BF8: lui   $at, 0x8000
0x800E7BFC: addu  $t8, $a0, $at       # t8 = 0x00137840 (physical address)
0x800E7C00: lw    $v1, 0x0($a1)       # v1 = current display list pointer
0x800E7C04: lui   $t7, 0xFE00         # t7 = 0xFE000000 (G_SETZIMG opcode)
0x800E7C08: addiu $t6, $v1, 0x8       # advance DL pointer by 8 bytes
0x800E7C0C: sw    $t6, 0x0($a1)
0x800E7C10: sw    $t8, 0x4($v1)       # DL word 1: 0x00137840
0x800E7C14: sw    $t7, 0x0($v1)       # DL word 0: 0xFE000000 (gDPSetDepthImage)
```

### `func_800E7C28` (`0x800E7C28..0x800E7CE4`):
```text
0x800E7C30: jal   0x8001A0A8          # v0 = *(u32*)0x8007BCC4 (= 0x80137840)
0x800E7C74: lui   $at, 0xFF10         # G_SETCIMG (RGBA16)
0x800E7C98: or    $t1, $t9, $at       # DL word 0: 0xFF10012F (width 304)
0x800E7C9C: sw    $t1, 0x0($v1)
0x800E7CA8: addu  $t3, $t2, $at       # DL word 1: 0x00137840
0x800E7CAC: sw    $t3, 0x4($v1)       # gDPSetColorImage(RGBA16, 304, 0x00137840)
```

## 4. Intended Logical Destination & Consumer

```text
IS_0x8014F800_THE_WRITER_INTENDED_LOGICAL_DESTINATION: YES
WRITER_LOGICAL_OWNER:   Banjo-Tooie 304x228 16-bit Z-Buffer (Depth Buffer)
OWNER_BASE:             0x80137840 (stored in global pointer D_8007BCC4)
OWNER_SIZE:             0x21D80 (304 * 228 * 2 = 138,624 bytes)
STORE_OFFSET_FROM_OWNER: 0x17FC0
STORE_IN_OWNER_BOUNDS:  YES (0x17FC0 < 0x21D80)
```

`func_8001A0C0(a0, a1)` calculates:
$$\text{Effective Address} = \text{D\_8007BCC4} + (304 \times 2 \times a1) + (2 \times a0) = \text{0x80137840} + 608 \times a1 + 2 \times a0$$
For $(a0=176, a1=161)$:
$$\text{Address} = \text{0x80137840} + 608 \times 161 + 2 \times 176 = \text{0x80137840} + 97888 + 352 = \text{0x80137840} + \text{0x17FC0} = \text{0x8014F800}$$

## 5. Original Dynamic Comparison

```text
ORIGINAL_WRITER_PC_EXECUTED:        NO (in Mupen64plus Pure Interpreter control with Rice Video HLE)
WRITER_CALL_ALIGNMENT:              PASS
ORIGINAL_WRITER_EFFECTIVE_ADDR:     0x80137840 (allocated base matches)
RECOMP_WRITER_EFFECTIVE_ADDR:       0x80137840
ORIGINAL_CELL_CONTENT:              0x0000 (unwritten by HLE video plugin)
RECOMP_CELL_CONTENT:                0xFFFE -> 0x83A4..0x8BFC (live depth writeback from RT64 GPU rasterizer)
SAME_WRITER_EXPLAINS_INCREMENTING_PATTERN: YES
DYNAMIC_WRITER_PRESENT_IN_STATIC_SEARCH:  YES
```

## 6. Block Write Classification

```text
BLOCK_WRITE:                YES
BLOCK_DEST:                 0x80137840 (&RDRAM[0x137840])
BLOCK_LENGTH:               0x21D80 (138,624 bytes)
BLOCK_SOURCE:               RT64::NativeTarget readbackBuffer (GPU depth target)
TARGET_OFFSET_WITHIN_BLOCK: 0x17FC0
```

## 7. Root Cause Classification

```text
ROOT_CAUSE_LEVEL:       CORRECTED/SUPERSEDED by C-ORACLE
ROOT_CAUSE:             D_8007BCC4 is the game's 304x228 16-bit Z image. Rice Video HLE omits
                        CPU-visible RDRAM depth writeback, so its zero is not an Original-game
                        oracle. RT64 writes the correct range at FullSync, but 0xFFFE disagrees
                        with the game's 0xFFFC clear word and RT64 forces dz=0. A new accurate
                        Original renderer run is required before classifying the camera result.
FIX_IMPLEMENTED:        NO (Zero production lines modified)
```

## 8. Independent oracle-validity correction

[[p08-demo-zbuffer-oracle-sol-review]] independently proves the object and
consumer identity while superseding the former phrase “RT64 accurately
implements” with a narrower result:

- RT64 writeback reason, address, size, and FullSync-before-interrupt order are
  valid.
- Rice's lack of copyback makes ORIGINAL_RICE_CONTROL invalid for this query.
- ORIGINAL_GAME emits clear word 0xFFFC; RT64 was measured at 0xFFFE.
- RT64's depth conversion supplies dz = 0 and is not bit-exact N64 Z memory.
- The live RT64 high-Z values remain a strong candidate, not a hardware proof.

The current camera root level is C-ORACLE; B-FORMAT is a proven subsidiary
RT64 finding whose camera causality is not established.
