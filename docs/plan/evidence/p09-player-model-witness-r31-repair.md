---
title: "P09 player model witness — retire r31 filter"
description: "Generated LOOKUP_FUNC at 0x800926D8 does not assign ctx->r31. Player body DE448 is scoped via D_80135490 + func_80092444 + func_80092B04."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-02
tags: [tooie-recomp, p09, de448, r31, player-model]
---

# P09 player model witness — retire r31 filter

No production fix. Generated `RecompiledFuncs/**` not edited.

## Failed capture (previous filter)

```text
DE448_TOTAL_SEEN=31020
PLAYER_RENDER_HIT_COUNT=0
DE448_REJECTED_RA=31020
```

Hard-coded `ctx->r31` whitelist `0x808172A0 / 0x808172F4 / 0x8081A4D4`
rejected every call. Those values are overlay-118 guest RAs, not a
generated host identity for `func_80092444`.

## Generated callsite (`PROVEN` `STATIC_MIPS` / generated C)

`RecompiledFuncs/funcs_385.c` `func_80092444`:

```text
// 0x800926C8: addiu $a0, $sp, 0x60     ctx->r4 = sp+0x60
// 0x800926CC: addiu $a1, $sp, 0x6C     ctx->r5 = sp+0x6C
// 0x800926D0: lw    $a2, 0x50($sp)     ctx->r6 = scale
// 0x800926D4: addiu $a3, $sp, 0x54     ctx->r7 = sp+0x54
// 0x800926D8: jal   0x800DE448
// 0x800926DC: sw    $t6, 0x10($sp)     5th arg = 0x3C($sp)
LOOKUP_FUNC(0x800DE448)(rdram, ctx);
```

No `ctx->r31 = ...` at this jal. Ordinary MIPS RA would be `0x800926E0`.
`LOOKUP_FUNC` does not make `ctx->r31` a reliable generated-callsite
identity (`PROVEN`; same class as P08 `func_80013A7C` LOOKUP_FUNC(0)).

Fifth argument is `v0` of `func_80092B04(a0=PlayerState, a1=1)` stored at
`0x3C($sp)`. That getter uses `lh(*(PlayerState+0x50)+0xC)`: if `==0x607`
then `func_80093F7C`, else `func_800D674C(id)`.

Resource ID is **not** a DE448 register. Owner is `s0` of `func_80092444`
(`a0` at entry), not passed into DE448.

## Chain (`PROVEN` `STATIC_MIPS`)

```text
func_800E9F20     // frame/render pipeline (not player-only)
  func_800F5008   // iterate D_80135490[0..7]; 80098BA8 if 80092258==0xFF
  func_800F50D0   // deferred 80098BA8 for remaining table members
    func_80098BA8 // gate func_80091E80(a0,1); then 80092444
      func_80092444  // unique LOOKUP_FUNC caller of 80092444
        func_800DE448 at 0x800926D8
```

`func_80092444` is **not** exclusively “Banjo” in the overlay-118 sense;
it is player-table object draw. Old `80092444 → LOOKUP_FUNC(0x80084500)
babackpack` remains `REFUTED_FOR_THAT_RUNTIME_PATH`. Independent of this
DE448.

## Repaired diagnostic

`TOOIE_DIAG_SECONDARY_SWAY=1` (default off):

- wrap `func_80098BA8` → `OUTER_PLAYER_SCOPE_HITS` if `a0 ∈ D_80135490`
- wrap `func_80092444` → TLS scope push/pop (depth 32)
- wrap `func_80092B04` → stash model_bin when `a0==scoped player && a1==1`
- wrap `func_800DE448` → sample iff 5th arg matches stashed bin

`OLD_R31_FILTER: RETIRED`. No r31 histogram.

Live body still `UNKNOWN` until the next user walk.
