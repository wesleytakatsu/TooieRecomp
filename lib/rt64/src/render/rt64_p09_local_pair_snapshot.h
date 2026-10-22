//
// RT64 - P09 PREV/CUR local-pair snapshot (pure CPU, default off).
//
// Copies original matrices and local positions before interpolation.
// Candidate class is a static source match. Snout and feather visual
// identity stay UNKNOWN. This list is not witness coverage.
//

#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "render/rt64_p09_exact_use_correlation.h"

namespace RT64 {
    constexpr uint32_t kP09LocalPairMaxWorlds = 16u;
    constexpr uint32_t kP09LocalPairMaxTris = 192u;
    constexpr uint32_t kP09LocalPairMaxPairs = 2u;
    constexpr uint32_t kP09LocalPairMaxBytes = 256u * 1024u;
    constexpr uint32_t kP09LocalPairMaxAttempts = 4u;

    // Diagnostic output directory (default-off feature, TOOIE_DIAG_P09_LOCAL_PAIR_SNAPSHOT): explicit override
    // TOOIE_DIAG_P09_LOCAL_PAIR_DIR, else the platform temp dir. Never a developer-specific absolute path.
    inline const std::string &p09LocalPairDir() {
        static const std::string dir = []() -> std::string {
            const char *v = std::getenv("TOOIE_DIAG_P09_LOCAL_PAIR_DIR");
            if (v == nullptr || v[0] == '\0') v = std::getenv("TMPDIR");
#ifdef _WIN32
            if (v == nullptr || v[0] == '\0') v = std::getenv("TEMP");
            if (v == nullptr || v[0] == '\0') v = std::getenv("TMP");
            std::string d = (v != nullptr && v[0] != '\0') ? v : ".";
#else
            std::string d = (v != nullptr && v[0] != '\0') ? v : "/tmp";
#endif
            while (d.size() > 1 && (d.back() == '/' || d.back() == '\\')) d.pop_back();
            return d;
        }();
        return dir;
    }

    inline const char *p09LocalPairArmMarkerDefault() {
        static const std::string path = p09LocalPairDir() + "/p09-local-pair-snapshot.arm";
        return path.c_str();
    }

    // Four attempts start only after an explicit scene arm. A busy or frozen slot is not shared.
    inline bool p09LocalPairShouldCapture(uint8_t sceneArmed, uint8_t slotBusy,
        uint8_t frozen, uint32_t attempts) {
        return sceneArmed != 0 && slotBusy == 0 && frozen == 0 && attempts < kP09LocalPairMaxAttempts;
    }

    inline int p09LocalPairOutputName(char *dst, size_t cap, long pid, uint64_t generation) {
        if (dst == nullptr || cap == 0) return -1;
        return std::snprintf(dst, cap, "%s/p09-local-pair-%ld-%llu.bin",
            p09LocalPairDir().c_str(), pid, static_cast<unsigned long long>(generation));
    }
    constexpr float kP09LocalPairEdgeFrac = 1.0e-4f;
    constexpr char kP09LocalPairMagic[8] = {'P', '0', '9', 'L', 'P', 'S', 'N', '1'};

    enum P09LocalPairReason : uint8_t {
        P09LocalPairReasonNone = 0,
        P09LocalPairReasonMissingMap,
        P09LocalPairReasonMapInvalid,
        P09LocalPairReasonUnmapped,
        P09LocalPairReasonNonFinite,
        P09LocalPairReasonMultiplicity,
        P09LocalPairReasonCapWorlds,
        P09LocalPairReasonCapTris,
        P09LocalPairReasonCapPairs,
        P09LocalPairReasonCapBytes,
        P09LocalPairReasonKeyInvalid,
        P09LocalPairReasonNoWitness,
        P09LocalPairReasonArmedWaiting,
        P09LocalPairReasonCount
    };

    enum P09LocalPairCandidate : uint8_t {
        P09LocalPairCandidateNone = 0,
        P09LocalPairCandidateSnoutStatic = 1,
        P09LocalPairCandidateFeatherStatic = 2
    };

    // Static source only: 0x62B node 0x11CB8 bone 11 anim 42, and feather
    // idx 26 anim 108 plus idx 30-35 anim 100-105. Not a visual name.
    inline uint8_t p09LocalPairCandidateClass(uint32_t resource, uint32_t nodeRel,
        uint32_t boneIndex, uint32_t boneAnim) {
        if (resource == 0x62Bu && nodeRel == 0x11CB8u && boneIndex == 11u && boneAnim == 42u) {
            return P09LocalPairCandidateSnoutStatic;
        }
        if (resource == 0x62Bu && boneIndex == 26u && boneAnim == 108u) {
            return P09LocalPairCandidateFeatherStatic;
        }
        if (resource == 0x62Bu && boneIndex >= 30u && boneIndex <= 35u &&
            boneAnim >= 100u && boneAnim <= 105u) {
            return P09LocalPairCandidateFeatherStatic;
        }
        return P09LocalPairCandidateNone;
    }

    inline const char *p09LocalPairReasonName(uint8_t reason) {
        switch (reason) {
        case P09LocalPairReasonNone: return "NONE";
        case P09LocalPairReasonMissingMap: return "MISSING_MAP";
        case P09LocalPairReasonMapInvalid: return "MAP_INVALID";
        case P09LocalPairReasonUnmapped: return "UNMAPPED";
        case P09LocalPairReasonNonFinite: return "NONFINITE";
        case P09LocalPairReasonMultiplicity: return "MULTIPLICITY";
        case P09LocalPairReasonCapWorlds: return "CAP_WORLDS";
        case P09LocalPairReasonCapTris: return "CAP_TRIS";
        case P09LocalPairReasonCapPairs: return "CAP_PAIRS";
        case P09LocalPairReasonCapBytes: return "CAP_BYTES";
        case P09LocalPairReasonKeyInvalid: return "KEY_INVALID";
        case P09LocalPairReasonNoWitness: return "NO_WITNESS";
        case P09LocalPairReasonArmedWaiting: return "ARMED_WAITING";
        default: return "UNKNOWN";
        }
    }

    // Snapshot-only runs the provenance setters. Heavy sway CSV stays off.
    inline bool p09LocalPairRunProvenance(bool swayEnabled, bool snapshotEnabled) {
        return swayEnabled || snapshotEnabled;
    }

    inline bool p09LocalPairRunHeavySway(bool swayEnabled) {
        return swayEnabled;
    }

    // rt64_transform_index belongs to the vertex-event workload only.
    inline bool p09LocalPairWitnessIndexMatch(uint64_t queryWorkload,
        uint64_t mappedWorkload, uint8_t vertexSeen) {
        return vertexSeen != 0 && queryWorkload != 0 && mappedWorkload == queryWorkload;
    }

    struct P09LocalPairWitnessIn {
        uint32_t worldIndex = 0;
        uint32_t resourceId = 0;
        uint32_t nodeRel = 0;
        uint32_t boneIndex = 0;
        uint32_t boneAnimId = 0;
        uint8_t provenanceKnown = 0;
        uint8_t candidateClass = 0;
    };

    struct P09LocalPairCallIn {
        uint32_t fbPair = 0;
        uint32_t projection = 0;
        uint32_t gameCall = 0;
        uint32_t faceStart = 0;
        uint32_t triangleCount = 0;
        uint32_t displayList = 0;
        uint8_t displayListKnown = 0;
    };

    struct P09LocalPairInput {
        uint64_t curWorkloadId = 0;
        uint64_t prevWorkloadId = 0;
        uint32_t prevRingIndex = 0;
        uint8_t frameMapped = 0;
        uint8_t mapInvalid = 0;
        uint32_t curWorldCount = 0;
        uint32_t prevWorldCount = 0;
        const uint8_t *worldMapped = nullptr;
        const uint32_t *prevTransformIndex = nullptr;
        const float *posFloats = nullptr;
        uint32_t posFloatCount = 0;
        const uint32_t *faceIndices = nullptr;
        uint32_t faceIndexCount = 0;
        const uint16_t *worldIndices = nullptr;
        uint32_t vertexCount = 0;
        const uint8_t *lightCounts = nullptr;
        uint32_t lightCountCount = 0;
        const uint16_t *lookAtIndices = nullptr;
        uint32_t lookAtCount = 0;
        const P09LocalPairWitnessIn *witnesses = nullptr;
        uint32_t witnessCount = 0;
        const P09LocalPairCallIn *calls = nullptr;
        uint32_t callCount = 0;
        void *matrixCtx = nullptr;
        void (*loadMatrix)(void *ctx, uint32_t worldIndex, int isPrev, float out[16]) = nullptr;
    };

    struct P09LocalPairWorld {
        uint32_t worldIndex = 0;
        uint32_t prevTransformIndex = 0;
        uint8_t mapped = 0;
        uint8_t provenanceKnown = 0;
        uint8_t candidateClass = 0;
        float cur[16] = {};
        float prev[16] = {};
        uint32_t resourceId = 0;
        uint32_t nodeRel = 0;
        uint32_t boneIndex = 0;
        uint32_t boneAnimId = 0;
    };

    struct P09LocalPairCorner {
        P09SeamExactUseKey key;
        float localPos[3] = {};
        uint16_t worldSlot = 0;
        uint8_t lightCount = 0;
        uint16_t lookAtIndex = 0;
        uint32_t displayList = 0;
        uint8_t displayListKnown = 0;
    };

    struct P09LocalPairEdge {
        uint8_t cornerA = 0;
        uint8_t cornerB = 0;
    };

    struct P09LocalPairTri {
        P09LocalPairCorner corner[3];
        P09LocalPairEdge edge[3];
        uint8_t candidateMask = 0;
    };

    struct P09LocalPairPair {
        uint64_t curWorkloadId = 0;
        uint64_t prevWorkloadId = 0;
        uint32_t prevRingIndex = 0;
        uint32_t worldCount = 0;
        uint32_t triCount = 0;
        P09LocalPairWorld worlds[kP09LocalPairMaxWorlds];
        P09LocalPairTri tris[kP09LocalPairMaxTris];
    };

    struct P09LocalPairSnapshot {
        uint8_t success = 0;
        uint8_t reason = P09LocalPairReasonNone;
        uint32_t reasonCounts[P09LocalPairReasonCount] = {};
        uint32_t pairCount = 0;
        uint32_t byteCount = 0;
        P09LocalPairPair pairs[kP09LocalPairMaxPairs];
    };

    static_assert(sizeof(P09LocalPairSnapshot) <= kP09LocalPairMaxBytes, "snapshot exceeds 256KiB");

    inline bool p09LocalPairFinite3(const float p[3]) {
        return std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2]);
    }

    inline bool p09LocalPairFinite16(const float m[16]) {
        for (int i = 0; i < 16; i++) {
            if (!std::isfinite(m[i])) return false;
        }
        return true;
    }

    // Row vector, translation in row 3: out = pos * M with w = 1.
    inline bool p09LocalPairRowVector(const float pos[3], const float m[16], float out[3]) {
        if (!p09LocalPairFinite3(pos) || !p09LocalPairFinite16(m)) return false;
        out[0] = pos[0] * m[0] + pos[1] * m[4] + pos[2] * m[8] + m[12];
        out[1] = pos[0] * m[1] + pos[1] * m[5] + pos[2] * m[9] + m[13];
        out[2] = pos[0] * m[2] + pos[1] * m[6] + pos[2] * m[10] + m[14];
        return p09LocalPairFinite3(out);
    }

    inline float p09LocalPairEdgeTol(const float a[3], const float b[3]) {
        const float dx = a[0] - b[0];
        const float dy = a[1] - b[1];
        const float dz = a[2] - b[2];
        return kP09LocalPairEdgeFrac * std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    inline void p09LocalPairFail(P09LocalPairSnapshot &s, uint8_t reason) {
        s.success = 0;
        s.reason = reason;
        if (reason < P09LocalPairReasonCount) s.reasonCounts[reason] += 1u;
        s.pairCount = 0;
    }

    inline void p09LocalPairSnapshotCloseEmpty(P09LocalPairSnapshot &s) {
        if (s.success != 0 && s.pairCount == 0) p09LocalPairFail(s, P09LocalPairReasonNoWitness);
    }

    inline void p09LocalPairMarkArmedWaiting(P09LocalPairSnapshot &s) {
        s.success = 0;
        s.reason = P09LocalPairReasonArmedWaiting;
        s.pairCount = 0;
        if (P09LocalPairReasonArmedWaiting < P09LocalPairReasonCount) {
            s.reasonCounts[P09LocalPairReasonArmedWaiting] += 1u;
        }
    }

    struct P09LocalPairArm {
        uint32_t attempts = 0;
        uint8_t frozen = 0;
        P09LocalPairSnapshot frame;
        P09LocalPairSnapshot held;
    };

    inline void p09LocalPairArmFinish(P09LocalPairArm &arm, P09LocalPairSnapshot &frame) {
        if (arm.frozen != 0) return;
        const bool complete = frame.success != 0 && frame.pairCount > 0;
        const bool hard = frame.success == 0 && frame.reason != P09LocalPairReasonNone &&
            frame.reason != P09LocalPairReasonArmedWaiting;
        if (!complete && !hard && arm.attempts >= kP09LocalPairMaxAttempts) {
            p09LocalPairSnapshotCloseEmpty(frame);
        }
        else if (!complete && !hard) {
            p09LocalPairMarkArmedWaiting(frame);
        }
        arm.held = frame;
        arm.frozen = (complete || hard || arm.attempts >= kP09LocalPairMaxAttempts) ? 1u : 0u;
    }

    inline uint32_t p09LocalPairPayloadChecksum(const void *data, uint32_t n) {
        const auto *bytes = static_cast<const unsigned char *>(data);
        uint32_t hash = 2166136261u;
        for (uint32_t i = 0; i < n; i++) {
            hash ^= bytes[i];
            hash *= 16777619u;
        }
        return hash;
    }

    inline int p09LocalPairUndirectedEdge(uint8_t a, uint8_t b) {
        if (a > 2u || b > 2u || a == b) return -1;
        const uint8_t lo = a < b ? a : b;
        const uint8_t hi = a < b ? b : a;
        if (lo == 0u && hi == 1u) return 0;
        if (lo == 1u && hi == 2u) return 1;
        if (lo == 0u && hi == 2u) return 2;
        return -1;
    }

    inline bool p09LocalPairSnapshotBoundsOk(const P09LocalPairSnapshot &s) {
        if (s.success > 1u) return false;
        if (s.reason >= P09LocalPairReasonCount) return false;
        if (s.pairCount > kP09LocalPairMaxPairs) return false;
        if (s.byteCount != uint32_t(sizeof(s))) return false;
        if (s.success == 1u && s.pairCount == 0u) return false;
        for (uint32_t pi = 0; pi < s.pairCount; pi++) {
            const P09LocalPairPair &pair = s.pairs[pi];
            if (pair.worldCount == 0u || pair.worldCount > kP09LocalPairMaxWorlds) return false;
            if (pair.triCount == 0u || pair.triCount > kP09LocalPairMaxTris) return false;
            uint32_t candidateCount = 0;
            for (uint32_t wi = 0; wi < pair.worldCount; wi++) {
                const P09LocalPairWorld &world = pair.worlds[wi];
                const uint8_t label = world.candidateClass;
                if (label > P09LocalPairCandidateFeatherStatic) return false;
                if (label != P09LocalPairCandidateNone) candidateCount += 1u;
                if (world.mapped > 1u || world.provenanceKnown > 1u) return false;
                if (!p09LocalPairFinite16(world.cur) || !p09LocalPairFinite16(world.prev)) return false;
            }
            if (candidateCount > pair.worldCount) return false;
            for (uint32_t ti = 0; ti < pair.triCount; ti++) {
                const P09LocalPairTri &tri = pair.tris[ti];
                uint8_t edgeSeen[3] = {};
                for (uint32_t c = 0; c < 3u; c++) {
                    const P09LocalPairCorner &corner = tri.corner[c];
                    if (corner.worldSlot >= pair.worldCount) return false;
                    if (!p09LocalPairFinite3(corner.localPos)) return false;
                    if (corner.key.workloadId != pair.curWorkloadId) return false;
                    if (corner.key.originalWorldIndex != pair.worlds[corner.worldSlot].worldIndex) return false;
                    if (corner.key.cornerInTriangle > 2u) return false;
                    if (corner.displayListKnown > 1u) return false;
                }
                for (uint32_t e = 0; e < 3u; e++) {
                    const int id = p09LocalPairUndirectedEdge(tri.edge[e].cornerA, tri.edge[e].cornerB);
                    if (id < 0 || edgeSeen[id] != 0u) return false;
                    edgeSeen[id] = 1u;
                }
            }
        }
        return true;
    }

    inline bool p09LocalPairSnapshotWrite(FILE *out, const P09LocalPairSnapshot &s) {
        if (out == nullptr) return false;
        const uint32_t bytes = uint32_t(sizeof(s));
        if (bytes == 0u || bytes > kP09LocalPairMaxBytes) return false;
        if (std::fwrite(kP09LocalPairMagic, 1, 8, out) != 8) return false;
        if (std::fwrite(&bytes, sizeof(bytes), 1, out) != 1) return false;
        if (std::fwrite(&s, 1, bytes, out) != bytes) return false;
        const uint32_t sum = p09LocalPairPayloadChecksum(&s, bytes);
        return std::fwrite(&sum, sizeof(sum), 1, out) == 1;
    }

    inline bool p09LocalPairSnapshotPublish(const char *path, const P09LocalPairSnapshot &s) {
        if (path == nullptr || path[0] == '\0') return false;
        char tmp[512];
        const int n = std::snprintf(tmp, sizeof(tmp), "%s.tmp", path);
        if (n <= 0 || size_t(n) >= sizeof(tmp)) return false;
        FILE *out = std::fopen(tmp, "wb");
        if (out == nullptr) return false;
        bool ok = p09LocalPairSnapshotWrite(out, s);
        if (ok && std::fflush(out) != 0) ok = false;
        if (std::fclose(out) != 0) ok = false;
        if (!ok) {
            std::remove(tmp);
            return false;
        }
        if (std::rename(tmp, path) != 0) {
            std::remove(tmp);
            return false;
        }
        return true;
    }

    inline bool p09LocalPairSnapshotRead(FILE *in, P09LocalPairSnapshot &s) {
        if (in == nullptr) return false;
        char magic[8];
        uint32_t bytes = 0;
        if (std::fread(magic, 1, 8, in) != 8) return false;
        if (std::memcmp(magic, kP09LocalPairMagic, 8) != 0) return false;
        if (std::fread(&bytes, sizeof(bytes), 1, in) != 1) return false;
        if (bytes != uint32_t(sizeof(s)) || bytes > kP09LocalPairMaxBytes) return false;
        if (std::fread(&s, 1, bytes, in) != bytes) return false;
        uint32_t sum = 0;
        if (std::fread(&sum, sizeof(sum), 1, in) != 1) return false;
        if (sum != p09LocalPairPayloadChecksum(&s, bytes)) return false;
        if (std::fgetc(in) != EOF) return false;
        return p09LocalPairSnapshotBoundsOk(s);
    }

    inline void p09LocalPairSnapshotReset(P09LocalPairSnapshot &s) {
        s.success = 1;
        s.reason = P09LocalPairReasonNone;
        for (uint32_t i = 0; i < P09LocalPairReasonCount; i++) s.reasonCounts[i] = 0;
        s.pairCount = 0;
        s.byteCount = uint32_t(sizeof(P09LocalPairSnapshot));
        if (s.byteCount > kP09LocalPairMaxBytes) p09LocalPairFail(s, P09LocalPairReasonCapBytes);
    }

    inline uint64_t p09LocalPairPrevWorkloadId(const P09LocalPairSnapshot &s, uint32_t pairIndex) {
        if (!p09LocalPairSnapshotBoundsOk(s) || pairIndex >= s.pairCount) return 0;
        return s.pairs[pairIndex].prevWorkloadId;
    }

    inline bool p09LocalPairReplayCorner(const P09LocalPairSnapshot &s, uint32_t pairIndex,
        uint32_t triIndex, uint32_t corner, int isPrev, float out[3]) {
        if (!p09LocalPairSnapshotBoundsOk(s) || s.success == 0 || pairIndex >= s.pairCount) return false;
        const P09LocalPairPair &pair = s.pairs[pairIndex];
        if (triIndex >= pair.triCount || corner > 2u) return false;
        const P09LocalPairCorner &c = pair.tris[triIndex].corner[corner];
        if (c.worldSlot >= pair.worldCount) return false;
        const P09LocalPairWorld &w = pair.worlds[c.worldSlot];
        return p09LocalPairRowVector(c.localPos, isPrev ? w.prev : w.cur, out);
    }

    inline bool p09LocalPairReplayEdge(const P09LocalPairSnapshot &s, uint32_t pairIndex,
        uint32_t triIndex, uint32_t edge, int isPrev, float a[3], float b[3]) {
        if (!p09LocalPairSnapshotBoundsOk(s) || s.success == 0 || pairIndex >= s.pairCount) return false;
        const P09LocalPairPair &pair = s.pairs[pairIndex];
        if (triIndex >= pair.triCount || edge > 2u) return false;
        const P09LocalPairEdge &e = pair.tris[triIndex].edge[edge];
        return p09LocalPairReplayCorner(s, pairIndex, triIndex, e.cornerA, isPrev, a) &&
            p09LocalPairReplayCorner(s, pairIndex, triIndex, e.cornerB, isPrev, b);
    }

    inline bool p09LocalPairIsWitness(const P09LocalPairInput &in, uint32_t world) {
        if (in.witnesses == nullptr) return false;
        for (uint32_t i = 0; i < in.witnessCount; i++) {
            if (in.witnesses[i].worldIndex == world) return true;
        }
        return false;
    }

    inline int p09LocalPairFindWorld(const P09LocalPairPair &pair, uint32_t world) {
        for (uint32_t i = 0; i < pair.worldCount; i++) {
            if (pair.worlds[i].worldIndex == world) return int(i);
        }
        return -1;
    }

    inline bool p09LocalPairKeyDup(const P09LocalPairPair &pair, uint32_t triCount,
        const P09SeamExactUseKey &key) {
        for (uint32_t t = 0; t < triCount; t++) {
            for (uint32_t c = 0; c < 3u; c++) {
                if (p09ExactUseEqual(pair.tris[t].corner[c].key, key)) return true;
            }
        }
        return false;
    }

    inline bool p09LocalPairCopyWorld(P09LocalPairSnapshot &s, const P09LocalPairInput &in,
        P09LocalPairPair &dst, uint32_t world, const P09LocalPairWitnessIn *prov) {
        if (p09LocalPairFindWorld(dst, world) >= 0) return true;
        if (dst.worldCount >= kP09LocalPairMaxWorlds) {
            p09LocalPairFail(s, P09LocalPairReasonCapWorlds);
            return false;
        }
        if (world >= in.curWorldCount || in.worldMapped == nullptr || in.prevTransformIndex == nullptr) {
            p09LocalPairFail(s, P09LocalPairReasonMapInvalid);
            return false;
        }
        if (in.worldMapped[world] == 0) {
            p09LocalPairFail(s, P09LocalPairReasonUnmapped);
            return false;
        }
        const uint32_t prevIndex = in.prevTransformIndex[world];
        if (prevIndex >= in.prevWorldCount) {
            p09LocalPairFail(s, P09LocalPairReasonMapInvalid);
            return false;
        }
        if (in.loadMatrix == nullptr) {
            p09LocalPairFail(s, P09LocalPairReasonMissingMap);
            return false;
        }
        P09LocalPairWorld &slot = dst.worlds[dst.worldCount];
        slot = P09LocalPairWorld();
        slot.worldIndex = world;
        slot.prevTransformIndex = prevIndex;
        slot.mapped = 1;
        in.loadMatrix(in.matrixCtx, world, 0, slot.cur);
        in.loadMatrix(in.matrixCtx, world, 1, slot.prev);
        if (!p09LocalPairFinite16(slot.cur) || !p09LocalPairFinite16(slot.prev)) {
            p09LocalPairFail(s, P09LocalPairReasonNonFinite);
            return false;
        }
        if (prov != nullptr && prov->provenanceKnown != 0) {
            slot.provenanceKnown = 1;
            slot.resourceId = prov->resourceId;
            slot.nodeRel = prov->nodeRel;
            slot.boneIndex = prov->boneIndex;
            slot.boneAnimId = prov->boneAnimId;
            slot.candidateClass = prov->candidateClass;
        }
        dst.worldCount += 1u;
        return true;
    }

    inline const P09LocalPairWitnessIn *p09LocalPairWitnessOf(const P09LocalPairInput &in, uint32_t world) {
        if (in.witnesses == nullptr) return nullptr;
        for (uint32_t i = 0; i < in.witnessCount; i++) {
            if (in.witnesses[i].worldIndex == world) return &in.witnesses[i];
        }
        return nullptr;
    }

    inline bool p09LocalPairReadCorner(P09LocalPairSnapshot &s, const P09LocalPairInput &in,
        const P09LocalPairCallIn &call, uint32_t tri, uint32_t corner,
        uint32_t &vertex, uint16_t &world, float pos[3], uint8_t &light, uint16_t &look) {
        if (call.triangleCount > (0xFFFFFFFFu / 3u)) {
            p09LocalPairFail(s, P09LocalPairReasonKeyInvalid);
            return false;
        }
        const uint64_t face = uint64_t(call.faceStart) + uint64_t(tri) * 3u + corner;
        if (in.faceIndices == nullptr || face >= in.faceIndexCount) {
            p09LocalPairFail(s, P09LocalPairReasonKeyInvalid);
            return false;
        }
        vertex = in.faceIndices[uint32_t(face)];
        if (in.worldIndices == nullptr || vertex >= in.vertexCount) {
            p09LocalPairFail(s, P09LocalPairReasonKeyInvalid);
            return false;
        }
        world = in.worldIndices[vertex];
        if (uint32_t(world) >= in.curWorldCount) {
            p09LocalPairFail(s, P09LocalPairReasonMapInvalid);
            return false;
        }
        const uint32_t p = vertex * 3u;
        if (in.posFloats == nullptr || (p + 2u) >= in.posFloatCount ||
            in.lightCounts == nullptr || vertex >= in.lightCountCount ||
            in.lookAtIndices == nullptr || vertex >= in.lookAtCount) {
            p09LocalPairFail(s, P09LocalPairReasonMissingMap);
            return false;
        }
        pos[0] = in.posFloats[p];
        pos[1] = in.posFloats[p + 1u];
        pos[2] = in.posFloats[p + 2u];
        if (!p09LocalPairFinite3(pos)) {
            p09LocalPairFail(s, P09LocalPairReasonNonFinite);
            return false;
        }
        light = in.lightCounts[vertex];
        look = in.lookAtIndices[vertex];
        return true;
    }

    inline bool p09LocalPairSnapshotAdd(P09LocalPairSnapshot &s, const P09LocalPairInput &in) {
        if (s.success == 0) return false;
        if (in.witnessCount == 0) return true;
        if (in.witnessCount > kP09LocalPairMaxWorlds) {
            p09LocalPairFail(s, P09LocalPairReasonCapWorlds);
            return false;
        }
        if (s.pairCount >= kP09LocalPairMaxPairs) {
            p09LocalPairFail(s, P09LocalPairReasonCapPairs);
            return false;
        }
        if (in.witnesses == nullptr) {
            p09LocalPairFail(s, P09LocalPairReasonMissingMap);
            return false;
        }
        for (uint32_t i = 0; i < in.witnessCount; i++) {
            for (uint32_t j = i + 1u; j < in.witnessCount; j++) {
                if (in.witnesses[i].worldIndex == in.witnesses[j].worldIndex) {
                    p09LocalPairFail(s, P09LocalPairReasonMultiplicity);
                    return false;
                }
            }
        }
        if (in.mapInvalid != 0) {
            p09LocalPairFail(s, P09LocalPairReasonMapInvalid);
            return false;
        }
        if (in.frameMapped == 0 || in.curWorkloadId == 0 || in.prevWorkloadId == 0 ||
            in.loadMatrix == nullptr || in.worldMapped == nullptr || in.prevTransformIndex == nullptr) {
            p09LocalPairFail(s, P09LocalPairReasonMissingMap);
            return false;
        }

        P09LocalPairPair &dst = s.pairs[s.pairCount];
        dst = P09LocalPairPair();
        dst.curWorkloadId = in.curWorkloadId;
        dst.prevWorkloadId = in.prevWorkloadId;
        dst.prevRingIndex = in.prevRingIndex;
        for (uint32_t i = 0; i < in.witnessCount; i++) {
            if (!p09LocalPairCopyWorld(s, in, dst, in.witnesses[i].worldIndex, &in.witnesses[i])) return false;
        }

        const P09LocalPairCallIn *calls = in.calls;
        const uint32_t callCount = calls == nullptr ? 0u : in.callCount;
        for (uint32_t ci = 0; ci < callCount; ci++) {
            const P09LocalPairCallIn &call = calls[ci];
            const uint32_t triBegin = dst.triCount;
            uint8_t multi = 0;
            uint32_t firstWorld = 0xFFFFFFFFu;
            for (uint32_t t = 0; t < call.triangleCount; t++) {
                uint32_t vertex[3];
                uint16_t world[3];
                float pos[3][3];
                uint8_t light[3];
                uint16_t look[3];
                bool witness = false;
                for (uint32_t c = 0; c < 3u; c++) {
                    if (!p09LocalPairReadCorner(s, in, call, t, c, vertex[c], world[c], pos[c], light[c], look[c])) {
                        return false;
                    }
                    if (firstWorld == 0xFFFFFFFFu) firstWorld = world[c];
                    else if (world[c] != firstWorld) multi = 1;
                    if (p09LocalPairIsWitness(in, world[c])) witness = true;
                }
                if (!witness) continue;
                if (dst.triCount >= kP09LocalPairMaxTris) {
                    p09LocalPairFail(s, P09LocalPairReasonCapTris);
                    return false;
                }
                P09LocalPairTri &tri = dst.tris[dst.triCount];
                tri = P09LocalPairTri();
                tri.edge[0] = P09LocalPairEdge{0, 1};
                tri.edge[1] = P09LocalPairEdge{1, 2};
                tri.edge[2] = P09LocalPairEdge{2, 0};
                for (uint32_t c = 0; c < 3u; c++) {
                    if (!p09LocalPairCopyWorld(s, in, dst, world[c], p09LocalPairWitnessOf(in, world[c]))) return false;
                    const int slot = p09LocalPairFindWorld(dst, world[c]);
                    if (slot < 0) {
                        p09LocalPairFail(s, P09LocalPairReasonMapInvalid);
                        return false;
                    }
                    P09ExactUseKeyInput keyIn;
                    keyIn.workloadId = in.curWorkloadId;
                    keyIn.fbPair = call.fbPair;
                    keyIn.projection = call.projection;
                    keyIn.gameCall = call.gameCall;
                    keyIn.triangleLocal = t;
                    keyIn.cornerInTriangle = c;
                    keyIn.faceStart = call.faceStart;
                    keyIn.storedVertexId = vertex[c];
                    keyIn.worldIndex = world[c];
                    keyIn.vertexCount = in.vertexCount;
                    P09LocalPairCorner &corner = tri.corner[c];
                    if (!p09ExactUseKeyMake(keyIn, corner.key) ||
                        p09LocalPairKeyDup(dst, dst.triCount, corner.key)) {
                        p09LocalPairFail(s, p09ExactUseKeyMake(keyIn, corner.key)
                            ? P09LocalPairReasonMultiplicity : P09LocalPairReasonKeyInvalid);
                        return false;
                    }
                    corner.localPos[0] = pos[c][0];
                    corner.localPos[1] = pos[c][1];
                    corner.localPos[2] = pos[c][2];
                    corner.worldSlot = uint16_t(slot);
                    corner.lightCount = light[c];
                    corner.lookAtIndex = look[c];
                    corner.displayList = call.displayList;
                    corner.displayListKnown = call.displayListKnown;
                }
                dst.triCount += 1u;
            }
            for (uint32_t t = triBegin; t < dst.triCount; t++) dst.tris[t].candidateMask = multi;
        }
        s.pairCount += 1u;
        return true;
    }
}
