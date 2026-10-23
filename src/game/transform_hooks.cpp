#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "recomp.h"
#include "funcs.h"
#include "tintro_projection_policy.h"
#include "tooie_diag_path.h"

// Saved pointers for func_800E4640 reload
static uint32_t s_saved_P_mtx = 0;
static uint32_t s_saved_V_mtx = 0;

static bool tooie_pv_scale_diag_on() {
    static const bool enabled = []() {
        const char *env = std::getenv("TOOIE_DIAG_FEATHER_TEMPORAL_COHERENCY");
        if ((env == nullptr) || (env[0] == '\0') || (env[0] == '0')) {
            env = std::getenv("TOOIE_DIAG_PV_SCALE");
        }
        return (env != nullptr) && (env[0] != '\0') && (env[0] != '0');
    }();
    return enabled;
}

static void tooie_pv_scale_note(int overflow, float scale, float inv_scale,
    const float proj[4][4], const float v_view[4][4]) {
    if (!tooie_pv_scale_diag_on()) {
        return;
    }
    static uint64_t seq = 0;
    static uint64_t scale_change = 0;
    static uint64_t overflow_change = 0;
    static int last_overflow = -1;
    static float last_scale = 0.0f;
    static bool have_last = false;
    ++seq;
    if (have_last) {
        if (std::fabs(scale - last_scale) > (1.0e-6f * std::max(1.0f, std::fabs(last_scale)))) {
            ++scale_change;
        }
        if (overflow != last_overflow) {
            ++overflow_change;
        }
    }
    last_overflow = overflow;
    last_scale = scale;
    have_last = true;
    FILE *f = std::fopen(tooie_diag_path("tooie-pv-scale-status.txt").c_str(), "w");
    if (f == nullptr) {
        return;
    }
    std::fprintf(f,
        "SCHEMA=P09_TOOIE_PV_SCALE_WITNESS\n"
        "VERSION=1\n"
        "GPU_READBACK_USED=NO\n"
        "CALLS=%llu\n"
        "LAST_OVERFLOW=%d\n"
        "LAST_SCALE=%.9g\n"
        "LAST_INV_SCALE=%.9g\n"
        "LAST_P00=%.9g\n"
        "LAST_P23=%.9g\n"
        "LAST_P33=%.9g\n"
        "LAST_V00=%.9g\n"
        "LAST_VTX=%.9g\n"
        "LAST_VTY=%.9g\n"
        "LAST_VTZ=%.9g\n"
        "SCALE_CHANGE_ADJACENT_COUNT=%llu\n"
        "OVERFLOW_CHANGE_ADJACENT_COUNT=%llu\n"
        "TOOIE_SCALE_CHANGES_BETWEEN_CALLS=%s\n"
        "TOOIE_OVERFLOW_BRANCH_DIFFERS_BETWEEN_CALLS=%s\n",
        static_cast<unsigned long long>(seq),
        overflow,
        static_cast<double>(scale),
        static_cast<double>(inv_scale),
        static_cast<double>(proj[0][0]),
        static_cast<double>(proj[2][3]),
        static_cast<double>(proj[3][3]),
        static_cast<double>(v_view[0][0]),
        static_cast<double>(v_view[3][0]),
        static_cast<double>(v_view[3][1]),
        static_cast<double>(v_view[3][2]),
        static_cast<unsigned long long>(scale_change),
        static_cast<unsigned long long>(overflow_change),
        (scale_change > 0) ? "YES" : "NO",
        (overflow_change > 0) ? "YES" : "NO");
    std::fclose(f);
}

static inline int32_t& GUEST_W(uint8_t* rdram, uint32_t vram) {
    return *reinterpret_cast<int32_t*>(rdram + (vram & 0x1FFFFFFFu));
}

static inline float& GUEST_F(uint8_t* rdram, uint32_t vram) {
    return *reinterpret_cast<float*>(rdram + (vram & 0x1FFFFFFFu));
}

// Helper: 4x4 float matrix multiplication out = a * b
static void mtxf_mul(float out[4][4], const float a[4][4], const float b[4][4]) {
    float temp[4][4];
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a[r][k] * b[k][c];
            }
            temp[r][c] = sum;
        }
    }
    std::memcpy(out, temp, sizeof(temp));
}

// Convert MtxF (float[4][4]) to N64 fixed-point Mtx in guest memory
static void mtxf_to_mtx(uint8_t* rdram, uint32_t mtx_addr, const float mf[4][4]) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 2; ++j) {
            int32_t e1 = static_cast<int32_t>(mf[i][j * 2] * 65536.0f);
            int32_t e2 = static_cast<int32_t>(mf[i][j * 2 + 1] * 65536.0f);

            uint32_t word_int = (static_cast<uint32_t>(e1) & 0xFFFF0000u) |
                                ((static_cast<uint32_t>(e2) >> 16) & 0xFFFFu);
            uint32_t word_frac = ((static_cast<uint32_t>(e1) << 16) & 0xFFFF0000u) |
                                 (static_cast<uint32_t>(e2) & 0xFFFFu);

            uint32_t int_offset = (i * 2 + j) * 4;
            uint32_t frac_offset = 0x20 + (i * 2 + j) * 4;

            GUEST_W(rdram, mtx_addr + int_offset) = static_cast<int32_t>(word_int);
            GUEST_W(rdram, mtx_addr + frac_offset) = static_cast<int32_t>(word_frac);
        }
    }
}

/**
 * @brief Strong native override for func_800E44FC:
 * Builds perspective view-projection matrix, splitting display-list submission into
 * pure P (G_MTX_PROJECTION | G_MTX_LOAD) followed by affine V (G_MTX_PROJECTION | G_MTX_MUL),
 * avoiding RT64's viewproj decomposition path while preserving guest-visible D_8012D558.
 */
extern "C" void func_800E44FC(uint8_t* rdram, recomp_context* ctx) {
    const uint64_t saved_ra = ctx->r31;
    const uint64_t saved_sp = ctx->r29;
    const uint64_t saved_s0 = ctx->r16;
    const uint64_t saved_s1 = ctx->r17;
    const uint64_t saved_s2 = ctx->r18;
    const uint64_t saved_s3 = ctx->r19;
    const uint64_t saved_s4 = ctx->r20;
    const uint64_t saved_s5 = ctx->r21;
    const uint64_t saved_s6 = ctx->r22;
    const uint64_t saved_s7 = ctx->r23;
    const uint64_t saved_s8 = ctx->r30;

    const uint32_t gfx_ctx = static_cast<uint32_t>(ctx->r4);
    if (!gfx_ctx) {
        ctx->r31 = saved_ra;
        ctx->r29 = saved_sp;
        return;
    }

    // Viewport D_8012D500
    const uint32_t vp_addr = static_cast<uint32_t>(GUEST_W(rdram, 0x8012D500u));
    if (!vp_addr) {
        ctx->r31 = saved_ra;
        ctx->r29 = saved_sp;
        return;
    }

    // Set up dedicated stack frame for inner calls
    ctx->r29 = (saved_sp - 0x100) | 0xFFFFFFFF00000000ULL;
    const uint32_t temp_proj_addr = static_cast<uint32_t>(ctx->r29 + 0x40);
    const uint32_t temp_trans_addr = static_cast<uint32_t>(ctx->r29 + 0x90);

    // 1. Get translation from viewport+0x00
    float translation[3];
    for (int k = 0; k < 3; ++k) {
        translation[k] = GUEST_F(rdram, vp_addr + k * 4);
    }

    // 2. Build projection matrix P and get persp_norm via func_800CB124
    ctx->r4 = (int64_t)(int32_t)vp_addr;
    ctx->r5 = (int64_t)(int32_t)temp_proj_addr;
    LOOKUP_FUNC(0x800CB124)(rdram, ctx);
    const uint16_t persp_norm = static_cast<uint16_t>(ctx->r2);

    float proj[4][4];
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            proj[r][c] = GUEST_F(rdram, temp_proj_addr + (r * 4 + c) * 4);
        }
    }

    // 3. Get scale from func_800CA6E8 (viewport + 0x164)
    ctx->r4 = (int64_t)(int32_t)vp_addr;
    LOOKUP_FUNC(0x800CA6E8)(rdram, ctx);
    float scale = ctx->f0.fl;

    // 4. Build combined matrix on CPU via func_800E443C (and handle overflow via func_80018BC4)
    for (int k = 0; k < 3; ++k) {
        GUEST_F(rdram, temp_trans_addr + k * 4) = translation[k];
    }
    ctx->r4 = (int64_t)(int32_t)temp_proj_addr;
    ctx->r5 = (int64_t)(int32_t)temp_trans_addr;
    ctx->r6 = (int64_t)(int32_t)ctx->f0.u32l;
    LOOKUP_FUNC(0x800E443C)(rdram, ctx);

    // Check fixed-point overflow
    LOOKUP_FUNC(0x80018BC4)(rdram, ctx);
    int overflow = 0;
    if (ctx->r2 != 0) {
        overflow = 1;
        scale = 2.0f * scale;
        ctx->r4 = (int64_t)(int32_t)temp_proj_addr;
        ctx->r5 = (int64_t)(int32_t)temp_trans_addr;
        uint32_t scale_raw;
        std::memcpy(&scale_raw, &scale, sizeof(float));
        ctx->r6 = (int64_t)(int32_t)scale_raw;
        LOOKUP_FUNC(0x800E443C)(rdram, ctx);
    }

    // 5. Build View matrix V_view = T_S * V_rot
    float v_rot[4][4];
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            v_rot[r][c] = GUEST_F(rdram, vp_addr + 0xA4 + (r * 4 + c) * 4);
        }
    }

    float inv_scale = (scale != 0.0f) ? (1.0f / scale) : 1.0f;
    float t_s[4][4] = {0};
    t_s[0][0] = inv_scale;
    t_s[1][1] = inv_scale;
    t_s[2][2] = inv_scale;
    t_s[3][0] = -translation[0] * inv_scale;
    t_s[3][1] = -translation[1] * inv_scale;
    t_s[3][2] = -translation[2] * inv_scale;
    t_s[3][3] = 1.0f;

    float v_view[4][4];
    mtxf_mul(v_view, t_s, v_rot);
    tooie_pv_scale_note(overflow, scale, inv_scale, proj, v_view);

    tooie_tintro_projection_policy::apply(rdram, proj);

    // 6. Allocate Mtx entries from dynamic matrix buffer arg0->unk4
    uint32_t cur_gfx = static_cast<uint32_t>(GUEST_W(rdram, gfx_ctx + 0x00));
    uint32_t cur_mtx = static_cast<uint32_t>(GUEST_W(rdram, gfx_ctx + 0x04));

    const uint32_t mtx_p = cur_mtx;
    const uint32_t mtx_v = cur_mtx + 0x40;
    const uint32_t mtx_combined = cur_mtx + 0x80;
    const uint32_t mtx_ident = cur_mtx + 0xC0;
    GUEST_W(rdram, gfx_ctx + 0x04) = static_cast<int32_t>(cur_mtx + 0x100);

    // Write fixed-point matrices
    mtxf_to_mtx(rdram, mtx_p, proj);
    mtxf_to_mtx(rdram, mtx_v, v_view);

    // Convert combined matrix from mlMtx stack into mtx_combined using mlMtxApply (func_80018C28)
    ctx->r4 = (int64_t)(int32_t)mtx_combined;
    LOOKUP_FUNC(0x80018C28)(rdram, ctx);

    // Convert identity to mtx_ident using guMtxIdent (func_8002EF3C)
    ctx->r4 = (int64_t)(int32_t)mtx_ident;
    LOOKUP_FUNC(0x8002EF3C)(rdram, ctx);

    // 7. Update guest-visible D_8012D558 and cache pointers
    GUEST_W(rdram, 0x8012D558u) = static_cast<int32_t>(mtx_combined);
    s_saved_P_mtx = mtx_p;
    s_saved_V_mtx = mtx_v;

    // 8. Emit display list commands
    // 8.1 gSPPerspNormalize(persp_norm)
    GUEST_W(rdram, cur_gfx + 0x00) = static_cast<int32_t>(0xDB0E0000u);
    GUEST_W(rdram, cur_gfx + 0x04) = static_cast<int32_t>(persp_norm);
    cur_gfx += 8;

    // 8.2 gSPMatrix(mtx_p, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH)
    GUEST_W(rdram, cur_gfx + 0x00) = static_cast<int32_t>(0xDA380007u);
    GUEST_W(rdram, cur_gfx + 0x04) = static_cast<int32_t>(mtx_p & 0x1FFFFFFFu);
    cur_gfx += 8;

    // 8.3 gSPMatrix(mtx_v, G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH)
    GUEST_W(rdram, cur_gfx + 0x00) = static_cast<int32_t>(0xDA380005u);
    GUEST_W(rdram, cur_gfx + 0x04) = static_cast<int32_t>(mtx_v & 0x1FFFFFFFu);
    cur_gfx += 8;

    // 8.4 gSPMatrix(mtx_ident, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH)
    GUEST_W(rdram, cur_gfx + 0x00) = static_cast<int32_t>(0xDA380003u);
    GUEST_W(rdram, cur_gfx + 0x04) = static_cast<int32_t>(mtx_ident & 0x1FFFFFFFu);
    cur_gfx += 8;

    GUEST_W(rdram, gfx_ctx + 0x00) = static_cast<int32_t>(cur_gfx);

    // Restore registers
    ctx->r31 = saved_ra;
    ctx->r29 = saved_sp;
    ctx->r16 = saved_s0;
    ctx->r17 = saved_s1;
    ctx->r18 = saved_s2;
    ctx->r19 = saved_s3;
    ctx->r20 = saved_s4;
    ctx->r21 = saved_s5;
    ctx->r22 = saved_s6;
    ctx->r23 = saved_s7;
    ctx->r30 = saved_s8;
}

/**
 * @brief Strong native override for func_800E4640:
 * Resubmits the cached projection matrix representation.
 */
extern "C" void func_800E4640(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t gfx_ptr_addr = static_cast<uint32_t>(ctx->r4);
    if (!gfx_ptr_addr) {
        return;
    }
    uint32_t cur_gfx = static_cast<uint32_t>(GUEST_W(rdram, gfx_ptr_addr + 0x00));
    const uint32_t d508 = static_cast<uint32_t>(GUEST_W(rdram, 0x8012D508u));

    if (d508 != 0) {
        // gSPDisplayList((*gfx)++, 0x08000000)
        GUEST_W(rdram, cur_gfx + 0x00) = static_cast<int32_t>(0xDE000000u);
        GUEST_W(rdram, cur_gfx + 0x04) = static_cast<int32_t>(0x08000000u);
        cur_gfx += 8;
    } else {
        if (s_saved_P_mtx != 0 && s_saved_V_mtx != 0) {
            // Re-emit pure P LOAD then V MUL
            GUEST_W(rdram, cur_gfx + 0x00) = static_cast<int32_t>(0xDA380007u);
            GUEST_W(rdram, cur_gfx + 0x04) = static_cast<int32_t>(s_saved_P_mtx & 0x1FFFFFFFu);
            cur_gfx += 8;

            GUEST_W(rdram, cur_gfx + 0x00) = static_cast<int32_t>(0xDA380005u);
            GUEST_W(rdram, cur_gfx + 0x04) = static_cast<int32_t>(s_saved_V_mtx & 0x1FFFFFFFu);
            cur_gfx += 8;
        } else {
            const uint32_t d558 = static_cast<uint32_t>(GUEST_W(rdram, 0x8012D558u));
            GUEST_W(rdram, cur_gfx + 0x00) = static_cast<int32_t>(0xDA380007u);
            GUEST_W(rdram, cur_gfx + 0x04) = static_cast<int32_t>(d558 & 0x1FFFFFFFu);
            cur_gfx += 8;
        }
    }
    GUEST_W(rdram, gfx_ptr_addr + 0x00) = static_cast<int32_t>(cur_gfx);
}

/**
 * @brief Strong native override for func_800E4628:
 * Clears projection matrix cache.
 */
extern "C" void func_800E4628(uint8_t* rdram, recomp_context* ctx) {
    GUEST_W(rdram, 0x8012D554u) = 0;
    GUEST_W(rdram, 0x8012D558u) = 0;
    s_saved_P_mtx = 0;
    s_saved_V_mtx = 0;
}
