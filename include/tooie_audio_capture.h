#pragma once

// Default-off capture of one queue_samples window. Copies only.
// Does not convert, resample, skip, or queue.

#include <cstddef>
#include <cstdint>

namespace tooie_audio_capture {

enum class InitResult : int { Disabled = 0, Armed = 1, Rejected = 2 };

struct Options {
    const char* absolute_dir = nullptr;
    int seconds = 12;          // inclusive 10..20; no default-to-infinite
    uint32_t max_chunks = 0;   // 0 -> 2400; larger values clamp down
    uint32_t max_bytes = 0;    // 0 -> 24 MiB shared arena; larger values clamp down
};

// guest_s16_samples counts individual int16 values, not stereo frames.
// pre_skip_bytes / post_skip_bytes are the host float buffer sizes in bytes.
struct Pre {
    const int16_t* guest = nullptr;
    size_t guest_s16_samples = 0;
    const float* pre_skip = nullptr;
    uint32_t pre_skip_bytes = 0;
    uint32_t input_rate = 0;
    uint32_t output_rate = 0;
    uint32_t skip_factor = 0;
    uint32_t sdl_queued_bytes_before = 0;
    uint32_t discarded_output_frames = 0;
    float pipeline_volume = 0.f;
};

struct Post {
    const float* post_skip = nullptr;
    uint32_t post_skip_bytes = 0;
    uint32_t sdl_queued_bytes_after = 0;
    int sdl_queue_result = 0;
};

// One acquire load. False -> caller must not touch capture state.
bool hot() noexcept;

// Main thread before production starts; re-init requires the old producer to
// have stopped. begin/commit pairs belong to the single queue_samples producer.
InitResult init_from_env();
InitResult init(const Options& options);
bool begin_and_stash(const Pre& in) noexcept;
void commit(const Post& in) noexcept;

// Main thread, after audio callbacks have stopped. Joins the flush worker.
void shutdown() noexcept;

bool flush_completed() noexcept;
const char* output_dir() noexcept;

}  // namespace tooie_audio_capture
