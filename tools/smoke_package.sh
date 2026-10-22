#!/usr/bin/env bash
# Launch smoke for a staged/extracted package dir in a fully isolated HOME/XDG (no user config or saves touched).
# Passes if the process stays alive for $SECS seconds (default 15) without crashing. Terminates ONLY its own PID.
#   tools/smoke_package.sh <package dir> [secs]
set -uo pipefail
PKG="$(cd "${1:?package dir}" && pwd)"; SECS="${2:-15}"
T="$(mktemp -d /tmp/tooie-smoke.XXXXXX)"
mkdir -p "$T/home" "$T/cfg" "$T/data" "$T/cache"
# TOOIE_SMOKE_LD_LIBRARY_PATH (e.g. /lib/x86_64-linux-gnu) makes the loader prefer the distro libraries for THIS process only;
# nothing global is changed. The mapped libz is printed so the run can be tied to the distro zlib.
LDP=(); [[ -z "${TOOIE_SMOKE_LD_LIBRARY_PATH:-}" ]] || LDP=(LD_LIBRARY_PATH="$TOOIE_SMOKE_LD_LIBRARY_PATH")
env -i "${LDP[@]}" PATH="$PATH" DISPLAY="${DISPLAY:-}" XAUTHORITY="${XAUTHORITY:-}" WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-}" XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-}" \
    HOME="$T/home" XDG_CONFIG_HOME="$T/cfg" XDG_DATA_HOME="$T/data" XDG_CACHE_HOME="$T/cache" \
    "$PKG/TooieRecompiled" >"$T/stdout.log" 2>"$T/stderr.log" &
PID=$!
sleep "$SECS"
if kill -0 "$PID" 2>/dev/null; then
    echo "mapped libz: $(awk '/libz\.so/ {print $6}' /proc/$PID/maps | sort -u | tr '\n' ' ')"
    cmd="$(tr '\0' ' ' < /proc/$PID/cmdline)"
    [[ "$cmd" == "$PKG/TooieRecompiled"* ]] || { echo "SMOKE: refusing to signal unexpected pid $PID ($cmd)"; exit 2; }
    kill -TERM "$PID"; for _ in 1 2 3 4 5 6 7 8 9 10; do kill -0 "$PID" 2>/dev/null || break; sleep 1; done
    kill -0 "$PID" 2>/dev/null && kill -KILL "$PID"
    wait "$PID" 2>/dev/null; echo "SMOKE PASS: alive ${SECS}s, terminated own pid $PID (logs: $T)"; rc=0
else
    wait "$PID"; echo "SMOKE FAIL: exited early rc=$? (logs: $T)"; tail -5 "$T/stderr.log"; rc=1
fi
echo "files created in isolated cfg/data:"; find "$T/cfg" "$T/data" "$T/home" -type f | head -20
exit $rc
