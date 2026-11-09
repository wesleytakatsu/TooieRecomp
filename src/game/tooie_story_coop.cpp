#include "tooie_guest_memory.h"
#include "tooie_story_coop.h"
#include "tooie_dev_cheats.h"
#include "recomp.h"
#include <atomic>
#include <cmath>

namespace tooie_story_coop {
namespace {
gpr guest(uint32_t p) { return int32_t(p); }
bool pointer(uint32_t p) { return tooie_memory::aligned(p, 0x201); }
struct Pair {
    uint16_t map = 0;
    bool creating = false, committed = false, allocating = false;
    unsigned allocations = 0;
    std::array<uint32_t, 2> owners{};
    std::array<int32_t, 2> cameras{}; // +0x184 of each owner when the pair was committed
} pair;
uint32_t serializing = 0;
// Epoch is an input ownership token: a port not polled while absent must still
// rearm when the same player returns. It is not telemetry.
uint64_t epoch = 0;
std::mutex shortcut_lock;
std::array<std::atomic<uint64_t>, 4> ports{};
struct Rearm { uint64_t token = 0; bool armed = false; };
std::array<Rearm, 4> rearm{}; // input callback only
uint32_t table(uint8_t* rdram, int n) { return MEM_W(n * 4, guest(0x80135490)); }
bool context(uint8_t* rdram) {
    return MEM_HU(0, guest(0x80132DC2)) == pair.map && MEM_BU(0, guest(0x8012762C)) != 0x1B;
}
bool published(uint8_t* rdram, int n) {
    const uint32_t current = table(rdram, n);
    // The game can move a live player and rewrite its table slot without the native teardown that retires an owner
    // (seen on Windows, map 0xAF; heap compaction is the likely cause). A committed owner follows its slot to the new
    // address when the object there still carries the camera index it had at creation. Identity still comes from the
    // creation-time allocation: a different player in the slot, or anything after a retire, is not adopted.
    if (pair.committed && pointer(pair.owners[n]) && pointer(current) && current != pair.owners[n] &&
        current != pair.owners[1 - n] && MEM_W(0x184, guest(current)) == pair.cameras[n]) {
        pair.owners[n] = current;
    }
    return pointer(pair.owners[n]) && current == pair.owners[n];
}
bool constructed(uint8_t* rdram) {
    return pair.allocations == 2 && published(rdram, 0) && published(rdram, 1) &&
        pair.owners[0] != pair.owners[1];
}
}
std::mutex& shortcut_mutex() { return shortcut_lock; }
void reset() {
    pair = {};
    serializing = 0;
    ++epoch;
    for (auto& port : ports) port.store(0);
}
void begin_creation(uint16_t map, bool promote) {
    reset();
    pair.map = map;
    pair.creating = promote;
}
void begin_normal_allocation() { pair.allocating = pair.creating; }
int allocation_slot() {
    return pair.allocating && pair.allocations < 2 ? int(pair.allocations) : -1;
}
void end_normal_allocation(uint8_t* rdram, int slot) {
    if (pair.creating) {
        const unsigned expected = pair.allocations++;
        if (expected < 2 && slot == int(expected) && pointer(table(rdram, slot)))
            pair.owners[expected] = table(rdram, slot);
    }
    pair.allocating = false;
}
bool finish_creation(uint8_t* rdram) {
    pair.committed = pair.creating && context(rdram) && constructed(rdram);
    pair.creating = false;
    if (pair.committed) {
        for (int n = 0; n < 2; ++n) pair.cameras[n] = MEM_W(0x184, guest(pair.owners[n]));
    }
    else pair.owners = {};
    return pair.committed;
}
bool created_pair(uint8_t* rdram) {
    return pair.committed && context(rdram) && constructed(rdram);
}
bool running(uint8_t* rdram) {
    return created_pair(rdram) && tooie_dev_cheats::two_players_active();
}
int owned_slot(uint8_t* rdram, uint32_t owner) {
    if (!context(rdram)) return -1;
    for (int n = 0; n < 2; ++n) if (pair.owners[n] == owner && published(rdram, n)) return n;
    return -1;
}
bool owns_second(uint8_t* rdram, uint32_t owner) { return running(rdram) && owned_slot(rdram, owner) == 1; }
void retire(uint8_t* rdram, uint32_t owner) {
    const int n = owned_slot(rdram, owner);
    if (n < 0 && (!pointer(owner) || table(rdram, 0) != owner)) return;
    if (n >= 0) {
        pair.owners[n] = 0;
        pair.committed = false;
    }
    ++epoch;
    for (auto& port : ports) port.store(0);
}
bool extra_activity(uint8_t* rdram, int camera) {
    if (!context(rdram) || !tooie_dev_cheats::two_players_active() || !published(rdram, 1) ||
        MEM_W(0x184, guest(pair.owners[1])) != camera) return false;
    return running(rdram) || (pair.creating && constructed(rdram)) || serializing == pair.owners[1];
}
SerializationScope::SerializationScope(uint8_t* rdram, uint32_t owner) : previous(serializing) {
    serializing = owned_slot(rdram, owner) == 1 ? owner : 0;
}
SerializationScope::~SerializationScope() { serializing = previous; }
void publish_input(uint8_t* rdram, uint32_t owner, int controller) {
    if (controller < 0 || controller >= int(ports.size())) return;
    int player = -1;
    if (pointer(owner) && table(rdram, 0) == owner) player = 0;
    else if (created_pair(rdram) && owned_slot(rdram, owner) == 1) player = 1;
    // Remove an old binding for this player when the native controller changes.
    if (player >= 0) for (int n = 0; n < int(ports.size()); ++n)
        if (n != controller && (ports[n].load() & 3) == uint64_t(player + 1)) ports[n].store(0);
    const uint64_t token = player < 0 ? 0 : (epoch << 2) | uint64_t(player + 1);
    // An auxiliary sharing a controller must not revoke the legitimate owner.
    if (player >= 0) ports[controller].store(token);
}
int input_player(int controller, bool debug, uint16_t buttons, float x, float y) {
    if (controller < 0 || controller >= int(ports.size())) return -1;
    auto& state = rearm[controller];
    const uint64_t token = ports[controller].load();
    if (state.token != token) state = {token, false};
    if (!token) return -1;
    if (!state.armed) {
        // Debug/R3, D-pad, A/B and menu navigation must all be released first.
        if (!debug && !(buttons & 0xCF00) && std::isfinite(x) && std::isfinite(y) &&
            std::abs(x) < .25f && std::abs(y) < .25f) state.armed = true;
        if (!state.armed) return -1;
    }
    return int(token & 3) - 1;
}
}
