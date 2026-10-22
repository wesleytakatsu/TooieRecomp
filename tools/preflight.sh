#!/usr/bin/env bash
# Fail-fast environment/input check. No side effects, no network.
#   tools/preflight.sh [build|generate|decomp]   (default: generate = build + ROM + generation tools; kit-free)
#   decomp = generate + the MAINTAINER-ONLY legacy compiler kit / MIPS toolchain checks (tools/build_linux.sh decomp, metadata regeneration)
# Env: TOOIE_ROM=<path to Banjo-Tooie (USA) .z64> (defaults to the repo-root file); TOOIE_TOOLCHAIN_KIT only for 'decomp'
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# shellcheck disable=SC1091
source "$ROOT/tools/toolchain.lock"; source "$ROOT/tools/paths.sh"
STAGE="${1:-generate}"
bad=0
need() { if command -v "$1" >/dev/null 2>&1; then printf 'ok   %-24s %s\n' "$1" "$("$1" --version 2>&1 | head -1)"; else printf 'MISS %-24s (%s)\n' "$1" "$2"; bad=1; fi; }
pkg()  { if pkg-config --exists "$1" 2>/dev/null; then printf 'ok   %-24s %s\n' "$1" "$(pkg-config --modversion "$1")"; else printf 'MISS %-24s (apt: %s)\n' "$1" "$2"; bad=1; fi; }

echo "== vendored sources (offline integrity)"
"$ROOT/tools/vendor_verify.sh" >/dev/null 2>&1 && echo "ok   tools/vendor_verify.sh" || { echo "BAD  tools/vendor_verify.sh fails: run it for details"; bad=1; }
echo "== tools"; need cmake "cmake >= 3.20"; need ninja "ninja-build"; need clang "clang (GCC is rejected by CMakeLists.txt)"; need clang++ "clang"
need ld.lld "lld"; need pkg-config "pkg-config"
echo "== libraries"; pkg sdl2 libsdl2-dev; pkg freetype2 libfreetype-dev; pkg gtk+-3.0 libgtk-3-dev
if [[ "$STAGE" == generate || "$STAGE" == decomp ]]; then
    echo "== generate-stage tools (extract + N64Recomp build; no compiler kit)"
    need make make; need g++ "g++ (build-essential)"; need python3 python3
    dpkg -s libfmt-dev zlib1g-dev >/dev/null 2>&1 && echo "ok   libfmt-dev zlib1g-dev" || { echo "MISS libfmt-dev/zlib1g-dev (tools/extract_rom.sh)"; bad=1; }
fi
if [[ "$STAGE" == decomp ]]; then
    echo "== maintainer-only decomp tools (Linux/WSL2 only)"
    need mips-linux-gnu-gcc gcc-mips-linux-gnu; need mips-linux-gnu-ld binutils-mips-linux-gnu; need mips-linux-gnu-objcopy binutils-mips-linux-gnu
    python3 -c 'import venv, ensurepip' 2>/dev/null && echo "ok   python3-venv" || { echo "MISS python3-venv"; bad=1; }
    [[ "$(python3 -c 'import sys; print("cp%d%d" % sys.version_info[:2])' 2>/dev/null)" == "${PYTHON_TOOLING_ABI%%-*}" ]] \
        && echo "ok   python ABI matches the wheelhouse ($PYTHON_TOOLING_ABI)" || { echo "BAD  python is not ${PYTHON_TOOLING_ABI%%-*}: tools/wheelhouse only holds $PYTHON_TOOLING_ABI wheels"; bad=1; }
    ( cd "$ROOT/tools/wheelhouse" && sha256sum --quiet -c SHA256SUMS ) 2>/dev/null && echo "ok   wheelhouse hashes" || { echo "BAD  tools/wheelhouse does not match SHA256SUMS"; bad=1; }
    dpkg -s libtoml11-dev >/dev/null 2>&1 && echo "ok   libtoml11-dev" || { echo "MISS libtoml11-dev (decomp tools)"; bad=1; }
    echo "== legacy compiler kit (explicit input, never downloaded)"
    if [[ -n "${TOOIE_TOOLCHAIN_KIT:-}" && -d "$TOOIE_TOOLCHAIN_KIT" ]]; then
        "$ROOT/tools/toolchain_kit.sh" verify "$TOOIE_TOOLCHAIN_KIT" >/dev/null 2>&1 && echo "ok   kit $TOOIE_TOOLCHAIN_KIT ($(wc -l < "$ROOT/$TOOLCHAIN_FILES_SHA256") files verified)" || { echo "BAD  kit does not match tools/toolchain.sha256"; bad=1; }
    elif [[ -x "$DECOMP_ROOT/tools/ido/cc" && -x "$DECOMP_ROOT/lib/ultralib/tools/gcc/gcc" ]]; then echo "note decomp work tree already has the compilers ($DECOMP_ROOT)"
    else echo "MISS TOOIE_TOOLCHAIN_KIT (see tools/toolchain.lock for the exact inputs)"; bad=1; fi
fi
if [[ "$STAGE" == generate || "$STAGE" == decomp ]]; then
    echo "== ROM"
    if "$ROOT/tools/rom_check.sh" "$TOOIE_ROM"; then
        # In-repo input is allowed ONLY as the exact canonical root file, and only while git ignores it and does not track it.
        case "$(readlink -f "$TOOIE_ROM")" in
            "$ROOT/$ROM_BASENAME") ;;
            "$ROOT"/*) echo "BAD  ROM inside the repo tree must be exactly \"$ROOT/$ROM_BASENAME\""; bad=1;;
        esac
        if [[ "$(readlink -f "$TOOIE_ROM")" == "$ROOT"/* ]]; then
            if [[ "$(git -C "$ROOT" rev-parse --show-toplevel 2>/dev/null)" == "$ROOT" ]]; then
                git -C "$ROOT" check-ignore -q -- "$TOOIE_ROM" && echo "ok   ROM is git-ignored" || { echo "BAD  ROM inside the repo is not git-ignored"; bad=1; }
                [[ -z "$(git -C "$ROOT" ls-files -- "$TOOIE_ROM")" ]] || { echo "BAD  ROM is tracked by git"; bad=1; }
            else # source archive / export without .git: canonical path + strict content only; the packagers refuse any .z64 (tools/package_source.sh)
                echo "note no git checkout at $ROOT: canonical ROM path and content accepted; keep it out of any archive you publish"
            fi
        fi
    else echo "MISS/BAD TOOIE_ROM ($TOOIE_ROM): need your own Banjo-Tooie (USA).z64, default \"$ROOT/$ROM_BASENAME\"; never copied into git or packages"; bad=1; fi
fi
# decomp/lib/ultralib/tools/compile_sjis.py picks the first argv containing ".c" as the C file, so any
# decomp work path containing ".c" (e.g. ~/.cache/..., ~/.config/...) breaks `make` with FileNotFoundError.
if [[ "$STAGE" == decomp && "$DECOMP_ROOT" == *.c* ]]; then
    echo "BAD  decomp work path '$DECOMP_ROOT' contains '.c' (upstream ultralib compile_sjis.py defect). Use a checkout/TOOIE_BUILD_DIR path without '.c'"; bad=1
fi
echo "== disk (GB free in repo fs)"; df -BG --output=avail "$ROOT" | tail -1
(( bad )) && { echo "PREFLIGHT FAIL"; exit 1; }; echo "PREFLIGHT OK"
