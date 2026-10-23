#pragma once
#include <cmath>

// Pure frustum math shared by production H-expand and the cull census.
// No RDRAM, no recomp types. Callers own guest-pointer checks.
namespace tooie_ws_frustum {

struct Volume {
    float cam[3];
    float fwd[3];
    float planes[4][4]; // planes[i] = {nx, ny, nz, d}
};

// target/source when both are finite and > 0 and the ratio is finite and > 1; otherwise 1.0f.
// A non-finite or <= 0 source is replaced by 4/3 first.
inline float k_for(float source_aspect, float target_aspect) {
    float source = source_aspect;
    if (!(source > 0.0f) || !std::isfinite(source)) {
        source = 4.0f / 3.0f;
    }
    if (!(source > 0.0f) || !std::isfinite(target_aspect)) {
        return 1.0f;
    }
    const float k = target_aspect / source;
    if (!(k > 1.0f) || !std::isfinite(k)) {
        return 1.0f;
    }
    return k;
}

// Normalises v in place; false (v untouched) when the length is non-finite or <= 1e-6.
inline bool normalize(float v[3]) {
    const float len = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (!(len > 1.0e-6f) || !std::isfinite(len)) {
        return false;
    }
    v[0] /= len;
    v[1] /= len;
    v[2] /= len;
    return true;
}

// Exactly today's tooie_ws_expand_h_plane math (fwd_unit already normalised; d recomputed from cam).
inline void expand_h_plane(float& nx, float& ny, float& nz, float& d, const float cam[3], const float fwd_unit[3], float k) {
    const float nf = nx * fwd_unit[0] + ny * fwd_unit[1] + nz * fwd_unit[2];
    nx = fwd_unit[0] * nf + (nx - fwd_unit[0] * nf) / k;
    ny = fwd_unit[1] * nf + (ny - fwd_unit[1] * nf) / k;
    nz = fwd_unit[2] * nf + (nz - fwd_unit[2] * nf) / k;
    d = -(cam[0] * nx + cam[1] * ny + cam[2] * nz);
}

// Mirrors tooie_cacec_eval_sphere: 1 accept, 0 reject, -1 invalid (fwd degenerate, radius < 0 or non-finite).
// Planes 0/1 are expanded only when k > 1. rel = pos - cam; reject when radius <= dot(n, rel) for any plane.
inline int sphere_accepts(const Volume& v, const float pos[3], float radius, float k) {
    if (!std::isfinite(radius) || radius < 0.0f) {
        return -1;
    }
    float fwd[3] = {v.fwd[0], v.fwd[1], v.fwd[2]};
    if (!normalize(fwd)) {
        return -1;
    }
    const float rel[3] = {pos[0] - v.cam[0], pos[1] - v.cam[1], pos[2] - v.cam[2]};
    for (int pl = 0; pl < 4; ++pl) {
        float nx = v.planes[pl][0];
        float ny = v.planes[pl][1];
        float nz = v.planes[pl][2];
        if (pl < 2 && k > 1.0f) {
            float d_unused = 0.0f;
            expand_h_plane(nx, ny, nz, d_unused, v.cam, fwd, k);
        }
        const float dot = nx * rel[0] + ny * rel[1] + nz * rel[2];
        if (radius <= dot) {
            return 0;
        }
    }
    return 1;
}

// Mirrors tooie_map_vis_aspect_eval_float_aabb: 1 accept, 0 reject, -1 invalid (fwd degenerate).
inline int aabb_accepts(const Volume& v, const float mn[3], const float mx[3], float k) {
    float fwd[3] = {v.fwd[0], v.fwd[1], v.fwd[2]};
    if (!normalize(fwd)) {
        return -1;
    }
    for (int pl = 0; pl < 4; ++pl) {
        float nx = v.planes[pl][0];
        float ny = v.planes[pl][1];
        float nz = v.planes[pl][2];
        float d = v.planes[pl][3];
        if (pl < 2 && k > 1.0f) {
            expand_h_plane(nx, ny, nz, d, v.cam, fwd, k);
        }
        const float px = (nx >= 0.0f) ? mn[0] : mx[0];
        const float py = (ny >= 0.0f) ? mn[1] : mx[1];
        const float pz = (nz >= 0.0f) ? mn[2] : mx[2];
        if ((nx * px + ny * py + nz * pz + d) >= 0.0f) {
            return 0;
        }
    }
    return 1;
}

enum class Census : int { Invalid = -1, Accept = 0, RejectBoth = 1, WidescreenOnly = 2 };

// original_v0 != 0 -> Accept; else expanded == 1 -> WidescreenOnly; expanded == 0 -> RejectBoth; else Invalid.
inline Census classify(unsigned original_v0, int expanded) {
    if (original_v0 != 0u) {
        return Census::Accept;
    }
    if (expanded == 1) {
        return Census::WidescreenOnly;
    }
    if (expanded == 0) {
        return Census::RejectBoth;
    }
    return Census::Invalid;
}

} // namespace tooie_ws_frustum
