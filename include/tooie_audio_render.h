#pragma once

#include <cstddef>
#include <cstdint>

#include "recomp.h"

// Maintainer tool for building music packs: TOOIE_TEST_AUDIO_RENDER=<plan.tsv> plays each planned sequence (and channel
// mask) on the game's own sequence player and records the native mix as 48 kHz stereo WAV files in
// TOOIE_TEST_AUDIO_RENDER_OUT (default: the plan's folder). Other sequence slots are muted while it runs and the output
// device receives silence. Off unless the variable is set; never part of normal play.
//
// Writes nothing else: no log. Plan lines: <name>\t<sequence hex>\t<channel mask hex or ->\t<seconds>[\t<bpm>]; '#' starts a comment line.
// The bpm (an area preset's tempo) is applied at once with 80017D74, as the game does when it enters the area.
namespace tooie_audio_render {
bool enabled();
// TOOIE_TEST_AUDIO_RENDER_MOD=1: enabled packs keep replacing music and the recording is the final mix (MP3s included),
// to check a pack against the native renders.
bool keeps_mods();
// Guest thread, once per game frame (after the original track update).
void on_frame(uint8_t* rdram, recomp_context* ctx);
// True when the slot's music must be silent because the renderer owns the output.
bool mutes(unsigned slot);
// Audio thread: the native mix converted to 48 kHz stereo float, before any mod is mixed in. Returns true when the
// buffer must be replaced by silence on the output device.
bool capture(float* stereo, size_t frames);
}
