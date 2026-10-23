---
title: "Reverse-engineering knowledge base"
description: "Current best model of Banjo-Tooie for TooieRecomp and future mods. Not forensic evidence."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-09-09
tags: [reverse-engineering, tooie, modding]
---

# Reverse-engineering knowledge base

This tree answers: **what do we currently know about the game?**

How a claim was proven or refuted (dumps, A/B runs) is recorded in commit
messages; the old `docs/plan/evidence/` folder was removed on 2026-10-23 and
stays in git history. Specs stay in `docs/specs/`. Current state stays in
`docs/HANDOFF.md`.

Do not copy full evidence logs here. Link them as provenance.

## Truth status

Every factual claim should carry one of:

| Status | Meaning |
| --- | --- |
| `PROVEN` | Primary evidence in-repo |
| `STRONG_INFERENCE` | Consistent static/runtime picture; consumer or live hit not yet sealed |
| `HYPOTHESIS` | Plausible, not established |
| `UNKNOWN` | Not determined |
| `REFUTED` | Primary evidence against |
| `CORRECTED` | Earlier claim replaced; see [[corrections]] |

Optional provenance tags: `STATIC_ROM`, `STATIC_MIPS`, `RUNTIME_CPU`,
`USER_OBSERVED`, `BANJORECOMP_STRUCTURAL_ANALOG`, `OTHER`.

## Layout

- [[glossary]]
- [[corrections]]
- [[symbols/functions]] / `symbols/functions.csv`
- [[symbols/overlays]]
- [[symbols/structs]]
- [[assets/README]]
- [[characters/banjo]] / [[characters/kazooie]] / [[characters/mumbo]]
- [[systems/resource-system]] / [[systems/animation]] /
  [[systems/secondary-animation]] / [[systems/model-renderer]] /
  [[systems/geocmd]] / [[systems/transform-interpolation]] /
  [[systems/overlays]] / [[systems/audio-sequence-volume]]
- [[p09-v3-real-guest-matrix-match]] — distilled V3 guest-matrix proof; its temporal seam is superseded by the V4 real-match witness.
- [[characters/kazooie]] — live `0x62B` right/left plumes + crest DLs; V7 VERSION=3 admits all 14; downstream closed; guest compose `parent⊗child` XYZW; clip/sample time open
- [[systems/geocmd]] — compact DCF48 + type-0x11/`0x18`; plume GeoCmd parent 107
- [[systems/animation]] — AE820 local 4x4 / first-class scale channels
- [[systems/transform-interpolation]] — RT64 AUTO match; V5C nine-bone freeze `SAME`; global CURRENT `FIXED`; projection-only `INVALID`; BK tagging gap `PROVEN`
- [[modding/README]]

## Seeded from

P09 locomotion secondary-sway investigation and stable P08 renderer /
GeoCmd findings. Character, systems, and modding pages are seeded.
Live BK walk body resource is `0x62B` **PROVEN_FOR_CAPTURE**; static `0x61F`
remains a separate `STRONG_INFERENCE` / `MEDIUM` table expectation.
