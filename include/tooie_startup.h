#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace recomp::config {
class Config;
}
namespace tooie_startup {
std::vector<std::string> candidates(const std::string &preference);
std::string preference();
bool save_preference(const std::string &value);
// Returns -1 in the renderer worker (or on platforms without supervision).
int enter(int argc, char **argv);
void launcher_ready();
// Called once per launcher frame. Reports readiness at once when unsupervised; a Windows worker first renders the
// hidden launcher for a warm-up period, so an API whose device is lost on its first frames fails before it is shown.
void launcher_frame();
inline constexpr char graphics_backend_option[] = "graphics_backend";
void add_graphics_option(recomp::config::Config &config);
bool is_ready();
bool is_worker();
void note_game_start();
} // namespace tooie_startup
