// P09 PREV/CUR local-pair snapshot: cross-process I/O fixture test.
//
// Standalone executable. Three separate invocations (run as distinct processes):
//   p09_local_pair_snapshot_io_test --write-fixture PATH
//       Producer. Builds a nonempty snapshot via the real p09LocalPairSnapshotAdd,
//       copies source arrays, MUTATES the source arrays AFTER Add and BEFORE
//       persist (proves the snapshot owns its data), then writes the real binary
//       via p09LocalPairSnapshotWrite. Exits 0 on success.
//   p09_local_pair_snapshot_io_test --replay-file PATH
//       Loader. Reopens the file with the real p09LocalPairSnapshotRead, validates
//       the exact fixture (9-key, matrices, localPos, triangle, light, lookAt,
//       resource, node provenance, candidate labels), then replays edge endpoints
//       at t = 0, 0.5, 1 with row-vector math. Exits 0 only if every check passes.
//   p09_local_pair_snapshot_io_test --check-negative PATH
//       Negative loader. Expects PATH to be REJECTED (bad magic, bad size,
//       truncated, corrupt payload, or unknown semantic candidate label).
//       Exits 0 only if rejected; nonzero if accepted.
//   p09_local_pair_snapshot_io_test --self-test
//       Usage only. Does NOT spawn subprocesses. Exits 2 so it never reports PASS.
//
// Offline CPU. No GPU, no game, no interpolation change.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "render/rt64_p09_local_pair_snapshot.h"

using namespace RT64;

namespace {

const char *kProg = "p09_local_pair_snapshot_io_test";

void usage(FILE *out) {
    std::fprintf(out,
        "usage:\n"
        "  %s --write-fixture PATH   produce nonempty snapshot file\n"
        "  %s --write-mutant CASE PATH\n"
        "  %s --replay-file PATH     read + validate + replay t=0,0.5,1\n"
        "  %s --check-negative PATH  expect PATH to be rejected (exit 0 iff rejected)\n"
        "  %s --self-test            usage only; run the three commands as separate processes\n",
        kProg, kProg, kProg, kProg, kProg);
}

bool nearEq(float a, float b) {
    return std::fabs(a - b) <= 1e-5f;
}

// Synthetic source arrays. Owned by the producer; mutated after capture.
struct Sources {
    std::vector<float> cur;        // 2 world matrices, 16 floats each
    std::vector<float> prev;       // 2 prev matrices, 16 floats each
    std::vector<uint32_t> prevIdx; // prev transform index per world
    uint8_t mapped[2] = {1, 1};
    float pos[9] = {0, 0, 0, 0, 0, 0, 1, 0, 0};
    uint32_t faces[3] = {0, 1, 2};
    uint16_t worlds[3] = {0, 1, 0};
    uint8_t lights[3] = {4, 0, 1};
    uint16_t look[3] = {5, 0, 0};
    P09LocalPairWitnessIn wits[2];
    P09LocalPairCallIn call;
};

void ident(float *m) {
    std::memset(m, 0, sizeof(float) * 16);
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void trans(float *m, float x, float y, float z) {
    ident(m);
    m[12] = x;
    m[13] = y;
    m[14] = z;
}

void buildSources(Sources &s) {
    float c[16], p[16];
    trans(c, 10.0f, 0.0f, 0.0f);
    trans(p, 1.0f, 0.0f, 0.0f);
    s.cur.insert(s.cur.end(), c, c + 16);
    s.prev.insert(s.prev.end(), p, p + 16);
    s.prevIdx.push_back(0);
    trans(c, 0.0f, 20.0f, 0.0f);
    trans(p, 0.0f, 2.0f, 0.0f);
    s.cur.insert(s.cur.end(), c, c + 16);
    s.prev.insert(s.prev.end(), p, p + 16);
    s.prevIdx.push_back(1);

    s.wits[0].worldIndex = 0;
    s.wits[0].resourceId = 0x62Bu;
    s.wits[0].nodeRel = 0x11CB8u;
    s.wits[0].boneIndex = 11u;
    s.wits[0].boneAnimId = 42u;
    s.wits[0].provenanceKnown = 1;
    s.wits[0].candidateClass = P09LocalPairCandidateSnoutStatic;
    s.wits[1].worldIndex = 1;
    s.wits[1].provenanceKnown = 0;

    s.call.fbPair = 3;
    s.call.projection = 4;
    s.call.gameCall = 5;
    s.call.faceStart = 0;
    s.call.triangleCount = 1;
    s.call.displayList = 0xABCu;
    s.call.displayListKnown = 1;
}

void loadMats(void *ctx, uint32_t world, int isPrev, float out[16]) {
    const Sources *s = static_cast<const Sources *>(ctx);
    const uint32_t slot = isPrev ? s->prevIdx[world] : world;
    const float *src = (isPrev ? s->prev.data() : s->cur.data()) + slot * 16u;
    std::memcpy(out, src, sizeof(float) * 16);
}

void bindInput(P09LocalPairInput &in, Sources &s) {
    in = P09LocalPairInput();
    in.curWorkloadId = 100;
    in.prevWorkloadId = 99;
    in.prevRingIndex = 1;
    in.frameMapped = 1;
    in.curWorldCount = 2;
    in.prevWorldCount = 2;
    in.worldMapped = s.mapped;
    in.prevTransformIndex = s.prevIdx.data();
    in.posFloats = s.pos;
    in.posFloatCount = 9;
    in.faceIndices = s.faces;
    in.faceIndexCount = 3;
    in.worldIndices = s.worlds;
    in.vertexCount = 3;
    in.lightCounts = s.lights;
    in.lightCountCount = 3;
    in.lookAtIndices = s.look;
    in.lookAtCount = 3;
    in.witnesses = s.wits;
    in.witnessCount = 2;
    in.calls = &s.call;
    in.callCount = 1;
    in.matrixCtx = &s;
    in.loadMatrix = loadMats;
}

// ---------------------------------------------------------------- producer

int writeFixture(const char *path) {
    Sources src;
    buildSources(src);
    P09LocalPairInput in;
    bindInput(in, src);

    static P09LocalPairSnapshot snap;
    p09LocalPairSnapshotReset(snap);
    if (!p09LocalPairSnapshotAdd(snap, in) || snap.success != 1 || snap.pairCount != 1) {
        std::fprintf(stderr, "PRODUCER capture failed reason=%s\n",
            p09LocalPairReasonName(snap.reason));
        return 3;
    }

    // Mutation-after-copy: scramble every source array AFTER capture and BEFORE persist.
    // The snapshot must already own its data; the persisted file must still hold the
    // original values. Replay in the loader process verifies exactly that.
    for (float &f : src.cur) f = 999.0f;
    for (float &f : src.prev) f = -999.0f;
    for (int i = 0; i < 9; i++) src.pos[i] = 777.0f;
    src.faces[0] = src.faces[1] = src.faces[2] = 0u;
    src.worlds[0] = src.worlds[1] = src.worlds[2] = 1u;
    src.lights[0] = src.lights[1] = src.lights[2] = 0u;
    src.look[0] = src.look[1] = src.look[2] = 0u;
    src.prevIdx[0] = src.prevIdx[1] = 0u;
    src.wits[0].candidateClass = P09LocalPairCandidateNone;

    FILE *f = std::fopen(path, "wb");
    if (f == nullptr) {
        std::perror("fopen");
        return 4;
    }
    const bool wrote = p09LocalPairSnapshotWrite(f, snap);
    std::fclose(f);
    if (!wrote) {
        std::fprintf(stderr, "PRODUCER write failed\n");
        return 5;
    }
    std::printf("PRODUCER written=1 pairs=%u success=%u reason=%s bytes_struct=%zu\n",
        snap.pairCount, snap.success, p09LocalPairReasonName(snap.reason), sizeof(P09LocalPairSnapshot));
    return 0;
}

static int writeMutant(const char *cas, const char *path) {
    Sources src;
    buildSources(src);
    P09LocalPairInput in;
    bindInput(in, src);
    static P09LocalPairSnapshot snap;
    p09LocalPairSnapshotReset(snap);
    if (!p09LocalPairSnapshotAdd(snap, in) || snap.success != 1 || snap.pairCount != 1) return 3;
    const std::string c = cas;
    P09LocalPairPair &p = snap.pairs[0];
    if (c == "nan_localPos") {
        p.tris[0].corner[0].localPos[0] = std::numeric_limits<float>::quiet_NaN();
    } else if (c == "nan_cur") {
        p.worlds[p.tris[0].corner[0].worldSlot].cur[0] = std::numeric_limits<float>::quiet_NaN();
    } else if (c == "nan_prev") {
        p.worlds[p.tris[0].corner[0].worldSlot].prev[0] = std::numeric_limits<float>::quiet_NaN();
    } else if (c == "key_workloadId_mismatch_pair") {
        p.tris[0].corner[0].key.workloadId = p.curWorkloadId + 1u;
    } else if (c == "key_originalWorldIndex_mismatch_worldslot") {
        const uint16_t slot = p.tris[0].corner[0].worldSlot;
        p.tris[0].corner[0].key.originalWorldIndex = uint16_t(p.worlds[slot].worldIndex + 1u);
    } else if (c == "byteCount_0") {
        snap.byteCount = 0;
    } else if (c == "success_1_pairCount_0") {
        snap.success = 1;
        snap.pairCount = 0;
    } else if (c == "topology_edge_wrong") {
        p.tris[0].edge[0].cornerB = 0;
    } else if (c == "topology_edge_equal_endpoints") {
        p.tris[0].edge[1].cornerA = p.tris[0].edge[1].cornerB;
    } else if (c == "control_identity") {
    } else {
        std::fprintf(stderr, "unknown mutant %s\n", cas);
        return 2;
    }
    FILE *f = std::fopen(path, "wb");
    if (f == nullptr) return 4;
    const bool wrote = p09LocalPairSnapshotWrite(f, snap);
    std::fclose(f);
    return wrote ? 0 : 5;
}

// ---------------------------------------------------------------- loader

#define FX_CHECK(cond, msg) do { \
    if (!(cond)) { \
        std::printf("CHECK_FAIL %s\n", msg); \
        return 1; \
    } \
    std::printf("CHECK_OK %s\n", msg); \
} while (0)

int replayFile(const char *path) {
    FILE *f = std::fopen(path, "rb");
    if (f == nullptr) {
        std::printf("READ_OPEN_FAIL %s\n", path);
        return 3;
    }
    static P09LocalPairSnapshot s;
    const bool ok = p09LocalPairSnapshotRead(f, s);
    std::fclose(f);
    if (!ok) {
        std::printf("READ_REJECTED\n");
        return 4;
    }
    std::printf("READ ok=1 success=%u pairs=%u reason=%s\n",
        s.success, s.pairCount, p09LocalPairReasonName(s.reason));

    // Exact fixture: one pair, one triangle, two worlds.
    FX_CHECK(s.success == 1 && s.pairCount == 1, "one successful pair persisted");
    const P09LocalPairPair &p = s.pairs[0];
    FX_CHECK(p.curWorkloadId == 100 && p.prevWorkloadId == 99 && p.prevRingIndex == 1,
        "workload ids and prev ring index");
    FX_CHECK(p.worldCount == 2 && p.triCount == 1, "world and triangle counts");

    const P09LocalPairTri &t = p.tris[0];
    FX_CHECK(t.edge[0].cornerA == 0 && t.edge[0].cornerB == 1, "edge 0 endpoints 0-1");
    FX_CHECK(t.edge[1].cornerA == 1 && t.edge[1].cornerB == 2, "edge 1 endpoints 1-2");
    FX_CHECK(t.edge[2].cornerA == 2 && t.edge[2].cornerB == 0, "edge 2 endpoints 2-0");

    // 9-key on corner 1 (triangle-local index 0, corner 1, face corner 1, stored vertex 1).
    const P09SeamExactUseKey &k = t.corner[1].key;
    FX_CHECK(k.workloadId == 100 && k.fbPair == 3 && k.projection == 4 && k.gameCall == 5,
        "exact key: workloadId, fbPair, projection, gameCall");
    FX_CHECK(k.triangleLocal == 0 && k.cornerInTriangle == 1 && k.faceCornerIndex == 1 &&
        k.storedVertexId == 1 && k.originalWorldIndex == 1, "exact key: triangle, corner, face, vertex, world");

    const P09LocalPairCorner &c0 = t.corner[0];
    FX_CHECK(c0.lightCount == 4, "corner 0 lightCount");
    FX_CHECK(c0.lookAtIndex == 5, "corner 0 lookAt index");
    FX_CHECK(c0.displayList == 0xABCu && c0.displayListKnown == 1, "display list and known flag");

    // localPos of corner 0 (copied before mutation) must be the original (0,0,0).
    FX_CHECK(nearEq(c0.localPos[0], 0.0f) && nearEq(c0.localPos[1], 0.0f) && nearEq(c0.localPos[2], 0.0f),
        "corner 0 localPos is original (mutation-after-copy isolated)");
    const P09LocalPairCorner &c1 = t.corner[1];
    FX_CHECK(nearEq(c1.localPos[0], 0.0f) && nearEq(c1.localPos[1], 0.0f) && nearEq(c1.localPos[2], 0.0f),
        "corner 1 localPos is original (not 777)");

    // Node provenance and resource on world slot of corner 0.
    const P09LocalPairWorld &w0 = p.worlds[c0.worldSlot];
    FX_CHECK(w0.provenanceKnown == 1 && w0.resourceId == 0x62Bu && w0.nodeRel == 0x11CB8u,
        "node provenance: resource 0x62B and node 0x11CB8");
    FX_CHECK(w0.boneIndex == 11 && w0.boneAnimId == 42, "bone index 11 and anim 42");
    FX_CHECK(w0.candidateClass == P09LocalPairCandidateSnoutStatic,
        "candidate label snout static (not mutated away)");

    // Matrices: mutation-after-copy must not have altered persisted cur/prev.
    FX_CHECK(nearEq(w0.cur[12], 10.0f) && nearEq(w0.prev[12], 1.0f),
        "cur x=10 and prev x=1 persisted from original sources");

    // Replay t = 0, 0.5, 1 on the edge 0 (corner 0 -> corner 1).
    float a[3], b[3], pa[3], pb[3];
    FX_CHECK(p09LocalPairReplayEdge(s, 0, 0, 0, 0, a, b), "replay current edge");
    FX_CHECK(p09LocalPairReplayEdge(s, 0, 0, 0, 1, pa, pb), "replay prev edge");
    std::printf("REPLAY_CUR a=(%g,%g,%g) b=(%g,%g,%g)\n", a[0], a[1], a[2], b[0], b[1], b[2]);
    std::printf("REPLAY_PREV a=(%g,%g,%g) b=(%g,%g,%g)\n", pa[0], pa[1], pa[2], pb[0], pb[1], pb[2]);

    // Expected row-vector endpoints: corner0 localPos (0,0,0) -> cur (10,0,0), prev (1,0,0);
    // corner1 localPos (0,0,0) with world 1 -> cur (0,20,0), prev (0,2,0).
    FX_CHECK(nearEq(a[0], 10.0f) && nearEq(a[1], 0.0f) && nearEq(a[2], 0.0f), "cur a = (10,0,0)");
    FX_CHECK(nearEq(b[0], 0.0f) && nearEq(b[1], 20.0f) && nearEq(b[2], 0.0f), "cur b = (0,20,0)");
    FX_CHECK(nearEq(pa[0], 1.0f) && nearEq(pa[1], 0.0f) && nearEq(pa[2], 0.0f), "prev a = (1,0,0)");
    FX_CHECK(nearEq(pb[0], 0.0f) && nearEq(pb[1], 2.0f) && nearEq(pb[2], 0.0f), "prev b = (0,2,0)");

    // Scalar replay at t = 0, 0.5, 1 (lerp prev -> cur per endpoint).
    const float ts[3] = {0.0f, 0.5f, 1.0f};
    const float expectX[3] = {1.0f, 5.5f, 10.0f};
    const float expectY[3] = {2.0f, 11.0f, 20.0f};
    for (int i = 0; i < 3; i++) {
        const float t0 = ts[i];
        const float ax = pa[0] + (a[0] - pa[0]) * t0;
        const float by = pb[1] + (b[1] - pb[1]) * t0;
        std::printf("SCALAR t=%g a.x=%g b.y=%g\n", t0, ax, by);
        FX_CHECK(nearEq(ax, expectX[i]), "scalar a.x at t");
        FX_CHECK(nearEq(by, expectY[i]), "scalar b.y at t");
    }

    std::printf("RESULT replay_ok\n");
    return 0;
}

// ---------------------------------------------------------------- negative

// Returns 0 only if the file is rejected by the real reader or by a semantic
// guard (unknown candidate label). Accepted-but-wrong content is a failure.
int checkNegative(const char *path) {
    FILE *f = std::fopen(path, "rb");
    if (f == nullptr) {
        std::printf("NEG_OPEN_FAIL %s\n", path);
        return 3;
    }
    static P09LocalPairSnapshot s;
    const bool ok = p09LocalPairSnapshotRead(f, s);
    std::fclose(f);
    if (!ok) {
        std::printf("NEG_REJECTED_BY_READER %s\n", path);
        std::printf("RESULT negative_rejected\n");
        return 0;
    }
    // Reader accepted the bytes. Semantic guards must still reject unknown labels.
    for (uint32_t pi = 0; pi < s.pairCount && pi < kP09LocalPairMaxPairs; pi++) {
        for (uint32_t wi = 0; wi < s.pairs[pi].worldCount; wi++) {
            const uint8_t label = s.pairs[pi].worlds[wi].candidateClass;
            if (label > P09LocalPairCandidateFeatherStatic) {
                std::printf("NEG_UNKNOWN_CANDIDATE_LABEL %u\n", unsigned(label));
                std::printf("RESULT negative_rejected\n");
                return 0;
            }
        }
    }
    std::printf("NEG_ACCEPTED %s (reader ok, no semantic reject)\n", path);
    std::printf("RESULT negative_NOT_rejected\n");
    return 1;
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(stderr);
        return 2;
    }
    const char *mode = argv[1];
    if (std::strcmp(mode, "--self-test") == 0) {
        std::printf("SELF_TEST_USAGE_ONLY: run separately:\n");
        usage(stdout);
        std::printf("RESULT not_run (no subprocess spawned; invoke each mode as its own process)\n");
        return 2;
    }
    if (argc < 3) {
        usage(stderr);
        return 2;
    }
    if (std::strcmp(mode, "--write-fixture") == 0) return writeFixture(argv[2]);
    if (std::strcmp(mode, "--write-mutant") == 0) {
        if (argc < 4) {
            usage(stderr);
            return 2;
        }
        return writeMutant(argv[2], argv[3]);
    }
    if (std::strcmp(mode, "--replay-file") == 0) return replayFile(argv[2]);
    if (std::strcmp(mode, "--check-negative") == 0) return checkNegative(argv[2]);
    usage(stderr);
    return 2;
}
