// Host test for the P09 ordered WORLD matcher in RT64:
//   hle/rt64_p09_ordered_match.h

#include "hle/rt64_p09_ordered_match.h"

#include <cstdio>
#include <set>
#include <utility>
#include <vector>

namespace {

using RT64::p09OrderedMatch::Call;
using Pairs = std::set<std::pair<uint32_t, uint32_t>>;

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
    std::printf("%d/%d passed p09_ordered_match_test\n", g_pass_count, g_test_count);
    return g_pass_count == g_test_count ? 0 : 1;
}
