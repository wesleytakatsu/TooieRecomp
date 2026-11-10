# Banjo-Tooie Recompiled

**English** · [Português (Portugal)](README-PT.md) · [Português (Brasil)](README-BR.md) · [Español](README-ES.md) · [Italiano](README-IT.md) · [日本語](README-JA.md)

Unofficial native PC port of Banjo-Tooie (N64) made with static recompilation (N64Recomp) for Windows x64 and Linux x64.

**HD music mod:** [download the HD soundtrack mod](https://drive.google.com/file/d/1GRaaK0vPQz0tbBfj6oja9Q0x98VPki4d/view?usp=drive_link) (v0.1.6 or newer). Install the ZIP from the in-game
`Mods` menu, or copy it unchanged to `%LOCALAPPDATA%\TooieRecompiled\mods` (Linux: `~/.config/TooieRecompiled/mods`),
then enable it in `Mods`.

---

## Important: ROM Required

This project contains **NO game assets and NO ROM**. You need your own legally obtained Banjo-Tooie (USA) ROM:

- **File Name:** `Banjo-Tooie (USA).z64` (or `.n64`)
- **File Size:** `33554432` bytes (32 MiB)
- **SHA-1 Hash:** `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`

Graphics, audio, and game data are read and decompressed directly from the ROM at run time.

---

## Features

- **Widescreen 16:9:** Native widescreen support with corrected aspect ratio.
- **Frame Interpolation:** Smooth 60 / 120 FPS gameplay via transform interpolation (`Graphics` → `Interpolation`).
- **Object distance (experimental):** `Graphics` → `Object Distance` draws and keeps objects farther away (2x, 4x,
  Unlimited). These settings need `General` → `Expanded Memory (experimental)`, which gives the game a larger memory
  area; choosing a higher distance turns it on, and it takes effect after restarting the game. Story split screen
  always uses the original distance.
- **Local Multiplayer (Up to 4 Players):** Enabled under `General` → `Multiplayer (up to 4 players)` (takes effect
  after restarting the game).
  - Per-player controller and/or keyboard input.
  - Swap controllers dynamically between players.
  - Per-player button mapping with a `Reset to defaults` option.
  - Controller identification tools in `Controls`: an input indicator and a `Rumble 1 s` test button.

  **Warning:** Story-mode split screen (Dev Mode → Experimental → `Two players (split screen)`) is experimental and can
  break the game (crashes, soft-locks, broken progress). There is still a lot to adjust and there are already many known
  bugs; it is being improved little by little. Save your game before using it; enabling `General` →
  `Expanded Memory (experimental)` is recommended.
- **Modern Controller & Keyboard Support:** Gamepad support powered by SDL2 alongside customizable keyboard controls.
- **Native Launcher:** Built-in graphical launcher with ROM selection.
- **Texture Packs:** RT64 texture packs (`.rtz`) via the in-game `Mods` menu.
- **Texture packs with OpenGL or DirectX 11:** textures from mods still need a fix in these modes (some appear
  black). Use Direct3D 12 or Vulkan for texture packs.
- **Dev Mode:** an ESC menu tab unlocked after a warning. `Warp` saves where Banjo stands and teleports back there
  later, also from another level. It changes the game's state directly: save your game before using it.

---

## Download & Play

### Windows (x64)

1. Download `BanjoTooieRecompiled-v<version>-Windows-x64.zip` from the
   [Releases page](https://github.com/wesleytakatsu/TooieRecomp/releases).
2. (Optional) Check the archive integrity against the accompanying `.sha256` file.
3. Extract the ZIP archive (creates a folder named `BanjoTooieRecompiled`).
4. Run `TooieRecompiled.exe`.
5. Select your ROM in the launcher.

**Requirements & Notes:**
- Requires a GPU supporting **Direct3D 12**.
- Settings and save files are stored in: `%LOCALAPPDATA%\TooieRecompiled`

### Linux (x64)

1. Download `BanjoTooieRecompiled-v<version>-Linux-x64.tar.gz` from the
   [Releases page](https://github.com/wesleytakatsu/TooieRecomp/releases).
2. Extract the archive and enter the directory.
3. Run `./run-tooie.sh` and select your ROM in the launcher.

The release also has a `.deb` package (Debian/Ubuntu/Mint) and an `.AppImage`.

**Requirements & Notes:**
- Requires a **Vulkan**-capable GPU.
- System dependencies: `SDL2`, `FreeType`, `GTK 3`.
- Settings and save files are stored in: `~/.config/TooieRecompiled`

### Handhelds (PortMaster, aarch64)

1. Install `BanjoTooieRecompiled-v<version>-Linux-arm64-PortMaster.zip` with PortMaster.
2. Copy your ROM to `ports/tooie` and start **Banjo-Tooie** from the Ports list. The first start converts the ROM once.

**Requirements & Notes:**
- Needs OpenGL ES 3.1 and a firmware with Wayland or X11. Tested only on the MagicX XU10 with ROCKNIX; other devices are untested.
- Original resolution and 4:3. The first time each scene appears, its shaders compile and the game can pause briefly.
- Select + Start asks whether to quit. Settings and saves stay in `ports/tooie`.

---

## Tips & Notes

- **In-Game Menu:** Open the in-game menu at any time by pressing `Esc`.
- **Xbox Controllers on Windows:** Xbox gamepads only accept the rumble test after you press a button on them
  ("Press a button on it first").
- **Identical Controllers:** If you connect multiple identical controllers without a serial number, they are
  re-assigned in connection order after a restart.

### Game languages

TooieRecomp supports in-game text languages: **English** (default), **Português (Brasil)**, **Français**, **Deutsch**, **Español**, **Italiano** (draft), and **日本語** (Japanese).

- **Português (Brasil), Français, Deutsch, Español, Italiano, 日本語:** Included in the release; only the USA ROM is needed.
- **Italiano (draft):** Combines the Rulesless v0.9 Italian translation with a supplemental translation awaiting editorial review. Credit: Rulesless (https://letraduzionidirulesless.wordpress.com/banjo-tooie/).
- **The Japanese language still needs many checks and tests**: line breaks and menu widths have not been verified on screen, and errors are expected. Reports are welcome.
- **Applying language:** Chosen in `Settings` → `General` → `Game Language` (accessible from the launcher or the in-game `Esc` menu). The selected language is applied when the game starts. If changed during gameplay, restart the game for it to take effect.

### Texture packs

- **BT Reloaded** (by GhostlyDark, a separate project): download `bt-reloaded-v<version>-rt64.rtz` from its
  [GitHub releases](https://github.com/GhostlyDark/BT-Reloaded/releases) or from the
  [pack's website](https://evilgames.eu/texture-packs/bt-reloaded.htm).
- Install it from the in-game `Mods` menu, or copy the file to `%LOCALAPPDATA%\TooieRecompiled\mods`
  (Linux: `~/.config/TooieRecompiled/mods`), then enable it in `Mods`. The pack's `gliden64-png` ZIP also works without
  extracting it.
- Texture packs are not included in the releases. BT Reloaded v0.1.1 replaces mostly the menus and HUD so far; it was
  tested only on Windows.

---

## Building from Source

To build TooieRecomp from source, place your verified `Banjo-Tooie (USA).z64` ROM in the repository root directory.

- **Windows:** Run `.\tools\windows\Build-Tooie.ps1 -Stage All` in PowerShell.
  Prerequisites: Visual Studio 2022+ (with clang-cl), Python 3.8+, and Git for Windows.
  See [tools/windows/README.md](tools/windows/README.md) for full instructions.
- **Linux x64:** Run `./build-linux.sh` to select tar, Debian, AppImage or all formats (tested on Ubuntu 24.04 / Linux Mint 22). Use `./build-linux.sh all --check` to check prerequisites only.
  Prerequisites: Clang, CMake, Ninja, SDL2, FreeType, GTK 3, and Python 3.
  See [tools/BUILDING.md](tools/BUILDING.md) for full instructions.
- **Handhelds (aarch64, PortMaster):** cross build from Windows (`tools/handheld/build-handheld.ps1`) or Linux
  (`tools/handheld/build-handheld.sh`), then `tools/handheld/package_portmaster.py`.
  See [tools/BUILDING.md](tools/BUILDING.md) for the sysroot and steps.

---

## Credits

- **Core Technologies:** Built with [N64Recomp](https://github.com/N64Recomp/N64Recomp) / N64ModernRuntime,
  [RT64](https://github.com/rt64/rt64), RecompFrontend ([RmlUi](https://github.com/mikke89/RmlUi)), and
  [SDL2](https://www.libsdl.org/).
- **Inspirations:** Inspired by [Zelda64Recomp](https://github.com/Zelda64Recomp/Zelda64Recomp) and
  [BanjoRecomp](https://github.com/BanjoRecomp/BanjoRecomp).
- **Decompilation:** The Banjo-Tooie decompilation community for reverse-engineering the original game.
- **Third-Party Licenses:** Listed in [`vendor/`](vendor/README.md) and inside each release package under `licenses/`
  and `THIRD-PARTY-MANIFEST.tsv`.

---

## License

TooieRecomp's own code is licensed under the [GNU General Public License v3.0](LICENSE). The code translated from the
game, the decompilation in `decomp/` and the game's content are not covered by this license. Third-party components
keep their own licenses (see above).

---

## Legal

Banjo-Tooie is © Nintendo / Rare.

This is an unofficial fan project and is not affiliated with or endorsed by Nintendo or Rare.

The executable contains code translated from the game; use a ROM dumped from a cartridge you own. Do not ask for ROMs.
