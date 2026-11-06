#!/usr/bin/env bash
# Cross-compiles the handheld build (CMake preset linux-arm64-handheld, TK64_MOBILE) for aarch64 Linux from a Linux x64 host.
# On Windows use build-handheld.ps1.
#   HANDHELD_SYSROOT=<dir> tools/handheld/build-handheld.sh [--configure] [--device rk3326] [--file-to-c <path>] [--target TooieRecompiled]... [-j N]
#   --target can be given several times; the default is TooieRecompiled.
# Needs clang, lld and llvm-ar/ranlib/strip on PATH (or HANDHELD_LLVM=<their folder>), cmake 3.21+, ninja and python3.
# Sysroot: python3 -I tools/handheld/fetch_sysroot.py <dir>. file_to_c: built for the host, by default from build-linux
# (tools/build_linux.sh configure build).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
CONFIGURE=0; DEVICE=""; FILE_TO_C=""; TARGETS=(); JOBS=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        --configure) CONFIGURE=1 ;;
        --device) DEVICE="$2"; shift ;;
        --file-to-c) FILE_TO_C="$2"; shift ;;
        --target) TARGETS+=("$2"); shift ;;
        -j) JOBS=(-j "$2"); shift ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
    shift
done
export HANDHELD_SYSROOT="${HANDHELD_SYSROOT:-${XU10_SYSROOT:-}}"
[[ -n "$HANDHELD_SYSROOT" ]] || { echo "Set HANDHELD_SYSROOT (python3 -I tools/handheld/fetch_sysroot.py <dir> makes one)." >&2; exit 1; }
export HANDHELD_LLVM="${HANDHELD_LLVM:-}"
export HANDHELD_PKG_CONFIG="$ROOT/tools/handheld/pkg-config.sh"
BUILD="$ROOT/build-handheld"
if [[ $CONFIGURE -eq 1 || ! -f "$BUILD/build.ninja" ]]; then
    FILE_TO_C="${FILE_TO_C:-$ROOT/build-linux/rt64/src/tools/file_to_c/file_to_c}"
    [[ -x "$FILE_TO_C" ]] || { echo "file_to_c not found: $FILE_TO_C (build Linux x64 first: tools/build_linux.sh configure build)" >&2; exit 1; }
    export TK64_HOST_FILE_TO_C="$(cd "$(dirname "$FILE_TO_C")" && pwd)/$(basename "$FILE_TO_C")"
    PRESET_ARGS=(--preset linux-arm64-handheld)
    [[ -z "$DEVICE" ]] || PRESET_ARGS+=("-DHANDHELD_DEVICE=$DEVICE")
    (cd "$ROOT" && cmake "${PRESET_ARGS[@]}")
fi
[[ ${#TARGETS[@]} -gt 0 ]] || TARGETS=(TooieRecompiled)
ninja -C "$BUILD" "${JOBS[@]}" -k 0 "${TARGETS[@]}"
