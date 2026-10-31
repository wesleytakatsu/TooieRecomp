#include "tooie_dev_cheats.h"

#include <atomic>
#include <fstream>
#include <mutex>
#include <sstream>
#include <system_error>

#include <json/json.hpp>

namespace tooie_dev_cheats {

namespace {

constexpr uint16_t kDpadMask = 0x0F00; // D-pad up/down/left/right

std::mutex settings_mutex;
Settings current_settings;
std::atomic<bool> unlocked{false};
std::array<std::atomic<bool>, kCheatCount> requests{};
Chord player1_chord; // only touched by the input callback

// Banjo and Kazooie standing or walking: the states seen before a flight pad takeoff and before the game switched the
// Breegull Blaster on.
bool on_ground(uint32_t transform, uint32_t state) {
    return (transform == kTransformBanjoKazooie) && ((state == kStateIdle) || (state == 0x2) || (state == 0x4));
}

} // namespace

uint16_t direction_button(Direction direction) {
    switch (direction) {
    case Direction::Up:
        return 0x0800;
    case Direction::Down:
        return 0x0400;
    case Direction::Left:
        return 0x0200;
    case Direction::Right:
        return 0x0100;
    }

    return 0;
}

const char* direction_name(Direction direction) {
    switch (direction) {
    case Direction::Up:
        return "up";
    case Direction::Down:
        return "down";
    case Direction::Left:
        return "left";
    case Direction::Right:
        return "right";
    }

    return "up";
}

std::optional<Direction> direction_from_name(std::string_view name) {
    for (Direction direction : {Direction::Up, Direction::Down, Direction::Left, Direction::Right}) {
        if (name == direction_name(direction)) {
            return direction;
        }
    }

    return std::nullopt;
}

const char* cheat_key(Cheat cheat) {
    switch (cheat) {
    case Cheat::Fly:
        return "fly";
    case Cheat::Blaster:
        return "blaster";
    case Cheat::Count:
        break;
    }

    return "";
}

std::optional<uint32_t> fly_state(uint32_t transform, uint32_t state) {
    if (on_ground(transform, state)) {
        return kStateFlyLaunch;
    }

    return std::nullopt;
}

std::optional<uint32_t> blaster_state(uint32_t transform, uint32_t state) {
    if (on_ground(transform, state)) {
        return kStateBlasterEnter;
    }

    // The Blaster's own idle and movement states, seen between the game's enter and exit requests.
    const bool in_blaster_move = (state == 0xCA) || (state == 0xCC) || (state == 0xCE) || (state == 0xD1);
    if ((transform == kTransformBlaster) && in_blaster_move) {
        return kStateBlasterExit;
    }

    return std::nullopt;
}

uint16_t Chord::step(bool debug_held, uint16_t buttons) {
    const uint16_t dpad = buttons & kDpadMask;
    const uint16_t pressed = dpad & ~previous_dpad;
    previous_dpad = dpad;
    return debug_held ? pressed : 0;
}

Settings settings() {
    std::lock_guard lock(settings_mutex);
    return current_settings;
}

void set_settings(const Settings& settings) {
    std::lock_guard lock(settings_mutex);
    current_settings = settings;
}

void set_unlocked(bool value) {
    unlocked = value;
}

void on_player1_input(bool debug_held, uint16_t buttons) {
    const uint16_t pressed = player1_chord.step(debug_held, buttons);
    if ((pressed == 0) || !unlocked) {
        return;
    }

    const Settings current = settings();
    for (size_t i = 0; i < kCheatCount; i++) {
        const CheatSettings& cheat = current.cheats[i];
        if (cheat.enabled && ((pressed & direction_button(cheat.direction)) != 0)) {
            requests[i] = true;
        }
    }
}

bool take_request(Cheat cheat) {
    return requests[size_t(cheat)].exchange(false);
}

Settings parse_settings(const std::string& json_text) {
    Settings result;
    const nlohmann::json root = nlohmann::json::parse(json_text, nullptr, false);
    if (!root.is_object()) {
        return result;
    }

    for (size_t i = 0; i < kCheatCount; i++) {
        const char* key = cheat_key(Cheat(i));
        if (!root.contains(key) || !root[key].is_object()) {
            continue;
        }

        const nlohmann::json& item = root[key];
        CheatSettings& cheat = result.cheats[i];
        if (item.contains("enabled") && item["enabled"].is_boolean()) {
            cheat.enabled = item["enabled"].get<bool>();
        }

        if (item.contains("direction") && item["direction"].is_string()) {
            cheat.direction = direction_from_name(item["direction"].get<std::string>()).value_or(cheat.direction);
        }
    }

    return result;
}

std::string serialize_settings(const Settings& settings) {
    nlohmann::json root = {{"version", 1}};
    for (size_t i = 0; i < kCheatCount; i++) {
        const CheatSettings& cheat = settings.cheats[i];
        root[cheat_key(Cheat(i))] = {{"enabled", cheat.enabled}, {"direction", direction_name(cheat.direction)}};
    }

    return root.dump(2) + "\n";
}

Settings load_settings(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }

    std::stringstream text;
    text << file.rdbuf();
    return parse_settings(text.str());
}

bool save_settings(const std::filesystem::path& path, const Settings& settings) {
    // Write a temporary file and rename it, so a crash never leaves a half-written file.
    std::filesystem::path temporary = path;
    temporary += ".tmp";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        if (!file) {
            return false;
        }

        file << serialize_settings(settings);
        if (!file) {
            return false;
        }
    }

    std::error_code error;
    std::filesystem::rename(temporary, path, error);
    return !error;
}

} // namespace tooie_dev_cheats
