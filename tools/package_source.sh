#!/usr/bin/env bash
# Standalone SOURCE archive: the git-visible working tree (tracked + untracked-not-ignored) as REGULAR files, minus personal agent/editor
# configs and local scratch. Contains the vendored lib/ decomp/ assets/ trees (so it builds offline) but NEVER a ROM, extracted assets,
# generated RecompiledFuncs, build outputs, game binaries or the legacy compiler kit. Symlinks are dropped (assets/ is a real directory).
#   tools/package_source.sh [OUTDIR=dist]      -> OUTDIR/TooieRecomp-source-<head>[-dirty].tar.gz (+ .sha256, .list)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; cd "$ROOT"
OUT="${1:-$ROOT/dist}"; mkdir -p "$OUT"
export LC_ALL=C
# Personal / agent / editor state and local-only files (regex on repo-relative path). Keep in sync with docs/specs/Tooie/packaging-and-dependencies.md.
EXCL='^(\.claude|\.cursor|\.gemini|\.grok|\.serena|\.opencode|\.agents|\.codex|\.idea|graft)/|^(GEMINI|CLAUDE)\.md$|^\.ignore$|^opencode(\.json|-compaction\.sh)$|^\.mcp\.json$|^\.vscode/(mcp|settings)\.json$|^cpu_math_check$|(^|/)\.gitrepo$|^dist/|^build[^/]*/|^RecompiledFuncs/'
head="$(git rev-parse --short=8 HEAD)"; dirty=""; [[ -z "$(git status --porcelain)" ]] || dirty="-dirty"
name="TooieRecomp-source-$head$dirty"
L="$OUT/$name.list"
git ls-files -z --cached --others --exclude-standard | tr '\0' '\n' | sort -u |
  while IFS= read -r f; do if [[ -f "$f" && ! -L "$f" ]] && ! grep -Eq "$EXCL" <<<"$f"; then printf '%s\n' "$f"; fi; done > "$L"
# Hard refusals: private/generated content must never be in the list.
if grep -Eiq '\.(z64|n64|v64|elf|rom|sav|eep|core)$|(^|/)(baserom\.us\.z64|decompressed\.[^/]*)$|(^|/)RecompiledFuncs/|^decomp/(build|asm|bin)/|^decomp/tools/(ido|gcc)/|^assets/overlays/.*\.bin$' "$L"; then
    echo "package_source: refusing, private/generated path in list:"; grep -Ei '\.(z64|n64|v64|elf|rom|sav|eep|core)$|(^|/)(baserom\.us\.z64|decompressed\.[^/]*)$|(^|/)RecompiledFuncs/|^decomp/(build|asm|bin)/|^decomp/tools/(ido|gcc)/|^assets/overlays/.*\.bin$' "$L" | head; exit 1; rm -f "$L"; exit 1
fi
tar --sort=name --owner=0 --group=0 --numeric-owner --mtime="@$(git log -1 --format=%ct)" --no-recursion -czf "$OUT/$name.tar.gz" --transform "s,^,$name/," -T "$L"
( cd "$OUT" && sha256sum "$name.tar.gz" > "$name.tar.gz.sha256" )
echo "$OUT/$name.tar.gz  files=$(wc -l < "$L")  bytes=$(stat -c %s "$OUT/$name.tar.gz")"
cat "$OUT/$name.tar.gz.sha256"
