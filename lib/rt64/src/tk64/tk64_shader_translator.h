//
// TK64
//
// SPIR-V to GLSL ES 3.10, GLSL 4.30 and HLSL 5.0 translation, shared by the GLES/GL and D3D11 backends and by the
// offline tools/shader-check harness. Contract: docs/specs/shader-translation.md.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace tk64 {
    enum class ShaderLanguage {
        GLSL_ES_310,
        GLSL_430,
        HLSL_50
    };

    enum class ShaderStage {
        Unknown,
        Vertex,
        Pixel,
        Compute
    };

    // Binding namespaces of the target APIs. GL has separate binding points for uniform blocks, storage blocks, texture
    // units and image units; D3D11 has b, t, u and s registers. Read-only storage buffers are SRVs (t) in D3D11.
    enum class ResourceClass {
        UniformBuffer,
        StorageBuffer,
        ReadOnlyStorageBuffer,
        Texture,
        StorageImage,
        Sampler
    };

    struct ResourceLocation {
        ResourceClass resourceClass = ResourceClass::UniformBuffer;
        uint32_t set = 0;
        uint32_t binding = 0;
        uint32_t arraySize = 1;
    };

    // Returns the binding point or register a resource gets in the target API. Backends derive it from the pipeline
    // layout so that every shader of a layout agrees; when no mapper is given, resources are numbered densely per class.
    typedef std::function<uint32_t(const ResourceLocation &location)> BindingMapper;

    // GLSL without Vulkan semantics only has combined image samplers: each one names the texture and the sampler it
    // was built from, so the backend can bind both to the combined sampler's texture unit.
    struct CombinedSampler {
        std::string name;
        uint32_t textureUnit = 0;
        uint32_t arraySize = 1;
        uint32_t imageSet = 0;
        uint32_t imageBinding = 0;
        uint32_t samplerSet = 0;
        uint32_t samplerBinding = 0;
        bool dummySampler = false;

        // The texture is a buffer (HLSL Buffer<T>, GLSL samplerBuffer) and is bound as a buffer texture.
        bool texelBuffer = false;
    };

    enum class ImageDimension {
        None,
        Texture1D,
        Texture2D,
        Texture3D,
        Cube,
        Buffer
    };

    // Every other resource of the shader, after remapping: where it was in the descriptor sets and where it is now.
    struct BoundResource {
        ResourceClass resourceClass = ResourceClass::UniformBuffer;
        uint32_t set = 0;
        uint32_t binding = 0;
        uint32_t arraySize = 1;
        uint32_t targetBinding = 0;

        // Storage images only: the image is a buffer (RWBuffer<T>, imageBuffer), the GL format the shader declares (0 when
        // it declares none), and whether the shader only writes it. A write-only image can be bound with any format; on
        // GLSL ES, which always declares one, TranslationOptions::imageFormats picks it.
        bool texelBuffer = false;
        uint32_t glImageFormat = 0;
        bool writeOnly = false;

        // Textures and storage images: the dimension the shader declares. D3D11 needs the bound view to have the same one.
        ImageDimension imageDimension = ImageDimension::None;
    };

    enum class ScalarType {
        Float,
        Int,
        UInt
    };

    // A member of the push constant block. GLSL without Vulkan semantics has no push constants, so the block becomes a
    // plain uniform struct and each member is set with glUniform* at "<blockName>.<name>".
    struct PushConstantMember {
        std::string name;
        uint32_t offset = 0;
        ScalarType type = ScalarType::Float;
        uint32_t vectorSize = 1;
        uint32_t columns = 1;
        uint32_t arraySize = 0;
        uint32_t arrayStride = 0;
        uint32_t matrixStride = 0;

        // The SPIR-V member is decorated RowMajor (DXC does this for HLSL's default column_major matrices), so GL must
        // transpose it when it is set.
        bool rowMajor = false;
    };

    struct TranslatedShader {
        bool ok = false;
        std::string source;
        std::string error;
        ShaderStage stage = ShaderStage::Unknown;
        std::vector<CombinedSampler> combinedSamplers;
        std::vector<BoundResource> resources;
        std::string pushConstantBlockName;
        std::vector<PushConstantMember> pushConstantMembers;
        std::vector<std::string> requiredExtensions;

        // HLSL: the push constant block is a constant buffer at this register (after the shader's own constant buffers),
        // laid out byte for byte like the SPIR-V block (packoffset), so the backend uploads the raw push constant bytes.
        // pushConstantRegister is UINT32_MAX when the shader has no push constants.
        uint32_t pushConstantRegister = UINT32_MAX;
        uint32_t pushConstantSize = 0;
    };

    struct TranslationOptions {
        // GL targets: map the [0, 1] clip depth of D3D/Vulkan to GL's [-1, 1] in vertex shaders. Y needs no change:
        // RT64's and RecompFrontend's vertex shaders are built with DXC's -fvk-invert-y (Vulkan clip space), which in
        // GL also puts row 0 of the framebuffer's memory at the top of the image.
        bool glClipSpaceFixup = true;

        // GLSL ES: the format to declare for write-only storage images, by their SPIR-V (set, binding), instead of the one
        // DXC derived from the element type. Desktop GLSL declares write-only images without a format and ignores this.
        struct ImageFormat {
            uint32_t set = 0;
            uint32_t binding = 0;
            uint32_t glFormat = 0;
        };

        std::vector<ImageFormat> imageFormats;
    };

    TranslatedShader translateSpirv(const uint32_t *words, size_t wordCount, ShaderLanguage language, const BindingMapper &mapper = BindingMapper(),
        const TranslationOptions &options = TranslationOptions());
    const char *shaderLanguageName(ShaderLanguage language);

    // HLSL 5.0 from translateSpirv: D3D11 links a pixel shader's inputs to the vertex shader's outputs by register, which is
    // the position in each stage's signature, so a pixel shader that reads fewer values than the vertex shader writes, or
    // reads them in another order, would read the wrong ones. Rewrites the pixel shader's input structure to follow the
    // vertex shader's output order, with unused placeholders where it reads nothing. Returns false when it already matches.
    bool matchPixelInputsToVertexOutputs(const std::string &vertexSource, std::string &pixelSource);
};
