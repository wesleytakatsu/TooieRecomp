// Gate/layout parity for TOOIE_DIAG_DEMO1_PARITY vs the oracle hook mirror (P08_DEMO1_PARITY).
// usage: tooie_demo1_parity_test <tools/p08_attract_egg_hook.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include "tooie_demo1_parity.h"

using namespace tooie_demo1_parity;

static_assert(in_window(5, 0, 0, 1) && in_window(5, 0, 0, 421), "consumed 0..420 <=> post 1..421");
static_assert(!in_window(5, 0, 0, 0) && !in_window(5, 0, 0, 422), "window edges");
static_assert(!in_window(4, 0, 0, 10) && !in_window(5, 1, 0, 10) && !in_window(5, 0, 8, 10), "mode/stage/sub gate");

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

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: %s hook.h\n", argv[0]); return 2; }
    std::ifstream f(argv[1]); std::stringstream ss; ss << f.rdbuf();
    const std::string hook = ss.str();
    const std::string hdr = macro_text(hook, "P08_DEMO1_HEADER");
    const std::string fmt = macro_text(hook, "P08_DEMO1_FMT");
    if (hdr != kCsvHeader) { std::fprintf(stderr, "FAIL header differs\nhook:  %s\ntooie: %s\n", hdr.c_str(), kCsvHeader); return 1; }
    if (fmt != kRowFmt) { std::fprintf(stderr, "FAIL row format differs\nhook:  %s\ntooie: %s\n", fmt.c_str(), kRowFmt); return 1; }
    const int cols = count_char(kCsvHeader, ',') + 1;
    const int convs = count_char(kRowFmt, '%');
    if (cols != convs || cols != 62) { std::fprintf(stderr, "FAIL columns=%d conversions=%d (want 62)\n", cols, convs); return 1; }
    std::puts("PASS tooie_demo1_parity_test");
    return 0;
}
