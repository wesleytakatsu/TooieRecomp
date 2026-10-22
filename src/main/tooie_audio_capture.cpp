#include "tooie_audio_capture.h"

#include <atomic>
#include <bit>
#include <chrono>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#define getpid _getpid
#define mkdir(path, mode) _wmkdir(path) // std::filesystem::path::c_str() is wchar_t* on Windows
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace tooie_audio_capture {
namespace {

constexpr int kMinSeconds = 10;
constexpr int kMaxSeconds = 20;
constexpr int kDefaultSeconds = 12;
constexpr uint32_t kDefaultMaxChunks = 2400;
constexpr uint32_t kDefaultMaxBytes = 24u * 1024u * 1024u;
constexpr uint32_t kMaxChunkCopy = 256u * 1024u;

constexpr uint32_t kReasonNone = 0;
constexpr uint32_t kReasonWindow = 1;
constexpr uint32_t kReasonBytes = 2;
constexpr uint32_t kReasonChunks = 3;
constexpr uint32_t kReasonShutdown = 4;
constexpr uint32_t kReasonOversize = 5;

struct Slot {
    uint64_t steady_ns = 0;
    uint64_t t_rel_ns = 0;
    uint32_t input_rate = 0;
    uint32_t output_rate = 0;
    uint32_t guest_s16_samples = 0;
    uint32_t pre_skip_bytes = 0;
    uint32_t post_skip_bytes = 0;
    uint32_t skip_factor = 0;
    uint32_t sdl_before = 0;
    uint32_t sdl_after = 0;
    int sdl_queue_result = 0;
    uint32_t discarded_output_frames = 0;
    float volume = 0.f;
    uint32_t arena_s16 = 0;
    uint32_t arena_pre = 0;
    uint32_t arena_post = 0;
    uint32_t post_reserved = 0;
    uint32_t rewind_used = 0;
};

std::atomic<uint32_t> g_gate{0};
std::atomic<uint32_t> g_stop{0};
std::atomic<uint32_t> g_shutdown{0};
std::atomic<uint32_t> g_in_chunk{0};
std::atomic<uint32_t> g_published{0};
std::atomic<uint64_t> g_t0{0};
std::atomic<uint32_t> g_reason{kReasonNone};
std::atomic<uint32_t> g_flushed{0};
std::atomic<uint32_t> g_flush_once{0};

uint64_t g_window_ns = 0;
uint32_t g_max_chunks = 0;
uint32_t g_max_bytes = 0;
uint32_t g_used = 0;
uint32_t g_count = 0;
bool g_open = false;
int g_seconds = 0;
std::string g_out_dir;
std::vector<unsigned char> g_arena;
std::vector<Slot> g_slots;
std::thread g_worker;
bool g_worker_started = false;
bool g_warned = false;

uint64_t steady_ns() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

void warn_once(const char* msg) {
    if (g_warned) {
        return;
    }
    g_warned = true;
    std::fprintf(stderr, "[tooie-audio-capture] %s\n", msg);
}

void set_reason(uint32_t reason) {
    uint32_t expected = kReasonNone;
    g_reason.compare_exchange_strong(expected, reason, std::memory_order_relaxed);
}

const char* reason_name(uint32_t reason) {
    switch (reason) {
    case kReasonWindow: return "window";
    case kReasonBytes: return "bytes";
    case kReasonChunks: return "chunks";
    case kReasonShutdown: return "shutdown";
    case kReasonOversize: return "chunk_oversize";
    default: return "none";
    }
}

std::string json_escape(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (unsigned char c : text) {
        if (c == '\\' || c == '"') {
            out.push_back('\\');
            out.push_back(static_cast<char>(c));
        } else if (c < 0x20) {
            out.push_back('?');
        } else {
            out.push_back(static_cast<char>(c));
        }
    }
    return out;
}

bool write_exclusive(const std::string& path, const void* data, size_t bytes) {
    FILE* file = std::fopen(path.c_str(), "wx");
    if (file == nullptr) {
        return false;
    }
    size_t wrote = bytes == 0 ? 0 : std::fwrite(data, 1, bytes, file);
    int flush_rc = std::fflush(file);
    int close_rc = std::fclose(file);
    return wrote == bytes && flush_rc == 0 && close_rc == 0;
}

bool inside_tooie_repo(const std::filesystem::path& canonical) {
    std::filesystem::path cursor = canonical;
    while (true) {
        std::error_code ec;
        if (std::filesystem::exists(cursor / ".git", ec) &&
            std::filesystem::exists(cursor / "src/main/main.cpp", ec)) {
            return true;
        }
        if (!cursor.has_parent_path() || cursor == cursor.root_path()) {
            return false;
        }
        std::filesystem::path parent = cursor.parent_path();
        if (parent == cursor) {
            return false;
        }
        cursor = parent;
    }
}

bool prepare_dir(const char* absolute_dir) {
    if (absolute_dir == nullptr || absolute_dir[0] == '\0') {
        warn_once("rejected: TOOIE_AUDIO_CAPTURE_DIR missing");
        return false;
    }
    std::filesystem::path input(absolute_dir);
    if (!input.is_absolute()) {
        warn_once("rejected: capture dir is not absolute");
        return false;
    }
    std::error_code ec;
    std::filesystem::path canonical = std::filesystem::weakly_canonical(input, ec);
    if (ec || !std::filesystem::is_directory(canonical, ec)) {
        warn_once("rejected: capture dir missing or not a directory");
        return false;
    }
    if (canonical == canonical.root_path() || inside_tooie_repo(canonical)) {
        warn_once("rejected: capture dir is root or inside the TooieRecomp repo");
        return false;
    }

    uint64_t stamp = steady_ns();
    int pid = static_cast<int>(::getpid());
    for (int attempt = 0; attempt < 16; ++attempt) {
        std::filesystem::path child = canonical /
            ("tooie-audio-" + std::to_string(pid) + "-" + std::to_string(stamp) + "-" +
             std::to_string(attempt));
        if (::mkdir(child.c_str(), 0700) == 0) {
            g_out_dir = child.string();
            return true;
        }
        if (errno != EEXIST) {
            warn_once("rejected: could not create a new capture directory");
            return false;
        }
    }
    warn_once("rejected: capture directory name collided");
    return false;
}

void flush_files() {
    uint32_t expected = 0;
    if (!g_flush_once.compare_exchange_strong(expected, 1, std::memory_order_acq_rel)) {
        return;
    }
    uint32_t count = g_published.load(std::memory_order_acquire);
    if (count > g_slots.size()) {
        count = static_cast<uint32_t>(g_slots.size());
    }
    size_t reserve_s16 = 0;
    size_t reserve_pre = 0;
    size_t reserve_post = 0;
    for (uint32_t i = 0; i < count; ++i) {
        reserve_s16 += static_cast<size_t>(g_slots[i].guest_s16_samples) * sizeof(int16_t);
        reserve_pre += g_slots[i].pre_skip_bytes;
        reserve_post += g_slots[i].post_skip_bytes;
    }

    std::string s16;
    std::string pre;
    std::string post;
    std::string jsonl;
    uint32_t off_s16 = 0;
    uint32_t off_pre = 0;
    uint32_t off_post = 0;
    s16.reserve(reserve_s16);
    pre.reserve(reserve_pre);
    post.reserve(reserve_post);
    jsonl.reserve(static_cast<size_t>(count) * 256u);

    for (uint32_t i = 0; i < count; ++i) {
        const Slot& slot = g_slots[i];
        s16.append(reinterpret_cast<const char*>(g_arena.data() + slot.arena_s16), slot.guest_s16_samples * sizeof(int16_t));
        pre.append(reinterpret_cast<const char*>(g_arena.data() + slot.arena_pre), slot.pre_skip_bytes);
        post.append(reinterpret_cast<const char*>(g_arena.data() + slot.arena_post), slot.post_skip_bytes);

        uint64_t dt_ns = 0;
        if (i > 0) {
            dt_ns = slot.t_rel_ns - g_slots[i - 1].t_rel_ns;
        }
        uint32_t guest_frames = slot.guest_s16_samples / 2u;
        uint32_t pre_frames = slot.pre_skip_bytes / 8u;
        uint32_t post_frames = slot.post_skip_bytes / 8u;
        char line[1024];
        int n = std::snprintf(
            line, sizeof(line),
            "{\"i\":%u,\"t_rel_ns\":%llu,\"dt_ns\":%llu,\"steady_ns\":%llu,"
            "\"input_rate\":%u,\"output_rate\":%u,"
            "\"guest_s16_samples\":%u,\"guest_frames\":%u,\"guest_trailing_samples\":%u,"
            "\"pre_skip_bytes\":%u,\"pre_skip_frames\":%u,"
            "\"post_skip_bytes\":%u,\"post_skip_frames\":%u,"
            "\"skip_factor\":%u,\"discarded_output_frames\":%u,"
            "\"sdl_queued_bytes_before\":%u,\"sdl_queued_bytes_after\":%u,\"sdl_queue_result\":%d,"
            "\"off_s16\":%u,\"off_pre_f32\":%u,\"off_post_f32\":%u,"
            "\"pipeline_volume\":%.8g}\n",
            i,
            static_cast<unsigned long long>(slot.t_rel_ns),
            static_cast<unsigned long long>(dt_ns),
            static_cast<unsigned long long>(slot.steady_ns),
            slot.input_rate, slot.output_rate,
            slot.guest_s16_samples, guest_frames, slot.guest_s16_samples % 2u,
            slot.pre_skip_bytes, pre_frames,
            slot.post_skip_bytes, post_frames,
            slot.skip_factor, slot.discarded_output_frames,
            slot.sdl_before, slot.sdl_after, slot.sdl_queue_result,
            off_s16, off_pre, off_post,
            static_cast<double>(slot.volume));
        if (n > 0 && static_cast<size_t>(n) < sizeof(line)) {
            jsonl += line;
        }
        off_s16 += slot.guest_s16_samples * static_cast<uint32_t>(sizeof(int16_t));
        off_pre += slot.pre_skip_bytes;
        off_post += slot.post_skip_bytes;
    }

    uint32_t reason = g_reason.load(std::memory_order_relaxed);
    std::string manifest;
    manifest.reserve(2048);
    manifest += "{\n";
    manifest += "  \"tool\": \"tooie_audio_capture\",\n";
    manifest += "  \"window_seconds\": " + std::to_string(g_seconds) + ",\n";
    manifest += "  \"chunk_count\": " + std::to_string(count) + ",\n";
    manifest += "  \"stopped_reason\": \"" + std::string(reason_name(reason)) + "\",\n";
    manifest += "  \"clock\": \"steady_clock nanoseconds; t_rel_ns is from the first accepted queue_samples, not process start\",\n";
    manifest += "  \"guest_s16_samples_are\": \"individual int16 values, not stereo frames\",\n";
    manifest += "  \"limits\": {\"max_chunks\": " + std::to_string(g_max_chunks) +
                ", \"max_bytes\": " + std::to_string(g_max_bytes) +
                ", \"max_seconds\": " + std::to_string(kMaxSeconds) + "},\n";
    manifest += "  \"files\": {\n";
    manifest += "    \"guest_raw_s16le\": \"guest_raw_s16le.pcm\",\n";
    manifest += "    \"post_convert_pre_skip_f32le\": \"post_convert_pre_skip_f32le.pcm\",\n";
    manifest += "    \"post_skip_sdl_queue_f32le\": \"post_skip_sdl_queue_f32le.pcm\",\n";
    manifest += "    \"chunks\": \"chunks.jsonl\"\n";
    manifest += "  },\n";
    manifest += "  \"guest_raw_s16le\": {\n";
    manifest += "    \"encoding\": \"signed16 little-endian interleaved pairs\",\n";
    manifest += "    \"order\": \"audio_data memory order BEFORE float conversion and BEFORE the host channel swap. slot0=audio_data[even], slot1=audio_data[odd]. Not labeled L/R.\"\n";
    manifest += "  },\n";
    manifest += "  \"post_convert_pre_skip_f32le\": {\n";
    manifest += "    \"encoding\": \"float32 little-endian interleaved pairs\",\n";
    manifest += "    \"stage\": \"after int16 to float, pipeline volume, channel swap, SDL_ConvertAudio, and discarded_output_frames head offset; before skip_factor compaction\",\n";
    manifest += "    \"order\": \"host queue order. slot0 is the swapped image of guest odd (audio_data[2k+1]) times (0.5/32768)*volume; slot1 is guest even. Not a claim that guest slot0 was left.\"\n";
    manifest += "  },\n";
    manifest += "  \"post_skip_sdl_queue_f32le\": {\n";
    manifest += "    \"encoding\": \"float32 little-endian interleaved pairs\",\n";
    manifest += "    \"stage\": \"exact bytes passed to SDL_QueueAudio after skip_factor\",\n";
    manifest += "    \"order\": \"same host queue pair order as post_convert_pre_skip_f32le\"\n";
    manifest += "  },\n";
    manifest += "  \"endian\": \"little\",\n";
    manifest += "  \"output_dir\": \"" + json_escape(g_out_dir) + "\"\n";
    manifest += "}\n";

    std::string summary;
    summary += "tooie_audio_capture\n";
    summary += "dir=" + g_out_dir + "\n";
    summary += "window_seconds=" + std::to_string(g_seconds) + "\n";
    summary += "chunks=" + std::to_string(count) + "\n";
    summary += "stopped_reason=" + std::string(reason_name(reason)) + "\n";
    summary += "bytes_s16=" + std::to_string(s16.size()) + "\n";
    summary += "bytes_pre_f32=" + std::to_string(pre.size()) + "\n";
    summary += "bytes_post_f32=" + std::to_string(post.size()) + "\n";
    summary += "guest order: audio_data even,odd before swap. Not L/R.\n";
    summary += "f32 order: host queue after swap. slot0 from guest odd, slot1 from guest even.\n";
    summary += "t_rel_ns starts at first accepted queue_samples.\n";

    bool ok = true;
    ok = write_exclusive(g_out_dir + "/guest_raw_s16le.pcm", s16.data(), s16.size()) && ok;
    ok = write_exclusive(g_out_dir + "/post_convert_pre_skip_f32le.pcm", pre.data(), pre.size()) && ok;
    ok = write_exclusive(g_out_dir + "/post_skip_sdl_queue_f32le.pcm", post.data(), post.size()) && ok;
    ok = write_exclusive(g_out_dir + "/chunks.jsonl", jsonl.data(), jsonl.size()) && ok;
    ok = write_exclusive(g_out_dir + "/manifest.json", manifest.data(), manifest.size()) && ok;
    ok = write_exclusive(g_out_dir + "/summary.txt", summary.data(), summary.size()) && ok;
    if (!ok) {
        warn_once("flush failed; capture files incomplete, audio path unchanged");
    }
    g_flushed.store(1, std::memory_order_release);
}

}  // namespace

extern "C" void tooie_audio_capture_atexit() {
    tooie_audio_capture::shutdown();
}

namespace {

void worker_main() {
    try {
    while (g_shutdown.load(std::memory_order_acquire) == 0) {
        uint64_t t0 = g_t0.load(std::memory_order_acquire);
        if (t0 != 0 && steady_ns() - t0 >= g_window_ns) {
            set_reason(kReasonWindow);
            g_stop.store(1, std::memory_order_seq_cst);
        }
        if (g_stop.load(std::memory_order_seq_cst) != 0) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (g_in_chunk.load(std::memory_order_seq_cst) != 0 &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    if (g_in_chunk.load(std::memory_order_seq_cst) != 0) {
        warn_once("producer still in chunk after 2s; flush skipped");
        g_gate.store(0, std::memory_order_release);
    } else {
        flush_files();
        g_gate.store(0, std::memory_order_release);
    }
    } catch (...) {
        warn_once("flush worker failed; audio path unchanged");
        g_gate.store(0, std::memory_order_release);
    }
}

void reset_state() {
    g_gate.store(0, std::memory_order_release);
    g_stop.store(0, std::memory_order_relaxed);
    g_shutdown.store(0, std::memory_order_relaxed);
    g_in_chunk.store(0, std::memory_order_relaxed);
    g_published.store(0, std::memory_order_relaxed);
    g_t0.store(0, std::memory_order_relaxed);
    g_reason.store(kReasonNone, std::memory_order_relaxed);
    g_flushed.store(0, std::memory_order_relaxed);
    g_flush_once.store(0, std::memory_order_relaxed);
    g_window_ns = 0;
    g_max_chunks = 0;
    g_max_bytes = 0;
    g_used = 0;
    g_count = 0;
    g_open = false;
    g_seconds = 0;
    g_out_dir.clear();
    g_arena.clear();
    g_slots.clear();
    g_worker_started = false;
    g_warned = false;
}

}  // namespace

bool hot() noexcept {
    return g_gate.load(std::memory_order_acquire) != 0;
}

InitResult init(const Options& options) {
    static_assert(static_cast<int>(std::endian::native) == static_cast<int>(std::endian::little),
                  "capture labels s16le/f32le");
    if (g_worker_started || hot()) {
        warn_once("rejected: capture already armed");
        return InitResult::Rejected;
    }
    if (options.absolute_dir == nullptr || options.absolute_dir[0] == '\0') {
        return InitResult::Disabled;
    }
    if (options.seconds < kMinSeconds || options.seconds > kMaxSeconds) {
        warn_once("rejected: TOOIE_AUDIO_CAPTURE_SECONDS outside 10..20");
        return InitResult::Rejected;
    }
    try {
        if (!prepare_dir(options.absolute_dir)) {
            return InitResult::Rejected;
        }

        g_seconds = options.seconds;
        g_window_ns = static_cast<uint64_t>(options.seconds) * 1000000000ull;
        g_max_chunks = options.max_chunks == 0 || options.max_chunks > kDefaultMaxChunks
                           ? kDefaultMaxChunks
                           : options.max_chunks;
        g_max_bytes = options.max_bytes == 0 || options.max_bytes > kDefaultMaxBytes
                          ? kDefaultMaxBytes
                          : options.max_bytes;
        g_arena.assign(g_max_bytes, 0);
        g_slots.assign(g_max_chunks, Slot{});
        g_used = 0;
        g_count = 0;
        g_open = false;
        g_stop.store(0, std::memory_order_relaxed);
        g_shutdown.store(0, std::memory_order_relaxed);
        g_in_chunk.store(0, std::memory_order_relaxed);
        g_published.store(0, std::memory_order_relaxed);
        g_t0.store(0, std::memory_order_relaxed);
        g_reason.store(kReasonNone, std::memory_order_relaxed);
        g_flushed.store(0, std::memory_order_relaxed);
        g_flush_once.store(0, std::memory_order_relaxed);
    } catch (...) {
        warn_once("rejected: capture init failed");
        return InitResult::Rejected;
    }

    static bool atexit_registered = false;
    if (!atexit_registered) {
        std::atexit(tooie_audio_capture_atexit);
        atexit_registered = true;
    }
    try {
        g_worker = std::thread(worker_main);
    } catch (...) {
        warn_once("rejected: flush worker failed to start");
        reset_state();
        return InitResult::Rejected;
    }
    g_worker_started = true;
    g_gate.store(1, std::memory_order_release);
    return InitResult::Armed;
}

InitResult init_from_env() {
    const char* dir = std::getenv("TOOIE_AUDIO_CAPTURE_DIR");
    if (dir == nullptr || dir[0] == '\0') {
        return InitResult::Disabled;
    }
    const char* seconds_text = std::getenv("TOOIE_AUDIO_CAPTURE_SECONDS");
    int seconds = kDefaultSeconds;
    if (seconds_text != nullptr && seconds_text[0] != '\0') {
        char* end = nullptr;
        long parsed = std::strtol(seconds_text, &end, 10);
        if (end == seconds_text || *end != '\0' || parsed < kMinSeconds || parsed > kMaxSeconds) {
            warn_once("rejected: TOOIE_AUDIO_CAPTURE_SECONDS outside 10..20");
            return InitResult::Rejected;
        }
        seconds = static_cast<int>(parsed);
    }
    Options options;
    options.absolute_dir = dir;
    options.seconds = seconds;
    return init(options);
}

bool begin_and_stash(const Pre& in) noexcept {
    if (g_gate.load(std::memory_order_acquire) == 0) {
        return false;
    }
    if (g_stop.load(std::memory_order_acquire) != 0) {
        return false;
    }
    uint64_t now = steady_ns();
    uint64_t t0 = g_t0.load(std::memory_order_acquire);
    if (t0 != 0 && now - t0 >= g_window_ns) {
        set_reason(kReasonWindow);
        g_stop.store(1, std::memory_order_seq_cst);
        return false;
    }
    if (g_count >= g_max_chunks) {
        set_reason(kReasonChunks);
        g_stop.store(1, std::memory_order_seq_cst);
        return false;
    }
    if (in.guest_s16_samples > (kMaxChunkCopy / sizeof(int16_t)) || in.pre_skip_bytes > kMaxChunkCopy) {
        set_reason(kReasonOversize);
        g_stop.store(1, std::memory_order_seq_cst);
        return false;
    }
    if ((in.guest_s16_samples > 0 && in.guest == nullptr) ||
        (in.pre_skip_bytes > 0 && in.pre_skip == nullptr)) {
        set_reason(kReasonOversize);
        g_stop.store(1, std::memory_order_seq_cst);
        return false;
    }

    uint32_t s16_bytes = static_cast<uint32_t>(in.guest_s16_samples * sizeof(int16_t));
    uint64_t need = static_cast<uint64_t>(s16_bytes) + in.pre_skip_bytes + in.pre_skip_bytes;
    if (need > g_max_bytes || static_cast<uint64_t>(g_used) + need > g_max_bytes) {
        set_reason(kReasonBytes);
        g_stop.store(1, std::memory_order_seq_cst);
        return false;
    }

    g_in_chunk.store(1, std::memory_order_seq_cst);
    if (g_stop.load(std::memory_order_seq_cst) != 0 ||
        g_gate.load(std::memory_order_acquire) == 0) {
        g_in_chunk.store(0, std::memory_order_seq_cst);
        return false;
    }

    if (g_t0.load(std::memory_order_relaxed) == 0) {
        g_t0.store(now, std::memory_order_release);
        t0 = now;
    }

    Slot& slot = g_slots[g_count];
    slot.rewind_used = g_used;
    slot.steady_ns = now;
    slot.t_rel_ns = now - t0;
    slot.input_rate = in.input_rate;
    slot.output_rate = in.output_rate;
    slot.guest_s16_samples = static_cast<uint32_t>(in.guest_s16_samples);
    slot.pre_skip_bytes = in.pre_skip_bytes;
    slot.post_skip_bytes = 0;
    slot.skip_factor = in.skip_factor;
    slot.sdl_before = in.sdl_queued_bytes_before;
    slot.sdl_after = 0;
    slot.discarded_output_frames = in.discarded_output_frames;
    slot.volume = in.pipeline_volume;
    slot.arena_s16 = g_used;
    if (s16_bytes != 0) {
        std::memcpy(g_arena.data() + g_used, in.guest, s16_bytes);
    }
    g_used += s16_bytes;
    slot.arena_pre = g_used;
    if (in.pre_skip_bytes != 0) {
        std::memcpy(g_arena.data() + g_used, in.pre_skip, in.pre_skip_bytes);
    }
    g_used += in.pre_skip_bytes;
    slot.arena_post = g_used;
    slot.post_reserved = in.pre_skip_bytes;
    g_used += in.pre_skip_bytes;
    g_open = true;
    return true;
}

void commit(const Post& in) noexcept {
    if (!g_open) {
        return;
    }
    Slot& slot = g_slots[g_count];
    uint32_t bytes = in.post_skip_bytes;
    if (bytes > slot.post_reserved || (bytes > 0 && in.post_skip == nullptr)) {
        g_used = slot.rewind_used;
        g_open = false;
        g_in_chunk.store(0, std::memory_order_seq_cst);
        set_reason(kReasonOversize);
        g_stop.store(1, std::memory_order_seq_cst);
        return;
    }
    if (g_gate.load(std::memory_order_acquire) == 0) {
        g_used = slot.rewind_used;
        g_open = false;
        g_in_chunk.store(0, std::memory_order_seq_cst);
        return;
    }
    if (bytes != 0) {
        std::memcpy(g_arena.data() + slot.arena_post, in.post_skip, bytes);
    }
    slot.post_skip_bytes = bytes;
    slot.sdl_after = in.sdl_queued_bytes_after;
    slot.sdl_queue_result = in.sdl_queue_result;
    g_used = slot.arena_post + bytes;
    g_count += 1;
    g_published.store(g_count, std::memory_order_release);
    g_open = false;
    g_in_chunk.store(0, std::memory_order_seq_cst);
}

void shutdown() noexcept {
    if (!g_worker_started) {
        return;
    }
    set_reason(kReasonShutdown);
    g_stop.store(1, std::memory_order_seq_cst);
    g_shutdown.store(1, std::memory_order_release);
    try {
        if (g_worker.joinable()) {
            g_worker.join();
        }
    } catch (...) {
        warn_once("flush worker join failed");
    }
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (g_in_chunk.load(std::memory_order_seq_cst) != 0 &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    g_gate.store(0, std::memory_order_release);
    if (g_in_chunk.load(std::memory_order_seq_cst) != 0) {
        warn_once("shutdown left capture buffers intact");
        return;
    }
    // A late callback may still be returning through begin_and_stash. Keep
    // stop asserted and its storage alive until explicit re-init or process
    // teardown. init/re-init is only allowed with the producer stopped.
    try {
        flush_files();
    } catch (...) {
        warn_once("shutdown flush failed; capture may be incomplete");
    }
    g_worker_started = false;
}

bool flush_completed() noexcept {
    return g_flushed.load(std::memory_order_acquire) != 0;
}

const char* output_dir() noexcept {
    return g_out_dir.c_str();
}

}  // namespace tooie_audio_capture
