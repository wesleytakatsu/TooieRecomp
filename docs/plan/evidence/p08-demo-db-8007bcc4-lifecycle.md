---
title: "P08 — D_8007BCC4 DB lifecycle / Q1 target cell differential"
description: "Bounded lifecycle differential of the D_8007BCC4 Z image and Q1 cell. Allocation/base/initial content match; the Rice control remains zero because Rice omits RDRAM depth copyback, while RT64 writes live depth. Hardware arbitration now requires a new accurate Original oracle."
status: current
type: evidence
project: TooieRecomp
tags: [p08, camera, projection, db, 8007bcc4, lifecycle, bwriter, differential, evidence]
---

# P08 — D_8007BCC4 DB lifecycle / Q1 target cell differential

- Date: 2026-08-20
- Model: DeepSeek V4 Flash HIGH
- Base HEAD: `76078c3dcff929609e828027339d5d59db655add`
- Upstream: [[p08-demo-1a0c0-state-differential]] (B-STATE at func_8001A0C0)
- Downstream: [[p08-demo-db-target-writer]] (B-WRITER: RT64 Framebuffer::copyNativeToRAM depth writeback)

## 0. Verdict

**B-WRITER.** The projection DB base `D_8007BCC4` and the Q1 target cell at
`D_8007BCC4 + 0x17FC0 = 0x8014F800` were traced from allocation through the
aligned Q1 classifier use in both runtimes. **Allocation, base pointer, effective
   address, and allocation-time cell content all match.** The Rice control boot-bzeros the
   region once and never materializes rendered depth there (store-watch: exactly one write,
the boot bzero, value 0). Recomp's cell is 0 right after allocation (ALLOC_AFTER
= 0x0000) but a **post-allocation writer** turns it to 0xFFFE (E82EC ord 2) and then
to growing/incrementing values (0x83A4..0x8BFC, ~+0x3C/update) by the Q1 window.
The first exact differential is the Q1 target cell content leaving 0 in RT64
while the Rice control never leaves 0. This does not establish a real-N64
differential. NO production fix.

## 0.1 Corrections to prior wording

- Prior [[p08-demo-1a0c0-state-differential]] open possibilities
  (misrelocated / mis-sourced / aliasing / uninitialized) are **REFUTED** by
  this runtime capture for the base/address/allocation-time content:
  **B-ALLOC and B-ALIAS dismissed** (base and effective address identical and
  stable), **B-INIT dismissed** (recomp cell is 0 right after allocation).
- Historical class: **B-WRITER** (a post-allocation RT64 writer modifies the
  cell; the Rice control has no corresponding RDRAM depth copyback).
- Current class: **C-ORACLE**, per
  [[p08-demo-zbuffer-oracle-sol-review]]. Rice zero is not a valid rendered-Z
  oracle and exact RT64 depth encoding is not hardware-faithful.
- Ad-hoc wording "impossible for a properly populated static table" in the prior
  doc is **CORRECTED/SUPERSEDED** here: this is a runtime-populated heap DB, so
  changing content is legitimate; the discriminating fact is the **absence of that
  writer in ORIGINAL_RICE_CONTROL**, not mere variation.
- The measured Rice-0 / RT64-incrementing evidence is preserved.

## 1. Environment

```text
MUPEN_ENV_REUSED:   YES  (/tmp/p08-m64p; core 6dca4c1, rice 038882d)
MUPEN_DIAGNOSTIC_CORE: rebuilt with store-watch + func_8001A0C0 entry capture; reverted
MUPEN_FINAL_CORE:     rebuilt from reverted source with NO_ASM=1 OSD=0 VULKAN=0; installed to /tmp/p08-m64p/inst/lib
ROM_MATCH:          YES  (SHA-1 af1a89e12b638b8d82cc4c085c8e01d4cba03fb3)
RECOMP_BUILD:       temporary p08_db_trace.cpp + -Wl,--wrap=func_8001A0C0,
                    func_800E82EC, func_8001A080; rebuilt; reverted; binary clean
```
Original captures: `/tmp/p08-a0c0-original.csv`, `/tmp/p08-orig-dbwatch.csv`.
Recomp capture: `/tmp/p08-db-trace-recomp.csv`.

## 2. DB allocation chain (original MIPS; matches generated)

```text
DB_ALLOC_CALLER:          boot init (0x8001230C)
DB_ALLOC_FUNCTION:        func_8001A080 -> func_8001A018(0x21D80)
                          -> heap_alloc(0x21D80 + 0x40) -> 64-byte align
DB_ALLOC_SIZE:            0x21D80 (138,624 bytes)
D_8007BCC4_STORE_PC:      0x8001A09C  (within func_8001A080)
D_8007BCC4_STORE_FUNCTION: func_8001A080
```

## 3. Pointer establishment & stability

```text
ORIGINAL_DB_BASE:          0x80137840 (constant; all 388 1A0C0 calls)
RECOMP_DB_BASE:            0x80137840 (constant; all 6607 1A0C0 calls)
DB_BASE_MATCH:             YES
DB_ALLOCATION_SIZE_MATCH:  YES (both 0x21D80)
DB_SEMANTIC_OBJECT_MATCH:  YES
DB_POINTER_WRITER_SET_MATCH: YES (single writer 0x8001A09C in both)
ORIGINAL_DB_POINTER_STABLE: YES
RECOMP_DB_POINTER_STABLE:   YES
```

## 4. Target cell

```text
TARGET_OFFSET:        0x17FC0 (= 608*161 + 2*176)
ORIGINAL_TARGET_ADDR: 0x8014F800
RECOMP_TARGET_ADDR:   0x8014F800 (identical)
TARGET_IN_DB_BOUNDS:  YES (0x8014F800 < 0x80137840 + 0x21D80 = 0x801595C0)
```

## 5. Allocation-time / zeroing contract

Original: the region containing the DB base is **explicitly bzero'd once at boot**
by `_bzero(0x80137800, 0x2C8800)` (store PC 0x8002D71C, value 0). The whole DB
region lies inside that oversized boot bzero, so the target cell is explicitly
initialized to 0.

```text
FULL_DB_ZEROED_EXPLICITLY:   YES (boot bzero covers the full 0x21D80 allocation)
FULL_DB_INITIALIZED_EXPLICITLY: YES (explicit zero initialization; DB-specific
                               population beyond zeroing is not fully traced)
TARGET_0x17FC0_EXPLICITLY_INITIALIZED: YES (boot bzero writes 0)
ORIGINAL_TARGET_AFTER_ALLOC: 0x0000
RECOMP_TARGET_AFTER_ALLOC:   0x0000  (ALLOC_AFTER row; MATCH)
ALLOCATION_INITIAL_BYTES_MATCH: YES
TARGET_INITIALIZER_SOURCE_MATCH: NOT_APPLICABLE (shared boot bzero; no separate
                                      target copy source was identified)
```

## 6. Stable pointer + changing memory (recomp)

```text
RECOMP_EFFECTIVE_TARGET_ADDRESS_STABLE: YES (0x8014F800 across all reads)
RECOMP_TARGET_CELL_CONTENT_CHANGES:     YES
```

At a **fixed** guest address 0x8014F800, the RT64 cell content changes with
E82EC ordinal while the Rice-control cell is constant 0. Mechanism = stable
pointer + renderer copyback difference, not a moving base/address.

## 7. Writer onset (first exact divergence)

Recomp E82EC_IN cell reads at 0x8014F800 (base 0x80137840):

```text
ord  1: 0x0000     (matches Rice control 0)
ord  2: 0xFFFE     <- first nonzero (post-allocation writer)
ord  8: 0xFFFE
ord   1065: 0x83A4
ord   1070: 0x8608
ord   1071: 0x865C  (Q1 first aligned sample)
ord   1076: 0x8788
ord   1094: 0x8BFC  (Q1 last aligned sample)
ord   1882: 0x7CBC
```

Rice-control E82EC / A0C0 reads at 0x8014F800: **0x0000 constant** throughout; the only
write to that address is the boot bzero (store-watch = 1 row: `1,0x8002D71C,
0x8014F800,0x00000000,4`).

```text
FIRST_DB_CELL_STATE_DIVERGENCE_BOUNDARY: between allocation (both 0) and E82EC ord 2,
            where RT64 leaves 0 while the Rice control remains 0
ORIGINAL_RICE_CONTROL_TARGET_WRITES: one observed overlapping write, boot bzero at PC
            0x8002D71C, effective address 0x8014F800, width 4, value 0.
RECOMP_TARGET_WRITES: exact instruction-level store set NOT_CAPTURED; the bounded
            snapshot trace proves a post-allocation mutation, and the exact store
            owner/PC is the next single B-WRITER action.
TARGET_WRITE_SET_MATCH: NO (Original has one observed boot write; Recomp has a
            post-allocation mutation whose exact store identity remains pending)
TARGET_CHANGED_WITHOUT_GUEST_STORE: UNKNOWN (state change observed; the recomp
            capture did not install an instruction-level target store-watch)
```

## 8. Classification

```text
REFUTED: B-ALLOC (base identical), B-ALIAS (effective addr identical),
         B-INIT (cell identical 0 at allocation)
ROOT_CAUSE_LEVEL: CORRECTED/SUPERSEDED by C-ORACLE
ROOT_CAUSE: same logical Z-image object and same boot-zeroed region; Rice never
            performs rendered-depth copyback. RT64 writes 0xFFFE then live values,
            but exact N64 format is refuted and a new Original oracle is required.
            func_8001A0C0(176,161) returns. First exact divergence = cell leaving 0.
FIX_IMPLEMENTED: NO
NEXT_SINGLE_ACTION: writer identified (see [[p08-demo-db-target-writer]]).
```

## 9. Q0 control & multi-sample

```text
Q0_1A0C0_CONTROL: NOT_APPLICABLE (no clean comparator argument pair)
MECHANISM_REPRODUCES_Q1_STREAM: YES (cell content changes each aligned sample in
            recomp; original 0 constant)
```

## 10. Method / provenance

- Original: extended the existing /tmp/p08-m64p pure-interpreter hook with a
  func_8001A0C0 entry capture (base/addr/cell) and a store-watch in
  r4300_write_aligned_word/dword on [0x8014F800,+1]; rebuilt core, updated
  inst/lib, ran --emumode 0 (Pure Interpreter, NO_ASM=1) to the Q1 frame.
- Recomp: temporary src/game/p08_db_trace.cpp wrapped func_8001A0C0 / func_800E82EC
  / func_8001A080 via -Wl,--wrap, capturing base (r14), addr (r9), v0 (r2) post-call
  plus ALLOC_AFTER / E82EC_IN / E82EC_OUT cell reads; ran with TOOIE_AUTO_START=1.
  Host rdram offset for KSEG0 guest G = rdram + (G & 0x1FFFFFFF).
- All temporary build changes reverted; TooieRecompiled rebuilt clean; git clean.

## 11. Housekeeping

- Temporary instrumentation fully reverted (CMakeLists, p08_db_trace.cpp, recomp
  binary, mupen core hook). Final Mupen core was rebuilt from the reverted source
  with `NO_ASM=1 OSD=0 VULKAN=0` and installed to `inst/lib` outside this repo.
- `markdown_vault_audit`: **UNAVAILABLE** in this session (MCP tool not exposed).
  Repository-local replacement checks: `git diff --check` PASS; changed-document
  wikilinks resolve to existing files PASS.
- Worktree remains docs-only; no production fix and no generated C edited.

## 12. Validation

- `cmake --build build --target TooieRecompiled -j2`: exit 0; Ninja reported no
  work to do.
- `window_display_test`: 38/38 PASS; `graphics_failure_test`: 25/25 PASS.
- `stick_angle_thunk_test`, `matrix_submission_test`, `input_direction_test`
  (failures=0), `bcf28_helpers_test`, `core1_ra_wrappers_test`: PASS.
- `python3 tests/attract_demo2_transition_jal_test.py`: exit 0, OVERALL PASS.
- Final original core: `make -C /tmp/p08-m64p/mupen64plus-core/projects/unix
  clean NO_ASM=1 OSD=0 VULKAN=0` followed by `all -j2 NO_ASM=1 OSD=0
  VULKAN=0`: exit 0; final installed core SHA-1
  `1086cf749e521ce759ab91be2eecd3fe8c59f632`; temporary-hook strings absent.
- `git diff --check`: PASS; repository-local wikilink check: 270 checked, PASS.

## 13. Oracle-validity correction

The store-watch remains valid evidence about **ORIGINAL_RICE_CONTROL**, not
REAL_N64_EXPECTATION. The independent review
[[p08-demo-zbuffer-oracle-sol-review]] establishes:

- D_8007BCC4 is proven to be the game's RDP Z image.
- Real RDP depth writes target RDRAM and can become CPU-readable after required
  synchronization/cache conditions.
- Rice has no CPU-visible Z writeback path.
- RT64's target ownership and writeback order are valid, but 0xFFFE does not
  match the game's emitted 0xFFFC clear word and RT64 does not preserve dz.

All unqualified “Original never writes” wording in this historical lifecycle
must be read as “the Rice-based control never writes/copies back rendered Z.”
