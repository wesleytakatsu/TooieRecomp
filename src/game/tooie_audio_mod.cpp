#include "tooie_audio_mod.h"

#define DR_MP3_NO_STDIO
#include "../../lib/dr_libs/dr_mp3.h"
#include <json/json.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <map>
#include <mutex>
#include <stdexcept>

namespace tooie_audio_mod {
namespace {
constexpr uint32_t rate = 48000;
constexpr size_t max_file = 32 * 1024 * 1024;
constexpr size_t max_pack = 256 * 1024 * 1024;
constexpr size_t max_layers = 4;

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

struct Clip {
    std::string file;
    std::vector<char> bytes;
    std::vector<drmp3_seek_point> seek;
    uint64_t start = 0, end = 0, loop_start = 0, loop_end = 0;
    float gain = 1;
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
std::mutex log_mutex;
FILE* playback_log = nullptr;
struct CloseLog { ~CloseLog() { if (playback_log) std::fclose(playback_log); } } close_log;
bool trace_enabled() {
    static const bool enabled = [] { const char* v = std::getenv("TOOIE_DIAG_AUDIO_MOD"); return v && std::string(v) == "1"; }();
    return enabled;
}
}

struct Pack { std::vector<Rule> rules; };

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
    const std::string* id = nullptr;
};
Selection select(const Catalog& catalog, int sequence, uint32_t channels) {
    // First entry in the Mods list wins, matching the texture pack order.
    for (const auto& entry : catalog) {
        for (const auto& rule : entry.pack->rules) {
            if (rule.sequence != sequence || (channels & rule.mask) != rule.value) continue;
            Selection result;
            result.id = &entry.id;
            if (!rule.native_mask) result.layers[result.count++] = &rule;
            else for (const auto& layer : entry.pack->rules)
                if (layer.sequence == sequence) result.layers[result.count++] = &layer;
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

// Owned exclusively by the audio consumer. Compressed bytes and seek tables are
// prepared at mod activation; playback performs bounded memory decoding only.
struct Stream {
    std::shared_ptr<const Clip> clip;
    std::unique_ptr<drmp3> decoder;
    uint64_t position = 0;
    bool ended = false;
    ~Stream() { clear(); }
    Stream() = default;
    Stream(const Stream&) = delete;
    Stream& operator=(const Stream&) = delete;
    void clear() {
        if (decoder) drmp3_uninit(decoder.get());
        decoder.reset(); clip.reset(); ended = false; position = 0;
    }
    void swap(Stream& other) {
        clip.swap(other.clip); decoder.swap(other.decoder);
        std::swap(position, other.position); std::swap(ended, other.ended);
    }
    bool open(std::shared_ptr<const Clip> target, uint64_t elapsed) {
        clear();
        if (!target) return false;
        auto next = std::make_unique<drmp3>();
        if (!drmp3_init_memory(next.get(), target->bytes.data(), target->bytes.size(), nullptr)) return false;
        decoder = std::move(next); clip = std::move(target);
        drmp3_bind_seek_table(decoder.get(), static_cast<drmp3_uint32>(clip->seek.size()), const_cast<drmp3_seek_point*>(clip->seek.data()));
        position = clip->start + elapsed;
        if (clip->loop_end && position >= clip->loop_end) position = clip->loop_start + (position - clip->loop_start) % (clip->loop_end - clip->loop_start);
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
        uint32_t native_mask = 0;
        float weight = 0;
        bool announced = false;
        uint32_t logged_channels = ~0u;
        uint64_t positive_samples = 0, reported_samples = 0;
        float logged_weight = -1;
    };
    std::array<Layer, max_layers> layers;
    int sequence = -1;
    uint64_t serial = 0, elapsed = 0;
    float gain = 0;
    std::string mod_id;
};
std::array<Voice, slot_count> voices;
struct PendingProof {
    bool available = false;
    uint32_t channels = 0;
    size_t frames = 0, positive = 0;
    uint64_t timeline = 0, source = 0;
    float weight = 0;
    unsigned native_min = 0, native_max = 0;
};
std::array<std::array<PendingProof, max_layers>, slot_count> pending_proofs;

template<bool RecordProof>
size_t mix_layer(Voice::Layer& layer, float* stereo, size_t frames, float wanted, float gain,
    float weight, std::array<float, 1024 * 2>& pcm) {
    size_t positive = 0;
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
                float contribution = pcm[i * 2 + c] * clip_gain * layer.weight * gain;
                stereo[(offset + i) * 2 + c] += contribution;
                if constexpr (RecordProof) {
                    if (contribution != 0) ++positive;
                }
            }
        }
        offset += n;
    }
    return positive;
}
}

void initialize_diagnostics() {
    const char* path = std::getenv("TOOIE_AUDIO_MOD_LOG");
    if (!path || !*path) return;
#ifdef _WIN32
    playback_log = _wfopen(std::filesystem::u8path(path).c_str(), L"w");
#else
    playback_log = std::fopen(path, "w");
#endif
    if (!playback_log) { std::fprintf(stderr, "[audio-mod] unable to open playback log: %s\n", path); return; }
    std::fprintf(playback_log, "AUDIO_MOD_LOG schema=2 rate=48000 channels=2\nPLAY means nonzero decoded MP3 samples mixed into a buffer accepted by SDL_QueueAudio. PACK_ENABLED alone is not playback proof.\n");
    std::fflush(playback_log);
}

void note_queue_result(int result, bool undecimated) {
    if (!playback_log) return;
    std::lock_guard lock(log_mutex);
    bool wrote = false;
    for (unsigned slot = 0; slot < slot_count; ++slot) {
        auto& voice = voices[slot];
        for (size_t index = 0; index < max_layers; ++index) {
            auto& pending = pending_proofs[slot][index];
            if (!pending.available) continue;
            auto& layer = voice.layers[index];
            if (result != 0) {
                std::fprintf(playback_log, "QUEUE_ERROR slot=%u sequence=%d result=%d\n", slot, voice.sequence, result);
                wrote = true;
            }
            // Only full buffers have verified per-source contribution counts.
            else if (undecimated && layer.stream.clip) {
                layer.positive_samples += pending.positive;
                const char* event = !layer.announced ? "PLAY" :
                    pending.channels != layer.logged_channels ? "CHANNELS" :
                    std::abs(pending.weight - layer.logged_weight) >= 0.05f ? "TRANSITION" :
                    layer.positive_samples - layer.reported_samples >= 48000 * 2 * 5 ? "PLAYING" : nullptr;
                if (event) {
                    std::fprintf(playback_log, "%s mod=\"%s\" file=\"%s\" sequence=%d slot=%u native_channels=0x%08X layer_channels=0x%04X timeline_frame=%llu source_frame=%llu mixed_frames=%zu nonzero_samples=%zu total_nonzero_samples=%llu gain=%.6f layer_gain=%.6f native_min=%u native_max=%u queue_result=0\n",
                        event, voice.mod_id.c_str(), layer.stream.clip->file.c_str(), voice.sequence, slot,
                        pending.channels, layer.native_mask,
                        static_cast<unsigned long long>(pending.timeline), static_cast<unsigned long long>(pending.source),
                        pending.frames, pending.positive, static_cast<unsigned long long>(layer.positive_samples),
                        voice.gain, pending.weight, pending.native_min, pending.native_max);
                    layer.announced = true; layer.logged_channels = pending.channels;
                    layer.logged_weight = pending.weight; layer.reported_samples = layer.positive_samples;
                    wrote = true;
                }
            }
            pending.available = false;
        }
    }
    if (wrote) std::fflush(playback_log);
}

void note_native_gain(unsigned slot, int sequence, int16_t posted, bool replaced) {
    if (!playback_log || slot >= slot_count) return;
    static std::array<bool, slot_count> suppressed{};
    static std::array<int, slot_count> ids{-1, -1, -1, -1, -1, -1};
    std::lock_guard lock(log_mutex);
    bool mute = replaced && posted == 0;
    if ((mute && (!suppressed[slot] || ids[slot] != sequence)) || (!mute && suppressed[slot])) {
        std::fprintf(playback_log, "%s sequence=%d slot=%u posted_guest_gain=%d\n", mute ? "MUTE_EVENT" : "RESTORE_EVENT", sequence, slot, posted);
        std::fflush(playback_log);
    }
    suppressed[slot] = mute; ids[slot] = sequence;
}

std::shared_ptr<const Pack> load_pack(const Reader& reader, std::string& error) {
    try {
        auto manifest = reader("audio.json", 1024 * 1024);
        auto j = nlohmann::json::parse(manifest.begin(), manifest.end());
        const auto& version = j.at("schema_version");
        if (!version.is_number_integer() || (version != 1 && version != 2) ||
            !j.at("sequences").is_array() || j.at("sequences").empty() || j.at("sequences").size() > 128)
            throw std::runtime_error("Unsupported or empty audio manifest");
        const int schema = version.get<int>();
        auto pack = std::make_shared<Pack>();
        size_t total = 0;
        for (const auto& item : j.at("sequences")) {
            const auto& id = item.at("id");
            if (!id.is_number_integer() || id < 0 || id > 32767) throw std::runtime_error("Invalid sequence id");
            int sequence = id.get<int>();
            std::string path = item.at("file").get<std::string>();
            if (!safe_path(path)) throw std::runtime_error("Invalid MP3 path: " + path);
            // Each file has one playback contract; variants can share the same contract.
            auto clip = std::make_shared<Clip>();
            clip->file = path;
            clip->bytes = reader(path, max_file);
            total += clip->bytes.size();
            if (clip->bytes.empty() || total > max_pack) throw std::runtime_error("Audio pack exceeds compressed-memory limit");
            auto decoder = std::make_unique<drmp3>();
            if (!drmp3_init_memory(decoder.get(), clip->bytes.data(), clip->bytes.size(), nullptr)) throw std::runtime_error("Invalid MP3: " + path);
            struct Cleanup { drmp3* p; ~Cleanup() { drmp3_uninit(p); } } cleanup{decoder.get()};
            if (decoder->sampleRate != rate || decoder->channels != 2) throw std::runtime_error("MP3 must be 48000 Hz stereo: " + path);
            auto count = drmp3_get_pcm_frame_count(decoder.get());
            if (!count || count > rate * 1200ull) throw std::runtime_error("Invalid MP3 length: " + path);
            clip->start = frame_field(item, "start_frame", 0);
            clip->end = frame_field(item, "end_frame", count);
            clip->loop_start = frame_field(item, "loop_start_frame", 0);
            clip->loop_end = frame_field(item, "loop_end_frame", 0);
            clip->gain = item.value("gain", 1.0f);
            if (!std::isfinite(clip->gain) || clip->gain < 0 || clip->gain > 2 || clip->start >= clip->end || clip->end > count ||
                (clip->loop_end && (clip->loop_start < clip->start || clip->loop_start >= clip->loop_end || clip->loop_end > clip->end)) ||
                (!clip->loop_end && clip->loop_start)) throw std::runtime_error("Invalid audio bounds or gain: " + path);
            drmp3_uint32 points = 1024;
            clip->seek.resize(points);
            if (!drmp3_calculate_seek_points(decoder.get(), &points, clip->seek.data())) throw std::runtime_error("Unable to index MP3: " + path);
            clip->seek.resize(points);
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
                if (!mask || mask > 0xFFFF || rule.mask != UINT32_MAX) throw std::runtime_error("Native layers require a nonempty 16-channel group and exact activation selector");
                rule.native_mask = static_cast<uint32_t>(mask);
            }
            for (const auto& previous : pack->rules) if (previous.sequence == sequence && previous.mask == rule.mask && previous.value == rule.value) throw std::runtime_error("Duplicate sequence selector");
            pack->rules.push_back(std::move(rule));
        }
        for (const auto& rule : pack->rules) {
            if (!rule.native_mask) continue;
            unsigned count = 0;
            uint32_t occupied = 0;
            for (const auto& layer : pack->rules) {
                if (layer.sequence != rule.sequence) continue;
                const auto& a = *rule.clip; const auto& b = *layer.clip;
                if (!layer.native_mask || (occupied & layer.native_mask) || ++count > max_layers ||
                    a.start != b.start || a.end != b.end || a.loop_start != b.loop_start || a.loop_end != b.loop_end)
                    throw std::runtime_error("Native layers require disjoint groups, at most four layers and identical timeline bounds");
                occupied |= layer.native_mask;
            }
        }
        error.clear(); return pack;
    }
    catch (const std::exception& e) { error = e.what(); return {}; }
}

void enable(const std::string& id, size_t order, std::shared_ptr<const Pack> pack) {
    if (!pack) return;
    std::lock_guard lock(catalog_mutex);
    std::erase_if(entries, [&](const Entry& e) { return e.id == id; });
    entries.push_back({id, order, std::move(pack)}); publish();
    if (playback_log) {
        std::lock_guard log_lock(log_mutex);
        std::fprintf(playback_log, "PACK_ENABLED id=\"%s\" (not playback proof)\n", id.c_str());
        std::fflush(playback_log);
    }
}
void disable(const std::string& id) {
    std::lock_guard lock(catalog_mutex);
    std::erase_if(entries, [&](const Entry& e) { return e.id == id; }); publish();
    if (playback_log) {
        std::lock_guard log_lock(log_mutex);
        std::fprintf(playback_log, "PACK_DISABLED id=\"%s\"\n", id.c_str());
        std::fflush(playback_log);
    }
}
void reorder(const std::function<size_t(const std::string&)>& get_order) {
    std::lock_guard lock(catalog_mutex);
    for (auto& entry : entries) entry.order = get_order(entry.id);
    publish();
}
bool replaces(int sequence, uint32_t channels) { return select(*published.load(), sequence, channels).count != 0; }
void update(unsigned slot, int sequence, int16_t authored, uint32_t channels, bool restart, const NativeChannels& native) {
    if (slot >= slot_count) return;
    std::lock_guard lock(state_mutex);
    auto& state = states[slot];
    if (trace_enabled() && (state.sequence != sequence || state.channels != channels || restart))
        std::fprintf(stderr, "[audio-mod] slot=%u sequence=%d channels=%08X volume=%d restart=%d\n", slot, sequence, channels, authored, restart);
    if (restart || state.sequence != sequence) ++state.serial;
    state.sequence = sequence; state.authored = authored; state.channels = channels; state.native = native;
}
void reset() {
    std::lock_guard lock(state_mutex);
    for (auto& state : states) { state.sequence = -1; ++state.serial; }
}
void mix(float* stereo, size_t frames, float main_volume, int bgm_percent) {
    auto catalog = published.load();
    std::array<Slot, slot_count> snapshot;
    { std::lock_guard lock(state_mutex); snapshot = states; }
    std::array<float, 1024 * 2> pcm{};
    bool mixed = false;
    const bool record_proof = playback_log != nullptr;
    if (record_proof) pending_proofs = {};
    for (unsigned slot = 0; slot < slot_count; ++slot) {
        auto& voice = voices[slot]; const auto& state = snapshot[slot];
        auto selected = select(*catalog, state.sequence, state.channels);
        const bool restart = voice.sequence != state.sequence || voice.serial != state.serial;
        if (restart) {
            voice.elapsed = 0; voice.gain = 0;
            for (auto& layer : voice.layers) { layer.stream.clear(); layer.weight = 0; }
        }
        voice.sequence = state.sequence; voice.serial = state.serial;
        if (record_proof) {
            if (selected.id) voice.mod_id = *selected.id;
            else voice.mod_id.clear();
        }
        const float wanted = std::clamp(state.authored / 32767.0f, 0.0f, 1.0f) *
            std::clamp(bgm_percent, 0, 100) / 100.0f * std::clamp(main_volume, 0.0f, 1.0f) * 0.5f;
        // The native sequence clock runs even without an enabled mapping. This
        // includes zero-weight layers, catalog reorder, fallback and re-enable.
        for (size_t index = 0; index < max_layers; ++index) {
            auto& layer = voice.layers[index];
            const Rule* rule = index < selected.count ? selected.layers[index] : nullptr;
            auto clip = rule ? rule->clip : nullptr;
            float weight = 1;
            unsigned low = 255, high = 0, count = 0;
            if (rule && rule->native_mask) {
                weight = 0;
                if (state.native.valid) for (unsigned channel = 0; channel < 16; ++channel) {
                    if (!(rule->native_mask & (1u << channel))) continue;
                    const unsigned q = state.native.gain[channel];
                    // 80029DCC: 255 bypasses multiplication, otherwise q/256.
                    weight += q == 255 ? 1.0f : q / 256.0f;
                    if (record_proof) { low = std::min(low, q); high = std::max(high, q); }
                    ++count;
                }
                if (count) weight /= count;
            }
            if (!count) low = high = 0;
            if (clip != layer.stream.clip || restart) {
                layer.stream.open(clip, voice.elapsed);
                layer.native_mask = rule ? rule->native_mask : 0;
                layer.weight = weight;
                if (record_proof) {
                    layer.announced = false; layer.logged_weight = -1;
                    layer.positive_samples = layer.reported_samples = 0;
                }
            }
            if (!clip) continue;
            if (record_proof) {
                const uint64_t source = layer.stream.position;
                const size_t positive = mix_layer<true>(layer, stereo, frames, wanted, voice.gain, weight, pcm);
                if (positive) pending_proofs[slot][index] = {true, state.channels, frames, positive,
                    voice.elapsed, source, layer.weight, low, high};
            }
            else mix_layer<false>(layer, stereo, frames, wanted, voice.gain, weight, pcm);
            mixed = true;
        }
        // Update the master ramp exactly once, independently of layer count.
        voice.gain = wanted + (voice.gain - wanted) * std::pow(1.0f - 1.0f / 240.0f, static_cast<float>(frames));
        if (state.sequence >= 0) voice.elapsed += frames;
    }
    if (mixed) for (size_t i = 0; i < frames * 2; ++i) stereo[i] = std::clamp(stereo[i], -1.0f, 1.0f);
}
}
