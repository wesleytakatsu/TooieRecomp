Banjo-Tooie Recompiled for PortMaster (aarch64)
===============================================

Unofficial native port of Banjo-Tooie (N64), made with static recompilation and the TK64 renderer (OpenGL ES 3.1).
This package contains no ROM and no game assets. You need your own copy of the game.

INSTALL
1. Install this zip with PortMaster, or copy Banjo-Tooie.sh and the tooie folder to your ports folder.
2. Copy your ROM to ports/tooie:
     Banjo-Tooie (USA), .z64, .n64 or .v64, 33554432 bytes. Other versions are not accepted.
     SHA-1 af1a89e12b638b8d82cc4c085c8e01d4cba03fb3 (for the .z64 byte order)
   The first start converts it once into bt.n64.us.1.0.z64 (35098224 bytes). You can delete your copy after that.
3. Start Banjo-Tooie from the Ports list.

CONTROLS
Select + Start asks whether to quit. Buttons are set in the game's menu.

WHAT TO EXPECT
- The first time each scene appears, its shaders are compiled: the game can pause for up to a second, and the first
  minutes run slowly. The compiled shaders stay in tooie/shadercache, and later runs are smooth.
- If the game closes by itself, the device ran out of memory. Restart the device and try again. It has 1 GB of memory
  and the menu of some firmwares grows over time.
- Target: original 320x240 and 4:3, without frame interpolation.

REQUIREMENTS
- A firmware with Wayland or X11. The renderer has no KMSDRM path: firmwares that run games directly on KMSDRM fail
  with "unsupported SDL window subsystem" in tooie/log.txt.
- OpenGL ES 3.1 with compute shaders. glibc 2.35 or newer. The libraries in DEPENDENCIES.txt come from the firmware.
- Tested only on the MagicX XU10 (RK3326, Mali-G31, 1 GB) with ROCKNIX, with both Panfrost and libmali. Other devices
  are untested, and reports are welcome.

FILES
Saves and settings stay in the tooie folder. Log of the last run: tooie/log.txt.

LEGAL
Banjo-Tooie is (c) Nintendo / Rare. This is an unofficial fan project, not affiliated with or endorsed by Nintendo or
Rare. The executable contains code translated from the game; use a ROM dumped from a cartridge you own.
TooieRecomp's own code is licensed under the GNU GPL v3.0 (LICENSE.txt); the code translated from the game is not.
Third-party components and their licences: licenses/.

Verify the files from this folder: sha256sum -c MANIFEST.sha256
Source: https://github.com/wesleytakatsu/TooieRecomp
