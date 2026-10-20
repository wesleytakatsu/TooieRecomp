#!/usr/bin/env bash
# Apply versioned N64ModernRuntime patches to the external runtime checkout.
# Also applies versioned RT64 patches to BanjoRecomp/lib/rt64.
# Idempotent: validates expected state, dry-runs, applies; recognizes already-applied.
# Does NOT git checkout/reset or touch other local changes.
set -euo pipefail

TOOL_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$TOOL_DIR/.." && pwd)"
WS="$(cd "$ROOT/.." && pwd)"
RUNTIME="$WS/BanjoRecomp/lib/N64ModernRuntime"
N64RECOMP="$RUNTIME/N64Recomp"

PATCH_DIR="$ROOT/patches/runtime"

fail() { echo "ERROR: $*" >&2; exit 1; }

# apply_one_patch <repo> <patchfile> <expected_base_commit> <expected_patch_sha256> <label>
apply_one_patch() {
    local repo="$1" patch="$2" expected_base="$3" expected_sha="$4" label="$5"

    [[ -f "$patch" ]] || fail "[$label] patch not found: $patch"

    local actual_sha
    actual_sha="$(sha256sum "$patch" | awk '{print $1}')"
    [[ "$actual_sha" == "$expected_sha" ]] \
        || fail "[$label] patch SHA256 mismatch: $actual_sha (expected $expected_sha)"

    cd "$repo"
    git rev-parse --git-dir >/dev/null 2>&1 || fail "[$label] not a git checkout: $repo"

    local actual_base
    actual_base="$(git rev-parse HEAD)"
    [[ "$actual_base" == "$expected_base" ]] \
        || fail "[$label] repo HEAD $actual_base != expected base $expected_base"

    # Already applied? reverse check succeeds.
    if git apply --reverse --check "$patch" >/dev/null 2>&1; then
        echo "[$label] already applied (base $actual_base). Nothing to do."
        return 0
    fi

    # Dry-run against the real tree.
    if ! git apply --check "$patch" >/dev/null 2>&1; then
        echo "[$label] patch does not apply cleanly (possibly conflicting local edits):"
        git apply --check "$patch" 2>&1 || true
        return 2
    fi

    git apply "$patch"
    echo "[$label] applied (base $actual_base):"
    git diff --stat
}

# Scheduler patch — N64ModernRuntime root.
apply_one_patch \
    "$RUNTIME" \
    "$PATCH_DIR/n64modernruntime-scheduler.patch" \
    "ca568b6ad79b9029d14077f0c3ffa757727c5559" \
    "1b3d3d4666e02f748c44d19d867d02669b83753d85fe7eb62d746a5a8cf38309" \
    "scheduler"

# Hybrid F' PATCH 1+2+3+3.5+RCP cumulative — runtime service + C ABI +
# cause-aware hold-until-eligible + per-OSThread RCP mask
# (feature OFF for games).
apply_one_patch \
    "$RUNTIME" \
    "$PATCH_DIR/n64modernruntime-preemption.patch" \
    "ca568b6ad79b9029d14077f0c3ffa757727c5559" \
    "8959055e6c51767fcd7c58b403a2c6ce8bc7aa0ab6c23ffab9dc181905eacd6b" \
    "preemption runtime service"

# Resolution Original3x/Original4x enum + JSON (config.hpp only).
apply_one_patch \
    "$RUNTIME" \
    "$PATCH_DIR/n64modernruntime-resolution-3x-4x.patch" \
    "ca568b6ad79b9029d14077f0c3ffa757727c5559" \
    "6955ca324fa255ffbe0e61806cb500c602a5818638ddd13bcc05d076e3e18b91" \
    "resolution 3x/4x runtime enum"

# N64Recomp Tooie generator patch — same-section JAL + PATCH 3 safepoints.
# Historical n64recomp-same-section-jal.patch is inactive (superseded).
apply_one_patch \
    "$N64RECOMP" \
    "$PATCH_DIR/n64recomp-tooie.patch" \
    "2b6f05688de2abc7d86da5b4a89b84c2c6acbabe" \
    "adf97c9dfe20220143939ca1aa8728afc14a4eff957e35d8d2352ef027b98dc7" \
    "generator jal+safepoints"

# N64Recomp static sizing: zero-sized symbol at a static's address must not bound it
# (empty static_*_80800000 bsrest helper = missing tspd/vel stop on state 4->116).
# Stacked on top of n64recomp-tooie.patch (needs it applied first).
apply_one_patch \
    "$N64RECOMP" \
    "$PATCH_DIR/n64recomp-zero-size-static.patch" \
    "2b6f05688de2abc7d86da5b4a89b84c2c6acbabe" \
    "48ca1e7710a7b6ecd2f4cfe3c8ac064c11b2307ad98f35ee53b70b3f1cfebb58" \
    "generator zero-size static"

# Resolution 3x/4x frontend options + RT64 manual scale/downsample (2 files, disjoint from the overlay patch).
FRONTEND="$WS/BanjoRecomp/lib/RecompFrontend"
apply_one_patch \
    "$FRONTEND" \
    "$ROOT/patches/frontend/recompfrontend-resolution-3x-4x.patch" \
    "d0d90ba49f46f4896aaeda362056c21b1e342561" \
    "60eb31406dc9ac6f88368e492dcbd65f22fc00ccb0529c43fdd9c211d07a6faa" \
    "recompfrontend resolution 3x/4x"

# Disabled config options must not change via any widget path (slider click/drag/nav, radio, toggle).
apply_one_patch \
    "$FRONTEND" \
    "$ROOT/patches/frontend/recompfrontend-disabled-option-guard.patch" \
    "d0d90ba49f46f4896aaeda362056c21b1e342561" \
    "9298ec60e89f4bb282b98d850e953285cf76af6fa0947d56a4f6d70c2eecc20c" \
    "recompfrontend disabled option guard"

# RT64 pin BanjoRecomp/lib/rt64 6f1c2d99. These three do not overlap.
# Do not also apply rt64-present-rate-decimator.patch,
# rt64-present-success-counter.patch, or p09-exact-use-pathology-correlation.patch.
RT64="$WS/BanjoRecomp/lib/rt64"
RT64_BASE="6f1c2d99a4ea571c139f449c326fd176ba8f3496"
RT64_PATCH_DIR="$ROOT/patches/rt64"

apply_one_patch \
    "$RT64" \
    "$RT64_PATCH_DIR/rt64-rsp-endpoint-current.patch" \
    "$RT64_BASE" \
    "49925480c2d1736c790ddf96152844951f6fe27a150ab1cc110347d557c0d813" \
    "rt64 rsp endpoint current"

apply_one_patch \
    "$RT64" \
    "$RT64_PATCH_DIR/rt64-present-fps-decimator-and-counter.patch" \
    "$RT64_BASE" \
    "6d746f27ed1bd3132dcea8464adeb25dcfac4ad38502bf92f402ea6e3a19836c" \
    "rt64 present fps decimator and counter"

apply_one_patch \
    "$RT64" \
    "$RT64_PATCH_DIR/rt64-p09-exact-use-correlation-header.patch" \
    "$RT64_BASE" \
    "0d4c250a5c4950dcdb1d0835ea7347014c90893e93332caa5576c67e03459561" \
    "rt64 p09 exact-use correlation header"

# RecompFrontend overlay hook. Existing artifact; pin d0d90ba.
# Do not apply the superseded RT64 incremental patches above.
FRONTEND="$WS/BanjoRecomp/lib/RecompFrontend"
apply_one_patch \
    "$FRONTEND" \
    "$ROOT/patches/frontend/recompfrontend-overlay-update-callback.patch" \
    "d0d90ba49f46f4896aaeda362056c21b1e342561" \
    "37266d0bd13d42c0b8f174c0118b6a9b2490784c5880f05e6ee37ab811cd48a7" \
    "recompfrontend overlay update callback"
