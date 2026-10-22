// P09 PREV/CUR local-pair snapshot. Header-only capture and row-vector replay.
// Offline CPU. No GPU, no game, no interpolation change.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "render/rt64_p09_local_pair_snapshot.h"

using namespace RT64;

static int g_test_count = 0;

#define TEST_ASSERT(cond, msg) do { \
    ++g_test_count; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL: %s (line %d): %s\n", __FUNCTION__, __LINE__, msg); \
        std::exit(1); \
    } \
} while (0)

struct Mats {
    std::vector<float> cur;
    std::vector<float> prev;
    std::vector<uint32_t> prevIndex;
};

static void ident(float *m) {
    std::memset(m, 0, sizeof(float) * 16);
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void trans(float *m, float x, float y, float z) {
    ident(m);
    m[12] = x;
    m[13] = y;
    m[14] = z;
}

static void loadMats(void *ctx, uint32_t world, int isPrev, float out[16]) {
    const Mats *m = static_cast<const Mats *>(ctx);
    const uint32_t slot = isPrev ? m->prevIndex[world] : world;
    const float *src = (isPrev ? m->prev.data() : m->cur.data()) + slot * 16u;
    std::memcpy(out, src, sizeof(float) * 16);
}

static void test_row_vector() {
    float m[16] = {};
    m[1] = 2.0f;
    m[4] = -3.0f;
    m[15] = 1.0f;
    const float pos[3] = {1.0f, 2.0f, 3.0f};
    float out[3];
    TEST_ASSERT(p09LocalPairRowVector(pos, m, out), "row vector finite");
    TEST_ASSERT(out[0] == -6.0f && out[1] == 2.0f && out[2] == 0.0f, "row vector algebra");
    const float a[3] = {0.0f, 0.0f, 0.0f};
    const float b[3] = {10000.0f, 0.0f, 0.0f};
    TEST_ASSERT(p09LocalPairEdgeTol(a, b) == 1.0f, "edge tol is 1e-4 * length");
    TEST_ASSERT(sizeof(P09LocalPairSnapshot) <= kP09LocalPairMaxBytes, "256KiB cap");
}

static void test_candidates() {
    TEST_ASSERT(p09LocalPairCandidateClass(0x62Bu, 0x11CB8u, 11u, 42u) ==
        P09LocalPairCandidateSnoutStatic, "snout static source");
    TEST_ASSERT(p09LocalPairCandidateClass(0x62Bu, 0u, 26u, 108u) ==
        P09LocalPairCandidateFeatherStatic, "idx 26 anim 108 static source");
    TEST_ASSERT(p09LocalPairCandidateClass(0x62Bu, 0u, 30u, 103u) ==
        P09LocalPairCandidateFeatherStatic, "idx 30 anim 103 static source");
    TEST_ASSERT(p09LocalPairCandidateClass(0x62Bu, 0u, 35u, 100u) ==
        P09LocalPairCandidateFeatherStatic, "idx 35 anim 100 static source");
    TEST_ASSERT(p09LocalPairCandidateClass(0x62Bu, 0u, 11u, 42u) ==
        P09LocalPairCandidateNone, "snout static source needs node 0x11CB8");
    TEST_ASSERT(p09LocalPairCandidateClass(0x62Bu, 0u, 26u, 79u) ==
        P09LocalPairCandidateNone, "anim 79 is outside this static list");
    TEST_ASSERT(p09LocalPairCandidateClass(0x61Cu, 0x11CB8u, 11u, 42u) ==
        P09LocalPairCandidateNone, "other resource is outside this static list");
}

struct Scene {
    Mats mats;
    std::vector<uint8_t> mapped;
    std::vector<float> pos;
    std::vector<uint32_t> faces;
    std::vector<uint16_t> worlds;
    std::vector<uint8_t> lights;
    std::vector<uint16_t> look;
    std::vector<P09LocalPairWitnessIn> wits;
    std::vector<P09LocalPairCallIn> calls;
    P09LocalPairInput in;
};

static void bind(Scene &sc, uint64_t cur, uint64_t prev, uint32_t ring) {
    sc.in = P09LocalPairInput();
    sc.in.curWorkloadId = cur;
    sc.in.prevWorkloadId = prev;
    sc.in.prevRingIndex = ring;
    sc.in.frameMapped = 1;
    sc.in.curWorldCount = uint32_t(sc.mapped.size());
    sc.in.prevWorldCount = uint32_t(sc.mats.prev.size() / 16u);
    sc.in.worldMapped = sc.mapped.data();
    sc.in.prevTransformIndex = sc.mats.prevIndex.data();
    sc.in.posFloats = sc.pos.data();
    sc.in.posFloatCount = uint32_t(sc.pos.size());
    sc.in.faceIndices = sc.faces.data();
    sc.in.faceIndexCount = uint32_t(sc.faces.size());
    sc.in.worldIndices = sc.worlds.data();
    sc.in.vertexCount = uint32_t(sc.worlds.size());
    sc.in.lightCounts = sc.lights.data();
    sc.in.lightCountCount = uint32_t(sc.lights.size());
    sc.in.lookAtIndices = sc.look.data();
    sc.in.lookAtCount = uint32_t(sc.look.size());
    sc.in.witnesses = sc.wits.data();
    sc.in.witnessCount = uint32_t(sc.wits.size());
    sc.in.calls = sc.calls.data();
    sc.in.callCount = uint32_t(sc.calls.size());
    sc.in.matrixCtx = &sc.mats;
    sc.in.loadMatrix = loadMats;
}

static void pushWorld(Scene &sc, float cx, float cy, float cz, float px, float py, float pz) {
    float c[16], p[16];
    trans(c, cx, cy, cz);
    trans(p, px, py, pz);
    sc.mats.cur.insert(sc.mats.cur.end(), c, c + 16);
    sc.mats.prev.insert(sc.mats.prev.end(), p, p + 16);
    sc.mats.prevIndex.push_back(uint32_t(sc.mapped.size()));
    sc.mapped.push_back(1);
}

static void test_snapshot_replay_and_prev_generation() {
    static P09LocalPairSnapshot snap;
    p09LocalPairSnapshotReset(snap);
    Scene sc;
    pushWorld(sc, 10.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    pushWorld(sc, 0.0f, 20.0f, 0.0f, 0.0f, 2.0f, 0.0f);
    sc.pos = {0, 0, 0, 0, 0, 0, 1, 0, 0};
    sc.faces = {0, 1, 2};
    sc.worlds = {0, 1, 0};
    sc.lights = {4, 0, 1};
    sc.look = {0x5, 0, 0};
    P09LocalPairWitnessIn w0{};
    w0.worldIndex = 0;
    w0.resourceId = 0x62B;
    w0.nodeRel = 0x11CB8;
    w0.boneIndex = 11;
    w0.boneAnimId = 42;
    w0.provenanceKnown = 1;
    w0.candidateClass = P09LocalPairCandidateSnoutStatic;
    P09LocalPairWitnessIn w1{};
    w1.worldIndex = 1;
    w1.provenanceKnown = 0;
    sc.wits = {w0, w1};
    P09LocalPairCallIn call{};
    call.fbPair = 3;
    call.projection = 4;
    call.gameCall = 5;
    call.faceStart = 0;
    call.triangleCount = 1;
    call.displayList = 0xABC;
    call.displayListKnown = 1;
    sc.calls = {call};
    bind(sc, 100, 99, 1);
    TEST_ASSERT(p09LocalPairSnapshotAdd(snap, sc.in), "capture");
    TEST_ASSERT(snap.success == 1 && snap.pairCount == 1 && snap.pairs[0].triCount == 1, "one pair");
    TEST_ASSERT(p09LocalPairPrevWorkloadId(snap, 0) == 99, "PREV generation is workloadId");
    TEST_ASSERT(snap.pairs[0].prevRingIndex == 1, "ring index stored separately");
    TEST_ASSERT(snap.pairs[0].curWorkloadId == 100, "CUR workloadId");
    const P09LocalPairTri &tri = snap.pairs[0].tris[0];
    TEST_ASSERT(tri.candidateMask == 1, "multi-world mask recorded");
    TEST_ASSERT(tri.edge[0].cornerA == 0 && tri.edge[0].cornerB == 1, "edge 0 both endpoints");
    TEST_ASSERT(tri.edge[1].cornerA == 1 && tri.edge[1].cornerB == 2, "edge 1 both endpoints");
    TEST_ASSERT(tri.edge[2].cornerA == 2 && tri.edge[2].cornerB == 0, "edge 2 both endpoints");
    TEST_ASSERT(tri.corner[0].lightCount == 4 && tri.corner[0].lookAtIndex == 0x5, "light and lookat stored");
    TEST_ASSERT(tri.corner[0].displayList == 0xABC && tri.corner[0].displayListKnown == 1, "dl stored");
    const P09LocalPairWorld &sw = snap.pairs[0].worlds[tri.corner[0].worldSlot];
    TEST_ASSERT(sw.provenanceKnown == 1 && sw.resourceId == 0x62B && sw.nodeRel == 0x11CB8 &&
        sw.boneIndex == 11 && sw.boneAnimId == 42, "token provenance stored");
    TEST_ASSERT(sw.candidateClass == P09LocalPairCandidateSnoutStatic, "class stored unapplied");
    TEST_ASSERT(sw.prevTransformIndex == 0 && sw.mapped == 1, "prev transform index stored");
    const P09SeamExactUseKey &k = tri.corner[1].key;
    TEST_ASSERT(k.workloadId == 100 && k.fbPair == 3 && k.projection == 4 && k.gameCall == 5 &&
        k.triangleLocal == 0 && k.cornerInTriangle == 1 && k.faceCornerIndex == 1 &&
        k.storedVertexId == 1 && k.originalWorldIndex == 1, "exact 9-key");
    float a[3], b[3], expectA[3], expectB[3];
    TEST_ASSERT(p09LocalPairReplayEdge(snap, 0, 0, 0, 0, a, b), "replay edge current");
    TEST_ASSERT(p09LocalPairRowVector(tri.corner[0].localPos, snap.pairs[0].worlds[tri.corner[0].worldSlot].cur, expectA), "row a");
    TEST_ASSERT(p09LocalPairRowVector(tri.corner[1].localPos, snap.pairs[0].worlds[tri.corner[1].worldSlot].cur, expectB), "row b");
    TEST_ASSERT(a[0] == expectA[0] && a[1] == expectA[1] && b[0] == expectB[0] && b[1] == 20.0f,
        "edge endpoints match row-vector, mask not applied");
    TEST_ASSERT(a[0] == 10.0f, "corner 0 current translation");
    float pa[3], pb[3];
    TEST_ASSERT(p09LocalPairReplayEdge(snap, 0, 0, 0, 1, pa, pb), "replay edge prev");
    TEST_ASSERT(pa[0] == 1.0f && pb[1] == 2.0f, "prev endpoints use prev matrices");
}

static void failCase(const char *name, const P09LocalPairInput &in, uint8_t reason) {
    static P09LocalPairSnapshot snap;
    p09LocalPairSnapshotReset(snap);
    const bool ok = p09LocalPairSnapshotAdd(snap, in);
    TEST_ASSERT(!ok && snap.success == 0 && snap.pairCount == 0 && snap.reason == reason, name);
    TEST_ASSERT(snap.reasonCounts[reason] >= 1u, "reason counted");
    float out[3] = {1, 2, 3};
    TEST_ASSERT(!p09LocalPairReplayCorner(snap, 0, 0, 0, 0, out), "failed snapshot does not replay");
}

static Scene oneTri() {
    Scene sc;
    pushWorld(sc, 1, 0, 0, 2, 0, 0);
    sc.pos = {1, 2, 3};
    sc.faces = {0, 0, 0};
    sc.worlds = {0};
    sc.lights = {1};
    sc.look = {1};
    P09LocalPairWitnessIn w{};
    w.worldIndex = 0;
    w.provenanceKnown = 1;
    w.resourceId = 1;
    sc.wits = {w};
    P09LocalPairCallIn call{};
    call.triangleCount = 1;
    sc.calls = {call};
    return sc;
}

static void test_fail_closed() {
    Scene missing = oneTri();
    bind(missing, 7, 8, 0);
    missing.in.frameMapped = 0;
    failCase("missing map", missing.in, P09LocalPairReasonMissingMap);

    Scene unmapped = oneTri();
    unmapped.mapped[0] = 0;
    bind(unmapped, 7, 8, 0);
    failCase("unmapped", unmapped.in, P09LocalPairReasonUnmapped);

    Scene badMap = oneTri();
    badMap.mats.prevIndex[0] = 5;
    bind(badMap, 7, 8, 0);
    badMap.in.prevWorldCount = 1;
    failCase("map invalid", badMap.in, P09LocalPairReasonMapInvalid);

    Scene nan = oneTri();
    nan.mats.cur[0] = std::numeric_limits<float>::quiet_NaN();
    bind(nan, 7, 8, 0);
    failCase("nonfinite", nan.in, P09LocalPairReasonNonFinite);

    Scene multi = oneTri();
    multi.wits.push_back(multi.wits[0]);
    bind(multi, 7, 8, 0);
    failCase("multiplicity", multi.in, P09LocalPairReasonMultiplicity);

    Scene badKey = oneTri();
    bind(badKey, 7, 8, 0);
    badKey.in.vertexCount = 0;
    failCase("key invalid", badKey.in, P09LocalPairReasonKeyInvalid);

    Scene worlds = oneTri();
    worlds.wits.clear();
    for (uint32_t i = 0; i < 17; i++) {
        P09LocalPairWitnessIn w{};
        w.worldIndex = i;
        worlds.wits.push_back(w);
    }
    bind(worlds, 7, 8, 0);
    failCase("cap worlds", worlds.in, P09LocalPairReasonCapWorlds);

    Scene tris = oneTri();
    tris.calls[0].triangleCount = 193;
    tris.faces.assign(193u * 3u, 0u);
    bind(tris, 7, 8, 0);
    failCase("cap tris", tris.in, P09LocalPairReasonCapTris);

    static P09LocalPairSnapshot snap;
    p09LocalPairSnapshotReset(snap);
    Scene a = oneTri();
    Scene b = oneTri();
    Scene c = oneTri();
    bind(a, 10, 9, 1);
    bind(b, 12, 11, 1);
    bind(c, 14, 13, 1);
    TEST_ASSERT(p09LocalPairSnapshotAdd(snap, a.in), "pair 0");
    TEST_ASSERT(p09LocalPairSnapshotAdd(snap, b.in), "pair 1");
    TEST_ASSERT(snap.pairCount == 2, "two pairs kept");
    const bool third = p09LocalPairSnapshotAdd(snap, c.in);
    TEST_ASSERT(!third && snap.success == 0 && snap.pairCount == 0 &&
        snap.reason == P09LocalPairReasonCapPairs, "third pair fail-closed");
}

static void test_mutation_nowitness_roundtrip_and_guards() {
    TEST_ASSERT(sizeof(P09LocalPairSnapshot) > 4096u, "persisted snapshot is the full record");
    TEST_ASSERT(sizeof(P09LocalPairSnapshot) + 12u <= kP09LocalPairMaxBytes, "file stays inside 256KiB");
    TEST_ASSERT(p09LocalPairRunProvenance(false, true), "snapshot-only runs provenance setters");
    TEST_ASSERT(!p09LocalPairRunHeavySway(false), "snapshot-only does not run heavy sway");
    TEST_ASSERT(!p09LocalPairRunProvenance(false, false), "both flags off stays idle");
    TEST_ASSERT(!p09LocalPairShouldCapture(0, 0, 0, 0), "diagnostic flag waits until scene arm");
    TEST_ASSERT(p09LocalPairShouldCapture(1, 0, 0, 0), "explicit arm starts the four attempts");
    TEST_ASSERT(!p09LocalPairShouldCapture(1, 1, 0, 0), "busy slot is not mutated by another process call");
    TEST_ASSERT(!p09LocalPairShouldCapture(1, 0, 0, kP09LocalPairMaxAttempts), "attempts stop after the cap");
    char pathA[256];
    char pathB[256];
    TEST_ASSERT(p09LocalPairOutputName(pathA, sizeof(pathA), 11, 100) > 0, "output name");
    TEST_ASSERT(p09LocalPairOutputName(pathB, sizeof(pathB), 12, 100) > 0, "other pid");
    TEST_ASSERT(std::strcmp(pathA, pathB) != 0, "pid keeps captures apart");
    TEST_ASSERT(p09LocalPairOutputName(pathB, sizeof(pathB), 11, 101) > 0, "other generation");
    TEST_ASSERT(std::strcmp(pathA, pathB) != 0, "workload generation keeps captures apart");
    TEST_ASSERT(p09LocalPairRunHeavySway(true), "sway flag still owns the heavy path");
    TEST_ASSERT(p09LocalPairWitnessIndexMatch(10, 10, 1), "vertex workload owns the world index");
    TEST_ASSERT(!p09LocalPairWitnessIndexMatch(10, 99, 1), "a different workload id is not that index");
    TEST_ASSERT(!p09LocalPairWitnessIndexMatch(10, 10, 0), "unbound vertex is not a witness");

    static P09LocalPairSnapshot snap;
    p09LocalPairSnapshotReset(snap);
    Scene sc;
    pushWorld(sc, 10.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    sc.pos = {0, 0, 0};
    sc.faces = {0, 0, 0};
    sc.worlds = {0};
    sc.lights = {2};
    sc.look = {0x1};
    P09LocalPairWitnessIn w{};
    w.worldIndex = 0;
    w.provenanceKnown = 1;
    w.resourceId = 0x62B;
    w.nodeRel = 0x11CB8;
    w.boneIndex = 11;
    w.boneAnimId = 42;
    sc.wits = {w};
    P09LocalPairCallIn call{};
    call.triangleCount = 1;
    call.displayList = 7;
    call.displayListKnown = 1;
    sc.calls = {call};
    bind(sc, 50, 40, 3);
    TEST_ASSERT(p09LocalPairSnapshotAdd(snap, sc.in), "copy");
    sc.mats.cur[12] = 999.0f;
    sc.pos[0] = 50.0f;
    float got[3];
    TEST_ASSERT(p09LocalPairReplayCorner(snap, 0, 0, 0, 0, got), "replay after source mutation");
    TEST_ASSERT(got[0] == 10.0f && got[1] == 0.0f && got[2] == 0.0f, "copied matrix and local pos stay");

    FILE *out = std::tmpfile();
    TEST_ASSERT(out != nullptr, "tmpfile");
    TEST_ASSERT(p09LocalPairSnapshotWrite(out, snap), "write full snapshot");
    std::fflush(out);
    std::rewind(out);
    static P09LocalPairSnapshot loaded;
    TEST_ASSERT(p09LocalPairSnapshotRead(out, loaded), "read full snapshot");
    std::fclose(out);
    TEST_ASSERT(loaded.success == 1 && loaded.pairCount == 1, "roundtrip status");
    TEST_ASSERT(loaded.pairs[0].curWorkloadId == 50 && loaded.pairs[0].prevWorkloadId == 40, "roundtrip ids");
    TEST_ASSERT(loaded.pairs[0].prevRingIndex == 3, "roundtrip ring");
    const P09SeamExactUseKey &key = loaded.pairs[0].tris[0].corner[0].key;
    TEST_ASSERT(key.workloadId == 50 && key.storedVertexId == 0 && key.originalWorldIndex == 0, "roundtrip key");
    TEST_ASSERT(loaded.pairs[0].tris[0].corner[0].localPos[0] == 0.0f, "roundtrip local pos");
    TEST_ASSERT(loaded.pairs[0].worlds[0].cur[12] == 10.0f && loaded.pairs[0].worlds[0].prev[12] == 1.0f, "roundtrip matrices");
    TEST_ASSERT(loaded.pairs[0].tris[0].corner[0].lightCount == 2 &&
        loaded.pairs[0].tris[0].corner[0].lookAtIndex == 0x1, "roundtrip light");
    float again[3];
    TEST_ASSERT(p09LocalPairReplayCorner(loaded, 0, 0, 0, 0, again) && again[0] == 10.0f, "replay loaded bytes");

    static P09LocalPairArm arm;
    arm = P09LocalPairArm();
    static P09LocalPairSnapshot empty;
    p09LocalPairSnapshotReset(empty);
    arm.attempts = 1;
    p09LocalPairArmFinish(arm, empty);
    TEST_ASSERT(arm.frozen == 0 && arm.held.success == 0 &&
        arm.held.reason == P09LocalPairReasonArmedWaiting, "waiting is not success");
    TEST_ASSERT(!p09LocalPairReplayCorner(arm.held, 0, 0, 0, 0, got), "waiting does not replay");
    const float kept = snap.pairs[0].worlds[0].cur[12];
    arm.frozen = 1;
    arm.held = snap;
    p09LocalPairSnapshotReset(empty);
    p09LocalPairArmFinish(arm, empty);
    TEST_ASSERT(arm.held.success == 1 && arm.held.pairs[0].worlds[0].cur[12] == kept,
        "later empty frame does not replace a frozen capture");

    static P09LocalPairArm exhausted;
    exhausted = P09LocalPairArm();
    exhausted.attempts = kP09LocalPairMaxAttempts;
    p09LocalPairSnapshotReset(empty);
    p09LocalPairArmFinish(exhausted, empty);
    TEST_ASSERT(exhausted.frozen == 1 && exhausted.held.success == 0 &&
        exhausted.held.reason == P09LocalPairReasonNoWitness && exhausted.held.pairCount == 0,
        "no witness is fail-closed");
}

static bool roundTripRejected(P09LocalPairSnapshot &s) {
    FILE *out = std::tmpfile();
    if (out == nullptr || !p09LocalPairSnapshotWrite(out, s)) return false;
    std::fflush(out);
    std::rewind(out);
    static P09LocalPairSnapshot loaded;
    const bool accepted = p09LocalPairSnapshotRead(out, loaded);
    std::fclose(out);
    return !accepted;
}

static void fillValid(P09LocalPairSnapshot &snap) {
    p09LocalPairSnapshotReset(snap);
    Scene sc;
    pushWorld(sc, 10.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    pushWorld(sc, 0.0f, 20.0f, 0.0f, 0.0f, 2.0f, 0.0f);
    sc.pos = {0, 0, 0, 0, 0, 0, 1, 0, 0};
    sc.faces = {0, 1, 2};
    sc.worlds = {0, 1, 0};
    sc.lights = {4, 0, 1};
    sc.look = {0x5, 0, 0};
    P09LocalPairWitnessIn w0{};
    w0.worldIndex = 0;
    w0.resourceId = 0x62B;
    w0.nodeRel = 0x11CB8;
    w0.boneIndex = 11;
    w0.boneAnimId = 42;
    w0.provenanceKnown = 1;
    w0.candidateClass = P09LocalPairCandidateSnoutStatic;
    P09LocalPairWitnessIn w1{};
    w1.worldIndex = 1;
    sc.wits = {w0, w1};
    P09LocalPairCallIn call{};
    call.triangleCount = 1;
    call.displayListKnown = 1;
    sc.calls = {call};
    bind(sc, 100, 99, 1);
    TEST_ASSERT(p09LocalPairSnapshotAdd(snap, sc.in) && snap.success == 1 && snap.pairCount == 1,
        "semantic control capture");
}

static void test_semantic_bounds_and_publish() {
    static P09LocalPairSnapshot base;
    fillValid(base);
    TEST_ASSERT(p09LocalPairSnapshotBoundsOk(base), "control identity bounds");
    TEST_ASSERT(!roundTripRejected(base), "control identity read accepts");

    static P09LocalPairSnapshot unused;
    unused = base;
    unused.pairs[0].worlds[kP09LocalPairMaxWorlds - 1u].cur[0] =
        std::numeric_limits<float>::quiet_NaN();
    TEST_ASSERT(p09LocalPairSnapshotBoundsOk(unused), "unused world slot NaN is ignored");

    auto reject = [&](const char *name, auto mutate) {
        static P09LocalPairSnapshot s;
        s = base;
        mutate(s);
        TEST_ASSERT(!p09LocalPairSnapshotBoundsOk(s), name);
        TEST_ASSERT(roundTripRejected(s), name);
    };
    reject("nan localPos", [](P09LocalPairSnapshot &s) {
        s.pairs[0].tris[0].corner[0].localPos[0] = std::numeric_limits<float>::quiet_NaN();
    });
    reject("nan cur", [](P09LocalPairSnapshot &s) {
        const uint16_t slot = s.pairs[0].tris[0].corner[0].worldSlot;
        s.pairs[0].worlds[slot].cur[0] = std::numeric_limits<float>::quiet_NaN();
    });
    reject("nan prev", [](P09LocalPairSnapshot &s) {
        const uint16_t slot = s.pairs[0].tris[0].corner[0].worldSlot;
        s.pairs[0].worlds[slot].prev[0] = std::numeric_limits<float>::quiet_NaN();
    });
    reject("key workload mismatch", [](P09LocalPairSnapshot &s) {
        s.pairs[0].tris[0].corner[0].key.workloadId = s.pairs[0].curWorkloadId + 1u;
    });
    reject("key world mismatch", [](P09LocalPairSnapshot &s) {
        const uint16_t slot = s.pairs[0].tris[0].corner[0].worldSlot;
        s.pairs[0].tris[0].corner[0].key.originalWorldIndex =
            uint16_t(s.pairs[0].worlds[slot].worldIndex + 1u);
    });
    reject("byteCount 0", [](P09LocalPairSnapshot &s) { s.byteCount = 0; });
    reject("success with no pairs", [](P09LocalPairSnapshot &s) {
        s.success = 1;
        s.pairCount = 0;
    });
    reject("edge not a triangle edge", [](P09LocalPairSnapshot &s) {
        s.pairs[0].tris[0].edge[0].cornerB = 0;
    });
    reject("edge endpoints equal", [](P09LocalPairSnapshot &s) {
        s.pairs[0].tris[0].edge[1].cornerA = s.pairs[0].tris[0].edge[1].cornerB;
    });

    static P09LocalPairSnapshot closed;
    p09LocalPairSnapshotReset(closed);
    closed.success = 0;
    closed.reason = P09LocalPairReasonNoWitness;
    closed.pairCount = 0;
    TEST_ASSERT(p09LocalPairSnapshotBoundsOk(closed), "fail-closed empty file stays readable");

    const std::string path = "/tmp/p09-publish-ok.bin";
    std::remove(path.c_str());
    std::remove((path + ".tmp").c_str());
    TEST_ASSERT(p09LocalPairSnapshotPublish(path.c_str(), base), "publish renames only after write");
    FILE *in = std::fopen(path.c_str(), "rb");
    static P09LocalPairSnapshot published;
    TEST_ASSERT(in != nullptr && p09LocalPairSnapshotRead(in, published), "published file reads");
    if (in != nullptr) std::fclose(in);
    std::remove(path.c_str());
    TEST_ASSERT(!p09LocalPairSnapshotPublish("/tmp/no-such-p09-dir/x.bin", base),
        "failed publish does not claim a capture");
}

int main() {
    test_row_vector();
    test_candidates();
    test_snapshot_replay_and_prev_generation();
    test_fail_closed();
    test_mutation_nowitness_roundtrip_and_guards();
    test_semantic_bounds_and_publish();
    std::printf("p09_local_pair_snapshot_test: %d/%d passed\n", g_test_count, g_test_count);
    return 0;
}
