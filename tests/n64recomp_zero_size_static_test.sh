#!/usr/bin/env bash
# Regression: a zero-sized function symbol at a static function's address must not produce an
# empty generated body (bsrest 0x80800000 helper: tspd=0 + velocity clear on state 4->116).
# usage: n64recomp_zero_size_static_test.sh <N64Recomp binary> <decompressed rom>
# Runs the generator twice in symbols mode on COPIES of metadata:
#   A = metadata as is, B = metadata + zero-sized func_80800000_bsrest.
# Generator exit codes are checked (no failure masking). The committed symbols file is lossless (vendor/patches-local/n64recomp-symbols-file-fidelity.patch):
# the full relocatable list is used unchanged, so any generator failure fails the test.
# Requires: bsrest static helper (the one called by bsrest_entrypoint_9's 0x8080020C) has its body,
# and output A == output B (the zero-size symbol must not change anything).
set -euo pipefail
N64R="$1"; ROM="$2"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
T="$(mktemp -d)"; trap 'rm -rf "$T"' EXIT

gen() { # gen <name> <extra zero-size symbol 0|1>
    local n="$1" z="$2" d="$T/$1"
    mkdir -p "$d/out"; cp -r "$ROOT/metadata" "$d/meta"
    if [[ "$z" == 1 ]]; then
        python3 - "$d/meta/tooie.us.syms.toml" <<'PY'
import sys
p = sys.argv[1]; s = open(p).read()
a = 'functions = [\n    { name = "bsrest_entrypoint_2"'
assert a in s
open(p, 'w').write(s.replace(a, 'functions = [\n    { name = "func_80800000_bsrest", vram = 0x80800000, size = 0x0 },\n    { name = "bsrest_entrypoint_2"', 1))
PY
    fi
    sed -e "s#rom_file_path = .*#rom_file_path = \"$ROM\"#" -e "s#output_func_path = .*#output_func_path = \"out\"#" \
        -e 's#^elf_path = .*#symbols_file_path = "meta/tooie.us.syms.toml"#' \
        -e 's#metadata/#meta/#g' "$ROOT/tooie.us.rev0.recomp.toml" > "$d/t.toml"
    local rc=0
    (cd "$d" && "$N64R" t.toml > log.txt 2>&1) || rc=$?
    if [[ $rc -ne 0 ]]; then
        echo "FAIL generator '$n' exit $rc:"; grep -v '^\[Info\]' "$d/log.txt" | tail -5; exit 1
    fi
    echo "generator '$n' exit 0"
}
fbody() { # fbody <outdir> <function name>
    awk -v f="$2" 'index($0,"RECOMP_FUNC void " f "(")==1{p=1} p{print} p&&/^;?}$/{exit}' "$1"/funcs_*.c
}

gen A 0
gen B 1

# Section index of the bsrest overlay = the N in static_N_8080020C called from bsrest_entrypoint_9.
n="$(fbody "$T/B/out" bsrest_entrypoint_9 | grep -o 'static_[0-9]*_8080020C' | head -1 | sed 's/static_\([0-9]*\)_8080020C/\1/')"
[[ -n "$n" ]] || { echo "FAIL cannot find bsrest static section (bsrest_entrypoint_9 -> static_N_8080020C)"; exit 1; }
helper="$(fbody "$T/B/out" "static_${n}_80800000")"
caller="$(fbody "$T/B/out" "static_${n}_8080020C")"
grep -q "static_${n}_80800000(rdram, ctx)" <<<"$caller" || { echo "FAIL static_${n}_8080020C does not call static_${n}_80800000"; exit 1; }
if grep -q "LOOKUP_FUNC(0x8009B9B0)" <<<"$helper" && grep -q "LOOKUP_FUNC(0x8009BA9C)" <<<"$helper"; then
    echo "bsrest helper static_${n}_80800000 keeps tspd/velocity stop body"
else
    echo "FAIL bsrest helper static_${n}_80800000 is empty/incomplete"; exit 1
fi
diff -rq "$T/A/out" "$T/B/out" > "$T/diff.txt" || { echo "FAIL zero-size symbol changed generated output:"; head -5 "$T/diff.txt"; exit 1; }
echo "PASS zero-size static (bsrest section $n): body kept, output identical with/without symbol"
