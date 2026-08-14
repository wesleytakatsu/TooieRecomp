---
title: "P08 — core1 RA wrapper fix"
description: "Tooie-native strong-symbol overrides for the proven core1 RA-wrapper cluster (0x800137D4, 0x80013A5C, 0x80013A7C). Old LOOKUP_FUNC(0) from func_80013A7C is resolved."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, core1, ra-wrapper, 80013a7c, 80013a5c, 800137d4, fix]
---

# P08 — CORE1 RA WRAPPER FIX

```text
BASE_HEAD:                 a6b024fcb321aab9b64258089d53696fac54d79b
MODEL:                     Grok 4.6 HIGH
MODE:                      NARROW IMPLEMENTATION + VALIDATION
ROOT_CAUSE_LEVEL:          A
FIX_OWNER:                 TOOIE_NATIVE
BINDING_METHOD:            STRONG_SYMBOL
FUNC_800137F4_CHANGED:     NO
FUNC_800139F8_CHANGED:     NO
GENERATOR_CHANGED:         NO
N64RECOMP_CHANGED:         NO
RECOMPILEDFUNCS_MANUAL_EDIT: NO
OLD_80013A7C_LOOKUP_ZERO:  RESOLVED
NEW_BLOCKER_DIAGNOSED:     NO
```

## 1. Root mechanism (PROVEN)

N64Recomp translates a guest `jal` into a host C call without writing the
guest return address into `ctx->r31`. That is normally correct: the host C
stack models the return.

Four hand-optimized `core1` math wrappers treat incoming `$ra` as ordinary
architectural data:

```text
move temp, ra
jal  helper
...
jr   temp
```

Generated bodies therefore copy unmaterialized `ctx->r31` (zero) and later
execute `LOOKUP_FUNC(0)`.

This is **not** a general ABI failure. The cluster is exactly:

| VRAM | Function | Status before this fix |
| --- | --- | --- |
| `0x800137D4` | `func_800137D4` | unfixed |
| `0x800137F4` | `func_800137F4` | already native in `boot_hw_stubs.cpp` |
| `0x80013A5C` | `func_80013A5C` | unfixed |
| `0x80013A7C` | `func_80013A7C` | current product blocker |

`func_800139F8` has a related-looking `move a3, ra` but restores `$ra` and
returns through standard `jr ra` / generated `return;`. It was not modified.

Diagnosis: [[p08-80013a7c-lookup-zero-diagnosis]].

## 2. Independent MIPS re-verification

Source: `banjo-tooie-Recomp/build/us/banjotooie_decompressed.elf`
via `mips-linux-gnu-objdump`.

### `func_800137D4` — MIPS_137D4_CONFIRMED: YES

```mips
800137d4:  lui    at, 0x8004
800137d8:  lwc1   $f14, 0x16CC(at)
800137dc:  lui    at, 0x8004
800137e0:  move   a1, ra
800137e4:  jal    80013818
800137e8:  lwc1   $f16, 0x16D0(at)   ; delay slot, before helper
800137ec:  jr     a1
800137f0:  mov.s  $f0, $f2           ; delay slot
```

### `func_80013A5C` — MIPS_13A5C_CONFIRMED: YES

```mips
80013a5c:  lui    at, 0x8004
80013a60:  move   a3, ra
80013a64:  jal    80013abc
80013a68:  lwc1   $f14, 0x1700(at)   ; delay slot, before helper
80013a6c:  lui    at, 0x8004
80013a70:  lwc1   $f2, 0x1704(at)    ; AFTER helper returns
80013a74:  jr     a3
80013a78:  sub.s  $f0, $f2, $f0
```

**CORRECTED vs diagnosis prose:** the diagnosis draft loaded `$f2` before
`func_80013ABC`. Independent disassembly loads `$f2` after the helper.
`func_80013ABC` itself clobbers `$f2` (`lwc1 $f2, 4(t0)` then `sub.s $f2,
$f2, $f0`). The native body follows the ELF order.

### `func_80013A7C` — MIPS_13A7C_CONFIRMED: YES

```mips
80013a7c:  lui    at, 0x3e34
80013a80:  mtc1   at, $f14
80013a84:  move   a3, ra
80013a88:  jal    80013abc
80013a8c:  nop
80013a90:  lui    at, 0x42b4         ; 90.0f
80013a94:  mtc1   at, $f2
80013a98:  jr     a3
80013a9c:  sub.s  $f0, $f2, $f0
```

## 3. Binding mechanism

```text
BINDING_METHOD: STRONG_SYMBOL
```

**WHY (PROVEN):**

1. File header of `src/game/boot_hw_stubs.cpp` and `CMakeLists.txt`: Clang
   `RECOMP_FUNC` symbols are weak; GCC is rejected because it emits strong
   generated definitions.
2. `RecompiledFuncs/recomp_overlays.inl` binds by symbol name
   (`{ .func = func_80013A7C, ... }`). Link-time interposition therefore
   updates both direct calls and `LOOKUP_FUNC(vram)` / `get_function`.
3. Precedent `func_800137F4` already used strong-symbol only (no
   `add_loaded_function`) and is unchanged.
4. `nm` of `build/TooieRecompiled` after this fix:

```text
000000000011c640 T func_800137D4
000000000011c5f0 T func_800137F4
0000000000978bc0 W func_800139F8
000000000011c680 T func_80013A5C
000000000011c6e0 T func_80013A7C
00000000001e4e40 W func_80013ABC
```

`add_loaded_function` was not added. Those registrations are for unnamed
helpers without a generated symbol to interpose.

## 4. Files changed

1. `src/game/boot_hw_stubs.cpp` — native `func_800137D4`, `func_80013A5C`,
   `func_80013A7C`; `extern` for `func_80013ABC`. `func_800137F4` untouched.
2. `tests/core1_ra_wrappers_test.cpp` — new focused regression.
3. `CMakeLists.txt` — target `core1_ra_wrappers_test`.
4. Evidence / handoff / P08 phase / indexes as listed in this note.

Not changed: `N64Recomp`, `N64ModernRuntime`, `RecompiledFuncs` (manual),
`func_800139F8`, generator policy.

## 5. Tests

### `core1_ra_wrappers_test`

```sh
cmake --build build --target core1_ra_wrappers_test -j$(nproc)
./build/core1_ra_wrappers_test
```

Exit `0`.

Proves, with `ctx->r31 = 0` on every host call:

- bindings of `0x800137D4` / `0x800137F4` / `0x80013A5C` / `0x80013A7C`
  resolve to the native bodies;
- wrappers return normally (do not depend on materialized `$ra`);
- representative constants and post-call arithmetic match the ELF
  (`137D4`/`137F4` copy helper `$f2`; `13A5C`/`13A7C` do `$f0 = $f2 - $f0`);
- `13A5C` reloads `$f2` after a helper that clobbers `$f2`.

### `bcf28_helpers_test`

```sh
cmake --build build --target bcf28_helpers_test -j$(nproc)
./build/bcf28_helpers_test
```

Exit `0`. Existing BCF28 helper bindings unchanged.

### `TooieRecompiled`

```sh
cmake --build build --target TooieRecompiled -j$(nproc)
```

Exit `0`. PATCH 4 remains default ON
(`[tooie] guest preemption: ON (ULTRAMODERN_GUEST_PREEMPTION=<unset>)`).

## 6. Canonical runtime

```sh
TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json \
timeout -k 2s 150s ./build/TooieRecompiled
```

PATCH4 env: unset / default ON.

| Check | Result |
| --- | --- |
| C2AB8 crossed | YES (historical path; BCF28 reached) |
| BCF28 crossed | YES — gdb: `MAIN` hit `func_800BCF28`, `finish` returned to `bakey_heldCount+146` |
| `func_80013A7C` entered | YES — gdb: `MAIN` hit `func_80013A7C` at native `0x11c6e0` |
| `LOOKUP_FUNC(0x00000000)` from `func_80013A7C` | **absent** in 150s clean run and gdb session |
| Execution past old 80013A7C failure | YES — thousands of later `ovl_load` + live audio |
| Visible attract/demo | NO (opening probe: mode=3 / map=0 / warp=0 / attr=0; mode-5 / map-335 not observed) |
| Residual process after cleanup | none |

Gdb first-stop sequence (Portuguese gdb, no `commands`/`silent`):

```text
Thread 43 "[Game] MAIN" hit Breakpoint 2, func_800BCF28 ()
AFTER_FINISH pc=bakey_heldCount + 146
Thread 43 "[Game] MAIN" hit Breakpoint 1, func_80013A7C ()
```

150s timeout exit 137 (`timeout -k`). No `Failed to find function`.
`tooie-diag` last tick: `rsp_aud=4438` / `queue_samp=6692080` / `stack=(idle)`.

## 7. New first divergence (NOT diagnosed)

High-level signature only:

```text
Non-fatal continuation after native func_80013A7C.
150s clean run: opening probe remains mode=3 map=0 warp=0 attr=0;
mode-5 / map-335 / gsattract not observed; audio and overlay loads continue
until timeout. No new crash string.
```

Do not treat this as a root-cause claim.

## 8. Truth labels

| Claim | Label |
| --- | --- |
| RA-wrapper / unmaterialized `ctx->r31` / `LOOKUP_FUNC(0)` | PROVEN |
| Cluster is exactly the four listed wrappers | PROVEN |
| `func_800139F8` does not need this fix | PROVEN (generated `return;`) |
| Strong-symbol interposition updates overlay `.func` pointers | PROVEN |
| Diagnosis prose that loaded `13A5C` `$f2` before the helper | CORRECTED/SUPERSEDED |
| Next post-80013A7C product blocker identity | UNKNOWN (not diagnosed) |
| Visible attract/demo in this 150s window | UNKNOWN / not observed |
| BCF28 / PATCH 4 / Hybrid F' | do not reopen |

## 9. Next single action

Independent Gemini 3.7 Flash HIGH review of this narrow RA-wrapper fix
before diagnosing the next blocker.
