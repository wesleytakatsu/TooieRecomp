#include "tooie_language_import_ui.h"

#include "banjo_config.h"
#include "tooie_language_import.h"
#include "tooie_language_pack.h"
#include "tooie_language_paths.h"
#include "recompui/config.h"
#include "recompui/recompui.h"
#include "util/file.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace {
    constexpr size_t kUsaImageSize = 35098224;
    bool restoring_language = false;

    struct Texts {
        const char* header;
        const char* body;
        const char* confirm;
        const char* cancel;
        const char* success_header;
        const char* success_body;
        const char* failure_header;
        const char* failure_body;
        const char* usa_header;
        const char* usa_body;
    };

    const Texts* texts_for(banjo::Language language) {
        static const Texts french = {
            "Importer le français",
            "Pour jouer en français, le jeu doit extraire les textes français de la ROM européenne « Banjo-Tooie (Europe) (En,Fr,De,Es).z64 ». Cette ROM sert UNIQUEMENT à cette extraction, faite une seule fois. Le jeu fonctionne toujours avec la ROM américaine (USA).",
            "Choisir la ROM européenne", "Annuler",
            "Français importé", "Textes français importés. Ils s'appliquent au démarrage du jeu ; si une partie est en cours, redémarrez le jeu.",
            "Échec de l'importation", "Impossible d'importer : {reason}. Vérifiez qu'il s'agit bien de la ROM « Banjo-Tooie (Europe) (En,Fr,De,Es).z64 ».",
            "ROM américaine requise", "Chargez d'abord la ROM américaine (USA) dans le lanceur (bouton « Load ROM »). Ensuite, sélectionnez à nouveau cette langue."
        };
        static const Texts german = {
            "Deutsch importieren",
            "Um auf Deutsch zu spielen, muss das Spiel die deutschen Texte aus dem europäischen ROM „Banjo-Tooie (Europe) (En,Fr,De,Es).z64“ extrahieren. Dieses ROM wird NUR für diese einmalige Extraktion verwendet. Das Spiel selbst läuft immer mit dem amerikanischen ROM (USA).",
            "Europäisches ROM auswählen", "Abbrechen",
            "Deutsch importiert", "Deutsche Texte importiert. Sie gelten beim Spielstart; läuft das Spiel bereits, starte es neu.",
            "Import fehlgeschlagen", "Import nicht möglich: {reason}. Bitte prüfe, ob es das ROM „Banjo-Tooie (Europe) (En,Fr,De,Es).z64“ ist.",
            "Amerikanisches ROM erforderlich", "Lade zuerst das amerikanische ROM (USA) im Launcher (Schaltfläche „Load ROM“). Wähle danach diese Sprache erneut."
        };
        static const Texts spanish = {
            "Importar español",
            "Para jugar en español, el juego necesita extraer los textos en español de la ROM europea «Banjo-Tooie (Europe) (En,Fr,De,Es).z64». Esta ROM se usa SOLO para esta extracción, que se hace una sola vez. El juego siempre funciona con la ROM estadounidense (USA).",
            "Elegir la ROM europea", "Cancelar",
            "Español importado", "Textos en español importados. Se aplican al iniciar el juego; si ya estás jugando, reinicia el juego.",
            "Error al importar", "No se pudo importar: {reason}. Comprueba que sea la ROM «Banjo-Tooie (Europe) (En,Fr,De,Es).z64».",
            "Se necesita la ROM estadounidense", "Primero carga la ROM estadounidense (USA) en el lanzador (botón «Load ROM»). Después, vuelve a elegir este idioma."
        };
        switch (language) {
            case banjo::Language::French:
                return &french;
            case banjo::Language::German:
                return &german;
            case banjo::Language::Spanish:
                return &spanish;
            default:
                return nullptr;
        }
    }

    bool has_valid_pack(banjo::Language language) {
        if (texts_for(language) == nullptr) {
            return true;
        }
        try {
            std::vector<uint8_t> bytes;
            tooie_language_pack::LanguagePack pack;
            std::string error;
            return tooie_language_paths::read_binary_file(tooie_language_paths::language_pack_path(banjo::language_tag(language)), bytes) &&
                tooie_language_pack::parse(bytes, kUsaImageSize, pack, error) &&
                pack.language == banjo::language_tag(language);
        } catch (const std::exception&) {
            return false;
        }
    }

    void restore_language(recomp::config::ConfigValueVariant previous_value) {
        const auto* previous = std::get_if<uint32_t>(&previous_value);
        if (previous == nullptr || !has_valid_pack(static_cast<banjo::Language>(*previous))) {
            previous_value = static_cast<uint32_t>(banjo::Language::English);
        }
        restoring_language = true;
        recompui::config::get_general_config().update_option_value(banjo::configkeys::general::language, previous_value);
        restoring_language = false;
    }

    bool write_pack(const std::filesystem::path& path, const std::vector<uint8_t>& bytes, std::string& error) {
        std::filesystem::create_directories(path.parent_path());
        std::random_device random;
        std::filesystem::path temporary;
        do {
            temporary = path;
            temporary += ".tmp-" + std::to_string(random()) + "-" + std::to_string(random());
        } while (std::filesystem::exists(temporary));
        try {
            std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
            file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            file.close();
            if (!file) {
                error = "cannot write language pack";
                std::filesystem::remove(temporary);
                return false;
            }
#ifdef _WIN32
            if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING)) {
                error = "cannot replace language pack";
                std::filesystem::remove(temporary);
                return false;
            }
#else
            std::filesystem::rename(temporary, path);
#endif
        } catch (...) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            throw;
        }
        return true;
    }

    bool import_language(const std::filesystem::path& europe_path, const std::vector<uint8_t>& usa,
                         banjo::Language language, std::string& error) {
        try {
            std::vector<uint8_t> europe;
            if (!tooie_language_paths::read_binary_file(europe_path, europe)) {
                error = "cannot read European ROM";
                return false;
            }
            tooie_language_pack::LanguagePack pack;
            std::vector<uint8_t> bytes;
            return tooie_text_assets::validate_image(europe, true, error) &&
                tooie_language_import::build_unchecked(europe, usa, banjo::language_tag(language), pack, error) &&
                tooie_language_pack::serialize(pack, bytes, error) &&
                write_pack(tooie_language_paths::language_pack_path(banjo::language_tag(language)), bytes, error);
        } catch (const std::exception&) {
            error = "file operation failed";
            return false;
        }
    }

}

void tooie_language_import_ui::on_language_change(recomp::config::ConfigValueVariant value,
                                                   recomp::config::ConfigValueVariant previous_value,
                                                   recomp::config::OptionChangeContext context) {
    if (restoring_language || context == recomp::config::OptionChangeContext::Load || recompui::is_prompt_open()) {
        return;
    }
    const auto* selected = std::get_if<uint32_t>(&value);
    if (selected == nullptr) {
        return;
    }
    auto language = static_cast<banjo::Language>(*selected);
    const Texts* texts = texts_for(language);
    if (texts == nullptr || has_valid_pack(language)) {
        return;
    }
    auto usa = std::make_shared<std::vector<uint8_t>>();
    std::string usa_error;
    bool valid_usa = false;
    try {
        valid_usa = tooie_language_paths::read_binary_file(tooie_language_paths::usa_image_path(), *usa) &&
            tooie_text_assets::validate_image(*usa, false, usa_error);
    } catch (const std::exception&) {
        valid_usa = false;
    }
    if (!valid_usa) {
        restore_language(previous_value);
        recompui::open_info_prompt(texts->usa_header, texts->usa_body, "OK", {}, recompui::ButtonStyle::Danger);
        return;
    }

    auto confirmed = std::make_shared<bool>(false);
    recompui::open_choice_prompt(texts->header, texts->body, texts->confirm, texts->cancel,
        [language, texts, previous_value, usa, confirmed]() {
            *confirmed = true;
            recompui::file::open_file_dialog([language, texts, previous_value, usa](bool success, const std::filesystem::path& path) {
                const auto start = std::chrono::steady_clock::now();
                if (!success) {
                    std::fprintf(stderr, "[tooie-lang] import %s failed: file selection cancelled\n", banjo::language_tag(language));
                    restore_language(previous_value);
                    return;
                }
                std::string error;
                if (import_language(path, *usa, language, error)) {
                    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start).count();
                    std::fprintf(stderr, "[tooie-lang] imported %s in %lld ms\n", banjo::language_tag(language),
                        static_cast<long long>(elapsed));
                    recompui::open_info_prompt(texts->success_header, texts->success_body, "OK", {}, recompui::ButtonStyle::Success);
                    return;
                }
                std::fprintf(stderr, "[tooie-lang] import %s failed: %s\n", banjo::language_tag(language), error.c_str());
                restore_language(previous_value);
                std::string body = texts->failure_body;
                body.replace(body.find("{reason}"), 8, error);
                recompui::open_info_prompt(texts->failure_header, body, "OK", {}, recompui::ButtonStyle::Danger);
            });
        },
        [previous_value, confirmed]() {
            if (!*confirmed) {
                restore_language(previous_value);
            }
        });
}
