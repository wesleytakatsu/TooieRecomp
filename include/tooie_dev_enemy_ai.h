#pragma once

#include "recomp.h"

namespace tooie_dev_enemy_ai {
// Reinstall only the audited enemy callbacks after either physical or canonical mapping.
void loaded_overlay(uint32_t rom, int32_t ram, uint32_t size);
void reset();
// Called after normal syscall overlay activation, preserving its load/unload lifecycle.
void dispatch(uint8_t* rdram, recomp_context* ctx, recomp_func_t* native);
}

extern "C" void tooie_dev_enemy_focus(uint8_t* rdram, recomp_context* ctx);
extern "C" void tooie_dev_enemy_range(uint8_t* rdram, recomp_context* ctx);
