#pragma once

#include <cstdint>

#include "recomp.h"

namespace tooie_dev_warp {
constexpr uint32_t kYawUpdateVram = 0x8009C88Cu;    // yaw_update(PlayerState*), once per frame for each player
constexpr uint32_t kPlayerUpdateVram = 0x800991B0u; // per-player update, every frame in every form
}

// Hook for kYawUpdateVram: runs yaw_update, then the Dev Mode warp for player 1 (src/game/tooie_dev_warp_hook.cpp).
extern "C" void tooie_dev_on_yaw_update(uint8_t* rdram, recomp_context* ctx);
// Hook for kPlayerUpdateVram: runs the player update, then the Dev Mode cheats queued for player 1.
extern "C" void tooie_dev_on_player_update(uint8_t* rdram, recomp_context* ctx);
