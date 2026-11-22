#pragma once

#include <cstdint>
#include "recomp.h"

namespace recomp::config { class Config; }

// Graphics > "Distant Detail": keeps the high-detail level of models far from the camera. Off by default (native
// behavior). Two native distance choices are affected, nothing else:
// - GeoCmd LOD nodes (0x800DD214 / 0x800DD2E8 / 0x800DD37C): a child is drawn when min^2 < d^2 <= max^2, with d^2
//   from func_800EEB40 at the node's test point. With the option on, that one call returns a tiny d^2, so the
//   near child (min 0) is drawn and the far children are not. The native forced-index mode of 0x800DD214 is kept.
// - Player model swap (0x80094070): the camera distance from func_800EEAD4 picks the player's model level from the
//   table at 0x80117DD0. With the option on, that one call returns 0, so the nearest level stays installed.
// Draw distance, culling, fade and actor updates are not changed (Object Distance owns those).
namespace tooie_distant_detail {
    inline constexpr char option[] = "distant_detail";
    inline constexpr uint32_t off = 0;
    inline constexpr uint32_t on = 1;

    void set_enabled(bool enabled);
    bool enabled();
    void add_config(recomp::config::Config& config);

    // Wrappers: each scope affects only the matching distance call made directly by that invocation.
    void lod_node(uint8_t* rdram, recomp_context* ctx, recomp_func_t* next);
    void lod_distance_squared(uint8_t* rdram, recomp_context* ctx, recomp_func_t* next);
    void player_model_level(uint8_t* rdram, recomp_context* ctx, recomp_func_t* next);
    void player_distance(uint8_t* rdram, recomp_context* ctx, recomp_func_t* next);

    // d^2 returned to a GeoCmd LOD node while the option is on: above 0 (so `min^2 < d^2` holds for min 0) and below
    // any real LOD boundary.
    inline constexpr float near_distance_squared = 1.0e-4f;
}
