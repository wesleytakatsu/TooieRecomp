---
title: "TooieRecomp Spec — Handheld Build and PortMaster Package"
description: "Compile-time isolation of the handheld build (TK64_MOBILE), its game behavior, the PortMaster package and what was and was not validated."
status: current
area: "specs/Tooie"
updated: 2026-11-05
tags: [specs, tooierecomp, handheld, portmaster, tk64, aarch64, packaging]
---

# Handheld Build and PortMaster Package

## 1. Scope

A build for aarch64 Linux handhelds, distributed as a PortMaster zip. It renders through TK64's OpenGL ES 3.1 backend with the
weak GPU profile. Windows x64 and Linux x64 builds are not part of it and compile the same sources they always did.

Measurements and the reasoning behind each change are in TK64's `docs/specs/mobile-driver-costs.md`. Why the game needs the
Z-buffer in RAM is in [camera controls](../../reverse-engineering/systems/camera-controls.md).

## 2. Isolation from desktop builds

| Rule | How it holds |
| --- | --- |
| The CMake option `TK64_MOBILE` is `OFF` by default | Only the preset `linux-arm64-handheld` turns it on |
| With it off, every changed source resolves to the base commit's text | `python tools/handheld/check_desktop_unchanged.py` compares them byte by byte and fails otherwise. It proves a handheld-only change; a change meant for every target (the Graphics API menu) fails it on purpose, so pass `--base` from before that change only for handheld-only work |
| Desktop OpenGL keeps its behavior where mobile drivers differ | The one `plume_gles.cpp` checks the GL profile (ES) or ARM's driver at run time, not `TK64_MOBILE` |
| Desktop shaders are not changed | `-D TK64_MOBILE` reaches DXC only with the option on |
| Build script changes do not touch a native build | They are behind `TK64_MOBILE` or `CMAKE_CROSSCOMPILING`; the check prints these two files as `reviewed` |

New handheld code goes behind `#ifdef TK64_MOBILE` with no other condition on that line, because the check resolves only that form.

The handheld OpenGL backend and the desktop one are the same file since phase 4: see
[graphics](graphics-and-presentation.md) section 3.5 for what changed on desktop and the measurements. The user tested the
Linux x64 build on 2026-11-06 and reported that it works (a general report, not per API).

## 3. What `TK64_MOBILE` changes

| Area | Behavior | Why |
| --- | --- | --- |
| RSP shader | Viewports and endpoint matrices come from texel buffers | Mobile drivers may offer 8 storage buffers to a compute shader |
| Raster vertex shader | Takes rect mode and fixed depth from push constants | Mobile drivers may offer no storage buffer to the vertex stage |
| Framebuffer changes | Boolean texture is `R32_UINT` | GLES 3.1 has no `R8UI` image storage |
| Weak profile | Render to RAM off; resolution and antialiasing stay the player's; specialized shaders, not only the ubershader | Each of these is measured in TK64's spec |
| Depth to RAM | The newest depth the GPU finished reaches RDRAM, a frame or more late, and only while that address is still the game's Z image. The GPU is never waited for | The game's camera reads the Z-buffer from RAM |
| Interface | No MSAA target for the interface in the weak profile | It is cleared, drawn and resolved every frame |
| Window | `TOOIE_HANDHELD=1`: fullscreen at the display's size, no `SDL_WINDOW_VULKAN` with `TK64_GRAPHICS_API=opengl*` | Handhelds may have no Vulkan loader |
| Quit | `TOOIE_HANDHELD=1`: Select + Start opens the quit prompt, or quits outside the game | There is no window to close |
| Backend selection | The menu lists Auto and OpenGL ES; startup sets `TK64_GRAPHICS_API=opengles`. A value from another platform in `graphics.json` becomes Auto. A `TK64_GRAPHICS_API` already set wins | Handhelds need OpenGL ES |
| ROM | With `TOOIE_AUTO_START`, an invalid stored ROM makes the game try each `.z64`, `.n64` and `.v64` in its folder through `recomp::select_rom`, and quit if none is accepted | Handhelds have no file dialog |

Switches, all optional: `TK64_DEPTH_TO_RAM=0|1`, `TK64_RENDER_TO_RAM=0|1`, `TK64_UBERSHADERS_ONLY=0|1`. The frame profiler used for the
measurements is not in the tree; it is kept outside the repository with the measurement scripts.

## 4. Build

```powershell
pwsh -NoProfile -File tools/handheld/build-handheld.ps1 -Sysroot <dir> -Configure    # Windows host
python tools/handheld/package_portmaster.py
```

```bash
HANDHELD_SYSROOT=<dir> tools/handheld/build-handheld.sh --configure                   # Linux host
python3 tools/handheld/package_portmaster.py
```

Both scripts configure the CMake preset `linux-arm64-handheld` into `build-handheld`, with the toolchain
`tools/handheld/aarch64-linux.cmake`. They take clang and lld from Visual Studio on Windows and from `PATH` (or `HANDHELD_LLVM`) on Linux, the
sysroot from `tools/handheld/fetch_sysroot.py` and the host's `file_to_c` from the desktop build. No WSL.
`-Device`/`--device` picks a file in `tools/handheld/devices`; `rk3326` (the default) is the only one. It covers the XU10, the R36S and other RK3326
handhelds; only the XU10 has run the build. The Linux-host route is `NOT_RUN`. Output:
`dist/BanjoTooieRecompiled-v<version>-Linux-arm64-PortMaster.zip` and its `.sha256`. `--release-version <ver>` names a release; otherwise the name
carries `git describe`, and `BUILD-INFO.txt` lists the dirty files.

## 5. Package contract

```text
Banjo-Tooie.sh            launcher, sources PortMaster's control.txt
tooie/
  TooieRecompiled         stripped aarch64 executable
  assets/ ui_assets/ icons/ recompcontrollerdb.txt
  graphics.default.json   copied to graphics.json once, so an update keeps the player's settings
  portable.txt            keeps saves and settings in this folder
  port.json gameinfo.xml  PortMaster metadata
  README.txt LICENSE.txt licenses/ BUILD-INFO.txt DEPENDENCIES.txt MANIFEST.sha256
```

- The package script stops on a file name that looks like a ROM, a save, a shader cache or a log, on a build without `TK64_MOBILE` and on
  a non-aarch64 binary.
- Libraries come from the firmware (`DEPENDENCIES.txt`), and `port.json` states the newest glibc symbol the binary uses.
- `port.json` has no screenshot: the repository carries no game images, and PortMaster's own catalog requires one. Submitting there is a
  separate decision.
- The launcher stops with a message when there is neither a converted image nor a ROM in the folder, and shows another when the game rejected the ROM.
- The launcher exports `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0`, which rendering on Panfrost needs: with the endpoint path, the shader reads zeros from its texel buffers (`UNKNOWN`).

## 6. Requirements on the device

- aarch64, OpenGL ES 3.1 with compute shaders, and the firmware's libraries.
- Wayland or X11. The backend's EGL surface has no KMSDRM path (`UNSUPPORTED`): it fails with `unsupported SDL window subsystem`.
- 1 GB of memory is the tested size, and the frontend stays resident while the game runs.

## 7. Validation boundaries

See `docs/HANDOFF.md` for the results of the current build. Known limits of this contract:

- Only the MagicX XU10 with ROCKNIX has run it, with `libmali` and with Panfrost. Other devices and firmwares are `NOT_RUN`. On Panfrost, a fresh install with an empty cache has not been measured.
- Linux x64 was not built or run for this change. It relies on the source check.
- CI (`.github/workflows/build.yml`, job `public-handheld-cross`, GitHub-hosted Ubuntu): fetches the Debian 12 sysroot, builds the host
  `file_to_c`, cross-builds `rt64` (with the OpenGL ES backend) and the pure host tests through `build-handheld.sh`, and runs those tests
  under `qemu-aarch64`. The game itself needs the ROM, so it is built and packaged only by `private-build.yml`
  (`private-game-build-handheld`, manual dispatch on the owner's runner, uploads checksums only). Both jobs: `NOT_RUN` on GitHub. The same
  target list was cross-built on Windows and its 9 tests passed on the XU10 itself. `rom_cart_import_test` does not build for aarch64
  (`sse2neon.h` is not on that test's include path) and is left out.
- A cold program stalls the OpenGL thread on first use, so a first run has pauses. Compiling off that thread is `NOT_IMPLEMENTED`.
- An out-of-memory close happened on the first run after a driver switch, on both drivers, with depth to RAM on one and off on the other. The cause is
  `UNKNOWN`, and later runs on the same boot did not repeat it.
- MSAA, a resolution scale above 1 and interpolation are not validated on a handheld.

Related: [packaging](packaging-and-dependencies.md), [startup and backends](startup-and-backends.md), [build](build-and-generator.md), [graphics](graphics-and-presentation.md).
