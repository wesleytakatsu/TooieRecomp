// P09 exact-use pathology correlation: host test of the production helper
// (rt64_p09_exact_use_correlation.h, header-only, no RT64 runtime types).
// Offline CPU only. No GPU, no game, no interpolation change.

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "render/rt64_p09_exact_use_correlation.h"

#ifndef _WIN32
extern "C" FILE *fmemopen(void *buf, size_t size, const char *mode);
#endif

using namespace RT64;

static int g_test_count = 0;

#define TEST_ASSERT(cond, msg) do { \
    ++g_test_count; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL: %s (line %d): %s\n", __FUNCTION__, __LINE__, msg); \
        std::exit(1); \
    } \
} while (0)

static const uint64_t kWl = 77;

static P09SeamExactUseKey makeKey(uint32_t call, uint32_t tri, uint32_t corner, uint32_t world,
    uint64_t workload = kWl, uint32_t faceStart = 300, uint32_t vertex = 10) {
    P09ExactUseKeyInput in;
    in.workloadId = workload;
    in.fbPair = 1;
    in.projection = 2;
    in.gameCall = call;
    in.triangleLocal = tri;
    in.cornerInTriangle = corner;
    in.faceStart = faceStart;
    in.storedVertexId = vertex;
    in.worldIndex = world;
    in.vertexCount = 1000;
    P09SeamExactUseKey key;
    const bool ok = p09ExactUseKeyMake(in, key);
    TEST_ASSERT(ok, "helper key must be valid");
    return key;
}

static P09ExactUsePlanResult makePlan(std::vector<P09SeamExactUseKey> uses, uint64_t workload = kWl,
    bool exhaustive = true, bool active = true) {
    P09ExactUsePlanResult plan;
    std::sort(uses.begin(), uses.end(), p09ExactUseLess);
    uses.erase(std::unique(uses.begin(), uses.end(), p09ExactUseEqual), uses.end());
    plan.uses = uses;
    plan.workloadId = workload;
    plan.active = active ? 1 : 0;
    plan.exhaustive = exhaustive ? 1 : 0;
    plan.prevFrameValid = 1;
    return plan;
}

static void test_key_make() {
    P09ExactUseKeyInput in;
    in.workloadId = kWl;
    in.fbPair = 3;
    in.projection = 4;
    in.gameCall = 5;
    in.triangleLocal = 6;
    in.cornerInTriangle = 2;
    in.faceStart = 100;
    in.storedVertexId = 9;
    in.worldIndex = 65535;
    in.vertexCount = 10;
    P09SeamExactUseKey key;
    TEST_ASSERT(p09ExactUseKeyMake(in, key), "all fields valid");
    TEST_ASSERT(key.faceCornerIndex == 100u + 6u * 3u + 2u, "faceCorner = faceStart + t*3 + c");
    TEST_ASSERT(key.originalWorldIndex == 65535u && key.storedVertexId == 9u, "world and vertex preserved");
    TEST_ASSERT(key.fbPair == 3u && key.projection == 4u && key.gameCall == 5u && key.triangleLocal == 6u,
        "f/pr/d/t preserved");

    P09ExactUseKeyInput bad = in;
    bad.worldIndex = 65536;
    TEST_ASSERT(!p09ExactUseKeyMake(bad, key), "WORLD outside uint16 domain is rejected");
    bad = in; bad.cornerInTriangle = 3;
    TEST_ASSERT(!p09ExactUseKeyMake(bad, key), "corner 3 rejected");
    bad = in; bad.storedVertexId = 10;
    TEST_ASSERT(!p09ExactUseKeyMake(bad, key), "vertex outside vertexCount rejected");
    bad = in; bad.workloadId = 0;
    TEST_ASSERT(!p09ExactUseKeyMake(bad, key), "workload 0 rejected");
    bad = in; bad.faceStart = 0xFFFFFFF0u; bad.triangleLocal = 100;
    TEST_ASSERT(!p09ExactUseKeyMake(bad, key), "faceCorner overflow rejected");
}

static void test_both_one_none() {
    const P09SeamExactUseKey a = makeKey(1, 0, 0, 5);
    const P09SeamExactUseKey b = makeKey(1, 0, 1, 6);
    const P09SeamExactUseKey c = makeKey(1, 4, 0, 7);
    P09ExactUseUnknownReason r;
    TEST_ASSERT(p09ExactUseClassifyPair(makePlan({ a, b }), 1, 1, a, true, b, true, false, &r) == P09PairCoverage::Full,
        "both keys selected is fully covered");
    TEST_ASSERT(p09ExactUseClassifyPair(makePlan({ a }), 1, 1, a, true, b, true, false, &r) == P09PairCoverage::OneSided,
        "one key is one-sided, not covered");
    TEST_ASSERT(p09ExactUseClassifyPair(makePlan({ c }), 1, 1, a, true, b, true, false, &r) == P09PairCoverage::NotCovered,
        "neither key is a definitive miss");
    TEST_ASSERT(p09ExactUseClassifyPair(makePlan({}), 1, 1, a, true, b, true, false, &r) == P09PairCoverage::NotCovered,
        "empty exhaustive plan is a definitive miss");
}

static void test_same_world_distinct_use() {
    const P09SeamExactUseKey planned = makeKey(1, 0, 0, 42);
    const P09ExactUsePlanResult plan = makePlan({ planned });
    const P09SeamExactUseKey diffCall = makeKey(2, 0, 0, 42);
    const P09SeamExactUseKey diffTri = makeKey(1, 1, 0, 42);
    const P09SeamExactUseKey diffCorner = makeKey(1, 0, 1, 42);
    TEST_ASSERT(p09ExactUseClassifyMember(plan, 1, 1, planned, true) == P09ExactUseMember::Selected, "planned key");
    TEST_ASSERT(p09ExactUseClassifyMember(plan, 1, 1, diffCall, true) == P09ExactUseMember::NotSelected,
        "same WORLD different GameCall is not selected");
    TEST_ASSERT(p09ExactUseClassifyMember(plan, 1, 1, diffTri, true) == P09ExactUseMember::NotSelected,
        "same WORLD different triangle is not selected");
    TEST_ASSERT(p09ExactUseClassifyMember(plan, 1, 1, diffCorner, true) == P09ExactUseMember::NotSelected,
        "same WORLD different corner is not selected");
}

static void test_invalid_plans_never_definitive() {
    const P09SeamExactUseKey a = makeKey(1, 0, 0, 5);
    const P09SeamExactUseKey b = makeKey(1, 0, 1, 6);
    P09ExactUseUnknownReason r = P09ExactUseUnknownReason::None;
    TEST_ASSERT(p09ExactUseClassifyPair(makePlan({ a, b }, kWl + 1), 1, 1, a, true, b, true, false, &r) ==
        P09PairCoverage::Ambiguous && r == P09ExactUseUnknownReason::WrongWorkload, "wrong workload is ambiguous");
    TEST_ASSERT(p09ExactUseClassifyPair(makePlan({ a, b }), 1, 2, a, true, b, true, false, &r) ==
        P09PairCoverage::Ambiguous && r == P09ExactUseUnknownReason::WrongGeneration, "wrong generation is ambiguous");
    TEST_ASSERT(p09ExactUseClassifyPair(makePlan({ a, b }, kWl, true, false), 1, 1, a, true, b, true, false, &r) ==
        P09PairCoverage::Ambiguous && r == P09ExactUseUnknownReason::PlanInactive, "inactive plan is ambiguous");
    TEST_ASSERT(p09ExactUseClassifyPair(makePlan({ a, b }, kWl, false), 1, 1, a, true, b, true, false, &r) ==
        P09PairCoverage::Ambiguous && r == P09ExactUseUnknownReason::PlanNotExhaustive,
        "non-exhaustive plan never claims covered or miss");
    TEST_ASSERT(p09ExactUseClassifyPair(makePlan({}, kWl, false), 1, 1, a, true, b, true, false, &r) ==
        P09PairCoverage::Ambiguous, "non-exhaustive empty plan is not a definitive miss");
    TEST_ASSERT(p09ExactUseClassifyPair(P09ExactUsePlanResult(), 1, 1, a, true, b, true, false, &r) ==
        P09PairCoverage::Ambiguous, "default plan is inactive");
}

static void test_tie_incomplete_duplicate() {
    const P09SeamExactUseKey a = makeKey(1, 0, 0, 5);
    const P09SeamExactUseKey b = makeKey(1, 0, 1, 6);
    const P09ExactUsePlanResult plan = makePlan({ a, b });
    P09ExactUseUnknownReason r = P09ExactUseUnknownReason::None;
    TEST_ASSERT(p09ExactUseClassifyPair(plan, 1, 1, a, true, b, true, true, &r) == P09PairCoverage::Ambiguous &&
        r == P09ExactUseUnknownReason::Tie, "tie is ambiguous even if both keys are planned");
    TEST_ASSERT(p09ExactUseClassifyPair(plan, 1, 1, a, true, b, false, false, &r) == P09PairCoverage::Ambiguous &&
        r == P09ExactUseUnknownReason::KeyIncomplete, "incomplete key is ambiguous");
    TEST_ASSERT(p09ExactUseClassifyPair(plan, 1, 1, a, true, a, true, false, &r) == P09PairCoverage::Ambiguous &&
        r == P09ExactUseUnknownReason::DuplicateMember, "identical members are ambiguous");
}

static void test_units_and_exposure() {
    const P09SeamExactUseKey a = makeKey(1, 0, 0, 5);
    const P09SeamExactUseKey b = makeKey(1, 0, 1, 6);
    const P09SeamExactUseKey bVariant = makeKey(1, 0, 2, 6); // same WORLD, different corner
    const P09SeamExactUseKey n1 = makeKey(9, 3, 0, 8);
    const P09SeamExactUseKey n2 = makeKey(9, 3, 1, 8);
    const P09ExactUsePlanResult plan = makePlan({ a, b, n1 });
    P09ExactUseCorrelation c;
    // Two pathological pairs share member a: occurrences 4, unique 3 (a, b, bVariant).
    p09ExactUseNotePair(c, plan, 1, 1, a, true, b, true, false, true);
    p09ExactUseNotePair(c, plan, 1, 1, a, true, bVariant, true, false, true);
    // One nonpath pair: n1 selected, n2 not selected.
    p09ExactUseNotePair(c, plan, 1, 1, n1, true, n2, true, false, false);
    // One ambiguous pathological pair (tie): keys still counted as occurrences.
    p09ExactUseNotePair(c, plan, 1, 1, b, true, n1, true, true, true);

    TEST_ASSERT(c.pathPairs == 3u, "three pathological pairs");
    TEST_ASSERT(c.pathFull == 1u && c.pathOneSided == 1u && c.pathNotCovered == 0u && c.pathAmbiguous == 1u,
        "pair outcomes: full, one-sided, ambiguous");
    TEST_ASSERT(c.ambiguousByReason[size_t(P09ExactUseUnknownReason::Tie)] == 1u, "tie reason counted");
    TEST_ASSERT(c.pathOccurrences == 6u, "occurrences are 2 per pair, repeats counted");
    const P09ExactUseUniqueSummary u = p09ExactUseSummarize(c);
    TEST_ASSERT(u.pathKeys == 4u, "unique path keys: a, b, bVariant, n1 (not twice pair count)");
    TEST_ASSERT(c.pathOccurrences != u.pathKeys, "occurrence and unique units differ");
    TEST_ASSERT(c.nonPathPairs == 1u && c.nonPathOccurrences == 2u, "nonpath pair/occurrence units");
    TEST_ASSERT(c.nonPathOccurrencesSelected == 1u && c.nonPathOccurrencesNotSelected == 1u,
        "nonpath selected exposure only inside observed universe");
    TEST_ASSERT(u.nonPathKeys == 2u && u.nonPathKeysSelected == 1u && u.nonPathKeysNotSelected == 1u,
        "nonpath unique keys");
    TEST_ASSERT(u.keysOnBoth == 1u, "n1 is both path and nonpath");

    bool valid = false;
    const double rate = p09ExactUsePairCoverageRate(c, valid);
    TEST_ASSERT(valid && rate == 0.5, "definitive rate = full / (full+one-sided+not-covered), ambiguous excluded");

    P09ExactUseCorrelation unknown;
    p09ExactUseNotePair(unknown, P09ExactUsePlanResult(), 1, 1, n1, true, n2, true, false, false);
    TEST_ASSERT(unknown.nonPathOccurrencesUnknown == 2u && unknown.nonPathOccurrencesSelected == 0u,
        "inactive plan leaves nonpath members unclassified");
    TEST_ASSERT(p09ExactUsePairCoverageRate(unknown, valid) == 0.0 && !valid, "no definitive pairs gives no rate");
}

static void test_budget_and_default_off() {
    TEST_ASSERT(!p09CorrelationFlagTruthy(nullptr), "unset flag is OFF");
    TEST_ASSERT(!p09CorrelationFlagTruthy(""), "empty flag is OFF");
    TEST_ASSERT(!p09CorrelationFlagTruthy("0"), "0 is OFF");
    TEST_ASSERT(p09CorrelationFlagTruthy("1"), "1 is ON");

    P09CorrelationEligibilityInput in;
    TEST_ASSERT(p09CorrelationEligibility(in) == P09CorrelationSkip::NoWorkload, "no workload first");
    in.hasWorkload = true;
    TEST_ASSERT(p09CorrelationEligibility(in) == P09CorrelationSkip::NoGeometry, "empty boot workload");
    in.hasGeometry = true;
    TEST_ASSERT(p09CorrelationEligibility(in) == P09CorrelationSkip::NoFrameMatch, "unmatched frame");
    in.frameMatched = true;
    TEST_ASSERT(p09CorrelationEligibility(in) == P09CorrelationSkip::NotIntermediate, "not intermediate");
    in.intermediate = true;
    TEST_ASSERT(p09CorrelationEligibility(in) == P09CorrelationSkip::NoModelBin, "no model bin");
    in.hasModelBin = true;
    TEST_ASSERT(p09CorrelationEligibility(in) == P09CorrelationSkip::NoTargetCall, "no target call");
    in.hasTargetCall = true;
    TEST_ASSERT(p09CorrelationEligibility(in) == P09CorrelationSkip::None, "eligible");

    P09CorrelationBudget b;
    b.maxAttempts = 3;
    b.cadence = 2;
    b.maxSeamSearches = 1;
    for (int i = 0; i < 1000; i++) {
        TEST_ASSERT(p09CorrelationDecide(b, P09CorrelationSkip::NoGeometry) == P09CorrelationDecision::SkipIneligible,
            "empty boot dispatch skipped");
    }
    TEST_ASSERT(b.attempts == 0 && b.eligibleSeen == 0, "empty boot never consumes budget or cadence");
    TEST_ASSERT(p09CorrelationDecide(b, P09CorrelationSkip::None) == P09CorrelationDecision::Collect, "first eligible collects");
    TEST_ASSERT(p09CorrelationDecide(b, P09CorrelationSkip::None) == P09CorrelationDecision::SkipCadence, "cadence skip");
    TEST_ASSERT(p09CorrelationDecide(b, P09CorrelationSkip::None) == P09CorrelationDecision::Collect, "second collect");
    TEST_ASSERT(p09CorrelationDecide(b, P09CorrelationSkip::None) == P09CorrelationDecision::SkipCadence, "cadence skip 2");
    TEST_ASSERT(p09CorrelationDecide(b, P09CorrelationSkip::None) == P09CorrelationDecision::Collect, "third collect");
    TEST_ASSERT(p09CorrelationBudgetExhausted(b), "budget exhausted after maxAttempts");
    TEST_ASSERT(p09CorrelationDecide(b, P09CorrelationSkip::None) == P09CorrelationDecision::SkipBudgetExhausted,
        "no collection after exhaustion");
    TEST_ASSERT(b.skippedBudget == 1u, "exhaustion counted");

    P09CorrelationBudget s;
    s.maxSeamSearches = 2;
    TEST_ASSERT(p09CorrelationSeamSearchAllowed(s), "seam search allowed");
    s.seamSearches = 2;
    TEST_ASSERT(!p09CorrelationSeamSearchAllowed(s), "seam search bounded");

    P09CorrelationBudget d;
    TEST_ASSERT(d.maxAttempts == 24u && d.cadence == 4u && d.maxSeamSearches == 8u, "documented default limits");
    TEST_ASSERT(kP09CorrelationAttemptIntervalMs == 2000u, "heavy attempts are spaced by 2s");
}

static void test_unique_and_pending_caps() {
    P09ExactUseCorrelation table;
    uint64_t made = 0;
    for (uint32_t i = 0; i < uint32_t(kP09ExactUseMaxUniqueKeys); i++) {
        P09ExactUseKeyInput in;
        in.workloadId = 1;
        in.fbPair = 1;
        in.projection = 1;
        in.gameCall = 1;
        in.triangleLocal = i;
        in.cornerInTriangle = 0;
        in.faceStart = 0;
        in.storedVertexId = 1;
        in.worldIndex = 1;
        in.vertexCount = 10;
        P09SeamExactUseKey key;
        if (p09ExactUseKeyMake(in, key)) made++;
        p09ExactUseNoteMember(table, key, true, P09ExactUseMember::Selected, true);
    }
    TEST_ASSERT(made == uint64_t(kP09ExactUseMaxUniqueKeys), "cap keys are complete");
    TEST_ASSERT(table.keys.size() == kP09ExactUseMaxUniqueKeys, "unique table fills to the cap");
    TEST_ASSERT(table.uniqueTableOverflow == 0u, "filling the cap is not an overflow");

    P09ExactUseKeyInput again;
    again.workloadId = 1;
    again.fbPair = 1;
    again.projection = 1;
    again.gameCall = 1;
    again.triangleLocal = 0;
    again.cornerInTriangle = 0;
    again.faceStart = 0;
    again.storedVertexId = 1;
    again.worldIndex = 1;
    again.vertexCount = 10;
    P09SeamExactUseKey existing;
    TEST_ASSERT(p09ExactUseKeyMake(again, existing), "existing key rebuilds");
    const uint8_t beforeBits = table.keys.find(existing)->second;
    p09ExactUseNoteMember(table, existing, true, P09ExactUseMember::NotSelected, false);
    TEST_ASSERT(table.keys.size() == kP09ExactUseMaxUniqueKeys, "re-note does not grow the table");
    TEST_ASSERT(table.uniqueTableOverflow == 0u, "re-note does not drop");
    const uint8_t afterBits = table.keys.find(existing)->second;
    TEST_ASSERT((afterBits & beforeBits) == beforeBits, "re-note keeps the stored key bits");
    TEST_ASSERT(afterBits != beforeBits, "re-note ORs the new membership");

    P09ExactUseKeyInput extraIn = again;
    extraIn.triangleLocal = uint32_t(kP09ExactUseMaxUniqueKeys);
    P09SeamExactUseKey extra;
    TEST_ASSERT(p09ExactUseKeyMake(extraIn, extra), "overflow key is complete");
    p09ExactUseNoteMember(table, extra, true, P09ExactUseMember::Selected, true);
    TEST_ASSERT(table.keys.size() == kP09ExactUseMaxUniqueKeys, "overflow does not insert");
    TEST_ASSERT(table.uniqueTableOverflow == 1u, "overflow is visible");
    TEST_ASSERT(table.keys.find(extra) == table.keys.end(), "dropped key is absent");
    TEST_ASSERT(table.keys.find(existing) != table.keys.end(), "an existing key is not overwritten");

    std::vector<uint32_t> pending;
    uint64_t dropped = 0;
    uint64_t accepted = 0;
    for (uint32_t i = 0; i < uint32_t(kP09ExactUseMaxPendingPairs); i++) {
        if (p09ExactUseAppendCapped(pending, kP09ExactUseMaxPendingPairs, dropped, i)) accepted++;
    }
    TEST_ASSERT(accepted == uint64_t(kP09ExactUseMaxPendingPairs) && dropped == 0u, "pending fill accepts every slot");
    TEST_ASSERT(pending.size() == kP09ExactUseMaxPendingPairs && pending.front() == 0u, "first pending pair kept");
    const uint32_t rejected = 0xFFFFFFFFu;
    TEST_ASSERT(!p09ExactUseAppendCapped(pending, kP09ExactUseMaxPendingPairs, dropped, rejected), "cap rejects");
    TEST_ASSERT(!p09ExactUseAppendCapped(pending, kP09ExactUseMaxPendingPairs, dropped, rejected), "cap rejects again");
    TEST_ASSERT(pending.size() == kP09ExactUseMaxPendingPairs, "reject does not grow or overwrite");
    TEST_ASSERT(dropped == 2u, "drops are counted");
    TEST_ASSERT(pending.front() == 0u, "first entry unchanged");
    TEST_ASSERT(pending.back() == uint32_t(kP09ExactUseMaxPendingPairs - 1u), "last stored entry is the last accepted pair");
    TEST_ASSERT(p09RecordedCohortTruncated(dropped, 0, table.uniqueTableOverflow, 0, 0), "drops and overflow truncate the recorded cohort");
}

static void test_arm_wall_clock_and_all_observed() {
    P09CaptureControl ctl;
    TEST_ASSERT(ctl.phase == P09CapturePhase::WaitingForRequest, "capture starts waiting");
    TEST_ASSERT(p09CaptureObserveMarker(ctl, true, false) == P09ArmEvent::None, "failed stale removal does not commit");
    TEST_ASSERT(ctl.staleIgnored == 0u && ctl.consumeFailures == 1u && ctl.consumeFailedLatched && !ctl.observedMarkerAbsent,
        "failed stale removal latches once and preserves waiting");
    TEST_ASSERT(p09CaptureObserveMarker(ctl, true, false) == P09ArmEvent::None && ctl.consumeFailures == 1u && ctl.staleIgnored == 0u,
        "persistent stale marker does not count again");
    TEST_ASSERT(p09CaptureObserveMarker(ctl, true, true) == P09ArmEvent::StaleIgnored, "leftover marker is stale only after removal");
    TEST_ASSERT(ctl.phase == P09CapturePhase::WaitingForRequest && ctl.staleIgnored == 1u && !ctl.observedMarkerAbsent && !ctl.consumeFailedLatched,
        "stale marker does not arm");
    TEST_ASSERT(p09CaptureObserveMarker(ctl, false, false) == P09ArmEvent::None && ctl.observedMarkerAbsent, "absence is observed");
    TEST_ASSERT(p09CaptureObserveMarker(ctl, true, false) == P09ArmEvent::None && ctl.phase == P09CapturePhase::WaitingForRequest && ctl.arms == 0u,
        "failed arm removal does not start capture");
    TEST_ASSERT(ctl.consumeFailures == 2u && ctl.consumeFailedLatched, "arm removal failure counted once");
    TEST_ASSERT(p09CaptureObserveMarker(ctl, true, false) == P09ArmEvent::None && ctl.consumeFailures == 2u && ctl.arms == 0u,
        "persistent arm marker does not rearm or recount");
    TEST_ASSERT(p09CaptureObserveMarker(ctl, true, true) == P09ArmEvent::Armed && ctl.phase == P09CapturePhase::Capturing && ctl.arms == 1u,
        "marker after absence arms once when removal succeeds");
    TEST_ASSERT(!ctl.consumeFailedLatched && ctl.consumeFailures == 2u, "successful arm clears the latch and keeps the failure count");
    TEST_ASSERT(p09CaptureObserveMarker(ctl, true, false) == P09ArmEvent::None && ctl.rearms == 0u && ctl.phase == P09CapturePhase::Capturing,
        "failed rearm removal preserves capturing");
    TEST_ASSERT(p09CaptureObserveMarker(ctl, true, false) == P09ArmEvent::None && ctl.consumeFailures == 3u && ctl.rearms == 0u,
        "persistent rearm marker does not reset");
    TEST_ASSERT(p09CaptureObserveMarker(ctl, true, true) == P09ArmEvent::Rearmed && ctl.rearms == 1u && ctl.phase == P09CapturePhase::Capturing,
        "marker during capture re-arms only after removal");
    ctl.phase = P09CapturePhase::Exhausted;
    const uint64_t failuresBeforeExhaustRearm = ctl.consumeFailures;
    TEST_ASSERT(p09CaptureObserveMarker(ctl, true, false) == P09ArmEvent::None && ctl.phase == P09CapturePhase::Exhausted && ctl.rearms == 1u,
        "failed removal after exhaustion does not restart capture");
    TEST_ASSERT(p09CaptureObserveMarker(ctl, true, false) == P09ArmEvent::None && ctl.consumeFailures == failuresBeforeExhaustRearm + 1u,
        "persistent post-exhaustion marker stays latched");
    TEST_ASSERT(p09CaptureObserveMarker(ctl, true, true) == P09ArmEvent::Rearmed && ctl.phase == P09CapturePhase::Capturing && ctl.rearms == 2u,
        "marker after exhaustion re-arms into capturing when removal succeeds");
    TEST_ASSERT(p09CaptureObserveMarker(ctl, false, false) == P09ArmEvent::None && ctl.phase == P09CapturePhase::Capturing && !ctl.consumeFailedLatched,
        "absence while capturing does not change phase and clears a latch");

    // RSP p09CorrResetCorrelationOnly runs only when this event is Rearmed.
    // That session wipe is not linked into this host test.
    P09CorrelationBudget rearmedBudget;
    rearmedBudget.cadence = 1;
    uint64_t rearmT = 50;
    TEST_ASSERT(p09CorrelationPreWalk(P09CapturePhase::Capturing, rearmedBudget, &rearmT) == P09PreWalk::Walk,
        "first pre-walk after a rearm boundary is a walk");
    TEST_ASSERT(rearmedBudget.attempts == 0u && rearmedBudget.eligibleSeen == 0u && rearmedBudget.skippedWallClock == 0u,
        "rearm-boundary walk consumes no attempt before decide");
    TEST_ASSERT(p09CorrelationDecide(rearmedBudget, P09CorrelationSkip::None, &rearmT) == P09CorrelationDecision::Collect,
        "first eligible attempt after arm collects");
    TEST_ASSERT(rearmedBudget.attempts == 1u && rearmedBudget.eligibleSeen == 1u && rearmedBudget.lastHeavyAttemptMs == rearmT,
        "first collect records the fake timestamp");

    P09CorrelationBudget fresh;
    TEST_ASSERT(!p09CorrelationCostlyWorkAllowed(P09CapturePhase::WaitingForRequest, fresh), "waiting does no costly work");
    TEST_ASSERT(!p09CorrelationCostlyWorkAllowed(P09CapturePhase::Exhausted, fresh), "exhausted phase does no costly work");
    TEST_ASSERT(p09CorrelationCostlyWorkAllowed(P09CapturePhase::Capturing, fresh), "capturing with budget allows work");
    TEST_ASSERT(p09CaptureRunIncomplete(P09CapturePhase::WaitingForRequest, fresh, 0), "waiting is an incomplete run");
    TEST_ASSERT(p09CaptureRunIncomplete(P09CapturePhase::Exhausted, fresh, 0), "exhausted is an incomplete run");
    P09CorrelationBudget sampled;
    sampled.cadence = 4;
    TEST_ASSERT(p09CaptureRunIncomplete(P09CapturePhase::Capturing, sampled, 0), "cadence sampling is an incomplete run");
    sampled.cadence = 1;
    TEST_ASSERT(!p09CaptureRunIncomplete(P09CapturePhase::Capturing, sampled, 0), "unsampled capturing run is not yet incomplete");
    TEST_ASSERT(p09CaptureRunIncomplete(P09CapturePhase::Capturing, sampled, 1), "skipped seam search makes the run incomplete");

    P09CorrelationBudget pre;
    pre.cadence = 1;
    uint64_t preT = 1000;
    TEST_ASSERT(p09CorrelationPreWalk(P09CapturePhase::WaitingForRequest, pre, &preT) == P09PreWalk::Waiting,
        "waiting pre-walk does not scan");
    TEST_ASSERT(pre.attempts == 0u && pre.eligibleSeen == 0u && pre.skippedWallClock == 0u && pre.skippedBudget == 0u,
        "waiting pre-walk changes no budget");
    TEST_ASSERT(p09CorrelationPreWalk(P09CapturePhase::Capturing, pre, &preT) == P09PreWalk::Walk,
        "first capturing pre-walk is eligible to walk");
    TEST_ASSERT(pre.skippedWallClock == 0u && pre.attempts == 0u, "the walk decision itself consumes nothing");
    TEST_ASSERT(p09CorrelationDecide(pre, P09CorrelationSkip::None, &preT) == P09CorrelationDecision::Collect,
        "decide collects only after the pre-walk allows it");
    const uint64_t preAttempts = pre.attempts;
    const uint64_t preEligible = pre.eligibleSeen;
    const uint64_t preIneligible = pre.skippedIneligible[size_t(P09CorrelationSkip::NoGeometry)];
    TEST_ASSERT(p09CorrelationPreWalk(P09CapturePhase::Capturing, pre, &preT) == P09PreWalk::Cooldown,
        "cooldown is decided before an eligibility walk");
    TEST_ASSERT(pre.skippedWallClock == 1u && pre.attempts == preAttempts && pre.eligibleSeen == preEligible,
        "one cooldown skip consumes neither attempt nor eligible slot");
    TEST_ASSERT(pre.skippedIneligible[size_t(P09CorrelationSkip::NoGeometry)] == preIneligible,
        "cooldown is not an eligibility result");
    TEST_ASSERT(p09CorrelationPreWalk(P09CapturePhase::Capturing, pre, &preT) == P09PreWalk::Cooldown && pre.skippedWallClock == 2u,
        "each cooled dispatch counts once and does not call decide");
    TEST_ASSERT(pre.attempts == preAttempts && pre.eligibleSeen == preEligible, "second cooldown still consumes nothing");
    preT = 1000 + kP09CorrelationAttemptIntervalMs;
    TEST_ASSERT(p09CorrelationPreWalk(P09CapturePhase::Capturing, pre, &preT) == P09PreWalk::Walk && pre.skippedWallClock == 2u,
        "interval boundary is a walk and does not add a cooldown skip");
    pre.attempts = pre.maxAttempts;
    const uint64_t budgetSkips = pre.skippedBudget;
    TEST_ASSERT(p09CorrelationPreWalk(P09CapturePhase::Capturing, pre, &preT) == P09PreWalk::Exhausted,
        "exhausted pre-walk does not scan eligibility");
    TEST_ASSERT(pre.skippedIneligible[size_t(P09CorrelationSkip::NoGeometry)] == preIneligible && pre.skippedBudget == budgetSkips,
        "post-exhaustion ineligible is not an eligibility scan and does not take attempt budget");
    TEST_ASSERT(pre.eligibleSeen == preEligible && pre.attempts == pre.maxAttempts, "exhausted pre-walk leaves the counters");
    TEST_ASSERT(p09CorrelationPreWalk(P09CapturePhase::Exhausted, pre, &preT) == P09PreWalk::Exhausted,
        "exhausted phase stays a pre-walk stop");

    P09CorrelationBudget wall;
    wall.cadence = 1;
    wall.maxAttempts = 24;
    uint64_t t = 1000;
    TEST_ASSERT(p09CorrelationDecide(wall, P09CorrelationSkip::None, &t) == P09CorrelationDecision::Collect, "first heavy collect");
    TEST_ASSERT(wall.attempts == 1u && wall.eligibleSeen == 1u && wall.lastHeavyAttemptMs == t, "first collect records time");
    TEST_ASSERT(p09CorrelationDecide(wall, P09CorrelationSkip::None, &t) == P09CorrelationDecision::SkipWallClock, "same timestamp waits");
    TEST_ASSERT(wall.attempts == 1u && wall.eligibleSeen == 1u && wall.skippedWallClock == 1u, "wall wait consumes neither attempt nor eligible slot");
    TEST_ASSERT(p09CorrelationDecide(wall, P09CorrelationSkip::NoGeometry, &t) == P09CorrelationDecision::SkipIneligible,
        "empty workload during the wait is ineligible");
    TEST_ASSERT(wall.attempts == 1u && wall.eligibleSeen == 1u, "ineligible does not consume the heavy slot");
    TEST_ASSERT(wall.skippedIneligible[size_t(P09CorrelationSkip::NoGeometry)] == 1u, "ineligible reason counted");
    t = 1000 + kP09CorrelationAttemptIntervalMs - 1u;
    TEST_ASSERT(p09CorrelationDecide(wall, P09CorrelationSkip::None, &t) == P09CorrelationDecision::SkipWallClock, "interval not elapsed");
    TEST_ASSERT(wall.eligibleSeen == 1u && wall.attempts == 1u, "early timestamp still holds the slot");
    t = 1000 + kP09CorrelationAttemptIntervalMs;
    TEST_ASSERT(p09CorrelationDecide(wall, P09CorrelationSkip::None, &t) == P09CorrelationDecision::Collect, "interval elapsed collects");
    for (uint32_t i = 2; i < wall.maxAttempts; i++) {
        t += kP09CorrelationAttemptIntervalMs;
        TEST_ASSERT(p09CorrelationDecide(wall, P09CorrelationSkip::None, &t) == P09CorrelationDecision::Collect, "spaced collect");
    }
    TEST_ASSERT(wall.attempts == 24u && p09CorrelationBudgetExhausted(wall), "attempt cap reached");
    TEST_ASSERT(!p09CorrelationCostlyWorkAllowed(P09CapturePhase::Capturing, wall), "attempt cap stops costly work");
    const uint64_t geometrySkips = wall.skippedIneligible[size_t(P09CorrelationSkip::NoGeometry)];
    TEST_ASSERT(p09CorrelationDecide(wall, P09CorrelationSkip::NoGeometry, &t) == P09CorrelationDecision::SkipBudgetExhausted,
        "exhaustion is not an eligibility result");
    TEST_ASSERT(wall.skippedIneligible[size_t(P09CorrelationSkip::NoGeometry)] == geometrySkips, "post-exhaustion does not count as ineligible");
    TEST_ASSERT(wall.skippedBudget == 1u, "post-exhaustion budget skip counted");
    TEST_ASSERT(p09CaptureRunIncomplete(P09CapturePhase::Capturing, wall, 0), "attempt cap makes the run incomplete");

    P09CorrelationBudget seam;
    seam.cadence = 1;
    seam.seamSearches = seam.maxSeamSearches;
    TEST_ASSERT(p09CorrelationBudgetExhausted(seam), "seam-search cap exhausts the budget");
    TEST_ASSERT(!p09CorrelationCostlyWorkAllowed(P09CapturePhase::Capturing, seam), "seam cap stops costly work");
    TEST_ASSERT(p09CorrelationDecide(seam, P09CorrelationSkip::None) == P09CorrelationDecision::SkipBudgetExhausted,
        "seam cap does not start another classification");
    TEST_ASSERT(seam.attempts == 0u && seam.eligibleSeen == 0u && seam.skippedBudget == 1u, "seam cap consumes no attempt");

    P09CorrelationBudget paced;
    paced.cadence = 4;
    paced.maxAttempts = 3;
    uint64_t t2 = 5000;
    TEST_ASSERT(p09CorrelationDecide(paced, P09CorrelationSkip::None, &t2) == P09CorrelationDecision::Collect, "cadence phase 0 collects");
    TEST_ASSERT(p09CorrelationDecide(paced, P09CorrelationSkip::None, &t2) == P09CorrelationDecision::SkipCadence, "cadence skip");
    TEST_ASSERT(p09CorrelationDecide(paced, P09CorrelationSkip::None, &t2) == P09CorrelationDecision::SkipCadence, "cadence skip");
    TEST_ASSERT(p09CorrelationDecide(paced, P09CorrelationSkip::None, &t2) == P09CorrelationDecision::SkipCadence, "cadence skip");
    TEST_ASSERT(p09CorrelationDecide(paced, P09CorrelationSkip::None, &t2) == P09CorrelationDecision::SkipWallClock, "next heavy slot waits");
    TEST_ASSERT(paced.eligibleSeen == 4u && paced.attempts == 1u && paced.skippedWallClock == 1u, "wall wait does not advance the cadence index");
    TEST_ASSERT(p09CorrelationDecide(paced, P09CorrelationSkip::None, &t2) == P09CorrelationDecision::SkipWallClock, "same heavy slot retries");
    TEST_ASSERT(paced.eligibleSeen == 4u, "retry stays on the same slot");
    t2 += kP09CorrelationAttemptIntervalMs;
    TEST_ASSERT(p09CorrelationDecide(paced, P09CorrelationSkip::None, &t2) == P09CorrelationDecision::Collect, "same slot collects after the interval");
    TEST_ASSERT(paced.attempts == 2u && paced.eligibleSeen == 5u, "slot consumed once");

    P09ExactUseCorrelation metric;
    metric.pathFull = 1;
    metric.pathAmbiguous = 1;
    bool valid = false;
    const double definitive = p09ExactUsePairCoverageRate(metric, valid);
    TEST_ASSERT(valid && definitive == 1.0, "definitive ratio can be 1 while ambiguous pairs remain");
    const double allObserved = p09ExactUsePairFullOfAllObserved(metric, valid);
    TEST_ASSERT(valid && allObserved == 0.5, "all-observed fraction keeps ambiguous pairs in the denominator");
    TEST_ASSERT(p09ExactUseAllObservedPathPairs(metric) == 2u, "all-observed total includes ambiguous");
    TEST_ASSERT(!p09ObservedCohortReadyForReview(metric, false), "ambiguous pairs block cohort review");
    metric.pathAmbiguous = 0;
    metric.pathOneSided = 1;
    TEST_ASSERT(!p09ObservedCohortReadyForReview(metric, false), "one-sided pairs block cohort review");
    metric.pathOneSided = 0;
    metric.pathNotCovered = 1;
    TEST_ASSERT(!p09ObservedCohortReadyForReview(metric, false), "not-covered pairs block cohort review");
    metric.pathNotCovered = 0;
    TEST_ASSERT(p09ObservedCohortReadyForReview(metric, false), "all observed pairs full and untruncated");
    TEST_ASSERT(!p09ObservedCohortReadyForReview(metric, true), "recorded truncation blocks cohort review");
    P09ExactUseCorrelation none;
    TEST_ASSERT(!p09ObservedCohortReadyForReview(none, false), "zero observed pairs are not ready");
    TEST_ASSERT(p09ExactUsePairFullOfAllObserved(none, valid) == 0.0 && !valid, "empty all-observed fraction is not valid");
    TEST_ASSERT(!p09RecordedCohortTruncated(0, 0, 0, 0, 0), "clean counters are not recorded truncation");
    TEST_ASSERT(p09RecordedCohortTruncated(1, 0, 0, 0, 0), "pending drops truncate");
    TEST_ASSERT(p09RecordedCohortTruncated(0, 1, 0, 0, 0), "vertex drops truncate");
    TEST_ASSERT(p09RecordedCohortTruncated(0, 0, 1, 0, 0), "unique overflow truncates");
    TEST_ASSERT(p09RecordedCohortTruncated(0, 0, 0, 1, 0), "non-exhaustive plan truncates");
    TEST_ASSERT(p09RecordedCohortTruncated(0, 0, 0, 0, 1), "inactive plan truncates");

    TEST_ASSERT(!p09CorrelationFlagTruthy("false"), "false is OFF");
    TEST_ASSERT(!p09CorrelationFlagTruthy("FALSE"), "FALSE is OFF");
    TEST_ASSERT(!p09CorrelationFlagTruthy("no"), "no is OFF");
    TEST_ASSERT(!p09CorrelationFlagTruthy("NO"), "NO is OFF");
    TEST_ASSERT(!p09CorrelationFlagTruthy("off"), "off is OFF");
    TEST_ASSERT(!p09CorrelationFlagTruthy("OFF"), "OFF is OFF");
    TEST_ASSERT(!p09CorrelationFlagTruthy("maybe"), "unrecognized is OFF");
    TEST_ASSERT(!p09CorrelationFlagTruthy("2"), "2 is OFF");
    TEST_ASSERT(!p09CorrelationFlagTruthy("true "), "trailing space is OFF");
    TEST_ASSERT(!p09CorrelationFlagTruthy(" true"), "leading space is OFF");
    TEST_ASSERT(p09CorrelationFlagTruthy("true"), "true is ON");
    TEST_ASSERT(p09CorrelationFlagTruthy("TRUE"), "TRUE is ON");
    TEST_ASSERT(p09CorrelationFlagTruthy("Yes"), "Yes is ON");
    TEST_ASSERT(p09CorrelationFlagTruthy("yes"), "yes is ON");
    TEST_ASSERT(p09CorrelationFlagTruthy("on"), "on is ON");
    TEST_ASSERT(p09CorrelationFlagTruthy("ON"), "ON is ON");
}

static P09TieMemberInput tieMember(const P09SeamExactUseKey &key, bool complete, bool selected) {
    P09TieMemberInput m;
    m.key = key;
    m.keyComplete = complete;
    m.selected = selected;
    m.gap = 1.25f;
    m.gapEqualsBest = true;
    m.prevNdcX = -0.25f;
    m.prevNdcY = 0.5f;
    m.curNdcX = 0.75f;
    m.curNdcY = -0.125f;
    m.prevNdcOk = true;
    m.curNdcOk = true;
    m.prevWorldX = 9.0f;
    m.prevWorldOk = false;
    m.curWorldX = 4.0f;
    m.curWorldY = 5.0f;
    m.curWorldZ = 6.0f;
    m.curWorldOk = true;
    return m;
}

static void test_pathological_tie_groups() {
    TEST_ASSERT(kP09TieGroupCap == 20, "group cap is 20");
    TEST_ASSERT(kP09TieMemberCap == 96, "member cap is 96");

    P09ExactUseCorrelation corr;
    const P09SeamExactUseKey notedA = makeKey(3, 1, 0, 4);
    const P09SeamExactUseKey notedB = makeKey(3, 1, 1, 5);
    p09ExactUseNotePair(corr, P09ExactUsePlanResult{}, 1, 1, notedA, true, notedB, true, true, true);
    const uint64_t pathPairsBefore = corr.pathPairs;
    const uint64_t ambiguousBefore = corr.pathAmbiguous;
    const size_t keysBefore = corr.keys.size();

    const P09SeamExactUseKey repeated = makeKey(8, 2, 0, 40);
    P09TieSample sample;
    for (uint32_t i = 0; i < 21u; i++) {
        P09TieMemberInput mem[2];
        mem[0] = tieMember(repeated, true, true);
        mem[1] = tieMember(repeated, true, false);
        mem[0].prevNdcX = -0.25f + float(i);
        P09TieGroupInput in;
        in.child = makeKey(1, 0, 0, i + 1u);
        in.childComplete = true;
        in.bestGap = 1.25f;
        in.multiplicity = 2;
        in.attemptGeneration = 11;
        in.members = mem;
        in.memberCount = 2;
        p09TieSampleRecord(sample, in);
    }
    TEST_ASSERT(corr.pathPairs == pathPairsBefore, "tie record does not change path pairs");
    TEST_ASSERT(corr.pathAmbiguous == ambiguousBefore, "tie record does not change ambiguous count");
    TEST_ASSERT(corr.keys.size() == keysBefore, "tie record does not change the key table");
    TEST_ASSERT(sample.storedGroups == 20u, "first 20 groups are stored");
    TEST_ASSERT(sample.skippedGroups == 1u, "group 21 is skipped");
    TEST_ASSERT(sample.observedPathologicalTieGroups == 21u, "observed count includes the skip");
    TEST_ASSERT(sample.groups[0].child.originalWorldIndex == 1u, "first group is kept");
    TEST_ASSERT(sample.groups[19].child.originalWorldIndex == 20u, "twentieth group is the last stored");
    TEST_ASSERT(sample.groups[0].identity == P09TieIdentity::SameExactUseRepeat, "equal complete keys repeat");
    TEST_ASSERT(std::strcmp(p09TieIdentityName(sample.groups[0].identity), "SAME_EXACT_USE_REPEAT") == 0,
        "repeat name");
    TEST_ASSERT(sample.groups[0].selectedStoredIndex == 0, "selected parent index is explicit");
    TEST_ASSERT(sample.groups[0].members[0].selected, "first candidate is the selected parent");
    TEST_ASSERT(!sample.groups[0].members[1].selected, "tied candidate is not selected");
    TEST_ASSERT(sample.groups[0].members[0].prevNdcX == -0.25f, "prev NDC X preserved");
    TEST_ASSERT(sample.groups[0].members[0].prevNdcY == 0.5f, "prev NDC Y preserved");
    TEST_ASSERT(sample.groups[0].members[0].curNdcX == 0.75f, "current NDC X preserved");
    TEST_ASSERT(sample.groups[0].members[0].curNdcY == -0.125f, "current NDC Y preserved");
    TEST_ASSERT(sample.groups[0].members[0].prevNdcOk && sample.groups[0].members[0].curNdcOk, "NDC flags preserved");
    TEST_ASSERT(sample.groups[0].members[0].gap == 1.25f, "member gap preserved");
    TEST_ASSERT(sample.groups[0].members[0].gapEqualsBest, "exact gap equality preserved");
    TEST_ASSERT(sample.groups[0].bestGap == 1.25f, "best gap preserved");
    TEST_ASSERT(!sample.groups[0].members[0].prevWorldOk, "unavailable previous world stays unavailable");
    TEST_ASSERT(sample.groups[0].members[0].prevWorldX == 0.0f, "unavailable previous world is not published");
    TEST_ASSERT(sample.groups[0].members[0].curWorldOk, "available current world is kept");
    TEST_ASSERT(sample.groups[0].members[0].curWorldX == 4.0f, "current world X preserved");
    TEST_ASSERT(sample.groups[0].members[0].curWorldY == 5.0f, "current world Y preserved");
    TEST_ASSERT(sample.groups[0].members[0].curWorldZ == 6.0f, "current world Z preserved");
    TEST_ASSERT(!sample.groups[0].planQueried, "membership is unqueried until the local plan is applied");
    TEST_ASSERT(sample.groups[0].members[0].membership == P09ExactUseMember::Unknown, "unqueried member is unknown");

    P09TieSample distinct;
    {
        const P09SeamExactUseKey a = makeKey(4, 1, 0, 10);
        const P09SeamExactUseKey b = makeKey(4, 1, 0, 11);
        P09TieMemberInput mem[2];
        mem[0] = tieMember(a, true, true);
        mem[1] = tieMember(b, true, false);
        P09TieGroupInput in;
        in.child = makeKey(4, 2, 1, 12);
        in.childComplete = true;
        in.bestGap = 2.0f;
        in.multiplicity = 2;
        in.attemptGeneration = 3;
        in.members = mem;
        in.memberCount = 2;
        p09TieSampleRecord(distinct, in);
    }
    TEST_ASSERT(distinct.groups[0].identity == P09TieIdentity::DistinctExactUses, "different full keys");
    TEST_ASSERT(std::strcmp(p09TieIdentityName(distinct.groups[0].identity), "DISTINCT_EXACT_USES") == 0,
        "distinct name does not claim seam legitimacy");
    TEST_ASSERT(distinct.groups[0].members[0].key.originalWorldIndex == 10u, "first world preserved");
    TEST_ASSERT(distinct.groups[0].members[1].key.originalWorldIndex == 11u, "second world preserved");

    P09TieSample incomplete;
    {
        P09TieMemberInput mem[2];
        mem[0] = tieMember(repeated, true, true);
        mem[1] = tieMember(repeated, false, false);
        P09TieGroupInput in;
        in.child = makeKey(1, 1, 0, 1);
        in.childComplete = false;
        in.bestGap = 1.25f;
        in.multiplicity = 2;
        in.attemptGeneration = 3;
        in.members = mem;
        in.memberCount = 2;
        p09TieSampleRecord(incomplete, in);
    }
    TEST_ASSERT(incomplete.groups[0].identity == P09TieIdentity::Unknown, "incomplete key stays unknown");
    TEST_ASSERT(!incomplete.groups[0].membersTruncated, "incomplete is not the truncation flag");

    P09TieSample truncated;
    {
        P09TieMemberInput many[97];
        for (uint32_t i = 0; i < 97u; i++) {
            many[i] = tieMember(repeated, true, i == 0u);
            many[i].prevNdcX = 0.01f * float(i);
        }
        P09TieGroupInput in;
        in.child = makeKey(9, 1, 2, 70);
        in.childComplete = true;
        in.bestGap = 1.25f;
        in.multiplicity = 97;
        in.attemptGeneration = 5;
        in.members = many;
        in.memberCount = 97;
        p09TieSampleRecord(truncated, in);
    }
    TEST_ASSERT(truncated.groups[0].stored == 96u, "member store stops at 96");
    TEST_ASSERT(truncated.groups[0].multiplicity == 97u, "full tie multiplicity is kept");
    TEST_ASSERT(truncated.groups[0].membersTruncated, "member list is truncated");
    TEST_ASSERT(truncated.memberTruncationEvents == 1u, "truncation event counted");
    TEST_ASSERT(truncated.groups[0].identity == P09TieIdentity::Unknown, "truncated group stays unknown");
    TEST_ASSERT(truncated.groups[0].members[95].prevNdcX == 0.01f * 95.0f, "last stored NDC preserved");
    TEST_ASSERT(truncated.groups[0].members[0].selected, "selected parent survives truncation");

    const P09SeamExactUseKey inPlan = makeKey(6, 1, 0, 20);
    const P09SeamExactUseKey missing = makeKey(6, 1, 1, 21);
    const P09SeamExactUseKey childKey = makeKey(6, 2, 0, 22);
    P09ExactUsePlanResult plan = makePlan({ inPlan, childKey });
    P09TieSample local;
    {
        P09TieMemberInput mem[2];
        mem[0] = tieMember(inPlan, true, true);
        mem[1] = tieMember(missing, true, false);
        P09TieGroupInput in;
        in.child = childKey;
        in.childComplete = true;
        in.bestGap = 3.0f;
        in.multiplicity = 2;
        in.attemptGeneration = 8;
        in.members = mem;
        in.memberCount = 2;
        p09TieSampleRecord(local, in);
        P09TieMemberInput otherMem[2];
        otherMem[0] = tieMember(inPlan, true, true);
        otherMem[1] = tieMember(inPlan, true, false);
        P09TieGroupInput other;
        other.child = childKey;
        other.childComplete = true;
        other.bestGap = 3.0f;
        other.multiplicity = 2;
        other.attemptGeneration = 9;
        other.members = otherMem;
        other.memberCount = 2;
        p09TieSampleRecord(local, other);
    }
    p09TieSampleApplyLocalPlan(local, 8, plan, 4, 4);
    TEST_ASSERT(local.groups[0].planQueried, "matching attempt is queried");
    TEST_ASSERT(!local.groups[1].planQueried, "other attempt is not queried");
    TEST_ASSERT(local.groups[0].planActive == 1u, "local plan active bit stored");
    TEST_ASSERT(local.groups[0].planExhaustive == 1u, "local plan exhaustive bit stored");
    TEST_ASSERT(local.groups[0].planWorkloadId == kWl, "local plan workload stored");
    TEST_ASSERT(local.groups[0].planGeneration == 4u, "local plan generation stored");
    TEST_ASSERT(local.groups[0].expectedGeneration == 4u, "attempt generation stored");
    TEST_ASSERT(local.groups[0].childMembership == P09ExactUseMember::Selected, "child in the local plan");
    TEST_ASSERT(local.groups[0].members[0].membership == P09ExactUseMember::Selected, "parent in the local plan");
    TEST_ASSERT(local.groups[0].members[0].reason == P09ExactUseUnknownReason::None, "hit has no unknown reason");
    TEST_ASSERT(local.groups[0].members[1].membership == P09ExactUseMember::NotSelected, "definitive miss on this plan");
    TEST_ASSERT(local.groups[1].members[0].membership == P09ExactUseMember::Unknown, "unqueried group stays unknown");
    TEST_ASSERT(local.groups[0].identity == P09TieIdentity::DistinctExactUses, "membership does not change identity");

    P09ExactUsePlanResult inactive;
    p09TieSampleApplyLocalPlan(local, 9, inactive, 4, 4);
    TEST_ASSERT(local.groups[1].planQueried, "inactive plan is still a query");
    TEST_ASSERT(local.groups[1].planActive == 0u, "inactive plan recorded");
    TEST_ASSERT(local.groups[1].childMembership == P09ExactUseMember::Unknown, "inactive plan is unknown");
    TEST_ASSERT(local.groups[1].childReason == P09ExactUseUnknownReason::PlanInactive, "inactive reason");
    TEST_ASSERT(local.groups[1].members[0].membership != P09ExactUseMember::NotSelected, "inactive is not a miss");

    P09TieSample badPlans;
    {
        P09TieMemberInput mem[2];
        mem[0] = tieMember(inPlan, true, true);
        mem[1] = tieMember(missing, false, false);
        P09TieGroupInput in;
        in.child = childKey;
        in.childComplete = true;
        in.bestGap = 3.0f;
        in.multiplicity = 2;
        in.attemptGeneration = 2;
        in.members = mem;
        in.memberCount = 2;
        p09TieSampleRecord(badPlans, in);
    }
    P09ExactUsePlanResult wrongWl = makePlan({ inPlan }, 99u);
    p09TieSampleApplyLocalPlan(badPlans, 2, wrongWl, 1, 1);
    TEST_ASSERT(badPlans.groups[0].members[0].membership == P09ExactUseMember::Unknown, "wrong workload is unknown");
    TEST_ASSERT(badPlans.groups[0].members[0].reason == P09ExactUseUnknownReason::WrongWorkload, "wrong workload reason");
    TEST_ASSERT(badPlans.groups[0].members[0].membership != P09ExactUseMember::NotSelected, "wrong workload is not a miss");

    P09ExactUsePlanResult thin = makePlan({ inPlan }, kWl, false, true);
    p09TieSampleApplyLocalPlan(badPlans, 2, thin, 7, 7);
    TEST_ASSERT(badPlans.groups[0].members[0].reason == P09ExactUseUnknownReason::PlanNotExhaustive, "thin plan reason");
    TEST_ASSERT(badPlans.groups[0].members[0].membership == P09ExactUseMember::Unknown, "thin plan is unknown");

    p09TieSampleApplyLocalPlan(badPlans, 2, plan, 1, 2);
    TEST_ASSERT(badPlans.groups[0].members[0].reason == P09ExactUseUnknownReason::WrongGeneration, "generation reason");
    TEST_ASSERT(badPlans.groups[0].members[0].membership != P09ExactUseMember::NotSelected, "wrong generation is not a miss");

    p09TieSampleApplyLocalPlan(badPlans, 2, plan, 2, 2);
    TEST_ASSERT(badPlans.groups[0].members[0].membership == P09ExactUseMember::Selected, "complete key can hit");
    TEST_ASSERT(badPlans.groups[0].members[1].membership == P09ExactUseMember::Unknown, "incomplete key is unknown");
    TEST_ASSERT(badPlans.groups[0].members[1].reason == P09ExactUseUnknownReason::KeyIncomplete, "incomplete reason");
    TEST_ASSERT(badPlans.groups[0].identity == P09TieIdentity::Unknown, "incomplete enumeration stays unknown");

    sample = P09TieSample{};
    TEST_ASSERT(sample.storedGroups == 0u, "value-init clears stored groups");
    TEST_ASSERT(sample.skippedGroups == 0u, "value-init clears skipped groups");
    TEST_ASSERT(sample.observedPathologicalTieGroups == 0u, "value-init clears observed groups");
    TEST_ASSERT(sample.memberTruncationEvents == 0u, "value-init clears truncation events");

    char text[96 * 1024];
    std::memset(text, 0, sizeof(text));
#ifdef _WIN32
    // No fmemopen on Windows: format into a binary temp file (tmpfile() is "wb+", so no
    // CRLF translation) and read it back into the same buffer.
    FILE *mem = std::tmpfile();
    TEST_ASSERT(mem != nullptr, "formatter buffer opens");
    p09TieFormatSample(mem, truncated);
    std::fflush(mem);
    const long written = std::ftell(mem);
    std::rewind(mem);
    if (written > 0 && static_cast<size_t>(written) < sizeof(text)) {
        TEST_ASSERT(std::fread(text, 1, static_cast<size_t>(written), mem) == static_cast<size_t>(written), "formatter buffer reads back");
    }
    std::fclose(mem);
#else
    FILE *mem = fmemopen(text, sizeof(text) - 1u, "w");
    TEST_ASSERT(mem != nullptr, "formatter buffer opens");
    p09TieFormatSample(mem, truncated);
    std::fflush(mem);
    const long written = std::ftell(mem);
    std::fclose(mem);
#endif
    TEST_ASSERT(written > 0, "formatter wrote bytes");
    TEST_ASSERT(std::strstr(text, "GROUP_CAP=20\n") != nullptr, "text exposes the group cap");
    TEST_ASSERT(std::strstr(text, "MEMBER_CAP=96\n") != nullptr, "text exposes the member cap");
    TEST_ASSERT(std::strstr(text, "STORED=96\n") != nullptr, "text stores 96 members");
    TEST_ASSERT(std::strstr(text, "MULTIPLICITY=97\n") != nullptr, "text keeps multiplicity");
    TEST_ASSERT(std::strstr(text, "TRUNCATED=YES\n") != nullptr, "text flags truncation");
    TEST_ASSERT(std::strstr(text, "IDENTITY=UNKNOWN\n") != nullptr, "truncated text stays unknown");
    TEST_ASSERT(std::strstr(text, "PREV_NDC=") != nullptr, "text includes previous NDC");
    TEST_ASSERT(std::strstr(text, "CUR_NDC=") != nullptr, "text includes current NDC");
    TEST_ASSERT(std::strstr(text, "PREV_WORLD=NA") != nullptr, "missing world prints NA");
    TEST_ASSERT(std::strstr(text, "CUR_WORLD=4,5,6") != nullptr, "available world is printed");
    TEST_ASSERT(std::strstr(text, "PLAN_QUERIED=NO\n") != nullptr, "unqueried plan is explicit");
    int groupLines = 0;
    for (const char *p = text; (p = std::strstr(p, "GROUP index=")) != nullptr; p++) {
        groupLines++;
    }
    TEST_ASSERT(groupLines == 1, "one truncated group is one text block");
}

int main() {
    test_key_make();
    test_both_one_none();
    test_same_world_distinct_use();
    test_invalid_plans_never_definitive();
    test_tie_incomplete_duplicate();
    test_units_and_exposure();
    test_budget_and_default_off();
    test_unique_and_pending_caps();
    test_arm_wall_clock_and_all_observed();
    test_pathological_tie_groups();
    std::printf("p09_exact_use_correlation_test: %d/%d passed\n", g_test_count, g_test_count);
    return 0;
}
