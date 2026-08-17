---
title: "P08 — ncbapivot 13B70 look-at inputs at glidx 384"
description: "Exact eye/target consumed by func_800F18FC / func_80013B70 at the SET80 ncbapivot call. Both bit-differ from glidx 70; only the eye vector grows at 384. Target offset is stable background."
phase: P08
status: evidence
tags: [tooie-recomp, p08, measured-original, measured-recomp, ncbapivot, func_80013B70, func_800F18FC, func_80112E98]
---

# P08 — 13B70 look-at inputs at consumed 384

No production fix. `func_80013B70` was **not** instruction-traced (inputs differ).

```text
BASE_HEAD:                    f37949a7a7f17661b1ed893d11bcf462d1dec4b4
MUPEN_ENV_REUSED:             YES
ROM_MATCH:                    YES
```

## Load PCs (PROVEN)

`func_800F18FC(a0=tgt, a1=eye, a2=dest)`:

```text
LOAD_TGT_X_PC:   0x800F1910  lwc1 $f6,  0x0($a0)
LOAD_EYE_X_PC:   0x800F1914  lwc1 $f4,  0x0($a1)
SUB_DX_PC:       0x800F191C  sub.s $f0, $f4, $f6     # dx = eye.x - tgt.x
LOAD_TGT_Z_PC:   0x800F1924  lwc1 $f4,  0x8($a0)
LOAD_EYE_Z_PC:   0x800F1928  lwc1 $f18, 0x8($a1)
SUB_DZ_PC:       0x800F1934  sub.s $f14, $f18, $f4   # dz = eye.z - tgt.z
JAL_13B70_PC:    0x800F1954
13B70_F12_LOAD:  0x800F1958  lwc1 $f12, 0x24($sp)    # dx
13B70_F14_LOAD:  0x800F194C  lwc1 $f14, 0x1C($sp)    # dz
STORE_ANG_PC:    0x800F1960  swc1 $f0, 0x4($dest)
```

Caller of this `func_800F18FC` is `func_800A516C` (`RA 0x800A518C`) from
`func_80110A68` (`RA overlay-rel 0x240` = `ncbapivot_entrypoint_3`).

`tgt` at `a0` is `sp+0x6C` (`0x80045644`), an exact copy of
`*(mode_obj+0x30)+4` (`A068_P30` bits == `A068_TGT` bits).

`eye` at `a1` is `mode_obj+0x74`.

## Consumed values at 384 (MEASURED)

Captured at `func_800F18FC` entry (same pointers the lwc1s read; no store
to those addresses between entry and the loads).

```text
                ORIGINAL                         RECOMP
mode_obj        0x801D3CF0                       0x801C7440
eye ptr         0x801D3D64  (obj+0x74)           0x801C74B4
tgt ptr         0x80045644  (sp+0x6C)            0x80045644
*(obj+0x30)     0x801D3F48                       0x801C769C
*(obj+0x30)+4   0x801D3F4C                       0x801C76A0

eye.x           -4446.002930  c58af006           -4439.804688  c58abe70
eye.y           -206.578674   c34e9424           -203.940430   c34bf0c0
eye.z            1266.833740  449e5aae            1272.204346  449f068a

tgt.x           -4858.607422  c597d4dc           -4858.596680  c597d4c6
tgt.y           -379.056274   c3bd8734           -379.057587   c3bd875f
tgt.z            963.191772   4470cc46            963.245972   4470cfbe

dx               412.604492                      418.791992   43d16560
dz               303.641968                      308.958374   439a7aac

13B70 dest[1]    53.650032    425699a2            53.582504    4256547c
```

Recomp `dx,dz` also measured at `func_80013B70` entry (`f12`/`f14`) and
match `eye-tgt` from `F18FC_IN`. Original FPR hook at `0x80013B70` was
mis-indexed; original `dx,dz` are computed from the F18FC memory operands
(the values the lwc1s actually consumed).

```text
EYE_INPUT_MATCH:              NO
TARGET_INPUT_MATCH:           NO
INSTRUCTION_TRACE_REQUIRED:   NO
```

## Both differ — shared-state decision (PROVEN)

Bit mismatch on both vectors starts at glidx 70 (ULP / sub-unit).

Numeric growth through the 384 split:

```text
gl   |Δeye.x|  |Δeye.z|  |Δtgt.x|  |Δtgt.z|
382  0.011     0.057     0.011     0.054
383  0.013     0.056     0.011     0.054
384  6.198     5.371     0.011     0.054
385  4.983     4.274     0.011     0.054
```

```text
SHARED_384_CAUSAL_DEPENDENCY:  NO
```

The target offset is a **stable background** (~0.011 x / ~0.054 z) across
380–385. The eye vector is the first **growing** look-at input at 384.

They share the mode object as a container. They do **not** share the 384
causal jump. `func_80013B70` is not blamed.

```text
FIRST_DIFFERENT_INPUT_COMPONENT:  eye  (mode_obj+0x74)
                                  first growing field eye.x at glidx 384
FIRST_UPSTREAM_WRITER:            func_80112E98
                                  jal func_80110770 at 0x80113020
                                  RA 0x80113028  (last SET74 this frame)
```

`func_80112E98` computes a stack candidate (`sp+0x78`) from `a1`/`a2`
plus tables `D_80124A38/40/48`, then copies it through `func_80110770`
onto `obj+0x74`. That store is the consumed eye.

Do not walk `func_80112E98` internals in this note.

## Overlay identity (unchanged)

```text
ncbapivot idx=781  entrypoint_3  RA offset 0x24C
80110A68 RA overlay-rel 0x240
```
