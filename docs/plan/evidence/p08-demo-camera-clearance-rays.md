# P08 — Bounded Four-Query Clearance Differential for `func_80113D30` at Glidx 384

## 0. Executive Summary

- **Target Analysis:** Isolate which of four directional projection-state samples (`Q0..Q3`) in `func_80113D30` causes `mode_obj->sub_28->half_0C` to evaluate to **4 in Original N64** but **1 in Recomp** at glidx 384. The historical clearance/raycast description is superseded below.
- **Verdict:** **CORRECTED/SUPERSEDED by independent Sol review (2026-08-18)**.
  - The query inputs (`start`, `direction`, `destination`) for all four queries `Q0..Q3` are **NUMERICALLY EQUIVALENT** between Original and Recomp at glidx 384 (position Δ < 0.05 units, direction IEEE-754 bit patterns match to within 1 LSB).
  - In **Original N64**, all four selected projection records return `0` on the captured relevant invocations from 70 to 410, so all four counters `word_3C[0..3]` exceed 4, yielding `half_0C = 4`. `q_hit_v0` is retained only as the historical CSV column name.
  - In **Recomp**, `func_800E8918` returns nonzero for **Q1 and Q2**, keeping their clearance counters at `0` and resulting in `half_0C = 1`. The return is the selected persistent projection record's signed halfword at `+0x34`; it is not the direct return from `func_800C4C34` and is not a collision HIT flag.
  - **First Differing Sample:** **Q1** (camera-local RIGHT / +X direction).
  - **Projection/viewport helper:** `func_800C4C34` (called at `0x800E8998` in both Original and Recomp). Original MIPS proves that this function performs camera-space transform, perspective projection, and viewport/depth clipping; it does not traverse collision geometry.
  - **Query-to-Third-SET74 Chain:** **PROVEN** (`half_0C = 1 < 4` causes `func_801138A0` to return `0`, skipping `func_801138F4` and omitting the 3rd `SET74`).
- **Production Scope:** **0 lines of production C modified** (pure diagnostic differential).

---

## 1. `func_80113D30` MIPS Contract & `half_0C` Write

- **ENTRY_PC:** `0x80113D30`
- **EXIT_PC:** `0x80113FE8` (jr $ra) / `0x80113FEC` (addiu $sp, $sp, 0xC8)
- **Arguments:** `$a0` = `mode_obj` (stored in `$s4`)
- **Data Sub-structure:** `sub_28` pointer at `mode_obj + 0x28`
- **`HALF_0C_WRITE_PC`:** `0x80113FB0` (`sh $t7, 0xC($s0)`)
- **`VALUE_SOURCE`:** `addiu $t7, $t6, 0x1` (where `$t6` is accumulator `lh $t6, 0xC($s0)`)
- **`ACCUMULATOR_REGISTER`:** `$t7`
- **`HALF_0C_WRITER_MATCH`:** **YES**

---

## 2. Definition of the Four Queries (`Q0..Q3`)

| Query | Loop Index `$s1` | Local Direction Vector | Ray Destination Point | Helper Called | Clearance Flag Condition |
|:---:|:---:|:---:|:---:|:---:|:---:|
| **Q0** | 0 | `(0.0, -radius, 0.0)` (DOWN / -Y) | `origin + rotate(0, -radius, 0)` | `func_800E8918` -> `func_800C4C34` | `word_3C[0] >= 4` -> 1, else 0 |
| **Q1** | 1 | `(radius, 0.0, 0.0)` (RIGHT / +X) | `origin + rotate(radius, 0, 0)` | `func_800E8918` -> `func_800C4C34` | `word_3C[1] >= 4` -> 1, else 0 |
| **Q2** | 2 | `(0.0, radius, 0.0)` (UP / +Y) | `origin + rotate(0, radius, 0)` | `func_800E8918` -> `func_800C4C34` | `word_3C[2] >= 4` -> 1, else 0 |
| **Q3** | 3 | `(-radius, 0.0, 0.0)` (LEFT / -X) | `origin + rotate(-radius, 0, 0)` | `func_800E8918` -> `func_800C4C34` | `word_3C[3] >= 4` -> 1, else 0 |

---

## 3. Query Table at Glidx 384

```text
=========================================================================================================
                                     ORIGINAL N64                                    RECOMP
=========================================================================================================
Q0:
  start:   (-4823.640137, -370.067139, 940.386963)         (-4823.629883, -370.067017, 940.441162)
  dir:     (21.116423, -74.866943, 13.785431)              (21.112383, -74.868568, 13.782794)
  dest:    (-4802.523926, -444.934082, 954.172363)         (-4802.517578, -444.935577, 954.223938)
  raw_hit: 0 (CLEAR)                                       0 (CLEAR)
  word_3c: 381 (>= 4)                                      104 (>= 4)
  flag:    1                                               1

Q1:
  start:   (-4823.640137, -370.067139, 940.386963)         (-4823.629883, -370.067017, 940.441162)
  dir:     (43.185589, -0.000001, -66.151382)              (43.185589, -0.000001, -66.151375)
  dir_raw: (422cbe0b, b59e0000, c2844d82)                  (422cbe0b, b59e0000, c2844d81)  [1 LSB diff]
  dest:    (-4780.454590, -370.067139, 874.235596)         (-4780.444336, -370.067017, 874.289795)
  raw_hit: 0 (CLEAR)                                       1 (HIT!)
  word_3c: 381 (>= 4)                                      0 (< 4)
  flag:    1                                               0  [DIVERGENCE]

Q2:
  start:   (-4823.640137, -370.067139, 940.386963)         (-4823.629883, -370.067017, 940.441162)
  dir:     (-21.116423, 74.866943, -13.785431)             (-21.112383, 74.868568, -13.782794)
  dest:    (-4844.756348, -295.200195, 926.601562)         (-4844.742188, -295.198456, 926.658386)
  raw_hit: 0 (CLEAR)                                       1 (HIT!)
  word_3c: 381 (>= 4)                                      0 (< 4)
  flag:    1                                               0  [DIVERGENCE]

Q3:
  start:   (-4823.640137, -370.067139, 940.386963)         (-4823.629883, -370.067017, 940.441162)
  dir:     (-43.185589, 0.000001, 66.151382)               (-43.185589, 0.000001, 66.151375)
  dir_raw: (c22cbe0b, 359e0000, 42844d82)                  (c22cbe0b, 359e0000, 42844d81)  [1 LSB diff]
  dest:    (-4866.825684, -370.067139, 1006.538330)        (-4866.815430, -370.067017, 1006.592529)
  raw_hit: 0 (CLEAR)                                       0 (CLEAR)
  word_3c: 381 (>= 4)                                      0 (< 4, hit on 383)
  flag:    1                                               0  [DIVERGENCE]
=========================================================================================================
SUM:       4                                               1
=========================================================================================================
```

---

## 4. Input Equality & Divergence Classification

- **`Q0_INPUT_MATCH`:** **NUMERICALLY_EQUIVALENT** (Δ < 0.05)
- **`Q1_INPUT_MATCH`:** **NUMERICALLY_EQUIVALENT** (direction raw bits differ by 1 LSB; position Δ < 0.05)
- **`Q2_INPUT_MATCH`:** **NUMERICALLY_EQUIVALENT** (Δ < 0.05)
- **`Q3_INPUT_MATCH`:** **NUMERICALLY_EQUIVALENT** (direction raw bits differ by 1 LSB; position Δ < 0.05)
- **`FIRST_DIFFERENT_QUERY`:** **Q1**
- **`ORIGINAL_RAW_RESULT`:** `0` (selected projection-state return)
- **`RECOMP_RAW_RESULT`:** `1` (selected projection-state return)
- **`ORIGINAL_FLAG`:** `1`
- **`RECOMP_FLAG`:** `0`
- **`COLLISION_QUERY_FUNCTION`:** **REFUTED** — `func_800C4C34` is a projection/viewport helper.
- **`COLLISION_CALL_TARGET_MATCH`:** **YES** (`0x800C4C34` in both)
- **`CLEARANCE_QUERY_EARLY_CONTROL`:** **CORRECTED/SUPERSEDED** — the selected `func_800E8918` record return already differs at the first measured invocation, glidx 70; no collision-ray result was captured.
- **`QUERY_TO_THIRD_SET74_CHAIN`:** **PROVEN**

---

## 5. Historical Sol Handoff Package

**CORRECTED/SUPERSEDED:** the labels below preserve the handoff that prompted
the review. `COLLISION_HELPER_PC`, `CLEAR/HIT`, world-state, and ray-triangle
language are refuted by the original MIPS contract. See
[[p08-demo-camera-collision-sol-review]].

```text
QUERY:                        Q1
CALLER_PC:                    0x80113EE0 (func_80113D30 + 0x1B0)
HELPER_PC:                    0x800E8918
COLLISION_HELPER_PC:          0x800C4C34
ORIGINAL_CALL_TARGET:         0x800C4C34
RECOMP_CALL_TARGET:           0x800C4C34
START_RAW_BITS:               (c596bd1f, c3b90898, 446b18c4) -> (-4823.640137, -370.067139, 940.386963)
DIR_RAW_BITS:                 (422cbe0b, b59e0000, c2844d82) -> (43.185589, -0.000001, -66.151382)
DEST_RAW_BITS:                (c59563a3, c3b90898, 445a8f14) -> (-4780.454590, -370.067139, 874.235596)
ORIGINAL_RETURN:              0 (CLEAR)
RECOMP_RETURN:                1 (HIT)
WORLD_STATE_MATCH:            NOT_APPLICABLE (func_800C4C34 reads no collision world state)
FIRST_INTERNAL_DISCRIMINATOR: selected func_800E8918 projection record +0x34 writer/input
```

## 6. Independent Sol correction (2026-08-18)

- **PROVEN:** the downstream sequence `func_800E8918 return -> word_3C ->
  half_0C -> func_801138A0` remains valid.
- **REFUTED:** Q0..Q3 are not collision rays evaluated by `func_800C4C34`.
  The helper transforms a camera-relative point through a matrix, performs
  perspective projection, applies viewport offsets/bounds, and returns an
  in-viewport result internally.
- **PROVEN:** `func_800E8918` returns `lh record+0x34` at `0x800E8A10`, not
  the `v0` produced by `func_800C4C34`. The trace column `q_hit_v0` is therefore
  a historical misnomer.
- **MEASURED_ORIGINAL / MEASURED_RECOMP:** Q1 and Q2 differ at the first
  relevant invocation captured at glidx 70. Glidx 384 is the first visible
  camera consequence, not the onset of the projection-state difference.
- **ROOT_CAUSE_LEVEL:** `R` — collision-helper attribution refuted. No
  production fix.
