#include "tooie_dev_cheats_hook.h"
#include "tooie_dev_cheats.h"
#include "tooie_story_coop.h"
#include <cstdio>

extern "C" void bs_getCurrentState(uint8_t*, recomp_context*);
extern "C" void bs_setState(uint8_t*, recomp_context*);
extern "C" void func_8009359C(uint8_t*, recomp_context*);
extern "C" void func_80092778(uint8_t*, recomp_context*);

namespace {
gpr guest(uint32_t address) { return static_cast<int32_t>(address); }
bool pointer(uint32_t address) { return address >= 0x80000000u && address < 0x807FFE00u && !(address & 3); }
std::array<bool, tooie_dev_cheats::kPlayerCount> blaster_reload_pending{};

recomp_context call_context(const recomp_context* ctx, uint32_t self) {
    auto local = *ctx;
    local.r29 = ADD32(ctx->r29, -0x40);
    local.r4 = guest(self);
    return local;
}

void reload_model(uint8_t* rdram, const recomp_context* ctx, uint32_t self) {
    auto local = call_context(ctx, self);
    local.r5 = 0;
    func_8009359C(rdram, &local);
    local = call_context(ctx, self);
    func_80092778(rdram, &local);
}
}

namespace tooie_dev_cheats {
void retire_player(int player) {
    if (player < 0 || player >= kPlayerCount) return;
    blaster_reload_pending[player] = false;
    for (size_t i = 0; i < kCheatCount; ++i) (void)take_request(player, Cheat(i));
}
void reset_gameplay() {
    blaster_reload_pending = {};
    reset_requests();
}

void update_player(uint8_t* rdram, const recomp_context* ctx, uint32_t self, bool gameplay, bool blocked) {
    if (!pointer(self)) return;
    int player = -1;
    for (int i = 0; i < kPlayerCount; ++i) {
        if (uint32_t(MEM_W(i * 4, guest(0x80135490))) == self) { player = i; break; }
    }
    if (player < 0) return;
    if (player > 0 && tooie_story_coop::owned_slot(rdram, self) != 1) {
        retire_player(player);
        return;
    }

    const uint32_t form = MEM_W(0xA0, guest(self));
    if (!pointer(form)) {
        blaster_reload_pending[player] = false;
        for (size_t i = 0; i < kCheatCount; ++i) (void)take_request(player, Cheat(i));
        return;
    }
    const uint32_t transform = MEM_BU(0x29, guest(form));
    // Finish native Blaster exit for this owner, even if its cheat has since been disabled.
    if (blaster_reload_pending[player]) {
        auto local = call_context(ctx, self);
        bs_getCurrentState(rdram, &local);
        const uint32_t current = uint32_t(local.r2);
        if (transform != kTransformBlaster && transform != kTransformBanjoKazooie) {
            blaster_reload_pending[player] = false;
        } else if (transform == kTransformBanjoKazooie && current != kStateBlasterExit) {
            blaster_reload_pending[player] = false;
            reload_model(rdram, ctx, self);
            std::fprintf(stderr, "[tooie-cheat] player=%d blaster: model reloaded in state 0x%X\n", player + 1, unsigned(current));
        }
    }

    for (size_t i = 0; i < kCheatCount; ++i) {
        const auto cheat = Cheat(i);
        if (!take_request(player, cheat)) continue;
        if (!gameplay || blocked) {
            std::fprintf(stderr, "[tooie-cheat] player=%d %s dropped (gameplay %d, blocked %d)\n",
                player + 1, cheat_key(cheat), int(gameplay), int(blocked));
            continue;
        }
        // A previous request in this frame may already have changed the behavior state.
        auto local = call_context(ctx, self);
        bs_getCurrentState(rdram, &local);
        const uint32_t current = uint32_t(local.r2);
        const uint32_t live_form = MEM_W(0xA0, guest(self));
        if (!pointer(live_form)) continue;
        const uint32_t live_transform = MEM_BU(0x29, guest(live_form));
        const auto next = cheat == Cheat::Fly ? fly_state(live_transform, current) : blaster_state(live_transform, current);
        if (!next) {
            std::fprintf(stderr, "[tooie-cheat] player=%d %s ignored (transformation 0x%X, state 0x%X)\n",
                player + 1, cheat_key(cheat), unsigned(live_transform), unsigned(current));
            continue;
        }
        local = call_context(ctx, self);
        local.r5 = *next;
        bs_setState(rdram, &local);
        std::fprintf(stderr, "[tooie-cheat] player=%d %s: state 0x%X -> 0x%X\n",
            player + 1, cheat_key(cheat), unsigned(current), unsigned(*next));
        if (*next == kStateBlasterExit) blaster_reload_pending[player] = true;
    }
}
}
