---
title: "Known extension points"
description: "Proven roles vs potential modding uses. No production renames."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-09-02
tags: [reverse-engineering, modding]
---

# Known extension points

Each row: **PROVEN ROLE** first, then **POTENTIAL MODDING USE**
(`HYPOTHESIS`).

## bawobble

- **PROVEN ROLE:** locomotion-gated type-3 Euler on boneAnimID 3 (hip).
- **POTENTIAL USE:** adjust secondary walk sway; change amplitude/rates
  if variables become known; apply wobble to other parts.

## `func_800E0B8C` (`PROPOSED_ALIAS` `model_bone_apply_additive_euler`)

- **PROVEN ROLE:** queue additive Euler on a boneAnimID.
- **POTENTIAL USE:** manipulate additive transforms on boneAnimID
  targets (snout 42, etc.) without renaming the symbol.

## Model resource registry

- **PROVEN ROLE:** IDs in [[../assets/resource-ids.csv]].
- **POTENTIAL USE:** model replacement; child geometry edits. Class
  depends on live ID (`0x61F` ExtraSegments vs `0x62A` GeoCmd).

## GeoCmd / node mapping

- **PROVEN ROLE:** compact `0x02` bone (`func_800DCF48` live type-2 slot);
  snout `0x62B+0x11CB8` is a sibling in the neck child list; fat dual-mtx
  `0x02` (`func_800E2AA4`, not live `0x62B` snout); `0x0A` tags; `0x11`/
  `0x0C`/`0x03` DLs.
- **POTENTIAL USE:** add/remove/move parts; attachment manipulation on
  GeoCmd models only.

## Player draw (`func_80092444` / `func_80817180` / `func_800DE448`)

- **PROVEN ROLE:** core player-table submit is `func_80092444` → DE448
  at `0x800926D8` (`func_80092B04` model_bin). Overlay 118
  `func_80817180` is a separate DE448 site.
- **POTENTIAL USE:** filtered replacement of the live body at DE448 5th
  arg or at `func_80092B04`. Prefer player-table scope over generic
  D674C traffic and over `ctx->r31`.

## Backpack (`babackpack` / `0x621`)

- **PROVEN ROLE:** attached gameplay backpack draw.
- **POTENTIAL USE:** backpack mesh/state replacement. Independent of
  Banjo body ExtraSegments/GeoCmd fork.

## Widescreen / projection (P08, already production)

- **PROVEN ROLE:** `func_800CAA24` map vis; `func_800CACEC` object
  frustum Expand; `func_800E44FC` T_INTRO true source aspect.
- **POTENTIAL USE:** further presentation tweaks. Not a P09 sway seam.
