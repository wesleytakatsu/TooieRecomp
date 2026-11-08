#include "tooie_language_import_ui.h"

#include "banjo_config.h"
#include "tooie_language_bundle.h"
#include "tooie_language_paths.h"
#include "recompui/config.h"
#include "recompui/recompui.h"

#include <cstdio>

namespace {
    bool restoring_language = false;

    bool has_language(banjo::Language language) {
        if (language == banjo::Language::English) return true;
        try {
            const char* tag = banjo::language_tag(language);
            const auto path = tooie_language_paths::language_bundle_path(tag);
            std::vector<uint8_t> bytes;
            std::string error;
            // Match boot-time precedence: repair a corrupt installed bundle instead
            // of silently hiding it behind an older user-imported pack.
            if (std::filesystem::exists(path)) {
                tooie_language_bundle::Bundle bundle;
                return tooie_language_paths::read_binary_file(path, bytes) &&
                    tooie_language_bundle::parse(bytes, bundle, error) && bundle.language == tag;
            }
            if (language == banjo::Language::PortugueseBrazil) return false;
            tooie_language_pack::LanguagePack pack;
            return tooie_language_paths::read_binary_file(tooie_language_paths::language_pack_path(tag), bytes) &&
                tooie_language_pack::parse(bytes, 35098224, pack, error) && pack.language == tag;
        } catch (const std::exception&) {
            return false;
        }
    }
}

void tooie_language_import_ui::on_language_change(recomp::config::ConfigValueVariant value,
                                                 recomp::config::ConfigValueVariant previous_value,
                                                 recomp::config::OptionChangeContext context) {
    if (restoring_language || context == recomp::config::OptionChangeContext::Load || recompui::is_prompt_open()) return;
    const auto* selected = std::get_if<uint32_t>(&value);
    if (selected == nullptr || has_language(static_cast<banjo::Language>(*selected))) return;

    const auto* previous = std::get_if<uint32_t>(&previous_value);
    if (previous == nullptr || !has_language(static_cast<banjo::Language>(*previous))) {
        previous_value = static_cast<uint32_t>(banjo::Language::English);
    }
    restoring_language = true;
    recompui::config::get_general_config().update_option_value(banjo::configkeys::general::language, previous_value);
    restoring_language = false;
    std::fprintf(stderr, "[tooie-lang] selected language files missing or invalid\n");
    recompui::open_info_prompt("Language files unavailable",
        "Restore the assets/languages files supplied with the game, then select the language again. "
        "Only the USA ROM is required. The previous language has been restored.",
        "OK", {}, recompui::ButtonStyle::Danger);
}
