//
// TK64
//
// Copyright (c) 2026 The TK64 contributors. Licensed under the MIT license. See LICENSE file for details.
//

#include "tk64_profile.h"

#include <cstdlib>
#include <cstring>

namespace tk64 {
#ifdef TK64_MOBILE
    static Profile lastSelectedProfile = Profile::Full;

    static Profile readProfile(bool openGLESBackend) {
#else
    Profile selectProfile(bool openGLESBackend) {
#endif
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

#ifdef TK64_MOBILE
    Profile selectProfile(bool openGLESBackend) {
        lastSelectedProfile = readProfile(openGLESBackend);
        return lastSelectedProfile;
    }

    Profile selectedProfile() {
        return lastSelectedProfile;
    }

#endif
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

#ifdef TK64_MOBILE
        // A target of 30 or less shows a game at 20 or 30 FPS nothing new, and each extra frame renders the whole scene again.
        // Frame matching only runs for a target, too. Higher targets are a request for interpolation and stay.
        if ((config.refreshRate == RT64::UserConfiguration::RefreshRate::Manual) && (config.refreshRateTarget <= 30)) {
            config.refreshRate = RT64::UserConfiguration::RefreshRate::Original;
        }

        // Resolution and antialiasing stay the player's choice: the host's menu writes them, and a port ships its defaults in
        // its own settings. These fields have no menu option and cost bandwidth on a weak GPU.
#else
        // These are the fields that dominate fill rate, bandwidth and extra frames on a weak GPU.
        config.resolution = RT64::UserConfiguration::Resolution::Original;
        config.downsampleMultiplier = 1;
        config.antialiasing = RT64::UserConfiguration::Antialiasing::None;
        config.refreshRate = RT64::UserConfiguration::RefreshRate::Original;
#endif
        config.internalColorFormat = RT64::UserConfiguration::InternalColorFormat::Standard;
        config.displayBuffering = RT64::UserConfiguration::DisplayBuffering::Double;
        config.filtering = RT64::UserConfiguration::Filtering::Linear;
        config.idleWorkActive = false;
    }

#ifdef TK64_MOBILE
    void applyProfile(Profile profile, RT64::EmulatorConfiguration &config) {
        if (profile == Profile::Weak) {
            config.framebuffer.renderToRAM = false;
        }

        const char *value = std::getenv("TK64_RENDER_TO_RAM");
        if (value != nullptr) {
            config.framebuffer.renderToRAM = (std::strcmp(value, "1") == 0);
        }
    }

    bool usesDepthToRAM(Profile profile, const RT64::EmulatorConfiguration &config) {
        const char *value = std::getenv("TK64_DEPTH_TO_RAM");
        if (value != nullptr) {
            return (std::strcmp(value, "1") == 0);
        }

        return (profile == Profile::Weak) && !config.framebuffer.renderToRAM;
    }

#endif
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

#ifdef TK64_MOBILE
        // Measured on a Mali-G31: the ubershader alone gives half the frame rate of the specialised shaders, and the shader
        // cache keeps their background compiles to the first run (docs/specs/weak-gpu-profile.md).
        return false;
#else
        return profile == Profile::Weak;
#endif
    }
};
