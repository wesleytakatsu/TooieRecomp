---
title: "Documentation and Specification Audit Ledger — 2026-10-16"
description: "Central multi-round audit ledger for the whole-project documentation and specification audit authorized by user (Rounds 1–7)."
status: current
type: evidence
project: TooieRecomp
phase: P08-P09-AUDIT
updated: 2026-10-17
tags: [audit, documentation, specifications, ledger, multi-round, inventory]
---

# Documentation and Specification Audit Ledger

## Audit Metadata

- **Session Date**: 2026-10-17
- **Current Round**: ROUND 7 (Targeted Residual Correction & Census Reconciliation) of authorized 5 to 10 actual sequential CLI invocations
- **Active Model**: Gemini 3.8 Flash (High)
- **Role**: Exclusively documentation and specification auditor in `/home/wesley/projetos/N64-Recomp/TooieRecomp`
- **Scope Authorization**: Whole-project documentation and specification audit, creating full project specs, inventorying all documentation inside and outside docs/, adversarial check, without code/build/game mutations.
- **Git HEAD**: `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf`
- **Protected State**: Worktree dirty state and untracked artifacts preserved as protected evidence.
- **Epistemic Limitation**: Raw `/tmp` transcripts and transient scratch files from prior CLI invocations (Rounds 1–3) were lost across session restart; persisted Markdown in the git repository serves as the sole canonical evidence base.
- **Scope Distinction**: Whole-tree structural inventory (file counts, frontmatter presence, link graph, MCP diagnostics) is explicitly distinguished from targeted semantic source verification (deep reality checks against C++ headers, CMake, stubs, and evidence logs). No exhaustive line-by-line semantic review of all 286 notes is claimed. No universal QA proof.

---

## Canonical Operational Facts & Constraints (Current Baseline)

1. **P09 Phase 1 Default Workaround**:
   - Status: `IMPLEMENTED`, `VALIDATED` (20/20 test assertions PASS, build exit 0, binary SHA256 `0d2e4ba5fa7e4ed0347d83180ff4c831c54b8f47567fa012de15dac3dc736b84`).
   - Behavior: `apply_default_before_renderer_init()` (`src/game/tooie_rsp_endpoint_current.cpp:14-38`) sets `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT="1"` if unset at startup before `activate_tooie_guest_preemption()` and `recomp::start()`.
   - Normal run: `./build/TooieRecompiled` runs without flags. User QA confirmed: "Está rodando bem."
   - Rollback: Explicit `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0` restores experimental interpolation.
   - Scoping: Strictly Tooie integration. RT64 default remains OFF (`TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` unset in RT64 itself). Uses shader-local `tfPos` CURRENT bypass (`RSPProcessCS.hlsl:78-84`), not global weights.
   - Future diagnostic rule: Diagnostic captures MUST explicitly set `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0` to avoid hiding glitches while CPU witnesses assume interpolation.
   - Permanent interpolation fix: `NOT_IMPLEMENTED`.
   - Latest canonical evidence: `docs/plan/evidence/p09-default-current-workaround-and-fps-plan.md`.

2. **P09 Phase 2 Exact FPS Presets**:
   - Status: `PLANNED`, `NOT_IMPLEMENTED`.
   - Presets planned: `{20, 30, 45, 60, 70, 100, 120, 144}`.
   - Constraints: Existing UI manual selector (20..240 step 1) and runtime (10..1000) are insufficient for below-native targets (`targetRate <= viOriginalRate`). RT64 sends one present per native workload without lower-rate decimation. True decimation hook and backpressure `WorkloadQueue::waitForWorkloadId` are `UNVERIFIED`.
   - Formal P09 phase status: `PENDING / NOT_FORMALLY_OPEN`. No whole-campaign parity claim.

3. **Audio Glitch Status**:
   - Status: Existing intermittent "tic tic" audio glitch is explicitly `DEFERRED` per user instruction. No audio changes.

4. **P08 Widescreen Edge Geometry Status**:
   - `func_800CAA24`: Aspect-aware H-plane culling (`n_cam_x /= k`) is production default ON (`TOOIE_TEST_MAP_VIS_ASPECT_AWARE=0` restores original).
   - `func_800CACEC`: Object model frustum side-plane culling is production default ON (`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0` restores original, DRAW-only).
   - Intro cinematic walls: Two named scenes (`SCENE_A_FIRST_VISIBLE_AFTER_LAUNCH`, `SCENE_B_NEW_GAME_INTRO`). First 16:9 RT64 divergence is `CLIP_X` (AUTO scales `viewProj[0][0]` by 4/3). Heap G_DLs are not content-stable. Production fix not implemented. Ultrawide pillarbox deferred.

5. **Guest Preemption (P08)**:
   - Hybrid F' architecture: PATCH 1-4 landed and accepted (verdict B). Cooperative yield at message queue safepoints and loop backedges.

---

## Entire First-Party Documentation Scope & Inventory

A comprehensive census of all 293 first-party markdown documents across the entire repository (286 inside the `docs/` vault and 7 outside the vault) was conducted:

### 1. In-Vault Documentation (`docs/`, 286 files)

#### A. Root & Management Documents (5 files)
- `docs/AGENTS.md`: Agent rules, read order, and operational contracts.
- `docs/HANDOFF.md`: Global operational handoff and living project state ledger.
- `docs/MARKDOWN_VAULT_TOOLS.md`: MCP markdown-vault configuration and usage.
- `docs/README.md`: Vault overview and navigation guide.
- `docs/VAULT.md`: Obsidian vault entrypoint.

#### B. Master Plan & Exit Criteria (3 files)
- `docs/plan/baseline-manifest.md`: P00 baseline verified hashes, commits, and tools.
- `docs/plan/definition-of-done.md`: Master Definition of Done criteria for project completion.
- `docs/plan/index.md`: Master project implementation plan (M0 through M6).

#### C. Implementation Stage Plans (`docs/plans/BT/`, 12 files)
- `00-baseline-and-decisions.md`: P00 Baseline and initial decisions (DONE).
- `01-linux-toolchain.md`: P01 Linux toolchain and build requirements (DONE).
- `02-rom-decompression-and-elf.md`: P02 ROM decompression and matching ELF (DONE).
- `03-recompiler-metadata.md`: P03 Symbol, section, and overlay metadata (DONE).
- `04-static-recompilation.md`: P04 Static recompilation via N64Recomp (DONE).
- `05-native-runtime-and-boot.md`: P05 Native runtime boot and first frame (DONE).
- `06-overlays-and-syscalls.md`: P06 Overlay system, syscall dispatch, dynamic loading (DONE/MAINTAINED).
- `07-rsp-and-audio.md`: P07 RSP microcode, audio task dispatch, SoundManager (ACTIVE/AUDIO_OK).
- `08-rt64-input-and-frontend.md`: P08 RT64 rendering, input mapping, windowing, widescreen (ACTIVE/MANUAL_VALIDATION_PENDING).
- `09-gameplay-parity-and-enhancements.md`: P09 Campaign parity, high framerate interpolation, saves (PENDING/NOT_FORMALLY_OPEN).
- `10-linux-packaging-and-release.md`: P10 Linux standalone packaging and release (PENDING).
- `index.md`: Ordered index of all implementation phases.

#### D. Reference & Input Specifications (`docs/specs/`, 45 files)
- `docs/specs/index.md` (1 file): Master specifications hub and requirements verification matrix.
- `docs/specs/Tooie/` (13 files total: 12 functional domain specifications + 1 hub/index):
  - Hub: `index.md`.
  - Domain specs (12): `build-and-generator.md`, `runtime-and-overlays.md`, `graphics-and-presentation.md`, `input-and-controllers.md`, `window-ui-and-config.md`, `audio-system.md`, `save-system.md`, `gameplay-parity.md`, `diagnostics-and-telemetry.md`, `testing-and-oracle.md`, `packaging-and-dependencies.md`, `modding-and-extensions.md`.
- `docs/specs/BK/` (14 files total: 13 reference topics + 1 index):
  - Hub: `index.md`.
  - Reference topic specs (13): `overview.md`, `config.md`, `symbols.md`, `patches.md`, `rsp.md`, `build.md`, `runtime.md`, `ui.md`, `input.md`, `features.md`, `mods.md`, `ci.md`, `reuse-contract.md`.
- `docs/specs/BT/` (17 files total: 16 decompilation insumos + 1 index):
  - Hub: `index.md`.
  - Insumos topic specs (16): `overview.md`, `setup.md`, `splat.md`, `build.md`, `symbols.md`, `overlays.md`, `syscalls.md`, `rsp-and-audio.md`, `input.md`, `compression.md`, `tools.md`, `angrylion-control-oracle.md`, `sources.md`, `matching.md`, `ci.md`, `recomp-readiness.md`.
*(Census Distinction: Filesystem count across `docs/specs/` is 45 `.md` files total: 41 domain/topic specifications + 4 index/hub documents. Do not conflate domain counts with filesystem totals.)*

#### E. Reverse Engineering Knowledge Base (`docs/reverse-engineering/`, 22 files)
- Asset knowledge: `assets/README.md`, `assets/animations.md`, `assets/models.md`.
- Character models: `characters/banjo.md`, `characters/kazooie.md`, `characters/mumbo.md`.
- Systems: `systems/animation.md`, `systems/geocmd.md`, `systems/model-renderer.md`, `systems/overlays.md`, `systems/resource-system.md`, `systems/secondary-animation.md`, `systems/transform-interpolation.md`.
- Symbols & Structs: `symbols/functions.md`, `symbols/overlays.md`, `symbols/structs.md`.
- Modding & Corrections: `modding/README.md`, `modding/known-extension-points.md`, `corrections.md`, `glossary.md`, `README.md`, `p09-v3-real-guest-matrix-match.md`.

#### F. Historical Roadmap (1 file)
- `docs/roadmap/tooie-recomp.md`: Early architectural roadmap (SUPERSEDED by `docs/plan/index.md`).

#### G. Evidence Notes (`docs/plan/evidence/`, 198 files)
- Central Audit Ledger: 1 note (`documentation-spec-audit-20261016.md`).
- P07 Audio Evidence: 1 note (`p07-boot-hang-audio-diag.md`).
- P08 RT64, Preemption, Attract & Input Evidence: 110 notes (covering stick-angle thunk, BCF28 binding, 80013A7C lookup-zero, guest preemption patches 1-4, slot 7 lifecycle, demo motion differentials, widescreen edge geometry, `p08-visual-validation/README.md`, etc.).
- P09 Interpolation, Seam Edge & FPS Evidence: 86 notes (covering feather investigations, snout layout, pathological tie groups, temporal cohort analysis, tfPos current workaround, etc.).
*(Total in-vault markdown count: 5 root + 3 master plan + 12 stage plans + 45 specs + 22 reverse engineering + 1 roadmap + 198 evidence = 286 files).*

### 2. Outside-Vault First-Party Documentation (7 files)

A thorough inventory of first-party Markdown files outside `docs/` was performed:
- **`AGENTS.md`** (Root repository): Root agent instructions, required read order, verified culling production defaults (`800CAA24` and `800CACEC`), overlay dispatch facts, and graft context usage.
- **`GEMINI.md`** (Root repository): Untracked first-party instructions for Gemini agent graft tool integration.
- **`tools/oracle/README.md`**: Guide for the `ANGRYLION_CONTROL` measurement oracle, launcher scripts (`run-tooie.sh`, `run-tooie-angrylion-rdp.sh`, `angrylion-control/run.sh`), and reference contracts.
- **`metadata/coverage_report.md`**: Generated P03 recompiler coverage report documenting 854 executable ELF sections, 3,310,496 executable bytes, relocations (`R_MIPS_26`, `HI16`, `LO16`), and exact overlay counts: 884 overlay table slots (33 empty gap slots, 851 named overlays). Classified as a **generated document (read-only; do not hand-edit)**.
- **`metadata/rsp_tasks.inventory.md`**: Generated P07 RSP and OSTask scheduler inventory documenting `M_GFXTASK` (F3DEX2 NoN/L3DEX2 via RT64 HLE), `M_AUDTASK` (`n_aspMain` recompiled microcode), static RDRAM templates (`D_8003F3B0` audio, `D_8003F3F0` graphics), ROM blobs, and SDL audio host sink contracts. Classified as a **generated document (read-only; do not hand-edit)**.
- **`.claude/skills/graft/SKILL.md`**: Untracked Claude agent skill guide for graft context graph queries.
- **`.grok/skills/graft/SKILL.md`**: Untracked Grok agent skill guide for graft context graph queries.

*Note on Root Guides*: Neither a root `README.md` nor a `.github/` folder exists in this repository.

*Note on Root Guides*: Neither a root `README.md` nor a `.github/` folder exists in this repository.

### 3. Explicit Exclusions from Documentation Scope

The following generated graphs, external checkouts, and build artifacts are explicitly excluded:
- **`graft/`**: Automatically generated Markdown context graph and index (ignored by `.gitignore`, refreshed deterministically via `graft build`).
- **Sibling Repositories & Dependencies**: External sibling checkouts (`../BanjoRecomp`, `../ROM`, `../banjo-tooie-Recomp`).
- **Ignored Build Outputs & Cache Directories**: `build/`, `build-recomp/`, `RecompiledFuncs/`, `.serena/cache/`, `.opencode/node_modules/`, `tools/__pycache__/`, `tools/oracle/angrylion-control/work/`.

---

## Vault Health & Diagnostic Issues Census (Reconciled)

### 1. Baseline Reality Audit of Initial Diagnostic Claims
Earlier Round 1–3 ledger summaries recorded "2 errors, 30 diagnostic issues, 44 lint warnings" and labeled the check as "PASS". A rigorous reality audit reveals that this summary mischaracterized tool output and conflated warnings with clean passes:

1. **Spurious "2 Errors" Resolved**:
   - `docs/plan/evidence/p09-projective-w-production-fix.md:154`: Contains C++ lambda code (`auto noteExplosion = [&] (float ratio, ...`) inside a standard fenced ````cpp` code block. Earlier diagnostic parsing misidentified `[&] (float...` as an unescaped wikilink. It is valid C++ in a code block and not a broken link.
   - `docs/VAULT.md:6`: The earlier ledger claimed a broken wikilink `[ [ plan/evidence ] ]` existed on line 6. In reality, line 6 of `docs/VAULT.md` is YAML frontmatter (`tags: [vault, index, tooierecomp]`). No broken directory wikilink exists.
   - **Ground Truth**: The true number of syntax or broken link errors in the vault baseline is **0**.

2. **Real Diagnostic Issues**:
   - `markdown_vault_diagnose_docs` baseline identified **23 issues** (not 30):
     - **14 Large Files**: Files exceeding 800 lines or 30,000 characters. These are living operational logs, master handoffs, and forensic evidence notes (`HANDOFF.md`, `p08-widescreen-edge-geometry.md`, `p08-session-handoff.md`, `p08-user-demo5-locomotion-first-divergence.md`, `p08-attract-movement-residuals.md`, `p08-demo-movement-precision-diagnosis.md`, `p09-session-handoff.md`, `plans/BT/08-rt64-input-and-frontend.md`, `plans/BT/09-gameplay-parity-and-enhancements.md`, `p08-preemption-design-adversarial-review.md`, `p08-preemption-patch35-design.md`, `p09-current-work.md`, `reverse-engineering/systems/transform-interpolation.md`, `VAULT.md`). These documents are essential historical evidence and operational handoffs that must not be truncated or deleted.
     - **9 Missing Frontmatter Warnings**: Evidence notes created during rapid forensic sessions lacking YAML frontmatter headers.
   - **44 Lint Warnings**: Non-blocking stylistic issues in historical evidence notes (multiple H1 headings, trailing spaces).

3. **Status Classification Rule**:
   - Audits that report warnings or diagnostic findings must not be labeled clean or `PASS` merely because the issues pre-existed. They are accurately classified as `NON_BLOCKING_WARNINGS` with remaining issues detailed explicitly.

### 2. Post-Repair Vault Health Status (Reconciled across Rounds 4–7)
- **Missing Frontmatter**: 0 (all 9 notes repaired with valid YAML frontmatter).
- **Broken Wikilinks / Errors**: 0.
- **Orphan Navigation**: Repaired (linked `symbols/structs` in `VAULT.md`; linked `p09-kazooie-feather-v3-implementation` and `v4-capture-analysis` in `plans/BT/09-gameplay-parity-and-enhancements.md`).
- **Remaining Diagnostic Issues**: 15 large file warnings (>800 lines or >30k characters):
  - **14 Pre-existing Large Files**: Essential historical evidence, living handoffs, and master plans (`HANDOFF.md`, `p08-widescreen-edge-geometry.md`, `p08-session-handoff.md`, `p08-user-demo5-locomotion-first-divergence.md`, `p08-attract-movement-residuals.md`, `p08-demo-movement-precision-diagnosis.md`, `p09-session-handoff.md`, `plans/BT/08-rt64-input-and-frontend.md`, `plans/BT/09-gameplay-parity-and-enhancements.md`, `p08-preemption-design-adversarial-review.md`, `p08-preemption-patch35-design.md`, `p09-current-work.md`, `reverse-engineering/systems/transform-interpolation.md`, `VAULT.md`).
  - **1 Newly Large File**: The central audit ledger itself (`docs/plan/evidence/documentation-spec-audit-20261016.md`), which grew beyond 30,000 characters as a result of documenting comprehensive multi-round findings.
- **Remaining Lint Warnings**: 44 (all pre-existing historical markdown formatting in evidence notes: multiple H1 headings, trailing spaces).
- **Current Vault Audit Verdict**: `NON_BLOCKING_WARNINGS` (0 errors, 0 missing frontmatter, 59 total warnings: 58 pre-existing warnings + 1 newly large audit ledger; do not misreport all 59 warnings as pre-existing).

---

## Code Subsystem Outline (`TooieRecomp/`)

The repository codebase was inventoried via jCodeMunch (`wesleytakatsu/TooieRecomp`, 115 tracked files, 2440 symbols):

| Subsystem | Key Directories & Files | Roles & Contracts |
| :--- | :--- | :--- |
| **Build & Toolchain** | `CMakeLists.txt`, `tools/run_recomp.sh`, `tools/run_rsprecomp.sh`, `tools/generate_metadata.py` | CMake build definition, N64Recomp / RSPRecomp runners, metadata generators. |
| **Metadata & Config** | `metadata/manifest.json`, `metadata/*.table.toml`, `tooie.us.rev0.*.toml`, `n_aspMain.us.rev0.toml` | Symbol, section, overlay, and syscall tables for static recompilation. |
| **Runtime & Boot** | `src/main/main.cpp`, `src/game/boot_hw_stubs.cpp`, `src/game/recomp_api.cpp` | Native initialization, CIC-6105 emulation, SI/PI stubs, guest preemption startup. |
| **Overlays & Syscalls** | `src/game/syscall_handler.cpp`, `src/main/register_overlays.cpp`, `tools/generate_overlay_dispatch.py` | Heap-based overlay relocation and loading across 884 overlay slots (`kTooieOverlayCount = 884`, ~851 populated). |
| **Graphics & RT64** | `src/game/transform_hooks.cpp`, `src/game/tintro_projection_policy.cpp`, `patches/graphics.h`, `patches/transform_ids.h` | RT64 draw hooks, projection ratio overrides, widescreen edge geometry gates. |
| **RSP & Interpolation**| `src/game/tooie_rsp_endpoint_current.cpp`, `include/tooie_rsp_endpoint_current.h`, `rsp/n_aspMain.cpp` | Shader-local CURRENT tfPos bypass workaround, ASP microcode execution. |
| **Input & Thunks** | `src/game/recomp_api.cpp`, `tests/stick_angle_thunk_test.cpp`, `tests/input_direction_test.cpp` | SDL2 controller polling, `0x80013B70` stick angle normalization override. |
| **Window & UI** | `src/game/tooie_window.cpp`, `src/game/tooie_error.cpp`, `src/main/theme.cpp`, `src/main/launcher_animation.cpp` | Window creation, Vulkan failure fallback, RecompUI theme and animation. |
| **Diagnostics** | `include/tooie_diag.h`, `src/game/tooie_diag.cpp`, `src/game/backpack_lookup_diag.cpp` | Default-OFF runtime telemetry, dry-run collectors, seam edge correlation. |
| **Test Suite** | `tests/*.cpp`, `tests/*.py` | 14 focused unit tests covering preemption, matrices, rounding, windowing, input, RSP bypass. |
| **Measurement Oracle**| `tools/oracle/angrylion-control/` | Mupen64Plus + Angrylion RDP Plus + CXD4 LLE bit-accurate ground truth oracle. |

---

## Gap Analysis: Missing TooieRecomp Specification Suite

Before this audit, `docs/specs/` contained:
- `docs/specs/BK/` (13 specs): Banjo-Kazooie reference implementation contracts.
- `docs/specs/BT/` (17 specs): Banjo-Tooie decompilation insumos contracts.

**Critical Gap**: TooieRecomp itself lacked a dedicated, coherent specification suite. Architectural contracts, runtime invariants, widescreen policies, interpolation bypass behaviors, and preemption semantics were documented only in phase plans (`plans/BT/00..10`) or scattered across 198 evidence notes.

To solve this gap, a full 11-domain specification suite for TooieRecomp is established under `docs/specs/Tooie/`:
1. `docs/specs/Tooie/index.md` — Specification Hub & System Architecture
2. `docs/specs/Tooie/build-and-generator.md` — Build Pipeline & Recompiler Generator
3. `docs/specs/Tooie/runtime-and-overlays.md` — ModernRuntime, Overlays & Guest Preemption
4. `docs/specs/Tooie/graphics-and-presentation.md` — RT64, Widescreen & Presentation Workaround
5. `docs/specs/Tooie/input-and-controllers.md` — SDL2 Input, Normalization & Attract Contracts
6. `docs/specs/Tooie/window-ui-and-config.md` — Window Management, Fallback & UI Configuration
7. `docs/specs/Tooie/audio-system.md` — RSP Audio, SoundManager & Scheduling
8. `docs/specs/Tooie/save-system.md` — EEPROM Persistence & Storage Architecture
9. `docs/specs/Tooie/gameplay-parity.md` — Parity Criteria, Physics & Camera Verification
10. `docs/specs/Tooie/diagnostics-and-telemetry.md` — Diagnostic System, Telemetry & Safety
11. `docs/specs/Tooie/packaging-and-dependencies.md` — Linux Packaging & Dependency Boundary
12. `docs/specs/Tooie/modding-and-extensions.md` — Modding, Extensions & Asset Customization

---

## Multi-Round Audit Plan (Rounds 1 to 7 + Contingency)

| Round | Focus / Objective | Target Scope | Execution Status & Actual Focus |
| :--- | :--- | :--- | :--- |
| **ROUND 1** | **Whole Vault Structural Census & Foundational Specifications** | Whole vault census, code outline, audit ledger, and foundational TooieRecomp spec suite. | **COMPLETED**: Central audit ledger (`documentation-spec-audit-20261016.md`), updated `docs/specs/index.md` master matrix, and complete 12-domain `docs/specs/Tooie/` suite created. |
| **ROUND 2** | **Specification Deepening & Requirements Traceability** | Deepen domain specs with formal REQ IDs, C++ declarations, invariants, and test coverage. | **COMPLETED**: REQ IDs across all 12 specs, explicit struct layouts, memory constants, and `docs/specs/Tooie/modding-and-extensions.md`. |
| **ROUND 3** | **Targeted Semantic Source Verification** | Targeted reality check of key specification contracts against primary C++ code, headers, metadata, and evidence logs. | **COMPLETED**: Targeted verification (distinguished from whole-vault line-by-line review) proving 884 overlays, dynamic/unknown cadence, tfPos CURRENT workaround vs NOT_IMPLEMENTED permanent fix, audio soak gap, and EEPROM runtime separation. |
| **ROUND 4** | **Vault Crosslinks, Frontmatter Repair, AGENTS.md Supersession & Ledger Count Reconciliation** | Audit whole docs vault links, anchors, frontmatter, orphan navigation; repair 9 missing frontmatters; supersede stale cinematic wall claims in `docs/AGENTS.md`; reconcile misleading ledger counts. | **COMPLETED**: Added YAML frontmatter to 9 evidence notes; linked orphan notes (`symbols/structs` in `VAULT.md`, feather v3/v4 in `plans/BT/09`); updated stale SCENE_A wall claim in `docs/AGENTS.md`; reconciled spurious "2 errors" finding; reported honest warning status. |
| **ROUND 5** | **Adversarial Semantic Audit of All 12 Spec Domains & BK/BT Reuse Boundaries** | Adversarial verification of 8 key claims against primary sources/evidence: 884 overlays, logic Hz UNKNOWN, 14-test catalogue reality, glibc 2.31 target, save QA, modding infrastructure, tfPos flag precedence, tfPos driving fog & below-native pacing. | **COMPLETED**: Audited all 12 Tooie domains and BK/BT contracts; corrected overstatements in `specs/index.md`, `save-system.md`, `modding-and-extensions.md`, `packaging-and-dependencies.md`, `testing-and-oracle.md`, and `graphics-and-presentation.md`; ran vault audit and diff check. |
| **ROUND 6** | **Fresh Context Final Audit: Outside-Docs First-Party Markdown & Final Consistency Reconciliation** | Independent audit of first-party Markdown outside docs (`AGENTS.md`, `GEMINI.md`, `tools/oracle/README.md`, `metadata/*.md`, `.claude/skills/`, `.grok/skills/`), complete whole-project inventory, adversarial recheck, handoff synchronization. | **COMPLETED**: Verified outside-docs contracts, complete 293-file inventory (286 inside docs, 7 outside), verified absence of root README/.github, synchronized handoff, executed full vault audit and whitespace check. |
| **ROUND 7** *(Current)* | **Targeted Residual Correction & Census Reconciliation** | Correct residual factual error in `save-system.md` (`osEepromProbe` return value vs Joybus raw status; REQ-SAV-03 criteria vs acceptance; REQ-SAV-01 provenance; atomic persistence crash-safety scope; Joybus vs SDK layer ownership); reconcile filesystem spec counts (45 total across BK/BT/Tooie without domain/index conflation); classify outside generated metadata docs as read-only; reconcile ledger large-file warning in 59-warning census; update handoff and audit matrix. | **COMPLETED**: Corrected `docs/specs/Tooie/save-system.md` and `docs/specs/index.md`; verified `N64ModernRuntime/librecomp/src/eep.cpp:11-24` and `src/game/boot_hw_stubs.cpp:656-668`; reconciled filesystem spec counts (45 files: 13 Tooie, 14 BK, 17 BT, 1 root index); classified generated metadata docs read-only; reconciled warning origins; ran vault audit and diff check. |
| **ROUNDS 8–10** | **Contingency / Future Deep Resolution** | Only if material unresolved gaps or architectural contradictions emerge from Round 7. | **CONTINGENCY**: Reserved for future sequential CLI invocations if authorized. |

---

## Round 1 Execution Checks & Exit Codes

| Command / Check | Tool / Runner | Result / Exit Code | Status |
| :--- | :--- | :--- | :--- |
| `markdown_vault_audit` | markdown-vault MCP | 0 errors, 23 diagnostic warnings (14 large files, 9 missing frontmatter), 44 lint warnings | NON_BLOCKING_WARNINGS (baseline characterized; spurious "2 errors" reconciled) |
| `markdown_vault_diagnose_docs` | markdown-vault MCP | Characterized 23 issues (14 large files, 9 missing frontmatter) | NON_BLOCKING_WARNINGS (categorized in ledger) |
| `jcodemunch` repository query | jCodeMunch MCP | 115 files, 2440 symbols outlined | PASS (code tree mapped) |
| `git diff --check` | bash / native git | Exit code 0 | PASS (no whitespace or conflict errors) |

---

## Round 2 Execution — Complete Whole-Project Specification Coverage

### 1. Objectives & Approach
- In this second actual CLI invocation, the whole-project specification coverage was completed.
- Rather than leaving skeletal specifications, every domain specification in `docs/specs/Tooie/` was deepened with:
  1. **Traceable Requirements**: Explicit REQ IDs with concrete acceptance criteria.
  2. **Exact Interfaces**: Verified C/C++ function declarations, struct layouts, memory constants, and environment flags.
  3. **Architectural Invariants**: Thread safety, memory ownership, circuit breakers, ordering constraints, and fallback rules.
  4. **Acceptance Test Coverage**: Assertion counts, execution commands, and golden comparisons.
  5. **Implementation vs Intended Status**: Honest classification across `IMPLEMENTED`, `VALIDATED`, `PLANNED`, and `UNKNOWN`.
- Established a dedicated 12th domain specification: `docs/specs/Tooie/modding-and-extensions.md` covering `src/game/recomp_extension_api.cpp`, `recomp_data_api.cpp`, `dod::slot_map32`, and `.rtz` texture pack loading.
- Synchronized `docs/specs/index.md` with REQ-21 and full 12-domain indexing.

### 2. Detailed Subsystem Audit & Contracts Formalized

| Subsystem Domain | Spec Document | Key Contracts, Interfaces & Invariants Formalized |
| :--- | :--- | :--- |
| **Build System & Generator** | [[Tooie/build-and-generator]] | CMake targets (`TooieRecompiled`, `libRecompiledFuncs.a`, `libRecompiledPatches.a`), Clang/GCC C++20 options, N64Recomp/RSPRecomp generation pipelines, metadata determinism via `tools/generate_metadata.py`, freshness invariants via `tools/check_generated_fresh.py`. |
| **Runtime, Overlays & Preemption**| [[Tooie/runtime-and-overlays]] | Memory constants (`kCore2Ram = 0x800815C0u`, `kOverlayVram = 0x80800000u`, 884 overlay slots), `ActiveOverlayMapping`, `s_resident_overlays`, `ovl_load`/`ovl_unload`, leaf wrapper RA preservation, Hybrid F' preemption (PATCH 1-4, TLS RCP contexts, cooperative yields in `func_800C2AB8`), boot IPL3 residue at `0x802FB1F4`/`0x802FE1C0`. |
| **Graphics & Presentation** | [[Tooie/graphics-and-presentation]] | `func_800CAA24` ($n_{\text{cam}, x}' = n_{\text{cam}, x} / k$), `func_800CACEC` object frustum expansion, `tooie_tintro_projection_policy`, `tooie_rsp_endpoint_current::apply_default_before_renderer_init()`, planned FPS presets {20..144} and below-native `WorkloadQueue` pacing constraint, causal baseline ground truth. |
| **Input & Controllers** | [[Tooie/input-and-controllers]] | `func_80013B70` stick angle normalization ($r1$ delay slot side effect), `0x800979D8` input helper contract, Demo 0 matching vs Demo 1/5 open residuals, SDL2 GameController database. |
| **Windowing & Config** | [[Tooie/window-ui-and-config]] | `tooie_window` sanitization/scaling, `tooie_error` Vulkan fallback, `graphics.json` persistence. |
| **Audio Subsystem** | [[Tooie/audio-system]] | `n_aspMain` microcode execution at `0x17D0`, `M_AUDTASK` dispatch, SoundManager delta queue, deferred "tic tic" audio glitch status. |
| **Save System** | [[Tooie/save-system]] | 16 Kbit EEPROM Joybus commands (`0x04`, `0x05`), title file selection validation, atomic `.tmp` persistence. |
| **Gameplay Parity** | [[Tooie/gameplay-parity]] | `round.w.s` nearest-even rounding, viewport clipping vs collision, attract sequence registry. |
| **Diagnostics & Telemetry** | [[Tooie/diagnostics-and-telemetry]] | Flag catalog, mandatory diagnostic rollback rule (`FORCE_ENDPOINT_CURRENT=0`), atomic file writes, circuit breakers (max 24 attempts). |
| **Testing & Oracle** | [[Tooie/testing-and-oracle]] | 14 automated unit tests, `ANGRYLION_CONTROL` pure interpreter software oracle. |
| **Packaging & Dependencies** | [[Tooie/packaging-and-dependencies]] | Standalone Linux release, clean-room ROM extraction, system dependency boundary (`glibc >= 2.31`). |
| **Modding & Extensions** | [[Tooie/modding-and-extensions]] | `ExtensionInfo`, `ExtensionData`, `dod::slot_map32`, `.rtz` container format. |

### 3. Round 2 Execution Checks & Exit Codes

| Command / Check | Tool / Runner | Result / Exit Code | Status |
| :--- | :--- | :--- | :--- |
| `markdown_vault_audit` | markdown-vault MCP | 0 errors, 23 diagnostic warnings (14 large files, 9 missing frontmatter), 44 lint warnings | NON_BLOCKING_WARNINGS (286 scanned notes; pre-existing warnings) |
| `git diff --check` | bash / native git | Exit code 0 | PASS (no whitespace or conflict errors) |
| Worktree status | bash / native git | Pre-existing dirty state preserved; only Markdown modified | PASS |

---

## Round 3 Execution — Independent Audit of Specifications Against Primary Code/Evidence

### 1. Objectives & Approach
- In this third actual CLI invocation (Round 3 of at least 5), a targeted semantic reality audit of key specification claims was conducted against primary C++ code, generated headers, metadata tables, and primary experimental evidence (distinguishing targeted verification from whole-vault line-by-line review).
- Prioritized audit domains:
  1. **Dynamic Overlays**: Verified total overlay index bounds, relocation, and dispatch headers.
  2. **Timing & Native Cadence**: Verified NTSC contracts, guest frame divisor status, and framerate API semantics.
  3. **Rendering Workaround vs Permanent Fix**: Verified shader-local `tfPos` bypass, interpolation state, and below-native pacing constraints.
  4. **Audio Subsystem**: Audited microcode execution status, soak testing requirements, and deferred artifact scope.
  5. **Save System & EEPROM**: Audited Joybus command handling, hardware stub jump bindings, and separation from recomp data API.
  6. **Configuration, Input & Packaging**: Verified SDL2 mappings, stick angle thunks, and standalone dependency isolation.

### 2. Discrepancies Identified and Corrections Applied

| Subsystem Domain | Specification Document | Discrepancy Found | Primary Code / Evidence Ground Truth | Action Taken in Round 3 |
| :--- | :--- | :--- | :--- | :--- |
| **Runtime & Overlays** | [[Tooie/runtime-and-overlays]], [[Tooie/build-and-generator]], [[Tooie/index]] | Earlier documentation loosely cited "851 dynamic overlays" based on early decompilation estimations. | `src/game/generated/overlay_dispatch.inc.h:13` defines `static constexpr uint32_t kTooieOverlayCount = 884;` (slots 1..884, slot 0 unused), matching `metadata/overlays.table.toml`. | Corrected overlay count to **884** across `runtime-and-overlays.md`, `build-and-generator.md`, and `docs/specs/Tooie/index.md`. |
| **Timing & Native Cadence** | [[Tooie/gameplay-parity]], [[Tooie/graphics-and-presentation]] | Specs loosely referenced "30/20 Hz guest tick cadence". | `docs/plan/evidence/p08-original-timing-4x3-baseline.md` establishes `EXPECTED_VI_HZ = 60.000 Hz`, but `EXPECTED_LOGIC_HZ`, `EXPECTED_RENDERED_GAME_FRAMES_HZ`, and `EXPECTED_ANIMATION_UPDATE_HZ` are formally `UNKNOWN`. `recomp_get_target_framerate` returns `60 / frame_divisor`, but the active divisor is dynamic and unproven. | Corrected specifications to explicitly declare guest logic Hz and animation Hz as strictly `UNKNOWN`. Added section 3.3 in `gameplay-parity.md`. |
| **Graphics & Presentation** | [[Tooie/graphics-and-presentation]] | Ambiguity regarding whether 60+ FPS high-framerate interpolation was fully solved. | `p09-default-current-workaround-and-fps-plan.md` and `main.cpp:1153`: `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=1` is strictly a palliative workaround. Continuous position interpolation at 60+ FPS is `NOT_IMPLEMENTED`. Permanent fix for `NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS` is `NOT_IMPLEMENTED`. | Explicitly categorized the current state as a palliative workaround, confirmed continuous interpolation is `NOT_IMPLEMENTED`, and documented below-native presentation pacing blocker. |
| **Audio Subsystem** | [[Tooie/audio-system]] | Initial specs did not track unexecuted soak tests. | `docs/plans/BT/07-rsp-and-audio.md:29` explicitly lists `P07-T07` soak testing for underrun, drift, and buffer leak as unexecuted (`TODO`). "Tic tic" popping artifact is explicitly `DEFERRED` per user direction. | Added `REQ-AUD-06` (`UNTESTED` / `PENDING`) and updated section 4 to explicitly state full campaign audio parity is unvalidated. |
| **Save System** | [[Tooie/save-system]] | Architecture diagram conflated low-level EEPROM joybus emulation with `recomp_data_api.cpp`. | `src/game/boot_hw_stubs.cpp:656-668` binds guest OS calls (`0x80030170`, `0x8002DE70`, etc.) to `N64ModernRuntime` functions. `recomp_data_api.cpp` provides high-level mod/extension data storage. | Clarified runtime layer ownership in architecture diagram and hardware protocol contracts. |

### 3. Round 3 Execution Checks & Exit Codes

| Command / Check | Tool / Runner | Result / Exit Code | Status |
| :--- | :--- | :--- | :--- |
| `markdown_vault_audit` | markdown-vault MCP | 0 errors, 23 diagnostic warnings (14 large files, 9 missing frontmatter), 44 lint warnings | NON_BLOCKING_WARNINGS (286 scanned notes; pre-existing warnings) |
| `git diff --check` | bash / native git | Exit code 0 | PASS (no whitespace or conflict errors) |
| Worktree status | bash / native git | Pre-existing dirty state preserved; only Markdown modified | PASS |

---

## Round 4 Execution — Vault Health Repair, Orphan Navigation, AGENTS.md Supersession & Ledger Reconciliation

### 1. Objectives & Approach
- In this fourth actual CLI invocation (Round 4 of at least 5, executed 2026-10-17), the focus was whole-vault link, anchor, frontmatter, and orphan navigation resolution, updating stale agent contracts, and reconciling misleading audit counts:
  1. **Missing Frontmatter Repair**: Added valid, standard YAML frontmatter to all 9 identified evidence notes, deriving title, description, phase, and tags from source evidence while strictly preserving existing technical logs and headings.
  2. **Orphan Navigation Repair**: Linked `docs/reverse-engineering/symbols/structs.md` into `docs/VAULT.md`, and added `docs/plan/evidence/p09-kazooie-feather-v3-implementation.md` and `v4-capture-analysis.md` into the canonical evidence index of `docs/plans/BT/09-gameplay-parity-and-enhancements.md`.
  3. **Stale Cinematic Wall Status Supersession in `docs/AGENTS.md`**: Marked the stale opening type-13 reject plane claim (`docs/AGENTS.md:200-201`) as `CORRECTED/SUPERSEDED` based on primary evidence (`SOURCE_REJECT_TARGET_ACCEPT: NO`, leaves not visible hole owner). Linked canonical contract `docs/specs/Tooie/graphics-and-presentation.md` (`REQ-GFX-08`), `docs/HANDOFF.md`, and `docs/plans/BT/index.md` (widescreen edge geometry closed for current scope, ultrawide residual deferred, active focus on P09). No new blocking policies were introduced.
  4. **Ledger Reality Reconciliation**: Reconciled misleading earlier ledger summaries claiming "2 errors" (spurious parsing artifacts in code block `[&]` and phantom `docs/VAULT.md:6` directory link). Reconciled audit status reporting to honestly classify non-blocking warnings rather than labeling audits with pre-existing issues clean or PASS.
  5. **Session Restart Acknowledgment**: Explicitly documented that transient `/tmp` transcripts from earlier CLI rounds were lost across session restart, establishing persisted repository Markdown as the sole canonical source of truth.

### 2. Repairs Executed

| Target File | Action Taken | Rationale & Evidence Ground Truth |
| :--- | :--- | :--- |
| `docs/plan/evidence/p07-boot-hang-audio-diag.md` | Added standard YAML frontmatter | Resolves missing frontmatter diagnostic warning; preserves digital silence and pulse sink analysis. |
| `docs/plan/evidence/p08-controller-host-path.md` | Added standard YAML frontmatter | Resolves missing frontmatter diagnostic warning; preserves controller stub and CIC-6105 isolation notes. |
| `docs/plan/evidence/p08-demo-camera-clearance-rays.md` | Added standard YAML frontmatter | Resolves missing frontmatter diagnostic warning; preserves Q0..Q3 projection sample differential. |
| `docs/plan/evidence/p08-demo-camera-eye-producer-differential.md` | Added standard YAML frontmatter | Resolves missing frontmatter diagnostic warning; preserves 3rd SET74 skip causal proof. |
| `docs/plan/evidence/p08-demo-ncbapivot-branch-predicate.md` | Added standard YAML frontmatter | Resolves missing frontmatter diagnostic warning; preserves CASE C branch predicate differential. |
| `docs/plan/evidence/p08-do-dma-segv-diagnosis.md` | Added standard YAML frontmatter | Resolves missing frontmatter diagnostic warning; preserves overlay base-vram collision diagnosis. |
| `docs/plan/evidence/p08-t07-t08-frontend-system-validation.md` | Added standard YAML frontmatter | Resolves missing frontmatter diagnostic warning; preserves windowing and Vulkan fallback test results. |
| `docs/plan/evidence/p08-type4-transition-semantics-audit.md` | Added standard YAML frontmatter | Resolves missing frontmatter diagnostic warning; preserves 8/8 run clean-freeze type-4 completion proof. |
| `docs/plan/evidence/p09-v2-snout-runtime-correlation.md` | Added standard YAML frontmatter | Resolves missing frontmatter diagnostic warning; preserves snout DCF48 single G_MTX correlation. |
| `docs/VAULT.md` | Added `[[reverse-engineering/symbols/structs]]` | Resolves orphan navigation for reverse engineering struct registry. |
| `docs/plans/BT/09-gameplay-parity-and-enhancements.md` | Added feather v3 and v4 evidence links | Resolves orphan navigation and integrates complete feather investigation sequence. |
| `docs/AGENTS.md` | Minimally marked stale cinematic wall claim as `CORRECTED/SUPERSEDED` | Reconciles stale SCENE_A type-13 reject plane claim with latest canonical contract [[specs/Tooie/graphics-and-presentation]], [[plans/BT/index]], and [[HANDOFF]]. |

### 3. Canonical Technical Reality Reaffirmed
- **P09 Phase 1 Workaround**: Default-ON CURRENT `tfPos` bypass (`apply_default_before_renderer_init()`) is the current running default; explicit `"0"` restores experimental interpolation.
- **Permanent Interpolation Fix**: `NOT_IMPLEMENTED`.
- **Exact FPS Presets**: {20, 30, 45, 60, 70, 100, 120, 144} `PLANNED`, `NOT_IMPLEMENTED` (blocked by below-native presentation pacing in RT64 `WorkloadQueue`).
- **Audio Subsystem**: "Tic tic" popping artifact is explicitly `DEFERRED` per user direction; no audio changes.
- **Formal P09 Phase**: `PENDING / NOT_FORMALLY_OPEN`.
- **Historical Testing**: Existing unit/build test results from prior checkpoints were not rerun this task.

### 4. Round 4 Execution Checks & Exit Codes

| Command / Check | Tool / Runner | Result / Exit Code | Status |
| :--- | :--- | :--- | :--- |
| `markdown_vault_audit` | markdown-vault MCP | 0 errors, 15 diagnostic warnings (large files), 44 lint warnings (historical markdown) | NON_BLOCKING_WARNINGS (0 missing frontmatter; 59 total non-blocking warnings) |
| `markdown_vault_diagnose_docs` | markdown-vault MCP | 0 missing frontmatter, 15 large files | NON_BLOCKING_WARNINGS (dropped from 23 to 15 issues) |
| `git diff --check` | bash / native git | Exit code 0 | PASS (no whitespace or conflict errors) |
| Worktree status | bash / native git | Pre-existing dirty state preserved; only first-party Markdown modified | PASS |

---

## Round 5 Execution — Adversarial Semantic Audit of All 12 Tooie Spec Domains & BK/BT Reuse Boundaries

### 1. Objectives & Approach
- In this fifth actual CLI invocation (Round 5 of authorized 5 to 10 rounds, executed 2026-10-17), an adversarial semantic audit of all 12 Tooie specification domains and BK/BT reuse boundaries was conducted against primary code, generated headers, metadata tables, and primary experimental evidence.
- No code/build/game launches were performed. Existing historical unit/build results were not rerun this task.
- Audited 8 specific critical claims against primary source and evidence ground truth:
  1. **884 Overlay Count**: Audited total overlay slot count versus earlier decompilation estimates (851).
  2. **Guest Logic Hz UNKNOWN**: Audited guest tick cadence, VI rates, and framerate API divisor behavior.
  3. **14-Test Catalogue Reality**: Audited test registry against CMake and historical run logs.
  4. **glibc 2.31 Intended Target Status**: Audited distribution compatibility claims vs P10 packaging state.
  5. **EEPROM 16K Capacity/API vs In-Game Save QA**: Audited Joybus protocol emulation vs actual save file QA.
  6. **Modding Inherited Infrastructure vs Tested Tooie Support**: Audited recomp data/extension APIs vs verified Tooie modding.
  7. **Default Flag Precedence**: Audited `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` unset vs explicit "0"/"" behavior.
  8. **Shader tfPos Driving Fog & Below-Native Pacing / Backpressure**: Audited HLSL compute shader fog math, RT64 below-native pacing logic, and `waitForWorkloadId` backpressure status.

### 2. Adversarial Findings & Corrections Executed

| Domain / Focus Area | Claim / Specification Document | Primary Source / Evidence Ground Truth | Action Taken in Round 5 |
| :--- | :--- | :--- | :--- |
| **Overlays (Domain 2 & BT)** | 851 overlays cited in `docs/specs/index.md` (sections 1, 3, REQ-03) and `docs/specs/BT/index.md:33`. | `src/game/generated/overlay_dispatch.inc.h:13` defines `static constexpr uint32_t kTooieOverlayCount = 884;` (slots 1..884, slot 0 unused; ~851 populated, 33 empty gap slots), generated from `metadata/overlays.table.toml`. | Corrected overlay count to **884 overlay slots** (`kTooieOverlayCount = 884`, ~851 populated) in `docs/specs/index.md` and `docs/specs/BT/index.md`. Updated audit ledger summary tables. |
| **Guest Timing & Cadence (Domain 8)** | Earlier discussions assumed fixed 30/20 Hz guest logic tick. | `docs/plan/evidence/p08-original-timing-4x3-baseline.md` proves `EXPECTED_VI_HZ = 60.000 Hz`, while `EXPECTED_LOGIC_HZ`, `EXPECTED_RENDERED_GAME_FRAMES_HZ`, and `EXPECTED_ANIMATION_UPDATE_HZ` are formally `UNKNOWN`. `recomp_get_target_framerate` returns `60 / frame_divisor` with dynamic, unproven divisor. | Reaffirmed `EXPECTED_LOGIC_HZ: UNKNOWN` and dynamic frame divisor in `docs/specs/Tooie/gameplay-parity.md` and `graphics-and-presentation.md`. |
| **Test Suite Registry (Domain 10)** | `docs/specs/Tooie/testing-and-oracle.md` listed all 14 tests as `PASS (exit 0)` in "Execution Status". | The repository catalogues 14 automated verification targets (12 CMake executables, 1 auxiliary harness `preemption_t15` with `EXCLUDE_FROM_ALL TRUE`, 1 Python script `attract_demo2_transition_jal_test.py`). Cataloguing 14 targets is NOT proof they all ran in any single session. Unit tests were not rerun this task. | Updated `docs/specs/Tooie/testing-and-oracle.md` Section 3: changed column to "Historical Milestone Status", added dates and caveats explicitly stating tests were not rerun this task and whole-suite execution requires explicit invocation. |
| **Packaging & glibc (Domain 11)** | `docs/specs/Tooie/packaging-and-dependencies.md` marked REQ-PKG-04 (`glibc >= 2.31`) as `IMPLEMENTED / VALIDATED`. | Phase P10 is `pending`. No standalone release artifact has been built and tested on a clean glibc 2.31 environment (Ubuntu 20.04 LTS). | Corrected REQ-PKG-04 to `INTENDED_REFERENCE_TARGET / UNVALIDATED` until a standalone artifact is built and verified in P10. |
| **Save System & EEPROM (Domain 7)** | `docs/specs/Tooie/save-system.md` marked REQ-SAV-03..05 as `IMPLEMENTED / VALIDATED`, citing direct Spiral Mountain transition. | `p08-title-file-select-validation.md` tested synthetic frontend input through confirm, warping to `MAP_A1_CS_INTRO (161)` in `mode=3` (opening story cutscenes; prior direct Spiral Mountain claim `CORRECTED/SUPERSEDED`). Joybus 16 Kbit EEPROM protocol (`0x04`/`0x05`) is mediated by ModernRuntime, but comprehensive in-game save QA (in-game save/copy/erase/reboot) is `UNTESTED / PENDING_QA`. | Updated `docs/specs/Tooie/save-system.md` and `docs/specs/index.md` REQ-16: marked in-game save QA as `PENDING_QA` and corrected post-confirm destination. |
| **Modding & Extensions (Domain 12)** | `docs/specs/Tooie/modding-and-extensions.md` and `docs/specs/index.md` (REQ-21) marked modding as `IMPLEMENTED / VALIDATED`. | `src/game/recomp_data_api.cpp`, `recomp_extension_api.cpp`, and `.rtz` registration compile inherited code from BanjoRecomp / RecompModTool. Active Tooie modding, actor extension data on Tooie entities, and `.rtz` texture replacement in Banjo-Tooie have NOT been validated in runtime QA. | Corrected REQ-MOD-01..05 and REQ-21 to `INHERITED_INFRASTRUCTURE / UNTESTED_FOR_TOOIE`. Added Section 4.2 validation boundary in `modding-and-extensions.md`. |
| **Workaround Flag Precedence (Domain 3 & 9)** | Verification of default-ON vs diagnostic rollback flag precedence. | `src/game/tooie_rsp_endpoint_current.cpp:14-38` and `tests/tooie_rsp_endpoint_current_test.cpp:47-85`: UNSET sets `"1"` (`AppliedDefault`). Explicit `"0"`, `""`, `"1"`, or other values are preserved (`PreservedExplicit`). Explicit `"0"` or `""` restores experimental interpolation in RT64. | Reaffirmed and verified flag precedence across `docs/specs/Tooie/graphics-and-presentation.md`, `diagnostics-and-telemetry.md`, and `HANDOFF.md`. |
| **Compute Shader Fog & Below-Native Pacing (Domain 3 & 5)** | Verification of shader fog computation and below-native target handling in RT64. | In `RSPProcessCS.hlsl:86-93`, `fogAlpha` is computed from `((max(tfPos.z, 0) / tfPos.w) * rspFog.mul + rspFog.offset)`. Thus `tfPos` directly drives fog depth as well as vertex positions. In `rt64_workload_queue.cpp:1175`, `targetRate <= viOriginalRate` renders 1 display frame per workload without decimation or sleep (does not implement a cap). `WorkloadQueue::waitForWorkloadId` backpressure across threads is `UNVERIFIED`. | Documented shader fog dependency on `tfPos` and below-native pacing / unverified backpressure constraints in `docs/specs/Tooie/graphics-and-presentation.md`. |

### 3. Canonical Technical Reality Reaffirmed
- **P09 Phase 1 Workaround**: Default-ON CURRENT `tfPos` bypass (`apply_default_before_renderer_init()`) is the current running default; explicit `"0"` and `""` restore experimental interpolation.
- **Permanent Interpolation Fix**: `NOT_IMPLEMENTED`.
- **Exact FPS Presets**: {20, 30, 45, 60, 70, 100, 120, 144} `PLANNED`, `NOT_IMPLEMENTED` (blocked by below-native presentation pacing in RT64 `WorkloadQueue`).
- **Audio Subsystem**: "Tic tic" popping artifact is explicitly `DEFERRED` per user direction; no audio changes.
- **Formal P09 Phase**: `PENDING / NOT_FORMALLY_OPEN`.
- **Historical Testing**: Existing unit/build test results from prior checkpoints were not rerun this task.

### 4. Round 5 Execution Checks & Exit Codes

| Command / Check | Tool / Runner | Result / Exit Code | Status |
| :--- | :--- | :--- | :--- |
| `markdown_vault_audit` | markdown-vault MCP | 0 errors, 15 diagnostic warnings (large files), 44 lint warnings (historical markdown) | NON_BLOCKING_WARNINGS (0 errors, 0 missing frontmatter, 59 total pre-existing warnings) |
| `markdown_vault_diagnose_docs` | markdown-vault MCP | 0 missing frontmatter, 15 large files | NON_BLOCKING_WARNINGS (strictly the 15 protected historical/operational files) |
| `git diff --check` | bash / native git | Exit code 0 | PASS (no whitespace or conflict errors) |
| Worktree status | bash / native git | Pre-existing dirty state preserved; only first-party Markdown modified | PASS |

---

## Round 6 Execution — Fresh Context Final: Outside-Docs First-Party Markdown Audit & Final Consistency Reconciliation

### 1. Objectives & Approach
- In this sixth actual CLI invocation (Round 6 Fresh Context Final, executed 2026-10-17), an independent audit of first-party Markdown outside `docs/` was conducted alongside a whole-project consistency reconciliation.
- Exclusions explicitly enforced:
  - `graft/`: Generated context graph (ignored in `.gitignore`, refreshed deterministically via `graft build`).
  - Sibling checkouts / vendor dependencies: `../BanjoRecomp`, `../ROM`, `../banjo-tooie-Recomp`.
  - Ignored builds/caches: `build/`, `build-recomp/`, `RecompiledFuncs/`, `.serena/cache/`, `.opencode/node_modules/`, `tools/__pycache__/`, `tools/oracle/angrylion-control/work/`.
- Audited all first-party Markdown files outside the vault:
  - Root `AGENTS.md`: Verified agent read order, workspace root, source-of-truth hierarchy, verified culling production defaults (`func_800CAA24` and `func_800CACEC` ON), overlay facts, and graft context guidelines.
  - Root `GEMINI.md`: Verified graft query instructions and syntax.
  - `tools/oracle/README.md`: Verified launcher scripts (`run-tooie.sh`, `run-tooie-angrylion-rdp.sh`, `angrylion-control/run.sh`), separation of `ANGRYLION_CONTROL` (Mupen64Plus + Angrylion RDP Plus + CXD4 LLE) as reference oracle vs RT64 as game renderer, and link to `docs/specs/BT/angrylion-control-oracle.md`.
  - `metadata/coverage_report.md`: Verified P03 recompiler coverage report documenting 854 executable ELF sections, 3,310,496 executable bytes, relocations (`R_MIPS_26`, `HI16`, `LO16`), and exact overlay counts: 884 overlay table slots (33 empty gap slots, 851 named overlays).
  - `metadata/rsp_tasks.inventory.md`: Verified P07 RSP and OSTask scheduler inventory documenting `M_GFXTASK` (F3DEX2 NoN/L3DEX2 via RT64 HLE), `M_AUDTASK` (`n_aspMain` recompiled microcode), static RDRAM templates (`D_8003F3B0` audio, `D_8003F3F0` graphics), ROM blobs, and SDL audio host sink contracts.
  - First-party agent skill guides (`.claude/skills/graft/SKILL.md`, `.grok/skills/graft/SKILL.md`): Verified graft command usage.
  - Verified absence of root `README.md` and `.github/` folder.
- Adversarially rechecked specs, requirements matrix, and current handoff for technical contradictions:
  - Verified consistent categorization across all 12 Tooie specs and `docs/specs/index.md`.
  - Reaffirmed that `apply_default_before_renderer_init()` default CURRENT `tfPos` bypass is the active workaround in Tooie integration; explicit `"0"` or `""` restores experimental interpolation in RT64.
  - Reaffirmed that continuous position interpolation is `NOT_IMPLEMENTED`.
  - Reaffirmed that exact FPS presets {20, 30, 45, 60, 70, 100, 120, 144} are `PLANNED` and `NOT_IMPLEMENTED` (blocked by below-native pacing in RT64 `WorkloadQueue`).
  - Reaffirmed that audio glitch is `DEFERRED` per user direction.
  - Reaffirmed that formal P09 phase status is `PENDING / NOT_FORMALLY_OPEN`.
  - Preserved Round 3 correction of 851 overlays to 884 overlay slots (`kTooieOverlayCount = 884`, ~851 populated, 33 empty gap slots) with historical citations preserved only as explicitly corrected history.
- Synchronized `docs/HANDOFF.md` with completion of docs audit while preserving all gameplay state and the singular `NEXT_SINGLE_ACTION`.
- Conducted full final `markdown_vault_audit` and `git diff --check`, recording exact results without inventing passes or claiming unobserved runtime behavior.

### 2. Outside-Docs Audit Findings & Cross-Checks

| File Path | Nature & Classification | Audited Contracts & Findings | Status / Alignment |
| :--- | :--- | :--- | :--- |
| `AGENTS.md` | Tracked Root Rule | Root instructions; required read order; workspace boundaries (`TooieRecomp/`); strict distinction between evidence and canonical RE; verified production defaults for `800CAA24` (H-plane aspect culling) and `800CACEC` (object frustum culling); graft context guidelines. | PASS / ALIGNED |
| `GEMINI.md` | Untracked First-Party Rule | Direct graft CLI command reference (`graft ask`, `graft grep`, `graft skeleton`, `graft callers`, `graft map`). | PASS / ALIGNED |
| `tools/oracle/README.md` | Tracked Guide | Defines `ANGRYLION_CONTROL` (Mupen64Plus + Angrylion RDP Plus + CXD4 LLE) as offline measurement oracle; explicitly states oracle is NOT the game renderer (TooieRecomp stays on RT64); documents launchers `run-tooie.sh`, `run-tooie-angrylion-rdp.sh`, `angrylion-control/run.sh`; links `docs/specs/BT/angrylion-control-oracle.md`. | PASS / ALIGNED |
| `metadata/coverage_report.md` | Tracked Metadata Report | P03 coverage report generated by `tools/generate_metadata.py`; classifies 854 executable ELF sections, 3,310,496 executable bytes; explicitly documents 884 overlay table slots (33 empty slots, 851 named overlays, 4232 entrypoints) and 4234 syscalls. Perfectly aligns with Round 3 correction (`kTooieOverlayCount = 884`). | PASS / ALIGNED |
| `metadata/rsp_tasks.inventory.md` | Tracked Metadata Report | P07 OSTask inventory from `src/core1/1E2B840.c`; documents `M_GFXTASK` (F3DEX2 NoN/L3DEX2 via RT64 HLE) and `M_AUDTASK` (`n_aspMain` via recompiled C++); static templates `D_8003F3B0` and `D_8003F3F0`; ROM offsets (`0x1E4F3B0`..`0x1E5A280`); SDL audio host sink contracts; SI+CIC-6105 requirement. | PASS / ALIGNED |
| `.claude/skills/graft/SKILL.md` | Untracked Agent Guide | Skill guide for Claude agents querying graft context graph. | PASS / ALIGNED |
| `.grok/skills/graft/SKILL.md` | Untracked Agent Guide | Skill guide for Grok agents querying graft context graph. | PASS / ALIGNED |
| Root `README.md` / `.github/` | Non-Existent | Verified that neither a root `README.md` nor a `.github/` directory exists. All vault documentation lives in `docs/` and root rules live in `AGENTS.md` / `GEMINI.md`. | VERIFIED_ABSENT |

### 3. Scope Distinction: Structural Inventory vs Targeted Semantic Source Verification
- **Whole-Tree Structural Inventory**: Spanned all 293 first-party Markdown files (286 inside `docs/`, 7 outside). Structural checks validated file counts, YAML frontmatter syntax, wikilink targets, large file thresholds, and vault diagnostic metrics.
- **Targeted Semantic Source Verification**: Deeply verified specific subsystem contracts against C++ source (`tooie_rsp_endpoint_current.cpp`, `main.cpp`, `boot_hw_stubs.cpp`, `recomp_api.cpp`), generated headers (`overlay_dispatch.inc.h`), compute shaders (`RSPProcessCS.hlsl`), RT64 workload queues (`rt64_workload_queue.cpp`), and primary evidence logs.
- **Epistemic Clarity**: No claim of exhaustive line-by-line semantic review of all 286 vault files or 198 evidence notes is made. Historical evidence notes remain as preserved provenance logs. No universal QA proof is claimed.

### 4. Canonical Technical Reality Reaffirmed
- **P09 Phase 1 Workaround**: Default-ON CURRENT `tfPos` bypass (`apply_default_before_renderer_init()`) is the current running default; explicit `"0"` and `""` restore experimental interpolation.
- **Permanent Interpolation Fix**: `NOT_IMPLEMENTED`.
- **Exact FPS Presets**: {20, 30, 45, 60, 70, 100, 120, 144} `PLANNED`, `NOT_IMPLEMENTED` (blocked by below-native presentation pacing in RT64 `WorkloadQueue`).
- **Audio Subsystem**: "Tic tic" popping artifact is explicitly `DEFERRED` per user direction; no audio changes.
- **Formal P09 Phase**: `PENDING / NOT_FORMALLY_OPEN`.
- **Overlay Count**: 884 overlay slots (`kTooieOverlayCount = 884`, ~851 populated, 33 empty gap slots); Round 3 correction preserved.
- **Guest Logic Cadence**: `EXPECTED_LOGIC_HZ` and `EXPECTED_ANIMATION_UPDATE_HZ` are strictly `UNKNOWN`.
- **Historical Testing**: Existing unit/build test results from prior checkpoints were not rerun this task.

### 5. Round 6 Execution Checks & Exact Tool Results

| Command / Check | Tool / Runner | Result / Exit Code | Status |
| :--- | :--- | :--- | :--- |
| `markdown_vault_audit` | markdown-vault MCP | 0 errors, 15 diagnostic warnings (large files), 44 lint warnings (historical markdown) | NON_BLOCKING_WARNINGS (0 errors, 0 missing frontmatter, 59 total warnings: 58 pre-existing + 1 ledger) |
| `markdown_vault_diagnose_docs` | markdown-vault MCP | 0 missing frontmatter, 15 large files | NON_BLOCKING_WARNINGS (strictly the 15 protected historical/operational files) |
| `git diff --check` | bash / native git | Exit code 0 | PASS (no whitespace or conflict errors) |
| Worktree status | bash / native git | Pre-existing dirty state preserved; only first-party Markdown modified | PASS |

---

## Round 7 Execution — Targeted Residual Correction & Census Reconciliation

### 1. Objectives & Approach
- In this seventh actual CLI invocation (Round 7 Targeted Final Correction, executed 2026-10-17), an independent root gate finding of a factual error in `docs/specs/Tooie/save-system.md` was investigated, verified against primary source code via jCodeMunch, and corrected.
- Concurrently, whole-project documentation census numbers, warning counts, and generated document classifications were reconciled to ensure total accuracy across the entire specification suite.

### 2. Primary Source Verification & Technical Corrections
1. **EEPROM Probe Return Value vs Joybus Raw Status**:
   - Primary source `N64ModernRuntime/librecomp/src/eep.cpp:11-24` defines `osEepromProbe_recomp`:
     ```cpp
     void osEepromProbe_recomp(uint8_t* rdram, recomp_context* ctx) {
         switch (recomp::get_save_type()) {
             case recomp::SaveType::AllowAll:
             case recomp::SaveType::Eep16k:
                 ctx->r2 = 0x02; // EEPROM_TYPE_16K
                 break;
             case recomp::SaveType::Eep4k:
                 ctx->r2 = 0x01; // EEPROM_TYPE_4K
                 break;
             default:
                 ctx->r2 = 0x00;
                 break;
         }
     }
     ```
   - In MIPS ABI, `ctx->r2` corresponds to register `$v0` (return value of `osEepromProbe`).
   - Prior text in `docs/specs/Tooie/save-system.md:52` incorrectly claimed: "16 Kbit EEPROM reports device type `0x0080` upon probe (`osEepromProbe`)".
   - **Correction**: Differentiated the Ultra64 OS SDK return code (`0x02` / `EEPROM_TYPE_16K`) from the raw Joybus/PIF serial bus device status word (`0x0080` was historically a Joybus raw device status bit, not the SDK function return code). TooieRecomp replaces the Ultra64 SDK function directly and does not guess or emulate raw Joybus protocol status values for EEPROM.
   - Verified that guest JOY thread (`func_800166DC`, registered at `src/game/boot_hw_stubs.cpp:656-659`) explicitly tests `if (osEepromProbe < EEPROM_TYPE_16K)` to set `errno = 8`; returning `0x02` satisfies this check and allows pad polling to proceed.

2. **Save Slot Navigation Acceptance Criteria vs Required Behavior (REQ-SAV-03)**:
   - Prior text in `REQ-SAV-03` acceptance criteria asserted "copy/erase verified" despite the requirement being marked `PARTIAL / PENDING_QA`.
   - **Correction**: Reconciled the acceptance criteria text to describe intended system behavior ("File selection menu navigates and confirms slots; copy and erase operations duplicate and clear slot payloads without memory corruption") while maintaining honest status classification (`PARTIAL / PENDING_QA`).
   - Reaffirmed that while frontend slot navigation and confirmation to opening cutscene `MAP_A1_CS_INTRO` were validated in `p08-title-file-select-validation.md`, copy, erase, in-game save writes, and power-cycle reboot persistence across all 4 save slots are unverified and pending QA.

3. **EEPROM Emulation Provenance & Scope Audit (REQ-SAV-01, REQ-SAV-02, REQ-SAV-05)**:
   - **REQ-SAV-01**: Updated status to `IMPLEMENTED / BOOT_VALIDATED`. Documented actual provenance: 2,048-byte buffer allocation in `librecomp/src/pi.cpp:get_save_size(SaveType::Eep16k) == 0x800` and boot probe initialization in `func_800166DC`; in-game gameplay save write cycle remains tracked for P09/P10.
   - **REQ-SAV-02**: Clarified as SDK function interception rather than raw PIF Joybus command decoding.
   - **REQ-SAV-05 & Atomic Persistence Scope**: Explicitly characterized atomic persistence (`recomp::open_output_file_with_backup` and `finalize_output_file_with_backup` writing `.tmp` followed by atomic rename) as a **host crash-safety and torn-write prevention mechanism**. It protects against incomplete file truncation on process crash or sudden power loss, but is not a universal prevention against in-game state corruption or bad guest game logic.
   - **Layer Ownership**: Clarified that TooieRecomp registers SDK-level jump hooks in `src/game/boot_hw_stubs.cpp:656-668` (`0x80030170`, `0x80032B10`, `0x80032D60`, `0x8002DE70`, `0x8002DF00`). Raw SI DMA (`tooie___osSiRawStartDma` at `0x80031A60`) is retained strictly for CIC-6105 PIF challenge/response traffic, not EEPROM.

4. **Census Reconciliation (Filesystem vs Domain Specifications)**:
   - Reconciled exact filesystem markdown file counts in `docs/specs/`:
     - `docs/specs/Tooie/`: 13 files (12 functional domain specifications + 1 hub/index: `index.md`).
     - `docs/specs/BK/`: 14 files (13 reference topic specifications + 1 hub/index: `index.md`).
     - `docs/specs/BT/`: 17 files (16 decompilation insumos specifications + 1 hub/index: `index.md`).
     - Master Hub: 1 file (`docs/specs/index.md`).
     - Total: **45 `.md` files** in `docs/specs/` (41 domain/topic specifications + 4 index/hub documents).
   - Reconciled evidence file count: 198 files (197 in `docs/plan/evidence/` + 1 in `docs/plan/evidence/p08-visual-validation/README.md`).
   - Total in-vault markdown count: 5 root + 3 master plan + 12 stage plans + 45 specs + 22 reverse engineering + 1 roadmap + 198 evidence = **286 files**.

5. **Outside-Vault Documentation Classification**:
   - Total 7 first-party files outside `docs/` (`AGENTS.md`, `GEMINI.md`, `tools/oracle/README.md`, `metadata/coverage_report.md`, `metadata/rsp_tasks.inventory.md`, `.claude/skills/graft/SKILL.md`, `.grok/skills/graft/SKILL.md`).
   - Explicitly classified `metadata/coverage_report.md` and `metadata/rsp_tasks.inventory.md` as **generated read-only documentation reports** that must not be hand-edited, upholding the project rule on generated code/metadata preservation.

6. **Warning Count Origin Reconciliation**:
   - `markdown_vault_audit` confirmed 59 non-blocking warnings (15 large files + 44 historical markdown lint issues).
   - Reconciled warning provenance: 14 large files and 44 lint issues were pre-existing historical artifacts; 1 large file warning (`plan/evidence/documentation-spec-audit-20261016.md`, 55k+ chars) was newly introduced by the thorough multi-round audit ledger itself. Total warnings are accurately reported as 58 pre-existing + 1 ledger-introduced.

### 3. Round 7 Execution Checks & Exact Tool Results

| Command / Check | Tool / Runner | Result / Exit Code | Status |
| :--- | :--- | :--- | :--- |
| `markdown_vault_audit` | markdown-vault MCP | 0 errors, 15 diagnostic warnings (large files), 44 lint warnings (historical markdown) | NON_BLOCKING_WARNINGS (0 errors, 0 missing frontmatter, 58 pre-existing + 1 ledger-introduced warning) |
| `markdown_vault_diagnose_docs` | markdown-vault MCP | 0 missing frontmatter, 15 large files | NON_BLOCKING_WARNINGS (14 pre-existing historical/operational notes + 1 audit ledger) |
| `git diff --check` | bash / native git | Exit code 0 | PASS (no whitespace or conflict errors) |
| Worktree status | bash / native git | Pre-existing dirty state preserved; only first-party Markdown modified | PASS |

### 4. Singular Next Technical Action
`NEXT_SINGLE_ACTION`: singular focused audit/measurement of native cadence and present-queue backpressure to choose a below-native presentation cap without clock changes; then implement presets with actual tests all 8 and timing QA.
