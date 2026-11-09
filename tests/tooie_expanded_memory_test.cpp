#include "tooie_expanded_memory.h"
#include "tooie_object_distance.h"
#include "banjo_config.h"
#include "librecomp/config.hpp"
#include "librecomp/files.hpp"
#include "librecomp/game.hpp"
#include "librecomp/overlays.hpp"
#include "hle/rt64_host_rdram.h"
#include "funcs.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

static int checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); std::exit(1); } } while (0)
static std::filesystem::path config_dir;
std::filesystem::path recomp::get_config_path() { return config_dir; }
std::ifstream recomp::open_input_backup_file(const std::filesystem::path& p, std::ios_base::openmode m) { return std::ifstream(p.string() + ".bak", m); }
std::ofstream recomp::open_output_file_with_backup(const std::filesystem::path& p, std::ios_base::openmode m) { return std::ofstream(p, m); }
bool recomp::finalize_output_file_with_backup(const std::filesystem::path&) { return true; }
const recomp::Version& recomp::get_project_version() { static recomp::Version v; return v; }
std::span<const uint8_t> recomp::overlays::get_patch_binary() { return {}; }
static tooie_object_distance::Mode saved_distance = tooie_object_distance::Mode::Original;
void tooie_object_distance::set_mode(uint32_t v) { saved_distance = Mode(v); }
tooie_object_distance::Mode tooie_object_distance::mode() { return saved_distance; }

extern "C" {
int32_t* section_addresses = nullptr;
void switch_error(const char*, uint32_t, uint32_t) { CHECK(false); }
void do_break(uint32_t) { CHECK(false); }
void cop0_status_write(recomp_context*, gpr) { CHECK(false); }
gpr cop0_status_read(recomp_context*) { CHECK(false); return 0; }
void recomp_syscall_handler(uint8_t*, recomp_context*, int32_t) { CHECK(false); }
void pause_self(uint8_t*) { CHECK(false); }
bool ultramodern_preemption_hint() { return false; }
void ultramodern_service_preemption(uint8_t*, void*) { CHECK(false); }
recomp_func_t* get_function(int32_t address) {
    switch (uint32_t(address)) {
    case 0x8001E7E0: return tooie_heap_end;
    case 0x8001E830: return tooie_heap_size;
    case 0x80012194: return func_80012194;
    case 0x8002D6E0: return [](uint8_t* rdram, recomp_context* c) {
        for (uint32_t i = 0; i < uint32_t(c->r5); ++i) MEM_B(i, c->r4) = 0;
    };
    case 0x8002D780: return [](uint8_t*, recomp_context*) {};
    // The real native heap implementation, not an allocator simulation.
    case 0x8001A6C0: return func_8001A6C0;
    case 0x8001A6EC: return heap_update_empty_block;
    case 0x8001A7B0: return heap_init_empty_block;
    case 0x8001A8B4: return heap_setup;
    case 0x8001AAD8: return heap_bytes_used;
    case 0x8001AAE4: return find_free_block;
    case 0x8001AC9C: return has_free_block_after;
    case 0x8001ACCC: return heap_alloc;
    case 0x8001ACEC: return heap_alloc_sided;
    case 0x8001AF14: return heap_merge_free_block;
    case 0x8001B084: return heap_free;
    case 0x8001B0C8: return heap_resize_block_internal;
    case 0x8001B1C8: return heap_resize_block;
    case 0x8001B208: return heap_realloc;
    case 0x8001B388: return heap_get_allocation_size;
    case 0x8001B3A0: return func_8001B3A0;
    case 0x8001B3B0: return func_8001B3B0;
    case 0x8001B3C0: return heap_bytes_free;
    case 0x8001B3E8: return heap_bytes_allocated;
    case 0x8001B3F4: return heap_largest_free;
    case 0x8001B4B0: return func_8001B4B0;
    case 0x8001B4E8: return func_8001B4E8;
    case 0x8001B50C: return func_8001B50C;
    case 0x8001B518: return func_8001B518;
    case 0x8001B668: return func_8001B668;
    case 0x8001B710: return func_8001B710;
    case 0x8001B754: return func_8001B754;
    case 0x8001B798: return func_8001B798;
    case 0x8001B7B8: return func_8001B7B8;
    case 0x8001B7F0: return func_8001B7F0;
    case 0x8001B840: return func_8001B840;
    case 0x8001B84C: return func_8001B84C;
    case 0x8001B858: return func_8001B858;
    case 0x8001B864: return func_8001B864;
    case 0x8001B870: return defrag_next;
    case 0x8001B9BC: return defrag_prev;
    case 0x8001BAFC: return defrag;
    case 0x8001BB58: return is_prev_free;
    case 0x8001BBA0: return rare_memcpy;
    case 0x8001BBC8: return aligned4_memcpy;
    case 0x8001BBF0: return rare_memmove;
    case 0x8001BC68: return aligned4_memmove;
    case 0x8001BCF8: return rare_memset;
    case 0x8001BD30: return func_8001BD30;
    case 0x8001BD50: return func_8001BD50;
    case 0x8001BDAC: return func_8001BDAC;
    case 0x8001BE94: return func_8001BE94;
    case 0x8001BF80: return func_8001BF80;
    default: std::fprintf(stderr, "Unexpected heap call %08X\n", uint32_t(address)); CHECK(false); return nullptr;
    }
}
}

static recomp_context context() {
    recomp_context c{};
    c.r29 = int32_t(0x807FF000u);
    return c;
}
static uint32_t word(uint8_t* rdram, uint32_t address) { return MEM_W(0, int32_t(address)); }
static void set_word(uint8_t* rdram, uint32_t address, uint32_t value) { MEM_W(0, int32_t(address)) = value; }
static void environment(const char* name, const char* value) {
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

int main(int argc, char** argv) {
    if (argc == 1) {
        for (const char* scenario : {"native", "expanded", "killed", "guard-off"}) {
            const std::string command = "\"" + std::filesystem::absolute(argv[0]).string() + "\" " + scenario;
            CHECK(std::system(command.c_str()) == 0);
        }
        std::printf("PASS: four isolated boot scenarios.\n");
        return 0;
    }
    const std::string scenario = argv[1];
    const bool expanded = scenario == "expanded" || scenario == "guard-off";
    environment("TOOIE_TEST_EXPANDED_HEAP", scenario == "killed" ? "0" : "1");
    environment("TOOIE_TEST_DISTANCE_HEAP_GUARD", scenario == "guard-off" ? "0" : "1");
    config_dir = std::filesystem::temp_directory_path() /
        ("tooie_heap_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(config_dir);
    using recomp::config::Config;
    Config general("General", "general", false);
    tooie_memory::add_config(general, [&]() -> Config& { return general; });
    CHECK(general.load_config());
    const auto& key = banjo::configkeys::general::expanded_memory;
    CHECK(std::get<uint32_t>(general.get_option_value(key)) == 0);
    Config graphics("Graphics", "graphics", true);
    tooie_object_distance::add_config(graphics);
    tooie_memory::install_distance_notice([&]() -> Config& { return graphics; });
    std::ofstream(config_dir / "graphics.json") << R"({"object_distance":"Unlimited"})";
    CHECK(graphics.load_config());
    CHECK(tooie_object_distance::mode() == tooie_object_distance::Mode::Unlimited);
    CHECK(std::get<uint32_t>(general.get_option_value(key)) == 0); // Load does not opt in.
    if (scenario != "native") general.set_option_value(key, uint32_t(1));
    CHECK(general.save_config());
    tooie_memory::finish_config();
    CHECK(tooie_memory::boot.active() == expanded);
    CHECK(RT64::hostRdramMask() == (expanded ? 0xFFFFFFu : 0x7FFFFFu));
    const auto notice = [&]() { return general.get_enum_option_details(0); };
    CHECK(notice().empty() == (scenario != "killed"));
    for (uint32_t distance = 1; distance <= 3; ++distance) {
        graphics.set_option_value(tooie_object_distance::option, distance);
        CHECK(graphics.save_config());
        CHECK(std::get<uint32_t>(general.get_option_value(key)) == 1);
        CHECK(tooie_memory::boot.active() == expanded);
        CHECK(notice().empty() == expanded);
        CHECK(graphics.get_enum_option_details(0).empty() == expanded);
    }
    Config restored("General", "general", false);
    restored.add_enum_option(key, "Memory", "", {{0u,"Off","Off"},{1u,"On","On"}}, 0u);
    CHECK(restored.load_config());
    CHECK(std::get<uint32_t>(restored.get_option_value(key)) == 1);
    general.set_option_value(key, uint32_t(0));
    CHECK(notice().empty() == !expanded);
    tooie_memory::finish_config(); // Process state cannot be relatched.
    CHECK(tooie_memory::boot.active() == expanded);
    tooie_memory::BootState isolated;
    isolated.latch(true, false);
    isolated.latch(true, true);
    CHECK(!isolated.active() && isolated.restart_required(true));
    CHECK(tooie_memory::contains(0x80000000u, 0x800000));
    CHECK(!tooie_memory::contains(0x807FFFFFu, 2));
    CHECK(!tooie_memory::contains(0x80800000u));
    CHECK(!tooie_memory::contains(tooie_memory::expanded_base - 1, 2));
    CHECK(tooie_memory::contains(tooie_memory::expanded_base, tooie_memory::expanded_size) == expanded);
    CHECK(!tooie_memory::contains(tooie_memory::expanded_end));
    CHECK(!tooie_memory::contains(0xFFFFFFFCu, size_t(-1)));
    CHECK(!tooie_memory::aligned(0x80000001u));
    CHECK(tooie_memory::contains_alias(0xA0000100u, 4));
    CHECK(tooie_memory::contains_alias(tooie_memory::expanded_base + 0x20000000u, 4) == expanded);
    CHECK(!tooie_memory::contains_alias(0xC0000000u));
    CHECK(tooie_memory::layout_available(tooie_memory::expanded_base, 0xB000));
    CHECK(!tooie_memory::layout_available(tooie_memory::expanded_base + 1, 0));
    CHECK(!tooie_memory::layout_available(0x8080B5E0, 0xB001));
    CHECK(tooie_memory::guard_allows(tooie_memory::guard_free_reserve, tooie_memory::guard_largest_reserve));
    CHECK(!tooie_memory::guard_allows(tooie_memory::guard_free_reserve - 1, 0x100000));
    CHECK(!tooie_memory::guard_allows(0x100000, tooie_memory::guard_largest_reserve - 1));

    std::vector<uint8_t> memory(0x1000000, 0xA5);
    uint8_t* rdram = memory.data();
    auto c = context();
    tooie_heap_end(rdram, &c);
    CHECK(uint32_t(c.r2) == tooie_memory::boot.end());
    tooie_heap_size(rdram, &c);
    CHECK(uint32_t(c.r2) == tooie_memory::boot.size());
    if (!expanded) {
        func_8001E7E0(rdram, &c);
        CHECK(uint32_t(c.r2) == tooie_memory::boot.end());
        func_8001E830(rdram, &c);
        CHECK(uint32_t(c.r2) == tooie_memory::boot.size());
    }
    func_8001E7E8(rdram, &c);
    const uint32_t base = tooie_memory::boot.end() - tooie_memory::boot.size();
    CHECK(word(rdram, 0x8007E990) == base);
    CHECK(word(rdram, 0x8007E994) == base);
    CHECK(word(rdram, 0x8007E998) == tooie_memory::boot.end());
    c.r4 = 1;
    heap_setup(rdram, &c);
    CHECK(c.r29 == context().r29);
    CHECK(word(rdram, 0x8007BF00) == tooie_memory::boot.size());
    CHECK(word(rdram, base + 4) == base + 0x20);
    CHECK(word(rdram, base + 0x24) == tooie_memory::boot.end() - 0x20);
    if (expanded) CHECK(word(rdram, 0x80137800) == 0xA5A5A5A5); // Native region unused.
    heap_bytes_free(rdram, &c);
    CHECK(uint32_t(c.r2) == tooie_memory::boot.size());
    c.r4 = int32_t(0x807FE000u);
    heap_largest_free(rdram, &c);
    CHECK(c.r2 == 1);
    CHECK(word(rdram, 0x807FE000) == tooie_memory::boot.size() - 0x40);
    for (int side : {0, 1}) {
        c = context();
        c.r4 = 0x50000;
        c.r5 = side;
        heap_alloc_sided(rdram, &c);
        const uint32_t allocation = uint32_t(c.r2);
        CHECK(allocation >= base + 0x30 && allocation < tooie_memory::boot.end());
        CHECK(tooie_memory::contains(allocation, 0x50000));
        CHECK(c.r29 == context().r29);
        c.r4 = int32_t(allocation);
        heap_free(rdram, &c);
        heap_bytes_free(rdram, &c);
        CHECK(uint32_t(c.r2) == tooie_memory::boot.size());
    }
    func_800121D0(rdram, &c);
    CHECK(uint32_t(c.r2) == tooie_memory::boot.size());
    CHECK(uint32_t(c.r3) == tooie_memory::boot.end() - 0x80137390u - 1);
    CHECK(tooie_memory::distance_heap_available(rdram));
    set_word(rdram, 0x8003F750, tooie_memory::boot.size() - tooie_memory::guard_free_reserve + 1);
    CHECK(tooie_memory::distance_heap_available(rdram) == (scenario == "guard-off"));
    set_word(rdram, 0x8003F750, 0);
    set_word(rdram, base + 0x24, base + 0x1000);
    set_word(rdram, 0x800459C8, 1); // Next frame refreshes largest block.
    CHECK(tooie_memory::distance_heap_available(rdram) == (scenario == "guard-off"));
    std::filesystem::remove_all(config_dir);
    std::printf("PASS: %s, %d assertions (config/latch/hooks/native heap/ranges/guard).\n", scenario.c_str(), checks);
}
