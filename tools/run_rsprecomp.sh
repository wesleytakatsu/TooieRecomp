#!/usr/bin/env bash
# Regenerate rsp/n_aspMain.cpp from Tooie decompressed ROM via RSPRecomp (P07).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WS="$(cd "$ROOT/.." && pwd)"
RSP="${RSPRECOMP:-$WS/BanjoRecomp/lib/N64ModernRuntime/N64Recomp/build/RSPRecomp}"
CFG="$ROOT/n_aspMain.us.rev0.toml"
ROM="${TOOIE_DECOMP_ROM:-$WS/banjo-tooie-Recomp/decompressed.us.z64}"

cd "$ROOT"
[[ -x "$RSP" ]] || { echo "missing RSPRecomp at $RSP (build N64Recomp tools first)"; exit 1; }
[[ -f "$CFG" ]] || { echo "missing $CFG"; exit 1; }
[[ -f "$ROM" ]] || { echo "missing decompressed ROM at $ROM"; exit 1; }

# Config points at ../banjo-tooie-Recomp/decompressed.us.z64 by default.
"$RSP" "$CFG"
# Tooie n_aspMain is larger than IMEM window; RSPRecomp masks branch labels
# with 0x1FFF — fix L_005C→L_205C style mismatches.
python3 "$ROOT/tools/fix_rsprecomp_labels.py" "$ROOT/rsp/n_aspMain.cpp" --write
echo "generated $(wc -l < rsp/n_aspMain.cpp) lines → rsp/n_aspMain.cpp"
