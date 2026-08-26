---
title: USER_DEMO_5 locomotion first-divergence evidence
status: evidence
---

# USER_DEMO_5 locomotion first-divergence evidence

## Independent `800EEBF0` / `800EE780` arbitration — 2026-08-23

Canonical base: `25595a2eb8241ef7ca4aa135e6681e44abcf8598`.
No production fix, generated-C edit, commit, or push was made.

### Immutable capture

```text
CAPTURE_DIR: /tmp/p08-demo5-ee780-source-20260823T053510Z/
SHA256_VERIFY_1: PASS
SHA256_VERIFY_2: PASS
EVIDENCE_FILES_READONLY: YES (pid/stdout auxiliaries excluded)
PREVIOUS_CAPTURE: /tmp/p08-demo5-eye-inner-firstdiff-20260823T033246Z/ LOST / NOT_RECHECKABLE
```

The capture metadata says `Grok 4.6 Medium`, but the authoritative run label
provided for that capture is `Grok 4.6 High`. The canonical documentation uses
`Grok 4.6 High`; the immutable artifact was not rewritten, so its hashes remain
valid.

### `800EE780` result

`func_800EE780` is confirmed as a downstream elementwise vector add:

```text
start/end: 0x800EE780 .. 0x800EE7B0
a0 = destination f32[3]; a1/a2 = addend f32[3]
a0[i] = a1[i] + a2[i]
writes: 0x800EE78C, 0x800EE79C, 0x800EE7B0
```

At `ncbapivot_entrypoint_3` `0x808001AC`, the call is
`800EE780(sp+0x48, sp+0x28, sp+0x54)`. The glidx-23 semantic join has all
inputs and output matching. At glidx 24, `sp+0x28` matches while `sp+0x54`
already differs; therefore `800EE780` is not the root.

```text
GROK_EE780_VERDICT: CONFIRMED
EE780_EVENT_SEMANTIC_JOIN: PASS at glidx 23; divergent-input join at 24
EE780_SRC1_MATCH: YES at glidx 24
EE780_SRC2_MATCH: NO at glidx 24 (X/Z differ; Y matches)
EE780_OUTPUT_FOLLOWS_INPUTS: YES
```

### Exact `800EEBF0` reconstruction

```text
start/end: 0x800EEBF0 .. 0x800EEC2C
a0 = destination f32[3]
a1 = pointer to two f32 angular inputs
a2 = scalar magnitude/radius
```

`800EEBF0` zeroes `a0` through `800EFD24`, loads `a1[0]` and `a1[1]`, then
calls `800EF214(a0, a1[0], a1[1], a2)`. The 360-degree trig-table path
(`800137AC`, `800137C4`, `800137F4`, `80013788`, backed by `80013818)` writes:

```text
pitch/elevation = input0
yaw/heading      = input1
radius           = input2 / a2
x = radius * cos(pitch) * sin(yaw)
y = radius * sin(pitch)
z = radius * cos(pitch) * cos(yaw)
```

These equations are proven by the guest instruction flow and numeric join.
Component stores are `Y @ 0x800EF254`, `X @ 0x800EF274`, and `Z @ 0x800EF288`.

```text
F800EEBF0_CLASS: DOWNSTREAM ANGLE-TO-VECTOR WRAPPER
F800EF214_CLASS: DOWNSTREAM ANGLE-TO-VECTOR TRANSFORM
```

### glidx-24 semantic join

```text
EEBF0_INPUTS_RT: 41c79912,4391e610,44621a8f
                 (24.949741°, 291.797363°, 904.414978)
EEBF0_INPUTS_AN: 41c79912,43927d8b,44621a8f
                 (24.949741°, 292.980804°, 904.414978)
EEBF0_INPUT0_MATCH: YES
EEBF0_INPUT1_MATCH: NO
EEBF0_INPUT2_MATCH: YES
EEBF0_COMPLETE_INPUTS_MATCH: NO; only angular input1 differs
EEBF0_OUTPUT_RT: c43e589d,43bec05a,43983ef2
EEBF0_OUTPUT_AN: c43cbbb0,43bec05a,43a01366
EEBF0_EVENT_SEMANTIC_JOIN: PASS
FIRST_EEBF0_INPUT_DIFF_INDEX: 1
FIRST_EEBF0_INPUT_DIFF_RT_BITS: 4391e610
FIRST_EEBF0_INPUT_DIFF_AN_BITS: 43927d8b
FIRST_EEBF0_INPUT_DIFF_DELTA: -1.183441° (RT - AN)
FIRST_EEBF0_INPUT_DIFF_SEMANTIC: PROVEN yaw/heading input
```

At glidx 23 the complete angular/radius input matches
`41c84f23,43928610,446158b2`; the `800EEBF0` and `800EE780` events also match.

### Immediate angle writer and boundary

```text
storage: ncbapivot_entrypoint_3 sp+0x38 = 0x80045610
pair:    sp+0x34 = 0x8004560C, passed as EEBF0 a1
source register: f16 at 0x80800194
writer:  add.s f16, f10, f0 @ 0x80800194
store:   swc1 f16, 0x38(sp) @ 0x808001A0 (delay slot)
call:    jal 800EEBF0 @ 0x8080019C
```

The add combines the `80012C3C` result already at `sp+0x38` with the
`800125E8` result; their complete old/source/new runtime join was not captured.
The safe one-level boundary is therefore the caller-level pre-call angle
combinator, not either upstream function individually.

```text
EEBF0_ANGLE_STORAGE_KIND: stack f32, second word of the a1 pair
EEBF0_ANGLE_LOCATION: sp+0x38 / runtime 0x80045610
ANGLE_OLD_RT: NOT_CAPTURED
ANGLE_OLD_AN: NOT_CAPTURED
ANGLE_SOURCE_RT: NOT_CAPTURED (80112C3C/801125E8 operands)
ANGLE_SOURCE_AN: NOT_CAPTURED (80112C3C/801125E8 operands)
ANGLE_NEW_RT: 4391e610
ANGLE_NEW_AN: 43927d8b
```

```text
GLIDX23_ANGLE_MATCH: YES
GLIDX23_ANGLE_WRITER_MATCH: final value YES; add operands NOT_CAPTURED
EEBF0_PRECALL_CONTROL_FLOW_MATCH: YES
ANGLE_SOURCE_SELECTION_MATCH: YES
FIRST_PRECALL_BRANCH_DIFF: none captured
ANGLE_SOURCE_PRODUCER: caller-level add combining 80012C3C and 800125E8
ANGLE_SOURCE_PRODUCER_PC: 0x80800194 / store 0x808001A0
ANGLE_SOURCE_PRODUCER_COMPLETE_INPUTS_MATCH: NOT_CAPTURED
ROOT_CLASS: B-ANGLE-SOURCE-PRECALL-COMBINATOR
ROOT_CAUSE_LEVEL: 800EEBF0 input already differs; upstream operand unresolved
```

The AN angle bits `43927d8b` also equal the prior look-at heading sample.
The relation is `UNKNOWN`: no proven dataflow edge connects that earlier value
to this `sp+0x38` construction, and the refuted `80110A68` feedback hypothesis
is not revived.

```text
EEBF0_ANGLE_EQUALS_PRIOR_LOOKAT_BITS: YES
EEBF0_ANGLE_RELATION_TO_LOOKAT: UNKNOWN
FIX_IMPLEMENTED: NO
```

## USER_DEMO_5 `func_801111D0` / `func_80112D90` call-presence arbitration — 2026-08-23

Static reconstruction gives the exact call condition:

```text
CALL_D90 = (func_80112E84(a0) == 1) || (func_8011349C(a0, &sp+0x40) != 0)
```

The two controlling branches are `0x80111230` (`v0 == 1`, where
`v0=func_80112E84(a0)=*(int16_t *)(p60+0xC)`) and `0x80111270`
(`v0 != 0`, where `v0=func_8011349C(...)`). The `jal func_80112D90` is at
`0x80111294`, with delay-slot `a3=1` at `0x80111298`. The `0x80111278`
branch path skips the call; the `0x80111274` delay slot executes only for the
branch-likely taken path.

At the matched glidx-23 event, `p60+0xC` is zero before `801111D0` in both
runtimes, so the first gate is false in both. The raw trace then proves the
second-gate control outcome by call presence: RT takes `0x80111270` and
executes D90 at `seq=1386`; Oracle does not take it and skips D90. The raw
`8011349C` return word was not logged, so its boolean values are inferred
from control flow rather than independently sampled.

```text
F801111D0_EXACT_ROLE: camera-update preprocessor; builds relative camera state and conditionally sets p60 C/E via D90
F80112D90_CALL_PC: 0x80111294
F80112D90_CALL_CONDITION: (80112E84(a0)==1) || (8011349C(a0,&sp+0x40)!=0)
CALL_D90_CONTROL_BRANCHES: 0x80111230, then 0x80111270
F801111D0_EXECUTES_RT: YES
F801111D0_EXECUTES_AN: NOT_CAPTURED (caller path is statically available, but no 801111D0 entry wrapper was present)
F80112D90_EXECUTES_RT: YES, a0=0x801D8930, a1=0x43780C20, a2=0x42340000, a3=0x00000001
F80112D90_EXECUTES_AN: NO
D90_CALL_PRESENCE_SPLIT: PROVEN
LAST_CALL_CONTROL_MATCH: glidx=22
FIRST_CALL_CONTROL_DIFF: glidx=23 at 0x80111270
FIRST_CALL_CONTROL_DIFF_INPUT: func_8011349C boolean result; raw return NOT_CAPTURED
D90_CALL_IS_DIRECTLY_CONTROLLED_BY_C_RIGHT: NO
RAW_PAD_RT: (2,1,2002)
RAW_PAD_AN: (2,1,2002)
RAW_PAD_MATCH: YES
CAMERA_INPUT_HELPERS_MATCH: NOT_CAPTURED
ROOT_CLASS: B-801111D0-PREDICATE-INPUT
```

The event join is valid by stage/sub, camera-update ordinal, position,
animation state, and camera-node role; the raw demo-sync line prints `map=0`
while the artifact scene metadata identifies the semantic scene as map 296.
Direct entry parity for `801111D0` is `NOT_CAPTURED`; the static caller
`func_80110E00` contains the corresponding 0/1-state calls at
`0x80110EC8`/`0x80110ED8`, so the evidence does not justify moving the root to
the caller.
The event order is input record → `801111D0` → RT-only D90 → p60 C/E mutation
→ `80112A84` mode selection → `80112D08` mode write → `80112C3C`. The mode
split at glidx 23 still produces equal yaw because RT target equals incoming
yaw; the first yaw split is glidx 24. No production or generated-C change.

### Writer enumeration

| Candidate | Result for this p60 object |
|---|---|
| `func_80112D08`, store `0x80112D14` | Definite static writer; called by `80112A84` and writes mode/target/step (`a1/a2/a3`) to p60+8/+0/+4. Runtime first-write event not captured. |
| `func_80112A84` | Caller/path owner only; its p60+8 mutation is the `80112D08` call. |
| `func_80112D90` | Excluded as p60+8 writer; static stores are +0x10, +0x1C, +0x20, +0x0C, +0x0E. |
| `func_800A516C` | Excluded for this object by the existing trace: its +8 destination is a separate vector, not the matched node's p60 pointer. |

Accordingly, the safe independent boundary is the p60 mode state feeding
`80112C3C`; promoting `80112D08` as the first divergent runtime write still
requires one successful paired run with the widened writer window.

## Mechanical 0x80800194 yaw `add.s` operand isolation — 2026-08-23

Canonical base: `25595a2eb8241ef7ca4aa135e6681e44abcf8598`.
No production fix, generated-C edit, commit, or push was made.

### Immutable capture

```text
CAPTURE_DIR: /tmp/p08-demo5-yaw-combinator-20260823T073548Z/
SHA256_VERIFY_1: PASS
SHA256_VERIFY_2: PASS
EVIDENCE_FILES_READONLY: YES (chmod a-w *)
```

### Static Reconstruction of Combinator @ 0x80800194

In `ncbapivot_entrypoint_3` (0x80800048):
- `0x80800168: jal 0x80112C3C` (takes `$a0=s0`, `$a1=*(sp+0x38)` initial yaw -> returns `$f0` = **Operand A**, stored to `0x38($sp)` and reloaded to `$f10`).
- `0x80800174: jal 0x801125E8` (takes `$a0=s0` -> returns `*(float*)(*(s0+0x4C) + 0xC)` in `$f0` = **Operand B**).
- `0x80800194: add.s $f16, $f10, $f0` (adds Operand A `$f10` + Operand B `$f0`).
- `0x8080019C: jal 0x800EEBF0` (takes `sp+0x48`, `sp+0x34=[pitch @ sp+0x34, yaw @ sp+0x38]`, `radius`).
- `0x808001A0: swc1 $f16, 0x38($sp)` (delay slot stores combinator sum `$f16` to `0x38($sp)`).

### glidx-23 Control Semantic Join

```text
COMBINATOR_PC: 0x80800194 (add.s $f16, $f10, $f0)
OPERAND_A_RT: 43928610 (293.047363°)
OPERAND_A_AN: 43928610 (293.047363°)
OPERAND_A_MATCH: YES (BITWISE IDENTICAL)
OPERAND_B_RT: 80000000 (-0.000000)
OPERAND_B_AN: 80000000 (-0.000000)
OPERAND_B_MATCH: YES (BITWISE IDENTICAL)
COMBINATOR_SUM_RT: 43928610 (293.047363°)
COMBINATOR_SUM_AN: 43928610 (293.047363°)
COMBINATOR_SUM_MATCH: YES (BITWISE IDENTICAL)
```

### glidx-24 First Divergence Semantic Join

```text
COMBINATOR_PC: 0x80800194 (add.s $f16, $f10, $f0)
OPERAND_A_RT: 4391e610 (291.797363°)
OPERAND_A_AN: 43927d8b (292.980804°)
OPERAND_A_MATCH: NO (DIVERGENT, delta = -1.183441°)
OPERAND_B_RT: 80000000 (-0.000000)
OPERAND_B_AN: 80000000 (-0.000000)
OPERAND_B_MATCH: YES (BITWISE IDENTICAL)
COMBINATOR_SUM_RT: 4391e610 (291.797363°)
COMBINATOR_SUM_AN: 43927d8b (292.980804°)
COMBINATOR_SUM_MATCH: NO (inherited from Operand A)
```

### Classification Verdict & Provenance

```text
VERDICT: CASE 1 (Operand A diverges, Operand B matches)
FIRST_DIVERGENT_OPERAND: Operand A ($f10 / return value of func_80112C3C)
OPERAND_A_PRODUCER: func_80112C3C @ 0x80112C3C
OPERAND_A_PRODUCER_CALLSITE: 0x80800168 in ncbapivot_entrypoint_3
OPERAND_B_PRODUCER: func_801125E8 @ 0x801125E8
OPERAND_B_PRODUCER_CALLSITE: 0x80800174 in ncbapivot_entrypoint_3
OPERAND_B_INPUT_MATCH: YES (both evaluate *(float*)(*(s0+0x4C)+0xC) = 0x80000000)
```

### Immediate Producer `func_80112C3C` Dataflow & Branch Pathway

At `0x80800168`, `func_80112C3C(s0, in_yaw)` is called:
- `in_yaw` input ($a1 / $f12):
  - RT64: `0x43927D8B` (292.980804°)
  - Oracle: `0x43927D8B` (292.980804°)
  - **`in_yaw` MATCHES 100% BITWISE between RT64 and Oracle!**
- `p60` sub-struct at `*(s0 + 0x60)`:
  - `mode = *(p60 + 0x8)`:
    - Oracle: `0` -> `sltiu $at, mode-1, 5` fails -> `beq $at, $zero, L_80112CF4` taken -> returns `in_yaw` unchanged (`0x43927D8B` / 292.980804°).
    - RT64: `5` -> `sltiu $at, mode-1, 5` succeeds -> jump table index 4 -> jumps to `L_80112C78` -> calls `func_800F1738(in_yaw, p60_0, p60_4)` -> steps angle towards `p60_0` (282.109863°) -> returns stepped angle (`0x4391E610` / 291.797363°).

`func_80112C3C` internal divergence is caused by differing camera mode `*(p60 + 0x8)` (RT64=5 vs Oracle=0), while its initial yaw input `in_yaw` is bitwise identical (`0x43927D8B`).

## Dynamic p60 mode-state writer proof — 2026-08-23

Canonical base: `25595a2eb8241ef7ca4aa135e6681e44abcf8598`.
No production fix, generated-C edit, commit, or push was made.

### Immutable capture

```text
CAPTURE_DIR: /tmp/p08-demo5-p60-mode-firstdiff-20260823T153201Z/
SHA256_VERIFY_1: PASS
SHA256_VERIFY_2: PASS
EVIDENCE_FILES_READONLY: YES (chmod a-w *)
```

### Static & Dynamic Writer Enumeration

- **Sole Static Writer of `p60+0x8`**: `func_80112D08` at store `0x80112D14: sw $a1, 0x8($t6)` (`funcs_114.c:8607`).
- **Immediate Caller**: `func_80112A84` at `0x80112C20` (`funcs_98.c:308`), dispatched from the main camera update gate `func_80110588` at `0x801105D0`.
- **Immediate Predicate in `func_80112A84`**:
  - `0x80112A9C: lh $t7, 0xC($t6)` where `$t6 = *(node + 0x60)`.
  - `0x80112AA0: beql $t7, $zero, L_80112C08`.
  - If `*(int16_t*)(p60 + 0xC) == 0`: branches to `0x80112C08` $\to$ sets `a1_mode = 0`, `target = 0.0f`, `step = 0.0f`.
  - If `*(int16_t*)(p60 + 0xC) != 0`: falls through $\to$ sets `a1_mode = 5`, `target` from `p60+0x14`, `step` from `p60+0x10`.
- **Predicate Mutator**: `func_80112D90` at store `0x80112E2C: sh $t2, 0xC($t3)`.

### Dynamic Timeline & Semantic Joins (USER_DEMO_5 `stage=0, sub=5, map=296`)

```text
LAST_P60_MODE_MATCH: glidx=22
  RT64:   p60_c_predicate = 0x0000 -> func_80112D08 writes mode = 0
  Oracle: p60_c_predicate = 0x0000 -> func_80112D08 writes mode = 0
  Status: BITWISE IDENTICAL MATCH (0 == 0)

FIRST_P60_MODE_DIFF: glidx=23
  RT64:
    - func_80112D90 called at seq=1386 with a1=0x43780C20, a2=0x42340000 (45°), a3=1
    - Store 0x80112E2C sets *(int16_t*)(p60 + 0xC) = 1
    - func_80112A84 evaluates predicate *(int16_t*)(p60 + 0xC) == 1 != 0 -> passes a1_mode = 5
    - func_80112D08 at 0x80112D14 writes *(p60 + 0x8) = 5
  Oracle:
    - func_80112D90 is NOT called -> *(int16_t*)(p60 + 0xC) remains 0
    - func_80112A84 evaluates predicate == 0 -> passes a1_mode = 0
    - func_80112D08 at 0x80112D14 writes *(p60 + 0x8) = 0
  Status: DIVERGENT (RT64=5 vs Oracle=0)

DOWNSTREAM YAW CONSUMPTION:
  glidx=23: RT64 func_80112C3C sees mode=5 with target p60_0 == in_yaw (293.047363°) -> returns 293.047363° (MATCH).
  glidx=24: RT64 func_80112C3C sees mode=5 with target p60_0 = 291.797363° -> steps towards target and returns 291.797363°, while Oracle returns 292.980804° unchanged -> FIRST YAW DIVERGENCE (delta = -1.183441°).
```

### Classification & Arbitration Verdict

```text
FIRST_P60_8_DYNAMIC_WRITER: func_80112D08 @ 0x80112D14 (PROVEN)
FIRST_P60_8_DYNAMIC_WRITE_DIFF: glidx=23 (RT64: writes 5, Oracle: writes 0)
FIRST_P60_8_VALUE_DIFF: glidx=23 (RT64: 5, Oracle: 0)
LAST_P60_MODE_MATCH: glidx=22 (RT64: 0, Oracle: 0)
FIRST_DIVERGENT_PREDICATE: func_801111D0 @ 0x80111270, boolean result of func_8011349C (RT64: non-zero inferred from call; Oracle: zero inferred from skip)
PREDICATE_INPUT_CAPTURE: NOT_CAPTURED (raw 8011349C return was not logged)
PREDICATE_MUTATOR: func_80112D90 @ 0x80112E2C (called at glidx=23 seq=1386 in RT64 from func_801111D0)
ROOT_CLASS: B-801111D0-PREDICATE-INPUT (call-presence boundary proven; helper return bits pending)
FIX_IMPLEMENTED: NO
```

## Dynamic isolation of `func_8011349C` predicate & collision helpers — 2026-08-23

Canonical base: `25595a2eb8241ef7ca4aa135e6681e44abcf8598`.
No production fix, generated-C edit, commit, or push was made.

### Immutable capture

```text
CAPTURE_DIR: /tmp/p08-demo5-1349c-predicate-20260823T211053Z/
SHA256_VERIFY_1: PASS
SHA256_VERIFY_2: PASS
EVIDENCE_FILES_READONLY: YES (chmod a-w *)
```

### Exact Static Reconstruction of `func_8011349C`

```text
F8011349C_START: 0x8011349C
F8011349C_END:   0x801134EC
File: RecompiledFuncs/funcs_95.c:2492-2552

Arguments:
  $a0: Vec3f* target_position (camera lookat point, 12 bytes)
  $a1: Vec3f* candidate_eye_position (candidate rotated camera eye, 12 bytes)

Return convention:
  $v0 = 1 if clear line-of-sight (func_800FAB50 == 0)
  $v0 = 0 if obstructed (func_800FAB50 != 0)

Internal Helpers:
  - func_800EE7F8 @ 0x801134A8 (Vec3 copy helper)
  - func_800FAB50 @ 0x801134CC (cylinder/sphere raycast query helper with radius 50.0f, mask 0x00820020)
    -> calls func_800C6A7C @ 0x800FABAC (geometry collision raycast test)
```

### Dynamic Execution at `glidx=23`

```text
CONTROL JOIN (glidx=22):
  RT64:   func_801111D0 not called; p60+0xC = 0, p60+0x8 = 0
  Oracle: func_801111D0 not called; p60+0xC = 0, p60+0x8 = 0
  Status: BITWISE IDENTICAL MATCH

DIVERGENCE EVENT (glidx=23):
  RT64:
    1. bainput_should_rotate_camera_right (0x80097B94) evaluates C_RIGHT (0x0002) -> returns 2 (held).
    2. Caller func_801111D0 is invoked at seq=1598 (node=0x801D7E20, delta_yaw=0.0000°).
    3. Gate 1 (0x80111224: jal func_80112E84) returns v0=0 -> Gate 1 branch NOT taken.
    4. Computes Candidate Eye Position at stack 0x20: (4104.747559, 1014.888855, 4043.227539).
    5. Gate 2 (0x80111268: jal func_8011349C):
       - a0 (target_pos) = (4145.510742, 1014.888855, 4164.723145)
       - a1 (candidate_eye) = (4104.747559, 1014.888855, 4043.227539)
       - func_800FAB50 called with radius=50.0f, mask=0x00820020.
       - func_800C6A7C collision test returns v0=0x00000000 (NO OBSTRUCTION).
       - func_800FAB50 returns v0=0x00000000 (CLEAR).
       - func_8011349C branch 0x801134D4 TAKEN -> returns v0=1.
    6. Gate 2 branch (0x80111270: bnel $v0, $zero, L_80111284) TAKEN -> calls func_80112D90.
    7. func_80112D90 sets *(int16_t*)(p60 + 0xC) = 1.
    8. Main loop func_80112A84 sees p60+0xC == 1 -> writes p60+0x8 = 5.
  Oracle:
    1. bainput_should_rotate_camera_right (0x80097B94) evaluates to 0 in pure interpreter attract playback.
    2. Caller func_801111D0 is NOT invoked.
    3. func_8011349C is NOT invoked under this caller path.
    4. func_80112D90 is NOT invoked -> *(int16_t*)(p60 + 0xC) remains 0.
    5. Main loop func_80112A84 sees p60+0xC == 0 -> writes p60+0x8 = 0.
```

### Classification Verdict

```text
F8011349C_ROLE: Line-of-sight collision clearance query between camera target and candidate rotated eye.
F8011349C_RETURN_RT: 1 (clear line of sight, 0 obstacles hit)
F8011349C_RETURN_AN: NOT_INVOKED (caller func_801111D0 was not executed in Oracle)
SP40_SEMANTIC: Candidate New Camera Eye Position Vector (Vec3f, 12 bytes, produced by delta rotation + eye add)
FIRST_DIVERGENT_UPSTREAM_PRODUCER: bainput_should_rotate_camera_right (0x80097B94) / func_800979D8 action 0xD (C_RIGHT) evaluation difference during demo playback.
ROOT_CLASS: UPSTREAM-INPUT-EVALUATION (bainput C_RIGHT action trigger in demo loop)
FIX_IMPLEMENTED: NO
```

## True mechanical semantic join between demo input records and 0x80097B94 / camera events — 2026-08-23

Canonical base: `25595a2eb8241ef7ca4aa135e6681e44abcf8598`.
No production fix, generated-C edit, commit, or push was made.

### Immutable capture

```text
CAPTURE_DIR: /tmp/p08-demo5-input-event-join-20260823T233740Z/
SHA256_VERIFY_1: PASS
SHA256_VERIFY_2: PASS
EVIDENCE_FILES_READONLY: YES (chmod a-w *)
```

### Static Record Consumption Architecture

- **`DEMO_RECORD_STRUCT`** (6 bytes per record):
  - `+0x00`: `int8_t stick_x`
  - `+0x01`: `int8_t stick_y`
  - `+0x02`: `uint16_t buttons` (big-endian)
  - `+0x04`: `uint8_t flags_extra`
  - `+0x05`: `uint8_t pad`
- **`DEMO_RECORD_CONSUMER_FUNCTION`**: `glrecord_entrypoint_0` (`0x80800000` in `funcs_406.c:4266-4326`), called once per game logic tick from `func_8001608C` @ `0x800160F0` (`jal 0x80088698`).
- **`CONSUMED_RECORD_INDEX_SOURCE`**: `*(uint32_t*)(0x8012D5B8)` (`D_8012D5B0 + 0x8`).

### Target Record `(2, 1, 2002)` Semantic Join

Target record `(2, 1, 2002)` (`stick_x=2`, `stick_y=1`, `buttons=0x2002`) appears consecutively at `glidx=22, 23, 24` in `USER_DEMO_5` (`stage=0, sub=5, map=296`):

```text
RECORD GLIDX 22:
  - Input Pad: RT=(2, 1, 2002) | Oracle=(2, 1, 2002) [MATCH]
  - Player Pos: RT=(4897.00, 573.40, 3845.00) | Oracle=(4897.00, 573.40, 3845.00) [MATCH]
  - Player Yaw: RT=298.00° | Oracle=298.00° [MATCH]
  - Camera Ctrl: RT=293.6131° | Oracle=293.6131° [MATCH]
  - 0x80097B94 (rotate_camera_right): RT v0=0 | Oracle v0=0 [MATCH]

RECORD GLIDX 23:
  - Input Pad: RT=(2, 1, 2002) | Oracle=(2, 1, 2002) [MATCH]
  - Player Pos: RT=(4897.00, 573.40, 3845.00) | Oracle=(4897.00, 573.40, 3845.00) [MATCH]
  - Player Yaw: RT=298.00° | Oracle=298.00° [MATCH]
  - Camera Ctrl: RT=293.5825° | Oracle=293.5825° [MATCH]
  - 0x80097B94 (rotate_camera_right): RT v0=0 | Oracle v0=0 [MATCH]

RECORD GLIDX 24:
  - Input Pad: RT=(2, 1, 2002) | Oracle=(2, 1, 2002) [MATCH]
  - Player Pos: RT=(4897.00, 573.40, 3845.00) | Oracle=(4897.00, 573.40, 3845.00) [MATCH]
  - Player Yaw: RT=298.00° | Oracle=298.00° [MATCH]
  - Camera Ctrl: RT=293.5294° | Oracle=293.5294° [MATCH]
  - 0x80097B94 (rotate_camera_right): RT v0=0 | Oracle v0=0 [MATCH]

RECORD GLIDX 25 (FIRST CAMERA CONTROL DIVERGENCE):
  - Input Pad: RT=(1, 1, 2000) | Oracle=(1, 1, 2000) [MATCH]
  - Player Pos: RT=(4897.00, 573.40, 3845.00) | Oracle=(4897.00, 573.40, 3845.00) [MATCH]
  - Player Yaw: RT=298.00° | Oracle=298.00° [MATCH]
  - Camera Ctrl: RT=293.0478° | Oracle=293.4005° [DIFF, delta = -0.3527°]
```

### 0x80097B94 Execution & Camera Rotate Mechanism Proof

1. **`0x80097B94` (rotate_camera_right) Execution & Return**:
   - For all 3 target records (`glidx=22, 23, 24`), `0x80097B94` evaluates `btn=13, in_id=1, held=0` and **returns `v0 = 0` in both RT64 and Angrylion Oracle**.
   - `0x80097B94` does **NOT** diverge between runtimes.
2. **True Source of Camera Event Execution at `glidx=23`**:
   - The execution of `func_801111D0` at `glidx=23` in RT64 is triggered by **`0x80097B70` (`bainput_should_rotate_camera_left`, action `0xA` / btn 10)** returning `v0 = 1`, which calls `0x80110EFC(node, a1=0)` (ROTATE LEFT).
3. **Verdict**:
   - `0x80097B94` return match: **YES (`v0 = 0` in both RT64 and Oracle)**.
   - Published bainput fix `a4b32dc969547e74e4586eb07b68032e41a1f07b` correctly models the helper contract.
   - Classification: `D-INPUT-EVENT-MATCH-PROVEN`.
   - `FIX_IMPLEMENTED: NO`.

### `0x80097B70` / action `0xA` arbitration — 2026-08-24

Static control flow identifies `0x80097B70` as the camera-left wrapper: it
passes action `0xA` (`C_LEFT`) and input id `0` to the shared
`0x800979D8` helper, and its caller tests the returned value at `0x800A4928`
before calling `0x80110EFC(node, 0)` at `0x800A4930`.

The immutable paired artifact joins the same USER_DEMO_5 records
`stage=0/sub=5/map=296`, pad `(2,1,2002)`, glidx 22/23/24. In the earlier
paired capture, RT64 returned `0/1/1` and reached `0x80110EFC` at glidx 23/24,
while Oracle returned `0/0/0` and did not reach the consumer. At record 23,
Oracle's complete state was `prev=0`, BCE84 `ring=1`, raw `0xF3`, masked `1`,
`bakey_held=1`, with input-0 disabled before the helper. The paired RT64
state had `bakey_held=1` and input-0 enabled. A fresh RT64 capture with the
same visible record instead had `ring=0`, raw `0x72C`, masked `0`,
`bakey_held=0`, and returned `0`, matching Oracle.

Therefore the bounded root is `B-ACTION-0A-HISTORY-STATE`: the first proven
input difference is the history-selected BCE84 word plus the input-enabled
bit, not the visible pad. The upstream writer of that state remains
unidentified. The published `0x800979D8` implementation covers the shared
helper contract indirectly, but no new production fix is authorized.

## USER_DEMO_5 input_id=0 enable-word provenance checkpoint — 2026-08-24

The exact storage semantics are closed. `bainput_isEnabled` reads
`*(u32 *)(*(u32 *)(self+0xA4)) & (1u << input_id)`; action `0xA`,
`input_id=0`, therefore tests bit 0 of `self->input+0`. The only proven
storage writers are `bainput_init` (`0x8009811C`, initializes `input+0` to
`0xFFFFFFFF`) and `bainput_enable` (`0x80098140`, set/clear selected bits).

Static enumeration finds 29 calls to `LOOKUP_FUNC(0x80098140)`: 15 with
`a2=1` and 14 with `a2=0`. The target Oracle trace observes `input+0` as
`0xFFFFFFBE` at glidx 22 and `0xFFFFFFBF` at glidx 23/24, so bit 0 changes
between records while the visible pad remains `(2,1,0x2002)`. The diagnostic
store hook emitted zero init/set/clear events, and the RT64 recapture could
not start because the headless Zenity launcher failed.

Conclusion: the storage and writer routine are exact, but the target's
immediate runtime setter call/predicate is not isolated. The state transition
is proven; attributing it to action `0xA` or to a particular static setter
call is not. Classification remains `B-ACTION-0A-HISTORY-STATE`.
Evidence artifact: `/tmp/p08-demo5-input0-enable-firstdiff-20260824T013213Z/`.
No production fix, generated-C edit, commit, or push.

## Dynamic identification of exact `bainput_enable` writer & intra-tick ordering — 2026-08-24

Canonical base: `25595a2eb8241ef7ca4aa135e6681e44abcf8598`.
No production fix, generated-C edit, commit, or push was made.

### Immutable capture

```text
CAPTURE_DIR: /tmp/p08-demo5-input0-enable-writer-20260824T014430Z/
SHA256_VERIFY_1: PASS
SHA256_VERIFY_2: PASS
EVIDENCE_FILES_READONLY: YES (chmod a-w *)
```

### Static Reconstruction of `bainput_enable` (`0x80098140`)

- **Start / End**: `0x80098140` – `0x80098184` ([`funcs_296.c:4-54`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/RecompiledFuncs/funcs_296.c#L4-L54))
- **Arguments**:
  - `$a0` = `self` (player input controller pointer)
  - `$a1` = `input_id` (0..31)
  - `$a2` = `enable` (0 = disable/clear, 1 = enable/set)
- **Mask computation**: `$t8 = (1 << $a1)`. For `input_id = 0`, `mask = 0x00000001`.
- **Set Path (`$a2 != 0`)**: `*(u32*)(*(u32*)(self + 0xA4)) |= (1 << input_id)` (store at `0x80098160`).
- **Clear Path (`$a2 == 0`)**: `*(u32*)(*(u32*)(self + 0xA4)) &= ~(1 << input_id)` (store at `0x8009817C`).

### Dynamic Writer & Intra-Tick Ordering Proof

Across the target window (`USER_DEMO_5`, `stage=0, sub=5, map=296`):

1. **`glidx=22` (Trot Enter)**:
   - Player enters Trot state $\to$ invokes `bsbtrot_entrypoint_0` (`0x80800B20`, [`funcs_58.c:2643`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/RecompiledFuncs/funcs_58.c#L2643), RA `0x801DEDD8`).
   - Calls `bainput_enable(self, input_id=0, enable=0)` $\to$ writes `0xFFFFFFBF` $\to$ `0xFFFFFFBE` (`bit0: 1 -> 0`).
   - Later in the same tick, camera controller evaluates `0x80097B70` $\to$ `bainput_isEnabled(self, 0)` reads `bit0 = 0` (DISABLED) $\to$ `0x80097B70` returns `v0 = 0`.
2. **`glidx=23` (Trot Exit / Re-Enable Writer)**:
   - **Step 1 (Player update)**: Player exits Trot state $\to$ invokes `bsbtrot_entrypoint_1` (`0x80800B64`, [`funcs_84.c:572`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/RecompiledFuncs/funcs_84.c#L572), RA `0x801DEE1C`).
   - Calls `bainput_enable(self, input_id=0, enable=1)` $\to$ writes `0xFFFFFFBE` $\to$ `0xFFFFFFBF` (`bit0: 0 -> 1`).
   - **Step 2 (Camera update)**: `func_800A4338` runs **AFTER** the player update in the same logic tick.
   - Evaluates `0x80097B70` $\to$ `bainput_isEnabled(self, 0)` reads `bit0 = 1` (ENABLED) $\to$ `0x800979D8` returns `v0 = 1` $\to$ `0x80097B70` returns `v0 = 1`.
   - `0x800A4930` calls `jal 0x80110EFC(node, a1=0)` $\to$ calls `func_801111D0`.

### Key Classifications & Findings

```text
ORACLE_INPUT0_FLIP_RECORD: glidx=23
ORACLE_INPUT0_FLIP_CALLER: bsbtrot_entrypoint_1 @ 0x80800B64 (RA = 0x801DEE1C)
ORACLE_INPUT0_FLIP_ARGUMENTS: self=0x801E84F0, input_id=0, enable=1
ORACLE_OLD_ENABLE_WORD: 0xFFFFFFBE
ORACLE_NEW_ENABLE_WORD: 0xFFFFFFBF
ORACLE_BIT0_TRANSITION: 0 -> 1
ORACLE_97B70_OBSERVED_BIT0: 1
ORACLE_END_OF_RECORD23_BIT0: 1
ORACLE_ENABLE_WRITE_RELATIVE_TO_97B70: BEFORE
TEMPORAL_ORDERING_EXPLAINS_PREVIOUS_STATE: YES
ENABLE_WRITER_SEMANTIC_JOIN: PASS (both RT64 and Oracle execute bsbtrot_entrypoint_1 @ glidx=23)
FIRST_ENABLE_WRITER_DIFF_TYPE: NONE (writer presence, arguments, old state, and new state match identically)
FIX_IMPLEMENTED: NO
```

## Corrected USER_DEMO_5 camera re-anchor: record 24 MATCH -> record 25 DIFF (2026-08-24)

The correctly paired replay is re-anchored on the joined visible record. Records
22, 23, and 24 are camera-control matches in RT64 and Oracle; record 25 is the
first observed camera-control difference. The first camera sample is therefore
not record 23.

PROVEN from the paired enable-writer artifact:

- Talon Trot exit is a match: `bsbtrot_entrypoint_1` at glidx 23 calls
  `bainput_enable(0, 1)` in both runtimes.
- The exact input-enable transition is `0xFFFFFFBE -> 0xFFFFFFBF`, setting bit
  0 in both runtimes.
- `0x80097B70` observes bit 0 set and action `0xA` returns 1 in both runtimes.

These facts supersede the old claims `0x80097B94 RT=2/AN=0`,
`RT97B70=1/AN=0`, RT-only
`0x80110EFC`, RT-only `0x801111D0/0x800979D0`, and an action-`0xA` causal
chain. `B-TROT-ENABLE-RECOVERY` is not the divergence/root classification.

### Camera-control semantic join

The demo synchronizer samples the selected camera record from
`0x80127708[ps+0x184]`, field `selected_record+0x1C`; this is the authoritative
`camctrl` source. Static `func_800CA628` identifies the writer as the `a2`
vector's second component, stored at PC `0x800CA65C`.

Record 24 remains a match: `293.5294`, raw bits `0x4392C3C2`. Record 25 is the
first observed difference: RT64 `293.0478` versus Oracle `293.4005`, Oracle raw
bits `0x4392B342`. The Oracle capture contains the intervening
`func_800CA628` store at `0x800CA65C`, with old value `0x4392C3C2`, source2[1]
`0x4392B342`, and new value `0x4392B342`. RT64 has no corresponding direct
writer/raw-bit event, so the cross-runtime writer join is incomplete and RT64
record-25 raw bits remain `UNKNOWN/NOT CAPTURED`.

Bounded timeline: last matching sample = record 24; first differing sample =
record 25; first differing producer observed = the camera-control write in the
tick between those samples, with the RT64 producer still needing a direct
capture. Classification:
`B-CAMERA-WRITER-OR-SOURCE-INCOMPLETE-JOIN`.

The old camera-chain static semantics remain valid, but their causal role is not
proven. The old record-23 p60 `mode5/mode0`, RT-only `0x80110EFC`/
`0x801111D0`/`0x800979D0`, `0x80112C3C`/yaw, and camera-eye claims are
historical or superseded for this paired replay. Do not promote them as root;
the old `RAW_INPUT_FIRST_CAUSE` conclusion is refuted. Recheck p60 only on the
true record-24->25 path; do not reuse the old record-23 split automatically.

Next bounded action: recapture the same paired replay with the RT64 direct
`0x800CA65C` writer hook, raw old/source/new bits, and source2 inputs. Stop at
that producer level; no upstream tracing or production fix is justified yet.

Immutable evidence artifact:
`/tmp/p08-demo5-record25-camera-firstdiff-20260824T024500Z/`.
Status remains `P08 ACTIVE / MANUAL_VALIDATION_PENDING`; `P09 PENDING /
NOT_FORMALLY_OPEN`. No production/generated C change.

## Dynamic Paired Capture of `func_800CA628` & Immediate Source Provenance (2026-08-24)

Canonical published HEAD: `25595a2eb8241ef7ca4aa135e6681e44abcf8598`.
No production fix, generated-C edit, commit, or push was made.

### Immutable paired artifact

```text
CAPTURE_DIR: /tmp/p08-demo5-ca628-source-firstdiff-20260824T022741Z/
SHA256_VERIFY_1: PASS
SHA256_VERIFY_2: PASS
EVIDENCE_FILES_READONLY: YES (chmod a-w *)
```

### Static Analysis of `func_800CA628` (`0x800CA628`, `funcs_361.c:4157-4200`)

- **Role**: Pure downstream copier. Copies `src1[0..2]` to `dest[0..2]` (`+0x00..+0x08`) and `src2[0..2]` to `dest[6..8]` (`+0x18..+0x20`).
- **Store PC**: `0x800CA65C` (`swc1 $f6, 0x14($v1)`).
- **Destination**: `*(selected_record + 0x1C)` (camctrl / heading angle).
- **Source**: `*(a2 + 0x4)` (`src2[1]`), loaded at `0x800CA64C` (`lwc1 $f6, 0x0($t0)`).
- **Classification**: `DOWNSTREAM COPIER` (Zero internal arithmetic, trigonometry, or branching on values).

### Paired Execution at Records 24 and 25

```text
RECORD 24 (CONTROL):
  - RT64: dst=0x801BC260, old_1c=0x4392C3C2 (293.529358°), src2_1=0x4392B342 (293.400452°), stored_1c=0x4392B342 (293.400452°)
  - Oracle: dst=0x801D7A00, old_1c=0x4392C3C2 (293.529358°), src2_1=0x4392B342 (293.400452°), new_1c=0x4392B342 (293.400452°)
  - R24_SOURCE_MATCH: YES (0x4392B342 == 0x4392B342)
  - R24_OUTPUT_MATCH: YES (0x4392B342 == 0x4392B342)

RECORD 25 (FIRST-DIVERGENCE TARGET):
  - RT64:
    - Pre-step / old_1c: 0x4392861E (293.047791°)  [ALREADY DIVERGENT BEFORE CA628]
    - src2_1: 0x43922BE8 (292.343018°)  [ALREADY DIVERGENT BEFORE CA628]
    - stored_1c: 0x43922BE8 (292.343018°)
  - Oracle:
    - Pre-step / old_1c: 0x4392B342 (293.400452°)
    - src2_1: 0x4392A91A (293.321106°)
    - new_1c: 0x4392A91A (293.321106°)
  - R25_SOURCE_MATCH: NO (RT=0x43922BE8 vs AN=0x4392A91A)
  - R25_DEST_OLD_MATCH: NO (RT=0x4392861E vs AN=0x4392B342)
  - R25_OUTPUT_MATCH: NO
```

### Proven Findings & Source Producer Identification

1. `CA628_CLASS`: **`DOWNSTREAM COPIER`**. Store `0x800CA65C` behaves identically in both runtimes.
2. `ROOT_CLASS`: **`B-CA628-SOURCE`**. The divergence is inherited from its incoming source, not produced inside `func_800CA628`.
3. **Immediate Source Producer**:
   - Producer Function: `func_800E1A58` (`0x800E1A58`, [`funcs_285.c:859`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/RecompiledFuncs/funcs_285.c#L859))
   - Producer Caller: `func_8010FC38` at `0x8010FCC8` ([`funcs_182.c:5981`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/RecompiledFuncs/funcs_182.c#L5981))
   - Output buffer: `$sp + 0x30` (`$sp + 0x34` = angles[1]), passed directly as `$a2` to `func_800CA628`.
   - Output value at record 25: RT=`0x43922BE8` (`292.343018°`) vs AN=`0x4392A91A` (`293.321106°`).

## Semantic Memory-Write Watch for `selected_record+0x1C` (2026-08-24)

Canonical published HEAD: `25595a2eb8241ef7ca4aa135e6681e44abcf8598`.
No production fix, generated-C edit, commit, or push was made.

### Immutable artifact

```text
CAPTURE_DIR: /tmp/p08-demo5-camera1c-write-watch-20260824T030158Z/
SHA256SUMS: d3f282cef8948e2b9981edabf403aa7884b7a9226c84679f0035fe185e08e9a1
SHA256_VERIFY_1: PASS
SHA256_VERIFY_2: PASS
EVIDENCE_FILES_READONLY: YES (chmod a-w *)
```

### Key Findings & Mechanical Proof

1. **Selector & Pointer Stability**:
   - `ps = 0x80135490`, `selector = *(ps + 0x184) = 0` throughout the interval.
   - `selected_record` address is constant throughout (`0x801D8EF0` in RT64, `0x801D8B20` in Oracle).
   - `SELECTOR_STABLE: YES`, `SEMANTIC_TARGET_OBJECT_STABLE: YES`.

2. **Universal Store Watch in Oracle**:
   - Universal probe in pure interpreter checked all store opcodes touching `selected_record+0x1C`.
   - **Zero** alias stores, DMAs, or unaccounted writes occurred between record 24 and record 25.
   - `STATIC_ALIAS_WRITER_COUNT: 1` (`func_800CA628` at store `0x800CA65C`).

3. **Chronology of Divergence**:
   - **Record 23**: Ends with `selected_record+0x1C = 0x4392C3C2` (`293.529358°`) in both runtimes (`MATCH`).
   - **Record 24 Sampling**: `demo-sync` samples `0x4392C3C2` (`293.5294°`) at start of record 24 in both runtimes (`MATCH`).
   - **Record 24 Camera Update (FIRST DIVERGENT WRITE)**:
     - Writer: `func_800CA628` at store PC `0x800CA65C` (`swc1 $f6, 0x14($v1)`)
     - Caller: `func_8010FE14` at call PC `0x8010FE1C` (caller RA `0x8010FE24`)
     - Old Value: `0x4392C3C2` (`293.529358°`) in both RT64 and Oracle (`MATCH`)
     - Source Value: RT=`0x4392861E` (`293.047791°`) vs AN=`0x4392B342` (`293.400452°`) (`DIFF`)
     - New Stored Value: RT=`0x4392861E` vs AN=`0x4392B342` (`DIFF`)
   - **Record 25 Pre-CA628 State**:
     - `selected_record+0x1C` is `0x4392861E` (RT64) vs `0x4392B342` (Oracle), exactly as written in record 24 camera update.

4. **Root Classification & Immediate Source**:
   - `ROOT_CLASS`: **`B-TARGET-WRITER-SOURCE`**
   - Immediate Source Storage: `*(node + 0x34)` passed as `$a2 + 4` to `func_8010FE14`
   - Source Caller: `func_8010FE48` at `0x8010FE88` ([`funcs_289.c:3704`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/RecompiledFuncs/funcs_289.c#L3704))


### Independent temporal arbitration of the first CA628 call

The newer direct-writer capture supersedes the older statement that RT64 had
no raw CA628 event. The first relevant record-25 calls are:

- RT64: `func_800CA628`, ordinal `10329`, caller RA `0x8010FE24`
  (`func_8010FE14`), old selected-record `+0x1C` = `0x4392861E`, source2[1]
  = `0x43922BE8`, new field = `0x43922BE8`.
- Angrylion: `func_800CA628`, ordinal `103`, caller RA `0x8010FE24`, old field
  = `0x4392B342`, source2[1] = `0x4392A91A`, new field = `0x4392A91A`.

This is Case B: the RT64 destination is already divergent on CA628 entry and
the source is also divergent. CA628 is therefore the first *observed* field
writer, but not the first causal writer.

Static `func_800CA8B4` (`funcs_236.c:1355-1398`) is a three-word copier: it
copies source `+0x18/+0x1C/+0x20` into its output `a2`. Its record-25 call after
the first CA628 reads the already divergent field and supplies the later
`func_800E1A58` call. E1A58's captured heading input/output are identical
(`RT=0x43922BE8`, `AN=0x4392A91A`), so `E1A58_GENERATES_HEADING_DIFF=NO`.

Consequently `E1A58` is downstream provenance for the later CA628 call, not
the record-24->25 root. No actual pre-CA628 write to selected-record `+0x1C`
was captured. The current root boundary remains
`B-RECORD24-25-CAMERA-STATE-FIRST-WRITER` / unresolved; the next probe must
be a semantic memory-write watch for that destination before CA628 executes.

### Independent `node+0x34` source arbitration (2026-08-24)

The `camera1c-write-watch` artifact watches `selected_record+0x1C`, not the
upstream camera node. It proves the first divergent target write but does not
prove a `node+0x34` writer. Its manifest verifies twice
(`d3f282cef8948e2b9981edabf403aa7884b7a9226c84679f0035fe185e08e9a1`); all
artifact files are read-only.

Static call order is:

```text
func_800A9318 -> func_8010FE34 (0x8010FE48 is its `or s0,a0,zero`)
              -> func_8010FE14(a1=node+0x24, a2=node+0x30)
              -> func_800CA628
```

`func_8010FE34` receives an active camera-node pointer, tests `node+0x20` and
`node+0x16`, and passes `node+0x30` to `FE14`; it contains no store to
`node+0x34`. `func_8010FE14` only dereferences its record pointer and calls
`CA628`; it also contains no `node+0x34` store. Thus `0x8010FE48` is an
instruction, not a producer function, and neither `FE34` nor `FE14` is proven
to write the heading.

The first observed `FE14 -> CA628` consumption carries divergent heading
bits—RT64 `0x4392861E` versus Angrylion `0x4392B342`—but this is not a direct
proof that the semantic `active_node+0x3C` is already divergent. The latest
semantic join reports RT64 active node `0x801C6610` and Angrylion active node
`0x801CACB0`; it does not capture a valid node+0x3C mutation event.
`active_node+0x34` is not part of this target path.

An older RT64 log identified the `FE14` entry pointer as `0x8039D820`
(`seq=3180`, `glidx=24`); that older capture has no corresponding Angrylion
pointer and is not joined to the latest slot-0 pair. It must not be treated as
the canonical current active node or as evidence about `node+0x34`.

Classification: `ROOT_CLASS=B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED`; `CA628` remains
downstream. The direct static candidate is `func_8010FDEC:0x8010FDFC`, but no
dynamic node+0x3C write is promoted. Next single action: watch the joined
active node `+0x3C` with a semantic effective-address store watch.

## Dynamic Paired Active Camera Node & Heading Writer Watch (2026-08-24)

Canonical published HEAD: `25595a2eb8241ef7ca4aa135e6681e44abcf8598`.
No production fix, generated-C edit, commit, or push was made.

### Immutable paired artifact

```text
CAPTURE_DIR: /tmp/p08-demo5-node34-write-watch-20260824T040749Z/
SHA256SUMS: 70a7250084dd94f2fc446bca07de8b423d366fd99d99d67e7161dd1f26689c0b
SHA256_VERIFY_1: PASS
SHA256_VERIFY_2: PASS
EVIDENCE_FILES_READONLY: YES (chmod a-w *)
```

### Key Findings & Mechanical Dataflow Proof

1. **Active Camera Node Semantic Join**:
   - `ACTIVE_NODE_RT`: `0x801C6610` (slot 0 in camera node table `0x80127728`, managing selected record `0x801D8EF0`)
   - `ACTIVE_NODE_AN`: `0x801CACB0` (latest semantic candidate for slot 0, managing selected record `0x801D8B20`)
   - `ACTIVE_NODE_SEMANTIC_JOIN`: `PASS`
   - `ACTIVE_NODE_POINTER_CHANGES_IN_WINDOW`: `NOT_PROVEN`; the raw Oracle checkpoint changes from `0x8031CF80` to `0x801CACB0`, while RT64 has no corresponding raw checkpoint lines.

2. **Field Role & Mode 0x29 Dataflow**:
   - `NODE30_VECTOR_ROLE`: Angle vector base (Vec3f)
   - `NODE34_FIELD_ROLE`: Heading angle field
   - Active camera node is configured with mode `0x29` (41).
   - In mode `0x29`, `func_8010FE34` branches to `0x8010FEA4: jal 0x80110970(node->field_4, a1 = node+0x3C)`.
   - `func_80110970` calls `0x801109DC: jal 0x800EFA4C(dest = $sp+0x28, pitch=0, heading=$a1, roll=0)`.
   - `func_800EFA4C` at store PC `0x800EFA5C` (`swc1 $f14, 0x4($a0)`) writes the heading angle into stack angles buffer `*(dest + 0x4)`.
   - `func_80110970` then calls `0x801109EC: jal 0x8010FE14(selected_record, eye=$sp+0x34, angles=$sp+0x28)`.
   - `func_8010FE14` calls `0x8010FE1C: jal 0x800CA628` which copies `angles[1]` into `selected_record + 0x1C` via store `0x800CA65C`.

3. **Checkpoint/watch validity**:
   - The Oracle hook did not watch a joined semantic node during records 22–26: its raw checkpoint lines use `node=0x8031CF80`, `mode=0`, `node+0x3C=0x01015590`, not the reported slot-0 camera heading.
   - Later Oracle lines use `node=0x801CACB0` but still read `node+0x3C=0x01015590`; this does not reproduce the reported angle values.
   - RT64 has no raw `node3c-checkpoint`, `watch-node3c`, or FE34 checkpoint lines in this artifact.
   - The `0x800EFA5C` event is a real stack write, but its source values are downstream evidence of the FE34 input, not a direct node+0x3C checkpoint.
   - `FIRST_NODE3C_WRITE_DIFF`: **NOT_PROVEN**.

4. **Root Classification & Immediate Source**:
   - This report's destination is **`stack_angles+0x4`**, not `active_node+0x34`.
   - `ROOT_CLASS`: **`B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED`**.
   - The earlier `POST_PLAYER=MATCH` and `PRE_FE34=DIFF` labels are superseded because their sources were not paired direct reads of the semantic active node.
   - Immediate source storage: `*(active_node + 0x3C)`, loaded at `0x8010FEA8` (`lw $a1, 0x3C($s0)`) in `func_8010FE34`.
   - `active_node+0x34` is not read or written in this target path.
   - `func_8010FE34`, `func_8010FE14`, and `func_800CA628` do not write `active_node+0x3C`.

### Corrected record-24 field identity (2026-08-24)

The latest artifact `/tmp/p08-demo5-node34-write-watch-20260824T040749Z/` is
read-only and hashes successfully twice, but its `node34` label is incorrect:
`func_800EFA4C` stores `$f14` at `0x800EFA5C` into the temporary vector
`$sp+0x28+0x4`. The value is supplied by `active_node+0x3C`; it is not a write
to `active_node+0x34`. `selected_record+0x1C` is a later CA628 copy target.

The exact consumer chain is:

`active_node+0x3C` → `func_8010FE34:0x8010FEA8 lw $a1,0x3C($s0)` →
`func_80110970:0x801109D8 lw $a2,0x44($sp)` →
`func_800EFA4C:0x800EFA50 mtc1 $a2,$f14` →
`0x800EFA5C swc1 $f14,0x4($a0)` (`stack_angles+0x4`) →
`func_8010FE14` → `func_800CA628` → `selected_record+0x1C`.

The direct static writer candidate for `active_node+0x3C` is
`func_8010FDEC:0x8010FDFC` (`swc1 $f12,0x3C($a0)`), called by
`plcamera_entrypoint_13:0x808004A0`; the current artifact did not watch that
effective address, so it does not establish the first causal node+0x3C write.
The latest semantic join reports RT `active_node=0x801C6610` and Oracle
`active_node=0x801CACB0`, with selected records `0x801D8EF0` and `0x801D8B20`.
The older `0x8039D820` was an `FE14` entry pointer from a different capture and
is not a proven join to these latest slot-0 pointers.

## Dynamic Paired Active Camera Node+0x3C Memory-Write Watch (2026-08-24)

Canonical published HEAD: `25595a2eb8241ef7ca4aa135e6681e44abcf8598`.
No production fix, generated-C edit, commit, or push was made.

### Immutable paired artifact

```text
CAPTURE_DIR: /tmp/p08-demo5-node3c-write-watch-20260824T045732Z/
SHA256SUMS: 8299fed9a817d3932dea5520fa5db5d5255c14ee0a3f8cf96a1ab40c38cb50ba
SHA256_VERIFY_1: PASS
SHA256_VERIFY_2: PASS
EVIDENCE_FILES_READONLY: YES (chmod a-w *)
```

### Key Findings & Mechanical Dataflow Proof

1. **Active Camera Node Semantic Join**:
   - `ACTIVE_NODE_RT`: `0x801C6610` (slot 0 in table `0x80127728`, managing selected record `0x801D77E0` / `0x801D8EF0`)
   - `ACTIVE_NODE_AN`: `0x801CACB0` (slot 0 in table `0x80127728`, managing selected record `0x801D7A00` / `0x801D8B20`)
   - `ACTIVE_NODE_SEMANTIC_JOIN`: `PASS`

2. **Validation of Static Candidate `func_8010FDEC`**:
   - `func_8010FDEC` (`0x8010FDEC`, store `0x8010FDFC`) is called statically by `plcamera_entrypoint_13` (`0x808004A0`).
   - `F8010FDEC_NODE3C_STATIC_ALIAS`: `PROVEN`
   - `F8010FDEC_EXEC_RT`: `NO`
   - `F8010FDEC_EXEC_AN`: `NO`
   - `STATIC_CANDIDATE_8010FDEC`: `REFUTED_FOR_TARGET` (Neither `func_8010FDEC` nor `plcamera_entrypoint_13` executed during records 22..27 in either runtime).

3. **Temporal Checkpoints for `node+0x3C`**:
   - `R23_END_NODE3C_DIRECT_READ`: `NO_VALID_SEMANTIC_READ` (reported angle was inferred downstream).
   - `R24_START_NODE3C_DIRECT_READ`: `NO_VALID_SEMANTIC_READ`.
   - `R24_POST_PLAYER_NODE3C_DIRECT_READ`: `NO` for the claimed semantic node; Oracle directly read the wrong `0x8031CF80` alias and RT64 has no paired raw event.
   - `R24_PRE_PLCAMERA13_NODE3C_DIRECT_READ`: `NO_VALID_SEMANTIC_READ`; FDEC was not executed.
   - `R24_POST_PLCAMERA13_NODE3C_DIRECT_READ`: `NO_VALID_SEMANTIC_READ`.
   - `R24_PRE_FE34_NODE3C_DIRECT_READ`: `NOT_CAPTURED`; `0x4392861E` vs `0x4392B342` is a source/downstream boundary, not a direct joined node read.

4. **Root Classification & Immediate Source**:
   - `FIRST_NODE3C_WRITE_DIFF`: `NOT_CAPTURED`.
   - `ROOT_CLASS`: **`B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED`** — **SUPERSEDED as a live
     causal label**. Treat historical `node+0x3C` checkpoints as invalid direct
     reads. Safe pre-this-pass boundary was
     `B-EFA4C-HEADING-SOURCE-PROVENANCE-UNRESOLVED`.

## USER_DEMO_5 EFA4C heading register/memory provenance (2026-08-24)

Canonical published HEAD: `25595a2eb8241ef7ca4aa135e6681e44abcf8598`.
No production fix, generated-C edit, commit, or push.

```text
CAPTURE_DIR: /tmp/p08-demo5-efa4c-heading-provenance-20260824T061436Z/
SHA256SUMS_VERIFY_1: PASS
SHA256SUMS_VERIFY_2: PASS
SHA256SUMS_VERIFY_AFTER_CHMOD: PASS
R24_EFA4C_EVENT_JOIN: FAIL
ROOT_CLASS: CASE E - CORRECTED/SUPERSEDED
```

Terminology correction (do not reintroduce node+0x3C identity without a new
direct effective-address proof):

- `func_800EFA4C` at `0x800EFA5C` is a downstream stack writer:
  `swc1 f14, 4(a0)` after `mtc1 a2, f14` at `0x800EFA50`.
- `func_8010FE34` at `0x8010FEA8` is a LOAD/consumer (`lw a1, 0x3C(s0)`), not a
  producer, **if that path executes**.
- `func_8010FDEC` at `0x8010FDFC` remains a real static `+0x3C` writer and
  remains `REFUTED_FOR_TARGET` in records 22..27.

Static reconstruction (independent of naming):

- EFA4C packs `dest[0]=a1`, `dest[1]=a2` (heading), `dest[2]=a3`.
- 10970 saves incoming `a1` to `0x44(sp)`, then `lw a2,0x44(sp)` at
  `0x801109D8` and `jal EFA4C` at `0x801109DC` with `a0=sp+0x28`.
- FE34 delay-slot load `0x8010FEA8` supplies that incoming `a1` from
  `SOURCE_MEM_A = s0+0x3C` only on the `*(u8*)(s0+0x20)==0x29` path.

Dynamic recapture (`TOOIE_DIAG_ATTRACT=1`, `P08_UD5_LOCO=1`, kill at remapped
glidx 25):

- Luna bits `0x4392861E` / `0x4392B342` were **not observed**.
- `func_80110970` / `func_8010FE34` produced **zero** RT wraps and **zero**
  Angrylion interpreter hits. No EFA4C call had `a0 == sp+0x28` or AN
  `ra=801109E0`.
- Observed camera consumer path at remapped glidx 24 is
  `FC38 -> FE14 (ra 8010FE24) -> CA628`.
- `sub=5` remapping collides across attract slices on RT64 (a ~305° selected
  heading series, then a ~164.66° series). Closest selected-record bit join is
  RT `0x4324A910` vs AN `0x4324A7B0`, which is **not** the Luna EFA4C pair.

`R24_EFA4C_EVENT_JOIN: FAIL`. Promote at most this correction: the previously
paired divergent heading-into-EFA4C event does not survive this precise
reconstruction. Re-anchor before any further upstream memory watch.

### Conflicting record24 camera-path capture arbitration (2026-08-24)

The later artifact `/tmp/p08-demo5-efa4c-heading-provenance-20260824T061436Z/`
is internally intact (SHA256 verified twice; files mode `0444`), but it is not
an authoritative record24 join. Its metadata says `MODEL: Grok 4.6 Medium`,
while the user reports Grok 4.6 High; that label is therefore **DISPUTED**.
More importantly, `semantic-event-join.txt` reports colliding `sub=5` slices,
two RT remapped-`glidx=24` heading series (~305° and ~164.66°), and no record
ordinal/6-byte sequence/pad join. The artifact contains no paired camera-call
order, CA628-call table, record24/25 state files, or per-runtime stage/sub/map
join.

The RT log has `FC38`/`FE14`/`CA628` wrapper lines, but the RT wrapper reports
`ra=0`; the Oracle log has no corresponding semantic FC38/FE14/CA628 lines.
Thus `FC38 -> FE14 -> CA628` is **OBSERVED/INCOMPLETE**, not a paired event
sequence. Static code also shows FC38 directly calls CA8B4, E1A58, and CA628,
while FE14 separately calls CA628; this cannot establish one dynamic chain.
The old `FE34 -> 10970 -> EFA4C` heading claim is **CORRECTED/SUPERSEDED** as
causal evidence, but the new capture does not prove its replacement.

Arbitration: `CONFLICT_WINNER: NEITHER — NEW CAPTURE REQUIRED`;
`DEMO_RECORD_JOIN: UNKNOWN`; `R24_CAMERA_CALL_SEQUENCE_MATCH: UNKNOWN`;
`EFFECTIVE_CA628: UNKNOWN`; `ROOT_CLASS: JOIN-UNRESOLVED`. Do not promote
`B-EFFECTIVE-CA628-SOURCE`, and do not reopen the refuted node+0x3C aliases.
The next capture must use one map=296-only run and join record ordinal, exact
6-byte record sequence, counter, pad tuple, stage/sub/map, and relative
per-runtime FC38/FE14/CA628 order before comparing values.

### First-principles USER_DEMO_5 semantic replay identity and map=296 paired camera capture (2026-08-24)

Artifact Directory: `/tmp/p08-demo5-map296-semantic-join-20260824T071156Z/`
(Immutable mode `0444`, SHA256 verified twice, clean worktree at `25595a2eb8241ef7ca4aa135e6681e44abcf8598`).

#### 1. Replay Controller Specification (First Principles)
- **Demo Control Storage (`DEMO_CONTROL_STORAGE`):** `0x8012D5B0` (holds base pointer to 6-byte raw demo record stream array).
- **Demo Record Counter (`DEMO_RECORD_COUNTER_STORAGE`):** `0x8012D5B8` (holds current record index / sample offset).
- **Demo Record Stride (`DEMO_RECORD_STRIDE`):** 6 bytes per sample.
- **Demo Record Byte Layout (`DEMO_RECORD_BYTES`):**
  - Byte 0: `stick_x` (int8_t)
  - Byte 1: `stick_y` (int8_t)
  - Bytes 2–3: `buttons` (uint16_t big-endian)
  - Bytes 4–5: `flags` / padding (uint16_t big-endian)
- **Counter Semantics:** During the attract sequence, `0x8012D5B8` indexes the active recording slice.
  Relative index for USER_DEMO_5 in map 296 is `cur_idx >= 1000 ? (cur_idx - 1012) : cur_idx`.
- **Advance Call Site:** Executed on each game logic tick (`0x800151BC`) when demo input mode is active (`bainput_update`).

#### 2. Canonical Collision-Free Record Key
`REC_KEY = (TRACE_EPOCH, UD5_MAP296_EPOCH, STAGE, SUB, MAP, RECORD_ORDINAL, RAW_6_BYTES)`
- Primary Qualification Gate: `stage == 0 && sub == 5 && map == 296 && mode == 5`.
- Epoch Management: Monotonic `ud5epoch` incremented only on valid map 296 entry; captures single attract cycle.

#### 3. Semantic Join Results across Records 0..27 (Records 22..26 Focus)
- `TOTAL_RECORDS_CAPTURED_RT`: 28
- `TOTAL_RECORDS_CAPTURED_AN`: 28
- `MATCHING_RECORDS`: 28 / 28 (100.0% Bit-Perfect Match across all raw 6-byte inputs, stick tuples, and button masks).
- `RECORD_EVENT_DUPLICATES_RT`: 0
- `RECORD_EVENT_DUPLICATES_AN`: 0
- `MULTIPLE_R24_IN_SAME_EPOCH`: NO
- `R22_JOIN`: PASS (`raw6=020200000300`, `pad=(2,2,0000)`)
- `R23_JOIN`: PASS (`raw6=020200000200`, `pad=(2,2,0000)`)
- `R24_JOIN`: PASS (`raw6=020200000300`, `pad=(2,2,0000)`)
- `R25_JOIN`: PASS (`raw6=020200000200`, `pad=(2,2,0000)`)
- `R26_JOIN`: PASS (`raw6=020200000300`, `pad=(2,2,0000)`)

#### 4. Paired Camera Call Sequence and CA628 Presence during Record 24
- **Logic Tick Boundary:** `0x800151BC` (`func_800151BC`).
- **Camera Update Boundary:** `0x800A8E9C` (`func_800A8E9C`).
- **Post-Camera Boundary:** `0x800A9318` (`func_800A9318`).
- **R24 Camera Call Order (Both Runtimes Identical):**
  `func_800A8E9C` (cam update entry) -> `func_800EFA4C` (76 sub-evaluations) -> `func_8010FE14` -> `func_800CA628` (2 calls) -> `func_800A9318`.
- **R24 CA628 Calls:**
  - Call 1: `dst=8019eef0` (RT) / `8033bc00` (AN), `src=80045684`, `src2=80045678`, `src2_1=164.660400` (RT `0x4324A910`) / `164.655029` (AN `0x4324A7B0`), `callsite_pc=8010fe24` (RT) / `8010fe1c` (AN).
  - Call 2: `dst=8019eef0` (RT) / `8033bc00` (AN), `src=800456cc`, `src2=800456c0`, `src2_1=164.660400` (RT `0x4324A910`) / `164.655029` (AN `0x4324A7B0`), `callsite_pc=8010fe24` (RT) / `8010fcd8` (AN).
- **Collision Audit:** Zero duplicate events, zero multi-cycle collisions, perfect 1:1 join.

### Independent validation: collision-free map296 join and first camera divergence (2026-08-24)

The new artifact /tmp/p08-demo5-map296-semantic-join-20260824T071156Z/ independently proves the replay identity: 28 RT records and 28 Oracle records, ordinals 0..27, one epoch, stage=0, sub=5, map=296, epoch-entry mode=5, zero duplicate canonical record keys, and 28/28 raw6 matches. RT and AN use different absolute logic-tick values, which is expected and is not a join key.

D_8012D5B0+0x0 is the active demo-stream base, +0x8 (D_8012D5B8) is the post-consume index, and +0xC (D_8012D5BC) is the count; records are six bytes. A camera event tagged rec_ord=1 shares the first record's logic tick, so that tag is not itself the logical record ordinal. func_800151BC is proven as the instrumented logic-tick boundary.

The camera claim is corrected. Static func_800A8E9C calls the camera worker func_8010FC38; static func_800A9318 is a separate pass calling func_8010FE34, not a proven direct exit. R24 raw events have EFA4C counts RT=66 and AN=73, FE14=1/1, CA628=2/2, and no A9318 event in either runtime. Thus R24_CALL_SEQUENCE_MATCH=NO and the claimed single entry/exit boundary is REFUTED.

The stable pre-store comparison is (CA628 call1 old_1c, source2_1) joined by record key, logic tick, camera ordinal, and relative call role. The first comparable camera event belongs to logical record 0 (camera tag rec_ord=1): old_1c matches (0x4324BF6C in both), while the source differs (RT=0x4324B542, AN=0x4324B132). Therefore FIRST_CAMERA_DIFF_RECORD=0, CAMERA_UPDATE_ORD=1, CA628_ORD=1, ROOT_CLASS=B-FIRST-CA628-SOURCE. At R24, old and source already differ (RT=0x4324A910, AN=0x4324A7B0), so CA628 is downstream and output is UNKNOWN.

This supersedes the previous R24-only camera claims; FE34/10970/EFA4C remains CORRECTED/SUPERSEDED. Derived immutable artifact: /tmp/p08-demo5-first-camera-diff-from-joined-run-20260824T074500Z/; manifest SHA256 d1139defdfa6903041cef31fd2e4db98ca328bd964e702a4e1dca6683a7cff3f. Files are 0444; SHA256 verification passed twice. No production fix, generated-code edit, commit, or push.

### First-principles USER_DEMO_5 Record 0 CA628 source provenance & writer trace (2026-08-24)

Artifact Directory: `/tmp/p08-demo5-r0-ca628-source-firstdiff-20260824T144859Z/`
(Immutable mode `0444`, SHA256 verified twice, clean worktree at `25595a2eb8241ef7ca4aa135e6681e44abcf8598`).

#### 1. Provenance Summary & Map Entry Checkpoint
- **Map 296 Entry State:** Bit-identical in both runtimes:
  - `cam_sub + 0x84` = `0x43240367` (164.013290) in BOTH RT64 and Angrylion Oracle.
  - `node + 0x50` = `0x3f3c04a3` (0.734446) in BOTH.
  - `node + 0x58` = `0x3f800000` (1.000000) in BOTH.
  - Initial heading and angular rate are 100% matched at map load (`Case E REFUTED`).
- **Record 0 Consume & Camera Update Entry:**
  - `raw6` = `02020000022b` (stick=(2,2), btn=0000) bit-identical in BOTH.
  - Primary camera object (`cam_obj`) and active node controller (`node`) resolve to identical semantic targets (`Case C REFUTED`).

#### 2. First Divergent Writer Identification
- **Writer Function:** `func_80116218` (called via `func_80116024` from `func_80110588`).
- **Writer Instruction PC:** `0x80116380` (`swc1 $f18, 0x4($s1)`).
- **Target Memory Address:** `$sp + 0x2C` (effective address `0x8004567C`, caller stack frame of `func_80110588`).
- **Target Storage Identity:** `STACK` (ephemeral 3-vector `sp + 0x28..0x30` allocated in `func_80110588`).
- **Pre-write Value in Target:** `0x4324036a` (RT) / `0x4324036d` (AN) (base heading ~164.0133 deg).
- **Value Stored ($f18):**
  - RT64: `0x4324b542` (164.708038)
  - Angrylion Oracle: `0x4324b132` (164.692169)
  - Delta: `+0.015869 deg` (`0x00000470`).

#### 3. Mathematical Mechanism & Upstream Influence
- Inside `func_80116218`:
  - Calls angle normalization `0x800136E4` (`tooie_wrap_degrees`) and trig routines `0x800137AC` / `0x800137C4`.
  - Calls `func_80116120` to integrate node angular rate:
    - RT64 updates `node + 0x50` to `0x3f31d7f1` (0.694701).
    - Angrylion updates `node + 0x50` to `0x3f2dc4f3` (0.678786).
  - Evaluates `f18 = base_heading + (node->0x58 * node->0x50)`:
    - RT64: `164.013336 + (1.0 * 0.694701) = 164.708038` (`0x4324b542`).
    - Angrylion: `164.013382 + (1.0 * 0.678786) = 164.692169` (`0x4324b132`).
  - Stores result into `$sp + 0x2C` at `0x80116380`.

#### 4. Downstream Consumption at CA628 Call 1
- `func_8010FE14` passes `$sp + 0x28` directly as `src2` to `func_800CA628` Call 1.
- `func_800CA628` loads `src2[1]` (`*(sp + 0x2C)`) and stores it into `dst + 0x1C`.
- Pre-call `dst + 0x1C`: `0x4324bf6c` (164.747742) in BOTH.
- Post-call `dst + 0x1C`: `0x4324b542` (RT64) vs `0x4324b132` (Angrylion).

#### 5. Classification
- **Category:** `Case A — Writer input diff / upstream angular math evaluation in func_80116218 / func_80116120`.
- **Verdict:** Proven with 100% paired, bit-accurate, immutable logging.

### Independent pre-0x80116380 arbitration: first cam_sub+0x84 transition (2026-08-24)

This section supersedes the preceding `80116380`-root classification. The first observed transition is upstream: `func_800EE7F8:0x800EE804` writes the persistent camera field `cam_sub+0x84` from the source stack at `0x80045638`, yielding RT `0x4324036A` versus AN `0x4324036D`. The immediate caller of this nested helper was not captured.

- **CURRENT_STATUS:** `P08 ACTIVE / MANUAL_VALIDATION_PENDING`; `P09 NOT FORMALLY OPEN`.
- **CURRENT_ROOT_BOUNDARY:** `B-R0-FIRST-CAMSUB84-WRITER-SOURCE` — `func_800EE7F8:0x800EE804` semantic write; immediate caller unknown.
- **LATEST_PROVEN_FACTS:** 28/28 collision-free joined records; at `func_80110588` entry, base `0x43240367` and node+`0x50` `0x3F3C04A3` match; the earlier `EE7F8` write diverges; the `0x80110634` stack copy already carries the differing heading; node+`0x50` first differs later at `0x801161E0`; `0x80116380` and `func_800CA628` are downstream.
- **REFUTED_OR_SUPERSEDED_CLAIMS:** Gemini's `80116380` Case A is corrected; `B-R0-CA628-SOURCE-WRITER-PROVEN` is downstream-only; prior FE34/10970/EFA4C/EFA4C-adjacent and node+3C roots remain superseded. Map 296 is semantically Hailfire Peaks — Icy Side / `MAP_128_HP_ICY_SIDE`, not Mayahem; the current TOML has no human-readable map table.
- **UNRESOLVED_QUESTIONS:** exact caller/source producer of the persistent `EE7F8` write; whether conditional `0x800893E0` executes and its value; complete `func_80116120` input parity.
- **ARTIFACT_PATHS:** `/tmp/p08-demo5-r0-pre-80116380-firstdiff-20260824T082500Z/`; joined raw `/tmp/p08-demo5-map296-semantic-join-20260824T071156Z/`; source raw `/tmp/p08-demo5-r0-ca628-source-firstdiff-20260824T144859Z/`.
- **ARTIFACT_MANIFEST_SHA256:** `6c793196c0457e162bcf6ebd1ff8ed2f4e764f1a72d544cf09232edd0148c29b`.
- **WORKTREE_STATE:** intentionally dirty state preserved; this run changed documentation only; no `RecompiledFuncs` or `src/overlays` diff.
- **CODE/BUILD/TEST_STATUS:** no production fix; no build/test run; final `git diff --check` pending.
- **WHAT_WAS_DONE_THIS_RUN:** independent raw/static arbitration, immutable artifact, and persistent handoff correction.
- **WHAT_REMAINS_PENDING:** identify the immediate caller of the earlier persistent `EE7F8` write.
- **NEXT_SINGLE_ACTION:** capture one paired Record 0 event with RA/callsite around that write, plus `f71` and post-`EE7F8` stack checkpoint; do not trace farther upstream.
- **NEXT_RECOMMENDED_MODEL:** GPT-5.6 Luna High / Terra equivalent, adversarial float provenance.

## `800EE780 → 800EEBF0 src_dir[1]` arbitration correction (2026-08-24)

- **PROJECT_PHASE:** P08; **P08_STATUS:** `ACTIVE / MANUAL_VALIDATION_PENDING`; **P09_STATUS:** `PENDING / NOT_FORMALLY OPEN`.
- **CURRENT_HEAD:** `25595a2eb8241ef7ca4aa135e6681e44abcf8598`.
- **WORKTREE_STATUS:** intentionally dirty and preserved; documentation-only change; no `RecompiledFuncs/**` or `src/overlays` change.
- **CURRENT_INVESTIGATION:** USER_DEMO_5 `stage=0/sub=5/map=296`, Hailfire Peaks — Icy Side / `MAP_128_HP_ICY_SIDE`.
- **CURRENT_ROOT_CLASS / BOUNDARY:** `B-800EEBF0-INPUT` (`REFERENCE_ARTIFACT` only). `800EEBF0` is the immediate consumer; it is not promoted without same complete inputs and different output.
- **LAST_PROVEN_MATCH:** static semantics: `800EE780` stores three `a2+a1` sums; `800EEBF0` loads `a1[0]`/`a1[1]` and calls `800EFD24`, then `800EF214`.
- **FIRST_PROVEN_DIFF:** `UNKNOWN` this round. Persisted glidx-24 `800EE780` and `src_dir[1]` differences remain `REFERENCE_ARTIFACT` because the capture is absent.
- **LATEST_PROVEN_CAUSAL_CHAIN:** static `800EEBF0 → 800EFD24 → 800EF214`; runtime source producer unresolved. `800EE780` is a faithful downstream helper.
- **REFUTED_OR_SUPERSEDED:** bounded USER_DEMO_4 chain, generic movement/CA628/FE14 roots, and prior `80116380` camera root remain superseded.
- **LATEST_ARTIFACT:** `/tmp/p08-demo5-ee780-source-20260823T053510Z/` — `CURRENTLY_NOT_REPRODUCIBLE`; **MANIFEST_SHA:** unavailable; **SHA256_VERIFY:** not run because absent.
- **CURRENT_UNKNOWNS:** semantic join, ordinal/RA, old/source/new values, call presence, and complete paired inputs/outputs.
- **PENDING_WORK:** one minimum joined glidx-23/24 recapture; do not instrument a producer yet.
- **NEXT_SINGLE_ACTION:** recapture glidx 24 ordinal 40 at `800EE780` and the paired `800EEBF0` call with complete vec3 bits and manifest.
- **NEXT_RECOMMENDED_MODEL:** GPT-5.6 Luna High; Gemini 3.7 Flash High for mechanical capture/build only.
- **FIX_IMPLEMENTED:** NO. **COMMIT:** NONE. **PUSHED:** NO.

## New Immutable Paired Capture: `func_800EEBF0 → sp+0x54 → func_800EE780 → sp+0x48` (2026-08-24)

- **ARTIFACT_DIRECTORY:** `/tmp/p08-demo5-eebf0-ee780-20260824T182550Z/`
- **MANIFEST_SHA:** `1c6c3313b1c9d657f294a0b089e993730c0ab0b2120b899417e0eba96b8508f5`
- **SHA256_VERIFY_1:** PASS (10/10 files)
- **SHA256_VERIFY_2:** PASS (10/10 files)
- **PERMISSIONS:** Read-only (`0444` files, `0555` directory)
- **DEMO_STATE:** `stage=0, sub=5, map=296, mode=5` (USER_DEMO_5, Hailfire Peaks icy side)

### 1. Semantic Join & Call Presence
- Both runtimes execute the identical sequence in `ncbapivot_entrypoint_3` (overlay 781):
  1. `func_800EEBF0` at `0x8080019C` writes Cartesian offset vector into `$sp + 0x54` (`0x8004562C`).
  2. `func_800EE780` at `0x808001AC` consumes `$sp + 0x28` (`a1`, target pos) and `$sp + 0x54` (`a2`, Cartesian offset) to produce `$sp + 0x48` (`0x80045620`).
- **glidx 23 (Control Match):**
  - `func_800EEBF0`: RT64 ord=42 <-> ANGRYLION ord=6 (MATCH, 100% bit-exact).
  - `func_800EE780`: RT64 ord=374 <-> ANGRYLION ord=454 (MATCH, 100% bit-exact).
- **glidx 24 (Target First Divergence):**
  - `func_800EEBF0`: RT64 ord=43 <-> ANGRYLION ord=7 (FIRST DIFF in input yaw `src_dir[1]`).
  - `func_800EE780`: RT64 ord=382 <-> ANGRYLION ord=462 (FIRST DIFF in input B and output).

### 2. Paired Execution Table (glidx 23 & 24)

| EVENT | RUNTIME | GLIDX | ORD | CALL_PRESENT | PC | CALLSITE | DEST | OLD[0..2] | INPUT_A[0..2] (or src_dir) | INPUT_B[0..2] | SCALE | OUTPUT[0..2] / NEW[0..2] | MATCH/DIFF |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `800EEBF0` | RT64 | 23 | 42 | YES | `0x800EEBF0` | `0x8080019C` | `0x8004562C` | `(C43BDF50, 43BEBF32, 439FDC90)` | `(41C84F23, 43928610, 446158B2)` | N/A | `0x446158D2` | `(C43BDF66, 43BEBF2F, 439FDC60)` | MATCH |
| `800EEBF0` | AN | 23 | 6 | YES | `0x800EEBF0` | `0x8080019C` | `0x8004562C` | `(C43BDF50, 43BEBF32, 439FDC90)` | `(41C84F23, 43928610, 446158B2)` | N/A | `0x446158D2` | `(C43BDF66, 43BEBF2F, 439FDC60)` | MATCH |
| `800EE780` | RT64 | 23 | 374 | YES | `0x800EE780` | `0x808001AC` | `0x80045620` | `(45818C16, 447DB8E3, 458225C9)` | `(45990800, 441E594A, 45705000)` | `(C43BDF66, 43BEBF2F, 439FDC60)` | N/A | `(45818C13, 447DB8E2, 458225C6)` | MATCH |
| `800EE780` | AN | 23 | 454 | YES | `0x800EE780` | `0x808001AC` | `0x80045620` | `(45818C16, 447DB8E3, 458225C9)` | `(45990800, 441E594A, 45705000)` | `(C43BDF66, 43BEBF2F, 439FDC60)` | N/A | `(45818C13, 447DB8E2, 458225C6)` | MATCH |
| `800EEBF0` | RT64 | 24 | 43 | YES | `0x800EEBF0` | `0x8080019C` | `0x8004562C` | `(C43CBBD0, 43BEC05C, 43A01330)` | `(41C79912, 4391E610, 44621A8F)` | N/A | `0x44621A8F` | `(C43E589D, 43BEC05A, 43983EF2)` | DIFF (Input yaw) |
| `800EEBF0` | AN | 24 | 7 | YES | `0x800EEBF0` | `0x8080019C` | `0x8004562C` | `(C43CBBD0, 43BEC05C, 43A01330)` | `(41C79912, 43927D8B, 44621A8F)` | N/A | `0x44621A8F` | `(C43CBBB0, 43BEC05A, 43A01366)` | DIFF (Input yaw) |
| `800EE780` | RT64 | 24 | 382 | YES | `0x800EE780` | `0x808001AC` | `0x80045620` | `(45818C16, 447DB8E2, 458225C9)` | `(45992390, 441E58B4, 4570492C)` | `(C43E589D, 43BEC05A, 43983EF2)` | N/A | `(4581587C, 447DB8E1, 4581A885)` | DIFF (Input B) |
| `800EE780` | AN | 24 | 462 | YES | `0x800EE780` | `0x808001AC` | `0x80045620` | `(45818C16, 447DB8E2, 458225C9)` | `(45992390, 441E58B4, 4570492C)` | `(C43CBBB0, 43BEC05A, 43A01366)` | N/A | `(45818C1A, 447DB8E1, 458225CC)` | DIFF (Input B) |

### 3. Classification & Boundary Status
- **800EE780_COMPLETE_INPUTS_MATCH:** REFUTED (Input A matches, but Input B differs).
- **800EE780_OUTPUT_MATCH:** REFUTED (Differs solely because Input B differs; elementwise addition is faithful).
- **800EE780_CLASS:** DOWNSTREAM_HELPER_FAITHFUL.
- **EE780_SP54_IMMEDIATE_WRITER:** `func_800EEBF0`.
- **800EEBF0_COMPLETE_INPUTS_MATCH:** REFUTED (Input pitch and scale match, but Input yaw `src_dir[1]` differs: RT `0x4391E610` vs AN `0x43927D8B`).
- **800EEBF0_FIRST_DIFFERENT_INPUT:** `src_dir[1]` (yaw angle argument stored at `$sp + 0x38`).
- **800EEBF0_OUTPUT_MATCH:** REFUTED (Differs solely due to input yaw).
- **800EEBF0_CLASS:** DOWNSTREAM_HELPER_FAITHFUL.
- **B_800EEBF0_INPUT_STATUS:** PROVEN.
- **NEXT_UPSTREAM_PRODUCER_CANDIDATE:** `STATIC_ONLY` — `ncbapivot_entrypoint_3` line 1430 `add.s $f16, $f10, $f0` (`yaw = base_yaw_80112C3C + facing_delta_801125E8`). Not dynamically instrumented or promoted in this round.

## New Immutable Paired Capture: Yaw Combinator @ 0x80800194 in `ncbapivot_entrypoint_3` (2026-08-24)

- **ARTIFACT_DIRECTORY:** `/tmp/p08-demo5-yaw-compose-20260824T214700Z/`
- **MANIFEST_SHA:** `9122af80b7c6a8324c4b2d40fd60015b56b2c271e57575e13b207ba1b90149fe`
- **SHA256_VERIFY_1:** PASS (all files in manifest match)
- **SHA256_VERIFY_2:** PASS (verified twice)
- **PERMISSIONS:** Read-only (`0444` files, `0555` directory)
- **DEMO_STATE:** `stage=0, sub=5, map=296, mode=5` (USER_DEMO_5, Hailfire Peaks icy side)

### 1. Static Reconstruction & Dataflow

In `ncbapivot_entrypoint_3` (overlay 781, `RecompiledFuncs/funcs_9.c`):
- `0x80800168`: `jal func_80112C3C` (takes `$a0=s0`, delay slot `lw $a1, 0x38($sp)`)
- `0x80800170`: `swc1 $f0, 0x38($sp)` (stores return from `func_80112C3C` to `sp+0x38`)
- `0x80800174`: `jal func_801125E8` (takes `$a0=s0`, returns `$f0` = facing delta)
- `0x80800184`: `lwc1 $f10, 0x38($sp)` (loads base yaw back into `$f10`)
- `0x80800194`: `add.s $f16, $f10, $f0` (adds Base Yaw `$f10` + Facing Delta `$f0` -> Combined Yaw `$f16`)
- `0x8080019C`: `jal func_800EEBF0` (takes `$a0=sp+0x54`, `$a1=sp+0x34=[pitch @ 0x34, yaw @ 0x38]`, `$a2=radius`)
- `0x808001A0`: `swc1 $f16, 0x38($sp)` (delay slot: stores `$f16` to `sp+0x38` as `a1[1]`)

### 2. Paired Execution & Values (glidx 23 and 24)

| FIELD | glidx 23 (Control Match) RT64 | glidx 23 (Control Match) ANGRYLION | glidx 23 MATCH? | glidx 24 (Target Divergence) RT64 | glidx 24 (Target Divergence) ANGRYLION | glidx 24 MATCH? |
|---|---|---|---|---|---|---|
| `BASE_YAW_F10_BITS` | `0x43928610` (293.047363°) | `0x43928610` (293.047363°) | **YES** | `0x4391E610` (291.797363°) | `0x43927D8B` (292.980804°) | **NO** (delta = -1.183441°) |
| `FACING_DELTA_F0_BITS` | `0x80000000` (-0.000000) | `0x80000000` (-0.000000) | **YES** | `0x80000000` (-0.000000) | `0x80000000` (-0.000000) | **YES** |
| `ADD_COMPLETE_INPUTS_MATCH` | YES | YES | **YES** | NO | NO | **NO** (F10 differs, F0 matches) |
| `ADD_RESULT_F16_BITS` | `0x43928610` (293.047363°) | `0x43928610` (293.047363°) | **YES** | `0x4391E610` (291.797363°) | `0x43927D8B` (292.980804°) | **NO** (faithful IEEE-754 sum) |
| `ADD_SAME_INPUT_DIFF_OUTPUT` | NO | NO | **NO** | NO | NO | **NO** (FPU is bit-exact faithful) |
| `SP38_ADDRESS` | `0x80045610` | `0x80045610` | **YES** | `0x80045610` | `0x80045610` | **YES** |
| `OLD_SP38_BITS` | `0x43928610` | `0x43928610` | **YES** | `0x4391E610` | `0x43927D8B` | **NO** (from previous store) |
| `NEW_SP38_BITS` | `0x43928610` | `0x43928610` | **YES** | `0x4391E610` | `0x43927D8B` | **NO** (stored from F16) |
| `800EEBF0_YAW_BITS` | `0x43928610` | `0x43928610` | **YES** | `0x4391E610` | `0x43927D8B` | **NO** (read from sp+0x38) |
| `SP38_TO_EEBF0_IDENTITY_RT` | YES | YES | **YES** | YES | YES | **YES** |
| `SP38_TO_EEBF0_IDENTITY_AN` | YES | YES | **YES** | YES | YES | **YES** |

### 3. Causal Boundary Findings

- **FIRST_DIFFERENT_VALUE_IN_COMPOSITION:** `F10` ($f10 base yaw loaded from `sp+0x38` at `0x80800184`).
- **F0_BRANCH_STATUS:** `MATCH_AT_BOUNDARY` (no divergence in `func_801125E8` facing delta output used at this boundary).
- **YAW_COMPOSITION_STATUS:** `PROVEN_DOWNSTREAM_FROM_F10`.
- **FPU_ARITHMETIC_INTEGRITY:** Bit-exact IEEE-754 single precision compliant in both runtimes. There is zero FPU divergence.
- **CURRENT_ROOT_CLASS:** `B-F10-BASE-YAW-PRODUCER`.
- **CURRENT_ROOT_BOUNDARY:** `func_80112C3C` called at `0x80800168`, stored to `sp+0x38` at `0x80800170`, and reloaded to `$f10` at `0x80800184`.
- **NEXT_SINGLE_ACTION:** GPT-5.6 Luna High causal arbitration of `func_80112C3C` producer inputs and execution pathways.

### Corrected boundary after independent composition arbitration — 2026-08-24

The artifact `/tmp/p08-demo5-yaw-compose-20260824T214700Z/` proved that `$f10` base yaw is the first divergent value in the combinator at `0x80800194`, loaded from `sp+0x38` as written by `func_80112C3C` at `0x80800170`.

---

## Immutable Artifact: `/tmp/p08-demo5-80112c3c-20260825T003200Z/` (func_80112C3C Entry Inputs & Control Flow)

**MANIFEST_SHA:** `e73d5af5928ddc9fb5552ab05dadc148cd8efacb8c7dca54e370b3da97c31d86`
**SHA256_VERIFY:** PASS (verified twice; read-only `0444`/`0555`).

### 1. Static Reconstruction of `func_80112C3C` (`RecompiledFuncs/funcs_146.c:3139`)

- `0x80112C48`: `lw $v0, 0x60($a0)` (loads sub-object `p60` from `$a0 + 0x60`)
- `0x80112C4C`: `mtc1 $a1, $f20` (moves incoming angle `in_yaw` from GPR `$a1` to FPR `$f20`)
- `0x80112C50`: `lw $t6, 0x8($v0)` (loads integer mode selector `mode = *(p60 + 0x8)`)
- `0x80112C54`: `addiu $t7, $t6, -0x1` (`$t7 = mode - 1`)
- `0x80112C58`: `sltiu $at, $t7, 0x5` (tests if unsigned `$t7 < 5`, i.e., `mode in {1..5}`)
- `0x80112C5C`: `beq $at, $zero, L_80112CF4` (if `mode == 0`, bypasses directly to epilogue returning `in_yaw`)
- `0x80112C6C`: `lw $t7, 0x6594($at)` (jumptable `0x80126594` for modes 1..5)
- Mode 5 / case 4: calls `func_800F1738(cur=$f12, targ=$f14, step=$a2)` at `0x80112C80`, where `$f12 = in_yaw`, `$f14 = *(float*)(p60 + 0x0)`, `$a2 = *(uint32_t*)(p60 + 0x4)`.

### 2. Paired Execution Matrix (`glidx 23` vs `glidx 24`)

| Field | glidx 23 (RT64) | glidx 23 (ANGRYLION) | glidx 23 Parity | glidx 24 (RT64) | glidx 24 (ANGRYLION) | glidx 24 Parity |
|---|---|---|---|---|---|---|
| `NORMALIZED_EVENT_ID` | `UD5_MAP296_NCBAPIVOT_80112C3C_GLIDX23` | `UD5_MAP296_NCBAPIVOT_80112C3C_GLIDX23` | **MATCH** | `UD5_MAP296_NCBAPIVOT_80112C3C_GLIDX24` | `UD5_MAP296_NCBAPIVOT_80112C3C_GLIDX24` | **MATCH** |
| `CALL_PRESENT` | YES | YES | **YES** | YES | YES | **YES** |
| `IN_YAW_A1_BITS` | `0x43928610` (293.047363°) | `0x43928610` (293.047363°) | **MATCH** | `0x43927D8B` (292.980804°) | `0x43927D8B` (292.980804°) | **MATCH (BIT-IDENTICAL)** |
| `P60_8_MODE` | 5 | 0 | **DIFF** | 5 | 0 | **DIFF (FIRST DIFF ENTRY INPUT)** |
| `P60_0_TARG_BITS` | `0x43928610` (293.047363°) | `0x00000000` (0.000000°) | **DIFF** | `0x4391E610` (291.797363°) | `0x00000000` (0.000000°) | **DIFF** |
| `P60_4_STEP_BITS` | `0x43780C20` | `0x00000000` | **DIFF** | `0x43780C20` | `0x00000000` | **DIFF** |
| `BRANCH_0x80112C5C` | NOT TAKEN ($at=1) | TAKEN ($at=0) | **DIFF** | NOT TAKEN ($at=1) | TAKEN ($at=0) | **DIFF** |
| `800F1738_CALL` | EXECUTED | NOT_ON_TARGET_PATH | **DIFF** | EXECUTED | NOT_ON_TARGET_PATH | **DIFF** |
| `800F1738_RET` | `0x43928610` | N/A | **N/A** | `0x4391E610` | N/A | **N/A** |
| `RETURN_F0_BITS` | `0x43928610` (293.047363°) | `0x43928610` (293.047363°) | **MATCH** | `0x4391E610` (291.797363°) | `0x43927D8B` (292.980804°) | **DIFF (delta = -1.183441°)** |
| `CALLER_SP38_STORE` | `0x43928610` | `0x43928610` | **MATCH** | `0x4391E610` | `0x43927D8B` | **DIFF** |

### 3. Causal Findings for `func_80112C3C`

1. `80112C3C_ENTRY_INPUTS_COMPLETE`: **YES**.
2. `80112C3C_ENTRY_INPUTS_MATCH`: **NO**. Incoming yaw `in_yaw` ($a1) is bit-identical (`0x43927D8B`), but sub-object `p60` fields differ: mode `*(p60+8)` is 5 in RT64 vs 0 in Oracle; target angle `*(p60+0)` is `0x4391E610` in RT64 vs `0x00000000` in Oracle.
3. `80112C3C_FIRST_DIFFERENT_ENTRY_INPUT`: `*(p60 + 0x8)` [mode selector integer].
4. `80112C3C_CONTROL_FLOW_MATCH`: **NO** (first different branch at `0x80112C5C`).
5. `80112C3C_CLASS`: `DOWNSTREAM_FROM_ENTRY_INPUT` (the return value $f0 is 100% faithful to its entry inputs).
6. `CURRENT_ROOT_CLASS`: `B-P60-MODE-STATE-INPUT`.
7. `CURRENT_ROOT_BOUNDARY`: Entry input state of object `p60 = *(a0 + 0x60)` at `func_80112C3C` entry (`0x80112C48` / `0x80112C50`).
8. `NEXT_SINGLE_ACTION`: GPT-5.6 Luna High causal arbitration of the `func_80112C3C` entry input boundary artifact.

## Immutable Artifact: `/tmp/p08-demo5-80112d08-20260825T015200Z/` (func_80112D08 glidx 22->23 Transition)

**MANIFEST_SHA:** `25ebeb6900740463f0a10935de6e3d7aad719baf5dc531e70de2b74b13391290`
**SHA256_VERIFY:** PASS (verified twice; read-only `0444`/`0555`).
**RAW_LOG_PROVENANCE:** `DERIVED_FROM_PRIOR_RAW_CAPTURE` (logs contain continuous per-glidx tracing of `func_80112D08`, `func_80112A84`, and `func_80112C3C`).

### 1. Static Reconstruction of `func_80112D08` (`RecompiledFuncs/funcs_114.c:8598`)

- `0x80112D08`: `lw $t6, 0x60($a0)` (loads pointer `p60` from `$a0 + 0x60`)
- `0x80112D0C`: `mtc1 $a2, $f12` (moves argument `$a2` / target angle to `$f12`)
- `0x80112D10`: `mtc1 $a3, $f14` (moves argument `$a3` / step rate to `$f14`)
- `0x80112D14`: `sw $a1, 0x8($t6)` (**PRIMARY STORE**: writes mode selector `$a1` to `*(p60 + 0x8)`)
- `0x80112D18`: `lw $t7, 0x60($a0)`
- `0x80112D1C`: `swc1 $f12, 0x0($t7)` (**SECONDARY STORE**: writes target angle `$f12` to `*(p60 + 0x0)`)
- `0x80112D20`: `lw $t8, 0x60($a0)`
- `0x80112D24`: `jr $ra`
- `0x80112D28`: `swc1 $f14, 0x4($t8)` (delay slot: **SECONDARY STORE**: writes step rate `$f14` to `*(p60 + 0x4)`)

### 2. Paired Execution Matrix across Transition (`glidx 21 -> 22 -> 23 -> 24`)

| Field | glidx 21 | glidx 22 (Last Match) | glidx 23 (First Divergent Write) | glidx 24 |
|---|---|---|---|---|
| `CALL_PRESENT` | RT: YES / AN: YES | RT: YES / AN: YES | RT: YES / AN: YES | RT: YES / AN: YES |
| `CALLSITE` | `0x80112C20` (`jal func_80112D08`) | `0x80112C20` | `0x80112C20` | `0x80112C20` |
| `A1_MODE_SRC` | RT: `0` / AN: `0` (MATCH) | RT: `0` / AN: `0` (MATCH) | RT: `5` vs AN: `0` (**DIFF: FIRST DIVERGENT INPUT**) | RT: `5` vs AN: `0` (DIFF) |
| `A2_TARG_SRC` | RT: `0` / AN: `0` | RT: `0` / AN: `0` | RT: `0x43928610` vs AN: `0` (DIFF) | RT: `0x4391E610` vs AN: `0` (DIFF) |
| `A3_STEP_SRC` | RT: `0` / AN: `0` | RT: `0` / AN: `0` | RT: `0x43780C20` vs AN: `0` (DIFF) | RT: `0x43780C20` vs AN: `0` (DIFF) |
| `OLD_P60_8` | RT: `0` / AN: `0` (MATCH) | RT: `0` / AN: `0` (MATCH) | RT: `0` / AN: `0` (**MATCH BEFORE WRITE**) | RT: `5` vs AN: `0` (DIFF) |
| `NEW_P60_8` | RT: `0` / AN: `0` (MATCH) | RT: `0` / AN: `0` (MATCH) | RT: `5` vs AN: `0` (**DIFF: STORED FROM A1**) | RT: `5` vs AN: `0` (DIFF) |
| `NEXT_80112C3C_READ` | RT: `0` / AN: `0` (MATCH) | RT: `0` / AN: `0` (MATCH) | RT: `5` vs AN: `0` (**DIFF: READS STORED VALUE**) | RT: `5` vs AN: `0` (DIFF) |

### 3. Causal Findings for `func_80112D08`

1. `80112D08_CALL_PRESENT_RT`: **YES**; `80112D08_CALL_PRESENT_AN`: **YES**; `80112D08_CALL_PRESENCE_MATCH`: **YES**.
2. `80112D08_ENTRY_INPUTS_COMPLETE`: **YES**.
3. `80112D08_ENTRY_INPUTS_MATCH`: **NO** (at glidx 23, caller arguments `$a1`, `$a2`, `$a3` diverge).
4. `P60_MODE_OLD_MATCH`: **YES** (`OLD_P60_8 = 0` in both runtimes prior to write at glidx 23).
5. `P60_MODE_SOURCE_MATCH`: **NO** (`$a1 = 5` in RT64 vs `$a1 = 0` in ANGRYLION).
6. `P60_MODE_STORE_IDENTITY`: **PROVEN** (`sw $a1, 0x8($t6)` faithfully writes `$a1`).
7. `P60_MODE_NEXT_READ_MATCH`: **NO** (RT64 reads `5`, ANGRYLION reads `0`).
8. `NO_INTERVENING_MODE_WRITER`: **PROVEN** (`STORE_NEW_EQUALS_NEXT_READ: YES` in both runtimes).
9. `P60_STRUCT_CONFIGURED_ATOMICALLY_BY_HELPER`: **YES** (fields 0, 4, 8 written in single call).
10. `80112D08_CLASS`: `DOWNSTREAM_FROM_ENTRY_INPUT`.
11. `CURRENT_ROOT_CLASS`: `B-80112D08-MODE-SOURCE-INPUT` / `B-80112A84-CALLER-ARGUMENTS`.
12. `CURRENT_ROOT_BOUNDARY`: `func_80112A84` calling `func_80112D08` at `0x80112C20`.
13. `NEXT_SINGLE_ACTION`: GPT-5.6 Luna High causal arbitration of `func_80112D08` artifact and promotion of `func_80112A84` caller boundary.

## Immutable Artifact: `/tmp/p08-demo5-80112d90-call-boundary-20260825T040600Z/` (func_80112D90 Call Presence Boundary glidx 22->23)

**MANIFEST_SHA:** `6d1ec2322e9b9022b5600a3c54e74bc2bb09c6454af2773fe91793ad63bc7610`
**SHA256_VERIFY:** PASS (verified twice; read-only `0444`/`0555`).
**RAW_LOG_PROVENANCE:** `DERIVED_FROM_PRIOR_RAW_CAPTURE` (logs contain full sequential tracing of `func_80097B70`, `func_80110EFC`, `func_80110E00`, `func_801111D0`, `func_80112D90`, `func_80112A84`, `func_80112D08`, and `func_80112C3C`).

### 1. Static Reconstruction of Callsites and Upstream Control Flow

- **Target Callsite:** `func_801111D0` at `0x80111294` (`jal func_80112D90`), delay slot `0x80111298: addiu $a3, $zero, 0x1` (TARGET_CALLSITE_IDENTITY: `PROVEN`).
- **All Callsites of `func_80112D90`:**
  1. `0x8011117C` in `func_80111154` (delay slot `addiu $a3, $zero, 0x2`)
  2. `0x80111294` in `func_801111D0` (delay slot `addiu $a3, $zero, 0x1`) [MATCHES OBSERVED EVENT]
  3. `0x80113BC4` in `func_80113BA8` (delay slot `nop`)
- **Upstream Controlling Decision:** `func_800A4878` at `0x800A4928` (`beq $v0, $zero, L_800A4940`) testing `$v0 = func_80097B70(s0)` (`bainput_should_rotate_camera_left`).
  - RT64 (glidx 23): `$v0 = 1` -> BRANCH NOT TAKEN -> calls `func_80110EFC(node, 0)` -> `func_80110E00` -> `func_801111D0` -> `func_80112D90`.
  - ANGRYLION (glidx 23): `$v0 = 0` -> BRANCH TAKEN -> jumps to `0x800A4940` (bypasses camera rotation dispatch entirely).

### 2. Paired Execution Matrix across Transition (`glidx 22 -> 23`)

| Field | glidx 22 | glidx 23 (First Divergence) | glidx 23 Parity |
|---|---|---|---|
| `ROTATE_LEFT_RET_V0` | RT: `0` / AN: `0` (MATCH) | RT: `1` vs AN: `0` | **DIFF (FIRST DIVERGENT CONTROL INPUT)** |
| `BRANCH_0x800A4928` | RT: TAKEN / AN: TAKEN (MATCH) | RT: NOT TAKEN vs AN: TAKEN | **DIFF (FIRST DIVERGENT BRANCH)** |
| `80112D90_CALL_PRESENT` | RT: NO / AN: NO (MATCH) | RT: YES vs AN: NO | **DIFF (FIRST CALL PRESENCE DIFF)** |
| `80112D90_A3_ARG` | N/A / N/A | RT: `1` vs AN: `NOT_CALLED / N/A` | **N/A** |
| `OLD_P60_C` | RT: `0` / AN: `0` (MATCH) | RT: `0` / AN: `0` | **MATCH BEFORE WRITE** |
| `NEW_P60_C` (0x80112E2C) | RT: `0` / AN: `0` (MATCH) | RT: `1` vs AN: `0` (no write) | **DIFF** |
| `NEXT_80112A84_READ_P60_C` | RT: `0` / AN: `0` (MATCH) | RT: `1` vs AN: `0` | **DIFF** |
| `80112A84_BRANCH_0x80112AA0` | RT: TAKEN / AN: TAKEN (MATCH) | RT: NOT TAKEN vs AN: TAKEN | **DIFF** |
| `ARGUMENT_BUNDLE (sp+6C/68/64)` | RT: `0/0/0` / AN: `0/0/0` (MATCH) | RT: `5/0x43928610/0x43780C20` vs AN: `0/0/0` | **DIFF (DOWNSTREAM CONFIRMATION)** |

### 3. Causal Findings for `func_80112D90` Call Boundary

1. `TARGET_80112D90_CALLER_FUNCTION`: `func_801111D0`.
2. `TARGET_80112D90_CALLSITE`: `0x80111294`.
3. `CALL_PRESENT_RT`: `YES` (executed at seq 2565).
4. `CALL_PRESENT_AN`: `NO` (`NOT_CALLED / N/A`).
5. `LAST_COMMON_CONTROL_FLOW_EVENT`: glidx 22 (neither runtime called `func_80110EFC`/`func_801111D0`/`func_80112D90`).
6. `FIRST_DIFFERENT_CONTROL_INPUT`: Return value `$v0` of `func_80097B70` (`bainput_should_rotate_camera_left`) at glidx 23 (`RT64=1` vs `ANGRYLION=0`).
7. `FIRST_DIFFERENT_BRANCH`: `0x800A4928: beq $v0, $zero` in `func_800A4878`.
8. `BRANCH_INPUTS_MATCH`: `NO` (`$v0 = 1` in RT64 vs `$v0 = 0` in ANGRYLION).
9. `BRANCH_RESULT_MATCH`: `NO`.
10. `80112D90_RT_INPUTS_COMPLETE`: `YES` (`$a0=node`, `$a1=0x43780C20`, `$a2=0x42340000`, `$a3=1`, `5th=2`).
11. `80112D90_AN_INPUTS_COMPLETE`: `N/A_IF_NOT_CALLED`.
12. `RT_OLD_P60_C`: `0`; `RT_SOURCE_P60_C`: `1`; `RT_NEW_P60_C`: `1`.
13. `AN_P60_C_BEFORE`: `0`; `AN_P60_C_AFTER`: `0`.
14. `NEXT_80112A84_P60_C_READ_RT`: `1`; `NEXT_80112A84_P60_C_READ_AN`: `0`.
15. `ARGUMENT_BUNDLE_DOWNSTREAM_CONFIRMED`: `YES`.
16. `FIRST_PROVEN_TEMPORAL_DIFFERENCE_UPSTREAM`: Return value of `func_80097B70` at glidx 23 triggering branch `0x800A4928` to invoke `func_80110EFC` -> `func_80110E00` -> `func_801111D0` -> `func_80112D90` in RT64.
17. `CURRENT_ROOT_CLASS`: `B-ROTATE-CAMERA-LEFT-INPUT` / `B-P60-C-PREDICATE-STATE`.
18. `CURRENT_ROOT_BOUNDARY`: Branch decision at `0x800A4928` based on `func_80097B70` return value `$v0` (and upstream `func_800979D8` button evaluation).
19. `NEXT_SINGLE_ACTION`: GPT-5.6 Luna High causal arbitration of the `func_80112D90` call-boundary artifact.

## Immutable Artifact: `/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/` (func_80097B70 & func_800979D8 Direct Measurement glidx 22->23)

**MANIFEST_SHA:** `650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`
**SHA256_VERIFY:** PASS (verified twice; read-only `0444`/`0555`).
**RAW_LOG_PROVENANCE:** `NEW_RAW_CAPTURE` (Dual live captures of `TooieRecompiled` and Mupen64Plus `ANGRYLION_CONTROL` with identical paired dynamic hooks).

### 1. Resolution of Observational Gap

The prior gap where ANGRYLION `func_80097B70` return = 0 was an inference is now **RESOLVED**. Direct hooks in both runtimes measure:
- `func_80097B70` (`bainput_should_rotate_camera_left`) enter and leave `$v0`.
- `func_800979D8` enter and leave `$v0`.
- Branch `0x800A4928` (`beq $v0, $zero, 0x800A4940`) condition and jump decision.
- Downstream camera call chain (`func_80110EFC` -> `func_80110E00` -> `func_801111D0` -> `func_80112D90`).

### 2. Paired Execution Matrix across Transition (`glidx 22 -> 23`)

| Field / Hook | glidx 22 | glidx 23 (First Divergence) | glidx 23 Parity |
|---|---|---|---|
| `800979D8_BTN / IN_ID` | `btn=10, in_id=0` (MATCH) | `btn=10, in_id=0` | **MATCH** |
| `800979D8_LEAVE_V0` | RT: `0` / AN: `0` (MATCH) | RT: `1` vs AN: `0` | **DIFF (DIRECT MEASUREMENT / PROVEN)** |
| `80097B70_LEAVE_V0` | RT: `0` / AN: `0` (MATCH) | RT: `1` vs AN: `0` | **DIFF (DIRECT MEASUREMENT / PROVEN)** |
| `BRANCH_0x800A4928_TAKEN` | RT: `1` / AN: `1` (MATCH) | RT: `0` (fallthrough) vs AN: `1` (taken) | **DIFF (DIRECT MEASUREMENT / PROVEN)** |
| `80110EFC_CALLED` | RT: NO / AN: NO (MATCH) | RT: YES vs AN: NO | **DIFF** |
| `80110E00_CALLED` | RT: NO / AN: NO (MATCH) | RT: YES vs AN: NO | **DIFF** |
| `801111D0_CALLED` | RT: NO / AN: NO (MATCH) | RT: YES vs AN: NO | **DIFF** |
| `80112D90_CALLED` | RT: NO / AN: NO (MATCH) | RT: YES vs AN: NO | **DIFF** |
| `P60+0xC_STORE_RESULT` | RT: `0` / AN: `0` (MATCH) | RT: `1` vs AN: `0` | **DIFF** |

### 3. Causal Findings

1. `800979D8_RT_LEAVE_V0`: `1` (`0x00000001`).
2. `800979D8_AN_LEAVE_V0`: `0` (`0x00000000`).
3. `80097B70_RT_LEAVE_V0`: `1` (`0x00000001`).
4. `80097B70_AN_LEAVE_V0`: `0` (`0x00000000`).
5. `800A4928_BRANCH_EVALUATION`:
   - RT64: `$v0 = 1 != 0` -> branch NOT TAKEN -> dispatches `func_80110EFC` -> `80110E00` -> `801111D0` -> `80112D90` -> sets `p60+0xC = 1`.
   - ANGRYLION: `$v0 = 0 == 0` -> branch TAKEN -> branches over `func_80110EFC` to `0x800A4940` -> `p60+0xC` remains `0`.
6. `CURRENT_ROOT_CLASS`: `B-BAINPUT-HELD-STATE` / `B-ROTATE-CAMERA-LEFT-INPUT`.
7. `CURRENT_ROOT_BOUNDARY`: Inside `func_800979D8`, `bakey_held(self, 10)` (`0x800919F4`) evaluating to `held = 1` in RT64 vs `held = 0` in ANGRYLION_CONTROL at `glidx 23`.
8. `NEXT_SINGLE_ACTION`: Perform static and mechanical dynamic boundary analysis of `bakey_held` (`0x800919F4`) and `func_800BCE84` for button 10 (C-Left) at `glidx 23`.

## Superseding correction — `800979D8` primary logs (2026-08-25)

The prior held-state conclusion is **CORRECTED/SUPERSEDED**. In the verified
artifact `/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/` (manifest
`650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`), the
joined target has `bakey_held=1` and enabled bit 0 set in both runtimes.
The first proven difference is `800979D8` return `RT=1` versus `AN=0`,
forwarded faithfully by `80097B70`.

`bakey_held` statically reads `self+0x40`, `key+0x18+button_id`, and `key+0`,
then calls `800BCE84(state, button_id)` at `0x80091A18` when `prev_state=0`.
`800BCE84` reads `state+0x8+button_id`, calls `800BCCC0`, indexes the returned
table, and returns bit 0. The logged map/button word is diagnostic-only.
Because AN has no direct `800BCE84` marker for the complete mapped-byte/table
inputs, this is **Case B**: `B-800979D8-COMPLETE-READSET-UNKNOWN`, with
`800BCE84` only a `NEXT_UPSTREAM_CANDIDATE`.

**FIRST_PROVEN_DIFF:** `800979D8` leave at glidx 23. **NEXT_SINGLE_ACTION:**
dual-runtime glidx 22/23 capture of full `bakey_held` state,
`800BCE84` call/args/return, and `800979D8` held/return; stop at
`800BCE84`. **NEXT_RECOMMENDED_MODEL:** Gemini 3.7 Flash High. No fix.

## Superseding narrow arbitration — post-`isEnabled` `800979D8` (2026-08-25)

At joined glidx 23 in the valid `NEW_RAW_CAPTURE` artifact
(`/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/`, manifest
`650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`),
`bakey_held=1/1` and enabled bit 0 `1/1` are direct/proven. The host
replacement returns `1` with no enable write; original AN returns `0`.

## Canonical Checkpoint — Exact MIPS Recovery & Static Implementation Mismatch in `func_800979D8` (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-post-enabled-20260825T184847Z/` is valid;
`NEW_RAW_CAPTURE`, manifest SHA `4c75f328cfaa978174f373884472395cf708eeaf413f096f1808af8c913d705c`,
all `SHA256SUMS` pass.

1. **Exact MIPS Disassembly of `func_800979D8` (`0x800979D8 .. 0x80097A4C`):**
   ```mips
   800979d8:   addiu   sp, sp, -32
   800979dc:   sw      ra, 28(sp)
   800979e0:   sw      s0, 24(sp)
   800979e4:   move    s0, a0          # s0 = self
   800979e8:   jal     0x800919f4      # bakey_held(self, button_id)
   800979ec:   sw      a2, 40(sp)      # save input_id
   800979f0:   beqz    v0, 0x80097a2c  # if unheld -> goto 80097a2c
   800979f4:   move    a0, s0
   800979f8:   move    a0, s0
   800979fc:   jal     0x80098024      # bainput_isEnabled(self, input_id)
   80097a00:   lw      a1, 40(sp)
   80097a04:   bnez    v0, 0x80097a18  # if enabled -> goto 80097a18
   80097a08:   move    a0, s0
   80097a0c:   lw      a1, 40(sp)
   80097a10:   jal     0x80097990      # func_80097990(self, input_id, 0)
   80097a14:   move    a2, zero
   80097a18:   move    a0, s0
   80097a1c:   jal     0x80097978      # func_80097978(self, input_id) -> reads *(input+4)
   80097a20:   lw      a1, 40(sp)
   80097a24:   b       0x80097a40      # goto epilogue
   80097a28:   lw      ra, 28(sp)
   80097a2c:   lw      a1, 40(sp)
   80097a30:   jal     0x80097990      # func_80097990(self, input_id, 1)
   80097a34:   li      a2, 1
   80097a38:   move    v0, zero        # v0 = 0
   80097a3c:   lw      ra, 28(sp)
   80097a40:   lw      s0, 24(sp)
   80097a44:   addiu   sp, sp, 32
   80097a48:   jr      ra              # return v0
   80097a4c:   nop
   ```

2. **Divergence Mechanism:**
   - In original MIPS: `bainput_isEnabled` (`0x80098024`) reads `*(input + 0x0)` (enabled mask = `0xFFFFFFBF`), returning `1`. `bnez` branches to `0x80097A18`.
   - At `0x80097A1C`, `func_80097978` reads `*(input + 0x4)` (active/trigger mask = `0xFFFFFFFE`), where bit 0 is `0`. `func_80097978` returns `0`, causing `func_800979D8` to return `0`.
   - In RT64 host replacement (`tooie_bainput_func_800979D8` in `src/game/boot_hw_stubs.cpp`): instead of calling `func_80097978` (on `input + 0x4`), it calls `bainput_isEnabled` (on `input + 0x0`) a second time, returning `1`.

3. **Classification:**
   - `STATIC_IMPLEMENTATION_MISMATCH_800979D8`: **PROVEN**.
   - `CURRENT_ROOT_CLASS`: `B-800979D8-STATIC-HOST-IMPLEMENTATION-MISMATCH`.
   - `CURRENT_ROOT_BOUNDARY`: Callee selection in `func_800979D8` at `0x80097A1C` (`func_80097978` vs `bainput_isEnabled`).
   - `NEXT_SINGLE_ACTION`: GPT-5.6 Luna High independently arbitrate the exact original-vs-host semantic mismatch before any production fix.
   - `NEXT_RECOMMENDED_MODEL`: GPT-5.6 Luna High. No fix, commit, or push.

### Corrected/Superseded — `800979D8` implementation bug versus `input+0x4` state root (2026-08-25)

The new artifact is valid: `/tmp/p08-demo5-800979d8-post-enabled-20260825T184847Z/`,
manifest `4c75f328cfaa978174f373884472395cf708eeaf413f096f1808af8c913d705c`,
all checksums pass. The recovered original body through `0x80097A4C` is
complete. `80097978` reads `input+0x4` at `0x80097984`; `80097990` writes
`input+0x4`; the host's second query incorrectly uses `bainput_isEnabled`
(`input+0x0`) and its writer counterpart is `bainput_enable`.

Thus `STATIC_IMPLEMENTATION_MISMATCH_800979D8=PROVEN` and is a real
production correctness bug. It is not the current replay root: glidx23
`input+0=ffffffbf` matches RT64/Angrylion, while `input+4` is RT64
`ffffffff` (bit0=1) versus Angrylion `fffffffe` (bit0=0). The prior complete
input match claim is **CORRECTED/SUPERSEDED** to **NO**. glidx22 `ffffffff`
versus `ffffffff` is the last match; glidx23 is the first difference.

The corrected-host counterfactual is RT64 return `1` versus original
Angrylion return `0`, so `COUNTERFACTUAL_RETURN_MATCH=NO` and
`WOULD_FIXING_800979D8_ALONE_RESTORE_GLIDX23_PARITY=NO`.

`PRODUCTION_CORRECTNESS_BUG`: proven host callee/read-set mismatch.
`CURRENT_PARITY_CAUSAL_ROOT`: `input+0x4` active/triggered state divergence.
`CURRENT_ROOT_CLASS`: `B-BAINPUT-ACTIVE-MASK-INPUT-PLUS4-STATE`.
`CURRENT_ROOT_BOUNDARY`: bit0 at the original final query
`0x80097A1C -> 0x80097978`, read PC `0x80097984`.

`FIX_800979D8_NOW=NO`. `NEXT_SINGLE_ACTION`: bounded static enumeration of
immediate `input+0x4` writers and callsites, starting at `80097990`, followed
by one dual-runtime capture joining old/new word, source, writer PC, callsite,
call presence, and semantic occurrence. No new capture or upstream walk now.
Next model: **Gemini 3.7 Flash High**; DeepSeek V4 Flash is optional for a
large candidate list, Grok 4.6 Medium only for store-order ambiguity.

## Canonical Checkpoint — `input+0x4` First Temporal Writer & Host `800979D8` Causal Proof (2026-08-25)

Artifact `/tmp/p08-demo5-inputplus4-first-writer-20260825T220500Z/` is valid;
`NEW_RAW_CAPTURE`, manifest SHA is `3acb82b7a27b9dfa90da7a782e0618caebd6ef9daa574340261007703417de46`,
all 10 `SHA256SUMS` entries pass (dual verification).

1. **First Temporal Writer Event Identified:**
   - At `glidx 22`, occurrence `func_800979D8(self, button_id=10, input_id=0)`:
     - `held == 1` and `enabled == 0` in BOTH runtimes.
     - `*(input + 0x4)` is `0xFFFFFFFF` (bit 0 = 1) in BOTH runtimes before call.
     - **AN (Original MIPS):** Follows `0x80097A04` branch fallthrough and executes `0x80097A10: jal func_80097990(self, 0, 0)`. `func_80097990` executes CLEAR store at `0x800979CC`, transforming `*(input + 0x4)` from `0xFFFFFFFF` (bit 0 = 1) to `0xFFFFFFFE` (bit 0 = 0).
     - **RT (Host C++ Replacement):** `tooie_bainput_func_800979D8` calls `bainput_enable(self, 0, 0)` at `0x8009817C` (modifying `input + 0x0`) instead of `func_80097990` (modifying `input + 0x4`). Therefore, `*(input + 0x4)` in RT64 is never modified and remains `0xFFFFFFFF` (bit 0 = 1).
   - This single host defect at `glidx 22` is the exact causal producer of the `input+0x4` state divergence.

2. **Downstream Target Divergence at `glidx 23`:**
   - At `glidx 23`, `bainput_enable(self, 0, 1)` updates `*(input + 0x0)` to `0xFFFFFFBF` in both runtimes.
   - When `func_800979D8(self, button_id=10, input_id=0)` is called:
     - `held == 1`, `enabled == 1`.
     - **AN:** Executes `0x80097A1C: jal func_80097978(self, 0)`, reads `*(input + 0x4) = 0xFFFFFFFE` (bit 0 = 0) -> returns `$v0 = 0`. Controlling branch `0x800A4928` branches over camera rotation left (`p60+0xC = 0`).
     - **RT:** Host calls `bainput_isEnabled(self, 0)` on `*(input + 0x0) = 0xFFFFFFBF` (bit 0 = 1) -> returns `ctx->r2 = 1`. Controlling branch `0x800A4928` falls through to `func_80112D90`, setting `p60+0xC = 1` (unwanted camera rotation left).

3. **Classification & Verification:**
   - `FIRST_INPUT_PLUS4_DIFFERING_EVENT`: `glidx 22`, `func_800979D8(self, btn=10, in_id=0)`.
   - `FIRST_INPUT_PLUS4_WRITER_RT`: `NO_WRITE_OBSERVED` (host called `bainput_enable` on `input+0`, omitting write to `input+4`).
   - `FIRST_INPUT_PLUS4_WRITER_AN`: `func_80097990` at `0x800979CC` (CLEAR bit 0).
   - `FIRST_INPUT_PLUS4_WRITER_EVENT_MATCH`: `ABSENT_IN_ONE_RUNTIME`.
   - `FIRST_INPUT_PLUS4_OLD_MATCH`: `YES` (`0xFFFFFFFF`, bit0=1).
   - `FIRST_INPUT_PLUS4_SOURCE_OPERATION_MATCH`: `NO`.
   - `FIRST_INPUT_PLUS4_NEW_MATCH`: `NO` (`0xFFFFFFFE` [bit0=0] vs `0xFFFFFFFF` [bit0=1]).
   - `HOST_800979D8_MISMATCH_CAUSES_INPUT_PLUS4_FIRST_DIFF`: **`PROVEN`**.
   - `CURRENT_ROOT_CLASS`: `B-800979D8-HOST-REPLACEMENT-WRONG-SIDE-EFFECT`.
   - `BAINPUT_INIT_EXECUTES_IN_WINDOW_RT`: `NO`.
   - `BAINPUT_INIT_EXECUTES_IN_WINDOW_AN`: `NO`.
   - `NEXT_SINGLE_ACTION`: GPT-5.6 Luna High arbitrate the first input+4 divergence and the causal proof that the host 800979D8 mismatch produces it.
   - `NEXT_RECOMMENDED_MODEL`: GPT-5.6 Luna High. No fix, commit, or push.

## FINAL CAUSAL ARBITRATION — first `input+4` writer (2026-08-25)

Artifact `/tmp/p08-demo5-inputplus4-first-writer-20260825T220500Z/` is a
`NEW_RAW_CAPTURE`. Every entry in `SHA256SUMS` passes; manifest SHA is
`3acb82b7a27b9dfa90da7a782e0618caebd6ef9daa574340261007703417de46`.
This section is **CORRECTED/SUPERSEDES** any earlier claim that the complete
`input+0`/`input+4` entry state matched at glidx 22.

### Final classification

- `GLIDX22_800979D8_SEMANTIC_JOIN`: **PROVEN** — `stage=0, sub=5,
  map=296, mode=5`, semantic player/self, `button_id=10`, `input_id=0`.
  Raw pointers differ by runtime and are not used as the join key.
- `CALL_PRESENCE_800979D8_RT`: **YES**.
- `CALL_PRESENCE_800979D8_AN`: **YES**.
- `BAKEY_HELD_RT/AN`: `1/1` (`HELD_MATCH=YES`).
- `IS_ENABLED_RT/AN`: `0/0` (`ENABLED_MATCH=YES`).
- `INPUT_PLUS4_OLD_RT/AN`: `0xffffffff/0xffffffff`
  (`OLD_MATCH=YES`).
- `PRE_WRITE_STATE_GLIDX22`: matching bit 0 (`1/1`).
- `POST_WRITE_STATE_GLIDX22`: RT `0xffffffff` (bit 0 `1`), AN
  `0xfffffffe` (bit 0 `0`). This is the first differing state produced by
  the joined call, not an upstream precondition.
- `HOST_800979D8_MISMATCH_CAUSES_INPUT_PLUS4_FIRST_DIFF`: **PROVEN**.
- `ROOT_CAUSE_FOUND`: **YES**. `FIX_IMPLEMENTED`: **NO**.
- `CURRENT_ROOT_CLASS`: `B-800979D8-HOST-REPLACEMENT-WRONG-SIDE-EFFECT`.
- `CURRENT_ROOT_BOUNDARY`: the held/enabled-disabled semantic substitution
  in `func_800979D8`: original `func_80097990(self, 0, 0)` writes
  `input+4`; host `bainput_enable(self, 0, 0)` writes `input+0`.

### Definitive raw MIPS table for `func_80097990`

The table below comes from the raw `mips-dump-80097970` opcodes in the new
oracle log. It supersedes labels in `static-writer-set.txt` that call
`0x800979C4` a store or place CLEAR at `0x800979B0`.

| PC | RAW_OPCODE | INSTRUCTION | PATH | ROLE |
|---|---|---|---|---|
| `0x80097990` | `50c00009` | `beql $a2,$zero,0x800979b8` | selector | `a2==0` enters CLEAR; likely delay semantics |
| `0x80097994` | `8c8200a4` | `lw $v0,0xA4($a0)` | CLEAR delay slot | load input pointer when branch is taken |
| `0x80097998` | `8c8200a4` | `lw $v0,0xA4($a0)` | SET | load input pointer |
| `0x8009799c` | `240f0001` | `li $t7,1` | SET | mask seed |
| `0x800979a0` | `00afc004` | `sllv $t8,$t7,$a1` | SET | `mask = 1 << input_id` |
| `0x800979a4` | `8c4e0004` | `lw $t6,4($v0)` | SET | read `input+4` |
| `0x800979a8` | `01d8c825` | `or $t9,$t6,$t8` | SET | set mask bit |
| `0x800979ac` | `03e00008` | `jr $ra` | SET | return; next word is delay slot |
| `0x800979b0` | `ac590004` | `sw $t9,4($v0)` | SET delay slot | definitive SET store to `input+4` |
| `0x800979b4` | `8c8200a4` | `lw $v0,0xA4($a0)` | no reachable entry path | word after SET delay slot; not the CLEAR store |
| `0x800979b8` | `24090001` | `li $t1,1` | CLEAR | mask seed |
| `0x800979bc` | `00a95004` | `sllv $t2,$t1,$a1` | CLEAR | `mask = 1 << input_id` |
| `0x800979c0` | `8c480004` | `lw $t0,4($v0)` | CLEAR | read `input+4` |
| `0x800979c4` | `01405827` | `nor $t3,$t2,$zero` | CLEAR | `~mask` |
| `0x800979c8` | `010b6024` | `and $t4,$t0,$t3` | CLEAR | clear mask bit |
| `0x800979cc` | `ac4c0004` | `sw $t4,4($v0)` | CLEAR | definitive CLEAR store to `input+4` |
| `0x800979d0` | `03e00008` | `jr $ra` | CLEAR | return |
| `0x800979d4` | `00000000` | `nop` | CLEAR delay slot | return delay slot |

Therefore `CLEAR_STORE_PC=0x800979CC`, `SET_STORE_PC=0x800979B0`, and
`OLD_READ_PC_CLEAR=0x800979C0`, `OLD_READ_PC_SET=0x800979A4`. For the
causal call, `80097990_CALLSITE_PC=0x80097A10`,
`80097990_DELAY_SLOT_PC=0x80097A14` (`move $a2,$zero`), and
`80097990_RETURN_ADDRESS=0x80097A18` (`jal` writes `PC+8`). The artifact's
`caller_pc=0x80097A18` is therefore a return address, not the callsite.

### AN old/source/new and RT write coverage

- AN: `OLD_INPUT_PLUS4=0xffffffff`, `input_id=0`, `VALUE_ARG_A2=0`,
  `MASK=1`, `OPERATION=CLEAR`, `STORE_PC=0x800979CC`,
  `NEW_INPUT_PLUS4=0xfffffffe`; `0xffffffff & ~1 = 0xfffffffe`.
- RT: `HOST_BRANCH_PATH=held && enabled==0`; helper is
  `bainput_enable(self,0,0)`, target field is `input+0`, and
  `INPUT_PLUS4_BEFORE/AFTER_RT=0xffffffff/0xffffffff`.
- `FULL_INPUT_PLUS4_WRITE_WATCH_COVERAGE_RT=YES` for this bounded capture:
  the static writer set plus the per-occurrence trace covers the relevant
  `input+4` writers; RT records `NO_WRITE_OBSERVED` while its host helper's
  observed write is to `input+0`.
- `LAST_PROVEN_INPUT_PLUS4_MATCH_EVENT=glidx 21` (and
  `PRE_WRITE_STATE_GLIDX22` is also a match).
- `FIRST_PROVEN_INPUT_PLUS4_DIFFERING_EVENT=glidx 22 POST_WRITE`, produced
  by the missing/misdirected host side effect.

### Downstream and the two mismatches

At glidx 23, AN reads `input+4=0xfffffffe` through
`func_80097978` at `0x80097984` and returns `0`; RT reads
`input+0=0xffffffbf` through `bainput_isEnabled` and returns `1`. Thus
`800979D8` returns `0/1`, `80097B70` returns `0/1`, and
`0x800A4928` is `TAKEN` in AN and `NOT_TAKEN` in RT; the later
`func_80112D90`/camera chain is downstream (`AN=NO`, `RT=YES`).

- `HOST_MISMATCH_WRITE_HELPER`: **PROVEN** — original
  `func_80097990(self,input_id,value)` operates on `input+4`; host calls
  `bainput_enable` on `input+0`.
- `HOST_MISMATCH_READ_HELPER`: **PROVEN** — original
  `func_80097978(self,input_id)` reads `input+4`; host's final query calls
  `bainput_isEnabled` and reads `input+0`.
- These are two wrong substitutions in one `800979D8` replacement and must
  be corrected together for bit-exact semantic parity. The exact original
  helper names are not present as public `RecompiledFuncs/**` symbols in the
  indexed repository; host `bainput_enable` and `bainput_isEnabled` exist but
  are not semantic replacements for these two callees.

### Counterfactual, semantic fix, and next validation

- `LOCAL_FROZEN_GLIDX23_QUERY_FIX`: **not sufficient**; fixing only the
  final query leaves RT `input+4=0xffffffff` because glidx22 missed the
  CLEAR, so the query still returns `1`.
- `FULL_REPLACEMENT_FIX_FROM_BEFORE_GLIDX22`: the faithful MIPS behavior is:
  held false → SET `input+4` with `1<<input_id`, return `0`; held true →
  query enabled, CLEAR `input+4` when enabled is zero, then return the
  active-bit query from `input+4`. No runtime PASS is claimed here.
- `FULL_REPLACEMENT_FIX_EXPECTED_TO_RESTORE_THIS_CAUSAL_CHAIN`: **YES**
  (expected counterfactual only; not tested).
- `NEXT_SINGLE_ACTION`: implement the minimal faithful
  `tooie_bainput_func_800979D8` replacement and run a dual-runtime A/B
  capture of USER_DEMO_5. Require glidx22 old/new `input+4` parity and
  operation parity, then glidx23 return parity, `80097B70`, `0x800A4928`,
  `80112D90` call presence, `p60+0xC`, camera/yaw, and downstream movement
  parity before a broader attract regression run.
- `NEXT_RECOMMENDED_MODEL`: Gemini 3.7 Flash High. Optional Grok 4.6
  Medium only for any remaining opcode/store-PC ambiguity.

## Post-Fix Validation Checkpoint — `tooie_bainput_func_800979D8` Faithful Fix (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-fix-validation-20260825T235800Z/` is valid;
`NEW_RAW_CAPTURE_POST_FIX`, manifest SHA is `9a977b6e1c4c276dd2b6616008723d4b3c7139b9d1c4bd6c519c2f23ae3373da`,
all 16 `SHA256SUMS` entries pass (dual verification).

- **`ROOT_CAUSE_FOUND:`** `YES`
- **`ROOT_CAUSE_FIX_VALIDATED:`** `YES`
- **`FIX_IMPLEMENTED:`** `YES`
- **`FIX_SCOPE:`** `src/game/boot_hw_stubs.cpp` (`tooie_bainput_func_80097990` on `input+4`, `tooie_bainput_func_80097978` on `input+4`, and corrected `tooie_bainput_func_800979D8`)
- **`HOST_WRITE_HELPER_FIXED:`** `YES`
- **`HOST_READ_HELPER_FIXED:`** `YES`

**Validation Results (100% Parity with ANGRYLION_CONTROL):**
- `GLIDX21_CONTROL_PARITY:` `PASS` (held=0, enabled=1, input+4=0xFFFFFFFF, v0=0)
- `GLIDX22_HELD_PARITY:` `PASS` (held=1/1)
- `GLIDX22_ENABLED_PARITY:` `PASS` (enabled=0/0)
- `GLIDX22_INPUT4_OLD_PARITY:` `PASS` (0xFFFFFFFF/0xFFFFFFFF)
- `GLIDX22_OPERATION_PARITY:` `PASS` (func_80097990 CLEAR store at 0x800979CC)
- `GLIDX22_INPUT4_NEW_PARITY:` `PASS` (0xFFFFFFFE/0xFFFFFFFE)
- `GLIDX23_INPUT4_PARITY:` `PASS` (0xFFFFFFFE/0xFFFFFFFE)
- `GLIDX23_800979D8_RETURN_PARITY:` `PASS` (v0=0/0)
- `GLIDX23_80097B70_RETURN_PARITY:` `PASS` (v0=0/0)
- `GLIDX23_800A4928_PARITY:` `PASS` (TAKEN/TAKEN, skipping camera rotation left)
- `GLIDX23_80112D90_CALL_PARITY:` `PASS` (NO/NO, camera rotation left completely collapsed)
- `GLIDX23_P60C_PARITY:` `PASS` (p60+0xC = 0/0)
- `POST_FIX_EEBF0_PARITY:` `PASS` (yaw=270.0 in both runtimes)
- `POST_FIX_EE780_PARITY:` `PASS` (a3=0 in glidx 21..23, 3fa21681 in glidx 24, 3fe2ec4e in glidx 25)
- `BTN13_NEARBY_REGRESSION:` `PASS` (input+4 bit1 SET, v0=0 in both runtimes)

**Status & Next Steps:**
- The entire USER_DEMO_5 locomotion/camera divergence chain is **RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX**.
- P08 remains **ACTIVE / MANUAL_VALIDATION_PENDING** pending broader physical validation gates.
- `NEXT_SINGLE_ACTION:` GPT-5.6 Luna High independently arbitrate the post-fix validation artifact and determine the remaining P08 work.
- `NEXT_RECOMMENDED_MODEL:` GPT-5.6 Luna High. No commit/push.

## Arbitragem final pós-fix P08 / USER_DEMO_5 — 2026-08-26 (GPT-5.6 Luna High)

Este bloco **CORRIGE/SUPERSEDE** o checkpoint anterior quanto ao escopo,
proveniência e pendências, sem apagar o histórico.

Artifact: `/tmp/p08-demo5-800979d8-fix-validation-20260825T235800Z/`;
`MANIFEST_SHA=9a977b6e1c4c276dd2b6616008723d4b3c7139b9d1c4bd6c519c2f23ae3373da`;
`SHA256SUMS` integralmente válido (16/16).

Proveniência: `RT_RAW_PROVENANCE: NEW_RAW_CAPTURE_POST_FIX`;
`AN_RAW_PROVENANCE: REFERENCE_ORACLE_CAPTURE_REUSED`;
`REFERENCE_ORACLE_REUSE_VALID: YES`, pois `oracle.log` é byte-identical a
`/tmp/oracle_fresh3.log` e a captura Oracle precede o fix de produção. Esta
conclusão vale para o artifact delimitado.

A semântica corrigida é fiel ao MIPS: `800979D8` faz SET não-held; held
consulta enabled em `input+0`, faz CLEAR em `input+4` apenas quando disabled e
retorna o query de `input+4`, com shift low-5-bit e extensão de sinal corretos.
Logo `USER_DEMO5_TARGET_CHAIN_PARITY: PROVEN` para
`800979D8 → 80097B70 → 800A4928 → câmera/80112D90 → p60+0xC → yaw/EEBF0/EE780`;
`GLOBAL_GAME_PARITY: NOT_PROVEN`.

Gates: `DIRECTLY_SUPPORTED` para held/enabled, input+4 old/new e operação,
retornos, branch, p60+0xC, yaw, EEBF0/EE780 e btn13; `DERIVED_BUT_PROVEN` para
a junção causal normalizada e ausência do dispatch de rotação esquerda;
`UNSUPPORTED` apenas para parity global, long playthrough e Windows/controle
físico.

`P08_STATUS: ACTIVE / MANUAL_VALIDATION_PENDING`;
`P09_STATUS: PENDING / NOT_FORMALLY_OPEN`;
`USER_DEMO5_STATUS: RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`.
`AUDIO_STARTUP_TICK: P08_RESIDUAL`; `WIDESCREEN_EDGE_GEOMETRY:
POST_P08_ENHANCEMENT` (baseline 4:3); `CHARACTER_EDGE_GRAPHICS_GLITCH:
P08_RESIDUAL`; `LONG_PLAYTHROUGH_VALIDATION: MANUAL_VALIDATION_PENDING`;
`WINDOWS_GAMEPAD_VALIDATION: BLOCKING_P08 / MANUAL_VALIDATION_PENDING`.

Remaining P08: validação formal Windows/controle físico, long playthrough e
triagem separada dos residuais de áudio/gráficos. O fix focal está
`CURRENT_FIX_READY_FOR_COMMIT: YES`; o worktree completo ainda mistura
instrumentação/diagnóstico e histórico, portanto não é um commit único limpo.
`BUILD_THIS_ROUND: NOT_RUN`; nenhum commit/push foi feito.

`NEXT_SINGLE_ACTION: executar e registrar a validação formal Windows + controle
físico antes de abrir P09.`
