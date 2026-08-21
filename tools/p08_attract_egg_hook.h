#ifndef P08_ATTRACT_EGG_HOOK_H
#define P08_ATTRACT_EGG_HOOK_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Temporary ANGRYLION_CONTROL USER_DEMO_4 egg + follow-camera probe. Logging only. */

static FILE* s_p08_egg_log = NULL;
static FILE* s_p08_motion = NULL;
static int s_p08_logged_ep5 = 0;
static uint32_t s_p08_last_post = 0xFFFFFFFFu;
static int s_p08_last_sub = -1;

static inline uint16_t p08_be16_from_word(uint32_t word, int low_half)
{
    return (uint16_t)(low_half ? (word & 0xFFFFu) : (word >> 16));
}

static inline float p08_f32(uint32_t word)
{
    float f = 0.0f;
    memcpy(&f, &word, sizeof(f));
    return f;
}

static inline FILE* p08_egg_fp(void)
{
    if (!s_p08_egg_log) {
        s_p08_egg_log = fopen("/tmp/p08-angrylion-attract-eggs.log", "a");
    }
    return s_p08_egg_log;
}

static inline void p08_read_attract(struct r4300_core* r4300, int* mode, int* map,
                                    int* stage, int* sub, uint32_t* post)
{
    uint32_t w_mode = 0, w_map = 0, w_stg = 0, w_post = 0;
    (void)r4300_read_aligned_word(r4300, 0x80127630u, &w_mode);
    (void)r4300_read_aligned_word(r4300, 0x80127640u, &w_map);
    (void)r4300_read_aligned_word(r4300, 0x80135470u, &w_stg);
    (void)r4300_read_aligned_word(r4300, 0x8012D5B8u, &w_post);
    *mode = (int)(int16_t)p08_be16_from_word(w_mode, 1);
    *map = (int)(int16_t)p08_be16_from_word(w_map, 0);
    *stage = (int)((w_stg >> 24) & 0xFFu);
    *sub = (int)((w_stg >> 16) & 0xFFu);
    *post = w_post;
}

static FILE* s_p08_ud4 = NULL;
static uint32_t s_p08_ud4_seq = 0;
static uint32_t s_p08_ca628_dst = 0;
static uint32_t s_p08_ca8b4_a0 = 0;
static uint32_t s_p08_ca8b4_a1 = 0;
static uint32_t s_p08_ca8b4_a2 = 0;
static int s_p08_ca8b4_glidx = -1;
static int s_p08_ca8b4_ord = 0;
static int s_p08_ca8b4_last_glidx = -1;
static int s_p08_ca8b4_n = 0;
static int s_p08_ncbapivot_n = 0;
static int s_p08_np_trace = 0;
static int s_p08_np_glidx = -1;
static uint32_t s_p08_np_node = 0;
static uint32_t s_p08_np_ret = 0;
static uint32_t s_p08_np_child_ra[8];
static uint32_t s_p08_np_child_pc[8];
static const char* s_p08_np_child_name[8];
static int s_p08_np_child_sp = 0;
static int s_p08_np_call_n = 0;
static int s_p08_np_insn = 0;
static uint32_t s_p08_np_ovl_base = 0;
static int s_p08_np_last_fpu = 0;
static int s_p08_np_last_fd = 0;
static uint32_t s_p08_np_last_fpu_pc = 0;
static uint32_t s_p08_np_last_fpu_op = 0;

static inline FILE* p08_ud4_fp(void)
{
    if (!s_p08_ud4) {
        s_p08_ud4 = fopen("/tmp/p08-angrylion-ud4-cam.log", "a");
    }
    return s_p08_ud4;
}

static inline int p08_ud4_window(struct r4300_core* r4300, int* glidx_out)
{
    int mode = 0, map = 0, stage = 0, sub = 0;
    uint32_t post = 0;
    p08_read_attract(r4300, &mode, &map, &stage, &sub, &post);
    const int glidx = (post > 0u) ? (int)post - 1 : 0;
    if (glidx_out) {
        *glidx_out = glidx;
    }
    return (mode == 5 && sub == 4 && glidx >= 0 && glidx <= 42);
}

static inline void p08_ud4_vec(struct r4300_core* r4300, uint32_t addr, float* x, float* y, float* z, uint32_t* zbits)
{
    uint32_t wx = 0, wy = 0, wz = 0;
    if ((addr >= 0x80000000u) && ((addr + 0x80000000u) < 0x00800000u) && ((addr & 3u) == 0u)) {
        (void)r4300_read_aligned_word(r4300, addr + 0u, &wx);
        (void)r4300_read_aligned_word(r4300, addr + 4u, &wy);
        (void)r4300_read_aligned_word(r4300, addr + 8u, &wz);
    }
    *x = p08_f32(wx); *y = p08_f32(wy); *z = p08_f32(wz);
    if (zbits) {
        *zbits = wz;
    }
}

static inline int p08_guest(uint32_t a)
{
    return (a >= 0x80000000u) && ((a + 0x80000000u) < 0x00800000u) && ((a & 3u) == 0u);
}

static inline void p08_ud4_dump_reads(struct r4300_core* r4300, FILE* fp, uint32_t rec)
{
    uint32_t e0 = 0, e1 = 0, e2 = 0, s0 = 0, s1 = 0, s2 = 0;
    if (p08_guest(rec)) {
        (void)r4300_read_aligned_word(r4300, rec + 0x00u, &e0);
        (void)r4300_read_aligned_word(r4300, rec + 0x04u, &e1);
        (void)r4300_read_aligned_word(r4300, rec + 0x08u, &e2);
        (void)r4300_read_aligned_word(r4300, rec + 0x18u, &s0);
        (void)r4300_read_aligned_word(r4300, rec + 0x1Cu, &s1);
        (void)r4300_read_aligned_word(r4300, rec + 0x20u, &s2);
    }
    fprintf(fp,
            " rec=%08x eye_in=%08x,%08x,%08x src2_in=%08x,%08x,%08x "
            "eye=(%.6f,%.6f,%.6f) src2=(%.6f,%.6f,%.6f)",
            rec, e0, e1, e2, s0, s1, s2,
            p08_f32(e0), p08_f32(e1), p08_f32(e2), p08_f32(s0), p08_f32(s1), p08_f32(s2));
}

static inline uint32_t p08_ud4_sel_rec(struct r4300_core* r4300)
{
    uint32_t ps = 0, selw = 0, rec = 0;
    (void)r4300_read_aligned_word(r4300, 0x80135490u, &ps);
    if (!p08_guest(ps)) {
        return 0;
    }
    (void)r4300_read_aligned_word(r4300, ps + 0x184u, &selw);
    if ((int32_t)selw < 0 || (int32_t)selw >= 8) {
        return 0;
    }
    (void)r4300_read_aligned_word(r4300, 0x80127708u + selw * 4u, &rec);
    return p08_guest(rec) ? rec : 0;
}

static inline void p08_ud4_dump_player(struct r4300_core* r4300, FILE* fp)
{
    uint32_t ps = 0, posp = 0, yawp = 0, selw = 0;
    uint32_t pxb = 0, pyb = 0, pzb = 0, yawb = 0, idb = 0;
    (void)r4300_read_aligned_word(r4300, 0x80135490u, &ps);
    if (!p08_guest(ps)) {
        fprintf(fp, " player=none");
        return;
    }
    (void)r4300_read_aligned_word(r4300, ps + 0xE4u, &posp);
    (void)r4300_read_aligned_word(r4300, ps + 0xF8u, &yawp);
    (void)r4300_read_aligned_word(r4300, ps + 0x184u, &selw);
    if (p08_guest(posp)) {
        (void)r4300_read_aligned_word(r4300, posp + 0u, &pxb);
        (void)r4300_read_aligned_word(r4300, posp + 4u, &pyb);
        (void)r4300_read_aligned_word(r4300, posp + 8u, &pzb);
    }
    if (p08_guest(yawp)) {
        (void)r4300_read_aligned_word(r4300, yawp + 0u, &yawb);
        (void)r4300_read_aligned_word(r4300, yawp + 4u, &idb);
    }
    fprintf(fp,
            " sel=%d pos=(%.6f,%.6f,%.6f) pos_bits=%08x,%08x,%08x "
            "yaw=%.6f ideal=%.6f yaw_bits=%08x ideal_bits=%08x",
            (int32_t)selw, p08_f32(pxb), p08_f32(pyb), p08_f32(pzb), pxb, pyb, pzb,
            p08_f32(yawb), p08_f32(idb), yawb, idb);
}

static inline uint32_t p08_fpr_bits(struct r4300_core* r4300, int i)
{
    uint32_t w = 0;
    if (r4300->cp1.regs_simple[i] != NULL) {
        memcpy(&w, r4300->cp1.regs_simple[i], 4);
    }
    return w;
}

static inline void p08_ud4_dump_vec3(struct r4300_core* r4300, FILE* fp, const char* tag, uint32_t addr)
{
    uint32_t b0 = 0, b1 = 0, b2 = 0;
    if (!p08_guest(addr)) {
        fprintf(fp, " %s=none", tag);
        return;
    }
    (void)r4300_read_aligned_word(r4300, addr + 0u, &b0);
    (void)r4300_read_aligned_word(r4300, addr + 4u, &b1);
    (void)r4300_read_aligned_word(r4300, addr + 8u, &b2);
    fprintf(fp, " %s=%08x,%08x,%08x", tag, b0, b1, b2);
}

static inline void p08_ud4_dump_node(struct r4300_core* r4300, FILE* fp, uint32_t node)
{
    uint32_t p4 = 0, p28 = 0, p30 = 0, p5c = 0, p5c0 = 0, w2e = 0;
    if (!p08_guest(node)) {
        fprintf(fp, " node=none");
        return;
    }
    (void)r4300_read_aligned_word(r4300, node + 0x4u, &p4);
    (void)r4300_read_aligned_word(r4300, node + 0x28u, &p28);
    (void)r4300_read_aligned_word(r4300, node + 0x30u, &p30);
    (void)r4300_read_aligned_word(r4300, node + 0x5Cu, &p5c);
    if (p08_guest(p5c)) {
        (void)r4300_read_aligned_word(r4300, p5c, &p5c0);
    }
    if (p08_guest(p4)) {
        (void)r4300_read_aligned_word(r4300, p4 + 0x2Cu, &w2e);
    }
    fprintf(fp, " node=%08x +4=%08x +28=%08x +30=%08x +5c=%08x *5c=%08x +4.2e=%04x",
            node, p4, p28, p30, p5c, p5c0, (unsigned)((w2e >> 16) & 0xFFFFu));
    p08_ud4_dump_vec3(r4300, fp, "n74", node + 0x74u);
    p08_ud4_dump_vec3(r4300, fp, "n80", node + 0x80u);
    if (p08_guest(p30)) {
        p08_ud4_dump_vec3(r4300, fp, "tgt", p30 + 0x4u);
    }
}

static inline const char* p08_np_child_name(uint32_t pc)
{
    switch (pc) {
    case 0x8011607Cu: return "8011607C";
    case 0x80113978u: return "80113978";
    case 0x80112524u: return "80112524";
    case 0x801107F0u: return "801107F0";
    case 0x800EE7F8u: return "800EE7F8";
    case 0x800EFB24u: return "800EFB24";
    case 0x800EEF94u: return "800EEF94";
    case 0x800F1A88u: return "800F1A88";
    case 0x80115358u: return "80115358";
    case 0x80115750u: return "80115750";
    case 0x80112C3Cu: return "80112C3C";
    case 0x801125E8u: return "801125E8";
    case 0x800EEBF0u: return "800EEBF0";
    case 0x800EE780u: return "800EE780";
    case 0x80111FD4u: return "80111FD4";
    case 0x801138A0u: return "801138A0";
    case 0x801138F4u: return "801138F4";
    case 0x80110F44u: return "80110F44";
    case 0x80115580u: return "80115580";
    case 0x80110A68u: return "80110A68";
    case 0x80110790u: return "80110790";
    case 0x80111310u: return "80111310";
    case 0x800A516Cu: return "800A516C";
    case 0x8002DCA0u: return "sqrtf";
    default: return NULL;
    }
}

static inline void p08_ud4_np_call_line(struct r4300_core* r4300, FILE* fp, const char* phase,
                                       const char* name, uint32_t pc)
{
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t a0 = (uint32_t)regs[4];
    const uint32_t a1 = (uint32_t)regs[5];
    const uint32_t a2 = (uint32_t)regs[6];
    const uint32_t a3 = (uint32_t)regs[7];
    fprintf(fp,
            "[ud4-np-call] %s n=%d name=%s pc=%08x glidx=%d ra=%08x "
            "a0=%08x a1=%08x a2=%08x a3=%08x v0=%08x "
            "f0=%08x f2=%08x f12=%08x f14=%08x fcr31=%08x",
            phase, s_p08_np_call_n, name, pc, s_p08_np_glidx, (uint32_t)regs[31],
            a0, a1, a2, a3, (uint32_t)regs[2],
            p08_fpr_bits(r4300, 0), p08_fpr_bits(r4300, 2),
            p08_fpr_bits(r4300, 12), p08_fpr_bits(r4300, 14),
            r4300->cp1.fcr31);
    p08_ud4_dump_vec3(r4300, fp, "va0", a0);
    p08_ud4_dump_vec3(r4300, fp, "va1", a1);
    p08_ud4_dump_vec3(r4300, fp, "va2", a2);
    p08_ud4_dump_node(r4300, fp, s_p08_np_node);
    p08_ud4_dump_reads(r4300, fp, p08_ud4_sel_rec(r4300));
    fprintf(fp, "\n");
}

static inline void p08_ud4_np_tick(struct r4300_core* r4300, uint32_t pc, uint32_t op)
{
    if (!s_p08_np_trace) {
        return;
    }
    FILE* fp = p08_ud4_fp();
    if (!fp) {
        return;
    }
    const int64_t* regs = r4300_regs(r4300);
    s_p08_np_insn += 1;

    if (s_p08_np_last_fpu) {
        fprintf(fp,
                "[ud4-np-fpu] after pc=%08x op=%08x fd=%d bits=%08x fcr31=%08x\n",
                s_p08_np_last_fpu_pc, s_p08_np_last_fpu_op, s_p08_np_last_fd,
                p08_fpr_bits(r4300, s_p08_np_last_fd), r4300->cp1.fcr31);
        s_p08_np_last_fpu = 0;
    }

    if (pc == s_p08_np_ret) {
        fprintf(fp,
                "[ud4-ncbapivot] leave seq=%u glidx=%d active=1 v0=%08x f0=%08x "
                "fcr31=%08x insn=%d ovl_base=%08x",
                ++s_p08_ud4_seq, s_p08_np_glidx, (uint32_t)regs[2],
                p08_fpr_bits(r4300, 0), r4300->cp1.fcr31, s_p08_np_insn,
                s_p08_np_ovl_base);
        p08_ud4_dump_node(r4300, fp, s_p08_np_node);
        p08_ud4_dump_reads(r4300, fp, p08_ud4_sel_rec(r4300));
        p08_ud4_dump_player(r4300, fp);
        fprintf(fp, "\n[ud4-ncbapivot] FIRST_TICK_STOP after glidx=%d\n", s_p08_np_glidx);
        fflush(fp);
        s_p08_np_trace = 0;
        exit(0);
    }

    if (s_p08_np_child_sp > 0 && pc == s_p08_np_child_ra[s_p08_np_child_sp - 1]) {
        const int top = s_p08_np_child_sp - 1;
        p08_ud4_np_call_line(r4300, fp, "leave", s_p08_np_child_name[top], s_p08_np_child_pc[top]);
        s_p08_np_child_sp = top;
        fflush(fp);
    }

    {
        const char* cname = p08_np_child_name(pc);
        if (cname != NULL && s_p08_np_child_sp < 8) {
            s_p08_np_call_n += 1;
            s_p08_np_child_name[s_p08_np_child_sp] = cname;
            s_p08_np_child_pc[s_p08_np_child_sp] = pc;
            s_p08_np_child_ra[s_p08_np_child_sp] = (uint32_t)regs[31];
            s_p08_np_child_sp += 1;
            p08_ud4_np_call_line(r4300, fp, "enter", cname, pc);
            fflush(fp);
        }
    }

    if (s_p08_np_ovl_base == 0u && pc != 0x80089488u && pc != 0x80800098u &&
        (pc & 0x80000000u) != 0u && pc != s_p08_np_ret) {
        /* First body PC after the resident stub: relocated overlay text. */
        if ((pc & 0xFFF00000u) != 0x80000000u) {
            s_p08_np_ovl_base = pc & ~0xFFu;
            fprintf(fp, "[ud4-np-ovl] first_body_pc=%08x guess_base=%08x\n", pc, s_p08_np_ovl_base);
        }
    }

    if (((op >> 26) & 0x3Fu) == 17u) {
        const int fs = (int)((op >> 11) & 0x1Fu);
        const int ft = (int)((op >> 16) & 0x1Fu);
        const int fd = (int)((op >> 6) & 0x1Fu);
        const int fmt = (int)((op >> 21) & 0x1Fu);
        const int fn = (int)(op & 0x3Fu);
        fprintf(fp,
                "[ud4-np-fpu] before pc=%08x op=%08x fmt=%d fn=%d fs=%d ft=%d fd=%d "
                "fs_bits=%08x ft_bits=%08x fcr31=%08x\n",
                pc, op, fmt, fn, fs, ft, fd,
                p08_fpr_bits(r4300, fs), p08_fpr_bits(r4300, ft), r4300->cp1.fcr31);
        s_p08_np_last_fpu = 1;
        s_p08_np_last_fd = fd;
        s_p08_np_last_fpu_pc = pc;
        s_p08_np_last_fpu_op = op;
        fflush(fp);
    }
}

static inline void p08_ud4_cam(struct r4300_core* r4300, uint32_t pc)
{
    int glidx = 0;
    if (!p08_ud4_window(r4300, &glidx)) {
        return;
    }
    FILE* fp = p08_ud4_fp();
    if (!fp) {
        return;
    }
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t a0 = (uint32_t)regs[4];
    const uint32_t a1 = (uint32_t)regs[5];
    const uint32_t a2 = (uint32_t)regs[6];
    const uint32_t a3 = (uint32_t)regs[7];
    const uint32_t ra = (uint32_t)regs[31];
    const uint32_t seq = ++s_p08_ud4_seq;
    if (pc == 0x800CA628u) {
        float x,y,z,s0,s1,s2; uint32_t zb = 0, zb1 = 0;
        uint32_t w1 = 0;
        s_p08_ca628_dst = a0;
        p08_ud4_vec(r4300, a1, &x, &y, &z, NULL);
        p08_ud4_vec(r4300, a2, &s0, &s1, &s2, &zb);
        (void)r4300_read_aligned_word(r4300, a2 + 4u, &w1);
        zb1 = w1;
        fprintf(fp,
                "[ud4-ca628] enter seq=%u glidx=%d ra=%08x dst=%08x src=%08x src2=%08x "
                "eye=(%.6f,%.6f,%.6f) src2=(%.6f,%.6f,%.6f) src2_1_bits=%08x src2_2_bits=%08x\n",
                seq, glidx, ra, a0, a1, a2, x, y, z, s0, s1, s2, zb1, zb);
    } else if (pc == 0x800CA65Cu) {
        const uint32_t v1 = (uint32_t)regs[3];
        const uint32_t store_addr = v1 + 0x14u;
        fprintf(fp, "[ud4-store] seq=%u glidx=%d pc=800CA65C v1=%08x store=%08x dst=%08x dst_plus_1c=%08x\n",
                seq, glidx, v1, store_addr, s_p08_ca628_dst, s_p08_ca628_dst + 0x1Cu);
    } else if (pc == 0x800CA660u) {
        uint32_t stored = 0;
        if (s_p08_ca628_dst >= 0x80000000u) {
            (void)r4300_read_aligned_word(r4300, s_p08_ca628_dst + 0x1Cu, &stored);
        }
        fprintf(fp, "[ud4-ca628] leave seq=%u glidx=%d dst=%08x stored_1c=%.6f bits=%08x\n",
                seq, glidx, s_p08_ca628_dst, p08_f32(stored), stored);
    } else if (pc == 0x8010FC38u) {
        uint32_t rec = 0;
        if ((a0 >= 0x80000000u) && ((a0 & 3u) == 0u)) {
            (void)r4300_read_aligned_word(r4300, a0, &rec);
        }
        fprintf(fp, "[ud4-fc38] enter seq=%u glidx=%d ra=%08x node=%08x rec=%08x\n",
                seq, glidx, ra, a0, rec);
    } else if (pc == 0x8010FE14u) {
        fprintf(fp, "[ud4-fe14] enter seq=%u glidx=%d ra=%08x a0=%08x\n", seq, glidx, ra, a0);
    } else if (pc == 0x800CA8B4u) {
        if (glidx != s_p08_ca8b4_last_glidx) {
            s_p08_ca8b4_last_glidx = glidx;
            s_p08_ca8b4_ord = 0;
        }
        s_p08_ca8b4_ord += 1;
        s_p08_ca8b4_a0 = a0;
        s_p08_ca8b4_a1 = a1;
        s_p08_ca8b4_a2 = a2;
        s_p08_ca8b4_glidx = glidx;
        {
            const int first = (s_p08_ca8b4_n++ == 0) ? 1 : 0;
            fprintf(fp,
                    "[ud4-ca8b4] enter seq=%u glidx=%d ord=%d first=%d ra=%08x "
                    "a0=%08x a1=%08x a2=%08x a3=%08x",
                    seq, glidx, s_p08_ca8b4_ord, first, ra, a0, a1, a2, a3);
            p08_ud4_dump_reads(r4300, fp, a0);
            p08_ud4_dump_player(r4300, fp);
            fprintf(fp, "\n");
        }
    } else if (pc == 0x800CA8ECu) {
        uint32_t e0 = 0, e1 = 0, e2 = 0, s0 = 0, s1 = 0, s2 = 0;
        if (p08_guest(s_p08_ca8b4_a1)) {
            (void)r4300_read_aligned_word(r4300, s_p08_ca8b4_a1 + 0u, &e0);
            (void)r4300_read_aligned_word(r4300, s_p08_ca8b4_a1 + 4u, &e1);
            (void)r4300_read_aligned_word(r4300, s_p08_ca8b4_a1 + 8u, &e2);
        }
        if (p08_guest(s_p08_ca8b4_a2)) {
            (void)r4300_read_aligned_word(r4300, s_p08_ca8b4_a2 + 0u, &s0);
            (void)r4300_read_aligned_word(r4300, s_p08_ca8b4_a2 + 4u, &s1);
            (void)r4300_read_aligned_word(r4300, s_p08_ca8b4_a2 + 8u, &s2);
        }
        fprintf(fp,
                "[ud4-ca8b4] leave seq=%u glidx=%d ord=%d eye_out=%08x,%08x,%08x "
                "src2_out=%08x,%08x,%08x eye=(%.6f,%.6f,%.6f) src2=(%.6f,%.6f,%.6f)\n",
                seq, s_p08_ca8b4_glidx, s_p08_ca8b4_ord, e0, e1, e2, s0, s1, s2,
                p08_f32(e0), p08_f32(e1), p08_f32(e2), p08_f32(s0), p08_f32(s1), p08_f32(s2));
    } else if (pc == 0x800E1A58u) {
        float x,y,z,s0,s1,s2; uint32_t zb = 0;
        p08_ud4_vec(r4300, a0, &x, &y, &z, NULL);
        p08_ud4_vec(r4300, a1, &s0, &s1, &s2, &zb);
        fprintf(fp,
                "[ud4-e1a58] enter seq=%u glidx=%d ra=%08x a0=%08x a1=%08x "
                "v0=(%.6f,%.6f,%.6f) v1=(%.6f,%.6f,%.6f) v1_2_bits=%08x\n",
                seq, glidx, ra, a0, a1, x, y, z, s0, s1, s2, zb);
    } else if (pc == 0x80089488u || pc == 0x80800098u) {
        const int first = (s_p08_ncbapivot_n++ == 0) ? 1 : 0;
        const uint32_t rec = p08_ud4_sel_rec(r4300);
        s_p08_np_node = a0;
        s_p08_np_glidx = glidx;
        fprintf(fp,
                "[ud4-ncbapivot] enter seq=%u glidx=%d first=%d ra=%08x a0=%08x pc=%08x "
                "a1=%08x a2=%08x a3=%08x f0=%08x f12=%08x fcr31=%08x",
                seq, glidx, first, ra, a0, pc, a1, a2, a3,
                p08_fpr_bits(r4300, 0), p08_fpr_bits(r4300, 12), r4300->cp1.fcr31);
        p08_ud4_dump_node(r4300, fp, a0);
        p08_ud4_dump_reads(r4300, fp, rec);
        p08_ud4_dump_player(r4300, fp);
        fprintf(fp, "\n");
        if (first && glidx == 1 && !s_p08_np_trace) {
            s_p08_np_trace = 1;
            s_p08_np_ret = ra;
            s_p08_np_call_n = 0;
            s_p08_np_insn = 0;
            s_p08_np_ovl_base = 0;
            s_p08_np_child_sp = 0;
            fprintf(fp, "[ud4-ncbapivot] TRACE_ON ret=%08x\n", s_p08_np_ret);
        }
    }
    fflush(fp);
}

static inline void p08_dump_egg_fields(struct r4300_core* r4300, FILE* fp, uint32_t obj)
{
    uint32_t w64 = 0, wtbl = 0, wmid = 0;
    if ((obj >= 0x80000000u) && ((obj + 0x80000000u) < 0x00800000u) && ((obj & 3u) == 0u)) {
        (void)r4300_read_aligned_word(r4300, obj + 0x64u, &w64);
        (void)r4300_read_aligned_word(r4300, obj + 0x6Cu, &wmid);
    }
    (void)r4300_read_aligned_word(r4300, 0x80117E20u, &wtbl);
    fprintf(fp, " obj64=%08x obj6c=%08x egg_tbl=%08x", w64, wmid, wtbl);
}

static inline void p08_baegg_pc(struct r4300_core* r4300, uint32_t pc)
{
    int mode = 0, map = 0, stage = 0, sub = 0;
    uint32_t post = 0;
    p08_read_attract(r4300, &mode, &map, &stage, &sub, &post);
    const int64_t* regs = r4300_regs(r4300);
    FILE* fp = p08_egg_fp();
    if (!fp) {
        return;
    }
    const char* name = (pc == 0x8080042Cu) ? "baeggsetup_ep5" : "baeggsetup_ep8";
    fprintf(fp,
            "[egg-pc] %s pc=%08x a0=%08x a1=%08x ra=%08x mode=%d map=%d stage=%d sub=%d post=%u",
            name, pc, (uint32_t)regs[4], (uint32_t)regs[5], (uint32_t)regs[31],
            mode, map, stage, sub, post);
    p08_dump_egg_fields(r4300, fp, (uint32_t)regs[4]);
    fprintf(fp, "\n");
    fflush(fp);
    if (pc == 0x8080042Cu) {
        s_p08_logged_ep5 = 1;
    }
}

static inline void p08_spawn_dispatch(struct r4300_core* r4300)
{
    int mode = 0, map = 0, stage = 0, sub = 0;
    uint32_t post = 0;
    p08_read_attract(r4300, &mode, &map, &stage, &sub, &post);
    if (mode != 5) {
        return;
    }
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t a0 = (uint32_t)regs[4];
    const uint32_t a1 = (uint32_t)regs[5];
    const uint32_t ra = (uint32_t)regs[31];
    uint32_t act184 = a1, egg_byte = 0, obj70 = 0;
    /* baeggsetup_entrypoint_5 stores a1 (egg enum) at *(s0+0x70)+0x24 */
    FILE* fp = p08_egg_fp();
    if (!fp) {
        return;
    }
    fprintf(fp,
            "[egg-dispatch] pc=80086D30 a0=%08x a1=%08x ra=%08x mode=%d map=%d "
            "stage=%d sub=%d post=%u",
            a0, a1, ra, mode, map, stage, sub, post);
    p08_dump_egg_fields(r4300, fp, a1);
    fprintf(fp, "\n");
    fflush(fp);
    (void)act184; (void)egg_byte; (void)obj70;
}

static inline void p08_motion_on_glrecord(struct r4300_core* r4300)
{
    int mode = 0, map = 0, stage = 0, sub = 0;
    uint32_t post = 0;
    p08_read_attract(r4300, &mode, &map, &stage, &sub, &post);
    if (mode != 5 || (sub != 4 && sub != 0)) {
        return;
    }
    if (sub == 0 && post > 30u) {
        return; /* landmark only */
    }
    if (sub == s_p08_last_sub && post == s_p08_last_post) {
        return;
    }
    s_p08_last_sub = sub;
    s_p08_last_post = post;
    if (!s_p08_motion) {
        s_p08_motion = fopen("/tmp/p08-angrylion-user_demo4.csv", "a");
        if (s_p08_motion && ftell(s_p08_motion) == 0) {
            fprintf(s_p08_motion,
                    "sub,glidx,post,pad_x,pad_y,pad_btn,bs,px,py,pz,vx,vy,vz,yaw,ideal,phys_yaw,"
                    "cx,cy,cz,camctrl,sel\n");
        }
    }
    if (!s_p08_motion) {
        return;
    }

    uint32_t gl_base_w = 0, ps_w = 0, cam_w = 0, rec_w = 0;
    (void)r4300_read_aligned_word(r4300, 0x8012D5B0u, &gl_base_w);
    (void)r4300_read_aligned_word(r4300, 0x80135490u, &ps_w);
    const int32_t glidx = (post > 0u) ? (int32_t)post - 1 : 0;
    int pad_x = 0, pad_y = 0;
    unsigned pad_btn = 0;
    if ((gl_base_w >= 0x80000000u) && ((gl_base_w + 0x80000000u) < 0x00800000u)) {
        uint32_t sample = gl_base_w + (uint32_t)glidx * 6u;
        uint32_t w0 = 0, w1 = 0;
        (void)r4300_read_aligned_word(r4300, sample & ~3u, &w0);
        (void)r4300_read_aligned_word(r4300, (sample + 4u) & ~3u, &w1);
        const int shift = (int)((sample & 3u) * 8);
        /* BE bytes in word */
        uint8_t b[8];
        b[0] = (uint8_t)(w0 >> 24); b[1] = (uint8_t)(w0 >> 16);
        b[2] = (uint8_t)(w0 >> 8);  b[3] = (uint8_t)w0;
        b[4] = (uint8_t)(w1 >> 24); b[5] = (uint8_t)(w1 >> 16);
        b[6] = (uint8_t)(w1 >> 8);  b[7] = (uint8_t)w1;
        const int off = (int)(sample & 3u);
        (void)shift;
        pad_x = (int)(int8_t)b[off];
        pad_y = (int)(int8_t)b[off + 1];
        pad_btn = ((unsigned)b[off + 2] << 8) | (unsigned)b[off + 3];
    }

    float px = 0, py = 0, pz = 0, yaw = 0, ideal = 0, phys = 0, vx = 0, vy = 0, vz = 0;
    float cx = 0, cy = 0, cz = 0, camctrl = 0;
    int32_t bs = -1, sel = -1;
    if ((ps_w >= 0x80000000u) && ((ps_w + 0x80000000u) < 0x00800000u)) {
        uint32_t posp = 0, yawp = 0, physp = 0, bsp = 0, selw = 0;
        (void)r4300_read_aligned_word(r4300, ps_w + 0xE4u, &posp);
        (void)r4300_read_aligned_word(r4300, ps_w + 0xF8u, &yawp);
        (void)r4300_read_aligned_word(r4300, ps_w + 0xC8u, &physp);
        (void)r4300_read_aligned_word(r4300, ps_w + 0x120u, &bsp);
        (void)r4300_read_aligned_word(r4300, ps_w + 0x184u, &selw);
        sel = (int32_t)selw;
        if ((posp >= 0x80000000u)) {
            uint32_t x = 0, y = 0, z = 0;
            (void)r4300_read_aligned_word(r4300, posp + 0, &x);
            (void)r4300_read_aligned_word(r4300, posp + 4, &y);
            (void)r4300_read_aligned_word(r4300, posp + 8, &z);
            px = p08_f32(x); py = p08_f32(y); pz = p08_f32(z);
        }
        if ((yawp >= 0x80000000u)) {
            uint32_t y0 = 0, y1 = 0;
            (void)r4300_read_aligned_word(r4300, yawp + 0, &y0);
            (void)r4300_read_aligned_word(r4300, yawp + 4, &y1);
            yaw = p08_f32(y0); ideal = p08_f32(y1);
        }
        if ((physp >= 0x80000000u)) {
            uint32_t wx, wy, wz, wyaw;
            wx = wy = wz = wyaw = 0;
            (void)r4300_read_aligned_word(r4300, physp + 0x10u, &wx);
            (void)r4300_read_aligned_word(r4300, physp + 0x14u, &wy);
            (void)r4300_read_aligned_word(r4300, physp + 0x18u, &wz);
            (void)r4300_read_aligned_word(r4300, physp + 0x40u, &wyaw);
            vx = p08_f32(wx); vy = p08_f32(wy); vz = p08_f32(wz); phys = p08_f32(wyaw);
        }
        if ((bsp >= 0x80000000u)) {
            uint32_t bsw = 0;
            (void)r4300_read_aligned_word(r4300, bsp, &bsw);
            bs = (int32_t)bsw;
        }
        if (sel >= 0 && sel < 8) {
            uint32_t rec = 0, cw = 0;
            (void)r4300_read_aligned_word(r4300, 0x80127708u + (uint32_t)sel * 4u, &rec);
            if ((rec >= 0x80000000u)) {
                (void)r4300_read_aligned_word(r4300, rec + 0x1Cu, &cw);
                camctrl = p08_f32(cw);
            }
        }
    }
    (void)r4300_read_aligned_word(r4300, 0x8012D500u, &cam_w);
    if ((cam_w >= 0x80000000u)) {
        uint32_t x = 0, y = 0, z = 0;
        (void)r4300_read_aligned_word(r4300, cam_w + 0, &x);
        (void)r4300_read_aligned_word(r4300, cam_w + 4, &y);
        (void)r4300_read_aligned_word(r4300, cam_w + 8, &z);
        cx = p08_f32(x); cy = p08_f32(y); cz = p08_f32(z);
    }
    (void)rec_w;
    fprintf(s_p08_motion,
            "%d,%d,%u,%d,%d,0x%04x,%d,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,"
            "%.4f,%.4f,%.4f,%.4f,%d\n",
            sub, glidx, post, pad_x, pad_y, pad_btn, bs, px, py, pz, vx, vy, vz,
            yaw, ideal, phys, cx, cy, cz, camctrl, sel);
    fflush(s_p08_motion);
}

static inline void p08_attract_egg_on_syscall(struct r4300_core* r4300, uint32_t op)
{
    const uint32_t code = (op >> 6) & 0xFFFFFu;
    const int interesting =
        (code == 126u) || (code == 129u) || (code >= 534u && code <= 540u) ||
        (code == 552u) || (code == 698u) || (code == 731u) || (code == 781u);
    if (!interesting) {
        return;
    }

    if (code == 698u) {
        p08_motion_on_glrecord(r4300);
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
    const uint32_t ra = (uint32_t)regs[31];
    int mode = 0, map = 0, stage = 0, sub = 0;
    uint32_t post = 0;
    p08_read_attract(r4300, &mode, &map, &stage, &sub, &post);

    FILE* fp = p08_egg_fp();
    if (!fp) {
        return;
    }
    const char* spawn = "";
    if (code == 535u) spawn = " cheggfire";
    else if (code == 539u) spawn = " cheggnormal";
    else if (code == 781u) spawn = " ncbapivot";
    fprintf(fp,
            "[egg-sys] pc=%08x ovl=%u ep=%u a0=%08x a1=%08x ra=%08x mode=%d map=%d "
            "stage=%d sub=%d post=%u%s",
            pc, code, ep, a0, a1, ra, mode, map, stage, sub, post, spawn);
    if ((code >= 534u && code <= 540u) || code == 126u) {
        p08_dump_egg_fields(r4300, fp, a0);
    }
    fprintf(fp, "\n");
    fflush(fp);
}

#endif
