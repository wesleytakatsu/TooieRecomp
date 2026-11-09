#include "tooie_language_pack_loader.h"
#include "tooie_language_pack.h"
#include "tooie_language_paths.h"
#include "tooie_language_bundle.h"
#include "tooie_language_font.h"
#include "tooie_text_font_map.h"
#include "banjo_config.h"
#include "librecomp/game.hpp"
#include "ultramodern/ultramodern.hpp"


#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>

void tooie_apply_language_pack() {
    const auto start = std::chrono::steady_clock::now();
    tooie_text_font_map::reset();
    const char* override_path = std::getenv("TOOIE_TEST_LANGUAGE_PACK");
    bool is_override = override_path != nullptr && override_path[0] != '\0';
    banjo::Language language = banjo::Language::English;
    bool is_bundle = false;
    std::filesystem::path path;
    auto fail = [&](const std::string& reason) {
        std::fprintf(stderr, "[tooie-lang] %s: %s\n", reason.c_str(), path.string().c_str());
        if (!is_override) {
            std::string message = "Language pack missing or invalid: " + path.string() + " (" + reason + "). The game starts in English.";
            message += language == banjo::Language::Japanese ? " Restore the Japanese language file (ja.tlpb)." :
                       " Restore the assets/languages files supplied with the game.";
            ultramodern::error_handling::message_box(message.c_str());
        }
    };

    try {
        if (is_override) {
            path = override_path;
        } else {
            language = banjo::get_language();
            if (language == banjo::Language::English) {
                return;
            }
            path = tooie_language_paths::language_bundle_path(banjo::language_tag(language));
            is_bundle = std::filesystem::exists(path);
            if (!is_bundle && language != banjo::Language::PortugueseBrazil) {
                path = tooie_language_paths::language_pack_path(banjo::language_tag(language));
            }
        }
        std::vector<uint8_t> bytes;
        if (!tooie_language_paths::read_binary_file(path, bytes)) {
            fail("cannot read pack");
            return;
        }

        auto image = recomp::get_rom();
        tooie_language_pack::LanguagePack pack;
        std::string error;
        std::vector<uint8_t> rewritten;
        is_bundle = is_bundle || (bytes.size() >= 4 && std::equal(bytes.begin(), bytes.begin() + 4, "TLPB"));
        if (is_bundle) {
            tooie_language_bundle::Bundle bundle;
            if (!tooie_language_bundle::parse(bytes, bundle, error) ||
                !tooie_language_bundle::build(bundle, image, pack, error)) {
                fail(error);
                return;
            }
        } else if (!tooie_language_pack::parse(bytes, image.size(), pack, error)) {
            fail(error);
            return;
        }
        if (!is_override && pack.language != banjo::language_tag(language)) {
            fail("language tag mismatch");
            return;
        }
        if (!tooie_language_font::normalize(pack, error) || !tooie_language_pack::rewrite(image, pack, rewritten, error)) {
            fail(error);
            return;
        }

        if (pack.language == "ja" && !tooie_text_font_map::set_japanese_font_map(pack.font1_charset)) {
            fail("invalid Japanese font maps");
            return;
        }
        const auto old_size = image.size();
        const auto new_size = rewritten.size();
        recomp::set_rom_contents(std::move(rewritten));
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        if (pack.language != "ja" && !pack.font1_charset.empty()) {
            tooie_text_font_map::set_european_font_map(pack.font1_charset, pack.font1_accent_map);
        }
        std::fprintf(stderr, "[tooie-lang] applied %zu assets (%s) from %s image %zu -> %zu bytes in %lld ms\n",
            pack.entries.size(), pack.language.c_str(), is_bundle ? "bundle" : "pack", old_size, new_size,
            static_cast<long long>(elapsed));
    } catch (const std::exception& exception) {
        fail(std::string("unable to load pack: ") + exception.what());
    }
}
