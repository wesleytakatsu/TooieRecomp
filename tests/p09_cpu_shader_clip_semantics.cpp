#include "common/rt64_math.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {

int g_test_count = 0;
int g_pass_count = 0;

#define TEST_ASSERT(cond, msg) do { \
    ++g_test_count; \
    if (cond) { \
        ++g_pass_count; \
    } else { \
        std::fprintf(stderr, "FAIL: %s (line %d): %s\n", __FUNCTION__, __LINE__, msg); \
        std::exit(1); \
    } \
} while (0)

hlslpp::float4 p09ShaderEquivalentClip(const hlslpp::float4 &pos4,
    const hlslpp::float4x4 &world, const hlslpp::float4x4 &viewProj) {
    return hlslpp::mul(pos4, hlslpp::mul(world, viewProj));
}

hlslpp::float4 p09OldColumnClip(const hlslpp::float4 &pos4,
    const hlslpp::float4x4 &world, const hlslpp::float4x4 &viewProj) {
    return hlslpp::mul(hlslpp::mul(viewProj, world), pos4);
}

struct ScalarMat4 {
    float m[4][4] = {};
};

struct ScalarVec4 {
    float v[4] = {};
};

ScalarMat4 copyMat(const hlslpp::float4x4 &src) {
    ScalarMat4 out;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            out.m[r][c] = float(src[r][c]);
        }
    }
    return out;
}

ScalarVec4 copyVec(const hlslpp::float4 &src) {
    return ScalarVec4{ { float(src.x), float(src.y), float(src.z), float(src.w) } };
}

ScalarMat4 scalarMulMM(const ScalarMat4 &a, const ScalarMat4 &b) {
    ScalarMat4 out;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            float s = 0.0f;
            for (int k = 0; k < 4; k++) {
                s += a.m[r][k] * b.m[k][c];
            }
            out.m[r][c] = s;
        }
    }
    return out;
}

ScalarVec4 scalarMulVM(const ScalarVec4 &p, const ScalarMat4 &m) {
    ScalarVec4 out;
    for (int c = 0; c < 4; c++) {
        float s = 0.0f;
        for (int r = 0; r < 4; r++) {
            s += p.v[r] * m.m[r][c];
        }
        out.v[c] = s;
    }
    return out;
}

ScalarVec4 scalarMulMV(const ScalarMat4 &m, const ScalarVec4 &p) {
    ScalarVec4 out;
    for (int r = 0; r < 4; r++) {
        float s = 0.0f;
        for (int c = 0; c < 4; c++) {
            s += m.m[r][c] * p.v[c];
        }
        out.v[r] = s;
    }
    return out;
}

bool vecNear(const hlslpp::float4 &a, const ScalarVec4 &b, float eps) {
    return (std::fabs(float(a.x) - b.v[0]) <= eps) &&
        (std::fabs(float(a.y) - b.v[1]) <= eps) &&
        (std::fabs(float(a.z) - b.v[2]) <= eps) &&
        (std::fabs(float(a.w) - b.v[3]) <= eps);
}

bool vecNear4(const hlslpp::float4 &a, float x, float y, float z, float w, float eps) {
    return (std::fabs(float(a.x) - x) <= eps) &&
        (std::fabs(float(a.y) - y) <= eps) &&
        (std::fabs(float(a.z) - z) <= eps) &&
        (std::fabs(float(a.w) - w) <= eps);
}

hlslpp::float4x4 makeRz90() {
    hlslpp::float4x4 r = hlslpp::float4x4::identity();
    r[0][0] = 0.0f;
    r[0][1] = 1.0f;
    r[1][0] = -1.0f;
    r[1][1] = 0.0f;
    return r;
}

void test_translation_fixture() {
    const hlslpp::float4x4 W = RT64::matrixTranslation(hlslpp::float3(10.0f, 20.0f, 30.0f));
    const hlslpp::float4x4 Q = hlslpp::float4x4::identity();
    const hlslpp::float4 p(1.0f, 2.0f, 3.0f, 1.0f);
    const hlslpp::float4 got = p09ShaderEquivalentClip(p, W, Q);
    TEST_ASSERT(vecNear4(got, 11.0f, 22.0f, 33.0f, 1.0f, 0.0f),
        "translation fixture must be (11,22,33,1)");
    const ScalarVec4 independent = scalarMulVM(copyVec(p), scalarMulMM(copyMat(W), copyMat(Q)));
    TEST_ASSERT(vecNear(got, independent, 0.0f),
        "hlslpp helper must match independent row-vector WQ algebra");
}

void test_noncommuting_fixture() {
    const hlslpp::float4x4 S = RT64::matrixScale(hlslpp::float3(2.0f, 3.0f, 4.0f));
    const hlslpp::float4x4 Rz = makeRz90();
    const hlslpp::float4x4 T = RT64::matrixTranslation(hlslpp::float3(10.0f, 20.0f, 30.0f));
    const hlslpp::float4x4 W = hlslpp::mul(hlslpp::mul(S, Rz), T);
    TEST_ASSERT(float(W[0][0]) == 0.0f && float(W[0][1]) == 2.0f &&
        float(W[1][0]) == -3.0f && float(W[3][0]) == 10.0f &&
        float(W[3][1]) == 20.0f && float(W[3][2]) == 30.0f,
        "Astra W fixture layout");
    const hlslpp::float4x4 Qt = RT64::matrixTranslation(hlslpp::float3(-2.0f, 5.0f, 7.0f));
    const hlslpp::float4x4 Qs = RT64::matrixScale(hlslpp::float3(0.5f, 2.0f, 1.0f));
    const hlslpp::float4x4 Q = hlslpp::mul(Qt, Qs);
    const hlslpp::float4 p(1.0f, 2.0f, 3.0f, 1.0f);
    const hlslpp::float4 got = p09ShaderEquivalentClip(p, W, Q);
    TEST_ASSERT(vecNear4(got, 1.0f, 54.0f, 49.0f, 1.0f, 0.0f),
        "noncommuting fixture must be (1,54,49,1)");
    const ScalarVec4 independent = scalarMulVM(copyVec(p), scalarMulMM(copyMat(W), copyMat(Q)));
    TEST_ASSERT(vecNear(got, independent, 0.0f),
        "noncommuting hlslpp helper must match independent scalar algebra");
}

hlslpp::quaternion quatNeg(const hlslpp::quaternion &q) {
    return hlslpp::quaternion(-q.f32[0], -q.f32[1], -q.f32[2], -q.f32[3]);
}

float quatDot4(const hlslpp::quaternion &a, const hlslpp::quaternion &b) {
    return a.f32[0] * b.f32[0] + a.f32[1] * b.f32[1] +
        a.f32[2] * b.f32[2] + a.f32[3] * b.f32[3];
}

hlslpp::quaternion standardSlerp(const hlslpp::quaternion &q0In,
    const hlslpp::quaternion &q1In, float w) {
    hlslpp::quaternion q0 = hlslpp::normalize(q0In);
    hlslpp::quaternion q1 = hlslpp::normalize(q1In);
    float c = quatDot4(q0, q1);
    if (c < 0.0f) {
        q1 = quatNeg(q1);
        c = -c;
    }
    if (c < -1.0f) {
        c = -1.0f;
    }
    if (c > 1.0f) {
        c = 1.0f;
    }
    hlslpp::quaternion out;
    if (c > 0.9995f) {
        const float s = 1.0f - w;
        out = hlslpp::quaternion(
            q0.f32[0] * s + q1.f32[0] * w,
            q0.f32[1] * s + q1.f32[1] * w,
            q0.f32[2] * s + q1.f32[2] * w,
            q0.f32[3] * s + q1.f32[3] * w);
        return hlslpp::normalize(out);
    }
    const float theta = std::acos(c);
    const float sinTheta = std::sin(theta);
    const float a = std::sin((1.0f - w) * theta) / sinTheta;
    const float b = std::sin(w * theta) / sinTheta;
    out = hlslpp::quaternion(
        a * q0.f32[0] + b * q1.f32[0],
        a * q0.f32[1] + b * q1.f32[1],
        a * q0.f32[2] + b * q1.f32[2],
        a * q0.f32[3] + b * q1.f32[3]);
    return hlslpp::normalize(out);
}

float quatZAngleDeg(const hlslpp::quaternion &q) {
    return (2.0f * std::atan2(q.f32[2], q.f32[3])) * (180.0f / 3.14159265358979323846f);
}

void test_standard_slerp_reference() {
    const hlslpp::quaternion q0 = hlslpp::quaternion::identity();
    const float theta = 120.0f * (3.14159265358979323846f / 180.0f);
    const hlslpp::quaternion q1 = hlslpp::quaternion::rotation_z(theta);
    const float weights[5] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
    const float expected[5] = { 0.0f, 30.0f, 60.0f, 90.0f, 120.0f };
    constexpr float kTolDeg = 1.0e-3f;
    for (int i = 0; i < 5; i++) {
        const hlslpp::quaternion q = standardSlerp(q0, q1, weights[i]);
        const float ang = quatZAngleDeg(q);
        char buf[128];
        std::snprintf(buf, sizeof(buf), "standard slerp w=%.2f expected %.1f deg got %.9g",
            double(weights[i]), double(expected[i]), double(ang));
        TEST_ASSERT(std::fabs(ang - expected[i]) <= kTolDeg, buf);
    }
}

void test_old_and_new_differ() {
    const hlslpp::float4x4 W = RT64::matrixTranslation(hlslpp::float3(10.0f, 20.0f, 30.0f));
    const hlslpp::float4x4 Q = hlslpp::float4x4::identity();
    const hlslpp::float4 p(1.0f, 2.0f, 3.0f, 1.0f);
    const hlslpp::float4 neu = p09ShaderEquivalentClip(p, W, Q);
    const hlslpp::float4 old = p09OldColumnClip(p, W, Q);
    TEST_ASSERT(vecNear4(old, 1.0f, 2.0f, 3.0f, 141.0f, 0.0f),
        "old column formula is (1,2,3,141)");
    TEST_ASSERT(vecNear4(neu, 11.0f, 22.0f, 33.0f, 1.0f, 0.0f),
        "new row formula is (11,22,33,1)");
    TEST_ASSERT((float(old.x) != float(neu.x)) || (float(old.w) != float(neu.w)),
        "old and new must differ");
    const ScalarVec4 oldScalar = scalarMulMV(scalarMulMM(copyMat(Q), copyMat(W)), copyVec(p));
    TEST_ASSERT(vecNear(old, oldScalar, 0.0f),
        "old hlslpp matches independent column algebra");
}

} // namespace

int main() {
    test_translation_fixture();
    test_noncommuting_fixture();
    test_old_and_new_differ();
    test_standard_slerp_reference();
    std::printf("p09_cpu_shader_clip_semantics: %d/%d passed\n", g_pass_count, g_test_count);
    std::printf("SYNTH_TRANSLATION_EXPECTED=(11,22,33,1)\n");
    std::printf("SYNTH_TRANSLATION_MATCH=YES\n");
    std::printf("SYNTH_NONCOMMUTING_MATCH=YES\n");
    std::printf("SYNTH_OLD_AND_NEW_DIFFER_AS_EXPECTED=YES\n");
    std::printf("STANDARD_WORLD_SLERP_REFERENCE_VALID=YES\n");
    return 0;
}
