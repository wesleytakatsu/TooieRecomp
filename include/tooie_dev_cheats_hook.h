#pragma once
#include "recomp.h"

namespace tooie_dev_cheats {
// Game-thread execution, called inside the owner's PlayerScope by the per-player update hook.
void update_player(uint8_t* rdram, const recomp_context* ctx, uint32_t self, bool gameplay, bool blocked);
void reset_gameplay(); // new map/session: discard requests and per-player Blaster cleanup
}
