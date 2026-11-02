// Real decoder/SDL dummy queue; all observation lives in this test target.
#include "tooie_launcher_audio.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
static std::filesystem::path test_dir;
namespace recompui::file {
std::filesystem::path get_app_folder_path() { return test_dir; }
} // namespace recompui::file
int main(int argc, char **argv) {
    if (argc != 2)
        return 2;
    test_dir = std::filesystem::temp_directory_path() /
               ("tooie-audio-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(test_dir / "launcher_audio");
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
        return 3;
    const auto audio = test_dir / "launcher_audio/pause.mp3";
    {
        std::ofstream f(audio);
        f << "invalid";
    }
    tooie_launcher_audio::start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    tooie_launcher_audio::stop();
    if (launcher_audio_test::queued_frames.load() != 0)
        return 4;
    std::filesystem::copy_file(argv[1], audio, std::filesystem::copy_options::overwrite_existing);
    // Short valid region makes playback cross the loop boundary repeatedly.
    {
        std::ofstream f(test_dir / "launcher_audio/loop.json");
        f << R"({"start_frame":4800,"end_frame":14400})";
    }
    tooie_launcher_audio::start();
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    if (launcher_audio_test::queued_frames.load() < 48000) {
        tooie_launcher_audio::stop();
        return 5;
    }
    tooie_launcher_audio::update_volume(0, 1);
    const auto start = std::chrono::steady_clock::now();
    tooie_launcher_audio::stop();
    if (std::chrono::steady_clock::now() - start > std::chrono::seconds(3))
        return 6;
    const auto before = launcher_audio_test::queued_frames.load();
    tooie_launcher_audio::start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    tooie_launcher_audio::stop();
    if (launcher_audio_test::queued_frames.load() <= before)
        return 7;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    std::filesystem::remove_all(test_dir);
    std::cout << "PASS: real MP3/SDL dummy loop, invalid input, bounded stop and restart; no production counters\n";
    return 0;
}
