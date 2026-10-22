#!/usr/bin/env bash
# Write THIRD-PARTY-MANIFEST.tsv into a staged package from the files that are actually there.
#   tools/package_manifest.sh <stage dir>
# Rules: tools/package_licenses.tsv. A staged file without a rule is an error (exit 1).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
STAGE="$(cd "${1:?stage dir}" && pwd)"
OUT="$STAGE/THIRD-PARTY-MANIFEST.tsv"
{
  printf '# THIRD-PARTY-MANIFEST: every file in this package, with provenance and licence status.\n'
  printf '# Generated from the bundled files by tools/package_manifest.sh using tools/package_licenses.tsv.\n'
  printf '# UNKNOWN/UNVERIFIED = not established from the bundled bytes or pinned sources: an owner decision, not a clearance.\n'
  printf '# This package contains no ROM and no extracted game asset; the executable embeds ROM-derived translated code.\n'
  printf 'path\tsha256\tbytes\tcategory\tlicence_status\torigin_notes\n'
  cd "$STAGE"
  find . -type f ! -name 'THIRD-PARTY-MANIFEST.tsv*' ! -name MANIFEST.sha256 | sed 's#^\./##' | LC_ALL=C sort | while IFS= read -r f; do
      rule=""
      while IFS=$'\t' read -r glob cat lic note; do
          [[ -z "$glob" || "$glob" == \#* ]] && continue
          # shellcheck disable=SC2053
          if [[ "$f" == $glob ]]; then rule="$cat"$'\t'"$lic"$'\t'"$note"; break; fi
      done < "$ROOT/tools/package_licenses.tsv"
      if [[ -z "$rule" ]]; then echo "NO RULE for $f" >&2; echo "NO-RULE"; continue; fi
      printf '%s\t%s\t%s\t%s\n' "$f" "$(sha256sum "$f" | cut -d' ' -f1)" "$(stat -c %s "$f")" "$rule"
  done
} > "$OUT.tmp"
if grep -q '^NO-RULE$' "$OUT.tmp"; then rm -f "$OUT.tmp"; echo "package_manifest: unmatched files (add rules to tools/package_licenses.tsv)" >&2; exit 1; fi
mv "$OUT.tmp" "$OUT"
rows="$(grep -vc '^#' "$OUT")"            # includes the column-header row
unknown="$(grep -v '^#' "$OUT" | grep -ciE 'UNKNOWN|UNVERIFIED' || true)"
echo "package_manifest: $((rows - 1)) files, $unknown with UNKNOWN/UNVERIFIED status -> $OUT"
