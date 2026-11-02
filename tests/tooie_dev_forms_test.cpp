#include "support/story_coop_fixture.h"
// Real menu state machine and integration hooks, with guest memory and native lifecycle stand-ins.
// The model-cache/Dragon selector regression executes generated native functions read-only.
// Asset allocation and other lifecycle APIs remain stand-ins; this is not a gameplay/render test.
#include "tooie_dev_forms.h"
#include "tooie_dev_forms_hook.h"
#include "tooie_dev_cheats.h"
#include <cstdio>
#include <cstdlib>
#include <set>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); std::exit(1); } } while (0)
static gpr guest(uint32_t a) { return static_cast<int32_t>(a); }
static constexpr uint32_t selves[2] = {0x80010000, 0x80011000};
static uint32_t last_state[2]{}, model_owner = 0;
static uint32_t setup_handoff[2]{};
static uint32_t goliath_duration[2]{};
static bool native_revert = false, native_dragon = true;
static int auxiliary_installs[2]{};
static int index_of(uint8_t* rdram, recomp_context* ctx) { return MEM_W(0x184, ctx->r4); }
static uint8_t read_form(uint8_t* rdram, uint32_t self) { return MEM_BU(0x29, MEM_W(0xA0, guest(self))); }
static void write_form(uint8_t* rdram, uint32_t self, uint8_t f) { MEM_B(0x29, MEM_W(0xA0, guest(self))) = f; }
extern "C" {
void func_800F43A8(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = guest(selves[int(ctx->r4)]);
    write_form(rdram, uint32_t(ctx->r2), MEM_BU(0, guest(0x8012704C)));
}
void _basetup_entrypoint_2(uint8_t* rdram, recomp_context* ctx) {
    setup_handoff[index_of(rdram, ctx)] = MEM_W(0, guest(0x80127044));
    write_form(rdram, uint32_t(ctx->r4), MEM_BU(0, guest(0x8012704C)));
}
void func_80098E64(uint8_t* rdram, recomp_context* ctx) {
    if (native_revert) write_form(rdram, uint32_t(ctx->r4), 1);
    auto local = *ctx;
    local.r4 = read_form(rdram, uint32_t(ctx->r4));
    tooie_forms_on_transition_form(rdram, &local);
    MEM_W(0, guest(0x80127044)) = 100 + index_of(rdram, ctx);
}
void func_8009E8D4(uint8_t*, recomp_context*) {}
void func_8009E9D8(uint8_t* rdram, recomp_context* ctx) { MEM_B(0, guest(0x8012704C)) = uint8_t(ctx->r4); }
void func_8009EB00(uint8_t* rdram, recomp_context* ctx) { MEM_W(0, guest(0x80127068)) = ctx->f12.u32l; }
void func_8009EA2C(uint8_t*, recomp_context* ctx) { ctx->r2 = native_dragon; }
void func_8009EAAC(uint8_t*, recomp_context* ctx) { native_dragon = ctx->r4 != 0; }
void func_800A3410(uint8_t* rdram, recomp_context* ctx) { write_form(rdram, uint32_t(ctx->r4), uint8_t(ctx->r5)); }
void func_80092778(uint8_t*, recomp_context*);
void func_80092EC8(uint8_t*, recomp_context*);
void func_80093FD4(uint8_t*, recomp_context*);
void func_80093F30(uint8_t*, recomp_context*);
void func_8009359C(uint8_t*, recomp_context*) {}
void _badata_entrypoint_34(uint8_t* rdram, recomp_context* ctx) { ctx->r2 = 0x100 + read_form(rdram, uint32_t(ctx->r4)); }
void _badata_entrypoint_5(uint8_t* rdram, recomp_context* ctx) { MEM_W(0, ctx->r5) = 100; MEM_W(0, ctx->r6) = 0x3F800000; }
void baanim_playForDuration_loopSmooth(uint8_t*, recomp_context*) {}
void func_800A8380(uint8_t*, recomp_context* ctx) { ctx->r2 = ctx->r4 + 1; }
void bs_setState(uint8_t* rdram, recomp_context* ctx) {
    last_state[index_of(rdram, ctx)] = uint32_t(ctx->r5);
    if (ctx->r5 == 0x10E) goliath_duration[index_of(rdram, ctx)] = MEM_W(0, guest(0x80127068));
    if (ctx->r5 == 0xC8) write_form(rdram, uint32_t(ctx->r4), 9);
}
}

// Native reload executes its actual same-ID early return and native Dragon table lookup.
// Synthetic assets avoid requiring a ROM; no model data is interpreted or rendered here.
static void model_pair(uint8_t* rdram, recomp_context* ctx) {
    model_owner = uint32_t(ctx->r4);
    const uint8_t f = uint8_t(ctx->r5);
    auto local = *ctx;
    tooie_forms_on_dragon_get(rdram, &local);
    MEM_H(0, ctx->r6) = f == 1 ? 0x607 : f == 0xB ? (local.r2 ? 0x83A : 0x61C) : 0x700 + f;
    MEM_H(0, ctx->r7) = 0;
}
static void install_auxiliary(uint8_t* rdram, recomp_context* ctx) {
    const gpr model = MEM_W(0x50, ctx->r4);
    MEM_W(0x10C, model) = ctx->r5;
    MEM_W(0x110, model) = 0;
    ++auxiliary_installs[index_of(rdram, ctx)];
}
static void no_op(uint8_t*, recomp_context*) {}
static void zero(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
static void one(uint8_t*, recomp_context* ctx) { ctx->r2 = 1; }
static void dummy_asset(uint8_t*, recomp_context* ctx) { ctx->r2 = guest(0x80018000); }
static void unit_float(uint8_t*, recomp_context* ctx) { ctx->f0.fl = 1.0f; }
extern "C" {
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
    case 0x80084F38: return model_pair;
    case 0x80092EC8: return func_80092EC8;
    case 0x80093FD4: return func_80093FD4;
    case 0x80093F30: return func_80093F30;
    case 0x8009EA2C: return tooie_forms_on_dragon_get;
    case 0x800A3274: return [](uint8_t* rdram, recomp_context* ctx) { ctx->r2 = read_form(rdram, uint32_t(ctx->r4)); };
    case 0x800941C8: return install_auxiliary;
    case 0x800EA05C: return zero; // Synthetic map matches the first LOD row.
    case 0x80092B04: case 0x800B2840: return dummy_asset;
    case 0x800B27E0: case 0x800B26F0: case 0x800A25D0: return zero;
    case 0x80100D24: return one;
    case 0x800B2354: return unit_float;
    case 0x8009312C: case 0x800D70F8: case 0x800D6CEC: case 0x80100E18:
    case 0x800ADD80: case 0x800B237C: case 0x800EE7F8: return no_op;
    default: std::fprintf(stderr, "Unexpected native call: 0x%08X\n", uint32_t(address)); CHECK(false); return nullptr;
    }
}
}

int main() {
    using namespace tooie_dev_forms;
    Menus local;
    // Every configured direction opens only its owner's menu. Holding the
    // opening Up/Down press must not immediately move the selected entry.
    for (auto direction : {tooie_dev_cheats::Direction::Up, tooie_dev_cheats::Direction::Down,
        tooie_dev_cheats::Direction::Left, tooie_dev_cheats::Direction::Right}) {
        local.reset();
        local.publish(0, true, {}, 1, false);
        local.publish(1, true, {}, 1, false);
        const auto trigger = tooie_dev_cheats::direction_button(direction);
        const uint16_t other = trigger == 0x0400 ? 0x0800 : 0x0400;
        CHECK(!local.input(1, true, true, other, 0, 0, 0, trigger));
        local.input(1, true, false, 0, 0, 0, .1, trigger);
        CHECK(local.input(1, true, true, trigger, 0, 0, .2, trigger));
        CHECK(local.snapshot()[1].open && !local.snapshot()[0].open);
        CHECK(local.input(1, true, true, trigger, 0, 0, .3, trigger));
        CHECK(local.snapshot()[1].selected == 0);
        local.input(1, true, false, 0, 0, 0, .4, trigger);
        local.input(1, true, false, 0x4000, 0, 0, .5, trigger);
        CHECK(!local.snapshot()[1].open);
    }
    // The configured opener wins over a cheat on the same direction, for both owners.
    {
        auto settings = tooie_dev_cheats::Settings{};
        settings.character_menu_direction = tooie_dev_cheats::Direction::Left;
        settings[tooie_dev_cheats::Cheat::Fly] = {true, tooie_dev_cheats::Direction::Left};
        tooie_dev_cheats::set_settings(settings);
        tooie_dev_cheats::set_unlocked(true);
        local.reset();
        for (int player = 0; player < kPlayers; ++player) {
            local.publish(player, true, {}, 1, false);
            tooie_dev_cheats::on_player_input(player, false, 0);
            const auto button = tooie_dev_cheats::direction_button(settings.character_menu_direction);
            const bool consumed = local.input(player, true, true, button, 0, 0, 1, button);
            tooie_dev_cheats::on_player_input(player, !consumed, button);
            CHECK(consumed && local.snapshot()[player].open);
            CHECK(!tooie_dev_cheats::take_request(player, tooie_dev_cheats::Cheat::Fly));
        }
        tooie_dev_cheats::set_settings(tooie_dev_cheats::Settings{});
    }
    local.reset();
    local.publish(0, true, {}, 1, false);
    local.publish(1, true, {0, .5f, 1, .5f}, 1, false);
    CHECK(!local.input(1, false, true, 0x0400, 0, 0, 0));
    local.input(1, true, false, 0, 0, 0, .1);
    CHECK(local.input(1, true, true, 0x0400, 0, 0, .2));
    CHECK(local.snapshot()[1].open && !local.snapshot()[0].open);
    CHECK(local.snapshot()[1].selected == 0);
    local.input(1, true, true, 0x0400, 0, 0, .3);
    CHECK(local.snapshot()[1].selected == 0); // opening press must not navigate
    local.input(1, true, false, 0, 0, 0, .4);
    local.input(1, true, false, 0x0400, 0, 0, .5);
    local.input(1, true, false, 0x8000, 0, 0, .6);
    CHECK(local.take(1) == 1 && !local.take(0));
    CHECK(local.input(1, true, false, 0x8000, 0, 0, .7)); // no A leak to game
    CHECK(local.input(1, true, false, 0, 0, 0, .8));
    CHECK(!local.input(1, true, false, 0, 0, 0, .9));
    local.input(0, true, true, 0x0400, 0, 0, 1);
    local.input(1, true, true, 0x0400, 0, 0, 1);
    CHECK(local.snapshot()[0].open && local.snapshot()[1].open);
    local.input(0, true, false, 0x4000, 0, 0, 1.1);
    CHECK(!local.snapshot()[0].open && local.snapshot()[1].open);
    local.input(1, true, false, 0x8000, 0, 0, 1.2);
    local.reset();
    CHECK(!local.take(1) && !local.snapshot()[1].open); // stale map confirmation discarded
    local.request_open(0);
    local.publish(0, true, {}, 6, false);
    CHECK(local.snapshot()[0].open && local.snapshot()[0].selected == 7);
    CHECK(local.input(0, true, false, 0x8000, 0, 0, 2));
    CHECK(!local.take(0)); // ESC button's A press cannot immediately confirm a form
    local.input(0, true, false, 0, 0, 0, 2.1);
    local.input(0, true, false, 0x4000, 0, 0, 2.2);
    std::set<std::pair<int, bool>> catalog;
    for (auto f : kForms) {
        CHECK(f.id != 0x11); // Clockwork requires the remote-owner lifecycle; never replace the owner with it.
        CHECK(catalog.insert({f.id, f.dragon}).second);
    }
    CHECK(catalog.size() == 17);
    CHECK(kForms[1].id == 1 && kForms[1].dragon);
    CHECK(kForms[3].id == 0xB && !kForms[3].dragon);
    CHECK(kForms[4].id == 0xB && kForms[4].dragon);
    CHECK(kForms[5].id == 0xD && kForms[6].id == 2);
    local.publish(0, true, {}, 6, true);
    CHECK(local.snapshot()[0].selected == 7); // global Dragon ability is not a Bee variant

    std::vector<uint8_t> memory(8 * 1024 * 1024);
    auto* rdram = memory.data();
    recomp_context ctx{};
    ctx.r29 = guest(0x807FF000);
    tooie_dev_cheats::set_unlocked(true);
    for (int p = 0; p < 2; ++p) {
        MEM_W(0x184, guest(selves[p])) = p;
        MEM_W(0xA0, guest(selves[p])) = guest(0x80012000 + p * 0x100);
        MEM_W(0x50, guest(selves[p])) = guest(0x80014000 + p * 0x200);
        MEM_W(p * 4, guest(0x80135490)) = guest(selves[p]);
        MEM_W(p * 4, guest(0x801276E8)) = p;
        write_form(rdram, selves[p], p == 0 ? 6 : 8);
        const gpr rect = guest(0x8011A420 + (p + 1) * 8);
        MEM_H(0, rect) = 0; MEM_H(2, rect) = p; MEM_H(4, rect) = 2; MEM_H(6, rect) = 1;
    }
    commit_fixture_story_pair(rdram);
    auto pair_settings = tooie_dev_cheats::settings(); pair_settings.two_players = true;
    tooie_dev_cheats::set_settings(pair_settings);
    MEM_H(0, guest(0x80117DF0)) = 0; // Map -> LOD row 0.
    MEM_H(2, guest(0x80117DF0)) = 0;
    MEM_H(4, guest(0x80117DF0)) = -1;
    MEM_W(0, guest(0x80117DD8)) = 0x900; // Synthetic normal/Dragon auxiliary IDs.
    MEM_W(0, guest(0x80117DDC)) = 0x901;
    MEM_H(0, guest(0x80127632)) = 3;
    begin_map(false); // Loaded before accepting Dev Mode / transitional load-time mode.
    update_player(rdram, &ctx, selves[0]);
    CHECK(menus().snapshot()[0].available);
    auto settings = tooie_dev_cheats::settings();
    settings.character_menu = false;
    tooie_dev_cheats::set_settings(settings);
    update_player(rdram, &ctx, selves[0]);
    CHECK(!menus().snapshot()[0].available);
    settings.character_menu = true;
    tooie_dev_cheats::set_settings(settings);
    begin_map(true);
    for (int p = 0; p < 2; ++p) {
        ctx.r4 = guest(selves[p]);
        tooie_forms_on_teardown(rdram, &ctx);
    }
    CHECK(MEM_BU(0, guest(0x8012704C)) == 6); // J2 did not replace J1 transition form
    CHECK(MEM_W(0, guest(0x80127044)) == 100); // its animation/movement handoff did not leak either
    tooie_story_coop::begin_creation(0, true);
    for (int p = 0; p < 2; ++p) {
        tooie_story_coop::begin_normal_allocation();
        ctx.r4 = p;
        tooie_forms_on_allocate(rdram, &ctx);
        tooie_story_coop::end_normal_allocation(rdram, p);
        CHECK(read_form(rdram, selves[p]) == (p == 0 ? 6 : 8));
        ctx.r4 = guest(selves[p]);
        tooie_forms_on_setup(rdram, &ctx);
        CHECK(setup_handoff[p] == 100u + p);
        CHECK(read_form(rdram, selves[p]) == (p == 0 ? 6 : 8));
        CHECK(MEM_BU(0, guest(0x8012704C)) == 6);
    }
    CHECK(tooie_story_coop::finish_creation(rdram));
    native_revert = true;
    ctx.r4 = guest(selves[0]);
    tooie_forms_on_teardown(rdram, &ctx);
    native_revert = false;
    ctx.r4 = 0;
    tooie_forms_on_allocate(rdram, &ctx);
    CHECK(read_form(rdram, selves[0]) == 1); // respect native reversion, never force the old Bee back

    commit_fixture_story_pair(rdram);
    double time = 10;
    auto select = [&](int p, int target) {
        update_player(rdram, &ctx, selves[p]);
        menus().input(p, true, false, 0, 0, 0, time += 1);
        menus().input(p, true, true, 0x0400, 0, 0, time += 1);
        while (menus().snapshot()[p].selected != target) {
            menus().input(p, true, false, 0, 0, 0, time += 1);
            menus().input(p, true, false, 0x0400, 0, 0, time += 1);
        }
        menus().input(p, true, false, 0x8000, 0, 0, time += 1);
        update_player(rdram, &ctx, selves[p]);
    };
    auto primary = [&](int p) { return MEM_H(0xC, MEM_W(0x50, guest(selves[p]))); };
    auto auxiliary = [&](int p) { return MEM_W(0x10C, MEM_W(0x50, guest(selves[p]))); };
    select(0, 0);
    select(1, 0);
    CHECK(primary(0) == 0x607 && primary(1) == 0x607);
    CHECK(auxiliary(0) == 0x900 && auxiliary(1) == 0x900);
    for (int p : {1, 0}) {
        const int other = 1 - p;
        const int other_installs = auxiliary_installs[other];
        select(p, 1); // Normal -> Dragon without changing the primary model.
        CHECK(primary(p) == 0x607 && auxiliary(p) == 0x901);
        CHECK(auxiliary(other) == 0x900 && auxiliary_installs[other] == other_installs);
        select(p, 0); // Dragon -> normal has the same cache hazard.
        CHECK(primary(p) == 0x607 && auxiliary(p) == 0x900);
        CHECK(auxiliary(other) == 0x900 && auxiliary_installs[other] == other_installs);
        CHECK(native_dragon); // Neither direction writes the shared save flag.
    }
    select(0, 1);
    select(1, 1);
    CHECK(auxiliary(0) == 0x901 && auxiliary(1) == 0x901);
    select(0, 0);
    CHECK(auxiliary(0) == 0x900 && auxiliary(1) == 0x901);
    const int installs_before_idle[2] = {auxiliary_installs[0], auxiliary_installs[1]};
    update_player(rdram, &ctx, selves[0]);
    update_player(rdram, &ctx, selves[1]);
    CHECK(auxiliary_installs[0] == installs_before_idle[0] && auxiliary_installs[1] == installs_before_idle[1]);
    for (int target = 0; target < int(kForms.size()); ++target) {
        const int p = target % 2, other = 1 - p;
        const uint8_t untouched = read_form(rdram, selves[other]);
        update_player(rdram, &ctx, selves[p]);
        menus().input(p, true, false, 0, 0, 0, time += 1);
        menus().input(p, true, true, 0x0400, 0, 0, time += 1);
        int selected = menus().snapshot()[p].selected;
        while (selected != target) {
            menus().input(p, true, false, 0, 0, 0, time += 1);
            menus().input(p, true, false, 0x0400, 0, 0, time += 1);
            selected = menus().snapshot()[p].selected;
        }
        menus().input(p, true, false, 0x8000, 0, 0, time += 1);
        const uint32_t untouched_duration = goliath_duration[other];
        MEM_W(0, guest(0x80127068)) = 0; // Regression: a menu selection has no spell handoff duration.
        update_player(rdram, &ctx, selves[p]);
        CHECK(MEM_W(0, guest(0x80127068)) == 0); // temporary seed must not leak into another transition
        CHECK(goliath_duration[other] == untouched_duration);
        if (kForms[target].id == 0xE) CHECK(goliath_duration[p] == 0x42F00000); // float 120 seconds
        CHECK(model_owner == selves[p]);
        CHECK(read_form(rdram, selves[p]) == kForms[target].id);
        CHECK(read_form(rdram, selves[other]) == untouched);
        CHECK(last_state[p] == (kForms[target].id == 9 ? 0xC8u : 0x100u + kForms[target].id));
        PlayerScope scope(rdram, selves[p]);
        tooie_forms_on_dragon_get(rdram, &ctx);
        CHECK(bool(ctx.r2) == kForms[target].dragon);
        CHECK(native_dragon); // menu never modifies shared save flag
        auto rect = menus().snapshot()[p].rect;
        CHECK(rect.width == 1 && rect.height == .5f && rect.y == p * .5f);
    }
    write_form(rdram, selves[0], 1); // native change while playing remains authoritative
    update_player(rdram, &ctx, selves[0]);
    CHECK(read_form(rdram, selves[0]) == 1);
    tooie_forms_on_reset(rdram, &ctx);
    CHECK(!menus().snapshot()[0].available && !menus().take(0));
    std::puts("PASS: independent menus, all 17 selections, Clockwork excluded, Dragon order, lifecycle routing, native reversion, native same-model cache/Dragon selection, Dragon isolation, viewport publication");
}
