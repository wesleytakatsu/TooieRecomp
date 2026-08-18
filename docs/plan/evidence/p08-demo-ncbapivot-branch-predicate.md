# P08 — Bounded Branch-Predicate Differential for ncbapivot at Glidx 384

## 0. Executive Summary

- **Target Analysis:** Exact branch predicate and return value chain causing the missing third `SET74` call at consumed `glidx 384` in `ncbapivot_entrypoint_3`.
- **Verdict:** **PROVEN (CASE C / EPISTEMIC CORRECTION APPLIED)**.
  - The branch at `0x808001C0: beql $v0, $zero, .L80800234` is **NOT TAKEN in both Original and Recomp** (`func_80111FD4` returns `v0 = 1` in both runtimes at glidx 384).
  - The previous inference that `0x808001C0` skipped the call is **CORRECTED/SUPERSEDED**.
  - Both runtimes proceed to execute `func_801138A0(mode_obj)`.
  - The exact causal gate divergence occurs at `0x808001D0: beqz $v0, .L808001F8` (the `func_801138A0` return value):
    - In **Original N64**, `mode_obj->sub_28` has `half_0C = 4` and `half_0E = 4`. `func_801138A0` increments `half_0E` to `5`, evaluates `(5 >= 5)`, and returns **`v0 = 1`**.
    - In **Recomp**, `mode_obj->sub_28` has `half_0C = 1` and `half_0E = 0`. Because `half_0C < 4`, `func_801138A0` resets `half_0E = 0` and returns **`v0 = 0`**.
    - Recomp then branches to `.L808001F8` to evaluate `func_80115580` distance thresholds (which also return `0`), bypassing `func_801138F4` and omitting the 3rd `SET74`.
  - **First Divergent Predicate Producer:** `func_80113D30` (called by `func_80110588`), which populates `sub_28->half_0C` via 4 raycast/clearance tests.
- **Production Changes:** **0 lines of production C modified** (pure diagnostic isolation).

---

## 1. MIPS Region & BEQL Branch-Likely Semantics

### A. MIPS Instruction Map (`0x808001B0 .. 0x80800240`)
```mips
/* 2145150 808001B0 27A60054 */   addiu     $a2, $sp, 0x54
/* 2145154 808001B4 02002025 */  or         $a0, $s0, $zero
/* 2145158 808001B8 0C0447F5 */  jal        func_80111FD4
/* 214515C 808001BC 27A50048 */   addiu     $a1, $sp, 0x48
/* 2145160 808001C0 5040001C */  beql       $v0, $zero, .L80800234_ncbapivot
/* 2145164 808001C4 02002025 */   or        $a0, $s0, $zero
/* 2145168 808001C8 0C044E28 */  jal        func_801138A0
/* 214516C 808001CC 02002025 */   or        $a0, $s0, $zero
/* 2145170 808001D0 10400009 */  beqz       $v0, .L808001F8_ncbapivot
/* 2145174 808001D4 02002025 */   or        $a0, $s0, $zero
/* 2145178 808001D8 0C044E3D */  jal        func_801138F4
/* 214517C 808001DC 02002025 */   or        $a0, $s0, $zero
/* 2145180 808001E0 50400014 */  beql       $v0, $zero, .L80800234_ncbapivot
/* 2145184 808001E4 02002025 */   or        $a0, $s0, $zero
/* 2145188 808001E8 0C0443D1 */  jal        func_80110F44
/* 214518C 808001EC 02002025 */   or        $a0, $s0, $zero
/* 2145190 808001F0 10000010 */  b          .L80800234_ncbapivot
/* 2145194 808001F4 02002025 */   or        $a0, $s0, $zero
.L808001F8_ncbapivot:
/* 2145198 808001F8 0C045560 */  jal        func_80115580
/* 214519C 808001FC 27A50028 */   addiu     $a1, $sp, 0x28
/* 21451A0 80800200 14400005 */  bnez       $v0, .L80800218_ncbapivot
/* 21451A4 80800204 02002025 */   or        $a0, $s0, $zero
/* 21451A8 80800208 0C045560 */  jal        func_80115580
/* 21451AC 8080020C 27A5006C */   addiu     $a1, $sp, 0x6C
/* 21451B0 80800210 50400008 */  beql       $v0, $zero, .L80800234_ncbapivot
/* 21451B4 80800214 02002025 */   or        $a0, $s0, $zero
.L80800218_ncbapivot:
/* 21451B8 80800218 0C044E3D */  jal        func_801138F4
/* 21451BC 8080021C 02002025 */   or        $a0, $s0, $zero
/* 21451C0 80800220 50400004 */  beql       $v0, $zero, .L80800234_ncbapivot
/* 21451C4 80800224 02002025 */   or        $a0, $s0, $zero
/* 21451C8 80800228 0C0443D1 */  jal        func_80110F44
/* 21451CC 8080022C 02002025 */   or        $a0, $s0, $zero
/* 21451D0 80800230 02002025 */  or         $a0, $s0, $zero
.L80800234_ncbapivot:
/* 21451D4 80800234 27A50060 */  addiu      $a1, $sp, 0x60
/* 21451D8 80800238 0C04429A */  jal        func_80110A68
```

### B. BEQL Specification
- **BEQL_PC:** `0x808001C0`
- **BEQL_DELAY_SLOT:** `0x808001C4` (`or $a0, $s0, $zero`)
- **DELAY_SLOT_IF_TAKEN:** EXECUTED
- **DELAY_SLOT_IF_NOT_TAKEN:** NULLIFIED / SKIPPED
- **Generated Recompiled C:** Matches MIPS branch-likely semantics faithfully (`if (ctx->r2 == 0) { ctx->r4 = ...; goto L_80800234; }`).

---

## 2. Direct Predicate & Return Value Trace (Glidx 382..387)

| glidx | O_11FD4_v0 | R_11FD4_v0 | O_C0_taken | R_C0_taken | O_138A0_v0 | R_138A0_v0 | O_138F4_calls | R_138F4_calls | O_SET74_count | R_SET74_count |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **382** | 0 | 0 | YES | YES | 0 (skipped) | 0 (skipped) | 0 | 0 | 2 | 2 |
| **383** | 0 | 0 | YES | YES | 0 (skipped) | 0 (skipped) | 0 | 0 | 2 | 2 |
| **384** | **1** | **1** | **NO** | **NO** | **1** | **0** | **1** | **0** | **3** | **2** |
| **385** | **1** | **1** | **NO** | **NO** | **1** | **0** | **1** | **0** | **3** | **2** |
| **386** | **1** | **1** | **NO** | **NO** | **1** | **0** | **1** | **0** | **3** | **3** (P2) |
| **387** | **1** | **1** | **NO** | **NO** | **1** | **0** | **1** | **0** | **3** | **3** (P2) |

---

## 3. Sub-Struct Counter Sequence & `func_801138A0` Details

`func_801138A0` inspects `mode_obj->sub_28`:
- `byte_14`: must be `0` (if `!= 0`, returns `0`).
- `half_0C`: required threshold is `>= 4`.
  - If `half_0C < 4`: resets `half_0E = 0`, returns `v0 = 0`.
  - If `half_0C >= 4`: increments `half_0E = half_0E + 1`, returns `v0 = (half_0E >= 5)`.

### Measured Counter Values at `138A0_ENTRY` (Glidx 380..388)

| glidx | Original `half_0C` | Original `half_0E` | Original `138A0_v0` | Recomp `half_0C` | Recomp `half_0E` | Recomp `138A0_v0` |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **380** | 4 | 0 | 0 | 1 | 0 | 0 |
| **381** | 4 | 1 | 0 | 1 | 0 | 0 |
| **382** | 4 | 2 | 0 | 1 | 0 | 0 |
| **383** | 4 | 3 | 0 | 1 | 0 | 0 |
| **384** | **4** | **4 -> 5** | **1** | **1** | **0 -> 0** | **0** |
| **385** | **4** | **5 -> 6** | **1** | **1** | **0 -> 0** | **0** |
| **386** | **4** | **6 -> 7** | **1** | **1** | **0 -> 0** | **0** |
| **387** | **4** | **7 -> 8** | **1** | **1** | **0 -> 0** | **0** |
| **388** | **4** | **8 -> 9** | **1** | **1** | **0 -> 0** | **0** |

---

## 4. Return Value Chain at Glidx 384

- **ORIG_OLD_EYE:** `(-4437.917969, -203.015472, 1276.530762)`
- **ORIG_NEW_EYE:** `(-4439.798828, -203.892792, 1272.196777)`
- **RECOMP_OLD_EYE:** `(-4437.902344, -203.053070, 1276.587891)`
- **RECOMP_NEW_EYE:** `(-4439.804688, -203.940430, 1272.204346)`
- **EECE0_SEMANTIC:** Exact IEEE-754 vector equality comparison (`vec1 == vec2`, no epsilon; returns 1 for equal, 0 for changed).
- **ORIG_EECE0_RETURN:** `0` (`OLD != NEW`)
- **RECOMP_EECE0_RETURN:** `0` (`OLD != NEW`)
- **ORIG_1329C_RETURN:** `1` (`sltiu 0, 1` = 1)
- **RECOMP_1329C_RETURN:** `1` (`sltiu 0, 1` = 1)
- **ORIG_11FD4_RETURN:** `1`
- **RECOMP_11FD4_RETURN:** `1`
- **ORIG_138A0_RETURN:** `1` (because `half_0C = 4 >= 4` and `half_0E: 4 -> 5 >= 5`)
- **RECOMP_138A0_RETURN:** `0` (because `half_0C = 1 < 4` and `half_0E: 0 -> 0 < 5`)
- **BRANCH_PREDICATE_CASE:** **CASE C** (The C0 branch matches; the downstream predicate divergence is at `0x808001D0` produced by `func_801138A0`).
- **FIRST_DIVERGENT_PREDICATE_PRODUCER:** `func_80113D30` (invoked by mode 2 dispatcher `func_80110588` to populate `sub_28->half_0C`).
- **BRANCH_TRACE_EARLY_CONTROL (70..95):** **PASS** (100% bit-exact branch outcomes and call counts).

---

## 5. Worktree Verification

- `git status --short`: clean of temporary code edits.
- `git diff --check`: passes cleanly with 0 errors.
- Unit regressions: 8/8 test suites passing (100%).

## 6. Independent semantic correction (2026-08-18)

The counter and branch-predicate chain above remains **PROVEN**. The earlier
interpretation of the nonzero `func_800E8918` return as a collision HIT is
**CORRECTED/SUPERSEDED**: `func_800C4C34` is a camera projection/viewport
clipping helper, and `func_800E8918` returns a persistent record halfword at
`+0x34`, not the direct helper `v0`. See
[[p08-demo-camera-collision-sol-review]].
