//
// RT64
//

#include "rt64_transform_processor.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif
#include <vector>

#include "common/rt64_math.h"
#include "hle/rt64_game_frame.h"
#include "render/rt64_p09_world_interpolation_guard.h"
#include "hle/rt64_workload_queue.h"
#include "render/rt64_p09_local_pair_snapshot.h"
#include "render/rt64_p09_rod_classify.h"
#include "render/rt64_p09_spike_guard.h"

namespace RT64 {
    // TransformProcessor
    
    TransformProcessor::TransformProcessor() { }

    TransformProcessor::~TransformProcessor() { }

    void TransformProcessor::setup(RenderWorker *worker) {
        bufferUploader = std::make_unique<BufferUploader>(worker->device);
    }

    static bool p09LocalPairSnapshotEnabled() {
        static int cached = -1;
        if (cached < 0) {
            const char *env = std::getenv("TOOIE_DIAG_P09_LOCAL_PAIR_SNAPSHOT");
            cached = (env != nullptr && env[0] != '\0' && env[0] != '0') ? 1 : 0;
        }
        return cached == 1;
    }

    struct P09PairMatrixCtx {
        const DrawData *cur;
        const DrawData *prev;
        const uint32_t *prevIndex;
    };

    static void p09PairLoadMatrix(void *v, uint32_t world, int isPrev, float out[16]) {
        const P09PairMatrixCtx *c = static_cast<const P09PairMatrixCtx *>(v);
        const interop::float4x4 *src = nullptr;
        if (!isPrev) {
            if (c->cur != nullptr && world < c->cur->worldTransforms.size()) src = &c->cur->worldTransforms[world];
        }
        else if (c->prev != nullptr && c->prevIndex != nullptr) {
            const uint32_t pi = c->prevIndex[world];
            if (pi < c->prev->worldTransforms.size()) src = &c->prev->worldTransforms[pi];
        }
        if (src == nullptr) {
            for (int i = 0; i < 16; i++) out[i] = std::numeric_limits<float>::quiet_NaN();
            return;
        }
        for (int r = 0; r < 4; r++) {
            for (int col = 0; col < 4; col++) out[r * 4 + col] = src->m[r][col];
        }
    }

    extern "C" uint32_t tooie_diag_p09_local_pair_witnesses(
        uint64_t workloadId, uint32_t *worldIndex, uint32_t *resourceId,
        uint32_t *nodeRel, uint32_t *boneIndex, uint32_t *boneAnimId,
        uint8_t *candidateClass, uint32_t cap);

    struct P09PairRuntime {
        std::mutex mu;
        P09LocalPairArm arm;
        uint8_t sceneArmed = 0;
        uint8_t slotBusy = 0;
        uint8_t writerStarted = 0;
        uint8_t stop = 0;
        std::thread writer;
        std::thread poller;
    };

    static P09PairRuntime &p09PairRuntime() {
        static P09PairRuntime runtime;
        return runtime;
    }

    static const char *p09PairMarkerPath() {
        const char *env = std::getenv("TOOIE_DIAG_P09_LOCAL_PAIR_ARM");
        if (env != nullptr && env[0] != '\0') return env;
        return p09LocalPairArmMarkerDefault();
    }

    static void p09PairPoller() {
        const char *path = p09PairMarkerPath();
        std::remove(path);
        for (;;) {
            {
                std::lock_guard<std::mutex> lock(p09PairRuntime().mu);
                if (p09PairRuntime().stop != 0 || p09PairRuntime().sceneArmed != 0 ||
                    p09PairRuntime().arm.frozen != 0) {
                    return;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            if (std::remove(path) != 0) continue;
            std::lock_guard<std::mutex> lock(p09PairRuntime().mu);
            if (p09PairRuntime().stop != 0 || p09PairRuntime().arm.frozen != 0) return;
            p09PairRuntime().sceneArmed = 1;
            return;
        }
    }

    static void p09PairJoinAll() {
        {
            std::lock_guard<std::mutex> lock(p09PairRuntime().mu);
            p09PairRuntime().stop = 1;
        }
        P09PairRuntime &runtime = p09PairRuntime();
        if (runtime.poller.joinable()) runtime.poller.join();
        if (runtime.writer.joinable()) runtime.writer.join();
    }

    static void p09PairEnsurePoller() {
        static std::once_flag once;
        std::call_once(once, [] {
            p09PairRuntime().poller = std::thread(p09PairPoller);
            std::atexit(p09PairJoinAll);
        });
    }

    static void p09PairWriterMain(P09LocalPairSnapshot *owned, std::string *path) {
        if (!p09LocalPairSnapshotPublish(path->c_str(), *owned)) {
            static std::once_flag once;
            std::call_once(once, [] {
                std::fprintf(stderr, "IO_FAILED\n");
            });
        }
        delete owned;
        delete path;
    }

    static void p09PairStartWriter(const P09LocalPairSnapshot &s) {
        P09PairRuntime &runtime = p09PairRuntime();
        if (runtime.writerStarted != 0) return;
        runtime.writerStarted = 1;
        uint64_t generation = 0;
        if (s.pairCount > 0) generation = s.pairs[0].curWorkloadId;
        char path[512];
        p09LocalPairOutputName(path, sizeof(path), static_cast<long>(getpid()), generation);
        runtime.writer = std::thread(p09PairWriterMain, new P09LocalPairSnapshot(s), new std::string(path));
    }

    static void p09LocalPairCapture(P09LocalPairSnapshot &snap, const Workload &workload,
        const GameFrameMap::WorkloadMap *workloadMap, const WorkloadQueue *queue, bool prevFrameValid) {
        if (snap.success == 0) return;
        uint32_t worlds[kP09LocalPairMaxWorlds];
        uint32_t resources[kP09LocalPairMaxWorlds];
        uint32_t nodes[kP09LocalPairMaxWorlds];
        uint32_t bones[kP09LocalPairMaxWorlds];
        uint32_t anims[kP09LocalPairMaxWorlds];
        uint8_t classes[kP09LocalPairMaxWorlds];
        const uint32_t n = tooie_diag_p09_local_pair_witnesses(
            workload.workloadId, worlds, resources, nodes, bones, anims, classes, kP09LocalPairMaxWorlds);
        if (n == 0xFFFFFFFFu) {
            p09LocalPairFail(snap, P09LocalPairReasonMissingMap);
            return;
        }
        if (n > kP09LocalPairMaxWorlds) {
            p09LocalPairFail(snap, P09LocalPairReasonCapWorlds);
            return;
        }
        if (n == 0u) return;

        const DrawData &draw = workload.drawData;
        const uint32_t worldCount = uint32_t(draw.worldTransforms.size());
        std::vector<uint8_t> mapped(worldCount, 0);
        std::vector<uint32_t> prevOf(worldCount, 0);
        uint8_t frameMapped = 0;
        uint8_t mapInvalid = 0;
        uint64_t prevId = 0;
        uint32_t prevRing = 0;
        uint32_t prevWorldCount = 0;
        const DrawData *prevDraw = nullptr;
        if (prevFrameValid && workloadMap != nullptr && workloadMap->mapped) {
            frameMapped = 1;
            prevRing = workloadMap->prevWorkloadIndex;
            if (queue == nullptr || prevRing >= queue->workloads.size()) mapInvalid = 1;
            else {
                prevId = queue->workloads[prevRing].workloadId;
                prevDraw = &queue->workloads[prevRing].drawData;
                prevWorldCount = uint32_t(prevDraw->worldTransforms.size());
                const uint32_t mapCount = uint32_t(workloadMap->transforms.size());
                for (uint32_t t = 0; t < worldCount && t < mapCount; t++) {
                    mapped[t] = workloadMap->transforms[t].mapped ? 1u : 0u;
                    prevOf[t] = workloadMap->transforms[t].prevTransformIndex;
                }
            }
        }

        if (workload.fbPairCount > workload.fbPairs.size()) {
            p09LocalPairFail(snap, P09LocalPairReasonMapInvalid);
            return;
        }
        std::vector<P09LocalPairCallIn> calls;
        for (uint32_t f = 0; f < workload.fbPairCount; f++) {
            const FramebufferPair &fb = workload.fbPairs[f];
            if (fb.projectionCount > fb.projections.size()) {
                p09LocalPairFail(snap, P09LocalPairReasonMapInvalid);
                return;
            }
            for (uint32_t pr = 0; pr < fb.projectionCount; pr++) {
                const Projection &proj = fb.projections[pr];
                if (proj.gameCallCount > proj.gameCalls.size()) {
                    p09LocalPairFail(snap, P09LocalPairReasonMapInvalid);
                    return;
                }
                for (uint32_t d = 0; d < proj.gameCallCount; d++) {
                    const GameCall &call = proj.gameCalls[d];
                    P09LocalPairCallIn row{};
                    row.fbPair = f;
                    row.projection = pr;
                    row.gameCall = d;
                    row.faceStart = call.meshDesc.faceIndicesStart;
                    row.triangleCount = call.callDesc.triangleCount;
                    row.displayList = fb.displayListAddress;
                    row.displayListKnown = 1;
                    calls.push_back(row);
                }
            }
        }

        P09LocalPairWitnessIn wits[kP09LocalPairMaxWorlds];
        for (uint32_t i = 0; i < n; i++) {
            wits[i].worldIndex = worlds[i];
            wits[i].resourceId = resources[i];
            wits[i].nodeRel = nodes[i];
            wits[i].boneIndex = bones[i];
            wits[i].boneAnimId = anims[i];
            wits[i].provenanceKnown = 1;
            wits[i].candidateClass = classes[i];
        }
        P09PairMatrixCtx ctx{&draw, prevDraw, prevOf.empty() ? nullptr : prevOf.data()};
        P09LocalPairInput input{};
        input.curWorkloadId = workload.workloadId;
        input.prevWorkloadId = prevId;
        input.prevRingIndex = prevRing;
        input.frameMapped = frameMapped;
        input.mapInvalid = mapInvalid;
        input.curWorldCount = worldCount;
        input.prevWorldCount = prevWorldCount;
        input.worldMapped = mapped.empty() ? nullptr : mapped.data();
        input.prevTransformIndex = prevOf.empty() ? nullptr : prevOf.data();
        input.posFloats = draw.posFloats.empty() ? nullptr : draw.posFloats.data();
        input.posFloatCount = uint32_t(draw.posFloats.size());
        input.faceIndices = draw.faceIndices.empty() ? nullptr : draw.faceIndices.data();
        input.faceIndexCount = uint32_t(draw.faceIndices.size());
        input.worldIndices = draw.worldIndices.empty() ? nullptr : draw.worldIndices.data();
        input.vertexCount = draw.vertexCount();
        input.lightCounts = draw.lightCounts.empty() ? nullptr : draw.lightCounts.data();
        input.lightCountCount = uint32_t(draw.lightCounts.size());
        input.lookAtIndices = draw.lookAtIndices.empty() ? nullptr : draw.lookAtIndices.data();
        input.lookAtCount = uint32_t(draw.lookAtIndices.size());
        input.witnesses = wits;
        input.witnessCount = n;
        input.calls = calls.empty() ? nullptr : calls.data();
        input.callCount = uint32_t(calls.size());
        input.matrixCtx = &ctx;
        input.loadMatrix = p09PairLoadMatrix;
        p09LocalPairSnapshotAdd(snap, input);
    }

    void TransformProcessor::process(const ProcessParams &p) {
        const bool localPairSnap = p09LocalPairSnapshotEnabled();
        if (localPairSnap) p09PairEnsurePoller();
        P09PairRuntime *pairRuntime = localPairSnap ? &p09PairRuntime() : nullptr;
        bool captureNow = false;
        if (pairRuntime != nullptr) {
            std::lock_guard<std::mutex> lock(pairRuntime->mu);
            if (p09LocalPairShouldCapture(pairRuntime->sceneArmed, pairRuntime->slotBusy,
                    pairRuntime->arm.frozen, pairRuntime->arm.attempts)) {
                pairRuntime->slotBusy = 1;
                pairRuntime->arm.attempts += 1u;
                p09LocalPairSnapshotReset(pairRuntime->arm.frame);
                captureNow = true;
            }
        }
        for (uint32_t w : p.curFrame->workloads) {
            Workload &workload = p.workloadQueue->workloads[w];
            DrawData &drawData = workload.drawData;
            const bool prevFrameValid = (p.prevFrame != nullptr) && p.curFrame->frameMap.workloads[w].mapped;
            auto &lerpWorldTransforms = drawData.lerpWorldTransforms;
            auto &invTWorldTransforms = drawData.invTWorldTransforms;
            auto &prevWorldTransforms = drawData.prevWorldTransforms;
            lerpWorldTransforms.clear();
            invTWorldTransforms.clear();
            prevWorldTransforms.clear();
            drawData.endpointWorldTransforms.resize(drawData.worldTransforms.size() * 2);

            hlslpp::float4x4 prevMatrix, curMatrix, invMatrix, invTMatrix;

            if (captureNow) {
                const GameFrameMap::WorkloadMap *pairMap = nullptr;
                if (prevFrameValid && (w < p.curFrame->frameMap.workloads.size())) {
                    pairMap = &p.curFrame->frameMap.workloads[w];
                }
                p09LocalPairCapture(pairRuntime->arm.frame, workload, pairMap, p.workloadQueue, prevFrameValid);
            }

            // Match with the previous frame and interpolate the transforms.
            if (prevFrameValid) {
                const GameFrameMap::WorkloadMap &workloadMap = p.curFrame->frameMap.workloads[w];
                const DrawData &prevDrawData = p.workloadQueue->workloads[workloadMap.prevWorkloadIndex].drawData;
                for (size_t t = 0; t < drawData.worldTransforms.size(); t++) {
                    const GameFrameMap::TransformMap &transformMap = workloadMap.transforms[t];
                    if (transformMap.mapped) {
                        const hlslpp::float4x4 &prevTransform = prevDrawData.worldTransforms[workloadMap.transforms[t].prevTransformIndex];
                        const hlslpp::float4x4 &curTransform = drawData.worldTransforms[t];
                        drawData.endpointWorldTransforms[t * 2] = prevTransform;
                        drawData.endpointWorldTransforms[t * 2 + 1] = curTransform;
                        prevMatrix = transformMap.rigidBody.lerp(p.prevFrameWeight, prevTransform, curTransform, true);
                        curMatrix = transformMap.rigidBody.lerp(p.curFrameWeight, prevTransform, curTransform, true);
                        invMatrix = hlslpp::inverse(curMatrix);
                        invTMatrix = hlslpp::transpose(invMatrix);
                        lerpWorldTransforms.emplace_back(curMatrix);
                        invTWorldTransforms.emplace_back(invTMatrix);
                        prevWorldTransforms.emplace_back(prevMatrix);
                    }
                    else {
                        invMatrix = hlslpp::inverse(drawData.worldTransforms[t]);
                        invTMatrix = hlslpp::transpose(invMatrix);
                        lerpWorldTransforms.emplace_back(drawData.worldTransforms[t]);
                        invTWorldTransforms.emplace_back(invTMatrix);
                        prevWorldTransforms.emplace_back(drawData.worldTransforms[t]);
                        drawData.endpointWorldTransforms[t * 2] = drawData.worldTransforms[t];
                        drawData.endpointWorldTransforms[t * 2 + 1] = drawData.worldTransforms[t];
                    }
                }
                p09ApplyWorldInterpolationGuards(workload, workloadMap, prevDrawData, p.prevFrameWeight, p.curFrameWeight);
                p09SpikeGuard::applySegmentCoherence(workload, workloadMap, prevDrawData, p.prevFrameWeight, p.curFrameWeight);
                p09SpikeGuard::applyJointCoherence(workload, workloadMap, prevDrawData, p.prevFrameWeight, p.curFrameWeight);
                p09SpikeGuard::apply(workload, workloadMap, prevDrawData, p.curFrameWeight);
                p09RodClassify::run(workload, workloadMap, prevDrawData, p.curFrameWeight);
            }
            // Copy as normal and just generate the inverse of the transforms.
            else {
                for (size_t t = 0; t < drawData.worldTransforms.size(); t++) {
                    invMatrix = hlslpp::inverse(drawData.worldTransforms[t]);
                    invTMatrix = hlslpp::transpose(invMatrix);
                    invTWorldTransforms.push_back(invTMatrix);
                    drawData.endpointWorldTransforms[t * 2] = drawData.worldTransforms[t];
                    drawData.endpointWorldTransforms[t * 2 + 1] = drawData.worldTransforms[t];
                }
            }
        }
        if (captureNow) {
            std::lock_guard<std::mutex> lock(pairRuntime->mu);
            p09LocalPairArmFinish(pairRuntime->arm, pairRuntime->arm.frame);
            pairRuntime->slotBusy = 0;
            if (pairRuntime->arm.frozen != 0) p09PairStartWriter(pairRuntime->arm.held);
        }
    }
    
    void TransformProcessor::upload(const ProcessParams &p) {
        uploads.clear();

        for (uint32_t w : p.curFrame->workloads) {
            const bool prevFrameValid = (p.prevFrame != nullptr);
            Workload &workload = p.workloadQueue->workloads[w];
            const DrawData &drawData = workload.drawData;
            DrawBuffers &drawBuffers = workload.drawBuffers;
            const interop::float4x4 *worldMatrices = prevFrameValid ? drawData.lerpWorldTransforms.data() : drawData.worldTransforms.data();
            const interop::float4x4 *prevWorldMatrices = prevFrameValid ? drawData.prevWorldTransforms.data() : drawData.worldTransforms.data();
            const interop::float4x4 *invTWorldMatrices = drawData.invTWorldTransforms.data();
            std::pair<size_t, size_t> uploadRange = { 0, drawData.worldTransforms.size() };
            uploads.emplace_back(BufferUploader::Upload{ worldMatrices, uploadRange, sizeof(interop::float4x4), RenderBufferFlag::STORAGE, { }, &drawBuffers.worldTransformsBuffer });
            uploads.emplace_back(BufferUploader::Upload{ prevWorldMatrices, uploadRange, sizeof(interop::float4x4), RenderBufferFlag::STORAGE, { }, &drawBuffers.prevWorldTransformsBuffer });
            uploads.emplace_back(BufferUploader::Upload{ invTWorldMatrices, uploadRange, sizeof(interop::float4x4), RenderBufferFlag::STORAGE, { }, &drawBuffers.invTWorldTransformsBuffer });
            std::pair<size_t, size_t> endpointRange = { 0, drawData.endpointWorldTransforms.size() };
            uploads.emplace_back(BufferUploader::Upload{ drawData.endpointWorldTransforms.data(), endpointRange, sizeof(interop::float4x4), RenderBufferFlag::STORAGE, { }, &drawBuffers.endpointWorldTransformsBuffer });
        }

        bufferUploader->submit(p.worker, uploads);
    }
};