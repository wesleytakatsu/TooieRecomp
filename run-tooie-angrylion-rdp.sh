#!/usr/bin/env bash
# Launch ANGRYLION_CONTROL for visual / parity tests:
# Mupen64Plus Pure Interpreter + Angrylion RDP Plus + CXD4 LLE.
#
# This is NOT TooieRecompiled. The port stays on RT64 (./run-tooie.sh).
# Angrylion is a Mupen video plugin and cannot be loaded into RT64.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ORACLE_RUN="$HERE/tools/oracle/angrylion-control/run.sh"

usage() {
  cat <<'USAGE'
Usage: ./run-tooie-angrylion-rdp.sh [options] [-- extra mupen64plus args]

Starts Banjo-Tooie in Mupen with the pinned Angrylion RDP Plus + CXD4 LLE
plugins (ANGRYLION_CONTROL). For the native RT64 port use ./run-tooie.sh.

Options:
  -h, --help          Show this help
  --measure           Dummy audio (capture/soak; same as tools/oracle run.sh)
  --no-audio          Same as --measure
  -f, --fullscreen    Fullscreen
  -w, --windowed      Windowed (default)
  -r, --resolution WxH
                      Output size (default 640x480)

Environment:
  TOOIE_CART_Z64      Cartridge path (SHA-1 af1a89e12b638b8d82cc4c085c8e01d4cba03fb3)
  TOOIE_ANGRYLION_AUDIO   sdl (default here) or dummy

Examples:
  ./run-tooie-angrylion-rdp.sh
  ./run-tooie-angrylion-rdp.sh --measure
  ./run-tooie-angrylion-rdp.sh -- --set 'Video-AngrylionPlus[ViVsync]=False'
USAGE
}

if [[ ! -x "$ORACLE_RUN" ]]; then
  echo "error: oracle launcher not found: $ORACLE_RUN" >&2
  echo "See docs/specs/BT/angrylion-control-oracle.md" >&2
  exit 1
fi

AUDIO="${TOOIE_ANGRYLION_AUDIO:-sdl}"
WINDOW="--windowed"
RESOLUTION="640x480"
EXTRA=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help) usage; exit 0 ;;
    --measure|--no-audio) AUDIO=dummy; shift ;;
    -f|--fullscreen) WINDOW="--fullscreen"; shift ;;
    -w|--windowed) WINDOW="--windowed"; shift ;;
    -r|--resolution)
      [[ $# -ge 2 ]] || { echo "error: $1 needs WxH" >&2; exit 1; }
      RESOLUTION="$2"
      shift 2
      ;;
    --) shift; EXTRA+=("$@"); break ;;
    *) EXTRA+=("$1"); shift ;;
  esac
done

export TOOIE_ANGRYLION_AUDIO="$AUDIO"
echo "[run-tooie-angrylion-rdp] Mupen Pure Interpreter + Angrylion RDP Plus + CXD4"
echo "[run-tooie-angrylion-rdp] audio=$AUDIO  $WINDOW  ${RESOLUTION}"
echo "[run-tooie-angrylion-rdp] TooieRecomp renderer remains RT64 (./run-tooie.sh)"
exec "$ORACLE_RUN" "$WINDOW" --resolution "$RESOLUTION" "${EXTRA[@]}"
