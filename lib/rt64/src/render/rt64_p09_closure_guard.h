//
// RT64
//
// P09 articulated-closure guard for AUTO WORLD interpolation.
//
// A game that composes skeletal bone matrices on the CPU hands RT64 one
// composed WORLD matrix per bone. RigidBody::lerp interpolates each of them
// independently: translation along a chord, rotation along an arc, each about
// its own origin. Every triangle that spans two WORLD slots (a joint) then
// connects two vertices that are interpolated by unrelated rigid motions.
// When the bones rotate by a large angle between the two frames the vertices
// separate and the triangle becomes a long sliver (a "rod") that exists in
// neither endpoint pose.
//
// Component-wise (elementwise) matrix lerp has the opposite property: vertex
// positions are convex combinations of their endpoint positions, so no edge
// can ever be longer than the longer of its endpoint edges. It is not rigid
// (it shrinks a bone slightly while it rotates) but it can never tear a joint.
//
// The guard keeps the rigid interpolation wherever it is closed and switches a
// transform to elementwise lerp only when rigid interpolation would make an
// edge of a triangle that spans several WORLD slots longer than both of its
// endpoint lengths. A switch propagates to the other end of the offending edge
// until no such edge remains. Transforms for which the two interpolations agree
// are never switched. No game, model, bone or display-list knowledge is used.
//
// Default ON. TOOIE_TEST_P09_CLOSURE_GUARD=0 restores the unguarded rigid
// interpolation for paired A/B tests.
//

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include "common/rt64_common.h"

namespace RT64 {
    namespace p09ClosureGuard {
        inline bool enabled() {
            static int cached = -1;
            if (cached < 0) {
                const char *env = std::getenv("TOOIE_TEST_P09_CLOSURE_GUARD");
                cached = (env != nullptr && env[0] == '0' && env[1] == '\0') ? 0 : 1;
            }
            return cached != 0;
        }

        struct Vec3 {
            float x = 0.0f, y = 0.0f, z = 0.0f;
        };

        inline Vec3 transformPoint(const hlslpp::float4x4 &m, float x, float y, float z) {
            const hlslpp::float4 r = hlslpp::mul(hlslpp::float4(x, y, z, 1.0f), m);
            const float w = (std::fabs(float(r.w)) > 1e-12f) ? float(r.w) : 1.0f;
            return { float(r.x) / w, float(r.y) / w, float(r.z) / w };
        }

        inline float distance(const Vec3 &a, const Vec3 &b) {
            const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
            return std::sqrt(dx * dx + dy * dy + dz * dz);
        }

        constexpr float CandidateDeviation = 1.0f; // rigid vs elementwise probe-point gap that makes a transform a candidate
        constexpr float EdgeRatio = 1.25f;         // interpolated edge vs longer endpoint edge
        constexpr float EdgeSlack = 1.0f;          // ... plus this absolute slack (world units)
        constexpr int MaxIterations = 8;

        // mapped[t]: transform t interpolates (paired with a previous transform); other transforms are CURRENT and must
        // carry prev == cur == rigid == elementwise (they stay fixed and can only be neighbours of a switched transform).
        // prev/cur: endpoint matrices. rigid/elementwise: both interpolations at the same weight.
        // useElementwise[t] is set for each transform that must use the elementwise matrix.
        // Returns the number of transforms switched.
        inline uint32_t compute(size_t worldCount, const uint32_t *faceIndices, size_t faceIndexCount,
            const uint16_t *worldIndices, uint32_t vertexCount, const float *posFloats, const std::vector<uint8_t> &mapped,
            const std::vector<hlslpp::float4x4> &prev, const std::vector<hlslpp::float4x4> &cur,
            const std::vector<hlslpp::float4x4> &rigid, const std::vector<hlslpp::float4x4> &elementwise,
            std::vector<uint8_t> &useElementwise)
        {
            useElementwise.assign(worldCount, 0);
            if (faceIndices == nullptr || worldIndices == nullptr || posFloats == nullptr) {
                return 0;
            }

            // Candidates: transforms where rigid and elementwise interpolation visibly differ.
            constexpr float Lever = 64.0f;
            const float probes[4][3] = { { 0, 0, 0 }, { Lever, 0, 0 }, { 0, Lever, 0 }, { 0, 0, Lever } };
            std::vector<uint8_t> candidate(worldCount, 0);
            bool anyCandidate = false;
            for (size_t t = 0; t < worldCount; t++) {
                if (t >= mapped.size() || mapped[t] == 0) continue;
                for (const auto &q : probes) {
                    if (distance(transformPoint(rigid[t], q[0], q[1], q[2]), transformPoint(elementwise[t], q[0], q[1], q[2])) > CandidateDeviation) {
                        candidate[t] = 1;
                        anyCandidate = true;
                        break;
                    }
                }
            }
            if (!anyCandidate) {
                return 0;
            }

            // Triangles that span several WORLD slots, at least one of them a candidate.
            struct Edge {
                uint32_t a, b;          // WORLD slots of the two end vertices
                float refLength;        // longer endpoint (prev/cur) length
                Vec3 rigidA, rigidB, elemA, elemB;
            };
            std::vector<Edge> edges;
            for (size_t f = 0; f + 2 < faceIndexCount; f += 3) {
                const uint32_t v[3] = { faceIndices[f], faceIndices[f + 1], faceIndices[f + 2] };
                if (v[0] >= vertexCount || v[1] >= vertexCount || v[2] >= vertexCount) continue;
                const uint32_t w[3] = { worldIndices[v[0]], worldIndices[v[1]], worldIndices[v[2]] };
                if (w[0] >= worldCount || w[1] >= worldCount || w[2] >= worldCount) continue;
                if (w[0] == w[1] && w[1] == w[2]) continue;
                if (!(candidate[w[0]] || candidate[w[1]] || candidate[w[2]])) continue;
                Vec3 pPrev[3], pCur[3], pRigid[3], pElem[3];
                for (int i = 0; i < 3; i++) {
                    const float x = posFloats[size_t(v[i]) * 3], y = posFloats[size_t(v[i]) * 3 + 1], z = posFloats[size_t(v[i]) * 3 + 2];
                    pPrev[i] = transformPoint(prev[w[i]], x, y, z);
                    pCur[i] = transformPoint(cur[w[i]], x, y, z);
                    pRigid[i] = transformPoint(rigid[w[i]], x, y, z);
                    pElem[i] = transformPoint(elementwise[w[i]], x, y, z);
                }
                for (int e = 0; e < 3; e++) {
                    const int a = e, b = (e + 1) % 3;
                    if (w[a] == w[b]) continue;
                    edges.push_back({ w[a], w[b], std::max(distance(pPrev[a], pPrev[b]), distance(pCur[a], pCur[b])),
                        pRigid[a], pRigid[b], pElem[a], pElem[b] });
                }
            }

            uint32_t switched = 0;
            for (int iteration = 0; iteration < MaxIterations; iteration++) {
                bool changed = false;
                for (const Edge &e : edges) {
                    if (useElementwise[e.a] && useElementwise[e.b]) continue;
                    const Vec3 &pa = useElementwise[e.a] ? e.elemA : e.rigidA;
                    const Vec3 &pb = useElementwise[e.b] ? e.elemB : e.rigidB;
                    const float length = distance(pa, pb);
                    const float ref = std::max(e.refLength, 1e-3f);
                    if (length > EdgeRatio * ref && (length - ref) > EdgeSlack) {
                        for (const uint32_t t : { e.a, e.b }) {
                            if (candidate[t] && !useElementwise[t]) {
                                useElementwise[t] = 1;
                                switched++;
                                changed = true;
                            }
                        }
                    }
                }
                if (!changed) break;
            }
            return switched;
        }
    }
}
