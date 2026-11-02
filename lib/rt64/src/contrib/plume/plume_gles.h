//
// plume (TK64)
//
// Copyright (c) 2026 The TK64 contributors. Licensed under the MIT license. See LICENSE file for details.
//
// OpenGL 4.3 core / OpenGL ES 3.2 backend. Contract: docs/specs/backend-contract.md in TK64.
//

#pragma once

#include "plume_render_interface.h"

namespace plume {
    enum class RenderGLProfile {
        // Desktop OpenGL 4.3 core profile.
        DESKTOP,

        // OpenGL ES 3.2.
        ES
    };

    // Returns nullptr when the platform has no OpenGL backend.
    std::unique_ptr<RenderInterface> CreateGLESInterface(RenderWindow renderWindow, RenderGLProfile profile);
};
