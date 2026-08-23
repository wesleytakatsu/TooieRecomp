#ifndef P08_ATTRACT_EGG_HOOK_H
#define P08_ATTRACT_EGG_HOOK_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Temporary ANGRYLION_CONTROL USER_DEMO_4 egg + follow-camera probe. Logging only.
 * Copied into Mupen pure_interp by tools/oracle/angrylion-control/scripts/build.sh.
 * See docs/specs/BT/angrylion-control-oracle.md. */

static FILE* s_p08_egg_log = NULL;
static FILE* s_p08_motion = NULL;
static FILE* s_p08_ud5 = NULL;
static int s_p08_logged_ep5 = 0;
static uint32_t s_p08_last_post = 0xFFFFFFFFu;
static int s_p08_last_sub = -1;

static inline int p08_ud5_on(void)
{
    const char* e = getenv("P08_UD5_LOCO");
    return e != NULL && e[0] != '\0' && e[0] != '0';
}

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
static uint32_t s_p08_np_frame = 0;
static int s_p08_np_last_fpu = 0;
static int s_p08_np_last_fd = 0;
static uint32_t s_p08_np_last_fpu_pc = 0;
static uint32_t s_p08_np_last_fpu_op = 0;
static int s_p08_np_stop = 0;

static inline int p08_env_enabled(const char* name)
{
    const char* raw = getenv(name);
    return raw != NULL && raw[0] != '\0' && raw[0] != '0';
}

static inline int p08_ud4_first_tick_env(void)
{
    return p08_env_enabled("TOOIE_UD4_FIRST_TICK");
}

static inline int p08_ud4_walkstart_env(void)
{
    return p08_env_enabled("TOOIE_UD4_WALKSTART");
}

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

static inline int p08_guest(uint32_t a);
static inline void p08_ud4_dump_vec3(struct r4300_core* r4300, FILE* fp, const char* tag, uint32_t addr);
static inline void p08_ud4_dump_node(struct r4300_core* r4300, FILE* fp, uint32_t node);
static inline void p08_ud4_dump_reads(struct r4300_core* r4300, FILE* fp, uint32_t rec);
static inline uint32_t p08_ud4_sel_rec(struct r4300_core* r4300);
static inline void p08_ud4_dump_player(struct r4300_core* r4300, FILE* fp);
static inline void p08_ud4_dump_p60_object(struct r4300_core* r4300, FILE* fp, uint32_t node);
static inline uint32_t p08_fpr_bits(struct r4300_core* r4300, int i);

static int s_p08_set74_n = 0;
static int s_p08_set80_n = 0;
static uint32_t s_p08_set74_node = 0;
static uint32_t s_p08_set74_src = 0;
static uint32_t s_p08_set80_node = 0;
static uint32_t s_p08_set80_src = 0;
static uint32_t s_p08_108a0_node = 0;
static uint32_t s_p08_ffb0_a1 = 0;
static uint32_t s_p08_ffb0_a2 = 0;

static int s_p08_seen_haunted = 0;
static inline int p08_ud4_writer_window(struct r4300_core* r4300, int* glidx_out);

static uint32_t s_p08_979d8_ord = 0;
static uint32_t s_p08_bce84_obj = 0;
static uint32_t s_p08_bce84_btn = 0;
static uint32_t s_p08_bakey_prev = 0;
static uint32_t s_p08_bakey_key = 0;
static uint32_t s_p08_bakey_executed = 0;

static inline uint32_t p08_read_u8(struct r4300_core* r4300, uint32_t addr)
{
    uint32_t w = 0;
    (void)r4300_read_aligned_word(r4300, addr & ~3u, &w);
    const uint32_t shift = (3u - (addr & 3u)) * 8u;
    return (w >> shift) & 0xFFu;
}

static inline void p08_ud4_read_consumed_pad(struct r4300_core* r4300, int glidx,
                                            int* pad_x, int* pad_y, unsigned* pad_btn)
{
    uint32_t gl_base_w = 0;
    *pad_x = 0;
    *pad_y = 0;
    *pad_btn = 0;
    (void)r4300_read_aligned_word(r4300, 0x8012D5B0u, &gl_base_w);
    if ((gl_base_w < 0x80000000u) || ((gl_base_w + 0x80000000u) >= 0x00800000u)) {
        return;
    }
    const uint32_t sample = gl_base_w + (uint32_t)(glidx < 0 ? 0 : glidx) * 6u;
    uint32_t w0 = 0, w1 = 0;
    (void)r4300_read_aligned_word(r4300, sample & ~3u, &w0);
    (void)r4300_read_aligned_word(r4300, (sample + 4u) & ~3u, &w1);
    uint8_t b[8];
    b[0] = (uint8_t)(w0 >> 24); b[1] = (uint8_t)(w0 >> 16);
    b[2] = (uint8_t)(w0 >> 8);  b[3] = (uint8_t)w0;
    b[4] = (uint8_t)(w1 >> 24); b[5] = (uint8_t)(w1 >> 16);
    b[6] = (uint8_t)(w1 >> 8);  b[7] = (uint8_t)w1;
    const int off = (int)(sample & 3u);
    *pad_x = (int)(int8_t)b[off];
    *pad_y = (int)(int8_t)b[off + 1];
    *pad_btn = ((unsigned)b[off + 2] << 8) | (unsigned)b[off + 3];
}

static inline void p08_ud4_bce84(struct r4300_core* r4300, uint32_t pc)
{
    int glidx = 0;
    int mode = 0, map = 0, stage = 0, sub = 0;
    uint32_t post = 0;
    if (!p08_ud4_writer_window(r4300, &glidx)) {
        return;
    }
    if (glidx < 36 || glidx > 42) {
        return;
    }
    FILE* fp = p08_ud4_fp();
    if (!fp) {
        return;
    }
    p08_read_attract(r4300, &mode, &map, &stage, &sub, &post);
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t a0 = (uint32_t)regs[4];
    const uint32_t a1 = (uint32_t)regs[5];
    const uint32_t a2 = (uint32_t)regs[6];
    const uint32_t v0 = (uint32_t)regs[2];
    const uint32_t ra = (uint32_t)regs[31];
    int pad_x = 0, pad_y = 0;
    unsigned pad_btn = 0;
    p08_ud4_read_consumed_pad(r4300, glidx, &pad_x, &pad_y, &pad_btn);
    if (pc == 0x800979D8u) {
        uint32_t key = 0;
        (void)r4300_read_aligned_word(r4300, a0 + 0x40u, &key);
        s_p08_979d8_ord += 1;
        s_p08_bakey_key = key;
        fprintf(fp,
                "[ud4-bainput-979d8-join] runtime=AN ord=%u stage=%d sub=%d glidx=%d "
                "pad=(%d,%d,%04x) btn=%u in_id=%u self=%08x key=%08x\n",
                s_p08_979d8_ord, stage, sub, glidx, pad_x, pad_y, pad_btn, a1, a2, a0, key);
    } else if (pc == 0x800919F4u) {
        uint32_t key = 0;
        (void)r4300_read_aligned_word(r4300, a0 + 0x40u, &key);
        s_p08_bakey_key = key;
        s_p08_bakey_prev = 0;
        s_p08_bakey_executed = 0;
        if (p08_guest(key)) {
            s_p08_bakey_prev = p08_read_u8(r4300, key + 0x18u + a1);
        }
        fprintf(fp,
                "[ud4-bakey-held] runtime=AN enter stage=%d sub=%d glidx=%d "
                "pad=(%d,%d,%04x) btn=%u self=%08x key=%08x prev_state=%u ra=%08x 979d8_ord=%u\n",
                stage, sub, glidx, pad_x, pad_y, pad_btn, a1, a0, key, s_p08_bakey_prev, ra,
                s_p08_979d8_ord);
    } else if (pc == 0x80091A18u) {
        uint32_t obj = 0;
        s_p08_bakey_executed = 1;
        (void)r4300_read_aligned_word(r4300, v0, &obj);
        fprintf(fp,
                "[ud4-bakey-held] runtime=AN jal_bce84 glidx=%d btn=%u prev_state=%u key=%08x obj=%08x\n",
                glidx, a1, s_p08_bakey_prev, v0, obj);
    } else if (pc == 0x80091A20u) {
        fprintf(fp,
                "[ud4-bakey-held] runtime=AN leave glidx=%d executed=%u prev_state=%u ret=%08x\n",
                glidx, s_p08_bakey_executed, s_p08_bakey_prev, v0);
    } else if (pc == 0x800BCE84u) {
        s_p08_bce84_obj = a0;
        s_p08_bce84_btn = a1;
        fprintf(fp,
                "[ud4-bce84] runtime=AN enter stage=%d sub=%d glidx=%d pad=(%d,%d,%04x) "
                "btn=%u obj=%08x key=%08x load_pc=800BCE84 ra=%08x 979d8_ord=%u\n",
                stage, sub, glidx, pad_x, pad_y, pad_btn, a1, a0, s_p08_bakey_key, ra,
                s_p08_979d8_ord);
    } else if (pc == 0x800BCEACu) {
        const uint32_t t0 = (uint32_t)regs[8];
        const uint32_t t8 = (uint32_t)regs[24];
        uint32_t loaded = 0;
        uint32_t ring = 0;
        uint32_t old_probe = 0;
        if (p08_guest(s_p08_bce84_obj)) {
            ring = p08_read_u8(r4300, s_p08_bce84_obj + 8u + s_p08_bce84_btn);
            (void)r4300_read_aligned_word(r4300, s_p08_bce84_obj + 0x24u + (s_p08_bce84_btn * 4u),
                                          &old_probe);
        }
        if (p08_guest(t0)) {
            (void)r4300_read_aligned_word(r4300, t0, &loaded);
        }
        fprintf(fp,
                "[ud4-bce84] runtime=AN stage=%d sub=%d glidx=%d pad=(%d,%d,%04x) btn=%u "
                "obj=%08x key=%08x load_pc=800BCEAC addr=%08x ring=%u t8=%u "
                "raw=%08x loaded=%08x masked=%u ra=%08x 979d8_ord=%u "
                "old_probe_word=%08x old_probe_bit0=%u\n",
                stage, sub, glidx, pad_x, pad_y, pad_btn, s_p08_bce84_btn,
                s_p08_bce84_obj, s_p08_bakey_key, t0, ring, t8,
                loaded, loaded, loaded & 1u, ra, s_p08_979d8_ord,
                old_probe, old_probe & 1u);
    } else if (pc == 0x800BCEB8u) {
        const uint32_t t1 = (uint32_t)regs[9];
        fprintf(fp,
                "[ud4-bce84] runtime=AN leave glidx=%d btn=%u loaded=%08x masked=%u ret_pending=%08x\n",
                glidx, s_p08_bce84_btn, v0, t1 & 1u, t1);
    }
    fflush(fp);
}

static inline int p08_ud4_writer_window(struct r4300_core* r4300, int* glidx_out)
{
    int mode = 0, map = 0, stage = 0, sub = 0;
    uint32_t post = 0;
    p08_read_attract(r4300, &mode, &map, &stage, &sub, &post);
    const int glidx = (post > 0u) ? (int)post - 1 : 0;
    if (glidx_out) {
        *glidx_out = glidx;
    }
    if (map == 235 || (mode == 5 && sub == 4)) {
        s_p08_seen_haunted = 1;
    }
    return s_p08_seen_haunted;
}

static inline void p08_ud4_writer(struct r4300_core* r4300, uint32_t pc)
{
    int glidx = 0;
    int mode = 0, map = 0, stage = 0, sub = 0;
    uint32_t post = 0;
    if (!p08_ud4_writer_window(r4300, &glidx)) {
        return;
    }
    FILE* fp = p08_ud4_fp();
    if (!fp) {
        return;
    }
    p08_read_attract(r4300, &mode, &map, &stage, &sub, &post);
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t a0 = (uint32_t)regs[4];
    const uint32_t a1 = (uint32_t)regs[5];
    const uint32_t a2 = (uint32_t)regs[6];
    const uint32_t a3 = (uint32_t)regs[7];
    const uint32_t ra = (uint32_t)regs[31];
    const uint32_t seq = ++s_p08_ud4_seq;
    if (pc == 0x80112A84u) {
        fprintf(fp,
                "[ud4-p60-writer] enter seq=%u glidx=%d pc=80112A84 ra=%08x node=%08x",
                seq, glidx, ra, a0);
        p08_ud4_dump_p60_object(r4300, fp, a0);
        fprintf(fp, "\n");
    } else if (pc == 0x80112D08u) {
        fprintf(fp,
                "[ud4-p60-write] enter seq=%u glidx=%d pc=80112D08 ra=%08x"
                " node=%08x a1=%08x a2=%08x a3=%08x",
                seq, glidx, ra, a0, a1, a2, a3);
        p08_ud4_dump_p60_object(r4300, fp, a0);
        fprintf(fp, "\n");
    } else if (pc == 0x80112D28u) {
        fprintf(fp, "[ud4-p60-write] after-store seq=%u glidx=%d pc=80112D28 node=%08x",
                seq, glidx, a0);
        p08_ud4_dump_p60_object(r4300, fp, a0);
        fprintf(fp, "\n");
    } else if (pc == 0x80112C34u) {
        fprintf(fp, "[ud4-p60-writer] leave seq=%u glidx=%d pc=80112C34 node=%08x",
                seq, glidx, a0);
        p08_ud4_dump_p60_object(r4300, fp, a0);
        fprintf(fp, "\n");
    } else if (pc == 0x800979D8u) {
        uint32_t key = 0, input = 0;
        (void)r4300_read_aligned_word(r4300, a0 + 0x40u, &key);
        (void)r4300_read_aligned_word(r4300, a0 + 0xA4u, &input);
        uint32_t held_w = 0, in0 = 0, in4 = 0;
        if (p08_guest(key)) {
            (void)r4300_read_aligned_word(r4300, key + 0x18u + (a1 & ~3u), &held_w);
        }
        if (p08_guest(input)) {
            (void)r4300_read_aligned_word(r4300, input + 0u, &in0);
            (void)r4300_read_aligned_word(r4300, input + 4u, &in4);
        }
        const uint32_t shift = (3u - ((0x18u + a1) & 3u)) * 8u;
        const uint32_t held = (held_w >> shift) & 0xFFu;
        fprintf(fp,
                "[ud4-bainput-979d8] enter glidx=%d self=%08x btn=%u in_id=%u key=%08x inp=%08x held=%u\n",
                glidx, a0, a1, a2, key, input, held);
    } else if (pc == 0x80097A48u) {
        fprintf(fp, "[ud4-bainput-979d8] leave glidx=%d v0=%08x\n",
                glidx, (uint32_t)regs[2]);
    } else if (pc == 0x80097B94u) {
        fprintf(fp, "[ud4-cam-chain] enter 80097B94 (rotate_camera_right) glidx=%d self=%08x\n",
                glidx, a0);
    } else if (pc == 0x80097BB4u) {
        fprintf(fp, "[ud4-cam-chain] leave 80097B94 (rotate_camera_right) glidx=%d v0=%08x\n",
                glidx, (uint32_t)regs[2]);
    } else if (pc == 0x800A4338u) {
        fprintf(fp, "[ud4-cam-chain] enter 800A4338 glidx=%d a0=%08x a1=%08x a2=%08x\n",
                glidx, a0, a1, a2);
    } else if (pc == 0x80110EFCu) {
        fprintf(fp, "[ud4-cam-chain] enter 80110EFC glidx=%d node=%08x a1=%08x\n",
                glidx, a0, a1);
    } else if (pc == 0x80110E00u) {
        fprintf(fp, "[ud4-cam-chain] enter 80110E00 glidx=%d node=%08x a1=%08x\n",
                glidx, a0, a1);
    } else if (pc == 0x801111D0u) {
        fprintf(fp, "[ud4-cam-chain] enter 801111D0 glidx=%d node=%08x a1=%.4f (bits=%08x)\n",
                glidx, a0, p08_f32(a1), a1);
    } else if (pc == 0x80112D90u) {
        fprintf(fp,
                "[ud4-p60-writer-2d90] enter seq=%u glidx=%d pc=80112D90 ra=%08x node=%08x a1=%.4f a2=%.4f a3=%u",
                seq, glidx, ra, a0, p08_f32(a1), p08_f32(a2), a3);
        p08_ud4_dump_p60_object(r4300, fp, a0);
        fprintf(fp, "\n");
    } else if (pc == 0x80112E4Cu) {
        fprintf(fp,
                "[ud4-p60-writer-2d90] leave seq=%u glidx=%d pc=80112D90 node=%08x",
                seq, glidx, a0);
        p08_ud4_dump_p60_object(r4300, fp, a0);
        fprintf(fp, "\n");
    } else if (pc == 0x80110770u) {
        s_p08_set74_n += 1;
        s_p08_set74_node = a0;
        s_p08_set74_src = a1;
        fprintf(fp,
                "[ud4-set74] enter ord=%d seq=%u ra=%08x node=%08x src=%08x"
                " glidx=%d mode=%d map=%d stage=%d sub=%d",
                s_p08_set74_n, seq, ra, a0, a1, glidx, mode, map, stage, sub);
        p08_ud4_dump_vec3(r4300, fp, "dest_before", a0 + 0x74u);
        p08_ud4_dump_vec3(r4300, fp, "src", a1);
        p08_ud4_dump_node(r4300, fp, a0);
        fprintf(fp, "\n");
    } else if (pc == 0x80110780u) {
        fprintf(fp, "[ud4-set74] leave ord=%d seq=%u node=%08x",
                s_p08_set74_n, seq, s_p08_set74_node);
        p08_ud4_dump_vec3(r4300, fp, "dest_after", s_p08_set74_node + 0x74u);
        p08_ud4_dump_vec3(r4300, fp, "src", s_p08_set74_src);
        fprintf(fp, "\n");
    } else if (pc == 0x80110790u) {
        s_p08_set80_n += 1;
        s_p08_set80_node = a0;
        s_p08_set80_src = a1;
        fprintf(fp,
                "[ud4-set80] enter ord=%d seq=%u ra=%08x node=%08x src=%08x"
                " glidx=%d mode=%d map=%d stage=%d sub=%d",
                s_p08_set80_n, seq, ra, a0, a1, glidx, mode, map, stage, sub);
        p08_ud4_dump_vec3(r4300, fp, "dest_before", a0 + 0x80u);
        p08_ud4_dump_vec3(r4300, fp, "src", a1);
        fprintf(fp, "\n");
    } else if (pc == 0x801107A0u) {
        fprintf(fp, "[ud4-set80] leave ord=%d seq=%u node=%08x",
                s_p08_set80_n, seq, s_p08_set80_node);
        p08_ud4_dump_vec3(r4300, fp, "dest_after", s_p08_set80_node + 0x80u);
        p08_ud4_dump_vec3(r4300, fp, "src", s_p08_set80_src);
        fprintf(fp, "\n");
    } else if (pc == 0x801108A0u) {
        uint32_t rec64 = 0;
        s_p08_108a0_node = a0;
        if (p08_guest(a0)) {
            (void)r4300_read_aligned_word(r4300, a0 + 0x64u, &rec64);
        }
        fprintf(fp, "[ud4-108a0] enter seq=%u ra=%08x node=%08x rec64=%08x"
                    " glidx=%d mode=%d map=%d stage=%d sub=%d",
                seq, ra, a0, rec64, glidx, mode, map, stage, sub);
        p08_ud4_dump_node(r4300, fp, a0);
        p08_ud4_dump_reads(r4300, fp, p08_ud4_sel_rec(r4300));
        fprintf(fp, "\n");
    } else if (pc == 0x80110920u) {
        fprintf(fp, "[ud4-108a0] leave seq=%u node=%08x", seq, s_p08_108a0_node);
        p08_ud4_dump_node(r4300, fp, s_p08_108a0_node);
        fprintf(fp, "\n");
    } else if (pc == 0x8010FFB0u) {
        uint32_t rec = 0;
        s_p08_ffb0_a1 = a1;
        s_p08_ffb0_a2 = a2;
        if (p08_guest(a0)) {
            (void)r4300_read_aligned_word(r4300, a0, &rec);
        }
        fprintf(fp, "[ud4-ffb0] enter seq=%u ra=%08x a0=%08x rec=%08x a1=%08x a2=%08x"
                    " glidx=%d mode=%d map=%d",
                seq, ra, a0, rec, a1, a2, glidx, mode, map);
        p08_ud4_dump_reads(r4300, fp, rec);
        p08_ud4_dump_vec3(r4300, fp, "a1_pre", a1);
        p08_ud4_dump_vec3(r4300, fp, "a2_pre", a2);
        fprintf(fp, "\n");
    } else if (pc == 0x8010FFC0u) {
        fprintf(fp, "[ud4-ffb0] leave seq=%u a1=%08x a2=%08x", seq, s_p08_ffb0_a1, s_p08_ffb0_a2);
        p08_ud4_dump_vec3(r4300, fp, "a1_post", s_p08_ffb0_a1);
        p08_ud4_dump_vec3(r4300, fp, "a2_post", s_p08_ffb0_a2);
        fprintf(fp, "\n");
    } else if (pc == 0x80089470u || pc == 0x80089478u || pc == 0x80089480u ||
               pc == 0x80089488u) {
        const unsigned ep = (unsigned)((pc - 0x80089470u) / 8u);
        fprintf(fp, "[ud4-np-stub] enter seq=%u pc=%08x ep=%u ra=%08x a0=%08x"
                    " glidx=%d mode=%d map=%d stage=%d sub=%d",
                seq, pc, ep, ra, a0, glidx, mode, map, stage, sub);
        p08_ud4_dump_node(r4300, fp, a0);
        p08_ud4_dump_reads(r4300, fp, p08_ud4_sel_rec(r4300));
        p08_ud4_dump_player(r4300, fp);
        fprintf(fp, "\n");
        if (pc == 0x80089488u && !s_p08_np_trace) {
            const int first = (s_p08_ncbapivot_n++ == 0) ? 1 : 0;
            const int walkstart = p08_ud4_walkstart_env() && glidx == 39;
            fprintf(fp,
                    "[ud4-ncbapivot] enter seq=%u glidx=%d first=%d ra=%08x a0=%08x pc=%08x "
                    "a1=%08x a2=%08x a3=%08x sp=%08x frame=%08x f0=%08x f12=%08x fcr31=%08x",
                    seq, glidx, first, ra, a0, pc, a1, a2, a3,
                    (uint32_t)regs[29], (uint32_t)regs[29] - 0x78u,
                    p08_fpr_bits(r4300, 0), p08_fpr_bits(r4300, 12), r4300->cp1.fcr31);
            p08_ud4_dump_node(r4300, fp, a0);
            p08_ud4_dump_reads(r4300, fp, p08_ud4_sel_rec(r4300));
            p08_ud4_dump_player(r4300, fp);
            fprintf(fp, "\n");
            if ((p08_ud4_first_tick_env() && first) || walkstart) {
                s_p08_np_trace = 1;
                s_p08_np_stop = p08_ud4_first_tick_env() ? 1 : 0;
                s_p08_np_node = a0;
                s_p08_np_glidx = glidx;
                s_p08_np_ret = ra;
                s_p08_np_frame = (uint32_t)regs[29] - 0x78u;
                s_p08_np_call_n = 0;
                s_p08_np_insn = 0;
                s_p08_np_ovl_base = 0;
                s_p08_np_child_sp = 0;
                fprintf(fp, "[ud4-ncbapivot] TRACE_ON ret=%08x frame=%08x bind_tick=%d stop=%d\n",
                        s_p08_np_ret, s_p08_np_frame, first, s_p08_np_stop);
            }
        }
    }
    fflush(fp);
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
    uint32_t p14 = 0, p3c = 0, p4c = 0, p60 = 0;
    (void)r4300_read_aligned_word(r4300, node + 0x14u, &p14);
    (void)r4300_read_aligned_word(r4300, node + 0x3Cu, &p3c);
    (void)r4300_read_aligned_word(r4300, node + 0x4Cu, &p4c);
    (void)r4300_read_aligned_word(r4300, node + 0x60u, &p60);
    fprintf(fp, " node=%08x +4=%08x +14=%08x +28=%08x +30=%08x +3c=%08x +4c=%08x +5c=%08x +60=%08x *5c=%08x +4.2e=%04x",
            node, p4, p14, p28, p30, p3c, p4c, p5c, p60, p5c0, (unsigned)((w2e >> 16) & 0xFFFFu));
    p08_ud4_dump_vec3(r4300, fp, "n74", node + 0x74u);
    p08_ud4_dump_vec3(r4300, fp, "n80", node + 0x80u);
    if (p08_guest(p30)) {
        p08_ud4_dump_vec3(r4300, fp, "tgt", p30 + 0x4u);
    }
    if (p08_guest(p14)) {
        uint32_t c = 0, d = 0;
        (void)r4300_read_aligned_word(r4300, p14 + 0xCu, &c);
        (void)r4300_read_aligned_word(r4300, p14 + 0x10u, &d);
        fprintf(fp, " p14c=%08x p14_10=%08x", c, d);
    }
    if (p08_guest(p3c)) {
        uint32_t w0 = 0, w4 = 0, w8 = 0, w10 = 0, w1c = 0;
        (void)r4300_read_aligned_word(r4300, p3c + 0u, &w0);
        (void)r4300_read_aligned_word(r4300, p3c + 4u, &w4);
        (void)r4300_read_aligned_word(r4300, p3c + 8u, &w8);
        (void)r4300_read_aligned_word(r4300, p3c + 0x10u, &w10);
        (void)r4300_read_aligned_word(r4300, p3c + 0x1Cu, &w1c);
        fprintf(fp, " p3c0=%08x p3c4=%08x p3c8=%08x p3c10=%02x p3c11=%02x p3c12=%02x p3c1c=%02x",
                w0, w4, w8,
                (unsigned)((w10 >> 24) & 0xFFu),
                (unsigned)((w10 >> 16) & 0xFFu),
                (unsigned)((w10 >> 8) & 0xFFu),
                (unsigned)((w1c >> 24) & 0xFFu));
    }
    if (p08_guest(p4c)) {
        uint32_t c = 0;
        (void)r4300_read_aligned_word(r4300, p4c + 0xCu, &c);
        fprintf(fp, " p4cC=%08x", c);
    }
    if (p08_guest(p60)) {
        uint32_t w0 = 0, w4 = 0, w8 = 0;
        (void)r4300_read_aligned_word(r4300, p60 + 0u, &w0);
        (void)r4300_read_aligned_word(r4300, p60 + 4u, &w4);
        (void)r4300_read_aligned_word(r4300, p60 + 8u, &w8);
        fprintf(fp, " p60_0=%08x p60_4=%08x p60_8=%08x", w0, w4, w8);
    }
}

static inline void p08_ud4_dump_p60_object(struct r4300_core* r4300, FILE* fp, uint32_t node)
{
    if (!p08_guest(node)) {
        fprintf(fp, " p60=none");
        return;
    }
    uint32_t p60 = 0;
    (void)r4300_read_aligned_word(r4300, node + 0x60u, &p60);
    if (!p08_guest(p60)) {
        fprintf(fp, " p60=%08x object=none", p60);
        return;
    }
    uint32_t w0 = 0, w4 = 0, w8 = 0, wc = 0;
    uint32_t w10 = 0, w14 = 0, w18 = 0, w1c = 0, w20 = 0;
    (void)r4300_read_aligned_word(r4300, p60 + 0x00u, &w0);
    (void)r4300_read_aligned_word(r4300, p60 + 0x04u, &w4);
    (void)r4300_read_aligned_word(r4300, p60 + 0x08u, &w8);
    (void)r4300_read_aligned_word(r4300, p60 + 0x0Cu, &wc);
    (void)r4300_read_aligned_word(r4300, p60 + 0x10u, &w10);
    (void)r4300_read_aligned_word(r4300, p60 + 0x14u, &w14);
    (void)r4300_read_aligned_word(r4300, p60 + 0x18u, &w18);
    (void)r4300_read_aligned_word(r4300, p60 + 0x1Cu, &w1c);
    (void)r4300_read_aligned_word(r4300, p60 + 0x20u, &w20);
    fprintf(fp,
            " p60=%08x p60_0=%08x p60_4=%08x p60_8=%08x p60_c=%04x p60_e=%04x"
            " p60_10=%08x p60_14=%08x p60_18=%08x p60_1c=%08x p60_20=%08x",
            p60, w0, w4, w8, (unsigned)(wc >> 16), (unsigned)(wc & 0xFFFFu),
            w10, w14, w18, w1c, w20);
}

static inline void p08_ud4_dump_np_frame(struct r4300_core* r4300, FILE* fp)
{
    uint32_t w38 = 0, w3c = 0, w40 = 0, w44 = 0;
    if (!p08_guest(s_p08_np_frame)) {
        fprintf(fp, " frame=none");
        return;
    }
    (void)r4300_read_aligned_word(r4300, s_p08_np_frame + 0x38u, &w38);
    (void)r4300_read_aligned_word(r4300, s_p08_np_frame + 0x3Cu, &w3c);
    (void)r4300_read_aligned_word(r4300, s_p08_np_frame + 0x40u, &w40);
    (void)r4300_read_aligned_word(r4300, s_p08_np_frame + 0x44u, &w44);
    fprintf(fp, " frame=%08x sp38=%08x sp3c=%08x sp40=%08x sp44=%08x",
            s_p08_np_frame, w38, w3c, w40, w44);
    p08_ud4_dump_vec3(r4300, fp, "sp28", s_p08_np_frame + 0x28u);
    p08_ud4_dump_vec3(r4300, fp, "sp34", s_p08_np_frame + 0x34u);
    p08_ud4_dump_vec3(r4300, fp, "sp48", s_p08_np_frame + 0x48u);
    p08_ud4_dump_vec3(r4300, fp, "sp54", s_p08_np_frame + 0x54u);
    p08_ud4_dump_vec3(r4300, fp, "sp60", s_p08_np_frame + 0x60u);
    p08_ud4_dump_vec3(r4300, fp, "sp6c", s_p08_np_frame + 0x6Cu);
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
    case 0x800F1738u: return "800F1738";
    case 0x800F1DCCu: return "800F1DCC";
    case 0x801125E8u: return "801125E8";
    case 0x800EEBF0u: return "800EEBF0";
    case 0x800EE780u: return "800EE780";
    case 0x800EFD24u: return "800EFD24";
    case 0x800EF214u: return "800EF214";
    case 0x80013B70u: return "80013B70";
    case 0x80013B7Cu: return "80013B7C";
    case 0x800137ACu: return "800137AC";
    case 0x800137C4u: return "800137C4";
    case 0x800137F4u: return "800137F4";
    case 0x80013788u: return "80013788";
    case 0x80013818u: return "80013818";
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
    if (pc == 0x80112C3Cu && phase[0] == 'e' && p08_guest(s_p08_np_node)) {
        uint32_t p60 = 0, w0 = 0, w4 = 0, w8 = 0;
        (void)r4300_read_aligned_word(r4300, s_p08_np_node + 0x60u, &p60);
        if (p08_guest(p60)) {
            (void)r4300_read_aligned_word(r4300, p60 + 0u, &w0);
            (void)r4300_read_aligned_word(r4300, p60 + 4u, &w4);
            (void)r4300_read_aligned_word(r4300, p60 + 8u, &w8);
            uint32_t t7 = w8 - 1u;
            fprintf(fp,
                    " 80112C3C_sel p60=%08x p60_0=%08x p60_4=%08x p60_8=%08x t7=%08x sltiu5=%d",
                    p60, w0, w4, w8, t7, t7 < 5u ? 1 : 0);
        }
    }
    p08_ud4_dump_vec3(r4300, fp, "va0", a0);
    p08_ud4_dump_vec3(r4300, fp, "va1", a1);
    p08_ud4_dump_vec3(r4300, fp, "va2", a2);
    p08_ud4_dump_np_frame(r4300, fp);
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
                "fcr31=%08x insn=%d ovl_base=%08x stop=%d",
                ++s_p08_ud4_seq, s_p08_np_glidx, (uint32_t)regs[2],
                p08_fpr_bits(r4300, 0), r4300->cp1.fcr31, s_p08_np_insn,
                s_p08_np_ovl_base, s_p08_np_stop);
        p08_ud4_dump_np_frame(r4300, fp);
        p08_ud4_dump_node(r4300, fp, s_p08_np_node);
        p08_ud4_dump_reads(r4300, fp, p08_ud4_sel_rec(r4300));
        p08_ud4_dump_player(r4300, fp);
        if (s_p08_np_stop) {
            fprintf(fp, "\n[ud4-ncbapivot] FIRST_TICK_STOP after glidx=%d\n", s_p08_np_glidx);
            fflush(fp);
        }
        s_p08_np_trace = 0;
        s_p08_np_child_sp = 0;
        if (s_p08_np_stop) {
            exit(0);
        }
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
        const int arith =
            (pc >= 0x800EFB24u && pc <= 0x800EFB54u) ||
            (pc >= 0x800EEF94u && pc <= 0x800EEFD0u) ||
            (pc >= 0x800F1A88u && pc <= 0x800F1AF8u) ||
            (pc >= 0x80013B70u && pc <= 0x80013C80u) ||
            (pc >= 0x80013700u && pc <= 0x80013A80u) ||
            (pc >= 0x800EE780u && pc <= 0x800EE7B0u) ||
            (pc >= 0x800EEBF0u && pc <= 0x800EEC2Cu) ||
            (pc >= 0x800EF214u && pc <= 0x800EF29Cu) ||
            (pc >= 0x800EFD24u && pc <= 0x800EFD80u);
        if (!arith) {
            return;
        }
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

static uint32_t s_p08_ud5_ca628_dst;
static uint32_t s_p08_ud5_ca628_old;
static uint32_t s_p08_ud5_ca628_src21;
static unsigned s_p08_ud5_ca628_ord;
static unsigned s_p08_ud5_ca8b4_ord;
static unsigned s_p08_ud5_e1a58_ord;
static uint32_t s_p08_ud5_ca8b4_a0;
static uint32_t s_p08_ud5_ca8b4_a1;
static uint32_t s_p08_ud5_ca8b4_a2;
static unsigned s_p08_ud5_abc_trot[8];
static unsigned s_p08_ud5_abc_pending[8];
static uint32_t s_p08_abc_cam;
static uint32_t s_p08_abc_stick;
static uint32_t s_p08_ud5_set80_node;
static uint32_t s_p08_ud5_set80_src;
static uint32_t s_p08_ud5_set80_ra;
static unsigned s_p08_ud5_set80_ord;
static uint32_t s_p08_ud5_set74_s0;
static unsigned s_p08_ud5_set74_ord;
static uint32_t s_p08_ud5_sp6c_prod_dst;
static unsigned s_p08_ud5_sp6c_prod_ord;
static uint32_t s_p08_ud5_f18fc_out;
static unsigned s_p08_ud5_f18fc_ord;
static uint32_t s_p08_ud5_a516c_dst;
static unsigned s_p08_ud5_a516c_ord;
static uint32_t s_p08_ud5_a68_dst;
static unsigned s_p08_ud5_a68_ord;

static inline int p08_ud5_cam1c_window(struct r4300_core* r4300, int* glidx_out)
{
    if (!p08_ud5_on()) {
        return 0;
    }
    int mode = 0, map = 0, stage = 0, sub = 0;
    uint32_t post = 0;
    p08_read_attract(r4300, &mode, &map, &stage, &sub, &post);
    const int glidx = (post > 0u) ? (int)post - 1 : 0;
    if (glidx_out) {
        *glidx_out = glidx;
    }
    return mode == 5 && sub == 5 && glidx >= 20 && glidx <= 28;
}

static inline void p08_ud5_cam1c(struct r4300_core* r4300, uint32_t pc)
{
    int glidx = 0;
    if (!p08_ud5_cam1c_window(r4300, &glidx)) {
        return;
    }
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t a0 = (uint32_t)regs[4];
    const uint32_t a1 = (uint32_t)regs[5];
    const uint32_t a2 = (uint32_t)regs[6];
    const uint32_t ra = (uint32_t)regs[31];
    int32_t sel = -1;
    uint32_t rec = p08_ud4_sel_rec(r4300);
    uint32_t ps = 0, selw = 0;
    (void)r4300_read_aligned_word(r4300, 0x80135490u, &ps);
    if (p08_guest(ps)) {
        (void)r4300_read_aligned_word(r4300, ps + 0x184u, &selw);
        sel = (int32_t)selw;
    }
    if (pc == 0x800CA628u) {
        uint32_t old_1c = 0, s0 = 0, s1 = 0, s2 = 0;
        s_p08_ud5_ca628_dst = a0;
        if (p08_guest(a0)) {
            (void)r4300_read_aligned_word(r4300, a0 + 0x1Cu, &old_1c);
        }
        if (p08_guest(a2)) {
            (void)r4300_read_aligned_word(r4300, a2 + 0u, &s0);
            (void)r4300_read_aligned_word(r4300, a2 + 4u, &s1);
            (void)r4300_read_aligned_word(r4300, a2 + 8u, &s2);
        }
        s_p08_ud5_ca628_old = old_1c;
        s_p08_ud5_ca628_src21 = s1;
        const unsigned ord = ++s_p08_ud5_ca628_ord;
        fprintf(stderr,
                "[ud5-cam1c] phase=ca628 enter glidx=%d ord=%u pc=800ca628 "
                "store=800ca65c ra=%08x dst=%08x src=%08x src2=%08x sel=%d rec=%08x "
                "selected=%d old_1c=%08x src2_0=%08x src2_1=%08x src2_2=%08x "
                "source_iter=1 source_reg=f6\n",
                glidx, ord, ra, a0, a1, a2, sel, rec,
                (a0 != 0 && a0 == rec) ? 1 : 0, old_1c, s0, s1, s2);
        fflush(stderr);
    } else if (pc == 0x800CA65Cu) {
        const uint32_t v0 = (uint32_t)regs[2];
        const uint32_t v1 = (uint32_t)regs[3];
        const uint32_t store_addr = v1 + 0x14u;
        const uint32_t f6 = p08_fpr_bits(r4300, 6);
        uint32_t old_at = 0;
        if (p08_guest(store_addr)) {
            (void)r4300_read_aligned_word(r4300, store_addr, &old_at);
        }
        fprintf(stderr,
                "[ud5-cam1c] phase=ca628 store glidx=%d pc=800ca65c "
                "loop_v0=%u v1=%08x store=%08x dst=%08x dst_plus_1c=%08x "
                "heading_store=%d old_at=%08x f6_bits=%08x\n",
                glidx, v0, v1, store_addr, s_p08_ud5_ca628_dst,
                s_p08_ud5_ca628_dst + 0x1Cu,
                (store_addr == s_p08_ud5_ca628_dst + 0x1Cu) ? 1 : 0,
                old_at, f6);
        fflush(stderr);
    } else if (pc == 0x800CA660u) {
        uint32_t new_1c = 0;
        if (p08_guest(s_p08_ud5_ca628_dst)) {
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_ca628_dst + 0x1Cu, &new_1c);
        }
        fprintf(stderr,
                "[ud5-cam1c] phase=ca628 leave glidx=%d ord=%u pc=800ca628 "
                "store=800ca65c dst=%08x selected=%d old_1c=%08x source_bits=%08x "
                "new_1c=%08x\n",
                glidx, s_p08_ud5_ca628_ord, s_p08_ud5_ca628_dst,
                (s_p08_ud5_ca628_dst != 0 && s_p08_ud5_ca628_dst == rec) ? 1 : 0,
                s_p08_ud5_ca628_old, s_p08_ud5_ca628_src21, new_1c);
        fflush(stderr);
    } else if (pc == 0x8010FC38u) {
        fprintf(stderr,
                "[ud5-cam1c] phase=fc38 enter glidx=%d pc=8010fc38 ra=%08x a0=%08x\n",
                glidx, ra, a0);
        fflush(stderr);
    } else if (pc == 0x800CA8B4u) {
        uint32_t e0 = 0, e1 = 0, e2 = 0, s0 = 0, s1 = 0, s2 = 0;
        s_p08_ud5_ca8b4_a0 = a0;
        s_p08_ud5_ca8b4_a1 = a1;
        s_p08_ud5_ca8b4_a2 = a2;
        if (p08_guest(a0)) {
            (void)r4300_read_aligned_word(r4300, a0 + 0x00u, &e0);
            (void)r4300_read_aligned_word(r4300, a0 + 0x04u, &e1);
            (void)r4300_read_aligned_word(r4300, a0 + 0x08u, &e2);
            (void)r4300_read_aligned_word(r4300, a0 + 0x18u, &s0);
            (void)r4300_read_aligned_word(r4300, a0 + 0x1Cu, &s1);
            (void)r4300_read_aligned_word(r4300, a0 + 0x20u, &s2);
        }
        fprintf(stderr,
                "[ud5-cam1c] phase=ca8b4 enter glidx=%d ord=%u pc=800ca8b4 ra=%08x "
                "a0=%08x a1=%08x a2=%08x sel=%d rec=%08x selected=%d "
                "eye_in=%08x,%08x,%08x src2_in=%08x,%08x,%08x\n",
                glidx, ++s_p08_ud5_ca8b4_ord, ra, a0, a1, a2, sel, rec,
                (a0 != 0 && a0 == rec) ? 1 : 0, e0, e1, e2, s0, s1, s2);
        fflush(stderr);
    } else if (pc == 0x800CA8ECu) {
        uint32_t s0 = 0, s1 = 0, s2 = 0;
        if (p08_guest(s_p08_ud5_ca8b4_a2)) {
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_ca8b4_a2 + 0u, &s0);
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_ca8b4_a2 + 4u, &s1);
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_ca8b4_a2 + 8u, &s2);
        }
        fprintf(stderr,
                "[ud5-cam1c] phase=ca8b4 leave glidx=%d ord=%u pc=800ca8b4 "
                "selected=%d src2_0=%08x src2_1=%08x src2_2=%08x\n",
                glidx, s_p08_ud5_ca8b4_ord,
                (s_p08_ud5_ca8b4_a0 != 0 && s_p08_ud5_ca8b4_a0 == rec) ? 1 : 0,
                s0, s1, s2);
        fflush(stderr);
    } else if (pc == 0x800E1A58u) {
        uint32_t in_h = 0, a10 = 0, a12 = 0;
        if (p08_guest(a1)) {
            (void)r4300_read_aligned_word(r4300, a1 + 0u, &a10);
            (void)r4300_read_aligned_word(r4300, a1 + 4u, &in_h);
            (void)r4300_read_aligned_word(r4300, a1 + 8u, &a12);
        }
        fprintf(stderr,
                "[ud5-cam1c] phase=e1a58 enter glidx=%d ord=%u pc=800e1a58 ra=%08x "
                "a0=%08x a1=%08x heading_src=%08x a1_0=%08x a1_2=%08x\n",
                glidx, ++s_p08_ud5_e1a58_ord, ra, a0, a1, in_h, a10, a12);
        fflush(stderr);
    } else if (pc == 0x80110790u) {
        uint32_t target_mode2 = 0;
        if (p08_guest(rec)) {
            (void)r4300_read_aligned_word(r4300, rec + 0x4u, &target_mode2);
        }
        uint32_t old_80 = 0, old_84 = 0, old_88 = 0;
        uint32_t src_0 = 0, src_1 = 0, src_2 = 0;
        if (p08_guest(a0)) {
            (void)r4300_read_aligned_word(r4300, a0 + 0x80u, &old_80);
            (void)r4300_read_aligned_word(r4300, a0 + 0x84u, &old_84);
            (void)r4300_read_aligned_word(r4300, a0 + 0x88u, &old_88);
        }
        if (p08_guest(a1)) {
            (void)r4300_read_aligned_word(r4300, a1 + 0x00u, &src_0);
            (void)r4300_read_aligned_word(r4300, a1 + 0x04u, &src_1);
            (void)r4300_read_aligned_word(r4300, a1 + 0x08u, &src_2);
        }
        s_p08_ud5_set80_node = a0;
        s_p08_ud5_set80_src = a1;
        s_p08_ud5_set80_ra = ra;
        const unsigned ord = ++s_p08_ud5_set80_ord;
        fprintf(stderr,
                "[ud5-set80] phase=set80 enter glidx=%d ord=%u pc=80110790 store=800ee804 ra=%08x "
                "node=%08x src=%08x sel=%d rec=%08x target_mode2=%08x selected=%d "
                "old_80=%08x old_84=%08x old_88=%08x src_0=%08x src_1=%08x src_2=%08x\n",
                glidx, ord, ra, a0, a1, sel, rec, target_mode2,
                (a0 != 0 && a0 == target_mode2) ? 1 : 0,
                old_80, old_84, old_88, src_0, src_1, src_2);
        fflush(stderr);
    } else if (pc == 0x801107A0u) {
        uint32_t target_mode2 = 0;
        if (p08_guest(rec)) {
            (void)r4300_read_aligned_word(r4300, rec + 0x4u, &target_mode2);
        }
        uint32_t new_80 = 0, new_84 = 0, new_88 = 0;
        if (p08_guest(s_p08_ud5_set80_node)) {
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_set80_node + 0x80u, &new_80);
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_set80_node + 0x84u, &new_84);
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_set80_node + 0x88u, &new_88);
        }
        fprintf(stderr,
                "[ud5-set80] phase=set80 leave glidx=%d ord=%u pc=80110790 store=800ee804 "
                "node=%08x selected=%d new_80=%08x new_84=%08x new_88=%08x\n",
                glidx, s_p08_ud5_set80_ord, s_p08_ud5_set80_node,
                (s_p08_ud5_set80_node != 0 && s_p08_ud5_set80_node == target_mode2) ? 1 : 0,
                new_80, new_84, new_88);
        fflush(stderr);
    } else if (pc == 0x80110770u) {
        uint32_t old_74 = 0, old_78 = 0, old_7c = 0;
        uint32_t s0_v = 0, s1_v = 0, s2_v = 0;
        if (p08_guest(a0)) {
            (void)r4300_read_aligned_word(r4300, a0 + 0x74u, &old_74);
            (void)r4300_read_aligned_word(r4300, a0 + 0x78u, &old_78);
            (void)r4300_read_aligned_word(r4300, a0 + 0x7Cu, &old_7c);
        }
        if (p08_guest(a1)) {
            (void)r4300_read_aligned_word(r4300, a1 + 0x0u, &s0_v);
            (void)r4300_read_aligned_word(r4300, a1 + 0x4u, &s1_v);
            (void)r4300_read_aligned_word(r4300, a1 + 0x8u, &s2_v);
        }
        s_p08_ud5_set74_s0 = a0;
        const unsigned uord = ++s_p08_ud5_set74_ord;
        fprintf(stderr,
                "[ud5-set74] phase=set74 enter glidx=%d ord=%u pc=80110770 ra=%08x "
                "s0=%08x src=%08x old_74=(%08x,%08x,%08x / %.4f,%.4f,%.4f) src=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, uord, ra, a0, a1,
                old_74, old_78, old_7c, p08_f32(old_74), p08_f32(old_78), p08_f32(old_7c),
                s0_v, s1_v, s2_v, p08_f32(s0_v), p08_f32(s1_v), p08_f32(s2_v));
        fflush(stderr);
    } else if (pc == 0x80110780u) {
        uint32_t new_74 = 0, new_78 = 0, new_7c = 0;
        if (p08_guest(s_p08_ud5_set74_s0)) {
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_set74_s0 + 0x74u, &new_74);
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_set74_s0 + 0x78u, &new_78);
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_set74_s0 + 0x7Cu, &new_7c);
        }
        fprintf(stderr,
                "[ud5-set74] phase=set74 leave glidx=%d ord=%u pc=80110770 "
                "s0=%08x new_74=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, s_p08_ud5_set74_ord, s_p08_ud5_set74_s0,
                new_74, new_78, new_7c, p08_f32(new_74), p08_f32(new_78), p08_f32(new_7c));
        fflush(stderr);
    } else if (pc == 0x80112524u) {
        uint32_t p30 = 0, p30_4 = 0, p30_8 = 0, p30_c = 0;
        if (p08_guest(a0)) {
            (void)r4300_read_aligned_word(r4300, a0 + 0x30u, &p30);
            if (p08_guest(p30)) {
                (void)r4300_read_aligned_word(r4300, p30 + 0x4u, &p30_4);
                (void)r4300_read_aligned_word(r4300, p30 + 0x8u, &p30_8);
                (void)r4300_read_aligned_word(r4300, p30 + 0xCu, &p30_c);
            }
        }
        s_p08_ud5_sp6c_prod_dst = a1;
        const unsigned uord = ++s_p08_ud5_sp6c_prod_ord;
        fprintf(stderr,
                "[ud5-sp6c-prod] phase=sp6c_prod enter glidx=%d ord=%u pc=80112524 ra=%08x "
                "s0=%08x dst=%08x p30=%08x src4=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, uord, ra, a0, a1, p30,
                p30_4, p30_8, p30_c, p08_f32(p30_4), p08_f32(p30_8), p08_f32(p30_c));
        fflush(stderr);
    } else if (pc == 0x80112548u) {
        uint32_t d0 = 0, d1 = 0, d2 = 0;
        if (p08_guest(s_p08_ud5_sp6c_prod_dst)) {
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_sp6c_prod_dst + 0x0u, &d0);
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_sp6c_prod_dst + 0x4u, &d1);
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_sp6c_prod_dst + 0x8u, &d2);
        }
        fprintf(stderr,
                "[ud5-sp6c-prod] phase=sp6c_prod leave glidx=%d ord=%u pc=80112524 "
                "dst=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, s_p08_ud5_sp6c_prod_ord,
                d0, d1, d2, p08_f32(d0), p08_f32(d1), p08_f32(d2));
        fflush(stderr);
    } else if (pc == 0x800F18FCu) {
        uint32_t f0 = 0, f1 = 0, f2 = 0, t0 = 0, t1 = 0, t2 = 0;
        if (p08_guest(a0)) {
            (void)r4300_read_aligned_word(r4300, a0 + 0x0u, &f0);
            (void)r4300_read_aligned_word(r4300, a0 + 0x4u, &f1);
            (void)r4300_read_aligned_word(r4300, a0 + 0x8u, &f2);
        }
        if (p08_guest(a1)) {
            (void)r4300_read_aligned_word(r4300, a1 + 0x0u, &t0);
            (void)r4300_read_aligned_word(r4300, a1 + 0x4u, &t1);
            (void)r4300_read_aligned_word(r4300, a1 + 0x8u, &t2);
        }
        s_p08_ud5_f18fc_out = a2;
        const unsigned uord = ++s_p08_ud5_f18fc_ord;
        fprintf(stderr,
                "[ud5-f18fc] phase=f18fc enter glidx=%d ord=%u pc=800f18fc ra=%08x "
                "from=%08x to=%08x out=%08x from_v=(%08x,%08x,%08x / %.4f,%.4f,%.4f) to_v=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, uord, ra, a0, a1, a2,
                f0, f1, f2, p08_f32(f0), p08_f32(f1), p08_f32(f2),
                t0, t1, t2, p08_f32(t0), p08_f32(t1), p08_f32(t2));
        fflush(stderr);
    } else if (pc == 0x800F1980u) {
        uint32_t o0 = 0, o1 = 0, o2 = 0;
        if (p08_guest(s_p08_ud5_f18fc_out)) {
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_f18fc_out + 0x0u, &o0);
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_f18fc_out + 0x4u, &o1);
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_f18fc_out + 0x8u, &o2);
        }
        fprintf(stderr,
                "[ud5-f18fc] phase=f18fc leave glidx=%d ord=%u pc=800f18fc "
                "out=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, s_p08_ud5_f18fc_ord,
                o0, o1, o2, p08_f32(o0), p08_f32(o1), p08_f32(o2));
        fflush(stderr);
    } else if (pc == 0x800A516Cu) {
        uint32_t t0 = 0, t1 = 0, t2 = 0, e0 = 0, e1 = 0, e2 = 0;
        if (p08_guest(a1)) {
            (void)r4300_read_aligned_word(r4300, a1 + 0x0u, &t0);
            (void)r4300_read_aligned_word(r4300, a1 + 0x4u, &t1);
            (void)r4300_read_aligned_word(r4300, a1 + 0x8u, &t2);
        }
        if (p08_guest(a2)) {
            (void)r4300_read_aligned_word(r4300, a2 + 0x0u, &e0);
            (void)r4300_read_aligned_word(r4300, a2 + 0x4u, &e1);
            (void)r4300_read_aligned_word(r4300, a2 + 0x8u, &e2);
        }
        s_p08_ud5_a516c_dst = a0;
        const unsigned uord = ++s_p08_ud5_a516c_ord;
        fprintf(stderr,
                "[ud5-a516c] phase=a516c enter glidx=%d ord=%u pc=800a516c ra=%08x "
                "dst=%08x target=%08x eye=%08x target_v=(%08x,%08x,%08x / %.4f,%.4f,%.4f) eye_v=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, uord, ra, a0, a1, a2,
                t0, t1, t2, p08_f32(t0), p08_f32(t1), p08_f32(t2),
                e0, e1, e2, p08_f32(e0), p08_f32(e1), p08_f32(e2));
        fflush(stderr);
    } else if (pc == 0x800A51B4u) {
        uint32_t o0 = 0, o1 = 0, o2 = 0;
        if (p08_guest(s_p08_ud5_a516c_dst)) {
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_a516c_dst + 0x0u, &o0);
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_a516c_dst + 0x4u, &o1);
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_a516c_dst + 0x8u, &o2);
        }
        fprintf(stderr,
                "[ud5-a516c] phase=a516c leave glidx=%d ord=%u pc=800a516c "
                "out=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, s_p08_ud5_a516c_ord,
                o0, o1, o2, p08_f32(o0), p08_f32(o1), p08_f32(o2));
        fflush(stderr);
    } else if (pc == 0x80110A68u) {
        uint32_t sp6c_0 = 0, sp6c_1 = 0, sp6c_2 = 0;
        uint32_t s0_74 = 0, s0_78 = 0, s0_7c = 0;
        uint32_t p30 = 0;
        if (p08_guest(a2)) {
            (void)r4300_read_aligned_word(r4300, a2 + 0x0u, &sp6c_0);
            (void)r4300_read_aligned_word(r4300, a2 + 0x4u, &sp6c_1);
            (void)r4300_read_aligned_word(r4300, a2 + 0x8u, &sp6c_2);
        }
        if (p08_guest(a0)) {
            (void)r4300_read_aligned_word(r4300, a0 + 0x74u, &s0_74);
            (void)r4300_read_aligned_word(r4300, a0 + 0x78u, &s0_78);
            (void)r4300_read_aligned_word(r4300, a0 + 0x7Cu, &s0_7c);
            (void)r4300_read_aligned_word(r4300, a0 + 0x30u, &p30);
        }
        s_p08_ud5_a68_dst = a1;
        const unsigned uord = ++s_p08_ud5_a68_ord;
        fprintf(stderr,
                "[ud5-a68] phase=a68 enter glidx=%d ord=%u pc=80110a68 ra=%08x "
                "s0=%08x dst_sp60=%08x in_sp6c=%08x sp6c=(%08x,%08x,%08x / %.4f,%.4f,%.4f) s0_74=(%08x,%08x,%08x / %.4f,%.4f,%.4f) p30=%08x\n",
                glidx, uord, ra, a0, a1, a2,
                sp6c_0, sp6c_1, sp6c_2, p08_f32(sp6c_0), p08_f32(sp6c_1), p08_f32(sp6c_2),
                s0_74, s0_78, s0_7c, p08_f32(s0_74), p08_f32(s0_78), p08_f32(s0_7c),
                p30);
        fflush(stderr);
    } else if (pc == 0x80110A94u) {
        uint32_t out_80 = 0, out_84 = 0, out_88 = 0;
        if (p08_guest(s_p08_ud5_a68_dst)) {
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_a68_dst + 0x0u, &out_80);
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_a68_dst + 0x4u, &out_84);
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_a68_dst + 0x8u, &out_88);
        }
        fprintf(stderr,
                "[ud5-a68] phase=a68 leave glidx=%d ord=%u pc=80110a68 "
                "out_sp60=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, s_p08_ud5_a68_ord,
                out_80, out_84, out_88,
                p08_f32(out_80), p08_f32(out_84), p08_f32(out_88));
        fflush(stderr);
    }
}

static uint32_t s_p08_ud5_eye_s0;
static unsigned s_p08_ud5_fd4_ord;
static unsigned s_p08_ud5_ee780_ord;
static unsigned s_p08_ud5_efb24_ord;
static unsigned s_p08_ud5_eebf0_ord;
static uint32_t s_p08_ud5_ee780_dst;
static uint32_t s_p08_ud5_ee780_glidx;

static inline void p08_rd3(struct r4300_core* r4300, uint32_t a, uint32_t* x, uint32_t* y, uint32_t* z)
{
    *x = *y = *z = 0;
    if (p08_guest(a)) {
        (void)r4300_read_aligned_word(r4300, a + 0u, x);
        (void)r4300_read_aligned_word(r4300, a + 4u, y);
        (void)r4300_read_aligned_word(r4300, a + 8u, z);
    }
}

static inline void p08_ud5_eye(struct r4300_core* r4300, uint32_t pc)
{
    int glidx = 0;
    if (!p08_ud5_cam1c_window(r4300, &glidx)) {
        return;
    }
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t a0 = (uint32_t)regs[4];
    const uint32_t a1 = (uint32_t)regs[5];
    const uint32_t a2 = (uint32_t)regs[6];
    const uint32_t a3 = (uint32_t)regs[7];
    const uint32_t ra = (uint32_t)regs[31];
    uint32_t x = 0, y = 0, z = 0, t0 = 0, t1 = 0, t2 = 0;
    if (pc == 0x80111FD4u) {
        uint32_t ax = 0, ay = 0, az = 0;
        s_p08_ud5_eye_s0 = a0;
        p08_rd3(r4300, a0 + 0x74u, &x, &y, &z);
        p08_rd3(r4300, a0 + 0x8Cu, &t0, &t1, &t2);
        p08_rd3(r4300, a1, &ax, &ay, &az);
        fprintf(stderr,
                "[ud5-eye] phase=P0 enter glidx=%d ord=%u pc=80111fd4 ra=%08x s0=%08x arg=%08x "
                "eye74=(%08x,%08x,%08x / %.4f,%.4f,%.4f) s08c=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                "argv=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, ++s_p08_ud5_fd4_ord, ra, a0, a1,
                x, y, z, p08_f32(x), p08_f32(y), p08_f32(z),
                t0, t1, t2, p08_f32(t0), p08_f32(t1), p08_f32(t2),
                ax, ay, az, p08_f32(ax), p08_f32(ay), p08_f32(az));
        fflush(stderr);
        if (s_p08_ud5_ee780_dst != 0 && a1 == s_p08_ud5_ee780_dst) {
            fprintf(stderr,
                    "[ud5-ee780] edge_to_11fd4 glidx=%d dest=%08x out=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                    glidx, a1, ax, ay, az, p08_f32(ax), p08_f32(ay), p08_f32(az));
            fflush(stderr);
        }
    } else if (pc == 0x80111FFCu) {
        const uint32_t sp = (uint32_t)regs[29];
        p08_rd3(r4300, sp + 0x3Cu, &x, &y, &z);
        fprintf(stderr,
                "[ud5-eye] phase=P1 after_107F0 glidx=%d pc=80111ffc work=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, x, y, z, p08_f32(x), p08_f32(y), p08_f32(z));
        fflush(stderr);
    } else if (pc == 0x80112004u) {
        fprintf(stderr,
                "[ud5-eye] phase=BR_0FFD0 glidx=%d pc=80112004 v0=%08x skip_trans_path=%d\n",
                glidx, (uint32_t)regs[2], (regs[2] != 0) ? 1 : 0);
        fflush(stderr);
    } else if (pc == 0x80111F70u) {
        uint32_t sc = 0, ax = 0, ay = 0, az = 0;
        p08_rd3(r4300, a2, &x, &y, &z);
        p08_rd3(r4300, a1, &ax, &ay, &az);
        if (p08_guest(a3)) {
            (void)r4300_read_aligned_word(r4300, a3, &sc);
        }
        fprintf(stderr,
                "[ud5-eye] phase=P2 before_11F70 glidx=%d pc=80111f70 ra=%08x s0=%08x arg=%08x "
                "work=%08x a3=%08x a3_0=%08x workv=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                "argv=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, ra, a0, a1, a2, a3, sc, x, y, z, p08_f32(x), p08_f32(y), p08_f32(z),
                ax, ay, az, p08_f32(ax), p08_f32(ay), p08_f32(az));
        fflush(stderr);
    } else if (pc == 0x80111FC4u) {
        const uint32_t work = (uint32_t)regs[29] + 0x30u; /* 11F70 saved a2 at 0x30(sp) */
        (void)work;
        fprintf(stderr, "[ud5-eye] phase=P3 after_11F70 glidx=%d pc=80111fc4\n", glidx);
        fflush(stderr);
    } else if (pc == 0x80111DB0u) {
        uint32_t sc = 0, thr = 0, ax = 0, ay = 0, az = 0;
        p08_rd3(r4300, a2, &x, &y, &z);
        p08_rd3(r4300, a1, &ax, &ay, &az);
        if (p08_guest(a3)) {
            (void)r4300_read_aligned_word(r4300, a3, &sc);
        }
        (void)r4300_read_aligned_word(r4300, 0x8012655Cu, &thr);
        fprintf(stderr,
                "[ud5-eye] phase=P4 before_11DB0 glidx=%d pc=80111db0 ra=%08x s0=%08x arg=%08x "
                "work=%08x a3=%08x a3_0=%08x D_8012655C=%08x workv=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                "argv=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, ra, a0, a1, a2, a3, sc, thr, x, y, z, p08_f32(x), p08_f32(y), p08_f32(z),
                ax, ay, az, p08_f32(ax), p08_f32(ay), p08_f32(az));
        fflush(stderr);
    } else if (pc == 0x80111DD4u) {
        uint32_t f0 = p08_fpr_bits(r4300, 0);
        uint32_t thr = 0;
        (void)r4300_read_aligned_word(r4300, 0x8012655Cu, &thr);
        fprintf(stderr,
                "[ud5-eye] phase=BR_EEF94 glidx=%d pc=80111dd4 f0=%08x/%.4f D_8012655C=%08x/%.4f "
                "c_lt_thr_f0=%d\n",
                glidx, f0, p08_f32(f0), thr, p08_f32(thr),
                (p08_f32(thr) < p08_f32(f0)) ? 1 : 0);
        fflush(stderr);
    } else if (pc == 0x800EF174u) {
        p08_rd3(r4300, a0, &x, &y, &z);
        p08_rd3(r4300, a1, &t0, &t1, &t2);
        fprintf(stderr,
                "[ud5-eye] phase=P6 before_EF174 glidx=%d pc=800ef174 ra=%08x scale=%08x/%.4f "
                "eye=(%08x,%08x,%08x / %.4f,%.4f,%.4f) dir=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, ra, a2, p08_f32(a2),
                x, y, z, p08_f32(x), p08_f32(y), p08_f32(z),
                t0, t1, t2, p08_f32(t0), p08_f32(t1), p08_f32(t2));
        fflush(stderr);
    } else if (pc == 0x80111E64u) {
        p08_rd3(r4300, a0, &x, &y, &z);
        fprintf(stderr,
                "[ud5-eye] phase=P7 after_EF174 glidx=%d eye=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, x, y, z, p08_f32(x), p08_f32(y), p08_f32(z));
        fflush(stderr);
    } else if (pc == 0x80111E70u) {
        fprintf(stderr, "[ud5-eye] phase=BR_SKIP_EF174 glidx=%d pc=80111e70\n", glidx);
        fflush(stderr);
    } else if (pc == 0x80111E7Cu) {
        fprintf(stderr, "[ud5-eye] phase=P5 after_11DB0 glidx=%d pc=80111e7c\n", glidx);
        fflush(stderr);
    } else if (pc == 0x80112070u) {
        const uint32_t sp = (uint32_t)regs[29];
        p08_rd3(r4300, sp + 0x3Cu, &x, &y, &z);
        fprintf(stderr,
                "[ud5-eye] phase=P3 after_11F70 glidx=%d work=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, x, y, z, p08_f32(x), p08_f32(y), p08_f32(z));
        fflush(stderr);
    } else if (pc == 0x8011106Cu) {
        p08_rd3(r4300, a0 + 0x8Cu, &t0, &t1, &t2);
        fprintf(stderr,
                "[ud5-eye] phase=P8 before_1106C glidx=%d pc=8011106c ra=%08x s0=%08x dst=%08x "
                "s08c=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, ra, a0, a1, t0, t1, t2, p08_f32(t0), p08_f32(t1), p08_f32(t2));
        fflush(stderr);
    } else if (pc == 0x80111084u) {
        p08_rd3(r4300, a1, &x, &y, &z);
        fprintf(stderr,
                "[ud5-eye] phase=P9 after_1106C glidx=%d dst=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, x, y, z, p08_f32(x), p08_f32(y), p08_f32(z));
        fflush(stderr);
    } else if (pc == 0x800EF04Cu) {
        p08_rd3(r4300, a0, &x, &y, &z);
        p08_rd3(r4300, a1, &t0, &t1, &t2);
        fprintf(stderr,
                "[ud5-eye] phase=P10 before_EF04C glidx=%d pc=800ef04c "
                "work=(%08x,%08x,%08x / %.4f,%.4f,%.4f) add=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, x, y, z, p08_f32(x), p08_f32(y), p08_f32(z),
                t0, t1, t2, p08_f32(t0), p08_f32(t1), p08_f32(t2));
        fflush(stderr);
    } else if (pc == 0x80112088u) {
        const uint32_t sp = (uint32_t)regs[29];
        p08_rd3(r4300, sp + 0x3Cu, &x, &y, &z);
        p08_rd3(r4300, sp + 0x30u, &t0, &t1, &t2);
        fprintf(stderr,
                "[ud5-eye] phase=P11 after_EF04C glidx=%d work=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                "add=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, x, y, z, p08_f32(x), p08_f32(y), p08_f32(z),
                t0, t1, t2, p08_f32(t0), p08_f32(t1), p08_f32(t2));
        fflush(stderr);
    } else if (pc == 0x800EE7F8u) {
        if (s_p08_ud5_eye_s0 != 0 && a0 == s_p08_ud5_eye_s0 + 0x8Cu) {
            p08_rd3(r4300, a0, &x, &y, &z);
            p08_rd3(r4300, a1, &t0, &t1, &t2);
            fprintf(stderr,
                    "[ud5-eye] phase=s0_8c_write enter glidx=%d pc=800ee7f8 ra=%08x "
                    "old=(%08x,%08x,%08x / %.4f,%.4f,%.4f) srcv=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                    glidx, ra, x, y, z, p08_f32(x), p08_f32(y), p08_f32(z),
                    t0, t1, t2, p08_f32(t0), p08_f32(t1), p08_f32(t2));
            fflush(stderr);
        }
        if (glidx >= 22 && glidx <= 25) {
            uint32_t ox = 0, oy = 0, oz = 0, sx = 0, sy = 0, sz = 0;
            p08_rd3(r4300, a0, &ox, &oy, &oz);
            p08_rd3(r4300, a1, &sx, &sy, &sz);
            fprintf(stderr,
                    "[ud5-ee7f8] enter glidx=%d ra=%08x dst=%08x src=%08x "
                    "old=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                    "srcv=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                    glidx, ra, a0, a1,
                    ox, oy, oz, p08_f32(ox), p08_f32(oy), p08_f32(oz),
                    sx, sy, sz, p08_f32(sx), p08_f32(sy), p08_f32(sz));
            fflush(stderr);
        }
    } else if (pc == 0x800EE780u) {
        uint32_t ox = 0, oy = 0, oz = 0, s1x = 0, s1y = 0, s1z = 0, s2x = 0, s2y = 0, s2z = 0;
        const unsigned uord = ++s_p08_ud5_ee780_ord;
        s_p08_ud5_ee780_dst = a0;
        s_p08_ud5_ee780_glidx = (uint32_t)glidx;
        p08_rd3(r4300, a0, &ox, &oy, &oz);
        p08_rd3(r4300, a1, &s1x, &s1y, &s1z);
        p08_rd3(r4300, a2, &s2x, &s2y, &s2z);
        fprintf(stderr,
                "[ud5-ee780] enter glidx=%d ord=%u pc=800ee780 ra=%08x "
                "a0=%08x a1=%08x a2=%08x a3=%08x "
                "old=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                "src_a1=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                "src_a2=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, uord, ra, a0, a1, a2, a3,
                ox, oy, oz, p08_f32(ox), p08_f32(oy), p08_f32(oz),
                s1x, s1y, s1z, p08_f32(s1x), p08_f32(s1y), p08_f32(s1z),
                s2x, s2y, s2z, p08_f32(s2x), p08_f32(s2y), p08_f32(s2z));
        fflush(stderr);
    } else if (pc == 0x800EFB24u && glidx >= 22 && glidx <= 25) {
        uint32_t a1x = 0, a1y = 0, a1z = 0, a2x = 0, a2y = 0, a2z = 0;
        const unsigned uord = ++s_p08_ud5_efb24_ord;
        p08_rd3(r4300, a1, &a1x, &a1y, &a1z);
        p08_rd3(r4300, a2, &a2x, &a2y, &a2z);
        fprintf(stderr,
                "[ud5-efb24] enter glidx=%d ord=%u pc=800efb24 ra=%08x "
                "a0=%08x a1=%08x a2=%08x a3=%08x "
                "a1v=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                "a2v=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, uord, ra, a0, a1, a2, a3,
                a1x, a1y, a1z, p08_f32(a1x), p08_f32(a1y), p08_f32(a1z),
                a2x, a2y, a2z, p08_f32(a2x), p08_f32(a2y), p08_f32(a2z));
        fflush(stderr);
    } else if (pc == 0x800EEBF0u && glidx >= 22 && glidx <= 25) {
        uint32_t ox = 0, oy = 0, oz = 0, sx = 0, sy = 0, sz = 0;
        const unsigned uord = ++s_p08_ud5_eebf0_ord;
        p08_rd3(r4300, a0, &ox, &oy, &oz);
        p08_rd3(r4300, a1, &sx, &sy, &sz);
        fprintf(stderr,
                "[ud5-eebf0] enter glidx=%d ord=%u pc=800eebf0 ra=%08x "
                "a0=%08x a1=%08x a2=%08x/%.4f a3=%08x "
                "old=(%08x,%08x,%08x / %.4f,%.4f,%.4f) "
                "src_dir=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, uord, ra, a0, a1, a2, p08_f32(a2), a3,
                ox, oy, oz, p08_f32(ox), p08_f32(oy), p08_f32(oz),
                sx, sy, sz, p08_f32(sx), p08_f32(sy), p08_f32(sz));
        fflush(stderr);
    } else if (pc == 0x801107F0u) {
        p08_rd3(r4300, a1, &x, &y, &z);
        fprintf(stderr,
                "[ud5-eye] phase=P1 after_107F0 glidx=%d pc=801107f0 ra=%08x s0=%08x dst=%08x "
                "work=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, ra, a0, a1, x, y, z, p08_f32(x), p08_f32(y), p08_f32(z));
        fflush(stderr);
    } else if (pc == 0x80111DCCu) {
        p08_rd3(r4300, a0, &x, &y, &z);
        fprintf(stderr,
                "[ud5-efb24] leave glidx=%d ra=%08x dest=%08x "
                "delta=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, ra, a0, x, y, z, p08_f32(x), p08_f32(y), p08_f32(z));
        fflush(stderr);
    } else if (pc == 0x800EEC28u && glidx >= 22 && glidx <= 25) {
        p08_rd3(r4300, a0, &x, &y, &z);
        fprintf(stderr,
                "[ud5-eebf0] leave glidx=%d dest=%08x "
                "out=(%08x,%08x,%08x / %.4f,%.4f,%.4f)\n",
                glidx, a0, x, y, z, p08_f32(x), p08_f32(y), p08_f32(z));
        fflush(stderr);
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
        if (p08_ud4_walkstart_env() && glidx == 39 && !s_p08_np_trace) {
            s_p08_np_trace = 1;
            s_p08_np_stop = 0;
            s_p08_np_node = a0;
            s_p08_np_glidx = glidx;
            s_p08_np_ret = ra;
            s_p08_np_frame = (uint32_t)regs[29] - 0x78u;
            s_p08_np_call_n = 0;
            s_p08_np_insn = 0;
            s_p08_np_ovl_base = 0;
            s_p08_np_child_sp = 0;
            fprintf(fp, "[ud4-ncbapivot] TRACE_ON ret=%08x frame=%08x bind_tick=0 stop=0\n",
                    s_p08_np_ret, s_p08_np_frame);
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
    if (p08_ud5_on()) {
        if (mode != 5 || sub != 5) {
            return;
        }
    } else if (mode != 5 || (sub != 4 && sub != 0)) {
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
    if (p08_ud5_on()) {
        if (!s_p08_ud5) {
            const char* path = getenv("P08_UD5_CSV");
            if (path == NULL || path[0] == '\0') {
                path = "/tmp/p08-angrylion-user_demo5.csv";
            }
            s_p08_ud5 = fopen(path, "a");
            if (s_p08_ud5 && ftell(s_p08_ud5) == 0) {
                fprintf(s_p08_ud5,
                        "glidx,post,stage,sub,map,pad_x,pad_y,pad_btn,"
                        "bs_prev,bs_cur,bs_next,ptype,px,py,pz,"
                        "vx,vy,vz,tspd,accel,mult,yaw,ideal,phys_yaw,"
                        "camctrl,sel,anim_id,anim_timer,anim_dur,anim_phase\n");
            }
        }
        if (!s_p08_ud5) {
            return;
        }
    } else if (!s_p08_motion) {
        s_p08_motion = fopen("/tmp/p08-angrylion-user_demo4.csv", "a");
        if (s_p08_motion && ftell(s_p08_motion) == 0) {
            fprintf(s_p08_motion,
                    "sub,glidx,post,pad_x,pad_y,pad_btn,bs,px,py,pz,vx,vy,vz,yaw,ideal,phys_yaw,"
                    "cx,cy,cz,camctrl,sel\n");
        }
    }
    if (!p08_ud5_on() && !s_p08_motion) {
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
    float cx = 0, cy = 0, cz = 0, camctrl = 0, tspd = 0, accel = 0, mult = 0;
    int32_t bs = -1, sel = -1, bs_prev = -1, bs_cur = -1, bs_next = -1, ptype = -1;
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
            uint32_t wx, wy, wz, wyaw, wtspd, wacc, wmult, wtype;
            wx = wy = wz = wyaw = wtspd = wacc = wmult = wtype = 0;
            (void)r4300_read_aligned_word(r4300, physp + 0x00u, &wtype);
            (void)r4300_read_aligned_word(r4300, physp + 0x10u, &wx);
            (void)r4300_read_aligned_word(r4300, physp + 0x14u, &wy);
            (void)r4300_read_aligned_word(r4300, physp + 0x18u, &wz);
            (void)r4300_read_aligned_word(r4300, physp + 0x3Cu, &wtspd);
            (void)r4300_read_aligned_word(r4300, physp + 0x40u, &wyaw);
            (void)r4300_read_aligned_word(r4300, physp + 0x44u, &wacc);
            (void)r4300_read_aligned_word(r4300, physp + 0x50u, &wmult);
            vx = p08_f32(wx); vy = p08_f32(wy); vz = p08_f32(wz); phys = p08_f32(wyaw);
            tspd = p08_f32(wtspd); accel = p08_f32(wacc); mult = p08_f32(wmult);
            ptype = (int32_t)wtype;
        }
        if ((bsp >= 0x80000000u)) {
            uint32_t b0 = 0, b1 = 0, b2 = 0;
            (void)r4300_read_aligned_word(r4300, bsp + 0x0u, &b0);
            (void)r4300_read_aligned_word(r4300, bsp + 0x4u, &b1);
            (void)r4300_read_aligned_word(r4300, bsp + 0x8u, &b2);
            bs = (int32_t)b0;
            bs_prev = (int32_t)b0;
            bs_cur = (int32_t)b1;
            bs_next = (int32_t)b2;
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
    int16_t anim_id = -1;
    float anim_timer = 0.0f, anim_dur = 0.0f, anim_phase = 0.0f;
    uint8_t anim_type = 0, anim_flags = 0;
    if ((ps_w >= 0x80000000u) && ((ps_w + 0x80000000u) < 0x00800000u)) {
        uint32_t anim_p = 0;
        (void)r4300_read_aligned_word(r4300, ps_w + 0x04u, &anim_p);
        if (anim_p >= 0x80000000u && ((anim_p + 0x80000000u) < 0x00800000u)) {
            uint32_t anctrl_p = 0;
            (void)r4300_read_aligned_word(r4300, anim_p + 0x00u, &anctrl_p);
            if (anctrl_p >= 0x80000000u && ((anctrl_p + 0x80000000u) < 0x00800000u)) {
                uint32_t w_dur = 0, w_flags = 0, w_timer = 0, w_id = 0;
                (void)r4300_read_aligned_word(r4300, anctrl_p + 0x0Cu, &w_dur);
                (void)r4300_read_aligned_word(r4300, anctrl_p + 0x18u, &w_flags);
                (void)r4300_read_aligned_word(r4300, anctrl_p + 0x2Cu, &w_timer);
                (void)r4300_read_aligned_word(r4300, anctrl_p + 0x34u, &w_id);
                anim_dur = p08_f32(w_dur);
                anim_timer = p08_f32(w_timer);
                anim_type = (uint8_t)((w_flags >> 8) & 0xFFu);
                anim_flags = (uint8_t)(w_flags & 0xFFu);
                anim_id = (int16_t)(w_id >> 16);
                anim_phase = (anim_dur > 0.0f) ? (anim_timer / anim_dur) : 0.0f;
            }
        }
    }
    (void)rec_w;
    if (p08_ud5_on() && s_p08_ud5) {
        fprintf(s_p08_ud5,
                "%d,%u,%d,%d,%d,%d,%d,0x%04x,"
                "%d,%d,%d,%d,%.6f,%.6f,%.6f,"
                "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,"
                "%.6f,%d,%d,%.6f,%.6f,%.6f\n",
                glidx, post, stage, sub, map, pad_x, pad_y, pad_btn,
                bs_prev, bs_cur, bs_next, ptype, px, py, pz,
                vx, vy, vz, tspd, accel, mult, yaw, ideal, phys,
                camctrl, sel, (int)anim_id, anim_timer, anim_dur, anim_phase);
        fflush(s_p08_ud5);
        fprintf(stderr,
                "[demo-sync] glidx=%d post=%u pad=(%d,%d,%04x) "
                "bs_prev=%d bs_cur=%d bs_next=%d ptype=%d "
                "pos=(%.2f,%.2f,%.2f) vel=(%.2f,%.2f,%.2f) tspd=%.2f accel=%.2f "
                "yaw=%.2f ideal=%.2f phys_yaw=%.2f camctrl=%.4f "
                "anim=(%d,%.3f,%.3f) sub=%d map=%d\n",
                glidx, post, pad_x, pad_y, pad_btn, bs_prev, bs_cur, bs_next, ptype,
                px, py, pz, vx, vy, vz, tspd, accel, yaw, ideal, phys, camctrl,
                (int)anim_id, anim_timer, anim_dur, sub, map);
        fflush(stderr);
        if (sub == 5 && glidx >= 36) {
            exit(0);
        }
        return;
    }
    fprintf(s_p08_motion,
            "%d,%d,%u,%d,%d,0x%04x,%d,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,"
            "%.4f,%.4f,%.4f,%.4f,%d,%d,%.4f,%.4f,%u,%u,%.4f\n",
            sub, glidx, post, pad_x, pad_y, pad_btn, bs, px, py, pz, vx, vy, vz,
            yaw, ideal, phys, cx, cy, cz, camctrl, sel,
            (int)anim_id, anim_timer, anim_dur, (unsigned)anim_type, (unsigned)anim_flags, anim_phase);
    fflush(s_p08_motion);

    fprintf(stderr,
            "[demo-sync] glidx=%d post=%u pad=(%d,%d,%04x) bs=%d pos=(%.2f,%.2f,%.2f) "
            "vel=(%.2f,%.2f,%.2f) yaw=%.2f ideal=%.2f phys_yaw=%.2f cam=(%.2f,%.2f,%.2f) "
            "camctrl=%.4f sel=%d anim=(%d,%.3f,%.3f,%d,0x%02x,%.3f) sub=%d\n",
            glidx, post, pad_x, pad_y, pad_btn, bs, px, py, pz,
            vx, vy, vz, yaw, ideal, phys, cx, cy, cz,
            camctrl, sel, (int)anim_id, anim_timer, anim_dur, (int)anim_type, (unsigned)anim_flags, anim_phase, sub);
    fflush(stderr);
}

static inline void p08_ud5_dump_anctrl(struct r4300_core* r4300, const char* phase,
                                       int glidx, uint32_t extra0, uint32_t extra1)
{
    uint32_t ps = 0;
    int32_t bs_prev = -1, bs_cur = -1, bs_next = -1;
    uint32_t anctrl = 0, timer_bits = 0, dur_bits = 0, id_word = 0, flag_word = 0, rate_bits = 0;
    uint8_t playback = 0, flags = 0;
    int16_t anim_id = -1;
    int32_t zone = -1;
    uint32_t flag160 = 0, dist_bits = 0, stick_x_bits = 0, stick_y_bits = 0;
    (void)r4300_read_aligned_word(r4300, 0x80135490u, &ps);
    if (p08_guest(ps)) {
        uint32_t bsp = 0, stick = 0, anim_p = 0;
        (void)r4300_read_aligned_word(r4300, ps + 0x120u, &bsp);
        (void)r4300_read_aligned_word(r4300, ps + 0x160u, &flag160);
        (void)r4300_read_aligned_word(r4300, ps + 0x128u, &stick);
        (void)r4300_read_aligned_word(r4300, ps + 0x04u, &anim_p);
        if (p08_guest(bsp)) {
            uint32_t b0 = 0, b1 = 0, b2 = 0;
            (void)r4300_read_aligned_word(r4300, bsp + 0x0u, &b0);
            (void)r4300_read_aligned_word(r4300, bsp + 0x4u, &b1);
            (void)r4300_read_aligned_word(r4300, bsp + 0x8u, &b2);
            bs_prev = (int32_t)b0;
            bs_cur = (int32_t)b1;
            bs_next = (int32_t)b2;
        }
        if (p08_guest(stick)) {
            uint32_t zw = 0;
            (void)r4300_read_aligned_word(r4300, stick + 0x3Cu, &zw);
            (void)r4300_read_aligned_word(r4300, stick + 0x54u, &stick_x_bits);
            (void)r4300_read_aligned_word(r4300, stick + 0x58u, &stick_y_bits);
            (void)r4300_read_aligned_word(r4300, stick + 0x60u, &dist_bits);
            zone = (int32_t)zw;
        }
        if (p08_guest(anim_p)) {
            (void)r4300_read_aligned_word(r4300, anim_p + 0x00u, &anctrl);
            if (p08_guest(anctrl)) {
                (void)r4300_read_aligned_word(r4300, anctrl + 0x0Cu, &dur_bits);
                (void)r4300_read_aligned_word(r4300, anctrl + 0x10u, &rate_bits);
                (void)r4300_read_aligned_word(r4300, anctrl + 0x18u, &flag_word);
                (void)r4300_read_aligned_word(r4300, anctrl + 0x2Cu, &timer_bits);
                (void)r4300_read_aligned_word(r4300, anctrl + 0x34u, &id_word);
                playback = (uint8_t)((flag_word >> 8) & 0xFFu);
                flags = (uint8_t)((flag_word >> 0) & 0xFFu);
                anim_id = (int16_t)(id_word >> 16);
            }
        }
    }
    const float timer = p08_f32(timer_bits);
    const int isstopped = (playback == 3) ? 1 : 0;
    const int gt_half = (timer > 0.5f) ? 1 : 0;
    fprintf(stderr,
            "[ud5-gate] phase=%s glidx=%d bs_prev=%d bs_cur=%d bs_next=%d "
            "anim_id=%d anctrl=%08x timer_bits=%08x dur_bits=%08x rate_bits=%08x "
            "playback=%u flags=0x%02x flag_word=%08x id_word=%08x "
            "isstopped=%d gt_half=%d zone=%d flag160=%08x "
            "stick_x_bits=%08x stick_y_bits=%08x dist_bits=%08x extra0=%08x extra1=%08x\n",
            phase, glidx, bs_prev, bs_cur, bs_next, (int)anim_id, anctrl,
            timer_bits, dur_bits, rate_bits, (unsigned)playback, (unsigned)flags,
            flag_word, id_word, isstopped, gt_half, zone, flag160,
            stick_x_bits, stick_y_bits, dist_bits, extra0, extra1);
    fflush(stderr);
}

static inline void p08_ud5_gate(struct r4300_core* r4300, uint32_t pc)
{
    if (!p08_ud5_on()) {
        return;
    }
    int mode = 0, map = 0, stage = 0, sub = 0;
    uint32_t post = 0;
    p08_read_attract(r4300, &mode, &map, &stage, &sub, &post);
    if (mode == 5 && (sub == 1 || sub == 5) && sub >= 0 && sub < 8 &&
        (pc == 0x800CAF00u || pc == 0x800CAF2Cu) && s_p08_ud5_abc_pending[sub]) {
        const int abc_glidx = (post > 0u) ? (int)post - 1 : 0;
        const int64_t* aregs = r4300_regs(r4300);
        if (pc == 0x800CAF00u) {
            const uint32_t a0 = (uint32_t)aregs[4];
            s_p08_abc_stick = (uint32_t)aregs[5];
            s_p08_abc_cam = 0;
            if (p08_guest(a0)) {
                (void)r4300_read_aligned_word(r4300, a0 + 0x1Cu, &s_p08_abc_cam);
            }
        } else {
            s_p08_ud5_abc_pending[sub] = 0;
            fprintf(stderr,
                    "[ud5-abc] event=caf00 sub=%d trot_ord=%u glidx=%d "
                    "cam_plus_1c=%08x stick_ang=%08x f0_bits=%08x\n",
                    sub, s_p08_ud5_abc_trot[sub], abc_glidx, s_p08_abc_cam,
                    s_p08_abc_stick, p08_fpr_bits(r4300, 0));
            fflush(stderr);
        }
    }
    if (mode != 5 || sub != 5) {
        return;
    }
    const int glidx = (post > 0u) ? (int)post - 1 : 0;
    const int64_t* regs = r4300_regs(r4300);
    static unsigned p08_ud5_tspd_ord = 0;
    if (pc == 0x8009B9B0u || pc == 0x8009B9BCu) {
        const uint32_t a0 = (uint32_t)regs[4];
        const uint32_t source_bits = (uint32_t)regs[5];
        uint32_t phys = 0, old_bits = 0, new_bits = 0, bsp = 0;
        int32_t bs_prev = -1, bs_cur = -1, bs_next = -1;
        if (p08_guest(a0)) {
            (void)r4300_read_aligned_word(r4300, a0 + 0xC8u, &phys);
            (void)r4300_read_aligned_word(r4300, a0 + 0x120u, &bsp);
            if (p08_guest(phys)) {
                (void)r4300_read_aligned_word(r4300, phys + 0x3Cu, &old_bits);
                (void)r4300_read_aligned_word(r4300, phys + 0x3Cu, &new_bits);
            }
            if (p08_guest(bsp)) {
                (void)r4300_read_aligned_word(r4300, bsp + 0x00u, (uint32_t*)&bs_prev);
                (void)r4300_read_aligned_word(r4300, bsp + 0x04u, (uint32_t*)&bs_cur);
                (void)r4300_read_aligned_word(r4300, bsp + 0x08u, (uint32_t*)&bs_next);
            }
        }
        const unsigned ord = (pc == 0x8009B9B0u) ? ++p08_ud5_tspd_ord : p08_ud5_tspd_ord;
        fprintf(stderr,
                "[ud5-tspd] %s glidx=%d ord=%u pc=%08x store=8009b9bc ra=%08x "
                "a0=%08x phys=%08x state=%d>%d next=%d old_bits=%08x "
                "source_bits=%08x new_bits=%08x\n",
                (pc == 0x8009B9B0u) ? "enter" : "store", glidx, ord, pc,
                (uint32_t)regs[31], a0, phys, bs_prev, bs_cur, bs_next,
                old_bits, source_bits, new_bits);
        fflush(stderr);
        return;
    }
    if (glidx >= 20 && glidx <= 50) {
        static unsigned p08_ud5_rel_ord = 0;
        static unsigned p08_ud5_caf_ord = 0;
        static unsigned p08_ud5_yawideal_ord = 0;
        static unsigned p08_ud5_tyaw_ord = 0;
        static unsigned p08_ud5_hvel_ord = 0;
        static unsigned p08_ud5_d088_ord = 0;
        static unsigned p08_ud5_eeb_ord = 0;
        if (pc == 0x8009D088u) {
            const uint32_t a0 = (uint32_t)regs[4];
            uint32_t ctrl = 0, ytype = 0xffffffffu, stick = 0, ang = 0, dist = 0, zone = 0;
            uint32_t bsp = 0;
            int32_t bs_cur = -1;
            uint32_t cam_bits = 0, selw = 0, rec = 0;
            if (p08_guest(a0)) {
                (void)r4300_read_aligned_word(r4300, a0 + 0xFCu, &ctrl);
                (void)r4300_read_aligned_word(r4300, a0 + 0x128u, &stick);
                (void)r4300_read_aligned_word(r4300, a0 + 0x120u, &bsp);
                (void)r4300_read_aligned_word(r4300, a0 + 0x184u, &selw);
                if (p08_guest(ctrl)) {
                    (void)r4300_read_aligned_word(r4300, ctrl, &ytype);
                }
                if (p08_guest(stick)) {
                    (void)r4300_read_aligned_word(r4300, stick + 0x5Cu, &ang);
                    (void)r4300_read_aligned_word(r4300, stick + 0x60u, &dist);
                    (void)r4300_read_aligned_word(r4300, stick + 0x3Cu, &zone);
                }
                if (p08_guest(bsp)) {
                    (void)r4300_read_aligned_word(r4300, bsp + 0x4u, (uint32_t*)&bs_cur);
                }
                if ((int32_t)selw >= 0 && (int32_t)selw < 8) {
                    (void)r4300_read_aligned_word(r4300, 0x80127708u + selw * 4u, &rec);
                    if (p08_guest(rec)) {
                        (void)r4300_read_aligned_word(r4300, rec + 0x1Cu, &cam_bits);
                    }
                }
            }
            fprintf(stderr,
                    "[ud5-dir] phase=yaw_ctrl enter glidx=%d ord=%u pc=8009d088 ra=%08x "
                    "ytype=%d bs_cur=%d camctrl_bits=%08x stick_ang=%08x dist=%08x zone=%d\n",
                    glidx, ++p08_ud5_d088_ord, (uint32_t)regs[31], (int)ytype, bs_cur,
                    cam_bits, ang, dist, (int)zone);
            fflush(stderr);
            return;
        }
        if (pc == 0x8009EEB8u) {
            const uint32_t a0 = (uint32_t)regs[4];
            uint32_t stick = 0, ang = 0, sx = 0, sy = 0, dist = 0, zone = 0;
            uint32_t cam_bits = 0, selw = 0, rec = 0;
            if (p08_guest(a0)) {
                (void)r4300_read_aligned_word(r4300, a0 + 0x128u, &stick);
                (void)r4300_read_aligned_word(r4300, a0 + 0x184u, &selw);
                if (p08_guest(stick)) {
                    (void)r4300_read_aligned_word(r4300, stick + 0x54u, &sx);
                    (void)r4300_read_aligned_word(r4300, stick + 0x58u, &sy);
                    (void)r4300_read_aligned_word(r4300, stick + 0x5Cu, &ang);
                    (void)r4300_read_aligned_word(r4300, stick + 0x60u, &dist);
                    (void)r4300_read_aligned_word(r4300, stick + 0x3Cu, &zone);
                }
                if ((int32_t)selw >= 0 && (int32_t)selw < 8) {
                    (void)r4300_read_aligned_word(r4300, 0x80127708u + selw * 4u, &rec);
                    if (p08_guest(rec)) {
                        (void)r4300_read_aligned_word(r4300, rec + 0x1Cu, &cam_bits);
                    }
                }
            }
            fprintf(stderr,
                    "[ud5-dir] phase=rel_angle enter glidx=%d ord=%u pc=8009eeb8 ra=%08x "
                    "a0=%08x camctrl_bits=%08x stick_ang=%08x stick_xy=%08x,%08x dist=%08x zone=%d\n",
                    glidx, ++p08_ud5_rel_ord, (uint32_t)regs[31], a0, cam_bits, ang, sx, sy,
                    dist, (int)zone);
            fflush(stderr);
            return;
        }
        if (pc == 0x800CAF00u) {
            const uint32_t a0 = (uint32_t)regs[4];
            const uint32_t stick_bits = (uint32_t)regs[5];
            uint32_t cam_field = 0;
            if (p08_guest(a0)) {
                (void)r4300_read_aligned_word(r4300, a0 + 0x1Cu, &cam_field);
            }
            fprintf(stderr,
                    "[ud5-dir] phase=caf00 enter glidx=%d ord=%u pc=800caf00 ra=%08x "
                    "cam_obj=%08x cam_plus_1c=%08x stick_ang=%08x\n",
                    glidx, ++p08_ud5_caf_ord, (uint32_t)regs[31], a0, cam_field, stick_bits);
            fflush(stderr);
            return;
        }
        if (pc == 0x800CAF2Cu) {
            fprintf(stderr,
                    "[ud5-dir] phase=caf00 leave glidx=%d pc=800caf2c f0_bits=%08x\n",
                    glidx, p08_fpr_bits(r4300, 0));
            fflush(stderr);
            return;
        }
        if (pc == 0x8009C914u) {
            const uint32_t a0 = (uint32_t)regs[4];
            const uint32_t src_bits = (uint32_t)regs[5];
            uint32_t yawp = 0, old_ideal = 0;
            if (p08_guest(a0)) {
                (void)r4300_read_aligned_word(r4300, a0 + 0xF8u, &yawp);
                if (p08_guest(yawp)) {
                    (void)r4300_read_aligned_word(r4300, yawp + 0x4u, &old_ideal);
                }
            }
            fprintf(stderr,
                    "[ud5-dir] phase=yaw_setIdeal enter glidx=%d ord=%u pc=8009c914 "
                    "store=8009c930 ra=%08x a0=%08x src_bits=%08x old_ideal=%08x\n",
                    glidx, ++p08_ud5_yawideal_ord, (uint32_t)regs[31], a0, src_bits, old_ideal);
            fflush(stderr);
            return;
        }
        if (pc == 0x8009C930u) {
            fprintf(stderr,
                    "[ud5-dir] phase=yaw_setIdeal store glidx=%d pc=8009c930 f0_bits=%08x\n",
                    glidx, p08_fpr_bits(r4300, 0));
            fflush(stderr);
            return;
        }
        if (pc == 0x8009B9C0u) {
            const uint32_t a0 = (uint32_t)regs[4];
            const uint32_t src_bits = (uint32_t)regs[5];
            uint32_t phys = 0, old_bits = 0;
            if (p08_guest(a0)) {
                (void)r4300_read_aligned_word(r4300, a0 + 0xC8u, &phys);
                if (p08_guest(phys)) {
                    (void)r4300_read_aligned_word(r4300, phys + 0x40u, &old_bits);
                }
            }
            fprintf(stderr,
                    "[ud5-dir] phase=set_target_yaw enter glidx=%d ord=%u pc=8009b9c0 "
                    "store=8009b9dc ra=%08x a0=%08x phys=%08x src_bits=%08x old_bits=%08x\n",
                    glidx, ++p08_ud5_tyaw_ord, (uint32_t)regs[31], a0, phys, src_bits, old_bits);
            fflush(stderr);
            return;
        }
        if (pc == 0x8009BA68u) {
            fprintf(stderr,
                    "[ud5-dir] phase=set_hvel enter glidx=%d ord=%u pc=8009ba68 ra=%08x "
                    "yaw_bits=%08x spd_bits=%08x\n",
                    glidx, ++p08_ud5_hvel_ord, (uint32_t)regs[31],
                    (uint32_t)regs[5], (uint32_t)regs[6]);
            fflush(stderr);
            return;
        }
        if (pc == 0x800EEB9Cu) {
            const uint32_t dst = (uint32_t)regs[4];
            fprintf(stderr,
                    "[ud5-dir] phase=sincos_vel enter glidx=%d ord=%u pc=800eeb9c ra=%08x "
                    "dst=%08x yaw_bits=%08x spd_bits=%08x\n",
                    glidx, ++p08_ud5_eeb_ord, (uint32_t)regs[31], dst,
                    (uint32_t)regs[5], (uint32_t)regs[6]);
            fflush(stderr);
            return;
        }
    }
    if (pc == 0x80800B3Cu) {
        p08_ud5_dump_anctrl(r4300, "ep1-enter", glidx, (uint32_t)regs[4], (uint32_t)regs[5]);
        fprintf(stderr, "[ud5-gate] phase=order glidx=%d event=ep1-enter pc=%08x\n", glidx, pc);
        fflush(stderr);
        return;
    }
    if (pc == 0x80800B80u) {
        fprintf(stderr, "[ud5-gate] phase=isstopped glidx=%d v0=%d ra=%08x\n",
                glidx, (int)regs[2], (uint32_t)regs[31]);
        fflush(stderr);
        return;
    }
    if (pc == 0x80800B94u) {
        const uint32_t bits = p08_fpr_bits(r4300, 0);
        const int gt = (p08_f32(bits) > 0.5f) ? 1 : 0;
        fprintf(stderr, "[ud5-gate] phase=timer glidx=%d f0_bits=%08x gt_half=%d s0=%d\n",
                glidx, bits, gt, (int)regs[16]);
        fflush(stderr);
        return;
    }
    if (pc == 0x80800BA8u) {
        const uint32_t fcc = (r4300->cp1.fcr31 >> 23) & 1u;
        fprintf(stderr, "[ud5-gate] phase=timer-compare glidx=%d fcc=%u take_arbiter=%d fcr31=%08x\n",
                glidx, fcc, fcc ? 1 : 0, r4300->cp1.fcr31);
        fflush(stderr);
        return;
    }
    if (pc == 0x80800BB0u) {
        p08_ud5_dump_anctrl(r4300, "arbiter-call", glidx, (uint32_t)regs[4], (uint32_t)regs[16]);
        fprintf(stderr, "[ud5-gate] phase=arbiter-enter glidx=%d a0=%08x a1=%d\n",
                glidx, (uint32_t)regs[4], (int)regs[16]);
        fflush(stderr);
        return;
    }
    if (pc == 0x80800BB8u) {
        fprintf(stderr, "[ud5-gate] phase=arbiter-leave glidx=%d v0=%d s0=%d\n",
                glidx, (int)regs[2], (int)regs[16]);
        fflush(stderr);
        return;
    }
    if (pc == 0x808007E0u) {
        p08_ud5_dump_anctrl(r4300, "arbiter-fn", glidx, (uint32_t)regs[4], (uint32_t)regs[5]);
        fprintf(stderr, "[ud5-gate] phase=arbiter-fn glidx=%d a0=%08x a1=%d ra=%08x\n",
                glidx, (uint32_t)regs[4], (int)regs[5], (uint32_t)regs[31]);
        fflush(stderr);
        return;
    }
    if (pc == 0x8008C894u) {
        p08_ud5_dump_anctrl(r4300, "baanim-update-enter", glidx, (uint32_t)regs[4], 0);
        fprintf(stderr, "[ud5-gate] phase=order glidx=%d event=baanim-update\n", glidx);
        fflush(stderr);
        return;
    }
    if (pc == 0x8008ADE4u) {
        p08_ud5_dump_anctrl(r4300, "anctrl-update-enter", glidx, (uint32_t)regs[4], 0);
        fprintf(stderr, "[ud5-gate] phase=order glidx=%d event=anctrl-update\n", glidx);
        fflush(stderr);
    }
}

static inline void p08_ud5_setstate(struct r4300_core* r4300)
{
    if (!p08_ud5_on()) {
        return;
    }
    int mode = 0, map = 0, stage = 0, sub = 0;
    uint32_t post = 0;
    p08_read_attract(r4300, &mode, &map, &stage, &sub, &post);
    const int64_t* regs0 = r4300_regs(r4300);
    const int32_t next_state0 = (int32_t)regs0[5];
    if (mode == 5 && (sub == 1 || sub == 5) && next_state0 == 22 &&
        sub >= 0 && sub < 8) {
        const unsigned tord = ++s_p08_ud5_abc_trot[sub];
        s_p08_ud5_abc_pending[sub] = 1;
        fprintf(stderr,
                "[ud5-abc] event=state22 sub=%d trot_ord=%u glidx=%d map=%d\n",
                sub, tord, (post > 0u) ? (int)post - 1 : 0, map);
        fflush(stderr);
    }
    if (mode != 5 || sub != 5) {
        return;
    }
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t a0 = (uint32_t)regs[4];
    const int32_t next_state = (int32_t)regs[5];
    const uint32_t ra = (uint32_t)regs[31];
    int32_t prev = -1, cur = -1, nxt = -1;
    if (a0 >= 0x80000000u && ((a0 + 0x80000000u) < 0x00800000u)) {
        uint32_t bsp = 0;
        (void)r4300_read_aligned_word(r4300, a0 + 0x120u, &bsp);
        if (bsp >= 0x80000000u) {
            uint32_t b0 = 0, b1 = 0, b2 = 0;
            (void)r4300_read_aligned_word(r4300, bsp + 0x0u, &b0);
            (void)r4300_read_aligned_word(r4300, bsp + 0x4u, &b1);
            (void)r4300_read_aligned_word(r4300, bsp + 0x8u, &b2);
            prev = (int32_t)b0;
            cur = (int32_t)b1;
            nxt = (int32_t)b2;
        }
    }
    fprintf(stderr,
            "[ud5-setstate] enter glidx=%d ra=%08x a0=%08x next=%d "
            "before prev=%d cur=%d nextf=%d\n",
            (post > 0u) ? (int)post - 1 : 0, ra, a0, next_state, prev, cur, nxt);
    fflush(stderr);

    /* Fill the dense transition window with a pre-state snapshot.  The
     * syscall-698 hook records the post-tick view, but the state setter is
     * the only stable observation point for ticks where the interpreter
     * does not reach that hook.  This is diagnostic-only and intentionally
     * stops before glidx 34, whose post-transition row is retained. */
    const int state_glidx = (post > 0u) ? (int)post - 1 : 0;
    if (state_glidx >= 20 && state_glidx <= 33) {
        p08_motion_on_glrecord(r4300);
    }
}

static inline void p08_attract_egg_on_syscall(struct r4300_core* r4300, uint32_t op)
{
    const uint32_t code = (op >> 6) & 0xFFFFFu;
    const int interesting =
        (code == 126u) || (code == 129u) || (code >= 534u && code <= 540u) ||
        (code == 552u) || (code == 698u) || (code == 731u) || (code == 781u) ||
        (code == 29u);
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
    else if (code == 29u) spawn = " bsbtrot";
    fprintf(fp,
            "[egg-sys] pc=%08x ovl=%u ep=%u a0=%08x a1=%08x ra=%08x mode=%d map=%d "
            "stage=%d sub=%d post=%u%s",
            pc, code, ep, a0, a1, ra, mode, map, stage, sub, post, spawn);
    if ((code >= 534u && code <= 540u) || code == 126u) {
        p08_dump_egg_fields(r4300, fp, a0);
    }
    fprintf(fp, "\n");
    fflush(fp);
    if (p08_ud5_on() && code == 29u && ep == 1u && mode == 5 && sub == 5) {
        const int glidx = (post > 0u) ? (int)post - 1 : 0;
        p08_ud5_dump_anctrl(r4300, "ovl29-ep1-enter", glidx, a0, a1);
        fprintf(stderr, "[ud5-gate] phase=order glidx=%d event=ovl29-ep1-enter\n", glidx);
        fflush(stderr);
    }
}

#endif
