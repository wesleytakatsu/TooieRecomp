#include "tooie_audio_mod_loader.h"
#include "tooie_audio_mod.h"
#include "tooie_bgm_hooks.h"
#include "tooie_bgm_volume.h"
#include "recomp.h"
#include "librecomp/mods.hpp"
#include "recompui/recompui.h"
#include "miniz.h"
#include <array>
#include <cstdio>
#include <stdexcept>

extern "C" void func_80025ED0(uint8_t*, recomp_context*);

namespace tooie_audio_mod {
namespace {
// Verified in 80016ED0 and 800178C4. Player IDs are distinct from the
// high-level track descriptors and include short collection sequences.
constexpr uint32_t sequence_offset = 0x26C;
constexpr uint32_t channel_base = 0x8007AED0;
gpr guest(uint32_t p) { return static_cast<int32_t>(p); }
std::array<bool, slot_count> muted{};
std::array<bool, slot_count> starting{};
int sequence_id(uint8_t* rdram, unsigned slot) { return MEM_H(sequence_offset, guest(tooie_bgm::player_addr(slot))); }
uint32_t channels(uint8_t* rdram, unsigned slot) { return MEM_W(slot * 4, guest(channel_base)); }
NativeChannels native_channels(uint8_t* rdram, unsigned slot) {
    NativeChannels result;
    // Rare CSP: seqp+0x60 -> channel records, stride 0x38, current gain +9.
    // Schema 2 layers currently address only the first 16 channels.
    // 8002418C writes it immediately; 80023EB8 advances the native envelope.
    uint32_t address = MEM_W(0x60, guest(tooie_bgm::seqp_addr(slot)));
    if (address < 0x80000000u || address > 0x807FFFFFu - 16 * 0x38) return result;
    for (unsigned channel = 0; channel < result.gain.size(); ++channel)
        result.gain[channel] = MEM_BU(channel * 0x38 + 9, guest(address));
    result.valid = true;
    return result;
}
void on_posted_gain(uint8_t* rdram, unsigned slot, int16_t posted) {
    int sequence = sequence_id(rdram, slot);
    note_native_gain(slot, sequence, posted, replaces(sequence, channels(rdram, slot)));
}
int16_t filter(uint8_t* rdram, unsigned slot, int16_t authored, int16_t effective) {
    int sequence = sequence_id(rdram, slot);
    uint32_t mask = channels(rdram, slot);
    // Volume events do not own sequence identity. The player retains its last
    // ID after a short sequence ends; publishing it here would resurrect it.
    muted[slot] = replaces(sequence, mask);
    return muted[slot] ? 0 : effective;
}
void on_frame(uint8_t* rdram, recomp_context* ctx) {
    for (unsigned slot = 0; slot < slot_count; ++slot) {
        int sequence = sequence_id(rdram, slot);
        // CSP state at +0x2C (player + 0x20C) is zero after stop/end.
        if (MEM_W(0x20C, guest(tooie_bgm::player_addr(slot))) != 0) starting[slot] = false;
        else if (!starting[slot]) sequence = -1;
        uint32_t mask = channels(rdram, slot);
        int16_t authored = MEM_H(0, guest(tooie_bgm::player_addr(slot)));
        update(slot, sequence, authored, mask, false, native_channels(rdram, slot));
        bool replacement = replaces(sequence, mask);
        if (muted[slot] != replacement) {
            muted[slot] = replacement;
            recomp_context local = *ctx;
            local.r4 = guest(tooie_bgm::seqp_addr(slot));
            local.r5 = replacement ? 0 : tooie_bgm::apply_unscaled(authored, tooie_bgm::get_percent());
            const int16_t posted = static_cast<int16_t>(local.r5);
            func_80025ED0(rdram, &local);
            on_posted_gain(rdram, slot, posted);
        }
    }
}
void on_sequence(uint8_t* rdram, unsigned slot, int sequence) {
    // Original routine can recursively stop the previous sequence; notify only
    // after each original call so the outer start is the final event.
    starting[slot] = sequence >= 0;
    update(slot, sequence >= 0 ? sequence_id(rdram, slot) : -1,
        MEM_H(0, guest(tooie_bgm::player_addr(slot))), channels(rdram, slot), sequence >= 0, native_channels(rdram, slot));
}
}
void install_hooks() { initialize_diagnostics(); tooie_bgm::set_content_hooks(filter, on_frame, on_sequence, on_posted_gain); }

void on_enabled(recomp::mods::ModContext& context, const recomp::mods::ModHandle& mod) {
    std::string error;
    auto reader = [&](const std::string& path, size_t limit) {
        // Inspect archive size before read_file allocates the uncompressed data.
        auto* handle = mod.manifest.file_handle.get();
        if (auto* zip = dynamic_cast<recomp::mods::ZipModFileHandle*>(handle)) {
            std::string full = zip->content_root.empty() ? path : zip->content_root + "/" + path;
            int index = mz_zip_reader_locate_file(zip->archive.get(), full.c_str(), nullptr, MZ_ZIP_FLAG_CASE_SENSITIVE);
            mz_zip_archive_file_stat stat{};
            if (index < 0 || !mz_zip_reader_file_stat(zip->archive.get(), index, &stat) || stat.m_uncomp_size > limit) throw std::runtime_error("Missing or oversized audio file: " + path);
        }
        else if (auto* loose = dynamic_cast<recomp::mods::LooseModFileHandle*>(handle)) {
            std::error_code ec;
            auto size = std::filesystem::file_size(loose->root_path / path, ec);
            if (ec || size > limit) throw std::runtime_error("Missing or oversized audio file: " + path);
        }
        bool exists = false;
        auto bytes = handle->read_file(path, exists);
        if (!exists || bytes.size() > limit) throw std::runtime_error("Missing or oversized audio file: " + path);
        return bytes;
    };
    auto pack = load_pack(reader, error);
    if (!pack) {
        // The original audio remains active if validation fails.
        std::string message = "Unable to enable audio mod " + mod.manifest.display_name + ": " + error;
        std::fprintf(stderr, "[audio-mod] %s\n", message.c_str());
        recompui::message_box(message.c_str());
        return;
    }
    enable(mod.manifest.mod_id, context.get_mod_order_index(mod.manifest.mod_id), std::move(pack));
    std::fprintf(stderr, "[audio-mod] enabled %s\n", mod.manifest.mod_id.c_str());
}
void on_disabled(recomp::mods::ModContext&, const recomp::mods::ModHandle& mod) { disable(mod.manifest.mod_id); }
void on_reordered(recomp::mods::ModContext& context) { reorder([&](const std::string& id) { return context.get_mod_order_index(id); }); }
}
