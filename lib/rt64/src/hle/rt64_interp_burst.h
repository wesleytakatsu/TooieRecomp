//
// RT64
//
// P09 diagnostic: F8 "interpolated frame burst". Saves up to 20 numbered PNGs of real rendered
// intermediate (interpolated) frames, taken from the PRE_VI_RESOLVED_COLOR_TARGET (game scene only;
// no VI filter, no UI/overlay, no desktop). Header-only (inline) so no CMake change is needed.
//
// Ownership:
//   - hotkey thread        : Controller::request() only (state flip + notify).
//   - workload thread      : writes FrameMeta into InterpolatedFrameCounters under interpolatedMutex.
//   - present thread       : poll() / consider() / submit(); GPU readback; owns no files.
//   - writer thread (1)    : directory, PNG, manifest, status.txt, 10 s watchdog, finalization.
// The synchronous readback perturbs timing: runs with a burst are NOT a cadence/FPS proof.
//

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "hle/rt64_host_endpoint_current.h"

// Declared here instead of including miniz.h: RecompFrontend TUs also see N64ModernRuntime's own miniz.h and the two clash.
// Both are provided by the miniz.c already compiled into rt64 (src/contrib/miniz).
extern "C" {
    void *tdefl_write_image_to_png_file_in_memory(const void *pImage, int w, int h, int num_chans, size_t *pLen_out);
    void mz_free(void *p);
}

namespace RT64 {
namespace interpBurst {
    constexpr uint32_t kImages = 20;
    constexpr uint32_t kMaxMeta = 17; // index 0 = non-MSAA frame 0, else the present loop index i.
    constexpr size_t kQueueMax = 2;
    constexpr int64_t kTimeoutMs = 10000;
    constexpr const char *kStage = "PRE_VI_RESOLVED_COLOR_TARGET";

    enum class Reason : uint8_t {
        ACCEPTED, NO_META, NATIVE, PAUSED, FORCED_CURRENT, ENDPOINT_CURRENT, WEIGHT_RANGE, UNMATCHED,
        META_MISMATCH, DUPLICATE_KEY, SURPLUS, COUNT
    };

    inline const char *reasonName(Reason r) {
        static const char *names[] = { "INTERPOLATED", "NO_META", "NATIVE", "PAUSED", "FORCED_CURRENT", "ENDPOINT_CURRENT",
            "WEIGHT_RANGE", "UNMATCHED", "META_MISMATCH", "DUPLICATE", "SURPLUS" };
        return names[size_t(r)];
    }

    // Produced by the workload thread, stored next to the texture slot it describes.
    struct FrameMeta {
        bool valid = false;
        uint64_t workloadId = 0;
        uint64_t presentId = 0; // Workload::presentId (the present before this workload).
        uint32_t frameIndex = 0;
        uint32_t displayFrames = 0;
        float curW = 1.0f;  // effective (after TOOIE_DIAG_FORCE_CURRENT_FRAME)
        float prevW = 0.0f;
        float inputCurW = 1.0f;
        float inputPrevW = 0.0f;
        uint32_t targetRate = 0;
        uint32_t viOriginalRate = 0;
        bool generate = false;
        bool paused = false;
        bool matched = false; // prevFrame.matched
        uint32_t mappedWorkloads = 0;
        uint32_t totalWorkloads = 0;
        bool forceCurrent = false;
        bool forceEndpoint = false;
        bool msaa = false;
    };

    inline bool forceEndpointEnv() {
        return endpointCurrentActive();
    }

    // Same semantics as the workload queue's TOOIE_DIAG_FORCE_CURRENT_FRAME / RT64_DIAG_FORCE_CURRENT_FRAME.
    inline bool forceCurrentEnv() {
        static const bool v = [] {
            const char *e = std::getenv("TOOIE_DIAG_FORCE_CURRENT_FRAME");
            if ((e == nullptr) || (e[0] == '\0') || (e[0] == '0')) e = std::getenv("RT64_DIAG_FORCE_CURRENT_FRAME");
            return (e != nullptr) && (e[0] != '\0') && (e[0] != '0');
        }();
        return v;
    }

    // Frame-level classification only. Says nothing about individual transforms.
    inline Reason classify(const FrameMeta &m) {
        if (!m.valid) return Reason::NO_META;
        if (!m.generate) return Reason::NATIVE;
        if (m.paused) return Reason::PAUSED;
        if (m.forceCurrent || m.forceEndpoint) return Reason::FORCED_CURRENT;
        if (!(m.curW < 1.0f)) return Reason::ENDPOINT_CURRENT;
        if (!(m.curW > 0.0f)) return Reason::WEIGHT_RANGE; // also NaN
        if (!m.matched || (m.mappedWorkloads == 0)) return Reason::UNMATCHED;
        return Reason::ACCEPTED;
    }

    // Plume footprint rowWidth is in texels; Vulkan requires it to give a 256 B aligned pitch here.
    inline uint32_t alignedRowWidthTexels(uint32_t width, uint32_t bytesPerPixel = 4) {
        const uint32_t pitch = ((width * bytesPerPixel) + 255u) & ~255u;
        return pitch / bytesPerPixel;
    }

    // Copies padded rows into a tight buffer.
    inline void packRows(const uint8_t *src, size_t srcPitchBytes, uint32_t width, uint32_t height, uint32_t bytesPerPixel, std::vector<uint8_t> &dst) {
        const size_t row = size_t(width) * bytesPerPixel;
        dst.resize(row * height);
        for (uint32_t y = 0; y < height; y++) {
            std::memcpy(dst.data() + y * row, src + y * srcPitchBytes, row);
        }
    }

    // RGBA8 -> RGB8 PNG (alpha of a scene target is not meaningful). Returns false on failure.
    inline bool encodePng(const uint8_t *rgba, uint32_t width, uint32_t height, std::vector<uint8_t> &out) {
        std::vector<uint8_t> rgb(size_t(width) * height * 3);
        for (size_t p = 0, n = size_t(width) * height; p < n; p++) {
            std::memcpy(&rgb[p * 3], &rgba[p * 4], 3);
        }
        size_t len = 0;
        void *png = tdefl_write_image_to_png_file_in_memory(rgb.data(), int(width), int(height), 3, &len);
        if (png == nullptr) return false;
        out.assign(static_cast<uint8_t *>(png), static_cast<uint8_t *>(png) + len);
        mz_free(png);
        return true;
    }

    inline std::string imageName(uint32_t number) {
        char b[32];
        std::snprintf(b, sizeof(b), "image_%03u.png", number);
        return b;
    }

    struct Job {
        FrameMeta meta;
        uint64_t presentId = 0; // Present::presentId
        uint32_t width = 0, height = 0;
        float scaleX = 1.0f, scaleY = 1.0f;
        uint32_t downsample = 1;
        std::vector<uint8_t> rgba; // tight, owned by the job
    };

    class Controller {
    public:
        enum Phase : int { IDLE, REQUESTED, CAPTURING, FINISHING };
        enum class Result : int { NONE, DONE, TIMEOUT, NO_INTERPOLATION_ACTIVE, FAILED };

        explicit Controller(int64_t timeoutMs = kTimeoutMs, uint32_t images = kImages) : timeoutMs(timeoutMs), images(images) {}

        ~Controller() {
            {
                std::scoped_lock lock(m);
                stop = true;
            }
            cvWork.notify_all();
            cvSpace.notify_all();
            if (worker.joinable()) worker.join();
        }

        // Call once before any request (not on the hotkey path).
        void setBaseDir(const std::string &dir) {
            std::scoped_lock lock(m);
            baseDir = dir;
        }

        // Hotkey thread. Starts at most one burst; returns false (and logs) if one is active.
        bool request() {
            {
                std::scoped_lock lock(m);
                if (phase.load() != IDLE) {
                    std::printf("[tooie-interp-burst] F8 ignored: burst already active (%u/%u saved)\n", saved, images);
                    std::fflush(stdout);
                    return false;
                }
                saved = 0; committed = 0; seen = 0;
                std::fill(std::begin(rejects), std::end(rejects), 0u);
                accepted.clear(); queue.clear(); dir.clear(); errorText.clear();
                result = Result::NONE;
                deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
                phase.store(REQUESTED);
                if (!worker.joinable()) worker = std::thread(&Controller::run, this);
            }
            std::printf("[tooie-interp-burst] F8: burst requested (waiting for interpolated frames, %lld ms limit)\n", (long long)timeoutMs);
            std::fflush(stdout);
            cvWork.notify_all();
            return true;
        }

        Phase currentPhase() const { return Phase(phase.load(std::memory_order_acquire)); }
        bool capturing() const { return currentPhase() == CAPTURING; }

        // Present thread, once per present. Starts capture, or ends the request promptly if no interpolation is active.
        void poll(uint32_t viOriginalRate, uint32_t targetRate) {
            if (currentPhase() != REQUESTED) return;
            std::scoped_lock lock(m);
            if (phase.load() != REQUESTED) return;
            if ((targetRate == 0) || (viOriginalRate == 0) || (targetRate <= viOriginalRate)) {
                char b[192];
                std::snprintf(b, sizeof(b), "no interpolation active: targetRate=%u viOriginalRate=%u (normal CURRENT/30 path; use TOOIE_TEST_INTERPOLATION_FPS=60 or 120)", targetRate, viOriginalRate);
                finishLocked(Result::NO_INTERPOLATION_ACTIVE, b);
                return;
            }
            phase.store(CAPTURING);
        }

        // Present thread. ACCEPTED reserves one of the `images` slots; the caller must then submit() or abandon().
        // `formatOk=false` on an otherwise acceptable frame (e.g. HDR 16-bit target) fails the whole burst.
        Reason consider(const FrameMeta &meta, uint64_t presentWorkloadId, uint64_t presentId, bool formatOk) {
            std::scoped_lock lock(m);
            if (phase.load() != CAPTURING) return Reason::SURPLUS;
            seen++;
            Reason r = classify(meta);
            if ((r == Reason::ACCEPTED) && (meta.workloadId != presentWorkloadId)) r = Reason::META_MISMATCH;
            if (r == Reason::ACCEPTED) {
                const uint64_t key = (meta.workloadId << 8) | (meta.frameIndex & 0xFFu);
                if (accepted.count(key) != 0) r = Reason::DUPLICATE_KEY;
                else if (committed >= images) r = Reason::SURPLUS;
                else if (!formatOk) {
                    finishLocked(Result::FAILED, "UNSUPPORTED_FORMAT: scene target is not R8G8B8A8_UNORM (HDR); refusing to mis-encode");
                    return Reason::SURPLUS;
                }
                else {
                    accepted.insert(key);
                    committed++;
                    (void)presentId;
                    return Reason::ACCEPTED;
                }
            }
            rejects[size_t(r)]++;
            return r;
        }

        void abandon(const FrameMeta &meta) {
            std::scoped_lock lock(m);
            if (committed > 0) committed--;
            accepted.erase((meta.workloadId << 8) | (meta.frameIndex & 0xFFu));
        }

        // Present thread, only after swapChain->present() succeeded. Bounded queue: waits for the writer.
        bool submit(Job &&job) {
            std::unique_lock lock(m);
            cvSpace.wait(lock, [&] { return stop || (phase.load() != CAPTURING) || (queue.size() < kQueueMax); });
            if (stop || (phase.load() != CAPTURING)) return false;
            queue.push_back(std::move(job));
            lock.unlock();
            cvWork.notify_all();
            return true;
        }

        void fail(const std::string &reason) {
            std::scoped_lock lock(m);
            finishLocked(Result::FAILED, reason);
        }

        uint32_t savedCount() const { std::scoped_lock lock(m); return saved; }
        Result lastResult() const { std::scoped_lock lock(m); return result; }
        std::string lastDir() const { std::scoped_lock lock(m); return dir; }
        std::string lastMessage() const { std::scoped_lock lock(m); return errorText; }

        bool waitIdle(int64_t ms) {
            std::unique_lock lock(m);
            return cvIdle.wait_for(lock, std::chrono::milliseconds(ms), [&] { return phase.load() == IDLE; });
        }

        // "TOOIE overlay"-style short status; "" when idle with nothing to report.
        std::string statusLine() const {
            std::scoped_lock lock(m);
            const int p = phase.load();
            char b[64];
            if (p == IDLE) return (result == Result::NONE) ? "" : (result == Result::DONE ? "BURST DONE" : "BURST ENDED");
            std::snprintf(b, sizeof(b), "BURST %u/%u", saved, images);
            return b;
        }

    private:
        void summaryLocked(std::string &s) const {
            char b[96];
            std::snprintf(b, sizeof(b), "saved=%u/%u candidates=%u rejected:", saved, images, seen);
            s += b;
            for (size_t i = 1; i < size_t(Reason::COUNT); i++) {
                if (rejects[i] != 0) { std::snprintf(b, sizeof(b), " %s=%u", reasonName(Reason(i)), rejects[i]); s += b; }
            }
            if (seen == 0) s += " (no presented frame was considered)";
        }

        void finishLocked(Result r, const std::string &msg) {
            if ((phase.load() == FINISHING) || (phase.load() == IDLE)) return;
            result = r;
            errorText = msg;
            phase.store(FINISHING);
            cvWork.notify_all();
            cvSpace.notify_all();
        }

        bool ensureDirLocked(std::unique_lock<std::mutex> &lock) {
            if (!dir.empty()) return true;
            const std::string base = baseDir;
            lock.unlock();
            std::string made, err;
            try {
                namespace fs = std::filesystem;
                fs::create_directories(base);
                const std::time_t t = std::time(nullptr);
                char stamp[32];
                std::strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", std::localtime(&t));
                for (int n = 0; n < 1000 && made.empty(); n++) {
                    fs::path p = fs::path(base) / (std::string("interpolated-burst-") + stamp + (n ? ("-" + std::to_string(n)) : ""));
                    if (fs::create_directory(p)) made = p.string(); // exclusive: false if it already exists
                }
                if (made.empty()) err = "could not create a unique directory under " + base;
                else {
                    std::ofstream mf(fs::path(made) / "manifest.csv");
                    mf << "image,workloadId,presentId,frameIndex,displayFrames,curW,prevW,inputCurW,inputPrevW,kind,prevFrameMatched,mappedWorkloads,totalWorkloads,msaa,targetRate,viOriginalRate,width,height,resolutionScaleX,resolutionScaleY,downsample,stage\n";
                    if (!mf) err = "cannot write manifest.csv in " + made;
                }
            }
            catch (const std::exception &e) {
                err = std::string("filesystem error under ") + base + ": " + e.what();
            }
            lock.lock();
            if (!err.empty()) { finishLocked(Result::FAILED, err); return false; }
            dir = made;
            std::printf("[tooie-interp-burst] saving to %s\n", dir.c_str());
            std::fflush(stdout);
            return true;
        }

        // Writer thread, lock not held. Returns empty on success.
        std::string writeJob(const std::string &d, uint32_t number, const Job &job) {
            namespace fs = std::filesystem;
            std::vector<uint8_t> png;
            const fs::path final = fs::path(d) / imageName(number);
            if (!encodePng(job.rgba.data(), job.width, job.height, png)) return "PNG encode failed for " + final.string();
            const fs::path tmp = final.string() + ".tmp";
            {
                std::ofstream f(tmp, std::ios::binary);
                f.write(reinterpret_cast<const char *>(png.data()), std::streamsize(png.size()));
                f.flush();
                if (!f) return "cannot write " + tmp.string();
            }
            std::error_code ec;
            fs::rename(tmp, final, ec);
            if (ec) return "cannot rename " + tmp.string() + ": " + ec.message();
            const FrameMeta &m = job.meta;
            char row[512];
            std::snprintf(row, sizeof(row), "%s,%llu,%llu,%u,%u,%.6f,%.6f,%.6f,%.6f,%s,%d,%u,%u,%d,%u,%u,%u,%u,%.4f,%.4f,%u,%s\n",
                imageName(number).c_str(), (unsigned long long)m.workloadId, (unsigned long long)job.presentId, m.frameIndex, m.displayFrames,
                m.curW, m.prevW, m.inputCurW, m.inputPrevW, reasonName(classify(m)), m.matched ? 1 : 0, m.mappedWorkloads, m.totalWorkloads,
                m.msaa ? 1 : 0, m.targetRate, m.viOriginalRate, job.width, job.height, job.scaleX, job.scaleY, job.downsample, kStage);
            std::ofstream mf(fs::path(d) / "manifest.csv", std::ios::app);
            mf << row;
            if (!mf) return "cannot append manifest.csv in " + d;
            return {};
        }

        void finalizeLocked(std::unique_lock<std::mutex> &lock) {
            queue.clear();
            std::string msg = errorText;
            std::string sum;
            summaryLocked(sum);
            const Result r = result;
            const char *name = (r == Result::DONE) ? "DONE" : (r == Result::TIMEOUT) ? "TIMEOUT" : (r == Result::NO_INTERPOLATION_ACTIVE) ? "NO_INTERPOLATION_ACTIVE" : "ERROR";
            const std::string d = dir;
            lock.unlock();
            if (!d.empty()) {
                const std::filesystem::path p = std::filesystem::path(d) / "status.txt";
                const std::filesystem::path t = p.string() + ".tmp";
                std::ofstream f(t);
                f << "result=" << name << "\ndir=" << d << "\n" << sum << "\nmessage=" << msg << "\nstage=" << kStage
                  << "\nnote=timing perturbed by synchronous readback; not an FPS/cadence proof\n";
                f.close();
                std::error_code ec;
                std::filesystem::rename(t, p, ec);
            }
            std::printf("[tooie-interp-burst] %s dir=%s %s %s\n", name, d.empty() ? "(none)" : d.c_str(), sum.c_str(), msg.c_str());
            std::fflush(stdout);
            lock.lock();
            phase.store(IDLE);
            cvIdle.notify_all();
        }

        void run() {
            std::unique_lock lock(m);
            while (!stop) {
                const int p = phase.load();
                if (p == IDLE) { cvWork.wait(lock, [&] { return stop || (phase.load() != IDLE); }); continue; }
                if (p == FINISHING) { finalizeLocked(lock); continue; }

                if ((std::chrono::steady_clock::now() >= deadline)) {
                    finishLocked(Result::TIMEOUT, "watchdog expired after " + std::to_string(timeoutMs) + " ms");
                    continue;
                }
                if (!queue.empty()) {
                    if (!ensureDirLocked(lock)) continue;
                    Job job = std::move(queue.front());
                    queue.pop_front();
                    const std::string d = dir;
                    const uint32_t number = saved + 1;
                    cvSpace.notify_all();
                    lock.unlock();
                    const std::string err = writeJob(d, number, job);
                    lock.lock();
                    if (!err.empty()) { finishLocked(Result::FAILED, err); continue; }
                    saved = number;
                    std::printf("[tooie-interp-burst] saved %s (%u/%u) workload=%llu frame=%u curW=%.4f\n", imageName(number).c_str(), saved, images,
                        (unsigned long long)job.meta.workloadId, job.meta.frameIndex, job.meta.curW);
                    std::fflush(stdout);
                    if (saved >= images) finishLocked(Result::DONE, "");
                    continue;
                }
                cvWork.wait_until(lock, deadline, [&] { return stop || !queue.empty() || (phase.load() != p); });
            }
        }

        mutable std::mutex m;
        std::condition_variable_any cvWork, cvSpace, cvIdle;
        std::atomic<int> phase{ IDLE };
        std::thread worker;
        bool stop = false;
        const int64_t timeoutMs;
        const uint32_t images;
        std::string baseDir, dir, errorText;
        uint32_t saved = 0, committed = 0, seen = 0;
        uint32_t rejects[size_t(Reason::COUNT)] = {};
        std::unordered_set<uint64_t> accepted;
        std::deque<Job> queue;
        Result result = Result::NONE;
        std::chrono::steady_clock::time_point deadline;
    };

    inline Controller &controller() {
        static Controller instance;
        return instance;
    }

    // The only entry point the hotkey needs.
    inline bool request() { return controller().request(); }

    // Default-OFF test hook for paired A/B runs: TOOIE_TEST_INTERP_BURST_AT_WORKLOAD=<id> requests one burst the first time
    // the workload thread reaches workload >= id, so two runs capture the same workload ids without hotkey timing.
    inline void requestAtWorkload(uint64_t workloadId) {
        static const uint64_t target = [] {
            const char *e = std::getenv("TOOIE_TEST_INTERP_BURST_AT_WORKLOAD");
            return ((e != nullptr) && (e[0] != '\0')) ? uint64_t(std::strtoull(e, nullptr, 10)) : uint64_t(0);
        }();
        static std::atomic<bool> fired{ false };
        if ((target == 0) || (workloadId < target) || fired.exchange(true)) return;
        request();
    }
};
};
