#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <optional>

namespace tooie_dev_forms {
constexpr int kPlayers = 2;
struct Form { uint8_t id; const char* name; bool dragon = false; };
inline constexpr std::array<Form, 17> kForms{{
    {1, "Banjo & Kazooie"}, {1, "Banjo & Dragon Kazooie", true}, {0xA, "Banjo"}, {0xB, "Kazooie"},
    {0xB, "Dragon Kazooie", true}, {0xD, "Mumbo"}, {2, "Snowball"},
    {6, "Bee"}, {7, "Washing machine"}, {8, "Stony"},
    {9, "Breegull Blaster"}, {0xC, "Submarine"},
    {0xE, "Golden Goliath"}, {0xF, "Detonator"}, {0x10, "Van"},
    {0x12, "Small T. rex"}, {0x13, "Large T. rex"}
}};
struct Rect { float x = 0, y = 0, width = 1, height = 1; };
struct Menu {
    bool available = false, open = false;
    int selected = 0;
    Rect rect;
};
// Host-only state machine. Input and game publication share a lock; render receives a value copy.
class Menus {
public:
    void reset(bool rearm = false);
    void cancel(int player);
    void request_open(int player);
    void publish(int player, bool available, Rect rect, uint8_t form, bool dragon);
    bool input(int player, bool unlocked, bool debug, uint16_t buttons, float x, float y, double time,
        uint16_t open_button = 0x0400);
    std::optional<int> take(int player);
    std::array<Menu, kPlayers> snapshot();
private:
    struct State {
        Menu menu;
        uint16_t previous = 0;
        bool drain = false;
        bool rearm = false;
        bool open_requested = false;
        double repeat_at = 0;
        uint16_t repeat_direction = 0;
        std::optional<int> request;
    };
    std::mutex mutex;
    std::array<State, kPlayers> players{};
};
Menus& menus();
// Present-thread callback, composed with the existing FPS overlay.
bool update_overlay(bool shutting_down);
}
