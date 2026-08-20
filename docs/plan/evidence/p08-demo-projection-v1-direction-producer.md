---
title: "P08 — Projection v1[2] direction producer"
description: "Original-vs-Recomp producer trace for func_800E82EC first-loop v1[2] (classifier a3). Properly aligned Q1 a3 is bit-identical; previous 147.82677 vs 147.41731 split is a phi ping-pong / glidx join artifact."
status: current
type: evidence
project: TooieRecomp
tags: [p08, camera, projection, v1, direction, ca6d4, a8984, differential, mips, evidence]
---

# P08 — Projection `v1[2]` direction producer

- Date: 2026-08-20
- Model: Grok 4.6 HIGH
- Base HEAD: `7f769f70d1e29c794f893577527098266aa24112`
- Upstream: [[p08-demo-projection-record-34-differential]] (`+0x34` writer / B-INPUT as previously joined)

## 0. Verdict

**R (previous a3-difference join CORRECTED/SUPERSEDED).** The DeepSeek
`a3 = v1[2]` producer path is **PROVEN**. The claimed first Q1 numeric split

```text
ORIGINAL a3 = 147.82677  (0x4313D3A7)
RECOMP   a3 = 147.41731  (0x43136AD5)
```

is **not** an Original-vs-Recomp value split. Those two bit patterns are
consecutive phi-bank stores on **both** runtimes (phi=0 then phi=1). Joining
Q1 classifier rows by `e82ec_ord + phi + s3=0 + (a0,a1)=(176,161)` yields

```text
Q1_A3_ALIGNED_MATCH:  26/26 bit-identical
```

`func_800CA6D4` is a two-float copy. Its guest-visible inputs at the selected
producer store match. `func_800A8984(s3)` is on the causal path. `0x800E83D4`
remains a same-writer store of already-computed `v1[2]`, not `FIRST_BAD_PC`.

NO production fix.

## 1. Environment

```text
MUPEN_ENV_REUSED:          NO  (/tmp/p08-m64p reconstructed; prior tree was gone)
MUPEN_CORE_COMMIT:         6dca4c15370ac3e2171ce7b31426695f8f39b460
RICE_COMMIT:               038882dd2e7cb660fd7512c0745032f17edd0bfa
UI_CONSOLE_COMMIT:         c8ac4862a019d7885b24927d9b4db5dd3e42a528
RSP_HLE_COMMIT:            8a7a472a7172eb2c8725b305eae26818ed7b51a2
CPU_MODE:                  Pure Interpreter (--emumode 0, NO_ASM=1)
ROM_SHA1:                  af1a89e12b638b8d82cc4c085c8e01d4cba03fb3
ROM_MATCH:                 YES
RECOMP_HEAD:               7f769f70d1e29c794f893577527098266aa24112
```

Original PC hooks lived only in `/tmp/p08-m64p` `pure_interp.c` (not
committed). Recomp used env-gated `--wrap` of `func_800E82EC` /
`func_800CA6D4` / `func_800A8984` / `func_800E81E0` (reverted before
checkpoint). Generated `RecompiledFuncs` were not edited.

Captures (not committed): `/tmp/p08-v1dir-original.csv`,
`/tmp/p08-v1dir-recomp.csv`.

Do not join this trace by raw `glidx` at `func_800E82EC` time: `D_8012D5B0+8`
stays at 767/511 for most classifier rows. Join by `e82ec_ord`.

## 2. Alignment

```text
E82EC_CALL_ALIGNMENT:      PASS  (same e82ec_ord 1072..1096 for the Q1 176/161 stream)
SELECTED_LOOP_ORDINAL:     s3 = 0  (first-loop slot 0 / sub-slot 0)
SELECTED_PHI:              0 for 0x4313D3A7; 1 for 0x43136AD5 (ping-pong)
SELECTED_SUB:              0
Q1_SELECTOR:               a0=176, a1=161, a2=0x3F820408  (unchanged)
```

Phi ping-pong inside `func_800E82EC`: the first loop writes
`D_8012D600 + phi*60 + s3*12`, then `D_8012D67C` toggles 0↔1, then the
classifier reads the **other** bank. Therefore E81E0 at invocation `N` with
visible `phi=P` consumes the first-loop store from invocation `N-1` that
wrote bank `P`.

## 3. Store at `0x800E83D0` / `0x800E83D4`

Selected producer of the previously cited `0x4313D3A7` value: first-loop
`s3=0` of `e82ec_ord=1091` (consumed by E81E0 `e82ec_ord=1092`, phi=0).

```text
ORIGINAL_F4:               0xC313D3A7   (-147.82677)   at 0x800E83D0
ORIGINAL_F6:               0x4313D3A7   (+147.82677)   at 0x800E83D4
ORIGINAL_T3:               0x8012D600
ORIGINAL_V1_2:             0x4313D3A7
RECOMP_F4:                 not instruction-traced (wrap reconstructed; see note)
RECOMP_V1_2 (E81E0 a3):    0x4313D3A7   (same invocation)
STORE_PC_MATCH:            YES  (0x800E83D4 swc1 f6, 8(t3))
```

The consecutive bank (`e82ec_ord=1092` store / E81E0 `1093` phi=1) is
`0x43136AD5` on **both** sides. Previous Original-vs-Recomp pairing compared
these two banks.

Host `float` reconstruction of `-(2*f2*f12)/(f2-f12)` can be 1 ULP off the
MIPS `mul.s`/`div.s` sequence. Do not treat that predicted ULP as a store
diff. Classifier-consumed `a3` is the ground truth for Recomp.

## 4. `func_800CA6D4` contract (original MIPS)

```text
ENTRY_PC:                  0x800CA6D4
EXIT_PC:                   0x800CA6E4  (jr ra delay: swc1)
SIZE:                      5 instructions / 0x14 bytes
CA6D4_CONTRACT:
  a0 = source object pointer (GPR)
  a1 = dest pointer for source+0xEC (GPR)
  a2 = dest pointer for source+0xF0 (GPR)
  no FPRs as arguments
  no v0 return; outputs are the two stores
  lwc1 f4, 0xEC(a0) ; swc1 f4, 0(a1)
  lwc1 f6, 0xF0(a0) ; jr ra / swc1 f6, 0(a2)
CA6D4_SELECTED_CALL_PC:    0x800E8368  (jal in E82EC first loop)
CA6D4_SELECTED_CALL_ORDINAL: e82ec_ord=1091, first-loop s3=0
```

Generated `RecompiledFuncs/funcs_402.c` matches this instruction-for-instruction
(read-only).

Selected entry inputs (raw bits):

| INPUT        | ORIGINAL     | RECOMP       | MATCH |
| ------------ | ------------ | ------------ | ----- |
| a0 (object)  | 0x801D5470   | 0x801CA950   | semantic (run-local alloc) |
| MEM[a0+0xEC] | 0x4292AC00 (73.3359375) | 0x4292AC00 | YES |
| MEM[a0+0xF0] | 0x4612AC00 (9387.0)     | 0x4612AC00 | YES |
| a1 / a2      | caller stack dests      | caller stack dests | YES (roles) |

```text
CA6D4_ENTRY_INPUTS_MATCH:  YES  (the two floats actually read)
FIRST_DIFFERENT_CA6D4_INPUT: none
CA6D4_BAD_INPUT_PRODUCER:  n/a
FIRST_CA6D4_REGISTER_DIFFERENCE_PC: n/a (Decision A/B not taken; inputs match and consumed a3 matches)
```

Decision B (instruction-trace CA6D4) was not required: the function is a
copy, the copied floats match, and the consumed `v1[2]` matches.

## 5. `func_800A8984` relation

```text
ENTRY_PC:                  0x800A8984
EXIT_PC:                   0x800A899C
SIZE:                      7 instructions / 0x1c bytes
CONTRACT:                  v0 = *(0x8012768C + a0*12)   # table of source-object pointers
A8984_ON_CAUSAL_PATH:      YES  (runtime: jal 0x800E8354, a0=s3, v0 -> CA6D4 a0)
A8984_CALL_PC:             0x800E8354
A8984_RETURN/OUTPUT:       orig v0=0x801D5470 ; recomp v0=0x801CA950  (slot 0)
A8984_OUTPUT_OR_FIELD_USED_BY_CA6D4: v0 as CA6D4 a0; fields +0xEC and +0xF0
A8984_ENTRY_INPUTS_MATCH:  YES  (a0 = s3 = 0)
FIRST_DIFFERENT_A8984_INPUT: none
FIRST_DIFFERENT_A8984_INPUT_SOURCE: n/a
```

Collision-review wording "select the slot's point/transform source" matches
this table lookup. It is proven dynamically, not assumed.

## 6. First-loop formula (for the selected `v1[2]`)

With `f2 = MEM[src+0xEC]`, `f12 = MEM[src+0xF0]`, `f20=1.0`, `f22=2.0`:

```text
f4 = 2 * f2 * f12 / (f2 - f12)
v1[2] = -f4     # stored at 0x800E83D4
```

Selected: `f2=73.3359375`, `f12=9387.0` → stored `0x4313D3A7`.

## 7. Earliest numeric onset

Inside the Q1 `(176,161)` stream (`e82ec_ord` 1072..1096 plus later 1883):

```text
EARLIEST_V1_2_NUMERIC_DIFF:   none (26/26 bit-identical)
EARLIEST_V1_2_CAUSAL_DIFF:    none
```

Do not equate the previously *captured* pair `(0x4313D3A7, 0x43136AD5)` with
the first *created* Original-vs-Recomp difference. Those two values coexist
in both runtimes as consecutive phi banks.

A host-side reconstruction of the store can be 1 ULP off (example
`e82ec_ord=84`: `0x430F366D` vs predicted `0x430F366C`). That is not a guest
store difference.

## 8. Controls

```text
Q0_DIRECTION_PRODUCER_CONTROL: PASS
```

Same E82EC `1091` classifier wave also runs Q0-like `(a0,a1)=(152,161)` /
`(127,161)` against the **same** `v1[2]` (`s3=0`). Instrumentation reproduces
the shared table on both sides.

```text
Q2_SAME_DIRECTION_ROOT: YES
```

Q2 uses the same first-loop `s3=0` `v1[2]` slot at the same E82EC update (the
direction table is not per-selector). No second deep trace. Because aligned
`v1[2]` matches, Q2's historical `+0x34` split is not explained by this
component either.

## 9. Classification

```text
FIRST_DIVERGENT_STORE_PC:  n/a (selected store writes matching bits)
FIRST_BAD_PC:              n/a
FIRST_BAD_MIPS_INSTRUCTION: n/a
ROOT_CAUSE_LEVEL:          R
ROOT_CAUSE:                previous Q1 a3 Original-vs-Recomp inequality is a
                           phi ping-pong / glidx-join artifact. Producer is
                           E82EC first loop <- CA6D4(src+0xEC, src+0xF0) <-
                           A8984(s3=0). Copied floats and consumed v1[2] match.
FIX_OWNER_CANDIDATE:       none for this v1[2] component
FIX_IMPLEMENTED:           NO
```

The `record+0x34` writer identity (`func_800E82EC` mode==1) is **not**
reopened. What is **CORRECTED/SUPERSEDED** is only the claim that `a3=v1[2]`
already differs at that writer.

## 10. Critical FPU

Not a matching-inputs / different-output FPU case. CA6D4 is a copy with
matching float bits. The subsequent MIPS `div.s`/`mul.s`/`neg.s` sequence
produces the same consumed `v1[2]` on both sides. No FMA/rounding/NaN
investigation required.

## 11. Tests

Temporary wrap/CMake instrumentation reverted before these runs.

```text
window_display_test:                    PASS (38/38)
graphics_failure_test:                  PASS (25/25)
stick_angle_thunk_test:                 PASS
matrix_submission_test:                 PASS (MAX_MATRIX_ERROR 0)
input_direction_test:                   PASS (failures=0)
bcf28_helpers_test:                     PASS
core1_ra_wrappers_test:                 PASS
tests/attract_demo2_transition_jal_test.py: PASS
TooieRecompiled rebuild:                PASS
```
