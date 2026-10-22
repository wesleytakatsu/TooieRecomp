#!/usr/bin/env bash
# Prepare the decompressed ROM image the runtime launcher needs, from the private retail ROM. Uses the vendored decomp/tools/rom_decompressor,
# built from a COPY under <build>/extract (decomp/ is never written). Needs host g++, make, zlib and libfmt-dev only - NOT the legacy IDO/gcc kit.
#   tools/extract_rom.sh [rom]     rom default: $TOOIE_ROM or "<repo>/Banjo-Tooie (USA).z64"
# Output: $TOOIE_BUILD_DIR(default <repo>/build-linux)/extract/decompressed.us.z64  (git-ignored; ROM-derived, never commit or package it).
# The ROM is validated (tools/rom_check.sh) BEFORE any output is touched; the result is verified (size + SHA1) in a temp file and then renamed,
# so a failure never leaves a half-written or wrong output behind. No cache is reused: the 0.2 s decompression always reruns.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# shellcheck disable=SC1091
source "$ROOT/tools/toolchain.lock"; source "$ROOT/tools/paths.sh"
ROM="$(readlink -f "${1:-$TOOIE_ROM}")"
"$ROOT/tools/rom_check.sh" "$ROM"
for t in g++ make; do command -v "$t" >/dev/null || { echo "extract_rom: missing $t (apt install build-essential libfmt-dev zlib1g-dev)"; exit 1; }; done
OUT="$BUILD/extract"; WORK="$OUT/work"
mkdir -p "$OUT"; rm -rf "$WORK"; mkdir -p "$WORK"
cp -a "$ROOT/decomp/tools" "$WORK/tools"
find "$WORK/tools" \( -name '*.o' -o -name '*.a' \) -delete; rm -f "$WORK/tools/rom_decompressor"
make -C "$WORK/tools" rom_decompressor >"$OUT/rom_decompressor.build.log" 2>&1 || { echo "extract_rom: rom_decompressor build failed, see $OUT/rom_decompressor.build.log"; exit 1; }
TMP="$OUT/.decompressed.us.z64.tmp"; rm -f "$TMP"
trap 'rm -f "$TMP"' EXIT
# rom_decompressor ALSO writes assets/overlays/*/relocs.bin relative to its CWD: run it inside $WORK so nothing lands in the repo (removed below).
( cd "$WORK" && "$WORK/tools/rom_decompressor" "$ROM" "$TMP" ) >"$OUT/rom_decompressor.run.log" 2>&1 || { echo "extract_rom: rom_decompressor failed, see $OUT/rom_decompressor.run.log"; exit 1; }
"$ROOT/tools/rom_check.sh" --decompressed "$TMP"
mv -f "$TMP" "$OUT/decompressed.us.z64"; trap - EXIT
rm -rf "$WORK"   # also drops the relocs.bin side outputs
echo "extract_rom: wrote $OUT/decompressed.us.z64"
echo "extract_rom: launcher: start ./TooieRecompiled (or run-tooie.sh), click the game's ROM select, choose THAT decompressed.us.z64 (NOT the retail .z64: the launcher hashes the file it is given against the decompressed image)."
