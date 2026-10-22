---
title: "TooieRecomp Native Port Specifications — Index"
description: "Hub and architectural index for the technical specifications of TooieRecomp, the native static recompilation of Banjo-Tooie."
status: current
area: "specs/Tooie"
updated: 2026-10-16
tags: [specs, tooierecomp, index, architecture]
---

# TooieRecomp Native Port Specifications — Index

## Architectural Overview

`TooieRecomp` is a native Linux x86-64 port of *Banjo-Tooie* (NTSC-U Rev. 0) generated through static binary recompilation. The port executes entirely natively without hardware emulation, using:
- **N64Recomp / RSPRecomp**: Static binary translators converting MIPS R4300i and RSP microcode to native C/C++.
- **N64ModernRuntime / ultramodern**: High-performance guest runtime providing memory management, OSThread scheduling, message queues, and PI/SI/VI hardware virtualization.
- **RT64**: Modern graphics renderer executing N64 display lists via Vulkan / Direct3D 12 with support for high framerates and widescreen aspect ratios.
- **RecompUI / SDL2**: Window management, user interface, controller configuration, and audio streaming.

```text
+-------------------------------------------------------+
|                 Banjo-Tooie NTSC-U ROM                |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|  banjo-tooie-Recomp (Decompilation & Metadata Tools)  |
|  - Matching ELF, Splat Segments, 884 Overlays         |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|               N64Recomp & RSPRecomp                   |
|  - C/C++ Recompiled Code (RecompiledFuncs)           |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|                 TooieRecomp Project                   |
|  - Native Patches, Syscall Dispatch, HW Stubs         |
|  - ModernRuntime + RT64 + SDL2 + RecompUI             |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|        TooieRecompiled (Native Linux Executable)       |
+-------------------------------------------------------+
```

---

## Specification Domain Suite

The TooieRecomp system is specified across 13 core functional domains:

1. **[Build System & Recompiler Generator](build-and-generator.md)**: CMake configuration, N64Recomp/RSPRecomp pipelines, metadata manifests, vendored standalone dependency closure (`vendor/`, `tools/vendor_verify.sh`), and symbols.
2. **[Runtime, Overlays & Guest Preemption](runtime-and-overlays.md)**: ModernRuntime integration, 884 dynamic overlays, syscall table dispatch, and Hybrid F' guest preemption.
3. **[Graphics, Presentation & Interpolation](graphics-and-presentation.md)**: RT64 renderer, aspect-aware culling (800CAA24/800CACEC), cinematic wall handling, shader-local CURRENT tfPos workaround, below-native presentation decimation, and top-left present FPS counter.
4. **[Input & Controller Subsystem](input-and-controllers.md)**: SDL2 input mapping, stick-angle thunk override (80013B70), input helper contracts (800979D8), and attract playback.
5. **[Windowing, UI & Configuration](window-ui-and-config.md)**: SDL2 window lifecycle, graphics driver failure fallback, and RecompUI configuration integration.
6. **[Audio Subsystem & RSP Microcode](audio-system.md)**: n_aspMain ASP microcode execution, M_AUDTASK scheduling, SoundManager delta queues, and deferred audio glitch status.
7. **[Save System & Persistence](save-system.md)**: EEPROM emulation, save slot management, and filesystem persistence.
8. **[Gameplay Parity & Verification](gameplay-parity.md)**: Parity contracts, attract sequence verification, camera mechanics, and physics invariants.
9. **[Diagnostics & Telemetry](diagnostics-and-telemetry.md)**: Default-OFF runtime telemetry, dry-run collectors, and diagnostic safety constraints.
10. **[Testing Framework & Measurement Oracle](testing-and-oracle.md)**: Unit test suite and ANGRYLION_CONTROL software-rendered ground truth oracle.
11. **[Packaging & Dependency Boundaries](packaging-and-dependencies.md)**: Linux standalone packaging, dependency boundaries, candidate verification, and clean-room asset extraction.
12. **[Modding, Extensions & Asset Customization](modding-and-extensions.md)**: Extended object data, slot maps, texture pack container formats (.rtz), and mod callback hooks.
13. **[Asset Ownership & Licensing Manifest](asset-and-licensing-manifest.md)**: Copyright boundaries, legal ROM requirements, ROM-derived code classification, third-party component licenses, and package distribution policies.

---

## Relation to Sister Specifications

- **[[../BK/index|BK Specifications]]**: Banjo-Kazooie reference port contracts (`BanjoRecomp`). Defines reusable runtime architecture, UI, and controller abstractions.
- **[[../BT/index|BT Specifications]]**: Banjo-Tooie decompilation contracts (`banjo-tooie-Recomp`). Defines upstream data inputs, Splat layout, and symbol addresses.
- **[[../../plan/index|Master Implementation Plan]]**: Master roadmap detailing project gates M0 through M6.
- **[[../../plan/evidence/documentation-spec-audit-20261016|Audit Ledger]]**: Multi-round documentation and specification audit ledger.
