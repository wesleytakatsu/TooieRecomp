#include <cstdio>
#include <cstdlib>
#include "tooie_dev_cheats.h"

using namespace tooie_dev_cheats;

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); std::exit(1); } } while (0)

int main() {
    const uint16_t up = direction_button(Direction::Up);
    const uint16_t left = direction_button(Direction::Left);
    const uint16_t b_button = 0x4000;

    // Directions map to the N64 D-pad bits and round-trip through their names.
    {
        CHECK(up == 0x0800 && direction_button(Direction::Down) == 0x0400);
        CHECK(left == 0x0200 && direction_button(Direction::Right) == 0x0100);
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

    // The fly request needs the unlock, the cheat enabled and its own direction, and is taken once.
    {
        set_settings(Settings{});
        on_player1_input(true, 0);
        on_player1_input(true, up);
        CHECK(!take_fly_request()); // locked and disabled

        set_unlocked(true);
        on_player1_input(true, 0);
        on_player1_input(true, up);
        CHECK(!take_fly_request()); // disabled

        Settings enabled;
        enabled.fly_enabled = true;
        enabled.fly_direction = Direction::Left;
        set_settings(enabled);
        on_player1_input(true, 0);
        on_player1_input(true, up);
        CHECK(!take_fly_request()); // other direction
        on_player1_input(true, 0);
        on_player1_input(true, left);
        CHECK(take_fly_request());
        CHECK(!take_fly_request());

        set_unlocked(false);
        on_player1_input(true, 0);
        on_player1_input(true, left);
        CHECK(!take_fly_request());
    }

    // Only the ground states seen before a flight pad takeoff may launch.
    {
        CHECK(can_launch_flight(kStateIdle) && can_launch_flight(0x2) && can_launch_flight(0x4));
        CHECK(!can_launch_flight(0x0) && !can_launch_flight(kStateFlyLaunch) && !can_launch_flight(kStateFlying));
        CHECK(!can_launch_flight(0x2F));
    }

    // Settings round-trip, and bad input falls back to the defaults.
    {
        Settings settings;
        settings.fly_enabled = true;
        settings.fly_direction = Direction::Right;
        const Settings back = parse_settings(serialize_settings(settings));
        CHECK(back.fly_enabled && back.fly_direction == Direction::Right);

        const Settings defaults = parse_settings("{not json");
        CHECK(!defaults.fly_enabled && defaults.fly_direction == Direction::Up);
        const Settings partial = parse_settings("{\"fly\": {\"enabled\": true, \"direction\": \"sideways\"}}");
        CHECK(partial.fly_enabled && partial.fly_direction == Direction::Up);
        CHECK(!parse_settings("{\"fly\": 1}").fly_enabled);
    }

    std::puts("tooie_dev_cheats_test: OK");
    return 0;
}
