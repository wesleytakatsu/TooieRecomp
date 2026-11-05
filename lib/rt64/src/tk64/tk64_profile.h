//
// TK64
//
// Copyright (c) 2026 The TK64 contributors. Licensed under the MIT license. See LICENSE file for details.
//
// GPU profile applied on top of the host's RT64 settings. See docs/specs/weak-gpu-profile.md.
//

#pragma once

#ifdef TK64_MOBILE
#include "common/rt64_emulator_configuration.h"
#endif
#include "common/rt64_user_configuration.h"

namespace tk64 {
    enum class Profile {
        Full,
        Weak
    };

    // TK64_PROFILE: "weak", "full", or "auto"/unset. Auto picks Weak when the OpenGL ES backend is in use.
    Profile selectProfile(bool openGLESBackend);

#ifdef TK64_MOBILE
    // What selectProfile last returned, for a host's own rendering such as its menus. Full before the first selection.
    Profile selectedProfile();

#endif
    // "full" or "weak".
    const char *profileName(Profile profile);

    // Overrides the settings of docs/specs/weak-gpu-profile.md for Weak; leaves the configuration alone for Full.
    void applyProfile(Profile profile, RT64::UserConfiguration &config);

#ifdef TK64_MOBILE
    // Weak: framebuffers are not written back to RAM, which makes the CPU wait for the GPU. TK64_RENDER_TO_RAM=1 or =0 overrides it.
    void applyProfile(Profile profile, RT64::EmulatorConfiguration &config);

    // Weak with render to RAM off: the depth still reaches RAM, a frame or more late, for games that read it. TK64_DEPTH_TO_RAM=1 or =0 overrides it.
    bool usesDepthToRAM(Profile profile, const RT64::EmulatorConfiguration &config);

    // False unless TK64_UBERSHADERS_ONLY=1.
#else
    // Weak: true. Full: false. TK64_UBERSHADERS_ONLY=1 or =0, when set, overrides the profile's value.
#endif
    bool usesUbershadersOnly(Profile profile);
};
