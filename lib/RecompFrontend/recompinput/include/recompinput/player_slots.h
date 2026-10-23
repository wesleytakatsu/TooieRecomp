#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace recompinput {
    constexpr int max_players = 4;

    struct ControllerIdentity {
        uint64_t hash = 0;
        std::string serial;
        int vendor = 0;
        int product = 0;
        int version = 0;
        int crc16 = 0;
    };

    template <typename Handle> struct PlayerSlot {
        Handle controller{};
        bool keyboard_enabled = false;
        std::optional<ControllerIdentity> identity;
        std::chrono::steady_clock::time_point last_button_press_timestamp{};

        void button_pressed() {
            last_button_press_timestamp = std::chrono::steady_clock::now();
        }
    };

    // Handles are opaque. Identity survives disconnect; explicit clearing forgets it.
    template <typename Handle> class PlayerSlots : public std::array<PlayerSlot<Handle>, max_players> {
    public:
        static bool valid(int player) { return player >= 0 && player < max_players; }

        bool player_is_assigned(int player) const {
            return valid(player) && ((*this)[player].controller != Handle{} || (*this)[player].keyboard_enabled);
        }

        size_t get_count() const {
            size_t count = 0;
            for (int i = 0; i < max_players; i++) count += player_is_assigned(i);
            return count;
        }

        int first_empty(size_t limit = max_players) const {
            for (int i = 0; i < static_cast<int>(limit); i++) {
                if (!player_is_assigned(i)) return i;
            }
            return -1;
        }

        int controller_player(Handle controller) const {
            if (controller != Handle{}) {
                for (int i = 0; i < max_players; i++) {
                    if ((*this)[i].controller == controller) return i;
                }
            }
            return -1;
        }

        int keyboard_player() const {
            for (int i = 0; i < max_players; i++) {
                if ((*this)[i].keyboard_enabled) return i;
            }
            return -1;
        }

        bool set_controller(int player, Handle controller, std::optional<ControllerIdentity> identity = {}) {
            if (!valid(player)) return false;
            int other = controller_player(controller);
            if (other == player) return true;
            auto& slot = (*this)[player];
            if (other >= 0) {
                std::swap(slot.controller, (*this)[other].controller);
                std::swap(slot.identity, (*this)[other].identity);
            } else {
                slot.controller = controller;
                slot.identity = controller == Handle{} ? std::nullopt : identity;
            }
            return true;
        }

        void set_keyboard(int player, bool enabled) {
            if (!valid(player)) return;
            if (enabled) {
                for (auto& slot : *this) slot.keyboard_enabled = false;
            }
            (*this)[player].keyboard_enabled = enabled;
        }

        void disconnect(Handle controller) {
            int player = controller_player(controller);
            if (player >= 0) (*this)[player].controller = Handle{};
        }

        int reconnect(Handle controller, const ControllerIdentity& identity) {
            if (controller == Handle{} || controller_player(controller) >= 0) return -1;
            // Never substitute a different serialized device for a known serial.
            for (int pass = 0; pass < 2; pass++) {
                for (int i = 0; i < max_players; i++) {
                    auto& slot = (*this)[i];
                    if (slot.controller != Handle{} || !slot.identity) continue;
                    const auto& remembered = *slot.identity;
                    bool matches = pass == 0
                        ? !identity.serial.empty() && remembered.serial == identity.serial
                        : (remembered.serial.empty() || identity.serial.empty()) && remembered.hash == identity.hash;
                    if (matches) {
                        slot.controller = controller;
                        slot.identity = identity;
                        return i;
                    }
                }
            }
            return -1;
        }
    };
}
