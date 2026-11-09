#include "tooie_guest_memory.h"
#include "tooie_dev_travel_menu.h"
#include "tooie_dev_cheats.h"
#include "tooie_story_coop.h"

#include <vector>

extern "C" void func_800EB51C(uint8_t*, recomp_context*);
extern "C" void func_800EC7D4(uint8_t*, recomp_context*);
extern "C" void func_80106790(uint8_t*, recomp_context*);
extern "C" void func_80100368(uint8_t*, recomp_context*);
extern "C" void func_80100094(uint8_t*, recomp_context*);

namespace {
constexpr uint32_t kMenuQueue = 0x80132E8C, kView = 0x80127674, kPass = 0x80132E9C;
struct Menu { uint32_t marker, actor, option, prefix; };
thread_local std::vector<Menu> pending;

gpr guest(uint32_t p) { return static_cast<int32_t>(p); }
bool readable(uint32_t p, uint32_t size) {
    return tooie_memory::aligned(p, size);
}
bool enabled(uint8_t* rdram) {
    if (!tooie_story_coop::running(rdram) || MEM_H(0, guest(0x80127632)) != 3 ||
        MEM_BU(0, guest(0x8012762C)) == 0x1B || MEM_H(0, guest(0x801276D2)) != 2) return false;
    const uint32_t first = MEM_W(0, guest(0x80135490)), second = MEM_W(4, guest(0x80135490));
    return first != second && readable(first, 4) && readable(second, 4);
}
recomp_context call(const recomp_context& input, uint32_t arg) {
    auto c = input; c.r29 = ADD32(input.r29, -0x80); c.r4 = guest(arg); return c;
}
bool active_menu(uint8_t* rdram, const recomp_context& input, uint32_t marker, Menu& out) {
    if (!readable(marker, 0x2C)) return false;
    auto c = call(input, marker); func_80106790(rdram, &c);
    const uint32_t actor = uint32_t(c.r2);
    if (!readable(actor, 0x80) || uint32_t(MEM_W(0, guest(actor))) != marker ||
        (MEM_BU(0x65, guest(actor)) & 2) != 0) return false;
    c = call(input, actor); func_80100368(rdram, &c);
    const uint32_t descriptor = uint32_t(c.r2);
    if (!readable(descriptor, 4)) return false;
    // Canonical descriptor prefixes from the verified ROM: Warp Pad and Silo.
    const uint32_t prefix = MEM_W(0, guest(descriptor));
    if (prefix != 0x01C401E5 && prefix != 0x00B9019D) return false;
    c = call(input, actor); c.r5 = 0; func_80100094(rdram, &c);
    const uint32_t option = uint32_t(c.r2);
    if (!readable(option, 4)) return false;
    const int state = MEM_H(0, guest(option));
    // The native option draw routine admits precisely states 2..11.
    if (state < 2 || state >= 12) return false;
    out = {marker, actor, option, prefix}; return true;
}
template<class Visitor> void menus_in_queue(uint8_t* rdram, Visitor visit) {
    const uint32_t vec = MEM_W(0, guest(kMenuQueue));
    if (!readable(vec, 16) || MEM_W(0, guest(vec)) != 8) return;
    const uint32_t begin = MEM_W(4, guest(vec)), end = MEM_W(8, guest(vec));
    if (end < begin || !readable(begin, 0) || !readable(end, 0) || ((end - begin) & 7u)) return;
    for (uint32_t p = begin; p < end; p += 8) {
        const uint32_t entry = MEM_W(0, guest(p));
        // World draw records with bit 0 set contain an actor marker, not another renderable type.
        if (readable(entry, 12) && (MEM_W(8, guest(entry)) & 1)) visit(uint32_t(MEM_W(0, guest(entry))));
    }
}
bool queued(uint8_t* rdram, uint32_t marker) {
    bool found = false;
    menus_in_queue(rdram, [&](uint32_t candidate) { if (candidate == marker) found = true; });
    return found;
}
} // namespace

namespace tooie_dev_travel_menu {
void reset() { pending.clear(); }
}

extern "C" void tooie_dev_on_draw_queue(uint8_t* rdram, recomp_context* ctx) {
    if (!enabled(rdram)) { pending.clear(); func_800EB51C(rdram, ctx); return; }
    const auto input = *ctx;
    const int view = MEM_W(0, guest(kView));
    // Queue 1 is the first queue rendered after this viewport's collection. Capture
    // only J1's current-frame menus before J2 rebuilds the shared queues.
    if (input.r4 == 1 && view == 0) {
        pending.clear();
        menus_in_queue(rdram, [&](uint32_t marker) {
            Menu menu{};
            if (active_menu(rdram, input, marker, menu)) {
                for (const auto& old : pending) if (old.marker == marker) return;
                pending.push_back(menu);
            }
        });
    }
    func_800EB51C(rdram, ctx);
    if (input.r4 != 3 || view != 4) return;

    // Native global projection/scissor setup is already active here. Restore only
    // missing UI callbacks, never world geometry or the guest focus/player state.
    const int16_t previous = MEM_H(0, guest(kPass));
    for (const auto& menu : pending) {
        Menu current{};
        if (queued(rdram, menu.marker) || !active_menu(rdram, input, menu.marker, current) ||
            current.actor != menu.actor || current.option != menu.option || current.prefix != menu.prefix) continue;
        MEM_H(0, guest(kPass)) = 3;
        auto c = call(input, menu.marker); c.r5 = input.r5;
        func_800EC7D4(rdram, &c); // Native actor draw gate, callback and option renderer.
    }
    MEM_H(0, guest(kPass)) = previous;
    pending.clear();
}
