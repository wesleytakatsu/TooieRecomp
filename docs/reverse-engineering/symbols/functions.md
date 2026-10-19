---
title: "Function registry"
description: "Seeded function catalog. Machine-searchable copy: functions.csv."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-09-02
tags: [reverse-engineering, symbols]
---

# Function registry

Canonical table: [`functions.csv`](functions.csv).

GeoCmd walker: `func_800DE2A4` (list-head `a1`; siblings stay inside the
call). Compact type-2 bone: `func_800DCF48` / `func_800ADE2C`. Fat dual-mtx
type-2: `func_800E2AA4` (`STATIC_PATH_CANDIDATE`; not live `0x62B` snout).
Type `0x11` visible DL: `func_800DD65C` (`G_DL` via `D_8012C960` +
`(lh(node+8)<<3)+8`). Type `0x18` join DL: `func_800DD998` (`G_DL` then
restore `G_MTX DA380002`). CPU compose: `func_80019AA0` into `D_8007BCB0`
stack; `mlMtxApply` / `guMtxF2L`; G_MTX operand is `gfx_ctx+4`.

Do not populate thousands of unknown functions. Add a row when a role is
known at `PROVEN` / `STRONG_INFERENCE` / useful `HYPOTHESIS`.

Subsystems: `PLAYER` `ANIMATION` `MODEL` `GEOCMD` `RENDER` `RESOURCE`
`AUDIO` `MAP` `OVERLAY` `OTHER`.

Production symbols stay as `func_*`. Readable names in [[glossary]] are
`PROPOSED_ALIAS` only.
