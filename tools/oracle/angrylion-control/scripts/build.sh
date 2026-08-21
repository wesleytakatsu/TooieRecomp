#!/usr/bin/env bash
# Build the pinned ANGRYLION_CONTROL prefix. Idempotent if binaries already exist.
set -euo pipefail
# shellcheck disable=SC1091
source "$(cd "$(dirname "$0")" && pwd)/common.sh"

FORCE="${1:-}"
if [[ "$FORCE" != "--force" ]] && oracle_paths_ok; then
    echo "ANGRYLION_CONTROL binaries already present. Pass --force to rebuild."
    echo "  gfx: $GFX_SO"
    echo "  rsp: $RSP_SO"
    echo "  ui:  $MUPEN_BIN"
    exit 0
fi

[[ -d "$CORE_SRC/.git" ]] || fail "missing sources; run $ORACLE_DIR/scripts/fetch.sh"
[[ -f "$HOOK_SRC" ]] || fail "missing hook $HOOK_SRC"

cp "$HOOK_SRC" "$CORE_SRC/src/device/r4300/p08_attract_egg_hook.h"
if git -C "$CORE_SRC" apply --check --reverse "$PATCH" >/dev/null 2>&1; then
    echo "core instrumentation already applied"
elif git -C "$CORE_SRC" apply --check "$PATCH" >/dev/null 2>&1; then
    git -C "$CORE_SRC" apply "$PATCH"
    echo "applied $PATCH"
else
    fail "cannot apply $PATCH to $CORE_SRC"
fi

# OSD=0 avoids GLU; NO_ASM=1 keeps the Pure Interpreter path as the only CPU.
CORE_MAKE=(OSD=0 VULKAN=0 NO_ASM=1 PREFIX="$PREFIX")
make -C "$CORE_SRC/projects/unix" all "${CORE_MAKE[@]}" -j"$(nproc)"
make -C "$CORE_SRC/projects/unix" install "${CORE_MAKE[@]}"

API="$CORE_SRC/src/api"
for pair in \
    "$UI_SRC/projects/unix|$PREFIX" \
    "$AUDIO_SRC/projects/unix|$PREFIX" \
    "$INPUT_SRC/projects/unix|$PREFIX" \
    "$RSP_HLE_SRC/projects/unix|$PREFIX"
do
    src="${pair%%|*}"
    make -C "$src" all APIDIR="$API" PREFIX="$PREFIX" -j"$(nproc)"
    make -C "$src" install APIDIR="$API" PREFIX="$PREFIX"
done

cmake -S "$ANGRYLION_SRC" -B "$ANGRYLION_SRC/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$ANGRYLION_SRC/build" -j"$(nproc)"

make -C "$CXD4_SRC/projects/unix" all APIDIR="$API" -j"$(nproc)"

mkdir -p "$PLUGIN_DIR"
ln -sfn "$AUDIO_SO" "$PLUGIN_DIR/mupen64plus-audio-sdl.so"
ln -sfn "$INPUT_SO" "$PLUGIN_DIR/mupen64plus-input-sdl.so"
ln -sfn "$RSP_SO" "$PLUGIN_DIR/mupen64plus-rsp-cxd4-sse2.so"
ln -sfn "$GFX_SO" "$PLUGIN_DIR/mupen64plus-video-angrylion-plus.so"

oracle_paths_ok || fail "build finished but expected binaries are missing"
echo "build done. Run: $ORACLE_DIR/run.sh"
