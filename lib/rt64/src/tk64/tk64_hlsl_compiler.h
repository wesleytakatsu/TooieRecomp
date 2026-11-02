//
// TK64
//
// Copyright (c) 2026 The TK64 contributors. Licensed under the MIT license. See LICENSE file for details.
//
// HLSL 5.0 to DXBC for the Direct3D 11 backend: D3DCompile (d3dcompiler_47.dll, loaded at run time), a disk cache, and
// the shaders compiled when the project was built. See docs/specs/d3d11-backend.md.
//

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "tk64_shader_translator.h"

namespace tk64 {
    // A shader compiled at build time, found by the key of its HLSL (see hlslKey).
    struct BakedShader {
        uint64_t key;
        const uint8_t *dxbc;
        uint32_t size;
    };

    // The key of an HLSL source for a stage: it covers the profile and the exact text, so any change to the translation
    // gives another key.
    uint64_t hlslKey(const std::string &source, ShaderStage stage);
    const char *hlslProfile(ShaderStage stage);

    class HLSLCompiler {
    public:
        // cacheDirectory: TK64_SHADER_CACHE, else %LOCALAPPDATA%\TK64\shadercache-d3d11; empty disables the cache.
        bool open();
        bool available() const { return compileFunction != nullptr; }

        // Looks in the baked shaders, then the disk cache, then compiles (and stores the result in the cache).
        bool compile(const std::string &source, ShaderStage stage, std::vector<uint8_t> &dxbc, std::string &errors) const;

        // Compiles an HLSL source with an entry point and profile of its own (internal shaders).
        bool compileRaw(const std::string &source, const char *entryPoint, const char *profile, std::vector<uint8_t> &dxbc, std::string &errors) const;

    private:
        void *compileFunction = nullptr;
        std::string cacheDirectory;
    };

    // The shaders baked into this build; defined by the generated tk64_dxbc_baked.cpp of the project that builds them.
    const BakedShader *findBakedShader(uint64_t key);
};
