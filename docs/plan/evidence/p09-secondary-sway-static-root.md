---
title: "P09 secondary-sway static root — Banjo snout"
description: "0x56 is a walk animation. Wobble part 3 is hip boneAnimID 3. Snout is boneAnimID 42. Expected walk body 0x61F MEDIUM. Model-id witness exists default-off; live id unrun."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-02
tags: [tooie-recomp, p09, snout, wobble, dual-mtx, geocmd]
---

# P09 secondary-sway static root — Banjo snout

Bounded static RE. No production fix. Player DE448 model-id witness now
exists default-off (`TOOIE_DIAG_SECONDARY_SWAY`); it is **not** a snout
matrix hook. Operational current state: [[p09-session-handoff]].

`D_80801448[0x6]/[0xF]=0x56` as Banjo **body model** is `CORRECTED/SUPERSEDED`: it is the walk **animation**. Geometry candidates are `0x61F` (ovl118 BK slot, ExtraSegments, no GeoCmd 0x02) and `0x62A` (GeoCmd 0x02 snout node). Type-3 consumer is `func_800E0E94` via `func_800AF04C` matching bone `+0xC` = boneAnimID.

```text
BANJO_WALK_ANIM_RESOURCE_ID: 0x56
MUMBO_ANIM_RESOURCE_ID: 0xEC
KAZOOIE_ANIM_RESOURCE_ID: 0x13A
BANJO_LIVE_WALK_MODEL: 0x61F STRONG_INFERENCE / MEDIUM static; live UNKNOWN
PLAYER_MODEL_ID_WITNESS_IMPLEMENTED: YES (default-off; see HANDOFF)
LOCOMOTION_SECONDARY: bawobble -> 800E0B8C(part=3 type=3) -> 800E0E94 in 800AF04C
FUNC_800E0B8C_PART_ID_SEMANTICS: boneAnimID
PART3_TARGET: boneAnimID 3 HIP  (0x61F/0x62A index 1)
BANJO_SNOUT_NODE: boneAnimID 42  (0x61F idx 4; 0x62A idx 10 GeoCmd 0x02 DL198)
BANJO_SNOUT_CONFIDENCE: MEDIUM
PART3_TAG5_RELATION: REFUTED (identity)
WOBBLE_PART3_DRIVES_SNOUT: INDIRECT
BANJO_SNOUT_USES_E2AA4: NO if 0x61F; YES if 0x62A
FILTER_CONFIDENCE: LOW (cannot distinguish state vs tagging until live model is known)
NEW_SNOUT_WITNESS_IMPLEMENTED: NO
CANDIDATE_IMPLEMENTED: NO
ROOT_CANDIDATE: MORE_EVIDENCE_REQUIRED
ROOT_CANDIDATE_CONFIDENCE: LOW
ARTIFACT: /tmp/p09-banjo-wobble-part3-20260902T060241Z/
GPU_READBACK_USED: NO
```

Do not treat `INTERPOLATION_TAGGING_BUG` as proven.
Do not filter E2180 under resource 0x56.
Do not equate wobble part 3 with tag 5 or with the snout node.
Do not revive D674C 0x621-first, `func_80092444`, overlay 456, `0x80800320`, syscall 113/3.

Full tables and four-case comparison live in the artifact directory.
