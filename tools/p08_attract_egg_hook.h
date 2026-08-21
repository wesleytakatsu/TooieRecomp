#ifndef P08_ATTRACT_EGG_HOOK_H
#define P08_ATTRACT_EGG_HOOK_H

#include <stdio.h>
#include <stdint.h>

/* Temporary ANGRYLION_CONTROL egg/attract syscall probe. Logging only. */

static FILE* s_p08_egg_log = NULL;

static inline uint16_t p08_be16_from_word(uint32_t word, int low_half)
{
    return (uint16_t)(low_half ? (word & 0xFFFFu) : (word >> 16));
}

static inline void p08_attract_egg_on_syscall(struct r4300_core* r4300, uint32_t op)
{
    const uint32_t code = (op >> 6) & 0xFFFFFu;
    const int interesting =
        (code == 126u) || (code >= 534u && code <= 540u) || (code == 552u) ||
        (code == 731u);
    if (!interesting) {
        return;
    }

    const uint32_t pc = r4300->interp_PC.addr;
    uint32_t next = 0;
    (void)r4300_read_aligned_word(r4300, pc + 4u, &next);
    uint32_t ep = 0;
    const uint32_t op1 = (next >> 26) & 0x3Fu;
    const uint32_t rs = (next >> 21) & 0x1Fu;
    const uint32_t rt = (next >> 16) & 0x1Fu;
    const uint32_t imm = next & 0xFFFFu;
    if ((op1 == 8u || op1 == 0x0Eu) && rs == 0u && rt == 8u && (imm & 3u) == 0u) {
        ep = imm >> 2;
    }

    const int64_t* regs = r4300_regs(r4300);
    const uint32_t a0 = (uint32_t)regs[4];
    const uint32_t a1 = (uint32_t)regs[5];

    uint32_t w_mode = 0, w_map = 0, w_stg = 0;
    (void)r4300_read_aligned_word(r4300, 0x80127630u, &w_mode);
    (void)r4300_read_aligned_word(r4300, 0x80127640u, &w_map);
    (void)r4300_read_aligned_word(r4300, 0x80135470u, &w_stg);
    const int mode = (int)(int16_t)p08_be16_from_word(w_mode, 1);
    const int map = (int)(int16_t)p08_be16_from_word(w_map, 0);
    const int stage = (int)((w_stg >> 24) & 0xFFu);
    const int sub = (int)((w_stg >> 16) & 0xFFu);

    if (!s_p08_egg_log) {
        s_p08_egg_log = fopen("/tmp/p08-angrylion-attract-eggs.log", "a");
    }
    if (!s_p08_egg_log) {
        return;
    }
    fprintf(s_p08_egg_log,
            "[egg-sys] pc=%08x ovl=%u ep=%u a0=%08x a1=%08x mode=%d map=%d stage=%d sub=%d\n",
            pc, code, ep, a0, a1, mode, map, stage, sub);
    fflush(s_p08_egg_log);
}

#endif
