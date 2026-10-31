#pragma once

// Dev Mode cheats: shortcuts made of the Debug Button (a recompinput input, R3 by default) held with a D-pad direction.
//
// The input callback (src/main/main.cpp) feeds player 1's buttons to on_player1_input on the game thread; a chord of an
// enabled cheat queues a request, and the yaw_update hook (src/game/tooie_dev_warp_hook.cpp) carries it out on player 1's
// next frame. The Dev Mode tab (src/game/tooie_dev_mode_ui.cpp) edits the settings. This file's functions do not touch
// guest memory. See docs/specs/Tooie/window-ui-and-config.md (Dev Mode).

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace tooie_dev_cheats {

enum class Direction { Up, Down, Left, Right };

enum class Cheat {
    Fly,     // take off as from a flight pad
    Blaster, // switch the Breegull Blaster on or off
    Count
};

constexpr size_t kCheatCount = size_t(Cheat::Count);

struct CheatSettings {
    bool enabled = false;
    Direction direction = Direction::Up;
};

struct Settings {
    std::array<CheatSettings, kCheatCount> cheats = {{
        {false, Direction::Up},    // Fly
        {false, Direction::Right}, // Blaster
    }};

    CheatSettings& operator[](Cheat cheat) { return cheats[size_t(cheat)]; }
    const CheatSettings& operator[](Cheat cheat) const { return cheats[size_t(cheat)]; }
};

// Player 1's transformation (PlayerState+0xA0 -> +0x29) and the player states the cheats use. See
// docs/reverse-engineering/systems/warp-and-player-position.md.
constexpr uint32_t kTransformBanjoKazooie = 0x1;
constexpr uint32_t kTransformBlaster = 0x9;
constexpr uint32_t kStateIdle = 0x1;
constexpr uint32_t kStateFlyLaunch = 0x23;
constexpr uint32_t kStateFlying = 0x24;
constexpr uint32_t kStateBlasterEnter = 0xC8;
constexpr uint32_t kStateBlasterExit = 0xC9;

// The N64 D-pad button bit of a direction (the same values recompinput sends to the game).
uint16_t direction_button(Direction direction);
const char* direction_name(Direction direction); // "up", "down", "left", "right"
std::optional<Direction> direction_from_name(std::string_view name);
const char* cheat_key(Cheat cheat); // "fly", "blaster": the cheat's key in dev_cheats.json

// The state to request for a cheat, or nullopt when it must not run in the given transformation and state. Fly and
// entering the Breegull Blaster need Banjo and Kazooie standing or walking on the ground; leaving the Blaster needs one of
// its own idle or movement states.
std::optional<uint32_t> fly_state(uint32_t transform, uint32_t state);
std::optional<uint32_t> blaster_state(uint32_t transform, uint32_t state);

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
bool take_request(Cheat cheat); // true once per queued request of that cheat

// Settings file: <app folder>/dev_cheats.json.
Settings parse_settings(const std::string& json_text);
std::string serialize_settings(const Settings& settings);
Settings load_settings(const std::filesystem::path& path);
bool save_settings(const std::filesystem::path& path, const Settings& settings);

} // namespace tooie_dev_cheats
