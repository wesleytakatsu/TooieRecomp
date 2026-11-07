#pragma once
#include <filesystem>
#include <string>
#include <utility>
#include <vector>
namespace recomp::config {
class Config;
}
namespace tooie_startup {
std::vector<std::string> candidates(const std::string &preference);
// Windows supervisor order of (API, GPU) attempts: every API on the chosen or high-performance GPU first, then the other
// GPUs. The GPU is empty when none was found, and always for OpenGL, whose GPU the driver picks.
std::vector<std::pair<std::string, std::string>> attempts(const std::string &preference);
std::string preference();
bool save_preference(const std::string &value);
// Returns -1 in the renderer worker (or on platforms without supervision).
int enter(int argc, char **argv);
void launcher_ready();
// Called once per launcher frame. Reports readiness at once when unsupervised; a Windows worker first renders the
// hidden launcher for a warm-up period, so an API whose device is lost on its first frames fails before it is shown.
void launcher_frame();
inline constexpr char graphics_backend_option[] = "graphics_backend";
// "auto" or a GPU name; listed in the Graphics tab on Windows when there are at least two GPUs.
inline constexpr char graphics_device_option[] = "graphics_device";
void add_graphics_option(recomp::config::Config &config);
bool is_ready();
bool is_worker();
void note_game_start();
} // namespace tooie_startup
