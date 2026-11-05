#!/bin/bash
# PORTMASTER: tooie.zip, Banjo-Tooie.sh
# Banjo-Tooie Recompiled (TooieRecomp with the TK64 renderer).

XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}

if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi

source $controlfolder/control.txt
[ -f "${controlfolder}/mod_${CFW_NAME}.txt" ] && source "${controlfolder}/mod_${CFW_NAME}.txt"
get_controls

GAMEDIR="/$directory/ports/tooie"
cd "$GAMEDIR" || exit 1
> "$GAMEDIR/log.txt" && exec > >(tee "$GAMEDIR/log.txt") 2>&1

# The game needs the player's copy: the cart is converted once, on first start, into bt.n64.us.1.0.z64.
if [ ! -f bt.n64.us.1.0.z64 ] && [ -z "$(find . -maxdepth 1 -type f \( -iname '*.z64' -o -iname '*.n64' -o -iname '*.v64' \))" ]; then
  pm_message "Copy your Banjo-Tooie (USA) ROM to ports/tooie. See README.txt there."
  sleep 8
  pm_finish
  exit 1
fi

# Settings for a weak GPU the first time; the player's changes stay after that, also across updates of this port.
[ -f graphics.json ] || cp graphics.default.json graphics.json

export TK64_GRAPHICS_API=opengles
export TK64_PROFILE=weak
export TOOIE_HANDHELD=1
export TOOIE_AUTO_START=1
export TOOIE_DISABLE_LAUNCHER_MUSIC=1
export TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0
export MESA_SHADER_CACHE_DIR="$GAMEDIR/shadercache"
export MESA_SHADER_CACHE_MAX_SIZE=64M
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
mkdir -p "$MESA_SHADER_CACHE_DIR"

# No gptokeyb: the game handles Select + Start itself and asks before quitting. gptokeyb's hotkey would kill it during that question.
pm_platform_helper "$GAMEDIR/TooieRecompiled"
./TooieRecompiled

# The game closes by itself when it finds no acceptable ROM.
sleep 1
if grep -aq "ROM not valid" "$GAMEDIR/log.txt"; then
  pm_message "This ROM was not accepted: the game needs Banjo-Tooie (USA). See README.txt in ports/tooie."
  sleep 8
fi

pm_finish
