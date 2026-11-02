# Banjo-Tooie Recompiled

**English** · [Português (Portugal)](README-PT.md) · [Português (Brasil)](README-BR.md) · [Español](README-ES.md)

Unofficial native PC port of Banjo-Tooie (N64) made with static recompilation (N64Recomp) for Windows x64 and Linux x64.

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
- **Local Multiplayer (Up to 4 Players):** Enabled under `General` → `Multiplayer (up to 4 players)` (takes effect
  after restarting the game).
  - Per-player controller and/or keyboard input.
  - Swap controllers dynamically between players.
  - Per-player button mapping with a `Reset to defaults` option.
  - Controller identification tools in `Controls`: an input indicator and a `Rumble 1 s` test button.
- **Modern Controller & Keyboard Support:** Gamepad support powered by SDL2 alongside customizable keyboard controls.
- **Native Launcher:** Built-in graphical launcher with ROM selection.
- **Texture Packs:** RT64 texture packs (`.rtz`) via the in-game `Mods` menu.
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

---

## Tips & Notes

- **In-Game Menu:** Open the in-game menu at any time by pressing `Esc`.
- **Xbox Controllers on Windows:** Xbox gamepads only accept the rumble test after you press a button on them
  ("Press a button on it first").
- **Identical Controllers:** If you connect multiple identical controllers without a serial number, they are
  re-assigned in connection order after a restart.

### Game languages

TooieRecomp supports in-game text languages: **English** (default), **Português (Brasil)**, **Français**, **Deutsch**, and **Español**.

- **Português (Brasil):** Built into the game. No additional files or European ROM required.
- **Français, Deutsch, Español:** Select the language in **Settings** → **General** → **Game Language** (accessible from the launcher or the in-game `Esc` menu). The game will ask you for your European ROM (`Banjo-Tooie (Europe) (En,Fr,De,Es).z64`, SHA-1 `93bf2fac1387320ad07251cb4b64fd36bac1d7a6`) once to extract the language pack. The European ROM is used ONLY to extract that language; the game always runs from your USA ROM.
- **Applying language:** The selected language is applied when the game starts. If changed during gameplay, restart the game for it to take effect.

---

## Building from Source

To build TooieRecomp from source, place your verified `Banjo-Tooie (USA).z64` ROM in the repository root directory.

- **Windows:** Run `.\tools\windows\Build-Tooie.ps1 -Stage All` in PowerShell.
  Prerequisites: Visual Studio 2022+ (with clang-cl), Python 3.8+, and Git for Windows.
  See [tools/windows/README.md](tools/windows/README.md) for full instructions.
- **Linux:** Run `tools/build_linux.sh` (tested on Ubuntu 24.04 / Linux Mint 22).
  Prerequisites: Clang, CMake, Ninja, SDL2, FreeType, GTK 3, and Python 3.
  See [tools/BUILDING.md](tools/BUILDING.md) for full instructions.

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
