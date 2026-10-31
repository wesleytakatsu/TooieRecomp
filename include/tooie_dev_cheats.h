#pragma once

// Dev Mode cheats: shortcuts made of the Debug Button (a recompinput input, R3 by default) held with a D-pad direction.
//
// The input callback feeds each player's buttons to on_player_input; an enabled chord queues a request for its owner.
// The per-player update hook carries it out on that player's next frame. The Dev Mode tab edits shared settings.
// This file's functions do not touch
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
constexpr int kPlayerCount = 2;

struct CheatSettings {
    bool enabled = false;
    Direction direction = Direction::Up;
};

struct Settings {
    std::array<CheatSettings, kCheatCount> cheats = {{
        {false, Direction::Up},    // Fly
        {false, Direction::Right}, // Blaster
    }};

    // Experimental: story maps start with two players (split screen) instead of one, from the next map load.
    bool two_players = false;
    bool character_menu = true;

    CheatSettings& operator[](Cheat cheat) { return cheats[size_t(cheat)]; }
    const CheatSettings& operator[](Cheat cheat) const { return cheats[size_t(cheat)]; }
};

// The player's transformation (PlayerState+0xA0 -> +0x29) and the player states the cheats use. See
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
bool is_unlocked();
void on_player_input(int player, bool debug_held, uint16_t buttons);
bool take_request(int player, Cheat cheat); // consumes only this owner's request; rechecks unlock/enable
void reset_requests(); // discard pending actions; keep held-button history until release
bool two_players_active();      // unlocked and the two-player option is on
// Whether the two-player option applies to this map: not the title intro, the game select or the cutscene maps.
bool two_players_map(int map);

// Settings file: <app folder>/dev_cheats.json.
Settings parse_settings(const std::string& json_text);
std::string serialize_settings(const Settings& settings);
Settings load_settings(const std::filesystem::path& path);
bool save_settings(const std::filesystem::path& path, const Settings& settings);

} // namespace tooie_dev_cheats
