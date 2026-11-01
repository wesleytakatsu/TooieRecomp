// Game-thread side of the Dev Mode warp and cheats. A hook on yaw_update (0x8009C88C), which the game runs once per frame
// for each player in most forms, reads player 1's map, entrance, position and facing, publishes them and carries out the
// teleport the UI queued; a hook on the per-player update (0x800991B0) carries out the cheats the Debug Button queued.
// See docs/reverse-engineering/systems/warp-and-player-position.md for the addresses.

#include "tooie_dev_warp.h"
#include "tooie_dev_cheats.h"
#include "tooie_dev_cheats_hook.h"
#include "tooie_dev_forms_hook.h"
#include "tooie_dev_enemy_ai.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>

#include "recomp.h"

extern "C" void yaw_update(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800991B0(uint8_t* rdram, recomp_context* ctx); // per-player update (PlayerState*)
extern "C" void func_8009359C(uint8_t* rdram, recomp_context* ctx); // set the second model (0 removes it)
extern "C" void func_800F73C4(uint8_t* rdram, recomp_context* ctx); // create the players of a loaded map (count)
extern "C" void func_800A8FDC(uint8_t* rdram, recomp_context* ctx); // add one split-screen viewport
extern "C" void func_80016BDC(uint8_t* rdram, recomp_context* ctx); // osContSetCh(channels): controller ports read
extern "C" void func_800F65D0(uint8_t* rdram, recomp_context* ctx); // is player index active (collides, takes damage)
extern "C" void func_80092EC8(uint8_t* rdram, recomp_context* ctx); // primary player model setter
extern "C" void func_800941C8(uint8_t* rdram, recomp_context* ctx); // player LOD model setter
extern "C" void func_80100D24(uint8_t* rdram, recomp_context* ctx); // allocate model instance (a2: private vertex buffers)
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
constexpr float kSecondPlayerSide = 70.0f;      // how far beside player 1 player 2 appears (game units)
constexpr double kSecondPlayerDelay = 2.0;      // after a map load, wait this long and for player 1 to stand still
constexpr float kSecondPlayerLift = 60.0f;      // player 2 appears this far above player 1's height and lands
constexpr uint32_t kViewportCount = 0x801276D2u; // s16: split-screen viewports of the map, set by func_800A8D70
constexpr uint32_t kStateWaiting = 0x74;         // state the game leaves an extra player in (no input, base animation)

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

// Experimental two players: puts player 2 (D_80135490[1]) close to player 1's right, facing the same way, so the two do
// not overlap.
void place_second_player(uint8_t* rdram, const recomp_context* ctx, const tooie_dev_warp::Position& first) {
    const uint32_t second = static_cast<uint32_t>(MEM_W(4, kseg0(kPlayers)));
    if (!is_rdram_pointer(second)) {
        return;
    }

    tooie_dev_warp::Position beside = first;
    const float radians = first.yaw * 3.14159265f / 180.0f;
    // (sin, cos) of the facing points to the player's right (first two-player playtest).
    beside.pos[0] += std::sin(radians) * kSecondPlayerSide;
    beside.pos[2] += std::cos(radians) * kSecondPlayerSide;
    apply_position(rdram, ctx, second, beside);
}

// Time from which player 2 may be placed beside player 1 after a map load, or a negative value when nothing is pending.
double second_player_due = -1.0;
// Set when player 2 has just been placed beside player 1 and must be started on its next frame.
bool second_player_start_next = false;
std::optional<tooie_dev_warp::Position> second_player_last_first;
double second_player_still_since = -1.0;

// Latched before native player creation so model setup can request private lit vertices for this story map.
bool story_two_players = false;
// Player initialization builds the model before publishing the new PlayerState in kPlayers.
bool creating_story_players = false;
thread_local bool private_player_vertices_scope = false;
thread_local uint32_t player_model_owner = 0;
thread_local const char* player_model_kind = "none";

// Opt-in model-allocation trace. No per-frame hooks, RGB hashing or guest function calls are added for logging.
double player_lighting_time() {
    static const double start = now_seconds();
    return now_seconds() - start;
}

FILE* player_lighting_log() {
    static FILE* file = []() -> FILE* {
        const char* path = std::getenv("TOOIE_DIAG_2P_LIGHTING_LOG");
        if (path == nullptr || path[0] == '\0') {
            return nullptr;
        }
        FILE* opened = std::fopen(path, "w");
        if (opened != nullptr) {
            const auto utc_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            (void)player_lighting_time();
            std::fprintf(opened, "# Player lighting allocation trace; opened_utc_ms=%lld; t=seconds since log opened.\n",
                static_cast<long long>(utc_ms));
            std::fflush(opened);
        } else {
            std::fprintf(stderr, "[tooie-2p-lighting] cannot open log: %s\n", path);
        }
        return opened;
    }();
    return file;
}

uint32_t lighting_word(uint8_t* rdram, uint32_t pointer, uint32_t offset) {
    return is_rdram_pointer(pointer) && offset <= 0x807FFFFCu - pointer ?
        static_cast<uint32_t>(MEM_W(offset, kseg0(pointer))) : 0;
}

uint32_t lighting_instance(uint8_t* rdram, uint32_t index) {
    const uint32_t pool = lighting_word(rdram, 0x80135A60u, 0);
    if (!is_rdram_pointer(pool) || index == 0 || index > 0xFF) {
        return 0;
    }
    const int stride = MEM_H(0, kseg0(pool)); // freelist_at: pool + 4 + index * signed stride
    if (stride < 12 || stride > 256 || (stride & 3) != 0) {
        return 0;
    }
    const uint32_t entry = pool + 4 + index * static_cast<uint32_t>(stride);
    return is_rdram_pointer(entry) && entry <= 0x807FFFF4u ? entry : 0;
}

bool private_player_vertices_enabled() {
    static const bool enabled = [] {
        const char* value = std::getenv("TOOIE_TEST_2P_PRIVATE_VERTICES");
        return value == nullptr || value[0] != '0';
    }();
    return enabled;
}

void set_player_model(uint8_t* rdram, recomp_context* ctx, void (*native)(uint8_t*, recomp_context*), const char* kind) {
    const uint32_t self = static_cast<uint32_t>(ctx->r4);
    tooie_dev_forms::PlayerScope player_scope(rdram, self);
    const bool previous = private_player_vertices_scope;
    const uint32_t previous_owner = player_model_owner;
    const char* previous_kind = player_model_kind;
    player_model_owner = self;
    player_model_kind = kind;
    private_player_vertices_scope = previous || (story_two_players && private_player_vertices_enabled() &&
        is_rdram_pointer(self) && (creating_story_players || self == static_cast<uint32_t>(MEM_W(0, kseg0(kPlayers))) ||
        self == static_cast<uint32_t>(MEM_W(4, kseg0(kPlayers)))));
    native(rdram, ctx);
    private_player_vertices_scope = previous;
    player_model_owner = previous_owner;
    player_model_kind = previous_kind;
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
            if (tooie_dev_cheats::two_players_active()) {
                place_second_player(rdram, ctx, target);
            }
        }
        break;
    case tooie_dev_warp::Action::GiveUp:
        std::fprintf(stderr, "[tooie-warp] teleport to map %d timed out\n", int(target.map));
        break;
    default:
        break;
    }
}

// The game skips yaw_update in some forms (the Breegull Blaster turns the player itself), so cheats run from the per-player
// update instead, which runs every gameplay frame for each player in every form.
extern "C" void tooie_dev_on_player_update(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t self = static_cast<uint32_t>(ctx->r4);
    tooie_dev_forms::PlayerScope player_scope(rdram, self);
    func_800991B0(rdram, ctx);
    tooie_dev_forms::update_player(rdram, ctx, self);

    const uint32_t player = static_cast<uint32_t>(MEM_W(0, kseg0(kPlayers)));
    const uint32_t second = static_cast<uint32_t>(MEM_W(4, kseg0(kPlayers)));
    if (tooie_dev_cheats::two_players_active() && (self == second) && is_rdram_pointer(second) &&
        is_rdram_pointer(player)) {
        // Outside the multiplayer minigames the game leaves player 2 waiting in state 0x74, parked where there is no
        // ground. Once the map has been loaded for kSecondPlayerDelay and player 1 stands still, put player 2 beside
        // player 1 while it still waits (its physics are stopped, so it keeps the new place), then start it idle on the
        // next frame so it takes its controller.
        recomp_context local = *ctx;
        local.r29 = ADD32(ctx->r29, -0x40);
        local.r4 = kseg0(second);
        bs_getCurrentState(rdram, &local);
        const uint32_t second_state = static_cast<uint32_t>(local.r2);
        local = *ctx;
        local.r29 = ADD32(ctx->r29, -0x40);
        local.r4 = kseg0(player);
        bs_getCurrentState(rdram, &local);
        const uint32_t first_state = static_cast<uint32_t>(local.r2);
        if (second_player_start_next) {
            second_player_start_next = false;
            if (second_state == kStateWaiting) {
                local = *ctx;
                local.r29 = ADD32(ctx->r29, -0x40);
                local.r4 = kseg0(second);
                local.r5 = tooie_dev_forms::default_state(rdram, ctx, second);
                bs_setState(rdram, &local);
            }
        }

        if (second_player_due >= 0.0 && second_state != kStateWaiting && !second_player_start_next) {
            // A menu transformation can start J2 before the delayed release; do not teleport it again later.
            second_player_due = -1.0;
        }
        if (second_player_due >= 0.0) {
            const tooie_dev_warp::GameState first_frame = read_state(rdram, player);
            if (first_frame.player != 0) {
                const double now = now_seconds();
                float movement = 0;
                if (second_player_last_first) {
                    for (int axis = 0; axis < 3; ++axis) {
                        const float delta = first_frame.position.pos[axis] - second_player_last_first->pos[axis];
                        movement += delta * delta;
                    }
                }
                if (!second_player_last_first || movement > 0.01f) second_player_still_since = now;
                second_player_last_first = first_frame.position;
                // Observe position instead of a Banjo-specific idle ID (Blaster/Bee/etc. have other states).
                if (now >= second_player_due && now - second_player_still_since >= 0.2 && first_state != kStateWaiting) {
                    second_player_due = -1.0;
                    // Placed at player 1's exact height, player 2 starts inside the ground and falls through it.
                    tooie_dev_warp::Position above = first_frame.position;
                    above.pos[1] += kSecondPlayerLift;
                    place_second_player(rdram, ctx, above);
                    second_player_start_next = true;
                }
            }
        }
    }

    const int16_t mode = MEM_H(0x2, kseg0(kGameState));
    const bool gameplay = (mode == kPlayMode) || (mode == kDemoMode);
    tooie_dev_cheats::update_player(rdram, ctx, self, gameplay,
        tooie_dev_warp::teleport_busy() || MEM_BU(kWarpPendingOffset, kseg0(kGameState)) != 0);
}

// Experimental two players: the game creates the players of every map it loads here, one outside the multiplayer
// minigames (gcstatusDll_entrypoint_11). Asking for two makes both appear at the entrance; player 2 is then moved ahead.
// Each new player takes the next free controller and the viewport of that number, clamped to the map's viewport count
// (func_800A907C); func_800A8D70 sets that count to 1 outside the multiplayer maps, so add a second viewport first. The
// game select also sets the controller ports read to one (osContSetCh), so ask for two.
extern "C" void tooie_dev_on_create_players(uint8_t* rdram, recomp_context* ctx) {
    tooie_dev_enemy_ai::reset();
    const int32_t requested = static_cast<int32_t>(ctx->r4);
    tooie_dev_cheats::reset_gameplay();
    second_player_due = -1.0;
    second_player_start_next = false;
    second_player_last_first.reset();
    second_player_still_since = -1.0;
    tooie_dev_forms::begin_map(requested == 1 && MEM_BU(0, kseg0(0x8012762Cu)) != 0x1B &&
        tooie_dev_cheats::two_players_map(MEM_HU(0, kseg0(kCurrentMap))));
    const bool two = tooie_dev_cheats::two_players_active() && (requested == 1) &&
        tooie_dev_cheats::two_players_map(MEM_HU(0, kseg0(kCurrentMap)));
    story_two_players = two;
    if (FILE* file = player_lighting_log()) {
        std::fprintf(file, "MAP t=%.3f map=%u requested=%d two=%u private_vertices=%u\n", player_lighting_time(),
            unsigned(MEM_HU(0, kseg0(kCurrentMap))), requested, unsigned(two), unsigned(private_player_vertices_enabled()));
        std::fflush(file);
    }
    recomp_context local = *ctx;
    if (two) {
        local.r29 = ADD32(ctx->r29, -0x40);
        if (MEM_H(0, kseg0(kViewportCount)) == 1) {
            func_800A8FDC(rdram, &local);
        }

        // The game select limits the controller ports read to one for a save file; player 2 reads port 2.
        local.r4 = 2;
        func_80016BDC(rdram, &local);
        ctx->r4 = 2;
    }

    local = *ctx;
    const bool previous_creation = creating_story_players;
    creating_story_players = two;
    func_800F73C4(rdram, ctx);
    creating_story_players = previous_creation;
    if (!two) {
        return;
    }

    const uint32_t first = static_cast<uint32_t>(MEM_W(0, kseg0(kPlayers)));
    // Put player 2 beside player 1 at once (it waits there with its physics stopped); after kSecondPlayerDelay it is
    // placed beside player 1 again, wherever player 1 walked to, and started.
    if (is_rdram_pointer(first)) {
        const tooie_dev_warp::GameState state = read_state(rdram, first);
        if (state.player != 0) {
            place_second_player(rdram, &local, state.position);
        }
    }

    second_player_due = now_seconds() + kSecondPlayerDelay;
}

extern "C" void tooie_dev_on_primary_player_model(uint8_t* rdram, recomp_context* ctx) {
    set_player_model(rdram, ctx, func_80092EC8, "primary");
}

extern "C" void tooie_dev_on_lod_player_model(uint8_t* rdram, recomp_context* ctx) {
    set_player_model(rdram, ctx, func_800941C8, "lod");
}

extern "C" void tooie_dev_on_secondary_player_model(uint8_t* rdram, recomp_context* ctx) {
    set_player_model(rdram, ctx, func_8009359C, "secondary");
}

extern "C" void tooie_dev_on_model_instance(uint8_t* rdram, recomp_context* ctx) {
    FILE* file = player_model_owner != 0 ? player_lighting_log() : nullptr;
    const uint32_t requested = static_cast<uint32_t>(ctx->r6);
    const uint32_t lighting = static_cast<uint32_t>(ctx->r7);
    const uint32_t model = static_cast<uint32_t>(ctx->r4);
    const uint32_t asset = static_cast<uint32_t>(ctx->r5);
    // Native player setup requests no private vertices for models without vertex animation. CPU lighting then writes
    // into the cached model's vertices, shared by two identical characters. Ask the existing model-instance allocator
    // for one private vertex buffer when this scoped player model uses CPU lighting. Native instance teardown owns it.
    if (private_player_vertices_scope && ctx->r6 == 0 && ctx->r7 != 0) {
        ctx->r6 = 1;
    }
    const uint32_t actual = static_cast<uint32_t>(ctx->r6);
    func_80100D24(rdram, ctx);
    if (file != nullptr) {
        const uint32_t index = static_cast<uint32_t>(ctx->r2);
        const uint32_t instance = lighting_instance(rdram, index);
        const uint32_t private_vertices = lighting_word(rdram, instance, 0);
        const uint32_t output = private_vertices != 0 ? private_vertices : model + lighting_word(rdram, model, 0x10);
        int slot = -1; // Initial model setup may precede publication in the player table.
        for (int i = 0; i < 2; ++i) {
            if (lighting_word(rdram, kPlayers, i * 4) == player_model_owner) slot = i;
        }
        std::fprintf(file, "MODEL t=%.3f map=%u kind=%s owner=%08X slot=%d creating=%u asset=%X instance=%u requested=%u actual=%u lighting=%u model=%08X output=%08X private=%08X private1=%08X lightstate=%08X tintstate=%08X\n",
            player_lighting_time(), unsigned(MEM_HU(0, kseg0(kCurrentMap))), player_model_kind, player_model_owner, slot,
            unsigned(creating_story_players), asset, index, requested, actual, lighting, model, output, private_vertices,
            lighting_word(rdram, instance, 4), lighting_word(rdram, player_model_owner, 0xB4),
            lighting_word(rdram, player_model_owner, 0x108));
        std::fflush(file);
    }
}

// Experimental two players: func_800F65D0(index) says whether a player is active; the player update only checks
// collisions with items, enemies and doors for active players (func_8008E6F0 -> func_8008DAA8). It answers yes for
// every player in the multiplayer minigames (game mode 0x1B) and only for the focused player (D_801354DC) otherwise,
// so answer yes for player 2 too.
extern "C" void tooie_dev_on_player_active(uint8_t* rdram, recomp_context* ctx) {
    const int32_t index = static_cast<int32_t>(ctx->r4);
    func_800F65D0(rdram, ctx);
    if ((ctx->r2 == 0) && (index == 1) && tooie_dev_cheats::two_players_active() &&
        is_rdram_pointer(static_cast<uint32_t>(MEM_W(4, kseg0(kPlayers))))) {
        ctx->r2 = 1;
    }
}
