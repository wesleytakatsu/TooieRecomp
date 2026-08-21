# shellcheck shell=bash
# Shared paths for ANGRYLION_CONTROL. Sourced by fetch/build/run/status.

ORACLE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TOOIE_ROOT="$(cd "$ORACLE_DIR/../../.." && pwd)"
# shellcheck disable=SC1091
source "$ORACLE_DIR/pins.env"

WORK="$ORACLE_DIR/work"
WORK_MUPEN="$WORK/mupen"
WORK_LLE="$WORK/lle"
PREFIX="$WORK_MUPEN/inst"
CFG_TEMPLATE="$ORACLE_DIR/cfg/mupen64plus.cfg"
CFG_RUNTIME="$WORK_LLE/cfg/mupen64plus.cfg"
PATCH="$ORACLE_DIR/patches/mupen64plus-core-p08-instrumentation.patch"
HOOK_SRC="$TOOIE_ROOT/tools/p08_attract_egg_hook.h"

CORE_SRC="$WORK_MUPEN/mupen64plus-core"
UI_SRC="$WORK_MUPEN/mupen64plus-ui-console"
AUDIO_SRC="$WORK_MUPEN/mupen64plus-audio-sdl"
INPUT_SRC="$WORK_MUPEN/mupen64plus-input-sdl"
RSP_HLE_SRC="$WORK_MUPEN/mupen64plus-rsp-hle"
ANGRYLION_SRC="$WORK_LLE/angrylion-rdp-plus"
CXD4_SRC="$WORK_LLE/mupen64plus-rsp-cxd4"

MUPEN_BIN="$PREFIX/bin/mupen64plus"
CORE_LIB="$PREFIX/lib/libmupen64plus.so.2"
SHARE_DIR="$PREFIX/share/mupen64plus"
INPUT_SO="$PREFIX/lib/mupen64plus/mupen64plus-input-sdl.so"
AUDIO_SO="$PREFIX/lib/mupen64plus/mupen64plus-audio-sdl.so"
GFX_SO="$ANGRYLION_SRC/build/mupen64plus-video-angrylion-plus.so"
RSP_SO="$CXD4_SRC/projects/unix/mupen64plus-rsp-cxd4-sse2.so"
PLUGIN_DIR="$WORK_LLE/plugins"

fail() { echo "ERROR: $*" >&2; exit 1; }

oracle_paths_ok() {
    [[ -x "$MUPEN_BIN" && -e "$CORE_LIB" && -f "$GFX_SO" && -f "$RSP_SO" && -f "$INPUT_SO" && -f "$AUDIO_SO" ]]
}
