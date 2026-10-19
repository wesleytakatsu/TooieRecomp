---
title: "P08 — Causal Isolation of Post-Egg Camera Eye Vector Divergence at Glidx 384"
description: "Isolamento causal da divergência do vetor de câmera eye no attract demo de Wooded Hollow."
status: proven
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-18
tags: [p08, camera, eye, demo, wooded-hollow, set74, ncbapivot]
---

# P08 — Causal Isolation of the Post-Egg Camera Eye Vector Divergence at Glidx 384

## 0. Executive Summary

- **Target Analysis:** Post-egg camera eye vector divergence occurring at consumed `glidx 384` during the Wooded Hollow attract demo.
- **Verdict:** **PROVEN (A & B RESOLVED)**.
  - `func_80112E98` itself is an unconstrained clearance candidate generator that produces bit-exact candidate vectors when called.
  - The divergence at `glidx 384` is **causal invocation skip / phase lag**: in **Original N64**, `func_80112E98` is invoked via `ncbapivot_entrypoint_3 -> func_801138F4` starting at `glidx 384`, executing a **3rd `SET74`** (`RA = 0x80113028`) that overwrites `mode_obj+0x74` with candidate `(-4446.002930, -206.578674, 1266.833740)`.
  - In **Recomp**, `func_801138F4` / `func_80112E98` is **NOT invoked** at `glidx 384` (or `glidx 385`), so `mode_obj+0x74` remains at the **2nd `SET74`** value `(-4439.804688, -203.940430, 1272.204346)`.
  - As a direct result, `func_800F18FC` in Recomp consumes the 2nd `SET74` eye vector instead of the 3rd `SET74` eye vector, producing `dx = 418.802734, dz = 309.012573` (angle `53.585548°`) instead of Original's `dx = 412.604492, dz = 303.641968` (angle `53.650032°`), creating the 0.0645° yaw delta.
- **Production Changes:** **0 lines of production C modified** (pure diagnostic analysis).

---

## 1. Full Causal Call Chain in `ncbapivot` (Overlay 781, Entrypoint 3)

During each update tick in Wooded Hollow attract demo mode 5:

```text
ncbapivot_entrypoint_3 (0x80800098)
  │
  ├─> 1. Compute damped/ideal target and orientation spherical angles
  ├─> 2. Assemble unclamped ideal candidate eye vector at sp+0x48
  │
  ├─> 3. jal func_80111FD4(mode_obj, candidate_eye) at 0x808001B8
  │     ├─> jal func_80110770 (1st SET74 at 0x8011208C, RA 0x80112094): writes unclamped candidate to mode_obj+0x74
  │     ├─> jal func_8011329C(mode_obj) at 0x80112094
  │     │     ├─> jal func_801107F0 at 0x801132C8: reads mode_obj+0x74 into sp+0x28
  │     │     ├─> jal func_800EE7F8 at 0x801132D4: copies sp+0x28 to sp+0x34 (old eye)
  │     │     ├─> jal func_80113688 at 0x801132E4: raycasts ground/collision, modifies sp+0x28 if obstructed
  │     │     ├─> jal func_80110770 (2nd SET74 at 0x801132F0, RA 0x801132F8): writes clamped eye to mode_obj+0x74
  │     │     ├─> jal func_800EECE0(sp+0x34, sp+0x28) at 0x801132FC: returns 1 if old_eye == new_eye, 0 if changed
  │     │     └─> sltiu v0, v0, 1: returns 1 if eye changed, 0 if unchanged
  │     └─> returns 1 if func_8011329C returned 1 (eye adjusted), else returns 0
  │
  ├─> 4. Branch check at 0x808001C0: beql $v0, $zero, .L80800234_ncbapivot
  │     [If func_80111FD4 returned 0, JUMPS to .L80800234, SKIPPING func_801138F4 completely!]
  │
  ├─> 5. jal func_801138A0(mode_obj) at 0x808001C8
  │     [Checks mode sub-struct counter at 0x28($a0)+0xE; returns 1 if frame counter >= 5]
  │     beqz $v0, .L808001F8_ncbapivot
  │
  ├─> 6. jal func_801138F4(mode_obj) at 0x808001D8 (Path 1) OR at 0x80800218 (Path 2)
  │     ├─> loads table mode $s7 = 0, collision mask $s5 = 0x20020, base angle/dist
  │     └─> jal func_80112E98 at 0x80113954 (RA 0x8011395C)
  │           ├─> computes candidate eye at sp+0x78 via func_800EE780
  │           ├─> evaluates clearance via func_800EEAD4
  │           ├─> if clearance > threshold:
  │           │     jal func_80110770 (3rd SET74 at 0x80113020, RA 0x80113028)
  │           │     [Overwrites mode_obj+0x74 with 3rd candidate vector!]
  │           └─> returns v0 = 1
  │
  └─> 7. .L80800234_ncbapivot:
        ├─> jal func_80110A68 at 0x80800238
        │     └─> calls func_800F18FC:
        │           reads eye at mode_obj+0x74
        │           reads target at mode_obj+0x80
        │           computes dx = eye.x - target.x, dz = eye.z - target.z
        │           calls func_80013B70(dx, dz) -> updates camera yaw!
        └─> jal func_80110790 (SET80 at 0x80800244): updates target at mode_obj+0x80
```

---

## 2. Glidx 384 Measured Trace Comparison

### A. Original N64 Execution Sequence at Glidx 384
1. **1st SET74 (`RA = 0x80112094`):**
   - Unclamped eye written: `(-4437.917969, -203.015472, 1276.530762)` (hex: `c58aaf58, c34b03f6, 449f90fc`).
2. **2nd SET74 (`RA = 0x801132F8`):**
   - Clamped eye written: `(-4439.798828, -203.892792, 1272.196777)` (hex: `c58abe64, c34be48e, 449f064c`).
3. **`func_801138F4` -> `func_80112E98` Execution:**
   - `func_80112E98` called with `$a0 = 0x801d3cf0, $a1 = target (-4823.640137, -370.067139, 940.386963), $a3 = 533.896790, $s7 = 0`.
   - Generates candidate: `(-4446.002930, -206.578674, 1266.833740)` (hex: `c58af006, c34e9424, 449e5aae`).
   - Clearance check passes (`$f0 > $f26`).
4. **3rd SET74 (`RA = 0x80113028`):**
   - Candidate written to `mode_obj+0x74`: `(-4446.002930, -206.578674, 1266.833740)`.
5. **Consumer (`func_800F18FC`):**
   - Consumes `eye = (-4446.002930, -206.578674, 1266.833740)`, `target = (-4858.607422, -379.056274, 963.191772)`.
   - `dx = 412.604492`, `dz = 303.641968`.
   - `func_80013B70(dx, dz) = 53.650032°`.

### B. Recomp Execution Sequence at Glidx 384
1. **1st SET74 (`RA = 0x80112094`):**
   - Unclamped eye written: `(-4437.902344, -203.053070, 1276.587891)` (hex: `c58aaf38, c34b0d96, 449f92d0`).
2. **2nd SET74 (`RA = 0x801132F8`):**
   - Clamped eye written: `(-4439.804688, -203.940430, 1272.204346)` (hex: `c58abe70, c34bf0c0, 449f068a`).
3. **`func_801138F4` -> `func_80112E98` Execution:**
   - **NOT CALLED** (branch in `ncbapivot` at `0x808001C0` or `0x808001D0` bypassed `func_801138F4`).
   - 3rd SET74 did NOT occur.
4. **Consumer (`func_800F18FC`):**
   - Consumes remaining 2nd SET74 `eye = (-4439.804688, -203.940430, 1272.204346)`, `target = (-4858.607422, -379.056274, 963.191772)`.
   - `dx = 418.802734`, `dz = 309.012573`.
   - `func_80013B70(dx, dz) = 53.585548°` (divergence: `-0.064484°`).

---

## 3. Hypotheses Resolution Table

| Hypothesis | Description | Status | Proof / Evidence |
|:---|:---|:---|:---|
| **H1: Internal arithmetic error in `func_80112E98`** | `func_80112E98` produces wrong float arithmetic when called with equivalent inputs. | `REFUTED` | Assembly and decompiled C for `func_80112E98` match 1:1; `func_80112E98` candidate generator is bit-faithful. |
| **H2: Causal invocation omission in Recomp at Glidx 384** | `func_80112E98` is called in Original at glidx 384 (producing the 3rd SET74), but is bypassed in Recomp. | `PROVEN` | Original trace `/tmp/p08-12e98-original.csv` records `ENTRY,384,385,80112e98...` and `SET74_CALL` with `RA = 0x80113028`, whereas Recomp `/tmp/p08-12e98-recomp.csv` has 0 calls to `func_80112E98` at glidx 384-385. |
| **H3: Target vector `mode_obj+0x80` is root divergence** | The target vector diverged prior to eye vector divergence. | `REFUTED` | Target divergence at glidx 384 is < 0.0001 units (pure rounding), while eye vector differs by > 6.2 units due to 3rd SET74 omission. |

---

## 4. Worktree State & Git Verification

- **Workspace:** `/home/wesley/projetos/N64-Recomp/TooieRecomp`
- `git status --short`: clean (no dirty production files).
- `git diff --check`: passes cleanly.
- Build target `TooieRecompiled`: compiles cleanly with 0 warnings.
