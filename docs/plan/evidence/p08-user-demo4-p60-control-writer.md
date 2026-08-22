---
status: active
type: evidence
phase: P08
date: 2026-08-21
tags: [p08, user-demo-4, camera, p60, control-writer, first-discriminator, glidx38, glidx39]
---

# USER_DEMO_4 `p60+0xC` First-Writer Discriminator & Dynamic Paired Trace

This note documents the discovery and dynamic trace of the first writer of the
camera control flag `*(node+0x60)+0xC` (`p60+0xC`) leading into `glidx=39` of
`USER_DEMO_4` (Haunted Cavern, `stage=0 sub=4`).

No production fix is applied in this session (diagnosis-only).

---

## 1. Executive Summary & Root Discriminator

```text
FIRST_PROVEN_DISCRIMINATOR:   0x800979D8 (__bainput_func_800979D8) stubbed to v0=0 in RT64 lookup_gap_stub
DIVERGENCE_MECHANISM:         MISSING_WRITE in RT64 (Oracle executes writer, RT64 skips due to zero return)
FIRST_WRITTEN_FRAME:          glidx=38 (during player button/camera input processing)
FIRST_DIVERGED_OBSERVATION:   glidx=39 enter 80112A84 (RT64 p60+0xC=0, Oracle p60+0xC=1)
STATIC_WRITER_OF_P60_0xC:     80112D90 (sh $t2, 0xC($t3)) called by 801111D0
PRODUCER_CALL_CHAIN:          bainput_should_rotate_camera_right (80097B94)
                                -> 800A4854 (camera trigger dispatch at 800A4950)
                                -> func_80110EFC (controller enable check at 80110F24)
                                -> func_80110E00 (mode dispatch at 80110ED8)
                                -> func_801111D0 (rotation setup at 80111294)
                                -> func_80112D90 (store to p60+0xC = 1, p60+0xE = 2)
RT64_EXECUTION:               800979D8 returns 0 -> bainput_should_rotate_camera_right returns 0 -> NO WRITE
ORACLE_EXECUTION:             800979D8 returns 1 -> bainput_should_rotate_camera_right returns 1 -> WRITES p60+0xC=1
PRIOR_STATE_INTEGRITY:        glidx 0..38 player movement, positions, velocities, yaw, demo script ticks MATCH
FIX_IMPLEMENTED:              NO (diagnosis only, worktree clean)
```

---

## 2. Static Writer Enumeration for `p60+0xC`

Static analysis across `banjo-tooie-Recomp/` assembly and decompilation reveals all accessors and modifiers of the `p60` (`node+0x60`) sub-object:

| Function | VRAM | Access Type | Role in `p60+0xC` Lifecycle |
|---|---|---|---|
| `func_80112D90` | `0x80112D90` | **WRITE** (`sh $t2, 0xC($t3)`) | Stores argument `$a3` (1 or 2) into `p60+0xC`, and arg 5 into `p60+0xE`. Computes transition rate into `p60+0x20`. |
| `func_80112A84` | `0x80112A84` | **READ & WRITE** (`sh $zero, 0xC($t2)`) | Reads `p60+0xC`. If non-zero, sets local `a1 = 5` for `80112D08`. Clears `p60+0xC = 0` when transition finishes. |
| `func_80112E50` | `0x80112E50` | **READ** (`lh $t6, 0xC($v0)`) | Checks if `p60+0xC != 0` and mode matches `p60+0xE` to advance yaw angle. |
| `func_80112E84` | `0x80112E84` | **READ** (`lh $v0, 0xC($t6)`) | Accessor returning `*(int16_t*)(p60 + 0xC)`. |
| `func_80114BD4` | `0x80114BD4` | **ALLOC/CLEAR** | Allocates 0x9C byte header and clears struct pointers. |
| `func_80110280` | `0x80110280` | **INIT** | Calculates embedded sub-block offsets and initializes `node+0x00..0x64` pointer table. |

### Upstream Call Chain to `func_80112D90`

`func_80112D90` has exactly 3 callers in the entire codebase:
1. `func_80111154` (`0x80111154`): passes `a3 = 2` (zoom/center mode, sets `p60+0xC = 2`).
2. `func_801111D0` (`0x801111D0`): passes `a3 = 1` (camera step rotation, sets `p60+0xC = 1`, `p60+0xE = 2`).
3. `func_80113978` (`0x80113978`): passes `a3 = 1` (camera tracking pivot helper, sets `p60+0xC = 1`, `p60+0xE = 1`).

In `USER_DEMO_4` at `glidx=38`, the producer is **`func_801111D0`** because `p60+0xE` is written as `2`.

---

## 3. Dynamic Paired Values Across `glidx 36..40`

Tracking the semantically joined camera node (`node = 0x8033f110` in RT64, `node = 0x8033f760` in Oracle) across frames:

| `glidx` | RT64 `p60+0xC` | RT64 `p60+8` | Oracle `p60+0xC` | Oracle `p60+8` | Event / Status |
|---|---|---|---|---|---|
| **36** | `0` | `0` | `0` | `0` | Both runtimes idle, full camera/player match. |
| **37** | `0` | `0` | `0` | `0` | Both runtimes idle, full camera/player match. |
| **38** | `0` | `0` | `0` (before write) <br> **`1`** (after write) | `0` (before write) | Demo inputs press C-Right. Oracle executes write to `p60+0xC = 1`. RT64 skips due to gap stub. |
| **39** | `0` | `0` | **`1`** | **`5`** | `80112A84` checks `p60+0xC`. Oracle writes `p60+8 = 5`; RT64 writes `p60+8 = 0`. `ncbapivot` diverges. |
| **40** | `0` | `0` | **`1`** | **`5`** | Active transition continues in Oracle. |

---

## 4. Discovery of the Missing Write in RT64

### Oracle Execution Path (glidx 38)
1. Controller pad input for `USER_DEMO_4` contains C-Right button press.
2. `func_800A4854` queries player input via `bainput_should_rotate_camera_right` (`0x80097B94`).
3. `bainput_should_rotate_camera_right` calls `0x800979D8` (`__bainput_func_800979D8`).
4. In Oracle, `__bainput_func_800979D8` executes real MIPS instructions: checks `bakey_held(self, BUTTON_C_RIGHT)` and returns `1`.
5. `func_800A4854` receives `1`, calling `func_80110EFC(node, a1 = 1)`.
6. `func_80110EFC` calls `func_80110E00(node, a1 = 1)`.
7. `func_80110E00` branch `a1 == 1` calls `func_801111D0(node, a1 = +45.0f)`.
8. `func_801111D0` computes target angle `353.041443f` (`0x43b0854e`) and calls `func_80112D90`.
9. `func_80112D90` stores `p60+0xC = 1`, `p60+0xE = 2`, `p60+0x10 = 0x43b0854e`, `p60+0x14 = 0x439a054e`, `p60+0x20 = 0x3f99999a`.

### RT64 Execution Path (glidx 38)
1. `bainput_should_rotate_camera_right` calls `LOOKUP_FUNC(0x800979D8)(rdram, ctx)`.
2. In `TooieRecomp`, `0x800979D8` is bound to `lookup_gap_stub<0x800979D8>` in `src/game/boot_hw_stubs.cpp`:
   ```cpp
   // boot_hw_stubs.cpp:245
   static constexpr int32_t kLookupGapAddrs[] = {
       static_cast<int32_t>(0x80001998),
       static_cast<int32_t>(0x80031178),
       static_cast<int32_t>(0x80036670),
       static_cast<int32_t>(0x80036728),
       static_cast<int32_t>(0x800979D8),  // <--- Returns ctx->r2 = 0
   };
   ```
3. `LOOKUP_FUNC(0x800979D8)` immediately returns `0`.
4. `bainput_should_rotate_camera_right` returns `0`.
5. `func_800A4854` does NOT call `func_80110EFC`.
6. `p60+0xC` remains `0`.

---

## 5. Guardrail Verifications

- **No Production Fix Applied**: The worktree remains clean; no C code or stub tables modified.
- **No Generated C Edit**: Recompiled C files (`RecompiledFuncs/`) were inspected via search only, not edited.
- **No `round.w.s` Reopen**: `round.w.s` fix is confirmed active and stable.
- **No Egg / Math Trace Scope Creep**: Focused strictly on the `p60+0xC` first-writer discriminator.
- **Pre-Divergence State**:
  - `glidx 0..38`: Player pos `(-84.00, 0.00, -1855.00)`, yaw `108.00`, ideal `108.00`, stick `(0.00, 0.00)`.
  - Camera `eye = (-763.909851, 393.176239, -1333.737305)`.
  - Full match between Angrylion control oracle and RT64 runtime.

---

## 6. Next Steps for P08

When authorized for implementation:
1. Recompile / implement `__bainput_func_800979D8` at `0x800979D8` so it executes `bakey_held` logic instead of the zero-return gap stub.
2. Verify with paired RT64 vs Angrylion execution run that `p60+0xC` receives `1` at `glidx=38` and `p60+8` receives `5` at `glidx=39`.
3. Verify complete attract demo parity through `USER_DEMO_4`.
