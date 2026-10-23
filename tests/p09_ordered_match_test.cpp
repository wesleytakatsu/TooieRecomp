// Host test for the P09 ordered WORLD matcher in RT64:
//   hle/rt64_p09_ordered_match.h

#include "hle/rt64_p09_ordered_match.h"

#include <cmath>
#include <cstdio>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace {

using RT64::p09OrderedMatch::Call;
using Pairs = std::set<std::pair<uint32_t, uint32_t>>;
using RT64::p09OrderedMatch::AlignMaxDistance;
using Origin = std::map<uint32_t, float>;

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

Call call(uint64_t hash, uint32_t minWorld, uint32_t worldCount = 1, bool doTransformMatching = true) {
    return Call{ hash, minWorld, worldCount, doTransformMatching };
}

std::vector<const Call *> toPointers(const std::vector<Call> &calls) {
    std::vector<const Call *> out;
    for (const Call &c : calls) {
        out.push_back(&c);
    }
    return out;
}

// Distance between two calls is the 1-D origin difference, looked up by each call's minWorld.
auto originDistance(const Origin &origin) {
    return [&origin](const Call &a, const Call &b) {
        return std::fabs(origin.at(a.minWorld) - origin.at(b.minWorld));
    };
}

Pairs alignedPairs(const std::vector<Call> &cur, const std::vector<Call> &prev, const Origin &origin, float maxDistance) {
    Pairs pairs;
    RT64::p09OrderedMatch::alignGroup(toPointers(cur), toPointers(prev), maxDistance, originDistance(origin), pairs);
    return pairs;
}

void testEqualCountsPairByRank() {
    const std::vector<Call> cur = { call(1, 10), call(2, 20), call(1, 11), call(2, 21) };
    const std::vector<Call> prev = { call(2, 40), call(1, 30), call(2, 41), call(1, 31) };
    Pairs pairs;
    RT64::p09OrderedMatch::buildOrderedPairs(cur, prev, pairs);
    const Pairs expected = { { 10, 30 }, { 11, 31 }, { 20, 40 }, { 21, 41 } };
    CHECK(pairs == expected);
}

void testUnequalCountsSkipOnlyThatHash() {
    const std::vector<Call> cur = { call(1, 10), call(1, 11), call(2, 20) };
    const std::vector<Call> prev = { call(1, 30), call(2, 40) };
    Pairs pairs;
    RT64::p09OrderedMatch::buildOrderedPairs(cur, prev, pairs);
    const Pairs expected = { { 20, 40 } };
    CHECK(pairs == expected);
}

void testHashMissingFromPrevIsSkipped() {
    const std::vector<Call> cur = { call(1, 10), call(2, 20) };
    const std::vector<Call> prev = { call(2, 40) };
    Pairs pairs;
    RT64::p09OrderedMatch::buildOrderedPairs(cur, prev, pairs);
    const Pairs expected = { { 20, 40 } };
    CHECK(pairs == expected);
}

void testMultiWorldCallsExpandWithOffsets() {
    const std::vector<Call> cur = { call(7, 100, 3) };
    const std::vector<Call> prev = { call(7, 200, 3) };
    Pairs pairs;
    RT64::p09OrderedMatch::buildOrderedPairs(cur, prev, pairs);
    const Pairs expected = { { 100, 200 }, { 101, 201 }, { 102, 202 } };
    CHECK(pairs == expected);
}

void testDifferentWorldCountIsSkipped() {
    const std::vector<Call> cur = { call(7, 100, 3) };
    const std::vector<Call> prev = { call(7, 200, 2) };
    Pairs pairs;
    RT64::p09OrderedMatch::buildOrderedPairs(cur, prev, pairs);
    CHECK(pairs.empty());
}

void testDoTransformMatchingFalseIsSkipped() {
    Pairs pairs;
    const std::vector<Call> cur = { call(1, 10, 1, false), call(2, 20) };
    const std::vector<Call> prev = { call(1, 30), call(2, 40) };
    RT64::p09OrderedMatch::buildOrderedPairs(cur, prev, pairs);
    CHECK(pairs == Pairs({ { 20, 40 } }));

    const std::vector<Call> cur2 = { call(1, 10), call(2, 20) };
    const std::vector<Call> prev2 = { call(1, 30, 1, false), call(2, 40) };
    RT64::p09OrderedMatch::buildOrderedPairs(cur2, prev2, pairs);
    CHECK(pairs == Pairs({ { 20, 40 } }));
}

void testEmptyInputs() {
    Pairs pairs;
    RT64::p09OrderedMatch::buildOrderedPairs({}, {}, pairs);
    CHECK(pairs.empty());

    RT64::p09OrderedMatch::buildOrderedPairs({ call(1, 10) }, {}, pairs);
    CHECK(pairs.empty());

    RT64::p09OrderedMatch::buildOrderedPairs({}, { call(1, 10) }, pairs);
    CHECK(pairs.empty());
}

void testPairsClearedOnEntry() {
    Pairs pairs = { { 999, 999 } };
    RT64::p09OrderedMatch::buildOrderedPairs({}, {}, pairs);
    CHECK(pairs.empty());

    pairs = { { 999, 999 } };
    RT64::p09OrderedMatch::buildOrderedPairs({ call(1, 10) }, { call(1, 30) }, pairs);
    CHECK(pairs == Pairs({ { 10, 30 } }));
}

void testSwappedInstancesPairByOrderNotPosition() {
    // Three calls share one hash. Previous frame draws them in reverse minWorld order.
    const std::vector<Call> cur = { call(5, 10), call(5, 20), call(5, 30) };
    const std::vector<Call> prev = { call(5, 30), call(5, 20), call(5, 10) };
    Pairs pairs;
    RT64::p09OrderedMatch::buildOrderedPairs(cur, prev, pairs);
    const Pairs expected = { { 10, 30 }, { 20, 20 }, { 30, 10 } };
    CHECK(pairs == expected);
    CHECK(pairs.size() == 3);
}

void testScaleCompatibleBoundaries() {
    using RT64::p09OrderedMatch::scaleCompatible;
    CHECK(scaleCompatible(1.0f, 1.0f));
    CHECK(scaleCompatible(1.0f, 0.126f));
    CHECK(!scaleCompatible(1.0f, 0.125f));
    CHECK(!scaleCompatible(0.125f, 1.0f));
    CHECK(scaleCompatible(0.0f, 0.0f));
    CHECK(!scaleCompatible(0.0f, 1.0f));
    CHECK(!scaleCompatible(1.0f, 0.0f));
}

void testScaleCompatibleUsesAbsoluteDeterminant() {
    using RT64::p09OrderedMatch::scaleCompatible;
    CHECK(scaleCompatible(-1.0f, 1.0f));
    CHECK(scaleCompatible(-1.0f, -0.9f));
    CHECK(!scaleCompatible(-1.0f, 0.125f));
    CHECK(!scaleCompatible(-0.125f, 1.0f));
}

void testEnabledCallable() {
    const bool on = RT64::p09OrderedMatch::enabled();
    CHECK(on == RT64::p09OrderedMatch::enabled());
}

void testVanishedCallInMiddleAligns() {
    const Origin origin = { { 10, 0.0f }, { 11, 100.0f }, { 12, 200.0f }, { 13, 300.0f }, { 20, 0.0f }, { 21, 200.0f }, { 22, 300.0f } };
    const std::vector<Call> cur = { call(1, 20), call(1, 21), call(1, 22) };
    const std::vector<Call> prev = { call(1, 10), call(1, 11), call(1, 12), call(1, 13) };
    Pairs pairs;
    RT64::p09OrderedMatch::buildOrderedPairs(cur, prev, pairs, AlignMaxDistance, originDistance(origin));
    const Pairs expected = { { 20, 10 }, { 21, 12 }, { 22, 13 } };
    CHECK(pairs == expected);
    CHECK(pairs.size() == 3);
}

void testAppearedCallAtStartKeepsOldPairs() {
    const Origin origin = { { 30, 50.0f }, { 31, 150.0f }, { 40, 0.0f }, { 41, 50.0f }, { 42, 150.0f } };
    const std::vector<Call> cur = { call(1, 40), call(1, 41), call(1, 42) };
    const std::vector<Call> prev = { call(1, 30), call(1, 31) };
    Pairs pairs;
    RT64::p09OrderedMatch::buildOrderedPairs(cur, prev, pairs, AlignMaxDistance, originDistance(origin));
    const Pairs expected = { { 41, 30 }, { 42, 31 } };
    CHECK(pairs == expected);
}

void testAlignDistanceLimitRejectsFarCalls() {
    const Origin far = { { 50, 0.0f }, { 60, 500.0f } };
    const Origin near = { { 50, 0.0f }, { 60, 50.0f } };
    const std::vector<Call> cur = { call(1, 50) };
    const std::vector<Call> prev = { call(1, 60) };
    const Pairs farPairs = alignedPairs(cur, prev, far, AlignMaxDistance);
    CHECK(farPairs.empty());
    const Pairs nearPairs = alignedPairs(cur, prev, near, AlignMaxDistance);
    const Pairs expected = { { 50, 60 } };
    CHECK(nearPairs == expected);
}

void testAlignWorldCountAndTransformFlagGate() {
    const Origin origin = { { 50, 0.0f }, { 60, 0.0f } };
    const Pairs worldMismatch = alignedPairs({ call(1, 50, 3) }, { call(1, 60, 2) }, origin, AlignMaxDistance);
    CHECK(worldMismatch.empty());
    const Pairs currentDisabled = alignedPairs({ call(1, 50, 1, false) }, { call(1, 60) }, origin, AlignMaxDistance);
    CHECK(currentDisabled.empty());
    const Pairs previousDisabled = alignedPairs({ call(1, 50) }, { call(1, 60, 1, false) }, origin, AlignMaxDistance);
    CHECK(previousDisabled.empty());
}

void testAlignMultiWorldPairsAllOffsets() {
    const Origin origin = { { 100, 0.0f }, { 200, 0.0f } };
    const Pairs pairs = alignedPairs({ call(7, 100, 3) }, { call(7, 200, 3) }, origin, AlignMaxDistance);
    const Pairs expected = { { 100, 200 }, { 101, 201 }, { 102, 202 } };
    CHECK(pairs == expected);
}

void testAlignTieBreakPrefersSmallerDistance() {
    const Origin origin = { { 70, 0.0f }, { 71, 10.0f }, { 72, 2.0f } };
    const Pairs pairs = alignedPairs({ call(1, 70) }, { call(1, 71), call(1, 72) }, origin, AlignMaxDistance);
    const Pairs expected = { { 70, 72 } };
    CHECK(pairs == expected);
}

void testAlignPreservesOrder() {
    // Distance 100 pairs would form two non-crossing alignments; maxDistance 50 leaves only the crossing candidates.
    const Origin origin = { { 80, 0.0f }, { 81, 100.0f }, { 82, 100.0f }, { 83, 0.0f } };
    const Pairs pairs = alignedPairs({ call(1, 80), call(1, 81) }, { call(1, 82), call(1, 83) }, origin, 50.0f);
    CHECK(pairs.size() == 1);
    const bool oneOfCrossingPairs = (pairs == Pairs({ { 80, 83 } })) || (pairs == Pairs({ { 81, 82 } }));
    CHECK(oneOfCrossingPairs);
}

void testOverloadMixesRankAndAlignedGroups() {
    const Origin origin = { { 90, 0.0f }, { 91, 1000.0f }, { 92, 5000.0f }, { 93, 9000.0f }, { 94, 0.0f }, { 95, 50.0f }, { 96, 50.0f }, { 97, 0.0f } };
    const std::vector<Call> cur = { call(1, 90), call(1, 91), call(2, 94), call(2, 95), call(3, 97) };
    const std::vector<Call> prev = { call(1, 92), call(1, 93), call(2, 96) };
    Pairs pairs;
    RT64::p09OrderedMatch::buildOrderedPairs(cur, prev, pairs, AlignMaxDistance, originDistance(origin));
    const Pairs expected = { { 90, 92 }, { 91, 93 }, { 95, 96 } };
    CHECK(pairs == expected);
}

void testCellCapSkipsLargeGroups() {
    const auto zero = [](const Call &, const Call &) { return 0.0f; };
    std::vector<Call> cur;
    std::vector<Call> prev;
    for (uint32_t i = 0; i < 600; i++) {
        cur.push_back(call(1, i));
    }

    for (uint32_t i = 0; i < 601; i++) {
        prev.push_back(call(1, 1000 + i));
    }

    Pairs pairs;
    RT64::p09OrderedMatch::buildOrderedPairs(cur, prev, pairs, AlignMaxDistance, zero);
    CHECK(pairs.empty());

    cur.resize(100);
    prev.resize(101);
    pairs.clear();
    RT64::p09OrderedMatch::buildOrderedPairs(cur, prev, pairs, AlignMaxDistance, zero);
    CHECK(pairs.size() == 100);
}

} // namespace

int main() {
    testEqualCountsPairByRank();
    testUnequalCountsSkipOnlyThatHash();
    testHashMissingFromPrevIsSkipped();
    testMultiWorldCallsExpandWithOffsets();
    testDifferentWorldCountIsSkipped();
    testDoTransformMatchingFalseIsSkipped();
    testEmptyInputs();
    testPairsClearedOnEntry();
    testSwappedInstancesPairByOrderNotPosition();
    testScaleCompatibleBoundaries();
    testScaleCompatibleUsesAbsoluteDeterminant();
    testEnabledCallable();
    testVanishedCallInMiddleAligns();
    testAppearedCallAtStartKeepsOldPairs();
    testAlignDistanceLimitRejectsFarCalls();
    testAlignWorldCountAndTransformFlagGate();
    testAlignMultiWorldPairsAllOffsets();
    testAlignTieBreakPrefersSmallerDistance();
    testAlignPreservesOrder();
    testOverloadMixesRankAndAlignedGroups();
    testCellCapSkipsLargeGroups();
    std::printf("%d/%d passed p09_ordered_match_test\n", g_pass_count, g_test_count);
    return g_pass_count == g_test_count ? 0 : 1;
}
