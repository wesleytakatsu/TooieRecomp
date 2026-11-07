#include "tooie_audio_mod.h"
#include <json/json.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace {
int failures = 0;
void expect(bool value, const char* name) {
    if (!value) { std::fprintf(stderr, "FAIL %s\n", name); ++failures; }
}
std::vector<char> input;
auto load(nlohmann::json manifest, std::string& error) {
    auto reader = [&](const std::string& name, size_t limit) {
        if (name == "audio.json") { auto t = manifest.dump(); return std::vector<char>(t.begin(), t.end()); }
        if (input.size() > limit) throw std::runtime_error("oversized fixture");
        return input;
    };
    return tooie_audio_mod::load_pack(reader, error);
}
float energy(const std::vector<float>& data) {
    double value = 0;
    for (float v : data) { expect(std::isfinite(v) && std::abs(v) <= 1.0f, "finite bounded sample"); value += v*v; }
    return static_cast<float>(value);
}
}
int main(int argc, char** argv) {
    using namespace tooie_audio_mod;
    std::string error;
    initialize_diagnostics();
    const bool check_log = argc >= 3 && std::string(argv[2]) != "-";
    std::ofstream pcm_capture;
    if (argc >= 5) {
        pcm_capture.open(std::filesystem::u8path(argv[4]), std::ios::binary);
        expect(bool(pcm_capture), "PCM capture opened");
    }
    auto capture_pcm = [&](const std::vector<float>& samples) {
        if (pcm_capture.is_open()) {
            pcm_capture.write(reinterpret_cast<const char*>(samples.data()),
                static_cast<std::streamsize>(samples.size() * sizeof(float)));
            expect(bool(pcm_capture), "PCM capture written");
        }
    };
    auto log_text = [&]() {
        if (!check_log) return std::string{};
        std::ifstream log(argv[2]);
        return std::string(std::istreambuf_iterator<char>(log), {});
    };
    nlohmann::json j = {{"schema_version", 1}, {"sequences", nlohmann::json::array({{{"id", 4}, {"file", "audio/test.mp3"}}})}};
    expect(!load(j, error) && !error.empty(), "empty MP3 rejected");
    auto bad = j; bad["schema_version"] = 3;
    expect(!load(bad, error), "unknown version rejected");
    bad = j; bad["schema_version"] = 2.5;
    expect(!load(bad, error), "fractional version rejected");
    bad = j; bad["schema_version"] = 0x100000002ull;
    expect(!load(bad, error) && error == "Unsupported or empty audio manifest", "oversized version cannot wrap into schema 2");
    for (auto path : {"../test.mp3", "/test.mp3", "C:/test.mp3", "audio\\test.mp3", "audio/./test.mp3", "audio//test.mp3", "test.wav"}) {
        bad = j; bad["sequences"][0]["file"] = path;
        expect(!load(bad, error), "unsafe path rejected");
    }
    {
        // Layer weights: the closest non-negative mix of layer channel groups to the native gains.
        auto gains = [](uint32_t full, uint32_t half = 0) {
            NativeChannels n; n.valid = true;
            for (unsigned c = 0; c < channel_count; ++c) n.gain[c] = (full >> c) & 1 ? 255 : (half >> c) & 1 ? 128 : 0;
            return n;
        };
        auto near = [](const std::vector<float>& w, std::vector<float> want) {
            if (w.size() != want.size()) return false;
            for (size_t i = 0; i < w.size(); ++i) if (std::abs(w[i] - want[i]) > 0.01f) return false;
            return true;
        };
        // Mayahem Temple style: the kickball lobby and the approach share channel 30 (0x40000000).
        const std::vector<uint32_t> mt = {0x73FF, 0xC00, 0x41C00000, 0x401F0000, 0xBE200000};
        expect(near(layer_weights(mt, gains(0x41C00000)), {0, 0, 1, 0, 0}), "overlapping area resolves to its own layer");
        expect(near(layer_weights(mt, gains(0x401F0000)), {0, 0, 0, 1, 0}), "the other overlapping area resolves to its layer");
        expect(near(layer_weights(mt, gains(0x73FF)), {1, 0, 0, 0, 0}), "disjoint area resolves to its layer");
        expect(near(layer_weights(mt, gains(0x73FF, 0xC00)), {1, 0.5f, 0, 0, 0}), "disjoint crossfade keeps each group's mean");
        auto mid = layer_weights(mt, gains(0x40000000, 0x01C00000 | 0x001F0000));
        expect(mid.size() == 5 && mid[2] > 0.2f && mid[3] > 0.2f && mid[0] == 0 && mid[1] == 0, "crossfade between overlapping areas blends both");
        expect(near(layer_weights({0x63FF, 0x1C00}, gains(0)), {0, 0}), "silent channels silence every layer");
        expect(near(layer_weights({0x63FF, 0x1C00}, gains(0x63FF, 0x1C00)), {1, 0.5f}), "Spiral Mountain weights unchanged");
    }
    reset(); update(0, 4, 32767, ~0u, true);
    std::vector<float> pcm(48000 * 2, 0.125f), original = pcm;
    mix(pcm.data(), pcm.size()/2, 1, 100);
    expect(pcm == original, "no mod leaves original samples bit-identical");
    if (argc >= 2) {
        std::ifstream file(argv[1], std::ios::binary);
        input.assign(std::istreambuf_iterator<char>(file), {});
        expect(!input.empty(), "fixture read");
        bad = j; bad["sequences"][0]["id"] = 0x100000004ull;
        expect(!load(bad, error) && error == "Invalid sequence id", "oversized id cannot replace sequence 4");
        bad = j; bad["sequences"][0]["channel_value"] = 1u;
        expect(!load(bad, error), "orphan selector value cannot become an unconditional replacement");
        auto pack = load(j, error);
        if (!pack) { std::fprintf(stderr, "decode: %s\n", error.c_str()); return 1; }
        enable("fixture", 1, pack);
        expect(replaces(4, ~0u) && !replaces(103, ~0u), "only mapped sequence replaced");
        std::fill(pcm.begin(), pcm.end(), 0); update(0, 4, 32767, ~0u, true);
        mix(pcm.data(), pcm.size()/2, 1, 100); float loud = energy(pcm);
        expect(loud > 0, "MP3 decoded and mixed");
        capture_pcm(pcm);
        if (check_log) {
            expect(log_text().find("PACK_ENABLED") != std::string::npos && log_text().find("PLAY mod=") == std::string::npos, "enabled alone is not playback proof");
            note_queue_result(-1);
            expect(log_text().find("QUEUE_ERROR") != std::string::npos && log_text().find("PLAY mod=") == std::string::npos, "failed queue cannot report PLAY");
            update(0, 4, 32767, ~0u, true);
            std::fill(pcm.begin(), pcm.end(), 0);
            mix(pcm.data(), pcm.size()/2, 1, 100); note_queue_result(0, false);
            expect(log_text().find("PLAY mod=") == std::string::npos, "decimated block cannot claim unverified contribution counts");
            update(0, 4, 32767, ~0u, true);
            std::fill(pcm.begin(), pcm.end(), 0);
            mix(pcm.data(), pcm.size()/2, 1, 100);
            note_queue_result(0); // Simulated acceptance; no audio device in this host test.
            expect(log_text().find("PLAY mod=\"fixture\" file=\"audio/test.mp3\"") != std::string::npos && log_text().find("queue_result=0") != std::string::npos, "PLAY identifies decoded asset after queue success");
            update(0, 4, 32767, 0xAABBCCDD);
            std::fill(pcm.begin(), pcm.end(), 0);
            mix(pcm.data(), pcm.size()/2, 1, 100); note_queue_result(0);
            expect(log_text().find("CHANNELS") != std::string::npos && log_text().find("0xAABBCCDD") != std::string::npos, "native channel changes logged for manual mapping");
        }
        std::fill(pcm.begin(), pcm.end(), 0); update(0, 4, 32767, ~0u, true);
        mix(pcm.data(), pcm.size()/2, 0.5f, 50);
        expect(std::abs(energy(pcm) / loud - 0.0625f) < 0.001f, "main and BGM gain each applied once");
        // Same ID retrigger must rewind even when no intervening frame sees idle.
        std::fill(pcm.begin(), pcm.end(), 0); update(0, 4, 32767, ~0u, true);
        mix(pcm.data(), pcm.size()/2, 1, 100);
        expect(std::abs(energy(pcm) - loud) < 0.001f, "same sequence retrigger rewinds");
        disable("fixture");
        std::fill(pcm.begin(), pcm.end(), 0); mix(pcm.data(), pcm.size()/2, 1, 100);
        enable("fixture", 1, pack);
        std::fill(pcm.begin(), pcm.end(), 0); mix(pcm.data(), pcm.size()/2, 1, 100);
        const auto resumed = pcm;
        // A fresh decode advanced to the same native time must agree, including
        // gain settling. Disable/re-enable must not reset the musical phase.
        update(0, 4, 32767, ~0u, true);
        std::vector<float> reference(48000 * 2 * 3, 0);
        mix(reference.data(), reference.size()/2, 1, 100);
        bool phase_agrees = true;
        for (size_t i = 0; i < resumed.size(); ++i)
            phase_agrees &= std::abs(resumed[i] - reference[48000 * 2 * 2 + i]) < 0.00002f;
        expect(phase_agrees, "live re-enable preserves native timeline and PCM phase");
        {
            // An MP3 without a loop that ends while its sequence still plays hands the slot back to the original.
            enable("fixture", 1, pack);
            update(2, 4, 32767, ~0u, true);
            expect(replaces(2u, 4, ~0u), "slot replaced while its MP3 plays");
            std::vector<float> block(48000 * 2);
            for (int s = 0; s < 30; ++s) { std::fill(block.begin(), block.end(), 0.0f); mix(block.data(), 48000, 1, 100); }
            expect(!replaces(2u, 4, ~0u) && replaces(4, ~0u), "ended MP3 lets the original sequence continue in that slot");
            update(2, 4, 32767, ~0u, true);
            expect(replaces(2u, 4, ~0u), "a new start of the sequence is replaced again");
            update(2, -1, 0, ~0u, true);
            std::fill(block.begin(), block.end(), 0.0f); mix(block.data(), 16, 1, 100);
            disable("fixture");
        }
        bad = j; bad["sequences"][0]["loop_start_frame"] = 200; bad["sequences"][0]["loop_end_frame"] = 100;
        expect(!load(bad, error), "reversed loop rejected");
        bad = j; bad["sequences"].push_back(bad["sequences"][0]);
        expect(!load(bad, error), "duplicate selector rejected");
        auto loop = j; loop["sequences"][0]["loop_start_frame"] = 4800; loop["sequences"][0]["loop_end_frame"] = 24000;
        auto loop_pack = load(loop, error); expect(bool(loop_pack), "valid loop indexed");
        enable("fixture", 1, loop_pack); update(0, 4, 32767, ~0u, true);
        std::fill(pcm.begin(), pcm.end(), 0); mix(pcm.data(), pcm.size()/2, 1, 100);
        expect(energy(pcm) > 0, "multiple loop wraps within output block");
        auto variant = j; variant["sequences"][0]["channel_mask"] = 1; variant["sequences"][0]["channel_value"] = 1;
        auto variant_pack = load(variant, error); enable("variant", 0, variant_pack);
        disable("fixture");
        expect(replaces(4, 1) && !replaces(4, 0), "channel selector matched");
        disable("variant"); pcm = original; mix(pcm.data(), pcm.size()/2, 1, 100);
        expect(!replaces(4, 1) && pcm == original, "disable removes replacement and preserves original PCM");
        auto family = loop;
        family["schema_version"] = 2;
        auto& dry = family["sequences"][0];
        dry["id"] = 50; dry["channel_mask"] = 0xFFFFFFFFu; dry["channel_value"] = 0x63FFu;
        dry["native_channel_mask"] = 0x63FFu; dry["gain"] = 0.75f;
        auto wet = dry; wet["file"] = "audio/wet.mp3";
        wet["channel_value"] = 0x1C00u; wet["native_channel_mask"] = 0x1C00u; wet["gain"] = 0.25f;
        family["sequences"].push_back(wet);
        auto layered = load(family, error);
        expect(bool(layered), "synchronized native layer family accepted");
        auto overlap = family; overlap["sequences"][1]["native_channel_mask"] = 0x1C01u;
        expect(bool(load(overlap, error)), "overlapping native channel groups accepted (areas that add channels)");
        bad = family; bad["sequences"][1]["native_channel_mask"] = 0x1FE0000u; bad["sequences"][1]["channel_value"] = 0x1FE0000u;
        expect(bool(load(bad, error)), "native channel groups above channel 15 accepted");
        bad = family; bad["sequences"][1]["loop_end_frame"] = 23999u;
        expect(bool(load(bad, error)), "a layer may loop with a period of its own (per-channel loops)");
        bad = family; bad["sequences"][1]["start_frame"] = 10u;
        expect(!load(bad, error), "layers with different starts rejected");
        bad = family; bad["schema_version"] = 1;
        expect(!load(bad, error), "schema 1 cannot silently ignore native layers");
        auto render = [](size_t frames) {
            std::vector<float> result(frames * 2, 0);
            mix(result.data(), frames, 1, 100);
            return result;
        };
        NativeChannels native; native.valid = true;
        for (unsigned c = 0; c < 16; ++c) native.gain[c] = (0x63FFu & (1u << c)) ? 255 : 0;
        // Obtain decoded PCM ground truth at 1:20 from a continuously running
        // single-track reference. The short fixture loop exercises many wraps.
        auto reference_rule = loop; reference_rule["sequences"][0]["id"] = 50;
        auto reference_pack = load(reference_rule, error);
        enable("reference", 0, reference_pack);
        update(0, 50, 32767, 0x63FFu, true, native);
        render(80 * 48000);
        const auto phase_reference = render(4800);
        disable("reference");
        // Let the original sequence reach 1:20 with no mod, then enable layers.
        update(0, 50, 32767, 0x63FFu, true, native);
        render(80 * 48000);
        enable("layers", 0, layered);
        expect(replaces(50, 0x63FF) && replaces(50, 0x1C00) && !replaces(50, 0x7FFF), "only observed arrangements activate layered family");
        const auto first = render(480);
        capture_pcm(first);
        for (size_t i = 0; i < first.size(); ++i)
            if (std::abs(first[i] - phase_reference[i] * 0.75f) > 0.00002f) { expect(false, "enable at 1:20 seeks exact decoded loop phase"); break; }
        note_queue_result(0);
        float dry_weight = 1, wet_weight = 0;
        size_t reference_offset = first.size();
        // Partial native envelope, then rapid reversal, then complete immersion.
        for (auto gains : {std::pair{128u, 64u}, std::pair{255u, 0u}, std::pair{0u, 255u}}) {
            for (unsigned c = 0; c < 16; ++c)
                native.gain[c] = static_cast<uint8_t>((0x63FFu & (1u << c)) ? gains.first : gains.second);
            update(0, 50, 32767, gains.second ? 0x1C00u : 0x63FFu, false, native);
            auto actual = render(480);
            capture_pcm(actual);
            float dry_target = gains.first == 255 ? 1 : gains.first / 256.0f;
            float wet_target = gains.second == 255 ? 1 : gains.second / 256.0f;
            bool agrees = true;
            for (size_t frame = 0; frame < 480; ++frame) {
                dry_weight += (dry_target - dry_weight) / 240;
                wet_weight += (wet_target - wet_weight) / 240;
                for (unsigned c = 0; c < 2; ++c) {
                    auto i = frame * 2 + c;
                    float expected = phase_reference[reference_offset + i] * (0.75f * dry_weight + 0.25f * wet_weight);
                    agrees &= std::abs(actual[i] - expected) < 0.00002f;
                }
            }
            expect(agrees, "native envelope and reversal preserve both decoded phases and current gains");
            reference_offset += actual.size(); note_queue_result(0);
        }
        if (check_log) expect(log_text().find("timeline_frame=3840000 source_frame=19200") != std::string::npos &&
            log_text().find("layer_channels=0x1C00") != std::string::npos, "accepted-buffer logs expose shared clock and underwater contribution");
        disable("layers");
        {
            // Layers recorded at different area tempos follow the native beat position (Mayahem Temple: land 120 BPM,
            // underwater 105 BPM). The loop bounds of the slower layer are the same beats, scaled by 120/105.
            auto tempo = family;
            auto& land = tempo["sequences"][0];
            auto& water = tempo["sequences"][1];
            land["bpm"] = 120; water["bpm"] = 105;
            water["loop_start_frame"] = static_cast<uint64_t>(std::llround(4800 * 120.0 / 105.0));
            water["loop_end_frame"] = static_cast<uint64_t>(std::llround(24000 * 120.0 / 105.0));
            water["gain"] = 0.75f;
            land["end_frame"] = 30000u;
            water["end_frame"] = static_cast<uint64_t>(std::llround(30000 * 120.0 / 105.0));
            auto tempo_pack = load(tempo, error);
            expect(bool(tempo_pack), "tempo layers with beat-equal bounds accepted");
            if (!tempo_pack) std::fprintf(stderr, "tempo: %s\n", error.c_str());
            {
                // The same channels recorded at another area tempo are a separate rule, chosen by the requested tempo.
                auto two = tempo;
                auto slow = two["sequences"][1];
                slow["bpm"] = 90; slow["file"] = "audio/slow.mp3";
                for (const char* k : {"loop_start_frame", "loop_end_frame", "end_frame"})
                    slow[k] = static_cast<uint64_t>(std::llround(two["sequences"][0][k].get<uint64_t>() * 120.0 / 90.0));
                two["sequences"].push_back(slow);
                auto two_pack = load(two, error);
                expect(bool(two_pack), "same channels at two area tempos accepted");
                enable("two", 0, two_pack);
                NativeChannels at; at.valid = true; at.bpm = at.target_bpm = 105;
                update(3, 50, 32767, 0x1C00u, true, at);
                expect(replaces(3u, 50, 0x1C00u), "rule for the requested tempo replaces");
                at.bpm = at.target_bpm = 90; update(3, 50, 32767, 0x1C00u, false, at);
                expect(replaces(3u, 50, 0x1C00u), "the other recorded tempo replaces too");
                at.bpm = at.target_bpm = 142; update(3, 50, 32767, 0x1C00u, false, at);
                expect(!replaces(3u, 50, 0x1C00u), "an area tempo with no recording keeps the original");
                update(3, -1, 0, ~0u, true); render(16);
                disable("two");
            }
            auto bad_tempo = tempo; bad_tempo["sequences"][1]["start_frame"] = 4800u;
            expect(!load(bad_tempo, error), "tempo layers whose starts differ in beats rejected");
            // References: the file decoded continuously, played as a plain rule with the same loop.
            auto reference_at = [&](uint64_t loop_start, uint64_t loop_end, uint64_t offset) {
                auto r = loop; r["sequences"][0]["id"] = 61;
                r["sequences"][0]["loop_start_frame"] = loop_start; r["sequences"][0]["loop_end_frame"] = loop_end;
                enable("tempo-reference", 0, load(r, error));
                update(1, 61, 32767, ~0u, true);
                std::vector<float> skip(48000 * 2);
                for (uint64_t left = offset; left;) {
                    size_t n = static_cast<size_t>(std::min<uint64_t>(left, 48000));
                    std::fill(skip.begin(), skip.end(), 0.0f); mix(skip.data(), n, 1, 100); left -= n;
                }
                auto out = render(480);
                update(1, -1, 0, ~0u, true); render(16);
                disable("tempo-reference");
                return out;
            };
            const uint64_t water_offset = static_cast<uint64_t>(std::llround(80.0 * 2.0 * 60.0 / 105.0 * 48000.0));
            const auto land_ref = reference_at(4800, 24000, 80 * 48000);
            const auto water_ref = reference_at(water["loop_start_frame"].get<uint64_t>(), water["loop_end_frame"].get<uint64_t>(), water_offset);
            NativeChannels t; t.valid = true; t.bpm = 120;
            for (unsigned c = 0; c < 16; ++c) t.gain[c] = (0x63FFu & (1u << c)) ? 255 : 0;
            enable("tempo", 0, tempo_pack);
            update(0, 50, 32767, 0x63FFu, true, t);
            render(80 * 48000);
            for (unsigned c = 0; c < 16; ++c) t.gain[c] = (0x1C00u & (1u << c)) ? 255 : 0;
            t.bpm = 105;
            update(0, 50, 32767, 0x1C00u, false, t);
            auto actual = render(480);
            float w_land = 1, w_water = 0;
            bool agrees = true;
            for (size_t frame = 0; frame < 480; ++frame) {
                w_land += (0 - w_land) / 240; w_water += (1 - w_water) / 240;
                for (unsigned c = 0; c < 2; ++c) {
                    auto i = frame * 2 + c;
                    agrees &= std::abs(actual[i] - (land_ref[i] * 0.75f * w_land + water_ref[i] * 0.75f * w_water)) < 0.00003f;
                }
            }
            expect(agrees, "slower layer enters at the native beat position; the playing layer keeps its phase");
            disable("tempo");
            update(0, -1, 0, ~0u, true); render(16);
        }
        {
            // On-demand pack: nothing replaces the original until the loader has checked and loaded the MP3.
            set_memory_limits(64 * 1024 * 1024, 0);
            nlohmann::json lazy_manifest = j;
            auto reader = [lazy_manifest](const std::string& name, size_t limit) {
                if (name == "audio.json") { auto t = lazy_manifest.dump(); return std::vector<char>(t.begin(), t.end()); }
                if (input.size() > limit) throw std::runtime_error("oversized fixture");
                return input;
            };
            Sizer sizer = [](const std::string&) { return std::optional<size_t>(input.size()); };
            auto lazy = load_pack(reader, error, sizer);
            expect(bool(lazy), "on-demand pack accepted");
            reset();
            enable("lazy", 0, lazy);
            expect(wait_loader_idle(10000), "loader checks the MP3s in the background");
            update(0, 4, 32767, ~0u, true);
            expect(wait_loader_idle(10000) && replaces(4, ~0u), "playing sequence loads its MP3 on demand");
            std::fill(pcm.begin(), pcm.end(), 0);
            mix(pcm.data(), pcm.size() / 2, 1, 100);
            expect(energy(pcm) > 0, "on-demand MP3 decoded and mixed");
            disable("lazy");
            Sizer missing = [](const std::string&) { return std::optional<size_t>(); };
            expect(!load_pack(reader, error, missing), "on-demand pack with a missing MP3 rejected");
            auto layered_lazy = family;
            for (auto& s : layered_lazy["sequences"]) s.erase("end_frame");
            nlohmann::json layered_manifest = layered_lazy;
            auto layered_reader = [layered_manifest](const std::string& name, size_t limit) {
                if (name == "audio.json") { auto t = layered_manifest.dump(); return std::vector<char>(t.begin(), t.end()); }
                if (input.size() > limit) throw std::runtime_error("oversized fixture");
                return input;
            };
            expect(!load_pack(layered_reader, error, sizer), "on-demand layers must state their end frame");
            set_memory_limits(384 * 1024 * 1024, 96 * 1024 * 1024);
        }
        if (argc >= 4) {
            const auto root = std::filesystem::u8path(argv[3]);
            auto actual_pack = load_pack([&](const std::string& name, size_t limit) {
                auto path = root / std::filesystem::u8path(name);
                if (std::filesystem::file_size(path) > limit) throw std::runtime_error("oversized pack fixture");
                std::ifstream file(path, std::ios::binary);
                return std::vector<char>(std::istreambuf_iterator<char>(file), {});
            }, error);
            expect(bool(actual_pack), "built private pack and all four real MP3s validate");
            if (!actual_pack) std::fprintf(stderr, "pack: %s\n", error.c_str());
            else {
                enable("real-pack-fixture", 0, actual_pack);
                for (unsigned c = 0; c < 16; ++c) native.gain[c] = (0x63FFu & (1u << c)) ? 255 : 0;
                update(0, 50, 32767, 0x63FF, true, native);
                render(80 * 48000); note_queue_result(0);
                for (unsigned c = 0; c < 16; ++c) native.gain[c] = 128;
                update(0, 50, 32767, 0x1C00, false, native);
                auto actual = render(4800);
                expect(energy(actual) > 0, "real surface and underwater MP3s mix at shared 1:20 phase");
                capture_pcm(actual);
                note_queue_result(0);
                if (check_log) expect(log_text().find("source_frame=3840000") != std::string::npos &&
                    log_text().find("audio/09. Spiral Mountain (Underwater).mp3") != std::string::npos,
                    "real pack underwater queue proof includes 1:20 source position");
                disable("real-pack-fixture");
            }
        }
        std::printf("MP3 playback, loops, retriggers, native layers at 1:20, reversals, selectors and gains tested.\n");
    }
    else std::printf("SKIP MP3 playback fixture (pass a 48000 Hz stereo MP3); manifest and passthrough checks ran.\n");
    if (pcm_capture.is_open()) {
        pcm_capture.flush();
        expect(bool(pcm_capture), "PCM capture flushed");
    }
    std::printf("audio mod failures=%d\n", failures);
    return failures ? 1 : 0;
}
