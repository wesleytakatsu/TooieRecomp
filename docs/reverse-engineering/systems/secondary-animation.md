---
title: "Secondary animation (bawobble)"
description: "Locomotion-gated additive Euler on boneAnimID 3 (hip)."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-09-04
tags: [reverse-engineering, bawobble, animation]
---

# Secondary animation (bawobble)

Locomotion-dependent secondary system (`PROVEN`). Observed glitch class
`LOCOMOTION_SECONDARY_SWAY_GLITCH` (`USER_OBSERVED`): Banjo snout,
backpack flap, Mumbo head feathers, Kazooie head feathers — only while
walking/moving.

## Known path (`PROVEN`)

```text
bawobble_entrypoint_8     // update if unkC == 2
func_808001C0_bawobble    // apply
func_800E0B8C(part=3, type=3)
func_800E0E94             // consume during func_800AF04C
```

`part` semantics: **boneAnimID**, not GeoCmd tag.

| Part | Meaning | Status |
| --- | --- | --- |
| 3 | hip | `PROVEN` |
| 42 | snout (not the wobble write target) | `PROVEN` |
| 69 / 79 / 100–105 | Kazooie crest/plumes | `UNKNOWN` as wobble parts |

Kazooie head-feather **4x4** is produced by `func_800AE820` /
`func_800AE8A8` from the per-bone 0x28 records. Live producer rows
for 79 / 100–105 / 107 have `a3+8=0` and `a3+9=0`, so `AEDA0` /
`AF04C` are **not** reached (`PROVEN_FOR_CAPTURE`). That is **base
skeletal composition**. No static `E0B8C` immediate for 100–105 / 79
/ 103 was found. Do not say “secondary animation active” merely
because those boneAnimIDs exist.

A captured 0.417→1 uniform scale on bone 103 (carry-Banjo V1, frames
1420–1424) is a legal scaled rotation (`TOO_STRONG` as degenerate).
Whether the scale is clip-encoded, parent-accumulated, or `AF04C`
additive is `UNKNOWN` pending the V2 producer dump.

```text
part 3 == tag 5: REFUTED identity
WOBBLE_PART3_DRIVES_SNOUT: INDIRECT
```

`func_800E0B8C` is a **record writer**, not an immediate transform. It
is **not** executed every BK walking frame. `bawobble_entrypoint_8`
returns if `lbu(*(PlayerState+0x154)+0xC) != 2`. `entrypoint_6` sets
that byte to 2. Consumer is `func_800E0E94` during `func_800AF04C`.

The submitted walk had `E0B8C_RAW_ENTRY_COUNT=0` with a working E2180
runtime-map hook (`PROVEN` asymmetry). Treat E0B8C as `CONDITIONAL` for
normal BK walk, not as a missing wrapper.

Amplitude/rates of bawobble: `UNKNOWN` (variables not named here).

## Pre-runtime-map-repair snout witness attempt (2026-09-02, superseded)

The fresh `TOOIE_DIAG_SECONDARY_SWAY=1` run is usable for the player-model
identity but did not produce the new transform witness files:

```text
PLAYER_MODEL_ROWS: 1934
PLAYER_MODEL_RESOURCE_PHASES: 0x62A (135 rows, frames 503–637); 0x62B (1799 rows, frames 638–2436)
PLAYER_MODEL_BIN_0x62B: 0x80224AA0
BONES_CSV_ROWS: 0 (file missing)
SNOUT_CSV_ROWS: 0 (file missing)
BONE_ANIM_HITS: 0
SNOUT_MATRIX_HITS: 0
SNOUT_TRANSFORM_WITNESS_CAPTURE_USABLE: NO
```

This does not prove that E0B8C or E2180 was never entered. The submitted
binary only incremented those counters after the selected bone-ID/node filters,
so the exact failing stage was not observable. Static inspection identifies the
gap: both targets are core2 `LOOKUP_FUNC` address-taken calls
(`recomp_overlays.inl`), and linker `--wrap` alone is not an authoritative hook
for the runtime function map. The player scope itself remained proven: 1,934
of 1,934 DE448 samples reached the player callsite.

The diagnostic was then repaired, still default-off, by explicitly binding the
E0B8C/E2180 wrappers through `recomp::overlays::add_loaded_function` and adding
raw wrapper-entry plus snout rejection counters (`NO_SCOPE`, `NO_GETTER`,
`WRONG_RESOURCE`, `WRONG_NODE`). This is an instrumentation repair only; no
production transform or interpolation behavior changed.

```text
HIP3_GUEST_STATE_VALID: UNKNOWN (no bone rows)
SNOUT42_CURRENT_GUEST_TRANSFORM_VALID: UNKNOWN (no snout rows)
PREVIOUS_CURRENT_LOGICAL_PAIRING: UNKNOWN (no snout rows)
PREVIOUS_EQUALS_PRIOR_FRAME_CURRENT: UNKNOWN
FIRST_BAD_LAYER: UNKNOWN
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
CANDIDATE_IMPLEMENTED: NO
```

## Repaired runtime-map witness (2026-09-02)

The post-repair capture proves that the E2180 wrapper is reachable through the
runtime function map, but it does not produce a guest transform witness. E0B8C
has no raw entry during the same sustained walk. The player model identity is
still valid: 0x62A appears for 142 rows and 0x62B for 1,984 rows.

```text
E0B8C_RAW_ENTRY_COUNT: 0
E0B8C_FILTER_ACCEPT_COUNT: 0
E0B8C_REJECTION_BREAKDOWN: no entry
E2180_RAW_ENTRY_COUNT: 3491
E2180_FILTER_ACCEPT_COUNT: 0
E2180_REJECTION_BREAKDOWN: NO_SCOPE=3491, NO_GETTER=0, WRONG_RESOURCE=0, WRONG_NODE=0
PLAYER_SCOPE_HITS: 2126
LIVE_0X62B_SCOPE_HITS: 1984 model rows
BONES_CSV_ROWS: 0 (missing)
SNOUT_CSV_ROWS: 0 (missing)
E0B8C_RUNTIME_BINDING_WORKING: NO (not demonstrated)
E2180_RUNTIME_BINDING_WORKING: YES
FILTERS_WORKING: PARTIAL
SNOUT_TRANSFORM_WITNESS_CAPTURE_USABLE: NO
HIP3_GUEST_STATE_VALID: UNKNOWN
SNOUT_CURRENT_GUEST_MATRIX_VALID: UNKNOWN
PREVIOUS_CURRENT_LOGICAL_PAIRING: UNKNOWN
FIRST_BAD_LAYER: UNKNOWN
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
CANDIDATE_IMPLEMENTED: NO
NEXT_SINGLE_ACTION: reconcile the actual E0B8C runtime dispatch/call path
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
GPU_READBACK_USED: NO
```

The exact E2180 rejection is `NO_SCOPE`: its calls do not occur while the
TLS `func_80092444` player scope is active. That TLS assumption is
`RETIRED` for snout. E0B8C’s zero entry is a path/gate question
(`unkC==2`), not proof of a bad bone state. The known render chain remains
`GeoCmd02 → func_800E2AA4 → func_800E2180`; it does not imply that E0B8C is
executed during rendering.

Player `+0x7A` used by overlay-118 DE448 branch is **not** bawobble
`unkC` (`PROVEN` as a different field).

## Persistent snout-filter runtime boundary (`PROVEN`, `RUNTIME_CPU`)

The current default-off witness reaches the runtime-map E2180 wrapper and the
durable live-model registration path. In the corrected 2026-09-02 capture,
resource `0x62B` produced one unique registered model bin (`0x80226130`) from
2,693 model rows and 5,386 registration hits. E2180 entered 1,062 times; 216
were inside the registered span. The matcher accepted zero rows. See
`p09-corrected-snout-e2180-capture`.

All 1,062 observed entries failed the diagnostic's `word(node+0)==2` and
`hword(node+0xA)==42` tests; none was at relative offset `0x11CB8`. The only
bounded in-span tuple was `a1=0x80241E50` at `+0x1BD20`, with raw
`WORD0=0x000A0004` and `HWORD_A=0x0020`. This proves the current E2180
population does not match the assumed live `0x62B` snout identity, or that
the runtime node-field decoding is wrong. It does not identify a matrix or
interpolation defect.

This is a runtime boundary finding, not a transform-root finding:
guest-state validity, current/previous matrices, logical pairing, and
interpolation identity remain `UNKNOWN`. No production behavior is changed.

`NEXT_SINGLE_ACTION: static E2180/GeoCmd reverse engineering of the live a1
population and node-field semantics`
`NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium`

## Modding relevance

**PROVEN ROLE:** additive Euler on hip while `unkC==2`.

**POTENTIAL MODDING USE:**

- adjust secondary walk sway timing/gate
- change amplitude/rates if those variables become known
- apply wobble to other parts by changing `part`

Proposed aliases (`PROPOSED_ALIAS` only):
`secondary_walk_sway_update`, `secondary_walk_sway_apply`,
`model_bone_apply_additive_euler`.

## Historical live Banjo body witness status (superseded by the authoritative capture above)

The first `TOOIE_DIAG_SECONDARY_SWAY=1` capture (`DE448_TOTAL_SEEN=31020`,
all `ctx->r31` rejects) is **NOT USABLE**. The r31 whitelist is
`RETIRED`. The repaired default-off probe uses `D_80135490` +
`func_80092444` TLS scope + `func_80092B04(a0,1)` model_bin match.
The authoritative capture above supersedes the old unrun status.

```text
LIVE_BANJO_BODY_RESOURCE: 0x62B
OLD_R31_FILTER: RETIRED
PLAYER_MODEL_WITNESS_REPAIRED: YES (captured)
```

## Repaired live Banjo witness (2026-09-02)

The repaired capture is usable. The player scope and model-bin identity agree
for 2,067 samples: `OUTER_PLAYER_SCOPE_HITS=2067`,
`FUNC_80092444_IN_PLAYER_SCOPE_HITS=2067`,
`DE448_CALLSITE_800926D8_HITS=2067`, `PLAYER_SCOPE_MAX_DEPTH=1`, and zero
scope-overflow, wrong-callsite, or other rejects. The remaining 25,619 of
27,686 DE448 calls were correctly rejected as outside the player scope.

Accepted player rows were:

| Resource | Model bin | Bones | Frames | Rows |
| --- | --- | ---: | ---: | ---: |
| `0x62A` | `0x801BEBE0` | 68 | 486–609 | 124 |
| `0x62B` | `0x8020E340` | 66 | 610–2552 | 1,943 |

`0x62A` is an earlier player-model phase; `0x62B` is the sustained normal-
walk body in this reproduction. Thus the live body identity is `0x62B` with
HIGH confidence. The static `0x61F` walk expectation is REFUTED for this
runtime capture, not as a universal asset-table statement.

The live normal-walk snout is compact `GEOCMD_02`, not ExtraSegments and
not the fat E2AA4 dual-mtx node. Static layout places bone-42 at
model-relative `0x11CB8` (INDEX 11). The older `0x14960` figure belongs
to `0x62A` only. Runtime `E2AA4_RAW_ENTRY_COUNT=0` with registered
`0x62B` `CORRECTED` the E2AA4 mapping. Expected live handler is
`func_800DCF48` (single G_MTX via `func_800ADE2C`). Geometry DLs are
child `0x11`/`0x0C` nodes. Hip `boneAnimID=3` still affects the snout
indirectly through parents 3 → 1 → 18 → 42.

A default-off witness is implemented under `TOOIE_DIAG_SECONDARY_SWAY=1`:
DE498-scoped `0x62B` tree CSV (sibling scan) + `func_800DCF48` + nested
`func_800ADE2C`. `func_800E2AA4` is `DEMOTED`. Do not treat
`DE2A4_REL_0X11CB8=0` as missing snout geometry.

```text
CAPTURE_USABLE: YES
LIVE_BANJO_BODY_RESOURCE: 0x62B
LIVE_BANJO_MODEL_BIN: 0x8020E340
BANJO_SNOUT_RENDER_CLASS: GEOCMD_02
BANJO_SNOUT_USES_E2AA4_E2180: NO
SNOUT_TRANSFORM_WITNESS_IMPLEMENTED: YES (default-off)
CURRENT_ROOT_CANDIDATE: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
CURRENT_MOST_LIKELY_LAYER: BONE/HIERARCHY TRANSFORM (DCF48/ADE2C)
```
