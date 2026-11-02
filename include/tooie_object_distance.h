#pragma once
#include <cstdint>
#include "recomp.h"

namespace recomp::config { class Config; }
namespace tooie_object_distance {
enum class Mode : uint32_t { Original, Double, Quadruple, Unlimited };
inline constexpr char option[] = "object_distance";
void set_mode(uint32_t value);
Mode mode();
void add_config(recomp::config::Config& config);

// Compose with existing wrappers; each scope affects only the native distance
// operation belonging to this invocation. Frustum, actor flags and callbacks remain native.
void draw(uint8_t* rdram, recomp_context* ctx, recomp_func_t* next);
void player_draw(uint8_t* rdram, recomp_context* ctx, recomp_func_t* next);
void actor_range(uint8_t* rdram, recomp_context* ctx, recomp_func_t* next);
void residency(uint8_t* rdram, recomp_context* ctx, recomp_func_t* next);
}
extern "C" {
void tooie_distance_view_scale(uint8_t*, recomp_context*);
void tooie_distance_radius(uint8_t*, recomp_context*);
void tooie_distance_squared(uint8_t*, recomp_context*);
void tooie_distance_collect(uint8_t*, recomp_context*);
void tooie_distance_cell(uint8_t*, recomp_context*);
void tooie_distance_sprite(uint8_t*, recomp_context*);
}
