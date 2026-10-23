---
title: "TooieRecomp Native Port Specifications — Index"
description: "Hub and architectural index for the technical specifications of TooieRecomp, the native static recompilation of Banjo-Tooie."
status: current
area: "specs/Tooie"
updated: 2026-10-23
tags: [specs, tooierecomp, index, architecture]
---

# TooieRecomp Native Port Specifications — Index

## Architectural Overview

`TooieRecomp` is a native multiplatform port of *Banjo-Tooie* (NTSC-U Rev. 0) targeting Linux x86-64 and Windows x64 generated through static binary recompilation. The port executes entirely natively without hardware emulation, using:
- **N64Recomp / RSPRecomp**: Static binary translators converting MIPS R4300i and RSP microcode to native C/C++.
- **N64ModernRuntime / ultramodern**: High-performance guest runtime providing memory management, OSThread scheduling, message queues, and PI/SI/VI hardware virtualization.
- **RT64**: Modern graphics renderer executing N64 display lists via Vulkan (Linux) or Direct3D 12 (Windows default) with support for high framerates and widescreen aspect ratios.
- **RecompUI / SDL2**: Window management, user interface, controller configuration, and audio streaming.

```text
+-------------------------------------------------------+
|                 Banjo-Tooie NTSC-U ROM                |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|        decomp/ (Vendored Decompilation Sources)       |
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
|       TooieRecompiled (Native Platform Executable)    |
+-------------------------------------------------------+
```

---

## Specification Domain Suite

The TooieRecomp system is specified across 14 core functional domains:

1. **[Build System & Recompiler Generator](build-and-generator.md)**: CMake configuration, Clang toolchain requirements, N64Recomp/RSPRecomp pipelines, metadata manifests, vendored dependency closure, and symbols.
2. **[Runtime, Overlays & Guest Preemption](runtime-and-overlays.md)**: ModernRuntime integration, 884 dynamic overlays, syscall table dispatch, and Hybrid F' guest preemption.
3. **[Graphics, Presentation & Interpolation](graphics-and-presentation.md)**: RT64 renderer, native-style 16:9 culling planes, host endpoint-CURRENT fallback, presentation decimation, P09 transform interpolation guards, and top-left present FPS counter.
4. **[Input & Controller Subsystem](input-and-controllers.md)**: SDL2 input mapping, stick-angle thunk override (80013B70), input helper contracts (800979D8), and attract playback.
5. **[Windowing, UI & Configuration](window-ui-and-config.md)**: SDL2 window lifecycle, graphics driver failure fallback, and RecompUI configuration integration.
6. **[Audio Subsystem & RSP Microcode](audio-system.md)**: `n_aspMain` microcode execution, `M_AUDTASK` scheduling, SoundManager delta queues, and audio stream stability.
7. **[Save System & Persistence](save-system.md)**: EEPROM emulation, save slot management, and filesystem persistence (`<config>/saves/<game_id>.bin`).
8. **[Gameplay Parity & Verification](gameplay-parity.md)**: Parity contracts, attract sequence verification, camera mechanics, and physics invariants.
9. **[Diagnostics & Telemetry](diagnostics-and-telemetry.md)**: Default-OFF runtime telemetry, environment control flags, and diagnostic safety constraints.
10. **[Testing Framework & Measurement Oracle](testing-and-oracle.md)**: Unit test suite and automated test targets on Linux and Windows.
11. **[Angrylion Control Oracle](angrylion-control-oracle.md)**: Software-rendered reference oracle for bit-accurate visual and behavioral verification.
12. **[Packaging & Dependency Boundaries](packaging-and-dependencies.md)**: Standalone packaging (`.tar.gz` and `.zip`), dependency boundaries, candidate verification, and on-the-fly ROM decompression.
13. **[Modding, Extensions & Asset Customization](modding-and-extensions.md)**: Extended object data, slot maps, texture pack container formats (.rtz), and mod callback hooks.
14. **[Asset Ownership & Licensing Manifest](asset-and-licensing-manifest.md)**: Copyright boundaries, legal ROM requirements, ROM-derived code classification, third-party component licenses, and package distribution policies.

---

## Related Documentation

- **[Reverse Engineering Knowledge Base](../../reverse-engineering/README.md)**: Game mechanics, decompiled data structures, and function contracts.
- **[Operational Handoff](../../HANDOFF.md)**: Active work items, known residuals, and current operational status.