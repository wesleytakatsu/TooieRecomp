#!/usr/bin/env bash
# Clone pinned Mupen + Angrylion + CXD4 into work/. Does not download a ROM.
set -euo pipefail
# shellcheck disable=SC1091
source "$(cd "$(dirname "$0")" && pwd)/common.sh"

clone_pin() {
    local url="$1" dest="$2" commit="$3" label="$4"
    if [[ -d "$dest/.git" ]]; then
        local have
        have="$(git -C "$dest" rev-parse HEAD)"
        [[ "$have" == "$commit" ]] \
            && echo "[$label] already $commit" \
            || fail "[$label] $dest HEAD $have != pinned $commit (refusing to move HEAD)"
        return 0
    fi
    echo "[$label] clone $commit"
    git clone --filter=blob:none "$url" "$dest"
    git -C "$dest" checkout --detach "$commit"
}

mkdir -p "$WORK_MUPEN" "$WORK_LLE/cfg" "$PLUGIN_DIR" "$PREFIX"

clone_pin "$MUPEN_CORE_REPO" "$CORE_SRC" "$MUPEN_CORE_COMMIT" "mupen64plus-core"
clone_pin "$MUPEN_UI_REPO" "$UI_SRC" "$MUPEN_UI_COMMIT" "mupen64plus-ui-console"
clone_pin "$MUPEN_AUDIO_REPO" "$AUDIO_SRC" "$MUPEN_AUDIO_COMMIT" "mupen64plus-audio-sdl"
clone_pin "$MUPEN_INPUT_REPO" "$INPUT_SRC" "$MUPEN_INPUT_COMMIT" "mupen64plus-input-sdl"
clone_pin "$MUPEN_RSP_HLE_REPO" "$RSP_HLE_SRC" "$MUPEN_RSP_HLE_COMMIT" "mupen64plus-rsp-hle"
clone_pin "$ANGRYLION_REPO" "$ANGRYLION_SRC" "$ANGRYLION_COMMIT" "angrylion-rdp-plus"
clone_pin "$CXD4_REPO" "$CXD4_SRC" "$CXD4_COMMIT" "mupen64plus-rsp-cxd4"

if [[ "${FETCH_RICE:-0}" == 1 ]]; then
    clone_pin "$MUPEN_RICE_REPO" "$WORK_MUPEN/mupen64plus-video-rice" "$MUPEN_RICE_COMMIT" "mupen64plus-video-rice"
fi

echo "fetch done. Next: $ORACLE_DIR/scripts/build.sh"
