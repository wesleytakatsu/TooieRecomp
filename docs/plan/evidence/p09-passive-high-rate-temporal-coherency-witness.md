---
title: "P09 passive high-rate temporal coherency witness"
description: "Default-off CPU reconstruction of the live intermediate clip equation. Capture: 58.596x explosion. No weight change."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-09
tags: [tooie-recomp, p09, kazooie, feather, interpolation, high-framerate, coherency]
---

# P09 passive high-rate temporal coherency witness

Schema: `P09_PASSIVE_HIGH_RATE_TEMPORAL_COHERENCY_WITNESS`.

No GPU readback, copy, layout, sync, staging, or occlusion query.
No temporal-weight mutation. No per-component CURRENT override.
User capture is in `/tmp/tooie-feather-temporal-coherency-status.txt`.
Interpretation: [[p09-world-occurrence-identity-root-cause-audit]],
[[p09-boned-interpolation-counterfactual]].
Schema version 2 adds analysis-only SIMPLE (`lerpMatrixComponents`)
and does not feed it to rendering. Prior capture is `VERSION=1`.

## USER_OBSERVED (prior, still binding)

```text
NORMAL_HIGH_RATE: GLITCH
ORIGINAL_FRAMERATE: FIXED
V5C_ALL_9: SAME
GLOBAL_CURRENT_FRAME: FIXED
PROJECTION_ONLY: INVALID_TEMPORALLY_INCOHERENT
```

## V7 reconciliation (`PROVEN` prior / `UNKNOWN` this capture)

V7 (`FULL_VISIBLE_FEATHER_CPU_HANDOFF_STATUS=CORRECT`,
`VISIBLE_REQUIRED_MATCH_DROP_COUNT=0`) validated **current-frame /
endpoint** handoff of the fourteen exact DLs. This witness does **not**
reopen that. It reconstructs the **normal high-rate intermediate**
sample after `GameFrame::match` and after Projection/Transform
processors, using live `lerpWorldTransforms` /
`modViewProjTransforms` / post-match `velFloats`.

## Implementation (`PROVEN`)

Default-off env `TOOIE_DIAG_FEATHER_TEMPORAL_COHERENCY=1`
(alias `RT64_DIAG_FEATHER_TEMPORAL_COHERENCY`). Cached enable.
Status: `/tmp/tooie-feather-temporal-coherency-status.txt`.

Hook: `WorkloadQueue::threadRenderFrame` after processors, next to the
existing Q witness. Analysis only; production matrices and weights
unchanged.

Identity: existing V7 full-width rule
`current_dl == physical(model_bin)+0x2670+logical*8` for the fourteen
V7 DLs, plus the same formula for the user-listed inventory extras
`0x4D8,0x4F0,0x508,0x520,0x5BE,0x5D6,0x5EE,0x606`. No low-address
match.

Live reconstruction (analysis only, not rendered):

```text
local = pos - vel*(1-w)
clip  = viewProj(w) * world(w) * local
ndc   = clip.xyz / clip.w
```

Offline variants A/B/C/D on exploding triangles only:

- A: interpolated viewProj + interpolated world + actual vel
- B: current viewProj + interpolated world + actual vel
- C: interpolated viewProj + current world + actual vel
- D: interpolated viewProj + interpolated world + vel=0

## Capture fields (UNKNOWN until user run)

```text
PASSIVE_COHERENCY_DIAGNOSTIC_IMPLEMENTED: YES
VISIBLE_WORLD_TRANSFORM_SET: UNKNOWN
UNEXPECTED_WORLD_TRANSFORM_COUNT: UNKNOWN
VISIBLE_WORLD_AUTO_PAIRING_STABLE: UNKNOWN
VISIBLE_WORLD_CROSS_IDENTITY_PAIRING_OBSERVED: UNKNOWN
VIEW_MATCH_STABLE: UNKNOWN
PROJECTION_MATCH_STABLE: UNKNOWN
VIEWPROJ_PAIRING_ANOMALY_OBSERVED: UNKNOWN
VIEWPROJ_DEGENERACY_OBSERVED: UNKNOWN
VISIBLE_FEATHER_EFFECTIVE_VELOCITY_ALWAYS_ZERO: UNKNOWN
MATCH_WRITTEN_VELOCITY_ON_VISIBLE_FEATHERS: UNKNOWN
MAX_INTERMEDIATE_EDGE_EXPANSION_RATIO: UNKNOWN
MAX_INTERMEDIATE_TRIANGLE_EXTENT_EXPANSION_RATIO: UNKNOWN
INTERMEDIATE_VERTEX_NONFINITE_COUNT: UNKNOWN
INTERMEDIATE_GEOMETRIC_EXPLOSION_OBSERVED: UNKNOWN
CPU_TEMPORAL_POSITION_MATH_EXPLAINS_VISUAL_GLITCH: UNKNOWN
FIRST_CPU_TEMPORAL_CAUSAL_TERM: UNKNOWN
```

If the CPU reconstruction does **not** explode while the user still
sees 10–30× stretch in the same normal high-rate mode, the next class
is post-CPU association (output buffer / world slot reuse / present
lifetime), still without GPU readback.

## User command

Walk Kazooie for several seconds, then exit. No visual classification.

```text
rm -f /tmp/tooie-feather-temporal-coherency-status.txt
TOOIE_DIAG_FEATHER_TEMPORAL_COHERENCY=1 ./build/TooieRecompiled
```

Do **not** set `TOOIE_DIAG_FORCE_CURRENT_FRAME` or
`TOOIE_DIAG_FEATHER_NO_INTERP`.
