#pragma once
#include <array>
#include <cstdint>
#include <mutex>

namespace tooie_story_coop {
// Replay flags are catalog identities, never runtime activity predicates.
struct Stage { uint16_t flag, room; };
inline constexpr std::array<Stage, 23> stages{{
    {0xBE3,0xC6}, {0xBE4,0xC8}, {0xBE5,0xC9}, {0xBE6,0xDC},
    {0xBE7,0xDE}, {0xBE8,0xDF}, {0xBE9,0xE0}, {0xBEA,0xE5},
    {0xBEB,0xE4}, {0xBEC,0x124}, {0xBED,0x181}, {0xBEE,0x123},
    {0xBEF,0x162}, {0xBF0,0x10A}, {0xBF1,0x12E}, {0xBF2,0x12F},
    {0xBF3,0x130}, {0xBF4,0x13D}, {0xBF5,0x137}, {0xBF6,0x139},
    {0xBF7,0x15F}, {0xBF8,0x15F}, {0xBF9,0x15F}
}};
constexpr bool challenge_room(unsigned map) noexcept {
    for (auto stage : stages) if (stage.room == map) return true;
    // Saucer segments and conservative Mayan aliases (reachability unknown).
    switch (map) {
    case 0x164: case 0x165: case 0x17B: case 0x17C: case 0x17D: case 0x17F: case 0x180:
    case 0x182: case 0x183: case 0x185: case 0x186: case 0x188: case 0x189: return true;
    default: break;
    }
    return map == 0x13B || map == 0x13C || (map >= 0xCA && map <= 0xCE);
}
constexpr bool original_map_allowed(unsigned map) noexcept {
    return !((map >= 0xA1 && map <= 0xAC) || map == 0x158 || map == 0x159 ||
        map == 0x17E || map == 0x18B || (map >= 0x191 && map <= 0x199) ||
        map == 0x19C || map == 0x19D);
}
enum class BlockReason : uint8_t {
    None, NativeMultiplayer, NativePlayerCount, OriginalMapExclusion,
    ChallengeRoom, SaucerReturn, OptionOff, DevModeLocked
};
struct CreationInput {
    uint16_t map;
    uint8_t native_subtype;
    int32_t native_requested_count;
    bool option_enabled, dev_mode_unlocked;
    uint8_t saucer_route = 0;
};
struct CreationDecision { bool inject_second_player; BlockReason reason; };
constexpr CreationDecision decide_creation(CreationInput in) noexcept {
    if (in.native_subtype == 0x1B) return {false, BlockReason::NativeMultiplayer};
    if (in.native_requested_count != 1) return {false, BlockReason::NativePlayerCount};
    if (!original_map_allowed(in.map)) return {false, BlockReason::OriginalMapExclusion};
    if (challenge_room(in.map)) return {false, BlockReason::ChallengeRoom};
    if (in.map == 0xD6 && in.saucer_route == 6) return {false, BlockReason::SaucerReturn};
    if (!in.option_enabled) return {false, BlockReason::OptionOff};
    if (!in.dev_mode_unlocked) return {false, BlockReason::DevModeLocked};
    return {true, BlockReason::None};
}

// Game-thread ownership. Only input_player touches input-side rearming state;
// communication with input uses atomic value tokens, never guest pointers.
// Hold shortcut_mutex across reset/begin_creation/retire and associated menu/request
// cancellation; input holds it across input_player and form/cheat admission. Native
// construction/update/teardown executes outside this lock.
std::mutex& shortcut_mutex();
void reset();
void begin_creation(uint16_t map, bool promote);
void begin_normal_allocation();
void end_normal_allocation(uint8_t* rdram, int slot);
int allocation_slot();
bool finish_creation(uint8_t* rdram);
bool created_pair(uint8_t* rdram);
bool running(uint8_t* rdram);
int owned_slot(uint8_t* rdram, uint32_t owner);
bool owns_second(uint8_t* rdram, uint32_t owner);
void retire(uint8_t* rdram, uint32_t owner);
bool extra_activity(uint8_t* rdram, int native_camera);
class SerializationScope {
public:
    SerializationScope(uint8_t* rdram, uint32_t owner);
    ~SerializationScope();
private:
    uint32_t previous;
};
void publish_input(uint8_t* rdram, uint32_t owner, int controller);
int input_player(int controller, bool debug, uint16_t buttons, float x, float y);
}
