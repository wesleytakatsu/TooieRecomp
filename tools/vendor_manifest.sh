#!/usr/bin/env bash
# Maintainer: regenerate vendor/MANIFEST.sha256 + vendor/MODES.tsv after an INTENTIONAL change to a vendored tree
# (a new vendor/patches-local/*.patch must accompany any edit under lib/ - see vendor/UPDATING.md). Offline, byte-exact, LC_ALL=C order.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; cd "$ROOT"
export LC_ALL=C
PATHS=(lib decomp assets recompcontrollerdb.txt tools/wheelhouse tools/requirements.lock)
# decomp/ may hold ignored build outputs if someone ran make in-tree: only git-visible (non-ignored) files are vendored content.
if git rev-parse --git-dir >/dev/null 2>&1; then
    list() { git ls-files -z --cached --others --exclude-standard -- "${PATHS[@]}" | tr '\0' '\n'; }
else
    list() { find "${PATHS[@]}" -type f; }
fi
list | while IFS= read -r f; do if [[ -f "$f" && ! -L "$f" ]]; then printf '%s\n' "$f"; fi; done | sort -u > /tmp/tooie-vendor-files.$$
xargs -d '\n' sha256sum < /tmp/tooie-vendor-files.$$ > vendor/MANIFEST.sha256
# git tracks only the exec bit (100644/100755); umask-dependent group bits are deliberately not recorded.
{ echo "# Vendored files with the executable bit (git mode 100755). Everything else is 100644. Checked by tools/vendor_verify.sh."
  xargs -d '\n' stat -c '%A %n' < /tmp/tooie-vendor-files.$$ | awk '$1 ~ /^-..x/ {sub(/^[^ ]+ /,""); print "755\t" $0}'; } > vendor/MODES.tsv
rm -f /tmp/tooie-vendor-files.$$
echo "manifest: $(wc -l < vendor/MANIFEST.sha256) files, $(grep -vc '^#' vendor/MODES.tsv || true) executable files"
