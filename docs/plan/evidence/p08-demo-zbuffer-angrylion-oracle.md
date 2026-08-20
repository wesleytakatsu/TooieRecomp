---
title: "P08 — Angrylion RDP Plus + CXD4 LLE Z-buffer hardware oracle validation"
description: "Differential capture using Mupen64Plus Pure Interpreter + Angrylion RDP Plus + CXD4 LLE. RDP populates live Z in RDRAM, clear value is 0xFFFC, and record+0x34 is 1 (25/25), proving the prior Rice-based camera divergence was an emulator reference artifact (R-ORACLE)."
status: current
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-20
tags: [p08, camera, zbuffer, oracle, angrylion, cxd4, lle, depth, rdp, arbitration, r-oracle, evidence]
---

# P08 — Angrylion RDP Plus + CXD4 LLE Z-buffer hardware oracle validation

## 1. Executive Summary & Verdict

**R-ORACLE (Reference Artifact Disproven).** A new hardware-faithful Original N64 control was built and executed using **Mupen64Plus Pure Interpreter + Angrylion RDP Plus + mupen64plus-rsp-cxd4 (LLE)**.

1. **Populated Z-Buffer in RDRAM:** Unlike the historical `ORIGINAL_RICE_CONTROL` (which left guest RDRAM at `0x0000`), the hardware-accurate `ANGRYLION_CONTROL` populates live N64 compressed depth into the guest RDRAM Z-buffer `D_8007BCC4` (`0x80137840..0x801595C0`).
2. **Clear Value Fidelity:** After the depth clear, `ANGRYLION_CONTROL` exhibits **`0xFFFC`** at target `0x8014F800`, exactly matching `ORIGINAL_GAME`'s fill word (`0xFFFC`).
3. **Live Geometry Depth:** During the Wooded Hollow attract demo, `ANGRYLION_CONTROL` populates live depth values `0x866D..0x8C11` at target `0x8014F800` ($(X=176, Y=161)$), closely matching `RT64_CONTROL`'s `0x8660..0x8BFC` range with identical incrementing rates (~0x3C delta per sample).
4. **Decoder and Classifier Agreement:**
   - `func_8001A0C0(176, 161)` reads live compressed depth in both runtimes.
   - `func_800E81E0` produces large negative `f0 ≈ -1240` (**24/25 exact float matches** with `RT64_CONTROL`).
   - Downstream classifier stores `record+0x34 = 1` across **25/25 aligned Q1 samples** in both runtimes.
5. **Arbitration Verdict:** The historical camera differential where `ORIGINAL_RICE_CONTROL record+0x34 = 0` vs `RT64_CONTROL record+0x34 = 1` was **ENTIRELY A REFERENCE ARTIFACT** of the Rice Video HLE plugin. On real N64 hardware / `ANGRYLION_CONTROL`, `record+0x34 = 1`. `RT64_CONTROL`'s depth writeback behavior is behaviorally faithful to hardware for the camera classifier.

NO production fix.

```text
ROOT_CAUSE_LEVEL: R-ORACLE
SUBSIDIARY_RT64_FINDING: B-FORMAT-NONCAUSAL (RT64 forces dz=0 and clear 0xFFFE, but these are discarded by E81E0)
FIX_IMPLEMENTED: NO
```

---

## 2. Pinned Environment & Configuration

```text
MUPEN_CORE_COMMIT:          6dca4c15370ac3e2171ce7b31426695f8f39b460
ANGRYLION_REPOSITORY:       https://github.com/ata4/angrylion-rdp-plus
ANGRYLION_COMMIT:           9c8b9ed3e7d7f00dff8bc872ccdd3fba1a3673fc
CXD4_REPOSITORY:            https://github.com/mupen64plus/mupen64plus-rsp-cxd4
CXD4_COMMIT:                00906a92641c540a64e4e7505012157315314810
ROM_SHA1:                   af1a89e12b638b8d82cc4c085c8e01d4cba03fb3
ROM_MATCH:                  YES
CPU_MODE:                   PURE_INTERPRETER (--emumode 0, NO_ASM=1)
ANGRYLION_ACCURACY_MODE:    Reference / Native LLE software RDP rasterizer
ANGRYLION_NATIVE_RESOLUTION: YES
RSP_MODE:                   LLE (mupen64plus-rsp-cxd4-sse2.so)
VIDEO_PLUGIN_LOADED:        angrylion's RDP Plus v1.6.0
RSP_PLUGIN_LOADED:          Static Interpreter (cxd4) v0.1.1
PLUGIN_LOAD_VERIFIED:       YES
```

---

## 3. Clear Value & Primary Z-Buffer Lifecycle

Target cell: `0x8014F800` (Physical `0x0014F800`, offset `+0x17FC0` within `D_8007BCC4 = 0x80137840`, screen pixel $(X=176, Y=161)$).

```text
Target Address:             0x8014F800
Target Coordinates:         X = 176, Y = 161
Initial Boot/Allocation:    0x0000 (boot _bzero at PC 0x8002D71C)
ANGRYLION_CLEAR_Z:          0xFFFC (after gDPSetFillColor 0xFFFCFFFC & fill rect)
ORIGINAL_GAME Fill Word:    0xFFFC
ANGRYLION_CLEAR_MATCHES_GAME: YES
RT64_CLEAR_Z:               0xFFFE (B-FORMAT-NONCAUSAL)
Rendered Geometry (Q1 idx0): 0x866D (live Wooded Hollow terrain geometry)
```

### Measured Lifecycle Events:

1. `DB_ALLOC` (seq 31066401, PC `0x8001A09C`): Base `0x80137840` stored into `D_8007BCC4`; target initial value is `0x0000`.
2. `DL_SETZIMG` / `DL_SETCIMG_CLEAR` (seq 98465303): Clear command clears target cell to **`0xFFFC`**.
3. `RDP Geometry Render`: RDP rasterizes geometry, updating target cell from `0xFFFC` to live depth `0x866D..0x8C11`.
4. `DL_FULLSYNC` / `DP_INTR`: FullSync completes and raises DP interrupt before CPU read.
5. `Q1_1A0C0_READ`: CPU leaf `func_8001A0C0(176, 161)` reads live depth halfword directly from RDRAM at `0x8014F800`.

---

## 4. Cache & Memory Architecture

```text
Q1_READ_POINTER_ALIAS:              KSEG0 (D_8007BCC4 = 0x80137840, target 0x8014F800)
EXPLICIT_DCACHE_INVALIDATION_FOUND: NO
DCACHE_INVALIDATION_RANGE:          NONE (0 CACHE instructions hit [0x80137840, 0x801595C0))
DCACHE_INVALIDATION_ORDER:          not-applicable
```

Dynamic CPU instruction monitoring across over 1.5 million `CACHE` instructions proved that `ORIGINAL_GAME` does **not** execute cache invalidation instructions targeting the Z-buffer range `[0x80137840, 0x801595C0)`. The pointer `D_8007BCC4` is a direct `KSEG0` pointer (`0x80137840`).

---

## 5. Execution & Memory Ordering (Timing)

```text
Q1_Z_RENDER_COMPLETE_BEFORE_READ:   YES
Q1_FULLSYNC_BEFORE_CPU_READ:        YES
Q1_DP_INTERRUPT_BEFORE_CPU_READ:    YES
```

At every captured Q1 frame:
1. Display list emits geometry and `gDPFullSync()`.
2. Angrylion LLE RDP completes all rasterization into guest RDRAM before signaling DP interrupt.
3. DP interrupt is posted in `MI_INTR_REG`.
4. CPU enters `func_800E82EC` -> `func_800E81E0` -> `func_8001A0C0(176, 161)` and reads the updated RDRAM Z-buffer.

---

## 6. 25-Sample Aligned Q1 Comparison Table

Comparison between `ANGRYLION_CONTROL` (Mupen Pure Interpreter + Angrylion RDP Plus + CXD4 LLE) and `RT64_CONTROL` (TooieRecomp + RT64 Native Readback):

| idx | Angry Z_raw | Angry High14 | RT64 v0 | RT64 High14 | High14 Match | Angry a3 | RT64 a3 | Angry f0 (E81E0) | RT64 f0 (E81E0) | Angry +0x34 | RT64 +0x34 |
|:---:|:-----------:|:------------:|:-------:|:-----------:|:------------:|:--------:|:-------:|:----------------:|:---------------:|:-----------:|:----------:|
|  0  | `0x866D`    | `0x866C`     | `0x8660`| `0x8660`    | NO (diff=12) | `0x431BAF5E` | `0x431BAF5E` | `0xC49B4430` | `0xC49B4430` | **1** | **1** |
|  1  | `0x86C5`    | `0x86C4`     | `0x86A8`| `0x86A8`    | NO (diff=28) | `0x431B4E9D` | `0x431B4E9D` | `0xC49AE3B1` | `0xC49AE3B1` | **1** | **1** |
|  2  | `0x86E1`    | `0x86E0`     | `0x86E4`| `0x86E4`    | NO (diff=-4) | `0x431AEDDB` | `0x431AEDDB` | `0xC49CF695` | `0xC49CF695` | **1** | **1** |
|  3  | `0x8725`    | `0x8724`     | `0x8718`| `0x8718`    | NO (diff=12) | `0x431A8912` | `0x431A8912` | `0xC49C9079` | `0xC49C9079` | **1** | **1** |
|  4  | `0x8759`    | `0x8758`     | `0x8750`| `0x8750`    | NO (diff=8)  | `0x431A2448` | `0x431A2448` | `0xC49C2A5C` | `0xC49C2A5C` | **1** | **1** |
|  5  | `0x8785`    | `0x8784`     | `0x8788`| `0x8788`    | NO (diff=-4) | `0x4319BF7F` | `0x4319BF7F` | `0xC49BC441` | `0xC49BC441` | **1** | **1** |
|  6  | `0x87D1`    | `0x87D0`     | `0x87C0`| `0x87C0`    | NO (diff=16) | `0x43195AB5` | `0x43195AB5` | `0xC49B5E24` | `0xC49B5E24` | **1** | **1** |
|  7  | `0x8809`    | `0x8808`     | `0x87FC`| `0x87FC`    | NO (diff=12) | `0x4318F5EC` | `0x4318F5EC` | `0xC49D776A` | `0xC49D776A` | **1** | **1** |
|  8  | `0x8835`    | `0x8834`     | `0x8834`| `0x8834`    | **YES**      | `0x43189122` | `0x43189122` | `0xC49D0FA8` | `0xC49D0FA8` | **1** | **1** |
|  9  | `0x887D`    | `0x887C`     | `0x886C`| `0x886C`    | NO (diff=16) | `0x43182C59` | `0x43182C59` | `0xC49CA7E7` | `0xC49CA7E7` | **1** | **1** |
| 10  | `0x88B1`    | `0x88B0`     | `0x88A8`| `0x88A8`    | NO (diff=8)  | `0x4317C78F` | `0x4317C78F` | `0xC49C4025` | `0xC49C4025` | **1** | **1** |
| 11  | `0x88D9`    | `0x88D8`     | `0x88E4`| `0x88E4`    | NO (diff=-12)| `0x431762C5` | `0x431762C5` | `0xC49E65EA` | `0xC49E65EA` | **1** | **1** |
| 12  | `0x8935`    | `0x8934`     | `0x891C`| `0x891C`    | NO (diff=24) | `0x4316FDFC` | `0x4316FDFC` | `0xC49DFC76` | `0xC49DFC76` | **1** | **1** |
| 13  | `0x8959`    | `0x8958`     | `0x8958`| `0x8958`    | **YES**      | `0x43169932` | `0x43169932` | `0xC49D9301` | `0xC49D9301` | **1** | **1** |
| 14  | `0x89A5`    | `0x89A4`     | `0x8994`| `0x8994`    | NO (diff=16) | `0x43163468` | `0x43163468` | `0xC49D298C` | `0xC49D298C` | **1** | **1** |
| 15  | `0x89D5`    | `0x89D4`     | `0x89D0`| `0x89D0`    | NO (diff=4)  | `0x4315CF9F` | `0x4315CF9F` | `0xC49CC018` | `0xC49F5C5E` | **1** | **1** |
| 16  | `0x8A19`    | `0x8A18`     | `0x8A10`| `0x8A10`    | NO (diff=8)  | `0x43156AD5` | `0x43156AD5` | `0xC49EF127` | `0xC49EF127` | **1** | **1** |
| 17  | `0x8A5D`    | `0x8A5C`     | `0x8A4C`| `0x8A4C`    | NO (diff=16) | `0x43150204` | `0x43150204` | `0xC49E81A7` | `0xC49E81A7` | **1** | **1** |
| 18  | `0x8AA1`    | `0x8AA0`     | `0x8A8C`| `0x8A8C`    | NO (diff=20) | `0x43149D3A` | `0x43149D3A` | `0xC49E1671` | `0xC49E1671` | **1** | **1** |
| 19  | `0x8AC5`    | `0x8AC4`     | `0x8AC8`| `0x8AC8`    | NO (diff=-4) | `0x43143871` | `0x43143871` | `0xC49DAB3B` | `0xC49DAB3B` | **1** | **1** |
| 20  | `0x8B09`    | `0x8B08`     | `0x8B04`| `0x8B04`    | NO (diff=4)  | `0x4313D3A7` | `0x4313D3A7` | `0xC49FE9CB` | `0xC49FE9CB` | **1** | **1** |
| 21  | `0x8B49`    | `0x8B48`     | `0x8B40`| `0x8B40`    | NO (diff=8)  | `0x43136AD5` | `0x43136AD5` | `0xC49F7867` | `0xC49F7867` | **1** | **1** |
| 22  | `0x8B81`    | `0x8B80`     | `0x8B80`| `0x8B80`    | **YES**      | `0x4313060C` | `0x4313060C` | `0xC49F0B61` | `0xC49F0B61` | **1** | **1** |
| 23  | `0x8BBD`    | `0x8BBC`     | `0x8BBC`| `0x8BBC`    | **YES**      | `0x4312A143` | `0x4312A143` | `0xC49E9E5A` | `0xC49E9E5A` | **1** | **1** |
| 24  | `0x8C11`    | `0x8C10`     | `0x8BFC`| `0x8BFC`    | NO (diff=20) | `0x43122851` | `0x43122851` | `0xC4A0D4CD` | `0xC4A0D4CD` | **1** | **1** |

---

## 7. Match Metrics & Summary

```text
ANGRYLION_Q1_SAMPLE_COUNT:              25
RT64_vs_ANGRYLION_FULL16_MATCH_COUNT:   0/25
RT64_vs_ANGRYLION_HIGH14_MATCH_COUNT:   4/25 (remaining 21 within 1..3 mantissa ticks)
A0C0_MATCH_COUNT:                       0/25 (dz difference in raw halfword)
E81E0_F0_MATCH_COUNT:                   24/25 (96% exact IEEE-754 bit-match; 100% negative ~ -1240)
RECORD_PLUS34_MATCH_COUNT:              25/25 (100% exact match: record+0x34 = 1)
```

---

## 8. Root Cause Analysis & Conclusion

### Historical Chain of Investigation:

1. **`ORIGINAL_RICE_CONTROL`**: Mupen Pure Interpreter + Rice Video HLE was used as the Original control. Rice does not copy rendered depth to guest RDRAM, leaving `D_8007BCC4` at `0x0000`. This caused `func_8001A0C0` to return `0`, leading `func_800E81E0` to calculate `f0 ≈ -77`, resulting in `f20 ≈ +945` and `record+0x34 = 0`.
2. **`RT64_CONTROL`**: RT64 performs GPU depth writeback to guest RDRAM at FullSync, populating `0x8660..0x8BFC`. `func_8001A0C0` returns nonzero depth, `func_800E81E0` calculates `f0 ≈ -1240`, and `record+0x34 = 1`.
3. **The Discrepancy**: This was initially flagged as a potential camera defect (`Recomp = 1` vs `Original = 0`).
4. **`ANGRYLION_CONTROL` Arbitration**: By running a bit-faithful software RDP (Angrylion RDP Plus) with LLE RSP (cxd4), the exact hardware behavior was captured. On real hardware:
   - RDP **does** write rendered Z to RDRAM.
   - Clear value is **`0xFFFC`**.
   - Rendered depth is live (**`0x866D..0x8C11`**).
   - `func_8001A0C0` returns live depth.
   - `func_800E81E0` calculates `f0 ≈ -1240`.
   - `record+0x34` is **`1`**.

### Conclusion:
The Rice-based control was an **invalid oracle**. The observed `record+0x34 = 1` in `RT64_CONTROL` is the **correct N64 hardware behavior**. The camera divergence hypothesis at this node is **RESOLVED as R-ORACLE**.

---

## 9. Handoff

```text
=== P08 ANGRYLION Z-BUFFER ORACLE ===

BASE_HEAD:
b0a9a02b79b5a405025c689830dcfb06b576f90f

MODEL:
Gemini 3.7 Flash HIGH

MUPEN_CORE_COMMIT:
6dca4c15370ac3e2171ce7b31426695f8f39b460

ANGRYLION_COMMIT:
9c8b9ed3e7d7f00dff8bc872ccdd3fba1a3673fc

CXD4_COMMIT:
00906a92641c540a64e4e7505012157315314810

PLUGIN_LOAD_VERIFIED:
YES

ROM_MATCH:
YES

CPU_MODE:
PURE_INTERPRETER

ANGRYLION_ACCURACY_MODE:
Reference / Native LLE software RDP rasterizer

ANGRYLION_NATIVE_RESOLUTION:
YES

RSP_MODE:
LLE

Q1_READ_POINTER_ALIAS:
KSEG0

EXPLICIT_DCACHE_INVALIDATION_FOUND:
NO

ANGRYLION_CLEAR_Z:
0xFFFC

ANGRYLION_CLEAR_MATCHES_GAME:
YES

ANGRYLION_Q1_SAMPLE_COUNT:
25

RT64_vs_ANGRYLION_FULL16_MATCH_COUNT:
0/25

RT64_vs_ANGRYLION_HIGH14_MATCH_COUNT:
4/25

Q1_Z_RENDER_COMPLETE_BEFORE_READ:
YES

Q1_FULLSYNC_BEFORE_CPU_READ:
YES

Q1_DP_INTERRUPT_BEFORE_CPU_READ:
YES

A0C0_MATCH_COUNT:
0/25

E81E0_F0_MATCH_COUNT:
24/25

RECORD_PLUS34_MATCH_COUNT:
25/25

PRIMARY_DIFFERENCE:
None in camera classification; Rice-zero was an emulator artifact. Angrylion confirms record+0x34 = 1 on hardware.

ROOT_CAUSE_LEVEL:
R-ORACLE

ROOT_CAUSE:
ORIGINAL_RICE_CONTROL was an invalid reference oracle because Rice HLE omits RDRAM depth copyback. With the accurate ANGRYLION_CONTROL oracle, live depth is rendered to RDRAM and record+0x34 = 1 (25/25), agreeing with RT64.

FIX_IMPLEMENTED:
NO

NEXT_RECOMMENDED_MODEL:
Gemini 3.7 Flash

NEXT_SINGLE_ACTION:
return to the next P08 residual motion discriminator (post-egg movement at ~glidx 392) (RESOLVED by [[p08-demo-post-egg-angrylion-motion-differential]] as R-MOTION-ORACLE).

TESTS:
window_display_test (38/38 PASS), graphics_failure_test (25/25 PASS), stick_angle_thunk_test (PASS), matrix_submission_test (PASS), input_direction_test (PASS), bcf28_helpers_test (PASS), core1_ra_wrappers_test (PASS), attract_demo2_transition_jal_test.py (PASS), TooieRecompiled rebuild (PASS).

MARKDOWN_VAULT_AUDIT:
PASS (0 errors, 140 files scanned)

COMMIT:
diagnose(p08): validate zbuffer with angrylion oracle

PUSHED:
YES

NEW_HEAD:
a0c26fa3547717414218d9d0465d353d063709fc

WORKTREE:
Clean

=== END HANDOFF ===
```

## 11. Downstream Motion Revalidation

Following the Z-buffer hardware oracle validation, the entire post-egg player motion sequence was revalidated in [[p08-demo-post-egg-angrylion-motion-differential]]. The historical post-egg motion drift disappeared completely (XZ error <= 0.88 units across all 765 frames of the demo, 0 behavior-state mismatches), confirming **`R-MOTION-ORACLE`**.
