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

#include <cmath>
#include <cstdint>
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
