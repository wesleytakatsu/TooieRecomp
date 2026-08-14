---
title: "P08 — func_800BCF28 helper binding fix"
description: "Implementation and validation of Tooie-native recomp helper bindings for 0x800BCCC0 and 0x800BCCD8, replacing the erroneous lookup_gap_stub entries and resolving the func_800BCF28 SIGSEGV."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, bcf28, function-binding, fix, validation, boot-stubs]
---

# P08 — func_800BCF28 HELPER BINDING FIX

```text
BASE_HEAD:           870bd840c776c5b8a4e1025e7d9eff73defe8e8e
MODEL:               Gemini 3.7 Flash HIGH
MODE:                NARROW IMPLEMENTATION + VALIDATION
PATCH4_ACTIVE:       YES (default ON)
FIX_LAYER:           TOOIE_NATIVE (src/game/boot_hw_stubs.cpp)
OLD_SIGSEGV_STATUS:  RESOLVED
ROOT_CAUSE_LEVEL:    A
```

## 1. Fix layer and rationale

- **FIX_LAYER:** `TOOIE_NATIVE` (`src/game/boot_hw_stubs.cpp`).
- **WHY:** `0x800BCCC0` and `0x800BCCD8` are static inline / unnamed element accessors in `core2` without distinct symbols in the decompressed ELF (`banjotooie_decompressed.elf`). N64Recomp generates C calls to `LOOKUP_FUNC(0x800BCCC0)` and `LOOKUP_FUNC(0x800BCCD8)`. Following the established pattern for unexported core helpers in TooieRecomp (such as `0x800136E4`, `0x800138D0`, `0x80013970`), the smallest and structurally correct layer is to implement the exact native recomp bodies in `src/game/boot_hw_stubs.cpp` and register them into `func_map` via `recomp::overlays::add_loaded_function` during `register_boot_lookup_stubs()`.
- No hand-edits to `RecompiledFuncs/**`.
- No generator modifications needed.

## 2. Exact original helper semantics

Independently reconstructed from decompressed MIPS-III disassembly in `banjotooie_decompressed.elf`:

### `0x800BCCC0` (`__get_button_data_ptr(a0, a1)`)

```mips
800bccc0:  000570c0  sll   t6, a1, 0x3      ; t6 = a1 * 8
800bccc4:  01c57023  subu  t6, t6, a1       ; t6 = a1 * 7
800bccc8:  000e70c0  sll   t6, t6, 0x3      ; t6 = a1 * 56
800bcccc:  008e1021  addu  v0, a0, t6       ; v0 = a0 + a1 * 56
800bccd0:  03e00008  jr    ra
800bccd4:  24420024  addiu v0, v0, 36       ; v0 = a0 + a1 * 56 + 0x24 (36)
```

- **Semantics:** `v0 = a0 + a1 * 56 + 0x24`
- **Register updates:** `ctx->r2 = (gpr)(int32_t)(a0 + a1 * 56 + 0x24)`, `ctx->r14 = (gpr)(int32_t)(a1 * 56)`.

### `0x800BCCD8` (`__get_stick_data_ptr(a0)`)

```mips
800bccd8:  908e0004  lbu   t6, 4(a0)        ; t6 = byte at (a0 + 4)
800bccdc:  000e78c0  sll   t7, t6, 0x3      ; t7 = t6 * 8
800bcce0:  01ee7823  subu  t7, t7, t6       ; t7 = t6 * 7
800bcce4:  000f78c0  sll   t7, t7, 0x3      ; t7 = t6 * 56
800bcce8:  008f1021  addu  v0, a0, t7       ; v0 = a0 + t6 * 56
800bccec:  03e00008  jr    ra
800bccf0:  24420024  addiu v0, v0, 36       ; v0 = a0 + t6 * 56 + 0x24 (36)
```

- **Semantics:** `v0 = a0 + MEM_BU(4, a0) * 56 + 0x24`
- **Register updates:** `ctx->r2 = (gpr)(int32_t)(a0 + t6 * 56 + 0x24)`, `ctx->r14 = (gpr)t6`, `ctx->r15 = (gpr)(int32_t)(t6 * 56)`.

## 3. Removal of false gap classification

- `0x800BCCC0` and `0x800BCCD8` were removed from `kLookupGapAddrs` and `kLookupGapFuncs`.
- The remaining 5 true gap stubs (`0x80001998`, `0x80031178`, `0x80036670`, `0x80036728`, `0x800979D8`) remain intact and return `v0 = 0`.
- The 2 real helpers are registered directly via `recomp::overlays::add_loaded_function`.

## 4. Files changed

1. `src/game/boot_hw_stubs.cpp`:
   - Added native functions `tooie_helper_800BCCC0` and `tooie_helper_800BCCD8`.
   - Removed `0x800BCCC0` and `0x800BCCD8` from `kLookupGapAddrs` / `kLookupGapFuncs`.
   - Registered `tooie_helper_800BCCC0` and `tooie_helper_800BCCD8` in `register_boot_lookup_stubs()`.
2. `tests/bcf28_helpers_test.cpp`:
   - New unit regression test verifying function binding resolution, exact MIPS output across representative test vectors, and gap stub preservation.
3. `CMakeLists.txt`:
   - Registered target `bcf28_helpers_test`.

## 5. Regression test validation

Command:
```sh
cmake --build build --target bcf28_helpers_test -j$(nproc) && ./build/bcf28_helpers_test
```

Output:
```text
[bcf28_test] registering lookup stubs...
[tooie] registered 22 ultra LOOKUP hooks + 5 core helper hooks + 5 gap stubs
[bcf28_test] PASS: 0x800BCCC0 and 0x800BCCD8 are bound to dedicated functions != gap_stub
[bcf28_test] PASS: 0x800BCCC0 representative cases passed
[bcf28_test] PASS: 0x800BCCD8 representative cases passed
[bcf28_test] PASS: All 5 remaining gap stubs correctly return v0 = 0
[bcf28_test] ALL TESTS PASSED SUCCESSFULLY.
```
Exit code: `0`.

## 6. Primary acceptance validation

Command:
```sh
TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json \
./build/TooieRecompiled
```

Results:
- `C2AB8` crossed: **YES**
- `func_800BCF28` entered: **YES**
- `0x800BCCC0` returns valid guest pointer: **YES** (computes `0x801D262C` for `a0=0x801D2560`, `a1=3`)
- Old `func_800BCF28` SIGSEGV at `0x800BCF58`: **RESOLVED**
- Execution progresses past `bakey_heldCount`, `bainput_func_80097B4C`, `basetup_entrypoint_2`, `gsworldDll_entrypoint_2`: **YES**
- Overlays loaded and executed downstream: overlays 118, 90, 180, 778, 817, 849, 393, 813, 832, 818, 829, 831, 810, 293, 695, 731, 726 (ep 3: event 3 scheduler ack), 635, 674, 698, 748, 740, 697, 425, 372, 705, 843, 200, 203, 191, 505, 423, 462, 678, 730, 728, 421, 221, 814, 471, 677, 822.
- Stop point reached: Next blocker encountered (`Failed to find function at 0x00000000` via `func_80013A7C`). Stopped per contract (do not diagnose next blocker).
