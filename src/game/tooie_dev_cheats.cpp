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
std::atomic<bool> fly_requested{false};
Chord player1_chord; // only touched by the input callback

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

bool can_launch_flight(uint32_t state) {
    // Idle and the two ground movement states seen around the flight pad trace. Other states (swimming, other forms,
    // cutscenes) are left alone until each one is tried.
    return (state == kStateIdle) || (state == 0x2) || (state == 0x4);
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
    if (current.fly_enabled && ((pressed & direction_button(current.fly_direction)) != 0)) {
        fly_requested = true;
    }
}

bool take_fly_request() {
    return fly_requested.exchange(false);
}

Settings parse_settings(const std::string& json_text) {
    Settings result;
    const nlohmann::json root = nlohmann::json::parse(json_text, nullptr, false);
    if (!root.is_object() || !root.contains("fly") || !root["fly"].is_object()) {
        return result;
    }

    const nlohmann::json& fly = root["fly"];
    if (fly.contains("enabled") && fly["enabled"].is_boolean()) {
        result.fly_enabled = fly["enabled"].get<bool>();
    }

    if (fly.contains("direction") && fly["direction"].is_string()) {
        result.fly_direction = direction_from_name(fly["direction"].get<std::string>()).value_or(Direction::Up);
    }

    return result;
}

std::string serialize_settings(const Settings& settings) {
    const nlohmann::json root = {
        {"version", 1},
        {"fly", {{"enabled", settings.fly_enabled}, {"direction", direction_name(settings.fly_direction)}}},
    };
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
