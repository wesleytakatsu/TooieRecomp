// Real librecomp Config integration: saved 240/Manual must load as 30, survive a vector reallocation of the
// config registry (as in recompui), keep the load callback, and persist 30 on save.
// Needs librecomp/src/config.cpp + config_option.cpp (see command in docs/handoff or root CMake snippet).
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

static recomp::config::Config& get(const std::string& id) {
    for (auto& [k, c] : g_configs) if (k == id) return c;
    std::abort();
}

int main() {
    g_dir = std::filesystem::temp_directory_path() / "tooie_cfg_test";
    std::filesystem::create_directories(g_dir);
    { std::ofstream(g_dir / "graphics.json") << R"({"rr_option":"Manual","rr_manual_value":240})"; }

    g_configs.push_back({"general", recomp::config::Config("General", "general", false)});
    g_configs.push_back({"graphics", recomp::config::Config("Graphics", "graphics", true)});
    recomp::config::Config& c = g_configs.back().second;
    int applied = 0;
    c.add_enum_option("rr_option", "FR", "", {{0u, "Original"}, {1u, "Display"}, {2u, "Manual"}}, 1u);
    c.add_number_option("rr_manual_value", "", "", 20, 240, 1, 0, false, 60);
    c.set_load_callback([&] { applied++; });
    tooie_menu::install_forced_refresh([]() -> recomp::config::Config& { return get("graphics"); }, "rr_option", "rr_manual_value");

    // Later tabs force the registry vector to reallocate: `c` now dangles.
    for (int i = 0; i < 16; i++) {
        g_configs.push_back({"extra" + std::to_string(i), recomp::config::Config("X", "extra" + std::to_string(i), false)});
    }

    recomp::config::Config& g = get("graphics");
    CHECK(g.load_config());
    CHECK(applied == 1); // frontend load callback preserved and ran after enforcement
    CHECK(std::get<uint32_t>(g.get_option_value("rr_option")) == 2);
    CHECK(std::get<double>(g.get_option_value("rr_manual_value")) == 30.0);
    CHECK(std::get<double>(g.get_temp_option_value("rr_manual_value")) == 30.0);

    // A later set attempt (e.g. UI) is also forced back.
    g.set_option_value("rr_manual_value", 240.0);
    g.apply_option_value("rr_manual_value");
    CHECK(std::get<double>(g.get_option_value("rr_manual_value")) == 30.0);

    CHECK(g.save_config());
    nlohmann::json j;
    std::ifstream(g_dir / "graphics.json") >> j;
    CHECK(j["rr_manual_value"] == 30 && j["rr_option"] == "Manual");
    std::puts("tooie_menu_config_test PASS");
    return 0;
}
