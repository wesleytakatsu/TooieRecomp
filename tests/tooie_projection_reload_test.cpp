// Exercise the real native fixed-size cache builder with the production projection hooks.
// Matrix services are synthetic; no assets, renderer, window or game are started.
#include "recomp.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static int checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); std::exit(1); } } while (0)
static gpr guest(uint32_t p) { return int32_t(p); }
static constexpr uint32_t gfx = 0x80010000, commands = 0x80020000, matrices = 0x80030000;
static constexpr uint32_t volume = 0x80040000, cache = 0x801282E0;
static unsigned cache_bytes;
namespace tooie_tintro_projection_policy { void apply(uint8_t*, float[4][4]) {} }
extern "C" {
void func_800E44FC(uint8_t*, recomp_context*);
void func_800E4640(uint8_t*, recomp_context*);
void func_800E4628(uint8_t*, recomp_context*);
void func_800BC948(uint8_t*, recomp_context*);
void func_800BCC90(uint8_t*, recomp_context*);
int32_t* section_addresses = nullptr;
void switch_error(const char*, uint32_t, uint32_t) { CHECK(false); }
void do_break(uint32_t) { CHECK(false); }
void cop0_status_write(recomp_context*, gpr) { CHECK(false); }
gpr cop0_status_read(recomp_context*) { CHECK(false); return 0; }
void recomp_syscall_handler(uint8_t*, recomp_context*, int32_t) { CHECK(false); }
void pause_self(uint8_t*) { CHECK(false); }
bool ultramodern_preemption_hint() { return false; }
void ultramodern_service_preemption(uint8_t*, void*) { CHECK(false); }
}
static void no_op(uint8_t*, recomp_context*) {}
static void zero(uint8_t*, recomp_context* c) { c->r2 = 0; }
static void scale(uint8_t*, recomp_context* c) { c->f0.fl = 1; }
static void projection(uint8_t* rdram, recomp_context* c) {
    for (unsigned i=0; i<16; ++i) MEM_W(i*4,c->r5) = i%5 == 0 ? 0x3f800000 : 0;
    c->r2=42;
}
static void normalize(uint8_t* rdram, recomp_context* c) { MEM_W(0,guest(0x8012D508))=c->r4; }
static void writeback(uint8_t*, recomp_context* c) {
    if (uint32_t(c->r4)>=cache+0xC8 && uint32_t(c->r4)<cache+0x128) cache_bytes=uint32_t(c->r5);
}
extern "C" recomp_func_t* get_function(int32_t address) {
    switch(uint32_t(address)) {
    case 0x800CB124: return projection;
    case 0x800CA6E8: return scale;
    case 0x800E443C: case 0x80018C28: case 0x8002EF3C: return no_op;
    case 0x80018BC4: return zero;
    case 0x800E4640: return func_800E4640;
    case 0x800BCC90: return func_800BCC90;
    case 0x800E42E4: return normalize;
    case 0x8002E660: return writeback;
    case 0x800E46E0: case 0x800E7FF8: return no_op;
    default: std::fprintf(stderr,"Unexpected call %08x\n",uint32_t(address)); CHECK(false); return nullptr;
    }
}
static void run_bank(unsigned bank) {
    std::vector<uint8_t> memory(8*1024*1024, 0); auto* rdram=memory.data();
    recomp_context c{}; c.r29=guest(0x807FF000);
    MEM_W(0,guest(gfx))=commands; MEM_W(4,guest(gfx))=matrices;
    MEM_W(0,guest(0x8012D500))=volume;
    for(unsigned i=0;i<16;++i) MEM_W(0xA4+i*4,guest(volume))=i%5==0 ? 0x3f800000 : 0;
    c.r4=guest(gfx); func_800E44FC(rdram,&c);
    const uint32_t arena_end=MEM_W(4,guest(gfx));
    CHECK(arena_end>=matrices+0x100);
    // Cache constructor writes two reload lists into a 0x30-byte bank.
    const uint32_t start=cache+0xC8+bank*0x30;
    std::memset(rdram+((cache+0xC8)&0x1fffffff),0xA5,0x68);
    MEM_W(bank*4,guest(cache))=0x80050000;
    MEM_W(8,guest(cache))=commands+0x200;
    MEM_B(0xC,guest(cache))=bank; MEM_B(0xD,guest(cache))=1;
    MEM_W(0,guest(gfx))=0x80050008;
    c.r4=guest(gfx); func_800BC948(rdram,&c);
    CHECK(cache_bytes==40);
    for(unsigned i=40;i<56;++i) CHECK(rdram[((start+i)&0x1fffffff)]==0xA5);
    CHECK(uint32_t(MEM_W(0,guest(start)))==0xDE000000);
    const uint32_t reload=uint32_t(MEM_W(4,guest(start)))|0x80000000;
    CHECK(reload>=matrices+0x100 && reload+24<=arena_end);
    CHECK(uint32_t(MEM_W(0,guest(reload)))==0xDA380007);
    CHECK(uint32_t(MEM_W(4,guest(reload)))==(matrices&0x1fffffff));
    CHECK(uint32_t(MEM_W(8,guest(reload)))==0xDA380005);
    CHECK(uint32_t(MEM_W(12,guest(reload)))==((matrices+64)&0x1fffffff));
    CHECK(uint32_t(MEM_W(16,guest(reload)))==0xDF000000);
    CHECK(uint32_t(MEM_W(16,guest(start)))==0xDF000000);
    CHECK(uint32_t(MEM_W(24,guest(start)))==0xDE000000);
    CHECK(uint32_t(MEM_W(28,guest(start)))==(reload&0x1fffffff));
    CHECK(uint32_t(MEM_W(32,guest(start)))==0xDF000000);
    // The segment-8 path remains a single native command.
    MEM_W(0,guest(0x8012D508))=1; MEM_W(0,guest(gfx))=commands;
    c.r4=guest(gfx); func_800E4640(rdram,&c);
    CHECK(uint32_t(MEM_W(0,guest(gfx)))==commands+8);
    CHECK(uint32_t(MEM_W(4,guest(commands)))==0x08000000);
    MEM_W(0,guest(0x8012D508))=0;
    func_800E4628(rdram,&c);
    MEM_W(0,guest(gfx))=commands; c.r4=guest(gfx); func_800E4640(rdram,&c);
    CHECK(uint32_t(MEM_W(0,guest(gfx)))==commands+8);
    CHECK(uint32_t(MEM_W(0,guest(commands)))==0xDA380007);
    CHECK(MEM_W(4,guest(commands))==0);
}
int main() {
    run_bank(0); run_bank(1);
    std::printf("PASS projection reload: %d checks; native cache banks, P/V list, segment 8, reset\n",checks);
}
