---
title: "P08 — session handoff: intro para attract demo"
description: "Estado autossuficiente após PATCH 4: clean-build correction accepted; C2AB8 resolved; current first product divergence is func_800BCF28."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, handoff, intro, attract, preemption, patch4, bcf28]
---

# P08 — session handoff: INTRO → ATTRACT DEMO

## CURRENT_HEAD

- Branch: `main`.
- Independent review base:
  `f81d55289037404fa8f76b860a5c4182108775ad`.
- Pull the documentation commit containing
  [[p08-preemption-patch4-clean-build-independent-review]].

## CURRENT_PHASE

P08 — RT64, input e frontend. PATCH 4 is accepted: the historical verdict C
is preserved, and its tracked-state delivery finding is resolved by the
clean-build correction review (verdict B). Current priority is the first
product divergence, `func_800BCF28`. Title/file-select input continua diferido.

## CURRENT_PRODUCT_STATE

- **`func_800BCF28` SIGSEGV root cause PROVEN** (level A, 2026-08-14): the
  JAL to `0x800BCCC0` / `0x800BCCD8` resolves to the no-op `lookup_gap_stub`
  (`v0=0`) registered in `src/game/boot_hw_stubs.cpp` instead of the real
  element accessor. `func_800BCF28` dereferences `v0 + (index<<2)` as a
  pointer → `rdram+0x8000000c` → SIGSEGV. See
  [[p08-bcf28-crash-diagnosis]]. Fix not implemented (diagnosis only).
- Intro visual completa (céu, logo, Jiggy).
- Attract request correto; type-4 completa; warp consumido; mode-5 alcançado.
- Hybrid F' startup behavior is independently **CLASS A** in the local
  regenerated binary; rollback OFF reproduces the old freeze.
- `func_800C2AB8` is behaviorally resolved with PATCH4 default ON.
- Implementer clean-build correction tracks the empty
  `RecompiledPatches` scaffolds; a disposable clone now regenerates 4,298
  safepoints **and** builds. Independent review verdict B accepts delivery
  and resolves the tracked-state blocker. See
  [[p08-preemption-patch4-clean-build-independent-review]].
- Gameplay demo **não** começa: SIGSEGV em `func_800BCF28`.
- `RecompiledFuncs` é gerado — não editar.

## CURRENT_BLOCKER

**SIGSEGV in `func_800BCF28`** — ROOT CAUSE PROVEN (level A): the JAL target
`0x800BCCC0` (and `0x800BCCD8`) is a real unnamed element accessor with no
generated function, so `get_function` resolves it to the no-op
`lookup_gap_stub<0x800BCCC0>` (v0=0) in `boot_hw_stubs.cpp`. `func_800BCF28`
dereferences that 0 as a pointer → SIGSEGV at `rdram+0x8000000c`. Fix not
implemented.

```text
#0 func_800BCF28
#1 bakey_heldCount
#2 bainput_func_80097B4C
#5 func_800A4B08
#7 basetup_entrypoint_2
#10 gsworldDll_entrypoint_2
#14 func_800A76F4
```

Matches the 2026-08-12 temporary-yield observation. Not a scheduler or
PATCH4 regression.

## PROVEN_ROOT_MECHANISM (old C2AB8)

```text
YES — synchronous dependency cycle / busy-wait, starved because MAIN
held the host execution token in a pure MIPS loop:
  table[2].word0 != 0
  -> C269C(7)==0
  -> C2AB8 outer loop infinite
  -> AUDIO cannot run -> 205D0 never clears table[2]
```

## ROOT_CAUSE_STATUS

```text
ROOT_CAUSE_PROVEN: YES
LOCAL_BEHAVIOR:    FIXED for the observed C2AB8 path (PATCH 4, CLASS A)
DELIVERY_ACCEPTED: YES — clean-build independent review verdict B
```

Same generated binary:

- `ULTRAMODERN_GUEST_PREEMPTION` unset → C2AB8 returns, AUDIO progresses,
  crash in `func_800BCF28`.
- `ULTRAMODERN_GUEST_PREEMPTION=0` → old freeze (d/audio/last_ovl stable ≥58s).

No C2AB8 override, no `yield_self_1ms`, no forced table[2] clear.
`table[2].word0 == 0` is INFERENCE in PATCH 4; resolution of the C2AB8
pending condition is PROVEN.

## CURRENT_FIRST_DIVERGENCE

`func_800BCF28` SIGSEGV — **root cause PROVEN (A)**: `lookup_gap_stub`
masks real accessor `0x800BCCC0`/`0x800BCCD8` → v0=0 → null dereference.
See [[p08-bcf28-crash-diagnosis]]. Not a scheduler regression.

## RESOLVED_DO_NOT_REOPEN

1. Overlay data relocation / `LOOKUP_FUNC(0)`.
2. Actor manager null from artificial spawn.
3. `func800D6E54` as population pass.
4. Resident overlay / gcsection stale mapping.
5. Scheduler queue insert/remove / `No threads left` (without new proof).
6. IPL3 integrity gate.
7. R_MIPS_26 same-section / `do_dma` SIGSEGV.
8. Type-4 incomplete / warp gated as clean-freeze blocker.
9. Warp not consumed as clean-freeze blocker.
10. `func_80026500` list walk as current hang PC.
11. **`func_800A907C -> func_8001EE34` as object driver** — REFUTED.
12. **"table[2] object NÃO está agendado"** — REFUTED.
13. **C2AB8 cooperative freeze as current blocker** — RESOLVED by PATCH 4.
14. Hybrid F' architecture / PATCH 1 / 2 / 3 / 3.5 internals.

Do not reopen these without new evidence.

## MODEL_DISAGREEMENTS

Historical Gemini vs DeepSeek slot7 arbitration is preserved in
[[p08-slot7-object-lifecycle-arbitration]]. Not reopened. PATCH 4 did not
depend on object-origin remaining UNKNOWN.

## OPEN_QUESTIONS

1. **CURRENT BLOCKER:** SIGSEGV `func_800BCF28` — **root cause PROVEN** (level
   A): `lookup_gap_stub` masks real element accessor `0x800BCCC0`/`0x800BCCD8`.
   Fix layer pending (stub impl vs symbol metadata). See
   [[p08-bcf28-crash-diagnosis]].
2. Exact creation epoch/call chain RA for the historical table[2] object.
3. Generic non-RA JR still lacks safepoint coverage (deferred).
4. Timer IM7 / PI completion remain approved approximations (deferred).

## NEXT_SINGLE_ACTION

**Implement the proven minimal `func_800BCF28` binding fix** (restore the two
accessors `0x800BCCC0` / `0x800BCCD8`, either via `boot_hw_stubs.cpp` native
body or via symbol metadata so the generator emits them). PATCH 4 stays ON.
Do not reopen Hybrid F' / PATCH 1–3.5. E.g. diagnostic:
[[p08-bcf28-crash-diagnosis]].

## FILES_TO_READ_NEXT

0. [[p08-bcf28-crash-diagnosis]] — **CURRENT:** root cause PROVEN (A);
   `get_function(0x800BCCC0)` = `lookup_gap_stub` (v0=0)
1. [[p08-preemption-patch4-clean-build-independent-review]] — **CURRENT** verdict B; delivery accepted
2. [[p08-preemption-patch4-clean-build-correction]] — implementer correction
3. [[p08-preemption-patch4-independent-review]] — historical verdict C preserved; finding resolved
4. [[p08-preemption-patch4-validation]] — implementer evidence, preserved
5. [[p08-preemption-implementation-plan]]
6. `docs/HANDOFF.md`, `docs/plans/BT/08-rt64-input-and-frontend.md`

## CLEAN-FREEZE REPRO (rollback)

```sh
ULTRAMODERN_GUEST_PREEMPTION=0 \
TOOIE_AUTO_START=1 \
TOOIE_DIAG=1 \
TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json \
./build/TooieRecompiled
```

PATCH4 default (ON): omit the env var. Expect C2AB8 to return and then
SIGSEGV in `func_800BCF28`.

## CHECKPOINT SCOPE

```text
CODE_CHANGED             = YES (Tooie startup activation + T15)
RUNTIME_CODE_CHANGED     = NO (dirty tree protected, unused by this commit)
GENERATOR_CODE_CHANGED   = NO (already patched; used for regen)
PATCH_CODE_CHANGED       = NO
RECOMPILEDFUNCS          = regenerated, gitignored
```
