#include "support/story_coop_fixture.h"
// Native targeting/perception/chase and Shovel update. Player/world/animation
// services are synthetic; this is not a map, damage or rendering playtest.
#include "tooie_dev_enemy_ai.h"
#include <array>
#include <unordered_map>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); std::exit(1); } } while (0)
static gpr guest(uint32_t p) { return static_cast<int32_t>(p); }
static constexpr uint32_t p1 = 0x80020000, p2 = 0x80021000, origin = 0x80022000;
static float positions[2][3] = {{6000, 0, 0}, {100, 0, 0}};
static uint32_t forms[2] = {1, 1};
static bool valid[2] = {true, true};
static unsigned checks[2] = {};
static unsigned tick = 1;
static constexpr uint32_t world = 0x80030000, entries = 0x80031000;
static constexpr uint32_t marker1 = 0x80032000, marker2 = 0x80032100;
static constexpr uint32_t enemy1 = 0x80033000, enemy2 = 0x80033100, chase = 0x80400000;
static constexpr uint32_t desc = 0x80024000, extension = 0x80025000, other = 0x80023000;
static bool split = true, installed = false;
static bool check_chase_one = false;
static bool fly_chase = false;
static int observed = -1;
static uint32_t chase_result = 0;
static std::unordered_map<uint32_t, recomp_func_t*> registered;
namespace tooie_dev_cheats { bool two_players_active() { return split; } }
namespace recomp::overlays { void add_loaded_function(int32_t p, recomp_func_t* f) { registered[uint32_t(p)] = f; } }
extern "C" {
void func_800F6478(uint8_t*, recomp_context*);
void func_8010038C(uint8_t*, recomp_context*);
void func_800EFFB4(uint8_t*, recomp_context*);
void func_800EFB24(uint8_t*, recomp_context*);
void func_800EEFD4(uint8_t*, recomp_context*);
void func_80100368(uint8_t*, recomp_context* c) { c->r2 = guest(desc); }
void func_8001211C(uint8_t*, recomp_context* c) { c->r2 = tick; }
void func_80800930_chnewshovel(uint8_t*, recomp_context* c) { c->r2 = 0; }
void func_80800570_chnewshovel(uint8_t*, recomp_context*) {}
void func_80800BC0_chnewshovel(uint8_t*, recomp_context*) {}
void func_808006B0_chnewshovel(uint8_t* rdram, recomp_context* c) { MEM_H(0x72, c->r4) = uint16_t(c->r5 << 10); }
void suchase_entrypoint_0(uint8_t*, recomp_context*);
void suchase_entrypoint_1(uint8_t*, recomp_context*);
void sucoaster_entrypoint_2(uint8_t*, recomp_context*);
void sucoasterfly_entrypoint_3(uint8_t*, recomp_context*);
// Pose/behavior services around the native Ground/Fly targeting paths.
void sucoaster_entrypoint_22(uint8_t*, recomp_context*) {}
void func_8080027C_sucoaster(uint8_t*, recomp_context*) {}
void func_80801C44_sucoasterfly(uint8_t*, recomp_context*) {}
void func_808019A0_sucoasterfly(uint8_t*, recomp_context*) {}
void func_808014AC_sucoasterfly(uint8_t* rdram, recomp_context* c) { MEM_H(0x72, c->r4) = uint16_t(c->r5 << 10); }
void func_8010C9B0(uint8_t*, recomp_context*);
void func_8010C500(uint8_t*, recomp_context*);
void func_800EEB40(uint8_t*, recomp_context*);
void func_8010D290(uint8_t*, recomp_context*);
void func_800F54E4(uint8_t*, recomp_context*);
void func_800F6438(uint8_t*, recomp_context*);
void func_800F64A4(uint8_t*, recomp_context*);
void func_8010D1E8(uint8_t*, recomp_context*);
void func_8008FE68(uint8_t*, recomp_context*);
void func_800F5A00(uint8_t*, recomp_context*);
void func_8010D254(uint8_t*, recomp_context*);
void func_800EE7F8(uint8_t*, recomp_context*);
void func_8010C9FC(uint8_t*, recomp_context*);
void func_800F00A4(uint8_t*, recomp_context*);
void func_800EFC7C(uint8_t*, recomp_context*);
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
extern "C" recomp_func_t* get_function(int32_t address) {
    switch (uint32_t(address)) {
    case 0x8010D254: return func_8010D254;
    case 0x800EEB40: return func_800EEB40;
    case 0x8001211C: return [](uint8_t*, recomp_context* c) { c->r2 = tick; };
    case 0x800BE444: return [](uint8_t*, recomp_context* c) { c->r2 = guest(world); };
    case 0x800E9E88: return [](uint8_t*, recomp_context* c) { c->r2 = guest(entries); };
    case 0x800E9EB4: return [](uint8_t*, recomp_context* c) { c->r2 = guest(entries + 24); };
    case 0x80106790: return [](uint8_t*, recomp_context* c) {
        CHECK(uint32_t(c->r4) == marker1 || uint32_t(c->r4) == marker2);
        c->r2 = guest(uint32_t(c->r4) == marker1 ? enemy1 : enemy2);
    };
    case 0x800F54E4: return installed ? tooie_dev_enemy_focus : func_800F54E4;
    case 0x800F6478: return func_800F6478;
    case 0x8010D290: return func_8010D290;
    case 0x800EFFB4: return func_800EFFB4;
    case 0x800EFB24: return func_800EFB24;
    case 0x800EEFD4: return func_800EEFD4;
    case 0x800F6438: return func_800F6438;
    case 0x800F64A4: return func_800F64A4;
    case 0x8008FE68: return func_8008FE68;
    case 0x800F5A00: return func_800F5A00;
    case 0x800EE7F8: return func_800EE7F8;
    case 0x800F00A4: return func_800F00A4;
    case 0x800EFC7C: return func_800EFC7C;
    case 0x800F4244: case 0x800F424C: return [](uint8_t*, recomp_context* c) {
        CHECK(uint32_t(c->r4) == p1 || uint32_t(c->r4) == p2);
        unsigned i = uint32_t(c->r4) == p2;
        ++checks[i]; c->r2 = valid[i];
    };
    case 0x800F543C: return [](uint8_t*, recomp_context* c) {
        CHECK(c->r4 < 2); c->r2 = forms[c->r4];
    };
    case 0x800F3BB0: return [](uint8_t* rdram, recomp_context* c) {
        CHECK(uint32_t(c->r4) == p1 || uint32_t(c->r4) == p2);
        unsigned i = uint32_t(c->r4) == p2;
        for (int j = 0; j < 3; ++j) std::memcpy(&MEM_W(j * 4, c->r5), &positions[i][j], 4);
    };
    case 0x80090128: case 0x80090150: case 0x8008FD48: case 0x8008FF6C:
        return [](uint8_t*, recomp_context* c) { c->r2 = 1; };
    case 0x80100094: return [](uint8_t*, recomp_context* c) { c->r2 = c->r5 == 1 ? 0 : guest(extension); };
    case 0x8008A108: return sucoaster_entrypoint_2;
    case 0x8010C9B0: return func_8010C9B0;
    case 0x8010C500: return func_8010C500;
    case 0x801072A8: return [](uint8_t*, recomp_context* c) { c->r2 = 0; };
    case 0x80089F78: return [](uint8_t*, recomp_context* c) { c->r2 = 1; };
    case 0x80089F00: return [](uint8_t* rdram, recomp_context* c) {
        if (fly_chase) tooie_dev_enemy_ai::dispatch(rdram, c, suchase_entrypoint_1);
        else c->r2 = 1;
    };
    case 0x800D8FF8: return [](uint8_t*, recomp_context* c) { c->f0.fl = 1.0f / 30.0f; };
    case 0x800F1DCC: case 0x800F10B4: case 0x800F1FF0:
        return [](uint8_t*, recomp_context* c) { c->f0.fl = 0; };
    case 0x800D9078: case 0x80102D98: case 0x80102BF8: case 0x8010D690:
        return [](uint8_t*, recomp_context*) {};
    case 0x80089828: case 0x800897D8: return [](uint8_t*, recomp_context*) {};
    case 0x80101DDC: case 0x8010C550:
        return [](uint8_t*, recomp_context* c) { c->r2 = 1; };
    default: std::fprintf(stderr, "Unexpected call %08X\n", uint32_t(address)); CHECK(false); return nullptr;
    }
}
static recomp_context context() { recomp_context c{}; c.r29 = guest(0x807FF000); return c; }
static int target(uint8_t* r) { auto c = context(); c.r4 = 0x10000; func_8010D290(r, &c); return int32_t(c.r2); }
static void cache(uint8_t* r) { auto c = context(); func_8010D1E8(r, &c); }
static uint32_t selected_enemy(uint8_t* r) { auto c = context(); suchase_entrypoint_0(r, &c); return uint32_t(c.r2); }
static int detect(uint8_t* r) {
    auto c = context(); c.r4 = guest(origin); c.r5 = 1000; c.r6 = 500; c.r7 = guest(uint32_t(-50));
    func_8010C9FC(r, &c); return int32_t(c.r2);
}
static void probe(uint8_t* rdram, recomp_context* input) {
    CHECK(MEM_W(0, guest(0x801354DC)) == 0);
    auto c = context(); tooie_dev_enemy_focus(rdram, &c); observed = int(c.r2);
    CHECK(target(rdram) == observed);
    CHECK(std::memcmp(&MEM_W(0, guest(0x80136F50)), positions[observed], 12) == 0);
    c = context(); tooie_dev_enemy_ai::dispatch(rdram, &c, suchase_entrypoint_0); chase_result = uint32_t(c.r2);
    if (check_chase_one) {
        const auto saved_tick = MEM_W(0, guest(chase + 0x1D0));
        const auto saved_marker = MEM_W(4, guest(chase + 0x1D0));
        c = context(); c.r4 = input->r4;
        tooie_dev_enemy_ai::dispatch(rdram, &c, suchase_entrypoint_1);
        CHECK(MEM_W(0, guest(chase + 0x1D0)) == saved_tick);
        CHECK(MEM_W(4, guest(chase + 0x1D0)) == saved_marker);
        CHECK(c.r2 == (chase_result == uint32_t(MEM_W(0, input->r4))));
    }
    if (uint32_t(input->r4) == other) {
        CHECK(observed == 0 && chase_result == marker1);
        c = context(); c.r4 = guest(origin); registered.at(0x8050005C)(rdram, &c);
        c = context(); tooie_dev_enemy_focus(rdram, &c); CHECK(c.r2 == 0);
        CHECK(std::memcmp(&MEM_W(0, guest(0x80136F50)), positions[0], 12) == 0);
    }
}
extern "C" {
void func_808000A0_ch2dbaddy(uint8_t* r, recomp_context* c) { probe(r, c); }
void func_80800054_chcrystalbaddie(uint8_t* r, recomp_context* c) { probe(r, c); }
void func_8080005C_chflowerbaddie(uint8_t* r, recomp_context* c) { probe(r, c); }
void func_80800018_chseaweedbaddy(uint8_t* r, recomp_context* c) { probe(r, c); }
void func_80800064_chhillbilly(uint8_t* r, recomp_context* c) { probe(r, c); }
}
static void invoke(uint8_t* rdram, uint32_t vram, uint32_t actor = origin) {
    auto c = context(); c.r4 = guest(actor); auto sp = c.r29;
    registered.at(vram)(rdram, &c); CHECK(c.r29 == sp);
    c = context(); tooie_dev_enemy_focus(rdram, &c); CHECK(c.r2 == 0);
}
int main() {
    std::vector<uint8_t> memory(8 * 1024 * 1024); auto* r = memory.data(); auto* rdram = r;
    MEM_W(0, guest(0x80135490)) = p1; MEM_W(4, guest(0x80135490)) = p2;
    commit_fixture_story_pair(r);
    MEM_W(0, guest(0x801354DC)) = 0;
    CHECK(target(r) == 0 && checks[0] == 1 && checks[1] == 0);
    std::puts("PASS: native selector returns J1 without consulting eligible J2; J2 position is not an input.");
    cache(r); CHECK(detect(r) == 0);
    auto c = context(); c.r4 = guest(origin + 0x40); func_8010D254(r, &c);
    CHECK(MEM_W(0, guest(origin + 0x40)) == MEM_W(0, guest(0x80136F50)));
    float copied; std::memcpy(&copied, &MEM_W(0, guest(origin + 0x40)), 4); CHECK(copied == 6000.0f);
    std::puts("PASS: native actor cache contains J1 at 6000; detection ignores J2 at 100.");
    MEM_W(0, guest(0x801354DC)) = 1;
    CHECK(target(r) == 1); CHECK(detect(r) == 0);
    std::puts("PASS: changing focus affects selector but leaves old actor cache intact.");
    cache(r); CHECK(detect(r) == 1);
    std::puts("PASS: rebuilding cache from J2 at 100 makes the same native range test succeed.");
    MEM_W(0, guest(0x801354DC)) = 0; forms[0] = 0x10000; forms[1] = 1;
    CHECK(target(r) == 1);
    std::puts("PASS: selector can fall back to J2 when J1 intersects exclusion mask; it is not hardcoded to slot 0.");
    forms[0] = 1; valid[0] = false; CHECK(target(r) == 1);
    valid[1] = false; CHECK(target(r) == -1);
    std::puts("PASS: unavailable J1 falls back to J2; no eligible players returns -1.");
    static int32_t sections[4000]{}; sections[3214] = int32_t(chase); section_addresses = sections;
    float limit = 1.0e9f; std::memcpy(&MEM_W(0x1C0, guest(chase)), &limit, 4);
    MEM_W(0, guest(world)) = 1;
    MEM_W(0, guest(entries)) = marker1; MEM_W(8, guest(entries)) = 1;
    MEM_W(12, guest(entries)) = marker2; MEM_W(20, guest(entries)) = 1;
    MEM_H(0x18, guest(marker1)) = 1; MEM_H(0x18, guest(marker2)) = 1;
    MEM_W(0x64, guest(enemy1)) = 0x40000000; MEM_W(0x64, guest(enemy2)) = 0x40000000;
    float x1 = 5800, x2 = 200;
    std::memcpy(&MEM_W(4, guest(enemy1)), &x1, 4); std::memcpy(&MEM_W(4, guest(enemy2)), &x2, 4);
    MEM_W(0, guest(0x801354DC)) = 0; cache(r); CHECK(selected_enemy(r) == marker1);
    MEM_W(0, guest(0x801354DC)) = 1; cache(r); CHECK(selected_enemy(r) == marker1);
    ++tick; CHECK(selected_enemy(r) == marker2);
    std::puts("PASS: native suchase caches one enemy marker per tick; second player context reuses first result until next tick.");
    installed = true; valid[0] = valid[1] = true;
    MEM_W(0, guest(0x801354DC)) = 0; MEM_H(0, guest(0x80127632)) = 3;
    MEM_W(0, guest(origin)) = marker2; MEM_W(0, guest(other)) = marker1;
    MEM_W(0x7C, guest(origin)) = 0x1388C000;
    MEM_H(0x72, guest(origin)) = 1 << 10; MEM_W(0, guest(desc)) = 0x000A011A;
    sections[1516] = int32_t(0x80500000);
    tooie_dev_enemy_ai::loaded_overlay(0x02025560, int32_t(0x80500000), 0x10D0);
    tooie_dev_enemy_ai::loaded_overlay(0x0200F9B0, int32_t(0x80500000), 0xD40);
    auto count = registered.size();
    tooie_dev_enemy_ai::loaded_overlay(0x11111111, int32_t(0x80600000), 0x10D0); CHECK(registered.size() == count);
    tooie_dev_enemy_ai::loaded_overlay(0x02025560, int32_t(0x80800000), 0x10D0); CHECK(registered.contains(0x8080004C));
    cache(r); std::array<int32_t, 6> before{};
    for (unsigned i = 0; i < 6; ++i) before[i] = MEM_W(i * 4, guest(0x80136F50));
    invoke(r, 0x8050004C); CHECK((MEM_HU(0x72, guest(origin)) >> 10) == 2);
    invoke(r, 0x8050004C); CHECK((MEM_HU(0x72, guest(origin)) >> 10) == 3);
    invoke(r, 0x8050004C); CHECK(std::memcmp(&MEM_W(0x18, guest(origin)), positions[1], 12) == 0);
    for (unsigned i = 0; i < 6; ++i) CHECK(before[i] == MEM_W(i * 4, guest(0x80136F50)));
    positions[0][0] = 10; MEM_H(0x72, guest(origin)) = 4 << 10; invoke(r, 0x8050004C);
    CHECK(std::memcmp(&MEM_W(0x18, guest(origin)), positions[1], 12) == 0);
    forms[1] = 0x10000; invoke(r, 0x8050004C);
    CHECK(std::memcmp(&MEM_W(0x18, guest(origin)), positions[0], 12) == 0);
    forms[1] = 1;
    std::puts("PASS: native Shovel acquisition/pursuit/attack targeting uses J2; global focus/perception restored.");
    positions[0][0] = 6000; cache(r);
    c = context(); c.r4 = guest(origin); c.r5 = guest(0x80136F50); func_8010038C(r, &c); CHECK(c.r2 == 0);
    c = context(); c.r4 = guest(origin); c.r5 = guest(0x80136F50); tooie_dev_enemy_range(r, &c); CHECK(c.r2 == 1);
    MEM_W(0x7C, guest(origin)) = 0x13888000;
    c = context(); c.r4 = guest(origin); c.r5 = guest(0x80136F50); tooie_dev_enemy_range(r, &c); CHECK(c.r2 == 0);
    MEM_W(0x7C, guest(origin)) = 0x1388C000;
    std::puts("PASS: native range gate admits J2 without overriding inactive flags.");
    MEM_W(0, guest(desc)) = 0x01A9046B; MEM_H(0x72, guest(origin)) = 1 << 10;
    check_chase_one = true;
    MEM_W(0, guest(chase + 0x1D0)) = 0; MEM_W(4, guest(chase + 0x1D0)) = 0;
    tooie_dev_enemy_ai::reset(); invoke(r, 0x8050005C); CHECK(observed == 1 && chase_result == marker2);
    float other_x = 5900; std::memcpy(&MEM_W(4, guest(other)), &other_x, 4);
    invoke(r, 0x8050005C, other); CHECK(observed == 1);
    CHECK(MEM_W(0, guest(chase + 0x1D0)) == 0 && MEM_W(4, guest(chase + 0x1D0)) == 0);
    std::puts("PASS: per-player same-tick chase, nested scopes and shared native-cache restoration.");
    positions[0][0] = 110; positions[1][0] = 100; tooie_dev_enemy_ai::reset();
    invoke(r, 0x8050005C); CHECK(observed == 1);
    positions[0][0] = 90; invoke(r, 0x8050005C); CHECK(observed == 1);
    positions[0][0] = 50; invoke(r, 0x8050005C); CHECK(observed == 0);
    positions[0][0] = 6000; positions[1][0] = 100; tooie_dev_enemy_ai::reset();
    for (const auto row : {std::array<uint32_t, 4>{0x020058F0, 0xA0, 0x02F70464, 0xB10},
                          std::array<uint32_t, 4>{0x0200F1F0, 0x54, 0x03040426, 0x700},
                          std::array<uint32_t, 4>{0x01FBCDC0, 0x18, 0x02EF035C, 0xFC0}}) {
        tooie_dev_enemy_ai::loaded_overlay(row[0], int32_t(0x80600000), row[3]);
        MEM_W(0, guest(desc)) = row[2]; invoke(r, 0x80600000 + row[1]); CHECK(observed == 1);
    }
    MEM_W(0, guest(desc)) = 0x01A9046B;
    std::puts("PASS: native form exclusion, idle hysteresis and all five family bindings.");
    check_chase_one = false;
    split = false; invoke(r, 0x8050005C); CHECK(observed == 0); split = true;
    MEM_BU(0, guest(0x8012762C)) = 0x1B; invoke(r, 0x8050005C); CHECK(observed == 0);
    MEM_BU(0, guest(0x8012762C)) = 0;
    MEM_W(4, guest(0x80135490)) = 0; invoke(r, 0x8050005C); CHECK(observed == 0);
    MEM_W(4, guest(0x80135490)) = p2;
    MEM_W(0, guest(desc)) = 0x11111111; invoke(r, 0x8050005C); CHECK(observed == 0);
    MEM_W(0, guest(desc)) = 0x01A9046B; valid[1] = false; invoke(r, 0x8050005C); CHECK(observed == 0);
    valid[1] = true; tooie_dev_enemy_ai::reset(); invoke(r, 0x8050005C); CHECK(observed == 1);
    std::puts("PASS: disabled/native multiplayer/missing J2/other classes/unavailable target/reset guards.");
    // Classes observed in the user's live Jinjo Village map. Ground delegates to
    // sucoaster; Fly delegates its descriptor callback directly to a shared syscall.
    tooie_dev_enemy_ai::reset();
    MEM_W(0, guest(desc)) = 0x01A20131;
    MEM_W(0x64, guest(origin)) = 0xC1150000;
    MEM_B(0x17, guest(extension)) = -1;
    MEM_W(0x18, guest(extension)) = 0x00080000; // Native controller chooses focused target.
    tooie_dev_enemy_ai::loaded_overlay(0x01F9DE20, int32_t(0x80600000), 0x480);
    CHECK(registered.contains(0x8060000C));
    invoke(r, 0x8060000C);
    CHECK(std::memcmp(&MEM_W(0x18, guest(origin)), positions[1], 12) == 0);
    std::puts("PASS: real Digger Ground callback and native sucoaster target/position path acquire J2.");
    MEM_W(0, guest(desc)) = 0x01A10150;
    tooie_dev_enemy_ai::loaded_overlay(0x01F9D080, int32_t(0x80600000), 0xD30);
    invoke(r, 0x80600064); CHECK(observed == 1);
    MEM_W(0, guest(desc)) = 0x01A40133;
    MEM_W(0x64, guest(origin)) = 0xC3050000;
    MEM_H(0x72, guest(origin)) = 1 << 10;
    sections[3210] = int32_t(0x80600000);
    fly_chase = true;
    c = context(); c.r4 = guest(origin);
    tooie_dev_enemy_ai::dispatch(r, &c, sucoasterfly_entrypoint_3);
    CHECK((MEM_HU(0x72, guest(origin)) >> 10) == 4);
    c = context(); tooie_dev_enemy_focus(r, &c); CHECK(c.r2 == 0);
    std::puts("PASS: real shared Digger Fly update, native perception/pursuit predicate acquire J2; Hillbilly binding scoped.");
}
