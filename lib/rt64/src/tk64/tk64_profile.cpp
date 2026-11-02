//
// TK64
//
// Copyright (c) 2026 The TK64 contributors. Licensed under the MIT license. See LICENSE file for details.
//

#include "tk64_profile.h"

#include <cstdlib>
#include <cstring>

namespace tk64 {
    Profile selectProfile(bool openGLESBackend) {
        const Profile automatic = openGLESBackend ? Profile::Weak : Profile::Full;
        const char *value = std::getenv("TK64_PROFILE");
        if (value == nullptr) {
            return automatic;
        }

        if (std::strcmp(value, "weak") == 0) {
            return Profile::Weak;
        }

        if (std::strcmp(value, "full") == 0) {
            return Profile::Full;
        }

        // "auto" and any unrecognized value follow the backend, same as an unset variable.
        return automatic;
    }

    const char *profileName(Profile profile) {
        switch (profile) {
        case Profile::Weak:
            return "weak";
        case Profile::Full:
        default:
            return "full";
        }
    }

    void applyProfile(Profile profile, RT64::UserConfiguration &config) {
        if (profile != Profile::Weak) {
            return;
        }

        // These are the fields that dominate fill rate, bandwidth and extra frames on a weak GPU.
        config.resolution = RT64::UserConfiguration::Resolution::Original;
        config.downsampleMultiplier = 1;
        config.antialiasing = RT64::UserConfiguration::Antialiasing::None;
        config.refreshRate = RT64::UserConfiguration::RefreshRate::Original;
        config.internalColorFormat = RT64::UserConfiguration::InternalColorFormat::Standard;
        config.displayBuffering = RT64::UserConfiguration::DisplayBuffering::Double;
        config.filtering = RT64::UserConfiguration::Filtering::Linear;
        config.idleWorkActive = false;
    }

    bool usesUbershadersOnly(Profile profile) {
        const char *value = std::getenv("TK64_UBERSHADERS_ONLY");
        if (value != nullptr) {
            if (std::strcmp(value, "1") == 0) {
                return true;
            }

            if (std::strcmp(value, "0") == 0) {
                return false;
            }
        }

        return profile == Profile::Weak;
    }
};
