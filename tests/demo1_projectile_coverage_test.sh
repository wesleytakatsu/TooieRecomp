#!/usr/bin/env bash
# Wrapper-coverage gate for TOOIE_DIAG_DEMO1_PROJECTILE / P08_DEMO1_PROJECTILE: every wrapped function
# must (a) have the expected vram in metadata, (b) be declared in funcs.h, (c) be reached by generated
# code ONLY through LOOKUP_FUNC(<vram>) (so add_loaded_function wrappers see all calls) and never by a
# direct named call. usage: demo1_projectile_coverage_test.sh   (run from anywhere)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
fail=0
check() { # name vram
    local n="$1" v="$2"
    grep -q "name = \"$n\", vram = $v," "$ROOT/metadata/tooie.us.syms.toml" || { echo "FAIL $n vram != $v"; fail=1; return; }
    grep -q "^void $n(" "$ROOT/RecompiledFuncs/funcs.h" || { echo "FAIL $n not declared"; fail=1; return; }
    local direct lookups
    direct=$({ grep -hE "^\s+$n\(rdram, ctx\);" "$ROOT"/RecompiledFuncs/funcs_*.c || true; } | wc -l)
    lookups=$({ grep -h "LOOKUP_FUNC($v)" "$ROOT"/RecompiledFuncs/funcs_*.c || true; } | wc -l)
    echo "$n $v lookup_sites=$lookups direct_calls=$direct"
    [[ "$direct" -eq 0 && "$lookups" -ge 1 ]] || { echo "FAIL $n coverage"; fail=1; }
}
check _baeggsetup_entrypoint_5 0x80084A60
check _cheggfire_entrypoint_2 0x800871F0
check _suegg_entrypoint_9 0x8008A3C0
check _sueggmove_entrypoint_9 0x8008A480
check func_800CD820 0x800CD820
# helpers hooked inside the traced func_800CD820 query (sweep first-branch trace)
check func_800BE444 0x800BE444
check func_800E9E88 0x800E9E88
check func_800E9EB4 0x800E9EB4
check func_80106790 0x80106790
check _subaddiecustomhits_entrypoint_5 0x8008A018
check func_800EEB40 0x800EEB40
check func_800EC3C4 0x800EC3C4
check func_800CD6B4 0x800CD6B4
check func_800CB854 0x800CB854
# first-writer prefix (TOOIE_DIAG_DEMO1_TARGET): actor loop + its per-actor getter call
check func_800FF62C 0x800FF62C
check func_80100368 0x80100368
# primary source proof for the SP-scope gate: func_800FF62C never assigns r31 before its func_80100368 calls
# (only the epilogue restore does), so a host-side ra test cannot identify the direct loop calls.
f338="$ROOT/RecompiledFuncs/funcs_338.c"
start=$(grep -n "^RECOMP_FUNC void func_800FF62C(" "$f338" | cut -d: -f1)
body=$(awk -v s="$start" 'NR>=s { print; if ($0 ~ /^;?}$/) exit }' "$f338")
first_call=$(printf '%s\n' "$body" | awk '/LOOKUP_FUNC\(0x80100368\)/ { print NR; exit }')
r31_before=$(printf '%s\n' "$body" | awk -v n="$first_call" 'NR < n && /ctx->r31 = / { c++ } END { print c + 0 }')
r31_total=$(printf '%s\n' "$body" | { grep -c "ctx->r31 = " || true; })
sites=$(printf '%s\n' "$body" | { grep -c "LOOKUP_FUNC(0x80100368)" || true; })
echo "func_800FF62C r31_assign_before_first_getter_call=$r31_before r31_assign_total=$r31_total getter_sites=$sites"
[[ "$r31_before" -eq 0 && "$r31_total" -eq 1 && "$sites" -eq 2 ]] || { echo "FAIL func_800FF62C r31/site proof"; fail=1; }
# exactly one generated caller of the loop and a single host binding each (nothing else may override production)
loop_callers=$({ grep -h "LOOKUP_FUNC(0x800FF62C)" "$ROOT"/RecompiledFuncs/funcs_*.c || true; } | wc -l)
b_loop=$({ grep -rh "add_loaded_function(.*0x800FF62C" "$ROOT/src" || true; } | wc -l)
b_get=$({ grep -rh "add_loaded_function(.*0x80100368" "$ROOT/src" || true; } | wc -l)
echo "loop_lookup_sites=$loop_callers host_bindings_800FF62C=$b_loop host_bindings_80100368=$b_get"
[[ "$loop_callers" -eq 1 && "$b_loop" -eq 1 && "$b_get" -eq 1 ]] || { echo "FAIL first-writer binding coverage"; fail=1; }
# ABI source guard (independent of the pure helper test): the wrapper passes the call arg (record) and the RESOLVED record to
# upd_accept, never the MarkerHandle; "post" dumps the resolved record; the oracle compares record_arg with the resolved record.
H="$ROOT/src/game/boot_hw_stubs.cpp"; O="$ROOT/tools/p08_attract_egg_hook.h"
acc=$({ grep -hE "upd_accept\(s_d1t_loop_scope, sp, record_arg, expected," "$H" || true; } | wc -l)
bad_acc=$({ grep -hE "upd_accept\(.*s_d1t_marker" "$H" || true; } | wc -l)
post_rec=$({ grep -hE 'tooie_d1t_upd_dump\(rdram, "post", s_d1t_upd_post, rec, s_d1t_marker' "$H" || true; } | wc -l)
bad_post=$({ grep -hE 'tooie_d1t_upd_dump\(rdram, "post", s_d1t_upd_post, s_d1t_marker' "$H" || true; } | wc -l)
orc=$({ grep -hE "record_arg != expected" "$O" || true; } | wc -l)
orc_post=$({ grep -hE 'p08_d1t_upd_dump\(r4300, "post", s_p08_d1t_upd_post, rec, s_p08_d1t_marker' "$O" || true; } | wc -l)
echo "abi_guard host_accept=$acc host_accept_marker=$bad_acc host_post_record=$post_rec host_post_marker=$bad_post oracle_cmp=$orc oracle_post_record=$orc_post"
[[ "$acc" -eq 1 && "$bad_acc" -eq 0 && "$post_rec" -eq 1 && "$bad_post" -eq 0 && "$orc" -eq 1 && "$orc_post" -eq 1 ]] || { echo "FAIL first-writer ABI source guard"; fail=1; }
# RNG-consumer / expansion-scope trace: every call must be visible to the wrappers (LOOKUP only)
check func_800DC0C0 0x800DC0C0
check func_800DC090 0x800DC090
check func_800E3E8C 0x800E3E8C
check func_800CACEC 0x800CACEC
check func_8011447C 0x8011447C
for v in 0x800DC0C0 0x800DC090; do
    n=$({ grep -rh "add_loaded_function(.*$v" "$ROOT/src" || true; } | wc -l)
    echo "host_bindings_$v=$n"
    [[ "$n" -eq 1 ]] || { echo "FAIL binding $v"; fail=1; }
done
[[ $fail -eq 0 ]] && echo "PASS demo1 projectile wrapper coverage" || exit 1
