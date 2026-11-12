// Graphics > Distant Detail: each wrapper changes only the distance call its native function makes directly.
#include "tooie_distant_detail.h"

#include <cstdio>
#include <cstdlib>

namespace {
int failures = 0;
void check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

float native_distance = 0;
float seen_distance = -1;
bool nested_call = false;
float nested_distance = -1;

// Native distance functions: return native_distance in f0.
void native_squared(uint8_t*, recomp_context* ctx) { ctx->f0.fl = native_distance; }
void native_length(uint8_t*, recomp_context* ctx) { ctx->f0.fl = native_distance; }

void squared_call(uint8_t* rdram, recomp_context* ctx) {
    tooie_distant_detail::lod_distance_squared(rdram, ctx, native_squared);
}
void length_call(uint8_t* rdram, recomp_context* ctx) {
    tooie_distant_detail::player_distance(rdram, ctx, native_length);
}

// A LOD node: opens its 0x18 frame, asks d^2, then (optionally) walks a child that asks again from a deeper frame.
void lod_node(uint8_t* rdram, recomp_context* ctx) {
    ctx->r29 = ADD32(ctx->r29, -0x18);
    squared_call(rdram, ctx);
    seen_distance = ctx->f0.fl;
    if (nested_call) {
        ctx->r29 = ADD32(ctx->r29, -0x28);
        squared_call(rdram, ctx);
        nested_distance = ctx->f0.fl;
        ctx->r29 = ADD32(ctx->r29, 0x28);
    }
    ctx->r29 = ADD32(ctx->r29, 0x18);
}

// The player model level: 0x40 frame, one distance call.
void player_level(uint8_t* rdram, recomp_context* ctx) {
    ctx->r29 = ADD32(ctx->r29, -0x40);
    length_call(rdram, ctx);
    seen_distance = ctx->f0.fl;
    ctx->r29 = ADD32(ctx->r29, 0x40);
}

float run_lod(bool enabled, float distance, bool nested = false) {
    tooie_distant_detail::set_enabled(enabled);
    native_distance = distance;
    nested_call = nested;
    recomp_context ctx{};
    ctx.r29 = static_cast<gpr>(static_cast<int32_t>(0x80300000));
    tooie_distant_detail::lod_node(nullptr, &ctx, lod_node);
    return seen_distance;
}

float run_player(bool enabled, float distance) {
    tooie_distant_detail::set_enabled(enabled);
    native_distance = distance;
    recomp_context ctx{};
    ctx.r29 = static_cast<gpr>(static_cast<int32_t>(0x80300000));
    tooie_distant_detail::player_model_level(nullptr, &ctx, player_level);
    return seen_distance;
}
}

int main() {
    using tooie_distant_detail::near_distance_squared;
    check(run_lod(false, 5.0e6f) == 5.0e6f, "off: LOD node keeps the native distance");
    check(run_lod(true, 5.0e6f) == near_distance_squared, "on: LOD node gets the near distance");
    check(run_lod(true, 1.0e-6f) == 1.0e-6f, "on: a nearer native distance is kept");
    check(near_distance_squared > 0.0f, "near distance stays above a min of 0");

    run_lod(true, 5.0e6f, true);
    check(nested_distance == 5.0e6f, "on: a nested call from the child walk keeps the native distance");

    // Outside any wrapped node, nothing changes.
    tooie_distant_detail::set_enabled(true);
    native_distance = 7.0f;
    recomp_context ctx{};
    ctx.r29 = static_cast<gpr>(static_cast<int32_t>(0x80300000));
    squared_call(nullptr, &ctx);
    check(ctx.f0.fl == 7.0f, "on: an unrelated d^2 call keeps the native distance");
    length_call(nullptr, &ctx);
    check(ctx.f0.fl == 7.0f, "on: an unrelated distance call keeps the native distance");

    check(run_player(false, 3000.0f) == 3000.0f, "off: player level keeps the native distance");
    check(run_player(true, 3000.0f) == 0.0f, "on: player level gets distance 0");

    if (failures == 0) {
        std::printf("tooie_distant_detail_test: PASS\n");
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
