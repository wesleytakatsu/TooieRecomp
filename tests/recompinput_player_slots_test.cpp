#include <cstdio>
#include <cstdlib>
#include "recompinput/player_slots.h"
#include "recompinput/profiles.h"
#include "recompinput/input_binding.h"
#include "recompui/config.h"
#include "composites/ui_assign_players_modal.h"
#include "config/ui_config_page_controls.h"

// Keep UI and filesystem effects outside this host test. The actual slot,
// profile, binding and SDL state implementations are linked from recompinput.
static nlohmann::json saved_config;
static std::filesystem::path saved_path;
static int saves = 0;
static bool capturing_input = false;
static double rumble_strength = 100.0;
namespace recompinput {
    void purge_deferred_controller_profiles() {}
    bool read_json_with_backups(const std::filesystem::path&, nlohmann::json& out) {
        if (saved_config.is_null()) return false;
        out = saved_config;
        return true;
    }
    bool save_json_with_backups(const std::filesystem::path& path, const nlohmann::json& data) {
        saved_path = path;
        saved_config = data;
        saves++;
        return true;
    }
}
namespace recompui {
    ConfigPageControls* controls_page = nullptr;
    void ConfigPageControls::force_update() {}
    void AssignPlayersModal::close() {}
    bool is_context_capturing_input() { return capturing_input; }
    namespace config::general {
        bool has_rumble_strength_option() { return true; }
        double get_rumble_strength() { return rumble_strength; }
        double get_gyro_sensitivity() { return 100.0; }
        double get_mouse_sensitivity() { return 100.0; }
        double get_joystick_deadzone() { return 0.0; }
    }
}

using namespace recompinput;

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); std::exit(1); } } while (0)

static void assignment_tests() {
    PlayerSlots<int> slots{};
    CHECK(slots.get_count() == 0);
    CHECK(slots.first_empty() == 0);
    CHECK(!slots.set_controller(-1, 1));
    CHECK(!slots.set_controller(4, 1));
    slots.set_keyboard(-1, true);
    CHECK(slots.keyboard_player() == -1);
    CHECK(slots.controller_player(0) == -1);

    CHECK(slots.set_controller(1, 11, ControllerIdentity{10, "one"}));
    slots.set_keyboard(1, true);
    CHECK(!slots.player_is_assigned(0));
    CHECK(slots.player_is_assigned(1));
    CHECK(slots.get_count() == 1);
    CHECK(slots.first_empty() == 0);
    slots.set_controller(3, 22, ControllerIdentity{10, "two"});
    slots.set_controller(1, 22);
    CHECK(slots[1].controller == 22 && slots[3].controller == 11);
    CHECK(slots[1].identity->serial == "two" && slots[3].identity->serial == "one");
    CHECK(slots[1].keyboard_enabled && !slots[3].keyboard_enabled);
    slots.set_controller(0, 22);
    CHECK(slots[0].controller == 22 && slots[1].controller == 0);
    CHECK(slots[1].keyboard_enabled);
    slots.set_keyboard(3, true);
    CHECK(slots.keyboard_player() == 3 && !slots[1].keyboard_enabled);
    CHECK(slots.get_count() == 2);
    slots.set_controller(3, 0);
    CHECK(!slots[3].identity && slots.player_is_assigned(3));
    slots.set_keyboard(3, false);
    CHECK(!slots.player_is_assigned(3));

    // Every assignment mask, including gaps, counts devices only once per slot.
    for (unsigned mask = 0; mask < 16; mask++) {
        PlayerSlots<int> fixture{};
        size_t expected = 0;
        for (int i = 0; i < max_players; i++) {
            if (mask & (1u << i)) {
                fixture.set_controller(i, i + 1);
                expected++;
            }
        }
        CHECK(fixture.get_count() == expected);
        for (int i = 0; i < max_players; i++) CHECK(fixture.player_is_assigned(i) == bool(mask & (1u << i)));
        if (mask == 15) CHECK(fixture.first_empty() == -1);
    }
}

static void reconnect_tests() {
    PlayerSlots<int> slots{};
    slots.set_controller(0, 1, ControllerIdentity{7, ""});
    slots.set_controller(2, 2, ControllerIdentity{7, "serial"});
    slots.set_keyboard(2, true);
    slots.disconnect(1);
    slots.disconnect(2);
    CHECK(slots.get_count() == 1 && slots[2].controller == 0);
    CHECK(slots[2].identity->serial == "serial");
    CHECK(slots.reconnect(3, ControllerIdentity{7, "serial"}) == 2);
    CHECK(slots.reconnect(4, ControllerIdentity{7, ""}) == 0);
    CHECK(slots.reconnect(4, ControllerIdentity{7, ""}) == -1);
    CHECK(slots.reconnect(0, ControllerIdentity{7, ""}) == -1);
    slots.disconnect(3);
    CHECK(slots.reconnect(5, ControllerIdentity{7, "different"}) == -1);
    CHECK(slots.reconnect(5, ControllerIdentity{8, ""}) == -1);
    slots.set_controller(2, 0);
    CHECK(slots.reconnect(5, ControllerIdentity{7, "serial"}) == -1);

    // Identical serial-less devices reconnect in connection order into slot order.
    slots = {};
    slots[1].identity = ControllerIdentity{9, ""};
    slots[3].identity = ControllerIdentity{9, ""};
    CHECK(slots.reconnect(50, ControllerIdentity{9, ""}) == 1);
    CHECK(slots.reconnect(60, ControllerIdentity{9, ""}) == 3);
    CHECK(slots.reconnect(70, ControllerIdentity{9, ""}) == -1);
}

static int rumble_calls = 0;
static Uint16 rumble_low = 0;
static Uint16 rumble_high = 0;

static int SDLCALL virtual_rumble(void*, Uint16 low, Uint16 high) {
    rumble_calls++;
    rumble_low = low;
    rumble_high = high;
    return 0;
}

static SDL_GameController* connect_virtual(bool rumble = true) {
    SDL_VirtualJoystickDesc desc{};
    desc.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    desc.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    desc.naxes = SDL_CONTROLLER_AXIS_MAX;
    desc.nbuttons = SDL_CONTROLLER_BUTTON_MAX;
    desc.name = "MP2 identical virtual controller";
    desc.Rumble = rumble ? virtual_rumble : nullptr;
    int index = SDL_JoystickAttachVirtualEx(&desc);
    CHECK(index >= 0);
    auto* controller = SDL_GameControllerOpen(index);
    CHECK(controller);
    auto id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
    add_controller_state(id, controller);
    players::controller_connected(controller, id);
    return controller;
}

static void press(SDL_GameController* controller, SDL_GameControllerButton button) {
    CHECK(SDL_JoystickSetVirtualButton(SDL_GameControllerGetJoystick(controller), button, 1) == 0);
    SDL_JoystickUpdate();
    poll_inputs();
}

static uint16_t buttons(int player, bool merged = false) {
    uint16_t result = 0;
    float x = 0, y = 0;
    CHECK(merged ? profiles::get_merged_n64_input(&result, &x, &y)
        : profiles::get_n64_input(player, &result, &x, &y));
    return result;
}

static void frontend_tests() {
    CHECK(SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) == 0);
    CHECK(players::is_single_player_mode());
    CHECK(!profiles::load_controls_config("mp2-controls.json"));
    int sp_controller = profiles::get_sp_controller_profile_index();
    int sp_keyboard = profiles::get_sp_keyboard_profile_index();
    CHECK(profiles::get_input_profile_for_player(1, InputDevice::Keyboard) == -1);
    profiles::set_input_binding(sp_controller, GameInput::A, 0, InputField::controller_digital(SDL_CONTROLLER_BUTTON_Y));
    profiles::set_input_binding(sp_keyboard, GameInput::A, 0, InputField::keyboard(SDL_SCANCODE_F));
    auto* first = connect_virtual();
    auto* second = connect_virtual(false);
    CHECK(profiles::get_guid_from_sdl_controller(first).hash == profiles::get_guid_from_sdl_controller(second).hash);
    press(first, SDL_CONTROLLER_BUTTON_Y);
    CHECK(buttons(0) & 0x8000);
    uint16_t sp_buttons = buttons(0);
    CHECK(SDL_JoystickSetVirtualAxis(SDL_GameControllerGetJoystick(first), SDL_CONTROLLER_AXIS_LEFTX, 20000) == 0);
    SDL_JoystickUpdate();
    float sp_x = 0, sp_y = 0;
    CHECK(profiles::get_n64_input(0, &sp_buttons, &sp_x, &sp_y));
    CHECK(sp_x > 0.5f);
    players::set_single_player_mode(false);
    CHECK(buttons(0, true) == sp_buttons);
    float merged_x = 0, merged_y = 0;
    uint16_t merged_buttons = 0;
    CHECK(profiles::get_merged_n64_input(&merged_buttons, &merged_x, &merged_y));
    CHECK(merged_buttons == sp_buttons && merged_x == sp_x && merged_y == sp_y);
    CHECK(buttons(0) == 0 && buttons(1) == 0 && buttons(2) == 0 && buttons(3) == 0);

    int before = saves;
    CHECK(players::set_player_controller(0, first));
    CHECK(players::set_player_controller(2, second));
    players::set_player_keyboard(0, true);
    CHECK(saves == before + 3 && saved_path == "mp2-controls.json");
    CHECK(players::get_number_of_assigned_players() == 2);
    CHECK(!players::get_player_is_assigned(1) && players::get_player_is_assigned(2));
    int p1_controller = profiles::get_input_profile_for_player(0, InputDevice::Controller);
    int p1_keyboard = profiles::get_input_profile_for_player(0, InputDevice::Keyboard);
    int p3_controller = profiles::get_input_profile_for_player(2, InputDevice::Controller);
    int p3_keyboard = profiles::get_input_profile_for_player(2, InputDevice::Keyboard);
    CHECK(p1_controller != p3_controller && p1_keyboard != p3_keyboard);
    CHECK(profiles::get_input_profile_key(p1_controller) == "mp_player_1_controller");
    CHECK(profiles::get_input_profile_key(p3_keyboard) == "mp_player_3_keyboard");
    CHECK(profiles::get_input_binding(p1_controller, GameInput::A, 0) == profiles::get_input_binding(sp_controller, GameInput::A, 0));
    CHECK(profiles::get_input_binding(p1_keyboard, GameInput::A, 0) == profiles::get_input_binding(sp_keyboard, GameInput::A, 0));
    CHECK(!profiles::get_input_binding(p3_keyboard, GameInput::A, 0).is_empty());
    CHECK(profiles::get_input_binding(p3_keyboard, GameInput::A, 0).input_id == SDL_SCANCODE_SPACE);
    CHECK(buttons(0) & 0x8000);
    CHECK(!(buttons(2) & 0x8000));

    // Route a deterministic virtual button through the keyboard profile to prove
    // that profile reads follow keyboard ownership, independently of defaults.
    profiles::set_input_binding(p3_keyboard, GameInput::A, 0, InputField::controller_digital(SDL_CONTROLLER_BUTTON_Y));
    press(second, SDL_CONTROLLER_BUTTON_Y);
    CHECK(!(buttons(2) & 0x8000));
    players::set_player_keyboard(2, true);
    CHECK(buttons(2) & 0x8000);
    CHECK(!players::get_player_keyboard(0));
    profiles::reset_profile_bindings(p3_keyboard, InputDevice::Keyboard);
    CHECK(profiles::get_input_binding(p3_keyboard, GameInput::A, 0).input_id == SDL_SCANCODE_SPACE);
    CHECK(profiles::get_input_binding(p1_keyboard, GameInput::A, 0).input_id == SDL_SCANCODE_F);
    profiles::set_input_binding(p3_controller, GameInput::A, 0, InputField::controller_digital(SDL_CONTROLLER_BUTTON_Y));
    CHECK(buttons(2) & 0x8000);
    profiles::reset_profile_bindings(p3_controller, InputDevice::Controller);
    CHECK(!(buttons(2) & 0x8000));
    CHECK(profiles::get_input_binding(p1_controller, GameInput::A, 0).input_id == SDL_CONTROLLER_BUTTON_Y);

    CHECK(players::set_player_controller(0, second));
    CHECK(players::get_player_controller(2) == first);
    CHECK(profiles::get_input_profile_for_player(0, InputDevice::Controller) == p1_controller);
    CHECK(players::get_controller_player(first) == 2 && players::get_keyboard_player() == 2);
    auto connected = players::get_connected_controllers();
    CHECK(connected.size() == 2 && connected[0].ordinal < connected[1].ordinal);
    CHECK(!connected[0].name.empty());
    auto first_id = connected[0].id;
    CHECK(players::get_time_since_controller_input(first_id) == std::chrono::steady_clock::duration::max());
    SDL_Event event{};
    event.type = SDL_CONTROLLERAXISMOTION;
    event.caxis.which = first_id;
    event.caxis.value = 100;
    players::record_input_activity(event);
    CHECK(players::get_time_since_controller_input(first_id) == std::chrono::steady_clock::duration::max());
    event.caxis.value = -20000;
    players::record_input_activity(event);
    CHECK(players::get_time_since_controller_input(first_id) < std::chrono::seconds(1));
    event.type = SDL_KEYDOWN;
    players::record_input_activity(event);
    CHECK(players::get_time_since_keyboard_input() < std::chrono::seconds(1));
    players::set_single_player_mode(true);
    event.type = SDL_CONTROLLERBUTTONDOWN;
    event.cbutton.which = first_id;
    players::record_input_activity(event);
    CHECK(players::get_time_since_controller_input(first_id) < std::chrono::seconds(1));
    CHECK(profiles::get_input_profile_for_player(0, InputDevice::Controller) == sp_controller);
    CHECK(profiles::get_input_profile_for_player(0, InputDevice::Keyboard) == sp_keyboard);
    CHECK(buttons(0) == buttons(0, true));
    players::set_single_player_mode(false);

    rumble_strength = 0.0;
    CHECK(players::test_rumble(first));
    CHECK(rumble_low == 0xFFFF && rumble_high == 0xFFFF);
    int calls = rumble_calls;
    set_rumble(2, true);
    update_rumble();
    CHECK(rumble_calls == calls);
    CHECK(!players::test_rumble(second));
    CHECK(players::test_rumble(first, 1));
    SDL_Delay(5);
    CHECK(!players::is_rumble_test_active(first));
    calls = rumble_calls;
    update_rumble();
    CHECK(rumble_calls > calls); // Slot 2 must be visited even though only two slots are assigned.
    rumble_strength = 100.0;

    CHECK(profiles::save_controls_config("mp2-controls.json"));
    auto persisted = saved_config;
    CHECK(persisted["players"].size() == 4);
    CHECK(persisted["players"][2]["keyboard"] == true);
    CHECK(persisted["players"][1]["controller"].is_null());
    CHECK(persisted["players"][2]["controller"].contains("serial"));
    profiles::clear_input_binding(p1_controller, GameInput::A);
    players::set_player_keyboard(0, true);
    saved_config = persisted;
    CHECK(profiles::load_controls_config("mp2-controls.json"));
    CHECK(players::get_keyboard_player() == 2);
    CHECK(profiles::get_input_binding(p1_controller, GameInput::A, 0).input_id == SDL_CONTROLLER_BUTTON_Y);
    CHECK(players::get_player_controller(0) == first); // Identical devices resolve by connection order.
    CHECK(players::get_player_controller(2) == second);
    auto second_id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(second));
    binding::start_scanning(2, GameInput::A, 0, InputDevice::Controller);
    remove_controller_state(second_id);
    CHECK(!binding::is_binding());
    CHECK(!players::get_player_controller(2) && players::get_player_keyboard(2));
    CHECK(get_controller_from_joystick_id(second_id) == nullptr);
    poll_inputs();
    update_rumble();
    CHECK(players::get_connected_controllers().size() == 1);
    CHECK(players::save_slots()[2]["controller"].is_object());
    auto* third = connect_virtual(false);
    CHECK(players::get_player_controller(2) == third);
    CHECK(players::get_connected_controllers().back().ordinal > connected.back().ordinal);
    remove_controller_state(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(third)));
    remove_controller_state(first_id);
    poll_inputs();
    CHECK(players::get_player_controller(0) == nullptr);
    CHECK(buttons(0, true) == 0);

    // The legacy shortcut uses the same fixed-slot model, with one keyboard.
    playerassignment::start();
    playerassignment::add_keyboard_player();
    playerassignment::add_keyboard_player();
    CHECK(players::get_keyboard_player() == 2); // Committed state is still untouched.
    CHECK(players::get_player_is_assigned(0, true));
    CHECK(!players::get_player_is_assigned(1, true));
    auto* assigned = connect_virtual();
    event.type = SDL_CONTROLLERBUTTONDOWN;
    event.cbutton.which = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(assigned));
    playerassignment::process_sdl_event(&event); // Repeated input must not allocate another slot.
    playerassignment::process_sdl_event(&event);
    CHECK(!players::get_player_is_assigned(2, true));
    CHECK(players::get_player(1, true).controller == assigned);
    auto assigned_id = event.cbutton.which;
    remove_controller_state(assigned_id);
    CHECK(players::get_player(1, true).controller == nullptr);
    auto* reconnected = connect_virtual();
    CHECK(players::get_player(1, true).controller == reconnected);
    playerassignment::commit_player_assignment();
    CHECK(players::get_player_controller(1) == reconnected && players::get_keyboard_player() == 0);
    remove_controller_state(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(reconnected)));

    ControllerGUID serial_identity{};
    serial_identity.serial = "MP2 serial roundtrip";
    serial_identity.vendor = 123;
    serial_identity.product = 456;
    serial_identity.hash = profiles::hash_controller_guid(serial_identity);
    auto serial_slots = nlohmann::json::array({
        {{"controller", nullptr}, {"keyboard", false}},
        {{"controller", serial_identity}, {"keyboard", true}}
    });
    players::load_slots(nlohmann::json::parse(serial_slots.dump()));
    CHECK(players::get_player(1).identity->serial == serial_identity.serial);
    CHECK(players::get_player(1).identity->hash == serial_identity.hash);
    CHECK(players::get_keyboard_player() == 1 && players::get_number_of_assigned_players() == 1);
    SDL_Quit();
}

int main() {
    assignment_tests();
    reconnect_tests();
    frontend_tests();
    std::puts("recompinput_player_slots_test PASS");
}
