---
title: "Specifications Index and Master Requirements Matrix"
description: "Master technical specifications index and requirements verification matrix for the TooieRecomp project."
status: current
area: "specs"
updated: 2026-10-17
tags:
  - docs
  - index
  - specifications
  - matrix
  - requirements
---

# Specifications Index and Master Requirements Matrix

This document is the central hub for all technical specifications across the **TooieRecomp** project. It integrates:
1. **[TooieRecomp Specifications](Tooie/index.md)**: Native port implementation specifications.
2. **[Banjo: Recompiled Specifications (BK)](BK/index.md)**: Reference static recompilation architecture.
3. **[Banjo-Tooie Decompilation Specifications (BT)](BT/index.md)**: Decompilation inputs, Splat layout, and symbol tables.
4. **Master Requirements & Verification Matrix**: Complete status mapping across all functional domains.

---

## 1. TooieRecomp Native Port Specifications (`specs/Tooie/`)

Comprehensive technical specifications governing the native static recompilation of *Banjo-Tooie*:

- **[Tooie Specifications Hub](Tooie/index.md)** — Architectural index and component relationship overview.
- **[Build System & Recompiler Generator](Tooie/build-and-generator.md)** — CMake build pipeline, N64Recomp/RSPRecomp code generation, and metadata manifests.
- **[Runtime, Dynamic Overlays & Guest Preemption](Tooie/runtime-and-overlays.md)** — ModernRuntime integration, 884 dynamic overlay relocations (kTooieOverlayCount = 884, ~851 populated), syscall dispatch, and Hybrid F' guest preemption.
- **[Graphics, Presentation & High-Framerate Interpolation](Tooie/graphics-and-presentation.md)** — RT64 rendering, aspect-aware culling (800CAA24/800CACEC), shader-local CURRENT tfPos workaround, below-native presentation decimation, and top-left present FPS counter.
- **[Input & Controller Subsystem](Tooie/input-and-controllers.md)** — SDL2 controller mapping, stick-angle thunk override (80013B70), input helper contracts (800979D8), and attract playback.
- **[Windowing, UI & Configuration](Tooie/window-ui-and-config.md)** — SDL2 window lifecycle, Vulkan graphics driver failure fallback, and RecompUI configuration integration.
- **[Audio Subsystem & RSP Microcode](Tooie/audio-system.md)** — `n_aspMain` microcode execution, `M_AUDTASK` dispatch, SoundManager delta-queues, and deferred audio glitch status.
- **[Save System & Data Persistence](Tooie/save-system.md)** — 16 Kbit EEPROM emulation, save slot management, and filesystem persistence.
- **[Gameplay Parity & Verification Criteria](Tooie/gameplay-parity.md)** — Parity contracts, attract sequence verification, camera mechanics, and physics invariants.
- **[Diagnostics & Runtime Telemetry](Tooie/diagnostics-and-telemetry.md)** — `tooie_diag` framework, environment control flags, telemetry safety, and diagnostic launch rules.
- **[Testing Framework & Measurement Oracle](Tooie/testing-and-oracle.md)** — Automated host unit test suite and ANGRYLION_CONTROL software-rendered ground truth oracle.
- **[Packaging, Releases & Dependency Boundaries](Tooie/packaging-and-dependencies.md)** — Linux standalone packaging, clean-room asset extraction, and legal ROM requirements.
- **[Modding, Extensions & Asset Customization](Tooie/modding-and-extensions.md)** — Extended object data, slot maps, texture pack container formats (.rtz), and mod callback hooks.

---

## 2. Reference Specifications: Banjo-Kazooie (`specs/BK/`)

The reference static recompilation implementation (`BanjoRecomp`) used as the architectural baseline:

- **[BK Index](BK/index.md)** — Hub of all BK reference specifications.
- **[Overview](BK/overview.md)** — High-level architecture and libraries.
- **[Config](BK/config.md)** — `banjo.us.rev0.toml` recompiler options.
- **[RSP](BK/rsp.md)** — ASP microcode recompilation.
- **[Patches](BK/patches.md)** — Native patch layer and `patches.toml`.
- **[Build](BK/build.md)** — End-to-end build pipeline.
- **[Runtime](BK/runtime.md)** — Runtime launcher and function registration.
- **[UI](BK/ui.md)** — RecompUI launcher and theming.
- **[Input](BK/input.md)** — Controller database and input bindings.
- **[Symbols](BK/symbols.md)** — Symbol maps (`BanjoRecompSyms`).
- **[Features](BK/features.md)** — Framerate, widescreen, and audio features.
- **[Mods](BK/mods.md)** — Extension and modding APIs.
- **[CI](BK/ci.md)** — GitHub Actions release automation.
- **[Contrato de reutilização](BK/reuse-contract.md)** — Boundary between reusable infrastructure and BK-specific code.

---

## 3. Insumos Specifications: Banjo-Tooie Decompilation (`specs/BT/`)

Specifications for the local decompilation fork (`banjo-tooie-Recomp`):

- **[BT Index](BT/index.md)** — Hub of all decompilation specifications.
- **[Overview](BT/overview.md)** — Decompilation status and goals.
- **[Setup](BT/setup.md)** — Baserom requirements and dependencies.
- **[Splat](BT/splat.md)** — Segment layout (`baserom.us.yaml`).
- **[Build](BT/build.md)** — Makefile pipeline to matching ELF.
- **[Symbols](BT/symbols.md)** — Symbol and relocation maps.
- **[Overlays](BT/overlays.md)** — 884 dynamic overlay definitions (kTooieOverlayCount = 884, ~851 populated).
- **[Syscalls](BT/syscalls.md)** — Syscall dispatch table structure.
- **[RSP e áudio](BT/rsp-and-audio.md)** — Microcode extraction and audio tasks.
- **[Input e controles](BT/input.md)** — Pad input structures and attract data.
- **[Compression](BT/compression.md)** — Cartridge/overlay deflate format.
- **[Tools](BT/tools.md)** — C++/Python decompilation tooling.
- **[ANGRYLION_CONTROL oracle](BT/angrylion-control-oracle.md)** — Software-rendered reference oracle.
- **[Sources](BT/sources.md)** — Decompilation source tree layout.
- **[Matching](BT/matching.md)** — Per-function decompilation workflow.
- **[CI](BT/ci.md)** — Upstream CI build status.
- **[Prontidão para recompilação](BT/recomp-readiness.md)** — Readiness evaluation and migration gaps.

---

## 4. Master Requirements & Verification Matrix

This matrix tracks the formal implementation and verification status of all core TooieRecomp requirements:

| ID | Domain | Requirement Description | Spec Link | Status | As-Implemented vs Intended Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **REQ-01** | Build | CMake builds standalone `TooieRecompiled` on Linux x86-64 | [[Tooie/build-and-generator]] | `IMPLEMENTED` / `VALIDATED` | As-implemented matches intended. Clean link exit 0. |
| **REQ-02** | Recomp | N64Recomp generates ~21k C functions without undefined symbols | [[Tooie/build-and-generator]] | `IMPLEMENTED` / `VALIDATED` | As-implemented matches intended. `libRecompiledFuncs.a` links cleanly. |
| **REQ-03** | Overlays | Dynamic loading, heap relocation, and unload across 884 overlay slots | [[Tooie/runtime-and-overlays]] | `IMPLEMENTED` / `VALIDATED` | As-implemented matches intended. `kTooieOverlayCount = 884` (~851 populated). |
| **REQ-04** | Preemption | Hybrid F' guest preemption prevents thread starvation | [[Tooie/runtime-and-overlays]] | `IMPLEMENTED` / `VALIDATED` | PATCH 1–4 verified; cooperative yields in loops and safepoints. |
| **REQ-05** | Widescreen Map | Aspect-aware horizontal plane culling for map geometry (`800CAA24`) | [[Tooie/graphics-and-presentation]] | `IMPLEMENTED` / `VALIDATED` | Production default ON. `TOOIE_TEST_MAP_VIS_ASPECT_AWARE=0` restores original. |
| **REQ-06** | Widescreen Obj | Aspect-aware side-plane culling for dynamic objects (`800CACEC`) | [[Tooie/graphics-and-presentation]] | `IMPLEMENTED` / `VALIDATED` | Production default ON. `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0` restores original. |
| **REQ-07** | Intro Geometry | Widescreen edge geometry in opening cinematic scenes | [[Tooie/graphics-and-presentation]] | `PLANNED` / `NOT_IMPLEMENTED` | SCENE_A/B identified; 16:9 AUTO CLIP_X characterized; ultrawide pillarbox deferred. |
| **REQ-08** | Interpolation Workaround | Default shader-local CURRENT `tfPos` bypass prevents mesh tearing | [[Tooie/graphics-and-presentation]] | `IMPLEMENTED` / `VALIDATED` | Tooie default-ON; 20/20 test PASS; explicit `0` rolls back. RT64 default OFF. |
| **REQ-09** | Permanent Interp | Continuous bone-constrained matrix and vertex interpolation | [[Tooie/graphics-and-presentation]] | `PLANNED` / `NOT_IMPLEMENTED` | Seam edge and topological research deferred; no alias/snap implemented. |
| **REQ-10** | FPS Presets | Exact user-selectable presets {20, 30, 45, 60, 70, 100, 120, 144} | [[Tooie/graphics-and-presentation]] | `PARTIAL` / `PENDING_UI_AND_QA` | Below-native present decimator implemented in RT64; UI presets widget and runtime QA pending. |
| **REQ-11** | Input Stick | Stick angle normalization override (`func_80013B70`) | [[Tooie/input-and-controllers]] | `IMPLEMENTED` / `VALIDATED` | Verified across cardinals/diagonals (VERDICT A). Bidirectional input OK. |
| **REQ-12** | Input Helper | Stateful input evaluation helper contract (`0x800979D8`) | [[Tooie/input-and-controllers]] | `IMPLEMENTED` / `VALIDATED` | Verified against Angrylion; binds `bakey_held` and ring-1 buffer `0x800BCE84`. |
| **REQ-13** | Window & Fallback | Display modes, aspect safety, and Vulkan initialization fallback | [[Tooie/window-ui-and-config]] | `IMPLEMENTED` / `VALIDATED` | 38/38 window tests PASS; graphics failure test PASS; P08-T07/T08 validated. |
| **REQ-14** | Audio Microcode | RSP `n_aspMain` microcode execution and `M_AUDTASK` dispatch | [[Tooie/audio-system]] | `IMPLEMENTED` / `VALIDATED` | Initial music, sound effects, and jingles synthesize cleanly via SDL2. |
| **REQ-15** | Audio Glitch Fix | Eliminate intermittent "tic tic" popping sound in audio stream | [[Tooie/audio-system]] | `PLANNED` / `DEFERRED` | Explicitly deferred per user instruction; no speculative audio edits allowed. |
| **REQ-16** | Saves & EEPROM | 16 Kbit EEPROM emulation and save slot selection | [[Tooie/save-system]] | `PARTIAL` / `PENDING_QA` | Native SDK EEPROM replacement active; frontend slot confirm tested; in-game save/copy/erase QA pending. |
| **REQ-17** | Campaign Parity | Complete campaign gameplay parity from boot to credits | [[Tooie/gameplay-parity]] | `PLANNED` / `NOT_FORMALLY_OPEN` | Demo 0 provisional match; bounded Demo 1 hit accepted; broader sequence Demo 1/Demo 5 and campaign untested/open; P09 pending. |
| **REQ-18** | Test Oracle | Bit-accurate reference measurement via ANGRYLION_CONTROL | [[Tooie/testing-and-oracle]] | `IMPLEMENTED` / `VALIDATED` | Offline reference only; TooieRecomp client renders strictly via RT64. |
| **REQ-19** | Diagnostics | Default-OFF non-intrusive runtime telemetry and safety stops | [[Tooie/diagnostics-and-telemetry]] | `IMPLEMENTED` / `VALIDATED` | Status writes atomic; circuit breakers active; diagnostic launch rules enforced. |
| **REQ-20** | Packaging | Standalone Linux release package with clean-room asset extraction | [[Tooie/packaging-and-dependencies]] | `PLANNED` / `NOT_IMPLEMENTED` | P10 release stage; requires legitimate user-supplied ROM (`NB7E`). |
| **REQ-21** | Modding | Extensible object payloads, slot maps, and .rtz texture pack loading | [[Tooie/modding-and-extensions]] | `INHERITED_INFRASTRUCTURE` / `UNTESTED_FOR_TOOIE` | Compiled from BanjoRecomp architecture; runtime modding and .rtz unvalidated for Tooie. |
| **REQ-22** | FPS Counter | Visible top-left present FPS counter and Graphics settings toggle (`REQ-GFX-08`) | [[Tooie/graphics-and-presentation]] | `IMPLEMENTED` / `UNIT_TESTED` | Option in Graphics tab, default OFF; measures swapChain present success; unit test PASS; runtime QA pending. |

---

## 5. Master Roadmap & Navigation Links

- **[Master Implementation Plan](../../plan/index.md)** — Critical path from M0 (baseline) to M6 (release).
- **[BT Implementation Stages](../../plans/BT/index.md)** — Step-by-step progress tracking for phases 00 through 10.
- **[Central Audit Ledger](../../plan/evidence/documentation-spec-audit-20261016.md)** — Living record of the ongoing documentation audit.
- **[Current Operational Handoff](../../HANDOFF.md)** — Active operational state and handoff context.
