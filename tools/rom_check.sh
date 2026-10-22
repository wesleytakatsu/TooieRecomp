#!/usr/bin/env bash
# Strict, read-only validation of the private input ROM. Never writes, never copies, never prints ROM bytes.
#   tools/rom_check.sh [rom]            raw Banjo-Tooie (USA) rev 0 .z64: size + MD5 + SHA1 from tools/toolchain.lock (default: $TOOIE_ROM or repo-root file)
#   tools/rom_check.sh --decompressed <file>   decompressed.us.z64 produced by tools/extract_rom.sh: size + SHA1
# Exit 0 = exact match. Exit 1 = missing/wrong. Also usable via `source` is NOT supported; run it.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# shellcheck disable=SC1091
source "$ROOT/tools/toolchain.lock"; source "$ROOT/tools/paths.sh"
mode=raw
if [[ "${1:-}" == --decompressed ]]; then mode=dec; shift; fi
if [[ $mode == dec ]]; then f="${1:-}"; [[ -n "$f" ]] || { echo "rom_check: --decompressed needs a file" >&2; exit 2; }
else f="${1:-$TOOIE_ROM}"; fi
fail() { echo "rom_check: FAIL: $*" >&2; exit 1; }
if [[ ! -f "$f" ]]; then
    if [[ $mode == dec ]]; then fail "not a file: $f (create it with tools/build_linux.sh extract)"; else fail "not a file: $f (put your ROM at \"$ROOT/$ROM_BASENAME\" or set TOOIE_ROM)"; fi
fi
size="$(stat -c %s "$f")"
if [[ $mode == dec ]]; then
    [[ "$size" == "$DECOMPRESSED_ROM_SIZE" ]] || fail "size $size != $DECOMPRESSED_ROM_SIZE: $f"
    got="$(sha1sum "$f" | cut -d' ' -f1)"; [[ "$got" == "$DECOMPRESSED_ROM_SHA1" ]] || fail "SHA1 $got != $DECOMPRESSED_ROM_SHA1: $f"
    echo "rom_check: OK decompressed $size bytes sha1 $got"; exit 0
fi
[[ "$size" == "$ROM_SIZE" ]] || fail "size $size != $ROM_SIZE (need Banjo-Tooie USA rev 0, big-endian .z64): $f"
md5="$(md5sum "$f" | cut -d' ' -f1)"; [[ "$md5" == "$ROM_MD5" ]] || fail "MD5 $md5 != $ROM_MD5: $f"
sha="$(sha1sum "$f" | cut -d' ' -f1)"; [[ "$sha" == "$ROM_SHA1" ]] || fail "SHA1 $sha != $ROM_SHA1: $f"
echo "rom_check: OK $size bytes md5 $md5 sha1 $sha"
