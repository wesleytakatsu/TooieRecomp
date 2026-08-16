#!/usr/bin/env bash
# Apply versioned N64ModernRuntime patches to the external runtime checkout.
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

# N64Recomp Tooie generator patch — same-section JAL + PATCH 3 safepoints.
# Historical n64recomp-same-section-jal.patch is inactive (superseded).
apply_one_patch \
    "$N64RECOMP" \
    "$PATCH_DIR/n64recomp-tooie.patch" \
    "2b6f05688de2abc7d86da5b4a89b84c2c6acbabe" \
    "1f2ce7c0d9708df1024a332c1a162049158e368a4e86b747740bb1ccb4236034" \
    "generator jal+safepoints"
