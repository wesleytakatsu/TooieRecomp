// Contract of the USER_DEMO1 first-writer prefix (updrec rows, kUpd*/UpdLoopScope in tooie_demo1_projectile.h).
// usage: tooie_demo1_upd_layout_test <tgtrec_ident_current.csv> <tgtrec_ident_oracle.csv>
// Regression 1 (ra gate): the first version gated on ctx->r31; the generated func_800FF62C never assigns r31 before its
// func_80100368 calls (RecompiledFuncs/funcs_338.c), so valid direct calls were rejected.
// Regression 2 (ABI): func_80100368's a0 is the ActorRecordAddress (vector_at(*D_80136EE0, lhu(marker+0x1A)>>5)), NOT the
// MarkerHandle. The first version compared a0 with the MarkerHandle (always wrong). Fixtures are the REAL tgtrec identity
// tuples (a0 record, v0 marker, a1 idx, a2 vec begin, a3 stride) of the prefix-2 captures. Build with
// -DTOOIE_UPD_TEST_OLD_MARKER_COMPARE to compile the refuted predicate: it must exit 1 (OLD FAIL).
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "tooie_demo1_projectile.h"

using namespace tooie_demo1_projectile;

#define CHECK(c, m) do { if (!(c)) { std::printf("FAIL: %s\n", m); return 1; } } while (0)

struct Row { int post; uint32_t record, marker, idx, begin, stride; };

static std::vector<Row> load(const char* path) {
    std::vector<Row> v;
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#' || line.rfind("post,", 0) == 0) continue;
        std::stringstream ss(line);
        std::string t[6];
        for (auto& x : t) std::getline(ss, x, ',');
        v.push_back({std::atoi(t[0].c_str()), uint32_t(std::strtoul(t[1].c_str(), nullptr, 16)),
                     uint32_t(std::strtoul(t[2].c_str(), nullptr, 16)), uint32_t(std::strtoul(t[3].c_str(), nullptr, 16)),
                     uint32_t(std::strtoul(t[4].c_str(), nullptr, 16)), uint32_t(std::strtoul(t[5].c_str(), nullptr, 16))});
    }
    return v;
}

// decision used by the wrapper: a0 (call arg) vs the freshly resolved record
static bool accept_new(const UpdLoopScope& sc, uint32_t sp, uint32_t call_arg, const Row& fresh, int post, uint32_t rows) {
    const uint32_t expected = upd_expected_record(0x0671u, fresh.begin, fresh.stride, fresh.idx);
#ifdef TOOIE_UPD_TEST_OLD_MARKER_COMPARE
    return upd_accept(sc, sp, call_arg, fresh.marker, post, rows);  // refuted: marker as the expected call arg
#else
    return upd_accept(sc, sp, call_arg, expected, post, rows);
#endif
}

int main(int argc, char** argv) {
    CHECK(argc == 3, "usage: tooie_demo1_upd_layout_test <current.csv> <oracle.csv>");
    int words = 0;
    for (int c = 0; c < kUpdChunks; ++c) {
        CHECK(upd_chunk_words(c) >= 1 && upd_chunk_words(c) <= kUpdChunkWords, "chunk size within w0..w9");
        words += upd_chunk_words(c);
    }
    CHECK(words == kUpdRecWords && kUpdRecWords * 4 == 0x9C, "chunks cover exactly the 0x9C actor record");
    CHECK(kUpdChunkWords <= 10, "w10 = MarkerHandle, w11 = game speed");
    CHECK(upd_chunk_words(-1) == 0 && upd_chunk_words(kUpdChunks) == 0, "out-of-range chunk is empty");
    CHECK(kUpdRowsPerCall == 2 * kUpdChunks, "pre chunks + post chunks");
    CHECK(kUpdMaxRows % kUpdRowsPerCall == 0 && kUpdMaxRows >= kUpdRowsPerCall * (kUpdPostHi - kUpdPostLo + 1),
          "row cap covers one update per window post");
    CHECK(kUpdMaxRows + 359 < 500, "stays under the 500-row target cap");
    CHECK(in_upd_window(82) && in_upd_window(83) && !in_upd_window(79) && !in_upd_window(87), "window brackets posts 82/83");
    CHECK(is_upd_oracle_ra(0x800FF6DCu) && is_upd_oracle_ra(0x800FF718u) && !is_upd_oracle_ra(0) &&
              !is_upd_oracle_ra(0x800FF6D4u) && !is_upd_oracle_ra(0x80100000u),
          "oracle ra = exactly the two jal-site return addresses");

    // --- call-local SP scope (ra-gate regression) ---
    const uint32_t entry_sp = 0x80345000u, loop_sp = entry_sp - 0x58u, rec = 0x801d3028u;
    const UpdLoopScope off;
    const UpdLoopScope on = UpdLoopScope::enter(entry_sp);
    CHECK(on.active && on.loop_sp == loop_sp, "scope records entry SP - 0x58");
    CHECK(upd_accept(on, loop_sp, rec, rec, 82, 0), "direct loop call accepted irrespective of r31");
    CHECK(!upd_accept(on, loop_sp - 0x20u, rec, rec, 82, 0), "nested getter call (deeper SP) excluded");
    CHECK(!upd_accept(on, loop_sp + 0x8u, rec, rec, 82, 0), "SP above the loop frame excluded");
    CHECK(!upd_accept(off, loop_sp, rec, rec, 82, 0), "scope off: excluded");
    CHECK(!upd_accept(on, loop_sp, rec + 0x9C, rec, 82, 0), "other record excluded");
    CHECK(!upd_accept(on, loop_sp, rec, 0, 82, 0) && !upd_accept(on, loop_sp, 0, 0, 82, 0), "unresolved expected record: excluded");
    CHECK(!upd_accept(on, loop_sp, rec, rec, 79, 0) && !upd_accept(on, loop_sp, rec, rec, 87, 0), "outside window excluded");
    CHECK(!upd_accept(on, loop_sp, rec, rec, 82, uint32_t(kUpdMaxRows - kUpdRowsPerCall + 1)), "row budget exhausted excluded");
    CHECK(upd_accept(on, loop_sp, rec, rec, 82, uint32_t(kUpdMaxRows - kUpdRowsPerCall)), "last budgeted update accepted");
    CHECK(upd_expected_record(0x0670u, 0x801d2ef0u, 0x9C, 2) == 0 && upd_expected_record(0x0671u, 0, 0x9C, 2) == 0 &&
              upd_expected_record(0x0671u, 0x801d2ef0u, 0, 2) == 0,
          "dead marker / unusable vector => no expected record (postmiss path)");
    UpdLoopScope cur = on;
    const UpdLoopScope saved = cur;
    cur = UpdLoopScope::enter(entry_sp - 0x200u);
    CHECK(cur.direct(entry_sp - 0x200u - 0x58u) && !cur.direct(loop_sp), "inner scope owns its own SP only");
    cur = saved;
    CHECK(cur.direct(loop_sp) && !cur.direct(entry_sp - 0x200u - 0x58u), "outer scope restored after inner exit");

    // Retired helper-wide E3E8C scope: every caller was rewritten when the wider sphere accepted.
    auto old_e3e8c_wide_apply = [](uint32_t orig_v0, uint32_t expanded_accepts, bool enabled) -> uint32_t {
        if (!enabled || orig_v0 != 0u) return orig_v0;
        return expanded_accepts == 1u ? 1u : orig_v0;
    };
    CHECK(old_e3e8c_wide_apply(0u, 1u, true) == 1u, "OLD FAIL: helper-wide policy widens func_800DE498");
    CHECK(tooie_cacec_apply_v0(0x800DE498u, 0u, 1u, true) == 0u, "NEW keeps func_800DE498 original despite wider frustum");
    CHECK(tooie_cacec_apply_v0(0x808001C8u, 0u, 1u, true) == 0u, "NEW keeps chlightfader 0x808001C8 original");
    CHECK(tooie_cacec_apply_v0(0x80092444u, 0u, 1u, true) == 0u, "NEW keeps unclassified func_80092444 original");
    CHECK(!tooie_cacec_pure_draw_vram(0x800DE498u) && !tooie_cacec_pure_draw_vram(0x800DDDC4u) &&
              !tooie_cacec_pure_draw_vram(0x800DDF14u) && !tooie_cacec_pure_draw_vram(0x800DDFA0u),
          "logic caller and zero-LOOKUP model gates are not the running allowlist");
    CHECK(tooie_cacec_pure_draw_vram(kCacecPureDrawB97A4) && kCacecScopeKindB97A4 == 3, "func_800B97A4 is kind-3 pure draw");
    int pure_draw_accepts = 0;
    if (tooie_cacec_apply_v0(kCacecPureDrawB97A4, 0u, 1u, true) == 1u) ++pure_draw_accepts;
    std::printf("pure_draw_accepts=%d\n", pure_draw_accepts);
    CHECK(pure_draw_accepts > 0, "NEW PASS: known DRAW caller func_800B97A4 still expands");
    CHECK(tooie_cacec_apply_v0(kCacecPureDrawB97A4, 0u, 1u, false) == 0u, "flag off keeps original v0");
    CHECK(tooie_cacec_apply_v0(kCacecPureDrawB97A4, 1u, 1u, true) == 1u, "already-visible result stays 1");
    CHECK(tooie_cacec_apply_v0(kCacecPureDrawB97A4, 0u, 0u, true) == 0u, "expanded reject stays original 0");

    // --- RNG trace helpers: LCG formula vs the REAL captured states (POST81 -> oracle PRE82 = 4 steps, current PRE82 = 5) ---
    {
        uint32_t st = 0x28f1cd53u;
        for (int i = 0; i < 4; ++i) st = lcg_next(st);
        CHECK(st == 0x036f8437u, "oracle PRE82 = POST81 + 4 LCG steps");
        st = lcg_next(st);
        CHECK(st == 0xc125942au, "current PRE82 = POST81 + 5 LCG steps (one extra consumer step)");
        CHECK(in_rng_window(79) && in_rng_window(84) && !in_rng_window(78) && !in_rng_window(85), "rng window 79..84");
        CHECK(kRngMaxRows + kCacecMaxRows + 415 + 8 * 0 < 1000, "diagnostic rows stay bounded");
    }

    // --- ABI regression on the REAL prefix-2 identity tuples ---
    for (int k = 1; k <= 2; ++k) {
        const std::vector<Row> rows = load(argv[k]);
        CHECK(rows.size() == 355, "fixture must hold all 355 tgtrec rows");
        size_t relocs = 0, accepted = 0, marker_rejected = 0, old_record_rejected = 0, post82 = 0;
        for (size_t i = 0; i < rows.size(); ++i) {
            const Row& r = rows[i];
            CHECK(vector_at(r.begin, r.stride, r.idx) == r.record, "vector_at(begin,stride,idx) == real record a0 (all rows)");
            CHECK(upd_expected_record(0x0671u, r.begin, r.stride, r.idx) == r.record, "expected record == real record");
            CHECK(r.record != r.marker, "record and marker are different objects");
            if (accept_new(on, loop_sp, r.record, r, 82, 0)) ++accepted;
            if (!accept_new(on, loop_sp, r.marker, r, 82, 0)) ++marker_rejected;
            if (i > 0 && rows[i - 1].record != r.record) {
                ++relocs;
                if (!accept_new(on, loop_sp, rows[i - 1].record, r, 82, 0)) ++old_record_rejected;  // stale record vs fresh
            }
            if (r.post == 82 || r.post == 83) ++post82;
        }
        std::printf("%s rows=%zu accepted_real_call_arg=%zu marker_rejected=%zu relocations=%zu stale_record_rejected=%zu frames82_83=%zu\n",
                    argv[k], rows.size(), accepted, marker_rejected, relocs, old_record_rejected, post82);
        CHECK(accepted == rows.size(), "real call arg (record) accepted in all 355 rows");
        CHECK(marker_rejected == rows.size(), "marker as call arg rejected in all 355 rows");
        CHECK(relocs >= 1 && old_record_rejected == relocs, "after each relocation the old record is rejected, fresh record accepted");
        CHECK(post82 == 2, "frames 82/83 present");
    }
    std::puts("PASS tooie_demo1_upd_layout_test");
    return 0;
}
