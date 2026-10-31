#include <cstdio>
#include <cstdlib>
#include "tooie_dev_cheats.h"

using namespace tooie_dev_cheats;

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); std::exit(1); } } while (0)

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
    }

    // A request needs the unlock, the cheat enabled and its own direction, and is taken once.
    {
        set_settings(Settings{});
        on_player1_input(true, 0);
        on_player1_input(true, up);
        CHECK(!take_request(Cheat::Fly)); // locked and disabled

        set_unlocked(true);
        on_player1_input(true, 0);
        on_player1_input(true, up);
        CHECK(!take_request(Cheat::Fly)); // disabled

        Settings enabled;
        enabled[Cheat::Fly] = {true, Direction::Left};
        enabled[Cheat::Blaster] = {true, Direction::Right};
        set_settings(enabled);
        on_player1_input(true, 0);
        on_player1_input(true, up);
        CHECK(!take_request(Cheat::Fly) && !take_request(Cheat::Blaster)); // no cheat on up
        on_player1_input(true, 0);
        on_player1_input(true, left);
        CHECK(take_request(Cheat::Fly) && !take_request(Cheat::Blaster));
        CHECK(!take_request(Cheat::Fly));
        on_player1_input(true, 0);
        on_player1_input(true, right);
        CHECK(take_request(Cheat::Blaster) && !take_request(Cheat::Fly));

        set_unlocked(false);
        on_player1_input(true, 0);
        on_player1_input(true, left);
        CHECK(!take_request(Cheat::Fly));
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
        const Settings back = parse_settings(serialize_settings(settings));
        CHECK(back[Cheat::Fly].enabled && back[Cheat::Fly].direction == Direction::Down);
        CHECK(back[Cheat::Blaster].enabled && back[Cheat::Blaster].direction == Direction::Left);

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
