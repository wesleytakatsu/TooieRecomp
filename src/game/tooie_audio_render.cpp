#include "tooie_guest_memory.h"
#include "tooie_audio_render.h"

#include "recomp.h"
#include "tooie_bgm_volume.h"

#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

extern "C" void func_80016ED0(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_800178C4(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80017404(uint8_t* rdram, recomp_context* ctx);
extern "C" void func_80017D74(uint8_t* rdram, recomp_context* ctx);

namespace tooie_audio_render {
namespace {
// Low-level sequence player (see audio-sequence-volume): 80016ED0(slot, id) starts or stops (-1) a sequence,
// 800178C4(slot, mask, transition) requests a channel mask, stored at 0x8007AED0 + slot * 4.
constexpr uint32_t kSequenceOff = 0x26C;
constexpr uint32_t kCspStateOff = 0x20C;
constexpr uint32_t kTempoRequests = 0x8007AEA0u;  // 80017D74: last requested microseconds per beat per slot.
constexpr int kGameSelect = 0x67;
constexpr int kReadyFrames = 90;     // Game Select playing this long before the first entry.
constexpr int kGapFrames = 45;       // Silence between entries.
constexpr int kMaskFrames = 30;      // Re-request the mask and volume while the sequence starts.
constexpr int16_t kVolume = 0x7FFF;  // Full authored sequence volume (the high-level fades are bypassed).

struct Entry {
    std::string name;
    int sequence = -1;
    bool has_mask = false;
    uint32_t mask = 0;
    double seconds = 0;
    double bpm = 0;  // Area tempo from the preset; 0 keeps the sequence's own tempo.
};

gpr guest(uint32_t p) { return static_cast<gpr>(static_cast<int32_t>(p)); }

struct State {
    bool loaded = false, enabled = false;
    std::vector<Entry> plan;
    std::filesystem::path out;
    // Guest thread.
    enum class Phase { Wait, Gap, Play, Done } phase = Phase::Wait;
    int ready = 0;
    unsigned slot = 0;
    size_t index = 0;
    int phase_frames = 0;
    // Shared with the audio thread.
    std::mutex mutex;
    std::ofstream wav;
    uint64_t wav_frames = 0;
    std::atomic<bool> owns_output{false};
    std::atomic<bool> recording{false};
    std::atomic<uint64_t> recorded{0};
};

State& state() {
    static State s;
    return s;
}

void load() {
    State& s = state();
    if (s.loaded) return;
    s.loaded = true;
    const char* plan = std::getenv("TOOIE_TEST_AUDIO_RENDER");
    if (plan == nullptr || plan[0] == '\0') return;
    std::ifstream in{std::filesystem::u8path(plan)};
    for (std::string line; std::getline(in, line);) {
        if (line.empty() || line[0] == '#') continue;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::stringstream fields(line);
        Entry e;
        std::string seq, mask, seconds;
        if (!std::getline(fields, e.name, '\t') || !std::getline(fields, seq, '\t') ||
            !std::getline(fields, mask, '\t') || !std::getline(fields, seconds, '\t'))
            continue;
        e.sequence = static_cast<int>(std::strtol(seq.c_str(), nullptr, 16));
        e.has_mask = mask != "-";
        if (e.has_mask) e.mask = static_cast<uint32_t>(std::strtoul(mask.c_str(), nullptr, 16));
        e.seconds = std::atof(seconds.c_str());
        std::string bpm;
        if (std::getline(fields, bpm, '\t')) e.bpm = std::atof(bpm.c_str());
        if (e.sequence >= 0 && e.seconds > 0) s.plan.push_back(e);
    }
    const char* out = std::getenv("TOOIE_TEST_AUDIO_RENDER_OUT");
    s.out = out && out[0] ? std::filesystem::u8path(out) : std::filesystem::u8path(plan).parent_path();
    std::error_code ec;
    std::filesystem::create_directories(s.out, ec);
    s.enabled = !s.plan.empty();
}

void write_u32(std::ofstream& f, uint32_t v) { for (int i = 0; i < 4; ++i) f.put(static_cast<char>((v >> (8 * i)) & 0xFF)); }
void write_u16(std::ofstream& f, uint16_t v) { f.put(static_cast<char>(v & 0xFF)); f.put(static_cast<char>(v >> 8)); }

void wav_header(std::ofstream& f, uint64_t frames) {
    const uint32_t data = static_cast<uint32_t>(frames * 8);  // 32-bit float stereo
    f.write("RIFF", 4); write_u32(f, 36 + data); f.write("WAVE", 4);
    f.write("fmt ", 4); write_u32(f, 16); write_u16(f, 3); write_u16(f, 2); write_u32(f, 48000);
    write_u32(f, 48000 * 8); write_u16(f, 8); write_u16(f, 32);
    f.write("data", 4); write_u32(f, data);
}

void open_wav(const std::string& file) {
    State& s = state();
    std::lock_guard<std::mutex> lock(s.mutex);
    s.wav.open(s.out / std::filesystem::u8path(file), std::ios::binary | std::ios::trunc);
    wav_header(s.wav, 0);
    s.wav_frames = 0;
    s.recorded = 0;
    s.recording = true;
}

void close_wav() {
    State& s = state();
    std::lock_guard<std::mutex> lock(s.mutex);
    s.recording = false;
    if (!s.wav.is_open()) return;
    s.wav.seekp(0);
    wav_header(s.wav, s.wav_frames);
    s.wav.close();
}

void call_start(uint8_t* rdram, recomp_context* ctx, unsigned slot, int sequence) {
    recomp_context local = *ctx;
    local.r4 = static_cast<gpr>(slot);
    local.r5 = static_cast<gpr>(static_cast<int32_t>(sequence));
    func_80016ED0(rdram, &local);
}

void call_volume(uint8_t* rdram, recomp_context* ctx, unsigned slot, int16_t volume) {
    recomp_context local = *ctx;
    local.r4 = static_cast<gpr>(slot);
    local.r5 = static_cast<gpr>(static_cast<int32_t>(volume));
    func_80017404(rdram, &local);
}

// 80017D74(slot, microseconds per beat, transition seconds as float bits): 0.0 sets the tempo at once (80026330).
void call_tempo(uint8_t* rdram, recomp_context* ctx, unsigned slot, double bpm) {
    recomp_context local = *ctx;
    local.r4 = static_cast<gpr>(slot);
    local.r5 = static_cast<gpr>(static_cast<int32_t>(60000000.0 / bpm));
    local.r6 = 0;
    func_80017D74(rdram, &local);
}

// Current tempo as 800260B0 reads it: seqp+0x24 divided by the float at (seqp+0x18)+8 is microseconds per beat.
double current_bpm(uint8_t* rdram, unsigned slot) {
    const uint32_t seqp = tooie_bgm::seqp_addr(slot);
    const uint32_t seq = MEM_W(0x18, guest(seqp));
    if (!tooie_memory::contains(seq, 12)) return 0;
    const uint32_t bits = MEM_W(0x8, guest(seq));
    float divisor;
    std::memcpy(&divisor, &bits, sizeof(divisor));
    const double uspb = divisor > 0 ? static_cast<double>(MEM_W(0x24, guest(seqp))) / divisor : 0;
    return uspb > 0 ? 60000000.0 / uspb : 0;
}

void call_mask(uint8_t* rdram, recomp_context* ctx, unsigned slot, uint32_t mask) {
    recomp_context local = *ctx;
    local.r4 = static_cast<gpr>(slot);
    local.r5 = static_cast<gpr>(static_cast<int32_t>(mask));
    local.r6 = 0;
    func_800178C4(rdram, &local);
}

int sequence_in(uint8_t* rdram, unsigned slot) {
    return MEM_H(kSequenceOff, guest(tooie_bgm::player_addr(slot)));
}

std::string file_name(size_t index, const Entry& e) {
    std::string safe;
    for (char c : e.name) safe += (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') ? c : '_';
    char prefix[16];
    std::snprintf(prefix, sizeof(prefix), "%03zu_", index);
    return prefix + safe + ".wav";
}

}

bool enabled() {
    load();
    return state().enabled;
}

bool keeps_mods() {
    static const bool keep = [] { const char* v = std::getenv("TOOIE_TEST_AUDIO_RENDER_MOD"); return v && v[0] == '1'; }();
    return enabled() && keep;
}

bool mutes(unsigned slot) {
    State& s = state();
    return s.enabled && s.owns_output && slot != s.slot;
}

void on_frame(uint8_t* rdram, recomp_context* ctx) {
    if (!enabled()) return;
    State& s = state();
    switch (s.phase) {
    case State::Phase::Wait:
        for (unsigned slot = 0; slot < tooie_bgm::kSlotCount; ++slot) {
            if (sequence_in(rdram, slot) == kGameSelect && MEM_W(kCspStateOff, guest(tooie_bgm::player_addr(slot))) != 0) {
                if (++s.ready >= kReadyFrames) {
                    s.slot = slot;
                    s.owns_output = true;
                    call_start(rdram, ctx, s.slot, -1);
                    s.phase = State::Phase::Gap;
                    s.phase_frames = 0;
                }
                return;
            }
        }
        s.ready = 0;
        return;
    case State::Phase::Gap:
        if (++s.phase_frames < kGapFrames) return;
        if (s.index >= s.plan.size()) {
            s.phase = State::Phase::Done;
            return;
        }
        {
            const Entry& e = s.plan[s.index];
            open_wav(file_name(s.index, e));
            call_start(rdram, ctx, s.slot, e.sequence);
            call_volume(rdram, ctx, s.slot, kVolume);
            if (e.has_mask) call_mask(rdram, ctx, s.slot, e.mask);
            if (e.bpm > 0) call_tempo(rdram, ctx, s.slot, e.bpm);
        }
        s.phase = State::Phase::Play;
        s.phase_frames = 0;
        return;
    case State::Phase::Play: {
        const Entry& e = s.plan[s.index];
        ++s.phase_frames;
        if (s.phase_frames <= kMaskFrames) {
            call_volume(rdram, ctx, s.slot, kVolume);
            if (e.has_mask) call_mask(rdram, ctx, s.slot, e.mask);
        }
        // The sequence's own tempo event at its start overrides an early request, and 80017D74 ignores a repeated
        // request (it keeps the last one at 0x8007AEA0 + slot * 4): clear that and ask again until the tempo holds.
        if (e.bpm > 0) {
            const double now = current_bpm(rdram, s.slot);
            if (now > 0 && std::abs(now - e.bpm) > 0.25) {
                MEM_W(s.slot * 4, guest(kTempoRequests)) = 0xFFFFFFFFu;
                call_tempo(rdram, ctx, s.slot, e.bpm);
            }
        }
        const double seconds = static_cast<double>(s.recorded.load()) / 48000.0;
        if (seconds < e.seconds) return;
        close_wav();
        call_start(rdram, ctx, s.slot, -1);
        ++s.index;
        s.phase = State::Phase::Gap;
        s.phase_frames = 0;
        return;
    }
    case State::Phase::Done:
        return;
    }
}

bool capture(float* stereo, size_t frames) {
    State& s = state();
    if (!s.enabled || !s.owns_output) return false;
    if (s.recording) {
        std::lock_guard<std::mutex> lock(s.mutex);
        if (s.recording && s.wav.is_open()) {
            // The host already scaled the native samples by 0.5 and the main volume; float keeps the full range.
            s.wav.write(reinterpret_cast<const char*>(stereo), static_cast<std::streamsize>(frames * 2 * sizeof(float)));
            s.wav_frames += frames;
            s.recorded += frames;
        }
    }
    return true;
}
}
