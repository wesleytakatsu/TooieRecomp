#include "recomp.h"
#include "tooie_bgm_hooks.h"
#include "tooie_bgm_volume.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

int g_failures = 0;

void expect(bool condition, const char* name) {
    if (!condition) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failures;
    }
}

struct CspCall {
    recomp_context* ctx;
    uint32_t seqp;
    int16_t vol;
};

std::vector<CspCall> g_csp;
int g_updates = 0;
constexpr gpr kReturn = 0x5151u;
constexpr gpr kRa = 0xABCD0000u;

uint8_t* g_rdram = nullptr;

void store_h(uint32_t addr, int16_t value) {
    uint8_t* rdram = g_rdram;
    MEM_H(0, static_cast<gpr>(static_cast<int32_t>(addr))) = value;
}

int16_t load_h(uint32_t addr) {
    uint8_t* rdram = g_rdram;
    return MEM_H(0, static_cast<gpr>(static_cast<int32_t>(addr)));
}

recomp_context call_ctx() {
    recomp_context ctx{};
    ctx.r2 = kReturn;
    ctx.r31 = kRa;
    ctx.f0.u64 = 0x1234;
    return ctx;
}

} // namespace

extern "C" void func_80025ED0(uint8_t*, recomp_context* ctx) {
    g_csp.push_back(CspCall{ctx, static_cast<uint32_t>(ctx->r4), static_cast<int16_t>(ctx->r5)});
    ctx->r2 = 0x1111;
    ctx->r31 = 0x2222;
}

extern "C" void func_800FB968(uint8_t*, recomp_context* ctx) {
    ++g_updates;
    ctx->r2 = kReturn;
    ctx->r31 = kRa;
}

int main() {
    constexpr uint32_t kBytes = 0x136000u;
    g_rdram = new uint8_t[kBytes]();
    tooie_bgm::reset_slots();
    tooie_bgm::set_percent(100);

    const uint32_t seqp = tooie_bgm::seqp_addr(2);
    const uint32_t player = tooie_bgm::player_addr(2);
    const uint32_t id_addr = tooie_bgm::track_addr(2) + tooie_bgm::kTrackIdOff;
    store_h(player, 20000);
    store_h(id_addr, 4);
    for (unsigned slot = 0; slot < tooie_bgm::kSlotCount; ++slot) {
        if (slot == 2) {
            continue;
        }
        store_h(tooie_bgm::track_addr(slot) + tooie_bgm::kTrackIdOff, -1);
    }

    recomp_context ctx = call_ctx();
    ctx.r4 = static_cast<gpr>(static_cast<int32_t>(seqp));
    ctx.r5 = static_cast<gpr>(static_cast<int32_t>(20000));
    tooie_bgm_on_csp_set_vol(g_rdram, &ctx);
    expect(g_csp.size() == 1, "one_event_at_100");
    expect(g_csp[0].vol == 20000, "unscaled_forwarded");
    expect(g_csp[0].ctx == &ctx, "csp_uses_caller");
    expect(load_h(player) == 20000, "player0_untouched");

    tooie_bgm_on_update(g_rdram, &ctx);
    expect(g_updates == 1, "update_ran");
    expect(g_csp.size() == 1, "steady_100_no_extra");
    expect(ctx.r2 == kReturn, "r2_kept");
    expect(ctx.r31 == kRa, "ra_kept");
    expect(ctx.f0.u64 == 0x1234, "f0_kept");

    tooie_bgm::set_percent(0);
    ctx.r5 = 20000;
    tooie_bgm_on_csp_set_vol(g_rdram, &ctx);
    expect(g_csp.back().vol == 0, "zero");

    tooie_bgm::set_percent(50);
    ctx.r5 = 20000;
    tooie_bgm_on_csp_set_vol(g_rdram, &ctx);
    ctx.r5 = 20000;
    tooie_bgm_on_csp_set_vol(g_rdram, &ctx);
    expect(g_csp[g_csp.size() - 2].vol == 10000, "fifty");
    expect(g_csp.back().vol == 10000, "repeat_50");
    expect(load_h(player) == 20000, "still_authored");

    const size_t before_idle = g_csp.size();
    tooie_bgm_on_update(g_rdram, &ctx);
    expect(g_csp.size() == before_idle, "fade_idle_no_extra_at_same_gain");
    expect(ctx.r2 == kReturn, "r2_after_idle_update");

    tooie_bgm::set_percent(40);
    tooie_bgm_on_update(g_rdram, &ctx);
    expect(g_csp.size() == before_idle + 1, "live_one_extra");
    expect(g_csp.back().vol == 8000, "live_40");
    expect(g_csp.back().ctx != &ctx, "extra_uses_copy");
    expect(g_csp.back().seqp == seqp, "extra_seqp");
    expect(ctx.r2 == kReturn, "r2_after_live");
    expect(ctx.r31 == kRa, "ra_after_live");
    tooie_bgm_on_update(g_rdram, &ctx);
    expect(g_csp.size() == before_idle + 1, "live_holds");

    store_h(player, 10000);
    ctx.r5 = 10000;
    tooie_bgm_on_csp_set_vol(g_rdram, &ctx);
    expect(g_csp.back().vol == 4000, "fade_step_once");
    tooie_bgm::set_percent(100);
    tooie_bgm_on_update(g_rdram, &ctx);
    expect(g_csp.back().vol == 10000, "restore_100");
    const size_t after_restore = g_csp.size();
    tooie_bgm_on_update(g_rdram, &ctx);
    expect(g_csp.size() == after_restore, "restore_steady");

    store_h(id_addr, -1);
    tooie_bgm_on_update(g_rdram, &ctx);
    store_h(id_addr, 4);
    tooie_bgm::set_percent(100);
    const size_t after_idle = g_csp.size();
    tooie_bgm_on_update(g_rdram, &ctx);
    expect(g_csp.size() == after_idle, "cleared_slot_no_event_at_100");

    ctx.r4 = 0x12345678u;
    ctx.r5 = 1234;
    const size_t before_unknown = g_csp.size();
    tooie_bgm_on_csp_set_vol(g_rdram, &ctx);
    expect(g_csp.size() == before_unknown + 1, "unknown_one");
    expect(g_csp.back().vol == 1234, "unknown_unchanged");
    expect(g_csp.back().seqp == 0x12345678u, "unknown_seqp");

    delete[] g_rdram;
    if (g_failures != 0) {
        std::fprintf(stderr, "%d failures\n", g_failures);
        return 1;
    }
    std::printf("tooie_bgm_hooks_test ok\n");
    return 0;
}
