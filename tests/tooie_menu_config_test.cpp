// Real librecomp Config integration for the Interpolation option: persisted ON/OFF, legacy/invalid configs load OFF,
// ON unlocks and fixes the refresh rate, a stored 120 survives boot, UI toggles settle only on Apply (no restart),
// ON->OFF->ON round trips, cancel reverts, the registry may reallocate, and the saved JSON matches.
// Needs librecomp/src/config.cpp + config_option.cpp (see tooie_menu_config_test in CMakeLists.txt).
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include "tooie_menu_policy.h"
#include "librecomp/config.hpp"
#include "librecomp/files.hpp"
#include "librecomp/game.hpp"
#include "librecomp/mods.hpp"

static std::filesystem::path g_dir;
std::filesystem::path recomp::get_config_path() { return g_dir; }
std::ifstream recomp::open_input_backup_file(const std::filesystem::path& p, std::ios_base::openmode m) { return std::ifstream(p.string() + ".bak", m); }
std::ofstream recomp::open_output_file_with_backup(const std::filesystem::path& p, std::ios_base::openmode m) { return std::ofstream(p, m); }
bool recomp::finalize_output_file_with_backup(const std::filesystem::path&) { return true; }
const recomp::Version& recomp::get_project_version() { static recomp::Version v; return v; }

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); std::exit(1); } } while (0)

static std::vector<std::pair<std::string, recomp::config::Config>> g_configs; // same shape as recompui registry
static std::vector<bool> g_sunk;
static int g_load_callbacks = 0;

static recomp::config::Config& get(const std::string& id) {
    for (auto& [k, c] : g_configs) if (k == id) return c;
    std::abort();
}

static constexpr uint32_t kOrig = 0, kDisp = 1, kMan = 2;

// Boots a fresh graphics config from `json_text` (empty = no file) exactly as banjo::init_config does: rr options first,
// interpolation last, policy installed, later tabs created (registry reallocates), then loaded.
static recomp::config::Config& boot(const std::string& json_text) {
    g_configs.clear();
    g_sunk.clear();
    g_load_callbacks = 0;
    std::filesystem::remove(g_dir / "graphics.json");
    if (!json_text.empty()) std::ofstream(g_dir / "graphics.json") << json_text;

    g_configs.push_back({"general", recomp::config::Config("General", "general", false)});
    g_configs.push_back({"graphics", recomp::config::Config("Graphics", "graphics", true)});
    recomp::config::Config& c = g_configs.back().second;
    c.add_enum_option("rr_option", "FR", "", {{0u, "Original"}, {1u, "Display"}, {2u, "Manual"}}, 1u);
    c.add_number_option("rr_manual_value", "", "", 20, 240, 1, 0, false, 60);
    c.add_bool_option("interpolation", "Interpolation", "", false);
    c.set_load_callback([] { g_load_callbacks++; });
    tooie_menu::install_interpolation_policy([]() -> recomp::config::Config& { return get("graphics"); },
        "rr_option", "rr_manual_value", "interpolation", [](bool on) { g_sunk.push_back(on); });
    for (int i = 0; i < 16; i++) { // `c` now dangles
        g_configs.push_back({"extra" + std::to_string(i), recomp::config::Config("X", "extra" + std::to_string(i), false)});
    }
    recomp::config::Config& g = get("graphics");
    CHECK(g.load_config());
    return g;
}

static uint32_t mode(recomp::config::Config& g, bool temp = false) { return std::get<uint32_t>(temp ? g.get_temp_option_value("rr_option") : g.get_option_value("rr_option")); }
static double manual(recomp::config::Config& g, bool temp = false) { return std::get<double>(temp ? g.get_temp_option_value("rr_manual_value") : g.get_option_value("rr_manual_value")); }
static bool interp(recomp::config::Config& g, bool temp = false) { return std::get<bool>(temp ? g.get_temp_option_value("interpolation") : g.get_option_value("interpolation")); }
static bool locked(recomp::config::Config& g) {
    const auto& ids = g.get_config_schema().options_by_id;
    const bool a = g.is_config_option_disabled(ids.at("rr_option"));
    const bool b = g.is_config_option_disabled(ids.at("rr_manual_value"));
    CHECK(a == b);
    return a;
}
static nlohmann::json saved() { nlohmann::json j; std::ifstream(g_dir / "graphics.json") >> j; return j; }

int main() {
#if defined(_WIN32)
    _putenv_s("TOOIE_TEST_INTERPOLATION_FPS", "");
#else
    unsetenv("TOOIE_TEST_INTERPOLATION_FPS");
#endif
    g_dir = std::filesystem::temp_directory_path() / "tooie_cfg_test";
    std::filesystem::create_directories(g_dir);

    // Legacy config (no interpolation key, old saved 240/Manual): OFF, Manual/30 locked, nothing else lost.
    {
        auto& g = boot(R"({"rr_option":"Manual","rr_manual_value":240})");
        CHECK(g_load_callbacks == 1); // frontend load callback preserved, ran after enforcement
        CHECK(!interp(g) && mode(g) == kMan && manual(g) == 30.0 && manual(g, true) == 30.0 && locked(g));
        CHECK(g_sunk.size() == 1 && g_sunk[0] == false);
        // A UI attempt while OFF is forced back.
        g.set_option_value("rr_manual_value", 240.0);
        g.apply_option_value("rr_manual_value");
        CHECK(manual(g) == 30.0);
        CHECK(g.save_config());
        CHECK(saved()["rr_manual_value"] == 30 && saved()["rr_option"] == "Manual" && saved()["interpolation"] == false);
    }
    // Fresh install (no file) and invalid values: OFF.
    { auto& g = boot(""); CHECK(!interp(g) && manual(g) == 30.0 && locked(g) && g_sunk == std::vector<bool>{false}); }
    for (const char* bad : {R"({"interpolation":"yes"})", R"({"interpolation":1})", R"({"interpolation":null})", R"({"interpolation":"true","rr_option":"Display"})"}) {
        auto& g = boot(bad);
        CHECK(!interp(g) && mode(g) == kMan && manual(g) == 30.0 && locked(g) && g_sunk == std::vector<bool>{false});
    }
    // Persisted ON boots ON: a stored 120 survives, a useless 30/Original becomes 60, Display is kept; controls unlocked.
    {
        auto& g = boot(R"({"interpolation":true,"rr_option":"Manual","rr_manual_value":120})");
        CHECK(interp(g) && mode(g) == kMan && manual(g) == 120.0 && !locked(g) && g_sunk == (std::vector<bool>{false, true}));
        CHECK(g.save_config());
        CHECK(saved()["interpolation"] == true && saved()["rr_manual_value"] == 120);
    }
    { auto& g = boot(R"({"interpolation":true,"rr_option":"Manual","rr_manual_value":30})"); CHECK(interp(g) && manual(g) == 60.0 && !locked(g) && g_sunk == (std::vector<bool>{false, true})); }
    { auto& g = boot(R"({"interpolation":true,"rr_option":"Original"})"); CHECK(mode(g) == kMan && manual(g) == 60.0 && g_sunk == (std::vector<bool>{false, true})); }
    { auto& g = boot(R"({"interpolation":true,"rr_option":"Display"})"); CHECK(mode(g) == kDisp && !locked(g)); }

    // Runtime toggles in one session: OFF -> ON -> OFF -> ON. The sink (RT64 request) only moves on Apply.
    {
        auto& g = boot("");
        g.set_option_value("interpolation", true);
        CHECK(interp(g, true) && !interp(g)); // pending
        CHECK(g_sunk == std::vector<bool>{false}); // not applied yet
        CHECK(mode(g, true) == kMan && manual(g, true) == 60.0 && !locked(g)); // UI already shows a usable 60
        CHECK(manual(g) == 30.0); // applied rate untouched until Apply
        CHECK(g.is_dirty());
        // While ON a useless 20 is snapped to 60 in the UI.
        g.set_option_value("rr_manual_value", 20.0);
        CHECK(manual(g, true) == 60.0);
        CHECK(g.save_config());
        CHECK(interp(g) && manual(g) == 60.0 && mode(g) == kMan && g_sunk == (std::vector<bool>{false, true}));
        CHECK(saved()["interpolation"] == true && saved()["rr_manual_value"] == 60 && saved()["rr_option"] == "Manual");

        g.set_option_value("rr_manual_value", 90.0); // user picks 90 while ON
        CHECK(g.save_config() && manual(g) == 90.0 && g_sunk.size() == 2);

        g.set_option_value("interpolation", false);
        CHECK(manual(g, true) == 30.0 && locked(g) && g_sunk.size() == 2);
        CHECK(g.save_config());
        CHECK(!interp(g) && manual(g) == 30.0 && g_sunk == (std::vector<bool>{false, true, false}));
        CHECK(saved()["interpolation"] == false && saved()["rr_manual_value"] == 30);

        g.set_option_value("interpolation", true);
        CHECK(g.save_config());
        CHECK(interp(g) && manual(g) == 60.0 && !locked(g) && g_sunk == (std::vector<bool>{false, true, false, true}));
        CHECK(saved()["interpolation"] == true);
    }
    // Cancel (revert) restores the UI without touching the sink or the applied state.
    {
        auto& g = boot("");
        g.set_option_value("interpolation", true);
        CHECK(manual(g, true) == 60.0);
        g.revert_temp_config();
        CHECK(!interp(g, true) && manual(g, true) == 30.0 && locked(g) && g_sunk == std::vector<bool>{false});
    }
    // Same from a persisted-ON boot: revert after a pending OFF keeps ON and 120.
    {
        auto& g = boot(R"({"interpolation":true,"rr_option":"Manual","rr_manual_value":120})");
        g.set_option_value("interpolation", false);
        CHECK(locked(g) && manual(g, true) == 30.0 && manual(g) == 120.0);
        g.revert_temp_config();
        CHECK(interp(g, true) && manual(g, true) == 120.0 && !locked(g) && g_sunk == (std::vector<bool>{false, true}));
    }

    // Legacy env override is explicit: forces ON at 60/120 even with the option OFF; bad values are ignored.
    auto with_env = [&](const char* env, bool want_on, double want_rate) {
#if defined(_WIN32)
        CHECK(_putenv_s("TOOIE_TEST_INTERPOLATION_FPS", env) == 0);
#else
        CHECK(setenv("TOOIE_TEST_INTERPOLATION_FPS", env, 1) == 0);
#endif
        auto& g = boot(R"({"rr_option":"Manual","rr_manual_value":240})");
        CHECK(g_sunk == std::vector<bool>{want_on}); // install-time state already effective; Load does not repeat it
        CHECK(mode(g) == kMan && manual(g) == want_rate);
        CHECK(!interp(g)); // the stored option is not rewritten by the override
    };
    with_env("60", true, 60.0);
    with_env("120", true, 120.0);
    with_env("90", false, 30.0);
    with_env("", false, 30.0);
#if !defined(_WIN32)
    unsetenv("TOOIE_TEST_INTERPOLATION_FPS");
#endif

    std::puts("tooie_menu_config_test PASS");
    return 0;
}
