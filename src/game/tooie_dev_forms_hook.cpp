#include "tooie_dev_forms_hook.h"
#include "tooie_dev_forms.h"
#include "tooie_dev_cheats.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>

extern "C" {
void func_800F43A8(uint8_t*, recomp_context*);
void _basetup_entrypoint_2(uint8_t*, recomp_context*);
void func_80098E64(uint8_t*, recomp_context*);
void func_8009E8D4(uint8_t*, recomp_context*);
void func_8009EA2C(uint8_t*, recomp_context*);
void func_8009EAAC(uint8_t*, recomp_context*);
void func_8009E9D8(uint8_t*, recomp_context*);
void func_8009EB00(uint8_t*, recomp_context*); // transition duration in f12, read by Goliath's native entry
void func_800A3410(uint8_t*, recomp_context*);
void func_80092778(uint8_t*, recomp_context*);
void func_8009359C(uint8_t*, recomp_context*);
void _badata_entrypoint_34(uint8_t*, recomp_context*);
void _badata_entrypoint_5(uint8_t*, recomp_context*);
void baanim_playForDuration_loopSmooth(uint8_t*, recomp_context*);
void func_800A8380(uint8_t*, recomp_context*);
void bs_setState(uint8_t*, recomp_context*);
}

namespace {
gpr guest(uint32_t a) { return static_cast<int32_t>(a); }
bool pointer(uint32_t a) { return a >= 0x80000000 && a < 0x807FFE00 && !(a & 3); }
bool enabled_map = false;
bool feature_enabled() {
    static const bool enabled = [] {
        const char* value = std::getenv("TOOIE_TEST_PLAYER_FORMS");
        return value == nullptr || value[0] != '0';
    }();
    return enabled;
}
thread_local int current_player = -1;
thread_local std::optional<uint8_t>* serializing_form = nullptr;
// Native transition banks and their mode/timer fields (8009E880..8009EB24).
// These contain values, not PlayerState/model pointers. Keep the complete handoff per player, so J2's
// water/Blaster/Clockwork modes cannot change J1's initialization even when their forms differ.
using TransitionCache = std::array<uint32_t, 17>; // 80127040..80127083
TransitionCache read_transition(uint8_t* rdram) {
    TransitionCache result{};
    for (size_t i = 0; i < result.size(); ++i) result[i] = MEM_W(int(i * 4), guest(0x80127040));
    return result;
}
void write_transition(uint8_t* rdram, const TransitionCache& cache) {
    for (size_t i = 0; i < cache.size(); ++i) MEM_W(int(i * 4), guest(0x80127040)) = cache[i];
}
struct SavedForm { bool valid = false; uint8_t form = 1; std::optional<bool> dragon; TransitionCache transition{}; };
std::array<SavedForm, 2> saved{};

int slot(uint8_t* rdram, uint32_t self) {
    if (!pointer(self)) return -1;
    const int index = MEM_W(0x184, guest(self));
    return index >= 0 && index < 2 ? index : -1;
}
uint8_t form(uint8_t* rdram, uint32_t self) {
    const uint32_t state = MEM_W(0xA0, guest(self));
    return pointer(state) ? MEM_BU(0x29, guest(state)) : 0;
}
recomp_context call_context(const recomp_context* ctx, uint32_t self) {
    auto local = *ctx;
    local.r29 = ADD32(ctx->r29, -0x80);
    local.r4 = guest(self);
    return local;
}
bool dragon(uint8_t* rdram, const recomp_context* ctx, int index) {
    if (saved[index].dragon) return *saved[index].dragon;
    auto local = *ctx;
    local.r29 = ADD32(ctx->r29, -0x80);
    func_8009EA2C(rdram, &local); // original getter; never writes the save flag
    saved[index].dragon = local.r2 != 0;
    return *saved[index].dragon;
}
void trace(uint8_t* rdram, int index, const char* event, uint8_t value) {
    static const bool logging = std::getenv("TOOIE_DIAG_PLAYER_FORMS") != nullptr;
    if (logging) {
        const auto utc_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::fprintf(stderr, "[tooie-forms] utc_ms=%lld map=%u player=%d event=%s form=0x%X dragon=%d\n",
            static_cast<long long>(utc_ms), unsigned(MEM_HU(0, guest(0x80132DC2))), index + 1, event,
            unsigned(value), saved[index].dragon.value_or(false));
    }
}
// Select the player's native handoff only while initializing it. Map legality/range routines remain native.
class TransitionForm {
public:
    TransitionForm(uint8_t* memory, int index) : rdram(memory), previous(read_transition(memory)) {
        if (index >= 0 && index < 2) {
            if (saved[index].valid) {
                write_transition(rdram, saved[index].transition);
                MEM_B(0, guest(0x8012704C)) = saved[index].form;
            } else if (index == 1) MEM_B(0, guest(0x8012704C)) = 1;
        }
    }
    ~TransitionForm() { write_transition(rdram, previous); }
private:
    uint8_t* rdram;
    TransitionCache previous;
};
}

namespace tooie_dev_forms {
PlayerScope::PlayerScope(uint8_t* rdram, uint32_t self) : previous(current_player) {
    current_player = enabled_map && tooie_dev_cheats::is_unlocked() ? slot(rdram, self) : -1;
}
PlayerScope::~PlayerScope() { current_player = previous; }
void begin_map(bool enabled) {
    enabled_map = enabled && feature_enabled();
    menus().reset(); // A confirmation from the old map must never target a new PlayerState.
}
uint32_t default_state(uint8_t* rdram, const recomp_context* ctx, uint32_t self) {
    auto local = call_context(ctx, self);
    _badata_entrypoint_34(rdram, &local);
    return uint32_t(local.r2);
}
void update_player(uint8_t* rdram, const recomp_context* ctx, uint32_t self) {
    const int index = slot(rdram, self);
    if (index < 0) return;
    // The load-time game mode is transitional. Re-evaluate from live gameplay too, so accepting
    // Dev Mode in an already loaded map never requires another map load to make the selector available.
    const bool story = feature_enabled() && MEM_H(0, guest(0x80127632)) == 3 &&
        MEM_BU(0, guest(0x8012762C)) != 0x1B &&
        tooie_dev_cheats::two_players_map(MEM_HU(0, guest(0x80132DC2)));
    if (story) enabled_map = true;
    PlayerScope scope(rdram, self);
    const bool available = enabled_map && story && tooie_dev_cheats::is_unlocked() &&
        tooie_dev_cheats::settings().character_menu && MEM_BU(0, guest(0x80127642)) == 0;
    if (auto request = menus().take(index); request && available) {
        const Form target = kForms[*request];
        // End the old behavior before changing the form; waiting is the native, form-neutral extra-player state.
        auto local = call_context(ctx, self);
        local.r5 = 0x74;
        bs_setState(rdram, &local);
        saved[index].dragon = target.dragon;
        local = call_context(ctx, self);
        local.r5 = target.id == 9 ? 1 : target.id;
        func_800A3410(rdram, &local);
        local = call_context(ctx, self);
        local.r5 = 0;
        func_8009359C(rdram, &local);
        local = call_context(ctx, self);
        func_80092778(rdram, &local);
        // Same base animation refresh as the native transformation handler.
        local = call_context(ctx, self);
        local.r5 = ADD32(local.r29, 0x60);
        local.r6 = ADD32(local.r29, 0x64);
        _badata_entrypoint_5(rdram, &local);
        local = call_context(ctx, self);
        local.r5 = MEM_W(0x60, local.r29);
        local.r6 = MEM_W(0x64, local.r29);
        baanim_playForDuration_loopSmooth(rdram, &local);
        const uint32_t initial = target.id == 9 ? 0xC8 : default_state(rdram, ctx, self);
        // bsstatue's entry reads func_8009EA7C (80127068), then starts native HUD timer 2.
        // A direct menu selection has no Mumbo spell to seed that duration. Supply two minutes only
        // during this entry, so the zero handoff cannot expire the form immediately. No per-frame reset.
        const uint32_t previous_duration = MEM_W(0, guest(0x80127068));
        if (target.id == 0xE) {
            local = call_context(ctx, self);
            local.f12.fl = 120.0f;
            func_8009EB00(rdram, &local);
        }
        local = call_context(ctx, self);
        local.r5 = initial;
        bs_setState(rdram, &local);
        if (target.id == 0xE) MEM_W(0, guest(0x80127068)) = previous_duration;
        trace(rdram, index, "selected", target.id);
    }
    Rect rect{};
    if (available) {
        const int viewport = MEM_W(index * 4, guest(0x801276E8));
        if (viewport >= 0 && viewport < 4) {
            auto local = call_context(ctx, uint32_t(viewport));
            func_800A8380(rdram, &local);
            const uint32_t layout = uint32_t(local.r2);
            if (layout < 9) {
                // Native layout table, in half-screen units; follows horizontal/vertical split selection.
                const gpr entry = guest(0x8011A420 + layout * 8);
                rect = {MEM_H(0, entry) * 0.5f, MEM_H(2, entry) * 0.5f,
                    MEM_H(4, entry) * 0.5f, MEM_H(6, entry) * 0.5f};
            }
        }
    }
    menus().publish(index, available, rect, form(rdram, self), available ? dragon(rdram, ctx, index) : false);
}
}

extern "C" void tooie_forms_on_allocate(uint8_t* rdram, recomp_context* ctx) {
    const int index = int(ctx->r4);
    if (!enabled_map || !tooie_dev_cheats::is_unlocked() || index < 0 || index >= 2) { func_800F43A8(rdram, ctx); return; }
    const int previous = current_player;
    current_player = index;
    TransitionForm transition(rdram, index);
    (void)dragon(rdram, ctx, index);
    func_800F43A8(rdram, ctx);
    trace(rdram, index, "allocated", form(rdram, uint32_t(ctx->r2)));
    current_player = previous;
}
extern "C" void tooie_forms_on_setup(uint8_t* rdram, recomp_context* ctx) {
    const int index = slot(rdram, uint32_t(ctx->r4));
    tooie_dev_forms::PlayerScope scope(rdram, uint32_t(ctx->r4));
    if (enabled_map && tooie_dev_cheats::is_unlocked() && index >= 0) {
        TransitionForm transition(rdram, index);
        _basetup_entrypoint_2(rdram, ctx);
    } else _basetup_entrypoint_2(rdram, ctx);
}
extern "C" void tooie_forms_on_teardown(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t self = uint32_t(ctx->r4);
    const int index = slot(rdram, self);
    tooie_dev_forms::PlayerScope scope(rdram, self);
    const TransitionCache previous = read_transition(rdram);
    std::optional<uint8_t> serialized;
    auto* previous_serializing = serializing_form;
    serializing_form = &serialized;
    func_80098E64(rdram, ctx);
    serializing_form = previous_serializing;
    if (enabled_map && tooie_dev_cheats::is_unlocked() && index >= 0) {
        // Capture AFTER native teardown: legitimate native reversions (including Goliath) win.
        saved[index].form = serialized.value_or(form(rdram, self));
        saved[index].transition = read_transition(rdram);
        saved[index].valid = true;
        trace(rdram, index, "transition", saved[index].form);
        if (index == 1) write_transition(rdram, previous);
    }
}
extern "C" void tooie_forms_on_reset(uint8_t* rdram, recomp_context* ctx) {
    saved = {};
    enabled_map = false;
    tooie_dev_forms::menus().reset();
    func_8009E8D4(rdram, ctx);
}
extern "C" void tooie_forms_on_dragon_get(uint8_t* rdram, recomp_context* ctx) {
    if (enabled_map && current_player >= 0 && saved[current_player].dragon)
        ctx->r2 = *saved[current_player].dragon;
    else func_8009EA2C(rdram, ctx);
}
extern "C" void tooie_forms_on_dragon_set(uint8_t* rdram, recomp_context* ctx) {
    if (enabled_map && current_player >= 0) saved[current_player].dragon = ctx->r4 != 0;
    func_8009EAAC(rdram, ctx); // Preserve native progression; per-player appearance overrides stay isolated.
}
extern "C" void tooie_forms_on_transition_form(uint8_t* rdram, recomp_context* ctx) {
    if (serializing_form != nullptr) *serializing_form = uint8_t(ctx->r4);
    func_8009E9D8(rdram, ctx);
}
