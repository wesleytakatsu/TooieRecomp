// Host test for the P08 widescreen frustum header:
//   src/game/tooie_ws_frustum.h

#include "tooie_ws_frustum.h"

#include <cmath>
#include <cstdint>
#include <cstdio>

namespace {

using tooie_ws_frustum::Census;
using tooie_ws_frustum::Volume;

int g_test_count = 0;
int g_pass_count = 0;

#define CHECK(cond) do { \
    ++g_test_count; \
    if (cond) { \
        ++g_pass_count; \
    } else { \
        std::fprintf(stderr, "FAIL: %s line %d: %s\n", __FUNCTION__, __LINE__, #cond); \
    } \
} while (0)

const float kUp[3] = { 0.0f, 1.0f, 0.0f };
const float kDepth = 10.0f;
const float kTanV = std::tan(20.0f * 3.14159265358979f / 180.0f);
const float kTanH = (4.0f / 3.0f) * kTanV;
const float kKws = (16.0f / 9.0f) / (4.0f / 3.0f);

bool approx(float a, float b, float tol) { return std::fabs(a - b) <= tol; }

float dot3(const float a[3], const float b[3]) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

void unit3(float v[3]) {
    const float l = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] /= l;
    v[1] /= l;
    v[2] /= l;
}

struct Frame {
    float cam[3];
    float fwd[3];
    float right[3];
};

// fwd is unit length; right = fwd x up.
Frame makeFrame(float cx, float cy, float cz, float fx, float fy, float fz) {
    Frame f{};
    f.cam[0] = cx; f.cam[1] = cy; f.cam[2] = cz;
    f.fwd[0] = fx; f.fwd[1] = fy; f.fwd[2] = fz;
    f.right[0] = -fz; f.right[1] = 0.0f; f.right[2] = fx;
    return f;
}

const Frame kOrigin = makeFrame(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f);
const Frame kMoved = makeFrame(100.0f, 20.0f, -50.0f, 0.5f, 0.0f, -0.8660254f);

// Original (unexpanded) 4:3 planes with outward normals; planes = {right, left, top, bottom}.
Volume makeVolume(const Frame& f) {
    Volume v{};
    for (int i = 0; i < 3; ++i) {
        v.cam[i] = f.cam[i];
        v.fwd[i] = f.fwd[i];
    }
    for (int p = 0; p < 4; ++p) {
        const bool horizontal = p < 2;
        const float sign = (p % 2 == 0) ? 1.0f : -1.0f;
        const float tan = horizontal ? kTanH : kTanV;
        float n[3];
        for (int i = 0; i < 3; ++i) {
            const float axis = horizontal ? f.right[i] : kUp[i];
            n[i] = sign * axis - tan * f.fwd[i];
        }
        unit3(n);
        v.planes[p][0] = n[0];
        v.planes[p][1] = n[1];
        v.planes[p][2] = n[2];
        v.planes[p][3] = -dot3(n, f.cam);
    }
    return v;
}

void pointAt(const Frame& f, float lateral, float vertical, float out[3]) {
    for (int i = 0; i < 3; ++i)
        out[i] = f.cam[i] + f.fwd[i] * kDepth + f.right[i] * lateral + kUp[i] * vertical;
}

// m is the horizontal ratio x/|z| in units of the 4:3 half-tangent kTanH.
int sphereAt(const Volume& v, const Frame& f, float m, float vertical, float radius, float k) {
    float p[3];
    pointAt(f, m * kTanH * kDepth, vertical, p);
    return tooie_ws_frustum::sphere_accepts(v, p, radius, k);
}

int aabbAt(const Volume& v, const Frame& f, float m, float vertical, float half, float k) {
    float c[3];
    pointAt(f, m * kTanH * kDepth, vertical, c);
    float mn[3], mx[3];
    for (int i = 0; i < 3; ++i) {
        mn[i] = c[i] - half;
        mx[i] = c[i] + half;
    }
    return tooie_ws_frustum::aabb_accepts(v, mn, mx, k);
}

void testKFor() {
    using tooie_ws_frustum::k_for;
    CHECK(approx(k_for(4.0f / 3.0f, 16.0f / 9.0f), 4.0f / 3.0f, 1e-6f));
    CHECK(approx(k_for(4.0f / 3.0f, 4.0f / 3.0f), 1.0f, 1e-6f));
    CHECK(approx(k_for(4.0f / 3.0f, 1.0f), 1.0f, 1e-6f));
    CHECK(approx(k_for(std::nanf(""), 16.0f / 9.0f), 4.0f / 3.0f, 1e-6f));
    CHECK(approx(k_for(0.0f, 16.0f / 9.0f), 4.0f / 3.0f, 1e-6f));
    CHECK(approx(k_for(-1.0f, 16.0f / 9.0f), 4.0f / 3.0f, 1e-6f));
    CHECK(approx(k_for(4.0f / 3.0f, std::nanf("")), 1.0f, 1e-6f));
    CHECK(approx(k_for(4.0f / 3.0f, HUGE_VALF), 1.0f, 1e-6f));
}

void testNormalize() {
    float v[3] = { 3.0f, 0.0f, 4.0f };
    CHECK(tooie_ws_frustum::normalize(v));
    CHECK(approx(v[0], 0.6f, 1e-6f) && approx(v[1], 0.0f, 1e-6f) && approx(v[2], 0.8f, 1e-6f));

    float z[3] = { 0.0f, 0.0f, 0.0f };
    CHECK(!tooie_ws_frustum::normalize(z));
    CHECK(z[0] == 0.0f && z[1] == 0.0f && z[2] == 0.0f);

    float nan[3] = { std::nanf(""), 0.0f, 1.0f };
    CHECK(!tooie_ws_frustum::normalize(nan));
    CHECK(std::isnan(nan[0]) && nan[1] == 0.0f && nan[2] == 1.0f);

    float tiny[3] = { 0.0f, 1e-7f, 0.0f };
    CHECK(!tooie_ws_frustum::normalize(tiny));

    float inf[3] = { HUGE_VALF, 0.0f, 0.0f };
    CHECK(!tooie_ws_frustum::normalize(inf));
}

void testExpandHPlaneNoOp() {
    const Volume v = makeVolume(kOrigin);
    float nx = v.planes[0][0], ny = v.planes[0][1], nz = v.planes[0][2], d = v.planes[0][3];
    tooie_ws_frustum::expand_h_plane(nx, ny, nz, d, kOrigin.cam, kOrigin.fwd, 1.0f);
    CHECK(approx(nx, v.planes[0][0], 1e-6f) && approx(ny, v.planes[0][1], 1e-6f) &&
          approx(nz, v.planes[0][2], 1e-6f) && approx(d, v.planes[0][3], 1e-6f));
}

void testExpandHPlaneWidens() {
    const Volume w = makeVolume(kMoved);
    const float* p = w.planes[0];
    float nx = p[0], ny = p[1], nz = p[2], d = p[3];
    const float n0[3] = { p[0], p[1], p[2] };
    const Frame& f = kMoved;

    tooie_ws_frustum::expand_h_plane(nx, ny, nz, d, f.cam, f.fwd, kKws);
    const float n1[3] = { nx, ny, nz };

    CHECK(approx(dot3(n1, f.fwd), dot3(n0, f.fwd), 1e-5f));
    CHECK(approx(dot3(n1, f.right), dot3(n0, f.right) / kKws, 1e-5f));
    CHECK(approx(d, -dot3(f.cam, n1), 1e-4f));
    CHECK(approx(dot3(n1, f.cam) + d, 0.0f, 1e-4f));
}

void testExpandHPlaneBoundary() {
    const Volume v = makeVolume(kOrigin);
    float nx = v.planes[0][0], ny = v.planes[0][1], nz = v.planes[0][2], d = v.planes[0][3];
    tooie_ws_frustum::expand_h_plane(nx, ny, nz, d, kOrigin.cam, kOrigin.fwd, kKws);
    const float edge[3] = { kKws * kTanH * kDepth, 0.0f, -kDepth };
    const float n1[3] = { nx, ny, nz };
    CHECK(approx(dot3(n1, edge) + d, 0.0f, 1e-4f));
}

void testSphereOrigin() {
    const Volume v = makeVolume(kOrigin);
    const Frame& f = kOrigin;

    CHECK(sphereAt(v, f, 0.5f, 0.0f, 0.1f, 1.0f) == 1);
    CHECK(sphereAt(v, f, 0.5f, 0.0f, 0.1f, kKws) == 1);
    CHECK(sphereAt(v, f, 1.2f, 0.0f, 0.1f, 1.0f) == 0);
    CHECK(sphereAt(v, f, 1.2f, 0.0f, 0.1f, kKws) == 1);
    CHECK(sphereAt(v, f, 1.5f * kKws, 0.0f, 0.1f, 1.0f) == 0);
    CHECK(sphereAt(v, f, 1.5f * kKws, 0.0f, 0.1f, kKws) == 0);

    CHECK(sphereAt(v, f, -1.2f, 0.0f, 0.1f, 1.0f) == 0);
    CHECK(sphereAt(v, f, -1.2f, 0.0f, 0.1f, kKws) == 1);
    CHECK(sphereAt(v, f, -0.5f, 0.0f, 0.1f, kKws) == 1);

    CHECK(sphereAt(v, f, 0.0f, 5.0f, 0.1f, 1.0f) == 0);
    CHECK(sphereAt(v, f, 0.0f, 5.0f, 0.1f, kKws) == 0);

    CHECK(sphereAt(v, f, 1.02f, 0.0f, 0.2f, 1.0f) == 1);
    CHECK(sphereAt(v, f, 1.02f, 0.0f, 0.01f, 1.0f) == 0);

    CHECK(sphereAt(v, f, 0.5f, 0.0f, -1.0f, 1.0f) == -1);
    CHECK(sphereAt(v, f, 0.5f, 0.0f, std::nanf(""), kKws) == -1);

    Volume scaled = v;
    scaled.fwd[2] = -3.0f;
    CHECK(sphereAt(scaled, f, 1.2f, 0.0f, 0.1f, kKws) == 1);
    CHECK(sphereAt(scaled, f, 1.2f, 0.0f, 0.1f, 1.0f) == 0);

    Volume degenerate = v;
    degenerate.fwd[0] = degenerate.fwd[1] = degenerate.fwd[2] = 0.0f;
    CHECK(sphereAt(degenerate, f, 0.5f, 0.0f, 0.1f, kKws) == -1);
}

void testSphereMoved() {
    const Volume v = makeVolume(kMoved);
    const Frame& f = kMoved;

    CHECK(sphereAt(v, f, 0.5f, 0.0f, 0.1f, 1.0f) == 1);
    CHECK(sphereAt(v, f, 1.2f, 0.0f, 0.1f, 1.0f) == 0);
    CHECK(sphereAt(v, f, 1.2f, 0.0f, 0.1f, kKws) == 1);
    CHECK(sphereAt(v, f, -1.5f * kKws, 0.0f, 0.1f, kKws) == 0);
    CHECK(sphereAt(v, f, 0.0f, 5.0f, 0.1f, kKws) == 0);
}

void testAabbOrigin() {
    const Volume v = makeVolume(kOrigin);
    const Frame& f = kOrigin;

    CHECK(aabbAt(v, f, 0.0f, 0.0f, 0.1f, 1.0f) == 1);
    CHECK(aabbAt(v, f, 0.5f, 0.0f, 0.1f, 1.0f) == 1);
    CHECK(aabbAt(v, f, 0.5f, 0.0f, 0.1f, kKws) == 1);
    CHECK(aabbAt(v, f, 1.2f, 0.0f, 0.1f, 1.0f) == 0);
    CHECK(aabbAt(v, f, 1.2f, 0.0f, 0.1f, kKws) == 1);
    CHECK(aabbAt(v, f, 1.5f * kKws, 0.0f, 0.1f, kKws) == 0);
    CHECK(aabbAt(v, f, 1.5f * kKws, 0.0f, 0.1f, 1.0f) == 0);
    CHECK(aabbAt(v, f, -1.2f, 0.0f, 0.1f, 1.0f) == 0);
    CHECK(aabbAt(v, f, -1.2f, 0.0f, 0.1f, kKws) == 1);
    CHECK(aabbAt(v, f, 0.0f, 5.0f, 0.1f, kKws) == 0);

    float mn[3] = { 0.9f * kTanH * kDepth, -0.1f, -kDepth - 0.1f };
    float mx[3] = { 1.1f * kTanH * kDepth, 0.1f, -kDepth + 0.1f };
    CHECK(tooie_ws_frustum::aabb_accepts(v, mn, mx, 1.0f) == 1);
    CHECK(tooie_ws_frustum::aabb_accepts(v, mn, mx, kKws) == 1);

    const Volume degenerate = [&] {
        Volume d = v;
        d.fwd[0] = d.fwd[1] = d.fwd[2] = 0.0f;
        return d;
    }();
    CHECK(aabbAt(degenerate, f, 0.5f, 0.0f, 0.1f, kKws) == -1);
}

void testAabbMoved() {
    const Volume v = makeVolume(kMoved);
    const Frame& f = kMoved;

    CHECK(aabbAt(v, f, 0.5f, 0.0f, 0.1f, 1.0f) == 1);
    CHECK(aabbAt(v, f, 1.2f, 0.0f, 0.1f, 1.0f) == 0);
    CHECK(aabbAt(v, f, 1.2f, 0.0f, 0.1f, kKws) == 1);
    CHECK(aabbAt(v, f, 0.0f, 5.0f, 0.1f, kKws) == 0);
}

void testClassify() {
    using tooie_ws_frustum::classify;
    CHECK(classify(1u, 0) == Census::Accept);
    CHECK(classify(7u, 1) == Census::Accept);
    CHECK(classify(0u, 1) == Census::WidescreenOnly);
    CHECK(classify(0u, 0) == Census::RejectBoth);
    CHECK(classify(0u, -1) == Census::Invalid);
    CHECK(classify(0u, 2) == Census::Invalid);
}

} // namespace

int main() {
    testKFor();
    testNormalize();
    testExpandHPlaneNoOp();
    testExpandHPlaneWidens();
    testExpandHPlaneBoundary();
    testSphereOrigin();
    testSphereMoved();
    testAabbOrigin();
    testAabbMoved();
    testClassify();
    std::printf("%d/%d passed p08_ws_frustum_test\n", g_pass_count, g_test_count);
    return g_pass_count == g_test_count ? 0 : 1;
}
