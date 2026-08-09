#!/usr/bin/env bash
# Clean regenerate RecompiledFuncs (P04).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WS="$(cd "$ROOT/.." && pwd)"
N64R="$WS/BanjoRecomp/lib/N64ModernRuntime/N64Recomp/build/N64Recomp"
cd "$ROOT"
[[ -x "$N64R" ]] || { echo "missing N64Recomp at $N64R"; exit 1; }
[[ -f tooie.us.rev0.recomp.toml ]] || { echo "missing tooie.us.rev0.recomp.toml"; exit 1; }
rm -rf RecompiledFuncs
mkdir -p RecompiledFuncs
"$N64R" tooie.us.rev0.recomp.toml
echo "generated $(ls RecompiledFuncs | wc -l) files ($(du -sh RecompiledFuncs | awk '{print $1}'))"
