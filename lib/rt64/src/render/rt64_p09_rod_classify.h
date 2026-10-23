//
// RT64
//
// P09 default-OFF diagnostic. TOOIE_DIAG_P09_ROD_CLASSIFY=<path> classifies every
// triangle of interpolated workloads whose rendered (final) WORLD positions stretch
// an edge to more than twice its CURRENT endpoint length (and by more than 4 units).
// Reference = CURRENT edge only: the PREV endpoint may come from a wrong pairing and
// must not mask the stretch. Categories per stretched triangle:
//   I = single WORLD; U = mixes unmapped and mapped WORLDs; P = some edge's PREV
//   endpoint length disagrees with CURRENT (pairing/geometry mismatch); C = mixes
//   WORLDs shown at CURRENT with interpolated ones; N = everything else
//   (interpolation law). One WL line per interpolated workload, up to 8 TRI lines.
// One OBJ line per connected mapped object (triangles plus inferred joints) of at least 20 WORLDs: vertex-centroid motion
// versus the straight PREV-to-CURRENT chord (along, perp), the per-vertex rms miss, and the first WORLD's origin motion
// (rmot) with its rendered fraction along it (rlerp, 0.5 at the midpoint frame).
// Read-only: never alters what is rendered.
//

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <vector>

#include "hle/rt64_game_frame.h"
#include "hle/rt64_workload.h"
#include "render/rt64_p09_spike_guard.h"

namespace RT64 {
namespace p09RodClassify {
    struct State {
        FILE *file = nullptr;
        std::mutex mu;
        uint32_t workloads = 0;
        uint32_t triRows = 0;
        uint32_t worldRows = 0;
        uint32_t objRows = 0;
        State() {
            const char *p = std::getenv("TOOIE_DIAG_P09_ROD_CLASSIFY");
            if ((p != nullptr) && (p[0] != '\0')) file = std::fopen(p, "w");
        }
    };

    inline State &state() { static State s; return s; }
    inline bool enabled() { return state().file != nullptr; }

    inline hlslpp::float3 xf(const hlslpp::float4x4 &m, const hlslpp::float3 &p) {
        const hlslpp::float4 r = hlslpp::mul(hlslpp::float4(p, 1.0f), m);
        return r.xyz;
    }

    inline float len3(const hlslpp::float3 &v) {
        return std::sqrt(float(v.x) * float(v.x) + float(v.y) * float(v.y) + float(v.z) * float(v.z));
    }

    inline bool sameMtx(const hlslpp::float4x4 &a, const hlslpp::float4x4 &b, float eps = 1e-4f) {
        for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) {
            const float x = a[i][j], y = b[i][j];
            if (std::fabs(x - y) > eps * std::max(1.0f, std::fabs(y))) return false;
        }
        return true;
    }

    // Per-WORLD final state codes.
    enum : uint8_t { ST_UNMAPPED = 0, ST_RIGID = 1, ST_CURRENT = 2, ST_ELEMENTWISE = 3, ST_OTHER = 4 };
    inline char stChar(uint8_t s) { return "URCEO"[s]; }

    inline void run(const Workload &workload, const GameFrameMap::WorkloadMap &workloadMap, const DrawData &prevDrawData,
        float curWeight)
    {
        State &S = state();
        if ((S.file == nullptr) || (curWeight >= 1.0f)) return;
        const DrawData &d = workload.drawData;
        const size_t nWorld = d.worldTransforms.size();
        if ((d.lerpWorldTransforms.size() != nWorld) || (workloadMap.transforms.size() < nWorld)) return;
        const uint32_t nVert = d.vertexCount();
        if ((d.worldIndices.size() < nVert) || (d.posFloats.size() < size_t(nVert) * 3)) return;
        const bool hasVel = d.velFloats.size() >= size_t(nVert) * 3;

        std::vector<uint8_t> st(nWorld, ST_UNMAPPED);
        std::vector<hlslpp::float4x4> prevM(nWorld);
        uint32_t nMapped = 0, nCur = 0, nEw = 0, nOther = 0, nHeldT = 0, nNoDecomp = 0;
        for (size_t t = 0; t < nWorld; t++) {
            const hlslpp::float4x4 cur = d.worldTransforms[t];
            prevM[t] = cur;
            const GameFrameMap::TransformMap &tm = workloadMap.transforms[t];
            if (!tm.mapped || (tm.prevTransformIndex >= prevDrawData.worldTransforms.size())) continue;
            nMapped++;
            prevM[t] = prevDrawData.worldTransforms[tm.prevTransformIndex];
            const hlslpp::float4x4 fin = d.lerpWorldTransforms[t];
            if (!tm.rigidBody.lerpTranslation) nHeldT++;
            if (!tm.rigidBody.lerpDecompose) nNoDecomp++;
            if (sameMtx(fin, tm.rigidBody.lerp(curWeight, prevM[t], cur, true))) st[t] = ST_RIGID;
            else if (sameMtx(fin, cur)) { st[t] = ST_CURRENT; nCur++; }
            else if (sameMtx(fin, lerpMatrix(prevM[t], cur, curWeight))) { st[t] = ST_ELEMENTWISE; nEw++; }
            else { st[t] = ST_OTHER; nOther++; }
        }

        auto local = [&](uint32_t v) {
            return hlslpp::float3(d.posFloats[size_t(v) * 3 + 0], d.posFloats[size_t(v) * 3 + 1], d.posFloats[size_t(v) * 3 + 2]);
        };
        auto vel = [&](uint32_t v) {
            if (!hasVel) return hlslpp::float3(0.0f, 0.0f, 0.0f);
            return hlslpp::float3(d.velFloats[size_t(v) * 3 + 0], d.velFloats[size_t(v) * 3 + 1], d.velFloats[size_t(v) * 3 + 2]);
        };

        // Category counters.
        // U = triangle mixes unmapped and mapped WORLDs.
        // P = some edge's PREV endpoint length disagrees with CURRENT (pairing/geometry mismatch).
        // C = mapped, mixes WORLDs shown at CURRENT with interpolated ones.
        // N = everything mapped, consistent endpoints, no CURRENT mix: interpolation law.
        uint32_t faces = 0, cross = 0, bad = 0, badU = 0, badP = 0, badC = 0, badN = 0, badIntra = 0, endpointP = 0;
        float worst = 0.0f;
        const size_t nTri = d.faceIndices.size() / 3;
        uint32_t detail = 0;
        for (size_t f = 0; f < nTri; f++) {
            const uint32_t v[3] = { d.faceIndices[f * 3], d.faceIndices[f * 3 + 1], d.faceIndices[f * 3 + 2] };
            if ((v[0] >= nVert) || (v[1] >= nVert) || (v[2] >= nVert)) continue;
            const uint32_t w[3] = { d.worldIndices[v[0]], d.worldIndices[v[1]], d.worldIndices[v[2]] };
            if ((w[0] >= nWorld) || (w[1] >= nWorld) || (w[2] >= nWorld)) continue;
            faces++;
            const bool isCross = (w[0] != w[1]) || (w[1] != w[2]);
            if (isCross) cross++;
            hlslpp::float3 pp[3], pc[3], pl[3];
            for (int i = 0; i < 3; i++) {
                const hlslpp::float3 lc = local(v[i]);
                const hlslpp::float3 lv = vel(v[i]);
                pc[i] = xf(d.worldTransforms[w[i]], lc);
                pp[i] = xf(prevM[w[i]], lc - lv);
                pl[i] = xf(d.lerpWorldTransforms[w[i]], lc - lv * (1.0f - curWeight));
            }
            float ratio = 0.0f, pairDev = 0.0f;
            for (int e = 0; e < 3; e++) {
                const int a = e, b = (e + 1) % 3;
                const float lc = len3(pc[a] - pc[b]), lp = len3(pp[a] - pp[b]), ll = len3(pl[a] - pl[b]);
                const float ref = std::max(lc, 1e-3f);
                if ((ll > 2.0f * ref) && ((ll - ref) > 4.0f)) ratio = std::max(ratio, ll / ref);
                if ((std::fabs(lp - lc) > 0.5f * lc + 3.0f)) pairDev = std::max(pairDev, std::fabs(lp - lc));
            }
            if (pairDev > 0.0f) endpointP++;
            if (ratio <= 0.0f) continue;
            bad++;
            worst = std::max(worst, ratio);
            bool anyU = false, anyM = false, anyC = false, anyI = false;
            for (int i = 0; i < 3; i++) {
                if (st[w[i]] == ST_UNMAPPED) anyU = true; else anyM = true;
                if (st[w[i]] == ST_CURRENT) anyC = true;
                else if (st[w[i]] != ST_UNMAPPED) anyI = true;
            }
            char cat;
            if (!isCross) { badIntra++; cat = 'I'; }
            else if (anyU && anyM) { badU++; cat = 'U'; }
            else if (pairDev > 0.0f) { badP++; cat = 'P'; }
            else if (anyC && anyI) { badC++; cat = 'C'; }
            else { badN++; cat = 'N'; }

            if (detail < 8) {
                std::lock_guard<std::mutex> lock(S.mu);
                if (S.triRows < 20000) {
                    S.triRows++;
                    detail++;
                    std::fprintf(S.file, "TRI wl=%llu wc=%.3f f=%zu cat=%c ratio=%.2f pairDev=%.2f",
                        (unsigned long long)workload.workloadId, curWeight, f, cat, ratio, pairDev);
                    for (int i = 0; i < 3; i++) {
                        const GameFrameMap::TransformMap &tm = workloadMap.transforms[w[i]];
                        const hlslpp::float3 lv = vel(v[i]);
                        std::fprintf(S.file, " v%d[w=%u pw=%d st=%c lT=%d vel=%.1f P=%.1f,%.1f,%.1f C=%.1f,%.1f,%.1f L=%.1f,%.1f,%.1f]",
                            i, w[i], tm.mapped ? int(tm.prevTransformIndex) : -1, stChar(st[w[i]]), tm.rigidBody.lerpTranslation ? 1 : 0,
                            len3(lv), float(pp[i].x), float(pp[i].y), float(pp[i].z), float(pc[i].x), float(pc[i].y), float(pc[i].z),
                            float(pl[i].x), float(pl[i].y), float(pl[i].z));
                    }
                    std::fprintf(S.file, "\n");
                }
            }
        }

        // Joint gaps on segmented models. A pair is a joint when the child's origin stays put (drift < 0.5) in the parent's
        // local frame between the two endpoints (CURRENT origins within 25 units). Its gap is the distance between the
        // child's rendered origin and the joint point carried by the parent's rendered matrix (local point interpolated).
        // JG rows (gap > 1.5 units) name both final states and held flags.
        uint32_t gaps = 0, joints = 0;
        float worstGap = 0.0f;
        {
            std::lock_guard<std::mutex> lock(S.mu);
            std::vector<hlslpp::float4x4> curInv(nWorld), prevInv(nWorld);
            std::vector<uint8_t> inv(nWorld, 0);
            for (size_t t = 0; t < nWorld; t++) {
                if (st[t] == ST_UNMAPPED) continue;
                const hlslpp::float4x4 cur = d.worldTransforms[t];
                if (!(std::fabs(hlslpp::determinant(cur)) > 1e-9f) || !(std::fabs(hlslpp::determinant(prevM[t])) > 1e-9f)) continue;
                curInv[t] = hlslpp::inverse(cur);
                prevInv[t] = hlslpp::inverse(prevM[t]);
                inv[t] = 1;
            }

            for (size_t c = 0; c < nWorld; c++) {
                if (!inv[c]) continue;
                const hlslpp::float3 cc = hlslpp::float4x4(d.worldTransforms[c])[3].xyz;
                const hlslpp::float3 cp = prevM[c][3].xyz;
                for (size_t p = 0; p < nWorld; p++) {
                    if ((p == c) || !inv[p]) continue;
                    if (len3(hlslpp::float3(hlslpp::float4x4(d.worldTransforms[p])[3].xyz) - cc) > 25.0f) continue;
                    const hlslpp::float3 lc = xf(curInv[p], cc);
                    const hlslpp::float3 lp = xf(prevInv[p], cp);
                    if (len3(lc - lp) >= 0.5f) continue;
                    joints++;
                    const hlslpp::float3 local = lp + (lc - lp) * curWeight;
                    const hlslpp::float3 joint = xf(d.lerpWorldTransforms[p], local);
                    const float gap = len3(hlslpp::float3(hlslpp::float4x4(d.lerpWorldTransforms[c])[3].xyz) - joint);
                    if (gap <= 1.5f) continue;
                    gaps++;
                    worstGap = std::max(worstGap, gap);
                    if (S.triRows < 40000) {
                        S.triRows++;
                        const GameFrameMap::TransformMap &tc = workloadMap.transforms[c];
                        const GameFrameMap::TransformMap &tp = workloadMap.transforms[p];
                        std::fprintf(S.file, "JG wl=%llu wc=%.3f c=%zu p=%zu stc=%c stp=%c lTc=%d lTp=%d gap=%.2f\n",
                            (unsigned long long)workload.workloadId, curWeight, c, p, stChar(st[c]), stChar(st[p]),
                            tc.rigidBody.lerpTranslation ? 1 : 0, tp.rigidBody.lerpTranslation ? 1 : 0, gap);
                    }
                }
            }
        }

        // Connected mapped WORLDs. A fully mapped triangle or an inferred joint unites WORLDs; a component of
        // >= 20 WORLDs is one articulated object. Sums use the distinct vertices of its triangles.
        std::vector<uint32_t> parent(nWorld);
        for (size_t t = 0; t < nWorld; t++) parent[t] = uint32_t(t);
        auto find = [&](uint32_t x) {
            while (parent[x] != x) {
                parent[x] = parent[parent[x]];
                x = parent[x];
            }
            return x;
        };
        auto unite = [&](uint32_t a, uint32_t b) {
            a = find(a);
            b = find(b);
            if (a != b) parent[b] = a;
        };
        for (size_t f = 0; f < nTri; f++) {
            const uint32_t v[3] = { d.faceIndices[f * 3], d.faceIndices[f * 3 + 1], d.faceIndices[f * 3 + 2] };
            if ((v[0] >= nVert) || (v[1] >= nVert) || (v[2] >= nVert)) continue;
            const uint32_t w[3] = { d.worldIndices[v[0]], d.worldIndices[v[1]], d.worldIndices[v[2]] };
            if ((w[0] >= nWorld) || (w[1] >= nWorld) || (w[2] >= nWorld)) continue;
            if ((st[w[0]] == ST_UNMAPPED) || (st[w[1]] == ST_UNMAPPED) || (st[w[2]] == ST_UNMAPPED)) continue;
            unite(w[0], w[1]);
            unite(w[0], w[2]);
        }

        // Segmented models have no triangle across their joints: join them the way the spike guard does.
        p09SpikeGuard::uniteJoints(d, workloadMap, prevDrawData, parent);

        struct ObjAcc {
            hlslpp::float3 sumP = hlslpp::float3(0.0f, 0.0f, 0.0f);
            hlslpp::float3 sumC = hlslpp::float3(0.0f, 0.0f, 0.0f);
            hlslpp::float3 sumL = hlslpp::float3(0.0f, 0.0f, 0.0f);
            double sumSq = 0.0;
            uint32_t verts = 0;
            uint32_t worlds = 0;
            uint32_t nR = 0, nC = 0, nE = 0, nO = 0, nH = 0;
            float rootMot = -1.0f, rootLerp = -1.0f;
        };
        std::vector<ObjAcc> acc(nWorld);
        for (size_t t = 0; t < nWorld; t++) {
            if (st[t] == ST_UNMAPPED) continue;
            const uint32_t r = find(uint32_t(t));
            ObjAcc &A = acc[r];
            if (A.worlds == 0) {
                // First drawn WORLD of the object (usually its root): origin motion and rendered fraction along it.
                const hlslpp::float4x4 m1 = d.worldTransforms[t], ml = d.lerpWorldTransforms[t];
                const hlslpp::float3 o0 = prevM[t][3].xyz, o1 = m1[3].xyz, ol = ml[3].xyz;
                const hlslpp::float3 dm = o1 - o0, dl = ol - o0;
                A.rootMot = len3(dm);
                const float mm = float(dm.x) * float(dm.x) + float(dm.y) * float(dm.y) + float(dm.z) * float(dm.z);
                A.rootLerp = (mm > 1e-4f) ? (float(dl.x) * float(dm.x) + float(dl.y) * float(dm.y) + float(dl.z) * float(dm.z)) / mm : -1.0f;
            }
            A.worlds++;
            if (st[t] == ST_RIGID) A.nR++;
            else if (st[t] == ST_CURRENT) A.nC++;
            else if (st[t] == ST_ELEMENTWISE) A.nE++;
            else if (st[t] == ST_OTHER) A.nO++;
            const GameFrameMap::TransformMap &tm = workloadMap.transforms[t];
            if (tm.mapped && !tm.rigidBody.lerpTranslation) A.nH++;
        }

        std::vector<uint8_t> seen(nVert);
        for (size_t f = 0; f < nTri; f++) {
            const uint32_t v[3] = { d.faceIndices[f * 3], d.faceIndices[f * 3 + 1], d.faceIndices[f * 3 + 2] };
            if ((v[0] >= nVert) || (v[1] >= nVert) || (v[2] >= nVert)) continue;
            const uint32_t w[3] = { d.worldIndices[v[0]], d.worldIndices[v[1]], d.worldIndices[v[2]] };
            if ((w[0] >= nWorld) || (w[1] >= nWorld) || (w[2] >= nWorld)) continue;
            if ((st[w[0]] == ST_UNMAPPED) || (st[w[1]] == ST_UNMAPPED) || (st[w[2]] == ST_UNMAPPED)) continue;
            const uint32_t r = find(w[0]);
            if (acc[r].worlds < 20) continue;
            for (int i = 0; i < 3; i++) {
                if (seen[v[i]]) continue;
                seen[v[i]] = 1;
                const hlslpp::float3 lc = local(v[i]);
                const hlslpp::float3 lv = vel(v[i]);
                const hlslpp::float3 pc = xf(d.worldTransforms[w[i]], lc);
                const hlslpp::float3 pp = xf(prevM[w[i]], lc - lv);
                const hlslpp::float3 pl = xf(d.lerpWorldTransforms[w[i]], lc - lv * (1.0f - curWeight));
                const hlslpp::float3 miss = pl - (pp + (pc - pp) * curWeight);
                ObjAcc &A = acc[r];
                A.sumP = A.sumP + pp;
                A.sumC = A.sumC + pc;
                A.sumL = A.sumL + pl;
                A.sumSq += double(float(miss.x)) * double(float(miss.x)) + double(float(miss.y)) * double(float(miss.y)) + double(float(miss.z)) * double(float(miss.z));
                A.verts++;
            }
        }

        std::lock_guard<std::mutex> lock(S.mu);
        if (gaps > 0) {
            std::fprintf(S.file, "JGW wl=%llu joints=%u gaps=%u worst=%.1f\n", (unsigned long long)workload.workloadId, joints, gaps, worstGap);
        }

        if (S.workloads < 100000) {
            S.workloads++;
            std::fprintf(S.file, "WL wl=%llu wc=%.3f worlds=%zu mapped=%u cur=%u ew=%u other=%u heldT=%u noDecomp=%u faces=%u cross=%u endpointP=%u bad=%u U=%u P=%u C=%u N=%u I=%u worst=%.2f\n",
                (unsigned long long)workload.workloadId, curWeight, nWorld, nMapped, nCur, nEw, nOther, nHeldT, nNoDecomp, faces, cross,
                endpointP, bad, badU, badP, badC, badN, badIntra, worst);
            for (size_t t = 0; t < nWorld; t++) {
                const ObjAcc &A = acc[t];
                if ((A.worlds < 20) || (A.verts == 0)) continue;
                const float inv = 1.0f / float(A.verts);
                const hlslpp::float3 P = A.sumP * inv;
                const hlslpp::float3 C = A.sumC * inv;
                const hlslpp::float3 L = A.sumL * inv;
                const hlslpp::float3 m = C - P;
                const float mot = len3(m);
                float along = -1.0f, perp = 0.0f;
                if (mot > 0.01f) {
                    const hlslpp::float3 rel = L - P;
                    const float dot = float(rel.x) * float(m.x) + float(rel.y) * float(m.y) + float(rel.z) * float(m.z);
                    along = dot / (mot * mot);
                    perp = len3(rel - m * along);
                }
                const float rms = float(std::sqrt(A.sumSq / double(A.verts)));
                if (S.objRows < 400000) {
                    S.objRows++;
                    std::fprintf(S.file, "OBJ wl=%llu wc=%.3f worlds=%u verts=%u mot=%.2f along=%.3f perp=%.2f rms=%.2f cx=%.1f cy=%.1f cz=%.1f R=%u C=%u E=%u O=%u H=%u rmot=%.2f rlerp=%.3f\n",
                        (unsigned long long)workload.workloadId, curWeight, A.worlds, A.verts, mot, along, perp, rms,
                        float(C.x), float(C.y), float(C.z), A.nR, A.nC, A.nE, A.nO, A.nH, A.rootMot, A.rootLerp);
                }
            }
            std::fflush(S.file);
        }
    }
};
};
