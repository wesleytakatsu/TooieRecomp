#include "tooie_dev_forms.h"
#include <algorithm>
#include <cmath>

namespace tooie_dev_forms {
Menus& menus() { static Menus instance; return instance; }
void Menus::reset() {
    std::lock_guard lock(mutex);
    players = {};
}
void Menus::request_open(int player) {
    if (player < 0 || player >= kPlayers) return;
    std::lock_guard lock(mutex);
    players[player].open_requested = true;
    players[player].drain = true; // Do not reuse the A/Enter press that closed the ESC menu.
}
void Menus::publish(int player, bool available, Rect rect, uint8_t form, bool dragon) {
    if (player < 0 || player >= kPlayers) return;
    std::lock_guard lock(mutex);
    auto& state = players[player];
    state.menu.available = available;
    state.menu.rect = rect;
    if (!available) {
        state.menu.open = false;
        state.request.reset();
    }
    if (!state.menu.open) {
        dragon = dragon && (form == 1 || form == 0xB);
        for (size_t i = 0; i < kForms.size(); ++i) {
            if (kForms[i].id == form && kForms[i].dragon == dragon) {
                state.menu.selected = int(i);
                break;
            }
        }
    }
    if (available && state.open_requested) {
        state.menu.open = true;
        state.open_requested = false;
    }
}
bool Menus::input(int player, bool unlocked, bool debug, uint16_t buttons, float x, float y, double time) {
    if (player < 0 || player >= kPlayers) return false;
    std::lock_guard lock(mutex);
    auto& state = players[player];
    const uint16_t pressed = buttons & ~state.previous;
    state.previous = buttons;
    if (!unlocked || !state.menu.available) {
        state.menu.open = false;
        state.request.reset();
    }
    if (state.drain) {
        if (buttons == 0 && std::abs(x) < 0.25f && std::abs(y) < 0.25f) state.drain = false;
        return true;
    }
    if (!unlocked || !state.menu.available) return false;
    if (!state.menu.open) {
        if (!(debug && (pressed & 0x0400))) return false;
        state.menu.open = true;
        state.repeat_direction = 0x0400;
        state.repeat_at = time + 0.45;
        return true;
    }
    if (pressed & 0x4000) { // B: cancel
        state.menu.open = false;
        state.drain = true;
    } else if (pressed & 0x8000) { // A: confirm
        state.request = state.menu.selected;
        state.menu.open = false;
        state.drain = true;
    } else {
        const uint16_t direction = (buttons & 0x0800) || y > 0.6f ? 0x0800 :
            (buttons & 0x0400) || y < -0.6f ? 0x0400 : 0;
        if (direction == 0) state.repeat_direction = 0;
        else if (direction != state.repeat_direction || time >= state.repeat_at) {
            const bool first = direction != state.repeat_direction;
            state.menu.selected = (state.menu.selected + (direction == 0x0800 ? -1 : 1) + int(kForms.size())) % int(kForms.size());
            state.repeat_at = time + (first ? 0.45 : 0.12);
            state.repeat_direction = direction;
        }
    }
    return true;
}
std::optional<int> Menus::take(int player) {
    if (player < 0 || player >= kPlayers) return {};
    std::lock_guard lock(mutex);
    auto value = players[player].request;
    players[player].request.reset();
    return value;
}
std::array<Menu, kPlayers> Menus::snapshot() {
    std::lock_guard lock(mutex);
    return {players[0].menu, players[1].menu};
}
}
