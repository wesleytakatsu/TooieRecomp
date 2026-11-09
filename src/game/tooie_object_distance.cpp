#include "tooie_expanded_memory.h"
#include "tooie_object_distance.h"
#include "tooie_story_coop.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>

extern "C" {
void func_800E42C0(uint8_t*, recomp_context*);
void func_800EFFB4(uint8_t*, recomp_context*);
void func_800EEB40(uint8_t*, recomp_context*);
void func_800D3104(uint8_t*, recomp_context*);
void func_800D2F44(uint8_t*, recomp_context*);
void func_800E1C00(uint8_t*, recomp_context*);
}

namespace {
using Mode = tooie_object_distance::Mode;
std::atomic<Mode> selected{Mode::Original};
gpr guest(uint32_t p) { return static_cast<int32_t>(p); }
bool readable(uint32_t p, uint32_t n) {
    return tooie_memory::aligned(p, n);
}
Mode active(uint8_t* rdram) {
    // Extended modes need the expanded heap. Story co-op, attract replays and native
    // multiplayer keep native distances without changing the saved preference.
    if (!rdram || !tooie_memory::boot.active() || tooie_story_coop::running(rdram)) return Mode::Original;
    const bool story = MEM_H(0, guest(0x80127632)) == 3 && MEM_BU(0, guest(0x8012762C)) != 0x1B;
    return story ? selected.load(std::memory_order_relaxed) : Mode::Original;
}
float multiplier(Mode m) { return m == Mode::Double ? 2.0f : m == Mode::Quadruple ? 4.0f : 1.0f; }
struct Model {
    Mode setting;
    gpr native_stack;
    float initial_limit;
    bool world;
    bool consumed = false;
};
struct Actor {
    Mode setting;
    uint32_t address;
    gpr math_stack;
    bool consumed = false;
};
struct Selection { Mode setting; gpr native_stack; };
thread_local Selection* selection = nullptr;
thread_local Model* model = nullptr;
thread_local bool drawing_player = false;
thread_local Actor* updating = nullptr;
thread_local Actor* retaining = nullptr;
template<class T> struct Scope {
    T*& slot;
    T* previous;
    Scope(T*& s, T& value) : slot(s), previous(s) { slot = &value; }
    ~Scope() { slot = previous; }
};

float read_float(uint8_t* rdram, gpr base, uint32_t offset) {
    const auto bits = MEM_W(offset, base); float value;
    std::memcpy(&value, &bits, 4); return value;
}
void write_float(uint8_t* rdram, gpr base, uint32_t offset, float value) {
    std::memcpy(&MEM_W(offset, base), &value, 4);
}

// Native cutoffs have a 4000-unit fast path before the size-dependent limit.
// Resolve their effective boundary once, rather than changing physical distance
// or allowing that optimization to become a discontinuity inside an extended range.
float model_limit(uint8_t* rdram, const Model& frame, float view_scale) {
    float limit = frame.initial_limit * view_scale;
    if (frame.initial_limit == read_float(rdram, guest(0x80125D60), 0)) {
        const float measure = read_float(rdram, frame.native_stack, 0xD8);
        const float scale = read_float(rdram, frame.native_stack, 0x104);
        const float size = measure * scale * read_float(rdram, guest(0x8012C830), 0) * 50.0f;
        if (std::isfinite(size) && size >= 0)
            limit = std::min(limit, std::max(4000.0f, size * view_scale));
    }
    return limit;
}

void actor_gate(uint8_t* rdram, recomp_context* ctx, recomp_func_t* next, Actor*& slot) {
    const auto setting = active(rdram);
    if (setting == Mode::Original || !tooie_memory::distance_heap_available(rdram) ||
        !readable(uint32_t(ctx->r4), 0x80)) { next(rdram, ctx); return; }
    Actor frame{setting, uint32_t(ctx->r4), 0};
    // Match the native gate's stack and position, excluding initialization/event
    // callbacks and other distance queries nested under this call.
    frame.math_stack = ADD32(ctx->r29, -0x18);
    Scope scope(slot, frame);
    next(rdram, ctx);
}
}

namespace tooie_object_distance {
void set_mode(uint32_t value) {
    const auto next = value <= uint32_t(Mode::Unlimited) ? Mode(value) : Mode::Original;
    selected.store(next, std::memory_order_relaxed);
}
Mode mode() { return selected.load(std::memory_order_relaxed); }
void draw(uint8_t* rdram, recomp_context* ctx, recomp_func_t* next) {
    const auto setting = active(rdram);
    if (setting == Mode::Original ||
        !rdram || !readable(uint32_t(ctx->r29), 0x18)) { next(rdram, ctx); return; }
    const bool world = !drawing_player && MEM_W(0, guest(0x8012C828)) != 0;
    Model frame{setting, ADD32(ctx->r29, -0xF8),
        read_float(rdram, guest(0x8012C82C), 0), world};
    Scope scope(model, frame);
    next(rdram, ctx);
}
void player_draw(uint8_t* rdram, recomp_context* ctx, recomp_func_t* next) {
    struct Restore { bool previous; ~Restore() { drawing_player = previous; } } restore{drawing_player};
    drawing_player = true;
    next(rdram, ctx);
}
void actor_range(uint8_t* r, recomp_context* c, recomp_func_t* next) { actor_gate(r, c, next, updating); }
void residency(uint8_t* r, recomp_context* c, recomp_func_t* next) { actor_gate(r, c, next, retaining); }
}

extern "C" void tooie_distance_view_scale(uint8_t* rdram, recomp_context* ctx) {
    const bool applies = model && !model->consumed && ctx->r29 == model->native_stack;
    if (applies) model->consumed = true;
    func_800E42C0(rdram, ctx);
    const float scale = ctx->f0.fl;
    if (active(rdram) == Mode::Original) return;
    // Only these exact invocations use the getter for spatial distance. Do not
    // modify the viewport/FOV or queries nested in frustum/animation callbacks.
    if (selection && ctx->r29 == selection->native_stack && std::isfinite(scale) && scale > 0) {
        ctx->f0.fl = selection->setting == Mode::Unlimited ? 32768.0f : scale * multiplier(selection->setting);
        return;
    }
    if (!applies || !model->world ||
        !std::isfinite(scale) || scale <= 0 || !std::isfinite(model->initial_limit) || model->initial_limit <= 0) return;
    const float original = model_limit(rdram, *model, scale);
    const float extended = model->setting == Mode::Unlimited ? 100000000.0f : original * multiplier(model->setting);
    write_float(rdram, guest(0x8012C82C), 0, extended / scale);
    // Keep native alpha multiplication (including scripted transparency), using
    // physical distance and its original 700-unit transition at the new limit.
    MEM_W(0, guest(0x8012C834)) = 1;
}

extern "C" void tooie_distance_collect(uint8_t* rdram, recomp_context* ctx) {
    const auto setting = active(rdram);
    if (setting == Mode::Original) { func_800D3104(rdram, ctx); return; }
    // Native bounds start at the map's grid extents and are then clipped around
    // the camera. Unlimited prevents that clipping; it does not invent cells.
    Selection frame{setting, ADD32(ctx->r29, -0xC0)};
    Scope scope(selection, frame);
    func_800D3104(rdram, ctx);
}
extern "C" void tooie_distance_cell(uint8_t* rdram, recomp_context* ctx) {
    if (active(rdram) == Mode::Original || !selection || ctx->r29 != selection->native_stack) {
        func_800D2F44(rdram, ctx);
        return;
    }
    Selection frame{selection->setting, ADD32(ctx->r29, -0x58)};
    Scope scope(selection, frame);
    func_800D2F44(rdram, ctx); // Native AABB/frustum and cell flags still run.
}
extern "C" void tooie_distance_sprite(uint8_t* rdram, recomp_context* ctx) {
    const auto setting = active(rdram);
    const int view = MEM_W(0, guest(0x80127674));
    const int limit = MEM_W(0, guest(0x8012D494));
    if (setting == Mode::Original || drawing_player || view < 0 || view >= 4 || limit <= 0) {
        func_800E1C00(rdram, ctx);
        return;
    }
    // This function projects a world position; global UI viewport 4 and player
    // draw scopes retain native limits. Restore only input policy, not native
    // prepared position/alpha used by the following sprite geometry emission.
    struct Restore {
        uint8_t* rdram; int limit; uint8_t size_test, fade;
        ~Restore() {
            MEM_W(0, guest(0x8012D494)) = limit;
            MEM_B(0, guest(0x8012D491)) = size_test;
            MEM_B(0, guest(0x8012D492)) = fade;
        }
    } restore{rdram, limit, MEM_BU(0, guest(0x8012D491)), MEM_BU(0, guest(0x8012D492))};
    const int extended = setting == Mode::Unlimited ? 100000000 :
        int(std::min(double(limit) * multiplier(setting), double(std::numeric_limits<int>::max() / 16)));
    MEM_W(0, guest(0x8012D494)) = extended;
    // Avoid the default-limit-only projected-size shortcut reintroducing a
    // second abrupt cutoff (including a multiplied limit equal to its sentinel).
    MEM_B(0, guest(0x8012D491)) = 1;
    MEM_B(0, guest(0x8012D492)) = 1;
    func_800E1C00(rdram, ctx); // Physical view-depth and native 500-unit fade.
}
extern "C" void tooie_distance_radius(uint8_t* rdram, recomp_context* ctx) {
    float radius;
    const auto bits = uint32_t(ctx->r5); std::memcpy(&radius, &bits, 4);
    const bool applies = updating && !updating->consumed && ctx->r29 == updating->math_stack &&
        uint32_t(ctx->r4) == updating->address + 4 &&
        std::isfinite(radius) && radius > 0;
    if (applies && active(rdram) != Mode::Original && tooie_memory::distance_heap_available(rdram)) {
        updating->consumed = true;
        const float extended = updating->setting == Mode::Unlimited ? 100000000.0f : radius * multiplier(updating->setting);
        uint32_t extended_bits; std::memcpy(&extended_bits, &extended, 4);
        ctx->r5 = guest(extended_bits);
    }
    func_800EFFB4(rdram, ctx);
}
extern "C" void tooie_distance_squared(uint8_t* rdram, recomp_context* ctx) {
    const bool applies = retaining && !retaining->consumed && ctx->r29 == retaining->math_stack &&
        uint32_t(ctx->r5) == retaining->address + 4;
    if (applies) retaining->consumed = true;
    func_800EEB40(rdram, ctx);
    const float original = ctx->f0.fl;
    if (!applies || active(rdram) == Mode::Original ||
        !tooie_memory::distance_heap_available(rdram) || !std::isfinite(original) || original < 0) return;
    const float factor = multiplier(retaining->setting);
    ctx->f0.fl = retaining->setting == Mode::Unlimited ? 0.0f : original / (factor * factor);
}
