#pragma once

// Dev Mode cheats: shortcuts made of the Debug Button (a recompinput input, R3 by default) held with a D-pad direction.
//
// The input callback (src/main/main.cpp) feeds player 1's buttons to on_player1_input on the game thread; a chord of an
// enabled cheat queues a request, and the yaw_update hook (src/game/tooie_dev_warp_hook.cpp) carries it out on player 1's
// next frame. The Dev Mode tab (src/game/tooie_dev_mode_ui.cpp) edits the settings. This file's functions do not touch
// guest memory. See docs/specs/Tooie/window-ui-and-config.md (Dev Mode).

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace tooie_dev_cheats {

enum class Direction { Up, Down, Left, Right };

struct Settings {
    bool fly_enabled = false;
    Direction fly_direction = Direction::Up;
};

// Banjo states seen while taking off from a flight pad: idle -> launch -> flying.
constexpr uint32_t kStateIdle = 0x1;
constexpr uint32_t kStateFlyLaunch = 0x23;
constexpr uint32_t kStateFlying = 0x24;

// The N64 D-pad button bit of a direction (the same values recompinput sends to the game).
uint16_t direction_button(Direction direction);
const char* direction_name(Direction direction); // "up", "down", "left", "right"
std::optional<Direction> direction_from_name(std::string_view name);

// Whether forcing the launch state from this state is known to work: player 1 standing or walking on the ground.
bool can_launch_flight(uint32_t state);

// Reports the D-pad buttons pressed on this sample while the Debug Button is held, each once per press.
class Chord {
public:
    uint16_t step(bool debug_held, uint16_t buttons);

private:
    uint16_t previous_dpad = 0;
};

// Shared between the UI thread, the input callback and the game thread.
Settings settings();
void set_settings(const Settings& settings);
void set_unlocked(bool unlocked); // the Dev Mode warning was accepted; cheats do nothing before
void on_player1_input(bool debug_held, uint16_t buttons);
bool take_fly_request(); // true once per queued fly request

// Settings file: <app folder>/dev_cheats.json.
Settings parse_settings(const std::string& json_text);
std::string serialize_settings(const Settings& settings);
Settings load_settings(const std::filesystem::path& path);
bool save_settings(const std::filesystem::path& path, const Settings& settings);

} // namespace tooie_dev_cheats
