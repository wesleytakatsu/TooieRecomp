#!/usr/bin/env bash
# Apply versioned N64ModernRuntime patches to the external runtime checkout.
# Idempotent: validates expected state, dry-runs, applies; recognizes already-applied.
# Does NOT git checkout/reset or touch other local changes.
set -euo pipefail

TOOL_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$TOOL_DIR/.." && pwd)"
WS="$(cd "$ROOT/.." && pwd)"
RUNTIME="$WS/BanjoRecomp/lib/N64ModernRuntime"

PATCH_DIR="$ROOT/patches/runtime"
PATCH="$PATCH_DIR/n64modernruntime-scheduler.patch"

# Expected base commit of the external runtime.
EXPECTED_BASE="ca568b6ad79b9029d14077f0c3ffa757727c5559"
# SHA256 of the patch file itself.
EXPECTED_PATCH_SHA256="1b3d3d4666e02f748c44d19d867d02669b83753d85fe7eb62d746a5a8cf38309"
# Files touched by the patch.
PATCHED_FILES=(ultramodern/src/threadqueue.cpp ultramodern/src/threads.cpp)

fail() { echo "ERROR: $*" >&2; exit 1; }

[[ -f "$PATCH" ]] || fail "patch not found: $PATCH"

actual_patch_sha="$(sha256sum "$PATCH" | awk '{print $1}')"
[[ "$actual_patch_sha" == "$EXPECTED_PATCH_SHA256" ]] || fail "patch SHA256 mismatch: $actual_patch_sha (expected $EXPECTED_PATCH_SHA256)"

cd "$RUNTIME"

git rev-parse --git-dir >/dev/null 2>&1 || fail "runtime not a git checkout: $RUNTIME"

actual_base="$(git rev-parse HEAD)"
[[ "$actual_base" == "$EXPECTED_BASE" ]] || fail "runtime HEAD $actual_base != expected base $EXPECTED_BASE"

# Detect whether the patch is already applied (reverse check succeeds).
if git apply --reverse --check "$PATCH" >/dev/null 2>&1; then
    echo "scheduler patch already applied (base $actual_base). Nothing to do."
    exit 0
fi

# Dry-run against the real tree.
if ! git apply --check "$PATCH"; then
    echo "scheduler patch does not apply cleanly (possibly conflicting local edits):"
    git apply --check "$PATCH" 2>&1 || true
    exit 2
fi

git apply "$PATCH"
echo "applied scheduler patch (base $actual_base):"
git diff --stat -- "${PATCHED_FILES[@]}"
