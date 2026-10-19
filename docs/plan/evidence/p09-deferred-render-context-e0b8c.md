---
title: "P09 reconcile DE448 lifetime vs E2180 / E0B8C"
description: "DE448→DE498 is immediate; wrapped E2180 is not in 80092444 TLS. Durable 0x62B model_bin is the snout filter. E0B8C is a record writer gated by bawobble unkC==2."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-02
tags: [tooie-recomp, p09, de448, e2180, e0b8c, bawobble]
---

# P09 reconcile DE448 lifetime vs E2180 / E0B8C

No production fix. Generated `RecompiledFuncs/**` not edited. No user
capture this round.

## Capture that forced this round

```text
E2180_RAW_ENTRY_COUNT=3491
E2180_FILTER_ACCEPT_COUNT=0
E2180_REJECTION_BREAKDOWN=NO_SCOPE=3491
E0B8C_RAW_ENTRY_COUNT=0
LIVE_BANJO_BODY_RESOURCE=0x62B
```

`func_80092444` TLS is the wrong snout lifetime. Resource/node filters
were never reached. Do not widen a blind global TLS scope.

## DE448 execution (`PROVEN` `STATIC_MIPS` / generated C)

```text
func_800DE448
  LOOKUP_FUNC(0x800A7180)
  LOOKUP_FUNC(0x800DE498)   // immediate, same C++ call
func_800DE498
  ... cull / render-mode ...
  LOOKUP_FUNC(0x800DE2A4)(gfx, model_bin + *(model_bin+4))
func_800DE2A4
  table = *(D_8012CF8C)     // 0x80130000-0x3074; selected via D_80123198
  jalr LOOKUP_FUNC(ctx->r25) // GeoCmd type at node+0
```

`DE448_EXECUTION_MODEL: MIXED`

- The submit helper itself is **immediate** nested `DE498`.
- GeoCmd walk inside that submit is **immediate** `DE2A4` jalr.
- Observed wrapped `800E2180` is **not** nested under `wrap_func_80092444`
  (`PROVEN` runtime `NO_SCOPE=3491`). Those entries are other models
  and/or a later independent `LOOKUP_FUNC(0x800E2180)` pass.

`DE448_TO_DE498_CALL_RELATION:` immediate `LOOKUP_FUNC(0x800DE498)` from
`func_800DE448`.

`DE498_TO_E2AA4_RELATION:` not a direct jal. `DE2A4` jalr through the
GeoCmd handler table. Type `0x02` is the static membership path into
`func_800E2AA4` (`PROVEN` for live `0x62B` layout; the wrapped `800E2180`
traffic is not proof of nesting under `80092444`).

`E2AA4_TO_E2180_RELATION:` immediate `LOOKUP_FUNC(0x800E2180)` with
`a0=gfx`, `a1=node`, `a2=saved a2`.

`FIRST_POINT_WHERE_80092444_DYNAMIC_SCOPE_IS_LOST:`
`__wrap_func_80092444` pops TLS on return. Wrapped `800E2180` is not
observed before that pop. Do not treat `LOOKUP_FUNC(ctx->r25)` as a
reason to widen TLS.

`WHY_E2180_OCCURS_AFTER_SCOPE:` wrapped `func_800E2180` is a constant
`LOOKUP_FUNC(0x800E2180)` used by several GeoCmd dual-mtx helpers
(`E2AA4` / `E2C54` / `E2E48` / `E2A3C`) and overlay sites. Player
`0x62B` GeoCmd02 membership does **not** imply those wrapped entries
run while `func_80092444` is on the C++ stack.

## Renderer context (`PROVEN` `STATIC_MIPS`)

| Item | Location |
| --- | --- |
| `RENDER_CONTEXT_STRUCT` | `D_8012C948` (`0x80130000-0x36B8`) plus GeoCmd table ptr `D_8012CF8C` |
| `MODEL_BIN_FIELD` | DE448 5th arg / DE498 `0x10C($sp)` / DE2A4 geolist base |
| `MODEL_RESOURCE_FIELD` | **not** in renderer; player `lh(*(PlayerState+0x50)+0xC)` |
| `CURRENT_GEOCMD_FIELD` | DE2A4 `s0` current node |
| `CURRENT_NODE_FIELD` | E2AA4/E2180 `a1` |
| `MODEL_INSTANCE_IDENTITY_FIELD` | `model_bin` pointer |

`PLAYER_OWNER_SURVIVES_TO_E2180: NO`

`BEST_DURABLE_MODEL_IDENTITY:` live `0x62B` `model_bin` pointer
(header magic `0xB`, gtype not ExtraSegments `0x40`, bone count 66).

`IDENTITY_CREATED_OR_AVAILABLE_AT:` `func_800DE448` 5th argument.
`IDENTITY_OBSERVABLE_AT_E2180: YES` as `a1` inside `[model_bin, model_bin+span)`.
`IDENTITY_LIFETIME:` loaded model blob until replacement.
`IDENTITY_COLLISION_RISK:` low for one BK body; cap 4 bins.
`IDENTITY_CONFIDENCE: HIGH`

## Snout node at E2180 (`PROVEN` fields)

```text
SNOUT_GEOCMD_ROM_OR_MODEL_OFFSET: 0x14960
SNOUT_RUNTIME_NODE_POINTER_FORMULA: model_bin + 0x14960
SNOUT_RUNTIME_NODE_POINTER: dynamic (loaded blob)
E2180_ARGUMENT_CONTAINING_NODE: a1 / ctx->r5
E2180_SNOUT_FILTER: durable 0x62B model_bin AND word(node+0)==2 AND hword(node+0xA)==42
SNOUT_FILTER_CONFIDENCE: HIGH
```

Previous/current matrices (`PROVEN`):

```text
prev = node + lh(node+0x12)   // func_800AF5D8
curr = node + lw(node+0x14)   // func_800AF5E4
```

## E0B8C (`PROVEN` `STATIC_MIPS`)

```text
E0B8C_STATIC_ADDRESS: 0x800E0B8C
E0B8C_GENERATED_IMPLEMENTATION: RecompiledFuncs/funcs_322.c func_800E0B8C
E0B8C_RUNTIME_MAP_EXPECTED: add_loaded_function(0x800E0B8C, wrap)
E0B8C_RUNTIME_BINDING_CORRECT: YES (same class as working E2180 map hook)
E0B8C_ROLE: RECORD_WRITER
NORMAL_BK_WALK_SHOULD_CALL_E0B8C: CONDITIONAL
```

`func_800E0B8C` writes a 0x14-byte record into `a0` (anim list): Euler
floats, `part` at `+0x10`, type `3` at `+0x11`, bumps `lh +4`. No jal.
Consumer is `func_800E0E94` from `func_800AF04C`.

Callers of `LOOKUP_FUNC(0x800E0B8C)` include overlay `func_808001C0_bawobble`
plus unrelated overlays (`chfloaty`, `basquash`, …). BK walk does **not**
call this implementation every frame.

`bawobble` overlay 162:

- owner = `*(PlayerState+0x154)`
- `unkC` = `lbu owner+0xC`
- `bawobble_entrypoint_6` sets `unkC=2`
- `entrypoint_2` sets 0; `entrypoint_3` clears to 0; `entrypoint_1` sets 1
- `entrypoint_8` returns immediately if `unkC != 2`; else may call
  `func_808001C0_bawobble` → `E0B8C(part=3)`
- `+0x1C` selects substate 2 vs 3 inside ep8
- `entrypoint_7` sets `owner+0x1D` (apply gate)

Zero `E0B8C` raw entry on the submitted walk means ep8 apply did not run
(overlay not updating, or `unkC!=2`). Binding failure is `REFUTED` as the
default explanation because E2180 used the same map hook class and entered.

## Hip3 / snout42 compose (`PROVEN` `STATIC_MIPS`)

```text
LIVE_HIP3_BASE_ANIMATION_SOURCE: func_800AF04C primary clip (walk 0x56)
LIVE_HIP3_ADDITIVE_SOURCE: func_800E0E94 type-3 records IF E0B8C wrote them
LIVE_HIP3_FINAL_LOCAL_TRANSFORM_FUNCTION: func_800AF04C
LIVE_HIP3_RECORD_STORAGE: anim list from func_80092AD8 → 800AE080/800AE6BC
LIVE_HIP3_RECORD_WRITER: func_800E0B8C
LIVE_HIP3_RECORD_CONSUMER: func_800E0E94
HIP3_CURRENT_MATRIX_FUNCTION: func_800AF04C + GeoCmd02 E2AA4
TORSO1/NECK18/SNOUT42_COMPOSE_FUNCTION: same AF04C parent chain 3→1→18→42
SNOUT42_CURRENT_GUEST_MATRIX_SOURCE: AF5E4(node)
SNOUT42_PREVIOUS_MATRIX_SOURCE: AF5D8(node)
```

## Layer comparison (no production fix)

| Class | This round |
| --- | --- |
| BAWOBBLE / E0B8C | absent on captured walk; CONDITIONAL producer |
| BONE/HIERARCHY | AF04C still composes hip3 from the walk clip |
| INTERPOLATION TAGGING | live snout class is GeoCmd02/E2180; Tooie still lacks BK `set_model_matrix_group` |

`CURRENT_MOST_LIKELY_LAYER: MATRIX_IDENTITY / INTERPOLATION_TAGGING`
`CURRENT_ROOT_CONFIDENCE: LOW` (snout matrices not yet captured)

## Next witness (implemented, default-off)

TLS `func_80092444` is **retired for E2180**. Durable `model_bin` +
GeoCmd02 + boneAnimID 42. Hip consume at `func_800E0E94` independently.

```text
NEXT_RUNTIME_WITNESS_SEAM: wrap E2180 + E0E94 via add_loaded_function
NEXT_DURABLE_MODEL_FILTER: DE448 5th-arg header fingerprint 0x62B
NEXT_SNOUT_FILTER: a1 in bin span AND type 2 AND boneAnimID 42
NEXT_FILTER_CONFIDENCE: HIGH
WITNESS_DEPENDS_ON_CALLSTACK_SCOPE: NO
WITNESS_CAN_DISTINGUISH_GUEST_VS_INTERPOLATION: YES (prev/curr guest words; identity still UNKNOWN until capture)
OLD_80092444_TLS_DEPENDENCY_RETIRED: YES (snout)
NEW_PERSISTENT_MODEL_WITNESS_IMPLEMENTED: YES
```
