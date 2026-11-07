#include "tooie_launcher_audio.h"
#include "../../../lib/dr_libs/dr_mp3.h"
#include "SDL.h"
#include "util/file.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <fstream>
#include <json/json.hpp>
#include <thread>
#include <vector>

namespace {
std::atomic<bool> stopping{false};
std::atomic<float> volume{1.0f};
std::thread player;
std::filesystem::path audio_path() { return recompui::file::get_app_folder_path() / "launcher_audio/pause.mp3"; }
void play() {
    const auto path = audio_path();
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return;
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)), {});
    drmp3 decoder{};
    if (!drmp3_init_memory_with_metadata(&decoder, bytes.data(), bytes.size(), nullptr, nullptr, nullptr))
        return;
    if (decoder.channels != 2) {
        drmp3_uninit(&decoder);
        return;
    }
    const uint64_t length = drmp3_get_pcm_frame_count(&decoder);
    uint64_t loop_start = 0, loop_end = length;
    try {
        std::ifstream f(path.parent_path() / "loop.json");
        auto j = nlohmann::json::parse(f);
        loop_start = j.at("start_frame").get<uint64_t>();
        loop_end = j.at("end_frame").get<uint64_t>();
    } catch (...) {
    }
    if (loop_start >= loop_end || loop_end > length) {
        drmp3_uninit(&decoder);
        return;
    }
    SDL_AudioSpec desired{};
    desired.freq = decoder.sampleRate;
    desired.format = AUDIO_F32SYS;
    desired.channels = 2;
    desired.samples = 1024;
    auto device = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, 0);
    if (!device) {
        drmp3_uninit(&decoder);
        return;
    }
    SDL_PauseAudioDevice(device, 0);
    std::array<float, 2048> samples{};
    uint64_t position = 0;
    float gain = 0.0f;
    auto stop_deadline = std::chrono::steady_clock::time_point::max();
    // Natural playback starts at frame zero; only the repeating region seeks to loop_start.
    while (true) {
        if (stopping.load() && stop_deadline == std::chrono::steady_clock::time_point::max())
            stop_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        if (std::chrono::steady_clock::now() >= stop_deadline)
            break;
        if (SDL_GetQueuedAudioSize(device) > decoder.sampleRate * 2 * sizeof(float) / 20) {
            SDL_Delay(10);
            continue;
        }
        if (position >= loop_end) {
            if (!drmp3_seek_to_pcm_frame(&decoder, loop_start))
                break;
            position = loop_start;
        }
        const auto count =
            drmp3_read_pcm_frames_f32(&decoder, std::min<uint64_t>(1024, loop_end - position), samples.data());
        if (!count)
            break;
        const bool ending = stopping.load();
        const float target = ending ? 0.0f : volume.load();
        for (uint64_t i = 0; i < count; i++) {
            // 120 ms linear ramp to silence on stop; 5 ms dezipper for volume changes.
            const float step = 1.0f / (decoder.sampleRate * (ending ? 0.12f : 0.005f));
            gain += std::clamp(target - gain, -step, step);
            samples[i * 2] *= gain;
            samples[i * 2 + 1] *= gain;
        }
        if (SDL_QueueAudio(device, samples.data(), static_cast<Uint32>(count * 2 * sizeof(float))) != 0)
            break;
        position += count;
        if (ending && gain == 0) {
            while (SDL_GetQueuedAudioSize(device) > 0 && std::chrono::steady_clock::now() < stop_deadline)
                SDL_Delay(10);
            break;
        }
    }
    SDL_ClearQueuedAudio(device);
    SDL_CloseAudioDevice(device);
    drmp3_uninit(&decoder);
}
} // namespace
void tooie_launcher_audio::update_volume(float master, float bgm) {
    volume.store(std::clamp(master * bgm, 0.0f, 1.0f));
}
void tooie_launcher_audio::start() {
    if (player.joinable())
        return;
    const char *disabled = std::getenv("TOOIE_DISABLE_LAUNCHER_MUSIC");
    if (disabled && *disabled && *disabled != '0')
        return;
    stopping.store(false);
    player = std::thread(play);
}
void tooie_launcher_audio::stop() {
    stopping.store(true);
    if (player.joinable())
        player.join();
}
