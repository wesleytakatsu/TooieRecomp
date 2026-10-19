---
title: "P09 Kazooie feather V5 full-chain and post-match witness"
description: "Static audit and diagnostic implementation for the seven visible 0x62B feather bones."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-04
tags: [tooie-recomp, p09, kazooie, feather, rt64, post-match, interpolation]
---

# P09 Kazooie feather V5 full-chain and post-match witness

## Later result (V5B)

The full-chain no-interpolation capture was later completed with complete
seven-bone masks (`0x7F`) and `RESULT==CURRENT` 211/211. User visual
`SAME`. That negative result is canonical in
[[p09-kazooie-feather-v6-post-matrix-vertex-path]]. The remainder of this
note is the V5 implementation checkpoint and must not be read as leaving
full-chain interpolation open.

## Scope

This checkpoint is static analysis plus diagnostic implementation only. The
game was not run, no user capture was requested, and no GPU readback,
synchronization, copy, staging, or query was added. No production animation or
interpolation behavior was changed.

## Previous A/B scope

The earlier `TOOIE_DIAG_FEATHER_INTERP_SIMPLE=1` seam was called inside
`GameFrame::matchTransform` and required the unique primary token
`0x62B/+0x124A0`, index 30, `boneAnimID=103`. Its scope was therefore:

```text
PREVIOUS_SIMPLE_TEST_SCOPE: ROOT_ONLY
FULL_SKELETAL_SIMPLE_HYPOTHESIS_NOT_TESTED: YES
SIMPLE_OVERRIDE_BONES: 103 only
SIMPLE_OVERRIDE_DL_RANGES: the matched transform's runtime range; static visible set 0x5AA/0x592/0x57C/0x534
```

The user's result `SAME` refutes only that root-only experiment, not the
full-chain or post-match hypotheses.

## Full static ownership map

The seven bounded identities are all compact type-2 DCF48 nodes in live model
`0x62B`. The skeletal parent of both plume roots is index 25 / anim 107.
The display-tree nesting is separate from that skeletal parent relation.

| boneAnimID | bone index | GeoCmd node rel | parent / child | matrix class / G_MTX scope | first known G_VTX / draw ownership |
| ---: | ---: | ---: | --- | --- | --- |
| 79 | 27 | `0x12368` | head 108 display child / crest analog | single DCF48; node subtree | child subtree; dedicated DL not separately proven |
| 100 | 33 | `0x12590` | plume root, child 101 | single DCF48; node subtree | child subtree; dedicated DL not separately proven |
| 101 | 34 | `0x125A0` | child 100 → child 102 | single DCF48; node subtree | child subtree; dedicated DL not separately proven |
| 102 | 35 | `0x125B0` | child 101 | single DCF48; node subtree | child subtree; dedicated DL not separately proven |
| 103 | 30 | `0x124A0` | root, child 104 | single DCF48; modelview-parallel stack | `0x534` first in nested chain; own `0x5AA` at `+0x12580`, join `0x592` at `+0x12550` |
| 104 | 31 | `0x124B0` | child 103 → child 105 | single DCF48; nested child scope | `0x534` through child 105; own `0x57C` at `+0x12540`, join `0x564` |
| 105 | 32 | `0x124C0` | child 104 | single DCF48; distal child scope | own `0x534` at `+0x124D0`, join `0x54C` |

For 103–105, the complete proven display subtree is:

```text
103 +0x124A0 -> G_MTX -> 104 +0x124B0 -> 105 +0x124C0
  105 child +0x124D0 type-0x11 dl=0x534
  105 sibling +0x124E0 type-0x18 dl=0x54C
  104 sibling +0x12510 type-0x18 dl=0x564
  104 sibling +0x12540 type-0x11 dl=0x57C
  103 sibling +0x12550 type-0x18 dl=0x592
  103 sibling +0x12580 type-0x11 dl=0x5AA
```

The left plume and crest node identities are statically proven, but their
dedicated child-DL ownership is not present in the current bounded geometry
dump. The V5 runtime token/vertex range witness is deliberately retained to
resolve that remaining display ownership without broadening the filter.

## Token and semantic coverage

`classify_kazooie_feather` already accepts exactly the seven node/index/anim
tuples above. The V4 modelview-parallel token stack therefore generalizes to
all seven without changing the G_MTX key, stack lifetime, or guest matrix
producer. The new status emits one line per bone:

```text
BONE<anim>_AUTO_STATUS=REAL_CALL_MATCHED,REAL_CALL_UNMATCHED,
  REAL_MATCHED,REAL_UNMATCHED,SEMANTIC_MATCH_CORRECT,
  SEMANTIC_MATCH_FAILURE,SEMANTIC_CROSS_MATCH,SEMANTIC_DROPPED_MATCH
```

```text
FULL_CHAIN_V4_TOKEN_SUPPORT: YES (static implementation)
```

The counters are populated by the existing real RT64 CALL, decision, and
`matchTransform` callbacks. They do not synthesize a match from a flush
sentinel.

## Post-match interpolation witness

The actual CPU render path is:

```text
GameFrame::match
  -> selected current/previous transform pair
  -> GameFrame::matchTransform
  -> RigidBody update*
  -> TransformProcessor::process
  -> RigidBody::lerp(p.prevFrameWeight / p.curFrameWeight, ...)
```

`p.curFrameWeight` is the current render interpolation weight and is now
captured as `CURRENT_WEIGHT`; `p.prevFrameWeight` is captured as
`PREVIOUS_WEIGHT`. The new bounded CSV is:

```text
/tmp/tooie-kazooie-feather-postmatch.csv
```

It records the raw previous/current guest world matrices, the actual final
CPU matrix used by the transform processor, workload/transform/group identity,
previous transform selection, and the no-interpolation flag. This is
`POST_MATCH_RESULT_CPU_OBSERVABLE: YES` and
`POST_MATCH_MATRIX_WITNESS_IMPLEMENTED: YES`. The CSV also emits
scale-normalized quaternion triplets with hemisphere normalization and the
previous→current, previous→final, and final→current rotation angles. Raw
matrices remain the authoritative witness.

`GAME_UPDATE_VS_RENDER_FRAME_DISTINGUISHABLE` is `PARTIAL`: the workload and
render weights are CPU-observable, while this seam does not invent a separate
guest update counter.

## Full-chain no-interpolation A/B

`TOOIE_DIAG_FEATHER_NO_INTERP=1` is default-off. After the existing match has
selected the pair, it applies only to the seven classified feather tokens and
sets both the rendered current and previous upload matrices to the already
selected current guest transform. It does not alter:

- G_MTX identity or call hashes;
- candidate selection or one-to-one allocation;
- previous-transform selection;
- guest animation or guest matrices;
- any non-feather transform.

```text
FULL_CHAIN_NO_INTERP_AB_IMPLEMENTED: YES
IDENTITY_MATCHING_UNCHANGED: YES
GUEST_MATRICES_UNCHANGED: YES
ONLY_TEMPORAL_INTERPOLATION_REMOVED: YES
NO_INTERP_AB_CAN_PROVE_TEMPORAL_INTERPOLATION_CAUSALITY: YES
```

This A/B is more decisive than the previous root-only SIMPLE test because it
removes temporal interpolation for the whole visible feather identity set,
including the distal 104/105 chain.

## Hierarchical interpretation

RT64's `worldTransforms` are the effective model-view/world matrices captured
when `setVertexCommon` materializes a vertex transform. The guest DCF48 path
feeds a composed model-view matrix into G_MTX, so the render-side witness is:

```text
RT64_INTERPOLATES_LOCAL_OR_COMPOSED_BONE_MATRICES: COMPOSED
INDEPENDENT_COMPOSED_BONE_INTERPOLATION_CAN_DISTORT_CHAIN: YES
```

Independent interpolation of already-composed parent, child, and grandchild
matrices can yield a pose that is not equivalent to recomposing interpolated
local bones. This is a structural mechanism, not yet a proven graphical root.
Banjo has the same effective bone-level G_MTX/world-transform granularity but
adds an explicit matrix-group lifetime around the nested bone path:

```text
BANJO_BONE_INTERPOLATION_GRANULARITY: effective composed world transform per bone
BANJO_GROUP_SCOPE_RELATIVE_TO_NESTED_BONES: explicit group around each bone G_MTX subtree
BANJO_AVOIDS_OR_SHARES_COMPOSED_INTERPOLATION: SHARES effective granularity; group metadata differs
BANJO_ARCHITECTURE_RELEVANCE_AFTER_FULL_CHAIN_AUDIT: MEDIUM
```

The no-interpolation A/B is the next discriminator before any production group
tagging or hierarchy rewrite.

## Validation and gate

```text
BUILD_RESULT: PASS (TooieRecompiled)
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
DIFF_CHECK: PASS (Tooie and external RT64)
DIAGNOSTIC_DEFAULT: OFF
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
PRODUCTION_FIX: NO
NO_COMMIT: YES
NO_PUSH: YES
```

The next user run is authorized only after this build gate, with
`TOOIE_DIAG_SECONDARY_SWAY=1 TOOIE_DIAG_FEATHER_NO_INTERP=1`, reproducing the
same locomotion. Do not enable the earlier SIMPLE switch simultaneously.

## V5 runtime result and correction (2026-09-04)

The fresh capture postdates the current executable and reports
`DIAGNOSTIC_SCHEMA=P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V5` with
`NO_INTERP_AB_ENABLED=YES`. However, it does not prove full-chain coverage:

```text
NO_INTERP_OVERRIDE_COUNT: 1
POST_MATCH_MATRIX_ROWS: 1
RT64_MATCH_MATCHED: 1
RT64_MATCH_UNMATCHED: 0
RT64_MATCH_NO_CANDIDATE_OBSERVED: 970
```

The one observed override/post-match row is boneAnimID 103 at node `0x124A0`.
The producer witness independently contains 1,157 frames for each of
79, 100, 101, 102, 103, 104, and 105, but the RT64 identity and matrix
witnesses contain only boneAnimID 103. The six sibling identities therefore
did not reach the temporal-interpolation hook.

Static cause: `classify_kazooie_feather()` recognizes the sibling identities,
but `secondary_sway_note_dcf48()` sets its `accept` gate only for
`Primary62B/61C/83A`. `Sibling62B` is counted as a discovery hit and then
rejected before composition/token creation. Thus the V5 claim of full runtime
override coverage is **CORRECTED/SUPERSEDED** by this capture; the diagnostic
must be repaired before a causal A/B can close temporal matrix interpolation.

The single overridden row is internally valid: `NO_INTERPOLATION=1`,
`FINAL_MATRIX == CURRENT_MATRIX` to the emitted precision, and
`PREVIOUS_TO_FINAL_ANGLE == PREVIOUS_TO_CURRENT_ANGLE` while
`FINAL_TO_CURRENT_ANGLE == 0`. This proves the hook behavior for bone 103
only, not for the complete visible set. The user result `SAME` therefore
cannot refute temporal interpolation for the visible feather chain.

```text
ALL_VISIBLE_FEATHER_TRANSFORMS_OVERRIDE_ACTIVE: NO
TEMPORAL_MATRIX_INTERPOLATION_ACTUALLY_BYPASSED: PARTIAL (bone 103 only)
FULL_VISIBLE_CHAIN_AUTO_IDENTITY_STATUS: UNKNOWN
NO_INTERP_VISUAL_RESULT_CLASSIFICATION: A — coverage incomplete
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
NEXT_SINGLE_ACTION: repair the diagnostic DCF48 sibling acceptance gate; do not request an identical capture
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
```

## V5B diagnostic gate repair (2026-09-04)

The V5 capture above is retained as evidence of incomplete coverage, not as a
full-chain result. The repair is diagnostic-only and does not change guest
animation, guest matrices, RT64 identity matching, or production behavior.

`secondary_sway_note_dcf48()` now accepts a targeted sibling only when all of
the following hold:

- the live model resource is `0x62B`;
- the resolved `boneAnimID` is one of `79, 100, 101, 102, 103, 104, 105`;
- the existing proven type-2 node/index mapping classifies it as the primary
  `0x62B` feather or `Sibling62B`.

Unrelated `0x62B` siblings remain rejected. The resource ID is copied into
each matrix token, so every tracked bone retains its own node, composition,
F2L destination, G_MTX operand, stack slot, vertex range, workload, CALL, and
`matchTransform` identity. The V4 modelview-parallel stack and bounded
type-0x18 restore alias are unchanged.

The corrected schema is `P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V5B`. Status now
emits `BONE<anim>_STATUS` with `GUEST`, `MATRIX`,
`GMTX`, `VERTEX`, `MATCHED`, `POSTMATCH`, and `NO_INTERP`, plus masks including
`TRACKED_BONES_WITH_GMTX_MASK`, `TRACKED_BONES_WITH_POSTMATCH_MASK`, and
`TRACKED_BONES_WITH_NO_INTERP_MASK`. A complete capture is the seven-bit mask
`0x7F`; any other value self-declares incomplete.

```text
SIBLING62B_TARGETED_ACCEPT_IMPLEMENTED: YES
UNRELATED_62B_SIBLINGS_STILL_REJECTED: YES
BONE79_OWN_GMTX_TOKEN_SUPPORTED: YES
BONE100_OWN_GMTX_TOKEN_SUPPORTED: YES
BONE101_OWN_GMTX_TOKEN_SUPPORTED: YES
BONE102_OWN_GMTX_TOKEN_SUPPORTED: YES
BONE103_OWN_GMTX_TOKEN_SUPPORTED: YES
BONE104_OWN_GMTX_TOKEN_SUPPORTED: YES
BONE105_OWN_GMTX_TOKEN_SUPPORTED: YES
FULL_CHAIN_OWN_TOKEN_SUPPORT: YES
FULL_GPR_POINTER_PRESERVATION_RETAINED: YES
MODELVIEW_STACK_TOKEN_LOGIC_RETAINED: YES
FULL_CHAIN_NO_INTERP_OVERRIDE_SUPPORTED: YES
IDENTITY_MATCHING_UNCHANGED: YES
GUEST_MATRICES_UNCHANGED: YES
FULL_CHAIN_COVERAGE_STATUS_IMPLEMENTED: YES
CAPTURE_CAN_SELF_DECLARE_INCOMPLETE: YES
NEW_SCHEMA: P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V5B
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
DIFF_CHECK: PASS
USER_MANUAL_TEST_REQUIRED_NOW: YES
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

The next user test is authorized now because the static implementation and
CPU-only validation gate passed. After the run, the status file must report
all seven bits in the post-match and no-interpolation masks before its visual
result is interpreted. No game run was performed for this repair.
