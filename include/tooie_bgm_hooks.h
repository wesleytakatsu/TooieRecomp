#pragma once

#include "recomp.h"

extern "C" void tooie_bgm_on_csp_set_vol(uint8_t* rdram, recomp_context* ctx);
extern "C" void tooie_bgm_on_update(uint8_t* rdram, recomp_context* ctx);
