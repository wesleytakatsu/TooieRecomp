#!/usr/bin/env bash
# Gate: RecompiledFuncs/ must match metadata/recomp-output.sha256 (sha256 of the 420 generated files accepted as the functional baseline:
# produced by the ELF route and reproduced byte-for-byte by the kit-free symbols route). Hashes only, no generated code or ROM bytes.
#   tools/verify_generated.sh [dir]            verify (default dir: <repo>/RecompiledFuncs)
#   tools/verify_generated.sh --update [dir]   MAINTAINER: rewrite the manifest after an intentional generator/metadata change (review the diff!)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
upd=0; if [[ "${1:-}" == --update ]]; then upd=1; shift; fi
DIR="$(cd "${1:-$ROOT/RecompiledFuncs}" && pwd)"; M="$ROOT/metadata/recomp-output.sha256"
if (( upd )); then ( cd "$DIR" && LC_ALL=C sha256sum -- * | LC_ALL=C sort -k2 ) > "$M"; echo "verify_generated: wrote $(wc -l < "$M") entries to $M"; exit 0; fi
[[ -f "$M" ]] || { echo "verify_generated: missing $M"; exit 1; }
n="$(cd "$DIR" && ls | wc -l)"; want="$(wc -l < "$M")"
[[ "$n" == "$want" ]] || { echo "verify_generated: FAIL: $DIR has $n files, manifest lists $want"; exit 1; }
( cd "$DIR" && sha256sum --quiet -c "$M" ) || { echo "verify_generated: FAIL: generated output differs from the accepted baseline"; exit 1; }
echo "verify_generated: OK $want files match metadata/recomp-output.sha256"
