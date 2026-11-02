#pragma once
// Forced into the audio TEST target only; production source has no measurement hooks.
#define SDL_MAIN_HANDLED
#include "SDL.h"
#include <atomic>
#include <cstdint>
namespace launcher_audio_test {
inline std::atomic<uint64_t> queued_frames{0};
inline int queue_audio(SDL_AudioDeviceID device, const void *data, Uint32 length) {
    const int result = SDL_QueueAudio(device, data, length);
    if (result == 0)
        queued_frames.fetch_add(length / (2 * sizeof(float)));
    return result;
}
} // namespace launcher_audio_test
#define SDL_QueueAudio(...) launcher_audio_test::queue_audio(__VA_ARGS__)
