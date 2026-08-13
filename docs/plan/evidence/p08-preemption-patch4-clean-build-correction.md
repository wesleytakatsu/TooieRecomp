---
title: "P08 — PATCH 4 clean-build / tracked-state correction"
description: "Track empty RecompiledPatches scaffolds so a clean clone can build TooieRecompiled. Independent review verdict C is preserved; this is the implementer correction."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, patch4, reproducibility, clean-build]
---

# P08 — PATCH 4 CLEAN-BUILD CORRECTION

```text
MODE:                 IMPLEMENTATION / REPRODUCIBILITY
MODEL:                Grok 4.6 HIGH
TOOIE_BASE_HEAD:      c6c3ebc9e083b91c33dde57dbd58d2765677cac7
ROOT_BLOCKER:         ignored untracked RecompiledPatches scaffolds
SCAFFOLD_POLICY:      TRACK_STABLE_EMPTY
PATCH4_ACCEPTED:      PENDING INDEPENDENT REVIEW
```

This correction does **not** change Hybrid F' semantics, N64ModernRuntime
source, N64Recomp source, generator safepoint policy, or C2AB8 code. It does
not diagnose `func_800BCF28`. Independent review verdict **C** is preserved;
this file is the implementer fix for the distribution blocker only.

## 0. Canonical state

```text
TOOIE_HEAD:     c6c3ebc9e083b91c33dde57dbd58d2765677cac7
BRANCH:         main == origin/main
WORKTREE:       clean before this correction
```

## 1. Local scaffold inspection (protected tree)

The files already existed as ignored local P05 scaffolds. Full contents:

`RecompiledPatches/patches.c` (94 bytes):

```c
/* No native patches for Tooie yet (P05 scaffold). */
void tooie_patches_placeholder(void) {}
```

`RecompiledPatches/patches_bin.c` (81 bytes):

```c
const char bk_patches_bin[] = { 0 };
const unsigned int bk_patches_bin_size = 0;
```

`RecompiledPatches/recomp_overlays.inl` (163 bytes):

```c
// Empty patch overlays
static SectionTableEntry patch_section_table[] = {};
static int patch_overlay_sections_by_index[] = {};
static int patch_num_sections = 0;
```

```text
LOCAL_PATCHES_C_SHA256:
1756195e2047e5460d1845ec4ecac0523ec3a9af3d8ff4880cde7c82866687c7
LOCAL_PATCHES_BIN_C_SHA256:
9d4d5dba4bd0d0a4f86a795f1670b70e05c6f57f161de4ae8b2e764f899d794c
LOCAL_RECOMP_OVERLAYS_SHA256:
97439c004bfa8cd0bd1d886640d1e3d3fea36678c2f0906f64b348a169cc9085

PATCHES_C_GAME_DATA:        NONE
PATCHES_BIN_GAME_DATA:      NONE
RECOMP_OVERLAYS_GAME_DATA:  NONE
```

No ROM-derived bytes, no native patches, no addresses/payloads. Legacy
`bk_patches_bin` name is retained because `librecomp` / BK-style APIs already
expect it. Historical classification in
[[p08-active-banjo-reuse-audit]] is confirmed.

`register_tooie_patches()` remains empty. No C2AB8 patch. No overlay payload.

## 2. Why they are safe to track

These are constant source/build scaffolding, not generator output. CMake
already requires `patches.c` and `patches_bin.c` for `PatchesLib`. Tracking
the stable empty files is simpler and safer than inventing a bootstrap
generator for constant zeros.

## 3. `recomp_overlays.inl` consumer

```text
RECOMP_OVERLAYS_CONSUMER:
  BK contract / later integration:
    BanjoRecomp/src/main/register_patches.cpp includes
    ../../RecompiledPatches/recomp_overlays.inl and passes
    section_table to recomp::overlays::register_patches().
  Current Tooie compile:
    src/main/register_patches.cpp does NOT include it.
    register_tooie_patches() is intentionally empty.
    CMake compiles only patches.c + patches_bin.c.
```

Empty content is valid for current Tooie: no patch ELF, no overlay sections,
no `register_patches()` call. The include is **not** obsolete — it is the
BK-style patch contract the independent review listed as later integration.
The empty scaffold is tracked; the consumer is not removed.

Game overlays continue to come from `RecompiledFuncs/recomp_overlays.inl`
via `register_overlays.cpp`. That file remains generator output.

## 4. Gitignore policy

```gitignore
RecompiledFuncs/
RecompiledPatches/*
!RecompiledPatches/patches.c
!RecompiledPatches/patches_bin.c
!RecompiledPatches/recomp_overlays.inl
```

`RecompiledFuncs/` stays ignored. Other future `RecompiledPatches/` outputs
stay ignored unless explicitly approved.

```text
TRACKED_RECOMPILED_PATCHES:
  RecompiledPatches/patches.c
  RecompiledPatches/patches_bin.c
  RecompiledPatches/recomp_overlays.inl
OTHER_RECOMPILED_PATCHES_STILL_IGNORED: YES
RECOMPILEDFUNCS_STILL_IGNORED: YES
```

## 5. CMake contract

`PatchesLib` sources are unchanged. A tiny configure check now fails if any
of the three tracked scaffolds is missing, instead of synthesizing them.

A second tiny check requires Clang. First clean configure used GCC
(`/usr/bin/cc`). GCC `RECOMP_FUNC` is strong (`noipa`), so link failed with
multiple definitions of `boot_hw_stubs` overrides (`ovl_load`, `wait_one_frame`,
…). Protected Tooie objects are Ubuntu Clang 18.1.3, where `RECOMP_FUNC` is
weak. Classification: environment/compiler contract (E), not another hidden
scaffold. Not a missing file. Documented and gated so a clean GCC configure
fails immediately instead of after a full compile.

```text
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

## 6. Clean checkout procedure (no hidden copy)

Disposable tree: `/tmp/p08-p4-clean-build`

1. `git clone` Tooie @ `c6c3ebc` (committed HEAD). Confirmed absent:
   `RecompiledPatches/`, `RecompiledFuncs/`, `build/`, `build-recomp/`.
2. Apply only the candidate tracked patch
   (`.gitignore`, `CMakeLists.txt`, three scaffolds).
3. Reconstruct sibling `BanjoRecomp/lib/N64ModernRuntime` @
   `ca568b6ad79b9029d14077f0c3ffa757727c5559` from committed objects.
4. Reconstruct `N64Recomp` @ `2b6f05688de2abc7d86da5b4a89b84c2c6acbabe`.
5. Symlink clean third-party trees (`rt64`, `RecompFrontend`, `SlotMap`)
   and the decomp ELF/ROM sibling. **Did not** copy protected
   `RecompiledPatches`, `RecompiledFuncs`, `build`, or `build-recomp`.
6. `./tools/apply_runtime_patches.sh` twice.

```text
COPIED_RECOMPILED_PATCHES_FROM_PROTECTED_TREE: NO
COPIED_RECOMPILED_FUNCS: NO
COPIED_BUILD_OUTPUT: NO
USED_ONLY_TRACKED_SCAFFOLDS: YES
```

`git ls-files RecompiledPatches` on the disposable checkout:

```text
RecompiledPatches/patches.c
RecompiledPatches/patches_bin.c
RecompiledPatches/recomp_overlays.inl
```

`git check-ignore -v RecompiledFuncs/funcs_327.c` still matches
`.gitignore:2:RecompiledFuncs/`. The three scaffolds are not ignored.

## 7. Dependency reconstruction

```text
N64ModernRuntime: ca568b6ad79b9029d14077f0c3ffa757727c5559
N64Recomp:        2b6f05688de2abc7d86da5b4a89b84c2c6acbabe

scheduler:  1b3d3d4666e02f748c44d19d867d02669b83753d85fe7eb62d746a5a8cf38309
preemption: 8959055e6c51767fcd7c58b403a2c6ce8bc7aa0ab6c23ffab9dc181905eacd6b
N64Recomp:  2045cd90cfae70c881a9504146bac5174ca7a53ba1f5e6200a6a3e4adb5a044c

PATCH_HASHES_VALID:     YES
PATCH_SCRIPT_IDEMPOTENT: YES
  apply 1: applied
  apply 2: already applied / success
```

Protected dirty N64ModernRuntime / N64Recomp worktrees were not reset and
were not used as the proof binaries.

## 8. Reconstructed N64Recomp + lower-level tests

```text
CLEAN_N64RECOMP_BUILD: PASS
  /tmp/p08-p4-clean-build/BanjoRecomp/lib/N64ModernRuntime/N64Recomp/build/N64Recomp
  sha256 c2b837e2392980f501f6230868ef11d1952da2553755c80a62eaf74e717354b5
CLEAN_T11:             PASS (pass=61 fail=0)
CLEAN_PATCH35_HARNESS: PASS (pass=40 fail=0 skipped=0)
```

T15 was not rerun. Previous independent review remains the performance
evidence (~0.919 ns absolute fast-path). The correction does not touch
benchmark source.

## 9. Clean Tooie regen

Initially no `RecompiledFuncs/` in the disposable checkout.
`./tools/run_recomp.sh` twice, using the reconstructed N64Recomp.

```text
CLEAN_TOOIE_REGEN:       PASS
CLEAN_REGEN_FILE_COUNT:  420
CLEAN_SAFEPOINT_COUNT:   4298
REGEN_DETERMINISTIC:     YES
CLEAN_REGEN_DIGEST:
  1f184e3f866dfb9cf2d12a3ac3aa3e011de18f455e64b5f09a2e084ba5bf7663
  (sorted per-file sha256sum | sha256sum)
MATCHES_PATCH4_REGEN_DIGEST: YES (content)
```

`diff -rq` against the protected PATCH 4 `RecompiledFuncs/` is empty: the
regenerated tree is byte-identical. The previously published independent
digest `a54bcbbfa7a5e997084f9b511087169b95734ff68574a6f7cc91d25bd1ae3236`
used an unspecified aggregation; several common concatenations were tried
and did not reproduce that hex. Content identity is the discriminator.

## 10. Strict clean build

```text
CLEAN_TOOIE_CONFIGURE: PASS (Clang 18.1.3, Release, Ninja)
CLEAN_TOOIE_BUILD:     PASS
PATCH4_DISTRIBUTABLE_FROM_TRACKED_STATE: YES
CLEAN_BINARY_SHA256:
  8372c1f11dbdf08eb62938eb229e007d87d22cea29dc2ff5f02b4a1fa9934ea5
size: 28333640
```

`PatchesLib` compiled
`/tmp/p08-p4-clean-build/TooieRecomp/RecompiledPatches/patches.c` and
`patches_bin.c`. `nm` shows generated `wait_one_frame` as `W` (weak),
matching the protected Clang archive.

## 11. Same clean-built binary ON / OFF

Canonical env. Preemption unset for ON. Same executable, no rebuild.

```text
CLEAN_BINARY_SAME_ON_OFF: YES
```

ON (`ULTRAMODERN_GUEST_PREEMPTION` absent):

```text
[tooie] guest preemption: ON (ULTRAMODERN_GUEST_PREEMPTION=<unset>)
TOOIE_AUTO_START: starting game
exit 139 SIGSEGV ~65s
ovl_load events: 1283
audio: rsp_aud advanced through 1541; queue_samp through 2325392
ON_CROSSES_C2AB8: YES
ON_WORLD_INIT:    YES
ON_DOWNSTREAM:    func_800BCF28
```

Core of pid 2644623:

```text
#0  func_800BCF28 + 225
#1  bakey_heldCount
#2  bainput_func_80097B4C
#5  func_800A4B08
#7  basetup_entrypoint_2
#10 gsworldDll_entrypoint_2
#14 func_800A76F4
```

Matches the accepted local PATCH 4 ON path. Downstream cause not diagnosed.

OFF (`ULTRAMODERN_GUEST_PREEMPTION=0` only):

```text
[tooie] guest preemption: OFF (ULTRAMODERN_GUEST_PREEMPTION=0)
killed after 80s (no crash)
stable at last_ovl=635:2:
  d/r=178774/178774
  rsp_aud=1523
  queue_calls=1522
  queue_samp=2304416
16 diagnostic ticks (2 s interval) identical ≈ 32 s
OFF_REPRODUCES_OLD_FREEZE: YES
CAUSAL_CLASS_PRESERVED: A
```

## 12. What did not change

```text
HYBRID_F_SEMANTICS_CHANGED: NO
RUNTIME_PATCH_CHANGED:      NO
N64RECOMP_PATCH_CHANGED:    NO
GENERATOR_POLICY_CHANGED:   NO
MAIN_ACTIVATION_CHANGED:    NO
RECOMPILEDFUNCS_TRACKED:    NO
TABLE2_WORD0_ZERO:          INFERENCE
DOWNSTREAM_DIAGNOSIS_PERFORMED: NO
```

T15 C-mode assertion residual is still nonblocking and was not modified.

## 12b. Vault / git

```text
MARKDOWN_VAULT_AUDIT: PASS
  repository-local audit of TooieRecomp/docs:
  filesScanned=97 errors=0 warnings=49
  warnings are pre-existing (missing frontmatter, multiple H1,
  trailing spaces, two large historical notes).
  no new errors in this correction.
git diff --check: PASS
```

## 13. Gate

```text
CLEAN_TOOIE_REGEN: PASS
CLEAN_TOOIE_BUILD: PASS
PATCH4_DISTRIBUTABLE_FROM_TRACKED_STATE: YES
4298 safepoints reproduced
same-binary ON/OFF causal behavior preserved
PATCH4_DELIVERY_CORRECTED_BY_IMPLEMENTER: YES
PATCH4_ACCEPTED: PENDING INDEPENDENT REVIEW
```

## 14. Next

Independent review of this clean-build correction. Do **not** investigate
`func_800BCF28` until that review closes.

## 15. Independent review pointer

Reviewed in [[p08-preemption-patch4-clean-build-independent-review]]:
verdict B, tracked-state blocker **RESOLVED**, PATCH 4 accepted. The implementer
record above is preserved as written.
