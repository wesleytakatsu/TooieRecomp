#include "tooie_audio_capture.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include <sys/mman.h>
#include <unistd.h>

namespace {

int g_failures = 0;

void expect(bool condition, const char* name) {
    if (!condition) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failures;
    }
}

std::string read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::string make_parent() {
    std::string tmpl = "/tmp/tooie-audio-cap-XXXXXX";
    if (::mkdtemp(tmpl.data()) == nullptr) {
        std::perror("mkdtemp");
        std::exit(1);
    }
    return tmpl;
}

tooie_audio_capture::Pre pre_of(const int16_t* guest, size_t guest_n, const float* pre, uint32_t pre_bytes,
                                 uint32_t skip, uint32_t before) {
    tooie_audio_capture::Pre out;
    out.guest = guest;
    out.guest_s16_samples = guest_n;
    out.pre_skip = pre;
    out.pre_skip_bytes = pre_bytes;
    out.input_rate = 22050;
    out.output_rate = 48000;
    out.skip_factor = skip;
    out.sdl_queued_bytes_before = before;
    out.discarded_output_frames = 8;
    out.pipeline_volume = 0.5f;
    return out;
}

void commit_of(const float* post, uint32_t post_bytes, uint32_t after) {
    tooie_audio_capture::Post out;
    out.post_skip = post;
    out.post_skip_bytes = post_bytes;
    out.sdl_queued_bytes_after = after;
    tooie_audio_capture::commit(out);
}

}  // namespace

int main() {
    unsetenv("TOOIE_AUDIO_CAPTURE_DIR");
    unsetenv("TOOIE_AUDIO_CAPTURE_SECONDS");

    void* locked = ::mmap(nullptr, 4096, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    expect(locked != MAP_FAILED, "mmap_guard");
    tooie_audio_capture::Pre poison;
    poison.guest = static_cast<int16_t*>(locked);
    poison.guest_s16_samples = 2;
    poison.pre_skip = static_cast<float*>(locked);
    poison.pre_skip_bytes = 8;
    expect(!tooie_audio_capture::hot(), "disabled_not_hot");
    expect(!tooie_audio_capture::begin_and_stash(poison), "disabled_begin_no_touch");
    tooie_audio_capture::Post poison_post;
    poison_post.post_skip = static_cast<float*>(locked);
    poison_post.post_skip_bytes = 8;
    tooie_audio_capture::commit(poison_post);
    expect(tooie_audio_capture::init_from_env() == tooie_audio_capture::InitResult::Disabled,
           "env_unset_disabled");

    std::string parent = make_parent();
    std::ofstream(parent + "/keep.txt") << "keep";
    tooie_audio_capture::Options bad;
    bad.absolute_dir = "relative/capture";
    bad.seconds = 12;
    expect(tooie_audio_capture::init(bad) == tooie_audio_capture::InitResult::Rejected, "relative_rejected");
    bad.absolute_dir = "/tmp/tooie-audio-missing-no-such-dir";
    expect(tooie_audio_capture::init(bad) == tooie_audio_capture::InitResult::Rejected, "missing_rejected");
    bad.absolute_dir = parent.c_str();
    bad.seconds = 9;
    expect(tooie_audio_capture::init(bad) == tooie_audio_capture::InitResult::Rejected, "seconds_low_rejected");
    bad.seconds = 21;
    expect(tooie_audio_capture::init(bad) == tooie_audio_capture::InitResult::Rejected, "seconds_high_rejected");
    bad.seconds = 0;
    expect(tooie_audio_capture::init(bad) == tooie_audio_capture::InitResult::Rejected, "seconds_zero_rejected");
    setenv("TOOIE_AUDIO_CAPTURE_DIR", parent.c_str(), 1);
    setenv("TOOIE_AUDIO_CAPTURE_SECONDS", "99", 1);
    expect(tooie_audio_capture::init_from_env() == tooie_audio_capture::InitResult::Rejected,
           "env_seconds_rejected");
    unsetenv("TOOIE_AUDIO_CAPTURE_DIR");
    unsetenv("TOOIE_AUDIO_CAPTURE_SECONDS");
    std::string mock_repo = make_parent();
    std::filesystem::create_directories(mock_repo + "/src/main");
    std::ofstream(mock_repo + "/.git") << "gitdir: dummy\n";
    std::ofstream(mock_repo + "/src/main/main.cpp") << "int main(){return 0;}\n";
    bad.absolute_dir = mock_repo.c_str();
    bad.seconds = 12;
    expect(tooie_audio_capture::init(bad) == tooie_audio_capture::InitResult::Rejected, "repo_dir_rejected");
    expect(read_file(parent + "/keep.txt") == "keep", "parent_sentinel_untouched");
    expect(std::filesystem::directory_iterator(parent) != std::filesystem::directory_iterator{},
           "parent_still_has_sentinel");
    int children = 0;
    for (const auto& entry : std::filesystem::directory_iterator(parent)) {
        (void)entry;
        ++children;
    }
    expect(children == 1, "rejects_created_no_capture_dir");

    tooie_audio_capture::Options opt;
    opt.absolute_dir = parent.c_str();
    opt.seconds = 10;
    opt.max_chunks = 8;
    opt.max_bytes = 4096;
    expect(tooie_audio_capture::init(opt) == tooie_audio_capture::InitResult::Armed, "armed");
    int16_t guest[8] = {1, -2, 3, -4, 5, -6, 7, -8};
    float pre[8] = {0.25f, 0.5f, 0.75f, 1.f, 1.25f, 1.5f, 1.75f, 2.f};
    float post[4] = {0.25f, 0.5f, 9.f, 9.f};
    int16_t guest_copy[8];
    float pre_copy[8];
    float post_copy[4];
    std::memcpy(guest_copy, guest, sizeof(guest));
    std::memcpy(pre_copy, pre, sizeof(pre));
    std::memcpy(post_copy, post, sizeof(post));
    expect(tooie_audio_capture::begin_and_stash(pre_of(guest, 8, pre, sizeof(pre), 0, 100)), "stash0");
    commit_of(post, sizeof(post), 180);
    int16_t guest_b[8] = {8, 7, 6, 5, 4, 3, 2, 1};
    float pre_b[8] = {3.f, 3.f, 3.f, 3.f, 4.f, 4.f, 4.f, 4.f};
    float post_b[4] = {3.f, 3.f, 4.f, 4.f};
    expect(tooie_audio_capture::begin_and_stash(pre_of(guest_b, 8, pre_b, sizeof(pre_b), 1, 200)), "stash1");
    commit_of(post_b, sizeof(post_b), 220);
    expect(std::memcmp(guest, guest_copy, sizeof(guest)) == 0, "guest_unchanged");
    expect(std::memcmp(pre, pre_copy, sizeof(pre)) == 0, "pre_unchanged");
    expect(std::memcmp(post, post_copy, sizeof(post)) == 0, "post_unchanged");
    std::string first_dir = tooie_audio_capture::output_dir();
    tooie_audio_capture::shutdown();
    std::string s16 = read_file(first_dir + "/guest_raw_s16le.pcm");
    std::string pre_file = read_file(first_dir + "/post_convert_pre_skip_f32le.pcm");
    std::string post_file = read_file(first_dir + "/post_skip_sdl_queue_f32le.pcm");
    std::string jsonl = read_file(first_dir + "/chunks.jsonl");
    std::string manifest = read_file(first_dir + "/manifest.json");
    expect(s16.size() == 32, "s16_bytes");
    expect(std::memcmp(s16.data(), guest_copy, 16) == 0, "s16_chunk0");
    expect(std::memcmp(s16.data() + 16, guest_b, 16) == 0, "s16_chunk1");
    expect(pre_file.size() == 64, "pre_bytes");
    expect(std::memcmp(pre_file.data(), pre_copy, 32) == 0, "pre_chunk0");
    expect(std::memcmp(pre_file.data() + 32, pre_b, 32) == 0, "pre_chunk1_skip");
    expect(post_file.size() == 32, "post_bytes");
    expect(std::memcmp(post_file.data(), post_copy, 16) == 0, "post_chunk0");
    expect(std::memcmp(post_file.data() + 16, post_b, 16) == 0, "post_chunk1_skip");
    expect(jsonl.find("\"input_rate\":22050") != std::string::npos, "json_input_rate");
    expect(jsonl.find("\"output_rate\":48000") != std::string::npos, "json_output_rate");
    expect(jsonl.find("\"guest_s16_samples\":8") != std::string::npos, "json_s16_samples");
    expect(jsonl.find("\"guest_frames\":4") != std::string::npos, "json_guest_frames");
    expect(jsonl.find("\"off_s16\":0") != std::string::npos, "json_off0");
    expect(jsonl.find("\"off_s16\":16") != std::string::npos, "json_off1");
    expect(jsonl.find("\"off_pre_f32\":32") != std::string::npos, "json_pre_off1");
    expect(jsonl.find("\"off_post_f32\":16") != std::string::npos, "json_post_off1");
    expect(jsonl.find("\"skip_factor\":1") != std::string::npos, "json_skip");
    expect(jsonl.find("\"sdl_queued_bytes_before\":100") != std::string::npos, "json_before");
    expect(jsonl.find("\"sdl_queued_bytes_after\":220") != std::string::npos, "json_after");
    expect(manifest.find("Not labeled L/R") != std::string::npos, "manifest_no_false_lr");
    expect(manifest.find("guest odd") != std::string::npos, "manifest_swap_order");
    expect(read_file(parent + "/keep.txt") == "keep", "sentinel_after_capture");

    std::string first_pcm = s16;
    expect(tooie_audio_capture::init(opt) == tooie_audio_capture::InitResult::Armed, "second_dir_armed");
    std::string second_dir = tooie_audio_capture::output_dir();
    expect(second_dir != first_dir, "exclusive_new_dir");
    expect(tooie_audio_capture::begin_and_stash(pre_of(guest, 8, pre, sizeof(pre), 0, 1)), "second_stash");
    commit_of(post, sizeof(post), 2);
    tooie_audio_capture::shutdown();
    expect(read_file(first_dir + "/guest_raw_s16le.pcm") == first_pcm, "first_dir_not_overwritten");

    opt.max_bytes = 80;
    opt.max_chunks = 8;
    expect(tooie_audio_capture::init(opt) == tooie_audio_capture::InitResult::Armed, "byte_cap_armed");
    expect(tooie_audio_capture::begin_and_stash(pre_of(guest, 8, pre, sizeof(pre), 0, 1)), "byte_cap_first");
    commit_of(post, sizeof(post), 2);
    expect(!tooie_audio_capture::begin_and_stash(pre_of(guest_b, 8, pre_b, sizeof(pre_b), 0, 1)),
           "byte_cap_second_rejected");
    std::string cap_dir = tooie_audio_capture::output_dir();
    tooie_audio_capture::shutdown();
    expect(read_file(cap_dir + "/guest_raw_s16le.pcm").size() == 16, "byte_cap_one_chunk");

    opt.max_bytes = 4096;
    opt.max_chunks = 1;
    expect(tooie_audio_capture::init(opt) == tooie_audio_capture::InitResult::Armed, "chunk_cap_armed");
    expect(tooie_audio_capture::begin_and_stash(pre_of(guest, 8, pre, sizeof(pre), 0, 1)), "chunk_cap_first");
    commit_of(post, sizeof(post), 2);
    expect(!tooie_audio_capture::begin_and_stash(pre_of(guest_b, 8, pre_b, sizeof(pre_b), 0, 1)),
           "chunk_cap_second_rejected");
    tooie_audio_capture::shutdown();

    opt.max_chunks = 4;
    expect(tooie_audio_capture::init(opt) == tooie_audio_capture::InitResult::Armed, "cross_armed");
    std::string cross_dir = tooie_audio_capture::output_dir();
    std::atomic<int> stage{0};
    std::atomic<int> stashed{0};
    std::thread producer([&] {
        stashed.store(tooie_audio_capture::begin_and_stash(pre_of(guest, 8, pre, sizeof(pre), 0, 7)) ? 1 : -1);
        stage.store(1);
        while (stage.load() == 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (stashed.load() == 1) {
            commit_of(post, sizeof(post), 8);
        }
        stage.store(3);
    });
    auto cross_wait = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (stage.load() != 1 && std::chrono::steady_clock::now() < cross_wait) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    expect(stashed.load() == 1, "cross_stash");
    std::thread stopper([] { tooie_audio_capture::shutdown(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    expect(!tooie_audio_capture::flush_completed(), "cross_waits_for_producer");
    stage.store(2);
    producer.join();
    stopper.join();
    expect(read_file(cross_dir + "/guest_raw_s16le.pcm").size() == 16, "cross_flushed_one");
    expect(std::memcmp(read_file(cross_dir + "/guest_raw_s16le.pcm").data(), guest, 16) == 0,
           "cross_bytes");

    opt.max_chunks = 8;
    opt.max_bytes = 4096;
    expect(tooie_audio_capture::init(opt) == tooie_audio_capture::InitResult::Armed, "race_armed");
    std::string race_dir = tooie_audio_capture::output_dir();
    std::atomic<int> race_go{0};
    std::thread racer([&] {
        if (tooie_audio_capture::begin_and_stash(pre_of(guest, 8, pre, sizeof(pre), 1, 1))) {
            commit_of(post, sizeof(post), 2);
        }
        race_go.store(1, std::memory_order_release);
        for (int i = 0; i < 32 && tooie_audio_capture::hot(); ++i) {
            if (tooie_audio_capture::begin_and_stash(pre_of(guest, 8, pre, sizeof(pre), 1, 1))) {
                commit_of(post, sizeof(post), 2);
            }
        }
    });
    while (race_go.load(std::memory_order_acquire) == 0) {
    }
    std::thread race_stop([] { tooie_audio_capture::shutdown(); });
    racer.join();
    race_stop.join();
    std::string race_pcm = read_file(race_dir + "/guest_raw_s16le.pcm");
    expect(!race_pcm.empty(), "race_not_vacuous");
    expect(race_pcm.size() % 16 == 0, "race_aligned");
    bool race_bytes_ok = true;
    for (size_t off = 0; off < race_pcm.size(); off += 16) {
        if (std::memcmp(race_pcm.data() + off, guest, 16) != 0) {
            race_bytes_ok = false;
        }
    }
    expect(race_bytes_ok, "race_bytes");

    opt.seconds = 10;
    opt.max_chunks = 4;
    opt.max_bytes = 4096;
    expect(tooie_audio_capture::init(opt) == tooie_audio_capture::InitResult::Armed, "window_armed");
    std::string window_dir = tooie_audio_capture::output_dir();
    auto started = std::chrono::steady_clock::now();
    expect(tooie_audio_capture::begin_and_stash(pre_of(guest, 8, pre, sizeof(pre), 0, 3)), "window_stash");
    commit_of(post, sizeof(post), 4);
    while (!tooie_audio_capture::flush_completed()) {
        if (std::chrono::steady_clock::now() - started > std::chrono::seconds(13)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    auto elapsed = std::chrono::steady_clock::now() - started;
    expect(tooie_audio_capture::flush_completed(), "window_flushed_without_more_chunks");
    expect(elapsed >= std::chrono::milliseconds(9500), "window_not_early");
    expect(elapsed <= std::chrono::seconds(13), "window_not_late");
    expect(!tooie_audio_capture::begin_and_stash(pre_of(guest_b, 8, pre_b, sizeof(pre_b), 0, 5)),
           "window_closed");
    expect(read_file(window_dir + "/summary.txt").find("stopped_reason=window") != std::string::npos,
           "window_reason");
    expect(read_file(window_dir + "/guest_raw_s16le.pcm").size() == 16, "window_one_chunk");
    tooie_audio_capture::shutdown();

    if (locked != MAP_FAILED) {
        ::munmap(locked, 4096);
    }
    if (g_failures != 0) {
        std::fprintf(stderr, "%d failure(s)\n", g_failures);
        return 1;
    }
    std::printf("tooie_audio_capture_test PASS parent=%s\n", parent.c_str());
    return 0;
}
