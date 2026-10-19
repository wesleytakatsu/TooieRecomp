// Gate/layout parity for TOOIE_DIAG_DEMO1_PARITY vs the oracle hook mirror (P08_DEMO1_PARITY).
// usage: tooie_demo1_projectile_test <tools/p08_attract_egg_hook.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include "tooie_demo1_projectile.h"

using namespace tooie_demo1_projectile;

static_assert(in_window(5, 0, 0, 306) && in_window(5, 0, 0, 390), "window post 306..390");
static_assert(!in_window(5, 0, 0, 305) && !in_window(5, 0, 0, 391), "window edges");
static_assert(!in_window(4, 0, 0, 340) && !in_window(5, 1, 0, 340) && !in_window(5, 0, 8, 340), "mode/stage/sub gate");
static_assert(in_sweep_trace(354) && in_sweep_trace(355) && !in_sweep_trace(353) && !in_sweep_trace(356) &&
              kMaxTraceRows == 300 && kMaxCandEntries == 12, "sweep trace window/caps");
static_assert(in_tgt_window(5, 0, 0, 1) && in_tgt_window(5, 0, 0, 356) && !in_tgt_window(5, 0, 0, 0) &&
              !in_tgt_window(5, 0, 0, 357) && !in_tgt_window(5, 0, 8, 100) && kTgtMaxRows == 500,
              "target prefix window gl0..355 / cap");
static_assert(is_target_marker(0x0671, 0x0E21) && is_target_marker(0x0671, 0x0E3F) && !is_target_marker(0, 0x0E21) &&
              !is_target_marker(0x0671, 0x0E20), "target marker id predicate (0671 + P6 bit0)");
static_assert(vector_at(0x80200000u, 0x9C, 3) == 0x80200000u + 3 * 0x9C, "vector_at formula");
static_assert(kSweepRadiusBits == 0x42180000u && kMaxRows == 600, "constants");

// Concatenate the "..." literals of a backslash-continued `#define NAME` and unescape \n.
static std::string macro_text(const std::string& src, const std::string& name) {
    const size_t at = src.find("#define " + name);
    if (at == std::string::npos) return "<missing>";
    std::string out;
    size_t line_start = at;
    for (;;) {
        size_t eol = src.find('\n', line_start);
        std::string line = src.substr(line_start, eol - line_start);
        bool cont = !line.empty() && line.back() == '\\';
        for (size_t p = 0; (p = line.find('"', p)) != std::string::npos;) {
            size_t q = line.find('"', p + 1);
            out += line.substr(p + 1, q - p - 1);
            p = q + 1;
        }
        if (!cont || eol == std::string::npos) break;
        line_start = eol + 1;
    }
    std::string r;
    for (size_t k = 0; k < out.size(); ++k) {
        if (out[k] == '\\' && k + 1 < out.size() && out[k + 1] == 'n') { r += '\n'; ++k; }
        else r += out[k];
    }
    return r;
}

static int count_char(const std::string& s, char c) {
    int n = 0; for (char x : s) n += (x == c); return n;
}

// Liveness / relocation counterexample: pointer-only tracking loses an object whose pointer changes
// (relocation/defrag candidate); owner-keyed resolve keeps the ordinal and flags the alias.
static int liveness_checks() {
    uint32_t ptrs[2] = {0x801c767c, 0x801c9000};
    uint32_t owners[2] = {0x80133440, 0x80133500};
    auto same = resolve(ptrs, owners, 2, 0x801c767c, 0x80133440);
    if (same.idx != 0 || same.alias) { std::fprintf(stderr, "FAIL same pointer\n"); return 1; }
    auto moved = resolve(ptrs, owners, 2, 0x801d0000, 0x80133500);  // new pointer, known owner
    if (moved.idx != 1 || !moved.alias) { std::fprintf(stderr, "FAIL relocated object lost\n"); return 1; }
    auto other = resolve(ptrs, owners, 2, 0x801e0000, 0x80133999);  // unknown owner
    if (other.idx != -1) { std::fprintf(stderr, "FAIL unknown owner matched\n"); return 1; }
    auto zero = resolve(ptrs, owners, 2, 0x801e0000, 0);  // owner word 0 must not alias
    if (zero.idx != -1) { std::fprintf(stderr, "FAIL zero owner aliased\n"); return 1; }
    // pointer-only baseline (previous diagnostic) would have returned -1 for `moved`:
    int ptr_only = -1; for (int i = 0; i < 2; ++i) if (ptrs[i] == 0x801d0000) ptr_only = i;
    if (ptr_only != -1) return 1;
    if (!is_retry(true, 0x80105000, 0x80105000, true) || is_retry(true, 0x80105000, 0x80105004, true) ||
        is_retry(false, 0x80105000, 0x80105000, true) || is_retry(true, 0x80105000, 0x80105000, false)) {
        std::fprintf(stderr, "FAIL retry predicate\n"); return 1;
    }
    return 0;
}

int main(int argc, char** argv) {
    if (liveness_checks() != 0) return 1;
    if (argc < 2) { std::fprintf(stderr, "usage: %s hook.h\n", argv[0]); return 2; }
    std::ifstream f(argv[1]); std::stringstream ss; ss << f.rdbuf();
    const std::string hook = ss.str();
    const std::string hdr = macro_text(hook, "P08_DEMO1P_HEADER");
    const std::string fmt = macro_text(hook, "P08_DEMO1P_FMT");
    if (hdr != kCsvHeader) { std::fprintf(stderr, "FAIL header differs\nhook:  %s\ntooie: %s\n", hdr.c_str(), kCsvHeader); return 1; }
    if (fmt != kRowFmt) { std::fprintf(stderr, "FAIL row format differs\nhook:  %s\ntooie: %s\n", fmt.c_str(), kRowFmt); return 1; }
    const int cols = count_char(kCsvHeader, ',') + 1;
    const int convs = count_char(kRowFmt, '%');
    if (cols != convs || cols != 37) { std::fprintf(stderr, "FAIL columns=%d conversions=%d (want 37)\n", cols, convs); return 1; }
    std::puts("PASS tooie_demo1_projectile_test");
    return 0;
}
