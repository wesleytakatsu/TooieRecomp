#include <cstdio>
#include <cstdlib>
#include "tooie_dev_cheats.h"
#include "tooie_dev_cheats_hook.h"
#include <vector>

using namespace tooie_dev_cheats;

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); std::exit(1); } } while (0)

// Exercise the actual game-thread executor with two guest PlayerStates and native lifecycle stand-ins.
static constexpr uint32_t owners[2] = {0x80010000, 0x80011000};
static uint32_t states[2]{}, requested[2]{};
static int reloads[2]{}, secondary_removals[2]{};
static gpr guest(uint32_t address) { return static_cast<int32_t>(address); }
static int owner(recomp_context* ctx) {
    for (int i = 0; i < 2; ++i) if (uint32_t(ctx->r4) == owners[i]) return i;
    CHECK(false); return -1;
}
static void form(uint8_t* rdram, int p, uint8_t value) { MEM_B(0x29, MEM_W(0xA0, guest(owners[p]))) = value; }
extern "C" {
void bs_getCurrentState(uint8_t*, recomp_context* ctx) { ctx->r2 = states[owner(ctx)]; }
void bs_setState(uint8_t* rdram, recomp_context* ctx) {
    const int p = owner(ctx);
    requested[p] = states[p] = uint32_t(ctx->r5);
    if (states[p] == kStateBlasterEnter) {
        form(rdram, p, kTransformBlaster);
        states[p] = 0xCA; // Stand-in finishes native entry; tests finish exit explicitly.
    }
}
void func_8009359C(uint8_t*, recomp_context* ctx) { CHECK(ctx->r5 == 0); ++secondary_removals[owner(ctx)]; }
void func_80092778(uint8_t*, recomp_context* ctx) { ++reloads[owner(ctx)]; }
}

int main() {
    const uint16_t up = direction_button(Direction::Up);
    const uint16_t left = direction_button(Direction::Left);
    const uint16_t right = direction_button(Direction::Right);
    const uint16_t b_button = 0x4000;

    // Directions map to the N64 D-pad bits and round-trip through their names.
    {
        CHECK(up == 0x0800 && direction_button(Direction::Down) == 0x0400);
        CHECK(left == 0x0200 && right == 0x0100);
        for (Direction direction : {Direction::Up, Direction::Down, Direction::Left, Direction::Right}) {
            CHECK(direction_from_name(direction_name(direction)) == direction);
        }

        CHECK(!direction_from_name("diagonal").has_value());
    }

    // A chord fires once per D-pad press, only while the Debug Button is held, and ignores other buttons.
    {
        Chord chord;
        CHECK(chord.step(false, up) == 0);        // D-pad without the Debug Button
        CHECK(chord.step(true, up) == 0);         // still the same press
        CHECK(chord.step(true, 0) == 0);
        CHECK(chord.step(true, up) == up);        // new press while held
        CHECK(chord.step(true, up) == 0);         // held, not repeated
        CHECK(chord.step(true, up | left) == left);
        CHECK(chord.step(true, b_button) == 0);   // face buttons are not part of a chord
        CHECK(chord.step(false, 0) == 0);
        CHECK(chord.step(true, left) == left);    // Debug Button pressed first, then the direction
    }

    // Defaults: everything off, Fly on up and the Blaster on right.
    {
        const Settings defaults;
        CHECK(!defaults[Cheat::Fly].enabled && defaults[Cheat::Fly].direction == Direction::Up);
        CHECK(!defaults[Cheat::Blaster].enabled && defaults[Cheat::Blaster].direction == Direction::Right);
        CHECK(defaults.character_menu_direction == Direction::Down);
    }

    // A request needs the unlock, the cheat enabled and its own direction, and is taken once.
    {
        set_settings(Settings{});
        on_player_input(0, true, 0);
        on_player_input(0, true, up);
        CHECK(!take_request(0, Cheat::Fly)); // locked and disabled

        set_unlocked(true);
        on_player_input(0, true, 0);
        on_player_input(0, true, up);
        CHECK(!take_request(0, Cheat::Fly)); // disabled

        Settings enabled;
        enabled[Cheat::Fly] = {true, Direction::Left};
        enabled[Cheat::Blaster] = {true, Direction::Right};
        set_settings(enabled);
        on_player_input(0, true, 0);
        on_player_input(0, true, up);
        CHECK(!take_request(0, Cheat::Fly) && !take_request(0, Cheat::Blaster)); // no cheat on up
        on_player_input(0, true, 0);
        on_player_input(0, true, left);
        CHECK(take_request(0, Cheat::Fly) && !take_request(0, Cheat::Blaster));
        CHECK(!take_request(0, Cheat::Fly));
        on_player_input(0, true, 0);
        on_player_input(0, true, right);
        CHECK(take_request(0, Cheat::Blaster) && !take_request(0, Cheat::Fly));

        set_unlocked(false);
        on_player_input(0, true, 0);
        on_player_input(0, true, left);
        CHECK(!take_request(0, Cheat::Fly));

        // The two-player option also needs the unlock.
        Settings two;
        two.two_players = true;
        set_settings(two);
        CHECK(!two_players_active());
        set_unlocked(true);
        CHECK(two_players_active());
        set_settings(Settings{});
        CHECK(!two_players_active());
        set_unlocked(false);

        CHECK(two_players_map(0x142) && two_players_map(0xB8)); // Jinjo Village, Mayahem Temple
        CHECK(!two_players_map(0x158) && !two_players_map(0x18B) && !two_players_map(0xA1) && !two_players_map(0x19C));
    }

    // Independent edge history and queues, including the same direction pressed on both controllers.
    {
        Settings enabled;
        enabled[Cheat::Fly] = {true, Direction::Up};
        enabled[Cheat::Blaster] = {true, Direction::Right};
        set_settings(enabled);
        set_unlocked(true);
        for (int p : {0, 1}) {
            on_player_input(p, true, 0);
            on_player_input(p, true, up);
        }
        CHECK(take_request(0, Cheat::Fly) && !take_request(0, Cheat::Fly));
        CHECK(take_request(1, Cheat::Fly));
        on_player_input(1, true, up);
        CHECK(!take_request(1, Cheat::Fly)); // Held on J2 does not repeat.
        on_player_input(1, true, 0);
        on_player_input(1, true, right);
        reset_requests();
        on_player_input(1, true, right);
        CHECK(!take_request(1, Cheat::Blaster)); // Map reset does not replay the held chord.
        on_player_input(1, true, 0);
        on_player_input(1, false, right); // Input consumed by a character menu cannot queue a cheat.
        on_player_input(1, true, right);
        CHECK(!take_request(1, Cheat::Blaster));
        on_player_input(-1, true, up);
        on_player_input(kPlayerCount, true, up);
        CHECK(!take_request(-1, Cheat::Fly) && !take_request(kPlayerCount, Cheat::Fly));
        CHECK(!take_request(0, Cheat::Count));
        on_player_input(1, true, 0);
        on_player_input(1, true, right);
        set_settings(Settings{});
        set_settings(enabled);
        CHECK(!take_request(1, Cheat::Blaster)); // Off/on cannot revive an old queued action.
        on_player_input(1, true, 0);
        on_player_input(1, true, up);
        set_unlocked(false);
        set_unlocked(true);
        CHECK(!take_request(1, Cheat::Fly));
    }

    // Actual per-player executor: J2 cannot trigger J1, and Blaster exit cleanup belongs to its owner.
    {
        std::vector<uint8_t> memory(8 * 1024 * 1024);
        auto* rdram = memory.data();
        recomp_context ctx{};
        ctx.r29 = guest(0x807FF000);
        for (int p : {0, 1}) {
            MEM_W(p * 4, guest(0x80135490)) = guest(owners[p]);
            MEM_W(0xA0, guest(owners[p])) = guest(0x80012000 + p * 0x100);
            form(rdram, p, kTransformBanjoKazooie);
            states[p] = kStateIdle;
        }
        auto queue = [&](int p, uint16_t button) {
            on_player_input(p, true, 0);
            on_player_input(p, true, button);
        };
        auto update = [&](int p, bool gameplay = true, bool blocked = false) {
            update_player(rdram, &ctx, owners[p], gameplay, blocked);
        };
        auto finish_exit = [&](int p) { form(rdram, p, kTransformBanjoKazooie); states[p] = kStateIdle; };
        reset_gameplay();
        queue(1, up);
        update(0);
        CHECK(states[0] == kStateIdle && states[1] == kStateIdle);
        update(1);
        CHECK(requested[1] == kStateFlyLaunch && states[0] == kStateIdle);
        finish_exit(1);
        queue(0, up); queue(1, up);
        update(1); update(0);
        CHECK(states[0] == kStateFlyLaunch && states[1] == kStateFlyLaunch);
        for (int p : {0, 1}) finish_exit(p);

        queue(1, right); update(1);
        CHECK(requested[1] == kStateBlasterEnter && states[0] == kStateIdle);
        queue(0, right); update(0);
        CHECK(states[0] == 0xCA && states[1] == 0xCA);
        queue(1, right); update(1);
        CHECK(requested[1] == kStateBlasterExit && states[0] == 0xCA);
        update(0); update(1);
        CHECK(reloads[0] == 0 && reloads[1] == 0); // Wait for the native exit animation.
        finish_exit(1);
        update(0); update(1);
        CHECK(reloads[0] == 0 && reloads[1] == 1 && secondary_removals[1] == 1);
        CHECK(states[0] == 0xCA);
        queue(0, right); update(0);
        set_settings(Settings{}); // Disabling new requests must still allow the pending exit to finish.
        finish_exit(0); update(0);
        CHECK(reloads[0] == 1 && secondary_removals[0] == 1 && reloads[1] == 1);
        queue(1, up); update(1);
        CHECK(states[1] == kStateIdle);

        Settings enabled;
        enabled[Cheat::Fly] = {true, Direction::Up};
        enabled[Cheat::Blaster] = {true, Direction::Right};
        set_settings(enabled);
        queue(1, up); update(1, false);
        update(1);
        CHECK(states[1] == kStateIdle); // Loading drops the request rather than deferring it.
        queue(1, up); update(1, true, true);
        CHECK(states[1] == kStateIdle);
        form(rdram, 1, 6);
        queue(1, up); update(1);
        queue(1, right); update(1);
        CHECK(states[1] == kStateIdle); // Bee remains subject to the original restrictions.
        finish_exit(1);

        for (int p : {0, 1}) { queue(p, right); update(p); queue(p, right); update(p); queue(p, up); }
        reset_gameplay();
        for (int p : {0, 1}) { finish_exit(p); update(p); }
        CHECK(states[0] == kStateIdle && states[1] == kStateIdle);
        CHECK(reloads[0] == 1 && reloads[1] == 1); // New PlayerStates cannot inherit old cleanup/requests.
        queue(1, up);
        update_player(rdram, &ctx, 0x80013000, true, false); // Valid pointer but not an active player slot.
        update(1);
        CHECK(states[1] == kStateFlyLaunch && states[0] == kStateIdle);
    }

    // Fly: only Banjo and Kazooie standing or walking.
    {
        CHECK(fly_state(kTransformBanjoKazooie, kStateIdle) == kStateFlyLaunch);
        CHECK(fly_state(kTransformBanjoKazooie, 0x2) == kStateFlyLaunch);
        CHECK(fly_state(kTransformBanjoKazooie, 0x4) == kStateFlyLaunch);
        CHECK(!fly_state(kTransformBanjoKazooie, kStateFlying));
        CHECK(!fly_state(kTransformBanjoKazooie, 0x2F));
        CHECK(!fly_state(kTransformBlaster, kStateIdle));
        CHECK(!fly_state(0xD, kStateIdle)); // Mumbo
    }

    // Blaster: enters from the ground, leaves from its own idle or movement states, nothing else.
    {
        CHECK(blaster_state(kTransformBanjoKazooie, kStateIdle) == kStateBlasterEnter);
        CHECK(blaster_state(kTransformBanjoKazooie, 0x4) == kStateBlasterEnter);
        for (uint32_t state : {0xCAu, 0xCCu, 0xCEu, 0xD1u}) {
            CHECK(blaster_state(kTransformBlaster, state) == kStateBlasterExit);
        }

        CHECK(!blaster_state(kTransformBlaster, kStateBlasterEnter)); // still switching on
        CHECK(!blaster_state(kTransformBlaster, kStateBlasterExit));
        CHECK(!blaster_state(kTransformBanjoKazooie, kStateFlying));
        CHECK(!blaster_state(kTransformBanjoKazooie, 0xCA));
        CHECK(!blaster_state(0xD, kStateIdle));
    }

    // Settings round-trip, and bad input falls back to the defaults.
    {
        Settings settings;
        settings[Cheat::Fly] = {true, Direction::Down};
        settings[Cheat::Blaster] = {true, Direction::Left};
        settings.two_players = true;
        settings.character_menu = false;
        settings.character_menu_direction = Direction::Left;
        const Settings back = parse_settings(serialize_settings(settings));
        CHECK(back[Cheat::Fly].enabled && back[Cheat::Fly].direction == Direction::Down);
        CHECK(back[Cheat::Blaster].enabled && back[Cheat::Blaster].direction == Direction::Left);
        CHECK(back.two_players);
        CHECK(!back.character_menu);
        CHECK(back.character_menu_direction == Direction::Left);
        CHECK(parse_settings("{}").character_menu_direction == Direction::Down);
        CHECK(parse_settings("{\"character_menu_direction\":\"sideways\"}").character_menu_direction == Direction::Down);
        CHECK(parse_settings("{\"character_menu_direction\":1}").character_menu_direction == Direction::Down);
        CHECK(parse_settings("{}").character_menu);
        CHECK(parse_settings("{\"character_menu\": 1}").character_menu);
        CHECK(!parse_settings("{\"two_players\": 1}").two_players);

        const Settings defaults = parse_settings("{not json");
        CHECK(!defaults[Cheat::Fly].enabled && defaults[Cheat::Blaster].direction == Direction::Right);
        const Settings partial = parse_settings("{\"fly\": {\"enabled\": true, \"direction\": \"sideways\"}}");
        CHECK(partial[Cheat::Fly].enabled && partial[Cheat::Fly].direction == Direction::Up);
        CHECK(!partial[Cheat::Blaster].enabled && partial[Cheat::Blaster].direction == Direction::Right);
        CHECK(!parse_settings("{\"fly\": 1}")[Cheat::Fly].enabled);
    }

    std::puts("tooie_dev_cheats_test: OK");
    return 0;
}
