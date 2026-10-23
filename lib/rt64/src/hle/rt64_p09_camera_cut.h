//
// RT64
//
// P09 camera cut detection for untagged (G_EX_ID_AUTO) view matrices.
//
// GameFrame::matchScene maps an AUTO camera to the previous frame's camera unconditionally, so a camera cut (a cutscene
// changing shots, a camera reset, a warp or an area transition) is interpolated: the in-between frame shows the scene
// from a camera halfway between two unrelated shots, a one-frame flash the original game never draws. A view change
// larger than any continuous camera motion between two game frames is treated as a cut and the camera is not
// interpolated for that frame (it is shown at CURRENT). Thresholds: the camera turns by more than CutAngleDegrees or
// moves by more than CutDistance game units in one game frame. Default ON; TOOIE_TEST_P09_CAMERA_CUT=0 disables it.
//

#pragma once

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "common/rt64_hlslpp.h"

namespace RT64 {
namespace p09CameraCut {
    constexpr float CutAngleDegrees = 30.0f;
    constexpr float CutDistance = 250.0f;

    inline bool enabled() {
        static const bool on = [] {
            const char *e = std::getenv("TOOIE_TEST_P09_CAMERA_CUT");
            return (e == nullptr) || (e[0] == '\0') || (e[0] != '0');
        }();
        return on;
    }

    // Plain arithmetic so the rule can be unit tested without hlslpp: rows of the view 3x3 and camera positions.
    inline float rowCosine(const float a[3], const float b[3]) {
        const float la = std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
        const float lb = std::sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
        if (!(la > 1e-6f) || !(lb > 1e-6f)) {
            return 1.0f;
        }

        return (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) / (la * lb);
    }

    // Camera position of a row-vector view matrix [M3; t] (p_view = p_world * M3 + t) whose M3 is s times a rotation:
    // p = -(t * M3^T) / s^2. False when M3 is not a uniformly scaled rotation (row lengths differ by more than 5% or s ~ 0).
    inline bool cameraPosition(const float rows[3][3], const float t[3], float pos[3]) {
        float lengthSq[3];
        for (int r = 0; r < 3; r++) {
            lengthSq[r] = rows[r][0] * rows[r][0] + rows[r][1] * rows[r][1] + rows[r][2] * rows[r][2];
        }

        const float scaleSq = (lengthSq[0] + lengthSq[1] + lengthSq[2]) / 3.0f;
        if (!(scaleSq > 1e-8f) || !std::isfinite(scaleSq)) {
            return false;
        }

        for (int r = 0; r < 3; r++) {
            if (std::fabs(lengthSq[r] - scaleSq) > 0.1f * scaleSq) {
                return false;
            }
        }

        for (int c = 0; c < 3; c++) {
            // (t * M3^T)[c] = sum_k t[k] * M3[c][k]
            pos[c] = -(t[0] * rows[c][0] + t[1] * rows[c][1] + t[2] * rows[c][2]) / scaleSq;
        }

        return std::isfinite(pos[0]) && std::isfinite(pos[1]) && std::isfinite(pos[2]);
    }

    // prevRows/curRows: the three rows of each view 3x3; prevPos/curPos: camera positions in world space.
    inline bool isCut(const float prevRows[3][3], const float curRows[3][3], const float prevPos[3], const float curPos[3]) {
        const float minCosine = std::cos(CutAngleDegrees * 3.14159265358979f / 180.0f);
        for (int r = 0; r < 3; r++) {
            if (rowCosine(prevRows[r], curRows[r]) < minCosine) {
                return true;
            }
        }

        const float dx = curPos[0] - prevPos[0], dy = curPos[1] - prevPos[1], dz = curPos[2] - prevPos[2];
        return std::sqrt(dx * dx + dy * dy + dz * dz) > CutDistance;
    }

    inline bool isCut(const hlslpp::float4x4 &prevView, const hlslpp::float4x4 &curView) {
        float prevRows[3][3], curRows[3][3], prevPos[3], curPos[3];
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                prevRows[r][c] = prevView[r][c];
                curRows[r][c] = curView[r][c];
            }
        }

        // Only a rigid camera (3x3 = uniform scale times a rotation) is judged; HUD/orthographic or degenerate views are not.
        float prevT[3], curT[3];
        for (int c = 0; c < 3; c++) {
            prevT[c] = prevView[3][c];
            curT[c] = curView[3][c];
        }

        if (!cameraPosition(prevRows, prevT, prevPos) || !cameraPosition(curRows, curT, curPos)) {
            return false;
        }

        const bool cut = isCut(prevRows, curRows, prevPos, curPos);
        static const bool debug = std::getenv("TOOIE_DIAG_P09_CAMERA_CUT") != nullptr;
        if (cut && debug) {
            const float c0 = rowCosine(prevRows[0], curRows[0]), c1 = rowCosine(prevRows[1], curRows[1]), c2 = rowCosine(prevRows[2], curRows[2]);
            const float dx = curPos[0] - prevPos[0], dy = curPos[1] - prevPos[1], dz = curPos[2] - prevPos[2];
            std::fprintf(stderr, "[p09-cut] cos=%.3f,%.3f,%.3f dist=%.1f prevPos=%.1f,%.1f,%.1f curPos=%.1f,%.1f,%.1f\n", c0, c1, c2,
                std::sqrt(dx * dx + dy * dy + dz * dz), prevPos[0], prevPos[1], prevPos[2], curPos[0], curPos[1], curPos[2]);
        }

        return cut;
    }

    // Count of detected cuts, logged to stderr every 16 cuts so measurement runs can read it.
    inline void noteCut() {
        static std::atomic<uint32_t> cuts{ 0 };
        const uint32_t n = ++cuts;
        if ((n == 1) || ((n % 16) == 0)) {
            std::fprintf(stderr, "[tooie-interp] camera cuts=%u\n", n);
        }
    }
};
};
