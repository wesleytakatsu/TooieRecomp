#!/usr/bin/env bash
# Offline integrity check of the vendored sources (lib/, decomp/, assets/, controller DB, wheelhouse). No network, no git needed.
#   tools/vendor_verify.sh                 manifest hashes + exec bits (Linux only) + no symlinks/gitlinks/nested .git + no extra files under lib/ assets/ + binary allowlist (needs Python 3.8+)
#   tools/vendor_verify.sh --reverse-check additionally prove the recorded patch chain is present (reverse-applies on a temp copy)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; cd "$ROOT"
bad=0; fail() { echo "vendor_verify: FAIL: $*" >&2; bad=1; }
[[ -f vendor/MANIFEST.sha256 ]] || { echo "vendor_verify: vendor/MANIFEST.sha256 missing" >&2; exit 1; }

sha256sum --quiet -c vendor/MANIFEST.sha256 || fail "content differs from vendor/MANIFEST.sha256"
echo "manifest: $(wc -l < vendor/MANIFEST.sha256) files checked"

case "$(uname -s 2>/dev/null || echo unknown)" in
MSYS*|MINGW*|CYGWIN*)
    # NTFS/MSYS checkouts have no faithful POSIX exec bit, so these two checks would be false results there.
    echo "vendor_verify: SKIP: POSIX exec-bit checks (vendor/MODES.tsv, executable files under lib/ assets/) on $(uname -s): not applicable to NTFS/MSYS. Byte, symlink, nested-git, extra-file and binary-allowlist checks still run. Linux enforces exec bits." ;;
*)
    while IFS=$'\t' read -r mode path; do
        [[ -z "$mode" || "$mode" == \#* ]] && continue
        [[ -x "$path" ]] || fail "$path lost its executable bit (vendor/MODES.tsv)"
    done < vendor/MODES.tsv
    unlisted="$(comm -13 <(awk -F'\t' '!/^#/ && NF {print $2}' vendor/MODES.tsv | LC_ALL=C sort) \
                         <(find lib assets -type f -perm /100 | LC_ALL=C sort) | head -3)"
    [[ -z "$unlisted" ]] || fail "executable files not listed in vendor/MODES.tsv: $(echo "$unlisted" | tr '\n' ' ')"
    ;;
esac

n="$(find lib assets decomp -type l 2>/dev/null | wc -l)"; [[ "$n" == 0 ]] || fail "$n symlink(s) under lib/ assets/ decomp/"
n="$(find lib assets decomp \( -name .git -o -name .gitmodules -o -name .gitrepo \) 2>/dev/null | wc -l)"; [[ "$n" == 0 ]] || fail "$n nested .git/.gitmodules/.gitrepo under lib/ assets/ decomp/"
# lib/ and assets/ must contain exactly the manifest (decomp/ legitimately gains ignored build outputs if someone runs make there).
extra="$(comm -13 <(sed 's/^[0-9a-f]\{64\}  //' vendor/MANIFEST.sha256 | LC_ALL=C sort) \
                  <(find lib assets -type f | LC_ALL=C sort) | head -5)"
[[ -z "$extra" ]] || fail "files under lib/ assets/ not in the manifest: $(echo "$extra" | tr '\n' ' ')"

# Python 3.8+ for the binary audit below: first of python3, python, py -3 (Windows launcher) that runs and passes the version check.
PY=()
for cand in "python3" "python" "py -3"; do
    read -r -a argv <<< "$cand"
    command -v "${argv[0]}" >/dev/null 2>&1 || continue
    "${argv[@]}" -c "import sys; sys.exit(sys.version_info < (3, 8))" >/dev/null 2>&1 && { PY=("${argv[@]}"); break; }
done
[[ ${#PY[@]} -gt 0 ]] || { echo "vendor_verify: FAIL: no Python 3.8+ found (tried python3, python, py -3). Install Python 3 and put it on PATH; on Windows Git Bash must see it." >&2; exit 1; }
echo "python: ${PY[*]} $("${PY[@]}" -c 'import sys; print(sys.version.split()[0])')"

# Any ELF/PE/ar file under lib/ assets/ that is not in vendor/BINARIES.tsv (sha256-pinned allowlist) fails the audit.
"${PY[@]}" - <<'PY' || bad=1
import os, sys, hashlib
allow = {}
for l in open("vendor/BINARIES.tsv"):
    l = l.rstrip("\r\n")   # CRLF checkouts keep \r on every line
    if l.strip() and not l.startswith("#"): path, h = l.split("\t")[:2]; allow[path] = h
seen = set(); bad = 0
for top in ("lib", "assets"):
    for dp, _, fn in os.walk(top):
        for f in fn:
            p = os.path.join(dp, f).replace(os.sep, "/")   # forward slashes to match BINARIES.tsv on Windows
            with open(p, "rb") as fh: b = fh.read(8)
            if b[:4] == b"\x7fELF" or b[:2] == b"MZ" or b[:7] == b"!<arch>" or b[:4] in (b"\xcf\xfa\xed\xfe",):
                seen.add(p)
                if p not in allow: print("vendor_verify: FAIL: binary not in vendor/BINARIES.tsv:", p, file=sys.stderr); bad = 1
for p, h in allow.items():
    if p not in seen: print("vendor_verify: FAIL: BINARIES.tsv entry missing on disk:", p, file=sys.stderr); bad = 1
    elif hashlib.sha256(open(p, "rb").read()).hexdigest() != h: print("vendor_verify: FAIL: binary hash differs:", p, file=sys.stderr); bad = 1
sys.exit(bad)
PY

if [[ "${1:-}" == "--reverse-check" ]]; then
    T="$(mktemp -d /tmp/tooie-vendor-rev.XXXXXX)"; trap 'rm -rf "$T"' EXIT
    # vendor/patches-applied/ORDER.tsv: group <TAB> vendored dir <TAB> patch (relative to vendor/) <TAB> sha256, in application order.
    groups="$(awk -F'\t' '!/^#/ && NF {print $1}' vendor/patches-applied/ORDER.tsv | awk '!s[$0]++')"
    total=0
    for g in $groups; do
        dir="$(awk -F'\t' -v g="$g" '$1==g {print $2; exit}' vendor/patches-applied/ORDER.tsv)"
        mapfile -t pl < <(awk -F'\t' -v g="$g" '!/^#/ && $1==g {print $3 "\t" $4}' vendor/patches-applied/ORDER.tsv)
        mkdir -p "$T/$g"
        for e in "${pl[@]}"; do
            p="${e%%$'\t'*}"; h="${e##*$'\t'}"
            [[ "$(sha256sum "vendor/$p" | cut -d' ' -f1)" == "$h" ]] || fail "patch $p sha256 differs from ORDER.tsv"
            grep -E '^\+\+\+ b/' "vendor/$p" | sed 's#^+++ b/##; s#\t.*##' | while read -r f; do
                [[ -e "$dir/$f" ]] && { mkdir -p "$T/$g/$(dirname "$f")"; [[ -e "$T/$g/$f" ]] || cp "$dir/$f" "$T/$g/$f"; } || true
            done
        done
        for ((i=${#pl[@]}-1; i>=0; i--)); do   # newest first: stacked patches only reverse cleanly in reverse order
            p="${pl[$i]%%$'\t'*}"
            ( cd "$T/$g" && git apply --reverse --whitespace=nowarn "$ROOT/vendor/$p" ) || fail "patch not present in vendored tree: $p"
            total=$((total+1))
        done
    done
    echo "reverse-check: $total patches reverse-applied on temp copies (chain present in lib/)"
fi
(( bad )) && exit 1
echo "VENDOR VERIFY OK"
