// Game-thread side of the Dev Mode warp: a hook on yaw_update (0x8009C88C), which the game runs once per frame for each
// player in gameplay. For player 1 it reads the map, entrance, position and facing, publishes them and carries out the
// teleport the UI queued. See docs/reverse-engineering/systems/warp-and-player-position.md for the addresses.

#include "tooie_dev_warp.h"
#include "tooie_dev_cheats.h"

#include <chrono>
#include <cstdio>
#include <cstring>

#include "recomp.h"

extern "C" void yaw_update(uint8_t* rdram, recomp_context* ctx);
extern "C" void yaw_set(uint8_t* rdram, recomp_context* ctx);
extern "C" void yaw_setIdeal(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_8009C0F8(uint8_t* rdram, recomp_context* ctx); // player position setter (PlayerState*, f32[3])
extern "C" void bs_setState(uint8_t* rdram, recomp_context* ctx);        // (PlayerState*, state)
extern "C" void bs_getCurrentState(uint8_t* rdram, recomp_context* ctx); // (PlayerState*) -> state
extern "C" void _gcgoto_entrypoint_1(uint8_t* rdram, recomp_context* ctx); // go to (map, entrance), as warp pads do

namespace {

constexpr uint32_t kPlayers = 0x80135490u;     // PlayerState* D_80135490[]: index 0 is player 1
constexpr uint32_t kGameState = 0x80127630u;   // D_80127630: game mode at +0x2 (s16)
constexpr int16_t kPlayMode = 3;               // playing a save file
constexpr int16_t kDemoMode = 5;               // attract demo
constexpr int32_t kWarpPendingOffset = 0x12;   // u8 D_80127630+0x12: set by func_800A7B18 when a map change is requested,
                                               // cleared by the main loop (func_800A7D84) when it starts the change
constexpr uint32_t kCurrentMap = 0x80132DC2u;      // u16, read by func_800EA05C (the game's current-map getter)
constexpr uint32_t kCurrentEntrance = 0x80132DC8u; // s16, read by func_800EA090 (compared with warp pad entrances)
constexpr int32_t kPlayerPositionOffset = 0xE4; // PlayerState+0xE4 -> struct with the position (f32[3]) at +0x0
constexpr int32_t kPlayerYawOffset = 0xF8;      // PlayerState+0xF8 -> BaYaw, value (f32, degrees) at +0x0

gpr kseg0(uint32_t addr) {
    return static_cast<gpr>(static_cast<int32_t>(addr));
}

bool is_rdram_pointer(uint32_t addr) {
    return (addr >= 0x80000000u) && (addr < 0x80800000u) && ((addr & 3u) == 0);
}

float read_float(uint8_t* rdram, uint32_t addr) {
    const uint32_t bits = static_cast<uint32_t>(MEM_W(0, kseg0(addr)));
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

uint32_t float_bits(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

double now_seconds() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

tooie_dev_warp::GameState read_state(uint8_t* rdram, uint32_t player) {
    tooie_dev_warp::GameState state;
    const gpr game = kseg0(kGameState);
    // Same test as the game's func_800A819C.
    const int16_t mode = MEM_H(0x2, game);
    state.gameplay = (mode == kPlayMode) || (mode == kDemoMode);
    state.warp_pending = (MEM_BU(kWarpPendingOffset, game) != 0);
    state.position.map = static_cast<int16_t>(MEM_HU(0, kseg0(kCurrentMap)));
    state.position.entrance = MEM_H(0, kseg0(kCurrentEntrance));
    const uint32_t position = static_cast<uint32_t>(MEM_W(kPlayerPositionOffset, kseg0(player)));
    const uint32_t yaw = static_cast<uint32_t>(MEM_W(kPlayerYawOffset, kseg0(player)));
    if (!is_rdram_pointer(position) || !is_rdram_pointer(yaw)) {
        return state;
    }

    state.player = player;
    for (uint32_t i = 0; i < 3; i++) {
        state.position.pos[i] = read_float(rdram, position + i * 4);
    }

    state.position.yaw = read_float(rdram, yaw);
    return state;
}

void apply_position(uint8_t* rdram, const recomp_context* ctx, uint32_t player, const tooie_dev_warp::Position& target) {
    // The setter takes a guest pointer: put the vector on the guest stack below the caller's frame.
    recomp_context local = *ctx;
    local.r29 = ADD32(ctx->r29, -0x40);
    const gpr vector = ADD32(local.r29, 0x20);
    for (int i = 0; i < 3; i++) {
        MEM_W(i * 4, vector) = static_cast<int32_t>(float_bits(target.pos[i]));
    }

    local.r4 = kseg0(player);
    local.r5 = vector;
    func_8009C0F8(rdram, &local);

    // The yaw setters take the angle's bits in a1 (o32: a float after an integer argument travels in a GPR).
    const gpr yaw = static_cast<gpr>(static_cast<int32_t>(float_bits(target.yaw)));
    local = *ctx;
    local.r29 = ADD32(ctx->r29, -0x40);
    local.r4 = kseg0(player);
    local.r5 = yaw;
    yaw_set(rdram, &local);
    local = *ctx;
    local.r29 = ADD32(ctx->r29, -0x40);
    local.r4 = kseg0(player);
    local.r5 = yaw;
    yaw_setIdeal(rdram, &local);
}

// Dev Mode fly cheat: put player 1 in the launch state a flight pad uses; the game then goes on to flying by itself.
void launch_flight(uint8_t* rdram, const recomp_context* ctx, uint32_t player) {
    recomp_context local = *ctx;
    local.r29 = ADD32(ctx->r29, -0x40);
    local.r4 = kseg0(player);
    bs_getCurrentState(rdram, &local);
    const uint32_t current = static_cast<uint32_t>(local.r2);
    if (!tooie_dev_cheats::can_launch_flight(current)) {
        std::fprintf(stderr, "[tooie-cheat] fly ignored in state 0x%X\n", unsigned(current));
        return;
    }

    local = *ctx;
    local.r29 = ADD32(ctx->r29, -0x40);
    local.r4 = kseg0(player);
    local.r5 = static_cast<gpr>(static_cast<int32_t>(tooie_dev_cheats::kStateFlyLaunch));
    bs_setState(rdram, &local);
    std::fprintf(stderr, "[tooie-cheat] fly from state 0x%X\n", unsigned(current));
}

} // namespace

extern "C" void tooie_dev_on_yaw_update(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t self = static_cast<uint32_t>(ctx->r4);
    yaw_update(rdram, ctx);

    const uint32_t player = static_cast<uint32_t>(MEM_W(0, kseg0(kPlayers)));
    if ((self != player) || !is_rdram_pointer(player)) {
        return;
    }

    tooie_dev_warp::Position target;
    const tooie_dev_warp::GameState state = read_state(rdram, player);
    switch (tooie_dev_warp::game_frame(state, now_seconds(), target)) {
    case tooie_dev_warp::Action::Goto: {
        recomp_context local = *ctx;
        local.r29 = ADD32(ctx->r29, -0x40);
        local.r4 = static_cast<gpr>(static_cast<int32_t>(target.map));
        local.r5 = static_cast<gpr>(static_cast<int32_t>(target.entrance));
        _gcgoto_entrypoint_1(rdram, &local);
        std::fprintf(stderr, "[tooie-warp] go to map %d entrance %d\n", int(target.map), int(target.entrance));
        break;
    }
    case tooie_dev_warp::Action::Apply:
        if (state.player != 0) {
            apply_position(rdram, ctx, player, target);
            std::fprintf(stderr, "[tooie-warp] placed player 1 at %.1f %.1f %.1f yaw %.1f in map %d\n",
                double(target.pos[0]), double(target.pos[1]), double(target.pos[2]), double(target.yaw), int(target.map));
        }
        break;
    case tooie_dev_warp::Action::GiveUp:
        std::fprintf(stderr, "[tooie-warp] teleport to map %d timed out\n", int(target.map));
        break;
    default:
        break;
    }

    if (tooie_dev_cheats::take_fly_request() && state.gameplay && (state.player != 0) && !tooie_dev_warp::teleport_busy()) {
        launch_flight(rdram, ctx, player);
    }
}
