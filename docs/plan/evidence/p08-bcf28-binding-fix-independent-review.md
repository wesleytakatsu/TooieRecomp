---
title: "P08 — func_800BCF28 helper binding fix independent review"
description: "Independent Grok 4.6 High review of the Tooie-native 0x800BCCC0/0x800BCCD8 binding fix. Verdict B: fix accepted with nonblocking integer-domain and test-strength notes. Old BCF28 SIGSEGV resolved; next blocker is func_80013A7C (not diagnosed)."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, bcf28, function-binding, independent-review, integer-semantics]
---

# P08 — BCF28 HELPER BINDING FIX INDEPENDENT REVIEW

```text
BASE_HEAD:           0bb38cc84147701654552e1b714f4a8053e4f481
MODEL:               Grok 4.6 HIGH
MODE:                NARROW SOURCE / SEMANTICS / VALIDATION REVIEW
VERDICT:             B — FIX ACCEPTED WITH NONBLOCKING ISSUES
SOURCE_CHANGED:      NO (docs-only checkpoint)
PATCH4_REOPENED:     NO
GENERATOR_CHANGED:   NO
RECOMPILEDFUNCS:     NOT EDITED
NEW_BLOCKER_DIAGNOSED: NO
```

This review covers only
`870bd840c776c5b8a4e1025e7d9eff73defe8e8e..0bb38cc84147701654552e1b714f4a8053e4f481`.
It does not diagnose `func_80013A7C`, implement corrections, reopen PATCH 4 /
Hybrid F', modify N64ModernRuntime or N64Recomp, or hand-edit
`RecompiledFuncs`.

Historical implementer evidence is preserved in
[[p08-bcf28-binding-fix]] and [[p08-bcf28-crash-diagnosis]].

## 1. Canonical state and scope

The protected checkout began clean on `main == origin/main`:

```text
HEAD:        0bb38cc84147701654552e1b714f4a8053e4f481
WORKTREE:    clean
ORIGIN_MAIN: 0bb38cc84147701654552e1b714f4a8053e4f481
```

The review range is a single commit:

```text
0bb38cc fix(p08): restore missing BCF28 helper bindings
```

Files changed:

```text
CMakeLists.txt
docs/HANDOFF.md
docs/VAULT.md
docs/plan/evidence/p08-bcf28-binding-fix.md          (new)
docs/plan/evidence/p08-bcf28-crash-diagnosis.md
docs/plan/evidence/p08-session-handoff.md
docs/plan/index.md
docs/plans/BT/08-rt64-input-and-frontend.md
src/game/boot_hw_stubs.cpp
tests/bcf28_helpers_test.cpp                         (new)
```

Semantic code changes are confined to `src/game/boot_hw_stubs.cpp`,
`tests/bcf28_helpers_test.cpp`, and the `bcf28_helpers_test` CMake target.
No runtime, generator, scheduler, preemption, or `RecompiledFuncs` files
change.

```text
FIX_SCOPE_NARROW: YES
UNEXPECTED_CHANGE: NONE
```

## 2. Independent MIPS reconstruction

Primary ELF: `banjo-tooie-Recomp/build/us/banjotooie_decompressed.elf`
(MIPS-III, `elf32-tradbigmips`).

`nm -S` around the site:

```text
800bcc90 00000030 T func_800BCC90
800bccf4 00000008 T func_800BCCF4
```

No symbol exists at `0x800BCCC0` or `0x800BCCD8`. Both helpers sit in the
unnamed gap after `func_800BCC90` (`vram..vram+0x30` = `0x800BCC90..0x800BCCBF`).

Independent `objdump` of `.core2`:

```text
800bccc0:  000570c0  sll    t6, a1, 0x3
800bccc4:  01c57023  subu   t6, t6, a1
800bccc8:  000e70c0  sll    t6, t6, 0x3
800bcccc:  008e1021  addu   v0, a0, t6
800bccd0:  03e00008  jr     ra
800bccd4:  24420024  addiu  v0, v0, 36

800bccd8:  908e0004  lbu    t6, 4(a0)
800bccdc:  000e78c0  sll    t7, t6, 0x3
800bcce0:  01ee7823  subu   t7, t7, t6
800bcce4:  000f78c0  sll    t7, t7, 0x3
800bcce8:  008f1021  addu   v0, a0, t7
800bccec:  03e00008  jr     ra
800bccf0:  24420024  addiu  v0, v0, 36
```

```text
MIPS_800BCCC0_MATCH:         YES
  v0 = a0 + a1*56 + 0x24   (32-bit ADDU/ADDIU)
  visible temp: t6 / r14   = a1*56
MIPS_800BCCD8_MATCH:         YES
  t6 = LBU(a0+4)           (zero-extended)
  v0 = a0 + t6*56 + 0x24
  visible temps: t6 / r14  = byte; t7 / r15 = byte*56
REGISTER_SIDE_EFFECTS_MATCH: YES
```

The native helpers write exactly those registers (`ctx->r2`, `ctx->r14`,
and for BCCD8 `ctx->r15`). Release codegen:

```text
tooie_helper_800BCCC0:
  mov    0x20(%rsi), %eax          ; a0 = ctx->r4
  imul   $0x38, 0x28(%rsi), %ecx   ; t6 = a1 * 56 (wrapping 32-bit IMUL)
  lea    0x24(%rax,%rcx,1), %eax   ; v0 = a0 + t6 + 0x24
  mov    %rcx, 0x70(%rsi)          ; ctx->r14
  mov    %rax, 0x10(%rsi)          ; ctx->r2
  ret
```

## 3. C++ integer semantics

Project standard is C++20 (`CMAKE_CXX_STANDARD 20` required).

Audited source:

```cpp
const int32_t a1 = static_cast<int32_t>(ctx->r5);
const uint32_t t6 =
    static_cast<uint32_t>((a1 << 3) - a1) << 3;
```

- C++20 signed left shift is defined wrap (`[expr.shift]` modulo `2^N`).
- Signed subtraction `(a1 << 3) - a1` is still undefined if `a1*7` is not
  representable as `int32_t` (for example `a1 >= 2^28`).
- MIPS `SLL`/`SUBU` are 32-bit modulo operations. Exact helper semantics
  would use `uint32_t` wrap for the whole `a1*56` chain.

```text
SIGNED_SHIFT_UB_POSSIBLE:     NO   (C++20)
SIGNED_SUB_OVERFLOW_POSSIBLE: YES  (full int32 domain)
```

### Callers of `0x800BCCC0`

All generated JAL sites are `LOOKUP_FUNC(0x800BCCC0)`. Count: **12**.

| # | Function | File | a1 provenance | Domain |
| --- | --- | --- | --- | --- |
| 1 | `func_800BCEC0` | `funcs_137.c` | `LBU(a0+a1+8)` | u8 |
| 2 | `func_800BCE84` | `funcs_143.c` | `LBU(a0+a1+8)` | u8 |
| 3 | `func_800BCD78` site 1 | `funcs_172.c` | `mfhi` of `div(a2, LBU(a0+4))` | rem of signed32 / u8 |
| 4 | `func_800BCD78` site 2 | `funcs_172.c` | `mfhi` of `div(t3, LBU(a0+4))` | rem of signed32 / u8 |
| 5 | `func_800BCFC4` | `funcs_201.c` | `LBU(a0+a1+8)` | u8 |
| 6 | `func_800BD2D0` site 1 | `funcs_221.c` | `LBU(s0+8)` | u8 |
| 7 | `func_800BD2D0` site 2 | `funcs_221.c` | `(mfhi) & 0xFF` | u8 |
| 8 | `func_800BD2D0` site 3 | `funcs_221.c` | `LBU(s0+8)` | u8 |
| 9 | `func_800BD2D0` site 4 | `funcs_221.c` | `(mfhi) & 0xFF` | u8 |
| 10 | `func_800BD030` | `funcs_303.c` | `LBU(a0+a1+8)` | u8 |
| 11 | `func_800BCF28` | `funcs_350.c` | `LBU(a0+a1+8)` | u8 |
| 12 | `func_800BCCFC` | `funcs_394.c` | `mfhi` of `div(a2, LBU(a0+4))` | rem of signed32 / u8 |

Divisors in the remainder cases are `LBU(base+4)` and therefore `1..255`
(zero hits `break 7`). Signed remainder of a 32-bit value by that divisor
is in `[-254, 254]`. Combined with the LBU / `andi 0xFF` sites:

```text
BCCC0_CALLER_COUNT:          12
BCCC0_A1_DOMAIN:             u8 [0,255] or signed rem-of-u8 ⊆ [-254,254]
ALL_CALLERS_SAFE_FROM_CPP_UB: YES
```

Classification:

```text
OBSERVED_GAME_PATH_SAFE:     YES   (PROVEN for all 12 static LOOKUP sites)
EXACT_MIPS_HELPER_SEMANTICS: NO    (source uses signed sub, not uint32 wrap)
```

This is **not** a production-input UB. It is a non-exact helper that remains
defined on every current caller. A one-line `uint32_t` wrap would make the
source match MIPS for the full 32-bit domain; it was **not** applied in this
review.

Release Clang lowers the expression to wrapping `imul $0x38`, so the
checked binary also wraps. That is observational, not a language-level
guarantee.

`tooie_helper_800BCCD8` computes `t6*56` in `uint32_t` with `t6` from
`MEM_BU` (`0..255`). No signed-overflow issue there.

## 4. `0x800BCCD8` memory semantics

`MEM_BU` in `N64Recomp/include/recomp.h`:

```c
#define MEM_BU(offset, reg) \
    (*(uint8_t*)(rdram + ((((reg) + (offset)) ^ 3) - 0xFFFFFFFF80000000)))
```

- `gpr` is `uint64_t`. Generated callers pass KSEG0 addresses through
  `ADD32`/`S32`, i.e. sign-extended into the 64-bit slot. The helper uses
  `ctx->r4` unchanged. Matches `kseg0_gpr` / recomp codegen.
- `MEM_BU` returns `uint8_t`; the helper zero-extends into `uint32_t` /
  `ctx->r14`. Matches MIPS `LBU`.
- XOR 3 is the standard recomp byte-endian transform. Release codegen
  folds `(a0+4)^3 - 0xFFFFFFFF80000000` to
  `(a0^3) + 0x80000004` and `movzbl`.

```text
BCCD8_LBU_SEMANTICS:     PASS
BCCD8_ADDRESS_SEMANTICS: PASS
```

## 5. Fix layer

The two helpers have no distinct ELF symbols. N64Recomp therefore emits
`LOOKUP_FUNC` rather than a generated `func_800BCCC0` / `func_800BCCD8`.
`metadata/tooie.us.syms.toml` lists `func_800BCC90` (`size = 0x30`) and
`func_800BCCF4`, and not the gap addresses.

A **small existing metadata mechanism** can mark extra function entries:
append `{ name = "...", vram = 0x800BCCC0, size = 0x18 }` (and the BCCD8
sibling) to the `.core2` `functions` list in `tooie.us.syms.toml`. That
does **not** require new N64Recomp architecture. It **does** require a
full `RecompiledFuncs` regen.

Native binding in `src/game/boot_hw_stubs.cpp` is the established Tooie
pattern for unnamed core helpers (`0x800136E4`, `0x800138D0`,
`0x80013970`). It restores LOOKUP targets without regen or generated-C
edits. For this task that is the preferred layer.

```text
NATIVE_FIX_LAYER: APPROPRIATE
```

## 6. Function map registration

`register_boot_lookup_stubs()` now:

1. registers `tooie_helper_800BCCC0` at `0x800BCCC0`;
2. registers `tooie_helper_800BCCD8` at `0x800BCCD8`;
3. then registers the remaining five `kLookupGapAddrs`.

`kLookupGapAddrs` / `kLookupGapFuncs` contain only:

```text
0x80001998  0x80031178  0x80036670  0x80036728  0x800979D8
```

Binary confirmation (`nm` on `build/TooieRecompiled`):

- `_ZL21tooie_helper_800BCCC0PhP14recomp_context` present
- `_ZL21tooie_helper_800BCCD8PhP14recomp_context` present
- exactly **5** `lookup_gap_stub<...>` instantiations, matching the five
  remaining gap addresses
- no `lookup_gap_stub` for `0x800BCCC0` / `0x800BCCD8`

No other Tooie `add_loaded_function` of these two addresses exists.
They are core2 VRAM, not overlay VRAM, so overlay load does not replace
them.

```text
BCCC0_BINDING:         PASS
BCCD8_BINDING:         PASS
LATE_BINDING_OVERRIDE: NONE
```

## 7. Regression test quality

`tests/bcf28_helpers_test.cpp` actually checks:

- both addresses resolve (`get_function` non-null);
- representative BCCC0 outputs + `t6`;
- representative BCCD8 outputs + `t6`/`t7`, via `MEM_BU`;
- the five remaining gap stubs still write `v0 = 0`.

The pointer-inequality `f_bccc0 != f_gap1998` is **not** a proof that
`0x800BCCC0` is not itself a gap stub: each `lookup_gap_stub<Address>`
instantiation is a distinct function. Semantic cases (non-zero KSEG0
results) and the binary's five remaining stubs close that hole.

BCCC0 cases use small nonnegative `a1` (`0,1,2,3,10,13`), including the
historical crash triple `(a0=0x801D2560, a1=3) → 0x801D262C`. They do not
cover `a1=255` (proven u8 domain) or a negative remainder.

```text
TEST_BINDING_STRENGTH:   ADEQUATE
TEST_SEMANTIC_COVERAGE:  representative positive a1 / u8 byte, temps, 5 remaining gaps
MISSING_HIGH_VALUE_CASE: a1=255 (u8 max); optional negative rem-of-u8
```

## 8. Builds and unit test

```sh
cmake --build build --target bcf28_helpers_test TooieRecompiled -j$(nproc)
./build/bcf28_helpers_test
```

```text
[tooie] registered 22 ultra LOOKUP hooks + 5 core helper hooks + 5 gap stubs
[bcf28_test] PASS: 0x800BCCC0 and 0x800BCCD8 are bound to dedicated functions != gap_stub
[bcf28_test] PASS: 0x800BCCC0 representative cases passed
[bcf28_test] PASS: 0x800BCCD8 representative cases passed
[bcf28_test] PASS: All 5 remaining gap stubs correctly return v0 = 0
[bcf28_test] ALL TESTS PASSED SUCCESSFULLY.
```

Exit code `0`.

```text
REGRESSION_TEST: PASS
TOOIE_BUILD:     PASS
```

`TooieRecompiled` was already current at HEAD; the test target rebuilt
cleanly. Clang Release.

```text
BINARY_SHA256: 77f4c4912c2aa76641eea03b84d44727293a8d2288be81dd76d7202f222dc890
TEST_SHA256:   937eca9c60b10294f47d93088b79274997296358c2d30cb1645645bd08f24b59
```

## 9. Runtime acceptance

Canonical Tooie, PATCH 4 default (env unset):

```sh
DISPLAY=:0 XDG_RUNTIME_DIR=/run/user/1000 \
TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json \
timeout -k 2s 120s gdb -batch -q -x /tmp/p08-bcf28-review.gdb \
  --args ./build/TooieRecompiled
```

Observation-only gdb (no source change): breakpoints on the native
helper, `func_800BCF28`, and `func_80013A7C`. After the run: no leftover
`TooieRecompiled` or `gdb`.

Observed:

```text
[tooie] guest preemption: ON (ULTRAMODERN_GUEST_PREEMPTION=<unset>)
[tooie] registered 22 ultra LOOKUP hooks + 5 core helper hooks + 5 gap stubs
[review] ENTER func_800BCF28 #1
[review] BCCC0#1 a0=0x801d2560 a1=0 expected_v0=0x801d2584
... BCCC0#2..#8 same base, a1=0, expected_v0=0x801d2584 ...
[tooie] ovl_load: idx=146 ...          ; basetup
[tooie-diag] leave ovl=726 ep=2 after 29 ms   ; gsworldDll_entrypoint_2
[review] ENTER func_80013A7C
Failed to find function at 0x00000000
```

`func_800BCF28` is the historical callee of `bakey_heldCount` /
`bainput_func_80097B4C`. It entered, `0x800BCCC0` produced a valid KSEG0
address from the same `a0=0x801D2560` base that previously yielded
`v0=0`, and execution continued through `basetup` (ovl 146) and
`gsworldDll` ep2. The old guest PC `0x800BCF58` did not SIGSEGV.

A later host SIGSEGV in `Rml::PluginRegistry::NotifyElementDestroy` is
frontend teardown after `get_function(0)` aborted. It is **not** the old
BCF28 crash and was not investigated.

```text
PATCH4_ACTIVE:                 YES
C2AB8_CROSSED:                 YES
OLD_BCF28_SIGSEGV:             RESOLVED
EXECUTION_PROGRESS_AFTER_BCF28: YES
NEW_BLOCKER_CONFIRMED:         YES
NEW_BLOCKER:                   func_80013A7C
NEW_BLOCKER_DIAGNOSED:         NO
```

## 10. New blocker boundary

Record only:

```text
function: func_80013A7C
message:  Failed to find function at 0x00000000
```

No trace of why the target is zero, caller/RA, generator behavior,
binding, or stack corruption. That is the next task.

## 11. Verdict

```text
VERDICT: B — FIX ACCEPTED WITH NONBLOCKING ISSUES
BCF28_FIX_ACCEPTED: YES
BLOCKING_ISSUES:    NONE
```

The binding layer, MIPS reconstruction, memory semantics, registration,
unit test, and runtime path all support accepting the fix.

Nonblocking issues:

1. `tooie_helper_800BCCC0` claims exact MIPS but uses signed `int32_t`
   subtract for `a1*7`. Production callers cannot hit that UB
   (`ALL_CALLERS_SAFE_FROM_CPP_UB=YES`). A `uint32_t` wrap rewrite would
   make the source exact for the full 32-bit domain; not required to
   accept this task.
2. The unit test's "not a gap stub" pointer compare is against a
   different template address. Semantic cases and `nm` close the gap.
3. Tests omit `a1=255` (and optional negative rem) even though that is
   inside the proven caller domain.

Not verdict C: signed overflow is not reachable from valid production
inputs.

## 12. Specs

Reviewed: recomp-readiness / overlays / input / matching / build as
already cited by the diagnosis. No stable contract change.

```text
SPECS_CHANGED: NONE
```
