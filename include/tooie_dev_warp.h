#pragma once

// Dev Mode warp: saved player positions and teleporting to them.
//
// The game thread publishes the player's position every frame and runs the teleport state machine from a hook on the
// player's yaw update (src/game/tooie_dev_warp_hook.cpp). The UI thread reads the published position and queues
// teleports (src/game/tooie_dev_mode_ui.cpp). This file's functions do not touch guest memory.
// See docs/specs/Tooie/window-ui-and-config.md (Dev Mode) and docs/reverse-engineering/systems/warp-and-player-position.md.

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace tooie_dev_warp {

// Where player 1 stands: the map, the entrance it was entered through, the position and the facing (degrees).
struct Position {
    int16_t map = 0;
    int16_t entrance = 0;
    float pos[3] = {0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;
};

struct Entry {
    std::string name;
    Position position;
};

// What the hook reads from the game on each frame of player 1.
struct GameState {
    bool gameplay = false;     // game mode 3 (save file) or 5 (attract demo)
    bool warp_pending = false; // a map change was requested and the game has not started it yet
    uint32_t player = 0;       // PlayerState address of player 1, 0 when there is none
    Position position;
};

// What the hook must do on this frame.
enum class Action {
    None,
    Goto,  // ask the game to load target().map through target().entrance
    Apply, // move player 1 to target().pos and turn it to target().yaw
    GiveUp // the teleport timed out
};

// Teleport state machine, driven once per frame of player 1. A teleport to the current map applies at once; to another
// map it asks the game for the map and applies kSettleSeconds after the game has started the change and the new map
// has loaded.
class Teleporter {
public:
    static constexpr double kTimeoutSeconds = 30.0;
    static constexpr double kLoadGapSeconds = 0.15; // no player frame for this long means the game was loading
    static constexpr double kSettleSeconds = 2.0;   // wait in the loaded map (fade-in, entry walk) before applying

    void request(const Position& target, double now);
    Action step(const GameState& state, double now);
    bool busy() const { return phase != Phase::Idle; }
    const Position& target() const { return target_position; }

private:
    enum class Phase { Idle, Requested, Loading };
    Phase phase = Phase::Idle;
    Position target_position;
    double requested_at = 0.0;
    double last_frame = 0.0;
    uint32_t old_player = 0;
    bool load_started = false; // the game took the map change request (warp_pending cleared)
    bool saw_load = false;
    bool arrived = false;      // the player is in the loaded target map
    double arrived_at = 0.0;
};

// Shared between the game thread and the UI thread.
// Called by the hook on each frame of player 1: publishes the position and steps the teleporter.
Action game_frame(const GameState& state, double now, Position& target);
// The position published by the last gameplay frame, if it is recent (the player is in a level and the game runs).
std::optional<Position> current_position();
void request_teleport(const Position& target);
bool teleport_busy();

// Readable map name from the decompilation's map list, or nullptr.
const char* map_name(int16_t map);
// "GGM Glitter Gulch Mine (199)", or "Map 199" for an unknown map.
std::string describe_map(int16_t map);

// Saved positions: <app folder>/dev_warps.json.
std::vector<Entry> parse_entries(const std::string& json_text);
std::string serialize_entries(const std::vector<Entry>& entries);
std::vector<Entry> load_entries(const std::filesystem::path& path);
bool save_entries(const std::filesystem::path& path, const std::vector<Entry>& entries);

} // namespace tooie_dev_warp
