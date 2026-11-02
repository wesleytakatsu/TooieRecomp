#pragma once

#include "recomp.h"

namespace tooie_dev_collectibles {
inline constexpr uint32_t kActorEventVram = 0x8010108Cu;
}

// Complete a collected Glowbo's native cleanup in experimental story splitscreen.
extern "C" void tooie_dev_on_actor_event(uint8_t* rdram, recomp_context* ctx);
