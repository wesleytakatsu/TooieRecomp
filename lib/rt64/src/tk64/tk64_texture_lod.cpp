//
// TK64
//
// Copyright (c) 2026 The TK64 contributors. Licensed under the MIT license. See LICENSE file for details.
//

#include "tk64_texture_lod.h"
#include "tk64_texture_residency.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace tk64 {
    static const char *textureLodReportPath() {
        static const char *path = []() -> const char * {
            const char *value = std::getenv("TK64_DIAG_TEXTURE_LOD");
            return ((value != nullptr) && (value[0] != '\0')) ? value : nullptr;
        }();

        return path;
    }

    static std::atomic<uint64_t> textureStreamEvents[3] = {};

    void countTextureStreamEvent(TextureStreamEvent event) {
        textureStreamEvents[int(event)].fetch_add(1);
    }

    bool textureLodEnabled() {
        return textureLodReportPath() != nullptr;
    }

    uint32_t textureTestFirstMip() {
        static const uint32_t firstMip = []() -> uint32_t {
            const char *value = std::getenv("TK64_TEST_TEXTURE_FIRST_MIP");
            if ((value == nullptr) || (value[0] == '\0')) {
                return 0;
            }

            const long parsed = std::strtol(value, nullptr, 10);
            return uint32_t(std::clamp(parsed, 0L, 15L));
        }();

        return firstMip;
    }

    static std::atomic<uint64_t> textureAllocationCounter{ 0 };
    static std::atomic<uint64_t> textureAllocationFailureCount{ 0 };

    bool textureAllocationTestFailure() {
        static const uint64_t every = []() -> uint64_t {
            const char *value = std::getenv("TK64_TEST_FAIL_TEXTURE_ALLOC");
            if ((value == nullptr) || (value[0] == '\0')) {
                return 0;
            }

            const long long parsed = std::strtoll(value, nullptr, 10);
            return (parsed > 0) ? uint64_t(parsed) : 0;
        }();

        if (every == 0) {
            return false;
        }

        return ((textureAllocationCounter.fetch_add(1) + 1) % every) == 0;
    }

    void countTextureAllocationFailure() {
        textureAllocationFailureCount.fetch_add(1);
    }

    uint64_t textureAllocationFailures() {
        return textureAllocationFailureCount.load();
    }

    uint32_t textureFirstMip(uint32_t requested, uint32_t mipCount) {
        if (mipCount <= 1) {
            return 0;
        }

        return std::min(requested, mipCount - 1);
    }

    // Level of detail of a pixel footprint given as the four derivatives of the texture coordinates per pixel.
    static float footprintLod(float dudx, float dudy, float dvdx, float dvdy, float maxAnisotropy) {
        // Singular values of the 2x2 Jacobian: the longest and shortest axis of the footprint in texels.
        const float e = 0.5f * (dudx + dvdy);
        const float f = 0.5f * (dudx - dvdy);
        const float g = 0.5f * (dvdx + dudy);
        const float h = 0.5f * (dvdx - dudy);
        const float q = std::sqrt(e * e + h * h);
        const float r = std::sqrt(f * f + g * g);
        const float sigmaMax = q + r;
        const float sigmaMin = std::fabs(q - r);
        if (!(sigmaMax > 1e-12f) || !std::isfinite(sigmaMax)) {
            return TextureLodFull;
        }

        // Anisotropic filtering samples along the long axis and picks the mip from the long axis divided by the number
        // of samples, never finer than the short axis.
        const float ratio = sigmaMax / std::max(sigmaMin, 1e-12f);
        const float sigma = sigmaMax / std::min(std::max(maxAnisotropy, 1.0f), ratio);
        return std::log2(sigma);
    }

    struct ClipVertex {
        float x, y, z, w;
    };

    // Sutherland-Hodgman against one plane, given as the signed distance d(vertex) >= 0 inside.
    template<typename Distance>
    static int clipPolygon(const ClipVertex *in, int inCount, ClipVertex *out, Distance distance) {
        int outCount = 0;
        for (int i = 0; i < inCount; i++) {
            const ClipVertex &a = in[i];
            const ClipVertex &b = in[(i + 1) % inCount];
            const float da = distance(a);
            const float db = distance(b);
            if (da >= 0.0f) {
                out[outCount++] = a;
            }

            if ((da >= 0.0f) != (db >= 0.0f)) {
                const float s = da / (da - db);
                out[outCount++] = { a.x + (b.x - a.x) * s, a.y + (b.y - a.y) * s, a.z + (b.z - a.z) * s, a.w + (b.w - a.w) * s };
            }
        }

        return outCount;
    }

    // Solves M * k = rhs for the 3x3 matrix whose rows are (x_i, y_i, w_i). Returns false when M is singular.
    static bool solvePlane(const double m[3][3], const double rhs[3], double k[3]) {
        const double det =
            m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
            m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
            m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
        if (!(std::fabs(det) > 1e-12)) {
            return false;
        }

        const double invDet = 1.0 / det;
        for (int c = 0; c < 3; c++) {
            double a[3][3];
            for (int r = 0; r < 3; r++) {
                for (int j = 0; j < 3; j++) {
                    a[r][j] = (j == c) ? rhs[r] : m[r][j];
                }
            }

            k[c] = (a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) -
                    a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) +
                    a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0])) * invDet;
        }

        return true;
    }

    float triangleTexelLod(const float clip[3][4], const float u[3], const float v[3], float viewportScaleX, float viewportScaleY, float maxAnisotropy) {
        // Texture coordinates that do not vary are what texture generation leaves on the CPU: unknown, so full.
        if ((u[0] == u[1]) && (u[1] == u[2]) && (v[0] == v[1]) && (v[1] == v[2])) {
            return TextureLodFull;
        }

        // 1/w, u/w and v/w are affine in normalized device coordinates (X, Y) = (x/w, y/w): a/w = kx X + ky Y + kw. With
        // homogeneous vertices that is kx x_i + ky y_i + kw w_i = a_i, which holds for vertices behind the camera too.
        const double m[3][3] = {
            { clip[0][0], clip[0][1], clip[0][3] },
            { clip[1][0], clip[1][1], clip[1][3] },
            { clip[2][0], clip[2][1], clip[2][3] },
        };

        const double one[3] = { 1.0, 1.0, 1.0 };
        const double us[3] = { u[0], u[1], u[2] };
        const double vs[3] = { v[0], v[1], v[2] };
        double kq[3], ku[3], kv[3];
        if (!solvePlane(m, one, kq) || !solvePlane(m, us, ku) || !solvePlane(m, vs, kv)) {
            // The plane of the triangle goes through the eye: seen edge-on, no area.
            return TextureLodNone;
        }

        // Visible part: near plane (z >= -w), in front of the eye (w > 0) and the image with a guard band.
        ClipVertex polygonA[16], polygonB[16];
        int count = 3;
        for (int i = 0; i < 3; i++) {
            polygonA[i] = { clip[i][0], clip[i][1], clip[i][2], clip[i][3] };
        }

        const float GuardX = TextureLodGuardX;
        count = clipPolygon(polygonA, count, polygonB, [](const ClipVertex &c) { return c.z + c.w; });
        count = clipPolygon(polygonB, count, polygonA, [](const ClipVertex &c) { return c.w - 1e-5f; });
        count = clipPolygon(polygonA, count, polygonB, [GuardX](const ClipVertex &c) { return GuardX * c.w - c.x; });
        count = clipPolygon(polygonB, count, polygonA, [GuardX](const ClipVertex &c) { return GuardX * c.w + c.x; });
        count = clipPolygon(polygonA, count, polygonB, [](const ClipVertex &c) { return c.w - c.y; });
        count = clipPolygon(polygonB, count, polygonA, [](const ClipVertex &c) { return c.w + c.y; });
        if (count < 3) {
            return TextureLodNone;
        }

        // At each corner of the visible polygon: u = (u/w) / (1/w), du/dX = (ku_x - u kq_x) / (1/w), then per pixel.
        const double pixelX = 1.0 / std::max(std::fabs(double(viewportScaleX)), 1e-6);
        const double pixelY = 1.0 / std::max(std::fabs(double(viewportScaleY)), 1e-6);
        float finest = TextureLodNone;
        for (int i = 0; i < count; i++) {
            const ClipVertex &c = polygonA[i];
            const double X = double(c.x) / c.w;
            const double Y = double(c.y) / c.w;
            const double q = kq[0] * X + kq[1] * Y + kq[2];
            if (!(q > 1e-12)) {
                continue;
            }

            const double uc = (ku[0] * X + ku[1] * Y + ku[2]) / q;
            const double vc = (kv[0] * X + kv[1] * Y + kv[2]) / q;
            const double dudx = (ku[0] - uc * kq[0]) / q * pixelX;
            const double dudy = (ku[1] - uc * kq[1]) / q * pixelY;
            const double dvdx = (kv[0] - vc * kq[0]) / q * pixelX;
            const double dvdy = (kv[1] - vc * kq[1]) / q * pixelY;
            finest = std::min(finest, footprintLod(float(dudx), float(dudy), float(dvdx), float(dvdy), maxAnisotropy));
        }

        return finest;
    }

    float rectangleTexelLod(int32_t dsdx, int32_t dtdy) {
        const float s = std::fabs(float(dsdx)) / 1024.0f;
        const float t = std::fabs(float(dtdy)) / 1024.0f;
        const float sigma = std::max(s, t);
        if (!(sigma > 0.0f)) {
            return TextureLodNone;
        }

        return std::log2(sigma);
    }

    uint32_t textureLodTopMip(float lod, float bias, uint32_t mipmaps) {
        if ((mipmaps <= 1) || !std::isfinite(lod)) {
            return 0;
        }

        const float level = std::floor(lod + bias);
        if (level <= 0.0f) {
            return 0;
        }

        return std::min(uint32_t(level), mipmaps - 1);
    }

    uint64_t textureLodBytes(uint64_t memorySize, uint32_t topMip) {
        // Each mip has a quarter of the texels of the one above it, so dropping the top r mips keeps about 4^-r of the
        // chain. Block alignment of the smallest mips makes this slightly low; the report says it is an estimate.
        return memorySize >> (2 * std::min(topMip, 31U));
    }

    struct TextureLodEntry {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t mipmaps = 0;
        uint64_t memorySize = 0;
        uint64_t residentSize = 0;
        float lod = TextureLodNone;
        uint64_t samples = 0;
    };

    struct TextureLodReport {
        std::mutex mutex;
        std::unordered_map<std::string, TextureLodEntry> window;
        std::chrono::steady_clock::time_point windowStart = std::chrono::steady_clock::now();
        FILE *file = nullptr;
        bool openFailed = false;
    };

    static TextureLodReport &textureLodReport() {
        static TextureLodReport report;
        return report;
    }

    static void writeTextureLodWindow(TextureLodReport &report) {
        if ((report.file == nullptr) && !report.openFailed) {
#       ifdef _WIN32
            if (fopen_s(&report.file, textureLodReportPath(), "w") != 0) {
                report.file = nullptr;
            }
#       else
            report.file = std::fopen(textureLodReportPath(), "w");
#       endif
            if (report.file == nullptr) {
                report.openFailed = true;
                fprintf(stderr, "TK64: cannot open the texture LOD report %s\n", textureLodReportPath());
                return;
            }

            fprintf(report.file, "# TK64 texture LOD report (estimate). lod = log2(texels per pixel), full = needs mip 0.\n");
            fprintf(report.file, "# window <time> files <n> resident <MiB> max <MiB> high <MiB> medium <MiB> low <MiB> minimum <MiB>\n");
            fprintf(report.file, "#   <path> <width>x<height> mips <n> <MiB> lod <value|full|none> top <maximum> <high> <medium> <low> <minimum>\n");
        }

        if (report.file == nullptr) {
            return;
        }

        std::vector<std::pair<std::string, TextureLodEntry>> entries(report.window.begin(), report.window.end());
        std::sort(entries.begin(), entries.end(), [](const auto &x, const auto &y) {
            return x.second.memorySize > y.second.memorySize;
        });

        uint64_t resident = 0;
        uint64_t projected[5] = {};
        for (const auto &it : entries) {
            const TextureLodEntry &e = it.second;
            resident += e.residentSize;
            for (int l = 0; l < 5; l++) {
                projected[l] += textureLodBytes(e.memorySize, textureLodTopMip(e.lod, TextureLodLevelBias[l], e.mipmaps));
            }
        }

        const double MiB = 1024.0 * 1024.0;
        const TextureStreamingStats streamStats = textureStreamingStats();
        const std::time_t wall = std::time(nullptr);
        std::tm local = {};
#   ifdef _WIN32
        localtime_s(&local, &wall);
#   else
        localtime_r(&wall, &local);
#   endif
        char clock[16];
        std::strftime(clock, sizeof(clock), "%H:%M:%S", &local);
        fprintf(report.file, "window %s files %zu resident %.1f max %.1f high %.1f medium %.1f low %.1f minimum %.1f loads %llu requests %llu swaps %llu failures %llu level %u used %.1f budget %.1f pressure %d\n", clock, entries.size(),
            resident / MiB, projected[0] / MiB, projected[1] / MiB, projected[2] / MiB, projected[3] / MiB, projected[4] / MiB,
            (unsigned long long)textureStreamEvents[0].exchange(0), (unsigned long long)textureStreamEvents[1].exchange(0),
            (unsigned long long)textureStreamEvents[2].exchange(0), (unsigned long long)textureAllocationFailures(),
            streamStats.level, streamStats.usedBytes / MiB, streamStats.budgetBytes / MiB, streamStats.underPressure ? 1 : 0);

        for (const auto &it : entries) {
            const TextureLodEntry &e = it.second;
            char lodText[32];
            if (e.lod == TextureLodFull) {
                snprintf(lodText, sizeof(lodText), "full");
            }
            else if (e.lod == TextureLodNone) {
                snprintf(lodText, sizeof(lodText), "none");
            }
            else {
                snprintf(lodText, sizeof(lodText), "%.2f", e.lod);
            }

            fprintf(report.file, "  %s %ux%u mips %u %.2f lod %s top %u %u %u %u %u\n", it.first.c_str(), e.width, e.height, e.mipmaps, e.memorySize / MiB, lodText,
                textureLodTopMip(e.lod, TextureLodLevelBias[0], e.mipmaps), textureLodTopMip(e.lod, TextureLodLevelBias[1], e.mipmaps),
                textureLodTopMip(e.lod, TextureLodLevelBias[2], e.mipmaps), textureLodTopMip(e.lod, TextureLodLevelBias[3], e.mipmaps),
                textureLodTopMip(e.lod, TextureLodLevelBias[4], e.mipmaps));
        }

        fflush(report.file);
    }

    void recordTextureLod(const TextureLodSample &sample) {
        if (!textureLodEnabled()) {
            return;
        }

        TextureLodReport &report = textureLodReport();
        std::unique_lock lock(report.mutex);
        TextureLodEntry &entry = report.window[sample.relativePath];
        entry.width = sample.width;
        entry.height = sample.height;
        entry.mipmaps = sample.mipmaps;
        entry.memorySize = sample.memorySize;
        entry.residentSize = sample.residentSize;
        entry.samples++;

        // A sample without a measurement still marks the file as used; it only lowers the level when one was measured.
        if ((entry.lod == TextureLodNone) || (sample.lod < entry.lod)) {
            entry.lod = sample.lod;
        }

        const auto now = std::chrono::steady_clock::now();
        if ((now - report.windowStart) >= std::chrono::seconds(5)) {
            writeTextureLodWindow(report);
            report.window.clear();
            report.windowStart = now;
        }
    }
};
