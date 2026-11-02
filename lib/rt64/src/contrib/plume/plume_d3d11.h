//
// plume (TK64)
//
// Copyright (c) 2026 The TK64 contributors. Licensed under the MIT license. See LICENSE file for details.
//
// Direct3D 11 (feature level 11_0) backend. Contract: docs/specs/backend-contract.md in TK64.
//

#pragma once

#include "plume_render_interface.h"

namespace plume {
    // Null on platforms other than Windows. Device creation fails when no adapter supports feature level 11_0.
    std::unique_ptr<RenderInterface> CreateD3D11Interface();
};
