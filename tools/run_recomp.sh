#!/usr/bin/env bash
# Clean regenerate RecompiledFuncs (P04).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# shellcheck disable=SC1091
source "$ROOT/tools/paths.sh"
N64R="${N64RECOMP:-$N64RECOMP_DIR/N64Recomp}"
cd "$ROOT"
[[ -x "$N64R" ]] || { echo "missing N64Recomp at $N64R"; exit 1; }
[[ -f tooie.us.rev0.recomp.toml ]] || { echo "missing tooie.us.rev0.recomp.toml"; exit 1; }
# The tracked config points at decomp/decompressed.us.z64; the decompressed ROM lives in the build tree (tools/extract_rom.sh), so retarget it
# in an untracked copy (tracked files are never edited). Kit-free: input = decompressed ROM + metadata/tooie.us.syms.toml, no ELF.
CFG=.tooie.recomp.gen.toml
sed "s#^rom_file_path = .*#rom_file_path = \"$DECOMPRESSED_ROM\"#" tooie.us.rev0.recomp.toml > "$CFG"
# Preflight: never delete RecompiledFuncs unless every generator input exists and the ROM image is exactly the expected one.
"$ROOT/tools/rom_check.sh" --decompressed "$DECOMPRESSED_ROM" || { echo "run tools/build_linux.sh extract first (RecompiledFuncs left untouched)"; exit 1; }
[[ -f metadata/tooie.us.syms.toml ]] || { echo "missing metadata/tooie.us.syms.toml (RecompiledFuncs left untouched)"; exit 1; }
rm -rf RecompiledFuncs
mkdir -p RecompiledFuncs
"$N64R" "$CFG"
echo "generated $(ls RecompiledFuncs | wc -l) files ($(du -sh RecompiledFuncs | awk '{print $1}'))"
