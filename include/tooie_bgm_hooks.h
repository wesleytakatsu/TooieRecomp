#pragma once

#include "recomp.h"

extern "C" void tooie_bgm_on_csp_set_vol(uint8_t* rdram, recomp_context* ctx);
extern "C" void tooie_bgm_on_update(uint8_t* rdram, recomp_context* ctx);
extern "C" void tooie_bgm_on_sequence_start(uint8_t* rdram, recomp_context* ctx);

namespace tooie_bgm {
// Installed once at startup. Optional external-audio integration; standalone
// volume tests and builds without a content provider retain the original path.
using GainFilter = int16_t (*)(uint8_t*, unsigned, int16_t, int16_t);
using FrameObserver = void (*)(uint8_t*, recomp_context*);
using SequenceObserver = void (*)(uint8_t*, unsigned, int);
using GainObserver = void (*)(uint8_t*, unsigned, int16_t);
void set_content_hooks(GainFilter gain, FrameObserver frame, SequenceObserver sequence, GainObserver posted = nullptr);
}
