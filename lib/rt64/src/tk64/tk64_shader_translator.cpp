//
// TK64
//

#include "tk64_shader_translator.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <set>

#include "spirv_glsl.hpp"
#include "spirv_hlsl.hpp"

namespace tk64 {
    namespace {
        ShaderStage toShaderStage(spv::ExecutionModel model) {
            switch (model) {
            case spv::ExecutionModelVertex:
                return ShaderStage::Vertex;
            case spv::ExecutionModelFragment:
                return ShaderStage::Pixel;
            case spv::ExecutionModelGLCompute:
                return ShaderStage::Compute;
            default:
                return ShaderStage::Unknown;
            }
        }

        uint32_t arraySizeOf(const spirv_cross::Compiler &compiler, const spirv_cross::Resource &resource) {
            const spirv_cross::SPIRType &type = compiler.get_type(resource.type_id);
            uint32_t arraySize = 1;
            for (uint32_t size : type.array) {
                arraySize *= std::max(size, 1U);
            }

            return arraySize;
        }

        ImageDimension imageDimensionOf(spv::Dim dim) {
            switch (dim) {
            case spv::Dim1D: return ImageDimension::Texture1D;
            case spv::Dim2D: return ImageDimension::Texture2D;
            case spv::Dim3D: return ImageDimension::Texture3D;
            case spv::DimCube: return ImageDimension::Cube;
            case spv::DimBuffer: return ImageDimension::Buffer;
            default: return ImageDimension::None;
            }
        }

        bool isTexelBuffer(const spirv_cross::Compiler &compiler, const spirv_cross::Resource &resource) {
            return compiler.get_type(resource.base_type_id).image.dim == spv::DimBuffer;
        }

        // SPIR-V storage image formats and the GL internal formats they declare.
        const std::pair<spv::ImageFormat, uint32_t> ImageFormatTable[] = {
            { spv::ImageFormatRgba32f, 0x8814 }, { spv::ImageFormatRgba16f, 0x881A }, { spv::ImageFormatR32f, 0x822E },
            { spv::ImageFormatRgba8, 0x8058 }, { spv::ImageFormatRgba8Snorm, 0x8F97 }, { spv::ImageFormatRg32f, 0x8230 },
            { spv::ImageFormatRg16f, 0x822F }, { spv::ImageFormatR16f, 0x822D }, { spv::ImageFormatRgba16, 0x805B },
            { spv::ImageFormatRgba32i, 0x8D82 }, { spv::ImageFormatRgba16i, 0x8D88 }, { spv::ImageFormatRgba8i, 0x8D8E },
            { spv::ImageFormatR32i, 0x8235 }, { spv::ImageFormatRgba32ui, 0x8D70 }, { spv::ImageFormatRgba16ui, 0x8D76 },
            { spv::ImageFormatRgba8ui, 0x8D7C }, { spv::ImageFormatR32ui, 0x8236 }, { spv::ImageFormatR16ui, 0x8234 },
            { spv::ImageFormatR8ui, 0x8232 },
        };

        // The GL internal format a storage image is declared with: the SPIR-V format, or one deduced from the sampled
        // type when the format is unknown. 0 means a write-only image declared without a format.
        uint32_t glImageFormatOf(const spirv_cross::Compiler &compiler, const spirv_cross::Resource &resource) {
            const spirv_cross::SPIRType &type = compiler.get_type(resource.base_type_id);
            if ((type.image.format == spv::ImageFormatUnknown) && compiler.has_decoration(resource.id, spv::DecorationNonReadable)) {
                return 0;
            }

            for (const auto &entry : ImageFormatTable) {
                if (entry.first == type.image.format) {
                    return entry.second;
                }
            }

            const spirv_cross::SPIRType &sampled = compiler.get_type(type.image.type);
            const bool isFloat = (sampled.basetype == spirv_cross::SPIRType::Float);
            const bool isInt = (sampled.basetype == spirv_cross::SPIRType::Int);
            switch (sampled.vecsize) {
            case 1: return isFloat ? 0x822E : (isInt ? 0x8235 : 0x8236);
            case 2: return isFloat ? 0x8230 : (isInt ? 0x823B : 0x823C);
            default: return isFloat ? 0x8814 : (isInt ? 0x8D82 : 0x8D70);
            }
        }

        // Rewrites every resource's (set, binding) into the binding point or register of the target API.
        void remapBindings(spirv_cross::CompilerGLSL &compiler, bool hlsl, const BindingMapper &mapper, TranslatedShader &translated) {
            const spirv_cross::ShaderResources resources = compiler.get_shader_resources();
            std::map<ResourceClass, uint32_t> denseCounters;

            // Pixel-shader UAVs share registers with the render targets in SM 5.0, so they start after the 8 possible targets.
            if (hlsl && (compiler.get_execution_model() == spv::ExecutionModelFragment)) {
                denseCounters[ResourceClass::StorageBuffer] = 8;
                denseCounters[ResourceClass::StorageImage] = 8;
            }

            auto assign = [&](const spirv_cross::Resource &resource, ResourceClass resourceClass) {
                ResourceLocation location;
                location.resourceClass = resourceClass;
                location.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                location.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                location.arraySize = arraySizeOf(compiler, resource);

                // In D3D11 UAVs share one register range, whether buffers or textures.
                ResourceClass counterClass = resourceClass;
                if (hlsl && (resourceClass == ResourceClass::StorageImage)) {
                    counterClass = ResourceClass::StorageBuffer;
                }
                else if (hlsl && ((resourceClass == ResourceClass::ReadOnlyStorageBuffer) || (resourceClass == ResourceClass::Texture))) {
                    counterClass = ResourceClass::Texture;
                }

                uint32_t target = 0;
                if (mapper) {
                    target = mapper(location);
                }
                else {
                    target = denseCounters[counterClass];
                    denseCounters[counterClass] += location.arraySize;
                }

                compiler.unset_decoration(resource.id, spv::DecorationDescriptorSet);
                compiler.set_decoration(resource.id, spv::DecorationBinding, target);

                // GL matches buffer blocks between the stages of a program by name, and a matched block must be declared
                // identically everywhere. SPIRV-Cross names anonymous blocks after SPIR-V ids, which can collide between
                // modules, and some drivers (AMD) reject a runtime-sized array that one stage only indexes with constants.
                // Blocks are named after their descriptor location and the stage instead: no block is matched across
                // stages, and each stage binds its own block to the same binding point.
                const bool bufferBlock = (resourceClass == ResourceClass::UniformBuffer) || (resourceClass == ResourceClass::StorageBuffer) ||
                    (resourceClass == ResourceClass::ReadOnlyStorageBuffer);
                if (!hlsl && bufferBlock) {
                    const char *stageSuffix = (translated.stage == ShaderStage::Vertex) ? "_vs" : ((translated.stage == ShaderStage::Pixel) ? "_ps" : "");
                    const std::string suffix = "s" + std::to_string(location.set) + "_b" + std::to_string(location.binding) + stageSuffix;
                    compiler.set_name(resource.base_type_id, "tk64_block_" + suffix);
                    compiler.set_name(resource.id, "tk64_buffer_" + suffix);
                }

                BoundResource bound;
                bound.resourceClass = resourceClass;
                bound.set = location.set;
                bound.binding = location.binding;
                bound.arraySize = location.arraySize;
                bound.targetBinding = target;
                if (resourceClass == ResourceClass::StorageImage) {
                    bound.texelBuffer = isTexelBuffer(compiler, resource);
                    bound.glImageFormat = glImageFormatOf(compiler, resource);
                    bound.writeOnly = compiler.has_decoration(resource.id, spv::DecorationNonReadable);
                }

                if ((resourceClass == ResourceClass::StorageImage) || (resourceClass == ResourceClass::Texture)) {
                    bound.imageDimension = imageDimensionOf(compiler.get_type(resource.base_type_id).image.dim);
                }

                translated.resources.emplace_back(bound);
            };

            for (const spirv_cross::Resource &resource : resources.uniform_buffers) {
                assign(resource, ResourceClass::UniformBuffer);
            }

            for (const spirv_cross::Resource &resource : resources.storage_buffers) {
                const bool readOnly = compiler.get_buffer_block_flags(resource.id).get(spv::DecorationNonWritable);
                assign(resource, (hlsl && readOnly) ? ResourceClass::ReadOnlyStorageBuffer : ResourceClass::StorageBuffer);
            }

            for (const spirv_cross::Resource &resource : resources.storage_images) {
                assign(resource, ResourceClass::StorageImage);
            }

            // In GLSL only the combined samplers exist in the output; they get their texture units separately.
            if (!hlsl) {
                return;
            }

            for (const spirv_cross::Resource &resource : resources.sampled_images) {
                assign(resource, ResourceClass::Texture);
            }

            for (const spirv_cross::Resource &resource : resources.separate_images) {
                assign(resource, ResourceClass::Texture);
            }

            for (const spirv_cross::Resource &resource : resources.separate_samplers) {
                assign(resource, ResourceClass::Sampler);
            }
        }

        // Gives every combined sampler its texture unit (dense from 0, or through the mapper with the texture's location).
        void assignTextureUnits(spirv_cross::CompilerGLSL &compiler, const BindingMapper &mapper, TranslatedShader &translated) {
            const auto &combinedSamplers = compiler.get_combined_image_samplers();
            uint32_t nextUnit = 0;
            for (size_t i = 0; i < combinedSamplers.size(); i++) {
                CombinedSampler &entry = translated.combinedSamplers[i];
                const spirv_cross::SPIRType &combinedType = compiler.get_type_from_variable(combinedSamplers[i].combined_id);
                entry.arraySize = combinedType.array.empty() ? 1 : std::max(combinedType.array[0], 1U);
                if (mapper) {
                    ResourceLocation location;
                    location.resourceClass = ResourceClass::Texture;
                    location.set = entry.imageSet;
                    location.binding = entry.imageBinding;
                    location.arraySize = entry.arraySize;
                    entry.textureUnit = mapper(location);
                }
                else {
                    entry.textureUnit = nextUnit;
                    nextUnit += entry.arraySize;
                }

                compiler.set_decoration(combinedSamplers[i].combined_id, spv::DecorationBinding, entry.textureUnit);
            }

            // Uniform texel buffers (HLSL Buffer<T>) are not combined: GLSL declares them as samplerBuffer directly. They
            // still take a texture unit, and are reported like combined samplers without a sampler.
            const spirv_cross::ShaderResources resources = compiler.get_shader_resources();
            for (const spirv_cross::Resource &resource : resources.separate_images) {
                if (compiler.get_type_from_variable(resource.id).image.dim != spv::DimBuffer) {
                    continue;
                }

                bool combined = false;
                for (const spirv_cross::CombinedImageSampler &existing : combinedSamplers) {
                    combined = combined || (existing.image_id == resource.id);
                }

                if (combined) {
                    continue;
                }

                CombinedSampler entry;
                entry.name = compiler.get_name(resource.id);
                entry.imageSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                entry.imageBinding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                entry.dummySampler = true;
                entry.texelBuffer = true;
                if (mapper) {
                    ResourceLocation location;
                    location.resourceClass = ResourceClass::Texture;
                    location.set = entry.imageSet;
                    location.binding = entry.imageBinding;
                    location.arraySize = 1;
                    entry.textureUnit = mapper(location);
                }
                else {
                    entry.textureUnit = nextUnit;
                    nextUnit += 1;
                }

                compiler.unset_decoration(resource.id, spv::DecorationDescriptorSet);
                compiler.set_decoration(resource.id, spv::DecorationBinding, entry.textureUnit);
                translated.combinedSamplers.emplace_back(entry);
            }
        }

        // GLSL ES only accepts the output index qualifier with EXT_blend_func_extended. Index 0 alone is the default,
        // so it is dropped; a second index (dual-source blending) requires the extension.
#ifdef TK64_MOBILE
        bool fixOutputIndices(spirv_cross::CompilerGLSL &compiler, TranslatedShader &translated, const TranslationOptions &translationOptions) {
#else
        void fixOutputIndices(spirv_cross::CompilerGLSL &compiler, TranslatedShader &translated) {
#endif
            bool dualSource = false;
            const spirv_cross::ShaderResources resources = compiler.get_shader_resources();
            for (const spirv_cross::Resource &output : resources.stage_outputs) {
                if (compiler.has_decoration(output.id, spv::DecorationIndex) && (compiler.get_decoration(output.id, spv::DecorationIndex) > 0)) {
                    dualSource = true;
                }
            }

#ifdef TK64_MOBILE
            if (dualSource && !translationOptions.dualSourceAsVariable) {
                compiler.require_extension("GL_EXT_blend_func_extended");
                translated.requiredExtensions.emplace_back("GL_EXT_blend_func_extended");
                return false;
            }
#else
            if (dualSource) {
                compiler.require_extension("GL_EXT_blend_func_extended");
                translated.requiredExtensions.emplace_back("GL_EXT_blend_func_extended");
                return;
            }
#endif

            for (const spirv_cross::Resource &output : resources.stage_outputs) {
#ifdef TK64_MOBILE
                if (dualSource && (compiler.get_decoration(output.id, spv::DecorationIndex) > 0)) {
                    compiler.set_decoration(output.id, spv::DecorationLocation, 1);
                }

#endif
                compiler.unset_decoration(output.id, spv::DecorationIndex);
            }
#ifdef TK64_MOBILE

            if (dualSource && translationOptions.dualSourceBlend) {
                compiler.require_extension("GL_EXT_shader_framebuffer_fetch");
                translated.requiredExtensions.emplace_back("GL_EXT_shader_framebuffer_fetch");
            }

            return dualSource;
#endif
        }
#ifdef TK64_MOBILE

        // See TranslationOptions::dropLastTmemSlot.
        void dropLastTmemSlot(std::string &source) {
            const std::string type = "usampler2D ";
            const size_t position = source.find(type);
            if (position == std::string::npos) {
                return;
            }

            const size_t nameStart = position + type.size();
            const size_t nameEnd = source.find('[', nameStart);
            const size_t declarationEnd = source.find(';', nameStart);
            if ((nameEnd == std::string::npos) || (nameEnd > declarationEnd) || (source.compare(nameEnd, 3, "[8]") != 0)) {
                return;
            }

            const std::string name = source.substr(nameStart, nameEnd - nameStart);
            source[nameEnd + 1] = '7';
            const std::string last = name + "[7]";
            for (size_t at = source.find(last, declarationEnd); at != std::string::npos; at = source.find(last, at + last.size())) {
                source[at + name.size() + 1] = '6';
            }
        }

        // See TranslationOptions::depthFetch. A multisampled or array texture has another declaration and stays a texture read.
        void rewriteDepthFetch(std::string &source, uint32_t textureUnit, TranslatedShader &translated) {
            const std::string declaration = "layout(binding = " + std::to_string(textureUnit) + ") uniform highp sampler2D ";
            const size_t position = source.find(declaration);
            if (position == std::string::npos) {
                return;
            }

            const size_t nameStart = position + declaration.size();
            const size_t declarationEnd = source.find(';', nameStart);
            const std::string call = "texelFetch(" + source.substr(nameStart, declarationEnd - nameStart) + ",";
            bool rewritten = false;
            for (size_t at = source.find(call, declarationEnd); at != std::string::npos; at = source.find(call, at)) {
                size_t end = at + call.size();
                for (int open = 1; (open > 0) && (end < source.size()); end++) {
                    open += (source[end] == '(') ? 1 : ((source[end] == ')') ? -1 : 0);
                }

                source.replace(at, end - at, "vec4(gl_LastFragDepthARM)");
                rewritten = true;
            }

            if (rewritten) {
                const std::string extension = "GL_ARM_shader_framebuffer_fetch_depth_stencil";
                source.insert(source.find('\n') + 1, "#extension " + extension + " : require\n");
                translated.requiredExtensions.emplace_back(extension);
            }
        }

        // See TranslationOptions::dualSourceAsVariable. fixOutputIndices left the outputs at locations 0 and 1.
        void rewriteDualSourceOutputs(std::string &source, const TranslationOptions &translationOptions) {
            const std::string colorDeclaration = "layout(location = 0) out ";
            const std::string factorDeclaration = "layout(location = 1) out ";
            const auto declaredName = [&](const std::string &declaration) {
                const size_t position = source.find(declaration);
                if (position == std::string::npos) {
                    throw std::runtime_error("dual-source output not found: " + declaration);
                }

                const size_t end = source.find(';', position);
                const size_t start = source.rfind(' ', end) + 1;
                return source.substr(start, end - start);
            };

            const std::string color = declaredName(colorDeclaration);
            const std::string factor = declaredName(factorDeclaration);
            source.erase(source.find(factorDeclaration), factorDeclaration.size());
            if (!translationOptions.dualSourceBlend) {
                return;
            }

            source.replace(source.find(colorDeclaration), colorDeclaration.size(), "layout(location = 0) inout ");

            // main is the last function SPIRV-Cross writes, and this kind of shader leaves it early only by discarding.
            std::string blend = "    " + color + ".rgb = (" + color + ".rgb * " + factor + ".a) + (tk64Target.rgb * (1.0 - " + factor + ".a));\n";
            if (translationOptions.dualSourceAddDstAlpha) {
                blend += "    " + color + ".a += tk64Target.a;\n";
            }

            source.insert(source.rfind('}'), blend);
            source.insert(source.find('{', source.find("void main()")) + 1, "\n    highp vec4 tk64Target = " + color + ";");
        }
#endif

        // DXC gives every RWTexture2D<T> a format from T alone (float4 becomes rgba32f), while the images bound to it at run
        // time have their own formats (RT64 decodes textures into RGBA8). GL requires the image unit format to match the
        // declared one. Images the shader never reads are marked write-only; desktop GLSL then declares them without a
        // format (only loads need one), and GLSL ES, which always needs one, takes it from the options when given. DXC does
        // not mark write-only images itself (SPIRV-Cross only deduces it while compiling), so readImages comes from
        // scanning the module.
        void prepareWriteOnlyImages(spirv_cross::CompilerGLSL &compiler, const std::set<uint32_t> &readImages, bool es,
            const std::vector<TranslationOptions::ImageFormat> &imageFormats)
        {
            const spirv_cross::ShaderResources resources = compiler.get_shader_resources();
            std::set<uint32_t> readTypes;
            for (const spirv_cross::Resource &image : resources.storage_images) {
                if (readImages.count(image.id) > 0) {
                    readTypes.insert(image.base_type_id);
                }
            }

            for (const spirv_cross::Resource &image : resources.storage_images) {
                if (readTypes.count(image.base_type_id) > 0) {
                    continue;
                }

                spv::ImageFormat format = spv::ImageFormatUnknown;
                if (es) {
                    format = compiler.get_type(image.base_type_id).image.format;
                    const uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
                    const uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);
                    for (const TranslationOptions::ImageFormat &imageFormat : imageFormats) {
                        if ((imageFormat.set != set) || (imageFormat.binding != binding)) {
                            continue;
                        }

                        for (const auto &entry : ImageFormatTable) {
                            format = (entry.second == imageFormat.glFormat) ? entry.first : format;
                        }
                    }
                }

                // SPIRV-Cross has no setter for the format; the variable's pointer type carries its own copy of it.
                const_cast<spirv_cross::SPIRType &>(compiler.get_type(image.base_type_id)).image.format = format;
                const_cast<spirv_cross::SPIRType &>(compiler.get_type(image.type_id)).image.format = format;
                compiler.set_decoration(image.id, spv::DecorationNonReadable);
            }
        }

        // Gives the push constant block fixed names. GLSL turns it into a plain uniform struct that every stage of a program
        // must declare identically, and the host sets it member by member by name. Optimized SPIR-V (RT64's specialized
        // raster shaders) carries no names at all, so names come from the member offsets instead of the source.
        void namePushConstants(spirv_cross::Compiler &compiler) {
            const spirv_cross::ShaderResources resources = compiler.get_shader_resources();
            if (resources.push_constant_buffers.empty()) {
                return;
            }

            const spirv_cross::Resource &block = resources.push_constant_buffers[0];
            const spirv_cross::SPIRType &type = compiler.get_type(block.base_type_id);
            compiler.set_name(block.id, "tk64_push");
            compiler.set_name(type.self, "tk64_push_block");
            for (uint32_t m = 0; m < uint32_t(type.member_types.size()); m++) {
                const uint32_t offset = compiler.get_member_decoration(type.self, m, spv::DecorationOffset);
                compiler.set_member_name(type.self, m, "m" + std::to_string(offset));
            }
        }

        // Describes the push constant block's members; GLSL emits it as a plain uniform struct.
        void reflectPushConstants(const spirv_cross::CompilerGLSL &compiler, TranslatedShader &translated) {
            const spirv_cross::ShaderResources resources = compiler.get_shader_resources();
            if (resources.push_constant_buffers.empty()) {
                return;
            }

            const spirv_cross::Resource &block = resources.push_constant_buffers[0];
            translated.pushConstantBlockName = compiler.get_name(block.id);
            const spirv_cross::SPIRType &type = compiler.get_type(block.base_type_id);
            for (uint32_t m = 0; m < uint32_t(type.member_types.size()); m++) {
                const spirv_cross::SPIRType &memberType = compiler.get_type(type.member_types[m]);
                PushConstantMember member;
                member.name = compiler.get_member_name(type.self, m);
                member.offset = compiler.get_member_decoration(type.self, m, spv::DecorationOffset);
                member.vectorSize = memberType.vecsize;
                member.columns = memberType.columns;
                member.arraySize = memberType.array.empty() ? 0 : memberType.array[0];
                if (!memberType.array.empty()) {
                    member.arrayStride = compiler.type_struct_member_array_stride(type, m);
                }

                if (memberType.columns > 1) {
                    member.matrixStride = compiler.type_struct_member_matrix_stride(type, m);
                    member.rowMajor = compiler.has_member_decoration(type.self, m, spv::DecorationRowMajor);
                }

                switch (memberType.basetype) {
                case spirv_cross::SPIRType::Int:
                    member.type = ScalarType::Int;
                    break;
                case spirv_cross::SPIRType::UInt:
                case spirv_cross::SPIRType::Boolean:
                    member.type = ScalarType::UInt;
                    break;
                default:
                    member.type = ScalarType::Float;
                    break;
                }

                translated.pushConstantMembers.emplace_back(member);
            }
        }
    };

    // The image variables a module reads from: image reads and texel pointers (atomics), traced back through loads and
    // access chains (arrays of images) to the variable.
    static std::set<uint32_t> findReadImages(const uint32_t *words, size_t wordCount) {
        std::map<uint32_t, uint32_t> source;
        std::set<uint32_t> readIds;
        size_t i = 5;
        while (i < wordCount) {
            const uint32_t opcode = words[i] & 0xFFFFu;
            const uint32_t length = words[i] >> 16;
            if ((length == 0) || ((i + length) > wordCount)) {
                break;
            }

            const bool chain = (opcode == spv::OpLoad) || (opcode == spv::OpAccessChain) || (opcode == spv::OpInBoundsAccessChain);
            if (chain && (length >= 4)) {
                source[words[i + 2]] = words[i + 3];
            }
            else if (((opcode == spv::OpImageRead) || (opcode == spv::OpImageSparseRead) || (opcode == spv::OpImageTexelPointer)) && (length >= 4)) {
                readIds.insert(words[i + 3]);
            }

            i += length;
        }

        std::set<uint32_t> readImages;
        for (uint32_t id : readIds) {
            for (size_t step = 0; step < source.size(); step++) {
                const auto it = source.find(id);
                if (it == source.end()) {
                    break;
                }

                id = it->second;
            }

            readImages.insert(id);
        }

        return readImages;
    }

    // One member of a SPIRV-Cross HLSL stage structure: its text line and its semantic.
    struct InterfaceMember {
        std::string line;
        std::string semantic;
    };

    // Finds "struct <name>\n{\n ... };" and splits its members. Returns false when the structure is absent.
    static bool findInterface(const std::string &source, const char *name, size_t &bodyBegin, size_t &bodyEnd, std::vector<InterfaceMember> &members) {
        const std::string header = std::string("struct ") + name + "\n{\n";
        const size_t begin = source.find(header);
        if (begin == std::string::npos) {
            return false;
        }

        bodyBegin = begin + header.size();
        bodyEnd = source.find("};\n", bodyBegin);
        if (bodyEnd == std::string::npos) {
            return false;
        }

        size_t lineBegin = bodyBegin;
        while (lineBegin < bodyEnd) {
            const size_t lineEnd = source.find('\n', lineBegin);
            const std::string line = source.substr(lineBegin, lineEnd - lineBegin);
            lineBegin = lineEnd + 1;
            const size_t colon = line.rfind(" : ");
            const size_t semicolon = line.rfind(';');
            if ((colon == std::string::npos) || (semicolon == std::string::npos) || (semicolon < colon)) {
                continue;
            }

            InterfaceMember member;
            member.line = line;
            member.semantic = line.substr(colon + 3, semicolon - colon - 3);
            members.emplace_back(member);
        }

        return true;
    }

    // Values the rasterizer generates for the pixel shader; they are not linked to vertex shader outputs.
    static bool isPixelSystemValue(const std::string &semantic) {
        return (semantic == "SV_IsFrontFace") || (semantic == "SV_SampleIndex") || (semantic == "SV_Coverage") || (semantic == "SV_PrimitiveID");
    }

    bool matchPixelInputsToVertexOutputs(const std::string &vertexSource, std::string &pixelSource) {
        size_t vertexBegin = 0;
        size_t vertexEnd = 0;
        std::vector<InterfaceMember> outputs;
        if (!findInterface(vertexSource, "SPIRV_Cross_Output", vertexBegin, vertexEnd, outputs)) {
            return false;
        }

        size_t pixelBegin = 0;
        size_t pixelEnd = 0;
        std::vector<InterfaceMember> inputs;
        if (!findInterface(pixelSource, "SPIRV_Cross_Input", pixelBegin, pixelEnd, inputs)) {
            return false;
        }

        // Already linked when the linked inputs are the first outputs, in the same order.
        std::vector<InterfaceMember> linked;
        std::vector<InterfaceMember> systemValues;
        for (const InterfaceMember &input : inputs) {
            (isPixelSystemValue(input.semantic) ? systemValues : linked).emplace_back(input);
        }

        bool matches = (linked.size() <= outputs.size());
        for (size_t i = 0; matches && (i < linked.size()); i++) {
            matches = (linked[i].semantic == outputs[i].semantic);
        }

        if (matches) {
            return false;
        }

        // Follow the vertex outputs up to the last one the pixel shader reads; placeholders fill the rest.
        size_t lastRead = 0;
        for (size_t o = 0; o < outputs.size(); o++) {
            for (const InterfaceMember &input : linked) {
                lastRead = (input.semantic == outputs[o].semantic) ? (o + 1) : lastRead;
            }
        }

        std::string body;
        for (size_t o = 0; o < lastRead; o++) {
            bool found = false;
            for (const InterfaceMember &input : linked) {
                if (input.semantic == outputs[o].semantic) {
                    body += input.line + "\n";
                    found = true;
                    break;
                }
            }

            // A placeholder declares the output's own type, so its component mask is the one the vertex shader writes.
            if (!found) {
                const std::string &line = outputs[o].line;
                const size_t colon = line.rfind(" : ");
                const size_t nameBegin = line.rfind(' ', colon - 1) + 1;
                std::string declaration = line.substr(0, nameBegin);
                for (const char *modifier : { "nointerpolation ", "noperspective ", "centroid ", "sample " }) {
                    const size_t at = declaration.find(modifier);
                    if (at != std::string::npos) {
                        declaration.erase(at, strlen(modifier));
                    }
                }

                body += declaration + "tk64_unused" + std::to_string(o) + line.substr(colon) + "\n";
            }
        }

        // Inputs no vertex output provides keep their place after the linked ones, so the shader still compiles.
        for (const InterfaceMember &input : linked) {
            bool provided = false;
            for (const InterfaceMember &output : outputs) {
                provided = provided || (output.semantic == input.semantic);
            }

            if (!provided) {
                body += input.line + "\n";
            }
        }

        for (const InterfaceMember &input : systemValues) {
            body += input.line + "\n";
        }

        pixelSource.replace(pixelBegin, pixelEnd - pixelBegin, body);
        return true;
    }

    const char *shaderLanguageName(ShaderLanguage language) {
        switch (language) {
        case ShaderLanguage::GLSL_ES_310:
            return "glsl-es-310";
        case ShaderLanguage::GLSL_430:
            return "glsl-430";
        case ShaderLanguage::HLSL_50:
            return "hlsl-50";
        default:
            return "unknown";
        }
    }

    TranslatedShader translateSpirv(const uint32_t *words, size_t wordCount, ShaderLanguage language, const BindingMapper &mapper,
        const TranslationOptions &translationOptions)
    {
        TranslatedShader translated;
        try {
            if (language == ShaderLanguage::HLSL_50) {
                spirv_cross::CompilerHLSL compiler(words, wordCount);
                spirv_cross::CompilerHLSL::Options hlslOptions;
                hlslOptions.shader_model = 50;
                compiler.set_hlsl_options(hlslOptions);

                // The blobs are built for Vulkan's clip space (-fvk-invert-y); D3D's Y points the other way.
                spirv_cross::CompilerGLSL::Options commonOptions = compiler.get_common_options();
                commonOptions.vertex.flip_vert_y = true;
                compiler.set_common_options(commonOptions);
                translated.stage = toShaderStage(compiler.get_execution_model());
                remapBindings(compiler, true, mapper, translated);

                // Without a layout, SPIRV-Cross leaves the push constant cbuffer without a register and FXC picks one.
                const spirv_cross::ShaderResources resources = compiler.get_shader_resources();
                if (!resources.push_constant_buffers.empty()) {
                    uint32_t nextConstantBuffer = 0;
                    for (const BoundResource &bound : translated.resources) {
                        if (bound.resourceClass == ResourceClass::UniformBuffer) {
                            nextConstantBuffer = std::max(nextConstantBuffer, bound.targetBinding + bound.arraySize);
                        }
                    }

                    spirv_cross::RootConstants rootConstants;
                    rootConstants.start = 0;
                    rootConstants.end = UINT32_MAX;
                    rootConstants.binding = nextConstantBuffer;
                    rootConstants.space = 0;
                    compiler.set_root_constant_layouts({ rootConstants });
                    const spirv_cross::SPIRType &blockType = compiler.get_type(resources.push_constant_buffers[0].base_type_id);
                    translated.pushConstantRegister = nextConstantBuffer;
                    translated.pushConstantSize = uint32_t(compiler.get_declared_struct_size(blockType));
                }

                translated.source = compiler.compile();
            }
            else {
                spirv_cross::CompilerGLSL compiler(words, wordCount);
                spirv_cross::CompilerGLSL::Options options;
                options.es = (language == ShaderLanguage::GLSL_ES_310);
                options.version = options.es ? 310 : 430;
                options.vulkan_semantics = false;
                options.enable_420pack_extension = false;
                options.vertex.fixup_clipspace = translationOptions.glClipSpaceFixup;
                compiler.set_common_options(options);
                translated.stage = toShaderStage(compiler.get_execution_model());

                const uint32_t dummySampler = compiler.build_dummy_sampler_for_combined_images();
                compiler.build_combined_image_samplers();
#ifdef TK64_MOBILE
                const bool rewriteDualSource = options.es && fixOutputIndices(compiler, translated, translationOptions);
#else
                if (options.es) {
                    fixOutputIndices(compiler, translated);
                }
#endif

                // Record what each combined sampler is made of before the bindings are rewritten.
                for (const spirv_cross::CombinedImageSampler &combined : compiler.get_combined_image_samplers()) {
                    CombinedSampler entry;
                    entry.name = compiler.get_name(combined.combined_id);
                    entry.imageSet = compiler.get_decoration(combined.image_id, spv::DecorationDescriptorSet);
                    entry.imageBinding = compiler.get_decoration(combined.image_id, spv::DecorationBinding);
                    entry.dummySampler = (dummySampler != 0) && (combined.sampler_id == dummySampler);
                    entry.texelBuffer = (compiler.get_type_from_variable(combined.image_id).image.dim == spv::DimBuffer);
                    if (!entry.dummySampler) {
                        entry.samplerSet = compiler.get_decoration(combined.sampler_id, spv::DecorationDescriptorSet);
                        entry.samplerBinding = compiler.get_decoration(combined.sampler_id, spv::DecorationBinding);
                    }

                    translated.combinedSamplers.emplace_back(entry);
                }

                prepareWriteOnlyImages(compiler, findReadImages(words, wordCount), options.es, translationOptions.imageFormats);

                remapBindings(compiler, false, mapper, translated);
                assignTextureUnits(compiler, mapper, translated);
                namePushConstants(compiler);
                translated.source = compiler.compile();
#ifdef TK64_MOBILE
                if (translationOptions.clampVertexDepth && (translated.stage == ShaderStage::Vertex)) {
                    translated.source.insert(translated.source.rfind('}'), "    gl_Position.z = clamp(gl_Position.z, -abs(gl_Position.w), abs(gl_Position.w));\n");
                }

                if (rewriteDualSource) {
                    rewriteDualSourceOutputs(translated.source, translationOptions);
                }

                if (translationOptions.dropLastTmemSlot) {
                    dropLastTmemSlot(translated.source);
                }

                if (translationOptions.depthFetch) {
                    for (const CombinedSampler &combined : translated.combinedSamplers) {
                        if ((combined.imageSet == translationOptions.depthFetchSet) && (combined.imageBinding == translationOptions.depthFetchBinding)) {
                            rewriteDepthFetch(translated.source, combined.textureUnit, translated);
                        }
                    }
                }

#endif
                reflectPushConstants(compiler, translated);
            }

            translated.ok = true;
        }
        catch (const std::exception &e) {
            translated.error = e.what();
        }

        return translated;
    }
};
