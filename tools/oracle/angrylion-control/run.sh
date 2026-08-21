#!/usr/bin/env bash
# Launch ANGRYLION_CONTROL: Mupen Pure Interpreter + Angrylion RDP Plus + CXD4 LLE.
# Oracle only. Does not start TooieRecompiled / RT64.
set -euo pipefail
# shellcheck disable=SC1091
source "$(cd "$(dirname "$0")" && pwd)/scripts/common.sh"

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    cat <<'USAGE'
Usage: tools/oracle/angrylion-control/run.sh [mupen64plus args]

Mupen Pure Interpreter + Angrylion RDP Plus + CXD4 LLE (ANGRYLION_CONTROL).
Does not start TooieRecompiled. Interactive wrapper:

  ./run-tooie-angrylion-rdp.sh

Environment:
  TOOIE_CART_Z64            cartridge .z64/.n64
  TOOIE_ANGRYLION_AUDIO     dummy (default here) or sdl
USAGE
    exit 0
fi

oracle_paths_ok || fail "ANGRYLION_CONTROL is not built. See docs/specs/BT/angrylion-control-oracle.md"

resolve_rom() {
    if [[ -n "${TOOIE_CART_Z64:-}" ]]; then
        printf '%s' "$TOOIE_CART_Z64"
        return
    fi
    local candidate
    for candidate in \
        "$TOOIE_ROOT/../Banjo-Tooie (USA).z64" \
        "$TOOIE_ROOT/../Banjo-Tooie (USA).n64"
    do
        if [[ -f "$candidate" ]]; then
            printf '%s' "$candidate"
            return
        fi
    done
    fail "set TOOIE_CART_Z64 to the NTSC-U Rev. 0 cartridge (SHA-1 $ROM_SHA1). Do not use the decompressed TooieRecomp build ROM."
}

ROM="$(resolve_rom)"
HAVE_SHA1="$(sha1sum "$ROM" | awk '{print $1}')"
[[ "$HAVE_SHA1" == "$ROM_SHA1" ]] || fail "ROM SHA-1 $HAVE_SHA1 != $ROM_SHA1 ($ROM)"

mkdir -p "$(dirname "$CFG_RUNTIME")"
sed \
    -e "s|@ORACLE_SHARE@|$SHARE_DIR|g" \
    -e "s|@ORACLE_PLUGINDIR@|$PLUGIN_DIR|g" \
    -e "s|@ORACLE_GFX@|$GFX_SO|g" \
    -e "s|@ORACLE_INPUT@|$INPUT_SO|g" \
    -e "s|@ORACLE_RSP@|$RSP_SO|g" \
    "$CFG_TEMPLATE" > "$CFG_RUNTIME"

AUDIO_MODE="${TOOIE_ANGRYLION_AUDIO:-dummy}"
case "$AUDIO_MODE" in
    dummy) AUDIO_PLUGIN="dummy" ;;
    sdl|audio) AUDIO_PLUGIN="$AUDIO_SO" ;;
    *) fail "TOOIE_ANGRYLION_AUDIO must be dummy or sdl (got $AUDIO_MODE)" ;;
esac

echo "ANGRYLION_CONTROL"
echo "  ui:     $MUPEN_BIN"
echo "  gfx:    $GFX_SO"
echo "  rsp:    $RSP_SO"
echo "  audio:  $AUDIO_MODE"
echo "  cpu:    Pure Interpreter (--emumode 0)"
echo "  rom:    $ROM"
echo "  sha1:   $HAVE_SHA1"
echo "  renderer of TooieRecomp: RT64 (unchanged)"

exec "$MUPEN_BIN" \
    --corelib "$CORE_LIB" \
    --configdir "$(dirname "$CFG_RUNTIME")" \
    --datadir "$SHARE_DIR" \
    --plugindir "$PLUGIN_DIR" \
    --gfx "$GFX_SO" \
    --rsp "$RSP_SO" \
    --audio "$AUDIO_PLUGIN" \
    --input "$INPUT_SO" \
    --emumode 0 \
    --nosaveoptions \
    "$@" \
    "$ROM"
