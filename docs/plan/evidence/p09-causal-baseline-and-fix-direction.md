---
title: "P09 causal baseline and fix direction"
description: "Causal baseline established by diagnostic controls; fix direction grounded in composed WORLD root."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-10-15
tags: [tooie-recomp, p09, interpolation, causal-baseline, diagnostic-controls, rt64]
---

# P09 causal baseline and fix direction

## Decisive diagnostic controls (USER_OBSERVED PASS)

Glitch elimination was decisively proven via two diagnostic commands:
1. `TOOIE_DIAG_FORCE_CURRENT_FRAME=1 ./build/TooieRecompiled`
   - Scope: global weights (`rt64_workload_queue.cpp:24-34,455-456`) forced to `curFrameWeight=1, prevFrameWeight=0` across Projection, Transform, Tile, LookAt, RSP, Vertex. Present cadence and extra presents remain (`p09-high-rate-force-current-frame.md`). Diagnostic only; repeats current game frame.
   - Result: `GLOBAL_CURRENT_FRAME: FIXED` (`p09-temporal-coherency-root-cause.md:20-50`).
2. `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1 ./build/TooieRecompiled`
   - Scope: shader-local (`rt64_rsp_processor.cpp:60-67,7161`, `RSPProcessCS.hlsl:78-84`). Replaces `tfPos` with endpoint CURRENT (`endpointViewProj · endpointWorld · pos`). Position interpolation is bypassed (does not preserve real position interpolation). Fog derives from `tfPos.z/w` so downstream fog follows CURRENT. Texgen `tc` still uses `curFrameWeight`. Projection, Transform, Tile, LookAt, Vertex weights unchanged.
   - Result: `RSPPROCESS_TFPOS_CAUSAL: PROVEN`, `SHADER_LOCAL_CURRENT_VISUAL_RESULT: PASS` (`p09-projective-w-production-fix.md:365-376,709-717`).

Together these prove causal localization of the tested visible defect to interpolated vertex screen position (`tfPos`), rather than subsequent raster, depth, or presentation stages, but this does not prove a unique detailed formula nor establish global exclusion of every possible downstream defect.

## Established root cause class

The accepted causal class is:
```text
ROOT_CAUSE_CLASS=NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS
```
Proven via repaired V2 CPU pairing (`p09-elementwise-world-interpolation-causal-discriminator.md`, 2026-09-16) with row-vector `mul(pos4, mul(WORLD, VIEWPROJ))` matching Vulkan SPIR-V (`p09-cpu-diagnostic-shader-semantics-repair.md`). Articulated parent/child attachments agree at both endpoints but diverge mid-step under independent `RigidBody::lerp` decomposition on already-composed WORLD matrices. No parent hierarchy exists in `TransformProcessor`.

## Failed and refuted production attempts

| Attempt | Outcome / Reason for Refutation |
| --- | --- |
| Projection-only CURRENT | `INVALID_TEMPORALLY_INCOHERENT` (severe ghosting/overlay; `p09-temporal-coherency-root-cause.md:28-29`). Removed. |
| Endpoint W-sign crossing (`prevW*curW<=0`) | Visual `FAIL` (`ENDPOINT_SIGN_CROSSING_ONLY_POLICY: VISUALLY INSUFFICIENT`). Branch removed (`RSPProcessCS.hlsl:74-77`). Old default-ON text in `p09-projective-w-production-fix.md` is `CORRECTED/SUPERSEDED`. |
| CLIP_LERP / NDC_LERP | Closed 2026-09-14 (`p09-articulated-temporal-coherence-root.md:140-175`). Rigid coherence `BAD`; does not beat baseline. |
| Global elementwise WORLD | Fixes joins (join max `1.0001`), but shears rigid meshes. Cannot ship (`transform-interpolation.md:33-62`). |
| Slot-global selective elementwise | `PARTIAL_FIX_WITH_REGRESSIONS` (`p09-selective-world-selector-granularity-audit.md`): `USER_OBSERVED` feather, backpack, and Mumbo-feather improvements, while snout remained glitched, along with new Klungo cuts and occasional Mumbo disappearance. Slot policy applies to all consumers of selected WORLD (`PROVEN_STATIC`); attribution of observed regressions specifically to shared-slot leakage is `STRONG_INFERENCE` (consistent), not separately proven causal attribution. |
| Multi-only elementwise WORLD | `INSUFFICIENT_GRANULARITY` (`docs/HANDOFF.md`, 2026-09-24): feathers return while regressions persist. |
| Nearest-NDC / Point-coincidence edge join | Point coincidence is not seam identity (`p09-seam-alias-dry-run-validity-review.md`). Saturated by 194 ties (`PATH_PAIR_AMBIGUOUS_TIE=194`), blocking argmin authorization. |

## Multi-agent arbitration and provenance

- **Haiku report** (`haiku-endpoint-causal-output.json` RESULT): accurately mapped flag mechanics, push constants, and cbuffer fields. However, recommended new visual discriminator experiments for CLIP_LERP/NDC_LERP. This recommendation is arbitrated as `SUPERSEDED` by the 2026-09-14 closure in `p09-articulated-temporal-coherence-root.md` and subsequent selector audits. No further shader-formula or projective-W experiments will be pursued. Haiku noted a jCodeMunch policy violation during source review. Shader binary mtime is noted, but no new SPIR-V link proof was established.
- **Grok report** (`grok-endpoint-investigation-output.json` text): correctly retrieved the 2026-09-14 closure, identified the established composed-WORLD causal class, explained why slot-global lerp leaks across calls, and detailed why the 194 ties block nearest-NDC alias authorization without reopening root cause investigation. Both reports preserved as provenance.

## Current implementation boundary

- `CAUSE_ALREADY_ESTABLISHED: YES` (`NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`).
- `PRODUCTION_FIX_NOT_IMPLEMENTED: YES` (no production aliases, vertex cloning, or lerp remaps exist).
- Call-local architecture B+C (reserve `(GameCall, original WORLD)` alias and clone only conflicting vertices) remains `CONDITIONAL`: requires certified paired-edge physical world-space match before upload; `State::fullSync` uploads draw data before `TransformProcessor`, so render-time remap alone is invalid.
- Existing 194-TIE capture (`capture-gameplay-20261015.txt`, SHA256 `b6be018a...`) is an exact-use diagnostic blocker for nearest-NDC join authorization onto alias keys, NOT an unknown root cause.

## Single next discriminator

`NEXT_SINGLE_ACTION` (historical, completed): execute a bounded default-OFF CPU log of the first ~20 tie groups in RT64 `p09EmitNearestJoinPairs` / correlation helper, logging full 9-field keys (`workloadId`, `fbPair`, `projection`, `gameCall`, `triangleLocal`, `corner`, `faceCorner`, `storedVertex`, `originalWorld`), PREV/CURRENT positions, `gap`/multiplicity, and candidate membership in the V2 certified paired-edge plan. Classify entries into `SAME_EXACT_USE_REPEAT`, `DISTINCT_LEGITIMATE_USE`, or `UNKNOWN`.
This diagnostic serves selection and authorization for the known cause; it is not a fresh search for root cause. No tie suppression, no nearest-NDC alias authorization, and no production remap/lerp edits. Instrumentation remains `NOT_IMPLEMENTED` this turn.

- **Latest result (2026-10-16)**: first-20 tie groups diagnostic captured and arbitrated in [[p09-pathological-tie-groups]]. Active next single action updated to: a READ-ONLY focused audit of existing paired-boundary-edge/provenance data and its correlation to the sampled pathological uses, starting with producer/topology semantics and what current artifacts actually contain; no new lerp/alias or telemetry implementation automatically.
