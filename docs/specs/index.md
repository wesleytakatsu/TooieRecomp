---
title: "Specifications Index and Master Requirements Matrix"
description: "Master technical specifications index and requirements verification matrix for the TooieRecomp project."
status: current
area: "specs"
updated: 2026-10-28
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
2. **Master Requirements & Verification Matrix**: Complete status mapping across all functional domains.
3. **[Reverse Engineering Knowledge Base](../reverse-engineering/README.md)**: Current analytical and architectural models of original game subsystems.
4. **[Operational Handoff](../HANDOFF.md)**: Current system state, open items, and next actions.

---

## 1. TooieRecomp Native Port Specifications (`specs/Tooie/`)

Comprehensive technical specifications governing the native static recompilation of *Banjo-Tooie*:

- **[Tooie Specifications Hub](Tooie/index.md)** — Architectural index and component relationship overview.
- **[Build System & Recompiler Generator](Tooie/build-and-generator.md)** — CMake build pipeline, Clang toolchain requirements, N64Recomp/RSPRecomp code generation, and metadata manifests.
- **[Runtime, Dynamic Overlays & Guest Preemption](Tooie/runtime-and-overlays.md)** — ModernRuntime integration, 884 dynamic overlay relocations, syscall dispatch, and Hybrid F' guest preemption.
- **[Graphics, Presentation & High-Framerate Interpolation](Tooie/graphics-and-presentation.md)** — RT64 rendering, native-style 16:9 culling planes, host endpoint-CURRENT fallback, presentation decimation, P09 transform interpolation guards, and top-left present FPS counter.
- **[Input & Controller Subsystem](Tooie/input-and-controllers.md)** — SDL2 controller mapping, local multiplayer for up to 4 players, stick-angle thunk override (80013B70), and input helper contracts (800979D8).
- **[Windowing, UI & Configuration](Tooie/window-ui-and-config.md)** — SDL2 window lifecycle, graphics driver failure fallback, and RecompUI configuration integration.
- **[In-Game Language Packs](Tooie/language-packs.md)** — Game Language option, built-in pt-BR bundle (`.tlpb`), FR/DE/ES import from the player's European ROM (`.tlp`), font remapping, and in-memory ROM rewrite.
- **[Audio Subsystem & RSP Microcode](Tooie/audio-system.md)** — `n_aspMain` microcode execution, `M_AUDTASK` dispatch, SoundManager delta queues, and SDL2 audio stream management.
- **[Save System & Data Persistence](Tooie/save-system.md)** — 16 Kbit EEPROM emulation, save slot management, and filesystem persistence (`<config>/saves/<game_id>.bin`).
- **[Gameplay Parity & Verification Criteria](Tooie/gameplay-parity.md)** — Parity contracts, attract sequence verification, camera mechanics, and physics invariants.
- **[Diagnostics & Runtime Telemetry](Tooie/diagnostics-and-telemetry.md)** — `tooie_diag` framework, environment control flags, telemetry safety, and diagnostic launch rules.
- **[Testing Framework & Measurement Oracle](Tooie/testing-and-oracle.md)** — Automated host unit test suites (Linux and Windows) and verification targets.
- **[Angrylion Control Oracle](Tooie/angrylion-control-oracle.md)** — Software-rendered ground truth reference oracle for visual and behavioural comparison.
- **[Packaging, Releases & Dependency Boundaries](Tooie/packaging-and-dependencies.md)** — Standalone Linux (`.tar.gz`) and Windows (`.zip`) release packaging, in-memory ROM decompression, and legal boundaries.
- **[Modding, Extensions & Asset Customization](Tooie/modding-and-extensions.md)** — Extended object data, slot maps, texture pack container formats (.rtz), and mod callback hooks.
- **[Asset Ownership & Licensing Manifest](Tooie/asset-and-licensing-manifest.md)** — Copyright boundaries, legal ROM requirements, ROM-derived code classification, third-party component licenses, and package distribution policies.

---

## 2. Master Requirements & Verification Matrix

This matrix tracks the implementation and verification status of all core TooieRecomp requirements:

| ID | Domain | Requirement Description | Spec Link | Status | As-Implemented Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **REQ-01** | Build | Standalone native builds on Linux x86-64 and Windows x64 | [[Tooie/build-and-generator]] | `IMPLEMENTED` | Standard CMake build via `clang` (Linux) and `clang-cl` + `lld-link` (Windows). GCC is rejected. |
| **REQ-02** | Recomp | N64Recomp generates C functions without undefined symbols | [[Tooie/build-and-generator]] | `IMPLEMENTED` | Functions compiled into `RecompiledFuncs` with overrides supplied via strong native symbols. |
| **REQ-03** | Overlays | Dynamic loading, heap relocation, and unload across 884 overlay slots | [[Tooie/runtime-and-overlays]] | `IMPLEMENTED` | Loads from ROM table (`kOvlTableRom` at `0x01F00020`), relocates via host-safe loader, and caches resident overlays. |
| **REQ-04** | Preemption | Hybrid F' guest preemption prevents thread starvation | [[Tooie/runtime-and-overlays]] | `IMPLEMENTED` | Backedge preemption hints emitted by generator; controlled by `ULTRAMODERN_GUEST_PREEMPTION` (default ON). |
| **REQ-05** | Widescreen Map | Aspect-aware horizontal plane culling for map geometry (`800CAA24`) | [[Tooie/graphics-and-presentation]] | `IMPLEMENTED` | Production uses native-style planes (`TOOIE_TEST_WS_NATIVE_PLANES`, default ON). Test-expansion fallback active in attract demos or when disabled. |
| **REQ-06** | Widescreen Obj | Aspect-aware side-plane culling for dynamic objects (`800CACEC`) | [[Tooie/graphics-and-presentation]] | `IMPLEMENTED` | Production uses native-style planes (`TOOIE_TEST_WS_NATIVE_PLANES`, default ON). Allowlisted draw-scope expansion active in attract demos. |
| **REQ-07** | Intro Geometry | Widescreen edge geometry in opening cinematic scenes | [[Tooie/graphics-and-presentation]] | `PARTIAL` | 16:9 projection compensation implemented; edge geometry and ultrawide adjustments remain open. |
| **REQ-08** | Interpolation Workaround | Host endpoint-CURRENT fallback prevents mesh tearing | [[Tooie/graphics-and-presentation]] | `IMPLEMENTED` | Host requests endpoint-CURRENT while Interpolation is OFF (`RT64::requestHostEndpointCurrent(!interpolation)`). |
| **REQ-09** | Transform Interp | High-framerate transform interpolation with P09 guards | [[Tooie/graphics-and-presentation]] | `IMPLEMENTED` | Hash-ordered pairing, gap alignment, translation coherence gate, camera cut, segment/joint coherence, and spike guards active when interpolation enabled. |
| **REQ-10** | Framerate Control | User framerate unlocking and present rate control | [[Tooie/graphics-and-presentation]] | `PARTIAL` | Unlocked to display/configured rate (min 60) when Interpolation is ON; locked to Manual 30 when Interpolation is OFF. Discrete preset selector is not implemented. |
| **REQ-11** | Input Stick | Stick angle normalization override (`func_80013B70`) | [[Tooie/input-and-controllers]] | `IMPLEMENTED` | Native strong symbol override in `boot_hw_stubs.cpp` handles empty stub generator omission. |
| **REQ-12** | Input Helper | Stateful input evaluation helper contract (`0x800979D8`) | [[Tooie/input-and-controllers]] | `IMPLEMENTED` | Intercept helper binds `bakey_held` and ring buffer `0x800BCE84`. |
| **REQ-13** | Window & Fallback | Display modes, aspect safety, and Vulkan/D3D12 fallback | [[Tooie/window-ui-and-config]] | `IMPLEMENTED` | Window lifecycle handled via SDL2; graphics initialization handles failure gracefully. |
| **REQ-14** | Audio Microcode | RSP `n_aspMain` microcode execution and `M_AUDTASK` dispatch | [[Tooie/audio-system]] | `IMPLEMENTED` | `M_AUDTASK` intercepted in `src/main/main.cpp`; samples queued via SDL2 audio output stream at 48 kHz. |
| **REQ-15** | Audio Popping | Eliminate intermittent popping sound in audio stream | [[Tooie/audio-system]] | `PARTIAL` | Stream buffering and decimation safeguards active; extended soak validation remains open. |
| **REQ-16** | Saves & EEPROM | 16 Kbit EEPROM emulation and save slot persistence | [[Tooie/save-system]] | `IMPLEMENTED` | Native EEPROM emulation saves to `<config_path>/saves/<game_id>.bin` with atomic rename. |
| **REQ-17** | Campaign Parity | Campaign gameplay parity and attract verification | [[Tooie/gameplay-parity]] | `PARTIAL` | Attract demos verified via regression suite; full campaign validation remains ongoing. |
| **REQ-18** | Test Oracle | Reference measurement via ANGRYLION_CONTROL | [[Tooie/testing-and-oracle]] | `IMPLEMENTED` | Standalone Linux reference oracle available for ground-truth rendering verification. |
| **REQ-19** | Diagnostics | Default-OFF non-intrusive runtime telemetry and safety stops | [[Tooie/diagnostics-and-telemetry]] | `IMPLEMENTED` | Heavy diagnostic telemetry defaults to OFF; controlled via environment flags. |
| **REQ-20** | Packaging | Standalone Linux and Windows release packages | [[Tooie/packaging-and-dependencies]] | `IMPLEMENTED` | Packages generated via `package_linux.sh` (`.tar.gz`) and `Build-Tooie.ps1` (`.zip`). Launcher decompresses retail ROM on the fly or accepts pre-decompressed image. |
| **REQ-21** | Modding | Extensible object payloads, slot maps, and .rtz texture pack loading | [[Tooie/modding-and-extensions]] | `PARTIAL` | `.rtz` texture packs validated on Windows; object extension data and runtime toggles inherited and unvalidated for Tooie. |
| **REQ-22** | FPS Counter | Visible top-left present FPS counter toggle in Graphics settings | [[Tooie/graphics-and-presentation]] | `IMPLEMENTED` | Config toggle in Graphics menu; measures swap chain presentation rate. |
| **REQ-23** | Oracle Verification | Automated comparison against software reference | [[Tooie/angrylion-control-oracle]] | `IMPLEMENTED` | Dedicated oracle harness and frame comparison scripts on Linux. |
| **REQ-24** | Local Multiplayer | Up to 4 players, each with controller and/or keyboard and per-player mapping | [[Tooie/input-and-controllers]] | `IMPLEMENTED` | Option "Multiplayer (up to 4 players)" (default Off, applies after restart). User-validated on Windows; Linux `NOT_RUN`. |
| **REQ-25** | Game Languages | English, Português (Brasil), Français, Deutsch, Español selectable in launcher and ESC menu | [[Tooie/language-packs]] | `IMPLEMENTED` | pt-BR ships as `assets/languages/pt-BR.tlpb`; FR/DE/ES are extracted once from the player's European ROM. Applied at ROM load (restart after changing). Validated on Windows; Linux `NOT_RUN`. |

---

## 3. Related Documentation

- **[Tooie Specifications Directory](Tooie/index.md)** — Domain specification index.
- **[Reverse Engineering Knowledge Base](../reverse-engineering/README.md)** — Decompiled structures, function contracts, and system analysis.
- **[Operational Handoff](../HANDOFF.md)** — Project status, open residuals, and active focus areas.