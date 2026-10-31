#pragma once

#include <cstddef>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tooie_audio_mod {
constexpr unsigned slot_count = 6;
struct Pack;
// The reader must reject oversized files before allocating/decompressing them.
using Reader = std::function<std::vector<char>(const std::string&, size_t)>;
std::shared_ptr<const Pack> load_pack(const Reader& reader, std::string& error);
void enable(const std::string& id, size_t order, std::shared_ptr<const Pack> pack);
void disable(const std::string& id);
void reorder(const std::function<size_t(const std::string&)>& get_order);
bool replaces(int sequence, uint32_t channels);
// Guest producer. These functions never decode or access files.
struct NativeChannels {
    std::array<uint8_t, 16> gain{};
    bool valid = false;
};
void update(unsigned slot, int sequence, int16_t authored, uint32_t channels, bool restart = false,
    const NativeChannels& native = {});
void reset();
// Audio consumer. Adds stereo float PCM at 48 kHz to the existing output buffer.
void mix(float* stereo, size_t frames, float main_volume, int bgm_percent);
// Optional playback proof, emitted only after SDL accepts a mixed buffer.
void initialize_diagnostics();
void note_queue_result(int result, bool undecimated = true);
void note_native_gain(unsigned slot, int sequence, int16_t posted, bool replaced);
}
