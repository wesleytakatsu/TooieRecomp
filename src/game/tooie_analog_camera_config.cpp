#include "tooie_analog_camera.h"
#include "banjo_config.h"
#include "librecomp/config.hpp"

void tooie_analog_camera::install_config(recomp::config::Config& config) {
    using namespace banjo::configkeys::general;
    using recomp::config::OptionChangeContext;
    // Config's missing-file path saves defaults without invoking Load callbacks.
    set_enabled(std::get<uint32_t>(config.get_option_value(analog_cam_mode)) == unsigned(banjo::AnalogCamMode::On));
    set_sensitivity(unsigned(std::get<double>(config.get_option_value(analog_camera_sensitivity))));
    set_invert(std::get<uint32_t>(config.get_option_value(third_person_camera_invert_mode)));
    config.add_option_change_callback(analog_cam_mode,
        [](recomp::config::ConfigValueVariant v, recomp::config::ConfigValueVariant, OptionChangeContext context) {
            if (context != OptionChangeContext::Temporary)
                set_enabled(std::get<uint32_t>(v) == unsigned(banjo::AnalogCamMode::On));
        });
    config.add_option_change_callback(analog_camera_sensitivity,
        [](recomp::config::ConfigValueVariant v, recomp::config::ConfigValueVariant, OptionChangeContext context) {
            if (context != OptionChangeContext::Temporary) set_sensitivity(unsigned(std::get<double>(v)));
        });
    config.add_option_change_callback(third_person_camera_invert_mode,
        [](recomp::config::ConfigValueVariant v, recomp::config::ConfigValueVariant, OptionChangeContext context) {
            if (context != OptionChangeContext::Temporary) set_invert(std::get<uint32_t>(v));
        });
}
