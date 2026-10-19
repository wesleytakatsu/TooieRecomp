---
title: "Evidence — Host Launcher Banjo-Tooie Wordmark & Asset Isolation"
description: "Implementation evidence, build verification, and independent visual QA for the two-line Banjo-Tooie / Recompiled host launcher wordmark and CMake asset isolation."
status: evidence
phase: P08
updated: 2026-10-19
tags: [tooie-recomp, p08, frontend, launcher, wordmark, logo, svg, ui_assets, recompui, visual-verification]
---

# Evidence — Host Launcher Banjo-Tooie Wordmark & Asset Isolation

## 1. Context & Objective

Prior to this work, the host launcher UI displayed the upstream Banjo-Kazooie wordmark (`Logo.svg` from the shared `BanjoRecomp` submodule). The bounded objective of this task was to replace the launcher wordmark with a dedicated, Tooie-owned vector asset rendering exactly two lines:
1. Line 1: **"BANJO-TOOIE"** (blue fill with yellow outline, artist-styled caps);
2. Line 2: **"RECOMPILED"** (red fill with yellow outline, artist-styled caps).

In addition, the implementation isolates Tooie-owned UI assets from the shared `BanjoRecomp` repository assets to prevent upstream mutation.

## 2. Epistemic Classification

- **`PROVEN`**:
  - Two-line SVG wordmark asset generation: `tools/make_tooie_launcher_logo.py` (SHA256 `d13cb8d92dad9fc0784344307548bf9961fb337356afbedb168e1de62f267497`) reads `BanjoRecomp/assets/Logo.svg` read-only and outputs `ui_assets/TooieLogo.svg` (viewBox `0 0 7300 2450`, 42,342 bytes, SHA256 `036d3cb20c97b2b37a427e16c2fa4190b6bc676aae1655a037d7bf4785d5c5b7`).
  - Upstream asset protection: `BanjoRecomp/assets/Logo.svg` (SHA256 `41d7f3768fff8bcb84f3d2c40c706eaddd521ebb708b87ffaf9fed47fb725bb3`) remains unchanged and tracked (clean), not untracked; `git status -- assets` in `BanjoRecomp` is clean (0 lines).
  - CMake asset copy rule: `CMakeLists.txt` defines custom command `copy_if_different` copying `ui_assets/TooieLogo.svg` to `${CMAKE_CURRENT_BINARY_DIR}/ui_assets/TooieLogo.svg` (SHA256 `036d3cb2...`) under target `TooieLauncherLogo`, a direct dependency of `TooieRecompiled`.
  - Path resolution: `src/main/launcher_animation.cpp` references `"../ui_assets/TooieLogo.svg"` with dimensions `7300.0f * 0.106f` x `2450.0f * 0.106f` (~774x260 dp, compared to prior 773x328 dp). Rml document source `assets/` strips `../` lexically to resolve `ui_assets/TooieLogo.svg` relative to the working directory (`build/`).
  - Incremental build: `cmake --build build --target TooieRecompiled -j2` exit 0 (NOT clean build), producing executable `build/TooieRecompiled` (SHA256 `ffe3d7a52e2e4b97d159e79fb156ab27e3a909dfa73a44633f7ff15b8bfb46d4`).
  - Prior checkpoint publication: Commit `2c52af0c7d8287d9003db839ffbe636d585e900a` is published bit-exact on `origin/main` (`ahead: 0, behind: 0`, 199 paths, push exit 0, remote verification exit 0; verified by `/home/wesley/.cache/tooie-fps-investigation/checkpoint-publication-proof.json`).
  - Working tree hygiene: `git diff --check` exits 0; root graph refresh `graft` indexed 1,632 nodes (exit 0); jCode source indexes refreshed; pre-existing local tool/config state preserved.

- **`VISUALLY_CONFIRMED_FOR_TESTED_RESOLUTIONS`** (`USER_REPORTED` & Haiku independent reviewer):
  - Independent visual inspection performed by Claude Haiku 5.5 medium (exit 0, session `a6291041-e941-46cb-8528-d06718145437`, PID 129590, log `/home/wesley/.cache/tooie-fps-investigation/menu-title/haiku/game-launch.log`).
  - Tested at **1600x900** (16:9 window, initial capture, screenshots `frame_1.png` through `frame_6.png` in `/home/wesley/.cache/tooie-fps-investigation/menu-title/haiku/`):
    - Title text renders exactly two lines: line 1 "BANJO-TOOIE" in blue with yellow outline; line 2 "RECOMPILED" in red with yellow outline.
    - No clipping of title glyphs (letters "B" through "E" and "R" through "D" fully inside window).
    - Spacing between letters and lines is adequate; "J" descender in BANJO sits close to top of "P" in RECOMPILED but does not collide or overlap.
    - Title block sits centered above the character art; clouds and Banjo/Kazooie art unobstructed.
    - Launcher menu buttons ("Start Game", "Controls", "Settings", "Mods", "Exit") are present, right-aligned, and legible.
    - No SVG loading errors in log (SVG plugin initialized cleanly).
  - Tested at **1024x768** (4:3 resized window, screenshot `resize_1024x768_4x3.png`):
    - Title remains two lines, fits window, no clipping, character art and clouds present, menu buttons legible.
  - Root visually inspected ONLY `frame_1.png` (1600x900); Haiku inspected 4:3 (`resize_1024x768_4x3.png`), with no root 4:3 inspection claim.
  - **Launcher entry animation** (bounded PASS, Haiku follow-up): Claude Haiku 5.5 medium exit 0 (session `a6291041-e941-46cb-8528-d06718145437`, addendum `/home/wesley/.cache/tooie-fps-investigation/menu-title-haiku-animation-addendum.txt`) independently viewed 5 discrete images captured at ~0.28s, 0.79s, 1.42s, 2.57s, 5.87s after window detection (`/home/wesley/.cache/tooie-fps-investigation/menu-title/haiku/anim/{t_0_2s,t_0_5s,t_1s,t_2s,t_5s}.png`). Logo entry composition PASS: blank start (0.28s) -> characters slide in from edges (0.79s) -> characters converge (1.42s) -> title logo and menu appear fully formed without clipping (2.57s) -> clouds enter and layout settles fully (5.87s). Logo is fully drawn when appearing at ~2.57s and not clipped during entry. Settled state at 5.87s matches earlier settled captures (~18-30s post-launch).

- **`NOT_TESTED` / Bounded Limitations**:
  - **Animation smoothness / easing**: `NOT_TESTED`. Only 5 discrete frames evaluated; frame-level smoothness or easing curve not captured at high frame rate.
  - **Startup and post-settled frames**: Entry phase before ~0.2s post-window detection (<0.28s) and frames 6-12s NOT_TESTED in animation follow-up. Existing settled 1600x900 and 1024x768 PASS preserved.
  - **Other aspect ratios / resolutions**: Untested beyond 1600x900 and 1024x768.
  - **Gameplay & menus**: NO gameplay entered, no menu buttons clicked, no user configuration modified. No broad gameplay or GUI campaign assertion.
  - **Asset aesthetics & legal**: Drop-shadow layer from original source logo was omitted; line 1 is artist caps styling rather than mixed case; trademark/licensing status of the fan-recreated wordmark is `UNKNOWN`.
  - **Uncommitted status**: This UI wordmark update is uncommitted in the local working tree; pre-existing local tool/config state preserved.

## 3. Implementation Details

### 3.1 SVG Logo Generator (`tools/make_tooie_launcher_logo.py`)
- Script reads `../BanjoRecomp/assets/Logo.svg` read-only.
- Reuses per-letter vector path groups for `B`, `A`, `N`, `J`, `O`, `-`, `O`, `O`, `R`, `E`, `C`, `O`, `M`, `P`, `I`, `L`, `E`, `D`.
- Recolors source letters to the Tooie palette (blue line 1, red line 2, yellow outline).
- Inserts a hand-drawn vector polygon for the letter `T`.
- Sets viewBox to `0 0 7300 2450`.
- Outputs standalone SVG to `ui_assets/TooieLogo.svg`.

### 3.2 CMake Asset Rule (`CMakeLists.txt`)
```cmake
set(TOOIE_LAUNCHER_LOGO "${CMAKE_CURRENT_BINARY_DIR}/ui_assets/TooieLogo.svg")
add_custom_command(
    OUTPUT "${TOOIE_LAUNCHER_LOGO}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/ui_assets/TooieLogo.svg"
        "${TOOIE_LAUNCHER_LOGO}"
    DEPENDS "${CMAKE_SOURCE_DIR}/ui_assets/TooieLogo.svg"
    VERBATIM
)
add_custom_target(TooieLauncherLogo DEPENDS "${TOOIE_LAUNCHER_LOGO}")
add_dependencies(TooieRecompiled TooieLauncherLogo)
```

### 3.3 Launcher Animation Binding (`src/main/launcher_animation.cpp`)
```cpp
launcher_context.logo_svg = // Tooie-owned wordmark: copied to <build>/ui_assets by CMake, resolved relative to the shared assets/ dir.
create_animated_svg(context, background_container, "../ui_assets/TooieLogo.svg", 7300.0f * 0.106f, 2450.0f * 0.106f);
```

## 4. Verification & Artifacts

| Item | Value / Path | Result / Status |
| :--- | :--- | :--- |
| Generator Script | `tools/make_tooie_launcher_logo.py` | SHA256 `d13cb8d92dad9fc0784344307548bf9961fb337356afbedb168e1de62f267497` |
| Source Logo SVG | `ui_assets/TooieLogo.svg` | SHA256 `036d3cb20c97b2b37a427e16c2fa4190b6bc676aae1655a037d7bf4785d5c5b7` |
| Build Logo SVG | `build/ui_assets/TooieLogo.svg` | SHA256 `036d3cb20c97b2b37a427e16c2fa4190b6bc676aae1655a037d7bf4785d5c5b7` |
| Upstream Shared Logo | `BanjoRecomp/assets/Logo.svg` | SHA256 `41d7f3768fff8bcb84f3d2c40c706eaddd521ebb708b87ffaf9fed47fb725bb3` (TRACKED, CLEAN) |
| Binary Hash | `build/TooieRecompiled` | SHA256 `ffe3d7a52e2e4b97d159e79fb156ab27e3a909dfa73a44633f7ff15b8bfb46d4` |
| CMake Target Incremental Build | `cmake --build build --target TooieRecompiled -j2` | Exit 0 (not clean build) |
| Haiku Visual QA Report (Settled) | `/home/wesley/.cache/tooie-fps-investigation/menu-title-haiku-visual-report.txt` | PASS (exit 0) |
| Haiku Animation Addendum | `/home/wesley/.cache/tooie-fps-investigation/menu-title-haiku-animation-addendum.txt` | PASS (exit 0) |
| Haiku Animation Frames | `/home/wesley/.cache/tooie-fps-investigation/menu-title/haiku/anim/{t_0_2s,t_0_5s,t_1s,t_2s,t_5s}.png` | PASS (entry composition verified across 5 discrete frames) |
| Haiku 1600x900 Screenshot | `/home/wesley/.cache/tooie-fps-investigation/menu-title/haiku/frame_1.png` | PASS (two-line title legible, no clipping; root inspected ONLY this frame) |
| Haiku 1024x768 Screenshot | `/home/wesley/.cache/tooie-fps-investigation/menu-title/haiku/resize_1024x768_4x3.png` | PASS (4:3 scaling verified by Haiku; no root 4:3 claim) |
| Sonnet Handoff Report | `/home/wesley/.cache/tooie-fps-investigation/menu-title-sonnet-handoff.txt` | Implementation handoff |
| Checkpoint Publication | `/home/wesley/.cache/tooie-fps-investigation/checkpoint-publication-proof.json` | Commit `2c52af0c...` published on `origin/main` |

## 5. Model Identities & Contributions

- **Claude Sonnet 5.5 high** (`ActualSonnet5.5high`): Author of implementation (SVG generator script, CMake target, `launcher_animation.cpp` patch).
- **Claude Haiku 5.5 medium** (`ActualHaiku5.5medium`): Independent visual QA reviewer (exit 0, launch verification, screenshot capture and visual analysis across settled frames and 5-frame entry animation sequence).
- **Grok**: Preliminary wordmark styling investigation (superseded by actual implementation).
- **Gemini 3.8 Flash High**: Canonical documentation author across evidence, handoff, phase plans, and specs.
- **Root**: Independent visual inspection of `frame_1.png` (1600x900), graft graph refresh (1632 nodes), and repository oversight.
