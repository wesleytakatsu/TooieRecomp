#!/usr/bin/env bash
# Print pinned vs actual ANGRYLION_CONTROL HEADs and binary presence.
set -euo pipefail
# shellcheck disable=SC1091
source "$(cd "$(dirname "$0")" && pwd)/common.sh"

check_commit() {
    local dest="$1" expect="$2" label="$3"
    if [[ ! -d "$dest/.git" ]]; then
        echo "[$label] MISSING $dest"
        return
    fi
    local have
    have="$(git -C "$dest" rev-parse HEAD)"
    if [[ "$have" == "$expect" ]]; then
        echo "[$label] PIN $have"
    else
        echo "[$label] DRIFT have=$have pin=$expect"
    fi
}

echo "TOOIE_ROOT=$TOOIE_ROOT"
echo "WORK=$WORK"
check_commit "$CORE_SRC" "$MUPEN_CORE_COMMIT" "core"
check_commit "$UI_SRC" "$MUPEN_UI_COMMIT" "ui"
check_commit "$AUDIO_SRC" "$MUPEN_AUDIO_COMMIT" "audio"
check_commit "$INPUT_SRC" "$MUPEN_INPUT_COMMIT" "input"
check_commit "$RSP_HLE_SRC" "$MUPEN_RSP_HLE_COMMIT" "rsp-hle"
check_commit "$ANGRYLION_SRC" "$ANGRYLION_COMMIT" "angrylion"
check_commit "$CXD4_SRC" "$CXD4_COMMIT" "cxd4"

if [[ -f "$GFX_SO" ]]; then
    strings "$GFX_SO" | grep -m1 "RDP Plus" || true
fi
if oracle_paths_ok; then
    echo "BINARIES: present"
else
    echo "BINARIES: missing (run scripts/build.sh)"
fi
