//
// TK64
//
// Copyright (c) 2026 The TK64 contributors. Licensed under the MIT license. See LICENSE file for details.
//
// GPU profile applied on top of the host's RT64 settings. See docs/specs/weak-gpu-profile.md.
//

#pragma once

#include "common/rt64_user_configuration.h"

namespace tk64 {
    enum class Profile {
        Full,
        Weak
    };

    // TK64_PROFILE: "weak", "full", or "auto"/unset. Auto picks Weak when the OpenGL ES backend is in use.
    Profile selectProfile(bool openGLESBackend);

    // "full" or "weak".
    const char *profileName(Profile profile);

    // Overrides the settings of docs/specs/weak-gpu-profile.md for Weak; leaves the configuration alone for Full.
    void applyProfile(Profile profile, RT64::UserConfiguration &config);

    // Weak: true. Full: false. TK64_UBERSHADERS_ONLY=1 or =0, when set, overrides the profile's value.
    bool usesUbershadersOnly(Profile profile);
};
