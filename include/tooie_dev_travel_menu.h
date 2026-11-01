#pragma once
#include "recomp.h"

namespace tooie_dev_travel_menu {
void reset();
}

extern "C" void tooie_dev_on_draw_queue(uint8_t* rdram, recomp_context* ctx);
