---
title: "Modding map"
description: "How current RE knowledge maps to future mods. Not a shipped API."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-10-28
tags: [reverse-engineering, modding]
---

# Modding map

TooieRecomp has no public mod API yet; only `.rtz` texture packs and language packs are supported
(see `specs/Tooie/modding-and-extensions.md`).
This tree records **seams that already have proven roles**, plus
**potential uses** that must stay labelled `HYPOTHESIS`.

Canonical facts: [[../README]]. Correction log: [[../corrections]].
Extension list: [[known-extension-points]].

Rules for agents/modders reading this:

1. Never treat `PROPOSED_ALIAS` as a renamed production symbol.
2. Never treat a clip ID as a mesh (`0x56` / `0xEC` / `0x13A`).
3. Live BK walk body is `0x61F` `STRONG_INFERENCE` until the
   `TOOIE_DIAG_SECONDARY_SWAY` witness runs.
4. ExtraSegments vs GeoCmd `0x02` changes which renderer hooks matter.
