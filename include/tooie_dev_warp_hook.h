#pragma once

#include <cstdint>

#include "recomp.h"

namespace tooie_dev_warp {
constexpr uint32_t kYawUpdateVram = 0x8009C88Cu;    // yaw_update(PlayerState*), once per frame for each player
constexpr uint32_t kPlayerUpdateVram = 0x800991B0u; // per-player update, every frame in every form
constexpr uint32_t kCreatePlayersVram = 0x800F73C4u; // creates the players of a loaded map (count in a0)
constexpr uint32_t kPlayerActiveVram = 0x800F65D0u;  // is player index active (collision gate)
constexpr uint32_t kPrimaryPlayerModelVram = 0x80092EC8u;
constexpr uint32_t kLodPlayerModelVram = 0x800941C8u;
constexpr uint32_t kSecondaryPlayerModelVram = 0x8009359Cu;
constexpr uint32_t kModelInstanceVram = 0x80100D24u; // allocates an instance and optional private output vertices
}

// Hook for kYawUpdateVram: runs yaw_update, then the Dev Mode warp for player 1 (src/game/tooie_dev_warp_hook.cpp).
extern "C" void tooie_dev_on_yaw_update(uint8_t* rdram, recomp_context* ctx);
// Hook for kPlayerUpdateVram: runs the player update, then the Dev Mode cheats queued for player 1.
extern "C" void tooie_dev_on_player_update(uint8_t* rdram, recomp_context* ctx);
// Hook for kCreatePlayersVram: asks for two players when the experimental option is on.
extern "C" void tooie_dev_on_create_players(uint8_t* rdram, recomp_context* ctx);
// Hook for kPlayerActiveVram: with two players, player 2 is active too.
extern "C" void tooie_dev_on_player_active(uint8_t* rdram, recomp_context* ctx);
// Model setup scopes isolate CPU-lit output vertices for the experimental story players.
extern "C" void tooie_dev_on_primary_player_model(uint8_t* rdram, recomp_context* ctx);
extern "C" void tooie_dev_on_lod_player_model(uint8_t* rdram, recomp_context* ctx);
extern "C" void tooie_dev_on_secondary_player_model(uint8_t* rdram, recomp_context* ctx);
extern "C" void tooie_dev_on_model_instance(uint8_t* rdram, recomp_context* ctx);

extern "C" void tooie_dev_on_normal_player_allocate(uint8_t*, recomp_context*);
