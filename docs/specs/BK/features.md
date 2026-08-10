---
title: "BK — Features"
status: current
area: "specs/BK"
updated: 2026-08-08
tags:
  - banjo-recomp
  - features
---

# BK — Features

Feature set of the port, mostly provided by the `RT64` rendering engine,
`N64ModernRuntime` runtime, `RecompFrontend` UI, and the native patch layer.

## Plug and Play

Provide your own copy of the NTSC-U 1.0 ROM in the main menu. No separate
extraction step and no self-build required. All assets (textures, models,
audio) are loaded from the user's ROM at runtime.

## Faithful N64 effects

Original graphical effects stay intact and are adapted for widescreen and
high framerate — e.g. the jigsaw puzzle transition and Bottles' Moving
Picture Game. Mipmap fidelity is preserved.

## Audio

Music and SFX are faithful to the N64 version: original `n_aspMain` DSP
microcode is recompiled (see [RSP](rsp.md)), playback has no popping, and
audio-to-visual sync is corrected.

## Menus

In-game config menu for gameplay, graphics, input, and audio. Fully usable
with mouse, gamepad, or keyboard.

## High framerate

Play at any framerate via RT64. Default is the monitor's refresh rate. The
original 30/20 FPS mode remains available. **Changing framerate does not
affect gameplay** (frame-rate independent).

> Avoid external frame limiters (e.g. NVIDIA Control Panel); they can cause
> stutter. Use the in-game slider instead.

## Widescreen / ultrawide

Any aspect ratio supported; effects modded for widescreen. HUD can be pinned
to 16:9 position when using ultrawide. Game models extended where needed so
cutscenes look right in 16:9. Optional pillarboxing of cutscenes to 16:9 or
original ratio.

## Mod support

- Install mods by dragging them onto the window or via the mod menu.
- Mods can change anything, including adding wholly new content.
- Thunderstore: https://thunderstore.io/c/banjo-recompiled/.
- Modding references: [BKRecompModTemplate](https://github.com/BanjoRecomp/BKRecompModTemplate),
  RT64 textures: `rt64/TEXTURE-PACKS.md`.

## Dual analog camera

Right stick controls camera when enabled; C buttons still work.

## Note saving

Notes persist after leaving a level or dying, like the later releases.
Optional — the original score mechanic can be restored. Setting applies on
return to Grunty's Lair or on restart.

## Low latency + instant loading

Optimized input lag; saves/loads, warps, and pause take "the blink of an
eye" since the game runs natively.

## Platforms

- Windows: D3D12 / Vulkan; GPU at least D3D12 SM6.
- Linux / Steam Deck: native binary + Flatpak.
- macOS: Metal, Apple Silicon or Intel 7th gen+.

**System requirements**: GPU with Direct3D 12 (SM 6) / Vulkan 1.2 / Metal
Argument Buffers Tier 2. x86-64 needs SSE4.1 (Penryn / Bulldozer+). ARM64
builds run on any ARM64.

## Known issues

- Renders can misbehave with GPU overlays (MSI/Afterburner, Wallpaper
  Engine).
- Prebuilt releases never contain game assets.

## Distribution

Prebuilt binaries are portable via `portable.txt` in the executable folder
(saves/config/mods next to the binary). Saves data goes to
`%LOCALAPPDATA%\BanjoRecompiled\saves` on Windows,
`~/.config/BanjoRecompiled/saves` on Linux, etc.

## Tooie outlook

Almost the whole feature stack is reusable for a Tooie port: the rendering
engine, runtime replacement, patch ELF mechanism, and the "load assets from
the user ROM" model are all architecture-level.

See [TooieRecomp roadmap](../../roadmap/tooie-recomp.md).