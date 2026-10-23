#include "recompinput/recompinput.h"
#include "recompinput/players.h"
#include "recompinput/profiles.h"
#include "recompinput/input_events.h"
#include "recompinput/input_binding.h"
#include <mutex>
#include <algorithm>
#include <cstring>
#include "ultramodern/ultramodern.hpp"
#include "composites/ui_assign_players_modal.h"
#include "config/ui_config_page_controls.h"

namespace recompinput {

using PlayerArray = PlayerSlots<SDL_GameController*>;
static std::recursive_mutex players_mutex;

static struct {
    bool single_player_mode = true;
    bool is_assigning = false;
    size_t min_players = 1;
    size_t max_players = 4;
    std::vector<players::ConnectedController> controllers;
    std::unordered_map<SDL_JoystickID, std::chrono::steady_clock::time_point> activity;
    std::unordered_map<SDL_GameController*, std::chrono::steady_clock::time_point> rumble_tests;
    std::chrono::steady_clock::time_point keyboard_activity{};
    int next_ordinal = 1;
    PlayerArray players;
    PlayerArray temp_players;

    PlayerArray& get_player_array(bool temp) {
        return temp ? temp_players : players;
    }

    bool queue_close_player_assignment_modal = false;
} PlayerState;

size_t players::get_number_of_assigned_players() {
    std::lock_guard lock{ players_mutex };
    return PlayerState.players.get_count();
}

size_t players::get_max_number_of_players() {
    std::lock_guard lock{ players_mutex };
    if (PlayerState.single_player_mode) {
        return 1;
    }
    return PlayerState.max_players;
}

bool players::is_single_player_mode() {
    std::lock_guard lock{ players_mutex };
    return PlayerState.single_player_mode;
}

void players::set_single_player_mode(bool single_player) {
    std::lock_guard lock{ players_mutex };
    if (!single_player && single_player != PlayerState.single_player_mode) {
        purge_deferred_controller_profiles();
    }

    PlayerState.single_player_mode = single_player;
}

void players::set_min_number_of_players(size_t min_players) {
    std::lock_guard lock{ players_mutex };
    assert(min_players >= 1 && min_players <= PlayerState.max_players);
    PlayerState.min_players = min_players;
}

void players::set_max_number_of_players(size_t max_players) {
    std::lock_guard lock{ players_mutex };
    assert(max_players >= PlayerState.min_players && max_players <= recompinput::max_num_players_supported);
    PlayerState.max_players = max_players;
}

void players::set_player_count_range(size_t min_players, size_t max_players) {
    std::lock_guard lock{ players_mutex };
    assert(min_players >= 1 && max_players <= recompinput::max_num_players_supported && min_players <= max_players);
    PlayerState.min_players = min_players;
    PlayerState.max_players = max_players;
}

recompinput::Player players::get_player(int player_index, bool temp_player) {
    std::lock_guard lock{ players_mutex };
    return PlayerArray::valid(player_index) ? PlayerState.get_player_array(temp_player)[player_index] : Player{};
}

bool players::get_player_is_assigned(int player_index, bool temp_player) {
    std::lock_guard lock{ players_mutex };
    return PlayerState.get_player_array(temp_player).player_is_assigned(player_index);
}

bool players::has_enough_players_assigned() {
    std::lock_guard lock{ players_mutex };
    return PlayerState.players.get_count() >= PlayerState.min_players;
}

InputDevice players::get_player_input_device(int player_index, bool temp_player) {
    std::lock_guard lock{ players_mutex };
    auto player_array = PlayerState.get_player_array(temp_player);
    if (!player_array.player_is_assigned(player_index)) {
        return InputDevice::COUNT;
    }

    const auto& assigned_player = player_array[player_index];
    if (assigned_player.controller != nullptr) {
        return InputDevice::Controller;
    } else if (assigned_player.keyboard_enabled) {
        return InputDevice::Keyboard;
    } else {
        return InputDevice::COUNT;
    }
}

// playerassignment start

bool playerassignment::is_active() {
    std::lock_guard lock{ players_mutex };
    return PlayerState.is_assigning;
}

void playerassignment::start(void) {
    std::lock_guard lock{ players_mutex };
    PlayerState.is_assigning = true;
    PlayerState.temp_players = PlayerArray{};
}

void playerassignment::stop(void) {
    std::lock_guard lock{ players_mutex };
    PlayerState.is_assigning = false;
}

void playerassignment::stop_and_close_modal() {
    std::lock_guard lock{ players_mutex };
    playerassignment::stop();
    PlayerState.queue_close_player_assignment_modal = true;
}

void playerassignment::commit_player_assignment() {
    std::lock_guard lock{ players_mutex };
    stop_and_close_modal();

    PlayerState.players = PlayerState.temp_players;
    for (int i = 0; i < max_players; i++) {
        if (PlayerState.players[i].controller) profiles::get_or_create_mp_controller_profile_index(i);
        if (PlayerState.players[i].keyboard_enabled) profiles::get_or_create_mp_keyboard_profile_index(i);
    }
    profiles::save_player_config();
}

void playerassignment::add_keyboard_player() {
    std::lock_guard lock{ players_mutex };
    int slot = PlayerState.temp_players.first_empty(PlayerState.max_players);
    if (slot >= 0 && PlayerState.temp_players.keyboard_player() < 0) {
        PlayerState.temp_players.set_keyboard(slot, true);
        PlayerState.temp_players[slot].button_pressed();
    }
}

bool playerassignment::met_assignment_requirements() {
    std::lock_guard lock{ players_mutex };
    return PlayerState.temp_players.get_count() >= PlayerState.min_players;
}

bool playerassignment::is_blocking_input() {
    std::lock_guard lock{ players_mutex };
    return playerassignment::is_active() && !playerassignment::met_assignment_requirements();
}

bool playerassignment::is_player_currently_assigning(int player_index) {
    std::lock_guard lock{ players_mutex };
    return playerassignment::is_active() && player_index == PlayerState.temp_players.first_empty(PlayerState.max_players);
}

bool playerassignment::was_keyboard_assigned() {
    std::lock_guard lock{ players_mutex };
    size_t num_temp = PlayerState.temp_players.size();
    if (num_temp == 0) {
        return false;
    }

    for (size_t i = 0; i < num_temp; i++) {
        if (PlayerState.temp_players[i].keyboard_enabled) {
            return true;
        }
    }

    return false;
}

std::chrono::steady_clock::duration playerassignment::get_player_time_since_last_button_press(int player_index) {
    std::lock_guard lock{ players_mutex };
    if (!PlayerState.temp_players.player_is_assigned(player_index)) {
        return std::chrono::steady_clock::duration::zero();
    }
    return std::chrono::steady_clock::now() - PlayerState.temp_players[player_index].last_button_press_timestamp;
}

void playerassignment::process_sdl_event(SDL_Event* event) {
    std::lock_guard lock{ players_mutex };
    if (PlayerState.queue_close_player_assignment_modal) {
        recompui::AssignPlayersModal::close();
        PlayerState.queue_close_player_assignment_modal = false;
        if (recompui::controls_page != nullptr) {
            recompui::controls_page->force_update();
        }
        return;
    }

    if (!playerassignment::is_active()) {
        return;
    }

    switch (event->type) {
    case SDL_EventType::SDL_KEYDOWN: {
        SDL_KeyboardEvent* keyevent = &event->key;

        switch (keyevent->keysym.scancode) {
        case SDL_Scancode::SDL_SCANCODE_ESCAPE:
            // TODO: Restore previous assignment?
            playerassignment::stop();
            return;
        case SDL_Scancode::SDL_SCANCODE_RIGHT:
        case SDL_Scancode::SDL_SCANCODE_LEFT:
        case SDL_Scancode::SDL_SCANCODE_UP:
        case SDL_Scancode::SDL_SCANCODE_DOWN:
            break;
        default:
            if (playerassignment::was_keyboard_assigned()) {
                for (size_t i = 0; i < PlayerState.temp_players.size(); i++) {
                    if (PlayerState.temp_players[i].keyboard_enabled && PlayerState.temp_players[i].controller == nullptr) {
                        PlayerState.temp_players[i].button_pressed();
                    }
                }
            } else {
                playerassignment::add_keyboard_player();
            }
            break;
        }
        break;
    }
    case SDL_EventType::SDL_CONTROLLERBUTTONDOWN: {
        SDL_ControllerButtonEvent* button_event = &event->cbutton;
        SDL_JoystickID joystick_id = button_event->which;
        SDL_GameController* controller = nullptr;
        for (const auto& connected : PlayerState.controllers) {
            if (connected.id == joystick_id) controller = connected.controller;
        }

        bool can_be_mapped = true;
        for (size_t i = 0; i < PlayerState.temp_players.size(); i++) {
            if (PlayerState.temp_players[i].controller == controller) {
                can_be_mapped = false;
                PlayerState.temp_players[i].button_pressed();
                break;
            }
        }

        int slot = PlayerState.temp_players.first_empty(PlayerState.max_players);
        if (can_be_mapped && controller != nullptr && slot >= 0) {
            PlayerState.temp_players.set_controller(slot, controller, profiles::get_guid_from_sdl_controller(controller));
            PlayerState.temp_players[slot].button_pressed();
        }

        break;
    }
    }
}
// player assignment end

bool players::set_player_controller(int player, SDL_GameController* controller) {
    std::lock_guard lock{ players_mutex };
    if (controller && std::none_of(PlayerState.controllers.begin(), PlayerState.controllers.end(),
        [controller](const auto& connected) { return connected.controller == controller; })) return false;
    if (!PlayerState.players.set_controller(player, controller,
        controller ? std::optional{profiles::get_guid_from_sdl_controller(controller)} : std::nullopt)) return false;
    if (controller) profiles::get_or_create_mp_controller_profile_index(player);
    profiles::save_player_config();
    return true;
}

void players::set_player_keyboard(int player, bool enabled) {
    std::lock_guard lock{ players_mutex };
    if (!PlayerArray::valid(player)) return;
    PlayerState.players.set_keyboard(player, enabled);
    if (enabled) profiles::get_or_create_mp_keyboard_profile_index(player);
    profiles::save_player_config();
}

int players::get_controller_player(SDL_GameController* controller) {
    std::lock_guard lock{ players_mutex };
    return PlayerState.players.controller_player(controller);
}

int players::get_keyboard_player() {
    std::lock_guard lock{ players_mutex };
    return PlayerState.players.keyboard_player();
}

SDL_GameController* players::get_player_controller(int player) { return get_player(player).controller; }
bool players::get_player_keyboard(int player) { return get_player(player).keyboard_enabled; }

std::vector<players::ConnectedController> players::get_connected_controllers() {
    std::lock_guard lock{ players_mutex };
    return PlayerState.controllers;
}

void players::controller_connected(SDL_GameController* controller, SDL_JoystickID id) {
    std::lock_guard lock{ players_mutex };
    const char* name = SDL_GameControllerName(controller);
    PlayerState.controllers.push_back({controller, id, name ? name : "Unknown controller", PlayerState.next_ordinal++});
    auto identity = profiles::get_guid_from_sdl_controller(controller);
    PlayerState.temp_players.reconnect(controller, identity);
    int slot = PlayerState.players.reconnect(controller, identity);
    if (slot >= 0) {
        profiles::get_or_create_mp_controller_profile_index(slot);
        profiles::save_player_config();
    }
}

void players::controller_disconnected(SDL_GameController* controller) {
    std::lock_guard lock{ players_mutex };
    for (const auto& connected : PlayerState.controllers) {
        if (connected.controller == controller && binding::is_binding() && binding::is_controller_being_bound(connected.id)) {
            binding::stop_scanning();
        }
    }
    PlayerState.players.disconnect(controller);
    PlayerState.temp_players.disconnect(controller);
    PlayerState.rumble_tests.erase(controller);
    std::erase_if(PlayerState.controllers, [controller](const auto& connected) {
        if (connected.controller != controller) return false;
        PlayerState.activity.erase(connected.id);
        return true;
    });
}

void players::record_input_activity(const SDL_Event& event) {
    std::lock_guard lock{ players_mutex };
    auto now = std::chrono::steady_clock::now();
    if (event.type == SDL_KEYDOWN) PlayerState.keyboard_activity = now;
    if (event.type == SDL_CONTROLLERBUTTONDOWN) PlayerState.activity[event.cbutton.which] = now;
    if (event.type == SDL_CONTROLLERAXISMOTION &&
        std::abs(static_cast<int>(event.caxis.value)) > axis_digital_threshold * 32768.0f) {
        PlayerState.activity[event.caxis.which] = now;
    }
}

std::chrono::steady_clock::duration players::get_time_since_controller_input(SDL_JoystickID id) {
    std::lock_guard lock{ players_mutex };
    auto it = PlayerState.activity.find(id);
    return it == PlayerState.activity.end() ? std::chrono::steady_clock::duration::max()
        : std::chrono::steady_clock::now() - it->second;
}

std::chrono::steady_clock::duration players::get_time_since_keyboard_input() {
    std::lock_guard lock{ players_mutex };
    return PlayerState.keyboard_activity == std::chrono::steady_clock::time_point{}
        ? std::chrono::steady_clock::duration::max() : std::chrono::steady_clock::now() - PlayerState.keyboard_activity;
}

bool players::test_rumble(SDL_GameController* controller, uint32_t ms, RumbleTestResult* result) {
    std::lock_guard lock{ players_mutex };
    if (result) *result = RumbleTestResult::Failed;
    if (!controller || std::none_of(PlayerState.controllers.begin(), PlayerState.controllers.end(),
        [controller](const auto& connected) { return connected.controller == controller; })) return false;
#if SDL_VERSION_ATLEAST(2, 0, 18)
    if (!SDL_GameControllerHasRumble(controller)) {
        if (result) *result = RumbleTestResult::NoRumble;
        return false;
    }
#endif
    SDL_ClearError();
    if (SDL_GameControllerRumble(controller, 0xFFFF, 0xFFFF, ms) != 0) {
        if (result) *result = std::strstr(SDL_GetError(), "correlated") ? RumbleTestResult::NeedsInput : RumbleTestResult::Failed;
        return false;
    }
    PlayerState.rumble_tests[controller] = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    if (result) *result = RumbleTestResult::Ok;
    return true;
}

bool players::get_controller_has_rumble(SDL_GameController* controller) {
    std::lock_guard lock{ players_mutex };
    if (!controller || std::none_of(PlayerState.controllers.begin(), PlayerState.controllers.end(),
        [controller](const auto& connected) { return connected.controller == controller; })) return false;
#if SDL_VERSION_ATLEAST(2, 0, 18)
    return SDL_GameControllerHasRumble(controller) == SDL_TRUE;
#else
    return false;
#endif
}

bool players::is_rumble_test_active(SDL_GameController* controller) {
    std::lock_guard lock{ players_mutex };
    auto it = PlayerState.rumble_tests.find(controller);
    return it != PlayerState.rumble_tests.end() && std::chrono::steady_clock::now() < it->second;
}

int players::apply_game_rumble(SDL_GameController* controller, uint16_t strength, uint32_t duration) {
    std::lock_guard lock{ players_mutex };
    // Keep the test-window check and SDL write atomic with respect to test_rumble.
    if (is_rumble_test_active(controller)) return 0;
    return SDL_JoystickRumble(SDL_GameControllerGetJoystick(controller), 0, strength, duration);
}

nlohmann::json players::save_slots() {
    std::lock_guard lock{ players_mutex };
    auto slots = nlohmann::json::array();
    for (const auto& player : PlayerState.players) {
        slots.push_back({{"controller", player.identity ? nlohmann::json(*player.identity) : nlohmann::json(nullptr)},
            {"keyboard", player.keyboard_enabled}});
    }
    return slots;
}

void players::load_slots(const nlohmann::json& slots) {
    std::lock_guard lock{ players_mutex };
    PlayerState.players = {};
    if (!slots.is_array()) return;
    for (int i = 0; i < std::min(static_cast<int>(slots.size()), max_players); i++) {
        if (!slots[i].is_object()) continue;
        PlayerState.players.set_keyboard(i, slots[i].value("keyboard", false));
        if (slots[i].contains("controller") && slots[i]["controller"].is_object()) {
            PlayerState.players[i].identity = slots[i]["controller"].get<ControllerGUID>();
        }
    }
    for (const auto& connected : PlayerState.controllers) {
        PlayerState.players.reconnect(connected.controller, profiles::get_guid_from_sdl_controller(connected.controller));
    }
    for (int i = 0; i < max_players; i++) {
        if (PlayerState.players[i].controller) profiles::get_or_create_mp_controller_profile_index(i);
        if (PlayerState.players[i].keyboard_enabled) profiles::get_or_create_mp_keyboard_profile_index(i);
    }
}

}
