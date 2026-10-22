//
// RT64
//
// Host-controlled switch for the shader-local "CURRENT endpoint" tfPos path (RSPProcessCS forceEndpointCurrent) and
// everything that must agree with it (P09 WORLD guards, burst classification).
//
// Precedence: explicit TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT in the environment (legacy test override) > host request
// > off (Banjo and any host that never calls requestHostEndpointCurrent). The environment is read once; the host
// request is an atomic that the render thread latches once per game workload, so a runtime change takes effect at a
// workload boundary and never splits one workload between the two paths.
//

#pragma once

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace RT64 {
    // -1: no host request, 0: interpolate, 1: CURRENT endpoint.
    inline std::atomic<int> &hostEndpointCurrentRequestSlot() {
        static std::atomic<int> slot{ -1 };
        return slot;
    }

    inline std::atomic<int> &hostEndpointCurrentLatchedSlot() {
        static std::atomic<int> slot{ -1 };
        return slot;
    }

    // Any thread (UI/config callback). Takes effect at the next latch.
    inline void requestHostEndpointCurrent(bool endpointCurrent) {
        hostEndpointCurrentRequestSlot().store(endpointCurrent ? 1 : 0, std::memory_order_release);
    }

    // Render thread only, once at the start of each workload.
    inline void latchHostEndpointCurrent() {
        hostEndpointCurrentLatchedSlot().store(hostEndpointCurrentRequestSlot().load(std::memory_order_acquire), std::memory_order_release);
    }

    // -1: variable absent or empty (host decides), otherwise 0/1 with the historical "empty/0 = off" parse.
    inline int endpointCurrentEnvState() {
        static const int state = [] {
            const char *env = std::getenv("TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT");
            if ((env == nullptr) || (env[0] == '\0')) {
                return -1;
            }
            return (env[0] != '0') ? 1 : 0;
        }();
        return state;
    }

    inline bool endpointCurrentActive() {
        const int env = endpointCurrentEnvState();
        if (env >= 0) {
            return env != 0;
        }
        return hostEndpointCurrentLatchedSlot().load(std::memory_order_acquire) == 1;
    }

    // Host-visible interpolation state: one stderr line per change (never per frame). Render thread only.
    struct HostInterpState {
        bool valid = false;
        bool endpointCurrent = false;
        bool generate = false;
        uint32_t targetRate = 0;
        uint32_t viRate = 0;
        bool loggedInterpolatedRender = false;
    };

    inline HostInterpState &hostInterpState() {
        static HostInterpState state;
        return state;
    }

    // First frame of each game workload: latch the host request, log when the state changed.
    inline void hostInterpBeginWorkload(uint64_t workloadId, bool generate, uint32_t targetRate, uint32_t viRate, uint32_t displayFrames) {
        latchHostEndpointCurrent();
        const bool endpointCurrent = endpointCurrentActive();
        HostInterpState &s = hostInterpState();
        if (s.valid && (s.endpointCurrent == endpointCurrent) && (s.generate == generate) && (s.targetRate == targetRate) && (s.viRate == viRate)) {
            return;
        }

        s.valid = true;
        s.endpointCurrent = endpointCurrent;
        s.generate = generate;
        s.targetRate = targetRate;
        s.viRate = viRate;
        s.loggedInterpolatedRender = false;
        std::fprintf(stderr, "[tooie-interp] workload=%llu endpointCurrent=%d generate=%d targetRate=%u viRate=%u displayFrames=%u\n",
            static_cast<unsigned long long>(workloadId), endpointCurrent ? 1 : 0, generate ? 1 : 0, targetRate, viRate, displayFrames);
    }

    inline void hostInterpNoteRender(uint32_t frame, float curWeight, float prevWeight) {
        HostInterpState &s = hostInterpState();
        if (!s.loggedInterpolatedRender && s.generate && (curWeight < 1.0f)) {
            s.loggedInterpolatedRender = true;
            std::fprintf(stderr, "[tooie-interp] first interpolated render since last state change: frame=%u curW=%.3f prevW=%.3f endpointCurrent=%d\n",
                frame, static_cast<double>(curWeight), static_cast<double>(prevWeight), s.endpointCurrent ? 1 : 0);
        }
    }
};
