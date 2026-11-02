#include "support/story_coop_fixture.h"
// Real native queue insertion/clear/dispatch and Warp Pad/Silo draw callbacks.
// Actor lookup/extension storage, world visibility and the final UI renderer are synthetic.
#include "tooie_dev_travel_menu.h"
#include "tooie_dev_cheats.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); std::exit(1); } } while (0)
static gpr guest(uint32_t p) { return static_cast<int32_t>(p); }
static constexpr uint32_t kQueues = 0x80132E80, kView = 0x80127674, kPass = 0x80132E9C;
static constexpr uint32_t kFirst = 0x80020000, kSecond = 0x80020100, kGfx = 0x80022000, kPoint = 0x80023000;
static constexpr uint32_t kActors[] = {0x80030000, 0x80031000, 0x80032000};
static constexpr uint32_t kMarkers[] = {0x80040000, 0x80041000, 0x80042000};
static constexpr uint32_t kRecords[] = {0x80050000, 0x80050100, 0x80050200};
static constexpr uint32_t kDescriptors[] = {0x804012FC, 0x80501384, 0x80601000};
static constexpr uint32_t kOptions[] = {0x80060000, 0x80061000, 0x80062000};
static constexpr uint32_t kPrefixes[] = {0x01C401E5, 0x00B9019D, 0x12345678};
static std::array<unsigned, 3> draws{};
static unsigned model_draws = 0;
static bool hook = true;
static unsigned actor_index(uint32_t actor) {
    for (unsigned i = 0; i < 3; ++i) if (kActors[i] == actor) return i;
    CHECK(false); return 0;
}
extern "C" {
void func_800EB51C(uint8_t*, recomp_context*);
void func_800EB800(uint8_t*, recomp_context*);
void func_800EB3D0(uint8_t*, recomp_context*);
void func_800EC7D4(uint8_t*, recomp_context*);
void func_8010A79C(uint8_t*, recomp_context*);
void func_800EB854(uint8_t*, recomp_context*);
void vector_begin(uint8_t*, recomp_context*);
void vector_end(uint8_t*, recomp_context*);
void vector_clear(uint8_t*, recomp_context*);
void vector_push_back(uint8_t*, recomp_context*);
void func_8080000C_chwarppad(uint8_t*, recomp_context*);
void func_808011C8_chsilo(uint8_t*, recomp_context*);
void func_80106790(uint8_t*, recomp_context* c) {
    for (unsigned i = 0; i < 3; ++i) if (uint32_t(c->r4) == kMarkers[i]) { c->r2 = guest(kActors[i]); return; }
    CHECK(false);
}
void func_80100368(uint8_t*, recomp_context* c) { c->r2 = guest(kDescriptors[actor_index(uint32_t(c->r4))]); }
void func_80100094(uint8_t*, recomp_context* c) {
    CHECK(c->r5 == 0); c->r2 = guest(kOptions[actor_index(uint32_t(c->r4))]);
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
extern "C" recomp_func_t* get_function(int32_t address) {
    switch (uint32_t(address)) {
    case 0x800B28B8: return vector_clear;
    case 0x800B28C4: return vector_begin;
    case 0x800B296C: return vector_end;
    case 0x800B2974: return vector_push_back;
    case 0x800EC7D4: return func_800EC7D4;
    case 0x80106790: return func_80106790;
    case 0x8010A79C: return func_8010A79C;
    case 0x80100368: return func_80100368;
    case 0x80100094: return func_80100094;
    case 0x800EB854: return func_800EB854;
    case 0x8040000C: return func_8080000C_chwarppad;
    case 0x805011C8: return func_808011C8_chsilo;
    case 0x80600000: return [](uint8_t*, recomp_context*) { ++draws[2]; };
    case 0x80087CA0: return [](uint8_t* rdram, recomp_context* c) {
        CHECK(uint32_t(c->r5) == kGfx); CHECK(MEM_H(0, guest(kPass)) == 3);
        for (unsigned i = 0; i < 3; ++i) if (uint32_t(c->r4) == kOptions[i]) { ++draws[i]; return; }
        CHECK(false);
    };
    case 0x80101870: case 0x801018A4: return [](uint8_t*, recomp_context*) { ++model_draws; };
    case 0x8001BC68: return [](uint8_t* rdram, recomp_context* c) {
        // Native queue sorting uses aligned memcpy; simulate overlapping guest words.
        std::vector<int32_t> words;
        CHECK((c->r6 & 3) == 0);
        for (unsigned i = 0; i < c->r6; i += 4) words.push_back(MEM_W(i, c->r5));
        for (unsigned i = 0; i < words.size(); ++i) MEM_W(i * 4, c->r4) = words[i];
    };
    default: std::fprintf(stderr, "Unexpected native call %08X\n", uint32_t(address)); CHECK(false); return nullptr;
    }
}
static recomp_context context() { recomp_context c{}; c.r29 = guest(0x807FF000); return c; }
static void clear(uint8_t* rdram) { auto c = context(); func_800EB800(rdram, &c); }
static void enqueue(uint8_t* rdram, unsigned actor, unsigned pass = 3) {
    auto c = context(); c.r4 = guest(kRecords[actor]); c.r5 = pass; c.r6 = guest(kPoint);
    func_800EB3D0(rdram, &c);
}
static void render(uint8_t* rdram, unsigned pass, unsigned view) {
    MEM_W(0, guest(kView)) = view;
    auto c = context(); c.r4 = pass; c.r5 = guest(kGfx); auto sp = c.r29;
    if (hook) tooie_dev_on_draw_queue(rdram, &c); else func_800EB51C(rdram, &c);
    CHECK(c.r29 == sp);
    CHECK(MEM_W(0, guest(0x801354DC)) == 0);
}
static void setup(uint8_t* rdram) {
    tooie_dev_travel_menu::reset(); draws = {}; model_draws = 0;
    tooie_dev_cheats::Settings settings{}; settings.two_players = true;
    tooie_dev_cheats::set_settings(settings); tooie_dev_cheats::set_unlocked(true);
    MEM_H(0, guest(0x80127632)) = 3; MEM_BU(0, guest(0x8012762C)) = 0;
    MEM_H(0, guest(0x801276D2)) = 2;
    MEM_W(0, guest(0x80135490)) = kFirst; MEM_W(4, guest(0x80135490)) = kSecond;
    commit_fixture_story_pair(rdram);
    MEM_W(0, guest(0x801354DC)) = 0;
    for (unsigned pass = 0; pass < 7; ++pass) {
        const uint32_t vec = 0x80070000 + pass * 0x100;
        MEM_W(pass * 4, guest(kQueues)) = vec;
        MEM_W(0, guest(vec)) = 8; MEM_W(4, guest(vec)) = vec + 0x10;
        MEM_W(8, guest(vec)) = vec + 0x10; MEM_W(12, guest(vec)) = vec + 0xF0;
    }
    for (unsigned i = 0; i < 3; ++i) {
        MEM_W(0, guest(kActors[i])) = kMarkers[i];
        MEM_W(0x64, guest(kActors[i])) = 0x00010000;
        MEM_H(0x7E, guest(kActors[i])) = int16_t(0x8000);
        MEM_W(0, guest(kDescriptors[i])) = kPrefixes[i];
        MEM_W(0x14, guest(kDescriptors[i])) = i == 0 ? 0x8040000C : i == 1 ? 0x805011C8 : 0x80600000;
        MEM_W(0, guest(kRecords[i])) = kMarkers[i]; MEM_W(8, guest(kRecords[i])) = 1;
        MEM_H(0, guest(kOptions[i])) = 3;
    }
}
static void first_camera(uint8_t* rdram, unsigned actor = 0) {
    clear(rdram); enqueue(rdram, actor); render(rdram, 1, 0);
}
int main() {
    std::vector<uint8_t> memory(8 * 1024 * 1024); auto* rdram = memory.data();
    setup(rdram); hook = false;
    first_camera(rdram); clear(rdram); render(rdram, 3, 4);
    CHECK(draws[0] == 0);
    std::puts("PASS: native second-camera clear loses J1's Warp Pad menu before global pass 3.");
    hook = true;
    for (unsigned i = 0; i < 2; ++i) {
        setup(rdram); first_camera(rdram, i); clear(rdram); render(rdram, 3, 4);
        CHECK(draws[i] == 1 && model_draws == 0);
        render(rdram, 3, 4); CHECK(draws[i] == 1); // Consumed, not retained for later frames.
    }
    std::puts("PASS: native Warp Pad/Silo callbacks draw once with J2 visibility absent.");
    setup(rdram); first_camera(rdram); clear(rdram); enqueue(rdram, 0); render(rdram, 3, 4);
    CHECK(draws[0] == 1);
    std::puts("PASS: J2 sees the same menu: native queue wins, without duplicate UI.");
    setup(rdram); first_camera(rdram, 2); clear(rdram); render(rdram, 3, 4); CHECK(draws[2] == 0);
    std::puts("PASS: unsupported actor/UI classes retain native culling.");
    for (unsigned guard = 0; guard < 7; ++guard) {
        setup(rdram);
        switch (guard) {
        case 0: tooie_dev_cheats::set_unlocked(false); break;
        case 1: { auto s = tooie_dev_cheats::settings(); s.two_players = false; tooie_dev_cheats::set_settings(s); break; }
        case 2: MEM_H(0, guest(0x80127632)) = 5; break;
        case 3: MEM_BU(0, guest(0x8012762C)) = 0x1B; break;
        case 4: MEM_W(4, guest(0x80135490)) = 0; break;
        case 5: MEM_H(0, guest(0x801276D2)) = 1; break;
        case 6: MEM_W(4, guest(0x80135490)) = kFirst; break;
        }
        first_camera(rdram); clear(rdram); render(rdram, 3, 4); CHECK(draws[0] == 0);
        enqueue(rdram, 0); render(rdram, 3, 4); CHECK(draws[0] == 1); // Native visible case unchanged.
    }
    std::puts("PASS: real Dev Mode lock/option, one viewport, missing J2, attract and native multiplayer pass through.");
    for (unsigned guard = 0; guard < 5; ++guard) {
        setup(rdram); first_camera(rdram); clear(rdram);
        switch (guard) {
        case 0: MEM_H(0, guest(kOptions[0])) = 0; break;
        case 1: MEM_BU(0x65, guest(kActors[0])) |= 2; break;
        case 2: tooie_dev_travel_menu::reset(); break;
        case 3: tooie_dev_cheats::set_unlocked(false); break;
        case 4: MEM_W(0, guest(kDescriptors[0])) = kPrefixes[2]; break;
        }
        render(rdram, 3, 4); CHECK(draws[0] == 0);
    }
    setup(rdram); first_camera(rdram); clear(rdram); render(rdram, 1, 0); render(rdram, 3, 4); CHECK(draws[0] == 0);
    std::puts("PASS: closed/deleted/replaced/reset menus, disabled debug and the next frame cannot replay stale UI.");
    setup(rdram); clear(rdram); enqueue(rdram, 0); render(rdram, 1, 1); clear(rdram); render(rdram, 3, 4);
    CHECK(draws[0] == 0); // Never capture a J2-only collection.
    setup(rdram); clear(rdram); enqueue(rdram, 0); enqueue(rdram, 1); render(rdram, 1, 0); clear(rdram);
    MEM_H(0, guest(kPass)) = 6; render(rdram, 3, 4);
    CHECK(draws[0] == 1 && draws[1] == 1 && MEM_H(0, guest(kPass)) == 6);
    std::puts("PASS: only J1 collection retained; multiple menus and original pass index restored.");
}
