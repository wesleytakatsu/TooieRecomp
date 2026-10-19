---
title: "P08/P09 — Evidence checkpoint: pre-commit review, RT64 clean-pin patch replay and getter gate fix"
description: "Pre-commit review provenance for 196-file manifest. P1 RT64 packaging replay resolved with 3 clean-pin patches and frontend registration in tools/apply_runtime_patches.sh; P2 getter diagnostic gate resolved and verified; automated tests and builds pass; user A6A2 visual confirmation preserved."
status: evidence
phase: P08
updated: 2026-10-19
tags: [tooie-recomp, p08, p09, checkpoint, precommit, review, rt64, patches, replay, getter-gate, clean-pin]
---

# P08/P09 — Evidence Checkpoint: Pre-Commit Review, RT64 Clean-Pin Patch Replay and Getter Gate Fix

## 1. Context and Review Scope

This checkpoint documents pre-commit review provenance, automated test gates, and the resolution of findings P1 and P2 identified during pre-commit audit of the 196-file working tree manifest before final staging, commit, and publication by Root.

> [!IMPORTANT]
> **Pre-Commit Review Provenance**: This document records pre-commit validation and patch replay provenance. Root owns Git staging, commit, and push. The Git commit itself identifies the final snapshot; Git log and remote verification post-commit establish publication truth. No commit SHA or push completion is invented here.

Both code writers (Grok 4.7 Medium sessions) completed execution with `end_turn`. Working tree state prior to this documentation update:
- TooieRecomp HEAD: `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf`
- RT64 pinned base: `6f1c2d99a4ea571c139f449c326fd176ba8f3496`
- Plume submodule: `d890ac899e505fb30040e037a4037cdeca68f033` (unmodified)
- BanjoRecomp HEAD: `ec859632cfa584e2272d32e46051543429a2baf4`
- N64ModernRuntime HEAD: `ca568b6ad79b9029d14077f0c3ffa757727c5559`
- N64Recomp HEAD: `2b6f05688de2abc7d86da5b4a89b84c2c6acbabe`
- RecompFrontend pinned base: `d0d90ba49f46f4896aaeda362056c21b1e342561`

---

## 2. P1 Resolution: RT64 Clean-Pin Patch Replay & Applicator Chain

### 2.1 Problem Statement
The initial read-only review identified that the three older RT64 patch files in the repository (`rt64-present-rate-decimator.patch`, `rt64-present-success-counter.patch`, and `p09-exact-use-pathology-correlation.patch`) had been authored against the local dirty RT64 worktree. When tested against a clean clone of pinned RT64 commit `6f1c2d99a4ea571c139f449c326fd176ba8f3496`, all three patches failed `git apply --check` with exit code 1. Furthermore, the production CURRENT `tfPos` and post-snap fog behavior in `RSPProcessCS.hlsl` existed only in the dirty worktree and was absent from the clean pin apply chain.

### 2.2 Production Clean-Pin Patch Chain
To ensure clean-clone reproducibility without exporting forbidden experimental diagnostics, three non-overlapping versioned patch files were constructed against clean RT64 pin `6f1c2d99a4ea571c139f449c326fd176ba8f3496`:

1. **`patches/rt64/rt64-rsp-endpoint-current.patch`**:
   - Size: 16,629 bytes | SHA256: `49925480c2d1736c790ddf96152844951f6fe27a150ab1cc110347d557c0d813`
   - Role: Production CURRENT `tfPos` snap and CPU data path. Snaps `tfPos` to endpoint CURRENT (`endpointViewProj` slot t22, `endpointWorld` slot t23) when enabled via `tooie_rsp_endpoint_current` (default ON in TooieRecomp). Fog calculation uses `tfPos.z/w` after the snap. Extends Process push constant size to `sizeof(uint32_t) * 8`.
   - Covered files: `src/hle/rt64_workload.cpp`, `src/hle/rt64_workload.h`, `src/render/rt64_descriptor_sets.h`, `src/render/rt64_projection_processor.cpp`, `src/render/rt64_rsp_processor.cpp`, `src/render/rt64_rsp_processor.h`, `src/render/rt64_shader_library.cpp`, `src/render/rt64_transform_processor.cpp`, `src/shaders/RSPProcessCS.hlsl`.

2. **`patches/rt64/rt64-present-fps-decimator-and-counter.patch`**:
   - Size: 5,415 bytes | SHA256: `6d746f27ed1bd3132dcea8464adeb25dcfac4ad38502bf92f402ea6e3a19836c`
   - Role: Cumulative presentation patch. Installs `rt64_present_decimator.h` (1,298 bytes) and `rt64_present_stats.h` (353 bytes), wraps `viOriginalRate` writes in `rt64_workload_queue.cpp` under `configurationMutex`, integrates cadence decimation into `rt64_present_queue.cpp`, and increments `RT64::presentSuccessCount` upon successful swapchain present.
   - Covered files: `src/hle/rt64_present_decimator.h`, `src/hle/rt64_present_queue.cpp`, `src/hle/rt64_present_stats.h`, `src/hle/rt64_workload_queue.cpp`.

3. **`patches/rt64/rt64-p09-exact-use-correlation-header.patch`**:
   - Size: 43,083 bytes | SHA256: `0d4c250a5c4950dcdb1d0835ea7347014c90893e93332caa5576c67e03459561`
   - Role: Installs header `src/render/rt64_p09_exact_use_correlation.h` (41,930 bytes, SHA256 `793dc75e...`), enabling `tests/p09_exact_use_correlation_test.cpp` to compile against a patched clean pin.
   - Covered file: `src/render/rt64_p09_exact_use_correlation.h`.

4. **`patches/frontend/recompfrontend-overlay-update-callback.patch`**:
   - Size: 2,732 bytes | SHA256: `37266d0bd13d42c0b8f174c0118b6a9b2490784c5880f05e6ee37ab811cd48a7`
   - Pin: `d0d90ba49f46f4896aaeda362056c21b1e342561`
   - Role: Registers `recompui::set_overlay_update_callback` hook in RecompFrontend. Byte-matched to live frontend files.

### 2.3 Applicator Registration
`tools/apply_runtime_patches.sh` was updated to register the 3 RT64 clean-pin patches and the RecompFrontend patch using the existing idempotent `apply_one_patch` helper:
- Diff stat: `tools/apply_runtime_patches.sh` | 49 insertions (+49)
- Script SHA256: `87a6455f8297749a3b48c86521fd225dc8cfa6966553b9ce4cb499d9377998a1`
- Validation: `bash -n tools/apply_runtime_patches.sh` exit 0; `git diff --check` exit 0.
- Safe execution boundary: The applicator script was NOT executed against live checkouts because the live checkouts contain protected uncommitted diagnostic supersets.

### 2.4 Scratch Replay Validation
Replay verification was conducted in owned isolated scratch checkouts:
- **`rt64-scratch-apply`** (clean clone of `6f1c2d99` + 3 patches applied):
  - `git apply --reverse --check` exit 0 for all 3 patches.
  - Subsequent forward `git apply --check` exit 1 (expected: already applied).
  - 14 covered paths compared against scratch edited tree: all 14 match.
  - 6 paths byte-identical to live RT64: `rt64_descriptor_sets.h`, `rt64_shader_library.cpp`, `RSPProcessCS.hlsl`, `rt64_present_decimator.h`, `rt64_present_stats.h`, `rt64_p09_exact_use_correlation.h`.
  - 8 paths differ from live RT64 strictly due to excluded experimental diagnostics (seam dry-runs, transform instrumentation, composite logging) preserved only in the protected worktree.
- **`frontend-scratch-clean`** (clean clone of `d0d90ba4` + frontend patch applied):
  - `git apply --check` exit 0; `git apply` exit 0; `git apply --reverse --check` exit 0.
  - Applied files byte-identical to live frontend files (`recompui.h` 6,923 bytes, `ui_state.cpp` 42,248 bytes).
- **Standalone host tests compiled and executed against `rt64-scratch-apply/src`**:
  - `rt64_present_decimator_test`: compiled exit 0; executed exit 0 (`rt64_present_decimator_test PASS`).
  - `p09_exact_use_correlation_test`: compiled exit 0; executed exit 0 (`1354/1354 passed`).
  - `p09_cpu_shader_clip_semantics`: compiled exit 0 with read-only `hlslpp`; executed exit 0 (`14/14 passed`, synthetic matches confirmed).

### 2.5 Historical Supersession & Explicit Limits
- **Older incremental patches retained for provenance only**:
  - `patches/rt64/rt64-present-rate-decimator.patch` (SHA256: `024bbc20...`) — SUPERSEDED for apply (clean pin apply fails at `present_queue.cpp:7`).
  - `patches/rt64/rt64-present-success-counter.patch` (SHA256: `173c9b7c...`) — SUPERSEDED for apply (clean pin apply fails at `present_queue.cpp:8`).
  - `patches/rt64/p09-exact-use-pathology-correlation.patch` (SHA256: `7b93258c...`, 86,535 bytes) — NOT APPLICABLE to clean pin (fails at `rsp_processor.cpp:17`, `transform_processor.cpp:1562`, `transform_processor.h:7`).
- **Pathology C++ Hook Integration Not Reproduced**: The C++ hooks (RSP producer calls, transform seam plan builder) in `p09-exact-use-pathology-correlation.patch` depend on the non-reproducible diagnostic worktree and were explicitly NOT integrated into the production patch chain. Not all P09 diagnostic hooks are claimed reproducible from the clean pin.

---

## 3. P2 Resolution: Model-Getter Diagnostic Gate Fix

### 3.1 Problem Statement
In `src/game/backpack_lookup_diag.cpp`, the wrap `__wrap_func_80092B04` unconditionally invoked `tooie_diag::v7_note_model_getter`. In turn, `v7_note_model_getter` incremented host atomic counters, read guest memory words, and called `secondary_sway_register_durable_bin`, which acquired host mutex `g_secondary_sway_mu` and wrote to host container `g_durable_bins` on every model getter lookup during default gameplay when diagnostic environment variables were unset.

### 3.2 Gate Implementation
In `include/tooie_diag.h` and `src/game/tooie_diag.cpp`:
- Introduced `getter_durable_metadata_requested()` evaluating diagnostic consumers that require durable metadata:
  - `TOOIE_DIAG_SECONDARY_SWAY`
  - `TOOIE_DIAG_FEATHER_HANDOFF_V7`
  - `TOOIE_DIAG_FEATHER_POSTPROCESS_Q`
  - `TOOIE_DIAG_FEATHER_TEMPORAL_COHERENCY` (or RT64 `RT64_DIAG_FEATHER_TEMPORAL_COHERENCY`)
  - `TOOIE_DIAG_RSP_ENDPOINT_BINDING`
  - `TOOIE_DIAG_P09_SEAM_EDGE_PATHOLOGY_CORRELATION` (strict boolean tokens)
- Stored boolean gate in atomic `g_getter_durable_needed` during `ensure_init()`.
- In `v7_note_model_getter()`, early return when `!g_getter_durable_needed` occurs prior to counter increments, guest memory reads, mutex acquisition, and metadata registration.
- Underlying guest function `__real_func_80092B04` executes normally once and its return value is preserved.

### 3.3 Gate Verification & Binary Hash Distinction
- **Host Unit Test**: `tests/getter_diag_gate_test.cpp` compiled and executed:
  `g++ -std=c++20 -Wall -Wextra -Werror -Iinclude -o /tmp/getter_diag_gate_test tests/getter_diag_gate_test.cpp && /tmp/getter_diag_gate_test`
  Exit code: 0 (`PASS getter_diag_gate_test`).
- **Game Build**: `cmake --build build --target TooieRecompiled -j2` exited 0.
- **Binary Hash Distinction**:
  - Newly compiled binary: `build/TooieRecompiled` SHA256 `74a0cea04a2bc3704860388673c40b2e73ee34d284f3a2afa0faebe91a9c05f5` (size: 29,418,200 bytes, mtime 2026-10-19 02:00:32 -0300).
  - Prior user-tested candidate binary: SHA256 `a6a2c023a9e3b0688cac15249cda61b5c8b8ccdf68c7c40f91d8c512356bef5e` (size: 29,413,928 bytes, mtime 2026-10-19 00:56:09 -0300).
  - **Epistemic boundary**: Direct user visual confirmation ("Sim, continua funcionando") was conducted on binary `A6A2` (`a6a2c023...`). Do NOT claim user visual confirmation for the newly compiled binary `74a0cea0...`. Automated unit tests and compilation exit 0 validate `74a0cea0...`.

---

## 4. Preserved Invariants and Gameplay Behavior

1. **Aspect-Aware Frustum Culling**:
   - `func_800CAA24` horizontal plane scaling remains default ON (`TOOIE_TEST_MAP_VIS_ASPECT_AWARE=1`).
   - `func_800CACEC` object side-plane expansion remains default ON (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1`) with narrow allowlist: pure draw `func_800B97A4` (kind 3) + preserved `func_8011447C` (kind 2). Mixed renderer `func_800DE498` stays on original unexpanded `v0`.
   - Widescreen object edge pop elimination is visually confirmed for the tested gameplay area on candidate binary A6A2 (`VISUALLY_CONFIRMED_FOR_TESTED_AREA`, `USER_REPORTED`). Broader maps and movement parity remain an untested residual.
2. **CURRENT `tfPos` Palliative Workaround**:
   - `apply_default_before_renderer_init()` remains default ON, setting `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1` when unset.
   - At presentation rates above native, intermediate presents repeat vertex poses. Continuous high-framerate vertex interpolation remains `NOT_IMPLEMENTED`.
3. **Attract Demo & Cutscene Parity**:
   - USER_DEMO1 trajectory (355 samples), actor records (546 words), LCG RNG snapshots (14), and Fire Egg sweep hit (`v0=0x80132f60`, `func_800CD6B4=1`) match original bit-exact under default ON.
   - Demo 7 cutscene stop is visually fixed via N64Recomp zero-size static symbol fix (`n64recomp-zero-size-static.patch`).
4. **Deferred Items**:
   - Audio glitch remains explicitly deferred per user direction.
   - 8-preset UI selector widget remains `PLANNED` / `NOT_IMPLEMENTED`.
   - Formal P09 phase status remains `PENDING / NOT_FORMALLY_OPEN`.

---

## 5. Automated Gates Executed in this Review

| Test / Gate Command | Target / Scope | Exit Code | Result |
| :--- | :--- | :---: | :--- |
| `./build/tooie_demo7_drift_test` | Demo 7 kinematic drift | 0 | PASS |
| `./build/tooie_rsp_endpoint_current_test` | CURRENT helper env logic | 0 | 20/20 PASS |
| `./build/rt64_present_decimator_test` | RT64 presentation decimator | 0 | PASS |
| `./build/tooie_fps_meter_test` | FPS rate meter calculation | 0 | PASS |
| `./build/p09_cpu_shader_clip_semantics_test` | Shader clip semantics | 0 | 14/14 PASS |
| `./build/p09_exact_use_correlation_test` | Correlation header keys | 0 | 1354/1354 PASS |
| `./build/tooie_demo1_parity_test tools/p08_attract_egg_hook.h` | Demo 1 hook parity | 0 | PASS |
| `./build/tooie_demo1_projectile_test tools/p08_attract_egg_hook.h` | Egg projectile sweep | 0 | PASS |
| `./build/tooie_demo1_target_selector_test ...` | Target selector enter CSV | 0 | PASS |
| `./build/tooie_demo1_upd_layout_test ...` | Updrec layout CSV compare | 0 | PASS |
| `bash tests/demo1_projectile_coverage_test.sh` | Projectile coverage script | 0 | PASS |
| `/tmp/getter_diag_gate_test` | P2 getter gate predicate check | 0 | PASS |
| `cmake --build build --target TooieRecompiled -j2` | Game executable build | 0 | BUILD_EXIT:0 |
| `bash -n tools/apply_runtime_patches.sh` | Applicator syntax check | 0 | PASS |
| `git diff --check` | Whitespace integrity | 0 | PASS |

---

## 6. Explicitly Not Run / Operational Boundaries

1. **No Game Launch or Termination**: The game was not launched and running processes were not terminated; the user may have the game open.
2. **No Re-Run of Native A6A2 Gate**: Native capture gate results (`demo1-frustum-candidate-native-gate.txt`) are cited from historical cache.
3. **No Applicator Execution on Live Checkouts**: `tools/apply_runtime_patches.sh` was not executed against live checkouts to preserve existing protected worktrees.
4. **No Pathology C++ Reproduction**: Pathology C++ hooks were not reproduced from clean pin.
5. **No Full Campaign QA**: General gameplay parity across all maps, mobs, and cameras remains an untested residual.

---

## 7. Next Single Action

`NEXT_SINGLE_ACTION`: extend oracle comparison to another attract demo when user requests next game adjustment.
