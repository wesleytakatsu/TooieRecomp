#include "tooie_dev_enemy_ai.h"
#include "tooie_dev_cheats.h"
#include "tooie_story_coop.h"
#include "librecomp/overlays.hpp"

#include <array>
#include <cmath>
#include <cstring>
#include <unordered_map>

extern "C" {
void func_800F54E4(uint8_t*, recomp_context*);
void func_800F6438(uint8_t*, recomp_context*);
void func_800F6478(uint8_t*, recomp_context*);
void func_800F64A4(uint8_t*, recomp_context*);
void func_800F5A00(uint8_t*, recomp_context*);
void func_80100368(uint8_t*, recomp_context*);
void func_8010038C(uint8_t*, recomp_context*);
void func_8010D1E8(uint8_t*, recomp_context*);
void func_8001211C(uint8_t*, recomp_context*);
void suchase_entrypoint_0(uint8_t*, recomp_context*);
void suchase_entrypoint_1(uint8_t*, recomp_context*);
void sucoasterfly_entrypoint_3(uint8_t*, recomp_context*);
void func_80800064_chhillbilly(uint8_t*, recomp_context*);
void func_8080000C_chdiggerground(uint8_t*, recomp_context*);
void func_808000A0_ch2dbaddy(uint8_t*, recomp_context*);
void func_80800054_chcrystalbaddie(uint8_t*, recomp_context*);
void func_8080005C_chflowerbaddie(uint8_t*, recomp_context*);
void func_8080004C_chnewshovel(uint8_t*, recomp_context*);
void func_80800018_chseaweedbaddy(uint8_t*, recomp_context*);
}

namespace {
constexpr uint32_t kPlayers = 0x80135490, kPerception = 0x80136F50;
struct Family {
    uint32_t rom, offset, prefix, excluded_forms;
    recomp_func_t* native;
};
// Canonical class descriptors and callbacks, read from the verified decompressed ROM.
constexpr Family families[] = {
    {0x020058F0, 0xA0, 0x02F70464, 0, func_808000A0_ch2dbaddy},
    {0x0200F1F0, 0x54, 0x03040426, 0, func_80800054_chcrystalbaddie},
    {0x0200F9B0, 0x5C, 0x01A9046B, 0, func_8080005C_chflowerbaddie},
    {0x02025560, 0x4C, 0x000A011A, 0x10000, func_8080004C_chnewshovel},
    {0x01FBCDC0, 0x18, 0x02EF035C, 0, func_80800018_chseaweedbaddy},
    {0x01F9D080, 0x64, 0x01A10150, 0, func_80800064_chhillbilly},
    {0x01F9DE20, 0x0C, 0x01A20131, 0, func_8080000C_chdiggerground},
    // Digger Fly's descriptor delegates update to syscall 0x80089ED8. Its
    // controller is scoped after the normal syscall dispatcher loads the overlay.
    {0, 0, 0x01A40133, 0, sucoasterfly_entrypoint_3},
};
gpr guest(uint32_t p) { return static_cast<int32_t>(p); }
bool readable(uint32_t p, uint32_t size) {
    return (p & 3u) == 0 && p >= 0x80000000u && p <= 0x80800000u - size;
}
recomp_context local_call(const recomp_context& input) {
    auto c = input;
    c.r29 = ADD32(input.r29, -0x80);
    return c;
}
bool enabled(uint8_t* rdram) {
    if (!tooie_story_coop::running(rdram) || MEM_H(0, guest(0x80127632)) != 3 ||
        MEM_BU(0, guest(0x8012762C)) == 0x1B) return false;
    const uint32_t first = MEM_W(0, guest(kPlayers)), second = MEM_W(4, guest(kPlayers));
    return first != second && readable(first, 4) && readable(second, 4);
}
using Point = std::array<float, 3>;
bool point(uint8_t* rdram, uint32_t p, Point& out) {
    if (!readable(p, 12)) return false;
    for (unsigned i = 0; i < 3; ++i) {
        const auto bits = MEM_W(i * 4, guest(p));
        std::memcpy(&out[i], &bits, 4);
        if (!std::isfinite(out[i])) return false;
    }
    return true;
}
double distance(const Point& a, const Point& b) {
    double sum = 0;
    for (unsigned i = 0; i < 3; ++i) { const double d = double(a[i]) - b[i]; sum += d * d; }
    return sum;
}
bool player_point(uint8_t* rdram, const recomp_context& input, unsigned index, Point& out, uint32_t excluded = 0) {
    auto c = local_call(input); c.r4 = index;
    func_800F6438(rdram, &c);
    if (c.r2 == 0) return false;
    c = local_call(input); c.r4 = index;
    func_800F6478(rdram, &c);
    if (c.r2 == 0) return false;
    if (excluded) {
        c = local_call(input); c.r4 = index; c.r5 = excluded;
        func_800F64A4(rdram, &c);
        if (c.r2 != 0) return false;
    }
    c = local_call(input); c.r4 = index; c.r5 = ADD32(input.r29, -0x20);
    func_800F5A00(rdram, &c);
    return point(rdram, uint32_t(ADD32(input.r29, -0x20)), out);
}
const Family* family(uint8_t* rdram, const recomp_context& input, uint32_t actor) {
    if (!readable(actor, 0x80)) return nullptr;
    auto c = local_call(input); c.r4 = guest(actor);
    func_80100368(rdram, &c);
    const uint32_t descriptor = uint32_t(c.r2);
    if (!readable(descriptor, 4)) return nullptr;
    const uint32_t prefix = MEM_W(0, guest(descriptor));
    for (const auto& f : families) if (prefix == f.prefix) return &f;
    return nullptr;
}
struct Target { uint32_t marker, prefix; unsigned player; };
struct Chase { std::array<int32_t, 2> words{}; bool initialized = false; };
thread_local std::unordered_map<uint32_t, Target> targets;
thread_local std::array<Chase, 2> chase;
thread_local int ai_player = -1;

int choose(uint8_t* rdram, const recomp_context& input, uint32_t actor, const Family& f) {
    Point origin{}, positions[2]{};
    if (!point(rdram, actor + 4, origin)) return -1;
    bool valid[2]{}; double d[2]{};
    for (unsigned i = 0; i < 2; ++i) {
        valid[i] = player_point(rdram, input, i, positions[i], f.excluded_forms);
        if (valid[i]) d[i] = distance(origin, positions[i]);
    }
    if (!valid[0] && !valid[1]) return -1;
    unsigned selected = valid[0] && (!valid[1] || d[0] <= d[1]) ? 0 : 1;
    const uint32_t marker = MEM_W(0, guest(actor));
    const auto old = targets.find(actor);
    if (old != targets.end() && old->second.marker == marker && old->second.prefix == f.prefix &&
        valid[old->second.player]) {
        const unsigned previous = old->second.player;
        const unsigned state = MEM_HU(0x72, guest(actor)) >> 10;
        // Keep an engaged target through its native animation/state. Idle can acquire
        // another player only when meaningfully closer, preventing boundary oscillation.
        const double radius = MEM_HU(0x7C, guest(actor));
        const double release = radius > 0 ? radius * radius : 25000000.0;
        if ((state > 2 && d[previous] <= release) || d[selected] >= d[previous] * 0.64) selected = previous;
    }
    targets[actor] = {marker, f.prefix, selected};
    return int(selected);
}

class PerceptionScope {
    uint8_t* rdram;
    std::array<int32_t, 6> saved{};
    int previous;
public:
    PerceptionScope(uint8_t* memory, const recomp_context& input, int player) : rdram(memory), previous(ai_player) {
        for (unsigned i = 0; i < saved.size(); ++i) saved[i] = MEM_W(i * 4, guest(kPerception));
        ai_player = player;
        auto c = local_call(input);
        func_8010D1E8(rdram, &c);
    }
    ~PerceptionScope() {
        for (unsigned i = 0; i < saved.size(); ++i) MEM_W(i * 4, guest(kPerception)) = saved[i];
        ai_player = previous;
    }
};
template<unsigned I> void update(uint8_t* rdram, recomp_context* ctx) {
    const auto& f = families[I];
    if (!enabled(rdram) || !readable(uint32_t(ctx->r4), 0x80)) { f.native(rdram, ctx); return; }
    const uint32_t actor = uint32_t(ctx->r4);
    const auto* actual = family(rdram, *ctx, actor);
    if (actual != &f) { f.native(rdram, ctx); return; }
    const int player = choose(rdram, *ctx, actor, f);
    if (player < 0) { f.native(rdram, ctx); return; }
    PerceptionScope scope(rdram, *ctx, player);
    f.native(rdram, ctx);
}
constexpr recomp_func_t* wrappers[] = {update<0>, update<1>, update<2>, update<3>, update<4>, update<5>, update<6>, update<7>};
} // namespace

namespace tooie_dev_enemy_ai {
void loaded_overlay(uint32_t rom, int32_t ram, uint32_t size) {
    for (unsigned i = 0; i < std::size(families); ++i) {
        if (families[i].rom != 0 && rom == families[i].rom && families[i].offset < size)
            recomp::overlays::add_loaded_function(int32_t(uint32_t(ram) + families[i].offset), wrappers[i]);
    }
}
void reset() { targets.clear(); chase = {}; ai_player = -1; }
void dispatch(uint8_t* rdram, recomp_context* ctx, recomp_func_t* native) {
    if (native == sucoasterfly_entrypoint_3) { update<7>(rdram, ctx); return; }
    // Entry 1 calls entry 0 directly in generated/native code. Scope the whole
    // predicate as well, so that internal call cannot bypass cache isolation.
    if ((native != suchase_entrypoint_0 && native != suchase_entrypoint_1) ||
        ai_player < 0 || !enabled(rdram)) { native(rdram, ctx); return; }
    // This runs only after the original dispatcher activated suchase's relocation.
    const uint32_t data = uint32_t(section_addresses[3214]) + 0x1D0;
    if (!readable(data, 8)) { native(rdram, ctx); return; }
    std::array<int32_t, 2> saved{};
    auto& own = chase[ai_player];
    if (!own.initialized) {
        auto c = local_call(*ctx); func_8001211C(rdram, &c);
        own.words = {int32_t(~uint32_t(c.r2)), 0}; own.initialized = true;
    }
    for (unsigned i = 0; i < 2; ++i) { saved[i] = MEM_W(i * 4, guest(data)); MEM_W(i * 4, guest(data)) = own.words[i]; }
    native(rdram, ctx);
    for (unsigned i = 0; i < 2; ++i) { own.words[i] = MEM_W(i * 4, guest(data)); MEM_W(i * 4, guest(data)) = saved[i]; }
}
} // namespace tooie_dev_enemy_ai

extern "C" void tooie_dev_enemy_focus(uint8_t* rdram, recomp_context* ctx) {
    if (ai_player >= 0) ctx->r2 = ai_player;
    else func_800F54E4(rdram, ctx);
}
extern "C" void tooie_dev_enemy_range(uint8_t* rdram, recomp_context* ctx) {
    if (!enabled(rdram) || !family(rdram, *ctx, uint32_t(ctx->r4))) { func_8010038C(rdram, ctx); return; }
    Point origin{}, current{}, second{};
    const auto input = *ctx;
    if (!point(rdram, uint32_t(ctx->r4) + 4, origin) || !point(rdram, uint32_t(ctx->r5), current) ||
        !player_point(rdram, input, 1, second) || distance(origin, current) <= distance(origin, second)) {
        func_8010038C(rdram, ctx); return;
    }
    // Keep all original activity/initialization flags and side effects: run the gate
    // once, changing only its position argument to the closer active player's point.
    const auto scratch = ADD32(input.r29, -0x20);
    for (unsigned i = 0; i < 3; ++i) std::memcpy(&MEM_W(i * 4, scratch), &second[i], 4);
    ctx->r29 = ADD32(input.r29, -0x80);
    ctx->r5 = scratch;
    func_8010038C(rdram, ctx);
    ctx->r29 = input.r29;
}
