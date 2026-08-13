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

# Hybrid F' PATCH 1+2 cumulative — runtime service (feature OFF for games).
apply_one_patch \
    "$RUNTIME" \
    "$PATCH_DIR/n64modernruntime-preemption.patch" \
    "ca568b6ad79b9029d14077f0c3ffa757727c5559" \
    "c2e2a908cd7be0ddafb92405b2f3f8c37b27624815c95a9131f8f9467f4c7ade" \
    "preemption runtime service"

# N64Recomp generator policy patch — N64Recomp submodule.
apply_one_patch \
    "$N64RECOMP" \
    "$PATCH_DIR/n64recomp-same-section-jal.patch" \
    "2b6f05688de2abc7d86da5b4a89b84c2c6acbabe" \
    "0afeb0e57bfdd18be7a125bebe4f7bd48273622c5f5f626ae3279fd1ded0efa8" \
    "generator same-section jal"
