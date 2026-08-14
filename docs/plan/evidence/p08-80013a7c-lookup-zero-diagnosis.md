---
title: "P08 — func_80013A7C lookup-zero diagnosis"
description: "Proven root cause (Level A): func_80013A7C saves caller return address in a3 (move a3, ra), calls func_80013ABC, and returns via jr a3. N64Recomp host C direct calls do not materialize return addresses in ctx->r31, so ctx->r7 copies zero, causing LOOKUP_FUNC(0x00000000). Fix implemented: see p08-core1-ra-wrapper-fix."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, 80013a7c, lookup-zero, return-trampoline, diagnosis]
---

# P08 — func_80013A7C LOOKUP-ZERO DIAGNOSIS

```text
BASE_HEAD:           bbdc304ba717b0c30372365343270e7ccae54ba1
MODEL:               Gemini 3.7 Flash HIGH
MODE:                DIAGNOSIS ONLY (no fix authored)
BCF28_FIX_ACCEPTED:  YES
CRASH_REPRODUCED:    YES
CRASH_DETERMINISTIC: YES
ROOT_CAUSE_LEVEL:    A — ROOT CAUSE PROVEN
FIX_IMPLEMENTED:     YES — [[p08-core1-ra-wrapper-fix]]
```

Resolution (2026-08-14): Tooie-native strong-symbol overrides for
`func_800137D4` / `func_80013A5C` / `func_80013A7C` landed in
`src/game/boot_hw_stubs.cpp`. Old `LOOKUP_FUNC(0)` from `func_80013A7C`
is **RESOLVED**. See [[p08-core1-ra-wrapper-fix]].

## 1. Executive summary

Execution progresses past the previously resolved `C2AB8` and `BCF28` milestones into `func_800EA170` and downstream world setup overlays. The first divergence is a fatal lookup failure inside `func_80013A7C`:
```text
Failed to find function at 0x00000000
```

The root cause is proven (Level A): `func_80013A7C` is a hand-optimized assembly math wrapper in `core1` that saves its caller's `$ra` into temporary register `$a3` (`move a3, ra`), calls helper `func_80013ABC` via `jal 80013abc`, and returns to its caller via `jr a3`.

Because N64Recomp translates normal MIPS JAL calls into host C function calls without updating `ctx->r31`, `ctx->r31` is `0` upon entry to `func_80013A7C`. The generated code copies `ctx->r31` into `ctx->r7` (`$a3`), calls `func_80013ABC`, and attempts an indirect jump to `ctx->r7` via `LOOKUP_FUNC(ctx->r7)`. Since `ctx->r7 == 0`, this evaluates `LOOKUP_FUNC(0x00000000)` and aborts.

This is the exact same architectural pattern as the existing `func_800137F4` precedent in `src/game/boot_hw_stubs.cpp`.

## 2. Reproduction and dynamic confirmation

### 2.1 Environment and command
```sh
TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json \
timeout -k 2s 120s gdb -batch -q \
  -ex "b *func_80013A7C+28" \
  -ex "b *func_80013A7C+99" \
  -ex "run" \
  -ex "x/gx \$rsi+0xf8" \
  -ex "c" \
  -ex "x/gx \$rbx+0x38" \
  -ex "c" \
  ./build/TooieRecompiled
```

### 2.2 Results
1. `C2AB8` crossed: **YES**
2. `BCF28` entered and executed cleanly: **YES**
3. `func_80013A7C` entered from caller `func_800963E4`: **YES**
4. Breakpoint 1 (`*func_80013A7C+28`, reading `ctx->r31` at offset `0xf8(%rsi)`):
   `0x0000000000000000` (zero)
5. Breakpoint 2 (`*func_80013A7C+99`, reading `ctx->r7` at offset `0x38(%rbx)` before `LOOKUP_FUNC`):
   `0x0000000000000000` (zero)
6. Immediate fatal failure:
   `Failed to find function at 0x00000000`
7. Determinism: **YES (100% deterministic)**

## 3. Disassembly and generated C analysis

### 3.1 Original MIPS disassembly (`banjotooie_decompressed.elf`)

```mips
80013a7c <func_80013A7C>:
80013a7c:  3c013e34  lui    at, 0x3e34
80013a80:  44817000  mtc1   at, $f14
80013a84:  03e03825  move   a3, ra            ; $a3 = $ra (caller return address)
80013a88:  0c004eaf  jal    80013abc          ; call func_80013ABC (overwrites $ra)
80013a8c:  00000000  nop
80013a90:  3c0142b4  lui    at, 0x42b4
80013a94:  44811000  mtc1   at, $f2           ; $f2 = 90.0f
80013a98:  00e00008  jr     a3                ; return to original caller in $a3!
80013a9c:  46001001  sub.s  $f0, $f2, $f0     ; delay slot: $f0 = 90.0f - cos_result
```

### 3.2 Generated C code (`RecompiledFuncs/funcs_391.c:2283-2319`)

```c
RECOMP_FUNC void func_80013A7C(uint8_t* rdram, recomp_context* ctx) {
    uint64_t hi = 0, lo = 0, result = 0;
    int c1cs = 0;
    // 0x80013A7C: lui         $at, 0x3E34
    ctx->r1 = S32(0X3E34 << 16);
    // 0x80013A80: mtc1        $at, $f14
    ctx->f14.u32l = ctx->r1;
    // 0x80013A84: or          $a3, $ra, $zero
    ctx->r7 = ctx->r31 | 0;                     // <--- Copies unmaterialized ctx->r31 (0)
    // 0x80013A88: jal         0x80013ABC
    // 0x80013A8C: nop

    LOOKUP_FUNC(0x80013ABC)(rdram, ctx);
        goto after_0;
    // 0x80013A8C: nop

    after_0:
    // 0x80013A90: lui         $at, 0x42B4
    ctx->r1 = S32(0X42B4 << 16);
    // 0x80013A94: mtc1        $at, $f2
    ctx->f2.u32l = ctx->r1;
    // 0x80013A98: jr          $a3
    // 0x80013A9C: sub.s       $f0, $f2, $f0
    CHECK_FR(ctx, 0);
    CHECK_FR(ctx, 2);
    NAN_CHECK(ctx->f2.fl); NAN_CHECK(ctx->f0.fl);
    ctx->f0.fl = ctx->f2.fl - ctx->f0.fl;
    LOOKUP_FUNC(ctx->r7)(rdram, ctx);          // <--- Evaluates LOOKUP_FUNC(0x0) -> ABORT
    return;
}
```

### 3.3 Caller call site (`func_800963E4` in `funcs_5.c:626-629`)

```c
    // 0x800963F4: jal         0x80013A7C
    // 0x800963F8: lwc1        $f12, 0x20($sp)
    ctx->f12.u32l = MEM_W(ctx->r29, 0X20);
    LOOKUP_FUNC(0x80013A7C)(rdram, ctx);       // <--- Direct C call: DOES NOT write ctx->r31
        goto after_1;
    // 0x800963F8: lwc1        $f12, 0x20($sp)
    ctx->f12.u32l = MEM_W(ctx->r29, 0X20);
    after_1:
    // 0x800963FC: lw          $ra, 0x14($sp)
    ctx->r31 = MEM_W(ctx->r29, 0X14);
```

## 4. RA materialization analysis

1. **Expected RA:** `0x800963FC` (the address following the delay slot of `jal 0x80013A7C` in caller `func_800963E4`).
2. **Actual `ctx->r31`:** `0x0000000000000000`.
3. **RA writer in guest PC:** `0x800963F4` (in caller `func_800963E4`).
4. **Generated call form:** `LOOKUP_FUNC(0x80013A7C)(rdram, ctx);`
5. **RA materialized:** **NO**.
6. **Guest observes RA as data:** **YES** (`func_80013A7C` moves `$ra` to `$a3` and uses `$a3` as jump target).
7. **Missing RA semantically relevant:** **YES**.

## 5. Precedent comparison (`func_800137F4`)

In `src/game/boot_hw_stubs.cpp:949-963`:
```cpp
// sinf wrapper in core1 uses a non-standard return sequence:
//   move a1, ra; jal func_80013818; jr a1
// Direct calls emitted by N64Recomp do not materialize the MIPS return address
// in ctx->r31 because the host C call already models jal/return. Consequently
// the generated wrapper copies a zero ra into a1 and performs LOOKUP_FUNC(0).
// Keep the original constants and approximation helper, but return through the
// host call normally instead of replaying the synthetic MIPS return jump.
extern "C" void func_800137F4(uint8_t* rdram, recomp_context* ctx) {
    ctx->r1 = S32(0x8004 << 16);
    ctx->f14.u32l = MEM_W(ctx->r1, 0x16D4);
    ctx->r1 = S32(0x43B4 << 16);
    ctx->f16.u32l = ctx->r1;
    func_80013818(rdram, ctx);
    ctx->f0.fl = ctx->f2.fl;
}
```

Comparison:
- `SAME_RA_PATTERN_AS_800137F4`: **YES**
- `800137F4_PRECEDENT_RELEVANT`: **YES**
- `SAME_FIX_CLASS_LIKELY`: **YES**

## 6. Full cluster audit of non-standard return wrappers

An exhaustive scan of `core1` angle math wrappers shows exactly 4 functions with this pattern:

| Address | Function | Pattern | Status |
| --- | --- | --- | --- |
| `0x800137D4` | `func_800137D4` | `move a1, ra; jal 80013818; jr a1` | Needs native wrapper |
| `0x800137F4` | `func_800137F4` | `move a1, ra; jal 80013818; jr a1` | **Already fixed** in `boot_hw_stubs.cpp` |
| `0x80013A5C` | `func_80013A5C` | `move a3, ra; jal 80013abc; jr a3` | Needs native wrapper |
| `0x80013A7C` | `func_80013A7C` | `move a3, ra; jal 80013abc; jr a3` | **Current blocker** |

Note: `func_800139F8` at `0x800139F8` has `move a3, ra; jal 80013818; move ra, a3; jr ra`. Because it ends with `jr ra`, N64Recomp emitted a standard `return;` without `LOOKUP_FUNC(ra)`.

Total similar pattern count in codebase: **4 functions** (confined to `core1` math wrappers).

## 7. Refuted hypotheses

1. **Missing symbol in `func_map` (REFUTED):** The target is explicitly `0x00000000`, not a valid non-zero address missing from the function map.
2. **Actor callback pointer uninitialized (REFUTED):** The call is not a dynamic callback; it is a leaf return trampoline.
3. **Scheduler / preemption regression (REFUTED):** The failure is pure synchronous single-thread control flow in `[Game] MAIN`.
4. **BCF28 regression (REFUTED):** `BCF28` executes cleanly before `func_80013A7C` is reached.

## 8. Minimal fix plan (Design only — NOT implemented)

- **FIX_OWNER:** `TOOIE_NATIVE` (`src/game/boot_hw_stubs.cpp`)
- **WHY_MINIMAL:** Confined to the 3 remaining un-overridden math wrappers in `core1` (`0x800137D4`, `0x80013A5C`, `0x80013A7C`). Requires zero generator changes and zero edits to `RecompiledFuncs`.
- **PROPOSED NATIVE IMPLEMENTATIONS:**
  ```cpp
  extern "C" void func_80013ABC(uint8_t* rdram, recomp_context* ctx);

  extern "C" void func_80013A7C(uint8_t* rdram, recomp_context* ctx) {
      ctx->r1 = S32(0x3E34 << 16);
      ctx->f14.u32l = ctx->r1;
      func_80013ABC(rdram, ctx);
      ctx->r1 = S32(0x42B4 << 16);
      ctx->f2.u32l = ctx->r1;
      ctx->f0.fl = ctx->f2.fl - ctx->f0.fl;
  }

  extern "C" void func_80013A5C(uint8_t* rdram, recomp_context* ctx) {
      ctx->r1 = S32(0x8004 << 16);
      ctx->f14.u32l = MEM_W(ctx->r1, 0x1700);
      ctx->f2.u32l = MEM_W(ctx->r1, 0x1704);
      func_80013ABC(rdram, ctx);
      ctx->f0.fl = ctx->f2.fl - ctx->f0.fl;
  }

  extern "C" void func_800137D4(uint8_t* rdram, recomp_context* ctx) {
      ctx->r1 = S32(0x8004 << 16);
      ctx->f14.u32l = MEM_W(ctx->r1, 0x16CC);
      ctx->f16.u32l = MEM_W(ctx->r1, 0x16D0);
      func_80013818(rdram, ctx);
      ctx->f0.fl = ctx->f2.fl;
  }
  ```
- **REGISTRATION:** Register in `register_boot_lookup_stubs()` via `recomp::overlays::add_loaded_function` or export as strong symbols overriding generated weak symbols.
