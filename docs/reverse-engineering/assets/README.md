---
title: "Assets and resources"
description: "Resource ID catalog. Types are never inferred from the ID number alone."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-09-02
tags: [reverse-engineering, assets]
---

# Assets and resources

Machine table: [`resource-ids.csv`](resource-ids.csv).

Asset table (cart): base `0x5188 + id*4`, payload
`ROM = 0x12B24 + (entry>>8)*4`, skip-2 raw deflate. Flag `0x16` →
animation-like; flag `0x10` + magic `0000000B` → model. `PROVEN`
(`STATIC_ROM`).

Never identify a resource as a character **model** without the
`0000000B` header (or equivalent live fingerprint).

[[animations]] · [[models]]
