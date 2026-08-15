#include <cstdint>
#include <cstring>
#include <cmath>
#include <cassert>
#include <cstdio>
#include <cstdlib>

// RT64 classification logic
static bool is_matrix_view_proj(const float m[4][4]) {
    return std::fabs(m[3][3]) >= 1e-6f && std::fabs(1.0f - m[3][3]) >= 1e-6f;
}

static bool is_matrix_affine(const float m[4][4]) {
    return std::fabs(m[0][3]) < 1e-6f &&
           std::fabs(m[1][3]) < 1e-6f &&
           std::fabs(m[2][3]) < 1e-6f &&
           std::fabs(m[3][3] - 1.0f) < 1e-6f;
}

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

static void gu_perspective(float p[4][4], float fovy_deg, float aspect, float near_z, float far_z, float scale = 0.5f) {
    const float yscale = 1.0f / std::tan(fovy_deg * (3.14159265358979323846f / 180.0f) * 0.5f);
    std::memset(p, 0, sizeof(float) * 16);
    p[0][0] = (yscale / aspect) * scale;
    p[1][1] = yscale * scale;
    p[2][2] = ((near_z + far_z) / (near_z - far_z)) * scale;
    p[2][3] = -1.0f * scale; // -0.5f
    p[3][2] = ((2.0f * near_z * far_z) / (near_z - far_z)) * scale;
    p[3][3] = 0.0f;
}

int main() {
    std::printf("=== MATRIX SUBMISSION REGRESSION TEST ===\n");

    // Representative Wooded Hollow attract camera values
    const float r[4][4] = {
        { 0.5843f, -0.3564f,  0.7291f, 0.0f},
        { 0.0000f,  0.8984f,  0.4392f, 0.0f},
        {-0.8115f, -0.2566f,  0.5249f, 0.0f},
        { 0.0000f,  0.0000f,  0.0000f, 1.0f}
    };
    const float eye[3] = {2091.67f, 313.14f, 560.81f};
    const float scale = 1.0f;

    // 1. Perspective projection matrix P
    float P[4][4];
    gu_perspective(P, 50.0f, 4.0f / 3.0f, 10.0f, 8000.0f, 0.5f);

    // 2. Translation/Scale matrix T_S
    const float inv_scale = 1.0f / scale;
    float T_S[4][4] = {0};
    T_S[0][0] = inv_scale;
    T_S[1][1] = inv_scale;
    T_S[2][2] = inv_scale;
    T_S[3][0] = -eye[0] * inv_scale;
    T_S[3][1] = -eye[1] * inv_scale;
    T_S[3][2] = -eye[2] * inv_scale;
    T_S[3][3] = 1.0f;

    // 3. Original Tooie combined: VP_orig = T_S * (V_rot * P)
    float V_rot_P[4][4];
    mtxf_mul(V_rot_P, r, P);
    float VP_orig[4][4];
    mtxf_mul(VP_orig, T_S, V_rot_P);

    // 4. New incremental representation:
    // V_view = T_S * V_rot
    float V_view[4][4];
    mtxf_mul(V_view, T_S, r);
    // Effective matrix in vertex shader: VP_incr = V_view * P
    float VP_incr[4][4];
    mtxf_mul(VP_incr, V_view, P);

    // 5. Numerical equivalence check
    float max_err = 0.0f;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float err = std::fabs(VP_orig[i][j] - VP_incr[i][j]);
            if (err > max_err) {
                max_err = err;
            }
        }
    }
    std::printf("MAX_MATRIX_ERROR: %.6e\n", max_err);
    if (max_err > 1e-5f) {
        std::printf("MATRIX_EQUIVALENCE: FAIL\n");
        return 1;
    }
    std::printf("MATRIX_EQUIVALENCE: PASS\n");

    // 6. RT64 path classification checks
    const bool is_vp_P = is_matrix_view_proj(P);
    const bool is_affine_V = is_matrix_affine(V_view);
    std::printf("PURE_P_IS_VIEWPROJ: %s (expected False)\n", is_vp_P ? "TRUE" : "FALSE");
    std::printf("V_VIEW_IS_AFFINE: %s (expected True)\n", is_affine_V ? "TRUE" : "FALSE");

    if (is_vp_P) {
        std::printf("RT64_BAD_DECOMPOSE_PATH_AVOIDED: NO\n");
        return 2;
    }
    if (!is_affine_V) {
        std::printf("RT64_VIEW_AFFINE_PATH_CONFIRMED: NO\n");
        return 3;
    }
    std::printf("RT64_BAD_DECOMPOSE_PATH_AVOIDED: YES\n");

    // 7. Test that reversed multiplication order fails
    float VP_reversed[4][4];
    mtxf_mul(VP_reversed, P, V_view); // Intentionally reversed
    float rev_err = 0.0f;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float err = std::fabs(VP_orig[i][j] - VP_reversed[i][j]);
            if (err > rev_err) {
                rev_err = err;
            }
        }
    }
    std::printf("REVERSED_ORDER_ERROR: %.4f (must be large > 1.0)\n", rev_err);
    if (rev_err < 1.0f) {
        std::printf("ORDER_SENSITIVITY_CHECK: FAIL\n");
        return 4;
    }
    std::printf("ORDER_SENSITIVITY_CHECK: PASS\n");

    std::printf("ALL MATRIX SUBMISSION TESTS PASSED!\n");
    return 0;
}
