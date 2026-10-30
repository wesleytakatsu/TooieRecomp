#include "tooie_dev_warp.h"

#include <chrono>
#include <fstream>
#include <mutex>
#include <sstream>
#include <system_error>

#include <json/json.hpp>

namespace tooie_dev_warp {

void Teleporter::request(const Position& target, double now) {
    target_position = target;
    phase = Phase::Requested;
    requested_at = now;
    last_frame = now;
    old_player = 0;
    saw_load = false;
    stable = 0;
}

Action Teleporter::step(const GameState& state, double now) {
    if (phase == Phase::Idle) {
        return Action::None;
    }

    if (now - requested_at > kTimeoutSeconds) {
        phase = Phase::Idle;
        return Action::GiveUp;
    }

    const bool ready = state.gameplay && (state.player != 0);
    switch (phase) {
    case Phase::Requested:
        if (!ready) {
            return Action::None;
        }

        if (state.position.map == target_position.map) {
            phase = Phase::Idle;
            return Action::Apply;
        }

        phase = Phase::Loading;
        old_player = state.player;
        last_frame = now;
        saw_load = false;
        stable = 0;
        return Action::Goto;
    case Phase::Loading:
        // The old map can keep running for a few frames after the request, so the map number alone does not say the
        // new map is ready: wait for a sign of loading (a gap without player frames, a new player, a non-gameplay mode).
        if ((now - last_frame > kLoadGapSeconds) || (state.player != old_player) || !state.gameplay) {
            saw_load = true;
        }

        last_frame = now;
        if (saw_load && ready && (state.position.map == target_position.map)) {
            if (++stable >= kStableFrames) {
                phase = Phase::Idle;
                return Action::Apply;
            }
        }
        else {
            stable = 0;
        }

        return Action::None;
    default:
        return Action::None;
    }
}

namespace {

// A published position older than this is stale: the game is paused, loading, in a cutscene or out of gameplay.
constexpr double kPositionFreshSeconds = 1.5;

std::mutex g_mutex;
std::optional<Position> g_current;
double g_current_at = 0.0;
Teleporter g_teleporter;

double now_seconds() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

} // namespace

Action game_frame(const GameState& state, double now, Position& target) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (state.gameplay && (state.player != 0)) {
        g_current = state.position;
        g_current_at = now;
    }

    const Action action = g_teleporter.step(state, now);
    target = g_teleporter.target();
    return action;
}

std::optional<Position> current_position() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (now_seconds() - g_current_at > kPositionFreshSeconds) {
        return std::nullopt;
    }

    return g_current;
}

void request_teleport(const Position& target) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_teleporter.request(target, now_seconds());
}

bool teleport_busy() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_teleporter.busy();
}

std::string describe_map(int16_t map) {
    const char* name = map_name(map);
    if (name == nullptr) {
        return "Map " + std::to_string(map);
    }

    return std::string(name) + " (" + std::to_string(map) + ")";
}

std::vector<Entry> parse_entries(const std::string& json_text) {
    std::vector<Entry> entries;
    const nlohmann::json root = nlohmann::json::parse(json_text, nullptr, false);
    if (!root.is_object() || !root.contains("positions") || !root["positions"].is_array()) {
        return entries;
    }

    for (const nlohmann::json& item : root["positions"]) {
        if (!item.is_object() || !item.contains("map") || !item["map"].is_number_integer()) {
            continue;
        }

        Entry entry;
        entry.name = item.value("name", std::string());
        entry.position.map = int16_t(item["map"].get<int>());
        entry.position.entrance = int16_t(item.value("entrance", 0));
        entry.position.pos[0] = item.value("x", 0.0f);
        entry.position.pos[1] = item.value("y", 0.0f);
        entry.position.pos[2] = item.value("z", 0.0f);
        entry.position.yaw = item.value("yaw", 0.0f);
        entries.push_back(std::move(entry));
    }

    return entries;
}

std::string serialize_entries(const std::vector<Entry>& entries) {
    nlohmann::json positions = nlohmann::json::array();
    for (const Entry& entry : entries) {
        positions.push_back({
            {"name", entry.name},
            {"map", entry.position.map},
            {"entrance", entry.position.entrance},
            {"x", entry.position.pos[0]},
            {"y", entry.position.pos[1]},
            {"z", entry.position.pos[2]},
            {"yaw", entry.position.yaw},
        });
    }

    const nlohmann::json root = {{"version", 1}, {"positions", positions}};
    return root.dump(2) + "\n";
}

std::vector<Entry> load_entries(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }

    std::stringstream text;
    text << file.rdbuf();
    return parse_entries(text.str());
}

bool save_entries(const std::filesystem::path& path, const std::vector<Entry>& entries) {
    // Write a temporary file and rename it, so a crash never leaves a half-written list.
    std::filesystem::path temporary = path;
    temporary += ".tmp";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        if (!file) {
            return false;
        }

        file << serialize_entries(entries);
        if (!file) {
            return false;
        }
    }

    std::error_code error;
    std::filesystem::rename(temporary, path, error);
    return !error;
}

} // namespace tooie_dev_warp
