#include "tooie_audio_mod.h"

#define DR_MP3_NO_STDIO
#include "../../lib/dr_libs/dr_mp3.h"
#include <json/json.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace tooie_audio_mod {
namespace {
constexpr uint32_t rate = 48000;
constexpr size_t max_file = 32 * 1024 * 1024;
constexpr size_t max_pack = 2048ull * 1024 * 1024;  // Compressed MP3 bytes listed by one pack.
constexpr size_t max_rules = 1024;
constexpr size_t max_layers = 16;
constexpr size_t small_clip = 1024 * 1024;           // Jingles: kept in memory once loaded.
std::atomic<size_t> eager_limit{96ull * 1024 * 1024};
std::atomic<size_t> resident_budget{[] {
    const char* v = std::getenv("TOOIE_AUDIO_MOD_CACHE_MB");
    const long mb = v ? std::strtol(v, nullptr, 10) : 0;
    return static_cast<size_t>(mb > 0 ? mb : 384) * 1024 * 1024;
}()};

bool safe_path(const std::string& path) {
    if (path.empty() || path.front() == '/' || path.find_first_of(":\\") != std::string::npos) return false;
    size_t begin = 0;
    while (begin <= path.size()) {
        size_t end = path.find('/', begin);
        auto part = path.substr(begin, end == std::string::npos ? end : end - begin);
        if (part.empty() || part == "." || part == "..") return false;
        if (end == std::string::npos) break;
        begin = end + 1;
    }
    return path.ends_with(".mp3");
}

using Bytes = std::vector<char>;
using SeekTable = std::vector<drmp3_seek_point>;
// One MP3 and its playback contract. Bounds are fixed when the pack loads; for on-demand packs the decoded length is
// checked by the loader, which publishes the seek table and then the ready state (release/acquire).
struct Clip {
    std::string file;
    size_t size = 0;
    uint64_t start = 0, end = 0, loop_start = 0, loop_end = 0;
    bool end_given = false;
    float gain = 1;
    double bpm = 0;  // Tempo the MP3 was recorded at; layers at other tempos follow the native beat position.
    mutable std::atomic<int> state{0};  // 0 not checked yet, 1 checked, -1 rejected
    mutable std::shared_ptr<const SeekTable> seek;
    mutable std::atomic<std::shared_ptr<const Bytes>> bytes;
    mutable std::atomic<uint64_t> last_use{0};
    bool lazy = false;
    bool ready() const { return state.load(std::memory_order_acquire) == 1 && bytes.load() != nullptr; }
};
struct Rule {
    int sequence;
    uint32_t mask = 0, value = 0;
    std::shared_ptr<const Clip> clip;
    uint32_t native_mask = 0;
};
struct Slot {
    int sequence = -1;
    int16_t authored = 0;
    uint32_t channels = ~0u;
    uint64_t serial = 0;
    NativeChannels native;
};
std::mutex state_mutex;
std::array<Slot, slot_count> states;
std::array<std::atomic<uint64_t>, slot_count> current_serial{};
std::array<std::atomic<uint64_t>, slot_count> finished_serial{};
std::array<std::atomic<double>, slot_count> target_tempo{};
// Problems with a pack (a rejected MP3) go to stderr; the original music keeps playing.
void report(const std::string& line) { std::fprintf(stderr, "[audio-mod] %s\n", line.c_str()); }
}

struct Pack {
    std::vector<Rule> rules;
    Reader reader;  // On-demand packs only.
    bool lazy = false;
};

namespace {
struct Entry { std::string id; size_t order; std::shared_ptr<const Pack> pack; };
using Catalog = std::vector<Entry>;
std::mutex catalog_mutex;
Catalog entries;
std::atomic<std::shared_ptr<const Catalog>> published{std::make_shared<const Catalog>()};

void publish() {
    std::stable_sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.order < b.order; });
    published.store(std::make_shared<const Catalog>(entries));
}
struct Selection {
    std::array<const Rule*, max_layers> layers{};
    size_t count = 0;
    const Rule* active = nullptr;  // The rule whose selector matched.
};
// An MP3 recorded at an area tempo replaces only that area: the same channels at another tempo keep the original.
bool tempo_fits(const Clip& clip, double target) { return clip.bpm <= 0 || target <= 0 || std::abs(clip.bpm - target) < 0.5; }
Selection select(const Catalog& catalog, int sequence, uint32_t channels, double target_bpm = 0) {
    // First entry in the Mods list wins, matching the texture pack order. A rule (or layer family) whose MP3s are not
    // ready yet is skipped, so the original music keeps playing meanwhile.
    for (const auto& entry : catalog) {
        for (const auto& rule : entry.pack->rules) {
            if (rule.sequence != sequence || (channels & rule.mask) != rule.value || !tempo_fits(*rule.clip, target_bpm)) continue;
            Selection result;
            result.active = &rule;
            bool ready = true;
            if (!rule.native_mask) { result.layers[result.count++] = &rule; ready = rule.clip->ready(); }
            else for (const auto& layer : entry.pack->rules) {
                if (layer.sequence != sequence || !layer.native_mask) continue;
                // One layer per channel group: of the same channels recorded at several tempos, the closest one.
                size_t same = result.count;
                for (size_t i = 0; i < result.count; ++i) if (result.layers[i]->native_mask == layer.native_mask) same = i;
                auto distance = [&](const Rule* r) { return std::abs(r->clip->bpm - (target_bpm > 0 ? target_bpm : r->clip->bpm)); };
                if (same < result.count) {
                    if (&layer == &rule || (result.layers[same] != &rule && distance(&layer) < distance(result.layers[same])))
                        result.layers[same] = &layer;
                    continue;
                }
                if (result.count < max_layers) result.layers[result.count++] = &layer;
            }
            for (size_t i = 0; i < result.count; ++i) ready &= result.layers[i]->clip->ready();
            if (!ready) break;  // Try the next pack.
            return result;
        }
    }
    return {};
}
uint64_t frame_field(const nlohmann::json& j, const char* key, uint64_t fallback) {
    if (!j.contains(key)) return fallback;
    if (!j.at(key).is_number_unsigned()) throw std::runtime_error(std::string(key) + " must be a nonnegative integer");
    return j.at(key).get<uint64_t>();
}

// Decodes the MP3 once to check its format and bounds and to build the seek table. Throws on a bad file.
void check_clip(const Clip& clip, const Bytes& bytes) {
    auto decoder = std::make_unique<drmp3>();
    if (!drmp3_init_memory(decoder.get(), bytes.data(), bytes.size(), nullptr)) throw std::runtime_error("Invalid MP3: " + clip.file);
    struct Cleanup { drmp3* p; ~Cleanup() { drmp3_uninit(p); } } cleanup{decoder.get()};
    if (decoder->sampleRate != rate || decoder->channels != 2) throw std::runtime_error("MP3 must be 48000 Hz stereo: " + clip.file);
    auto count = drmp3_get_pcm_frame_count(decoder.get());
    if (!count || count > rate * 1200ull) throw std::runtime_error("Invalid MP3 length: " + clip.file);
    auto& c = const_cast<Clip&>(clip);
    if (!c.end_given) c.end = count;
    if (c.start >= c.end || c.end > count || (c.loop_end && c.loop_end > c.end))
        throw std::runtime_error("Invalid audio bounds or gain: " + clip.file);
    drmp3_uint32 points = 1024;
    auto seek = std::make_shared<SeekTable>(points);
    if (!drmp3_calculate_seek_points(decoder.get(), &points, seek->data())) throw std::runtime_error("Unable to index MP3: " + clip.file);
    seek->resize(points);
    clip.seek = std::move(seek);
    clip.state.store(1, std::memory_order_release);
}

// Background loader for on-demand packs: checks every MP3 once, keeps small ones, and loads the MP3s of the
// sequences the game is playing, evicting the least recently used others beyond the memory budget.
struct Job { std::weak_ptr<const Pack> pack; std::shared_ptr<const Clip> clip; bool keep; };
struct Loader {
    std::mutex mutex;
    std::condition_variable wake, idle;
    std::deque<Job> jobs;
    bool busy = false, started = false;
    std::atomic<size_t> resident{0};
    std::vector<std::weak_ptr<const Clip>> resident_clips;
    std::atomic<uint64_t> clock{0};
    std::array<std::atomic<int>, slot_count> playing{};
    Loader() { for (auto& p : playing) p = -1; }
};
Loader& loader() { static Loader* l = new Loader(); return *l; }  // Never destroyed: the thread may outlive main.

bool pinned(const Rule& rule) {
    for (auto& p : loader().playing) if (p.load() == rule.sequence) return true;
    return false;
}
void evict(const Pack* current) {
    auto& l = loader();
    const size_t budget = resident_budget.load();
    while (l.resident.load() > budget) {
        // Least recently used clip that is not small and not used by a playing sequence.
        auto catalog = published.load();
        std::shared_ptr<const Clip> victim;
        for (auto& weak : l.resident_clips) {
            auto clip = weak.lock();
            if (!clip || !clip->bytes.load() || clip->size <= small_clip) continue;
            bool in_use = false;
            for (const auto& e : *catalog) for (const auto& r : e.pack->rules) if (r.clip == clip && pinned(r)) in_use = true;
            if (in_use) continue;
            if (!victim || clip->last_use.load() < victim->last_use.load()) victim = clip;
        }
        (void)current;
        if (!victim) return;
        victim->bytes.store(nullptr);
        l.resident -= victim->size;
    }
}
void process(const Job& job) {
    auto pack = job.pack.lock();
    if (!pack) return;
    const Clip& clip = *job.clip;
    if (clip.state.load() < 0) return;
    const bool need_check = clip.state.load() == 0;
    const bool need_bytes = job.keep && !clip.bytes.load();
    if (!need_check && !need_bytes) { clip.last_use = ++loader().clock; return; }
    try {
        auto bytes = std::make_shared<const Bytes>(pack->reader(clip.file, max_file));
        if (bytes->empty() || bytes->size() != clip.size) throw std::runtime_error("Missing or changed audio file: " + clip.file);
        if (need_check) check_clip(clip, *bytes);
        if (job.keep || clip.size <= small_clip) {
            clip.last_use = ++loader().clock;
            clip.bytes.store(bytes);
            auto& l = loader();
            l.resident += clip.size;
            l.resident_clips.push_back(job.clip);
            std::erase_if(l.resident_clips, [](const std::weak_ptr<const Clip>& w) { return w.expired(); });
            evict(pack.get());
        }
    }
    catch (const std::exception& e) {
        clip.state.store(-1);
        report(std::string("rejected ") + e.what() + " (the original music plays for this rule)");
    }
}
void loader_thread() {
    auto& l = loader();
    for (;;) {
        Job job;
        {
            std::unique_lock lock(l.mutex);
            l.busy = false;
            if (l.jobs.empty()) l.idle.notify_all();
            l.wake.wait(lock, [&] { return !l.jobs.empty(); });
            job = std::move(l.jobs.front());
            l.jobs.pop_front();
            l.busy = true;
        }
        process(job);
    }
}
void queue(Job job, bool urgent) {
    auto& l = loader();
    std::lock_guard lock(l.mutex);
    if (!l.started) { std::thread(loader_thread).detach(); l.started = true; }
    if (urgent) l.jobs.push_front(std::move(job));
    else l.jobs.push_back(std::move(job));
    l.wake.notify_one();
}
// The game started (or kept) a sequence: make its MP3s resident before the next ones.
void request(int sequence) {
    if (sequence < 0) return;
    auto catalog = published.load();
    for (const auto& entry : *catalog) {
        if (!entry.pack->lazy) continue;
        for (const auto& rule : entry.pack->rules) {
            if (rule.sequence != sequence) continue;
            if (rule.clip->bytes.load()) { rule.clip->last_use = ++loader().clock; continue; }
            if (rule.clip->state.load() >= 0) queue({entry.pack, rule.clip, true}, true);
        }
    }
}

// Owned exclusively by the audio consumer. Compressed bytes and seek tables are
// prepared before a clip is selected; playback performs bounded memory decoding only.
struct Stream {
    std::shared_ptr<const Clip> clip;
    std::shared_ptr<const Bytes> bytes;
    std::unique_ptr<drmp3> decoder;
    uint64_t position = 0;
    bool ended = false;
    ~Stream() { clear(); }
    Stream() = default;
    Stream(const Stream&) = delete;
    Stream& operator=(const Stream&) = delete;
    void clear() {
        if (decoder) drmp3_uninit(decoder.get());
        decoder.reset(); clip.reset(); bytes.reset(); ended = false; position = 0;
    }
    // Position in the file for a timeline offset from the clip start, wrapped into the loop.
    static uint64_t place(const Clip& c, uint64_t timeline) {
        uint64_t position = c.start + timeline;
        if (c.loop_end && position >= c.loop_end) position = c.loop_start + (position - c.loop_start) % (c.loop_end - c.loop_start);
        return position;
    }
    bool open(std::shared_ptr<const Clip> target, uint64_t elapsed) {
        clear();
        if (!target) return false;
        auto data = target->bytes.load();
        if (!data || target->state.load(std::memory_order_acquire) != 1) return false;
        auto next = std::make_unique<drmp3>();
        if (!drmp3_init_memory(next.get(), data->data(), data->size(), nullptr)) return false;
        decoder = std::move(next); clip = std::move(target); bytes = std::move(data);
        drmp3_bind_seek_table(decoder.get(), static_cast<drmp3_uint32>(clip->seek->size()), const_cast<drmp3_seek_point*>(clip->seek->data()));
        position = place(*clip, elapsed);
        if (position >= clip->end) { ended = true; return true; }
        if (!drmp3_seek_to_pcm_frame(decoder.get(), position)) { clear(); return false; }
        return true;
    }
    void read(float* out, size_t frames) {
        std::fill(out, out + frames * 2, 0.0f);
        if (!decoder || ended) return;
        size_t done = 0;
        while (done < frames) {
            const uint64_t end = clip->loop_end ? clip->loop_end : clip->end;
            if (position >= end) {
                if (!clip->loop_end) { ended = true; break; }
                if (!drmp3_seek_to_pcm_frame(decoder.get(), clip->loop_start)) { ended = true; break; }
                position = clip->loop_start;
            }
            size_t count = static_cast<size_t>(std::min<uint64_t>(frames - done, end - position));
            size_t got = static_cast<size_t>(drmp3_read_pcm_frames_f32(decoder.get(), count, out + done * 2));
            done += got; position += got;
            if (got != count) { ended = true; break; }
        }
    }
};
struct Voice {
    struct Layer {
        Stream stream;
        std::shared_ptr<const Clip> assigned;  // Kept while the layer is silent and its decoder is closed.
        uint32_t native_mask = 0;
        float weight = 0;
    };
    std::array<Layer, max_layers> layers;
    int sequence = -1;
    uint64_t serial = 0, elapsed = 0;
    double beats = 0;     // Native beat position, advanced at the sequence's current tempo.
    bool beats_valid = false;
    uint64_t timeline(const Clip& c) const {
        if (c.bpm > 0 && beats_valid) return static_cast<uint64_t>(std::llround(beats * 60.0 / c.bpm * rate));
        return elapsed;
    }
    float gain = 0;
};
std::array<Voice, slot_count> voices;
void mix_layer(Voice::Layer& layer, float* stereo, size_t frames, float wanted, float gain,
    float weight, std::array<float, 1024 * 2>& pcm) {
    const float clip_gain = layer.stream.clip ? layer.stream.clip->gain : 0;
    for (size_t offset = 0; offset < frames;) {
        size_t n = std::min<size_t>(1024, frames - offset);
        layer.stream.read(pcm.data(), n);
        for (size_t i = 0; i < n; ++i) {
            gain += (wanted - gain) * (1.0f / 240.0f);
            // Follow the native envelope, with a 5 ms dezipper.
            layer.weight += (weight - layer.weight) * (1.0f / 240.0f);
            if (weight == 0 && layer.weight < 0.000001f) layer.weight = 0;
            for (unsigned c = 0; c < 2; ++c) {
                stereo[(offset + i) * 2 + c] += pcm[i * 2 + c] * clip_gain * layer.weight * gain;
            }
        }
        offset += n;
    }
}
float native_level(uint8_t q) { return q == 255 ? 1.0f : q / 256.0f; }  // 80029DCC: 255 bypasses multiplication.
}

std::vector<float> layer_weights(const std::vector<uint32_t>& masks, const NativeChannels& native) {
    // Non-negative least squares (Lawson-Hanson) over the channels the layers cover: each layer file is the mix of
    // its channel group, so the closest non-negative combination of groups reproduces the native channel gains. With
    // disjoint groups this is each group's mean gain; overlapping groups (one area adds channels to another) still
    // resolve to the single matching layer in steady state and blend during the native crossfade.
    const size_t n = masks.size();
    std::vector<float> w(n, 0.0f);
    if (!n) return w;
    uint32_t cover = 0;
    for (auto m : masks) cover |= m;
    std::vector<unsigned> rows;
    for (unsigned c = 0; c < channel_count; ++c) if (cover & (1u << c)) rows.push_back(c);
    auto a = [&](size_t r, size_t j) { return (masks[j] >> rows[r]) & 1u ? 1.0 : 0.0; };
    std::vector<double> g(rows.size());
    for (size_t r = 0; r < rows.size(); ++r) g[r] = native_level(native.gain[rows[r]]);
    std::vector<double> x(n, 0.0);
    std::vector<bool> passive(n, false);
    auto solve = [&](std::vector<double>& z) {
        // Least squares restricted to the passive set: (A_P' A_P) z_P = A_P' g, with a tiny ridge for identical groups.
        std::vector<size_t> p;
        for (size_t j = 0; j < n; ++j) if (passive[j]) p.push_back(j);
        const size_t k = p.size();
        std::vector<double> m(k * (k + 1), 0.0);
        for (size_t i = 0; i < k; ++i) {
            for (size_t j = 0; j < k; ++j) {
                double s = 0;
                for (size_t r = 0; r < rows.size(); ++r) s += a(r, p[i]) * a(r, p[j]);
                m[i * (k + 1) + j] = s + (i == j ? 1e-9 : 0.0);
            }
            double s = 0;
            for (size_t r = 0; r < rows.size(); ++r) s += a(r, p[i]) * g[r];
            m[i * (k + 1) + k] = s;
        }
        for (size_t col = 0; col < k; ++col) {
            size_t pivot = col;
            for (size_t r = col + 1; r < k; ++r) if (std::abs(m[r * (k + 1) + col]) > std::abs(m[pivot * (k + 1) + col])) pivot = r;
            if (pivot != col) for (size_t c = 0; c <= k; ++c) std::swap(m[col * (k + 1) + c], m[pivot * (k + 1) + c]);
            const double d = m[col * (k + 1) + col];
            if (std::abs(d) < 1e-12) continue;
            for (size_t r = 0; r < k; ++r) {
                if (r == col) continue;
                const double f = m[r * (k + 1) + col] / d;
                for (size_t c = col; c <= k; ++c) m[r * (k + 1) + c] -= f * m[col * (k + 1) + c];
            }
        }
        std::fill(z.begin(), z.end(), 0.0);
        for (size_t i = 0; i < k; ++i) {
            const double d = m[i * (k + 1) + i];
            z[p[i]] = std::abs(d) < 1e-12 ? 0.0 : m[i * (k + 1) + k] / d;
        }
    };
    std::vector<double> z(n);
    for (size_t outer = 0; outer < 3 * n + 3; ++outer) {
        // Gradient of the residual for the active (zero) variables.
        size_t best = n;
        double best_gradient = 1e-7;
        for (size_t j = 0; j < n; ++j) {
            if (passive[j]) continue;
            double s = 0;
            for (size_t r = 0; r < rows.size(); ++r) {
                double ax = 0;
                for (size_t q = 0; q < n; ++q) ax += a(r, q) * x[q];
                s += a(r, j) * (g[r] - ax);
            }
            if (s > best_gradient) { best_gradient = s; best = j; }
        }
        if (best == n) break;
        passive[best] = true;
        for (size_t inner = 0; inner < 3 * n + 3; ++inner) {
            solve(z);
            bool feasible = true;
            for (size_t j = 0; j < n; ++j) if (passive[j] && z[j] <= 1e-9) feasible = false;
            if (feasible) { x = z; break; }
            double alpha = 1.0;
            for (size_t j = 0; j < n; ++j)
                if (passive[j] && z[j] <= 1e-9) alpha = std::min(alpha, x[j] / std::max(1e-12, x[j] - z[j]));
            for (size_t j = 0; j < n; ++j) x[j] += alpha * (z[j] - x[j]);
            for (size_t j = 0; j < n; ++j) if (passive[j] && x[j] <= 1e-9) { passive[j] = false; x[j] = 0; }
        }
    }
    for (size_t j = 0; j < n; ++j) w[j] = static_cast<float>(std::clamp(x[j], 0.0, 1.0));
    return w;
}

std::shared_ptr<const Pack> load_pack(const Reader& reader, std::string& error, const Sizer& sizer) {
    try {
        auto manifest = reader("audio.json", 4 * 1024 * 1024);
        auto j = nlohmann::json::parse(manifest.begin(), manifest.end());
        const auto& version = j.at("schema_version");
        if (!version.is_number_integer() || (version != 1 && version != 2) ||
            !j.at("sequences").is_array() || j.at("sequences").empty() || j.at("sequences").size() > max_rules)
            throw std::runtime_error("Unsupported or empty audio manifest");
        const int schema = version.get<int>();
        auto pack = std::make_shared<Pack>();
        // Files shared by several rules (a layer used by more than one area) are one clip.
        std::map<std::string, std::shared_ptr<Clip>> clips;
        // A plain struct: std::pair<..., nlohmann::json> does not compile with clang and libstdc++ 12 (handheld).
        struct Contract { std::shared_ptr<Clip> clip; nlohmann::json terms; };
        std::vector<Contract> contracts;
        size_t total = 0;
        for (const auto& item : j.at("sequences")) {
            const auto& id = item.at("id");
            if (!id.is_number_integer() || id < 0 || id > 32767) throw std::runtime_error("Invalid sequence id");
            int sequence = id.get<int>();
            std::string path = item.at("file").get<std::string>();
            if (!safe_path(path)) throw std::runtime_error("Invalid MP3 path: " + path);
            auto& clip = clips[path];
            nlohmann::json contract = nlohmann::json::object();
            for (const char* key : {"start_frame", "end_frame", "loop_start_frame", "loop_end_frame", "gain", "bpm"})
                if (item.contains(key)) contract[key] = item.at(key);
            if (!clip) {
                clip = std::make_shared<Clip>();
                clip->file = path;
                if (sizer) {
                    auto size = sizer(path);
                    if (!size || !*size || *size > max_file) throw std::runtime_error("Missing or oversized audio file: " + path);
                    clip->size = *size;
                }
                clip->start = frame_field(item, "start_frame", 0);
                clip->end_given = item.contains("end_frame");
                clip->end = frame_field(item, "end_frame", 0);
                clip->loop_start = frame_field(item, "loop_start_frame", 0);
                clip->loop_end = frame_field(item, "loop_end_frame", 0);
                clip->gain = item.value("gain", 1.0f);
                if (item.contains("bpm") && (!item.at("bpm").is_number() || !(item.at("bpm").get<double>() > 0) || item.at("bpm").get<double>() > 1000))
                    throw std::runtime_error("Invalid tempo: " + path);
                clip->bpm = item.value("bpm", 0.0);
                if (!std::isfinite(clip->gain) || clip->gain < 0 || clip->gain > 2 || (clip->end_given && clip->start >= clip->end) ||
                    (clip->loop_end && (clip->loop_start < clip->start || clip->loop_start >= clip->loop_end ||
                        (clip->end_given && clip->loop_end > clip->end))) ||
                    (!clip->loop_end && clip->loop_start)) throw std::runtime_error("Invalid audio bounds or gain: " + path);
                total += clip->size;
                contracts.push_back({clip, contract});
            }
            // Each file has one playback contract; variants can share the same contract.
            else for (const auto& known : contracts) if (known.clip == clip && known.terms != contract)
                throw std::runtime_error("One MP3 cannot have two playback contracts: " + path);
            Rule rule{sequence, 0, 0, clip};
            if (item.contains("channel_mask") != item.contains("channel_value")) throw std::runtime_error("Channel selector requires both mask and value");
            if (item.contains("channel_mask")) {
                if (!item.at("channel_mask").is_number_unsigned() || !item.at("channel_value").is_number_unsigned() ||
                    item.at("channel_mask").get<uint64_t>() > UINT32_MAX || item.at("channel_value").get<uint64_t>() > UINT32_MAX) throw std::runtime_error("Invalid channel selector type");
                rule.mask = item.at("channel_mask").get<uint32_t>();
                rule.value = item.at("channel_value").get<uint32_t>();
                if (!rule.mask || (rule.value & ~rule.mask)) throw std::runtime_error("Invalid channel selector");
            }
            if (item.contains("native_channel_mask")) {
                if (schema != 2 || !item.at("native_channel_mask").is_number_unsigned()) throw std::runtime_error("Native channel layers require schema 2 and an unsigned mask");
                auto mask = item.at("native_channel_mask").get<uint64_t>();
                if (!mask || mask > UINT32_MAX || rule.mask != UINT32_MAX) throw std::runtime_error("Native layers require a nonempty channel group and exact activation selector");
                rule.native_mask = static_cast<uint32_t>(mask);
            }
            for (const auto& previous : pack->rules)
                if (previous.sequence == sequence && previous.mask == rule.mask && previous.value == rule.value && previous.clip->bpm == rule.clip->bpm)
                    throw std::runtime_error("Duplicate sequence selector");
            pack->rules.push_back(std::move(rule));
        }
        if (total > max_pack) throw std::runtime_error("Audio pack exceeds compressed-memory limit");
        // Small packs are read and checked now and stay in memory. Larger ones need a sizer: their MP3s are checked
        // in the background and loaded when their sequence plays.
        pack->lazy = sizer && total > eager_limit.load();
        auto check_layers = [&] {
            for (const auto& rule : pack->rules) {
                if (!rule.native_mask) continue;
                unsigned count = 0;
                for (const auto& layer : pack->rules) {
                    if (layer.sequence != rule.sequence) continue;
                    const auto& a = *rule.clip; const auto& b = *layer.clip;
                    // Layers share the native clock (in beats when they state their tempo); on-demand packs state the
                    // end so it is known before decoding.
                    auto same = [&](uint64_t x, uint64_t y) {
                        if (!a.bpm && !b.bpm) return x == y;
                        if (!a.bpm || !b.bpm) return false;
                        return std::abs(x / 48000.0 * a.bpm / 60.0 - y / 48000.0 * b.bpm / 60.0) < 0.02;
                    };
                    // Rare's player loops each channel on its own, so a channel group can repeat with a period of
                    // its own: layers share the start, and each wraps the shared timeline through its own loop.
                    if (!layer.native_mask || ++count > max_layers || !same(a.start, b.start) || (pack->lazy && !a.end_given))
                        throw std::runtime_error("Native layers require at most sixteen layers with the same start");
                }
            }
        };
        if (pack->lazy) check_layers();
        if (!pack->lazy) {
            size_t read = 0;
            for (auto& [path, clip] : clips) {
                auto bytes = std::make_shared<const Bytes>(reader(path, max_file));
                if (bytes->empty()) throw std::runtime_error("Missing or oversized audio file: " + path);
                read += bytes->size();
                if (read > eager_limit.load() * 2) throw std::runtime_error("Audio pack exceeds compressed-memory limit");
                clip->size = bytes->size();
                check_clip(*clip, *bytes);
                clip->bytes.store(std::move(bytes));
            }
            check_layers();
        }
        else {
            pack->reader = reader;
            for (auto& [path, clip] : clips) clip->lazy = true;
        }
        error.clear(); return pack;
    }
    catch (const std::exception& e) { error = e.what(); return {}; }
}

void enable(const std::string& id, size_t order, std::shared_ptr<const Pack> pack) {
    if (!pack) return;
    {
        std::lock_guard lock(catalog_mutex);
        std::erase_if(entries, [&](const Entry& e) { return e.id == id; });
        entries.push_back({id, order, pack}); publish();
    }
    if (pack->lazy) {
        // Check every MP3 in the background, smallest first (jingles end up in memory right away).
        std::vector<std::shared_ptr<const Clip>> all;
        for (const auto& rule : pack->rules) if (std::find(all.begin(), all.end(), rule.clip) == all.end()) all.push_back(rule.clip);
        std::stable_sort(all.begin(), all.end(), [](const auto& a, const auto& b) { return a->size < b->size; });
        for (auto& clip : all) queue({pack, clip, false}, false);
        for (auto& p : loader().playing) request(p.load());
    }
}
void disable(const std::string& id) {
    std::lock_guard lock(catalog_mutex);
    std::erase_if(entries, [&](const Entry& e) { return e.id == id; }); publish();
}
void reorder(const std::function<size_t(const std::string&)>& get_order) {
    std::lock_guard lock(catalog_mutex);
    for (auto& entry : entries) entry.order = get_order(entry.id);
    publish();
}
bool replaces(int sequence, uint32_t channels) { return select(*published.load(), sequence, channels).count != 0; }
bool replaces(unsigned slot, int sequence, uint32_t channels) {
    if (slot >= slot_count) return replaces(sequence, channels);
    if (finished_serial[slot].load() == current_serial[slot].load() + 1) return false;
    return select(*published.load(), sequence, channels, target_tempo[slot].load()).count != 0;
}
void update(unsigned slot, int sequence, int16_t authored, uint32_t channels, bool restart, const NativeChannels& native) {
    if (slot >= slot_count) return;
    bool started = false;
    {
        std::lock_guard lock(state_mutex);
        auto& state = states[slot];
        started = restart || state.sequence != sequence;
        if (started) ++state.serial;
        current_serial[slot] = state.serial;
        target_tempo[slot] = native.target_bpm;
        state.sequence = sequence; state.authored = authored; state.channels = channels; state.native = native;
    }
    if (started) {
        loader().playing[slot] = sequence;
        request(sequence);
    }
}
void reset() {
    std::lock_guard lock(state_mutex);
    for (auto& state : states) { state.sequence = -1; ++state.serial; }
    for (auto& p : loader().playing) p = -1;
}
void set_memory_limits(size_t budget, size_t eager) { resident_budget = budget; eager_limit = eager; }
bool wait_loader_idle(unsigned milliseconds) {
    auto& l = loader();
    std::unique_lock lock(l.mutex);
    return l.idle.wait_for(lock, std::chrono::milliseconds(milliseconds), [&] { return l.jobs.empty() && !l.busy; });
}
void mix(float* stereo, size_t frames, float main_volume, int bgm_percent) {
    auto catalog = published.load();
    std::array<Slot, slot_count> snapshot;
    { std::lock_guard lock(state_mutex); snapshot = states; }
    std::array<float, 1024 * 2> pcm{};
    bool mixed = false;
    for (unsigned slot = 0; slot < slot_count; ++slot) {
        auto& voice = voices[slot]; const auto& state = snapshot[slot];
        auto selected = select(*catalog, state.sequence, state.channels, state.native.target_bpm);
        const bool restart = voice.sequence != state.sequence || voice.serial != state.serial;
        if (restart) {
            voice.elapsed = 0; voice.gain = 0; voice.beats = 0; voice.beats_valid = false;
            for (auto& layer : voice.layers) { layer.stream.clear(); layer.assigned.reset(); layer.weight = 0; }
        }
        voice.sequence = state.sequence; voice.serial = state.serial;
        const float wanted = std::clamp(state.authored / 32767.0f, 0.0f, 1.0f) *
            std::clamp(bgm_percent, 0, 100) / 100.0f * std::clamp(main_volume, 0.0f, 1.0f) * 0.5f;
        // Layer weights from the native channel gains (or the matched layer when gains are unavailable).
        std::array<float, max_layers> weights{};
        bool native_layers = false;
        std::vector<uint32_t> masks;
        for (size_t index = 0; index < selected.count; ++index) {
            weights[index] = 1;
            if (selected.layers[index]->native_mask) { native_layers = true; masks.push_back(selected.layers[index]->native_mask); }
        }
        if (native_layers) {
            if (state.native.valid) {
                auto solved = layer_weights(masks, state.native);
                for (size_t index = 0; index < selected.count; ++index) weights[index] = solved[index];
            }
            else for (size_t index = 0; index < selected.count; ++index) weights[index] = selected.layers[index] == selected.active ? 1.0f : 0.0f;
        }
        // The native sequence clock runs even without an enabled mapping. This
        // includes zero-weight layers, catalog reorder, fallback and re-enable.
        for (size_t index = 0; index < max_layers; ++index) {
            auto& layer = voice.layers[index];
            const Rule* rule = index < selected.count ? selected.layers[index] : nullptr;
            auto clip = rule ? rule->clip : nullptr;
            const float weight = rule ? weights[index] : 0.0f;
            if (clip != layer.assigned || restart) {
                // A new assignment starts at the current weight; a layer that only went silent keeps its ramp.
                layer.stream.clear();
                layer.assigned = clip;
                layer.native_mask = rule ? rule->native_mask : 0;
                layer.weight = weight;
                if (clip && weight > 0) layer.stream.open(clip, voice.timeline(*clip));
            }
            if (!clip) continue;
            // Silent layers do not decode: they reopen at the native timeline when the game brings them back.
            if (weight == 0 && layer.weight == 0) { if (layer.stream.clip) layer.stream.clear(); continue; }
            if (!layer.stream.clip && !layer.stream.open(clip, voice.timeline(*clip))) continue;
            // A layer recorded at another tempo drifts while the native tempo moves between areas: past 25 ms it seeks
            // back to the native beat position.
            if (clip->bpm > 0 && !layer.stream.ended) {
                const uint64_t ideal = Stream::place(*clip, voice.timeline(*clip));
                const uint64_t actual = layer.stream.position;
                if ((actual > ideal ? actual - ideal : ideal - actual) > rate / 40 && !layer.stream.open(clip, voice.timeline(*clip))) continue;
            }
            mix_layer(layer, stereo, frames, wanted, voice.gain, weight, pcm);
            mixed = true;
        }
        // MP3s without a loop that have ended hand the slot back to the original sequence (marked with serial + 1,
        // so serial 0 never reads as finished).
        if (selected.count && state.sequence >= 0) {
            bool finished = true;
            for (size_t index = 0; index < selected.count; ++index) {
                const Clip& c = *selected.layers[index]->clip;
                if (c.loop_end || c.start + voice.timeline(c) < c.end) finished = false;
            }
            if (finished) finished_serial[slot] = state.serial + 1;
        }
        // Update the master ramp exactly once, independently of layer count.
        voice.gain = wanted + (voice.gain - wanted) * std::pow(1.0f - 1.0f / 240.0f, static_cast<float>(frames));
        if (state.sequence >= 0) {
            voice.elapsed += frames;
            // Beats advance at the native tempo, or at the playing layer's own tempo when the native one is unknown.
            double bpm = state.native.bpm;
            if (!(bpm > 0) && selected.active) bpm = selected.active->clip->bpm;
            if (bpm > 0) { voice.beats += frames / static_cast<double>(rate) * bpm / 60.0; voice.beats_valid = true; }
        }
    }
    if (mixed) for (size_t i = 0; i < frames * 2; ++i) stereo[i] = std::clamp(stereo[i], -1.0f, 1.0f);
}
}
