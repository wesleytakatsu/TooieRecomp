// Host test for the P09 articulated WORLD interpolation guards in RT64:
//   render/rt64_p09_translation_coherence.h
//   render/rt64_p09_closure_guard.h
// The fixture is real game data captured by TOOIE_DIAG_P09_WORLD_LERP_AUDIT in the title scene
// (workload 1594, WORLD slots 13/14, a joint triangle whose rigid interpolation stretched into a rod).

#include "common/rt64_math.h"
#include "hle/rt64_rigid_body.h"
#include "render/rt64_p09_closure_guard.h"
#include "render/rt64_p09_translation_coherence.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

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

hlslpp::float4x4 matrix16(const float (&v)[16]) {
    return hlslpp::float4x4(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7],
        v[8], v[9], v[10], v[11], v[12], v[13], v[14], v[15]);
}

// Reproduces what RT64 holds for a matched, ungrouped AUTO WORLD transform.
RT64::RigidBody makeBody(const hlslpp::float4x4 &prev, const hlslpp::float4x4 &cur, bool lerpTranslation) {
    RT64::RigidBody body;
    body.updateDecomposition(prev, true);
    body.updateDecomposition(cur, true);
    body.lerpTranslation = lerpTranslation;
    body.lerpRotation = true;
    body.lerpScale = true;
    body.lerpSkew = true;
    body.lerpPerspective = false;
    return body;
}

float maxAbsDiff(const hlslpp::float4x4 &a, const hlslpp::float4x4 &b) {
    float d = 0.0f;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) d = std::fmax(d, std::fabs(float(a[i][j]) - float(b[i][j])));
    }
    return d;
}

using RT64::p09ClosureGuard::Vec3;
using RT64::p09ClosureGuard::distance;
using RT64::p09ClosureGuard::transformPoint;

// Real captured joint: WORLD 13 (rotates ~88 degrees in one game frame, translation held by the AUTO gate)
// and WORLD 14 (small motion). Three vertices of one triangle: v0 in 13, v1/v2 in 14.
struct JointFixture {
    hlslpp::float4x4 prev[2], cur[2], capturedLerp[2];
    float local[3][3] = { { -2, 131, -33 }, { -1, 131, -29 }, { 1, 128, -29 } };
    uint16_t world[3] = { 0, 1, 1 };

    JointFixture() {
        static const float p13[16] = { 0.999649f, -0.0262756f, 0, 0, 0.0262604f, 0.999435f, -0.0208282f, 0, 0.000549316f, 0.020813f, 0.999786f, 0, -2.10638f, -31.1524f, -22.5307f, 1 };
        static const float c13[16] = { 0.999893f, 0.0150604f, 0, 0, -0.000289917f, 0.0197449f, 0.999786f, 0, 0.0150452f, -0.99968f, 0.0197449f, 0, -0.21022f, 62.8767f, -190.232f, 1 };
        static const float l13[16] = { 0.999958f, -0.00361128f, -0.00850006f, 0, 0.00871932f, 0.672226f, 0.74028f, 0, 0.00303567f, -0.740331f, 0.672229f, 0, -0.21022f, 62.8767f, -190.232f, 1 };
        static const float p14[16] = { 0.999649f, -0.0262756f, 0, 0, 0.0236053f, 0.898132f, -0.439072f, 0, 0.0115356f, 0.438919f, 0.898453f, 0, -1.43192f, -5.4882f, 28.4415f, 1 };
        static const float c14[16] = { 0.999893f, 0.0150604f, 0, 0, -0.0144043f, 0.957092f, -0.289398f, 0, -0.00436401f, 0.289368f, 0.957214f, 0, 1.04004f, -20.168f, 6.00218f, 1 };
        static const float l14[16] = { 0.999985f, -0.00559105f, -0.00083228f, 0, 0.00490518f, 0.930847f, -0.365353f, 0, 0.00281403f, 0.365344f, 0.930872f, 0, 1.04004f, -20.168f, 6.00218f, 1 };
        prev[0] = matrix16(p13); cur[0] = matrix16(c13); capturedLerp[0] = matrix16(l13);
        prev[1] = matrix16(p14); cur[1] = matrix16(c14); capturedLerp[1] = matrix16(l14);
    }

    // Longest edge ratio (edge under `lerp` vs the longer endpoint edge) over the triangle.
    float worstRatio(const hlslpp::float4x4 lerp[2]) const {
        float worst = 0.0f;
        for (int e = 0; e < 3; e++) {
            const int a = e, b = (e + 1) % 3;
            if (world[a] == world[b]) continue;
            const Vec3 pa = transformPoint(prev[world[a]], local[a][0], local[a][1], local[a][2]);
            const Vec3 pb = transformPoint(prev[world[b]], local[b][0], local[b][1], local[b][2]);
            const Vec3 ca = transformPoint(cur[world[a]], local[a][0], local[a][1], local[a][2]);
            const Vec3 cb = transformPoint(cur[world[b]], local[b][0], local[b][1], local[b][2]);
            const Vec3 la = transformPoint(lerp[world[a]], local[a][0], local[a][1], local[a][2]);
            const Vec3 lb = transformPoint(lerp[world[b]], local[b][0], local[b][1], local[b][2]);
            const float ref = std::fmax(std::fmax(distance(pa, pb), distance(ca, cb)), 1e-3f);
            worst = std::fmax(worst, distance(la, lb) / ref);
        }
        return worst;
    }
};

void testFixtureMatchesProductionRigidLerp() {
    JointFixture fx;
    for (int i = 0; i < 2; i++) {
        const RT64::RigidBody body = makeBody(fx.prev[i], fx.cur[i], false);
        const hlslpp::float4x4 lerp = body.lerp(0.5f, fx.prev[i], fx.cur[i], true);
        TEST_ASSERT(maxAbsDiff(lerp, fx.capturedLerp[i]) < 2e-3f, "RigidBody::lerp reproduces the captured in-between matrix");
    }
}

void testOriginalInterpolationTearsTheJoint() {
    JointFixture fx;
    hlslpp::float4x4 rigid[2];
    for (int i = 0; i < 2; i++) rigid[i] = makeBody(fx.prev[i], fx.cur[i], false).lerp(0.5f, fx.prev[i], fx.cur[i], true);
    TEST_ASSERT(fx.worstRatio(rigid) > 10.0f, "unguarded rigid interpolation stretches the joint edge by >10x (the observed rod)");
}

void testClosureGuardClosesTheJoint() {
    JointFixture fx;
    constexpr size_t N = 2;
    std::vector<hlslpp::float4x4> rigid(N), elem(N), prev(N), cur(N);
    for (size_t i = 0; i < N; i++) {
        prev[i] = fx.prev[i];
        cur[i] = fx.cur[i];
        rigid[i] = makeBody(prev[i], cur[i], false).lerp(0.5f, prev[i], cur[i], true);
        elem[i] = RT64::lerpMatrix(prev[i], cur[i], 0.5f);
    }
    const uint32_t faces[3] = { 0, 1, 2 };
    const float pos[9] = { -2, 131, -33, -1, 131, -29, 1, 128, -29 };
    std::vector<uint8_t> mapped(N, 1), useElem;
    const uint32_t switched = RT64::p09ClosureGuard::compute(N, faces, 3, fx.world, 3, pos, mapped, prev, cur, rigid, elem, useElem);
    TEST_ASSERT(switched >= 1, "guard switches at least one transform");
    TEST_ASSERT(useElem[0] != 0, "the bone that rotates ~88 degrees is switched");
    hlslpp::float4x4 result[2];
    for (size_t i = 0; i < N; i++) result[i] = useElem[i] ? elem[i] : rigid[i];
    TEST_ASSERT(fx.worstRatio(result) <= RT64::p09ClosureGuard::EdgeRatio + 1e-3f, "guarded edge is no longer than 1.25x the longer endpoint edge");
    TEST_ASSERT(fx.worstRatio(elem.data()) <= 1.0f + 1e-3f, "elementwise lerp is convex (reference property)");
}

void testClosureGuardKeepsClosedRigidInterpolation() {
    // Two bones with a small rotation that share a joint: rigid and elementwise agree, nothing is switched.
    const float rot = 0.05f;
    hlslpp::float4x4 prev0 = hlslpp::float4x4::identity(), cur0 = hlslpp::float4x4::identity();
    cur0[0][0] = std::cos(rot); cur0[0][1] = std::sin(rot); cur0[1][0] = -std::sin(rot); cur0[1][1] = std::cos(rot);
    hlslpp::float4x4 prev1 = prev0, cur1 = cur0;
    prev1[3][0] = 5.0f; cur1[3][0] = 6.0f;
    std::vector<hlslpp::float4x4> prev = { prev0, prev1 }, cur = { cur0, cur1 }, rigid(2), elem(2);
    for (size_t i = 0; i < 2; i++) {
        rigid[i] = makeBody(prev[i], cur[i], true).lerp(0.5f, prev[i], cur[i], true);
        elem[i] = RT64::lerpMatrix(prev[i], cur[i], 0.5f);
    }
    const uint32_t faces[3] = { 0, 1, 2 };
    const uint16_t world[3] = { 0, 1, 1 };
    const float pos[9] = { 1, 1, 0, 2, 1, 0, 1, 2, 0 };
    std::vector<uint8_t> mapped(2, 1), useElem;
    const uint32_t switched = RT64::p09ClosureGuard::compute(2, faces, 3, world, 3, pos, mapped, prev, cur, rigid, elem, useElem);
    TEST_ASSERT(switched == 0, "closed rigid interpolation is kept");
    TEST_ASSERT(useElem[0] == 0 && useElem[1] == 0, "no transform is switched when rigid and elementwise agree");
}

void testClosureGuardUnmappedNeighbourStaysFixed() {
    // WORLD 1 is unmapped (CURRENT); WORLD 0 rotates ~88 degrees. The edge to the fixed neighbour must not overshoot.
    JointFixture fx;
    std::vector<hlslpp::float4x4> prev = { fx.prev[0], fx.cur[1] }, cur = { fx.cur[0], fx.cur[1] }, rigid(2), elem(2);
    rigid[0] = makeBody(prev[0], cur[0], false).lerp(0.5f, prev[0], cur[0], true);
    elem[0] = RT64::lerpMatrix(prev[0], cur[0], 0.5f);
    rigid[1] = elem[1] = cur[1];
    const uint32_t faces[3] = { 0, 1, 2 };
    const float pos[9] = { -2, 131, -33, -1, 131, -29, 1, 128, -29 };
    std::vector<uint8_t> mapped = { 1, 0 }, useElem;
    RT64::p09ClosureGuard::compute(2, faces, 3, fx.world, 3, pos, mapped, prev, cur, rigid, elem, useElem);
    TEST_ASSERT(useElem[0] != 0, "mapped bone next to a CURRENT neighbour is switched when rigid would tear the edge");
    TEST_ASSERT(useElem[1] == 0, "an unmapped transform is never switched");
}

void testTranslationCoherence() {
    // WORLD slots 0..5. Face list builds sets {1,2,3} (joint triangles), {4,5}; slot 0 is isolated.
    const uint16_t worldIndices[9] = { 1, 2, 2, 2, 3, 3, 4, 5, 5 };
    const uint32_t faces[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    std::vector<uint8_t> mapped = { 1, 1, 1, 1, 1, 1 };
    std::vector<uint8_t> lerpT = { 0, 0, 0, 1, 0, 0 };
    std::vector<uint8_t> force, articulated;
    uint32_t n = RT64::p09TranslationCoherence::compute(6, faces, 9, worldIndices, 9, mapped, lerpT, force, &articulated);
    TEST_ASSERT(articulated[1] && articulated[2] && articulated[3] && articulated[4] && articulated[5], "joint-connected slots are articulated");
    TEST_ASSERT(articulated[0] == 0, "a slot no triangle connects to another is not articulated (held-at-CURRENT does not apply)");
    TEST_ASSERT(n == 2, "two held transforms of the interpolating set are forced");
    TEST_ASSERT(force[1] == 1 && force[2] == 1, "held members of a set with an interpolating member are forced");
    TEST_ASSERT(force[0] == 0, "an isolated held transform keeps the original decision");
    TEST_ASSERT(force[4] == 0 && force[5] == 0, "a set whose mapped transforms are all held (teleport) keeps the original decision");
    mapped[3] = 0;
    n = RT64::p09TranslationCoherence::compute(6, faces, 9, worldIndices, 9, mapped, lerpT, force);
    TEST_ASSERT(n == 0, "an unmapped transform does not make a set interpolating");
}

void testTranslationCoherenceClosesRealJoint() {
    // Bone 14 interpolates translation; bone 13 is held by the AUTO gate. Forcing the held bone to interpolate translation
    // shrinks the joint gap (the captured pair is held on both ends, so use bone 14 as the interpolating neighbour).
    JointFixture fx;
    const hlslpp::float4x4 heldRigid = makeBody(fx.prev[0], fx.cur[0], false).lerp(0.5f, fx.prev[0], fx.cur[0], true);
    const hlslpp::float4x4 forcedRigid = makeBody(fx.prev[0], fx.cur[0], true).lerp(0.5f, fx.prev[0], fx.cur[0], true);
    const hlslpp::float4x4 neighbour = makeBody(fx.prev[1], fx.cur[1], true).lerp(0.5f, fx.prev[1], fx.cur[1], true);
    hlslpp::float4x4 heldSet[2] = { heldRigid, neighbour };
    hlslpp::float4x4 forcedSet[2] = { forcedRigid, neighbour };
    TEST_ASSERT(fx.worstRatio(forcedSet) < fx.worstRatio(heldSet), "interpolating the held translation shortens the joint edge");
    const Vec3 held = transformPoint(heldRigid, 0, 0, 0), forced = transformPoint(forcedRigid, 0, 0, 0);
    const Vec3 chordMid = { (transformPoint(fx.prev[0], 0, 0, 0).x + transformPoint(fx.cur[0], 0, 0, 0).x) * 0.5f,
        (transformPoint(fx.prev[0], 0, 0, 0).y + transformPoint(fx.cur[0], 0, 0, 0).y) * 0.5f,
        (transformPoint(fx.prev[0], 0, 0, 0).z + transformPoint(fx.cur[0], 0, 0, 0).z) * 0.5f };
    TEST_ASSERT(distance(forced, chordMid) < 1e-2f, "forced translation interpolation places the bone origin on the endpoint chord");
    TEST_ASSERT(distance(held, chordMid) > 50.0f, "the original gate leaves the bone origin at CURRENT, far from the chord midpoint");
}

void testHeldTransformIsShownAtCurrent() {
    // The original gate leaves WORLD 13 with CURRENT translation but a half-way rotation: its far vertex leaves the
    // endpoint chord by tens of units. A transform that stays held after the coherence pass is shown wholly at CURRENT.
    JointFixture fx;
    const float lx = fx.local[0][0], ly = fx.local[0][1], lz = fx.local[0][2];
    const hlslpp::float4x4 hybrid = makeBody(fx.prev[0], fx.cur[0], false).lerp(0.5f, fx.prev[0], fx.cur[0], true);
    const Vec3 p0 = transformPoint(fx.prev[0], lx, ly, lz), p1 = transformPoint(fx.cur[0], lx, ly, lz);
    auto offChord = [&](const Vec3 &v) {
        const float ax = p1.x - p0.x, ay = p1.y - p0.y, az = p1.z - p0.z;
        const float l2 = ax * ax + ay * ay + az * az;
        float t = ((v.x - p0.x) * ax + (v.y - p0.y) * ay + (v.z - p0.z) * az) / (l2 > 1e-9f ? l2 : 1.0f);
        t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
        return distance(v, { p0.x + ax * t, p0.y + ay * t, p0.z + az * t });
    };
    TEST_ASSERT(offChord(transformPoint(hybrid, lx, ly, lz)) > 20.0f, "hybrid (held translation, interpolated rotation) leaves the chord by >20 units");
    const hlslpp::float4x4 &held = RT64::p09TranslationCoherence::heldMatrix(0.5f, fx.prev[0], fx.cur[0]);
    TEST_ASSERT(offChord(transformPoint(held, lx, ly, lz)) < 1e-3f, "held-at-CURRENT vertex stays on the endpoint chord");
    TEST_ASSERT(distance(transformPoint(held, lx, ly, lz), p1) < 1e-3f, "held transform is exactly CURRENT at weight 0.5");
    const hlslpp::float4x4 &heldAtZero = RT64::p09TranslationCoherence::heldMatrix(0.0f, fx.prev[0], fx.cur[0]);
    TEST_ASSERT(distance(transformPoint(heldAtZero, lx, ly, lz), p0) < 1e-3f, "held transform is the previous pose at weight 0");
}

// Values of G_EX_COMPONENT_SKIP / INTERPOLATE / AUTO in rt64_extended_gbi.h.
constexpr uint8_t G_EX_COMPONENT_SKIP = 0x0, G_EX_COMPONENT_INTERPOLATE = 0x1, G_EX_COMPONENT_AUTO = 0x2;

hlslpp::float4x4 translationMatrix(float x, float y, float z) {
    return hlslpp::float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1);
}

// The AUTO gate holds the translation for any turn of 90 degrees or more, however slow; the speed-only evaluation
// (lerpTranslationSpeedOnly) holds only on a speed spike. The production lerpTranslation must stay the original decision.
void testGateSpeedOnly() {
    RT64::RigidBody rb;
    rb.updateLinear(translationMatrix(0, 0, 0), translationMatrix(3, 0, 0), G_EX_COMPONENT_AUTO);
    TEST_ASSERT(!rb.lerpTranslation && rb.lerpTranslationSpeedOnly, "slow move from rest: the original gate holds (zero previous direction), speed-only interpolates");
    rb.updateLinear(translationMatrix(3, 0, 0), translationMatrix(0, 0, 0), G_EX_COMPONENT_AUTO);
    TEST_ASSERT(!rb.lerpTranslation, "original gate holds a slow 180 degree reversal (production decision unchanged)");
    TEST_ASSERT(rb.lerpTranslationSpeedOnly, "speed-only gate interpolates the slow reversal");
    rb.updateLinear(translationMatrix(0, 0, 0), translationMatrix(100, 0, 0), G_EX_COMPONENT_AUTO);
    TEST_ASSERT(!rb.lerpTranslation && !rb.lerpTranslationSpeedOnly, "a speed spike (teleport) is held by both evaluations");
    RT64::RigidBody steady;
    steady.updateLinear(translationMatrix(0, 0, 0), translationMatrix(10, 0, 0), G_EX_COMPONENT_AUTO);
    steady.updateLinear(translationMatrix(10, 0, 0), translationMatrix(22, 0, 0), G_EX_COMPONENT_AUTO);
    TEST_ASSERT(steady.lerpTranslation && steady.lerpTranslationSpeedOnly, "steady fast motion interpolates under both evaluations");
    RT64::RigidBody forced;
    forced.updateLinear(translationMatrix(0, 0, 0), translationMatrix(500, 0, 0), G_EX_COMPONENT_INTERPOLATE);
    TEST_ASSERT(forced.lerpTranslation && forced.lerpTranslationSpeedOnly, "explicit INTERPOLATE keeps both true");
    RT64::RigidBody skipped;
    skipped.updateLinear(translationMatrix(0, 0, 0), translationMatrix(1, 0, 0), G_EX_COMPONENT_SKIP);
    TEST_ASSERT(!skipped.lerpTranslation && !skipped.lerpTranslationSpeedOnly, "explicit SKIP keeps both false");
}

void testDefaultsAreOn() {
    TEST_ASSERT(RT64::p09TranslationCoherence::gateSpeedOnlyEnabled(), "gate speed-only release is ON by default");
    TEST_ASSERT(RT64::p09TranslationCoherence::enabled(), "translation coherence is ON by default");
    TEST_ASSERT(RT64::p09ClosureGuard::enabled(), "closure guard is ON by default");
}

} // namespace

int main() {
    testFixtureMatchesProductionRigidLerp();
    testOriginalInterpolationTearsTheJoint();
    testClosureGuardClosesTheJoint();
    testClosureGuardKeepsClosedRigidInterpolation();
    testClosureGuardUnmappedNeighbourStaysFixed();
    testTranslationCoherence();
    testTranslationCoherenceClosesRealJoint();
    testHeldTransformIsShownAtCurrent();
    testGateSpeedOnly();
    testDefaultsAreOn();
    std::printf("PASS p09_world_interpolation_guard_test %d/%d\n", g_pass_count, g_test_count);
    return 0;
}
