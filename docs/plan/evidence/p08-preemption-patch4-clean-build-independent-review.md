---
title: "P08 — PATCH 4 clean-build correction independent review"
description: "Independent reconstruction approves the tracked RecompiledPatches correction with nonblocking compiler-gate and digest-method notes; historical verdict C remains preserved."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, patch4, reproducibility, clean-build, independent-review]
---

# P08 — PATCH 4 CLEAN-BUILD CORRECTION INDEPENDENT REVIEW

```text
BASE_HEAD:       f81d55289037404fa8f76b860a5c4182108775ad
MODEL:           GPT-5.6 SOL HIGH
MODE:            READ / RECONSTRUCT / TEST / DOCS ONLY
VERDICT:         B — correction approved with nonblocking issues
SOURCE_CHANGED:  NO
```

This review is limited to the delivery correction in
`c6c3ebc9e083b91c33dde57dbd58d2765677cac7..f81d55289037404fa8f76b860a5c4182108775ad`.
It does not reopen Hybrid F', modify either dependency, change generator
policy, hand-edit `RecompiledFuncs`, or investigate the cause of the
downstream `func_800BCF28` crash.

The historical verdict C in
[[p08-preemption-patch4-independent-review]] remains preserved. Its behavior
finding remains CLASS A. This review closes only its tracked-state delivery
finding: a clean checkout now contains the stable patch scaffolds, regenerates
the expected output, and builds and runs with the documented Clang toolchain.

## 1. Canonical state and correction scope

The protected checkout began clean on `main == origin/main`:

```text
HEAD:       f81d55289037404fa8f76b860a5c4182108775ad
WORKTREE:   clean
ORIGIN_MAIN:f81d55289037404fa8f76b860a5c4182108775ad
```

The correction range changes only:

```text
.gitignore
CMakeLists.txt
RecompiledPatches/patches.c
RecompiledPatches/patches_bin.c
RecompiledPatches/recomp_overlays.inl
documentation
```

It does not change `src/main/main.cpp`, runtime or N64Recomp patch artifacts,
generator policy, `RecompiledFuncs`, or game logic.

```text
CORRECTION_SCOPE_NARROW: YES
UNEXPECTED_SEMANTIC_CHANGE: NONE
HYBRID_F_CHANGED:  NO
ACTIVATION_CHANGED:NO
GENERATOR_CHANGED: NO
```

## 2. Scaffold contents and safety

The three tracked files contain only the expected stable empty scaffolds:

```c
/* No native patches for Tooie yet (P05 scaffold). */
void tooie_patches_placeholder(void) {}
```

```c
const char bk_patches_bin[] = { 0 };
const unsigned int bk_patches_bin_size = 0;
```

```c
// Empty patch overlays
static SectionTableEntry patch_section_table[] = {};
static int patch_overlay_sections_by_index[] = {};
static int patch_num_sections = 0;
```

There are no ROM bytes, addresses, relocations, native patch bodies, or game
payload. The placeholder function is not called; the binary advertises size
zero; the patch tables are empty.

```text
ROM_DERIVED_DATA_PRESENT:        NO
NATIVE_PATCH_BEHAVIOR_PRESENT:   NO
GAME_SPECIFIC_PAYLOAD_PRESENT:   NO
SAFE_AS_TRACKED_SOURCE_SCAFFOLD: YES
```

## 3. Gitignore and consumers

`git ls-files RecompiledPatches` returns exactly:

```text
RecompiledPatches/patches.c
RecompiledPatches/patches_bin.c
RecompiledPatches/recomp_overlays.inl
```

`git check-ignore -v --no-index` resolves the three exception rules for those
paths, the default `RecompiledPatches/*` rule for arbitrary future output, and
`RecompiledFuncs/` for generated recompilation C.

```text
TRACKED_SCAFFOLD_SET_EXACT:                  YES
FUTURE_GENERATED_PATCH_OUTPUT_STILL_IGNORED: YES
RECOMPILEDFUNCS_STILL_IGNORED:               YES
```

`patches.c` and `patches_bin.c` are current `PatchesLib` source inputs, and
`PatchesLib` is linked into `TooieRecompiled`. Their symbols are present in
`libPatchesLib.a`; because no current Tooie path references them, normal static
archive extraction omits them from the final executable. They are nevertheless
required current CMake inputs.

Current `register_tooie_patches()` is intentionally empty and does not include
`RecompiledPatches/recomp_overlays.inl`. BanjoRecomp later-integration code does
include its generated counterpart and passes patch data/tables to
`recomp::overlays::register_patches()`. The Tooie empty file is therefore a
harmless forward scaffold. The correction's existence guard makes it a
configure-time tracked-state invariant, but there is no current compile or
runtime consumer.

```text
PATCHES_C_REQUIRED_NOW:                 YES
PATCHES_BIN_REQUIRED_NOW:               YES
RECOMP_OVERLAYS_REQUIRED_NOW:           NO (except the new presence guard)
RECOMP_OVERLAYS_VALID_STABLE_SCAFFOLD: YES
```

## 4. Compiler contract

The active `N64Recomp/include/recomp.h` defines:

```text
Clang: extern inline __attribute__((weak,noinline))
GCC:   __attribute__((noipa, optimize("rounding-math")))
```

Independent object inspection used real regenerated Tooie sources:

```text
Clang archive:
  0000000000000770 W ovl_load
  0000000000000930 W wait_one_frame

Same generated sources compiled with GCC 13.3.0:
  00000000000007f0 T ovl_load
  0000000000000930 T wait_one_frame
```

The actual `boot_hw_stubs.cpp.o` also defines `ovl_load` strongly. A relocatable
link of that object with the GCC-generated `funcs_155.c.o` fails with a
multiple definition of `ovl_load`. This independently proves that the weak
generated symbol is required for current native-stub interposition.

```text
GCC_RECOMP_FUNC_STRONG:                 PROVEN
CLANG_RECOMP_FUNC_WEAK:                 PROVEN
BOOT_HW_STUB_INTERPOSITION_REQUIRES_WEAK:YES
CLANG_REQUIRED_FOR_CURRENT_TOOIE_BUILD: YES
```

The new GCC rejection is intentional and accurately fails before the known
link error:

```text
GCC_CONFIGURE: exit 1
TooieRecompiled requires Clang ... GCC RECOMP_FUNC is not weak ...
```

The gate checks only `CMAKE_C_COMPILER_ID`. A configure with `C=clang` and
`CXX=g++` passes (`Clang 18.1.3` / `GNU 13.3.0`). CMake does not intrinsically
pair the compilers. For the demonstrated defect, this is sufficient because
the generated `RecompiledFuncs/*.c` symbols are controlled by the C compiler.
It does not, however, enforce the broader documented `clang` + `clang++`
toolchain contract. `MATCHES "Clang"` also accepts AppleClang; the weak macro
path is compatible, but the full Linux project was not validated with
AppleClang.

```text
COMPILER_GATE_CORRECT: CONDITIONAL
GCC_REJECTION_INTENTIONAL: YES
GCC_UNSUPPORTED_CONTRACT_VALID: YES
MIXED_COMPILER_REPRODUCIBILITY_BLOCKER: NO
```

Canonical build documentation already gives the required command in
`docs/plans/BT/05-native-runtime-and-boot.md`, including both `clang` and
`clang++`; CMake also prints the same correction on rejection.

```text
CLANG_REQUIREMENT_DOCUMENTED: YES
```

## 5. Independent disposable reconstruction

Disposable root:

```text
/tmp/p08-p4-independent-e3FmS3
```

The Tooie checkout was created from Git at exact HEAD. Before reconstruction:

```text
RecompiledFuncs/: absent
build/:           absent
build-recomp/:    absent
RecompiledPatches/: present with exactly the three tracked files
```

No file was copied from the protected Tooie `RecompiledPatches`,
`RecompiledFuncs`, `build`, or `build-recomp` trees. External clean tracked
third-party submodules and the documented user-provided decompressed ROM/ELF
were linked into the disposable dependency layout.

```text
CLEAN_CLONE_HAS_TRACKED_SCAFFOLDS: YES
COPIED_HIDDEN_FILES:               NO
```

Exact dependency bases:

```text
N64ModernRuntime: ca568b6ad79b9029d14077f0c3ffa757727c5559
N64Recomp:        2b6f05688de2abc7d86da5b4a89b84c2c6acbabe
```

Independently calculated artifact hashes:

```text
scheduler:  1b3d3d4666e02f748c44d19d867d02669b83753d85fe7eb62d746a5a8cf38309
preemption: 8959055e6c51767fcd7c58b403a2c6ce8bc7aa0ab6c23ffab9dc181905eacd6b
N64Recomp:  2045cd90cfae70c881a9504146bac5174ca7a53ba1f5e6200a6a3e4adb5a044c
```

First application reported all three applied. The second returned success and
reported all three already applied.

```text
PATCH_HASHES_VALID:      YES
PATCH_SCRIPT_IDEMPOTENT: YES
```

## 6. Tool build and lower-level tests

The reconstructed patched N64Recomp configured and built with Clang 18.1.3.

```text
CLEAN_N64RECOMP_BUILD: PASS
CLEAN_T11:             PASS (pass=61 fail=0)
CLEAN_PATCH35:         PASS (pass=40 fail=0 skipped=0)
```

## 7. Clean regeneration

Two executions of `./tools/run_recomp.sh` each produced:

```text
GENERATED_FILES: 420
SAFEPOINT_COUNT: 4298
```

The two per-file SHA-256 manifests were identical, and every generated file
was byte-identical to the protected PATCH 4 output.

For this review, the exact aggregate method was:

1. recursively list generated files in lexicographic relative-path order;
2. write GNU-style lines `<file-sha256><two spaces>RecompiledFuncs/<path>\n`;
3. SHA-256 the resulting 420-line manifest.

That digest is:

```text
0d171e3f04fcbc1c429b226332406f55f1e54d8371f8c94e85cc5e216ede775e
```

The implementer evidence reports `1f184e3f...` as "sorted per-file
sha256sum | sha256sum" but does not provide an exact command. Common
path-sorted, hash-sorted, relative-path, directory-prefixed, and hashes-only
aggregations do not reproduce it. This is a documentation precision issue,
not an output mismatch: independent clean output and protected output have
zero byte-different files.

```text
CLEAN_REGEN:                         PASS
SAFEPOINT_COUNT:                     4298
REGEN_DETERMINISTIC:                 YES
MATCHES_IMPLEMENTER_GENERATED_OUTPUT:YES (byte identity)
IMPLEMENTER_AGGREGATE_DIGEST:        CONDITIONAL / method underspecified
```

## 8. Clean Clang build and distributability

Explicit configure:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build --target TooieRecompiled -j4
```

Result:

```text
CLEAN_CLANG_CONFIGURE: PASS (Clang 18.1.3 C and C++)
CLEAN_CLANG_BUILD:     PASS
CLEAN_BINARY_SHA256:   565269c0e4b9d4fc6e22ab90df7b8841aa880451598a43e122e1a90e2333b094
CLEAN_BINARY_SIZE:     28333640
```

The executable is obtainable from tracked Tooie HEAD, the documented exact
dependency bases and patch artifacts, documented user-provided game inputs,
and the supported Clang toolchain, without hidden Tooie source files.

```text
PATCH4_DISTRIBUTABLE_FROM_TRACKED_STATE: YES
```

## 9. Same clean-built binary ON/OFF

Both controls used the independent clean executable above. The desktop
environment was made explicit with `DISPLAY=:0` and
`XDG_RUNTIME_DIR=/run/user/1000`; the canonical game variables were
`TOOIE_AUTO_START=1`, `TOOIE_DIAG=1`,
`TOOIE_DISABLE_LAUNCHER_MUSIC=1`, and the Radeon Vulkan ICD.

ON left `ULTRAMODERN_GUEST_PREEMPTION` absent:

```text
[tooie] guest preemption: ON (ULTRAMODERN_GUEST_PREEMPTION=<unset>)
[tooie] TOOIE_AUTO_START: starting game
ovl_load events: 1283
audio advanced through rsp_aud=1515 / queue_samp=2284912
signal: SIGSEGV after ~56 s
frame #0: func_800BCF28
world-init evidence: gsworldDll_entrypoint_2 on the minimal crash-site stack
```

Only the crash site and world-init marker were collected. The crash cause was
not investigated.

OFF added only `ULTRAMODERN_GUEST_PREEMPTION=0` to the same executable:

```text
[tooie] guest preemption: OFF (ULTRAMODERN_GUEST_PREEMPTION=0)
last_ovl=635:2
d/r=179280/179280
rsp_aud=1530
queue_calls=1529
queue_samp=2304048
13 trailing identical 2-second ticks (26 s stable)
process alive until the 80-second test timeout
```

The executable hash was unchanged before and after OFF.

```text
CLEAN_BINARY_SAME_ON_OFF: YES
ON_C2AB8_CROSSED:         YES
ON_WORLD_INIT:            YES
OFF_OLD_FREEZE:           YES
CAUSAL_CLASS:             A
```

## 10. Verdict and residuals

```text
TRACKED_RECOMPILEDPATCHES_BLOCKER: RESOLVED
BLOCKING_ISSUES:                    NONE
VERDICT:                            B
PATCH4_ACCEPTED:                    YES
PERMANENT_PREEMPTION_DELIVERED:     YES
C2AB8_BLOCKER_STATUS:               RESOLVED
CURRENT_PRODUCT_DIVERGENCE:         func_800BCF28
```

Nonblocking issues introduced or confirmed by this review:

1. `COMPILER_GATE_CORRECT: CONDITIONAL`: it proves/enforces the relevant C
   compiler property but does not enforce the documented `clang++` pairing;
   AppleClang is syntactically accepted without full-project validation.
2. The implementer regeneration digest methodology is underspecified; exact
   byte identity independently proves the generated output match.
3. `RecompiledPatches/recomp_overlays.inl` is unused by current Tooie code;
   keeping the empty forward scaffold is harmless baggage, not a blocker.

Preserved residuals, not reopened:

- T15 C assertion is weaker than the prose claim;
- dynamic safepoint density is unprofiled;
- `table[2].word0 == 0` remains INFERENCE;
- generic JR residual;
- Timer/PI approximation;
- startup-only flag setter;
- RCP-X2/MTC0 helper/analog-level evidence.

## 11. Documentation checks

`git diff --check` passes, all changed paths are under `docs/`, and the new
review note's local links resolve. The available `markdown_vault_audit` MCP is
configured for a different 14-file vault: auditing `.` reported that unrelated
vault's pre-existing issues, while the absolute TooieRecomp docs path was
denied as outside its configured root. Therefore no repository-local MCP audit
result can honestly be reported as PASS.

```text
MARKDOWN_VAULT_AUDIT: FAIL — connector vault-root mismatch
REPOSITORY_DOC_CHECKS: PASS
git diff --check:       PASS
```

## Handoff

```text
=== P08 PATCH 4 CLEAN-BUILD INDEPENDENT REVIEW ===

BASE_HEAD:
f81d55289037404fa8f76b860a5c4182108775ad

MODEL:
GPT-5.6 SOL HIGH

VERDICT:
B

CORRECTION_SCOPE_NARROW:
YES

TRACKED_SCAFFOLD_SET_EXACT:
YES

ROM_DERIVED_DATA_PRESENT:
NO

NATIVE_PATCH_BEHAVIOR_PRESENT:
NO

SAFE_AS_TRACKED_SOURCE_SCAFFOLD:
YES

RECOMPILEDFUNCS_STILL_IGNORED:
YES

FUTURE_GENERATED_PATCH_OUTPUT_STILL_IGNORED:
YES

PATCHES_C_REQUIRED_NOW:
YES

PATCHES_BIN_REQUIRED_NOW:
YES

RECOMP_OVERLAYS_REQUIRED_NOW:
NO (no current consumer; configure presence guard only)

GCC_RECOMP_FUNC_STRONG:
PROVEN

CLANG_RECOMP_FUNC_WEAK:
PROVEN

BOOT_HW_STUB_INTERPOSITION_REQUIRES_WEAK:
YES

CLANG_REQUIRED_FOR_CURRENT_TOOIE_BUILD:
YES

COMPILER_GATE_CORRECT:
CONDITIONAL

CLANG_REQUIREMENT_DOCUMENTED:
YES

CLEAN_CLONE_HAS_TRACKED_SCAFFOLDS:
YES

COPIED_HIDDEN_FILES:
NO

PATCH_HASHES_VALID:
YES

PATCH_SCRIPT_IDEMPOTENT:
YES

CLEAN_N64RECOMP_BUILD:
PASS

CLEAN_T11:
PASS (61/0)

CLEAN_PATCH35:
PASS (40/0/0)

CLEAN_REGEN:
PASS

SAFEPOINT_COUNT:
4298

REGEN_DETERMINISTIC:
YES

MATCHES_IMPLEMENTER_GENERATED_OUTPUT:
YES (byte identity; implementer aggregate digest method underspecified)

CLEAN_CLANG_CONFIGURE:
PASS

CLEAN_CLANG_BUILD:
PASS

GCC_REJECTION_INTENTIONAL:
YES

GCC_UNSUPPORTED_CONTRACT_VALID:
YES

PATCH4_DISTRIBUTABLE_FROM_TRACKED_STATE:
YES

CLEAN_BINARY_SAME_ON_OFF:
YES

ON_C2AB8_CROSSED:
YES

ON_WORLD_INIT:
YES

OFF_OLD_FREEZE:
YES

CAUSAL_CLASS:
A

HYBRID_F_CHANGED:
NO

ACTIVATION_CHANGED:
NO

GENERATOR_CHANGED:
NO

TRACKED_RECOMPILEDPATCHES_BLOCKER:
RESOLVED

BLOCKING_ISSUES:
NONE

NONBLOCKING_ISSUES:
compiler gate checks only C ID; AppleClang unvalidated; implementer aggregate
digest command underspecified; unused recomp_overlays.inl is harmless forward
scaffold

PATCH4_ACCEPTED:
YES

PERMANENT_PREEMPTION_DELIVERED:
YES

C2AB8_BLOCKER_STATUS:
RESOLVED

CURRENT_PRODUCT_DIVERGENCE:
func_800BCF28

SOURCE_CODE_CHANGED:
NO

EVIDENCE_CREATED:
docs/plan/evidence/p08-preemption-patch4-clean-build-independent-review.md

SESSION_HANDOFF_UPDATED:
YES

GLOBAL_HANDOFF_UPDATED:
YES

PHASE_P08_UPDATED:
YES

SPECS_REVIEWED:
recomp-readiness, rsp-and-audio, build, matching, overlays

SPECS_CHANGED:
recomp-readiness, rsp-and-audio (acceptance state only)

MARKDOWN_VAULT_AUDIT:
FAIL — connector is rooted at an unrelated vault and denied the repository path;
repository-local link/whitespace checks and git diff --check PASS

DOC_COMMIT:
docs(p08): review patch4 clean-build correction

PUSHED:
YES (this documentation commit; see final publication handoff)

NEW_HEAD:
documentation commit containing this note; exact hash in final publication handoff

WORKTREE:
expected clean after the documentation commit; verify in final publication handoff

NEXT_SINGLE_ACTION:
investigate func_800BCF28 as the new first product divergence

=== END HANDOFF ===
```
