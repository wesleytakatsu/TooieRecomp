#include "support/story_coop_fixture.h"
// Real actor loop, event dispatch, Glowbo behavior, range test and inventory arithmetic.
// World/assets, save/UI/effect services and actor-list reclamation are synthetic.
#include "tooie_dev_collectibles_hook.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); std::exit(1); } } while (0)
static gpr guest(uint32_t a) { return static_cast<int32_t>(a); }
static constexpr uint32_t actor = 0x80020000, extension = 0x80021000;
static constexpr uint32_t descriptor = 0x804014C0, point = 0x80023000, inventory = 0x80024000;
static constexpr uint32_t effect = 0x80025000, marker = 0x80026000;
static bool split_enabled = true, hook_installed = true;
static int updates = 0, range_checks = 0, effect_frees = 0, save_calls = 0, mega_dialogues = 0;
static uint32_t saved_item = 0, saved_type = 0, mega_flag = 0;

namespace tooie_dev_cheats {
bool two_players_active() { return split_enabled; }
}
extern "C" {
void func_800FF62C(uint8_t*, recomp_context*);
void func_8010038C(uint8_t*, recomp_context*);
void func_800EFFB4(uint8_t*, recomp_context*);
void func_800EFB24(uint8_t*, recomp_context*);
void func_800EEFD4(uint8_t*, recomp_context*);
void func_800EE7F8(uint8_t*, recomp_context*);
void func_80800880_chglowbo(uint8_t*, recomp_context*);
void func_808002F4_chglowbo(uint8_t*, recomp_context*);
void chglowbo_entrypoint_0(uint8_t*, recomp_context*);
void func_80100368(uint8_t*, recomp_context*);
void func_8010108C(uint8_t*, recomp_context*);
void func_8010114C(uint8_t*, recomp_context*);
void func_80109F18(uint8_t*, recomp_context*);
void func_80101FDC(uint8_t*, recomp_context*);
void func_80101BE0(uint8_t*, recomp_context*);
void func_8010D254(uint8_t*, recomp_context*);
void func_800FFAB0(uint8_t*, recomp_context*);
void func_800D1844(uint8_t*, recomp_context*);
void func_800D175C(uint8_t*, recomp_context*);
void func_800D16C4(uint8_t*, recomp_context*);
void func_800D16CC(uint8_t*, recomp_context*);

// Strong substitutes for the two external services called directly by the integration.
void func_80100094(uint8_t*, recomp_context* ctx) {
    CHECK(uint32_t(ctx->r4) == actor);
    ctx->r2 = guest(extension + uint32_t(ctx->r5) * 0x40);
}
void func_800C8B84(uint8_t* rdram, recomp_context* ctx) {
    CHECK(uint32_t(ctx->r4) == effect);
    CHECK((MEM_HU(0x72, guest(actor)) >> 10) == 1);
    CHECK(MEM_W(0, guest(0x80135A50)) == 0); // Release effect before marking actor for deletion.
    ++effect_frees;
}
int32_t* section_addresses = nullptr;
void switch_error(const char*, uint32_t, uint32_t) { CHECK(false); }
void do_break(uint32_t) { CHECK(false); }
void cop0_status_write(recomp_context*, gpr) { CHECK(false); }
gpr cop0_status_read(recomp_context*) { CHECK(false); return 0; }
void recomp_syscall_handler(uint8_t*, recomp_context*, int32_t) { CHECK(false); }
void pause_self(uint8_t*) { CHECK(false); }
bool ultramodern_preemption_hint() { return false; }
void ultramodern_service_preemption(uint8_t*, void*) { CHECK(false); }
}
static void zero(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
static void no_op(uint8_t*, recomp_context*) {}
static void glowbo_update(uint8_t* rdram, recomp_context* ctx) { ++updates; func_808002F4_chglowbo(rdram, ctx); }
static void range(uint8_t* rdram, recomp_context* ctx) { ++range_checks; func_800EFFB4(rdram, ctx); }
extern "C" recomp_func_t* get_function(int32_t address) {
    switch (uint32_t(address)) {
    case 0x8010038C: return func_8010038C;
    case 0x800EFFB4: return range;
    case 0x800EFB24: return func_800EFB24;
    case 0x800EEFD4: return func_800EEFD4;
    case 0x800EE7F8: return func_800EE7F8;
    case 0x80400880: return func_80800880_chglowbo;
    case 0x804002F4: return glowbo_update;
    case 0x80400A3C: return chglowbo_entrypoint_0;
    case 0x80100368: return func_80100368;
    case 0x8010108C: return hook_installed ? tooie_dev_on_actor_event : func_8010108C;
    case 0x8010114C: return func_8010114C;
    case 0x80101FDC: return func_80101FDC;
    case 0x80101BE0: return func_80101BE0;
    case 0x8010D254: return func_8010D254;
    case 0x800FFAB0: return func_800FFAB0;
    case 0x800C8B84: return func_800C8B84;
    case 0x800D1844: return func_800D1844;
    case 0x800D175C: return func_800D175C;
    case 0x800D16C4: return func_800D16C4;
    case 0x800D16CC: return func_800D16CC;
    case 0x8010D1E8: return [](uint8_t* rdram, recomp_context*) {
        // Native caller caches J1's position. The fixture supplies that position.
        for (int i = 0; i < 3; ++i) MEM_W(i * 4, guest(0x80136F50)) = MEM_W(i * 4, guest(point));
    };
    case 0x801067C4: return [](uint8_t*, recomp_context* ctx) { ctx->r2 = guest(actor); };
    case 0x80106790: return [](uint8_t*, recomp_context* ctx) { CHECK(uint32_t(ctx->r4) == marker); ctx->r2 = guest(actor); };
    case 0x80100094: return func_80100094;
    case 0x80101F64: return [](uint8_t* rdram, recomp_context* ctx) { ctx->r2 = MEM_HU(0x72, ctx->r4) >> 10; };
    case 0x800D8FF8: return [](uint8_t*, recomp_context* ctx) { ctx->f0.fl = 1.0f / 30.0f; };
    case 0x800D0BD4: return [](uint8_t*, recomp_context* ctx) { ++save_calls; saved_item = uint32_t(ctx->r4); saved_type = uint32_t(ctx->r5); };
    case 0x800DA544: return [](uint8_t*, recomp_context* ctx) { mega_flag = uint32_t(ctx->r4); };
    case 0x8008A208: return [](uint8_t*, recomp_context* ctx) { CHECK(ctx->r5 == 0xD03); ++mega_dialogues; };
    case 0x800BEC1C: case 0x800DB9B0: case 0x8010682C: case 0x80102394: case 0x80104248:
    case 0x80081D34: case 0x80089F40: // Unloaded reference guard; sector update deliberately blocked.
        return zero;
    case 0x80100120: case 0x80103014: case 0x80103040: case 0x8010DF3C:
    case 0x800888C8: case 0x8008A298: case 0x80089D18: case 0x800D24E8:
    case 0x800C8E54: case 0x800C8FB0:
        return no_op;
    default: std::fprintf(stderr, "Unexpected native call: %08X\n", uint32_t(address)); CHECK(false); return nullptr;
    }
}

int main() {
    std::vector<uint8_t> memory(0x800000);
    auto* rdram = memory.data();
    std::vector<int32_t> sections(2000);
    sections[1628] = static_cast<int32_t>(0x80400000u);
    section_addresses = sections.data();
    MEM_W(0, guest(descriptor)) = 0x0039021B;
    MEM_W(0x0C, guest(descriptor)) = 0x804002F4;
    MEM_W(0x40, guest(descriptor)) = 0x80400880;
    MEM_W(0, guest(0x8012B250)) = inventory;
    for (unsigned index : {8u, 20u}) MEM_H(0, guest(0x8011AF52 + index * 4)) = -1;
    auto reset = [&] {
        std::memset(rdram + (actor & 0x7fffff), 0, 0x100);
        std::memset(rdram + (extension & 0x7fffff), 0, 0x100);
        std::memset(rdram + (inventory & 0x7fffff), 0, 0x100);
        MEM_W(0, guest(actor)) = marker;
        MEM_W(0x10, guest(actor)) = 0x80400A3C;
        MEM_W(0x74, guest(actor)) = 0x10000000;
        MEM_W(0x7C, guest(actor)) = 0x1388C000; // Radius 5000; initialized, active, distance-gated.
        MEM_W(0, guest(extension)) = 7; // Synthetic collectible ID, passed unchanged to save service.
        MEM_W(0, guest(0x80135A50)) = 0;
        MEM_H(0, guest(0x80127632)) = 3;
        MEM_B(0, guest(0x8012762C)) = 0;
        MEM_W(0, guest(0x80135490)) = 0x80010000;
        MEM_W(4, guest(0x80135490)) = 0x80011000;
        commit_fixture_story_pair(rdram);
        MEM_W(0, guest(descriptor)) = 0x0039021B;
        MEM_W(0, guest(0x801354DC)) = 0;
        updates = range_checks = effect_frees = save_calls = mega_dialogues = 0;
        saved_item = saved_type = mega_flag = 0;
        split_enabled = hook_installed = true;
        const float distance = 6000.0f;
        std::memcpy(rdram + (point & 0x7fffff), &distance, 4);
    };
    auto count = [&](unsigned index = 8) { return MEM_H(0, guest(inventory + index * 2)); };
    auto removals = [&] { return MEM_W(0, guest(0x80135A50)); };
    auto collect = [&](int player) {
        recomp_context ctx{};
        ctx.r29 = guest(0x807FF000);
        ctx.r4 = guest(marker);
        ctx.r5 = guest(player ? 0x80027000 : 0x80028000);
        func_80109F18(rdram, &ctx);
        CHECK(ctx.r29 == guest(0x807FF000) && ctx.r2 == 1);
        CHECK(MEM_W(0, guest(0x801354DC)) == 0);
    };
    auto tick = [&] {
        recomp_context ctx{}; ctx.r29 = guest(0x807FF000);
        func_800FF62C(rdram, &ctx);
    };
    // Original native path demonstrates the bug rather than merely assuming the fixture triggers it.
    reset(); hook_installed = false;
    for (int i = 0; i < 3; ++i) { collect(1); tick(); }
    CHECK(count() == 3 && updates == 0 && removals() == 0 && range_checks == 3);

    reset(); collect(1); tick();
    CHECK(count() == 1 && removals() == 1 && updates == 0); // Reported J2-far case, before other controls.
    for (int player : {0, 1}) {
        for (float distance : {100.0f, 6000.0f}) {
            reset(); std::memcpy(rdram + (point & 0x7fffff), &distance, 4);
            MEM_W(4, guest(extension)) = effect;
            collect(player); collect(1 - player); collect(player); tick();
            CHECK(count() == 1 && removals() == 1 && effect_frees == 1);
            CHECK(save_calls == 1 && saved_item == 7 && saved_type == 3);
            CHECK(updates == 0); // Cleanup completed once; the normal loop skips the deletion-marked actor.
        }
    }
    // State 1 may already exist before the new hook sees this actor: suppress credit but finish cleanup.
    reset(); hook_installed = false; collect(1); hook_installed = true; collect(1);
    CHECK(count() == 1 && removals() == 1 && save_calls == 1);

    // JRL's independent sector gate can also sleep the native update. Cleanup must still finish.
    reset(); MEM_W(0x64, guest(actor)) = 0x00080000;
    collect(1); tick(); CHECK(count() == 1 && removals() == 1);

    // Mega Glowbo uses the same descriptor; its own flag, dialogue and distinct inventory stay native.
    reset(); MEM_W(0x74, guest(actor)) = 0x30000000;
    collect(1); collect(0); tick();
    CHECK(count() == 0 && count(20) == 1 && removals() == 1);
    CHECK(mega_flag == 0x56 && mega_dialogues == 1 && save_calls == 0);

    // Other actor classes, disabled experimental mode and native multiplayer keep their original lifecycle.
    for (int reason = 0; reason < 4; ++reason) {
        reset();
        if (reason == 0) MEM_W(0, guest(descriptor)) = 0x003A021B;
        if (reason == 1) split_enabled = false;
        if (reason == 2) MEM_B(0, guest(0x8012762C)) = 0x1B;
        if (reason == 3) MEM_W(4, guest(0x80135490)) = 0;
        collect(1); collect(1); tick();
        CHECK(count() == 2 && removals() == 0);
    }
    reset();
    recomp_context other{}; other.r29 = guest(0x807FF000); other.r4 = guest(actor); other.r5 = 0xB8;
    tooie_dev_on_actor_event(rdram, &other);
    CHECK(other.r2 == 0 && count() == 0 && removals() == 0);
    MEM_W(0x74, guest(actor)) = 0;
    other.r4 = guest(actor); other.r5 = 0x3E;
    tooie_dev_on_actor_event(rdram, &other);
    CHECK(int32_t(other.r2) == -1 && count() == 0 && removals() == 0);
    std::puts("PASS: native far-player reproduction; Glowbo credit/save/effect/deletion exactly once for both players, ranges, sector and Mega; original paths preserved");
}
