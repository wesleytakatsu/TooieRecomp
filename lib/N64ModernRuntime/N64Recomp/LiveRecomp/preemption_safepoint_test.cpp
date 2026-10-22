// T11: static C / LiveRecomp preemption safepoint placement.
// Proves request-site policy in recompilation.cpp, not backend-local guesses.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "recompiler/context.h"
#include "recompiler/live_recompiler.h"
#include "recomp.h"

namespace {

int g_fail = 0;
int g_pass = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (cond) {                                                          \
            ++g_pass;                                                        \
        } else {                                                             \
            ++g_fail;                                                        \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                    \
    } while (0)

constexpr uint32_t kVram = 0x80001000u;
constexpr int kRa = 31;
constexpr int kA0 = 4;
constexpr int kT0 = 8;
constexpr int kT1 = 9;
constexpr int kT2 = 10;

uint32_t r_type(uint32_t rs, uint32_t rt, uint32_t rd, uint32_t funct) {
    return (rs << 21) | (rt << 16) | (rd << 11) | funct;
}
uint32_t i_type(uint32_t op, uint32_t rs, uint32_t rt, uint32_t imm) {
    return (op << 26) | (rs << 21) | (rt << 16) | (imm & 0xFFFFu);
}
uint32_t j_type(uint32_t op, uint32_t target) {
    return (op << 26) | ((target >> 2) & 0x03FFFFFFu);
}

uint32_t addiu(uint32_t rt, uint32_t rs, int16_t imm) { return i_type(0x09, rs, rt, static_cast<uint16_t>(imm)); }
uint32_t bne(uint32_t rs, uint32_t rt, int16_t off) { return i_type(0x05, rs, rt, static_cast<uint16_t>(off)); }
uint32_t beq(uint32_t rs, uint32_t rt, int16_t off) { return i_type(0x04, rs, rt, static_cast<uint16_t>(off)); }
uint32_t beql(uint32_t rs, uint32_t rt, int16_t off) { return i_type(0x14, rs, rt, static_cast<uint16_t>(off)); }
uint32_t bnel(uint32_t rs, uint32_t rt, int16_t off) { return i_type(0x15, rs, rt, static_cast<uint16_t>(off)); }
uint32_t j(uint32_t target) { return j_type(0x02, target); }
uint32_t jal(uint32_t target) { return j_type(0x03, target); }
uint32_t jr(uint32_t rs) { return r_type(rs, 0, 0, 0x08); }
uint32_t lui(uint32_t rt, uint16_t imm) { return i_type(0x0F, 0, rt, imm); }
uint32_t lw(uint32_t rt, uint32_t rs, int16_t imm) { return i_type(0x23, rs, rt, static_cast<uint16_t>(imm)); }
uint32_t addu(uint32_t rd, uint32_t rs, uint32_t rt) { return r_type(rs, rt, rd, 0x21); }

void append_be(std::vector<uint8_t>& rom, uint32_t word) {
    rom.push_back(static_cast<uint8_t>(word >> 24));
    rom.push_back(static_cast<uint8_t>(word >> 16));
    rom.push_back(static_cast<uint8_t>(word >> 8));
    rom.push_back(static_cast<uint8_t>(word));
}

struct ToyFunc {
    N64Recomp::Context context;
    std::vector<uint32_t> words;
};

void finish_toy(ToyFunc& toy, const std::vector<uint32_t>& mips_words, const std::vector<uint32_t>& extra_rom_words) {
    toy.context.rom.clear();
    toy.words.clear();
    for (uint32_t w : mips_words) {
        append_be(toy.context.rom, w);
        toy.words.push_back(byteswap(w));
    }
    for (uint32_t w : extra_rom_words) {
        append_be(toy.context.rom, w);
    }
    toy.context.sections.resize(1);
    toy.context.sections[0].rom_addr = 0;
    toy.context.sections[0].ram_addr = kVram;
    toy.context.sections[0].size = static_cast<uint32_t>(toy.context.rom.size());
    toy.context.sections[0].executable = true;
    toy.context.sections[0].relocatable = false;
    toy.context.section_functions.resize(1);
    toy.context.functions_by_vram[kVram].emplace_back(0);
    toy.context.sections[0].function_addrs.emplace_back(kVram);
    toy.context.functions.emplace_back(kVram, 0, toy.words, "test_func", 0);
}

int count_substr(std::string_view hay, std::string_view needle) {
    int n = 0;
    for (size_t pos = 0; (pos = hay.find(needle, pos)) != std::string_view::npos; pos += needle.size()) {
        ++n;
    }
    return n;
}

bool recompile_static(ToyFunc& toy, std::string& out, std::vector<std::vector<uint32_t>>* static_funcs_out = nullptr) {
    std::ostringstream oss;
    std::vector<std::vector<uint32_t>> local_statics(toy.context.sections.size());
    auto& statics_ref = static_funcs_out ? *static_funcs_out : local_statics;
    if (statics_ref.size() < toy.context.sections.size()) {
        statics_ref.resize(toy.context.sections.size());
    }
    if (!N64Recomp::recompile_function(toy.context, 0, oss, statics_ref, false)) {
        return false;
    }
    out = oss.str();
    return true;
}

struct LiveCounts {
    int hint = 0;
    int service = 0;
    int pause = 0;
    void* last_status = nullptr;
    bool hint_true = true;
};

LiveCounts* g_live = nullptr;

bool live_hint(void) {
    ++g_live->hint;
    return g_live->hint_true;
}

recomp_context* g_hostile_ctx = nullptr;

void live_service(uint8_t* rdram, void* status_reg) {
    (void)rdram;
    ++g_live->service;
    g_live->last_status = status_reg;
}

void live_service_hostile(uint8_t* rdram, void* status_reg) {
    live_service(rdram, status_reg);
    if (rdram != nullptr) {
        rdram[0] ^= 0x5A;
        rdram[1] ^= 0xA5;
    }
    if (g_hostile_ctx != nullptr) {
        g_hostile_ctx->r2 = 0xDEADBEEF;
        g_hostile_ctx->r3 = 0xCAFEBABE;
        g_hostile_ctx->r5 ^= 0xFFFFFFFF;
    }
}

void live_pause(uint8_t* rdram) {
    (void)rdram;
    ++g_live->pause;
}

void live_switch_error(const char* func, uint32_t vram, uint32_t jtbl) {
    std::fprintf(stderr, "switch error %s 0x%08X 0x%08X\n", func, vram, jtbl);
}

bool run_live(ToyFunc& toy, recomp_context* ctx, LiveCounts& counts) {
    g_live = &counts;
    N64Recomp::live_recompiler_init();
    int32_t section_addr = static_cast<int32_t>(kVram);
    N64Recomp::LiveGeneratorInputs inputs{
        .switch_error = live_switch_error,
        .pause_self = live_pause,
        .preemption_hint = live_hint,
        .service_preemption = live_service,
        .local_section_addresses = &section_addr,
    };
    N64Recomp::LiveGenerator generator{1, inputs};
    std::ostringstream dummy_out;
    std::vector<std::vector<uint32_t>> dummy;
    if (!N64Recomp::recompile_function_live(generator, toy.context, 0, dummy_out, dummy, false)) {
        return false;
    }
    N64Recomp::LiveGeneratorOutput output = generator.finish();
    if (!output.good || output.functions.empty() || output.functions[0] == nullptr) {
        return false;
    }
    std::vector<uint8_t> rdram(0x1000, 0);
    output.functions[0](rdram.data(), ctx);
    return true;
}

// bne offset is from the delay-slot PC.
int16_t bne_off(uint32_t branch_vram, uint32_t target_vram) {
    return static_cast<int16_t>((static_cast<int32_t>(target_vram) - static_cast<int32_t>(branch_vram + 4)) / 4);
}

void t11_forward() {
    std::printf("T11-A forward branch: no safepoint\n");
    // beq zero, zero, L ; nop ; addiu ; L: jr ra ; nop
    ToyFunc toy;
    finish_toy(toy, {
        beq(0, 0, bne_off(kVram, kVram + 0x0C)),
        0,
        addiu(kA0, kA0, 1),
        jr(kRa),
        0,
    }, {});
    std::string c;
    CHECK(recompile_static(toy, c));
    CHECK(count_substr(c, "ultramodern_preemption_hint") == 0);
    CHECK(count_substr(c, "goto L_") >= 1);

    recomp_context ctx{};
    ctx.r4 = 0;
    LiveCounts counts{};
    CHECK(run_live(toy, &ctx, counts));
    CHECK(counts.service == 0);
    CHECK(counts.hint == 0);
}

void t11_backedge_and_delay() {
    std::printf("T11-B/D backedge after delay slot\n");
    // L: addiu a0, a0, -1 ; bne a0, zero, L ; addiu a0, a0, 0 ; jr ra ; nop
    // Delay slot is a distinctive addiu immediate 0x1234 so we can order-check.
    const uint32_t delay = addiu(kT0, kT0, 0x1234);
    ToyFunc toy;
    finish_toy(toy, {
        addiu(kA0, kA0, -1),
        bne(kA0, 0, bne_off(kVram + 4, kVram)),
        delay,
        jr(kRa),
        0,
    }, {});
    std::string c;
    CHECK(recompile_static(toy, c));
    CHECK(count_substr(c, "ultramodern_preemption_hint") == 1);
    CHECK(c.find("(void*)&ctx->status_reg") != std::string::npos);
    const size_t delay_pos = c.find("0x1234");
    const size_t hint_pos = c.find("ultramodern_preemption_hint");
    const size_t goto_pos = c.find("goto L_80001000");
    CHECK(delay_pos != std::string::npos);
    CHECK(hint_pos != std::string::npos);
    CHECK(goto_pos != std::string::npos);
    CHECK(delay_pos < hint_pos);
    CHECK(hint_pos < goto_pos);

    recomp_context ctx{};
    ctx.r4 = 3;
    ctx.status_reg = 0xA5A5A5A5u;
    LiveCounts counts{};
    CHECK(run_live(toy, &ctx, counts));
    CHECK(counts.service == 2);
    CHECK(counts.hint == 2);
    CHECK(counts.last_status == &ctx.status_reg);
}

void t11_self_loop() {
    std::printf("T11-C self-loop pause_self, no extra safepoint\n");
    ToyFunc toy;
    finish_toy(toy, {
        j(kVram),
        0,
        jr(kRa),
        0,
    }, {});
    std::string c;
    CHECK(recompile_static(toy, c));
    CHECK(count_substr(c, "pause_self") == 1);
    CHECK(count_substr(c, "ultramodern_preemption_hint") == 0);

    recomp_context ctx{};
    LiveCounts counts{};
    CHECK(run_live(toy, &ctx, counts));
    CHECK(counts.pause == 1);
    CHECK(counts.service == 0);
    CHECK(counts.hint == 0);
}

void t11_switch() {
    std::printf("T11-D jump table: safepoint before switch\n");
    const uint32_t case0 = kVram + 0x14;
    const uint32_t case1 = kVram + 0x1C;
    const uint32_t jtbl = kVram + 0x30;
    ToyFunc toy;
    finish_toy(toy, {
        lui(kT0, 0x8000),
        addu(kT0, kT0, kT1),
        lw(kT2, kT0, static_cast<int16_t>(jtbl & 0xFFFF)),
        jr(kT2),
        0,
        jr(kRa),
        0,
        jr(kRa),
        0,
    }, {
        0, 0, 0, // pad 0x24, 0x28, 0x2C
        case0, case1, 0,
    });
    std::string c;
    CHECK(recompile_static(toy, c));
    CHECK(count_substr(c, "ultramodern_preemption_hint") == 1);
    const size_t hint_pos = c.find("ultramodern_preemption_hint");
    const size_t switch_pos = c.find("switch (");
    CHECK(hint_pos != std::string::npos);
    CHECK(switch_pos != std::string::npos);
    CHECK(hint_pos < switch_pos);

    recomp_context ctx{};
    ctx.r9 = 0; // t1 addend = 0 -> case 0
    ctx.status_reg = 0x11223344u;
    LiveCounts counts{};
    CHECK(run_live(toy, &ctx, counts));
    CHECK(counts.service == 1);
    CHECK(counts.hint == 1);
    CHECK(counts.last_status == &ctx.status_reg);
}

void t11_flag_off_live() {
    std::printf("T11-G flag/hint false: no service\n");
    ToyFunc toy;
    finish_toy(toy, {
        addiu(kA0, kA0, -1),
        bne(kA0, 0, bne_off(kVram + 4, kVram)),
        0,
        jr(kRa),
        0,
    }, {});
    recomp_context ctx{};
    ctx.r4 = 3;
    LiveCounts counts{};
    counts.hint_true = false;
    CHECK(run_live(toy, &ctx, counts));
    CHECK(counts.hint == 2);
    CHECK(counts.service == 0);
}

void p3_x1_likely_taken() {
    std::printf("P3-X1 branch-likely TAKEN: delay then safepoint\n");
    const uint32_t delay = addiu(kT0, kT0, 0x1234);
    ToyFunc toy;
    finish_toy(toy, {
        addiu(kA0, kA0, -1),
        bnel(kA0, 0, bne_off(kVram + 4, kVram)),
        delay,
        jr(kRa),
        0,
    }, {});
    std::string c;
    CHECK(recompile_static(toy, c));
    CHECK(count_substr(c, "ultramodern_preemption_hint") >= 1);
    const size_t delay_pos = c.find("0x1234");
    const size_t hint_pos = c.find("ultramodern_preemption_hint");
    CHECK(delay_pos != std::string::npos);
    CHECK(hint_pos != std::string::npos);
    CHECK(delay_pos < hint_pos);

    recomp_context ctx{};
    ctx.r4 = 3;
    ctx.r8 = 0;
    ctx.status_reg = 0xA5A5A5A5u;
    LiveCounts counts{};
    CHECK(run_live(toy, &ctx, counts));
    CHECK(counts.service == 2);
    CHECK(ctx.r8 == 0x1234u * 2);
}

void p3_x2_likely_not_taken() {
    std::printf("P3-X2 branch-likely NOT taken: delay annulled\n");
    // beql zero, a0, back ; addiu t0, 0x1234 ; addiu t0, 1 ; jr ra
    // a0 != 0 => not taken, delay annulled, t0 becomes 1.
    ToyFunc toy;
    finish_toy(toy, {
        beql(0, kA0, bne_off(kVram, kVram)),
        addiu(kT0, kT0, 0x1234),
        addiu(kT0, kT0, 1),
        jr(kRa),
        0,
    }, {});
    std::string c;
    CHECK(recompile_static(toy, c));

    recomp_context ctx{};
    ctx.r4 = 1;
    ctx.r8 = 0;
    LiveCounts counts{};
    CHECK(run_live(toy, &ctx, counts));
    CHECK(ctx.r8 == 1);
    CHECK(counts.service == 0);
}

void p3_x4_switch_hostile() {
    std::printf("P3-X4 switch survives hostile service\n");
    const uint32_t case0 = kVram + 0x14;
    const uint32_t case1 = kVram + 0x20;
    const uint32_t jtbl = kVram + 0x34;
    ToyFunc toy;
    finish_toy(toy, {
        lui(kT0, 0x8000),
        addu(kT0, kT0, kT1),
        lw(kT2, kT0, static_cast<int16_t>(jtbl & 0xFFFF)),
        jr(kT2),
        0,
        addiu(kA0, kA0, 0x10), // case 0
        jr(kRa),
        0,
        addiu(kA0, kA0, 0x20), // case 1
        jr(kRa),
        0,
    }, {
        0, 0, // pad 0x30, 0x34
        case0, case1, 0,
    });

    std::string c;
    CHECK(recompile_static(toy, c));
    const size_t hint_pos = c.find("ultramodern_preemption_hint");
    const size_t switch_pos = c.find("switch (");
    CHECK(hint_pos != std::string::npos);
    CHECK(switch_pos != std::string::npos);
    CHECK(hint_pos < switch_pos);

    recomp_context ctx{};
    ctx.r4 = 0;
    ctx.r9 = 0; // case 0
    ctx.status_reg = 0x11223344u;
    LiveCounts counts{};
    g_live = &counts;
    g_hostile_ctx = &ctx;
    N64Recomp::live_recompiler_init();
    int32_t section_addr = static_cast<int32_t>(kVram);
    N64Recomp::LiveGeneratorInputs inputs{
        .switch_error = live_switch_error,
        .pause_self = live_pause,
        .preemption_hint = live_hint,
        .service_preemption = live_service_hostile,
        .local_section_addresses = &section_addr,
    };
    N64Recomp::LiveGenerator generator{1, inputs};
    std::ostringstream dummy_out;
    std::vector<std::vector<uint32_t>> dummy;
    CHECK(N64Recomp::recompile_function_live(generator, toy.context, 0, dummy_out, dummy, false));
    N64Recomp::LiveGeneratorOutput output = generator.finish();
    CHECK(output.good && !output.functions.empty() && output.functions[0] != nullptr);
    std::vector<uint8_t> rdram(0x1000, 0);
    output.functions[0](rdram.data(), &ctx);
    CHECK(counts.service == 1);
    CHECK(ctx.r4 == 0x10);
    CHECK(counts.last_status == &ctx.status_reg);

    ctx.r4 = 0;
    ctx.r9 = 4; // case 1 (word index)
    counts = {};
    g_live = &counts;
    output.functions[0](rdram.data(), &ctx);
    CHECK(ctx.r4 == 0x20);
    g_hostile_ctx = nullptr;
}

void t11_static_c_compiles() {
    std::printf("T11 static generated C mentions void* status_reg ABI\n");
    ToyFunc toy;
    finish_toy(toy, {
        addiu(kA0, kA0, -1),
        bne(kA0, 0, bne_off(kVram + 4, kVram)),
        0,
        jr(kRa),
        0,
    }, {});
    std::string c;
    CHECK(recompile_static(toy, c));
    CHECK(c.find("ultramodern_service_preemption(rdram, (void*)&ctx->status_reg)") != std::string::npos);
}

void test_jal_resolution_policy() {
    std::printf("JAL-1: relocatable same-section known target -> Match/direct\n");
    {
        ToyFunc toy;
        toy.context.rom.resize(0x2000, 0);
        toy.context.use_lookup_for_all_function_calls = true;
        toy.context.sections.resize(1);
        toy.context.sections[0].rom_addr = 0;
        toy.context.sections[0].ram_addr = 0x80800000;
        toy.context.sections[0].size = 0x1000;
        toy.context.sections[0].executable = true;
        toy.context.sections[0].relocatable = true;
        toy.context.section_functions.resize(1);

        std::vector<uint32_t> caller_words = {
            jal(0x80800400),
            0,
            jr(kRa),
            0
        };
        for (auto& w : caller_words) w = byteswap(w);
        toy.context.functions.emplace_back(0x80800000, 0, caller_words, "caller_func", 0);
        toy.context.functions_by_vram[0x80800000].push_back(0);
        toy.context.sections[0].function_addrs.push_back(0x80800000);

        std::vector<uint32_t> callee_words = {
            jr(kRa),
            0
        };
        for (auto& w : callee_words) w = byteswap(w);
        toy.context.functions.emplace_back(0x80800400, 0x400, callee_words, "callee_known_func", 0);
        toy.context.functions_by_vram[0x80800400].push_back(1);
        toy.context.sections[0].function_addrs.push_back(0x80800400);

        std::string c;
        CHECK(recompile_static(toy, c));
        CHECK(c.find("callee_known_func(rdram, ctx);") != std::string::npos);
        CHECK(c.find("LOOKUP_FUNC(0x80800400)") == std::string::npos);
    }

    std::printf("JAL-2: relocatable same-section unnamed target -> CreateStatic\n");
    {
        ToyFunc toy;
        toy.context.rom.resize(0x2000, 0);
        toy.context.use_lookup_for_all_function_calls = true;
        toy.context.sections.resize(2);
        // Section 0: current relocatable overlay (.bsrest)
        toy.context.sections[0].rom_addr = 0;
        toy.context.sections[0].ram_addr = 0x80800000;
        toy.context.sections[0].size = 0x1000;
        toy.context.sections[0].executable = true;
        toy.context.sections[0].relocatable = true;
        // Section 1: colliding overlay (.baflameline)
        toy.context.sections[1].rom_addr = 0x1000;
        toy.context.sections[1].ram_addr = 0x80800000;
        toy.context.sections[1].size = 0x1000;
        toy.context.sections[1].executable = true;
        toy.context.sections[1].relocatable = true;
        toy.context.section_functions.resize(2);

        // Caller at 0x80800000 in section 0
        std::vector<uint32_t> caller_words = {
            jal(0x808003FC),
            0,
            jr(kRa),
            0
        };
        for (auto& w : caller_words) w = byteswap(w);
        toy.context.functions.emplace_back(0x80800000, 0, caller_words, "bsrest_caller", 0);
        toy.context.functions_by_vram[0x80800000].push_back(0);
        toy.context.sections[0].function_addrs.push_back(0x80800000);

        // Named function at 0x808003FC in SECTION 1 (different overlay)
        std::vector<uint32_t> other_ovl_words = {
            jr(kRa),
            0
        };
        for (auto& w : other_ovl_words) w = byteswap(w);
        toy.context.functions.emplace_back(0x808003FC, 0x13FC, other_ovl_words, "baflameline_ep5", 1);
        toy.context.functions_by_vram[0x808003FC].push_back(1);
        toy.context.sections[1].function_addrs.push_back(0x808003FC);

        std::string c;
        std::vector<std::vector<uint32_t>> statics;
        CHECK(recompile_static(toy, c, &statics));
        CHECK(c.find("static_0_808003FC(rdram, ctx);") != std::string::npos);
        CHECK(c.find("LOOKUP_FUNC(0x808003FC)") == std::string::npos);
        CHECK(c.find("baflameline_ep5") == std::string::npos);
        CHECK(!statics.empty() && statics[0].size() == 1 && statics[0][0] == 0x808003FC);
    }

    std::printf("JAL-3: relocatable cross-section target -> lookup fallback\n");
    {
        ToyFunc toy;
        toy.context.rom.resize(0x2000, 0);
        toy.context.use_lookup_for_all_function_calls = true;
        toy.context.sections.resize(2);
        // Section 0: relocatable overlay
        toy.context.sections[0].rom_addr = 0;
        toy.context.sections[0].ram_addr = 0x80800000;
        toy.context.sections[0].size = 0x1000;
        toy.context.sections[0].executable = true;
        toy.context.sections[0].relocatable = true;
        // Section 1: core
        toy.context.sections[1].rom_addr = 0x1000;
        toy.context.sections[1].ram_addr = 0x800A0000;
        toy.context.sections[1].size = 0x1000;
        toy.context.sections[1].executable = true;
        toy.context.sections[1].relocatable = false;
        toy.context.section_functions.resize(2);

        // Caller at 0x80800000 in section 0 calls cross-section 0x800A0C20
        std::vector<uint32_t> caller_words = {
            jal(0x800A0C20),
            0,
            jr(kRa),
            0
        };
        for (auto& w : caller_words) w = byteswap(w);
        toy.context.functions.emplace_back(0x80800000, 0, caller_words, "ovl_caller", 0);
        toy.context.functions_by_vram[0x80800000].push_back(0);
        toy.context.sections[0].function_addrs.push_back(0x80800000);

        std::vector<uint32_t> core_words = {
            jr(kRa),
            0
        };
        for (auto& w : core_words) w = byteswap(w);
        toy.context.functions.emplace_back(0x800A0C20, 0x1C20, core_words, "func_800A0C20", 1);
        toy.context.functions_by_vram[0x800A0C20].push_back(1);
        toy.context.sections[1].function_addrs.push_back(0x800A0C20);

        std::string c;
        CHECK(recompile_static(toy, c));
        CHECK(c.find("LOOKUP_FUNC(0x800A0C20)(rdram, ctx);") != std::string::npos);
        CHECK(c.find("func_800A0C20(rdram, ctx);") == std::string::npos);
    }

    std::printf("JAL-4: non-relocatable same-address runtime hook scenario -> lookup fallback\n");
    {
        ToyFunc toy;
        toy.context.rom.resize(0x4000, 0);
        toy.context.use_lookup_for_all_function_calls = true;
        toy.context.sections.resize(1);
        // Section 0: non-relocatable boot/core
        toy.context.sections[0].rom_addr = 0;
        toy.context.sections[0].ram_addr = 0x80000000;
        toy.context.sections[0].size = 0x4000;
        toy.context.sections[0].executable = true;
        toy.context.sections[0].relocatable = false;
        toy.context.section_functions.resize(1);

        // Caller at 0x80001000 calls target in same non-relocatable section at 0x80002000
        std::vector<uint32_t> caller_words = {
            jal(0x80002000),
            0,
            jr(kRa),
            0
        };
        for (auto& w : caller_words) w = byteswap(w);
        toy.context.functions.emplace_back(0x80001000, 0x1000, caller_words, "boot_caller", 0);
        toy.context.functions_by_vram[0x80001000].push_back(0);
        toy.context.sections[0].function_addrs.push_back(0x80001000);

        std::vector<uint32_t> target_words = {
            jr(kRa),
            0
        };
        for (auto& w : target_words) w = byteswap(w);
        toy.context.functions.emplace_back(0x80002000, 0x2000, target_words, "scheduler_target", 0);
        toy.context.functions_by_vram[0x80002000].push_back(1);
        toy.context.sections[0].function_addrs.push_back(0x80002000);

        std::string c;
        CHECK(recompile_static(toy, c));
        CHECK(c.find("LOOKUP_FUNC(0x80002000)(rdram, ctx);") != std::string::npos);
        CHECK(c.find("scheduler_target(rdram, ctx);") == std::string::npos);
    }
}

} // namespace

int main() {
    N64Recomp::live_recompiler_init();
    t11_forward();
    t11_backedge_and_delay();
    t11_self_loop();
    t11_switch();
    t11_flag_off_live();
    t11_static_c_compiles();
    p3_x1_likely_taken();
    p3_x2_likely_not_taken();
    p3_x4_switch_hostile();
    test_jal_resolution_policy();

    std::printf("T11 %s  (pass=%d fail=%d)\n", g_fail == 0 ? "PASS" : "FAIL", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
