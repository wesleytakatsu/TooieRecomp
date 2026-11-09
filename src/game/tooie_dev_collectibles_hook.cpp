#include "tooie_guest_memory.h"
#include "tooie_dev_collectibles_hook.h"
#include "tooie_dev_cheats.h"
#include "tooie_story_coop.h"

extern "C" void func_8010108C(uint8_t*, recomp_context*); // original actor event dispatcher
extern "C" void func_80100368(uint8_t*, recomp_context*); // class descriptor
extern "C" void func_80100094(uint8_t*, recomp_context*); // actor's extension block
extern "C" void func_800C8B84(uint8_t*, recomp_context*); // release Glowbo's optional effect
extern "C" void func_800FFAB0(uint8_t*, recomp_context*); // native deferred actor deletion

namespace {
gpr guest(uint32_t address) { return static_cast<int32_t>(address); }
bool readable(uint32_t address, uint32_t size) {
    return tooie_memory::aligned(address, size);
}
bool enabled(uint8_t* rdram) {
    if (!tooie_story_coop::running(rdram)) return false;
    const int mode = MEM_H(0, guest(0x80127632));
    if ((mode != 3 && mode != 5) || MEM_BU(0, guest(0x8012762C)) == 0x1B) return false;
    const uint32_t first = MEM_W(0, guest(0x80135490));
    const uint32_t second = MEM_W(4, guest(0x80135490));
    return readable(first, 4) && readable(second, 4) && first != second;
}
unsigned state(uint8_t* rdram, uint32_t actor) { return MEM_HU(0x72, guest(actor)) >> 10; }
bool deleted(uint8_t* rdram, uint32_t actor) { return (MEM_BU(0x65, guest(actor)) & 2) != 0; }
recomp_context call(const recomp_context& input, uint32_t actor) {
    auto local = input;
    local.r29 = ADD32(input.r29, -0x40);
    local.r4 = guest(actor);
    return local;
}
} // namespace

extern "C" void tooie_dev_on_actor_event(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t actor = static_cast<uint32_t>(ctx->r4);
    // Most actor events take the original path without reading any additional guest state.
    if (ctx->r5 != 0x3E || !enabled(rdram) || !readable(actor, 0x80) ||
        (MEM_W(0x74, guest(actor)) & 0x10000000u) == 0) {
        func_8010108C(rdram, ctx);
        return;
    }
    const auto input = *ctx;
    auto local = call(input, actor);
    func_80100368(rdram, &local);
    const uint32_t descriptor = static_cast<uint32_t>(local.r2);
    // Canonical normal Glowbo descriptor prefix (also used by Mega Glowbo).
    // Class identity avoids hooking a transient overlay VRAM shared by unrelated actors.
    if (!readable(descriptor, 4) || uint32_t(MEM_W(0, guest(descriptor))) != 0x0039021Bu) {
        func_8010108C(rdram, ctx);
        return;
    }
    if (state(rdram, actor) == 1 || deleted(rdram, actor)) {
        ctx->r2 = 1; // Already handled; do not run the credit/save/effect callback a second time.
    } else {
        func_8010108C(rdram, ctx);
    }
    if (state(rdram, actor) != 1 || deleted(rdram, actor)) return;

    // Native func_808002F4_chglowbo's entire state-1 cleanup: release its optional
    // effect, then mark the actor for deferred deletion. That update can be asleep
    // outside J1's radius/sector even though J2 has already collected the actor.
    // The original event still owns inventory, save flags, dialogue and sound.
    local = call(input, actor);
    local.r5 = 0;
    func_80100094(rdram, &local);
    const uint32_t extension = static_cast<uint32_t>(local.r2);
    if (!readable(extension, 8)) return;
    const uint32_t effect = MEM_W(4, guest(extension));
    if (effect != 0) {
        local = call(input, effect);
        func_800C8B84(rdram, &local);
    }
    local = call(input, actor);
    func_800FFAB0(rdram, &local);
}
