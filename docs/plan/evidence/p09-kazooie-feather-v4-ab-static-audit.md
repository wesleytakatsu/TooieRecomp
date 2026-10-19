---
title: "P09 Kazooie feather V4 drop audit and post-match interpolation A/B"
description: "Static audit of the seven V4 one-to-one drops and the isolated RT64 interpolation-mode diagnostic seam."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-04
tags: [tooie-recomp, p09, kazooie, feather, rt64, interpolation, one-to-one]
---

# P09 Kazooie feather V4 drop audit and post-match interpolation A/B

## Scope and gate

This is a static audit and diagnostic-only implementation checkpoint. The game
was not run. No GPU readback, synchronization, copy, staging, or query was
added. No production group tag or production interpolation change is made.

The analyzed V4 capture is the existing fresh capture associated with
`P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V4`. Its real RT64 witness reaches
`GameFrame::matchTransform`; the seven `REAL_UNMATCHED` decisions are all
one-to-one allocator drops.

## Seven V4 drops

All seven rows are token-level records for the primary semantic feather
`0x62B+0x124A0`, bone index 30, `boneAnimID=103`. The source trace proves the
visible set includes the own DL `0x5AA` at GeoCmd `+0x12580`, the join DL
`0x592` at `+0x12550`, and inherited nested feather geometry `0x57C`/`0x534`.
The V4 CSV does not carry a final transform-to-DL/triangle association, so
the exact affected DL for an individual allocator row is not observable.

| DROP_FRAME | DROP_VISIBLE_DL | DROP_AFFECTS_PRIMARY_0X5AA | DROP_HAS_EQUIVALENT_SUCCESSFUL_MATCH | DROP_CAN_CAUSE_VISIBLE_NO_INTERPOLATION |
| ---: | --- | --- | --- | --- |
| 1195 | `0x5AA` + inherited set; exact row binding unknown | UNKNOWN | YES at allocator level (`CURRENT_ALREADY_MAPPED`) | UNKNOWN |
| 1259 | `0x5AA` + inherited set; exact row binding unknown | UNKNOWN | YES at allocator level (`CURRENT_ALREADY_MAPPED`) | UNKNOWN |
| 1299 | `0x5AA` + inherited set; exact row binding unknown | UNKNOWN | YES at allocator level (`CURRENT_ALREADY_MAPPED`) | UNKNOWN |
| 1311 | `0x5AA` + inherited set; exact row binding unknown | UNKNOWN | YES at allocator level (`CURRENT_ALREADY_MAPPED`) | UNKNOWN |
| 1339 | `0x5AA` + inherited set; exact row binding unknown | UNKNOWN | UNKNOWN at token telemetry level (`PREVIOUS_ALREADY_USED`) | UNKNOWN |
| 1609 | `0x5AA` + inherited set; exact row binding unknown | UNKNOWN | UNKNOWN at token telemetry level (`PREVIOUS_ALREADY_USED`) | UNKNOWN |
| 1624 | `0x5AA` + inherited set; exact row binding unknown | UNKNOWN | YES at allocator level (`CURRENT_ALREADY_MAPPED`) | UNKNOWN |

The five `CURRENT_ALREADY_MAPPED` rows prove that the current transform was
already claimed by another candidate. The two `PREVIOUS_ALREADY_USED` rows
prove that the previous transform candidate was already claimed. None is a
semantic ID mismatch, invalid candidate, determinant failure, or cross-context
match. Therefore:

```text
PRIMARY_VISIBLE_FEATHER_DROP_COUNT: 0 clearly causal drops proven
AUXILIARY_OR_DUPLICATE_DROP_COUNT: 0 proven by the available witness
UNCLASSIFIED_TOKEN_LEVEL_DROP_COUNT: 7
ONE_TO_ONE_DROPS_VISUALLY_CAUSAL: UNKNOWN (no clearly causal drop)
```

This clears the A/B gate without claiming that every drop is harmless. A
future diagnostic would need a final transform-range/DL or triangle witness to
turn the remaining `UNKNOWN` values into a visual causality result.

## Static RT64 interpolation path

For an ordinary ungrouped F3DEX2 `G_MTX`, the decoded group is:

```text
matrixId                 = G_EX_ID_AUTO
decompose                 = true
position/rotation/scale  = G_EX_COMPONENT_AUTO
skew/perspective/tile    = G_EX_COMPONENT_AUTO
ordering                  = G_EX_ORDER_AUTO
```

`GameFrame::match` builds call and candidate maps, then invokes
`GameFrame::matchTransform` only after candidate selection and one-to-one
checks. `matchTransform` preserves the selected previous transform, reads the
current and previous world-transform arrays, and updates the `RigidBody` using
the current `TransformGroup` properties. The actual post-match render result
is computed on the CPU by:

```text
TransformProcessor::setup
  -> RigidBody::lerp(prevFrameWeight, prevTransform, curTransform, true)
  -> lerpMatrixComponents when decomposition is unavailable/disabled
     or lerpTransforms + recomposeMatrix when decomposition is enabled
```

AUTO component modes use velocity heuristics for translation and rotation
(with the current RT64 rotation AUTO path setting rotation interpolation true);
explicit `INTERPOLATE` modes force the corresponding component interpolation.
The normal ungrouped path is therefore AUTO/default decomposition, not a
literal Banjo `G_EX_INTERPOLATE_SIMPLE` group.

## AUTO versus Banjo SIMPLE

`gEXMatrixGroup` supports `G_EX_ID_AUTO`. An AUTO-ID group still enters the
same AUTO `matchScene`/candidate/`matchTransform` path: it is included in the
call-hash matching behavior and is skipped by the stable explicit-ID map.
Changing only the group ID to AUTO does not alter call-hash semantics or the
transform-ID map.

Banjo's `gEXMatrixGroupSimpleNormal` changes more than a stable ID: it sets
`decompose=false`, position/rotation/scale/skew/perspective/tile to
`INTERPOLATE`, and ordering to `LINEAR`. Thus
`ID_AUTO_GROUP_CHANGES_ONLY_INTERPOLATION_PROPERTIES=NO` literally; order,
tile, and decomposition also differ, although order/tile do not affect the
current skeletal `matchTransform` call.

## Isolated A/B implementation

Option 1 was selected: a RT64 post-match callback overrides only the selected
feather transform after matching has already chosen the current/previous pair.
The default-off switch is `TOOIE_DIAG_FEATHER_INTERP_SIMPLE=1`, used together
with the existing `TOOIE_DIAG_SECONDARY_SWAY=1` witness.

When the switch is OFF, the original `TransformGroup` properties are passed
unchanged. When ON, and only for a unique token matching workload, final
transform index, group index, model `0x62B`, node `+0x124A0`, bone 30, and
animation 103, the callback supplies:

```text
decompose=false
position/rotation/scale/skew/perspective=G_EX_COMPONENT_INTERPOLATE
```

It does not change the matrix ID, call hash, transform-ID map, candidate
selection, one-to-one allocator, selected previous transform, or V4 match
telemetry. The diagnostic counters are:

```text
INTERP_MODE_CONTROL_COUNT
INTERP_MODE_SIMPLE_OVERRIDE_COUNT
INTERP_MODE_OVERRIDE_AMBIGUOUS_COUNT
```

The V4 counters remain active, so one future B run can prove identity is
unchanged while comparing the visual result. CPU-side post-match matrix
calculation is observable in RT64, but no additional matrix witness was added
in this narrow checkpoint.

```text
RT64_TOKEN_SCOPED_MODE_OVERRIDE_FEASIBLE: YES
AUTO_ID_SIMPLE_GROUP_IS_VALID_AB: YES (static, not selected)
FIRST_AB_CHANGES_IDENTITY: NO
FIRST_AB_CHANGES_INTERPOLATION_MODE: YES
AB_SWITCH_IMPLEMENTED: YES
CONTROL_PATH_UNCHANGED_WHEN_OFF: YES
IDENTITY_MATCHING_UNCHANGED: YES
AB_RUNTIME_CAN_PROVE_MATCH_IDENTITY_UNCHANGED: YES
POST_MATCH_RESULT_CPU_OBSERVABLE: YES
POST_MATCH_MATRIX_WITNESS_IMPLEMENTED: NO
```

## Validation

```text
BUILD_RESULT: PASS (`cmake --build build --target TooieRecompiled -j2`)
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
TOOIE_DIFF_CHECK: PASS
RT64_DIFF_CHECK: PASS
USER_MANUAL_TEST_REQUIRED_NOW: YES (one B run only)
PRODUCTION_FIX: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

The remaining graphical root is still `LOW` confidence. If SIMPLE mode is
visually better, that supports a post-match interpolation hypothesis but does
not authorize a production change. If it is unchanged, do not add explicit
IDs from this result; investigate a later rendering layer.
