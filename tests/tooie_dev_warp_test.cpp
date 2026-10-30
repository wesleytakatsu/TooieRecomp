#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "tooie_dev_warp.h"

using namespace tooie_dev_warp;

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); std::exit(1); } } while (0)

static GameState frame(int16_t map, uint32_t player, bool gameplay = true) {
    GameState state;
    state.gameplay = gameplay;
    state.player = player;
    state.position.map = map;
    return state;
}

static Position target(int16_t map) {
    Position position;
    position.map = map;
    position.entrance = 3;
    position.pos[0] = 10.0f;
    position.pos[1] = -20.5f;
    position.pos[2] = 30.25f;
    position.yaw = 270.0f;
    return position;
}

int main() {
    const double frame_time = 1.0 / 30.0;

    // Idle: nothing to do.
    {
        Teleporter t;
        CHECK(!t.busy());
        CHECK(t.step(frame(0xC7, 0x80200000u), 0.0) == Action::None);
    }

    // Same map: applies on the first gameplay frame.
    {
        Teleporter t;
        t.request(target(0xC7), 0.0);
        CHECK(t.busy());
        CHECK(t.step(frame(0xC7, 0x80200000u, false), frame_time) == Action::None); // not in gameplay yet
        CHECK(t.step(frame(0xC7, 0u), 2 * frame_time) == Action::None);            // no player
        CHECK(t.step(frame(0xC7, 0x80200000u), 3 * frame_time) == Action::Apply);
        CHECK(!t.busy());
        CHECK(t.target().entrance == 3 && t.target().yaw == 270.0f);
    }

    // Other map: Goto, then wait for the load and kSettleSeconds in the target map before applying.
    {
        Teleporter t;
        double now = 0.0;
        t.request(target(0xC7), now);
        CHECK(t.step(frame(0xAF, 0x80200000u), now += frame_time) == Action::Goto);
        // The old map keeps running; even a target map number without a load sign must not apply.
        CHECK(t.step(frame(0xC7, 0x80200000u), now += frame_time) == Action::None);
        CHECK(t.step(frame(0xC7, 0x80200000u), now += frame_time) == Action::None);
        // Loading: no player frames for a while, then a new player in the target map.
        now += 1.0;
        const double arrived = now + frame_time;
        Action last = Action::None;
        while (last == Action::None && now < 20.0) {
            last = t.step(frame(0xC7, 0x80210000u), now += frame_time);
        }

        CHECK(last == Action::Apply);
        CHECK(now - arrived >= Teleporter::kSettleSeconds && now - arrived < Teleporter::kSettleSeconds + 2 * frame_time);
        CHECK(!t.busy());
    }

    // A hitch while the game has not taken the request yet (old map fading out) is not a load.
    {
        Teleporter t;
        double now = 0.0;
        t.request(target(0xC7), now);
        CHECK(t.step(frame(0xAF, 0x80200000u), now += frame_time) == Action::Goto);
        GameState pending = frame(0xC7, 0x80200000u);
        pending.warp_pending = true;
        now += 1.0;
        for (int i = 0; i < 100; i++) {
            CHECK(t.step(pending, now += frame_time) == Action::None);
        }

        // The game takes the request, then loads the new map.
        CHECK(t.step(frame(0xC7, 0x80200000u), now += frame_time) == Action::None);
        now += 1.0;
        Action last = Action::None;
        while (last == Action::None && now < 20.0) {
            last = t.step(frame(0xC7, 0x80210000u), now += frame_time);
        }

        CHECK(last == Action::Apply);
    }

    // A new player pointer alone is a load sign.
    {
        Teleporter t;
        double now = 0.0;
        t.request(target(0xC7), now);
        CHECK(t.step(frame(0xAF, 0x80200000u), now += frame_time) == Action::Goto);
        Action last = Action::None;
        while (last == Action::None && now < Teleporter::kSettleSeconds + 1.0) {
            last = t.step(frame(0xC7, 0x80210000u), now += frame_time);
        }

        CHECK(last == Action::Apply);
    }

    // A map that never loads times out.
    {
        Teleporter t;
        double now = 0.0;
        t.request(target(0xC7), now);
        CHECK(t.step(frame(0xAF, 0x80200000u), now += frame_time) == Action::Goto);
        Action last = Action::None;
        while (last == Action::None && now < 60.0) {
            last = t.step(frame(0xAF, 0x80200000u), now += frame_time);
        }

        CHECK(last == Action::GiveUp);
        CHECK(now > Teleporter::kTimeoutSeconds && now < Teleporter::kTimeoutSeconds + 1.0);
        CHECK(!t.busy());
    }

    // A new request replaces a pending one.
    {
        Teleporter t;
        t.request(target(0xC7), 0.0);
        t.request(target(0xAF), 0.1);
        CHECK(t.step(frame(0xAF, 0x80200000u), 0.2) == Action::Apply);
    }

    // JSON round trip.
    {
        std::vector<Entry> entries;
        entries.push_back({"Mine lag spot", target(0xC7)});
        entries.push_back({"Ação & \"aspas\"", target(0x14F)});
        const std::vector<Entry> back = parse_entries(serialize_entries(entries));
        CHECK(back.size() == 2);
        CHECK(back[0].name == "Mine lag spot");
        CHECK(back[1].name == "Ação & \"aspas\"");
        CHECK(back[1].position.map == 0x14F);
        CHECK(back[0].position.entrance == 3);
        CHECK(back[0].position.pos[1] == -20.5f && back[0].position.pos[2] == 30.25f);
        CHECK(back[0].position.yaw == 270.0f);
    }

    // Malformed input gives no entries, and entries without a map are skipped.
    {
        CHECK(parse_entries("").empty());
        CHECK(parse_entries("{not json").empty());
        CHECK(parse_entries("[]").empty());
        CHECK(parse_entries("{\"positions\": 3}").empty());
        const std::vector<Entry> some = parse_entries(
            "{\"positions\": [{\"name\": \"no map\"}, {\"name\": \"ok\", \"map\": 199}]}");
        CHECK(some.size() == 1 && some[0].name == "ok" && some[0].position.map == 199);
        CHECK(some[0].position.entrance == 0 && some[0].position.yaw == 0.0f);
    }

    // Map names come from the decompilation's map list.
    {
        CHECK(map_name(0xC7) != nullptr && std::strcmp(map_name(0xC7), "GGM Glitter Gulch Mine") == 0);
        CHECK(map_name(0x7FFF) == nullptr);
        CHECK(describe_map(0xC7) == "GGM Glitter Gulch Mine (199)");
        CHECK(describe_map(0x7FFF) == "Map 32767");
    }

    std::puts("tooie_dev_warp_test: OK");
    return 0;
}
