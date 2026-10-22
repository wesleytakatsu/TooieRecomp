#!/usr/bin/env bash
# Verify / install the explicit legacy-compiler kit (IDO 5.3 + gcc 2.7.2/binutils) used only by the ROM decomp stage. Never downloads.
#   tools/toolchain_kit.sh verify  <kit-dir>            check every file against tools/toolchain.sha256
#   tools/toolchain_kit.sh install <kit-dir> <decomp-dir>   verify, then copy ido/ -> <decomp>/tools/ido and <decomp>/lib/ultralib/tools/{ido,gcc}
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cmd="${1:-}"; kit="${2:-}"
[[ -n "$cmd" && -d "$kit" ]] || { sed -n '2,5p' "$0"; exit 2; }
verify() {
    ( cd "$kit" && sha256sum --quiet -c "$ROOT/tools/toolchain.sha256" ) || { echo "toolchain kit $kit does not match tools/toolchain.sha256" >&2; exit 1; }
    echo "toolchain kit OK ($(wc -l < "$ROOT/tools/toolchain.sha256") files)"
}
case "$cmd" in
    verify) verify ;;
    install)
        dec="${3:-}"; [[ -d "$dec" ]] || { echo "install needs <decomp-dir>" >&2; exit 2; }
        verify
        mkdir -p "$dec/tools/ido" "$dec/lib/ultralib/tools/ido" "$dec/lib/ultralib/tools/gcc"
        cp -a "$kit/ido/." "$dec/tools/ido/"; cp -a "$kit/ido/." "$dec/lib/ultralib/tools/ido/"; cp -a "$kit/gcc/." "$dec/lib/ultralib/tools/gcc/"
        echo "installed into $dec" ;;
    *) exit 2 ;;
esac
