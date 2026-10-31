// Real menu state machine and integration hooks, with guest memory and native lifecycle stand-ins.
// This checks routing/lifecycle contracts; it does not claim to execute the native character behaviors.
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
void func_80092778(uint8_t*, recomp_context* ctx) { model_owner = uint32_t(ctx->r4); }
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

int main() {
    using namespace tooie_dev_forms;
    Menus local;
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
        MEM_W(p * 4, guest(0x80135490)) = guest(selves[p]);
        MEM_W(p * 4, guest(0x801276E8)) = p;
        write_form(rdram, selves[p], p == 0 ? 6 : 8);
        const gpr rect = guest(0x8011A420 + (p + 1) * 8);
        MEM_H(0, rect) = 0; MEM_H(2, rect) = p; MEM_H(4, rect) = 2; MEM_H(6, rect) = 1;
    }
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
    for (int p = 0; p < 2; ++p) {
        ctx.r4 = p;
        tooie_forms_on_allocate(rdram, &ctx);
        CHECK(read_form(rdram, selves[p]) == (p == 0 ? 6 : 8));
        ctx.r4 = guest(selves[p]);
        tooie_forms_on_setup(rdram, &ctx);
        CHECK(setup_handoff[p] == 100u + p);
        CHECK(read_form(rdram, selves[p]) == (p == 0 ? 6 : 8));
        CHECK(MEM_BU(0, guest(0x8012704C)) == 6);
    }
    native_revert = true;
    ctx.r4 = guest(selves[0]);
    tooie_forms_on_teardown(rdram, &ctx);
    native_revert = false;
    ctx.r4 = 0;
    tooie_forms_on_allocate(rdram, &ctx);
    CHECK(read_form(rdram, selves[0]) == 1); // respect native reversion, never force the old Bee back

    double time = 10;
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
    std::puts("PASS: independent menus, all 17 selections, Clockwork excluded, Dragon order, lifecycle routing, native reversion, Dragon isolation, viewport publication");
}
