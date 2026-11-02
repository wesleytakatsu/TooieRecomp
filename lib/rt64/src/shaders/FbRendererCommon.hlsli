//
// RT64
//

#pragma once

#include "shared/rt64_fb_common.h"
#include "shared/rt64_frame_params.h"
#include "shared/rt64_framebuffer_params.h"
#include "shared/rt64_gpu_tile.h"
#include "shared/rt64_rdp_params.h"
#include "shared/rt64_rdp_tile.h"
#include "shared/rt64_render_indices.h"
#include "shared/rt64_render_params.h"

// Set 0 - Global resources and Enhancements (Raytracing or other).
ConstantBuffer<FrameParams> FrParams : register(b1, space0);
StructuredBuffer<RDPParams> instanceRDPParams : register(t2, space0);
StructuredBuffer<RDPTile> RDPTiles : register(t3, space0);
StructuredBuffer<GPUTile> GPUTiles : register(t4, space0);
StructuredBuffer<RenderIndices> instanceRenderIndices : register(t5, space0);
StructuredBuffer<RenderParams> DynamicRenderParams : register(t6, space0);
SamplerState gLinearWrapWrapSampler : register(s7, space0);
SamplerState gLinearWrapMirrorSampler : register(s8, space0);
SamplerState gLinearWrapClampSampler : register(s9, space0);
SamplerState gLinearMirrorWrapSampler : register(s10, space0);
SamplerState gLinearMirrorMirrorSampler : register(s11, space0);
SamplerState gLinearMirrorClampSampler : register(s12, space0);
SamplerState gLinearClampWrapSampler : register(s13, space0);
SamplerState gLinearClampMirrorSampler : register(s14, space0);
SamplerState gLinearClampClampSampler : register(s15, space0);
SamplerState gNearestWrapWrapSampler : register(s16, space0);
SamplerState gNearestWrapMirrorSampler : register(s17, space0);
SamplerState gNearestWrapClampSampler : register(s18, space0);
SamplerState gNearestMirrorWrapSampler : register(s19, space0);
SamplerState gNearestMirrorMirrorSampler : register(s20, space0);
SamplerState gNearestMirrorClampSampler : register(s21, space0);
SamplerState gNearestClampWrapSampler : register(s22, space0);
SamplerState gNearestClampMirrorSampler : register(s23, space0);
SamplerState gNearestClampClampSampler : register(s24, space0);

#if defined(TEXTURE_SLOTS)
// TK64 texture-slot mode (docs/specs/texture-slots.md): sets 1 and 2 hold only the draw's own textures, and every access
// selects its slot with a literal index so the shader also works where resource arrays cannot be indexed dynamically.
#define TEXTURE_SLOT_COUNT 8

// Set 1 - RGBA32 textures of the draw.
Texture2D<float4> gTextures[TEXTURE_SLOT_COUNT] : register(t0, space1);

// Set 2 - TMEM textures of the draw.
Texture1D<uint> gTMEM[TEXTURE_SLOT_COUNT] : register(t0, space2);

#define TEXTURE_SLOT_SWITCH(slot, EXPR) \
    switch (slot) { \
    case 0: return EXPR(0); \
    case 1: return EXPR(1); \
    case 2: return EXPR(2); \
    case 3: return EXPR(3); \
    case 4: return EXPR(4); \
    case 5: return EXPR(5); \
    case 6: return EXPR(6); \
    default: return EXPR(7); \
    }

uint loadTMEMSlot(uint slot, int2 coord) {
#define TMEM_SLOT_LOAD(i) gTMEM[i].Load(coord)
    TEXTURE_SLOT_SWITCH(slot, TMEM_SLOT_LOAD)
#undef TMEM_SLOT_LOAD
}

float4 loadTextureSlot(uint slot, int3 coord) {
#define TEXTURE_SLOT_LOAD(i) gTextures[i].Load(coord)
    TEXTURE_SLOT_SWITCH(slot, TEXTURE_SLOT_LOAD)
#undef TEXTURE_SLOT_LOAD
}

float4 sampleLevelTextureSlot(uint slot, SamplerState samplerState, float2 uv, float lod) {
#define TEXTURE_SLOT_SAMPLE_LEVEL(i) gTextures[i].SampleLevel(samplerState, uv, lod)
    TEXTURE_SLOT_SWITCH(slot, TEXTURE_SLOT_SAMPLE_LEVEL)
#undef TEXTURE_SLOT_SAMPLE_LEVEL
}

float4 sampleGradTextureSlot(uint slot, SamplerState samplerState, float2 uv, float2 ddxUV, float2 ddyUV) {
#define TEXTURE_SLOT_SAMPLE_GRAD(i) gTextures[i].SampleGrad(samplerState, uv, ddxUV, ddyUV)
    TEXTURE_SLOT_SWITCH(slot, TEXTURE_SLOT_SAMPLE_GRAD)
#undef TEXTURE_SLOT_SAMPLE_GRAD
}

#define TEXTURE_TYPE uint
#define TEXTURE_SELECT(index) (index)
#define TEXTURE_LOAD(texture, coord) loadTextureSlot(texture, coord)
#define TEXTURE_SAMPLE_LEVEL(texture, samplerState, uv, lod) sampleLevelTextureSlot(texture, samplerState, uv, lod)
#define TEXTURE_SAMPLE_GRAD(texture, samplerState, uv, ddxUV, ddyUV) sampleGradTextureSlot(texture, samplerState, uv, ddxUV, ddyUV)
#define TMEM_TYPE uint
#define TMEM_SELECT(index) (index)
#define TMEM_LOAD(tmem, coord) loadTMEMSlot(tmem, coord)

// Hardware samplers are not used in this mode: every texture is read with Load and filtered in the shader, the path RT64
// already takes whenever a tile cannot use a native sampler. It removes the texture/sampler pairs that GLSL would need
// (8 slots x 18 sampler states), which exceed GL's sampler limits. Texture packs lose hardware trilinear filtering.
#define TEXTURE_NATIVE_SAMPLER(nativeSampler) NATIVE_SAMPLER_NONE
#else
// Set 1 - RGBA32 texture cache.
Texture2D<float4> gTextures[8192] : register(t0, space1);

// Set 2 - TMEM texture cache.
Texture1D<uint> gTMEM[8192] : register(t0, space2);

#define TEXTURE_TYPE Texture2D
#define TEXTURE_SELECT(index) gTextures[NonUniformResourceIndex(index)]
#define TEXTURE_LOAD(texture, coord) texture.Load(coord)
#define TEXTURE_SAMPLE_LEVEL(texture, samplerState, uv, lod) texture.SampleLevel(samplerState, uv, lod)
#define TEXTURE_SAMPLE_GRAD(texture, samplerState, uv, ddxUV, ddyUV) texture.SampleGrad(samplerState, uv, ddxUV, ddyUV)
#define TMEM_SELECT(index) gTMEM[NonUniformResourceIndex(index)]
#define TEXTURE_NATIVE_SAMPLER(nativeSampler) (nativeSampler)
#endif

// Set 3 - Framebuffer.
ConstantBuffer<FramebufferParams> FbParams : register(b0, space3);
