//
// TK64
//
// Copyright (c) 2026 The TK64 contributors. Licensed under the MIT license. See LICENSE file for details.
//

#include "tk64_hlsl_compiler.h"

#if defined(_WIN32)

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>

#include <windows.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace tk64 {
    static uint64_t fnv1a(const char *data, size_t size, uint64_t value) {
        for (size_t i = 0; i < size; i++) {
            value ^= uint8_t(data[i]);
            value *= 0x100000001b3ULL;
        }

        return value;
    }

    const char *hlslProfile(ShaderStage stage) {
        switch (stage) {
        case ShaderStage::Vertex: return "vs_5_0";
        case ShaderStage::Compute: return "cs_5_0";
        default: return "ps_5_0";
        }
    }

    uint64_t hlslKey(const std::string &source, ShaderStage stage) {
        const std::string profileLine = std::string(hlslProfile(stage)) + "\n";
        return fnv1a(source.data(), source.size(), fnv1a(profileLine.data(), profileLine.size(), 0xcbf29ce484222325ULL));
    }

    bool HLSLCompiler::open() {
        HMODULE module = LoadLibraryW(L"d3dcompiler_47.dll");
        if (module != nullptr) {
            compileFunction = reinterpret_cast<void *>(GetProcAddress(module, "D3DCompile"));
        }

        const char *cacheOverride = getenv("TK64_SHADER_CACHE");
        const char *localAppData = getenv("LOCALAPPDATA");
        if (cacheOverride != nullptr) {
            cacheDirectory = cacheOverride;
        }
        else if (localAppData != nullptr) {
            cacheDirectory = (std::filesystem::u8path(localAppData) / "TK64" / "shadercache-d3d11").u8string();
        }

        return compileFunction != nullptr;
    }

    bool HLSLCompiler::compileRaw(const std::string &source, const char *entryPoint, const char *profile, std::vector<uint8_t> &dxbc, std::string &errors) const {
        if (compileFunction == nullptr) {
            errors = "d3dcompiler_47.dll is not available";
            return false;
        }

        ComPtr<ID3DBlob> code;
        ComPtr<ID3DBlob> messages;
        const pD3DCompile compile = reinterpret_cast<pD3DCompile>(compileFunction);
        const HRESULT result = compile(source.data(), source.size(), "tk64.hlsl", nullptr, nullptr, entryPoint, profile, 0, 0, code.GetAddressOf(), messages.GetAddressOf());
        if (FAILED(result) || (code == nullptr)) {
            errors = (messages != nullptr) ? std::string(static_cast<const char *>(messages->GetBufferPointer()), messages->GetBufferSize()) : "D3DCompile failed";
            return false;
        }

        const uint8_t *bytes = static_cast<const uint8_t *>(code->GetBufferPointer());
        dxbc.assign(bytes, bytes + code->GetBufferSize());
        return true;
    }

    bool HLSLCompiler::compile(const std::string &source, ShaderStage stage, std::vector<uint8_t> &dxbc, std::string &errors) const {
        const uint64_t key = hlslKey(source, stage);
        const BakedShader *baked = findBakedShader(key);
        if (baked != nullptr) {
            dxbc.assign(baked->dxbc, baked->dxbc + baked->size);
            return true;
        }

        char fileName[32] = {};
        snprintf(fileName, sizeof(fileName), "%016llx.dxbc", (unsigned long long)(key));
        const std::filesystem::path cachePath = cacheDirectory.empty() ? std::filesystem::path() : (std::filesystem::u8path(cacheDirectory) / fileName);
        if (!cachePath.empty()) {
            std::ifstream cached(cachePath, std::ios::binary);
            if (cached.is_open()) {
                dxbc.assign(std::istreambuf_iterator<char>(cached), std::istreambuf_iterator<char>());
                if (!dxbc.empty()) {
                    return true;
                }
            }
        }

        if (!compileRaw(source, "main", hlslProfile(stage), dxbc, errors)) {
            return false;
        }

        if (!cachePath.empty()) {
            // Written under a temporary name first, so a reader never sees half a file.
            std::error_code error;
            std::filesystem::create_directories(cachePath.parent_path(), error);
            const std::filesystem::path temporaryPath = cachePath.string() + ".tmp" + std::to_string(GetCurrentThreadId());
            {
                std::ofstream file(temporaryPath, std::ios::binary);
                file.write(reinterpret_cast<const char *>(dxbc.data()), std::streamsize(dxbc.size()));
            }

            std::filesystem::rename(temporaryPath, cachePath, error);
            if (error) {
                std::filesystem::remove(temporaryPath, error);
            }
        }

        return true;
    }
};

#endif
