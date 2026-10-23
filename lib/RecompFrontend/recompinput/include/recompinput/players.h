#ifndef __RECOMP_INPUT_PLAYER_ASSIGNMENT_H__
#define __RECOMP_INPUT_PLAYER_ASSIGNMENT_H__

#include "player_slots.h"
#include "recompinput.h"
#include <vector>
#include "json/json.hpp"

namespace recompinput {
    using Player = PlayerSlot<SDL_GameController*>;

    namespace players {
        enum class RumbleTestResult { Ok, NoRumble, NeedsInput, Failed };

        struct ConnectedController {
            SDL_GameController* controller;
            SDL_JoystickID id;
            std::string name;
            int ordinal;
        };
        bool set_player_controller(int player, SDL_GameController* controller);
        void set_player_keyboard(int player, bool enabled);
        int get_controller_player(SDL_GameController* controller);
        int get_keyboard_player();
        SDL_GameController* get_player_controller(int player);
        bool get_player_keyboard(int player);
        std::vector<ConnectedController> get_connected_controllers();
        std::chrono::steady_clock::duration get_time_since_controller_input(SDL_JoystickID id);
        std::chrono::steady_clock::duration get_time_since_keyboard_input();
        bool test_rumble(SDL_GameController* controller, uint32_t ms = 1000, RumbleTestResult* result = nullptr);
        bool get_controller_has_rumble(SDL_GameController* controller);
        bool is_rumble_test_active(SDL_GameController* controller);
        int apply_game_rumble(SDL_GameController* controller, uint16_t strength, uint32_t duration);
        void controller_connected(SDL_GameController* controller, SDL_JoystickID id);
        void controller_disconnected(SDL_GameController* controller);
        void record_input_activity(const SDL_Event& event);
        nlohmann::json save_slots();
        void load_slots(const nlohmann::json& slots);
        size_t get_number_of_assigned_players();
        size_t get_max_number_of_players();
        bool is_single_player_mode();
        void set_single_player_mode(bool single_player);
        void set_min_number_of_players(size_t min_players);
        void set_max_number_of_players(size_t max_players);
        void set_player_count_range(size_t min_players, size_t max_players);
        Player get_player(int player_index, bool temp_player = false);
        bool get_player_is_assigned(int player_index, bool temp_player = false);
        bool has_enough_players_assigned();
        InputDevice get_player_input_device(int player_index, bool temp_player = false);
    }

    namespace playerassignment {
        bool is_active();
        void start();
        void stop();
        void stop_and_close_modal();
        void commit_player_assignment();
        void add_keyboard_player();
        bool met_assignment_requirements();
        bool is_blocking_input();
        bool is_player_currently_assigning(int player_index);
        bool was_keyboard_assigned();
        std::chrono::steady_clock::duration get_player_time_since_last_button_press(int player_index);

        void process_sdl_event(SDL_Event* event);
    }
}

#endif // __RECOMP_INPUT_PLAYER_ASSIGNMENT_H__
