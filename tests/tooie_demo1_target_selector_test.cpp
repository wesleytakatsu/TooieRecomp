// Selector regression for TOOIE_DIAG_DEMO1_TARGET / P08_DEMO1_TARGET (marker 0x0671 + P6 bit0).
// usage: tooie_demo1_target_selector_test <fixture_current.csv> <fixture_oracle.csv>
// Fixtures are the REAL s_106790 enter rows of the PAR3 sweep captures. The previous selector
// (lhu18 == 0x0E3F, taken from the oracle hit-leave row) matched NONE of them (old FAIL).
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "tooie_demo1_projectile.h"

using namespace tooie_demo1_projectile;

static bool old_is_target_marker(uint32_t l14, uint32_t l18) { return l14 == 0x0671u && l18 == 0x0E3Fu; }

struct Row { uint32_t a0, w0, l14, l18; int post; };

static std::vector<Row> load(const char* path) {
    std::vector<Row> v;
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#' || line.rfind("ev,", 0) == 0) continue;
        std::stringstream ss(line);
        std::string c[8];
        for (int i = 0; i < 8 && std::getline(ss, c[i], ','); ++i) {}
        v.push_back({uint32_t(std::strtoul(c[4].c_str(), nullptr, 16)), uint32_t(std::strtoul(c[5].c_str(), nullptr, 16)),
                     uint32_t(std::strtoul(c[6].c_str(), nullptr, 16)), uint32_t(std::strtoul(c[7].c_str(), nullptr, 16)),
                     std::atoi(c[3].c_str())});
    }
    return v;
}

#define CHECK(c, msg) do { if (!(c)) { std::fprintf(stderr, "FAIL: %s (line %d)\n", msg, __LINE__); return 1; } } while (0)

int main(int argc, char** argv) {
    // synthetic predicate counterexamples
    CHECK(is_target_marker(0x0671, 0x0E21), "0671/0E21 (current, enter) must match");
    CHECK(is_target_marker(0x0671, 0x0E3F), "0671/0E3F (oracle hit leave) must match");
    CHECK(!is_target_marker(0x0671, 0x0E20), "bit0 clear (P6 would reject) must not match");
    CHECK(!is_target_marker(0x0000, 0x0E21), "id 0 must not match");
    CHECK(!is_target_marker(0x0615, 0x1381) && !is_target_marker(0x0643, 0x0721), "other ids must not match");
    CHECK(!old_is_target_marker(0x0671, 0x0E21), "old selector rejected the real current marker (documented failure)");
    // ambiguity: second DIFFERENT handle with 0671+bit0 is counted, never rebinds, repeated same handle is idempotent
    {
        TargetSelector s;
        CHECK(s.feed(0x80132f60, 0x0671, 0x0E21) == 1 && s.marker == 0x80132f60, "bind first");
        CHECK(s.feed(0x80132f60, 0x0671, 0x0E3F) == 0, "same handle again is idempotent");
        CHECK(s.feed(0x80140000, 0x0671, 0x0E21) == 2 && s.marker == 0x80132f60 && s.ambiguous == 1 && s.nhandles == 2,
              "second distinct handle = ambiguity, no rebind");
        CHECK(s.feed(0x80150000, 0x0643, 0x0721) == 0 && s.nhandles == 2, "non-matching id ignored");
    }
    if (argc < 3) { std::fprintf(stderr, "usage: %s cur.csv orc.csv\n", argv[0]); return 2; }
    for (int k = 1; k <= 2; ++k) {
        const std::vector<Row> rows = load(argv[k]);
        CHECK(!rows.empty(), "fixture empty");
        TargetSelector s;
        TargetSelector old_sel_dummy;
        int matches = 0, old_matches = 0, first_post = -1;
        for (const Row& r : rows) {
            if (is_target_marker(r.l14, r.l18)) ++matches;
            if (old_is_target_marker(r.l14, r.l18)) ++old_matches;
            if (s.feed(r.a0, r.l14, r.l18) == 1) first_post = r.post;
        }
        std::printf("%s rows=%zu matches=%d old_matches=%d bound=%08x first_post=%d handles=%d ambiguous=%d\n",
                    argv[k], rows.size(), matches, old_matches, s.marker, first_post, s.nhandles, s.ambiguous);
        CHECK(old_matches == 0, "old selector must match nothing in the real fixture (old FAIL)");
        CHECK(matches >= 2 && s.marker == 0x80132f60u && s.nhandles == 1 && s.ambiguous == 0 && first_post == 354,
              "new selector: one unique handle 80132f60 bound at post 354, no ambiguity");
        (void)old_sel_dummy;
    }
    std::puts("PASS tooie_demo1_target_selector_test");
    return 0;
}
