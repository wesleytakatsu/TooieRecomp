//
// RT64
//
// P09 articulated-translation coherence for AUTO WORLD interpolation.
//
// RigidBody::updateLinear (AUTO) may "hold" a transform's translation at
// CURRENT (lerpTranslation == false) while rotation/scale/skew still
// interpolate. That heuristic is meant to suppress interpolation across a
// teleport of a whole object. It is evaluated per transform, so for an
// articulated mesh a single bone whose velocity direction reverses (or whose
// speed ratio jumps) is held while its neighbours keep interpolating. Any
// triangle that spans such a bone and a neighbour (a joint) then pairs a
// vertex placed with CURRENT translation and an interpolated rotation about
// the CURRENT origin with a vertex that is fully interpolated; the edge
// grows beyond both endpoint lengths (a stretched sliver).
//
// A teleport moves the whole object, so every transform of that object is
// held. Transforms connected by a triangle that references several WORLD
// slots are one rigid-articulated object. If at least one mapped transform
// of such a connected set still interpolates translation, the object is
// moving continuously and the held members are heuristic false positives:
// they must interpolate translation too. Sets whose mapped transforms are
// all held (a real teleport) and isolated transforms keep the original
// decision. No game, model, bone or display-list knowledge is used.
//
// A transform that stays held after this pass AND belongs to a connected set of
// several WORLD slots is a member of an articulated object the gate classed as a
// teleport. Interpolating only its rotation/scale about the held CURRENT origin
// swings its vertices off the endpoint chord and tears the joints, so such a
// transform is shown wholly at CURRENT (heldMatrix). An isolated held transform
// (a single rigid slot, no joint to tear) keeps the original RigidBody result.
//
// Default ON. TOOIE_TEST_P09_TRANSLATION_COHERENCE=0 restores the original
// per-transform decision for paired A/B tests.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <vector>

#include "common/rt64_common.h"

namespace RT64 {
    namespace p09TranslationCoherence {
        inline bool enabled() {
            static int cached = -1;
            if (cached < 0) {
                const char *env = std::getenv("TOOIE_TEST_P09_TRANSLATION_COHERENCE");
                cached = (env != nullptr && env[0] == '0' && env[1] == '\0') ? 0 : 1;
            }
            return cached != 0;
        }

        // Default ON. TOOIE_TEST_P09_GATE_SPEED_ONLY=0 keeps the AUTO gate's direction-change holds (paired A/B tests).
        // The AUTO gate divides the speed by the cosine of the turn angle before comparing it with its tolerance, so every turn
        // of 90 degrees or more holds the translation however slow the transform moves (a limb at the end of a swing).
        // A direction change alone is not a teleport: with this switch on, a held WORLD transform whose raw speed passes the gate
        // (RigidBody::lerpTranslationSpeedOnly) interpolates its translation. Only speed spikes keep the hold.
        inline bool gateSpeedOnlyEnabled() {
            static int cached = -1;
            if (cached < 0) {
                const char *env = std::getenv("TOOIE_TEST_P09_GATE_SPEED_ONLY");
                cached = (env != nullptr && env[0] == '0' && env[1] == '\0') ? 0 : 1;
            }
            return cached != 0;
        }

        // Matrix of a held-at-CURRENT transform at the given weight: the previous pose at weight 0, CURRENT otherwise.
        inline const hlslpp::float4x4 &heldMatrix(float weight, const hlslpp::float4x4 &prev, const hlslpp::float4x4 &cur) {
            return (weight <= 0.0f) ? prev : cur;
        }

        inline uint32_t findRoot(std::vector<uint32_t> &parent, uint32_t i) {
            while (parent[i] != i) {
                parent[i] = parent[parent[i]];
                i = parent[i];
            }
            return i;
        }

        // mapped[t] != 0: transform t is paired with a previous transform and interpolates.
        // lerpTranslation[t] != 0: the AUTO gate allows translation interpolation for t.
        // forceLerp[t] is set to 1 for each mapped, held transform that must interpolate translation.
        // articulated (optional)[t] is set to 1 when t shares a triangle-connected set with at least one other WORLD slot.
        // Returns the number of forced transforms.
        inline uint32_t compute(size_t worldCount, const uint32_t *faceIndices, size_t faceIndexCount,
            const uint16_t *worldIndices, uint32_t vertexCount, const std::vector<uint8_t> &mapped,
            const std::vector<uint8_t> &lerpTranslation, std::vector<uint8_t> &forceLerp,
            std::vector<uint8_t> *articulated = nullptr)
        {
            forceLerp.assign(worldCount, 0);
            if (articulated != nullptr) articulated->assign(worldCount, 0);
            bool anyHeld = false;
            for (size_t t = 0; t < worldCount && t < mapped.size() && t < lerpTranslation.size(); t++) {
                if (mapped[t] != 0 && lerpTranslation[t] == 0) {
                    anyHeld = true;
                    break;
                }
            }
            if (!anyHeld || faceIndices == nullptr || worldIndices == nullptr) {
                return 0;
            }

            std::vector<uint32_t> parent(worldCount);
            std::iota(parent.begin(), parent.end(), 0u);
            for (size_t f = 0; f + 2 < faceIndexCount; f += 3) {
                const uint32_t v0 = faceIndices[f], v1 = faceIndices[f + 1], v2 = faceIndices[f + 2];
                if (v0 >= vertexCount || v1 >= vertexCount || v2 >= vertexCount) continue;
                const uint32_t w0 = worldIndices[v0], w1 = worldIndices[v1], w2 = worldIndices[v2];
                if (w0 >= worldCount || w1 >= worldCount || w2 >= worldCount) continue;
                if (w0 == w1 && w1 == w2) continue;
                const uint32_t r0 = findRoot(parent, w0);
                const uint32_t r1 = findRoot(parent, w1);
                const uint32_t r2 = findRoot(parent, w2);
                if (r1 != r0) parent[r1] = r0;
                const uint32_t r2n = findRoot(parent, r2);
                if (r2n != findRoot(parent, r0)) parent[r2n] = findRoot(parent, r0);
            }

            if (articulated != nullptr) {
                std::vector<uint32_t> setSize(worldCount, 0);
                for (size_t t = 0; t < worldCount; t++) setSize[findRoot(parent, uint32_t(t))]++;
                for (size_t t = 0; t < worldCount; t++) (*articulated)[t] = (setSize[findRoot(parent, uint32_t(t))] > 1) ? 1 : 0;
            }

            std::vector<uint8_t> setInterpolates(worldCount, 0);
            for (size_t t = 0; t < worldCount && t < mapped.size() && t < lerpTranslation.size(); t++) {
                if (mapped[t] != 0 && lerpTranslation[t] != 0) {
                    setInterpolates[findRoot(parent, uint32_t(t))] = 1;
                }
            }

            uint32_t forced = 0;
            for (size_t t = 0; t < worldCount && t < mapped.size() && t < lerpTranslation.size(); t++) {
                if (mapped[t] != 0 && lerpTranslation[t] == 0 && setInterpolates[findRoot(parent, uint32_t(t))] != 0) {
                    forceLerp[t] = 1;
                    forced++;
                }
            }
            return forced;
        }
    }
}
