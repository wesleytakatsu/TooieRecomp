#include "banjo_config.h"
#include "recompui/recompui.h"
#include "recompui/config.h"
#include "recompinput/recompinput.h"
#include "banjo_sound.h"
#include "tooie_fps_overlay.h"
#include "banjo_support.h"
#include "ultramodern/config.hpp"
#include "librecomp/files.hpp"
#include "librecomp/config.hpp"
#include "tooie_menu_policy.h"
#include "tooie_bgm_volume.h"
#include "hle/rt64_host_endpoint_current.h"
#include "util/file.h"
#include <filesystem>
#include <fstream>
#include <iomanip>

#if defined(_WIN32)
#include <Shlobj.h>
#elif defined(__linux__)
#include <unistd.h>
#include <pwd.h>
#elif defined(__APPLE__)
#include "apple/rt64_apple.h"
#endif

static void add_general_options(recomp::config::Config &config) {
    using EnumOptionVector = const std::vector<recomp::config::ConfigOptionEnumOption>;

    static EnumOptionVector multiplayer_mode_options = {
        {banjo::MultiplayerMode::Off, "Off", "Off"},
        {banjo::MultiplayerMode::On, "On", "On"},
    };
    config.add_enum_option(
        banjo::configkeys::general::multiplayer_mode,
        "Multiplayer (up to 4 players)",
        "Enables per-player controller assignment in Controls. Takes effect after restarting the game.",
        multiplayer_mode_options,
        banjo::MultiplayerMode::Off
    );

    static EnumOptionVector note_saving_mode_options = {
        {banjo::NoteSavingMode::Off,        "Off",    "Off"},
        {banjo::NoteSavingMode::OnlyNotes,  "Notes",  "Only Notes"},
        {banjo::NoteSavingMode::OnlyJinjos, "Jinjos", "Only Jinjos"},
        // Keeping as "On" to preserve compatibility with previous configs before jinjo saving
        {banjo::NoteSavingMode::Both,       "On",     "Both"},
    };
    config.add_enum_option(
        banjo::configkeys::general::note_saving_mode,
        "Note & Jinjo Saving",
        "Currently unavailable in Banjo-Tooie.",
        note_saving_mode_options,
        banjo::NoteSavingMode::Off,
        true
    );
    static EnumOptionVector analog_cam_mode_options = {
        {banjo::AnalogCamMode::Off, "Off", "Off"},
        {banjo::AnalogCamMode::On, "On", "On"},
    };
    config.add_enum_option(
        banjo::configkeys::general::analog_cam_mode,
        "Analog Camera",
        "Currently unavailable in Banjo-Tooie.",
        analog_cam_mode_options,
        banjo::AnalogCamMode::Off
    );
    config.add_number_option(
        banjo::configkeys::general::analog_camera_sensitivity,
        "Analog Camera Sensitivity",
        "Currently unavailable in Banjo-Tooie.",
        1, 10, 1, 0, false, 3
    );
    static EnumOptionVector camera_invert_mode_options = {
        {banjo::CameraInvertMode::InvertNone, "InvertNone", "None"},
        {banjo::CameraInvertMode::InvertX, "InvertX", "Invert X"},
        {banjo::CameraInvertMode::InvertY, "InvertY", "Invert Y"},
        {banjo::CameraInvertMode::InvertBoth, "InvertBoth", "Invert Both"}
    };
    config.add_enum_option(
        banjo::configkeys::general::third_person_camera_invert_mode,
        "Invert Camera",
        "Currently unavailable in Banjo-Tooie.",
        camera_invert_mode_options,
        banjo::CameraInvertMode::InvertX
    );
    static EnumOptionVector first_person_invert_mode_options = {
        {banjo::CameraInvertMode::InvertNone, "InvertNone", "None"},
        {banjo::CameraInvertMode::InvertX, "InvertX", "Invert X"},
        {banjo::CameraInvertMode::InvertY, "InvertY", "Invert Y"},
        {banjo::CameraInvertMode::InvertBoth, "InvertBoth", "Invert Both"}
    };
    config.add_enum_option(
        banjo::configkeys::general::first_person_invert_mode,
        "Invert First Person View",
        "Currently unavailable in Banjo-Tooie.",
        first_person_invert_mode_options,
        banjo::CameraInvertMode::InvertY
    );
    static EnumOptionVector flying_and_swimming_invert_options = {
        {banjo::CameraInvertMode::InvertNone, "InvertNone", "None"},
        {banjo::CameraInvertMode::InvertX, "InvertX", "Invert X"},
        {banjo::CameraInvertMode::InvertY, "InvertY", "Invert Y"},
        {banjo::CameraInvertMode::InvertBoth, "InvertBoth", "Invert Both"}
    };
    config.add_enum_option(
        banjo::configkeys::general::flying_and_swimming_invert_mode,
        "Invert Flying & Swimming",
        "Currently unavailable in Banjo-Tooie.",
        flying_and_swimming_invert_options,
        banjo::CameraInvertMode::InvertY
    );
    for (const char* id : {
        banjo::configkeys::general::analog_cam_mode.c_str(),
        banjo::configkeys::general::analog_camera_sensitivity.c_str(),
        banjo::configkeys::general::third_person_camera_invert_mode.c_str(),
        banjo::configkeys::general::first_person_invert_mode.c_str(),
        banjo::configkeys::general::flying_and_swimming_invert_mode.c_str(),
    }) {
        config.update_option_disabled(id, true);
    }
}

template <typename T = uint32_t>
T get_general_config_enum_value(const std::string& option_id) {
    return static_cast<T>(std::get<uint32_t>(recompui::config::get_general_config().get_option_value(option_id)));
}

template <typename T = uint32_t>
T get_general_config_number_value(const std::string& option_id) {
    return static_cast<T>(std::get<double>(recompui::config::get_general_config().get_option_value(option_id)));
}

banjo::NoteSavingMode banjo::get_note_saving_mode() {
    return get_general_config_enum_value<banjo::NoteSavingMode>(banjo::configkeys::general::note_saving_mode);
}

banjo::MultiplayerMode banjo::get_multiplayer_mode() {
    return get_general_config_enum_value<banjo::MultiplayerMode>(banjo::configkeys::general::multiplayer_mode);
}

banjo::CameraInvertMode banjo::get_camera_invert_mode() {
    // Legacy key is not registered; alias the registered third-person option.
    return get_third_person_camera_mode();
}

banjo::CameraInvertMode banjo::get_third_person_camera_mode() {
    return get_general_config_enum_value<banjo::CameraInvertMode>(banjo::configkeys::general::third_person_camera_invert_mode);
}

banjo::CameraInvertMode banjo::get_flying_and_swimming_invert_mode() {
    return get_general_config_enum_value<banjo::CameraInvertMode>(banjo::configkeys::general::flying_and_swimming_invert_mode);
}

banjo::CameraInvertMode banjo::get_first_person_invert_mode() {
    return get_general_config_enum_value<banjo::CameraInvertMode>(banjo::configkeys::general::first_person_invert_mode);
}

banjo::AnalogCamMode banjo::get_analog_cam_mode() {
    return get_general_config_enum_value<banjo::AnalogCamMode>(banjo::configkeys::general::analog_cam_mode);
}

uint32_t banjo::get_analog_cam_sensitivity() {
    return get_general_config_number_value(banjo::configkeys::general::analog_camera_sensitivity);
}

template <typename T = uint32_t>
T get_graphics_config_enum_value(const std::string& option_id) {
    return static_cast<T>(std::get<uint32_t>(recompui::config::get_graphics_config().get_option_value(option_id)));
}

static void add_sound_options(recomp::config::Config &config) {
    tooie_bgm::set_percent(100);
    config.add_percent_number_option(
        banjo::configkeys::sound::bgm_volume,
        "Background Music Volume",
        "Controls the overall volume of background music.",
        100.0f
    );
    // cur_value only. Do not capture Config&: the frontend vector moves it.
    config.add_option_change_callback(
        banjo::configkeys::sound::bgm_volume,
        [](recomp::config::ConfigValueVariant cur, recomp::config::ConfigValueVariant, recomp::config::OptionChangeContext) {
            if (const auto* percent = std::get_if<double>(&cur)) {
                tooie_bgm::set_percent(*percent);
            }
        }
    );
}
template <typename T = uint32_t>
T get_sound_config_number_value(const std::string& option_id) {
    return static_cast<T>(std::get<double>(recompui::config::get_sound_config().get_option_value(option_id)));
}

int banjo::get_bgm_volume() {
    return get_sound_config_number_value<int>(banjo::configkeys::sound::bgm_volume);
}

static void add_graphics_options(recomp::config::Config &config) {
    using EnumOptionVector = const std::vector<recomp::config::ConfigOptionEnumOption>;
    static EnumOptionVector cutscene_aspect_ratio_mode_options = {
        {banjo::CutsceneAspectRatioMode::Original, "Original", "Original"},
        {banjo::CutsceneAspectRatioMode::Clamp16x9, "Clamp16x9", "16:9"},
        {banjo::CutsceneAspectRatioMode::Full, "Expand", "Expand"},
    };
    config.add_enum_option(
        banjo::configkeys::graphics::cutscene_aspect_ratio_mode,
        "Cutscene Aspect Ratio",
        "Currently unavailable in Banjo-Tooie.",
        cutscene_aspect_ratio_mode_options,
        banjo::CutsceneAspectRatioMode::Clamp16x9
    );
    config.update_option_disabled(banjo::configkeys::graphics::cutscene_aspect_ratio_mode, true);
    config.add_bool_option(
        banjo::configkeys::graphics::show_fps_counter,
        "Show Present FPS",
        "Shows the rate of frames actually presented to the display in the top-left corner. This is not the game's simulation or animation rate.",
        false
    );
    config.add_option_change_callback(
        banjo::configkeys::graphics::show_fps_counter,
        [](recomp::config::ConfigValueVariant cur_value, recomp::config::ConfigValueVariant, recomp::config::OptionChangeContext) {
            tooie::set_fps_overlay_enabled(std::get<bool>(cur_value));
        }
    );

    namespace gfx = recompui::config::graphics::options;
    using AR = ultramodern::renderer::AspectRatio;

    // Window aspect: queue only on an applied change; the SDL thread (update_gfx) consumes it.
    config.add_option_change_callback(
        gfx::ar_option,
        [](recomp::config::ConfigValueVariant cur_value, recomp::config::ConfigValueVariant, recomp::config::OptionChangeContext ctx) {
            if (ctx != recomp::config::OptionChangeContext::Permanent) {
                return;
            }
            double target = tooie_menu::window_aspect_target(static_cast<AR>(std::get<uint32_t>(cur_value)));
            if (target > 0.0) {
                tooie_menu::request_window_aspect(target);
            }
        }
    );

    // Interpolation OFF (default) = native 30 cadence + CURRENT endpoint; ON = real intermediate frames.
    // Persisted like any graphics option; missing/invalid values load as OFF. The policy keeps the refresh-rate
    // options coherent and hands the APPLIED state to RT64 (latched per workload, no restart, no setenv).
    config.add_bool_option(
        banjo::configkeys::graphics::interpolation,
        "Interpolation",
        "Generates intermediate frames between game updates (needs a refresh rate above 30 FPS; 60 is selected when needed). "
        "Off keeps the original 30 FPS cadence. <recomp-color warning>Experimental: animated parts can show glitches.</recomp-color>",
        false
    );
    tooie_menu::install_interpolation_policy(
        []() -> recomp::config::Config& { return recompui::config::get_graphics_config(); },
        gfx::rr_option, gfx::rr_manual_value, banjo::configkeys::graphics::interpolation,
        [](bool enabled) { RT64::requestHostEndpointCurrent(!enabled); });

    const std::string fr_note = "Locked to 30 FPS while Interpolation is off.<br /><br />";
    config.update_option_description(gfx::rr_option, fr_note + "Sets the game's output framerate. This option does not affect gameplay.");
    config.update_option_description(gfx::rr_manual_value, fr_note + "Sets the game's output framerate. This option does not affect gameplay.");

    config.update_option_disabled(gfx::hr_option, true);
    config.update_option_description(gfx::hr_option, "Currently unavailable in Banjo-Tooie.");
}

static void set_control_defaults() {
    using namespace recompinput;

    // Left shoulder -> C Down | Backwards eggs / zoom out
    set_default_mapping_for_controller(
        GameInput::C_DOWN,
        { 
            InputField::controller_analog(SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY, true),
            InputField::controller_digital(SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
        }
    );

    // Right shoulder -> C Up | Forwards eggs / first person
    set_default_mapping_for_controller(
        GameInput::C_UP,
        { 
            InputField::controller_analog(SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY, false),
            InputField::controller_digital(SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)
        }
    );

    // North button -> C Left | Talon trot / camera left
    set_default_mapping_for_controller(
        GameInput::C_LEFT,
        { 
            InputField::controller_analog(SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX, false),
            InputField::controller_digital(SDL_CONTROLLER_BUTTON_NORTH)
        }
    );

    // East button -> C Right | Wonderwing / camera right
    set_default_mapping_for_controller(
        GameInput::C_RIGHT,
        { 
            InputField::controller_analog(SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX, true),
            InputField::controller_digital(SDL_CONTROLLER_BUTTON_EAST)
        }
    );

    // R3 -> L | Unused in BK but can be used in mods
    set_default_mapping_for_controller(GameInput::L, { InputField::controller_digital(SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSTICK) });
}

static void set_control_descriptions() {
    recompinput::set_game_input_description(recompinput::GameInput::Y_AXIS_POS, "Used to move and for steering while flying and swimming. Axis inversion for flying and swimming can be configured in the General tab.");
    recompinput::set_game_input_description(recompinput::GameInput::Y_AXIS_NEG, "Used to move and for steering while flying and swimming. Axis inversion for flying and swimming can be configured in the General tab.");
    recompinput::set_game_input_description(recompinput::GameInput::X_AXIS_NEG, "Used to move and for steering while flying and swimming. Axis inversion for flying and swimming can be configured in the General tab.");
    recompinput::set_game_input_description(recompinput::GameInput::X_AXIS_POS, "Used to move and for steering while flying and swimming. Axis inversion for flying and swimming can be configured in the General tab.");
    recompinput::set_game_input_description(recompinput::GameInput::A, "Used to jump and select options in menus. Also used for flying upwards.");
    recompinput::set_game_input_description(recompinput::GameInput::B, "Used for attacks, which change depending on whether you are stationary, moving, in the air, or crouching.");
    recompinput::set_game_input_description(recompinput::GameInput::Z, "Used to crouch, which enables A, B and the C-Buttons to perform different actions.");
    recompinput::set_game_input_description(recompinput::GameInput::L, "Used to skip dialogue boxes during game replays (press L + R + B simultaneously). Mods may also use it separately for additional features.");
    recompinput::set_game_input_description(recompinput::GameInput::R, "Used to center the camera behind Banjo on the ground, and to perform tighter turns while flying or swimming.");
    recompinput::set_game_input_description(recompinput::GameInput::START, "Used for pausing and for skipping certain cutscenes.");
    recompinput::set_game_input_description(recompinput::GameInput::C_UP, "Used to enter first-person mode, and to shoot eggs while holding Z.");
    recompinput::set_game_input_description(recompinput::GameInput::C_DOWN, "Used to toggle between the different camera zoom levels, and to shoot eggs backwards while holding Z.");
    recompinput::set_game_input_description(recompinput::GameInput::C_LEFT, "Used to rotate the camera sideways. Axis inversion can be configured in the General tab. Also used to enter Talon Trot while holding Z.");
    recompinput::set_game_input_description(recompinput::GameInput::C_RIGHT, "Used to rotate the camera sideways. Axis inversion can be configured in the General tab). Also used to enter Wonderwing while holding Z.");
    recompinput::set_game_input_description(recompinput::GameInput::DPAD_UP, "Unused. Mods may use it for additional features.");
    recompinput::set_game_input_description(recompinput::GameInput::DPAD_DOWN, "Unused. Mods may use it for additional features.");
    recompinput::set_game_input_description(recompinput::GameInput::DPAD_LEFT, "Unused. Mods may use it for additional features.");
    recompinput::set_game_input_description(recompinput::GameInput::DPAD_RIGHT, "Unused. Mods may use it for additional features.");
}

banjo::CutsceneAspectRatioMode banjo::get_cutscene_aspect_ratio_mode() {
    return get_graphics_config_enum_value<banjo::CutsceneAspectRatioMode>(banjo::configkeys::graphics::cutscene_aspect_ratio_mode);
}

void banjo::init_config() {
    tooie::register_fps_overlay();

    std::filesystem::path recomp_dir = recompui::file::get_app_folder_path();

    if (!recomp_dir.empty()) {
        std::filesystem::create_directories(recomp_dir);
    }

    recompui::config::GeneralTabOptions general_options{};
    general_options.has_rumble_strength = true;
    general_options.has_gyro_sensitivity = false;
    general_options.has_mouse_sensitivity = false;

    auto &general_config = recompui::config::create_general_tab(general_options);
    add_general_options(general_config);

    auto &graphics_config = recompui::config::create_graphics_tab();
    add_graphics_options(graphics_config);

    set_control_defaults();
    set_control_descriptions();
    recompui::config::create_controls_tab();

    auto &sound_config = recompui::config::create_sound_tab();
    add_sound_options(sound_config);

    recompui::config::create_mods_tab();

    recompui::config::finalize();

}
