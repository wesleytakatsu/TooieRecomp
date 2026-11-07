#pragma once

#include <cstddef>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace tooie_audio_mod {
constexpr unsigned slot_count = 6;
// Rare's sequence player drives up to 32 channels; channel masks and native gains cover all of them.
constexpr unsigned channel_count = 32;
struct Pack;
// The reader must reject oversized files before allocating/decompressing them. A pack that is too large to keep in
// memory keeps its reader and calls it again later from a background loader thread, so it must stay valid and
// thread-safe for the pack's lifetime.
using Reader = std::function<std::vector<char>(const std::string&, size_t)>;
// Size of a file in the pack without reading it, or nullopt when it is missing. With a sizer, packs above the
// in-memory limit load their MP3s on demand; without one, every MP3 is read and checked by load_pack.
using Sizer = std::function<std::optional<size_t>(const std::string&)>;
std::shared_ptr<const Pack> load_pack(const Reader& reader, std::string& error, const Sizer& sizer = {});
void enable(const std::string& id, size_t order, std::shared_ptr<const Pack> pack);
void disable(const std::string& id);
void reorder(const std::function<size_t(const std::string&)>& get_order);
// True when an enabled pack plays this sequence and channel arrangement now. A pack whose MP3s are still loading
// does not replace anything yet, so the original music keeps playing until the MP3s are ready.
bool replaces(int sequence, uint32_t channels);
// Per slot: also false once the slot's MP3s ended (no loop) while its sequence keeps playing, so the original music
// carries on instead of silence.
bool replaces(unsigned slot, int sequence, uint32_t channels);
// Guest producer. These functions never decode or access files.
struct NativeChannels {
    std::array<uint8_t, channel_count> gain{};
    bool valid = false;
    double bpm = 0;         // The sequence's current tempo (0 when unknown).
    double target_bpm = 0;  // The tempo the game asked for (an area's tempo), or the current one; 0 when unknown.
};
void update(unsigned slot, int sequence, int16_t authored, uint32_t channels, bool restart = false,
    const NativeChannels& native = {});
void reset();
// Audio consumer. Adds stereo float PCM at 48 kHz to the existing output buffer.
void mix(float* stereo, size_t frames, float main_volume, int bgm_percent);
// Tests: memory limit for on-demand MP3s (bytes) and the in-memory limit for whole packs, and a wait for the loader.
void set_memory_limits(size_t resident_budget, size_t eager_limit);
bool wait_loader_idle(unsigned milliseconds);
// Layer weights for a native gain state: the non-negative combination of layer channel groups closest to the gains.
std::vector<float> layer_weights(const std::vector<uint32_t>& layer_masks, const NativeChannels& native);
}
