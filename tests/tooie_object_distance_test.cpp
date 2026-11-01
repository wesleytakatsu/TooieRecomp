// Production scopes + native model cutoff/reset and actor update/residency gates.
// Synthetic RDRAM, camera/frustum, resource services and player positions; no GPU/map.
#include "tooie_object_distance.h"
#include "librecomp/config.hpp"
#include "librecomp/files.hpp"
#include "librecomp/game.hpp"
#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>

#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); std::exit(1); } } while (0)
static int checks = 0, spheres = 0, events = 0;
static bool split = false, alive = true;
static bool event_probe = false;
static int event_range_result = -1;
static float second_x = 0;
static bool submit_probe = false, player_probe = false;
static bool world_probe = true;
static bool aabb_visible = true, cell_enabled = true;
static int queued_target = 0, aabbs = 0;
static float sprite_depth = 0;
static constexpr uint32_t grid = 0x80070000, cells = 0x80071000, target_cell = cells + 15 * 8;
static float zoom = 1.0f;
static uint16_t measure = 100;
static uint32_t fade_owner = 0;
static int captured_alpha = -1, base_alpha = 255;
static float captured_distance = -1, captured_limit = -1;
struct AfterFade {};
static std::filesystem::path config_dir;
std::filesystem::path recomp::get_config_path() { return config_dir; }
std::ifstream recomp::open_input_backup_file(const std::filesystem::path& p, std::ios_base::openmode m) { return std::ifstream(p.string() + ".bak", m); }
std::ofstream recomp::open_output_file_with_backup(const std::filesystem::path& p, std::ios_base::openmode m) { return std::ofstream(p, m); }
bool recomp::finalize_output_file_with_backup(const std::filesystem::path&) { return true; }
const recomp::Version& recomp::get_project_version() { static recomp::Version v; return v; }
namespace tooie_dev_cheats { bool two_players_active() { return split; } }
static gpr guest(uint32_t p) { return static_cast<int32_t>(p); }
static constexpr uint32_t actor = 0x80020000, point = 0x80022000, model = 0x80030000, stack = 0x807FF000;
static void put(uint8_t* rdram, uint32_t p, float f) { std::memcpy(&MEM_W(0, guest(p)), &f, 4); }
static uint32_t bits(float f) { uint32_t v; std::memcpy(&v, &f, 4); return v; }
static recomp_context context() { recomp_context c{}; c.r29 = guest(stack); return c; }
extern "C" {
void func_800DE498(uint8_t*, recomp_context*);
void func_800DE318(uint8_t*, recomp_context*);
void func_8010038C(uint8_t*, recomp_context*);
void func_80100230(uint8_t*, recomp_context*);
void func_800EEF94(uint8_t*, recomp_context*);
void func_800E42C0(uint8_t*, recomp_context*);
void func_800CA6E8(uint8_t*, recomp_context*);
void func_800EFFB4(uint8_t*, recomp_context*);
void func_800EEB40(uint8_t*, recomp_context*);
void func_800EFB24(uint8_t*, recomp_context*);
void func_800EEFD4(uint8_t*, recomp_context*);
void func_800EE7F8(uint8_t*, recomp_context*);
void func_800D3104(uint8_t*, recomp_context*);
void func_800D2F44(uint8_t*, recomp_context*);
void func_800E1C00(uint8_t*, recomp_context*);
void func_800F5A00(uint8_t* rdram, recomp_context* c) { put(rdram, uint32_t(c->r5), second_x); put(rdram, uint32_t(c->r5) + 4, 0); put(rdram, uint32_t(c->r5) + 8, 0); }
void func_800F6438(uint8_t*, recomp_context* c) { c->r2 = alive; }
void func_800F6478(uint8_t*, recomp_context* c) { c->r2 = alive; }
void osVirtualToPhysical_recomp(uint8_t* rdram, recomp_context* c) {
    c->r2 = uint32_t(c->r4) & 0x1FFFFFFF;
    if (submit_probe) {
        captured_alpha = MEM_W(0, guest(0x8012C9A4));
        const auto bits = MEM_W(0xDC, c->r29); std::memcpy(&captured_distance, &bits, 4);
        const auto limit = MEM_W(0, guest(0x8012C82C)); std::memcpy(&captured_limit, &limit, 4);
        throw AfterFade{}; // Stop before geometry traversal/GPU; native alpha block already executed.
    }
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
recomp_func_t* get_function(int32_t address) {
    switch (uint32_t(address)) {
    case 0x800EEF94: return func_800EEF94;
    case 0x800E42C0: return tooie_distance_view_scale;
    case 0x800D2F44: return tooie_distance_cell;
    case 0x800EB800: return [](uint8_t*, recomp_context*) { queued_target = 0; };
    case 0x800E3980: return [](uint8_t* rdram, recomp_context* c) {
        put(rdram, uint32_t(c->r4), 5500); put(rdram, uint32_t(c->r4) + 4, 500); put(rdram, uint32_t(c->r4) + 8, 500);
    };
    case 0x800E3958: return [](uint8_t* rdram, recomp_context* c) {
        put(rdram, uint32_t(c->r4), 1); put(rdram, uint32_t(c->r4) + 4, 0); put(rdram, uint32_t(c->r4) + 8, 0);
    };
    case 0x800BD64C: return [](uint8_t* rdram, recomp_context* c) {
        MEM_W(0, c->r4) = 5; MEM_W(4, c->r4) = 0; MEM_W(8, c->r4) = 0;
    };
    case 0x800EFB58: return [](uint8_t* rdram, recomp_context* c) {
        for (int i = 0; i < 12; i += 4) MEM_W(i, c->r4) = MEM_W(i, c->r5) - MEM_W(i, c->r6);
    };
    case 0x800EFD3C: return [](uint8_t* rdram, recomp_context* c) {
        for (int i = 0; i < 12; i += 4) MEM_W(i, c->r4) = 0;
    };
    case 0x800EFA88: return [](uint8_t* rdram, recomp_context* c) {
        MEM_W(0, c->r4) = c->r5; MEM_W(4, c->r4) = c->r6; MEM_W(8, c->r4) = c->r7;
    };
    case 0x800BCC58: return [](uint8_t*, recomp_context* c) { c->r2 = 0; };
    case 0x800BDC5C: return [](uint8_t*, recomp_context* c) { c->r2 = 1000; };
    case 0x800E9DCC: return [](uint8_t*, recomp_context* c) { c->r2 = cell_enabled; };
    case 0x800EB5E0: return [](uint8_t*, recomp_context* c) { if (uint32_t(c->r4) == target_cell) ++queued_target; };
    case 0x800E3D0C: return [](uint8_t*, recomp_context* c) { ++aabbs; c->r2 = aabb_visible; };
    case 0x800E3928: return [](uint8_t*, recomp_context* c) { c->f0.fl = sprite_depth; };
    case 0x800CA6E8: return func_800CA6E8;
    case 0x8002E330: return osVirtualToPhysical_recomp; // Test-only probe, no production OS hook.
    case 0x80400000: return [](uint8_t*, recomp_context*) {}; // Actor pre-draw callback, alpha already supplied.
    case 0x800EFFB4: return tooie_distance_radius;
    case 0x800EEB40: return tooie_distance_squared;
    case 0x800EFB24: return func_800EFB24;
    case 0x800EEFD4: return func_800EEFD4;
    case 0x800EE7F8: return func_800EE7F8;
    case 0x800DE318: return func_800DE318;
    case 0x800F2100: return [](uint8_t*, recomp_context* c) { c->f0.fl = std::fabs(c->f12.fl); };
    case 0x8002DCA0: return [](uint8_t*, recomp_context* c) { c->f0.fl = std::sqrt(c->f12.fl); };
    case 0x800DF744: return [](uint8_t*, recomp_context*) {};
    case 0x800E39A8: return [](uint8_t* rdram, recomp_context* c) {
        for (unsigned i = 0; i < 3; ++i) { put(rdram, uint32_t(c->r4) + i * 4, 0); put(rdram, uint32_t(c->r5) + i * 4, i == 2 ? 1 : 0); }
    };
    case 0x800E3E8C: return [](uint8_t*, recomp_context* c) { ++spheres; c->r2 = submit_probe; };
    case 0x8010108C: return [](uint8_t* rdram, recomp_context* c) {
        ++events;
        if (event_probe) {
            auto nested = *c; nested.r29 = ADD32(c->r29, -0x40);
            nested.r4 = guest(actor + 4); nested.r5 = bits(5000); nested.r6 = guest(point);
            tooie_distance_radius(rdram, &nested);
            event_range_result = int(nested.r2);
        }
        c->r2 = 0;
    };
    case 0x80081D34: return [](uint8_t*, recomp_context* c) { c->r2 = 0; };
    default: std::fprintf(stderr, "Unexpected native call %08X\n", uint32_t(address)); CHECK(false); return nullptr;
    }
}
}

static void config_test() {
    using namespace tooie_object_distance;
    config_dir = std::filesystem::temp_directory_path() /
        ("tooie_distance_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(config_dir);
    auto boot = [](const std::string& json) {
        if (json.empty()) std::filesystem::remove(config_dir / "graphics.json");
        else std::ofstream(config_dir / "graphics.json") << json;
        recomp::config::Config c("Graphics", "graphics", true);
        set_mode(0); add_config(c); CHECK(c.load_config()); return c;
    };
    auto c = boot(""); CHECK(mode() == Mode::Original);
    for (uint32_t value = 1; value <= 3; ++value) {
        c.set_option_value(option, value); CHECK(mode() == Mode(value - 1));
        c.revert_temp_config(); CHECK(mode() == Mode(value - 1));
        c.set_option_value(option, value); CHECK(c.save_config()); CHECK(mode() == Mode(value));
        recomp::config::Config restored("Graphics", "graphics", true);
        set_mode(0); add_config(restored); CHECK(restored.load_config()); CHECK(mode() == Mode(value));
    }
    for (const char* invalid : {"{}", R"({"object_distance":42})", R"({"object_distance":"invalid"})", R"({"object_distance":true})"}) {
        auto fresh = boot(invalid); CHECK(mode() == Mode::Original);
    }
    set_mode(999); CHECK(mode() == Mode::Original);
    std::filesystem::remove(config_dir / "graphics.json"); std::filesystem::remove(config_dir);
}
static int draw(uint8_t* r, unsigned mode, float d, float limit = 10000) {
    auto* rdram = r;
    tooie_object_distance::set_mode(mode);
    put(r, point, d); put(r, point + 4, 0); put(r, point + 8, 0);
    put(r, 0x80125D50, 10000); put(r, 0x80125D60, 10000);
    for (unsigned p : {0x80125D54, 0x80125D58, 0x80125D5C}) put(r, p, 32767);
    put(r, 0x8012C82C, limit); put(r, 0x8012C830, 1); MEM_W(0, guest(0x8012C828)) = world_probe;
    MEM_W(0, guest(0x8012C834)) = submit_probe;
    MEM_W(0, guest(0x8012C9A4)) = base_alpha;
    MEM_W(0, guest(0x8012C9A8)) = fade_owner ? 0x80400000 : 0;
    MEM_W(0, guest(0x8012C9AC)) = fade_owner;
    MEM_W(0, guest(0x8012D500)) = 0x80050000; put(r, 0x80050164, zoom);
    MEM_W(0, guest(0x8012C964)) = 0;
    MEM_W(0x10, guest(model)) = 0x40; MEM_H(0x52, guest(model)) = measure; MEM_H(0x54, guest(model)) = 50;
    auto c = context(); c.r4 = guest(0x80035000); c.r5 = guest(point); c.r7 = bits(1); MEM_W(0x14, c.r29) = model;
    MEM_W(0, c.r4) = 0x80036000;
    spheres = 0; captured_alpha = -1;
    try {
        if (player_probe) tooie_object_distance::player_draw(r, &c,
            [](uint8_t* r, recomp_context* c) { tooie_object_distance::draw(r, c, func_800DE498); });
        else tooie_object_distance::draw(r, &c, func_800DE498);
        CHECK(c.r29 == guest(stack) && c.r2 == 0);
    } catch (const AfterFade&) {
        auto reset = context(); func_800DE318(r, &reset);
    }
    CHECK(MEM_W(0, guest(0x8012C82C)) == int32_t(bits(10000))); // Native reset preserved.
    return submit_probe ? captured_alpha : spheres;
}
static int gate(uint8_t* r, bool residency, float x, uint32_t flags = 0x1388C000) {
    auto* rdram = r;
    put(r, actor + 4, x); put(r, actor + 8, 0); put(r, actor + 12, 0);
    put(r, point, 0); put(r, point + 4, 0); put(r, point + 8, 0);
    MEM_W(0x7C, guest(actor)) = flags; MEM_W(0x64, guest(actor)) = 0;
    auto c = context(); c.r4 = guest(actor); c.r5 = guest(point);
    if (residency) tooie_object_distance::residency(r, &c, func_80100230);
    else tooie_object_distance::actor_range(r, &c, func_8010038C);
    CHECK(c.r29 == guest(stack));
    CHECK(uint32_t(MEM_W(0x7C, guest(actor))) == (residency ? flags : flags | 0x8000u));
    return int(c.r2);
}
static int collect(uint8_t* rdram, unsigned mode) {
    tooie_object_distance::set_mode(mode);
    MEM_W(0, guest(0x8012D500)) = 0x80050000; put(rdram, 0x80050164, zoom);
    MEM_W(0, guest(grid)) = cells;
    MEM_W(0x1C, guest(grid)) = 25; MEM_W(0x20, guest(grid)) = 1;
    MEM_W(0x28, guest(grid)) = 25; MEM_W(0x2C, guest(grid)) = 1; MEM_W(0x30, guest(grid)) = 1;
    put(rdram, 0x80125B00, 1);
    auto c = context(); c.r4 = 0; c.r5 = guest(grid); c.r31 = guest(0x80400100);
    tooie_distance_collect(rdram, &c);
    CHECK(c.r29 == guest(stack) && c.r31 == guest(0x80400100));
    return queued_target;
}
static int sprite(uint8_t* rdram, unsigned mode, float depth, int limit = 10000) {
    tooie_object_distance::set_mode(mode); sprite_depth = depth;
    MEM_W(0, guest(0x8012D500)) = 0x80050000; put(rdram, 0x80050164, zoom);
    MEM_W(0, guest(0x8012D494)) = limit; MEM_B(0, guest(0x8012D491)) = 0;
    MEM_B(0, guest(0x8012D492)) = 1; MEM_W(0, guest(0x8012D4F0)) = 1;
    MEM_H(0, guest(0x8012D48C)) = base_alpha;
    put(rdram, 0x80125D90, 0.5f); put(rdram, 0x80125D94, 0.001f);
    MEM_H(4, guest(model)) = 1; MEM_H(6, guest(model)) = 1;
    auto c = context(); c.r4 = guest(model); c.r5 = guest(point); c.r6 = bits(1); c.r7 = bits(1);
    MEM_W(0x10, c.r29) = point + 0x20;
    if (player_probe) tooie_object_distance::player_draw(rdram, &c, tooie_distance_sprite);
    else tooie_distance_sprite(rdram, &c);
    CHECK(c.r29 == guest(stack));
    CHECK(MEM_W(0, guest(0x8012D494)) == limit && MEM_BU(0, guest(0x8012D491)) == 0 && MEM_BU(0, guest(0x8012D492)) == 1);
    return c.r2 ? MEM_H(0, guest(0x8012D48C)) : -1;
}
int main() {
    config_test();
    std::vector<uint8_t> memory(0x800000); auto* rdram = memory.data(); auto* r = rdram;
    MEM_H(0, guest(0x80127632)) = 3; MEM_H(0, guest(0x80132DC2)) = 322;
    CHECK(draw(r, 0, 6000) == 0); CHECK(draw(r, 1, 6000) == 1);
    CHECK(draw(r, 1, 18000) == 0); CHECK(draw(r, 2, 18000) == 1);
    CHECK(draw(r, 3, 30000) == 1); CHECK(draw(r, 3, 33000) == 0); // Coordinate guard unchanged.
    CHECK(draw(r, 1, 2500, 1000) == 0); CHECK(draw(r, 2, 2500, 1000) == 1);
    CHECK(draw(r, 3, 2500, 0) == 0); // Explicit hidden draw remains hidden.
    for (unsigned mode = 0; mode < 4; ++mode) {
        tooie_object_distance::set_mode(mode);
        const float inside = mode == 0 ? 4000 : mode == 1 ? 9000 : mode == 2 ? 19000 : 70000;
        CHECK(gate(r, false, inside) == 1); CHECK(gate(r, true, inside) == 0);
        if (mode != 3) { CHECK(gate(r, false, inside * 3) == 0); CHECK(gate(r, true, inside * 3) == 1); }
        CHECK(gate(r, false, inside, 0x13888000) == 0); // Activity bit not bypassed.
        CHECK(gate(r, true, inside, 0x13888000) == 0);
        CHECK(gate(r, false, inside, 0x0000C000) == 1); // Zero-radius special semantics preserved.
        CHECK(gate(r, true, inside, 0x0000C000) == 1);
    }
    // Native initialization still dispatches once. Its nested range query must
    // not consume the policy intended for the later actor scheduling query.
    tooie_object_distance::set_mode(1); event_probe = true;
    const int before_events = events;
    CHECK(gate(r, false, 6000, 0x13884000) == 1);
    CHECK(events == before_events + 1 && event_range_result == 0); event_probe = false;
    // Helpers outside their exact native invocation retain physical distances.
    tooie_object_distance::set_mode(3);
    put(r, point, 6000); put(r, actor + 4, 0);
    auto c = context(); c.r4 = guest(point); func_800EEF94(r, &c); CHECK(c.f0.fl == 6000);
    c = context(); c.r4 = guest(point); c.r5 = guest(actor + 4); tooie_distance_squared(r, &c); CHECK(c.f0.fl == 36000000);
    c = context(); c.r4 = guest(point); c.r5 = bits(5000); c.r6 = guest(actor + 4); tooie_distance_radius(r, &c); CHECK(c.r2 == 0);
    // J2 affects generic extended scheduling/retention, without modifying global focus.
    split = true; MEM_W(0, guest(0x80135490)) = 0x80040000; MEM_W(4, guest(0x80135490)) = 0x80041000;
    second_x = 29500; tooie_object_distance::set_mode(1);
    CHECK(gate(r, false, 30000) == 1); CHECK(gate(r, true, 30000) == 0);
    alive = false; CHECK(gate(r, false, 30000) == 0); CHECK(gate(r, true, 30000) == 1); split = false;
    for (int excluded : {0, 1, 2, 4, 5}) {
        MEM_H(0, guest(0x80127632)) = excluded;
        CHECK(draw(r, 3, 6000) == 0); CHECK(gate(r, false, 30000) == 0); CHECK(gate(r, true, 30000) == 1);
    }
    MEM_H(0, guest(0x80127632)) = 3; MEM_B(0, guest(0x8012762C)) = 0x1B;
    CHECK(draw(r, 3, 6000) == 0); CHECK(gate(r, false, 30000) == 0);
    MEM_B(0, guest(0x8012762C)) = 0;
    CHECK(draw(r, 3, 6000) == 1); CHECK(draw(r, 0, 6000) == 0); CHECK(draw(r, 1, 6000) == 1);
    // Execute the native alpha block, stopping at its first segment conversion.
    // The previous fixture rejected at frustum and never exercised fade.
    submit_probe = true;
    CHECK(draw(r, 0, 4650) == 127); CHECK(captured_distance == 4650);
    CHECK(draw(r, 1, 9650) == 127); CHECK(captured_distance == 9650 && captured_limit == 10000);
    CHECK(draw(r, 2, 19650) == 127); CHECK(captured_distance == 19650 && captured_limit == 20000);
    CHECK(draw(r, 1, 9300) == 255); CHECK(draw(r, 1, 9999) <= 1); CHECK(draw(r, 1, 10000) == -1);
    // Small-model 4000-unit optimization must not create an opaque pop.
    measure = 10; CHECK(draw(r, 1, 7650) == 127); CHECK(draw(r, 1, 8000) == -1); measure = 100;
    measure = 50; // Keep each fade sample inside the unchanged 32767 coordinate guard.
    for (float factor : {0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 4.0f}) {
        zoom = factor;
        const float cutoff = 2 * std::min(10000 * zoom, std::max(4000.0f, 2500 * zoom));
        CHECK(std::abs(draw(r, 1, cutoff - 350) - 127) <= 1);
        CHECK(captured_distance == cutoff - 350 && std::fabs(captured_limit - cutoff) < 0.01f);
        c = context(); tooie_distance_view_scale(r, &c); CHECK(c.f0.fl == factor);
    }
    zoom = 1; measure = 100;
    // Player/view models use their original cutoff/fade even when extension is enabled.
    player_probe = true; CHECK(draw(r, 2, 4650) == 127); CHECK(draw(r, 2, 9650) == -1); player_probe = false;
    world_probe = false; CHECK(draw(r, 2, 4650) == 127); CHECK(draw(r, 2, 9650) == -1); world_probe = true;
    // Existing actors must not become transparent after an omitted draw, pause,
    // map change or viewport change. Keep scripted alpha and native radial fade.
    fade_owner = actor; base_alpha = 128; put(r, 0x8012C760, 1.0f / 30);
    MEM_W(0, guest(0x800459C8)) = 100;
    CHECK(draw(r, 3, 30000) == 128); CHECK(draw(r, 3, 30000) == 128);
    MEM_W(0, guest(0x800459C8)) = 120; CHECK(draw(r, 3, 30000) == 128);
    MEM_W(0, guest(0x80127674)) = 1; CHECK(draw(r, 3, 30000) == 128);
    put(r, 0x8012C760, 0); MEM_H(0, guest(0x80132DC2)) = 323; CHECK(draw(r, 3, 30000) == 128);
    MEM_H(0, guest(0x80132DC2)) = 322;
    MEM_W(0, guest(0x80127674)) = 0; fade_owner = 0; base_alpha = 255;
    submit_probe = false;
    // Execute native grid collection + native radial cell rejection together.
    // The target is ten cells away: outside Original/2x, inside 4x/Unlimited.
    CHECK(collect(r, 0) == 0); CHECK(collect(r, 1) == 0);
    CHECK(collect(r, 2) == 1); CHECK(collect(r, 3) == 1);
    zoom = 2; CHECK(collect(r, 0) == 0); CHECK(collect(r, 1) == 1);
    zoom = 0.5f; CHECK(collect(r, 2) == 0); CHECK(collect(r, 3) == 1); zoom = 1;
    aabb_visible = false; CHECK(collect(r, 3) == 0); aabb_visible = true;
    cell_enabled = false; CHECK(collect(r, 3) == 0); cell_enabled = true;
    CHECK(aabbs > 0);
    auto direct = context(); direct.r4 = guest(target_cell); direct.r5 = guest(point + 0x40);
    MEM_W(0, direct.r5) = 15; MEM_W(4, direct.r5) = 0; MEM_W(8, direct.r5) = 0;
    direct.r6 = 1000; direct.r7 = guest(point + 0x50);
    put(r, point + 0x50, 5500); put(r, point + 0x54, 500); put(r, point + 0x58, 500);
    tooie_distance_cell(r, &direct); CHECK(direct.r2 == 0); // Outside a collector, keep native radius.
    direct = context(); tooie_distance_view_scale(r, &direct); CHECK(direct.f0.fl == zoom);
    CHECK(sprite(r, 0, 9750) == 127); CHECK(sprite(r, 1, 19750) == 127);
    CHECK(sprite(r, 1, 20001) == -1); CHECK(sprite(r, 2, 39750) == 127);
    CHECK(sprite(r, 3, 70000) == 255); CHECK(sprite(r, 3, -1) == -1);
    for (float factor : {0.5f, 1.0f, 2.0f}) {
        zoom = factor; CHECK(sprite(r, 1, 20000 * zoom - 250) == 127);
    }
    zoom = 1;
    base_alpha = 128; CHECK(sprite(r, 1, 19750) == 64); base_alpha = 255;
    player_probe = true; CHECK(sprite(r, 3, 15000) == -1); player_probe = false;
    MEM_W(0, guest(0x80127674)) = 4; CHECK(sprite(r, 3, 15000) == -1); MEM_W(0, guest(0x80127674)) = 0;
    CHECK(sprite(r, 3, 100, 0) == -1);
    for (int excluded : {0, 1, 2, 4, 5}) {
        MEM_H(0, guest(0x80127632)) = excluded;
        CHECK(collect(r, 3) == 0); CHECK(sprite(r, 3, 15000) == -1);
    }
    MEM_H(0, guest(0x80127632)) = 3; MEM_B(0, guest(0x8012762C)) = 0x1B;
    CHECK(collect(r, 3) == 0); CHECK(sprite(r, 3, 15000) == -1); MEM_B(0, guest(0x8012762C)) = 0;
    std::printf("PASS: %d assertions (native cutoff/range/residency + production config).\n", checks);
}
