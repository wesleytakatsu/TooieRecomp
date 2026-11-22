//
// RT64
//
// P09 ordered WORLD transform matching for untagged (G_EX_ID_AUTO) games.
//
// GameFrame::matchScene pairs current and previous WORLD transforms greedily by a
// cost dominated by the matrix origin (plus a velocity prediction inherited from
// the previous pairing). Bones that share a pivot (several feathers of one crest,
// instanced copies of one mesh piece) are indistinguishable by that cost, and a
// wrong pair poisons the inherited velocity of the next frame. A wrong pair
// interpolates a bone between two different bones, which renders as rods and
// detached parts on articulated models.
//
// The display list draws an object's calls in the same order every frame, so the
// k-th call with a given match hash in the current frame is the k-th call with
// that hash in the previous frame whenever both frames hold the same number of
// such calls. Those "ordered" pairs are assigned first; every other candidate
// keeps the original cost order as a fallback. Candidates whose volume scale
// changes by 8x or more (2x linear) in one frame are different objects and are
// rejected outright.
//
// Default ON. TOOIE_TEST_P09_ORDERED_MATCH=0 restores the original matching.
//

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace RT64 {
namespace p09OrderedMatch {
    inline bool enabled() {
        static const bool on = [] {
            const char *e = std::getenv("TOOIE_TEST_P09_ORDERED_MATCH");
            return (e == nullptr) || (e[0] == '\0') || (e[0] != '0');
        }();
        return on;
    }

    // One draw call as seen by the matcher, in submission order.
    struct Call {
        uint64_t hash;
        uint32_t minWorld;
        uint32_t worldCount;
        bool doTransformMatching;
    };

    // Pairs (current WORLD, previous WORLD) of calls that hold the same rank among the calls sharing their hash.
    inline void buildOrderedPairs(const std::vector<Call> &cur, const std::vector<Call> &prev, std::set<std::pair<uint32_t, uint32_t>> &pairs) {
        pairs.clear();
        std::map<uint64_t, std::vector<const Call *>> curByHash, prevByHash;
        for (const Call &call : cur) {
            curByHash[call.hash].push_back(&call);
        }

        for (const Call &call : prev) {
            prevByHash[call.hash].push_back(&call);
        }

        for (const auto &it : curByHash) {
            auto prevIt = prevByHash.find(it.first);
            if ((prevIt == prevByHash.end()) || (prevIt->second.size() != it.second.size())) {
                continue;
            }

            for (size_t k = 0; k < it.second.size(); k++) {
                const Call &curCall = *it.second[k];
                const Call &prevCall = *prevIt->second[k];
                if (!curCall.doTransformMatching || !prevCall.doTransformMatching || (curCall.worldCount != prevCall.worldCount)) {
                    continue;
                }

                for (uint32_t w = 0; w < curCall.worldCount; w++) {
                    pairs.emplace(curCall.minWorld + w, prevCall.minWorld + w);
                }
            }
        }
    }

    // Order-preserving alignment for hash groups whose call count changed between frames (an object or part appeared or
    // vanished). The greedy fallback pairs such groups bone by bone and can give the two bones of one skinned triangle
    // different offsets, which renders as a one-frame spike. Calls keep their submission order, so this aligns the two
    // sequences with gaps: it maximises the number of pairs whose first WORLD origins lie within maxDistance and breaks
    // ties by the smallest total distance. TOOIE_TEST_P09_ALIGN_MATCH=0 disables it (unequal groups use the fallback).
    inline bool alignEnabled() {
        static const bool on = [] {
            const char *e = std::getenv("TOOIE_TEST_P09_ALIGN_MATCH");
            return (e == nullptr) || (e[0] == '\0') || (e[0] != '0');
        }();
        return on;
    }

    // Groups above this many cell evaluations keep the fallback (bounded per-frame cost).
    constexpr size_t AlignMaxCells = 1u << 18;

    // Largest first-WORLD origin move accepted as the same call between two frames (game units; walking is ~10-30).
    constexpr float AlignMaxDistance = 96.0f;

    template <typename DistanceFn>
    inline void alignGroup(const std::vector<const Call *> &cur, const std::vector<const Call *> &prev, float maxDistance, DistanceFn distance, std::set<std::pair<uint32_t, uint32_t>> &pairs) {
        const size_t n = cur.size();
        const size_t m = prev.size();
        if ((n == 0) || (m == 0) || ((n + 1) * (m + 1) > AlignMaxCells)) {
            return;
        }

        // score = pairs * pairWeight - total distance; pairWeight dominates any sum of in-range distances.
        const double pairWeight = 1.0 + double(maxDistance) * double((n < m) ? n : m);
        const double reject = -1.0e300;
        std::vector<double> score((n + 1) * (m + 1), 0.0);
        std::vector<uint8_t> step((n + 1) * (m + 1), 0);
        auto at = [m](size_t i, size_t j) { return i * (m + 1) + j; };
        for (size_t i = 1; i <= n; i++) {
            step[at(i, 0)] = 1;
        }

        for (size_t j = 1; j <= m; j++) {
            step[at(0, j)] = 2;
        }

        for (size_t i = 1; i <= n; i++) {
            for (size_t j = 1; j <= m; j++) {
                double best = score[at(i - 1, j)];
                uint8_t bestStep = 1;
                if (score[at(i, j - 1)] > best) {
                    best = score[at(i, j - 1)];
                    bestStep = 2;
                }

                const Call &c = *cur[i - 1];
                const Call &p = *prev[j - 1];
                double diagonal = reject;
                if (c.doTransformMatching && p.doTransformMatching && (c.worldCount == p.worldCount)) {
                    const float d = distance(c, p);
                    if ((d >= 0.0f) && (d <= maxDistance)) {
                        diagonal = score[at(i - 1, j - 1)] + pairWeight - double(d);
                    }
                }

                if (diagonal > best) {
                    best = diagonal;
                    bestStep = 3;
                }

                score[at(i, j)] = best;
                step[at(i, j)] = bestStep;
            }
        }

        size_t i = n;
        size_t j = m;
        while ((i > 0) && (j > 0)) {
            const uint8_t s = step[at(i, j)];
            if (s == 3) {
                const Call &c = *cur[i - 1];
                const Call &p = *prev[j - 1];
                for (uint32_t w = 0; w < c.worldCount; w++) {
                    pairs.emplace(c.minWorld + w, p.minWorld + w);
                }

                i--;
                j--;
            }
            else if (s == 1) {
                i--;
            }
            else {
                j--;
            }
        }
    }

    // Rank pairing assumes the display list keeps the calls of one hash in the same order. Calls whose submission order
    // changes between frames (for example objects drawn in camera-distance order) break that: the fire billboards of two
    // torches share one hash (same combiner, other mode and triangle count), and when their ranks swap the k-th pair
    // interpolates one torch's fire halfway towards the other for a single frame. In an equal-count group, a call whose
    // ranked partner is far away (> ReorderMinMove) while another previous call of the group lies at least ReorderRatio
    // times closer is a swap suspect; the suspects are re-paired among themselves by nearest drawn position (world-space
    // centroid of the first WORLD's vertices). Bones sharing a pivot keep their ranks (their distances are all small), as
    // does a lone fast object (its only suspect partner is itself).
    // Default ON. TOOIE_TEST_P09_REORDER_MATCH=0 keeps rank pairing for every equal-count group.
    inline bool reorderEnabled() {
        static const bool on = [] {
            const char *e = std::getenv("TOOIE_TEST_P09_REORDER_MATCH");
            return (e == nullptr) || (e[0] == '\0') || (e[0] != '0');
        }();
        return on;
    }

    // TOOIE_DIAG_P09_REORDER=1 logs each re-paired call to stderr. Default OFF.
    inline bool reorderDiagEnabled() {
        static const bool on = [] {
            const char *e = std::getenv("TOOIE_DIAG_P09_REORDER");
            return (e != nullptr) && (e[0] != '\0') && (e[0] != '0');
        }();
        return on;
    }

    constexpr float ReorderMinMove = 16.0f;
    constexpr float ReorderRatio = 4.0f;
    // Groups larger than this keep rank pairing (bounds the worst case, a group where every call moved: n^2 distances).
    constexpr size_t ReorderMaxCalls = 256;

    inline bool pairable(const Call &c, const Call &p) {
        return c.doTransformMatching && p.doTransformMatching && (c.worldCount == p.worldCount);
    }

    // Re-pairs the swap suspects of one equal-count group. Returns the number of calls whose partner changed.
    template <typename DistanceFn>
    inline uint32_t reorderGroup(const std::vector<const Call *> &cur, const std::vector<const Call *> &prev, DistanceFn distance, std::set<std::pair<uint32_t, uint32_t>> &pairs) {
        const size_t n = cur.size();
        if ((n < 2) || (n != prev.size()) || (n > ReorderMaxCalls)) {
            return 0;
        }

        // Distances are evaluated lazily: a static group only costs its n ranked pairs. -1 marks an unpairable couple.
        auto dist = [&](size_t i, size_t j) {
            return pairable(*cur[i], *prev[j]) ? distance(*cur[i], *prev[j]) : -1.0f;
        };

        thread_local std::vector<float> own;
        thread_local std::vector<size_t> suspects;
        own.resize(n);
        suspects.clear();
        for (size_t k = 0; k < n; k++) {
            own[k] = dist(k, k);
        }

        for (size_t k = 0; k < n; k++) {
            if (!(own[k] > ReorderMinMove)) {
                continue;
            }

            for (size_t j = 0; j < n; j++) {
                if (j == k) {
                    continue;
                }

                const float d = dist(k, j);
                if ((d >= 0.0f) && ((d * ReorderRatio) < own[k])) {
                    suspects.push_back(k);
                    break;
                }
            }
        }

        if (suspects.size() < 2) {
            return 0;
        }

        // Greedy nearest assignment among the suspects (the previous calls ranked at the suspects' positions).
        struct Edge { float d; uint32_t i; uint32_t j; };
        thread_local std::vector<Edge> edges;
        edges.clear();
        for (size_t i : suspects) {
            for (size_t j : suspects) {
                const float d = (i == j) ? own[i] : dist(i, j);
                if (d >= 0.0f) {
                    edges.push_back({ d, uint32_t(i), uint32_t(j) });
                }
            }
        }

        std::stable_sort(edges.begin(), edges.end(), [](const Edge &a, const Edge &b) { return a.d < b.d; });
        thread_local std::vector<uint32_t> assigned;
        thread_local std::vector<float> assignedDistance;
        thread_local std::vector<uint8_t> prevUsed;
        assigned.assign(n, UINT32_MAX);
        assignedDistance.assign(n, -1.0f);
        prevUsed.assign(n, 0);
        for (const Edge &e : edges) {
            if ((assigned[e.i] == UINT32_MAX) && !prevUsed[e.j]) {
                assigned[e.i] = e.j;
                assignedDistance[e.i] = e.d;
                prevUsed[e.j] = 1;
            }
        }

        uint32_t changed = 0;
        for (size_t i : suspects) {
            const uint32_t j = assigned[i];
            if (j == i) {
                continue;
            }

            const Call &c = *cur[i];
            if (pairable(c, *prev[i])) {
                for (uint32_t w = 0; w < c.worldCount; w++) {
                    pairs.erase(std::make_pair(c.minWorld + w, prev[i]->minWorld + w));
                }
            }

            if (j != UINT32_MAX) {
                const Call &p = *prev[j];
                for (uint32_t w = 0; w < c.worldCount; w++) {
                    pairs.emplace(c.minWorld + w, p.minWorld + w);
                }
            }

            if (reorderDiagEnabled()) {
                std::fprintf(stderr, "[tooie-interp] reorder hash=%016llx n=%zu rank=%zu -> %d world=%u d=%.1f -> %.1f\n",
                    (unsigned long long)c.hash, n, i, (j != UINT32_MAX) ? int(j) : -1, c.minWorld, own[i], assignedDistance[i]);
            }

            changed++;
        }

        return changed;
    }

    // buildOrderedPairs plus alignment of the unequal-count groups and re-pairing of swapped calls in equal-count groups.
    // reorderDistance compares where two calls draw (the game may bake a billboard's position into its vertices).
    template <typename DistanceFn, typename ReorderDistanceFn>
    inline void buildOrderedPairs(const std::vector<Call> &cur, const std::vector<Call> &prev, std::set<std::pair<uint32_t, uint32_t>> &pairs, float maxDistance, DistanceFn distance, ReorderDistanceFn reorderDistance) {
        buildOrderedPairs(cur, prev, pairs);
        std::map<uint64_t, std::vector<const Call *>> curByHash, prevByHash;
        for (const Call &call : cur) {
            curByHash[call.hash].push_back(&call);
        }

        for (const Call &call : prev) {
            prevByHash[call.hash].push_back(&call);
        }

        for (const auto &it : curByHash) {
            auto prevIt = prevByHash.find(it.first);
            if (prevIt == prevByHash.end()) {
                continue;
            }

            if (prevIt->second.size() != it.second.size()) {
                if (alignEnabled()) {
                    alignGroup(it.second, prevIt->second, maxDistance, distance, pairs);
                }
            }
            else if (reorderEnabled()) {
                reorderGroup(it.second, prevIt->second, reorderDistance, pairs);
            }
        }
    }

    // |det| of the 3x3 part of both matrices. A ratio of 8 or more is a 2x linear scale jump: a different object.
    inline bool scaleCompatible(float curDeterminant, float prevDeterminant) {
        const float a = std::fabs(curDeterminant);
        const float b = std::fabs(prevDeterminant);
        const float lo = (a < b) ? a : b;
        const float hi = (a < b) ? b : a;
        return !(hi > 0.0f) || (lo > 0.125f * hi);
    }
};
};
