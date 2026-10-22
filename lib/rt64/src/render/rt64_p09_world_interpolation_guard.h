//
// RT64
//
// P09 articulated WORLD interpolation guards, applied as a post-pass over the
// WORLD matrices that TransformProcessor::process just interpolated:
//   1. translation coherence  (rt64_p09_translation_coherence.h)
//   2. articulated closure    (rt64_p09_closure_guard.h)
// See those headers for the mechanism. This pass only rewrites
// lerpWorldTransforms / prevWorldTransforms / invTWorldTransforms entries of
// transforms that must change; everything else is left exactly as produced by
// RigidBody::lerp. Default ON; each guard has its own =0 environment switch. Inert while the endpoint-CURRENT
// workaround is active (the Tooie default).
//
// When TOOIE_DIAG_P09_WORLD_LERP_AUDIT=<path> is set it also logs, per
// workload, the edge-stretch of every triangle with the guarded result and
// with the original (unguarded) interpolation of the same frame.
//

#pragma once

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "hle/rt64_game_frame.h"
#include "hle/rt64_host_endpoint_current.h"
#include "hle/rt64_workload.h"
#include "render/rt64_p09_closure_guard.h"
#include "render/rt64_p09_translation_coherence.h"
#include "render/rt64_p09_world_lerp_audit.h"

namespace RT64 {
    inline bool p09WorldGuardExplicitElementwiseCandidate() {
        static int cached = -1;
        if (cached < 0) {
            const char *a = std::getenv("TOOIE_TEST_P09_SELECTIVE_ELEMENTWISE_WORLD");
            const char *b = std::getenv("TOOIE_TEST_P09_MULTI_ONLY_ELEMENTWISE_WORLD");
            const bool on = (a != nullptr && a[0] != '\0' && a[0] != '0') || (b != nullptr && b[0] != '\0' && b[0] != '0');
            cached = on ? 1 : 0;
        }
        return cached != 0;
    }

    // With TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT active (the Tooie default workaround) RSPProcessCS positions every vertex
    // from the endpoint matrices, so the interpolated WORLD matrices do not reach the screen position. The guards stay
    // inert there so the stable CURRENT/30 path is bit-for-bit unchanged.
    inline bool p09WorldGuardEndpointCurrentActive() {
        return endpointCurrentActive();
    }

    // TOOIE_DIAG_P09_WORLD_LERP_SHADOW=1 (with the audit on): compute and audit/dump the guards but hand the renderer the
    // original interpolation, so a rendered frame and its guarded counterfactual come from the identical workload.
    inline bool p09WorldGuardShadow() {
        static int cached = -1;
        if (cached < 0) {
            const char *env = std::getenv("TOOIE_DIAG_P09_WORLD_LERP_SHADOW");
            cached = ((env != nullptr) && (env[0] != '\0') && (env[0] != '0')) ? 1 : 0;
        }
        return cached != 0;
    }

    // Must be called after the per-transform interpolation loop of one workload with a valid previous frame.
    inline void p09ApplyWorldInterpolationGuards(Workload &workload, const GameFrameMap::WorkloadMap &workloadMap,
        const DrawData &prevDrawData, float prevWeight, float curWeight)
    {
        DrawData &drawData = workload.drawData;
        const size_t worldCount = drawData.worldTransforms.size();
        P09WorldLerpAudit &audit = P09WorldLerpAudit::get();
        const bool auditOn = audit.enabled();
        const bool endpointCurrent = p09WorldGuardEndpointCurrentActive();
        const bool coherenceOn = p09TranslationCoherence::enabled() && !endpointCurrent;
        const bool guardOn = p09ClosureGuard::enabled() && !endpointCurrent;
        if ((!auditOn && !coherenceOn && !guardOn) || p09WorldGuardExplicitElementwiseCandidate()) {
            return;
        }

        if ((drawData.lerpWorldTransforms.size() != worldCount) || (drawData.prevWorldTransforms.size() != worldCount) ||
            (drawData.invTWorldTransforms.size() != worldCount) || (workloadMap.transforms.size() < worldCount)) {
            return;
        }

        const auto passStart = std::chrono::steady_clock::now();
        const bool shadow = auditOn && p09WorldGuardShadow();
        std::vector<interop::float4x4> shadowLerp, shadowPrev, shadowInvT;
        if (shadow) {
            shadowLerp = drawData.lerpWorldTransforms;
            shadowPrev = drawData.prevWorldTransforms;
            shadowInvT = drawData.invTWorldTransforms;
        }
        P09WorldLerpAuditFrame auditFrame;
        const bool gateSpeedOnly = coherenceOn && p09TranslationCoherence::gateSpeedOnlyEnabled();
        std::vector<uint8_t> mappedFlags(worldCount, 0), lerpTranslationFlags(worldCount, 0), gateReleased(worldCount, 0);
        std::vector<hlslpp::float4x4> prevEnd(worldCount), curEnd(worldCount), rigid(worldCount), elementwise(worldCount);
        for (size_t t = 0; t < worldCount; t++) {
            const GameFrameMap::TransformMap &transformMap = workloadMap.transforms[t];
            curEnd[t] = drawData.worldTransforms[t];
            prevEnd[t] = curEnd[t];
            rigid[t] = curEnd[t];
            elementwise[t] = curEnd[t];
            if (!transformMap.mapped) continue;
            prevEnd[t] = prevDrawData.worldTransforms[transformMap.prevTransformIndex];
            const hlslpp::float4x4 original = transformMap.rigidBody.lerp(curWeight, prevEnd[t], curEnd[t], true);
            // A transform frozen at CURRENT by a diagnostic keeps the matrix it was given.
            if ((curWeight < 1.0f) && (std::memcmp(&drawData.lerpWorldTransforms[t], &curEnd[t], sizeof(curEnd[t])) == 0) &&
                (std::memcmp(&original, &curEnd[t], sizeof(curEnd[t])) != 0)) {
                continue;
            }
            mappedFlags[t] = 1;
            lerpTranslationFlags[t] = transformMap.rigidBody.lerpTranslation ? 1 : 0;
            rigid[t] = original;
            // A hold that only the AUTO gate's direction-change amplification asked for is a false positive: interpolate it.
            if (gateSpeedOnly && !transformMap.rigidBody.lerpTranslation && transformMap.rigidBody.lerpTranslationSpeedOnly) {
                lerpTranslationFlags[t] = 1;
                gateReleased[t] = 1;
                auditFrame.gateReleased++;
                RigidBody released = transformMap.rigidBody;
                released.lerpTranslation = true;
                rigid[t] = released.lerp(curWeight, prevEnd[t], curEnd[t], true);
            }
        }

        std::vector<uint8_t> forceTranslation, articulated;
        if (coherenceOn) {
            auditFrame.coherenceForced = p09TranslationCoherence::compute(worldCount, drawData.faceIndices.data(),
                drawData.faceIndices.size(), drawData.worldIndices.data(), drawData.vertexCount(), mappedFlags,
                lerpTranslationFlags, forceTranslation, &articulated);
        }

        if (gateSpeedOnly) {
            if (forceTranslation.size() != worldCount) forceTranslation.assign(worldCount, 0);
            for (size_t t = 0; t < worldCount; t++) {
                if (gateReleased[t]) forceTranslation[t] = 1;
            }
        }

        // A transform whose translation is still held after the coherence pass and that belongs to a multi-slot set is part of
        // an articulated object the AUTO gate classed as a teleport (every mapped member of its set is held). Interpolating its
        // rotation about the held CURRENT origin would tear the joints, so the whole transform is shown at CURRENT. An isolated
        // held transform has no joint to tear and keeps the original RigidBody result.
        auto heldToCurrent = [&](size_t t) {
            return coherenceOn && mappedFlags[t] && !lerpTranslationFlags[t] && (t < articulated.size()) && (articulated[t] != 0) &&
                !((t < forceTranslation.size()) && (forceTranslation[t] != 0));
        };

        // Rigid interpolation with the coherence correction (the candidate the closure guard starts from).
        auto rigidAt = [&](size_t t, float weight) {
            if (heldToCurrent(t)) return p09TranslationCoherence::heldMatrix(weight, prevEnd[t], curEnd[t]);
            RigidBody body = workloadMap.transforms[t].rigidBody;
            if ((t < forceTranslation.size()) && (forceTranslation[t] != 0)) body.lerpTranslation = true;
            return body.lerp(weight, prevEnd[t], curEnd[t], true);
        };

        std::vector<hlslpp::float4x4> guardRigid(rigid);
        for (size_t t = 0; t < worldCount; t++) {
            if (!mappedFlags[t]) continue;
            guardRigid[t] = rigidAt(t, curWeight);
            elementwise[t] = lerpMatrix(prevEnd[t], curEnd[t], curWeight);
        }

        std::vector<uint8_t> useElementwise;
        if (guardOn) {
            auditFrame.guardElementwise = p09ClosureGuard::compute(worldCount, drawData.faceIndices.data(),
                drawData.faceIndices.size(), drawData.worldIndices.data(), drawData.vertexCount(),
                drawData.posFloats.data(), mappedFlags, prevEnd, curEnd, guardRigid, elementwise, useElementwise);
        }

        // Rewrite only the transforms whose matrix changes.
        for (size_t t = 0; t < worldCount; t++) {
            if (!mappedFlags[t]) continue;
            const bool coherence = (t < forceTranslation.size()) && (forceTranslation[t] != 0);
            const bool elementwiseUsed = (t < useElementwise.size()) && (useElementwise[t] != 0);
            const bool held = heldToCurrent(t);
            if (held) auditFrame.heldToCurrent++;
            else if (!lerpTranslationFlags[t] && !coherence) auditFrame.heldIsolated++;
            if (!coherence && !elementwiseUsed && !held) continue;
            hlslpp::float4x4 curMatrix, prevMatrix;
            if (elementwiseUsed) {
                curMatrix = elementwise[t];
                prevMatrix = lerpMatrix(prevEnd[t], curEnd[t], prevWeight);
            }
            else {
                curMatrix = guardRigid[t];
                prevMatrix = rigidAt(t, prevWeight);
            }
            drawData.lerpWorldTransforms[t] = curMatrix;
            drawData.prevWorldTransforms[t] = prevMatrix;
            drawData.invTWorldTransforms[t] = hlslpp::transpose(hlslpp::inverse(curMatrix));
        }

        if (auditOn) {
            auditFrame.guardMicros = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - passStart).count());
            auditFrame.transformFlags.assign(worldCount, 0);
            auditFrame.altLerp.resize(worldCount);
            auditFrame.altSet.assign(worldCount, 0);
            for (size_t t = 0; t < worldCount; t++) {
                if (!mappedFlags[t]) continue;
                const bool coherence = (t < forceTranslation.size()) && (forceTranslation[t] != 0);
                const bool elementwiseUsed = (t < useElementwise.size()) && (useElementwise[t] != 0);
                const bool held = heldToCurrent(t);
                // bit1 = translation interpolates in the final matrix; bit4 = shown at CURRENT (teleport semantics).
                auditFrame.transformFlags[t] = 1u | ((coherence || lerpTranslationFlags[t]) ? 2u : 0u) | (coherence ? 4u : 0u) | (elementwiseUsed ? 8u : 0u) | (held ? 16u : 0u);
                RigidBody finalBody = workloadMap.transforms[t].rigidBody;
                if (coherence) finalBody.lerpTranslation = true;
                audit.record(auditFrame, workload.workloadId, static_cast<uint32_t>(t), workloadMap.transforms[t].prevTransformIndex,
                    drawData.worldTransformGroups[t], prevWeight, curWeight, prevEnd[t], curEnd[t], finalBody,
                    workloadMap.transforms[t].rigidBody, drawData.lerpWorldTransforms[t], elementwiseUsed);
            }
            const uint32_t originalStretched = audit.auditTriangles(auditFrame, workload.workloadId, curWeight, drawData);
            if (originalStretched > 0) audit.dumpWorkload(auditFrame, workload.workloadId, curWeight, drawData);
            audit.endWorkload(auditFrame, workload.workloadId, prevWeight, curWeight, true);
        }
        if (shadow) {
            drawData.lerpWorldTransforms = shadowLerp;
            drawData.prevWorldTransforms = shadowPrev;
            drawData.invTWorldTransforms = shadowInvT;
        }
    }
}
