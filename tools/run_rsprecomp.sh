#!/usr/bin/env bash
# Regenerate rsp/n_aspMain.cpp from Tooie decompressed ROM via RSPRecomp (P07).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# shellcheck disable=SC1091
source "$ROOT/tools/paths.sh"
RSP="${RSPRECOMP:-$N64RECOMP_DIR/RSPRecomp}"
CFG="$ROOT/n_aspMain.us.rev0.toml"
ROM="${TOOIE_DECOMP_ROM:-$DECOMPRESSED_ROM}"

cd "$ROOT"
[[ -x "$RSP" ]] || { echo "missing RSPRecomp at $RSP (build N64Recomp tools first)"; exit 1; }
[[ -f "$CFG" ]] || { echo "missing $CFG"; exit 1; }
[[ -f "$ROM" ]] || { echo "missing decompressed ROM at $ROM"; exit 1; }

# Tracked config points at decomp/decompressed.us.z64; the image lives in the build tree (tools/extract_rom.sh): retarget in an untracked copy.
sed "s#^rom_file_path = .*#rom_file_path = \"$ROM\"#" "$CFG" > "$ROOT/.n_aspMain.gen.toml"; CFG="$ROOT/.n_aspMain.gen.toml"
"$RSP" "$CFG"
# Tooie n_aspMain is larger than IMEM window; RSPRecomp masks branch labels
# with 0x1FFF — fix L_005C→L_205C style mismatches.
python3 "$ROOT/tools/fix_rsprecomp_labels.py" "$ROOT/rsp/n_aspMain.cpp" --write
echo "generated $(wc -l < rsp/n_aspMain.cpp) lines → rsp/n_aspMain.cpp"
