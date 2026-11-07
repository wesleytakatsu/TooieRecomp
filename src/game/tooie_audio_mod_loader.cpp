#include "tooie_audio_mod_loader.h"
#include "tooie_audio_mod.h"
#include "tooie_audio_render.h"
#include "tooie_bgm_hooks.h"
#include "tooie_bgm_volume.h"
#include "recomp.h"
#include "librecomp/mods.hpp"
#include "recompui/recompui.h"
#include "miniz.h"
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
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
    // Rare CSP: seqp+0x60 -> channel records, stride 0x38, current gain +9, for all 32 channels (area masks use
    // bits up to 31; runtime STATUS logs show the gains of channels 16-31 following them).
    // 8002418C writes it immediately; 80023EB8 advances the native envelope.
    uint32_t address = MEM_W(0x60, guest(tooie_bgm::seqp_addr(slot)));
    if (address < 0x80000000u || address > 0x807FFFFFu - channel_count * 0x38) return result;
    for (unsigned channel = 0; channel < result.gain.size(); ++channel)
        result.gain[channel] = MEM_BU(channel * 0x38 + 9, guest(address));
    result.valid = true;
    // Current tempo as 800260B0 reads it: seqp+0x24 over the float at (seqp+0x18)+8 is microseconds per beat.
    const uint32_t seqp = tooie_bgm::seqp_addr(slot);
    const uint32_t seq = MEM_W(0x18, guest(seqp));
    if (seq >= 0x80000000u && seq < 0x80800000u) {
        const uint32_t bits = MEM_W(0x8, guest(seq));
        float divisor;
        std::memcpy(&divisor, &bits, sizeof(divisor));
        const double uspb = divisor > 0 ? static_cast<double>(MEM_W(0x24, guest(seqp))) / divisor : 0;
        if (uspb > 0) result.bpm = 60000000.0 / uspb;
    }
    // 80017D74 keeps the tempo the game asked for (microseconds per beat) at 0x8007AEA0 + slot * 4; -1 means none.
    // It is only the destination of a tempo ramp: the word is never cleared, so once the tempo stops moving (or when
    // a sequence starts at its own tempo) the current tempo is the area's.
    // A ramp moves in steps, so it counts as moving for 20 reads (frames) after the tempo last changed.
    // A new sequence starts settled: its own start tempo is not a ramp toward an old request.
    static std::array<double, slot_count> previous{};
    static std::array<int, slot_count> still{};
    static std::array<int, slot_count> last_sequence{-1, -1, -1, -1, -1, -1};
    const int sequence = sequence_id(rdram, slot);
    if (sequence != last_sequence[slot]) { last_sequence[slot] = sequence; still[slot] = 1000; }
    else if (std::abs(result.bpm - previous[slot]) > 0.01) still[slot] = 0;
    else if (still[slot] < 1000) ++still[slot];
    previous[slot] = result.bpm;
    const bool moving = still[slot] < 20;
    const uint32_t requested = MEM_W(slot * 4, guest(0x8007AEA0u));
    result.target_bpm = moving && requested != 0xFFFFFFFFu && requested > 0 && requested < 60000000u ? 60000000.0 / requested : result.bpm;
    return result;
}
// A replaced sequence is silent, and so is every other slot while the maintainer renderer owns the output.
// The renderer records native music, so no pack replaces anything while it runs.
bool silent(unsigned slot, int sequence, uint32_t mask) {
    if (tooie_audio_render::enabled() && !tooie_audio_render::keeps_mods()) return tooie_audio_render::mutes(slot);
    return tooie_audio_render::mutes(slot) || replaces(slot, sequence, mask);
}
void on_posted_gain(uint8_t* rdram, unsigned slot, int16_t posted) {
    int sequence = sequence_id(rdram, slot);
    note_native_gain(slot, sequence, posted, replaces(slot, sequence, channels(rdram, slot)));
}
int16_t filter(uint8_t* rdram, unsigned slot, int16_t authored, int16_t effective) {
    int sequence = sequence_id(rdram, slot);
    uint32_t mask = channels(rdram, slot);
    // Volume events do not own sequence identity. The player retains its last
    // ID after a short sequence ends; publishing it here would resurrect it.
    muted[slot] = silent(slot, sequence, mask);
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
        bool replacement = silent(slot, sequence, mask);
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
    tooie_audio_render::on_frame(rdram, ctx);
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

// The pack's own view of its files, opened separately from the mod manager's handle so the background loader can
// read MP3s later from another thread. Lives as long as the pack (the reader and sizer hold it).
struct PackFiles {
    std::mutex mutex;
    std::filesystem::path root;  // Zip file, or folder of a loose mod.
    std::string content_root;    // Folder inside the zip that holds audio.json.
    bool zip = false;
    mz_zip_archive archive{};
    bool open = false;
    FILE* file = nullptr;  // miniz does not close a FILE it was given.
    ~PackFiles() { if (open) mz_zip_reader_end(&archive); if (file) std::fclose(file); }
    std::string full(const std::string& path) const { return content_root.empty() ? path : content_root + "/" + path; }
    bool ensure() {
        if (open) return true;
#ifdef _WIN32
        // miniz takes a narrow path; read the archive through a FILE opened with the wide path.
        if (!file) file = _wfopen(root.c_str(), L"rb");
        if (!file) return false;
        open = mz_zip_reader_init_cfile(&archive, file, 0, 0);
#else
        open = mz_zip_reader_init_file(&archive, root.string().c_str(), 0);
#endif
        return open;
    }
    std::optional<size_t> size(const std::string& path) {
        std::lock_guard lock(mutex);
        if (!zip) {
            std::error_code ec;
            auto s = std::filesystem::file_size(root / std::filesystem::u8path(path), ec);
            return ec ? std::nullopt : std::optional<size_t>(static_cast<size_t>(s));
        }
        if (!ensure()) return std::nullopt;
        int index = mz_zip_reader_locate_file(&archive, full(path).c_str(), nullptr, MZ_ZIP_FLAG_CASE_SENSITIVE);
        mz_zip_archive_file_stat stat{};
        if (index < 0 || !mz_zip_reader_file_stat(&archive, index, &stat)) return std::nullopt;
        return static_cast<size_t>(stat.m_uncomp_size);
    }
    std::vector<char> read(const std::string& path, size_t limit) {
        auto s = size(path);
        if (!s || *s > limit) throw std::runtime_error("Missing or oversized audio file: " + path);
        std::lock_guard lock(mutex);
        std::vector<char> bytes(*s);
        if (!zip) {
            std::ifstream in(root / std::filesystem::u8path(path), std::ios::binary);
            if (!in.read(bytes.data(), static_cast<std::streamsize>(bytes.size()))) throw std::runtime_error("Unable to read audio file: " + path);
            return bytes;
        }
        int index = mz_zip_reader_locate_file(&archive, full(path).c_str(), nullptr, MZ_ZIP_FLAG_CASE_SENSITIVE);
        if (index < 0 || !mz_zip_reader_extract_to_mem(&archive, index, bytes.data(), bytes.size(), 0))
            throw std::runtime_error("Unable to read audio file: " + path);
        return bytes;
    }
};

void on_enabled(recomp::mods::ModContext& context, const recomp::mods::ModHandle& mod) {
    note("ENABLING id=\"" + mod.manifest.mod_id + "\"");
    std::string error;
    auto files = std::make_shared<PackFiles>();
    files->root = mod.manifest.mod_root_path;
    auto* handle = mod.manifest.file_handle.get();
    if (auto* zip = dynamic_cast<recomp::mods::ZipModFileHandle*>(handle)) { files->zip = true; files->content_root = zip->content_root; }
    else if (auto* loose = dynamic_cast<recomp::mods::LooseModFileHandle*>(handle)) files->root = loose->root_path;
    Reader reader = [files](const std::string& path, size_t limit) { return files->read(path, limit); };
    Sizer sizer = [files](const std::string& path) { return files->size(path); };
    auto pack = load_pack(reader, error, sizer);
    if (!pack) {
        // The original audio remains active if validation fails.
        std::string message = "Unable to enable audio mod " + mod.manifest.display_name + ": " + error;
        note("ENABLE_FAILED " + message);
        recompui::message_box(message.c_str());
        return;
    }
    enable(mod.manifest.mod_id, context.get_mod_order_index(mod.manifest.mod_id), std::move(pack));
    std::fprintf(stderr, "[audio-mod] enabled %s\n", mod.manifest.mod_id.c_str());
}
void on_disabled(recomp::mods::ModContext&, const recomp::mods::ModHandle& mod) { disable(mod.manifest.mod_id); }
void on_reordered(recomp::mods::ModContext& context) { reorder([&](const std::string& id) { return context.get_mod_order_index(id); }); }
}
