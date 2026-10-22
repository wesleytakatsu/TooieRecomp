//
// RT64
//
// P09 default-OFF audit of the per-transform WORLD interpolation decision.
// Enabled only when TOOIE_DIAG_P09_WORLD_LERP_AUDIT=<output path> is set.
// Records, per mapped WORLD transform, the RigidBody component flags and the
// geometric deviation of the lerped matrix from the endpoint chord of four
// probe points. No game/model/bone knowledge. Read-only: never alters the
// matrix handed to the renderer.
//

#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include <vector>

#include "common/rt64_math.h"
#include "hle/rt64_rigid_body.h"
#include "hle/rt64_workload.h"

namespace RT64 {
    struct P09WorldLerpAuditFrame {
        uint32_t mapped = 0;
        uint32_t translationHeld = 0;      // !lerpTranslation
        uint32_t translationHeldMoving = 0; // !lerpTranslation && |dT| > 1
        uint32_t overshoot = 0;            // lerp leaves the endpoint chord
        uint32_t invalidDecomp = 0;
        uint32_t decompMismatch = 0;       // stored prev decomposition != PREV matrix
        uint32_t coherenceForced = 0;      // held transforms forced to interpolate translation
        uint32_t guardElementwise = 0;     // transforms switched to elementwise by the closure guard
        uint32_t guardMicros = 0;          // wall time of the guard pass (prep + compute)
        uint32_t heldIsolated = 0;         // mapped, translation-held transforms outside any multi-slot set (original result kept)
        uint32_t heldToCurrent = 0;        // held transforms of multi-slot sets shown at CURRENT
        uint32_t gateReleased = 0;         // translation holds released because only the AUTO gate's direction-change amplification asked for them
        std::vector<uint8_t> transformFlags; // per WORLD index: bit0 mapped, bit1 lerpTranslation, bit2 coherence-forced, bit3 closure-guard elementwise
        std::vector<hlslpp::float4x4> altLerp; // per WORLD index: original (unguarded) RigidBody lerp (counterfactual)
        std::vector<uint8_t> altSet;
    };

    class P09WorldLerpAudit {
    public:
        static P09WorldLerpAudit &get() {
            static P09WorldLerpAudit inst;
            return inst;
        }

        bool enabled() const { return file != nullptr; }

        static hlslpp::float3 probe(const hlslpp::float4x4 &m, float x, float y, float z) {
            const hlslpp::float4 r = hlslpp::mul(hlslpp::float4(x, y, z, 1.0f), m);
            const float w = (std::fabs(float(r.w)) > 1e-12f) ? float(r.w) : 1.0f;
            return hlslpp::float3(float(r.x) / w, float(r.y) / w, float(r.z) / w);
        }

        static float len(const hlslpp::float3 &v) {
            return std::sqrt(float(v.x) * float(v.x) + float(v.y) * float(v.y) + float(v.z) * float(v.z));
        }

        // Distance from p to segment [a,b].
        static float distSegment(const hlslpp::float3 &p, const hlslpp::float3 &a, const hlslpp::float3 &b) {
            const hlslpp::float3 ab = b - a;
            const float abl2 = float(ab.x) * float(ab.x) + float(ab.y) * float(ab.y) + float(ab.z) * float(ab.z);
            float t = 0.0f;
            if (abl2 > 1e-12f) {
                const hlslpp::float3 ap = p - a;
                t = (float(ap.x) * float(ab.x) + float(ap.y) * float(ab.y) + float(ap.z) * float(ab.z)) / abl2;
                t = std::min(1.0f, std::max(0.0f, t));
            }
            return len(p - (a + ab * t));
        }

        void record(P09WorldLerpAuditFrame &frame, uint64_t workloadId, uint32_t t, uint32_t prevT, uint32_t group,
            float prevWeight, float curWeight, const hlslpp::float4x4 &prev, const hlslpp::float4x4 &cur,
            const RigidBody &rb, const RigidBody &originalBody, const hlslpp::float4x4 &lerped, bool elementwise)
        {
            frame.mapped++;
            if (frame.altLerp.size() <= t) {
                frame.altLerp.resize(t + 1);
                frame.altSet.resize(t + 1, 0);
            }
            // Counterfactual: the matrix the original per-transform RigidBody interpolation would have produced.
            frame.altLerp[t] = originalBody.lerp(curWeight, prev, cur, true);
            frame.altSet[t] = 1;
            const DecomposedTransform &dPrev = rb.transforms[rb.transformIndex ^ 1];
            const DecomposedTransform &dCur = rb.transforms[rb.transformIndex];
            if (!rb.lerpTranslation) frame.translationHeld++;
            if (rb.lerpDecompose && (!dPrev.valid || !dCur.valid)) frame.invalidDecomp++;

            const float dT = len(hlslpp::float3(cur[3].xyz) - hlslpp::float3(prev[3].xyz));
            if (!rb.lerpTranslation && dT > 1.0f) frame.translationHeldMoving++;

            constexpr float L = 64.0f;
            const float pts[4][3] = { { 0, 0, 0 }, { L, 0, 0 }, { 0, L, 0 }, { 0, 0, L } };
            float spread = 0.0f, over = 0.0f, ewdev = 0.0f;
            for (const auto &p : pts) {
                const hlslpp::float3 a = probe(prev, p[0], p[1], p[2]);
                const hlslpp::float3 b = probe(cur, p[0], p[1], p[2]);
                const hlslpp::float3 l = probe(lerped, p[0], p[1], p[2]);
                spread = std::max(spread, len(b - a));
                over = std::max(over, distSegment(l, a, b));
                ewdev = std::max(ewdev, len(l - (a + (b - a) * curWeight)));
            }

            float prevErr = -1.0f, curErr = -1.0f;
            if (rb.lerpDecompose && dPrev.valid && dCur.valid) {
                prevErr = matrixDifference(recomposeMatrix(dPrev.rotation, dPrev.scale, dPrev.skew, dPrev.translation, dPrev.perspective), prev);
                curErr = matrixDifference(recomposeMatrix(dCur.rotation, dCur.scale, dCur.skew, dCur.translation, dCur.perspective), cur);
                if (prevErr > 0.05f) frame.decompMismatch++;
            }

            const bool geomFlag = over > std::max(2.0f, 0.5f * spread);
            if (geomFlag) frame.overshoot++;
            const bool heldFlag = !rb.lerpTranslation && dT > 1.0f;
            if (!geomFlag && !heldFlag && prevErr <= 0.05f) return;

            std::lock_guard<std::mutex> lock(mu);
            if (rows >= RowCap) return;
            rows++;
            std::fprintf(file,
                "ROW wl=%llu t=%u prev=%u grp=%u wp=%.4f wc=%.4f ew=%d lerpT=%d lerpR=%d lerpS=%d lerpK=%d lerpP=%d dec=%d "
                "valid=%d%d flip=%d%d dT=%.4f spread=%.4f over=%.4f ewdev=%.4f vel=%.4f,%.4f,%.4f angVel=%.5f "
                "scaleP=%.5f,%.5f,%.5f scaleC=%.5f,%.5f,%.5f skewP=%.5f,%.5f,%.5f skewC=%.5f,%.5f,%.5f prevErr=%.5f curErr=%.5f",
                (unsigned long long)workloadId, t, prevT, group, prevWeight, curWeight, elementwise ? 1 : 0,
                rb.lerpTranslation ? 1 : 0, rb.lerpRotation ? 1 : 0, rb.lerpScale ? 1 : 0, rb.lerpSkew ? 1 : 0,
                rb.lerpPerspective ? 1 : 0, rb.lerpDecompose ? 1 : 0, dPrev.valid ? 1 : 0, dCur.valid ? 1 : 0,
                dPrev.coordinateFlip ? 1 : 0, dCur.coordinateFlip ? 1 : 0, dT, spread, over, ewdev,
                float(rb.linearVelocity.x), float(rb.linearVelocity.y), float(rb.linearVelocity.z), rb.angularVelocity,
                float(dPrev.scale.x), float(dPrev.scale.y), float(dPrev.scale.z),
                float(dCur.scale.x), float(dCur.scale.y), float(dCur.scale.z),
                float(dPrev.skew.x), float(dPrev.skew.y), float(dPrev.skew.z),
                float(dCur.skew.x), float(dCur.skew.y), float(dCur.skew.z), prevErr, curErr);
            auto dump = [&](const char *name, const hlslpp::float4x4 &m) {
                std::fprintf(file, " %s=", name);
                for (int i = 0; i < 4; i++)
                    for (int j = 0; j < 4; j++)
                        std::fprintf(file, "%s%.6g", (i | j) ? "," : "", float(m[i][j]));
            };
            dump("PREV", prev);
            dump("CUR", cur);
            dump("LERP", lerped);
            std::fprintf(file, "\n");
        }


        // Edge-stretch audit over every face of a workload. For each triangle edge the world-space length
        // under PREV, CUR and the interpolated matrices is compared; an edge that grows beyond both endpoint
        // lengths cannot come from a rigid in-between and is the rendered "rod" artefact.
        // Returns the number of cross-WORLD triangles the original (unguarded) interpolation would stretch.
        uint32_t auditTriangles(const P09WorldLerpAuditFrame &frame, uint64_t workloadId, float curWeight, const DrawData &d) {
            const size_t nWorld = d.lerpWorldTransforms.size();
            if (nWorld == 0 || d.endpointWorldTransforms.size() < nWorld * 2) return 0;
            const uint32_t nVert = d.vertexCount();
            auto world = [&](uint32_t v, int which) -> hlslpp::float3 {
                const uint32_t wi = d.worldIndices[v];
                const float x = d.posFloats[size_t(v) * 3 + 0], y = d.posFloats[size_t(v) * 3 + 1], z = d.posFloats[size_t(v) * 3 + 2];
                if (which == 3 && wi < frame.altSet.size() && frame.altSet[wi] != 0) {
                    return probe(frame.altLerp[wi], x, y, z);
                }
                const interop::float4x4 &m = (which == 0) ? d.endpointWorldTransforms[size_t(wi) * 2]
                    : (which == 1 ? d.endpointWorldTransforms[size_t(wi) * 2 + 1] : d.lerpWorldTransforms[wi]);
                return probe(*reinterpret_cast<const hlslpp::float4x4 *>(&m), x, y, z);
            };
            uint32_t faces = 0, cross = 0, crossBad = 0, intraBad = 0, detail = 0, altCrossBad = 0, altIntraBad = 0;
            float worstCross = 0.0f, worstIntra = 0.0f, baselineWorstCross = 0.0f;
            const size_t nFaceTri = d.faceIndices.size() / 3;
            for (size_t f = 0; f < nFaceTri; f++) {
                const uint32_t v[3] = { d.faceIndices[f * 3], d.faceIndices[f * 3 + 1], d.faceIndices[f * 3 + 2] };
                if (v[0] >= nVert || v[1] >= nVert || v[2] >= nVert) continue;
                const uint16_t w[3] = { d.worldIndices[v[0]], d.worldIndices[v[1]], d.worldIndices[v[2]] };
                if (w[0] >= nWorld || w[1] >= nWorld || w[2] >= nWorld) continue;
                faces++;
                const bool isCross = (w[0] != w[1]) || (w[1] != w[2]);
                if (isCross) cross++;
                hlslpp::float3 pp[3], pc[3], pl[3];
                for (int i = 0; i < 3; i++) { pp[i] = world(v[i], 0); pc[i] = world(v[i], 1); pl[i] = world(v[i], 2); }
                {
                    hlslpp::float3 pa[3];
                    for (int i = 0; i < 3; i++) pa[i] = world(v[i], 3);
                    float altWorst = 0.0f;
                    for (int e = 0; e < 3; e++) {
                        const int a = e, b = (e + 1) % 3;
                        const float lp = len(pp[a] - pp[b]), lc = len(pc[a] - pc[b]), la = len(pa[a] - pa[b]);
                        const float ref = std::max(std::max(lp, lc), 1e-3f);
                        if (la > 1.5f * ref && (la - ref) > 3.0f) altWorst = std::max(altWorst, la / ref);
                    }
                    if (altWorst > 0.0f) {
                        if (isCross) { altCrossBad++; baselineWorstCross = std::max(baselineWorstCross, altWorst); } else { altIntraBad++; }
                    }
                }
                float worst = 0.0f; int worstEdge = 0;
                for (int e = 0; e < 3; e++) {
                    const int a = e, b = (e + 1) % 3;
                    const float lp = len(pp[a] - pp[b]), lc = len(pc[a] - pc[b]), ll = len(pl[a] - pl[b]);
                    const float ref = std::max(std::max(lp, lc), 1e-3f);
                    if (ll > 1.5f * ref && (ll - ref) > 3.0f) {
                        const float ratio = ll / ref;
                        if (ratio > worst) { worst = ratio; worstEdge = e; }
                    }
                }
                if (worst <= 0.0f) continue;
                if (isCross) { crossBad++; worstCross = std::max(worstCross, worst); }
                else { intraBad++; worstIntra = std::max(worstIntra, worst); }
                if (detail < 6) {
                    detail++;
                    std::lock_guard<std::mutex> lock(mu);
                    if (triRows >= TriRowCap) continue;
                    triRows++;
                    std::fprintf(file, "TRI wl=%llu wc=%.4f face=%zu cross=%d ratio=%.3f edge=%d", (unsigned long long)workloadId, curWeight, f, isCross ? 1 : 0, worst, worstEdge);
                    for (int i = 0; i < 3; i++) {
                        const uint8_t fl = (w[i] < frame.transformFlags.size()) ? frame.transformFlags[w[i]] : 0;
                        std::fprintf(file, " v%d[w=%u map=%d lerpT=%d forcedT=%d guardE=%d local=%.3f,%.3f,%.3f P=%.3f,%.3f,%.3f C=%.3f,%.3f,%.3f L=%.3f,%.3f,%.3f]",
                            i, (unsigned)w[i], fl & 1, (fl >> 1) & 1, (fl >> 2) & 1, (fl >> 3) & 1,
                            d.posFloats[size_t(v[i]) * 3], d.posFloats[size_t(v[i]) * 3 + 1], d.posFloats[size_t(v[i]) * 3 + 2],
                            float(pp[i].x), float(pp[i].y), float(pp[i].z), float(pc[i].x), float(pc[i].y), float(pc[i].z),
                            float(pl[i].x), float(pl[i].y), float(pl[i].z));
                    }
                    std::fprintf(file, "\n");
                }
            }
            // Vertex overshoot: interpolated vertex farther from its endpoint chord than half the chord (+3 units).
            // Catches isolated fragments that drift without stretching any edge.
            uint32_t vertOver = 0, vertOverHeld = 0, vertOverDetail = 0;
            float worstVertOver = 0.0f;
            for (uint32_t v = 0; v < nVert; v++) {
                const uint16_t wi = d.worldIndices[v];
                if (wi >= nWorld || wi >= frame.transformFlags.size() || (frame.transformFlags[wi] & 1) == 0) continue;
                const hlslpp::float3 a = world(v, 0), b = world(v, 1), l = world(v, 2);
                const float chord = len(b - a);
                const float off = distSegment(l, a, b);
                if (off > 0.5f * chord + 3.0f) {
                    vertOver++;
                    if ((frame.transformFlags[wi] & 2) == 0) vertOverHeld++;
                    worstVertOver = std::max(worstVertOver, off);
                    if (vertOverDetail < 4) {
                        vertOverDetail++;
                        std::lock_guard<std::mutex> lock2(mu);
                        std::fprintf(file, "VOVER wl=%llu wc=%.4f v=%u w=%u lerpT=%d forcedT=%d guardE=%d chord=%.3f off=%.3f local=%.3f,%.3f,%.3f P=%.3f,%.3f,%.3f C=%.3f,%.3f,%.3f L=%.3f,%.3f,%.3f\n",
                            (unsigned long long)workloadId, curWeight, v, (unsigned)wi, (frame.transformFlags[wi] >> 1) & 1, (frame.transformFlags[wi] >> 2) & 1,
                            (frame.transformFlags[wi] >> 3) & 1, chord, off, d.posFloats[size_t(v) * 3], d.posFloats[size_t(v) * 3 + 1], d.posFloats[size_t(v) * 3 + 2],
                            float(a.x), float(a.y), float(a.z), float(b.x), float(b.y), float(b.z), float(l.x), float(l.y), float(l.z));
                    }
                }
            }
            std::lock_guard<std::mutex> lock(mu);
            std::fprintf(file, "VOVERSUM wl=%llu wc=%.4f vertices=%u overshoot=%u overshootHeldTranslation=%u worstOff=%.3f\n",
                (unsigned long long)workloadId, curWeight, nVert, vertOver, vertOverHeld, worstVertOver);
            std::fprintf(file, "TRISUM wl=%llu wc=%.4f faces=%u cross=%u crossStretched=%u intraStretched=%u worstCross=%.3f worstIntra=%.3f baselineCrossStretched=%u baselineIntraStretched=%u baselineWorstCross=%.3f coherenceForced=%u guardElementwise=%u guardMicros=%u\n",
                (unsigned long long)workloadId, curWeight, faces, cross, crossBad, intraBad, worstCross, worstIntra, altCrossBad, altIntraBad, baselineWorstCross, frame.coherenceForced, frame.guardElementwise, frame.guardMicros);
            return altCrossBad;
        }

        // Default-off (TOOIE_DIAG_P09_WORLD_LERP_DUMP=<dir>): binary dump of one workload so a CPU reconstruction can draw
        // the PREV, CUR, original-interpolation and guarded poses of the SAME frame. Little-endian, version 1:
        //   "P9WD" u32 version u64 workloadId f32 curWeight u32 nVert u32 nFace u32 nWorld u32 nViewProj
        //   u32 faces[3*nFace]; u32 worldIndex[nVert]; u32 viewProjIndex[nVert]; f32 local[3*nVert];
        //   per WORLD: f32 prev[16] cur[16] original[16] final[16] u32 flags; f32 viewProj[nViewProj][16].
        // Original = unguarded RigidBody lerp, final = matrix handed to the renderer (or the shadow guard result).
        bool dumpEnabled() const { return !dumpDir.empty(); }

        void dumpWorkload(const P09WorldLerpAuditFrame &frame, uint64_t workloadId, float curWeight, const DrawData &d) {
            if (dumpDir.empty()) return;
            const size_t nWorld = d.lerpWorldTransforms.size();
            const uint32_t nVert = d.vertexCount();
            if (nWorld == 0 || d.endpointWorldTransforms.size() < nWorld * 2 || d.viewProjIndices.size() < nVert) return;
            {
                std::lock_guard<std::mutex> lock(mu);
                if (dumps >= dumpMax) return;
                dumps++;
            }
            char path[1024];
            std::snprintf(path, sizeof(path), "%s/wl%llu_wc%.4f.p9wd", dumpDir.c_str(), (unsigned long long)workloadId, curWeight);
            FILE *out = std::fopen(path, "wb");
            if (out == nullptr) return;
            auto u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, out); };
            std::fwrite("P9WD", 1, 4, out);
            u32(1);
            std::fwrite(&workloadId, 8, 1, out);
            std::fwrite(&curWeight, 4, 1, out);
            const uint32_t nFace = uint32_t(d.faceIndices.size() / 3);
            const uint32_t nVP = uint32_t(d.viewProjTransforms.size());
            u32(nVert); u32(nFace); u32(uint32_t(nWorld)); u32(nVP);
            std::fwrite(d.faceIndices.data(), 4, size_t(nFace) * 3, out);
            for (uint32_t v = 0; v < nVert; v++) u32(d.worldIndices[v]);
            for (uint32_t v = 0; v < nVert; v++) u32(d.viewProjIndices[v]);
            std::fwrite(d.posFloats.data(), 4, size_t(nVert) * 3, out);
            for (size_t t = 0; t < nWorld; t++) {
                const bool set = t < frame.altSet.size() && frame.altSet[t] != 0;
                const interop::float4x4 &fin = d.lerpWorldTransforms[t];
                const interop::float4x4 &orig = set ? *reinterpret_cast<const interop::float4x4 *>(&frame.altLerp[t]) : fin;
                std::fwrite(&d.endpointWorldTransforms[t * 2], 64, 1, out);
                std::fwrite(&d.endpointWorldTransforms[t * 2 + 1], 64, 1, out);
                std::fwrite(&orig, 64, 1, out);
                std::fwrite(&fin, 64, 1, out);
                u32(t < frame.transformFlags.size() ? frame.transformFlags[t] : 0);
            }
            if (nVP > 0) std::fwrite(d.viewProjTransforms.data(), 64, nVP, out);
            std::fclose(out);
        }

        void endWorkload(const P09WorldLerpAuditFrame &frame, uint64_t workloadId, float prevWeight, float curWeight, bool prevValid) {
            std::lock_guard<std::mutex> lock(mu);
            if (frameLines >= FrameCap) return;
            frameLines++;
            std::fprintf(file, "FRAME wl=%llu wp=%.4f wc=%.4f prevValid=%d mapped=%u transHeld=%u transHeldMoving=%u overshoot=%u invalidDecomp=%u decompMismatch=%u coherenceForced=%u heldIsolated=%u heldToCurrent=%u gateReleased=%u\n",
                (unsigned long long)workloadId, prevWeight, curWeight, prevValid ? 1 : 0, frame.mapped, frame.translationHeld,
                frame.translationHeldMoving, frame.overshoot, frame.invalidDecomp, frame.decompMismatch, frame.coherenceForced,
                frame.heldIsolated, frame.heldToCurrent, frame.gateReleased);
            std::fflush(file);
        }

    private:
        static constexpr uint32_t RowCap = 400000;
        static constexpr uint32_t FrameCap = 400000;
        static constexpr uint32_t TriRowCap = 200000;
        uint32_t triRows = 0;
        FILE *file = nullptr;
        std::mutex mu;
        uint32_t rows = 0;
        uint32_t frameLines = 0;
        std::string dumpDir;
        uint32_t dumps = 0;
        uint32_t dumpMax = 600;

        P09WorldLerpAudit() {
            const char *path = std::getenv("TOOIE_DIAG_P09_WORLD_LERP_AUDIT");
            if (path != nullptr && path[0] != '\0') {
                file = std::fopen(path, "w");
            }
            const char *dir = std::getenv("TOOIE_DIAG_P09_WORLD_LERP_DUMP");
            if (file != nullptr && dir != nullptr && dir[0] != '\0') {
                dumpDir = dir;
            }
            const char *max = std::getenv("TOOIE_DIAG_P09_WORLD_LERP_DUMP_MAX");
            if (max != nullptr && max[0] != '\0') {
                dumpMax = uint32_t(std::strtoul(max, nullptr, 10));
            }
        }
    };
}
