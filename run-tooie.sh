#!/usr/bin/env bash
# Launch TooieRecompiled (RT64). Lives in TooieRecomp/; the monorepo-root
# run-tooie.sh execs this file.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${TOOIE_BUILD_DIR:-$HERE/build}"
BIN="$BUILD_DIR/TooieRecompiled"

usage() {
  cat <<'USAGE'
Usage: ./run-tooie.sh [options] [-- extra args for TooieRecompiled]

Starts TooieRecompiled with RT64. This is the TooieRecomp port, not the
Angrylion oracle. For Mupen + Angrylion RDP Plus + CXD4 LLE:

  ./run-tooie-angrylion-rdp.sh

Options:
  -h, --help            Show this help
  -a, --auto-start      Start game immediately (TOOIE_AUTO_START=1)
  -A, --attract         Auto-start + attract diagnostics
                        (TOOIE_AUTO_START=1 TOOIE_DIAG_ATTRACT=1)
  -d, --diag            Enable diagnostics (TOOIE_DIAG=1)
  -m, --launcher-music  Allow hidden launcher music preview (default: off)
  -n, --no-auto         Stay on launcher (default)

Environment (optional overrides):
  TOOIE_BUILD_DIR, TOOIE_AUTO_START, TOOIE_DIAG, TOOIE_DIAG_ATTRACT,
  TOOIE_DISABLE_LAUNCHER_MUSIC, TOOIE_DIAG_INTERVAL_MS

Examples:
  ./run-tooie.sh                 # launcher UI
  ./run-tooie.sh -a              # boot straight into game
  ./run-tooie.sh -A              # attract/demo capture (P08)
  ./run-tooie.sh -a -d           # auto-start + diag
USAGE
}

AUTO=0
DIAG=0
ATTRACT=0
LAUNCHER_MUSIC=0
EXTRA=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help) usage; exit 0 ;;
    -a|--auto-start) AUTO=1; shift ;;
    -A|--attract) AUTO=1; ATTRACT=1; shift ;;
    -d|--diag) DIAG=1; shift ;;
    -m|--launcher-music) LAUNCHER_MUSIC=1; shift ;;
    -n|--no-auto) AUTO=0; shift ;;
    --) shift; EXTRA+=("$@"); break ;;
    *) EXTRA+=("$1"); shift ;;
  esac
done

if [[ ! -x "$BIN" ]]; then
  echo "error: executable not found: $BIN" >&2
  echo "Build first, e.g.: cmake --build \"$BUILD_DIR\" -j\"\$(nproc)\"" >&2
  exit 1
fi

export TOOIE_AUTO_START="${TOOIE_AUTO_START:-$AUTO}"
export TOOIE_DIAG="${TOOIE_DIAG:-$DIAG}"
if [[ "$ATTRACT" -eq 1 ]]; then
  export TOOIE_DIAG_ATTRACT="${TOOIE_DIAG_ATTRACT:-1}"
fi

if [[ "$LAUNCHER_MUSIC" -eq 1 ]]; then
  unset TOOIE_DISABLE_LAUNCHER_MUSIC 2>/dev/null || true
  export TOOIE_DISABLE_LAUNCHER_MUSIC="${TOOIE_DISABLE_LAUNCHER_MUSIC:-0}"
else
  export TOOIE_DISABLE_LAUNCHER_MUSIC="${TOOIE_DISABLE_LAUNCHER_MUSIC:-1}"
fi

cd "$BUILD_DIR"
echo "[run-tooie] $BIN  (RT64)"
echo "[run-tooie] AUTO_START=$TOOIE_AUTO_START DIAG=$TOOIE_DIAG DIAG_ATTRACT=${TOOIE_DIAG_ATTRACT:-0} DISABLE_LAUNCHER_MUSIC=$TOOIE_DISABLE_LAUNCHER_MUSIC"
exec "$BIN" "${EXTRA[@]}"
