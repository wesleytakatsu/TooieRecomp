//
// RT64
//
// P09 spike guard, the last post-pass over the WORLD matrices TransformProcessor::process just interpolated.
//
// Whatever the matcher and the earlier guards decide, an interpolated frame must not show a triangle much longer than
// it is in the CURRENT frame: the game never draws it that way, and on screen it is a one-frame spike or rod. The
// remaining cases come from rigid interpolation of a skinned body, or from pairs that are wrong for reasons the matcher
// cannot see (an object or part that appears, vanishes or swaps level of detail between two frames). For every triangle
// whose rendered edge exceeds twice its CURRENT length (and by more than 4 units, the same rule as the rod classifier),
// the whole object (the WORLDs connected through shared triangles) is first shown with elementwise interpolation: each
// vertex stays on the chord between its endpoints, which keeps the interpolated time and does not step at 30 fps. Only
// if an edge is still stretched (a wrong pair) is that object shown at CURRENT. Default ON; TOOIE_TEST_P09_SPIKE_GUARD=0
// disables it and TOOIE_TEST_P09_SPIKE_ELEMENTWISE=0 restores the CURRENT-only behaviour. Inert while the endpoint-CURRENT
// workaround is active (the interpolated matrices do not reach the screen then).
//

#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <vector>

#include "hle/rt64_game_frame.h"
#include "hle/rt64_host_endpoint_current.h"
#include "hle/rt64_workload.h"

namespace RT64 {
namespace p09SpikeGuard {
    inline bool enabled() {
        static const bool on = [] {
            const char *e = std::getenv("TOOIE_TEST_P09_SPIKE_GUARD");
            return (e == nullptr) || (e[0] == '\0') || (e[0] != '0');
        }();
        return on;
    }

    // Elementwise chord for a stretched object. TOOIE_TEST_P09_SPIKE_ELEMENTWISE=0 snaps it to CURRENT instead.
    inline bool elementwiseEnabled() {
        static const bool on = [] {
            const char *e = std::getenv("TOOIE_TEST_P09_SPIKE_ELEMENTWISE");
            return (e == nullptr) || (e[0] == '\0') || (e[0] != '0');
        }();
        return on;
    }

    // Same edge rule as the rod classifier: rendered edge > 2x CURRENT edge and more than 4 units longer.
    inline bool stretched(float renderedLength, float currentLength) {
        const float ref = (currentLength > 1e-3f) ? currentLength : 1e-3f;
        return (renderedLength > 2.0f * ref) && ((renderedLength - ref) > 4.0f);
    }

    // Union-find over WORLD indices.
    inline uint32_t findRoot(std::vector<uint32_t> &parent, uint32_t x) {
        while (parent[x] != x) {
            parent[x] = parent[parent[x]];
            x = parent[x];
        }

        return x;
    }

    inline void unite(std::vector<uint32_t> &parent, uint32_t a, uint32_t b) {
        a = findRoot(parent, a);
        b = findRoot(parent, b);
        if (a != b) {
            parent[b] = a;
        }
    }

    // Segment coherence (TOOIE_TEST_P09_SEGMENT_COHERENCE=0 disables it). The AUTO gate in RigidBody::updateLinear holds the
    // translation of a WORLD whose speed jumps by 10x (a limb starting to swing, Kazooie leaving the backpack) as if it had
    // teleported. The translation coherence guard releases such holds only across shared triangles, which segmented models
    // (rigid limbs, no triangle between bones) do not have: the held limb is drawn at CURRENT while its body is halfway,
    // a misplaced part for one frame. A held WORLD whose moving neighbours (CURRENT origins within SegmentRadius) are
    // interpolated, and whose own jump is below SegmentMaxJump, belongs to a moving body and is interpolated too. A real
    // teleport moves the whole object, so its neighbours are held as well and nothing changes.
    constexpr float SegmentRadius = 80.0f;
    constexpr float SegmentMaxJump = 150.0f;

    inline bool segmentCoherenceEnabled() {
        static const bool on = [] {
            const char *e = std::getenv("TOOIE_TEST_P09_SEGMENT_COHERENCE");
            return (e == nullptr) || (e[0] == '\0') || (e[0] != '0');
        }();
        return on;
    }

    // Returns the number of WORLD translations released.
    inline uint32_t applySegmentCoherence(Workload &workload, const GameFrameMap::WorkloadMap &workloadMap, const DrawData &prevDrawData,
        float prevWeight, float curWeight)
    {
        if (!segmentCoherenceEnabled() || (curWeight >= 1.0f) || endpointCurrentActive()) {
            return 0;
        }

        DrawData &d = workload.drawData;
        const size_t nWorld = d.worldTransforms.size();
        if ((nWorld == 0) || (d.lerpWorldTransforms.size() != nWorld) || (d.prevWorldTransforms.size() != nWorld) ||
            (d.invTWorldTransforms.size() != nWorld) || (workloadMap.transforms.size() < nWorld)) {
            return 0;
        }

        // 0 = not moving or unmapped, 1 = moving and interpolated, 2 = moving and held at the CURRENT translation.
        std::vector<uint8_t> state(nWorld, 0);
        std::vector<hlslpp::float3> curPos(nWorld), prevPos(nWorld);
        for (size_t t = 0; t < nWorld; t++) {
            const hlslpp::float4x4 cur = d.worldTransforms[t];
            curPos[t] = cur[3].xyz;
            prevPos[t] = curPos[t];
            const GameFrameMap::TransformMap &tm = workloadMap.transforms[t];
            if (!tm.mapped || (tm.prevTransformIndex >= prevDrawData.worldTransforms.size())) continue;
            const hlslpp::float4x4 prev = prevDrawData.worldTransforms[tm.prevTransformIndex];
            prevPos[t] = prev[3].xyz;
            const float jump = float(hlslpp::length(curPos[t] - prevPos[t]));
            if (!(jump > 1e-2f)) continue;
            const hlslpp::float4x4 lerp = d.lerpWorldTransforms[t];
            const float fromCur = float(hlslpp::length(hlslpp::float3(lerp[3].xyz) - curPos[t]));
            state[t] = (fromCur < 1e-3f) ? 2 : 1;
        }

        uint32_t released = 0;
        for (size_t t = 0; t < nWorld; t++) {
            if (state[t] != 2) continue;
            if (float(hlslpp::length(curPos[t] - prevPos[t])) > SegmentMaxJump) continue;
            uint32_t moving = 0, held = 0;
            for (size_t u = 0; u < nWorld; u++) {
                if ((u == t) || (state[u] == 0)) continue;
                if (float(hlslpp::length(curPos[u] - curPos[t])) > SegmentRadius) continue;
                if (state[u] == 1) moving++; else held++;
            }

            if ((moving == 0) || (moving < held)) continue;
            const GameFrameMap::TransformMap &tm = workloadMap.transforms[t];
            const hlslpp::float4x4 prevEnd = prevDrawData.worldTransforms[tm.prevTransformIndex];
            const hlslpp::float4x4 curEnd = d.worldTransforms[t];
            RigidBody body = tm.rigidBody;
            body.lerpTranslation = true;
            const hlslpp::float4x4 curMatrix = body.lerp(curWeight, prevEnd, curEnd, true);
            d.lerpWorldTransforms[t] = curMatrix;
            d.prevWorldTransforms[t] = body.lerp(prevWeight, prevEnd, curEnd, true);
            d.invTWorldTransforms[t] = hlslpp::transpose(hlslpp::inverse(curMatrix));
            released++;
        }

        if (released > 0) {
            static std::atomic<uint64_t> total{ 0 };
            const uint64_t before = total.fetch_add(released);
            if ((before / 256) != ((before + released) / 256) || (before == 0)) {
                std::fprintf(stderr, "[tooie-interp] segment coherence released=%llu\n", (unsigned long long)(before + released));
            }
        }

        return released;
    }

    // Joint coherence (TOOIE_TEST_P09_JOINT_COHERENCE=0 disables it). Every WORLD is interpolated on its own: its origin
    // moves along a straight line while its rotation turns along an arc. A child piece hangs from its parent at a point
    // fixed in the parent's frame, so when the parent turns a lot between two game frames the child's straight-line
    // origin leaves that arc and the joint opens (wrist off the arm, shoulder off the body, on segmented models with no
    // triangle across the joint). The hierarchy is not in the display list, so it is inferred: a WORLD's parent is the
    // neighbouring WORLD (CURRENT origins within JointRadius) in whose local frame the child's origin stays put between
    // the two endpoints. The child keeps its own interpolated rotation and its origin is re-attached to the interpolated
    // parent at the interpolated local point. Roots are resolved first; cycles keep the lower index as the root.
    constexpr float JointRadius = 25.0f;
    constexpr float JointMaxDrift = 0.5f;

    inline bool jointCoherenceEnabled() {
        static const bool on = [] {
            const char *e = std::getenv("TOOIE_TEST_P09_JOINT_COHERENCE");
            return (e == nullptr) || (e[0] == '\0') || (e[0] != '0');
        }();
        return on;
    }

    // Returns the number of WORLD origins re-attached.
    inline uint32_t applyJointCoherence(Workload &workload, const GameFrameMap::WorkloadMap &workloadMap, const DrawData &prevDrawData,
        float prevWeight, float curWeight)
    {
        if (!jointCoherenceEnabled() || (curWeight >= 1.0f) || endpointCurrentActive()) {
            return 0;
        }

        DrawData &d = workload.drawData;
        const size_t nWorld = d.worldTransforms.size();
        if ((nWorld < 2) || (d.lerpWorldTransforms.size() != nWorld) || (d.prevWorldTransforms.size() != nWorld) ||
            (d.invTWorldTransforms.size() != nWorld) || (workloadMap.transforms.size() < nWorld)) {
            return 0;
        }

        auto xf = [](const hlslpp::float4x4 &m, const hlslpp::float3 &p) {
            const hlslpp::float4 r = hlslpp::mul(hlslpp::float4(p, 1.0f), m);
            return hlslpp::float3(r.xyz);
        };

        auto len3 = [](const hlslpp::float3 &v) {
            return std::sqrt(float(v.x) * float(v.x) + float(v.y) * float(v.y) + float(v.z) * float(v.z));
        };

        std::vector<uint8_t> mapped(nWorld, 0);
        std::vector<hlslpp::float4x4> prevEnd(nWorld), curEnd(nWorld), prevInv(nWorld), curInv(nWorld);
        for (size_t t = 0; t < nWorld; t++) {
            const GameFrameMap::TransformMap &tm = workloadMap.transforms[t];
            if (!tm.mapped || (tm.prevTransformIndex >= prevDrawData.worldTransforms.size())) continue;
            curEnd[t] = d.worldTransforms[t];
            prevEnd[t] = prevDrawData.worldTransforms[tm.prevTransformIndex];
            const float curDet = hlslpp::determinant(curEnd[t]);
            const float prevDet = hlslpp::determinant(prevEnd[t]);
            if (!(std::fabs(curDet) > 1e-9f) || !(std::fabs(prevDet) > 1e-9f)) continue;
            curInv[t] = hlslpp::inverse(curEnd[t]);
            prevInv[t] = hlslpp::inverse(prevEnd[t]);
            mapped[t] = 1;
        }

        // Parent choice: smallest drift of the child's origin in the candidate parent's local frame.
        std::vector<int32_t> parent(nWorld, -1);
        std::vector<hlslpp::float3> localPrev(nWorld), localCur(nWorld);
        for (size_t c = 0; c < nWorld; c++) {
            if (!mapped[c]) continue;
            const hlslpp::float3 cCur = curEnd[c][3].xyz;
            const hlslpp::float3 cPrev = prevEnd[c][3].xyz;
            float bestDrift = JointMaxDrift;
            for (size_t p = 0; p < nWorld; p++) {
                if ((p == c) || !mapped[p]) continue;
                if (len3(hlslpp::float3(curEnd[p][3].xyz) - cCur) > JointRadius) continue;
                const hlslpp::float3 lc = xf(curInv[p], cCur);
                const hlslpp::float3 lp = xf(prevInv[p], cPrev);
                const float drift = len3(lc - lp);
                if (drift < bestDrift) {
                    bestDrift = drift;
                    parent[c] = int32_t(p);
                    localCur[c] = lc;
                    localPrev[c] = lp;
                }
            }
        }

        // Break cycles: walk each chain; a WORLD met twice on its own walk is cut at the lowest index of the cycle.
        std::vector<uint8_t> mark(nWorld, 0); // 0 unseen, 1 on the current walk, 2 done
        for (size_t s = 0; s < nWorld; s++) {
            if (mark[s] != 0) continue;
            std::vector<uint32_t> walk;
            int32_t t = int32_t(s);
            while ((t >= 0) && (mark[t] == 0)) {
                mark[t] = 1;
                walk.push_back(uint32_t(t));
                t = parent[t];
            }

            if ((t >= 0) && (mark[t] == 1)) {
                uint32_t lowest = uint32_t(t);
                int32_t u = parent[t];
                while ((u >= 0) && (u != t)) {
                    lowest = std::min(lowest, uint32_t(u));
                    u = parent[u];
                }

                parent[lowest] = -1;
            }

            for (uint32_t w : walk) mark[w] = 2;
        }

        // Resolve from the roots down (memoised depth-first).
        std::vector<uint8_t> done(nWorld, 0);
        uint32_t attached = 0;
        std::vector<uint32_t> stack;
        for (size_t s = 0; s < nWorld; s++) {
            if (done[s] || (parent[s] < 0)) continue;
            stack.clear();
            int32_t t = int32_t(s);
            while ((t >= 0) && !done[t] && (parent[t] >= 0)) {
                stack.push_back(uint32_t(t));
                t = parent[t];
            }

            for (size_t i = stack.size(); i-- > 0;) {
                const uint32_t c = stack[i];
                const uint32_t p = uint32_t(parent[c]);
                const hlslpp::float4x4 parentCur = d.lerpWorldTransforms[p];
                const hlslpp::float4x4 parentPrev = d.prevWorldTransforms[p];
                hlslpp::float4x4 childCur = d.lerpWorldTransforms[c];
                hlslpp::float4x4 childPrev = d.prevWorldTransforms[c];
                const hlslpp::float3 localAtCur = localPrev[c] + (localCur[c] - localPrev[c]) * curWeight;
                const hlslpp::float3 localAtPrev = localPrev[c] + (localCur[c] - localPrev[c]) * prevWeight;
                childCur[3] = hlslpp::float4(xf(parentCur, localAtCur), 1.0f);
                childPrev[3] = hlslpp::float4(xf(parentPrev, localAtPrev), 1.0f);
                d.lerpWorldTransforms[c] = childCur;
                d.prevWorldTransforms[c] = childPrev;
                d.invTWorldTransforms[c] = hlslpp::transpose(hlslpp::inverse(childCur));
                done[c] = 1;
                attached++;
            }
        }

        static const bool debug = std::getenv("TOOIE_DIAG_P09_JOINT") != nullptr;
        if (debug) {
            static std::atomic<uint64_t> calls{ 0 };
            if ((++calls % 200) == 0) {
                // Best drift of every mapped WORLD against any neighbour, to see why parents are (not) found.
                uint32_t nb = 0, under05 = 0, under2 = 0, under5 = 0;
                for (size_t c = 0; c < nWorld; c++) {
                    if (!mapped[c]) continue;
                    float best = 1e9f;
                    for (size_t p = 0; p < nWorld; p++) {
                        if ((p == c) || !mapped[p]) continue;
                        if (len3(hlslpp::float3(curEnd[p][3].xyz) - hlslpp::float3(curEnd[c][3].xyz)) > JointRadius) continue;
                        best = std::min(best, len3(xf(curInv[p], curEnd[c][3].xyz) - xf(prevInv[p], prevEnd[c][3].xyz)));
                    }
                    if (best > 1e8f) continue;
                    nb++;
                    under05 += best < 0.5f;
                    under2 += best < 2.0f;
                    under5 += best < 5.0f;
                }
                std::fprintf(stderr, "[p09-joint] worlds=%zu withNeighbour=%u drift<0.5=%u <2=%u <5=%u attached=%u\n", nWorld, nb, under05, under2, under5, attached);
            }
        }

        return attached;
    }

    // Unites WORLDs joined by a joint (the child's origin stays within JointMaxDrift in the parent's local frame between the
    // two endpoints, CURRENT origins within JointRadius): segmented models have no triangle across their joints, so this is
    // what makes one articulated object a single group.
    inline void uniteJoints(const DrawData &d, const GameFrameMap::WorkloadMap &workloadMap, const DrawData &prevDrawData, std::vector<uint32_t> &parent) {
        const size_t nWorld = d.worldTransforms.size();
        auto xf = [](const hlslpp::float4x4 &m, const hlslpp::float3 &p) {
            const hlslpp::float4 r = hlslpp::mul(hlslpp::float4(p, 1.0f), m);
            return hlslpp::float3(r.xyz);
        };

        auto len3 = [](const hlslpp::float3 &v) {
            return std::sqrt(float(v.x) * float(v.x) + float(v.y) * float(v.y) + float(v.z) * float(v.z));
        };

        std::vector<uint8_t> ok(nWorld, 0);
        std::vector<hlslpp::float4x4> curEnd(nWorld), prevEnd(nWorld), curInv(nWorld), prevInv(nWorld);
        for (size_t t = 0; t < nWorld; t++) {
            const GameFrameMap::TransformMap &tm = workloadMap.transforms[t];
            if (!tm.mapped || (tm.prevTransformIndex >= prevDrawData.worldTransforms.size())) continue;
            curEnd[t] = d.worldTransforms[t];
            prevEnd[t] = prevDrawData.worldTransforms[tm.prevTransformIndex];
            if (!(std::fabs(hlslpp::determinant(curEnd[t])) > 1e-9f) || !(std::fabs(hlslpp::determinant(prevEnd[t])) > 1e-9f)) continue;
            curInv[t] = hlslpp::inverse(curEnd[t]);
            prevInv[t] = hlslpp::inverse(prevEnd[t]);
            ok[t] = 1;
        }

        for (size_t c = 0; c < nWorld; c++) {
            if (!ok[c]) continue;
            const hlslpp::float3 cc = curEnd[c][3].xyz;
            const hlslpp::float3 cp = prevEnd[c][3].xyz;
            for (size_t p = 0; p < nWorld; p++) {
                if ((p == c) || !ok[p]) continue;
                if (len3(hlslpp::float3(curEnd[p][3].xyz) - cc) > JointRadius) continue;
                if (len3(xf(curInv[p], cc) - xf(prevInv[p], cp)) < JointMaxDrift) {
                    unite(parent, uint32_t(c), uint32_t(p));
                }
            }
        }
    }

    // Returns the number of WORLD matrices rewritten (elementwise, or CURRENT when an edge stays stretched).
    inline uint32_t apply(Workload &workload, const GameFrameMap::WorkloadMap &workloadMap, const DrawData &prevDrawData, float prevWeight, float curWeight) {
        if (!enabled() || (curWeight >= 1.0f) || endpointCurrentActive()) {
            return 0;
        }

        DrawData &d = workload.drawData;
        const size_t nWorld = d.worldTransforms.size();
        if ((nWorld == 0) || (d.lerpWorldTransforms.size() != nWorld) || (d.prevWorldTransforms.size() != nWorld) ||
            (d.invTWorldTransforms.size() != nWorld) || (workloadMap.transforms.size() < nWorld)) {
            return 0;
        }

        const uint32_t nVert = d.vertexCount();
        if ((d.worldIndices.size() < nVert) || (d.posFloats.size() < size_t(nVert) * 3)) {
            return 0;
        }

        const bool hasVel = d.velFloats.size() >= size_t(nVert) * 3;
        auto xf = [](const hlslpp::float4x4 &m, const hlslpp::float3 &p) {
            const hlslpp::float4 r = hlslpp::mul(hlslpp::float4(p, 1.0f), m);
            return hlslpp::float3(r.xyz);
        };

        auto len3 = [](const hlslpp::float3 &v) {
            return std::sqrt(float(v.x) * float(v.x) + float(v.y) * float(v.y) + float(v.z) * float(v.z));
        };

        std::vector<uint32_t> parent(nWorld);
        std::iota(parent.begin(), parent.end(), 0u);
        const size_t nTri = d.faceIndices.size() / 3;
        for (size_t f = 0; f < nTri; f++) {
            const uint32_t v0 = d.faceIndices[f * 3], v1 = d.faceIndices[f * 3 + 1], v2 = d.faceIndices[f * 3 + 2];
            if ((v0 >= nVert) || (v1 >= nVert) || (v2 >= nVert)) continue;
            const uint32_t w0 = d.worldIndices[v0], w1 = d.worldIndices[v1], w2 = d.worldIndices[v2];
            if ((w0 >= nWorld) || (w1 >= nWorld) || (w2 >= nWorld)) continue;
            unite(parent, w0, w1);
            unite(parent, w1, w2);
        }

        uniteJoints(d, workloadMap, prevDrawData, parent);

        std::vector<uint8_t> snapRoot(nWorld, 0);
        bool any = false;
        for (size_t f = 0; f < nTri; f++) {
            const uint32_t v[3] = { d.faceIndices[f * 3], d.faceIndices[f * 3 + 1], d.faceIndices[f * 3 + 2] };
            if ((v[0] >= nVert) || (v[1] >= nVert) || (v[2] >= nVert)) continue;
            const uint32_t w[3] = { d.worldIndices[v[0]], d.worldIndices[v[1]], d.worldIndices[v[2]] };
            if ((w[0] >= nWorld) || (w[1] >= nWorld) || (w[2] >= nWorld)) continue;
            if (!workloadMap.transforms[w[0]].mapped && !workloadMap.transforms[w[1]].mapped && !workloadMap.transforms[w[2]].mapped) continue;
            const uint32_t root = findRoot(parent, w[0]);
            if (snapRoot[root]) continue;
            hlslpp::float3 pc[3], pl[3];
            for (int i = 0; i < 3; i++) {
                const hlslpp::float3 lc(d.posFloats[size_t(v[i]) * 3 + 0], d.posFloats[size_t(v[i]) * 3 + 1], d.posFloats[size_t(v[i]) * 3 + 2]);
                hlslpp::float3 lv(0.0f, 0.0f, 0.0f);
                if (hasVel) {
                    lv = hlslpp::float3(d.velFloats[size_t(v[i]) * 3 + 0], d.velFloats[size_t(v[i]) * 3 + 1], d.velFloats[size_t(v[i]) * 3 + 2]);
                }

                pc[i] = xf(d.worldTransforms[w[i]], lc);
                pl[i] = xf(d.lerpWorldTransforms[w[i]], lc - lv * (1.0f - curWeight));
            }

            for (int e = 0; e < 3; e++) {
                const int a = e, b = (e + 1) % 3;
                if (stretched(len3(pl[a] - pl[b]), len3(pc[a] - pc[b]))) {
                    snapRoot[root] = 1;
                    any = true;
                    break;
                }
            }
        }

        if (!any) {
            return 0;
        }

        // Elementwise chord first. TOOIE_TEST_P09_SPIKE_ELEMENTWISE=0 keeps the CURRENT snap for every flagged root.
        std::vector<hlslpp::float4x4> ew;
        std::vector<uint8_t> stillRoot;
        if (elementwiseEnabled()) {
            ew.resize(nWorld);
            stillRoot.assign(nWorld, 0);
            for (size_t t = 0; t < nWorld; t++) {
                const GameFrameMap::TransformMap &tm = workloadMap.transforms[t];
                const bool flagged = snapRoot[findRoot(parent, uint32_t(t))] != 0;
                if (flagged && tm.mapped && (tm.prevTransformIndex < prevDrawData.worldTransforms.size())) {
                    ew[t] = lerpMatrix(prevDrawData.worldTransforms[tm.prevTransformIndex], d.worldTransforms[t], curWeight);
                }
                else {
                    ew[t] = d.lerpWorldTransforms[t];
                }
            }

            for (size_t f = 0; f < nTri; f++) {
                const uint32_t v[3] = { d.faceIndices[f * 3], d.faceIndices[f * 3 + 1], d.faceIndices[f * 3 + 2] };
                if ((v[0] >= nVert) || (v[1] >= nVert) || (v[2] >= nVert)) continue;
                const uint32_t w[3] = { d.worldIndices[v[0]], d.worldIndices[v[1]], d.worldIndices[v[2]] };
                if ((w[0] >= nWorld) || (w[1] >= nWorld) || (w[2] >= nWorld)) continue;
                if (!workloadMap.transforms[w[0]].mapped && !workloadMap.transforms[w[1]].mapped && !workloadMap.transforms[w[2]].mapped) continue;
                const uint32_t root = findRoot(parent, w[0]);
                if (!snapRoot[root] || stillRoot[root]) continue;
                hlslpp::float3 pc[3], pl[3];
                for (int i = 0; i < 3; i++) {
                    const hlslpp::float3 lc(d.posFloats[size_t(v[i]) * 3 + 0], d.posFloats[size_t(v[i]) * 3 + 1], d.posFloats[size_t(v[i]) * 3 + 2]);
                    hlslpp::float3 lv(0.0f, 0.0f, 0.0f);
                    if (hasVel) {
                        lv = hlslpp::float3(d.velFloats[size_t(v[i]) * 3 + 0], d.velFloats[size_t(v[i]) * 3 + 1], d.velFloats[size_t(v[i]) * 3 + 2]);
                    }

                    pc[i] = xf(d.worldTransforms[w[i]], lc);
                    pl[i] = xf(ew[w[i]], lc - lv * (1.0f - curWeight));
                }

                for (int e = 0; e < 3; e++) {
                    const int a = e, b = (e + 1) % 3;
                    if (stretched(len3(pl[a] - pl[b]), len3(pc[a] - pc[b]))) {
                        stillRoot[root] = 1;
                        break;
                    }
                }
            }
        }

        uint32_t changed = 0;
        for (size_t t = 0; t < nWorld; t++) {
            if (!workloadMap.transforms[t].mapped || !snapRoot[findRoot(parent, uint32_t(t))]) continue;
            const uint32_t root = findRoot(parent, uint32_t(t));
            if (elementwiseEnabled() && !stillRoot[root]) {
                const GameFrameMap::TransformMap &tm = workloadMap.transforms[t];
                if (tm.prevTransformIndex < prevDrawData.worldTransforms.size()) {
                    const hlslpp::float4x4 prevEnd = prevDrawData.worldTransforms[tm.prevTransformIndex];
                    const hlslpp::float4x4 curEnd = d.worldTransforms[t];
                    d.lerpWorldTransforms[t] = ew[t];
                    d.prevWorldTransforms[t] = lerpMatrix(prevEnd, curEnd, prevWeight);
                    d.invTWorldTransforms[t] = hlslpp::transpose(hlslpp::inverse(ew[t]));
                    changed++;
                }

                continue;
            }

            const hlslpp::float4x4 cur = d.worldTransforms[t];
            d.lerpWorldTransforms[t] = cur;
            d.prevWorldTransforms[t] = cur;
            d.invTWorldTransforms[t] = hlslpp::transpose(hlslpp::inverse(cur));
            changed++;
        }

        return changed;
    }
};
};
