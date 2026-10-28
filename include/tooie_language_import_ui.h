#pragma once

#include "librecomp/config.hpp"

namespace tooie_language_import_ui {
    void on_language_change(recomp::config::ConfigValueVariant value,
                            recomp::config::ConfigValueVariant previous_value,
                            recomp::config::OptionChangeContext context);
}
