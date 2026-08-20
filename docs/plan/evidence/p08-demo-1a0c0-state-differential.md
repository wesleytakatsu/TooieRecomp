---
title: "P08 — func_8001A0C0 (176,161) state differential"
description: "Historical Rice-vs-RT64 state differential at func_8001A0C0(176,161). The measured zero/live split is preserved, but independent review proves D_8007BCC4 is the Z image and Rice is not a valid rendered-depth oracle. C-ORACLE."
status: current
type: evidence
project: TooieRecomp
tags: [p08, camera, projection, classifier, 1a0c0, e81e0, bstate, differential, mips, evidence]
---

# P08 — func_8001A0C0 (176,161) state differential

- Date: 2026-08-20
- Model: Grok 4.6 HIGH
- Base HEAD: `95a6c3915f8b9b73544c4c9e031fed65fa8370d8`
- Upstream: [[p08-demo-projection-classifier-rejoin]] (B-STATE at `func_8001A0C0`)
- Downstream: [[p08-demo-db-8007bcc4-lifecycle]], [[p08-demo-db-target-writer]],
  [[p08-demo-zbuffer-oracle-sol-review]] (current C-ORACLE arbitration)

## 0. Verdict

**B-STATE.** `func_8001A0C0(a0,a1)` is a pure **leaf table reader**:

```text
v0 = *(u16*)( D_8007BCC4 + 608*a1 + 2*a0 )
```

For the correctly aligned Q1 classifier invocation `func_8001A0C0(176,161)`
(called from `func_800E81E0` at `0x800E81EC`), the explicit arguments match
25/25. The function performs **no helper calls, no branches, no computation
beyond a single indexed halfword load** of a hidden heap table; the only
possible differing input is the hidden global/base/table memory. On Q1:

```text
ORIGINAL_RICE_CONTROL v0 = 0x00000000        (all 25 aligned samples)
RT64_CONTROL          v0 = 0x00008660..0x00008BFC  (nonzero, varying)
```

Original runtime base `D_8007BCC4 = 0x80137840` (constant across all 388
captured func_8001A0C0 calls), effective read address `0x8014F800`, cell 0.
Recomp returns **incrementing** values at a **fixed** index — impossible for a
static table — so the recomp hidden table/base input is not the original
static DB. NO production fix.

## 1. Environment

```text
MUPEN_ENV_AVAILABLE:  YES  (/tmp/p08-m64p; core 6dca4c1, rice 038882d)
MUPEN_CORE_REBUILT:   YES  (local, no sudo; hook extended for A0C0_ENTRY)
ROM_SHA1:             af1a89e12b638b8d82cc4c085c8e01d4cba03fb3
ROM_MATCH:            YES
CPU_MODE:             Pure Interpreter (--emumode 0, NO_ASM=1)
RECOMP_HEAD:          95a6c3915f8b9b73544c4c9e031fed65fa8370d8
```
Rejoin captures reused: `/tmp/p08-rejoin-original.csv`, `/tmp/p08-rejoin-recomp.csv`.
Fresh original runtime capture (base + cell): `/tmp/p08-a0c0-original.csv` (27017 rows;
temporary hook reverted after capture).

## 2. Alignment

Reuse the correctly aligned Q1 classifier stream ([[p08-demo-projection-classifier-rejoin]]).

```text
ONEA0C0_CALL_ALIGNMENT:    PASS
SELECTED_Q1_STREAM_INDEX:  0 (e82ec_ord 1072)
JOIN_KEY:                  Q1 stream ordinal; (a0,a1)=(176,161); s3=0
CALLER_PC:                 0x800E81EC  (within func_800E81E0)
CALLER_FUNCTION:           func_800E81E0
```

Q1 classifier row 0: Original e82ec_ord 1072, Recomp e82ec_ord 1071 (recomp
offset 7 known; not used as join key).

## 3. func_8001A0C0 exact definition (original MIPS; matches generated)

```text
ENTRY_PC:   0x8001A0C0
EXIT_PC:    0x8001A0E8 (jr $ra), delay slot 0x8001A0EC lhu v0
SIZE:       12 instructions
ABI ARGS:   a0, a1
RETURN:     v0 = u16 at (D_8007BCC4 + 608*a1 + 2*a0)

0x8001A0C0: sll  $t7, $a1, 2
0x8001A0C4: addu $t7, $t7, $a1
0x8001A0C8: lui  $t6, 0x8008
0x8001A0CC: lw   $t6, -0x433C($t6)   # t6 = *(u32*)0x8007BCC4
0x8001A0D0: sll  $t7, $t7, 2
0x8001A0D4: subu $t7, $t7, $a1
0x8001A0D8: sll  $t8, $t7, 5         # t8 = a1*608
0x8001A0DC: sll  $t0, $a0, 1         # t0 = a0*2
0x8001A0E0: addu $t9, $t6, $t8
0x8001A0E4: addu $t1, $t9, $t0
0x8001A0E8: jr   $ra
0x8001A0EC: lhu  $v0, 0x0($t1)
```

```text
DIRECT MEMORY READS (two):
  1. lw  t6, 0x8007BCC4  (base ptr, width 4)
  2. lhu v0, 0(t1) at base + 608*a1 + 2*a0  (width 2)
GLOBALS:          D_8007BCC4
INDIRECT READS:   table cell [base + 608*a1 + 2*a0]
HELPER CALLS:     none (leaf)
BRANCHES:         none (only jr $ra)
SIDE EFFECTS:     none
ONEA0C0_PURE_TWO_ARG_FUNCTION: NO
HIDDEN INPUTS:    D_8007BCC4 base + table content
```

## 4. D_8007BCC4 base — Rice control

```text
ORIGINAL_RICE_CONTROL D_8007BCC4 = 0x80137840
ORIGINAL_RICE_CONTROL READ ADDR  = 0x8014F800
ORIGINAL_RICE_CONTROL CELL (u16) = 0x0000 (all 25 Q1 samples)
```

`func_8001A080` -> `func_8001A018(0x21D80)` -> `heap_alloc` allocates the
base stored at `D_8007BCC4`, a 0x21D80-byte DB (`0x80137840..0x801595C0`).
Read is at `+0x17FC0` = 98240 bytes into the DB.

## 5. DIRECT LOAD SET MATCH

```text
DIRECT_LOAD_COUNT:        2 (lw base, lhu cell)
DIRECT_LOAD_SET_MATCH:    NO
FIRST_DIFFERENT_DIRECT_LOAD: u16 table cell at base+0x17FC0
                            (Rice 0x0000 vs RT64 0x00008660..)
```

## 6. RECOMP RETURN PATTERN (fixed index, incrementing value)

Recomp A0C0_RET stored (v0) across 25 aligned Q1 samples, same
(a0,a1)=(176,161), same offset 0x17FC0:

```text
0x8660,0x86A8,0x86E4,0x8718,0x8750,0x8788,0x87C0,0x87FC,
0x8834,0x886C,0x88A8,0x88E4,0x891C,0x8958,0x8994,0x89D0,
0x8A10,0x8A4C,0x8A8C,0x8AC8,0x8B04,0x8B40,0x8B80,0x8BBC,0x8BFC
```

Deltas vary around +0x3C (60) per Q1 sample. The former “static table” and
misrelocation interpretation is **CORRECTED/SUPERSEDED**: this allocation is a
live RDP Z image, so changing values are expected after rendering.

## 7. Classification

```text
ONEA0C0_PURE_TWO_ARG_FUNCTION:      NO  (hidden global base + table)
ONEA0C0_CALL_TARGET_MATCH:          YES (same logical MIPS 0x8001A0C0;
                                        callers 0x800E81EC / 0x800E8640)
ONEA0C0_ENTRY_STATE_MATCH:          YES for explicit a0/a1 (25/25);
                                        NO for hidden base/table
ONEA0C0_DIRECT_LOAD_COUNT:          2
ONEA0C0_DIRECT_LOAD_SET_MATCH:      NO   (table cell differs)
FIRST_DIFFERENT_DIRECT_LOAD:        u16 at [D_8007BCC4 + 0x17FC0]
ONEA0C0_INDIRECT_OBJECTS:           1 (heap DB at D_8007BCC4, 0x21D80)
INDIRECT_OBJECT_IDENTITY_MATCH:     Original base 0x80137840 const; recomp
                                    base cell live/incrementing
ROOT_CAUSE_LEVEL:                   CORRECTED/SUPERSEDED by C-ORACLE
ROOT_CAUSE:                         func_8001A0C0 returns a u16 from hidden
                                    heap DB base D_8007BCC4 at off 608*a1+2*a0.
                                    Explicit a0/a1 match 25/25; Rice cell=0
                                    while RT64 has live Z at the same index.
                                    Rice omits guest RDRAM depth copyback and
                                    cannot arbitrate the real camera result.
NEXT_SINGLE_ACTION:                 run the bounded Angrylion Plus + CXD4 LLE
                                    Original oracle experiment.
FIX_IMPLEMENTED:                    NO
```

## 8. Why not Level A / B-INPUT / B-CALL

- **B-INPUT excluded**: a0/a1 match 25/25; no internal operand diverges from args.
- **B-CALL excluded**: func_8001A0C0 has zero JALs; no helper chain.
- **Level A excluded**: execution is identical (pure leaf load); hidden table
  input already differs before v0 is finalized.
- **B-STATE**: a hidden global/table/memory input differs.

## 9. Multi-sample confirmation

```text
Q1 first  (1072 orig/1071 recomp): orig 0x0, recomp 0x8660
Q1 middle (1084/1083-1084):        orig 0x0, recomp 0x8958
Q1 last   (1096/1095):             orig 0x0, recomp 0x8BFC
MECHANISM_REPRODUCES_Q1_STREAM:    YES
```

## 10. Q0 control

```text
Q0_1A0C0_CONTROL:   NOT_APPLICABLE
```
func_800E8918 selector-1 (Q0) path returns 0/0 in both runtimes
([[p08-demo-projection-classifier-rejoin]] §7). No clean 1A0C0 control pair;
do not invent one.

## 11. Tests

```text
window_display_test:                        PASS (38/38)
graphics_failure_test:                      PASS (25/25)
stick_angle_thunk_test:                     PASS
matrix_submission_test:                     PASS (MAX_MATRIX_ERROR 0)
input_direction_test:                       PASS (failures=0)
bcf28_helpers_test:                         PASS
core1_ra_wrappers_test:                     PASS
tests/attract_demo2_transition_jal_test.py: PASS
TooieRecompiled rebuild:                    PASS
```

## 12. Housekeeping

- Temporary mupen hook extension (A0C0_ENTRY capture) reverted after capture.
- Original capture: `/tmp/p08-a0c0-original.csv`.
- HEAD `95a6c3915f8b9b73544c4c9e031fed65fa8370d8`; worktree clean before checkpoint.
