//
// plume (TK64)
//
// Copyright (c) 2026 The TK64 contributors. Licensed under the MIT license. See LICENSE file for details.
//
// Direct3D 11 (feature level 11_0) backend for plume.
//
// Design (docs/specs/d3d11-backend.md in TK64):
// - Command lists record closures; the queue replays them on the immediate context under one lock, in submission order.
//   ID3D11Device is free-threaded, so resources, views, states and shaders are created on the calling thread.
// - Shaders arrive as SPIR-V. createShader translates each one to HLSL 5.0 with tk64::translateSpirv (dense registers per
//   stage: D3D11 has one register space per stage, so no pipeline-wide map is needed) and compiles it to DXBC, read from
//   a disk cache when the same HLSL was compiled before.
// - Upload, readback and constant buffers keep a CPU copy. Copies from an upload buffer read it at replay time (as D3D12
//   reads an upload heap when the GPU runs the copy); copies into a readback buffer go through a staging resource and
//   reach its CPU copy before the replay ends. The replay is synchronous with submission, so fences need no GPU work.
//

#include "plume_d3d11.h"

#if defined(_WIN64)

#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include "tk64_hlsl_compiler.h"
#include "tk64_shader_translator.h"

using Microsoft::WRL::ComPtr;

namespace plume {
    // Conversions.

    static uint64_t roundUp(uint64_t value, uint64_t powerOf2Alignment) {
        return (value + powerOf2Alignment - 1) & ~(powerOf2Alignment - 1);
    }

    static DXGI_FORMAT toDXGI(RenderFormat format) {
        switch (format) {
        case RenderFormat::R32G32B32A32_TYPELESS: return DXGI_FORMAT_R32G32B32A32_TYPELESS;
        case RenderFormat::R32G32B32A32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case RenderFormat::R32G32B32A32_UINT: return DXGI_FORMAT_R32G32B32A32_UINT;
        case RenderFormat::R32G32B32A32_SINT: return DXGI_FORMAT_R32G32B32A32_SINT;
        case RenderFormat::R32G32B32_TYPELESS: return DXGI_FORMAT_R32G32B32_TYPELESS;
        case RenderFormat::R32G32B32_FLOAT: return DXGI_FORMAT_R32G32B32_FLOAT;
        case RenderFormat::R32G32B32_UINT: return DXGI_FORMAT_R32G32B32_UINT;
        case RenderFormat::R32G32B32_SINT: return DXGI_FORMAT_R32G32B32_SINT;
        case RenderFormat::R16G16B16A16_TYPELESS: return DXGI_FORMAT_R16G16B16A16_TYPELESS;
        case RenderFormat::R16G16B16A16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case RenderFormat::R16G16B16A16_UNORM: return DXGI_FORMAT_R16G16B16A16_UNORM;
        case RenderFormat::R16G16B16A16_UINT: return DXGI_FORMAT_R16G16B16A16_UINT;
        case RenderFormat::R16G16B16A16_SNORM: return DXGI_FORMAT_R16G16B16A16_SNORM;
        case RenderFormat::R16G16B16A16_SINT: return DXGI_FORMAT_R16G16B16A16_SINT;
        case RenderFormat::R32G32_TYPELESS: return DXGI_FORMAT_R32G32_TYPELESS;
        case RenderFormat::R32G32_FLOAT: return DXGI_FORMAT_R32G32_FLOAT;
        case RenderFormat::R32G32_UINT: return DXGI_FORMAT_R32G32_UINT;
        case RenderFormat::R32G32_SINT: return DXGI_FORMAT_R32G32_SINT;
        case RenderFormat::R8G8B8A8_TYPELESS: return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case RenderFormat::R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case RenderFormat::R8G8B8A8_UINT: return DXGI_FORMAT_R8G8B8A8_UINT;
        case RenderFormat::R8G8B8A8_SNORM: return DXGI_FORMAT_R8G8B8A8_SNORM;
        case RenderFormat::R8G8B8A8_SINT: return DXGI_FORMAT_R8G8B8A8_SINT;
        case RenderFormat::B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM;
        case RenderFormat::R16G16_TYPELESS: return DXGI_FORMAT_R16G16_TYPELESS;
        case RenderFormat::R16G16_FLOAT: return DXGI_FORMAT_R16G16_FLOAT;
        case RenderFormat::R16G16_UNORM: return DXGI_FORMAT_R16G16_UNORM;
        case RenderFormat::R16G16_UINT: return DXGI_FORMAT_R16G16_UINT;
        case RenderFormat::R16G16_SNORM: return DXGI_FORMAT_R16G16_SNORM;
        case RenderFormat::R16G16_SINT: return DXGI_FORMAT_R16G16_SINT;
        case RenderFormat::R32_TYPELESS: return DXGI_FORMAT_R32_TYPELESS;
        case RenderFormat::D32_FLOAT: return DXGI_FORMAT_D32_FLOAT;
        case RenderFormat::D32_FLOAT_S8_UINT: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        case RenderFormat::R32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
        case RenderFormat::R32_UINT: return DXGI_FORMAT_R32_UINT;
        case RenderFormat::R32_SINT: return DXGI_FORMAT_R32_SINT;
        case RenderFormat::R8G8_TYPELESS: return DXGI_FORMAT_R8G8_TYPELESS;
        case RenderFormat::R8G8_UNORM: return DXGI_FORMAT_R8G8_UNORM;
        case RenderFormat::R8G8_UINT: return DXGI_FORMAT_R8G8_UINT;
        case RenderFormat::R8G8_SNORM: return DXGI_FORMAT_R8G8_SNORM;
        case RenderFormat::R8G8_SINT: return DXGI_FORMAT_R8G8_SINT;
        case RenderFormat::R16_TYPELESS: return DXGI_FORMAT_R16_TYPELESS;
        case RenderFormat::R16_FLOAT: return DXGI_FORMAT_R16_FLOAT;
        case RenderFormat::D16_UNORM: return DXGI_FORMAT_D16_UNORM;
        case RenderFormat::R16_UNORM: return DXGI_FORMAT_R16_UNORM;
        case RenderFormat::R16_UINT: return DXGI_FORMAT_R16_UINT;
        case RenderFormat::R16_SNORM: return DXGI_FORMAT_R16_SNORM;
        case RenderFormat::R16_SINT: return DXGI_FORMAT_R16_SINT;
        case RenderFormat::R8_TYPELESS: return DXGI_FORMAT_R8_TYPELESS;
        case RenderFormat::R8_UNORM: return DXGI_FORMAT_R8_UNORM;
        case RenderFormat::R8_UINT: return DXGI_FORMAT_R8_UINT;
        case RenderFormat::R8_SNORM: return DXGI_FORMAT_R8_SNORM;
        case RenderFormat::R8_SINT: return DXGI_FORMAT_R8_SINT;
        case RenderFormat::BC1_TYPELESS: return DXGI_FORMAT_BC1_TYPELESS;
        case RenderFormat::BC1_UNORM: return DXGI_FORMAT_BC1_UNORM;
        case RenderFormat::BC1_UNORM_SRGB: return DXGI_FORMAT_BC1_UNORM_SRGB;
        case RenderFormat::BC2_TYPELESS: return DXGI_FORMAT_BC2_TYPELESS;
        case RenderFormat::BC2_UNORM: return DXGI_FORMAT_BC2_UNORM;
        case RenderFormat::BC2_UNORM_SRGB: return DXGI_FORMAT_BC2_UNORM_SRGB;
        case RenderFormat::BC3_TYPELESS: return DXGI_FORMAT_BC3_TYPELESS;
        case RenderFormat::BC3_UNORM: return DXGI_FORMAT_BC3_UNORM;
        case RenderFormat::BC3_UNORM_SRGB: return DXGI_FORMAT_BC3_UNORM_SRGB;
        case RenderFormat::BC4_TYPELESS: return DXGI_FORMAT_BC4_TYPELESS;
        case RenderFormat::BC4_UNORM: return DXGI_FORMAT_BC4_UNORM;
        case RenderFormat::BC4_SNORM: return DXGI_FORMAT_BC4_SNORM;
        case RenderFormat::BC5_TYPELESS: return DXGI_FORMAT_BC5_TYPELESS;
        case RenderFormat::BC5_UNORM: return DXGI_FORMAT_BC5_UNORM;
        case RenderFormat::BC5_SNORM: return DXGI_FORMAT_BC5_SNORM;
        case RenderFormat::BC6H_TYPELESS: return DXGI_FORMAT_BC6H_TYPELESS;
        case RenderFormat::BC6H_UF16: return DXGI_FORMAT_BC6H_UF16;
        case RenderFormat::BC6H_SF16: return DXGI_FORMAT_BC6H_SF16;
        case RenderFormat::BC7_TYPELESS: return DXGI_FORMAT_BC7_TYPELESS;
        case RenderFormat::BC7_UNORM: return DXGI_FORMAT_BC7_UNORM;
        case RenderFormat::BC7_UNORM_SRGB: return DXGI_FORMAT_BC7_UNORM_SRGB;
        default: return DXGI_FORMAT_UNKNOWN;
        }
    }

    // A depth texture that is also sampled must be created typeless, with typed depth and shader views.
    struct D3D11Formats {
        DXGI_FORMAT resource = DXGI_FORMAT_UNKNOWN;
        DXGI_FORMAT shaderView = DXGI_FORMAT_UNKNOWN;
        DXGI_FORMAT depthView = DXGI_FORMAT_UNKNOWN;
        bool depth = false;
    };

    static D3D11Formats toD3D11Formats(RenderFormat format) {
        D3D11Formats formats;
        formats.resource = toDXGI(format);
        formats.shaderView = formats.resource;
        formats.depthView = formats.resource;
        switch (format) {
        case RenderFormat::D32_FLOAT:
            formats.resource = DXGI_FORMAT_R32_TYPELESS;
            formats.shaderView = DXGI_FORMAT_R32_FLOAT;
            formats.depth = true;
            break;
        case RenderFormat::D16_UNORM:
            formats.resource = DXGI_FORMAT_R16_TYPELESS;
            formats.shaderView = DXGI_FORMAT_R16_UNORM;
            formats.depth = true;
            break;
        case RenderFormat::D32_FLOAT_S8_UINT:
            formats.resource = DXGI_FORMAT_R32G8X24_TYPELESS;
            formats.shaderView = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
            formats.depth = true;
            break;
        default:
            break;
        }

        return formats;
    }

    // The format a view of a texture is created with, when the view names its own format.
    static DXGI_FORMAT toShaderViewFormat(RenderFormat format) {
        const D3D11Formats formats = toD3D11Formats(format);
        return formats.shaderView;
    }

    static D3D11_BLEND toD3D11(RenderBlend blend) {
        switch (blend) {
        case RenderBlend::ZERO: return D3D11_BLEND_ZERO;
        case RenderBlend::ONE: return D3D11_BLEND_ONE;
        case RenderBlend::SRC_COLOR: return D3D11_BLEND_SRC_COLOR;
        case RenderBlend::INV_SRC_COLOR: return D3D11_BLEND_INV_SRC_COLOR;
        case RenderBlend::SRC_ALPHA: return D3D11_BLEND_SRC_ALPHA;
        case RenderBlend::INV_SRC_ALPHA: return D3D11_BLEND_INV_SRC_ALPHA;
        case RenderBlend::DEST_ALPHA: return D3D11_BLEND_DEST_ALPHA;
        case RenderBlend::INV_DEST_ALPHA: return D3D11_BLEND_INV_DEST_ALPHA;
        case RenderBlend::DEST_COLOR: return D3D11_BLEND_DEST_COLOR;
        case RenderBlend::INV_DEST_COLOR: return D3D11_BLEND_INV_DEST_COLOR;
        case RenderBlend::SRC_ALPHA_SAT: return D3D11_BLEND_SRC_ALPHA_SAT;
        case RenderBlend::BLEND_FACTOR: return D3D11_BLEND_BLEND_FACTOR;
        case RenderBlend::INV_BLEND_FACTOR: return D3D11_BLEND_INV_BLEND_FACTOR;
        case RenderBlend::SRC1_COLOR: return D3D11_BLEND_SRC1_COLOR;
        case RenderBlend::INV_SRC1_COLOR: return D3D11_BLEND_INV_SRC1_COLOR;
        case RenderBlend::SRC1_ALPHA: return D3D11_BLEND_SRC1_ALPHA;
        case RenderBlend::INV_SRC1_ALPHA: return D3D11_BLEND_INV_SRC1_ALPHA;
        default: return D3D11_BLEND_ONE;
        }
    }

    static D3D11_BLEND_OP toD3D11(RenderBlendOperation operation) {
        switch (operation) {
        case RenderBlendOperation::SUBTRACT: return D3D11_BLEND_OP_SUBTRACT;
        case RenderBlendOperation::REV_SUBTRACT: return D3D11_BLEND_OP_REV_SUBTRACT;
        case RenderBlendOperation::MIN: return D3D11_BLEND_OP_MIN;
        case RenderBlendOperation::MAX: return D3D11_BLEND_OP_MAX;
        default: return D3D11_BLEND_OP_ADD;
        }
    }

    static D3D11_COMPARISON_FUNC toD3D11(RenderComparisonFunction function) {
        switch (function) {
        case RenderComparisonFunction::LESS: return D3D11_COMPARISON_LESS;
        case RenderComparisonFunction::EQUAL: return D3D11_COMPARISON_EQUAL;
        case RenderComparisonFunction::LESS_EQUAL: return D3D11_COMPARISON_LESS_EQUAL;
        case RenderComparisonFunction::GREATER: return D3D11_COMPARISON_GREATER;
        case RenderComparisonFunction::NOT_EQUAL: return D3D11_COMPARISON_NOT_EQUAL;
        case RenderComparisonFunction::GREATER_EQUAL: return D3D11_COMPARISON_GREATER_EQUAL;
        case RenderComparisonFunction::ALWAYS: return D3D11_COMPARISON_ALWAYS;
        default: return D3D11_COMPARISON_NEVER;
        }
    }

    static D3D11_STENCIL_OP toD3D11(RenderStencilOp operation) {
        switch (operation) {
        case RenderStencilOp::ZERO: return D3D11_STENCIL_OP_ZERO;
        case RenderStencilOp::REPLACE: return D3D11_STENCIL_OP_REPLACE;
        case RenderStencilOp::INCREMENT_AND_CLAMP: return D3D11_STENCIL_OP_INCR_SAT;
        case RenderStencilOp::DECREMENT_AND_CLAMP: return D3D11_STENCIL_OP_DECR_SAT;
        case RenderStencilOp::INVERT: return D3D11_STENCIL_OP_INVERT;
        case RenderStencilOp::INCREMENT_AND_WRAP: return D3D11_STENCIL_OP_INCR;
        case RenderStencilOp::DECREMENT_AND_WRAP: return D3D11_STENCIL_OP_DECR;
        default: return D3D11_STENCIL_OP_KEEP;
        }
    }

    static D3D11_TEXTURE_ADDRESS_MODE toD3D11(RenderTextureAddressMode addressMode) {
        switch (addressMode) {
        case RenderTextureAddressMode::MIRROR: return D3D11_TEXTURE_ADDRESS_MIRROR;
        case RenderTextureAddressMode::CLAMP: return D3D11_TEXTURE_ADDRESS_CLAMP;
        case RenderTextureAddressMode::BORDER: return D3D11_TEXTURE_ADDRESS_BORDER;
        case RenderTextureAddressMode::MIRROR_ONCE: return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
        default: return D3D11_TEXTURE_ADDRESS_WRAP;
        }
    }

    static D3D11_PRIMITIVE_TOPOLOGY toD3D11(RenderPrimitiveTopology topology) {
        switch (topology) {
        case RenderPrimitiveTopology::POINT_LIST: return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        case RenderPrimitiveTopology::LINE_LIST: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        case RenderPrimitiveTopology::LINE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case RenderPrimitiveTopology::TRIANGLE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        }
    }

    static D3D11_FILTER toD3D11Filter(const RenderSamplerDesc &desc) {
        if (desc.anisotropyEnabled) {
            return desc.comparisonEnabled ? D3D11_FILTER_COMPARISON_ANISOTROPIC : D3D11_FILTER_ANISOTROPIC;
        }

        uint32_t filter = 0;
        filter |= (desc.mipmapMode == RenderMipmapMode::LINEAR) ? 0x1 : 0x0;
        filter |= (desc.magFilter == RenderFilter::LINEAR) ? 0x4 : 0x0;
        filter |= (desc.minFilter == RenderFilter::LINEAR) ? 0x10 : 0x0;
        filter |= desc.comparisonEnabled ? 0x80 : 0x0;
        return D3D11_FILTER(filter);
    }

    static std::string toUTF8(const wchar_t *text) {
        const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
        std::string result(size_t(std::max(size, 1)), '\0');
        WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), size, nullptr, nullptr);
        result.resize(strlen(result.c_str()));
        return result;
    }

    struct D3D11Device;

    // Resources.

    struct D3D11Buffer : RenderBuffer {
        D3D11Device *device = nullptr;
        RenderBufferDesc desc;
        ComPtr<ID3D11Buffer> buffer;
        bool constantBuffer = false;

        // Upload, readback and constant buffers: the bytes the CPU sees. Upload buffers send written ranges to the GPU copy
        // on unmap; constant buffers are always sent whole (D3D11.0 cannot update part of one).
        std::vector<uint8_t> shadow;

        D3D11Buffer(D3D11Device *device, const RenderBufferDesc &desc);
        void *map(uint32_t subresource, const RenderRange *readRange) override;
        void unmap(uint32_t subresource, const RenderRange *writtenRange) override;
        std::unique_ptr<RenderBufferFormattedView> createBufferFormattedView(RenderFormat format) override;
        void setName(const std::string &name) override { }
        uint64_t getDeviceAddress() const override { return 0; }
        bool isUpload() const { return (desc.heapType == RenderHeapType::UPLOAD) || (desc.heapType == RenderHeapType::GPU_UPLOAD); }
    };

    struct D3D11BufferFormattedView : RenderBufferFormattedView {
        RenderFormat format = RenderFormat::UNKNOWN;
        ComPtr<ID3D11ShaderResourceView> srv;
        ComPtr<ID3D11UnorderedAccessView> uav;

        D3D11BufferFormattedView(D3D11Buffer *buffer, RenderFormat format);
    };

    struct D3D11Texture : RenderTexture {
        D3D11Device *device = nullptr;
        RenderTextureDesc desc;
        D3D11Formats formats;
        ComPtr<ID3D11Resource> resource;
        ComPtr<ID3D11ShaderResourceView> srv;
        ComPtr<ID3D11UnorderedAccessView> uav;
        ComPtr<ID3D11RenderTargetView> rtv;
        ComPtr<ID3D11DepthStencilView> dsv;

        // Bound while a shader reads the same depth buffer (RT64's depth-read state); D3D11 allows that only read-only.
        ComPtr<ID3D11DepthStencilView> readOnlyDsv;

        D3D11Texture(D3D11Device *device, const RenderTextureDesc &desc);
        std::unique_ptr<RenderTextureView> createTextureView(const RenderTextureViewDesc &desc) const override;
        void setName(const std::string &name) override { }
        uint32_t mipLevels() const { return std::max(desc.mipLevels, 1U); }

        tk64::ImageDimension dimension() const {
            switch (desc.dimension) {
            case RenderTextureDimension::TEXTURE_1D: return tk64::ImageDimension::Texture1D;
            case RenderTextureDimension::TEXTURE_3D: return tk64::ImageDimension::Texture3D;
            default: return ((desc.flags & RenderTextureFlag::CUBE) != 0) ? tk64::ImageDimension::Cube : tk64::ImageDimension::Texture2D;
            }
        }
    };

    struct D3D11TextureView : RenderTextureView {
        const D3D11Texture *texture = nullptr;
        ComPtr<ID3D11ShaderResourceView> srv;
        tk64::ImageDimension dimension = tk64::ImageDimension::None;
    };

    struct D3D11Sampler : RenderSampler {
        ComPtr<ID3D11SamplerState> state;
    };

    // A shader is translated and compiled when it is created; pipelines copy what they need, since RT64 may destroy the
    // shader objects right after creating a pipeline.
    struct D3D11Shader : RenderShader {
        tk64::TranslatedShader translated;
        std::vector<uint8_t> dxbc;
        ComPtr<ID3D11VertexShader> vertexShader;
        ComPtr<ID3D11PixelShader> pixelShader;
        ComPtr<ID3D11ComputeShader> computeShader;
        bool valid = false;

        void setName(const std::string &name) override { }
    };

    struct D3D11PipelineLayout : RenderPipelineLayout {
        struct SetLayout {
            std::vector<RenderDescriptorRange> ranges;
            std::vector<uint32_t> rangeStart;
            std::vector<std::vector<const RenderSampler *>> immutableSamplers;
        };

        std::vector<RenderPushConstantRange> pushConstantRanges;
        std::vector<SetLayout> setLayouts;

        D3D11PipelineLayout(const RenderPipelineLayoutDesc &desc);
        bool findDescriptor(uint32_t set, uint32_t binding, uint32_t arrayIndex, uint32_t &descriptorIndex, const RenderSampler **immutableSampler) const;
    };

    struct D3D11DescriptorSet : RenderDescriptorSet {
        // Entries hold references to the D3D11 objects, not to the plume objects: RT64 may destroy a texture while a set
        // still names it, and like a D3D12 descriptor the entry must stay a valid (if stale) view until it is rewritten.
        struct Entry {
            ComPtr<ID3D11Buffer> buffer;
            uint64_t bufferTotalSize = 0;
            uint64_t bufferSize = 0;
            uint32_t structureStride = 0;
            uint32_t firstElement = 0;
            bool formatted = false;
            ComPtr<ID3D11Resource> texture;
            ComPtr<ID3D11ShaderResourceView> srv;
            ComPtr<ID3D11UnorderedAccessView> uav;
            tk64::ImageDimension dimension = tk64::ImageDimension::None;
            ComPtr<ID3D11SamplerState> sampler;

            // Raw buffer views, made on first use.
            mutable ComPtr<ID3D11ShaderResourceView> rawSRV;
            mutable ComPtr<ID3D11UnorderedAccessView> rawUAV;
        };

        std::vector<Entry> entries;

        D3D11DescriptorSet(const RenderDescriptorSetDesc &desc);
        void setBuffer(uint32_t descriptorIndex, const RenderBuffer *buffer, uint64_t bufferSize, const RenderBufferStructuredView *bufferStructuredView, const RenderBufferFormattedView *bufferFormattedView) override;
        void setTexture(uint32_t descriptorIndex, const RenderTexture *texture, RenderTextureLayout textureLayout, const RenderTextureView *textureView) override;
        void setSampler(uint32_t descriptorIndex, const RenderSampler *sampler) override;
        void setAccelerationStructure(uint32_t descriptorIndex, const RenderAccelerationStructure *accelerationStructure) override { }
    };

    // What a pipeline needs from one of its shaders to bind it.
    struct D3D11StageBindings {
        std::vector<tk64::BoundResource> resources;
        uint32_t pushConstantRegister = UINT32_MAX;
        uint32_t pushConstantSize = 0;

        void assign(const tk64::TranslatedShader &translated) {
            resources = translated.resources;
            pushConstantRegister = translated.pushConstantRegister;
            pushConstantSize = translated.pushConstantSize;
        }
    };

    struct D3D11Pipeline : RenderPipeline {
        enum class Type {
            Graphics,
            Compute
        };

        Type type = Type::Graphics;
        bool valid = false;
        const D3D11PipelineLayout *layout = nullptr;
        ComPtr<ID3D11VertexShader> vertexShader;
        ComPtr<ID3D11PixelShader> pixelShader;
        ComPtr<ID3D11ComputeShader> computeShader;
        D3D11StageBindings vertexBindings;
        D3D11StageBindings pixelBindings;
        D3D11StageBindings computeBindings;
        ComPtr<ID3D11InputLayout> inputLayout;
        ComPtr<ID3D11RasterizerState> rasterizerState;
        ComPtr<ID3D11DepthStencilState> depthStencilState;
        ComPtr<ID3D11BlendState> blendState;
        D3D11_RASTERIZER_DESC rasterizerDesc = {};
        D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        uint32_t stencilReference = 0;

        D3D11Pipeline(D3D11Device *device, const RenderGraphicsPipelineDesc &desc);
        D3D11Pipeline(D3D11Device *device, const RenderComputePipelineDesc &desc);
        void setName(const std::string &name) override { }
        RenderPipelineProgram getProgram(const std::string &name) const override { return RenderPipelineProgram(); }
    };

    struct D3D11Framebuffer : RenderFramebuffer {
        // The views bound for drawing: the colour targets themselves, or their proxies.
        std::vector<ComPtr<ID3D11RenderTargetView>> renderTargets;
        ComPtr<ID3D11Resource> depthResource;
        ComPtr<ID3D11DepthStencilView> depthStencil;
        ComPtr<ID3D11DepthStencilView> readOnlyDepthStencil;
        uint32_t width = 0;
        uint32_t height = 0;

        // D3D11 needs every view of a framebuffer to have the same size, while RT64 pairs an N64 colour image with a depth
        // buffer of another size (D3D12 and Vulkan draw in the common area). When the sizes differ, drawing goes to proxy
        // colour targets of the depth buffer's size; the colour targets are copied into them when the framebuffer is
        // bound and back when it stops being bound.
        std::vector<ComPtr<ID3D11Resource>> colorResources;
        std::vector<std::unique_ptr<D3D11Texture>> proxies;

        D3D11Framebuffer(D3D11Device *device, const RenderFramebufferDesc &desc);
        uint32_t getWidth() const override { return width; }
        uint32_t getHeight() const override { return height; }
    };

    struct D3D11CommandFence : RenderCommandFence {
    };

    struct D3D11CommandSemaphore : RenderCommandSemaphore {
    };

    struct D3D11QueryPool : RenderQueryPool {
        std::vector<uint64_t> results;

        D3D11QueryPool(uint32_t queryCount) : results(queryCount, 0) { }
        void queryResults() override { }
        const uint64_t *getResults() const override { return results.data(); }
        uint32_t getCount() const override { return uint32_t(results.size()); }
    };

    enum class D3D11StageKind {
        Vertex,
        Pixel,
        Compute,
        Count
    };

    // State kept while command lists are replayed on the immediate context.
    struct D3D11ReplayState {
        D3D11Device *device = nullptr;
        ID3D11DeviceContext *context = nullptr;
        const D3D11Pipeline *pipeline = nullptr;
        const D3D11PipelineLayout *graphicsLayout = nullptr;
        const D3D11PipelineLayout *computeLayout = nullptr;
        const D3D11DescriptorSet *graphicsSets[8] = {};
        const D3D11DescriptorSet *computeSets[8] = {};
        uint8_t graphicsPushConstants[256] = {};
        uint8_t computePushConstants[256] = {};
        const D3D11Framebuffer *framebuffer = nullptr;
        bool framebufferBound = false;
        bool depthReadOnly = false;
        bool graphicsStateBound = false;
        D3D11_VIEWPORT viewport = {};
        D3D11_RECT scissor = {};

        // Highest register + 1 bound per stage, so stale bindings past the current shader's are cleared.
        uint32_t shaderResourceCount[size_t(D3D11StageKind::Count)] = {};
        uint32_t samplerCount[size_t(D3D11StageKind::Count)] = {};
        uint32_t constantBufferCount[size_t(D3D11StageKind::Count)] = {};
    };

    typedef std::function<void(D3D11ReplayState &)> D3D11Command;

    struct D3D11CommandList : RenderCommandList {
        D3D11Device *device = nullptr;
        std::shared_ptr<std::vector<D3D11Command>> commands;
        const D3D11PipelineLayout *recordGraphicsLayout = nullptr;
        const D3D11PipelineLayout *recordComputeLayout = nullptr;

        // Whether the list has set a viewport or scissor since begin(); see setFramebuffer.
        bool recordViewportSet = false;
        bool recordScissorSet = false;

        D3D11CommandList(D3D11Device *device);
        void record(D3D11Command command) { commands->emplace_back(std::move(command)); }
        void begin() override;
        void end() override { }
        void barriers(RenderBarrierStages stages, const RenderBufferBarrier *bufferBarriers, uint32_t bufferBarriersCount, const RenderTextureBarrier *textureBarriers, uint32_t textureBarriersCount) override { }
        void dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) override;
        void traceRays(uint32_t width, uint32_t height, uint32_t depth, RenderBufferReference shaderBindingTable, const RenderShaderBindingGroupsInfo &shaderBindingGroupsInfo) override { }
        void drawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) override;
        void drawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation) override;
        void setPipeline(const RenderPipeline *pipeline) override;
        void setComputePipelineLayout(const RenderPipelineLayout *pipelineLayout) override;
        void setComputePushConstants(uint32_t rangeIndex, const void *data, uint32_t offset, uint32_t size) override;
        void setComputeDescriptorSet(RenderDescriptorSet *descriptorSet, uint32_t setIndex) override;
        void setGraphicsPipelineLayout(const RenderPipelineLayout *pipelineLayout) override;
        void setGraphicsPushConstants(uint32_t rangeIndex, const void *data, uint32_t offset, uint32_t size) override;
        void setGraphicsDescriptorSet(RenderDescriptorSet *descriptorSet, uint32_t setIndex) override;
        void setGraphicsRootDescriptor(RenderBufferReference bufferReference, uint32_t rootDescriptorIndex) override { }
        void setRaytracingPipelineLayout(const RenderPipelineLayout *pipelineLayout) override { }
        void setRaytracingPushConstants(uint32_t rangeIndex, const void *data, uint32_t offset, uint32_t size) override { }
        void setRaytracingDescriptorSet(RenderDescriptorSet *descriptorSet, uint32_t setIndex) override { }
        void setIndexBuffer(const RenderIndexBufferView *view) override;
        void setVertexBuffers(uint32_t startSlot, const RenderVertexBufferView *views, uint32_t viewCount, const RenderInputSlot *inputSlots) override;
        void setViewports(const RenderViewport *viewports, uint32_t count) override;
        void setScissors(const RenderRect *scissorRects, uint32_t count) override;
        void setFramebuffer(const RenderFramebuffer *framebuffer) override;
        void setDepthBias(float depthBias, float depthBiasClamp, float slopeScaledDepthBias) override;
        void clearColor(uint32_t attachmentIndex, RenderColor colorValue, const RenderRect *clearRects, uint32_t clearRectsCount) override;
        void clearDepthStencil(bool clearDepth, bool clearStencil, float depthValue, uint32_t stencilValue, const RenderRect *clearRects, uint32_t clearRectsCount) override;
        void copyBufferRegion(RenderBufferReference dstBuffer, RenderBufferReference srcBuffer, uint64_t size) override;
        void copyTextureRegion(const RenderTextureCopyLocation &dstLocation, const RenderTextureCopyLocation &srcLocation, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const RenderBox *srcBox) override;
        void copyBuffer(const RenderBuffer *dstBuffer, const RenderBuffer *srcBuffer) override;
        void copyTexture(const RenderTexture *dstTexture, const RenderTexture *srcTexture) override;
        void resolveTexture(const RenderTexture *dstTexture, const RenderTexture *srcTexture) override;
        void resolveTextureRegion(const RenderTexture *dstTexture, uint32_t dstX, uint32_t dstY, const RenderTexture *srcTexture, const RenderRect *srcRect, RenderResolveMode resolveMode) override;
        void buildBottomLevelAS(const RenderAccelerationStructure *dstAccelerationStructure, RenderBufferReference scratchBuffer, const RenderBottomLevelASBuildInfo &buildInfo) override { }
        void buildTopLevelAS(const RenderAccelerationStructure *dstAccelerationStructure, RenderBufferReference scratchBuffer, RenderBufferReference instancesBuffer, const RenderTopLevelASBuildInfo &buildInfo) override { }
        void discardTexture(const RenderTexture *texture) override { }
        void resetQueryPool(const RenderQueryPool *queryPool, uint32_t queryFirstIndex, uint32_t queryCount) override { }
        void writeTimestamp(const RenderQueryPool *queryPool, uint32_t queryIndex) override { }
    };

    struct D3D11CommandQueue : RenderCommandQueue {
        D3D11Device *device = nullptr;

        D3D11CommandQueue(D3D11Device *device) : device(device) { }
        std::unique_ptr<RenderCommandList> createCommandList() override;
        std::unique_ptr<RenderSwapChain> createSwapChain(const RenderSwapChainDesc &desc) override;
        void executeCommandLists(const RenderCommandList **commandLists, uint32_t commandListCount, RenderCommandSemaphore **waitSemaphores, uint32_t waitSemaphoreCount, RenderCommandSemaphore **signalSemaphores, uint32_t signalSemaphoreCount, RenderCommandFence *signalFence) override;
        void waitForCommandFence(RenderCommandFence *fence) override { }
    };

    struct D3D11SwapChain : RenderSwapChain {
        D3D11Device *device = nullptr;
        RenderSwapChainDesc desc;
        ComPtr<IDXGISwapChain> swapChain;
        ComPtr<ID3D11Resource> backBuffer;
        std::vector<std::unique_ptr<D3D11Texture>> textures;
        UINT swapChainFlags = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t nextIndex = 0;
        bool vsyncEnabled = true;

        D3D11SwapChain(D3D11Device *device, const RenderSwapChainDesc &desc);
        void queryWindowSize(uint32_t &w, uint32_t &h) const;
        void createTextures();
        bool present(uint32_t textureIndex, RenderCommandSemaphore **waitSemaphores, uint32_t waitSemaphoreCount) override;
        void wait() override { }
        bool resize() override;
        bool needsResize() const override;
        void setVsyncEnabled(bool vsyncEnabled) override { this->vsyncEnabled = vsyncEnabled; }
        bool isVsyncEnabled() const override { return vsyncEnabled; }
        uint32_t getWidth() const override { return width; }
        uint32_t getHeight() const override { return height; }
        RenderTexture *getTexture(uint32_t textureIndex) override { return textures[textureIndex].get(); }
        uint32_t getTextureCount() const override { return uint32_t(textures.size()); }
        bool acquireTexture(RenderCommandSemaphore *signalSemaphore, uint32_t *textureIndex) override;
        RenderWindow getWindow() const override { return desc.renderWindow; }
        bool isEmpty() const override { return (swapChain == nullptr) || (width == 0) || (height == 0); }
        uint32_t getRefreshRate() const override { return 0; }
    };

    struct D3D11Pool : RenderPool {
        D3D11Device *device = nullptr;

        D3D11Pool(D3D11Device *device) : device(device) { }
        std::unique_ptr<RenderBuffer> createBuffer(const RenderBufferDesc &desc) override;
        std::unique_ptr<RenderTexture> createTexture(const RenderTextureDesc &desc) override;
    };

    struct D3D11Device : RenderDevice {
        ComPtr<IDXGIFactory1> factory;
        ComPtr<ID3D11Device> d3d;
        ComPtr<ID3D11DeviceContext> context;
        ComPtr<ID3D11DeviceContext1> context1;
        ComPtr<ID3D11InfoQueue> infoQueue;
        std::mutex contextMutex;
        tk64::HLSLCompiler compiler;
        RenderDeviceCapabilities capabilities;
        RenderDeviceDescription description;

        // One dynamic constant buffer per stage carries the push constants.
        ComPtr<ID3D11Buffer> pushConstantBuffers[size_t(D3D11StageKind::Count)];

        // Clears limited to rectangles that D3D11.0 has no call for (depth) are drawn with this pipeline.
        ComPtr<ID3D11VertexShader> clearVertexShader;
        ComPtr<ID3D11PixelShader> clearPixelShader;
        ComPtr<ID3D11Buffer> clearConstants;
        ComPtr<ID3D11DepthStencilState> clearDepthState;
        ComPtr<ID3D11BlendState> clearBlendState;
        ComPtr<ID3D11RasterizerState> clearRasterizerState;

        bool create(IDXGIAdapter1 *adapter);
        void createClearPipeline();
        void printDebugMessages();
        std::unique_ptr<RenderDescriptorSet> createDescriptorSet(const RenderDescriptorSetDesc &desc) override { return std::make_unique<D3D11DescriptorSet>(desc); }
        std::unique_ptr<RenderShader> createShader(const void *data, uint64_t size, const char *entryPointName, RenderShaderFormat format) override;
        std::unique_ptr<RenderSampler> createSampler(const RenderSamplerDesc &desc) override;
        std::unique_ptr<RenderPipeline> createComputePipeline(const RenderComputePipelineDesc &desc) override { return std::make_unique<D3D11Pipeline>(this, desc); }
        std::unique_ptr<RenderPipeline> createGraphicsPipeline(const RenderGraphicsPipelineDesc &desc) override { return std::make_unique<D3D11Pipeline>(this, desc); }
        std::unique_ptr<RenderPipeline> createRaytracingPipeline(const RenderRaytracingPipelineDesc &desc, const RenderPipeline *previousPipeline) override { return nullptr; }
        std::unique_ptr<RenderCommandQueue> createCommandQueue(RenderCommandListType type) override { return std::make_unique<D3D11CommandQueue>(this); }
        std::unique_ptr<RenderBuffer> createBuffer(const RenderBufferDesc &desc) override { return std::make_unique<D3D11Buffer>(this, desc); }
        std::unique_ptr<RenderTexture> createTexture(const RenderTextureDesc &desc) override { return std::make_unique<D3D11Texture>(this, desc); }
        std::unique_ptr<RenderAccelerationStructure> createAccelerationStructure(const RenderAccelerationStructureDesc &desc) override { return nullptr; }
        std::unique_ptr<RenderPool> createPool(const RenderPoolDesc &desc) override { return std::make_unique<D3D11Pool>(this); }
        std::unique_ptr<RenderPipelineLayout> createPipelineLayout(const RenderPipelineLayoutDesc &desc) override { return std::make_unique<D3D11PipelineLayout>(desc); }
        std::unique_ptr<RenderCommandFence> createCommandFence() override { return std::make_unique<D3D11CommandFence>(); }
        std::unique_ptr<RenderCommandSemaphore> createCommandSemaphore() override { return std::make_unique<D3D11CommandSemaphore>(); }
        std::unique_ptr<RenderFramebuffer> createFramebuffer(const RenderFramebufferDesc &desc) override { return std::make_unique<D3D11Framebuffer>(this, desc); }
        std::unique_ptr<RenderQueryPool> createQueryPool(uint32_t queryCount) override { return std::make_unique<D3D11QueryPool>(queryCount); }
        void setBottomLevelASBuildInfo(RenderBottomLevelASBuildInfo &buildInfo, const RenderBottomLevelASMesh *meshes, uint32_t meshCount, bool preferFastBuild, bool preferFastTrace) override { }
        void setTopLevelASBuildInfo(RenderTopLevelASBuildInfo &buildInfo, const RenderTopLevelASInstance *instances, uint32_t instanceCount, bool preferFastBuild, bool preferFastTrace) override { }
        void setShaderBindingTableInfo(RenderShaderBindingTableInfo &tableInfo, const RenderShaderBindingGroups &groups, const RenderPipeline *pipeline, RenderDescriptorSet **descriptorSets, uint32_t descriptorSetCount) override { }
        const RenderDeviceCapabilities &getCapabilities() const override { return capabilities; }
        const RenderDeviceDescription &getDescription() const override { return description; }
        RenderSampleCounts getSampleCountsSupported(RenderFormat format) const override { return RenderSampleCount::COUNT_1; }
        bool beginCapture() override { return false; }
        bool endCapture() override { return false; }
    };

    // D3D11Device

    bool D3D11Device::create(IDXGIAdapter1 *adapter) {
        const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
        const bool debugLayer = (getenv("TK64_D3D11_DEBUG") != nullptr);
        UINT flags = debugLayer ? D3D11_CREATE_DEVICE_DEBUG : 0;
        const D3D_DRIVER_TYPE driverType = (adapter != nullptr) ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        HRESULT result = E_FAIL;
        for (int attempt = 0; attempt < 2; attempt++) {
            // A runtime without 11.1 rejects the 11_1 entry; retry with 11_0 alone.
            const UINT levelCount = (attempt == 0) ? 2 : 1;
            const D3D_FEATURE_LEVEL *levels = (attempt == 0) ? featureLevels : (featureLevels + 1);
            result = D3D11CreateDevice(adapter, driverType, nullptr, flags, levels, levelCount, D3D11_SDK_VERSION, d3d.GetAddressOf(), &featureLevel, context.GetAddressOf());
            if (FAILED(result) && debugLayer) {
                // The debug layer needs Windows' optional Graphics Tools.
                fprintf(stderr, "plume_d3d11: the debug layer is not available; continuing without it.\n");
                flags &= ~D3D11_CREATE_DEVICE_DEBUG;
                result = D3D11CreateDevice(adapter, driverType, nullptr, flags, levels, levelCount, D3D11_SDK_VERSION, d3d.GetAddressOf(), &featureLevel, context.GetAddressOf());
            }

            if (SUCCEEDED(result)) {
                break;
            }
        }

        if (FAILED(result)) {
            fprintf(stderr, "plume_d3d11: D3D11CreateDevice failed (0x%08lX).\n", (unsigned long)(result));
            return false;
        }

        context.As(&context1);
        if ((flags & D3D11_CREATE_DEVICE_DEBUG) != 0) {
            d3d.As(&infoQueue);
        }

        if (!compiler.open()) {
            fprintf(stderr, "plume_d3d11: d3dcompiler_47.dll is missing; only cached shaders can be used.\n");
        }

        for (size_t s = 0; s < size_t(D3D11StageKind::Count); s++) {
            D3D11_BUFFER_DESC bufferDesc = {};
            bufferDesc.ByteWidth = 256;
            bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            d3d->CreateBuffer(&bufferDesc, nullptr, pushConstantBuffers[s].GetAddressOf());
        }

        createClearPipeline();

        DXGI_ADAPTER_DESC1 adapterDesc = {};
        ComPtr<IDXGIDevice> dxgiDevice;
        ComPtr<IDXGIAdapter> dxgiAdapter;
        if (SUCCEEDED(d3d.As(&dxgiDevice)) && SUCCEEDED(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()))) {
            ComPtr<IDXGIAdapter1> dxgiAdapter1;
            if (SUCCEEDED(dxgiAdapter.As(&dxgiAdapter1))) {
                dxgiAdapter1->GetDesc1(&adapterDesc);
            }

            dxgiAdapter->GetParent(IID_PPV_ARGS(factory.GetAddressOf()));
        }

        description.name = toUTF8(adapterDesc.Description) + ((featureLevel == D3D_FEATURE_LEVEL_11_1) ? " (Direct3D 11, FL 11_1)" : " (Direct3D 11, FL 11_0)");
        description.dedicatedVideoMemory = adapterDesc.DedicatedVideoMemory;
        description.type = RenderDeviceType::UNKNOWN;
        switch (adapterDesc.VendorId) {
        case 0x10DE: description.vendor = RenderDeviceVendor::NVIDIA; break;
        case 0x1002: description.vendor = RenderDeviceVendor::AMD; break;
        case 0x8086: description.vendor = RenderDeviceVendor::INTEL; break;
        default: break;
        }

        capabilities.maxTextureSize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        capabilities.descriptorIndexing = false;
        capabilities.triangleFan = false;
        fprintf(stdout, "plume_d3d11: %s\n", description.name.c_str());
        return true;
    }

    // A full-screen triangle whose pixel shader writes a constant colour and depth; the scissor limits it to one rectangle.
    static const char ClearShaderSource[] =
        "cbuffer ClearParams : register(b0) { float4 clearColor; float clearDepth; };\n"
        "float4 vsMain(uint id : SV_VertexID) : SV_Position {\n"
        "    float2 uv = float2((id << 1) & 2, id & 2);\n"
        "    return float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), clearDepth, 1.0);\n"
        "}\n"
        "float4 psMain(float4 position : SV_Position, out float depth : SV_Depth) : SV_Target0 {\n"
        "    depth = clearDepth;\n"
        "    return clearColor;\n"
        "}\n";

    void D3D11Device::createClearPipeline() {
        std::vector<uint8_t> vertexCode;
        std::vector<uint8_t> pixelCode;
        std::string errors;
        if (!compiler.compileRaw(ClearShaderSource, "vsMain", "vs_5_0", vertexCode, errors) || !compiler.compileRaw(ClearShaderSource, "psMain", "ps_5_0", pixelCode, errors)) {
            fprintf(stderr, "plume_d3d11: the clear shaders failed to compile:\n%s\n", errors.c_str());
            return;
        }

        d3d->CreateVertexShader(vertexCode.data(), vertexCode.size(), nullptr, clearVertexShader.GetAddressOf());
        d3d->CreatePixelShader(pixelCode.data(), pixelCode.size(), nullptr, clearPixelShader.GetAddressOf());

        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.ByteWidth = 32;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        d3d->CreateBuffer(&bufferDesc, nullptr, clearConstants.GetAddressOf());

        D3D11_DEPTH_STENCIL_DESC depthDesc = {};
        depthDesc.DepthEnable = TRUE;
        depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        d3d->CreateDepthStencilState(&depthDesc, clearDepthState.GetAddressOf());

        D3D11_BLEND_DESC blendDesc = {};
        for (D3D11_RENDER_TARGET_BLEND_DESC &target : blendDesc.RenderTarget) {
            target.RenderTargetWriteMask = 0;
        }

        d3d->CreateBlendState(&blendDesc, clearBlendState.GetAddressOf());

        D3D11_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        rasterizerDesc.DepthClipEnable = FALSE;
        rasterizerDesc.ScissorEnable = TRUE;
        d3d->CreateRasterizerState(&rasterizerDesc, clearRasterizerState.GetAddressOf());
    }

    void D3D11Device::printDebugMessages() {
        if (infoQueue == nullptr) {
            return;
        }

        const UINT64 count = infoQueue->GetNumStoredMessages();
        for (UINT64 i = 0; i < count; i++) {
            SIZE_T length = 0;
            infoQueue->GetMessage(i, nullptr, &length);
            std::vector<uint8_t> storage(length);
            D3D11_MESSAGE *message = reinterpret_cast<D3D11_MESSAGE *>(storage.data());
            if (SUCCEEDED(infoQueue->GetMessage(i, message, &length)) && (message->Severity <= D3D11_MESSAGE_SEVERITY_WARNING)) {
                fprintf(stderr, "plume_d3d11: %.*s\n", int(message->DescriptionByteLength), message->pDescription);
            }
        }

        infoQueue->ClearStoredMessages();
    }

    std::unique_ptr<RenderShader> D3D11Device::createShader(const void *data, uint64_t size, const char *entryPointName, RenderShaderFormat format) {
        assert((format == RenderShaderFormat::SPIRV) && "The Direct3D 11 backend only accepts SPIR-V.");
        std::unique_ptr<D3D11Shader> shader = std::make_unique<D3D11Shader>();
        std::vector<uint32_t> words(size_t(size / sizeof(uint32_t)));
        memcpy(words.data(), data, words.size() * sizeof(uint32_t));
        shader->translated = tk64::translateSpirv(words.data(), words.size(), tk64::ShaderLanguage::HLSL_50);
        if (!shader->translated.ok) {
            fprintf(stderr, "plume_d3d11: shader translation failed: %s\n", shader->translated.error.c_str());
            return shader;
        }

        std::string errors;
        if (!compiler.compile(shader->translated.source, shader->translated.stage, shader->dxbc, errors)) {
            fprintf(stderr, "plume_d3d11: shader compilation failed:\n%s\n", errors.c_str());
            return shader;
        }

        HRESULT result = E_FAIL;
        switch (shader->translated.stage) {
        case tk64::ShaderStage::Vertex:
            result = d3d->CreateVertexShader(shader->dxbc.data(), shader->dxbc.size(), nullptr, shader->vertexShader.GetAddressOf());
            break;
        case tk64::ShaderStage::Pixel:
            result = d3d->CreatePixelShader(shader->dxbc.data(), shader->dxbc.size(), nullptr, shader->pixelShader.GetAddressOf());
            break;
        case tk64::ShaderStage::Compute:
            result = d3d->CreateComputeShader(shader->dxbc.data(), shader->dxbc.size(), nullptr, shader->computeShader.GetAddressOf());
            break;
        default:
            break;
        }

        shader->valid = SUCCEEDED(result);
        if (!shader->valid) {
            fprintf(stderr, "plume_d3d11: creating a shader failed (0x%08lX).\n", (unsigned long)(result));
        }

        return shader;
    }

    std::unique_ptr<RenderSampler> D3D11Device::createSampler(const RenderSamplerDesc &desc) {
        std::unique_ptr<D3D11Sampler> sampler = std::make_unique<D3D11Sampler>();
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = toD3D11Filter(desc);
        samplerDesc.AddressU = toD3D11(desc.addressU);
        samplerDesc.AddressV = toD3D11(desc.addressV);
        samplerDesc.AddressW = toD3D11(desc.addressW);
        samplerDesc.MipLODBias = desc.mipLODBias;
        samplerDesc.MaxAnisotropy = desc.anisotropyEnabled ? std::clamp(desc.maxAnisotropy, 1U, 16U) : 1;
        samplerDesc.ComparisonFunc = desc.comparisonEnabled ? toD3D11(desc.comparisonFunc) : D3D11_COMPARISON_NEVER;
        samplerDesc.MinLOD = desc.minLOD;
        samplerDesc.MaxLOD = desc.maxLOD;
        const float alpha = (desc.borderColor == RenderBorderColor::TRANSPARENT_BLACK) ? 0.0f : 1.0f;
        const float color = (desc.borderColor == RenderBorderColor::OPAQUE_WHITE) ? 1.0f : 0.0f;
        samplerDesc.BorderColor[0] = color;
        samplerDesc.BorderColor[1] = color;
        samplerDesc.BorderColor[2] = color;
        samplerDesc.BorderColor[3] = alpha;
        d3d->CreateSamplerState(&samplerDesc, sampler->state.GetAddressOf());
        return sampler;
    }

    // D3D11Pool

    std::unique_ptr<RenderBuffer> D3D11Pool::createBuffer(const RenderBufferDesc &desc) {
        return device->createBuffer(desc);
    }

    std::unique_ptr<RenderTexture> D3D11Pool::createTexture(const RenderTextureDesc &desc) {
        return device->createTexture(desc);
    }

    // D3D11Buffer

    D3D11Buffer::D3D11Buffer(D3D11Device *device, const RenderBufferDesc &desc) : device(device), desc(desc) {
        const RenderBufferFlags flags = desc.flags;
        constantBuffer = (flags & RenderBufferFlag::CONSTANT) != 0;
        if (isUpload() || (desc.heapType == RenderHeapType::READBACK) || constantBuffer) {
            shadow.resize(size_t(desc.size), 0);
        }

        // A readback buffer is only written by copies, which land in its CPU copy; an upload buffer that is never bound is
        // only read by copies, which use its CPU copy. Neither needs a GPU buffer.
        const RenderBufferFlags bindingFlags = RenderBufferFlag::VERTEX | RenderBufferFlag::INDEX | RenderBufferFlag::STORAGE | RenderBufferFlag::CONSTANT |
            RenderBufferFlag::FORMATTED | RenderBufferFlag::UNORDERED_ACCESS;
        if ((desc.heapType == RenderHeapType::READBACK) || (isUpload() && ((flags & bindingFlags) == 0))) {
            return;
        }

        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.ByteWidth = UINT(roundUp(std::max(desc.size, uint64_t(16)), 16));
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        if (constantBuffer) {
            // D3D11 does not allow a constant buffer to be bound any other way.
            bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        }
        else {
            bufferDesc.BindFlags |= ((flags & RenderBufferFlag::VERTEX) != 0) ? D3D11_BIND_VERTEX_BUFFER : 0;
            bufferDesc.BindFlags |= ((flags & RenderBufferFlag::INDEX) != 0) ? D3D11_BIND_INDEX_BUFFER : 0;
            bufferDesc.BindFlags |= ((flags & (RenderBufferFlag::STORAGE | RenderBufferFlag::FORMATTED)) != 0) ? D3D11_BIND_SHADER_RESOURCE : 0;
            bufferDesc.BindFlags |= ((flags & RenderBufferFlag::UNORDERED_ACCESS) != 0) ? D3D11_BIND_UNORDERED_ACCESS : 0;
            if (((flags & RenderBufferFlag::STORAGE) != 0) && ((bufferDesc.BindFlags & (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS)) != 0)) {
                // The translated shaders read and write storage buffers as (RW)ByteAddressBuffer.
                bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
            }
        }

        const HRESULT result = device->d3d->CreateBuffer(&bufferDesc, nullptr, buffer.GetAddressOf());
        if (FAILED(result)) {
            fprintf(stderr, "plume_d3d11: creating a %llu-byte buffer failed (0x%08lX).\n", (unsigned long long)(desc.size), (unsigned long)(result));
        }
    }

    void *D3D11Buffer::map(uint32_t subresource, const RenderRange *readRange) {
        assert(!shadow.empty() && "Only upload, readback and constant buffers can be mapped.");
        return shadow.data();
    }

    void D3D11Buffer::unmap(uint32_t subresource, const RenderRange *writtenRange) {
        if (!isUpload() || (buffer == nullptr)) {
            return;
        }

        uint64_t begin = 0;
        uint64_t end = desc.size;
        if ((writtenRange != nullptr) && (writtenRange->end > writtenRange->begin)) {
            begin = writtenRange->begin;
            end = std::min(writtenRange->end, desc.size);
        }

        if (end <= begin) {
            return;
        }

        // UpdateSubresource copies the bytes now, so the CPU copy can be written again right away.
        std::unique_lock<std::mutex> lock(device->contextMutex);
        if (constantBuffer) {
            device->context->UpdateSubresource(buffer.Get(), 0, nullptr, shadow.data(), 0, 0);
        }
        else {
            const D3D11_BOX box = { UINT(begin), 0, 0, UINT(end), 1, 1 };
            device->context->UpdateSubresource(buffer.Get(), 0, &box, shadow.data() + begin, 0, 0);
        }
    }

    std::unique_ptr<RenderBufferFormattedView> D3D11Buffer::createBufferFormattedView(RenderFormat format) {
        return std::make_unique<D3D11BufferFormattedView>(this, format);
    }

    // D3D11BufferFormattedView

    D3D11BufferFormattedView::D3D11BufferFormattedView(D3D11Buffer *buffer, RenderFormat format) : format(format) {
        if (buffer->buffer == nullptr) {
            return;
        }

        D3D11_BUFFER_DESC bufferDesc = {};
        buffer->buffer->GetDesc(&bufferDesc);
        const UINT elementCount = UINT(buffer->desc.size / std::max(RenderFormatSize(format), 1U));
        if ((bufferDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0) {
            D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
            viewDesc.Format = toDXGI(format);
            viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            viewDesc.Buffer.FirstElement = 0;
            viewDesc.Buffer.NumElements = elementCount;
            buffer->device->d3d->CreateShaderResourceView(buffer->buffer.Get(), &viewDesc, srv.GetAddressOf());
        }

        if ((bufferDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) != 0) {
            D3D11_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
            viewDesc.Format = toDXGI(format);
            viewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            viewDesc.Buffer.FirstElement = 0;
            viewDesc.Buffer.NumElements = elementCount;
            buffer->device->d3d->CreateUnorderedAccessView(buffer->buffer.Get(), &viewDesc, uav.GetAddressOf());
        }
    }

    // D3D11Texture

    D3D11Texture::D3D11Texture(D3D11Device *device, const RenderTextureDesc &desc) : device(device), desc(desc) {
        formats = toD3D11Formats(desc.format);
        const bool multisampled = (desc.multisampling.sampleCount > RenderSampleCount::COUNT_1);
        UINT bindFlags = D3D11_BIND_SHADER_RESOURCE;
        bindFlags |= ((desc.flags & RenderTextureFlag::RENDER_TARGET) != 0) ? D3D11_BIND_RENDER_TARGET : 0;
        bindFlags |= ((desc.flags & RenderTextureFlag::DEPTH_TARGET) != 0) ? D3D11_BIND_DEPTH_STENCIL : 0;
        bindFlags |= (((desc.flags & (RenderTextureFlag::STORAGE | RenderTextureFlag::UNORDERED_ACCESS)) != 0) && !multisampled) ? D3D11_BIND_UNORDERED_ACCESS : 0;
        const UINT width = std::max(desc.width, 1U);
        const UINT height = std::max(desc.height, 1U);
        const UINT arraySize = std::max(desc.arraySize, 1U);
        HRESULT result = E_FAIL;
        switch (desc.dimension) {
        case RenderTextureDimension::TEXTURE_1D: {
            D3D11_TEXTURE1D_DESC textureDesc = {};
            textureDesc.Width = width;
            textureDesc.MipLevels = mipLevels();
            textureDesc.ArraySize = arraySize;
            textureDesc.Format = formats.resource;
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags = bindFlags & ~(D3D11_BIND_DEPTH_STENCIL);
            ComPtr<ID3D11Texture1D> texture;
            result = device->d3d->CreateTexture1D(&textureDesc, nullptr, texture.GetAddressOf());
            resource = texture;
            break;
        }
        case RenderTextureDimension::TEXTURE_3D: {
            D3D11_TEXTURE3D_DESC textureDesc = {};
            textureDesc.Width = width;
            textureDesc.Height = height;
            textureDesc.Depth = std::max(desc.depth, 1U);
            textureDesc.MipLevels = mipLevels();
            textureDesc.Format = formats.resource;
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags = bindFlags & ~(D3D11_BIND_DEPTH_STENCIL);
            ComPtr<ID3D11Texture3D> texture;
            result = device->d3d->CreateTexture3D(&textureDesc, nullptr, texture.GetAddressOf());
            resource = texture;
            break;
        }
        case RenderTextureDimension::TEXTURE_2D:
        default: {
            D3D11_TEXTURE2D_DESC textureDesc = {};
            textureDesc.Width = width;
            textureDesc.Height = height;
            textureDesc.MipLevels = mipLevels();
            textureDesc.ArraySize = arraySize;
            textureDesc.Format = formats.resource;
            textureDesc.SampleDesc.Count = UINT(desc.multisampling.sampleCount);
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags = bindFlags;
            textureDesc.MiscFlags = ((desc.flags & RenderTextureFlag::CUBE) != 0) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;
            ComPtr<ID3D11Texture2D> texture;
            result = device->d3d->CreateTexture2D(&textureDesc, nullptr, texture.GetAddressOf());
            resource = texture;
            break;
        }
        }

        if (FAILED(result) || (resource == nullptr)) {
            fprintf(stderr, "plume_d3d11: creating a %ux%u texture (format %d) failed (0x%08lX).\n", width, height, int(desc.format), (unsigned long)(result));
            return;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = formats.shaderView;
        if (desc.dimension == RenderTextureDimension::TEXTURE_1D) {
            srvDesc.ViewDimension = (arraySize > 1) ? D3D11_SRV_DIMENSION_TEXTURE1DARRAY : D3D11_SRV_DIMENSION_TEXTURE1D;
            srvDesc.Texture1DArray.MipLevels = mipLevels();
            srvDesc.Texture1DArray.ArraySize = arraySize;
        }
        else if (desc.dimension == RenderTextureDimension::TEXTURE_3D) {
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            srvDesc.Texture3D.MipLevels = mipLevels();
        }
        else if (multisampled) {
            srvDesc.ViewDimension = (arraySize > 1) ? D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY : D3D11_SRV_DIMENSION_TEXTURE2DMS;
            srvDesc.Texture2DMSArray.ArraySize = arraySize;
        }
        else if ((desc.flags & RenderTextureFlag::CUBE) != 0) {
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.TextureCube.MipLevels = mipLevels();
        }
        else {
            srvDesc.ViewDimension = (arraySize > 1) ? D3D11_SRV_DIMENSION_TEXTURE2DARRAY : D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2DArray.MipLevels = mipLevels();
            srvDesc.Texture2DArray.ArraySize = arraySize;
        }

        device->d3d->CreateShaderResourceView(resource.Get(), &srvDesc, srv.GetAddressOf());

        if ((bindFlags & D3D11_BIND_UNORDERED_ACCESS) != 0) {
            D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = formats.shaderView;
            if (desc.dimension == RenderTextureDimension::TEXTURE_1D) {
                uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
            }
            else if (desc.dimension == RenderTextureDimension::TEXTURE_3D) {
                uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
                uavDesc.Texture3D.WSize = UINT(-1);
            }
            else {
                uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            }

            device->d3d->CreateUnorderedAccessView(resource.Get(), &uavDesc, uav.GetAddressOf());
        }

        if ((bindFlags & D3D11_BIND_RENDER_TARGET) != 0) {
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = formats.shaderView;
            rtvDesc.ViewDimension = multisampled ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
            device->d3d->CreateRenderTargetView(resource.Get(), &rtvDesc, rtv.GetAddressOf());
        }

        if ((bindFlags & D3D11_BIND_DEPTH_STENCIL) != 0) {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = formats.depthView;
            dsvDesc.ViewDimension = multisampled ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
            device->d3d->CreateDepthStencilView(resource.Get(), &dsvDesc, dsv.GetAddressOf());
            dsvDesc.Flags = D3D11_DSV_READ_ONLY_DEPTH | ((desc.format == RenderFormat::D32_FLOAT_S8_UINT) ? D3D11_DSV_READ_ONLY_STENCIL : 0);
            device->d3d->CreateDepthStencilView(resource.Get(), &dsvDesc, readOnlyDsv.GetAddressOf());
        }
    }

    std::unique_ptr<RenderTextureView> D3D11Texture::createTextureView(const RenderTextureViewDesc &viewDesc) const {
        std::unique_ptr<D3D11TextureView> view = std::make_unique<D3D11TextureView>();
        view->texture = this;
        switch (viewDesc.dimension) {
        case RenderTextureViewDimension::TEXTURE_1D: view->dimension = tk64::ImageDimension::Texture1D; break;
        case RenderTextureViewDimension::TEXTURE_3D: view->dimension = tk64::ImageDimension::Texture3D; break;
        case RenderTextureViewDimension::TEXTURE_CUBE: view->dimension = tk64::ImageDimension::Cube; break;
        default: view->dimension = tk64::ImageDimension::Texture2D; break;
        }

        if (resource == nullptr) {
            return view;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = (viewDesc.format != RenderFormat::UNKNOWN) ? toShaderViewFormat(viewDesc.format) : formats.shaderView;
        const UINT mipCount = (viewDesc.mipLevels == UINT32_MAX) ? UINT(-1) : viewDesc.mipLevels;
        const UINT arrayCount = (viewDesc.arraySize == UINT32_MAX) ? std::max(desc.arraySize, 1U) - viewDesc.arrayIndex : viewDesc.arraySize;
        switch (viewDesc.dimension) {
        case RenderTextureViewDimension::TEXTURE_1D:
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
            srvDesc.Texture1D.MostDetailedMip = viewDesc.mipSlice;
            srvDesc.Texture1D.MipLevels = mipCount;
            break;
        case RenderTextureViewDimension::TEXTURE_3D:
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            srvDesc.Texture3D.MostDetailedMip = viewDesc.mipSlice;
            srvDesc.Texture3D.MipLevels = mipCount;
            break;
        case RenderTextureViewDimension::TEXTURE_CUBE:
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.TextureCube.MostDetailedMip = viewDesc.mipSlice;
            srvDesc.TextureCube.MipLevels = mipCount;
            break;
        default:
            if (desc.multisampling.sampleCount > RenderSampleCount::COUNT_1) {
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
            }
            else if (arrayCount > 1) {
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.MostDetailedMip = viewDesc.mipSlice;
                srvDesc.Texture2DArray.MipLevels = mipCount;
                srvDesc.Texture2DArray.FirstArraySlice = viewDesc.arrayIndex;
                srvDesc.Texture2DArray.ArraySize = arrayCount;
            }
            else {
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MostDetailedMip = viewDesc.mipSlice;
                srvDesc.Texture2D.MipLevels = mipCount;
            }

            break;
        }

        device->d3d->CreateShaderResourceView(resource.Get(), &srvDesc, view->srv.GetAddressOf());
        return view;
    }

    // D3D11PipelineLayout

    D3D11PipelineLayout::D3D11PipelineLayout(const RenderPipelineLayoutDesc &desc) {
        pushConstantRanges.assign(desc.pushConstantRanges, desc.pushConstantRanges + desc.pushConstantRangesCount);
        setLayouts.resize(desc.descriptorSetDescsCount);
        for (uint32_t s = 0; s < desc.descriptorSetDescsCount; s++) {
            const RenderDescriptorSetDesc &setDesc = desc.descriptorSetDescs[s];
            SetLayout &setLayout = setLayouts[s];
            uint32_t start = 0;
            for (uint32_t r = 0; r < setDesc.descriptorRangesCount; r++) {
                // A boundless last range keeps its upper bound; each set allocates as many entries as it was created with.
                const RenderDescriptorRange &range = setDesc.descriptorRanges[r];
                std::vector<const RenderSampler *> samplers;
                if (range.immutableSampler != nullptr) {
                    samplers.assign(range.immutableSampler, range.immutableSampler + range.count);
                }

                setLayout.ranges.emplace_back(range);
                setLayout.rangeStart.emplace_back(start);
                setLayout.immutableSamplers.emplace_back(std::move(samplers));
                start += range.count;
            }
        }
    }

    bool D3D11PipelineLayout::findDescriptor(uint32_t set, uint32_t binding, uint32_t arrayIndex, uint32_t &descriptorIndex, const RenderSampler **immutableSampler) const {
        if (set >= setLayouts.size()) {
            return false;
        }

        const SetLayout &setLayout = setLayouts[set];
        for (size_t r = 0; r < setLayout.ranges.size(); r++) {
            const RenderDescriptorRange &range = setLayout.ranges[r];
            if ((binding < range.binding) || (binding >= (range.binding + range.count))) {
                continue;
            }

            const uint32_t offset = (binding - range.binding) + arrayIndex;
            if (offset >= range.count) {
                return false;
            }

            descriptorIndex = setLayout.rangeStart[r] + offset;
            if (immutableSampler != nullptr) {
                *immutableSampler = setLayout.immutableSamplers[r].empty() ? nullptr : setLayout.immutableSamplers[r][offset];
            }

            return true;
        }

        return false;
    }

    // D3D11DescriptorSet

    D3D11DescriptorSet::D3D11DescriptorSet(const RenderDescriptorSetDesc &desc) {
        uint32_t count = 0;
        for (uint32_t r = 0; r < desc.descriptorRangesCount; r++) {
            const bool boundless = desc.lastRangeIsBoundless && (r == (desc.descriptorRangesCount - 1));
            count += boundless ? std::max(desc.boundlessRangeSize, 1U) : desc.descriptorRanges[r].count;
        }

        entries.resize(count);
    }

    void D3D11DescriptorSet::setBuffer(uint32_t descriptorIndex, const RenderBuffer *buffer, uint64_t bufferSize, const RenderBufferStructuredView *bufferStructuredView, const RenderBufferFormattedView *bufferFormattedView) {
        if (descriptorIndex >= entries.size()) {
            return;
        }

        Entry &entry = entries[descriptorIndex];
        const D3D11Buffer *d3dBuffer = static_cast<const D3D11Buffer *>(buffer);
        const D3D11BufferFormattedView *formattedView = static_cast<const D3D11BufferFormattedView *>(bufferFormattedView);
        entry = Entry();
        entry.buffer = (d3dBuffer != nullptr) ? d3dBuffer->buffer : nullptr;
        entry.bufferTotalSize = (d3dBuffer != nullptr) ? d3dBuffer->desc.size : 0;
        entry.bufferSize = bufferSize;
        entry.structureStride = (bufferStructuredView != nullptr) ? bufferStructuredView->structureByteStride : 0;
        entry.firstElement = (bufferStructuredView != nullptr) ? bufferStructuredView->firstElement : 0;
        if (formattedView != nullptr) {
            entry.formatted = true;
            entry.srv = formattedView->srv;
            entry.uav = formattedView->uav;
            entry.dimension = tk64::ImageDimension::Buffer;
        }
    }

    void D3D11DescriptorSet::setTexture(uint32_t descriptorIndex, const RenderTexture *texture, RenderTextureLayout textureLayout, const RenderTextureView *textureView) {
        if (descriptorIndex >= entries.size()) {
            return;
        }

        Entry &entry = entries[descriptorIndex];
        const D3D11Texture *d3dTexture = static_cast<const D3D11Texture *>(texture);
        const D3D11TextureView *d3dView = static_cast<const D3D11TextureView *>(textureView);
        entry = Entry();
        if (d3dTexture != nullptr) {
            entry.texture = d3dTexture->resource;
            entry.srv = ((d3dView != nullptr) && (d3dView->srv != nullptr)) ? d3dView->srv : d3dTexture->srv;
            entry.uav = d3dTexture->uav;
            entry.dimension = (d3dView != nullptr) ? d3dView->dimension : d3dTexture->dimension();
        }
    }

    void D3D11DescriptorSet::setSampler(uint32_t descriptorIndex, const RenderSampler *sampler) {
        if (descriptorIndex >= entries.size()) {
            return;
        }

        const D3D11Sampler *d3dSampler = static_cast<const D3D11Sampler *>(sampler);
        entries[descriptorIndex].sampler = (d3dSampler != nullptr) ? d3dSampler->state : nullptr;
    }

    // D3D11Pipeline

    D3D11Pipeline::D3D11Pipeline(D3D11Device *device, const RenderGraphicsPipelineDesc &desc) : type(Type::Graphics) {
        layout = static_cast<const D3D11PipelineLayout *>(desc.pipelineLayout);
        const D3D11Shader *vertex = static_cast<const D3D11Shader *>(desc.vertexShader);
        const D3D11Shader *pixel = static_cast<const D3D11Shader *>(desc.pixelShader);
        if ((vertex == nullptr) || !vertex->valid || ((pixel != nullptr) && !pixel->valid)) {
            return;
        }

        vertexShader = vertex->vertexShader;
        vertexBindings.assign(vertex->translated);
        if (pixel != nullptr) {
            pixelShader = pixel->pixelShader;
            pixelBindings.assign(pixel->translated);
            std::string linkedSource = pixel->translated.source;
            if (tk64::matchPixelInputsToVertexOutputs(vertex->translated.source, linkedSource)) {
                std::vector<uint8_t> dxbc;
                std::string errors;
                ComPtr<ID3D11PixelShader> linkedShader;
                if (!device->compiler.compile(linkedSource, tk64::ShaderStage::Pixel, dxbc, errors) ||
                    FAILED(device->d3d->CreatePixelShader(dxbc.data(), dxbc.size(), nullptr, linkedShader.GetAddressOf())))
                {
                    fprintf(stderr, "plume_d3d11: relinking a pixel shader to its vertex shader failed:\n%s\n", errors.c_str());
                    return;
                }

                pixelShader = linkedShader;
            }
        }

        // SPIRV-Cross names vertex inputs TEXCOORD<location>.
        std::vector<D3D11_INPUT_ELEMENT_DESC> elements;
        for (uint32_t i = 0; i < desc.inputElementsCount; i++) {
            const RenderInputElement &element = desc.inputElements[i];
            D3D11_INPUT_ELEMENT_DESC elementDesc = {};
            elementDesc.SemanticName = "TEXCOORD";
            elementDesc.SemanticIndex = element.location;
            elementDesc.Format = toDXGI(element.format);
            elementDesc.InputSlot = element.slotIndex;
            elementDesc.AlignedByteOffset = element.alignedByteOffset;
            elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            for (uint32_t s = 0; s < desc.inputSlotsCount; s++) {
                if ((desc.inputSlots[s].index == element.slotIndex) && (desc.inputSlots[s].classification == RenderInputSlotClassification::PER_INSTANCE_DATA)) {
                    elementDesc.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                    elementDesc.InstanceDataStepRate = 1;
                }
            }

            elements.emplace_back(elementDesc);
        }

        if (!elements.empty()) {
            const HRESULT result = device->d3d->CreateInputLayout(elements.data(), UINT(elements.size()), vertex->dxbc.data(), vertex->dxbc.size(), inputLayout.GetAddressOf());
            if (FAILED(result)) {
                fprintf(stderr, "plume_d3d11: creating an input layout failed (0x%08lX).\n", (unsigned long)(result));
                return;
            }
        }

        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = (desc.cullMode == RenderCullMode::FRONT) ? D3D11_CULL_FRONT : ((desc.cullMode == RenderCullMode::BACK) ? D3D11_CULL_BACK : D3D11_CULL_NONE);
        rasterizerDesc.FrontCounterClockwise = (desc.frontFace == RenderFrontFace::COUNTER_CLOCKWISE);
        rasterizerDesc.DepthBias = desc.depthBias;
        rasterizerDesc.DepthBiasClamp = desc.depthBiasClamp;
        rasterizerDesc.SlopeScaledDepthBias = desc.slopeScaledDepthBias;
        rasterizerDesc.DepthClipEnable = desc.depthClipEnabled;
        rasterizerDesc.ScissorEnable = TRUE;
        rasterizerDesc.MultisampleEnable = (desc.multisampling.sampleCount > RenderSampleCount::COUNT_1);
        device->d3d->CreateRasterizerState(&rasterizerDesc, rasterizerState.GetAddressOf());

        D3D11_DEPTH_STENCIL_DESC depthDesc = {};
        depthDesc.DepthEnable = desc.depthEnabled;
        depthDesc.DepthWriteMask = desc.depthWriteEnabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        depthDesc.DepthFunc = toD3D11(desc.depthFunction);
        depthDesc.StencilEnable = desc.stencilEnabled;
        depthDesc.StencilReadMask = UINT8(desc.stencilReadMask);
        depthDesc.StencilWriteMask = UINT8(desc.stencilWriteMask);
        depthDesc.FrontFace = { toD3D11(desc.stencilFrontFace.failOp), toD3D11(desc.stencilFrontFace.depthFailOp), toD3D11(desc.stencilFrontFace.passOp), toD3D11(desc.stencilFrontFace.compareFunction) };
        depthDesc.BackFace = { toD3D11(desc.stencilBackFace.failOp), toD3D11(desc.stencilBackFace.depthFailOp), toD3D11(desc.stencilBackFace.passOp), toD3D11(desc.stencilBackFace.compareFunction) };
        device->d3d->CreateDepthStencilState(&depthDesc, depthStencilState.GetAddressOf());
        stencilReference = desc.stencilReference;

        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = desc.alphaToCoverageEnabled;
        blendDesc.IndependentBlendEnable = TRUE;
        for (uint32_t i = 0; i < desc.renderTargetCount; i++) {
            const RenderBlendDesc &blend = desc.renderTargetBlend[i];
            D3D11_RENDER_TARGET_BLEND_DESC &target = blendDesc.RenderTarget[i];
            target.BlendEnable = blend.blendEnabled;
            target.SrcBlend = toD3D11(blend.srcBlend);
            target.DestBlend = toD3D11(blend.dstBlend);
            target.BlendOp = toD3D11(blend.blendOp);
            target.SrcBlendAlpha = toD3D11(blend.srcBlendAlpha);
            target.DestBlendAlpha = toD3D11(blend.dstBlendAlpha);
            target.BlendOpAlpha = toD3D11(blend.blendOpAlpha);
            target.RenderTargetWriteMask = blend.renderTargetWriteMask;
        }

        device->d3d->CreateBlendState(&blendDesc, blendState.GetAddressOf());
        topology = toD3D11(desc.primitiveTopology);
        valid = (rasterizerState != nullptr) && (depthStencilState != nullptr) && (blendState != nullptr);
    }

    D3D11Pipeline::D3D11Pipeline(D3D11Device *device, const RenderComputePipelineDesc &desc) : type(Type::Compute) {
        layout = static_cast<const D3D11PipelineLayout *>(desc.pipelineLayout);
        const D3D11Shader *compute = static_cast<const D3D11Shader *>(desc.computeShader);
        if ((compute == nullptr) || !compute->valid) {
            return;
        }

        computeShader = compute->computeShader;
        computeBindings.assign(compute->translated);
        valid = true;
    }

    // D3D11Framebuffer

    D3D11Framebuffer::D3D11Framebuffer(D3D11Device *device, const RenderFramebufferDesc &desc) {
        std::vector<const D3D11Texture *> colorTextures;
        for (uint32_t i = 0; i < desc.colorAttachmentsCount; i++) {
            const D3D11Texture *texture = static_cast<const D3D11Texture *>(desc.colorAttachments[i]);
            colorTextures.emplace_back(texture);
            colorResources.emplace_back(texture->resource);
            renderTargets.emplace_back(texture->rtv);
            width = texture->desc.width;
            height = texture->desc.height;
        }

        const D3D11Texture *depthTexture = static_cast<const D3D11Texture *>(desc.depthAttachment);
        if (depthTexture != nullptr) {
            depthResource = depthTexture->resource;
            depthStencil = depthTexture->dsv;
            readOnlyDepthStencil = depthTexture->readOnlyDsv;
            if (colorTextures.empty()) {
                width = depthTexture->desc.width;
                height = depthTexture->desc.height;
            }
        }

        const bool sizesDiffer = (depthTexture != nullptr) && !colorTextures.empty() && ((depthTexture->desc.width != width) || (depthTexture->desc.height != height));
        if (sizesDiffer) {
            renderTargets.clear();
            for (const D3D11Texture *texture : colorTextures) {
                RenderTextureDesc proxyDesc = RenderTextureDesc::ColorTarget(depthTexture->desc.width, depthTexture->desc.height, texture->desc.format);
                proxyDesc.flags = RenderTextureFlag::RENDER_TARGET;
                proxies.emplace_back(std::make_unique<D3D11Texture>(device, proxyDesc));
                renderTargets.emplace_back(proxies.back()->rtv);
            }
        }
    }

    // Binding.

    static const D3D11DescriptorSet::Entry *findEntry(const D3D11PipelineLayout *layout, const D3D11DescriptorSet *const *sets, uint32_t set, uint32_t binding, uint32_t arrayIndex,
        const RenderSampler **immutableSampler)
    {
        uint32_t descriptorIndex = 0;
        if ((layout == nullptr) || !layout->findDescriptor(set, binding, arrayIndex, descriptorIndex, immutableSampler)) {
            return nullptr;
        }

        if ((set >= 8) || (sets[set] == nullptr) || (descriptorIndex >= sets[set]->entries.size())) {
            return nullptr;
        }

        return &sets[set]->entries[descriptorIndex];
    }

    // The byte range a structured buffer descriptor covers; raw views count 4-byte elements.
    static void rawRange(const D3D11DescriptorSet::Entry &entry, UINT &firstElement, UINT &elementCount) {
        const uint64_t offset = uint64_t(entry.firstElement) * entry.structureStride;
        const uint64_t available = (entry.bufferTotalSize > offset) ? (entry.bufferTotalSize - offset) : 0;
        const uint64_t size = (entry.bufferSize > 0) ? std::min(entry.bufferSize, available) : available;
        firstElement = UINT(offset / 4);
        elementCount = UINT(std::max<uint64_t>(size / 4, 1));
    }

    static ID3D11ShaderResourceView *shaderResourceOf(D3D11Device *device, const tk64::BoundResource &resource, const D3D11DescriptorSet::Entry &entry) {
        if (resource.resourceClass == tk64::ResourceClass::ReadOnlyStorageBuffer) {
            if (entry.buffer == nullptr) {
                return nullptr;
            }

            if (entry.rawSRV == nullptr) {
                D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
                viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
                viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
                viewDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
                rawRange(entry, viewDesc.BufferEx.FirstElement, viewDesc.BufferEx.NumElements);
                device->d3d->CreateShaderResourceView(entry.buffer.Get(), &viewDesc, entry.rawSRV.GetAddressOf());
            }

            return entry.rawSRV.Get();
        }

        return entry.srv.Get();
    }

    static ID3D11UnorderedAccessView *unorderedAccessOf(D3D11Device *device, const tk64::BoundResource &resource, const D3D11DescriptorSet::Entry &entry) {
        if (resource.resourceClass == tk64::ResourceClass::StorageBuffer) {
            if (entry.buffer == nullptr) {
                return nullptr;
            }

            if (entry.rawUAV == nullptr) {
                D3D11_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
                viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
                viewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
                viewDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
                rawRange(entry, viewDesc.Buffer.FirstElement, viewDesc.Buffer.NumElements);
                device->d3d->CreateUnorderedAccessView(entry.buffer.Get(), &viewDesc, entry.rawUAV.GetAddressOf());
            }

            return entry.rawUAV.Get();
        }

        return entry.uav.Get();
    }

    // RT64's texture-slot sets hold decoded (2D) and TMEM (1D) textures in the same slots, read through 2D and 1D arrays; a
    // view of the other dimension is left unbound, as D3D11 requires.
    static bool dimensionMatches(const tk64::BoundResource &resource, const D3D11DescriptorSet::Entry &entry) {
        if ((resource.resourceClass != tk64::ResourceClass::Texture) || (resource.imageDimension == tk64::ImageDimension::None)) {
            return true;
        }

        return (entry.dimension == tk64::ImageDimension::None) || (entry.dimension == resource.imageDimension);
    }

    // Whether a stage reads the depth buffer of the bound framebuffer.
    static bool readsTexture(const D3D11StageBindings &bindings, const D3D11PipelineLayout *layout, const D3D11DescriptorSet *const *sets, ID3D11Resource *texture) {
        if (texture == nullptr) {
            return false;
        }

        for (const tk64::BoundResource &resource : bindings.resources) {
            if (resource.resourceClass != tk64::ResourceClass::Texture) {
                continue;
            }

            for (uint32_t a = 0; a < resource.arraySize; a++) {
                const D3D11DescriptorSet::Entry *entry = findEntry(layout, sets, resource.set, resource.binding, a, nullptr);
                if ((entry != nullptr) && (entry->texture.Get() == texture)) {
                    return true;
                }
            }
        }

        return false;
    }

    // Binds one stage's resources from the current descriptor sets, and its push constants.
    static void bindStage(D3D11ReplayState &state, D3D11StageKind kind, const D3D11StageBindings &bindings, const D3D11PipelineLayout *layout,
        const D3D11DescriptorSet *const *sets, const uint8_t *pushConstants)
    {
        D3D11Device *device = state.device;
        ID3D11DeviceContext *context = state.context;
        ID3D11ShaderResourceView *shaderResources[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
        ID3D11UnorderedAccessView *unorderedAccess[D3D11_PS_CS_UAV_REGISTER_COUNT] = {};
        ID3D11SamplerState *samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT] = {};
        ID3D11Buffer *constantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = {};
        uint32_t shaderResourceCount = 0;
        uint32_t unorderedAccessCount = 0;
        uint32_t samplerCount = 0;
        uint32_t constantBufferCount = 0;
        for (const tk64::BoundResource &resource : bindings.resources) {
            for (uint32_t a = 0; a < resource.arraySize; a++) {
                const RenderSampler *immutableSampler = nullptr;
                const D3D11DescriptorSet::Entry *entry = findEntry(layout, sets, resource.set, resource.binding, a, &immutableSampler);
                const uint32_t target = resource.targetBinding + a;
                switch (resource.resourceClass) {
                case tk64::ResourceClass::UniformBuffer:
                    if ((entry != nullptr) && (entry->buffer != nullptr) && (target < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)) {
                        constantBuffers[target] = entry->buffer.Get();
                        constantBufferCount = std::max(constantBufferCount, target + 1);
                    }

                    break;
                case tk64::ResourceClass::ReadOnlyStorageBuffer:
                case tk64::ResourceClass::Texture:
                    if ((entry != nullptr) && (target < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT) && dimensionMatches(resource, *entry)) {
                        shaderResources[target] = shaderResourceOf(device, resource, *entry);
                        shaderResourceCount = std::max(shaderResourceCount, target + 1);
                    }

                    break;
                case tk64::ResourceClass::StorageBuffer:
                case tk64::ResourceClass::StorageImage:
                    if ((entry != nullptr) && (target < D3D11_PS_CS_UAV_REGISTER_COUNT)) {
                        // D3D11 refuses the same resource in two storage slots; RT64 binds one placeholder to unused ones.
                        ID3D11UnorderedAccessView *view = unorderedAccessOf(device, resource, *entry);
                        for (uint32_t u = 0; (u < D3D11_PS_CS_UAV_REGISTER_COUNT) && (view != nullptr); u++) {
                            view = (unorderedAccess[u] == view) ? nullptr : view;
                        }

                        unorderedAccess[target] = view;
                        unorderedAccessCount = std::max(unorderedAccessCount, target + 1);
                    }

                    break;
                case tk64::ResourceClass::Sampler: {
                    ID3D11SamplerState *sampler = (immutableSampler != nullptr) ? static_cast<const D3D11Sampler *>(immutableSampler)->state.Get() : nullptr;
                    if ((sampler == nullptr) && (entry != nullptr)) {
                        sampler = entry->sampler.Get();
                    }

                    if ((sampler != nullptr) && (target < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT)) {
                        samplers[target] = sampler;
                        samplerCount = std::max(samplerCount, target + 1);
                    }

                    break;
                }
                default:
                    break;
                }
            }
        }

        if ((bindings.pushConstantRegister < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT) && (bindings.pushConstantSize > 0)) {
            ID3D11Buffer *pushBuffer = device->pushConstantBuffers[size_t(kind)].Get();
            D3D11_MAPPED_SUBRESOURCE mapped = {};
            if (SUCCEEDED(context->Map(pushBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                memcpy(mapped.pData, pushConstants, std::min(bindings.pushConstantSize, 256U));
                context->Unmap(pushBuffer, 0);
            }

            constantBuffers[bindings.pushConstantRegister] = pushBuffer;
            constantBufferCount = std::max(constantBufferCount, bindings.pushConstantRegister + 1);
        }

        // Slots the previous shader of this stage used and this one does not are cleared.
        const size_t k = size_t(kind);
        const uint32_t shaderResourceClear = std::max(shaderResourceCount, state.shaderResourceCount[k]);
        const uint32_t samplerClear = std::max(samplerCount, state.samplerCount[k]);
        const uint32_t constantBufferClear = std::max(constantBufferCount, state.constantBufferCount[k]);
        state.shaderResourceCount[k] = shaderResourceCount;
        state.samplerCount[k] = samplerCount;
        state.constantBufferCount[k] = constantBufferCount;
        switch (kind) {
        case D3D11StageKind::Vertex:
            context->VSSetShaderResources(0, shaderResourceClear, shaderResources);
            context->VSSetSamplers(0, samplerClear, samplers);
            context->VSSetConstantBuffers(0, constantBufferClear, constantBuffers);
            break;
        case D3D11StageKind::Pixel:
            context->PSSetShaderResources(0, shaderResourceClear, shaderResources);
            context->PSSetSamplers(0, samplerClear, samplers);
            context->PSSetConstantBuffers(0, constantBufferClear, constantBuffers);
            if (unorderedAccessCount > 0) {
                static bool warned = false;
                if (!warned) {
                    warned = true;
                    fprintf(stderr, "plume_d3d11: pixel shader storage resources are not bound on Direct3D 11 yet.\n");
                }
            }

            break;
        case D3D11StageKind::Compute:
            context->CSSetShaderResources(0, shaderResourceClear, shaderResources);
            context->CSSetSamplers(0, samplerClear, samplers);
            context->CSSetConstantBuffers(0, constantBufferClear, constantBuffers);
            context->CSSetUnorderedAccessViews(0, D3D11_PS_CS_UAV_REGISTER_COUNT, unorderedAccess, nullptr);
            break;
        default:
            break;
        }
    }

    static void copyProxies(D3D11ReplayState &state, bool toProxies) {
        const D3D11Framebuffer *framebuffer = state.framebuffer;
        if ((framebuffer == nullptr) || framebuffer->proxies.empty()) {
            return;
        }

        for (size_t i = 0; i < framebuffer->proxies.size(); i++) {
            ID3D11Resource *color = framebuffer->colorResources[i].Get();
            ID3D11Resource *proxy = framebuffer->proxies[i]->resource.Get();
            const D3D11_BOX box = { 0, 0, 0, std::min(framebuffer->width, framebuffer->proxies[i]->desc.width), std::min(framebuffer->height, framebuffer->proxies[i]->desc.height), 1 };
            if (toProxies) {
                state.context->CopySubresourceRegion(proxy, 0, 0, 0, 0, color, 0, &box);
            }
            else {
                state.context->CopySubresourceRegion(color, 0, 0, 0, 0, proxy, 0, &box);
            }
        }
    }

    static void bindFramebuffer(D3D11ReplayState &state, bool depthReadOnly = false) {
        if (state.framebufferBound && (state.depthReadOnly == depthReadOnly)) {
            return;
        }

        ID3D11RenderTargetView *renderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
        UINT renderTargetCount = 0;
        ID3D11DepthStencilView *depthStencil = nullptr;
        if (state.framebuffer != nullptr) {
            for (const ComPtr<ID3D11RenderTargetView> &renderTarget : state.framebuffer->renderTargets) {
                renderTargets[renderTargetCount++] = renderTarget.Get();
            }

            depthStencil = depthReadOnly ? state.framebuffer->readOnlyDepthStencil.Get() : state.framebuffer->depthStencil.Get();
        }

        if (!state.framebufferBound) {
            copyProxies(state, true);
        }

        state.context->OMSetRenderTargets(renderTargetCount, renderTargets, depthStencil);
        state.framebufferBound = true;
        state.depthReadOnly = depthReadOnly;
    }

    // Ends drawing to the bound framebuffer: proxies go back to the colour targets, and the targets are unbound so other
    // commands can read or copy them.
    static void unbindFramebuffer(D3D11ReplayState &state) {
        if (!state.framebufferBound) {
            return;
        }

        state.context->OMSetRenderTargets(0, nullptr, nullptr);
        copyProxies(state, false);
        state.framebufferBound = false;
    }

    static void applyGraphicsState(D3D11ReplayState &state) {
        const D3D11Pipeline *pipeline = state.pipeline;
        ID3D11DeviceContext *context = state.context;
        context->IASetInputLayout(pipeline->inputLayout.Get());
        context->IASetPrimitiveTopology(pipeline->topology);
        context->VSSetShader(pipeline->vertexShader.Get(), nullptr, 0);
        context->PSSetShader(pipeline->pixelShader.Get(), nullptr, 0);
        context->RSSetState(pipeline->rasterizerState.Get());
        context->OMSetDepthStencilState(pipeline->depthStencilState.Get(), pipeline->stencilReference);
        const FLOAT blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        context->OMSetBlendState(pipeline->blendState.Get(), blendFactor, 0xFFFFFFFF);
        state.graphicsStateBound = true;
    }

    static bool prepareDraw(D3D11ReplayState &state) {
        if ((state.pipeline == nullptr) || !state.pipeline->valid || (state.pipeline->type != D3D11Pipeline::Type::Graphics)) {
            return false;
        }

        if (!state.graphicsStateBound) {
            applyGraphicsState(state);
        }

        ID3D11Resource *depthTexture = (state.framebuffer != nullptr) ? state.framebuffer->depthResource.Get() : nullptr;
        const bool depthRead = readsTexture(state.pipeline->pixelBindings, state.graphicsLayout, state.graphicsSets, depthTexture) ||
            readsTexture(state.pipeline->vertexBindings, state.graphicsLayout, state.graphicsSets, depthTexture);
        bindFramebuffer(state, depthRead);
        bindStage(state, D3D11StageKind::Vertex, state.pipeline->vertexBindings, state.graphicsLayout, state.graphicsSets, state.graphicsPushConstants);
        bindStage(state, D3D11StageKind::Pixel, state.pipeline->pixelBindings, state.graphicsLayout, state.graphicsSets, state.graphicsPushConstants);
        return true;
    }

    // D3D11CommandList

    D3D11CommandList::D3D11CommandList(D3D11Device *device) : device(device) {
        commands = std::make_shared<std::vector<D3D11Command>>();
    }

    void D3D11CommandList::begin() {
        // A new vector, so a list that was submitted keeps its own commands.
        commands = std::make_shared<std::vector<D3D11Command>>();
        recordGraphicsLayout = nullptr;
        recordComputeLayout = nullptr;
        recordViewportSet = false;
        recordScissorSet = false;
    }

    void D3D11CommandList::dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) {
        record([=](D3D11ReplayState &state) {
            if ((state.pipeline == nullptr) || !state.pipeline->valid || (state.pipeline->type != D3D11Pipeline::Type::Compute)) {
                return;
            }

            // A compute shader may read what is bound as a render target; D3D11 refuses a resource bound both ways.
            unbindFramebuffer(state);

            bindStage(state, D3D11StageKind::Compute, state.pipeline->computeBindings, state.computeLayout, state.computeSets, state.computePushConstants);
            state.context->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);

            // Views stay bound otherwise, and the next pass could not write or render to those resources.
            ID3D11UnorderedAccessView *noStorage[D3D11_PS_CS_UAV_REGISTER_COUNT] = {};
            ID3D11ShaderResourceView *noResources[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
            state.context->CSSetUnorderedAccessViews(0, D3D11_PS_CS_UAV_REGISTER_COUNT, noStorage, nullptr);
            state.context->CSSetShaderResources(0, state.shaderResourceCount[size_t(D3D11StageKind::Compute)], noResources);
            state.shaderResourceCount[size_t(D3D11StageKind::Compute)] = 0;
        });
    }

    void D3D11CommandList::drawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) {
        record([=](D3D11ReplayState &state) {
            if (prepareDraw(state)) {
                state.context->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
            }
        });
    }

    void D3D11CommandList::drawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation) {
        record([=](D3D11ReplayState &state) {
            if (prepareDraw(state)) {
                state.context->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
            }
        });
    }

    void D3D11CommandList::setPipeline(const RenderPipeline *pipeline) {
        const D3D11Pipeline *d3dPipeline = static_cast<const D3D11Pipeline *>(pipeline);
        record([d3dPipeline](D3D11ReplayState &state) {
            state.pipeline = d3dPipeline;
            state.graphicsStateBound = false;
            if ((d3dPipeline != nullptr) && d3dPipeline->valid && (d3dPipeline->type == D3D11Pipeline::Type::Compute)) {
                state.context->CSSetShader(d3dPipeline->computeShader.Get(), nullptr, 0);
            }
        });
    }

    void D3D11CommandList::setComputePipelineLayout(const RenderPipelineLayout *pipelineLayout) {
        const D3D11PipelineLayout *layout = static_cast<const D3D11PipelineLayout *>(pipelineLayout);
        recordComputeLayout = layout;
        record([layout](D3D11ReplayState &state) { state.computeLayout = layout; });
    }

    void D3D11CommandList::setGraphicsPipelineLayout(const RenderPipelineLayout *pipelineLayout) {
        const D3D11PipelineLayout *layout = static_cast<const D3D11PipelineLayout *>(pipelineLayout);
        recordGraphicsLayout = layout;
        record([layout](D3D11ReplayState &state) { state.graphicsLayout = layout; });
    }

    // Push constants are copied at record time, like the other APIs do.
    static void recordPushConstants(D3D11CommandList *list, bool compute, const D3D11PipelineLayout *layout, uint32_t rangeIndex, const void *data, uint32_t offset, uint32_t size) {
        uint32_t rangeOffset = 0;
        uint32_t rangeSize = size;
        if ((layout != nullptr) && (rangeIndex < layout->pushConstantRanges.size())) {
            rangeOffset = layout->pushConstantRanges[rangeIndex].offset;
            if (rangeSize == 0) {
                rangeSize = layout->pushConstantRanges[rangeIndex].size;
            }
        }

        const uint32_t begin = rangeOffset + offset;
        if ((begin + rangeSize) > 256) {
            return;
        }

        std::vector<uint8_t> bytes(static_cast<const uint8_t *>(data), static_cast<const uint8_t *>(data) + rangeSize);
        list->record([compute, begin, bytes](D3D11ReplayState &state) {
            uint8_t *target = compute ? state.computePushConstants : state.graphicsPushConstants;
            memcpy(target + begin, bytes.data(), bytes.size());
        });
    }

    void D3D11CommandList::setComputePushConstants(uint32_t rangeIndex, const void *data, uint32_t offset, uint32_t size) {
        recordPushConstants(this, true, recordComputeLayout, rangeIndex, data, offset, size);
    }

    void D3D11CommandList::setGraphicsPushConstants(uint32_t rangeIndex, const void *data, uint32_t offset, uint32_t size) {
        recordPushConstants(this, false, recordGraphicsLayout, rangeIndex, data, offset, size);
    }

    void D3D11CommandList::setComputeDescriptorSet(RenderDescriptorSet *descriptorSet, uint32_t setIndex) {
        const D3D11DescriptorSet *set = static_cast<const D3D11DescriptorSet *>(descriptorSet);
        record([set, setIndex](D3D11ReplayState &state) {
            if (setIndex < 8) {
                state.computeSets[setIndex] = set;
            }
        });
    }

    void D3D11CommandList::setGraphicsDescriptorSet(RenderDescriptorSet *descriptorSet, uint32_t setIndex) {
        const D3D11DescriptorSet *set = static_cast<const D3D11DescriptorSet *>(descriptorSet);
        record([set, setIndex](D3D11ReplayState &state) {
            if (setIndex < 8) {
                state.graphicsSets[setIndex] = set;
            }
        });
    }

    void D3D11CommandList::setIndexBuffer(const RenderIndexBufferView *view) {
        const RenderIndexBufferView copy = (view != nullptr) ? *view : RenderIndexBufferView();
        record([copy](D3D11ReplayState &state) {
            const D3D11Buffer *buffer = static_cast<const D3D11Buffer *>(copy.buffer.ref);
            const DXGI_FORMAT format = (copy.format == RenderFormat::R16_UINT) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            state.context->IASetIndexBuffer((buffer != nullptr) ? buffer->buffer.Get() : nullptr, format, UINT(copy.buffer.offset));
        });
    }

    void D3D11CommandList::setVertexBuffers(uint32_t startSlot, const RenderVertexBufferView *views, uint32_t viewCount, const RenderInputSlot *inputSlots) {
        std::vector<RenderVertexBufferView> viewCopies;
        std::vector<RenderInputSlot> slotCopies;
        if (views != nullptr) {
            viewCopies.assign(views, views + viewCount);
        }

        if (inputSlots != nullptr) {
            slotCopies.assign(inputSlots, inputSlots + viewCount);
        }

        record([startSlot, viewCopies, slotCopies](D3D11ReplayState &state) {
            ID3D11Buffer *buffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
            UINT strides[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
            UINT offsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
            const UINT count = UINT(std::min<size_t>(viewCopies.size(), D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT));
            for (UINT i = 0; i < count; i++) {
                const D3D11Buffer *buffer = static_cast<const D3D11Buffer *>(viewCopies[i].buffer.ref);
                buffers[i] = (buffer != nullptr) ? buffer->buffer.Get() : nullptr;
                offsets[i] = UINT(viewCopies[i].buffer.offset);
                strides[i] = (i < slotCopies.size()) ? slotCopies[i].stride : 0;
            }

            state.context->IASetVertexBuffers(startSlot, count, buffers, strides, offsets);
        });
    }

    void D3D11CommandList::setViewports(const RenderViewport *viewports, uint32_t count) {
        if ((viewports == nullptr) || (count == 0)) {
            return;
        }

        const RenderViewport viewport = viewports[0];
        recordViewportSet = true;
        record([viewport](D3D11ReplayState &state) {
            state.viewport = { viewport.x, viewport.y, viewport.width, viewport.height, viewport.minDepth, viewport.maxDepth };
            state.context->RSSetViewports(1, &state.viewport);
        });
    }

    void D3D11CommandList::setScissors(const RenderRect *scissorRects, uint32_t count) {
        if ((scissorRects == nullptr) || (count == 0)) {
            return;
        }

        const RenderRect rect = scissorRects[0];
        recordScissorSet = true;
        record([rect](D3D11ReplayState &state) {
            state.scissor = { LONG(rect.left), LONG(rect.top), LONG(rect.right), LONG(rect.bottom) };
            state.context->RSSetScissorRects(1, &state.scissor);
        });
    }

    void D3D11CommandList::setFramebuffer(const RenderFramebuffer *framebuffer) {
        const D3D11Framebuffer *d3dFramebuffer = static_cast<const D3D11Framebuffer *>(framebuffer);
        // As in D3D12 and Vulkan, binding a framebuffer keeps the viewport and scissor: RT64 sets the scissor only when it
        // changes, also across its switches between the depth-write and depth-read framebuffers of one pass (resetting it
        // here let the 3D scene draw over the letterbox bars of cutscenes). Only a list that has not set them yet
        // starts with ones covering the framebuffer, because the immediate context carries them over from other lists.
        const bool resetViewport = !recordViewportSet;
        const bool resetScissor = !recordScissorSet;
        record([d3dFramebuffer, resetViewport, resetScissor](D3D11ReplayState &state) {
            unbindFramebuffer(state);
            state.framebuffer = d3dFramebuffer;
            if (d3dFramebuffer != nullptr) {
                // Shader views of the new targets from earlier passes would make D3D11 refuse the targets.
                ID3D11ShaderResourceView *none[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
                state.context->PSSetShaderResources(0, state.shaderResourceCount[size_t(D3D11StageKind::Pixel)], none);
                state.context->VSSetShaderResources(0, state.shaderResourceCount[size_t(D3D11StageKind::Vertex)], none);
                state.shaderResourceCount[size_t(D3D11StageKind::Pixel)] = 0;
                state.shaderResourceCount[size_t(D3D11StageKind::Vertex)] = 0;
                if (resetViewport) {
                    state.viewport = { 0.0f, 0.0f, float(d3dFramebuffer->width), float(d3dFramebuffer->height), 0.0f, 1.0f };
                    state.context->RSSetViewports(1, &state.viewport);
                }

                if (resetScissor) {
                    state.scissor = { 0, 0, LONG(d3dFramebuffer->width), LONG(d3dFramebuffer->height) };
                    state.context->RSSetScissorRects(1, &state.scissor);
                }
            }

            bindFramebuffer(state);
        });
    }

    void D3D11CommandList::setDepthBias(float depthBias, float depthBiasClamp, float slopeScaledDepthBias) {
        record([=](D3D11ReplayState &state) {
            if ((state.pipeline == nullptr) || (state.pipeline->type != D3D11Pipeline::Type::Graphics)) {
                return;
            }

            D3D11_RASTERIZER_DESC rasterizerDesc = state.pipeline->rasterizerDesc;
            rasterizerDesc.DepthBias = INT(depthBias);
            rasterizerDesc.DepthBiasClamp = depthBiasClamp;
            rasterizerDesc.SlopeScaledDepthBias = slopeScaledDepthBias;
            ComPtr<ID3D11RasterizerState> rasterizerState;
            state.device->d3d->CreateRasterizerState(&rasterizerDesc, rasterizerState.GetAddressOf());
            state.context->RSSetState(rasterizerState.Get());
        });
    }

    static bool coversFramebuffer(const D3D11ReplayState &state, const std::vector<RenderRect> &rects) {
        if (rects.empty()) {
            return true;
        }

        const RenderRect &rect = rects[0];
        return (rects.size() == 1) && (state.framebuffer != nullptr) && (rect.left <= 0) && (rect.top <= 0) && (rect.right >= int32_t(state.framebuffer->width)) &&
            (rect.bottom >= int32_t(state.framebuffer->height));
    }

    // Draws the clear pipeline over each rectangle. Only depth is written: colour rectangles use ClearView.
    static void clearDepthRects(D3D11ReplayState &state, const std::vector<RenderRect> &rects, float depthValue) {
        D3D11Device *device = state.device;
        ID3D11DeviceContext *context = state.context;
        if ((device->clearPixelShader == nullptr) || (state.framebuffer == nullptr)) {
            return;
        }

        const float constants[8] = { 0.0f, 0.0f, 0.0f, 0.0f, depthValue, 0.0f, 0.0f, 0.0f };
        context->UpdateSubresource(device->clearConstants.Get(), 0, nullptr, constants, 0, 0);
        bindFramebuffer(state);
        const D3D11_VIEWPORT viewport = { 0.0f, 0.0f, float(state.framebuffer->width), float(state.framebuffer->height), 0.0f, 1.0f };
        context->RSSetViewports(1, &viewport);
        context->IASetInputLayout(nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(device->clearVertexShader.Get(), nullptr, 0);
        context->PSSetShader(device->clearPixelShader.Get(), nullptr, 0);
        context->VSSetConstantBuffers(0, 1, device->clearConstants.GetAddressOf());
        context->PSSetConstantBuffers(0, 1, device->clearConstants.GetAddressOf());
        context->RSSetState(device->clearRasterizerState.Get());
        context->OMSetDepthStencilState(device->clearDepthState.Get(), 0);
        context->OMSetBlendState(device->clearBlendState.Get(), nullptr, 0xFFFFFFFF);
        for (const RenderRect &rect : rects) {
            const D3D11_RECT scissor = { LONG(rect.left), LONG(rect.top), LONG(rect.right), LONG(rect.bottom) };
            context->RSSetScissorRects(1, &scissor);
            context->Draw(3, 0);
        }

        // The next draw binds its pipeline and resources again.
        context->RSSetViewports(1, &state.viewport);
        context->RSSetScissorRects(1, &state.scissor);
        state.graphicsStateBound = false;
        state.constantBufferCount[size_t(D3D11StageKind::Vertex)] = std::max(state.constantBufferCount[size_t(D3D11StageKind::Vertex)], 1U);
        state.constantBufferCount[size_t(D3D11StageKind::Pixel)] = std::max(state.constantBufferCount[size_t(D3D11StageKind::Pixel)], 1U);
    }

    void D3D11CommandList::clearColor(uint32_t attachmentIndex, RenderColor colorValue, const RenderRect *clearRects, uint32_t clearRectsCount) {
        std::vector<RenderRect> rects;
        if (clearRects != nullptr) {
            rects.assign(clearRects, clearRects + clearRectsCount);
        }

        record([=](D3D11ReplayState &state) {
            if ((state.framebuffer == nullptr) || (attachmentIndex >= state.framebuffer->renderTargets.size())) {
                return;
            }

            bindFramebuffer(state);
            ID3D11RenderTargetView *renderTarget = state.framebuffer->renderTargets[attachmentIndex].Get();
            if (coversFramebuffer(state, rects) || (state.device->context1 == nullptr)) {
                state.context->ClearRenderTargetView(renderTarget, colorValue.rgba);
                return;
            }

            std::vector<D3D11_RECT> d3dRects;
            for (const RenderRect &rect : rects) {
                d3dRects.push_back({ LONG(rect.left), LONG(rect.top), LONG(rect.right), LONG(rect.bottom) });
            }

            state.device->context1->ClearView(renderTarget, colorValue.rgba, d3dRects.data(), UINT(d3dRects.size()));
        });
    }

    void D3D11CommandList::clearDepthStencil(bool clearDepth, bool clearStencil, float depthValue, uint32_t stencilValue, const RenderRect *clearRects, uint32_t clearRectsCount) {
        std::vector<RenderRect> rects;
        if (clearRects != nullptr) {
            rects.assign(clearRects, clearRects + clearRectsCount);
        }

        record([=](D3D11ReplayState &state) {
            if ((state.framebuffer == nullptr) || (state.framebuffer->depthStencil == nullptr)) {
                return;
            }

            bindFramebuffer(state);
            if (coversFramebuffer(state, rects) || !clearDepth) {
                const UINT flags = (clearDepth ? D3D11_CLEAR_DEPTH : 0) | (clearStencil ? D3D11_CLEAR_STENCIL : 0);
                state.context->ClearDepthStencilView(state.framebuffer->depthStencil.Get(), flags, depthValue, UINT8(stencilValue));
                return;
            }

            clearDepthRects(state, rects, depthValue);
        });
    }

    // Staging copies for reading GPU data on the CPU.

    static bool readBufferRange(D3D11ReplayState &state, const D3D11Buffer *buffer, uint64_t offset, uint64_t size, uint8_t *destination) {
        if (!buffer->shadow.empty() && (buffer->isUpload() || (buffer->desc.heapType == RenderHeapType::READBACK) || buffer->constantBuffer)) {
            memcpy(destination, buffer->shadow.data() + offset, size_t(size));
            return true;
        }

        if (buffer->buffer == nullptr) {
            return false;
        }

        D3D11_BUFFER_DESC stagingDesc = {};
        stagingDesc.ByteWidth = UINT(roundUp(std::max(size, uint64_t(16)), 16));
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ComPtr<ID3D11Buffer> staging;
        if (FAILED(state.device->d3d->CreateBuffer(&stagingDesc, nullptr, staging.GetAddressOf()))) {
            return false;
        }

        const D3D11_BOX box = { UINT(offset), 0, 0, UINT(offset + size), 1, 1 };
        state.context->CopySubresourceRegion(staging.Get(), 0, 0, 0, 0, buffer->buffer.Get(), 0, &box);
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(state.context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) {
            return false;
        }

        memcpy(destination, mapped.pData, size_t(size));
        state.context->Unmap(staging.Get(), 0);
        return true;
    }

    // Writes bytes into a buffer: its CPU copy if it has one, and its GPU buffer.
    static void writeBufferRange(D3D11ReplayState &state, const D3D11Buffer *buffer, uint64_t offset, uint64_t size, const uint8_t *source) {
        D3D11Buffer *mutableBuffer = const_cast<D3D11Buffer *>(buffer);
        if (!mutableBuffer->shadow.empty()) {
            memcpy(mutableBuffer->shadow.data() + offset, source, size_t(size));
        }

        if (buffer->buffer == nullptr) {
            return;
        }

        if (buffer->constantBuffer) {
            state.context->UpdateSubresource(buffer->buffer.Get(), 0, nullptr, mutableBuffer->shadow.data(), 0, 0);
        }
        else {
            const D3D11_BOX box = { UINT(offset), 0, 0, UINT(offset + size), 1, 1 };
            state.context->UpdateSubresource(buffer->buffer.Get(), 0, &box, source, 0, 0);
        }
    }

    void D3D11CommandList::copyBufferRegion(RenderBufferReference dstBuffer, RenderBufferReference srcBuffer, uint64_t size) {
        const D3D11Buffer *dst = static_cast<const D3D11Buffer *>(dstBuffer.ref);
        const D3D11Buffer *src = static_cast<const D3D11Buffer *>(srcBuffer.ref);
        const uint64_t dstOffset = dstBuffer.offset;
        const uint64_t srcOffset = srcBuffer.offset;
        record([=](D3D11ReplayState &state) {
            unbindFramebuffer(state);
            const bool srcOnCPU = !src->shadow.empty() && (src->isUpload() || src->constantBuffer);
            const bool dstOnlyGPU = dst->shadow.empty() && (dst->buffer != nullptr);
            if (!srcOnCPU && dstOnlyGPU && (src->buffer != nullptr) && (src != dst)) {
                const D3D11_BOX box = { UINT(srcOffset), 0, 0, UINT(srcOffset + size), 1, 1 };
                state.context->CopySubresourceRegion(dst->buffer.Get(), 0, UINT(dstOffset), 0, 0, src->buffer.Get(), 0, &box);
                return;
            }

            std::vector<uint8_t> bytes(static_cast<size_t>(size));
            if (readBufferRange(state, src, srcOffset, size, bytes.data())) {
                writeBufferRange(state, dst, dstOffset, size, bytes.data());
            }
        });
    }

    void D3D11CommandList::copyBuffer(const RenderBuffer *dstBuffer, const RenderBuffer *srcBuffer) {
        const D3D11Buffer *dst = static_cast<const D3D11Buffer *>(dstBuffer);
        const D3D11Buffer *src = static_cast<const D3D11Buffer *>(srcBuffer);
        copyBufferRegion(RenderBufferReference(dst), RenderBufferReference(src), std::min(dst->desc.size, src->desc.size));
    }

    static UINT subresourceOf(const D3D11Texture *texture, const RenderTextureCopyLocation &location) {
        return D3D11CalcSubresource(location.subresource.mipLevel, location.subresource.arrayIndex, texture->mipLevels());
    }

    void D3D11CommandList::copyTextureRegion(const RenderTextureCopyLocation &dstLocation, const RenderTextureCopyLocation &srcLocation, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const RenderBox *srcBox) {
        const RenderTextureCopyLocation dst = dstLocation;
        const RenderTextureCopyLocation src = srcLocation;
        const bool hasBox = (srcBox != nullptr);
        const RenderBox box = hasBox ? *srcBox : RenderBox();
        record([=](D3D11ReplayState &state) {
            unbindFramebuffer(state);
            if ((dst.type == RenderTextureCopyType::SUBRESOURCE) && (src.type == RenderTextureCopyType::PLACED_FOOTPRINT)) {
                // Buffer to texture: the rows are sent from the CPU.
                const D3D11Texture *texture = static_cast<const D3D11Texture *>(dst.texture);
                const D3D11Buffer *buffer = static_cast<const D3D11Buffer *>(src.buffer);
                const RenderFormat format = src.placedFootprint.format;
                const uint32_t blockWidth = std::max(RenderFormatBlockWidth(format), 1U);
                const uint32_t width = hasBox ? uint32_t(box.right - box.left) : src.placedFootprint.width;
                const uint32_t height = hasBox ? uint32_t(box.bottom - box.top) : std::max(src.placedFootprint.height, 1U);
                const uint32_t depth = hasBox ? uint32_t(box.back - box.front) : std::max(src.placedFootprint.depth, 1U);
                const uint64_t rowPitch = uint64_t(src.placedFootprint.rowWidth) * RenderFormatSize(format) / blockWidth;
                const uint64_t rowCount = (uint64_t(std::max(src.placedFootprint.height, 1U)) + blockWidth - 1) / blockWidth;
                const uint64_t depthPitch = rowPitch * rowCount;
                const uint64_t offset = src.placedFootprint.offset + (hasBox ? ((uint64_t(box.top) / blockWidth) * rowPitch + uint64_t(box.left) * RenderFormatSize(format) / blockWidth) : 0);
                const uint64_t byteCount = depthPitch * (depth - 1) + rowPitch * ((height + blockWidth - 1) / blockWidth - 1) + uint64_t(width) * RenderFormatSize(format) / blockWidth;
                std::vector<uint8_t> staged;
                const uint8_t *bytes = nullptr;
                if (!buffer->shadow.empty() && (buffer->isUpload() || buffer->constantBuffer)) {
                    bytes = buffer->shadow.data() + offset;
                }
                else {
                    staged.resize(size_t(byteCount));
                    if (!readBufferRange(state, buffer, offset, byteCount, staged.data())) {
                        return;
                    }

                    bytes = staged.data();
                }

                const D3D11_BOX dstBox = { dstX, dstY, dstZ, dstX + width, dstY + height, dstZ + depth };
                state.context->UpdateSubresource(texture->resource.Get(), subresourceOf(texture, dst), &dstBox, bytes, UINT(rowPitch), UINT(depthPitch));
            }
            else if ((dst.type == RenderTextureCopyType::PLACED_FOOTPRINT) && (src.type == RenderTextureCopyType::SUBRESOURCE)) {
                // Texture to buffer, through a staging texture.
                const D3D11Texture *texture = static_cast<const D3D11Texture *>(src.texture);
                const D3D11Buffer *buffer = static_cast<const D3D11Buffer *>(dst.buffer);
                const RenderFormat format = dst.placedFootprint.format;
                const uint32_t x = hasBox ? uint32_t(box.left) : 0;
                const uint32_t y = hasBox ? uint32_t(box.top) : 0;
                const uint32_t width = hasBox ? uint32_t(box.right - box.left) : dst.placedFootprint.width;
                const uint32_t height = hasBox ? uint32_t(box.bottom - box.top) : std::max(dst.placedFootprint.height, 1U);
                const uint64_t rowPitch = uint64_t(dst.placedFootprint.rowWidth) * RenderFormatSize(format) / std::max(RenderFormatBlockWidth(format), 1U);
                const uint64_t rowBytes = uint64_t(width) * RenderFormatSize(format) / std::max(RenderFormatBlockWidth(format), 1U);
                D3D11_TEXTURE2D_DESC stagingDesc = {};
                stagingDesc.Width = width;
                stagingDesc.Height = height;
                stagingDesc.MipLevels = 1;
                stagingDesc.ArraySize = 1;
                stagingDesc.Format = texture->formats.resource;
                stagingDesc.SampleDesc.Count = 1;
                stagingDesc.Usage = D3D11_USAGE_STAGING;
                stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                ComPtr<ID3D11Texture2D> staging;
                if (FAILED(state.device->d3d->CreateTexture2D(&stagingDesc, nullptr, staging.GetAddressOf()))) {
                    return;
                }

                const D3D11_BOX sourceBox = { x, y, 0, x + width, y + height, 1 };
                const bool wholeDepth = texture->formats.depth;
                state.context->CopySubresourceRegion(staging.Get(), 0, 0, 0, 0, texture->resource.Get(), subresourceOf(texture, src), wholeDepth ? nullptr : &sourceBox);
                D3D11_MAPPED_SUBRESOURCE mapped = {};
                if (FAILED(state.context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) {
                    return;
                }

                std::vector<uint8_t> bytes(size_t(rowPitch * (height - 1) + rowBytes));
                for (uint32_t row = 0; row < height; row++) {
                    memcpy(bytes.data() + row * rowPitch, static_cast<const uint8_t *>(mapped.pData) + row * mapped.RowPitch, size_t(rowBytes));
                }

                state.context->Unmap(staging.Get(), 0);
                writeBufferRange(state, buffer, dst.placedFootprint.offset, bytes.size(), bytes.data());
            }
            else if ((dst.type == RenderTextureCopyType::SUBRESOURCE) && (src.type == RenderTextureCopyType::SUBRESOURCE)) {
                const D3D11Texture *dstTexture = static_cast<const D3D11Texture *>(dst.texture);
                const D3D11Texture *srcTexture = static_cast<const D3D11Texture *>(src.texture);
                const D3D11_BOX sourceBox = { UINT(box.left), UINT(box.top), UINT(box.front), UINT(box.right), UINT(box.bottom), UINT(box.back) };

                // Depth and multisampled resources can only be copied whole.
                const bool whole = !hasBox || srcTexture->formats.depth || (srcTexture->desc.multisampling.sampleCount > RenderSampleCount::COUNT_1);
                state.context->CopySubresourceRegion(dstTexture->resource.Get(), subresourceOf(dstTexture, dst), whole ? 0 : dstX, whole ? 0 : dstY, whole ? 0 : dstZ,
                    srcTexture->resource.Get(), subresourceOf(srcTexture, src), whole ? nullptr : &sourceBox);
            }
        });
    }

    void D3D11CommandList::copyTexture(const RenderTexture *dstTexture, const RenderTexture *srcTexture) {
        const D3D11Texture *dst = static_cast<const D3D11Texture *>(dstTexture);
        const D3D11Texture *src = static_cast<const D3D11Texture *>(srcTexture);
        record([=](D3D11ReplayState &state) {
            unbindFramebuffer(state);
            state.context->CopyResource(dst->resource.Get(), src->resource.Get());
        });
    }

    void D3D11CommandList::resolveTexture(const RenderTexture *dstTexture, const RenderTexture *srcTexture) {
        const D3D11Texture *dst = static_cast<const D3D11Texture *>(dstTexture);
        const D3D11Texture *src = static_cast<const D3D11Texture *>(srcTexture);
        record([=](D3D11ReplayState &state) {
            unbindFramebuffer(state);
            state.context->ResolveSubresource(dst->resource.Get(), 0, src->resource.Get(), 0, dst->formats.shaderView);
        });
    }

    void D3D11CommandList::resolveTextureRegion(const RenderTexture *dstTexture, uint32_t dstX, uint32_t dstY, const RenderTexture *srcTexture, const RenderRect *srcRect, RenderResolveMode resolveMode) {
        // Region resolves are not reported (resolveRegion = false); RT64 resolves with a shader instead.
        resolveTexture(dstTexture, srcTexture);
    }

    // D3D11CommandQueue

    std::unique_ptr<RenderCommandList> D3D11CommandQueue::createCommandList() {
        return std::make_unique<D3D11CommandList>(device);
    }

    std::unique_ptr<RenderSwapChain> D3D11CommandQueue::createSwapChain(const RenderSwapChainDesc &desc) {
        return std::make_unique<D3D11SwapChain>(device, desc);
    }

    void D3D11CommandQueue::executeCommandLists(const RenderCommandList **commandLists, uint32_t commandListCount, RenderCommandSemaphore **waitSemaphores, uint32_t waitSemaphoreCount, RenderCommandSemaphore **signalSemaphores, uint32_t signalSemaphoreCount, RenderCommandFence *signalFence) {
        std::unique_lock<std::mutex> lock(device->contextMutex);
        D3D11ReplayState state;
        state.device = device;
        state.context = device->context.Get();
        for (uint32_t i = 0; i < commandListCount; i++) {
            const D3D11CommandList *list = static_cast<const D3D11CommandList *>(commandLists[i]);
            const std::shared_ptr<std::vector<D3D11Command>> commands = list->commands;
            for (const D3D11Command &command : *commands) {
                command(state);
            }
        }

        unbindFramebuffer(state);

        // The next submission starts from a clean context, as each one starts from fresh replay state.
        state.context->ClearState();
        device->printDebugMessages();
    }

    // D3D11SwapChain

    D3D11SwapChain::D3D11SwapChain(D3D11Device *device, const RenderSwapChainDesc &desc) : device(device), desc(desc) {
        queryWindowSize(width, height);
        const DXGI_FORMAT format = toDXGI(desc.format);
        ComPtr<IDXGIFactory2> factory2;
        if ((device->factory != nullptr) && SUCCEEDED(device->factory.As(&factory2))) {
            // Flip model first (Windows 10, then 8); blt model on Windows 7.
            const DXGI_SWAP_EFFECT effects[] = { DXGI_SWAP_EFFECT_FLIP_DISCARD, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_SWAP_EFFECT_DISCARD };
            for (DXGI_SWAP_EFFECT effect : effects) {
                DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
                swapChainDesc.Width = std::max(width, 1U);
                swapChainDesc.Height = std::max(height, 1U);
                swapChainDesc.Format = format;
                swapChainDesc.SampleDesc.Count = 1;
                swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                swapChainDesc.BufferCount = (effect == DXGI_SWAP_EFFECT_DISCARD) ? 1 : 2;
                swapChainDesc.SwapEffect = effect;
                ComPtr<IDXGISwapChain1> swapChain1;
                if (SUCCEEDED(factory2->CreateSwapChainForHwnd(device->d3d.Get(), desc.renderWindow, &swapChainDesc, nullptr, nullptr, swapChain1.GetAddressOf()))) {
                    swapChain = swapChain1;
                    break;
                }
            }
        }

        if (swapChain == nullptr) {
            fprintf(stderr, "plume_d3d11: creating the swap chain failed.\n");
            return;
        }

        if (device->factory != nullptr) {
            device->factory->MakeWindowAssociation(desc.renderWindow, DXGI_MWA_NO_ALT_ENTER);
        }

        swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
        createTextures();
    }

    void D3D11SwapChain::queryWindowSize(uint32_t &w, uint32_t &h) const {
        RECT rect = {};
        GetClientRect(desc.renderWindow, &rect);
        w = uint32_t(std::max<LONG>(rect.right - rect.left, 0));
        h = uint32_t(std::max<LONG>(rect.bottom - rect.top, 0));
    }

    void D3D11SwapChain::createTextures() {
        // RT64 renders into one of several swap chain textures; D3D11 exposes one back buffer, so these are offscreen
        // textures copied to it on present.
        textures.clear();
        if ((width == 0) || (height == 0)) {
            return;
        }

        const uint32_t count = std::max(desc.textureCount, 2U);
        for (uint32_t i = 0; i < count; i++) {
            textures.emplace_back(std::make_unique<D3D11Texture>(device, RenderTextureDesc::ColorTarget(width, height, desc.format)));
        }
    }

    bool D3D11SwapChain::acquireTexture(RenderCommandSemaphore *signalSemaphore, uint32_t *textureIndex) {
        if (textures.empty()) {
            return false;
        }

        *textureIndex = nextIndex;
        nextIndex = (nextIndex + 1) % uint32_t(textures.size());
        return true;
    }

    bool D3D11SwapChain::present(uint32_t textureIndex, RenderCommandSemaphore **waitSemaphores, uint32_t waitSemaphoreCount) {
        if ((swapChain == nullptr) || (backBuffer == nullptr) || (textureIndex >= textures.size())) {
            return false;
        }

        std::unique_lock<std::mutex> lock(device->contextMutex);
        device->context->CopyResource(backBuffer.Get(), textures[textureIndex]->resource.Get());
        const HRESULT result = swapChain->Present(vsyncEnabled ? 1 : 0, 0);
        return SUCCEEDED(result);
    }

    bool D3D11SwapChain::resize() {
        queryWindowSize(width, height);
        if ((swapChain == nullptr) || (width == 0) || (height == 0)) {
            textures.clear();
            return false;
        }

        std::unique_lock<std::mutex> lock(device->contextMutex);
        backBuffer.Reset();
        device->context->ClearState();
        device->context->Flush();
        if (FAILED(swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0))) {
            return false;
        }

        swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
        lock.unlock();
        createTextures();
        nextIndex = 0;
        return !textures.empty();
    }

    bool D3D11SwapChain::needsResize() const {
        uint32_t w = 0;
        uint32_t h = 0;
        queryWindowSize(w, h);
        return (w != width) || (h != height);
    }

    // D3D11Interface

    struct D3D11Interface : RenderInterface {
        RenderInterfaceCapabilities capabilities;
        std::vector<std::string> deviceNames;
        std::vector<ComPtr<IDXGIAdapter1>> adapters;

        D3D11Interface() {
            capabilities.shaderFormat = RenderShaderFormat::SPIRV;
            ComPtr<IDXGIFactory1> factory;
            if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf())))) {
                return;
            }

            ComPtr<IDXGIAdapter1> adapter;
            for (UINT i = 0; factory->EnumAdapters1(i, adapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; i++) {
                DXGI_ADAPTER_DESC1 adapterDesc = {};
                adapter->GetDesc1(&adapterDesc);
                if ((adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) {
                    continue;
                }

                deviceNames.emplace_back(toUTF8(adapterDesc.Description));
                adapters.emplace_back(adapter);
            }
        }

        std::unique_ptr<RenderDevice> createDevice(const std::string &preferredDeviceName) override {
            // The adapter Windows assigns to the application comes first; a named one is used when it exists.
            IDXGIAdapter1 *adapter = nullptr;
            for (size_t i = 0; i < deviceNames.size(); i++) {
                if (!preferredDeviceName.empty() && (deviceNames[i] == preferredDeviceName)) {
                    adapter = adapters[i].Get();
                }
            }

            std::unique_ptr<D3D11Device> device = std::make_unique<D3D11Device>();
            if (!device->create(adapter)) {
                return nullptr;
            }

            return device;
        }

        const std::vector<std::string> &getDeviceNames() const override { return deviceNames; }
        const RenderInterfaceCapabilities &getCapabilities() const override { return capabilities; }
    };

    std::unique_ptr<RenderInterface> CreateD3D11Interface() {
        return std::make_unique<D3D11Interface>();
    }
};

#else

namespace plume {
    std::unique_ptr<RenderInterface> CreateD3D11Interface() {
        return nullptr;
    }
};

#endif
