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
static uint32_t s_p08_enable_self = 0;
static uint32_t s_p08_enable_inp = 0;
static uint32_t s_p08_enable_in_id = 0;
static uint32_t s_p08_enable_en = 0;
static uint32_t s_p08_enable_old_w = 0;
static uint32_t s_p08_enable_ra = 0;
static uint32_t s_p08_isen_self = 0;
static uint32_t s_p08_isen_in_id = 0;
static uint32_t s_p08_isen_ra = 0;

static inline int p08_ud5_on(void)
{
    const char* e = getenv("P08_UD5_LOCO");
    if (e != NULL && e[0] != '\0' && e[0] != '0') return 1;
    const char* e2 = getenv("TOOIE_UD5_LOCO");
    if (e2 != NULL && e2[0] != '\0' && e2[0] != '0') return 1;
    return 0;
}

static inline int p08_ud5_cam1c_window(struct r4300_core* r4300, int* glidx_out);

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
    p08_read_attract(r4300, &mode, &map, &stage, &sub, &post);
    const int ud5_window = (mode == 5 && sub == 5 && glidx >= 22 && glidx <= 24);
    if (!ud5_window && (glidx < 36 || glidx > 42)) {
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
    if (mode == 5 && sub == 5 && glidx >= 22 && glidx <= 24) {
        return 1;
    }
    return s_p08_seen_haunted;
}

static inline void p08_ud4_writer(struct r4300_core* r4300, uint32_t pc)
{
    int glidx = 0;
    int mode = 0, map = 0, stage = 0, sub = 0;
    uint32_t post = 0;
    const int win4 = p08_ud4_writer_window(r4300, &glidx);
    const int win5 = p08_ud5_cam1c_window(r4300, &glidx);
    if (!win4 && !win5) {
        return;
    }
    FILE* fp = (win5 ? stderr : (win4 ? p08_ud4_fp() : stderr));
    if (!fp) {
        fp = stderr;
    }
    p08_read_attract(r4300, &mode, &map, &stage, &sub, &post);
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t a0 = (uint32_t)regs[4];
    const uint32_t a1 = (uint32_t)regs[5];
    const uint32_t a2 = (uint32_t)regs[6];
    const uint32_t a3 = (uint32_t)regs[7];
    const uint32_t ra = (uint32_t)regs[31];
    const uint32_t seq = ++s_p08_ud4_seq;
    const char* tag = win4 ? "ud4" : "ud5";
    if (pc == 0x80112A84u) {
        fprintf(fp,
                "[%s-p60-writer] enter seq=%u glidx=%d pc=80112A84 ra=%08x node=%08x",
                tag, seq, glidx, ra, a0);
        p08_ud4_dump_p60_object(r4300, fp, a0);
        fprintf(fp, "\n");
    } else if (pc == 0x80112D08u) {
        fprintf(fp,
                "[%s-p60-write] enter seq=%u glidx=%d pc=80112D08 ra=%08x"
                " node=%08x a1=%08x a2=%08x a3=%08x",
                tag, seq, glidx, ra, a0, a1, a2, a3);
        p08_ud4_dump_p60_object(r4300, fp, a0);
        fprintf(fp, "\n");
    } else if (pc == 0x80112D28u) {
        fprintf(fp, "[%s-p60-write] after-store seq=%u glidx=%d pc=80112D28 node=%08x",
                tag, seq, glidx, a0);
        p08_ud4_dump_p60_object(r4300, fp, a0);
        fprintf(fp, "\n");
    } else if (pc == 0x80112C34u) {
        fprintf(fp, "[%s-p60-writer] leave seq=%u glidx=%d pc=80112C34 node=%08x",
                tag, seq, glidx, a0);
        p08_ud4_dump_p60_object(r4300, fp, a0);
        fprintf(fp, "\n");
    } else if (pc == 0x8009811Cu) {
        fprintf(fp, "[ud5-enable-init] runtime=AN seq=%u glidx=%d self=%08x ra=%08x\n",
                seq, glidx, a0, ra);
        fflush(fp);
    } else if (pc == 0x80098140u) {
        s_p08_enable_self = a0;
        s_p08_enable_in_id = a1;
        s_p08_enable_en = a2;
        s_p08_enable_ra = ra;
        s_p08_enable_inp = 0;
        s_p08_enable_old_w = 0;
        (void)r4300_read_aligned_word(r4300, a0 + 0xA4u, &s_p08_enable_inp);
        if (p08_guest(s_p08_enable_inp)) {
            (void)r4300_read_aligned_word(r4300, s_p08_enable_inp, &s_p08_enable_old_w);
        }
    } else if (pc == 0x8009815Cu || pc == 0x80098180u) {
        uint32_t new_word = 0;
        if (p08_guest(s_p08_enable_inp)) {
            (void)r4300_read_aligned_word(r4300, s_p08_enable_inp, &new_word);
        }
        const uint32_t old_bit0 = s_p08_enable_old_w & 1u;
        const uint32_t new_bit0 = new_word & 1u;
        fprintf(fp,
                "[ud5-enable] runtime=AN seq=%u glidx=%d self=%08x inp_ptr=%08x in_id=%u en=%u old_w=%08x new_w=%08x bit0_old=%u bit0_new=%u ra=%08x\n",
                seq, glidx, s_p08_enable_self, s_p08_enable_inp, s_p08_enable_in_id, s_p08_enable_en,
                s_p08_enable_old_w, new_word, old_bit0, new_bit0, s_p08_enable_ra);
        fflush(fp);
    } else if (pc == 0x80098024u) {
        s_p08_isen_self = a0;
        s_p08_isen_in_id = a1;
        s_p08_isen_ra = ra;
    } else if (pc == 0x80098034u) {
        uint32_t en_w = 0, inp_ptr = 0;
        (void)r4300_read_aligned_word(r4300, s_p08_isen_self + 0xA4u, &inp_ptr);
        if (p08_guest(inp_ptr)) {
            (void)r4300_read_aligned_word(r4300, inp_ptr, &en_w);
        }
        const uint32_t ret_v0 = (uint32_t)regs[2];
        fprintf(fp,
                "[ud5-isenabled] runtime=AN seq=%u glidx=%d self=%08x in_id=%u en_w=%08x bit0=%u ret_v0=%08x ra=%08x\n",
                seq, glidx, s_p08_isen_self, s_p08_isen_in_id, en_w, en_w & 1u, ret_v0, s_p08_isen_ra);
        fflush(fp);
    } else if (pc == 0x800979D8u) {
        uint32_t key = 0, input = 0;
        (void)r4300_read_aligned_word(r4300, a0 + 0x40u, &key);
        (void)r4300_read_aligned_word(r4300, a0 + 0xA4u, &input);
        uint32_t state = 0, held_w = 0, in0 = 0, in4 = 0;
        uint32_t button_word = 0, enabled_word = 0;
        unsigned prev = 0, map = 0;
        (void)r4300_read_aligned_word(r4300, key + 0u, &state);
        if (p08_guest(key)) {
            (void)r4300_read_aligned_word(r4300, key + 0x18u + (a1 & ~3u), &held_w);
            prev = (held_w >> ((3u - ((0x18u + a1) & 3u)) * 8u)) & 0xFFu;
        }
        if (p08_guest(state)) {
            uint32_t map_w = 0;
            (void)r4300_read_aligned_word(r4300, state + a2 + 0x8u, &map_w);
            map = (map_w >> ((3u - ((a2 + 0x8u) & 3u)) * 8u)) & 0xFFu;
            (void)r4300_read_aligned_word(r4300, state + map * 56u + 36u + (a1 << 2), &button_word);
        }
        if (p08_guest(input)) {
            (void)r4300_read_aligned_word(r4300, input + 0u, &in0);
            (void)r4300_read_aligned_word(r4300, input + 4u, &in4);
            enabled_word = in0;
        }
        const uint32_t shift = (3u - ((0x18u + a1) & 3u)) * 8u;
        const uint32_t held = (held_w >> shift) & 0xFFu;
        fprintf(fp,
                "[%s-bainput-979d8] enter glidx=%d self=%08x btn=%u in_id=%u key=%08x state=%08x prev=%u map=%u button_word=%08x inp=%08x enabled_word=%08x held=%u\n",
                tag, glidx, a0, a1, a2, key, state, prev, map, button_word, input, enabled_word, held);
    } else if (pc == 0x80097A48u) {
        fprintf(fp, "[%s-bainput-979d8] leave glidx=%d v0=%08x\n",
                tag, glidx, (uint32_t)regs[2]);
    } else if (pc == 0x80097B70u) {
        fprintf(fp, "[ud5-cam-chain] enter 80097B70 (rotate_camera_left) glidx=%d self=%08x a1=%08x a2=%08x\n",
                glidx, a0, a1, a2);
    } else if (pc == 0x80097B90u) {
        fprintf(fp, "[ud5-cam-chain] leave 80097B70 (rotate_camera_left) glidx=%d v0=%08x\n",
                glidx, (uint32_t)regs[2]);
    } else if (pc == 0x80097B94u) {
        fprintf(fp, "[%s-cam-chain] enter 80097B94 (rotate_camera_right) glidx=%d self=%08x\n",
                tag, glidx, a0);
    } else if (pc == 0x80097BB4u) {
        fprintf(fp, "[%s-cam-chain] leave 80097B94 (rotate_camera_right) glidx=%d v0=%08x\n",
                tag, glidx, (uint32_t)regs[2]);
    } else if (pc == 0x800A4338u) {
        fprintf(fp, "[%s-cam-chain] enter 800A4338 glidx=%d a0=%08x a1=%08x a2=%08x\n",
                tag, glidx, a0, a1, a2);
    } else if (pc == 0x80110EFCu) {
        fprintf(fp, "[%s-cam-chain] enter 80110EFC glidx=%d node=%08x a1=%08x\n",
                tag, glidx, a0, a1);
    } else if (pc == 0x80110E00u) {
        fprintf(fp, "[%s-cam-chain] enter 80110E00 glidx=%d node=%08x a1=%08x\n",
                tag, glidx, a0, a1);
    } else if (pc == 0x801111D0u) {
        fprintf(fp, "[%s-cam-chain] enter 801111D0 glidx=%d node=%08x a1=%.4f (bits=%08x)\n",
                tag, glidx, a0, p08_f32(a1), a1);
    } else if (pc == 0x80112D90u) {
        fprintf(fp,
                "[%s-p60-writer-2d90] enter seq=%u glidx=%d pc=80112D90 ra=%08x node=%08x a1=%.4f a2=%.4f a3=%u",
                tag, seq, glidx, ra, a0, p08_f32(a1), p08_f32(a2), a3);
        p08_ud4_dump_p60_object(r4300, fp, a0);
        fprintf(fp, "\n");
    } else if (pc == 0x80112E4Cu) {
        fprintf(fp,
                "[%s-p60-writer-2d90] leave seq=%u glidx=%d pc=80112D90 node=%08x",
                tag, seq, glidx, a0);
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
static uint32_t s_p08_ud5_e1a58_a0;
static uint32_t s_p08_ud5_e1a58_a1;
static uint32_t s_p08_ud5_e1a58_ra;
static uint32_t s_p08_ud5_ca8b4_a0;
static uint32_t s_p08_ud5_ca8b4_a1;
static uint32_t s_p08_ud5_ca8b4_a2;
static unsigned s_p08_ud5_abc_trot[8];
static unsigned s_p08_ud5_abc_pending[8];

static uint32_t s_p08_ud5_src_mem_a;
static uint32_t s_p08_ud5_src_base_a;
static unsigned s_p08_ud5_cam_upd_ord;
static unsigned s_p08_ud5_efa4c_ord;
static unsigned s_p08_ud5_10970_ord;
static unsigned s_p08_ud5_fe34_ord;
static int s_p08_ud5_heading_glidx = -1;
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
    int glidx = (post > 0u) ? (int)post - 1 : 0;
    if (glidx >= 1000) {
        glidx -= 1012;
    }
    if (glidx_out) {
        *glidx_out = glidx;
    }
    return mode == 5 && sub == 5 && glidx >= 0 && glidx <= 28;
}

static inline void p08_ud5_watch_store(struct r4300_core* r4300, uint32_t pc, uint32_t op)
{
    const uint32_t major = (op >> 26) & 0x3Fu;
    if (major != 40 && major != 41 && major != 42 && major != 43 &&
        major != 44 && major != 45 && major != 46 && major != 57 &&
        major != 61 && major != 63) {
        return;
    }

    int glidx = 0;
    if (!p08_ud5_cam1c_window(r4300, &glidx) || glidx < 22 || glidx > 27) {
        return;
    }

    // Resolve target address
    uint32_t ps = 0, selw = 0;
    (void)r4300_read_aligned_word(r4300, 0x80135490u, &ps);
    if (!p08_guest(ps)) return;
    (void)r4300_read_aligned_word(r4300, ps + 0x184u, &selw);
    if (selw > 7) return;
    uint32_t rec = 0;
    (void)r4300_read_aligned_word(r4300, 0x80127708u + selw * 4u, &rec);
    const uint32_t target_addr = (p08_guest(rec)) ? (rec + 0x1Cu) : 0u;

    uint32_t node = 0;
    (void)r4300_read_aligned_word(r4300, 0x80127728u + selw * 4u, &node);
    const uint32_t node_target_addr = (p08_guest(node)) ? (node + 0x34u) : 0u;
    const uint32_t node3c_target_addr = (p08_guest(node)) ? (node + 0x3Cu) : 0u;

    const int64_t* regs = r4300_regs(r4300);
    const uint32_t base_reg = (op >> 21) & 0x1Fu;
    const int16_t imm = (int16_t)(op & 0xFFFFu);
    const uint32_t ea = (uint32_t)(regs[base_reg] + imm);
    const uint32_t rt = (op >> 16) & 0x1Fu;

    uint32_t store_size = 4;
    uint32_t src_reg_val = 0;
    switch (major) {
    case 40: store_size = 1; src_reg_val = (uint32_t)regs[rt] & 0xFF; break;
    case 41: store_size = 2; src_reg_val = (uint32_t)regs[rt] & 0xFFFF; break;
    case 42:
    case 46:
    case 43: store_size = 4; src_reg_val = (uint32_t)regs[rt]; break;
    case 44:
    case 45:
    case 63: store_size = 8; src_reg_val = (uint32_t)regs[rt]; break;
    case 57: store_size = 4; src_reg_val = p08_fpr_bits(r4300, rt); break;
    case 61: store_size = 8; src_reg_val = p08_fpr_bits(r4300, rt); break;
    }

    if (target_addr != 0 && ea <= target_addr && (ea + store_size) > target_addr) {
        uint32_t old_val = 0;
        (void)r4300_read_aligned_word(r4300, target_addr, &old_val);
        const uint32_t ra = (uint32_t)regs[31];
        fprintf(stderr,
                "[ud5-watch-write] runtime=AN glidx=%d pc=%08x ra=%08x ea=%08x target=%08x op_major=%u rt=%u store_size=%u old_val=%08x (%.6f) src_reg=%08x (%.6f)\n",
                glidx, pc, ra, ea, target_addr, major, rt, store_size, old_val, p08_f32(old_val), src_reg_val, p08_f32(src_reg_val));
        fflush(stderr);
    }

    if (node_target_addr != 0 && ea <= node_target_addr && (ea + store_size) > node_target_addr) {
        uint32_t old_val = 0;
        (void)r4300_read_aligned_word(r4300, node_target_addr, &old_val);
        const uint32_t ra = (uint32_t)regs[31];
        fprintf(stderr,
                "[ud5-watch-node34] runtime=AN glidx=%d pc=%08x ra=%08x ea=%08x node=%08x target=%08x op_major=%u rt=%u store_size=%u old_val=%08x (%.6f) src_reg=%08x (%.6f)\n",
                glidx, pc, ra, ea, node, node_target_addr, major, rt, store_size, old_val, p08_f32(old_val), src_reg_val, p08_f32(src_reg_val));
        fflush(stderr);
    }

    if (node3c_target_addr != 0 && ea <= node3c_target_addr && (ea + store_size) > node3c_target_addr) {
        uint32_t old_val = 0;
        (void)r4300_read_aligned_word(r4300, node3c_target_addr, &old_val);
        const uint32_t ra = (uint32_t)regs[31];
        fprintf(stderr,
                "[ud5-watch-node3c] runtime=AN glidx=%d pc=%08x ra=%08x ea=%08x node=%08x target=%08x op_major=%u rt=%u store_size=%u old_val=%08x (%.6f) src_reg=%08x (%.6f)\n",
                glidx, pc, ra, ea, node, node3c_target_addr, major, rt, store_size, old_val, p08_f32(old_val), src_reg_val, p08_f32(src_reg_val));
        fflush(stderr);
    }

    if (s_p08_ud5_src_mem_a != 0 && ea <= s_p08_ud5_src_mem_a && (ea + store_size) > s_p08_ud5_src_mem_a) {
        uint32_t old_val = 0;
        (void)r4300_read_aligned_word(r4300, s_p08_ud5_src_mem_a, &old_val);
        const uint32_t ra = (uint32_t)regs[31];
        fprintf(stderr,
                "[ud5-watch-src-mem-a] runtime=AN glidx=%d pc=%08x ra=%08x ea=%08x src_base=%08x src_mem_a=%08x op_major=%u rt=%u store_size=%u old_val=%08x (%.6f) src_reg=%08x (%.6f)\n",
                glidx, pc, ra, ea, s_p08_ud5_src_base_a, s_p08_ud5_src_mem_a, major, rt, store_size,
                old_val, p08_f32(old_val), src_reg_val, p08_f32(src_reg_val));
        fflush(stderr);
    }

    if (p08_guest(node) && ea >= node && ea < (node + 0x80u)) {
        uint32_t old_val = 0;
        (void)r4300_read_aligned_word(r4300, ea, &old_val);
        const uint32_t ra = (uint32_t)regs[31];
        fprintf(stderr,
                "[ud5-watch-node] runtime=AN glidx=%d pc=%08x ra=%08x ea=%08x (node+%02x) op_major=%u rt=%u old_val=%08x (%.6f) src_reg=%08x (%.6f)\n",
                glidx, pc, ra, ea, (ea - node), major, rt, old_val, p08_f32(old_val), src_reg_val, p08_f32(src_reg_val));
        fflush(stderr);
    }
}

static inline void p08_ud5_heading_reset_ords(int glidx)
{
    if (glidx != s_p08_ud5_heading_glidx) {
        s_p08_ud5_heading_glidx = glidx;
        s_p08_ud5_cam_upd_ord = 0;
        s_p08_ud5_efa4c_ord = 0;
        s_p08_ud5_10970_ord = 0;
        s_p08_ud5_fe34_ord = 0;
    }
}

static inline void p08_ud5_heading_prov(struct r4300_core* r4300, uint32_t pc)
{
    int glidx = 0;
    if (!p08_ud5_cam1c_window(r4300, &glidx) || glidx < 22 || glidx > 27) {
        return;
    }
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t a0 = (uint32_t)regs[4];
    const uint32_t a1 = (uint32_t)regs[5];
    const uint32_t a2 = (uint32_t)regs[6];
    const uint32_t a3 = (uint32_t)regs[7];
    const uint32_t ra = (uint32_t)regs[31];
    const uint32_t sp = (uint32_t)regs[29];
    const uint32_t s0 = (uint32_t)regs[16];
    p08_ud5_heading_reset_ords(glidx);

    if (pc == 0x800A9318u) {
        const unsigned ord = ++s_p08_ud5_cam_upd_ord;
        uint32_t bits = 0;
        if (p08_guest(s_p08_ud5_src_mem_a)) {
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_src_mem_a, &bits);
        }
        fprintf(stderr,
                "[ud5-heading] runtime=AN phase=cam-update-enter glidx=%d cam_upd=%u pc=800a9318 ra=%08x a0=%08x src_base=%08x src_mem_a=%08x src_bits=%08x (%.6f)\n",
                glidx, ord, ra, a0, s_p08_ud5_src_base_a, s_p08_ud5_src_mem_a, bits, p08_f32(bits));
        fflush(stderr);
        return;
    }
    if (pc == 0x8010FE34u) {
        const unsigned ord = ++s_p08_ud5_fe34_ord;
        uint32_t src_bits = 0, word20 = 0, half16 = 0;
        const uint32_t src_mem = a0 + 0x3Cu;
        if (p08_guest(a0)) {
            (void)r4300_read_aligned_word(r4300, src_mem, &src_bits);
            (void)r4300_read_aligned_word(r4300, a0 + 0x20u, &word20);
            (void)r4300_read_aligned_word(r4300, a0 + 0x14u, &half16);
            s_p08_ud5_src_base_a = a0;
            s_p08_ud5_src_mem_a = src_mem;
        }
        fprintf(stderr,
                "[ud5-heading] runtime=AN phase=fe34-enter glidx=%d cam_upd=%u fe34_ord=%u pc=8010fe34 ra=%08x sp=%08x a0=%08x a1=%08x a2=%08x a3=%08x src_base=%08x src_mem_a=%08x src_bits=%08x (%.6f) word20=%08x half16=%04x 10970_call_pc=8010fea4 source_load_pc=8010fea8\n",
                glidx, s_p08_ud5_cam_upd_ord, ord, ra, sp, a0, a1, a2, a3, a0, src_mem, src_bits, p08_f32(src_bits), word20, half16 & 0xFFFFu);
        fflush(stderr);
        return;
    }
    if (pc == 0x8010FEA4u || pc == 0x8010FEA8u) {
        uint32_t src_bits = 0;
        const uint32_t src_mem = s0 + 0x3Cu;
        if (p08_guest(s0)) {
            (void)r4300_read_aligned_word(r4300, src_mem, &src_bits);
            s_p08_ud5_src_base_a = s0;
            s_p08_ud5_src_mem_a = src_mem;
        }
        fprintf(stderr,
                "[ud5-heading] runtime=AN phase=fe34-load glidx=%d cam_upd=%u pc=%08x ra=%08x s0=%08x a0=%08x a1=%08x src_mem_a=%08x src_bits=%08x (%.6f)\n",
                glidx, s_p08_ud5_cam_upd_ord, pc, ra, s0, a0, a1, src_mem, src_bits, p08_f32(src_bits));
        fflush(stderr);
        return;
    }
    if (pc == 0x80110970u) {
        const unsigned ord = ++s_p08_ud5_10970_ord;
        fprintf(stderr,
                "[ud5-heading] runtime=AN phase=10970-enter glidx=%d cam_upd=%u ord=%u pc=80110970 ra=%08x sp=%08x a0=%08x a1=%08x (%.6f) a2=%08x a3=%08x heading_arg=a1 heading_arg_kind=REGISTER slot_after_prolog=sp+0x44 efa4c_call_pc=801109dc heading_def_pc=801109d8\n",
                glidx, s_p08_ud5_cam_upd_ord, ord, ra, sp, a0, a1, p08_f32(a1), a2, a3);
        fflush(stderr);
        return;
    }
    if (pc == 0x801109DCu) {
        fprintf(stderr,
                "[ud5-heading] runtime=AN phase=10970-jal-efa4c glidx=%d cam_upd=%u pc=801109dc ra=%08x sp=%08x a0=%08x a1=%08x (%.6f) a2=%08x (%.6f) a3=%08x\n",
                glidx, s_p08_ud5_cam_upd_ord, ra, sp, a0, a1, p08_f32(a1), a2, p08_f32(a2), a3);
        fflush(stderr);
        return;
    }
    if (pc == 0x800EFA4Cu) {
        const unsigned ord = ++s_p08_ud5_efa4c_ord;
        uint32_t dest_old4 = 0;
        if (p08_guest(a0)) {
            (void)r4300_read_aligned_word(r4300, a0 + 4u, &dest_old4);
        }
        fprintf(stderr,
                "[ud5-heading] runtime=AN phase=efa4c-enter glidx=%d cam_upd=%u efa4c_ord=%u pc=800efa4c ra=%08x sp=%08x a0=%08x a1=%08x (%.6f) a2=%08x (%.6f) a3=%08x dest_old4=%08x (%.6f) src_fpr=f14 src_fpr_defined_at=800efa50 heading_arg_gpr=a2\n",
                glidx, s_p08_ud5_cam_upd_ord, ord, ra, sp, a0, a1, p08_f32(a1), a2, p08_f32(a2), a3, dest_old4, p08_f32(dest_old4));
        fflush(stderr);
        return;
    }
    if (pc == 0x800EFA50u) {
        fprintf(stderr,
                "[ud5-heading] runtime=AN phase=efa4c-mtc1 glidx=%d cam_upd=%u pc=800efa50 a2=%08x (%.6f) f14=%08x (%.6f)\n",
                glidx, s_p08_ud5_cam_upd_ord, a2, p08_f32(a2), p08_fpr_bits(r4300, 14), p08_f32(p08_fpr_bits(r4300, 14)));
        fflush(stderr);
        return;
    }
    if (pc == 0x800EFA5Cu) {
        fprintf(stderr,
                "[ud5-heading] runtime=AN phase=efa4c-store glidx=%d cam_upd=%u pc=800efa5c a0=%08x dest_plus4=%08x f14=%08x (%.6f)\n",
                glidx, s_p08_ud5_cam_upd_ord, a0, a0 + 4u, p08_fpr_bits(r4300, 14), p08_f32(p08_fpr_bits(r4300, 14)));
        fflush(stderr);
        return;
    }
}

static inline void p08_ud5_fdec(struct r4300_core* r4300, uint32_t pc)
{
    int glidx = 0;
    if (!p08_ud5_cam1c_window(r4300, &glidx) || glidx < 22 || glidx > 27) {
        return;
    }
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t a0 = (uint32_t)regs[4];
    const uint32_t a1 = (uint32_t)regs[5];
    const uint32_t ra = (uint32_t)regs[31];
    uint32_t old_3c = 0, old_20 = 0;
    if (p08_guest(a0)) {
        (void)r4300_read_aligned_word(r4300, a0 + 0x3Cu, &old_3c);
        (void)r4300_read_aligned_word(r4300, a0 + 0x20u, &old_20);
    }
    uint32_t node = 0, ps = 0, selw = 0;
    (void)r4300_read_aligned_word(r4300, 0x80135490u, &ps);
    if (p08_guest(ps)) {
        (void)r4300_read_aligned_word(r4300, ps + 0x184u, &selw);
        if (selw <= 7) {
            (void)r4300_read_aligned_word(r4300, 0x80127728u + selw * 4u, &node);
        }
    }
    fprintf(stderr,
            "[ud5-fdec] runtime=AN glidx=%d pc=%08x ra=%08x a0=%08x (active_node=%08x is_active=%d) a1=%08x (%.6f) old_3c=%08x (%.6f) mode=%u\n",
            glidx, pc, ra, a0, node, (a0 == node) ? 1 : 0, a1, p08_f32(a1), old_3c, p08_f32(old_3c), (old_20 >> 24) & 0xFF);
    fflush(stderr);
}

static inline void p08_ud5_dump_node3c_checkpoint(struct r4300_core* r4300, const char* name, uint32_t pc)
{
    int glidx = 0;
    if (!p08_ud5_cam1c_window(r4300, &glidx) || glidx < 22 || glidx > 27) {
        return;
    }
    uint32_t node = 0, ps = 0, selw = 0;
    (void)r4300_read_aligned_word(r4300, 0x80135490u, &ps);
    if (p08_guest(ps)) {
        (void)r4300_read_aligned_word(r4300, ps + 0x184u, &selw);
        if (selw <= 7) {
            (void)r4300_read_aligned_word(r4300, 0x80127728u + selw * 4u, &node);
        }
    }
    uint32_t node3c = 0, node34 = 0, node20 = 0;
    if (p08_guest(node)) {
        (void)r4300_read_aligned_word(r4300, node + 0x3Cu, &node3c);
        (void)r4300_read_aligned_word(r4300, node + 0x34u, &node34);
        (void)r4300_read_aligned_word(r4300, node + 0x20u, &node20);
    }
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t ra = (uint32_t)regs[31];
    fprintf(stderr,
            "[ud5-node3c-checkpoint] runtime=AN glidx=%d name=%s pc=%08x ra=%08x node=%08x "
            "node3c=%08x (%.6f) node34=%08x (%.6f) mode=%u\n",
            glidx, name, pc, ra, node, node3c, p08_f32(node3c), node34, p08_f32(node34), (node20 >> 24) & 0xFF);
    fflush(stderr);
}

// === USER_DEMO_5 MAP=296 CLEAN SEMANTIC JOIN & CALL-PRESENCE MODULE ===
static uint32_t s_ud5_trace_epoch = 0;
static uint32_t s_ud5_map296_epoch = 0;
static int s_ud5_in_map296 = 0;
static uint64_t s_ud5_logic_tick = 0;
static uint32_t s_ud5_cam_update_ord = 0;
static uint32_t s_ud5_cam_call_seq = 0;
static int s_ud5_last_logged_rec_ord = -1;

static inline void p08_ud5_dump_checkpoint(struct r4300_core* r4300, const char* phase)
{
    if (!s_ud5_in_map296) return;
    uint32_t cam_obj = 0, cam_sub = 0, node = 0;
    (void)r4300_read_aligned_word(r4300, 0x80127728u, &cam_obj);
    if (p08_guest(cam_obj)) {
        (void)r4300_read_aligned_word(r4300, cam_obj + 0x4u, &cam_sub);
    }
    if (p08_guest(cam_sub)) {
        (void)r4300_read_aligned_word(r4300, cam_sub + 0x4u, &node);
    }
    uint32_t sub_80 = 0, sub_84 = 0, sub_88 = 0;
    if (p08_guest(cam_sub)) {
        (void)r4300_read_aligned_word(r4300, cam_sub + 0x80u, &sub_80);
        (void)r4300_read_aligned_word(r4300, cam_sub + 0x84u, &sub_84);
        (void)r4300_read_aligned_word(r4300, cam_sub + 0x88u, &sub_88);
    }
    uint32_t node_34 = 0, node_3c = 0, node_50 = 0, node_58 = 0;
    if (p08_guest(node)) {
        (void)r4300_read_aligned_word(r4300, node + 0x34u, &node_34);
        (void)r4300_read_aligned_word(r4300, node + 0x3Cu, &node_3c);
        (void)r4300_read_aligned_word(r4300, node + 0x50u, &node_50);
        (void)r4300_read_aligned_word(r4300, node + 0x58u, &node_58);
    }
    fprintf(stderr,
            "[ud5-checkpoint] runtime=AN phase=%s tick=%llu rec_ord=%d cam_obj=%08x cam_sub=%08x node=%08x sub_80=%08x sub_84=%08x (%.6f) sub_88=%08x node_34=%08x node_3c=%08x node_50=%08x (%.6f) node_58=%08x (%.6f)\n",
            phase, (unsigned long long)s_ud5_logic_tick, s_ud5_last_logged_rec_ord,
            cam_obj, cam_sub, node, sub_80, sub_84, p08_f32(sub_84), sub_88,
            node_34, node_3c, node_50, p08_f32(node_50), node_58, p08_f32(node_58));
    fflush(stderr);
}

static inline void p08_ud5_check_map_entry(struct r4300_core* r4300)
{
    if (s_ud5_trace_epoch == 0) {
        s_ud5_trace_epoch = 0x54524143; // 'TRAC'
    }
    int mode = 0, map = 0, stage = 0, sub = 0;
    uint32_t post = 0;
    p08_read_attract(r4300, &mode, &map, &stage, &sub, &post);
    const int in_target = (mode == 5 && stage == 0 && sub == 5 && map == 296);
    if (in_target && !s_ud5_in_map296) {
        s_ud5_map296_epoch++;
        s_ud5_in_map296 = 1;
        s_ud5_last_logged_rec_ord = -1;
        fprintf(stderr,
                "[ud5-epoch-entry] runtime=AN epoch=%u ud5epoch=%u stage=%d sub=%d map=%d mode=%d logic_tick=%llu\n",
                s_ud5_trace_epoch, s_ud5_map296_epoch, stage, sub, map, mode, (unsigned long long)s_ud5_logic_tick);
        fflush(stderr);
        p08_ud5_dump_checkpoint(r4300, "map-entry");
    } else if (!in_target && s_ud5_in_map296) {
        s_ud5_in_map296 = 0;
    }
}

static inline void p08_ud5_on_logic_tick(struct r4300_core* r4300)
{
    p08_ud5_check_map_entry(r4300);
    s_ud5_logic_tick++;
    s_ud5_cam_update_ord = 0;
    s_ud5_cam_call_seq = 0;

    int mode = 0, map = 0, stage = 0, sub = 0;
    uint32_t post = 0;
    p08_read_attract(r4300, &mode, &map, &stage, &sub, &post);
    if (!s_ud5_in_map296) return;

    uint32_t gl_base = 0;
    uint32_t cur_idx = 0;
    (void)r4300_read_aligned_word(r4300, 0x8012D5B0u, &gl_base);
    (void)r4300_read_aligned_word(r4300, 0x8012D5B8u, &cur_idx);

    if (p08_guest(gl_base)) {
        const int raw_ord = (int)cur_idx;
        const int rec_ord = (raw_ord >= 1000) ? (raw_ord - 1012) : raw_ord;
        const uint32_t rec_ptr = gl_base + (uint32_t)rec_ord * 6u;
        uint32_t w0 = 0, w1 = 0;
        (void)r4300_read_aligned_word(r4300, rec_ptr & ~3u, &w0);
        (void)r4300_read_aligned_word(r4300, (rec_ptr + 4u) & ~3u, &w1);
        uint8_t raw[8];
        raw[0] = (uint8_t)(w0 >> 24); raw[1] = (uint8_t)(w0 >> 16);
        raw[2] = (uint8_t)(w0 >> 8);  raw[3] = (uint8_t)w0;
        raw[4] = (uint8_t)(w1 >> 24); raw[5] = (uint8_t)(w1 >> 16);
        raw[6] = (uint8_t)(w1 >> 8);  raw[7] = (uint8_t)w1;
        const int off = (int)(rec_ptr & 3u);
        const int8_t sx = (int8_t)raw[off];
        const int8_t sy = (int8_t)raw[off + 1];
        const uint16_t btn = ((uint16_t)raw[off + 2] << 8) | (uint16_t)raw[off + 3];

        if (rec_ord != s_ud5_last_logged_rec_ord) {
            s_ud5_last_logged_rec_ord = rec_ord;
            fprintf(stderr,
                    "[ud5-record] runtime=AN epoch=%u ud5epoch=%u stage=%d sub=%d map=%d rec_ord=%d rec_ptr=%08x raw6=%02x%02x%02x%02x%02x%02x stick=(%d,%d) btn=%04x logic_tick=%llu\n",
                    s_ud5_trace_epoch, s_ud5_map296_epoch, stage, sub, map, rec_ord, rec_ptr,
                    (uint8_t)raw[off], (uint8_t)raw[off+1], (uint8_t)raw[off+2], (uint8_t)raw[off+3], (uint8_t)raw[off+4], (uint8_t)raw[off+5],
                    (int)sx, (int)sy, (unsigned)btn, (unsigned long long)s_ud5_logic_tick);
            fflush(stderr);
            if (rec_ord == 0) {
                p08_ud5_dump_checkpoint(r4300, "r0-record-consume");
            }
        }
    }
}

static inline void p08_ud5_on_cam_update_entry(struct r4300_core* r4300)
{
    if (!s_ud5_in_map296) return;
    s_ud5_cam_update_ord++;
    if (s_ud5_last_logged_rec_ord == 0 && s_ud5_cam_update_ord == 1) {
        p08_ud5_dump_checkpoint(r4300, "r0-cam-entry");
    }
}

static inline void p08_ud5_on_110588_entry(struct r4300_core* r4300)
{
    if (!s_ud5_in_map296) return;
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t a0 = (uint32_t)regs[4];
    uint32_t sub_84 = 0, node = 0, node_50 = 0, node_58 = 0;
    if (p08_guest(a0)) {
        (void)r4300_read_aligned_word(r4300, a0 + 0x84u, &sub_84);
        (void)r4300_read_aligned_word(r4300, a0 + 0x4u, &node);
    }
    if (p08_guest(node)) {
        (void)r4300_read_aligned_word(r4300, node + 0x50u, &node_50);
        (void)r4300_read_aligned_word(r4300, node + 0x58u, &node_58);
    }
    fprintf(stderr,
            "[ud5-110588-entry] runtime=AN tick=%llu rec_ord=%d a0=%08x sub_84=%08x (%.6f) node=%08x node_50=%08x (%.6f) node_58=%08x (%.6f)\n",
            (unsigned long long)s_ud5_logic_tick, s_ud5_last_logged_rec_ord,
            a0, sub_84, p08_f32(sub_84), node, node_50, p08_f32(node_50), node_58, p08_f32(node_58));
    fflush(stderr);
}

static inline void p08_ud5_on_ee7f8_copy(struct r4300_core* r4300)
{
    if (!s_ud5_in_map296) return;
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t a0 = (uint32_t)regs[4];
    const uint32_t a1 = (uint32_t)regs[5];
    uint32_t w0 = 0, w1 = 0, w2 = 0;
    if (p08_guest(a1)) {
        (void)r4300_read_aligned_word(r4300, a1 + 0u, &w0);
        (void)r4300_read_aligned_word(r4300, a1 + 4u, &w1);
        (void)r4300_read_aligned_word(r4300, a1 + 8u, &w2);
    }
    fprintf(stderr,
            "[ud5-ee7f8-copy] runtime=AN tick=%llu rec_ord=%d dst=%08x src=%08x pitch=%08x yaw=%08x (%.6f) roll=%08x\n",
            (unsigned long long)s_ud5_logic_tick, s_ud5_last_logged_rec_ord,
            a0, a1, w0, w1, p08_f32(w1), w2);
    fflush(stderr);
}

static inline void p08_ud5_on_116218_step(struct r4300_core* r4300)
{
    if (!s_ud5_in_map296) return;
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t a0 = (uint32_t)regs[4];
    const uint32_t a1 = (uint32_t)regs[5];
    uint32_t in_yaw = 0, node = 0, node_50 = 0, node_58 = 0;
    if (p08_guest(a1)) {
        (void)r4300_read_aligned_word(r4300, a1 + 4u, &in_yaw);
    }
    if (p08_guest(a0)) {
        (void)r4300_read_aligned_word(r4300, a0 + 4u, &node);
    }
    if (p08_guest(node)) {
        (void)r4300_read_aligned_word(r4300, node + 0x50u, &node_50);
        (void)r4300_read_aligned_word(r4300, node + 0x58u, &node_58);
    }
    fprintf(stderr,
            "[ud5-116218-calc] runtime=AN tick=%llu rec_ord=%d s0=%08x s1=%08x in_yaw=%08x (%.6f) node=%08x node_50=%08x (%.6f) node_58=%08x (%.6f)\n",
            (unsigned long long)s_ud5_logic_tick, s_ud5_last_logged_rec_ord,
            a0, a1, in_yaw, p08_f32(in_yaw), node, node_50, p08_f32(node_50), node_58, p08_f32(node_58));
    fflush(stderr);
}

static inline void p08_ud5_on_116380_store(struct r4300_core* r4300)
{
    if (!s_ud5_in_map296) return;
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t s1 = (uint32_t)regs[17];
    const uint32_t target_addr = s1 + 4u;
    const uint32_t f18 = p08_fpr_bits(r4300, 18);
    uint32_t old_bits = 0;
    if (p08_guest(target_addr)) {
        (void)r4300_read_aligned_word(r4300, target_addr, &old_bits);
    }
    fprintf(stderr,
            "[ud5-116380-store] runtime=AN tick=%llu rec_ord=%d addr=%08x old_bits=%08x (%.6f) input_f18=%08x (%.6f)\n",
            (unsigned long long)s_ud5_logic_tick, s_ud5_last_logged_rec_ord,
            target_addr, old_bits, p08_f32(old_bits), f18, p08_f32(f18));
    fflush(stderr);
}

static inline void p08_ud5_on_cam_call(struct r4300_core* r4300, uint32_t pc, const char* fn_name)
{
    if (!s_ud5_in_map296) return;
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t ra = (uint32_t)regs[31];
    const uint32_t callsite_pc = (ra >= 8) ? (ra - 8) : 0;
    s_ud5_cam_call_seq++;

    uint32_t gl_base = 0, cur_idx = 0;
    (void)r4300_read_aligned_word(r4300, 0x8012D5B0u, &gl_base);
    (void)r4300_read_aligned_word(r4300, 0x8012D5B8u, &cur_idx);
    const int raw_ord = (int)cur_idx;
    const int rec_ord = (raw_ord >= 1000) ? (raw_ord - 1012) : raw_ord;

    char raw_hex[16] = "000000000000";
    if (p08_guest(gl_base)) {
        const uint32_t rec_ptr = gl_base + (uint32_t)rec_ord * 6u;
        uint32_t w0 = 0, w1 = 0;
        (void)r4300_read_aligned_word(r4300, rec_ptr & ~3u, &w0);
        (void)r4300_read_aligned_word(r4300, (rec_ptr + 4u) & ~3u, &w1);
        uint8_t raw[8];
        raw[0] = (uint8_t)(w0 >> 24); raw[1] = (uint8_t)(w0 >> 16);
        raw[2] = (uint8_t)(w0 >> 8);  raw[3] = (uint8_t)w0;
        raw[4] = (uint8_t)(w1 >> 24); raw[5] = (uint8_t)(w1 >> 16);
        raw[6] = (uint8_t)(w1 >> 8);  raw[7] = (uint8_t)w1;
        const int off = (int)(rec_ptr & 3u);
        snprintf(raw_hex, sizeof(raw_hex), "%02x%02x%02x%02x%02x%02x",
                 raw[off], raw[off+1], raw[off+2], raw[off+3], raw[off+4], raw[off+5]);
    }

    fprintf(stderr,
            "[ud5-cam-call] runtime=AN epoch=%u ud5epoch=%u stage=0 sub=5 map=296 rec_ord=%d raw6=%s logic_tick=%llu cam_ord=%u fn=%s pc=%08x callsite_pc=%08x ra=%08x call_seq=%u\n",
            s_ud5_trace_epoch, s_ud5_map296_epoch, rec_ord, raw_hex,
            (unsigned long long)s_ud5_logic_tick, s_ud5_cam_update_ord, fn_name, pc, callsite_pc, ra, s_ud5_cam_call_seq);
    fflush(stderr);
}

static inline void p08_ud5_on_ca628(struct r4300_core* r4300)
{
    if (!s_ud5_in_map296) return;
    const int64_t* regs = r4300_regs(r4300);
    const uint32_t a0 = (uint32_t)regs[4];
    const uint32_t a1 = (uint32_t)regs[5];
    const uint32_t a2 = (uint32_t)regs[6];
    const uint32_t ra = (uint32_t)regs[31];
    const uint32_t callsite_pc = (ra >= 8) ? (ra - 8) : 0;

    uint32_t old_1c = 0, src2_1 = 0;
    if (p08_guest(a0)) {
        (void)r4300_read_aligned_word(r4300, a0 + 0x1Cu, &old_1c);
    }
    if (p08_guest(a2)) {
        (void)r4300_read_aligned_word(r4300, a2 + 4u, &src2_1);
    }

    uint32_t gl_base = 0, cur_idx = 0;
    (void)r4300_read_aligned_word(r4300, 0x8012D5B0u, &gl_base);
    (void)r4300_read_aligned_word(r4300, 0x8012D5B8u, &cur_idx);
    const int raw_ord = (int)cur_idx;
    const int rec_ord = (raw_ord >= 1000) ? (raw_ord - 1012) : raw_ord;

    char raw_hex[16] = "000000000000";
    if (p08_guest(gl_base)) {
        const uint32_t rec_ptr = gl_base + (uint32_t)rec_ord * 6u;
        uint32_t w0 = 0, w1 = 0;
        (void)r4300_read_aligned_word(r4300, rec_ptr & ~3u, &w0);
        (void)r4300_read_aligned_word(r4300, (rec_ptr + 4u) & ~3u, &w1);
        uint8_t raw[8];
        raw[0] = (uint8_t)(w0 >> 24); raw[1] = (uint8_t)(w0 >> 16);
        raw[2] = (uint8_t)(w0 >> 8);  raw[3] = (uint8_t)w0;
        raw[4] = (uint8_t)(w1 >> 24); raw[5] = (uint8_t)(w1 >> 16);
        raw[6] = (uint8_t)(w1 >> 8);  raw[7] = (uint8_t)w1;
        const int off = (int)(rec_ptr & 3u);
        snprintf(raw_hex, sizeof(raw_hex), "%02x%02x%02x%02x%02x%02x",
                 raw[off], raw[off+1], raw[off+2], raw[off+3], raw[off+4], raw[off+5]);
    }

    fprintf(stderr,
            "[ud5-ca628] runtime=AN epoch=%u ud5epoch=%u stage=0 sub=5 map=296 rec_ord=%d raw6=%s logic_tick=%llu cam_ord=%u dst=%08x src=%08x src2=%08x old_1c=%08x (%.6f) src2_1=%08x (%.6f) callsite_pc=%08x ra=%08x\n",
            s_ud5_trace_epoch, s_ud5_map296_epoch, rec_ord, raw_hex,
            (unsigned long long)s_ud5_logic_tick, s_ud5_cam_update_ord, a0, a1, a2, old_1c, p08_f32(old_1c), src2_1, p08_f32(src2_1), callsite_pc, ra);
    fflush(stderr);
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
        s_p08_ud5_e1a58_a0 = a0;
        s_p08_ud5_e1a58_a1 = a1;
        s_p08_ud5_e1a58_ra = ra;
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
    } else if (pc == 0x800E1BF4u) {
        uint32_t out_h = 0, a10 = 0, a12 = 0;
        if (p08_guest(s_p08_ud5_e1a58_a1)) {
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_e1a58_a1 + 0u, &a10);
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_e1a58_a1 + 4u, &out_h);
            (void)r4300_read_aligned_word(r4300, s_p08_ud5_e1a58_a1 + 8u, &a12);
        }
        fprintf(stderr,
                "[ud5-cam1c] phase=e1a58 leave glidx=%d ord=%u pc=800e1a58 ra=%08x "
                "a0=%08x a1=%08x heading_out=%08x (%.6f) a1_0=%08x a1_2=%08x\n",
                glidx, s_p08_ud5_e1a58_ord, s_p08_ud5_e1a58_ra,
                s_p08_ud5_e1a58_a0, s_p08_ud5_e1a58_a1, out_h, p08_f32(out_h), a10, a12);
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
                "old=(%08x,%08x,%08x / %.6f,%.6f,%.6f) "
                "src_a1=(%08x,%08x,%08x / %.6f,%.6f,%.6f) "
                "src_a2=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                glidx, uord, ra, a0, a1, a2, a3,
                ox, oy, oz, p08_f32(ox), p08_f32(oy), p08_f32(oz),
                s1x, s1y, s1z, p08_f32(s1x), p08_f32(s1y), p08_f32(s1z),
                s2x, s2y, s2z, p08_f32(s2x), p08_f32(s2y), p08_f32(s2z));
        fflush(stderr);
    } else if ((pc == 0x808001B4u || pc == 0x800EE7ACu) && glidx >= 0 && glidx <= 28) {
        uint32_t ex = 0, ey = 0, ez = 0;
        if (p08_guest(s_p08_ud5_ee780_dst)) {
            p08_rd3(r4300, s_p08_ud5_ee780_dst, &ex, &ey, &ez);
        }
        fprintf(stderr,
                "[ud5-ee780] leave glidx=%d ord=%u dest=%08x "
                "out=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                glidx, s_p08_ud5_ee780_ord, s_p08_ud5_ee780_dst,
                ex, ey, ez, p08_f32(ex), p08_f32(ey), p08_f32(ez));
        fflush(stderr);
    } else if (pc == 0x800EFB24u && glidx >= 22 && glidx <= 25) {
        uint32_t a1x = 0, a1y = 0, a1z = 0, a2x = 0, a2y = 0, a2z = 0;
        const unsigned uord = ++s_p08_ud5_efb24_ord;
        p08_rd3(r4300, a1, &a1x, &a1y, &a1z);
        p08_rd3(r4300, a2, &a2x, &a2y, &a2z);
        fprintf(stderr,
                "[ud5-efb24] enter glidx=%d ord=%u pc=800efb24 ra=%08x "
                "a0=%08x a1=%08x a2=%08x a3=%08x "
                "a1v=(%08x,%08x,%08x / %.6f,%.6f,%.6f) "
                "a2v=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                glidx, uord, ra, a0, a1, a2, a3,
                a1x, a1y, a1z, p08_f32(a1x), p08_f32(a1y), p08_f32(a1z),
                a2x, a2y, a2z, p08_f32(a2x), p08_f32(a2y), p08_f32(a2z));
        fflush(stderr);
    } else if (pc == 0x800EEBF0u && glidx >= 0 && glidx <= 28) {
        uint32_t ox = 0, oy = 0, oz = 0, sx = 0, sy = 0, sz = 0;
        const unsigned uord = ++s_p08_ud5_eebf0_ord;
        p08_rd3(r4300, a0, &ox, &oy, &oz);
        p08_rd3(r4300, a1, &sx, &sy, &sz);
        fprintf(stderr,
                "[ud5-eebf0] enter glidx=%d ord=%u pc=800eebf0 ra=%08x "
                "a0=%08x a1=%08x a2=%08x/%.6f a3=%08x "
                "old=(%08x,%08x,%08x / %.6f,%.6f,%.6f) "
                "src_dir=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                glidx, uord, ra, a0, a1, a2, p08_f32(a2), a3,
                ox, oy, oz, p08_f32(ox), p08_f32(oy), p08_f32(oz),
                sx, sy, sz, p08_f32(sx), p08_f32(sy), p08_f32(sz));
        fflush(stderr);
    } else if (pc == 0x801107F0u) {
        p08_rd3(r4300, a1, &x, &y, &z);
        fprintf(stderr,
                "[ud5-eye] phase=P1 after_107F0 glidx=%d pc=801107f0 ra=%08x s0=%08x dst=%08x "
                "work=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                glidx, ra, a0, a1, x, y, z, p08_f32(x), p08_f32(y), p08_f32(z));
        fflush(stderr);
    } else if (pc == 0x80111DCCu) {
        p08_rd3(r4300, a0, &x, &y, &z);
        fprintf(stderr,
                "[ud5-efb24] leave glidx=%d ra=%08x dest=%08x "
                "delta=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                glidx, ra, a0, x, y, z, p08_f32(x), p08_f32(y), p08_f32(z));
        fflush(stderr);
    } else if (pc == 0x800EEC28u && glidx >= 0 && glidx <= 28) {
        uint32_t ox = 0, oy = 0, oz = 0;
        p08_rd3(r4300, a0, &ox, &oy, &oz);
        fprintf(stderr,
                "[ud5-eebf0] leave glidx=%d ord=%u dest=%08x "
                "out=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                glidx, s_p08_ud5_eebf0_ord, a0, ox, oy, oz, p08_f32(ox), p08_f32(oy), p08_f32(oz));
        fflush(stderr);
    }
}

static inline void p08_ud5_yaw(struct r4300_core* r4300, uint32_t pc)
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

    if (pc == 0x800F1A88u) {
        uint32_t vx = 0, vy = 0, vz = 0;
        p08_rd3(r4300, a0, &vx, &vy, &vz);
        fprintf(stderr,
                "[ud5-f1a88] enter glidx=%d pc=800f1a88 ra=%08x in_vec=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                glidx, ra, vx, vy, vz, p08_f32(vx), p08_f32(vy), p08_f32(vz));
        fflush(stderr);
    } else if (pc == 0x800F1AF4u) {
        uint32_t sp = (uint32_t)regs[29];
        uint32_t a1_dst = 0;
        (void)r4300_read_aligned_word(r4300, sp + 0x24u, &a1_dst);
        uint32_t pitch = 0, yaw = 0;
        if (p08_guest(a1_dst)) {
            (void)r4300_read_aligned_word(r4300, a1_dst + 0u, &pitch);
            (void)r4300_read_aligned_word(r4300, a1_dst + 4u, &yaw);
        }
        fprintf(stderr,
                "[ud5-f1a88] leave glidx=%d pc=800f1af4 pitch=%08x/%.6f init_yaw=%08x/%.6f\n",
                glidx, pitch, p08_f32(pitch), yaw, p08_f32(yaw));
        fflush(stderr);
    } else if (pc == 0x80112C3Cu) {
        uint32_t p60 = 0, p60_0 = 0, p60_4 = 0, p60_8 = 0;
        if (p08_guest(a0)) {
            (void)r4300_read_aligned_word(r4300, a0 + 0x60u, &p60);
            if (p08_guest(p60)) {
                (void)r4300_read_aligned_word(r4300, p60 + 0u, &p60_0);
                (void)r4300_read_aligned_word(r4300, p60 + 4u, &p60_4);
                (void)r4300_read_aligned_word(r4300, p60 + 8u, &p60_8);
            }
        }
        fprintf(stderr,
                "[ud5-2c3c] enter glidx=%d pc=80112c3c ra=%08x s0=%08x in_yaw=%08x/%.6f "
                "p60=%08x p60_8=%08x p60_0=%08x/%.6f p60_4=%08x\n",
                glidx, ra, a0, a1, p08_f32(a1), p60, p60_8, p60_0, p08_f32(p60_0), p60_4);
        fflush(stderr);
    } else if (pc == 0x80112CF8u || pc == 0x80112D00u) {
        uint32_t f0 = p08_fpr_bits(r4300, 0);
        fprintf(stderr,
                "[ud5-2c3c] leave glidx=%d pc=%08x out_f0=%08x/%.6f (OPERAND_A)\n",
                glidx, pc, f0, p08_f32(f0));
        fflush(stderr);
    } else if (pc == 0x801125E8u) {
        uint32_t t6 = 0, t6_c = 0;
        if (p08_guest(a0)) {
            (void)r4300_read_aligned_word(r4300, a0 + 0x4Cu, &t6);
            if (p08_guest(t6)) {
                (void)r4300_read_aligned_word(r4300, t6 + 0xCu, &t6_c);
            }
        }
        fprintf(stderr,
                "[ud5-25e8] enter glidx=%d pc=801125e8 ra=%08x s0=%08x t6=%08x t6_c=%08x/%.6f\n",
                glidx, ra, a0, t6, t6_c, p08_f32(t6_c));
        fflush(stderr);
    } else if (pc == 0x801125ECu) {
        uint32_t t6 = (uint32_t)regs[14];
        uint32_t t6_c = 0;
        if (p08_guest(t6)) {
            (void)r4300_read_aligned_word(r4300, t6 + 0xCu, &t6_c);
        }
        fprintf(stderr,
                "[ud5-25e8] leave glidx=%d pc=801125ec out_f0=%08x/%.6f (OPERAND_B)\n",
                glidx, t6_c, p08_f32(t6_c));
        fflush(stderr);
    } else if (pc == 0x800EEBF0u) {
        uint32_t pitch = 0, yaw_sum = 0;
        if (p08_guest(a1)) {
            (void)r4300_read_aligned_word(r4300, a1 + 0u, &pitch);
            (void)r4300_read_aligned_word(r4300, a1 + 4u, &yaw_sum);
        }
        fprintf(stderr,
                "[ud5-add194] glidx=%d pc=80800194 opA_prod=80112c3c opB_prod=801125e8 "
                "pitch=%08x/%.6f yaw_sum=%08x/%.6f radius=%08x/%.6f stored_sp38=%08x/%.6f\n",
                glidx, pitch, p08_f32(pitch), yaw_sum, p08_f32(yaw_sum), a2, p08_f32(a2), yaw_sum, p08_f32(yaw_sum));
        fprintf(stderr,
                "[ud5-eebf0] enter glidx=%d pc=800eebf0 ra=%08x dst=%08x "
                "in_pitch=%08x/%.6f in_yaw=%08x/%.6f in_radius=%08x/%.6f\n",
                glidx, ra, a0, pitch, p08_f32(pitch), yaw_sum, p08_f32(yaw_sum), a2, p08_f32(a2));
        fflush(stderr);
    } else if (pc == 0x800EEC28u) {
        uint32_t sp = (uint32_t)regs[29];
        uint32_t a0_dst = 0;
        (void)r4300_read_aligned_word(r4300, sp + 0x18u, &a0_dst);
        uint32_t dx = 0, dy = 0, dz = 0;
        p08_rd3(r4300, a0_dst, &dx, &dy, &dz);
        fprintf(stderr,
                "[ud5-eebf0] leave glidx=%d pc=800eec28 dst=%08x cart_delta=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                glidx, a0_dst, dx, dy, dz, p08_f32(dx), p08_f32(dy), p08_f32(dz));
        fflush(stderr);
    } else if (pc == 0x800EFD24u) {
        uint32_t f12 = p08_fpr_bits(r4300, 12);
        fprintf(stderr,
                "[ud5-efd24] enter glidx=%d pc=800efd24 in_angle=%08x/%.6f\n",
                glidx, f12, p08_f32(f12));
        fflush(stderr);
    } else if (pc == 0x800EFD38u) {
        uint32_t f0 = p08_fpr_bits(r4300, 0);
        fprintf(stderr,
                "[ud5-efd24] leave glidx=%d pc=800efd38 out_angle=%08x/%.6f\n",
                glidx, f0, p08_f32(f0));
        fflush(stderr);
    } else if (pc == 0x800EF214u) {
        uint32_t f12 = (uint32_t)regs[5]; // a1 = pitch
        uint32_t f14 = (uint32_t)regs[6]; // a2 = yaw
        uint32_t a3_r = (uint32_t)regs[7]; // a3 = radius
        fprintf(stderr,
                "[ud5-ef214] enter glidx=%d pc=800ef214 dst=%08x pitch=%08x/%.6f yaw=%08x/%.6f radius=%08x/%.6f\n",
                glidx, a0, f12, p08_f32(f12), f14, p08_f32(f14), a3_r, p08_f32(a3_r));
        fflush(stderr);
    } else if (pc == 0x800EF2ACu) {
        uint32_t sp = (uint32_t)regs[29];
        uint32_t a0_dst = 0;
        (void)r4300_read_aligned_word(r4300, sp + 0x18u, &a0_dst);
        uint32_t dx = 0, dy = 0, dz = 0;
        p08_rd3(r4300, a0_dst, &dx, &dy, &dz);
        fprintf(stderr,
                "[ud5-ef214] leave glidx=%d pc=800ef2ac dst=%08x out_cart=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                glidx, a0_dst, dx, dy, dz, p08_f32(dx), p08_f32(dy), p08_f32(dz));
        fflush(stderr);
    } else if (pc == 0x800F1738u) {
        uint32_t f12 = p08_fpr_bits(r4300, 12);
        uint32_t f14 = p08_fpr_bits(r4300, 14);
        fprintf(stderr,
                "[ud5-f1738] enter glidx=%d pc=800f1738 cur=%08x/%.6f targ=%08x/%.6f step=%08x\n",
                glidx, f12, p08_f32(f12), f14, p08_f32(f14), a2);
        fflush(stderr);
    } else if (pc == 0x800F1798u) {
        uint32_t f0 = p08_fpr_bits(r4300, 0);
        fprintf(stderr,
                "[ud5-f1738] leave glidx=%d pc=800f1798 out=%08x/%.6f\n",
                glidx, f0, p08_f32(f0));
        fflush(stderr);
    } else if (pc == 0x800F1DCCu) {
        uint32_t f12 = p08_fpr_bits(r4300, 12);
        uint32_t f14 = p08_fpr_bits(r4300, 14);
        fprintf(stderr,
                "[ud5-f1dcc] enter glidx=%d pc=800f1dcc a0=%08x/%.6f a1=%08x/%.6f\n",
                glidx, f12, p08_f32(f12), f14, p08_f32(f14));
        fflush(stderr);
    } else if (pc == 0x800F1E14u) {
        uint32_t f0 = p08_fpr_bits(r4300, 0);
        fprintf(stderr,
                "[ud5-f1dcc] leave glidx=%d pc=800f1e14 out=%08x/%.6f\n",
                glidx, f0, p08_f32(f0));
        fflush(stderr);
    } else if (pc == 0x80112D08u) {
        fprintf(stderr,
                "[ud5-p60-write] enter seq=0 glidx=%d pc=80112D08 ra=%08x"
                " node=%08x a1_mode=%u a2_targ=%08x/%.6f a3_step=%08x/%.6f",
                glidx, ra, a0, a1, a2, p08_f32(a2), a3, p08_f32(a3));
        p08_ud4_dump_p60_object(r4300, stderr, a0);
        fprintf(stderr, "\n");
        fflush(stderr);
    } else if (pc == 0x80112D24u || pc == 0x80112D28u) {
        fprintf(stderr,
                "[ud5-p60-write] leave seq=0 glidx=%d pc=80112D08 node=%08x",
                glidx, a0);
        p08_ud4_dump_p60_object(r4300, stderr, a0);
        fprintf(stderr, "\n");
        fflush(stderr);
    } else if (pc == 0x80112A84u) {
        uint32_t p60 = 0, p60_c_word = 0;
        if (p08_guest(a0)) {
            (void)r4300_read_aligned_word(r4300, a0 + 0x60u, &p60);
            if (p08_guest(p60)) {
                (void)r4300_read_aligned_word(r4300, p60 + 0x0Cu, &p60_c_word);
            }
        }
        const uint16_t p60_c = (uint16_t)(p60_c_word >> 16);
        fprintf(stderr,
                "[ud5-p60-writer-2a84] enter seq=0 glidx=%d pc=80112A84 ra=%08x node=%08x p60_c_predicate=%04x",
                glidx, ra, a0, (unsigned)p60_c);
        p08_ud4_dump_p60_object(r4300, stderr, a0);
        fprintf(stderr, "\n");
        fflush(stderr);
    } else if (pc == 0x80112C34u) {
        fprintf(stderr,
                "[ud5-p60-writer-2a84] leave seq=0 glidx=%d pc=80112A84 node=%08x",
                glidx, a0);
        p08_ud4_dump_p60_object(r4300, stderr, a0);
        fprintf(stderr, "\n");
        fflush(stderr);
    } else if (pc == 0x80112D90u) {
        uint32_t sp = (uint32_t)regs[29];
        uint32_t sp30 = 0;
        (void)r4300_read_aligned_word(r4300, sp + 0x30u, &sp30);
        fprintf(stderr,
                "[ud5-p60-set-c-2d90] enter seq=0 glidx=%d pc=80112D90 ra=%08x node=%08x a1=%08x a2=%08x a3=%08x sp30=%08x",
                glidx, ra, a0, a1, a2, a3, sp30);
        p08_ud4_dump_p60_object(r4300, stderr, a0);
        fprintf(stderr, "\n");
        fflush(stderr);
    } else if (pc == 0x80112E48u || pc == 0x80112E4Cu) {
        fprintf(stderr,
                "[ud5-p60-set-c-2d90] leave seq=0 glidx=%d pc=80112D90 node=%08x",
                glidx, a0);
        p08_ud4_dump_p60_object(r4300, stderr, a0);
        fprintf(stderr, "\n");
        fflush(stderr);
    } else if (pc == 0x80110588u) {
        uint32_t w70 = 0;
        (void)r4300_read_aligned_word(r4300, a0 + 0x70u, &w70);
        const unsigned f70 = (w70 >> 24) & 0xFFu;
        const unsigned f72 = (w70 >> 8) & 0xFFu;
        fprintf(stderr,
                "[ud5-p60-gate-0588] enter seq=0 glidx=%d pc=80110588 ra=%08x node=%08x f70=%u f72=%u",
                glidx, ra, a0, f70, f72);
        p08_ud4_dump_p60_object(r4300, stderr, a0);
        fprintf(stderr, "\n");
        fflush(stderr);
    } else if (pc == 0x80110690u || pc == 0x80110694u) {
        fprintf(stderr,
                "[ud5-p60-gate-0588] leave seq=0 glidx=%d pc=80110588 node=%08x",
                glidx, a0);
        p08_ud4_dump_p60_object(r4300, stderr, a0);
        fprintf(stderr, "\n");
        fflush(stderr);
    } else if (pc == 0x801111D0u) {
        uint32_t f12 = p08_fpr_bits(r4300, 12);
        fprintf(stderr,
                "[ud5-111d0] enter seq=0 glidx=%d pc=801111D0 ra=%08x node=%08x delta_yaw=%.4f (bits=%08x)\n",
                glidx, ra, a0, p08_f32(f12), f12);
        fflush(stderr);
    } else if (pc == 0x801112A0u || pc == 0x801112A8u) {
        uint32_t v0 = (uint32_t)regs[2];
        fprintf(stderr,
                "[ud5-111d0] leave seq=0 glidx=%d pc=801111D0 ret_v0=%u\n",
                glidx, v0);
        fflush(stderr);
    } else if (pc == 0x80112E84u) {
        fprintf(stderr,
                "[ud5-12e84] enter seq=0 glidx=%d pc=80112E84 ra=%08x a0=%08x\n",
                glidx, ra, a0);
        fflush(stderr);
    } else if (pc == 0x80112E8Cu || pc == 0x80112E90u) {
        uint32_t v0 = (uint32_t)regs[2];
        fprintf(stderr,
                "[ud5-12e84] leave seq=0 glidx=%d pc=80112E84 ret_v0=%u (raw=%08x)\n",
                glidx, v0, v0);
        fflush(stderr);
    } else if (pc == 0x8011349Cu) {
        uint32_t tx = 0, ty = 0, tz = 0;
        uint32_t ex = 0, ey = 0, ez = 0;
        p08_rd3(r4300, a0, &tx, &ty, &tz);
        p08_rd3(r4300, a1, &ex, &ey, &ez);
        fprintf(stderr,
                "[ud5-1349c] enter seq=0 glidx=%d pc=8011349C ra=%08x a0=%08x target=(%08x,%08x,%08x / %.6f,%.6f,%.6f) a1=%08x cand_eye=(%08x,%08x,%08x / %.6f,%.6f,%.6f)\n",
                glidx, ra, a0,
                tx, ty, tz, p08_f32(tx), p08_f32(ty), p08_f32(tz),
                a1,
                ex, ey, ez, p08_f32(ex), p08_f32(ey), p08_f32(ez));
        fflush(stderr);
    } else if (pc == 0x801134E4u || pc == 0x801134E8u) {
        uint32_t v0 = (uint32_t)regs[2];
        fprintf(stderr,
                "[ud5-1349c] leave seq=0 glidx=%d pc=8011349C ret_v0=%u\n",
                glidx, v0);
        fflush(stderr);
    } else if (pc == 0x800FAB50u) {
        uint32_t sp = (uint32_t)regs[29];
        uint32_t sp10 = 0;
        uint32_t a0x = 0, a0y = 0, a0z = 0;
        uint32_t a1x = 0, a1y = 0, a1z = 0;
        (void)r4300_read_aligned_word(r4300, sp + 0x10u, &sp10);
        p08_rd3(r4300, a0, &a0x, &a0y, &a0z);
        p08_rd3(r4300, a1, &a1x, &a1y, &a1z);
        fprintf(stderr,
                "[ud5-fab50] enter seq=0 glidx=%d pc=800FAB50 ra=%08x a0=%08x=(%08x,%08x,%08x / %.6f,%.6f,%.6f) a1=%08x=(%08x,%08x,%08x / %.6f,%.6f,%.6f) a2=%08x a3=%08x radius=%08x/%.6f\n",
                glidx, ra, a0,
                a0x, a0y, a0z, p08_f32(a0x), p08_f32(a0y), p08_f32(a0z),
                a1,
                a1x, a1y, a1z, p08_f32(a1x), p08_f32(a1y), p08_f32(a1z),
                a2, a3, sp10, p08_f32(sp10));
        fflush(stderr);
    } else if (pc == 0x800FABE0u || pc == 0x800FABECu) {
        uint32_t v0 = (uint32_t)regs[2];
        fprintf(stderr,
                "[ud5-fab50] leave seq=0 glidx=%d pc=800FAB50 ret_v0=%08x\n",
                glidx, v0);
        fflush(stderr);
    } else if (pc == 0x800C6A7Cu) {
        uint32_t a0x = 0, a0y = 0, a0z = 0;
        uint32_t a1x = 0, a1y = 0, a1z = 0;
        p08_rd3(r4300, a0, &a0x, &a0y, &a0z);
        p08_rd3(r4300, a1, &a1x, &a1y, &a1z);
        fprintf(stderr,
                "[ud5-c6a7c] enter seq=0 glidx=%d pc=800C6A7C ra=%08x a0=%08x=(%08x,%08x,%08x / %.6f,%.6f,%.6f) a1=%08x=(%08x,%08x,%08x / %.6f,%.6f,%.6f) a2=%08x a3=%08x\n",
                glidx, ra, a0,
                a0x, a0y, a0z, p08_f32(a0x), p08_f32(a0y), p08_f32(a0z),
                a1,
                a1x, a1y, a1z, p08_f32(a1x), p08_f32(a1y), p08_f32(a1z),
                a2, a3);
        fflush(stderr);
    } else if (pc == 0x800C6B48u || pc == 0x800C6B6Cu) {
        uint32_t v0 = (uint32_t)regs[2];
        fprintf(stderr,
                "[ud5-c6a7c] leave seq=0 glidx=%d pc=800C6A7C ret_v0=%08x\n",
                glidx, v0);
        fflush(stderr);
    } else if (pc == 0x800A4920u) {
        fprintf(stderr,
                "[ud5-cam-chain] 800A4878 at 800A4920 calling 80097B70 glidx=%d self=%08x\n",
                glidx, a0);
        fflush(stderr);
    } else if (pc == 0x80097B70u) {
        fprintf(stderr,
                "[ud5-cam-chain] enter 80097B70 (rotate_camera_left) glidx=%d self=%08x\n",
                glidx, a0);
        fflush(stderr);
    } else if (pc == 0x80097B8Cu || pc == 0x80097B90u) {
        uint32_t v0 = (uint32_t)regs[2];
        fprintf(stderr,
                "[ud5-cam-chain] leave 80097B70 (rotate_camera_left) glidx=%d v0=%u (raw=%08x)\n",
                glidx, v0, v0);
        fflush(stderr);
    } else if (pc == 0x800A4928u) {
        uint32_t v0 = (uint32_t)regs[2];
        fprintf(stderr,
                "[ud5-branch-800A4928] glidx=%d pc=800A4928 v0_before=%u (raw=%08x) taken=%u target=800A4940\n",
                glidx, v0, v0, (v0 == 0) ? 1u : 0u);
        fflush(stderr);
    } else if (pc == 0x800979D8u) {
        static int s_mips_dump_done = 0;
        if (!s_mips_dump_done) {
            s_mips_dump_done = 1;
            FILE* df = fopen("/tmp/mips_80097970.bin", "wb");
            for (uint32_t addr = 0x80097970u; addr <= 0x80097A60u; addr += 4) {
                uint32_t op = 0;
                (void)r4300_read_aligned_word(r4300, addr, &op);
                fprintf(stderr, "[mips-dump-80097970] pc=%08x raw=%08x\n", addr, op);
                if (df) {
                    uint8_t bytes[4];
                    bytes[0] = (uint8_t)(op >> 24);
                    bytes[1] = (uint8_t)(op >> 16);
                    bytes[2] = (uint8_t)(op >> 8);
                    bytes[3] = (uint8_t)op;
                    fwrite(bytes, 1, 4, df);
                }
            }
            if (df) fclose(df);
            for (uint32_t addr = 0x80098024u; addr <= 0x80098040u; addr += 4) {
                uint32_t op = 0;
                (void)r4300_read_aligned_word(r4300, addr, &op);
                fprintf(stderr, "[mips-dump-80098024] pc=%08x raw=%08x\n", addr, op);
            }
            for (uint32_t addr = 0x80098140u; addr <= 0x80098190u; addr += 4) {
                uint32_t op = 0;
                (void)r4300_read_aligned_word(r4300, addr, &op);
                fprintf(stderr, "[mips-dump-80098140] pc=%08x raw=%08x\n", addr, op);
            }
            fflush(stderr);
        }
        uint32_t key = 0, input = 0;
        (void)r4300_read_aligned_word(r4300, a0 + 0x40u, &key);
        (void)r4300_read_aligned_word(r4300, a0 + 0xA4u, &input);
        uint32_t state = 0, button_word = 0, enabled_word0 = 0, enabled_word4 = 0;
        unsigned prev = 0, map = 0;
        if (p08_guest(key)) {
            prev = p08_read_u8(r4300, key + 0x18u + a1);
            (void)r4300_read_aligned_word(r4300, key + 0u, &state);
        }
        if (p08_guest(state)) {
            map = p08_read_u8(r4300, state + a1);
            (void)r4300_read_aligned_word(r4300, state + 0x24u + (map * 56u), &button_word);
        }
        if (p08_guest(input)) {
            (void)r4300_read_aligned_word(r4300, input + 0u, &enabled_word0);
            (void)r4300_read_aligned_word(r4300, input + 4u, &enabled_word4);
        }
        fprintf(stderr,
                "[ud5-bainput-979d8] enter glidx=%d self=%08x btn=%u in_id=%u key=%08x state=%08x prev=%u map=%u button_word=%08x input=%08x enabled_word0=%08x enabled_word4=%08x ra=%08x\n",
                glidx, a0, a1, a2, key, state, prev, map, button_word, input, enabled_word0, enabled_word4, ra);
        fflush(stderr);
    } else if (pc > 0x800979D8u && pc <= 0x80097A50u) {
        uint32_t op = 0;
        (void)r4300_read_aligned_word(r4300, pc, &op);
        uint32_t cur_self = (uint32_t)regs[4];
        uint32_t inp_ptr = 0, inp_plus0 = 0, inp_plus4 = 0;
        (void)r4300_read_aligned_word(r4300, cur_self + 0xA4u, &inp_ptr);
        if (p08_guest(inp_ptr)) {
            (void)r4300_read_aligned_word(r4300, inp_ptr + 0u, &inp_plus0);
            (void)r4300_read_aligned_word(r4300, inp_ptr + 4u, &inp_plus4);
        }
        fprintf(stderr,
                "[ud5-mips-step] glidx=%d pc=%08x raw=%08x v0=%08x v1=%08x a0=%08x a1=%08x a2=%08x a3=%08x t0=%08x t1=%08x t2=%08x t3=%08x s0=%08x s1=%08x sp=%08x ra=%08x inp=%08x inp0=%08x inp4=%08x\n",
                glidx, pc, op,
                (uint32_t)regs[2], (uint32_t)regs[3],
                (uint32_t)regs[4], (uint32_t)regs[5],
                (uint32_t)regs[6], (uint32_t)regs[7],
                (uint32_t)regs[8], (uint32_t)regs[9],
                (uint32_t)regs[10], (uint32_t)regs[11],
                (uint32_t)regs[16], (uint32_t)regs[17],
                (uint32_t)regs[29], (uint32_t)regs[31],
                inp_ptr, inp_plus0, inp_plus4);
        if (pc == 0x80097A48u || pc == 0x80097A4Cu) {
            uint32_t v0 = (uint32_t)regs[2];
            fprintf(stderr,
                    "[ud5-bainput-979d8] leave glidx=%d v0=%u (raw=%08x)\n",
                    glidx, v0, v0);
        }
        fflush(stderr);
    } else if (pc == 0x800919F4u) {
        uint32_t key = 0;
        (void)r4300_read_aligned_word(r4300, a0 + 0x40u, &key);
        unsigned prev = 0;
        if (p08_guest(key)) {
            prev = p08_read_u8(r4300, key + 0x18u + a1);
        }
        fprintf(stderr,
                "[ud5-bakey-held] enter glidx=%d self=%08x btn=%u prev_state=%u ra=%08x\n",
                glidx, a0, a1, prev, ra);
        fflush(stderr);
    } else if (pc == 0x80091A18u || pc == 0x80091A20u || pc == 0x80091A24u) {
        uint32_t v0 = (uint32_t)regs[2];
        fprintf(stderr,
                "[ud5-bakey-held] leave glidx=%d pc=%08x held=%u (ret_v0=%08x)\n",
                glidx, pc, (v0 != 0) ? 1u : 0u, v0);
        fflush(stderr);
    } else if (pc == 0x80098024u) {
        uint32_t input = 0, en_w0 = 0, en_w4 = 0;
        (void)r4300_read_aligned_word(r4300, a0 + 0xA4u, &input);
        if (p08_guest(input)) {
            (void)r4300_read_aligned_word(r4300, input + 0u, &en_w0);
            (void)r4300_read_aligned_word(r4300, input + 4u, &en_w4);
        }
        fprintf(stderr,
                "[ud5-isenabled] enter glidx=%d self=%08x in_id=%u inp=%08x enabled_word0=%08x enabled_word4=%08x ra=%08x\n",
                glidx, a0, a1, input, en_w0, en_w4, ra);
        fflush(stderr);
    } else if (pc == 0x80098034u) {
        uint32_t v0 = (uint32_t)regs[2];
        fprintf(stderr,
                "[ud5-isenabled] jr_pc glidx=%d pc=80098034 pre_delay_v0=%08x\n",
                glidx, v0);
        fflush(stderr);
    } else if (pc == 0x80098038u) {
        uint32_t v0 = (uint32_t)regs[2];
        fprintf(stderr,
                "[ud5-isenabled] delay_slot glidx=%d pc=80098038 raw_v0=%08x\n",
                glidx, v0);
        fflush(stderr);
    } else if (pc == 0x80097990u) {
        static uint32_t s_97990_seq = 0;
        s_97990_seq++;
        s_p08_enable_self = a0;
        s_p08_enable_in_id = a1;
        s_p08_enable_en = a2;
        s_p08_enable_ra = ra;
        s_p08_enable_inp = 0;
        s_p08_enable_old_w = 0;
        (void)r4300_read_aligned_word(r4300, a0 + 0xA4u, &s_p08_enable_inp);
        if (p08_guest(s_p08_enable_inp)) {
            (void)r4300_read_aligned_word(r4300, s_p08_enable_inp + 4u, &s_p08_enable_old_w);
        }
        fprintf(stderr,
                "[ud5-97990-call] runtime=AN seq=%u glidx=%d caller_pc=%08x self=%08x id=%u val=%u old_w4=%08x bit0_old=%u\n",
                s_97990_seq, glidx, ra, a0, a1, a2, s_p08_enable_old_w, s_p08_enable_old_w & 1u);
        fflush(stderr);
    } else if (pc == 0x800979ACu || pc == 0x800979B0u || pc == 0x800979C0u || pc == 0x800979C4u || pc == 0x800979CCu) {
        uint32_t new_w4 = 0;
        if (p08_guest(s_p08_enable_inp)) {
            (void)r4300_read_aligned_word(r4300, s_p08_enable_inp + 4u, &new_w4);
        }
        fprintf(stderr,
                "[ud5-input4-store] runtime=AN glidx=%d pc=%08x op=%s self=%08x id=%u old_w4=%08x new_w4=%08x bit0_old=%u bit0_new=%u ra=%08x\n",
                glidx, pc, (s_p08_enable_en == 0) ? "CLEAR" : "SET",
                s_p08_enable_self, s_p08_enable_in_id, s_p08_enable_old_w, new_w4,
                s_p08_enable_old_w & 1u, new_w4 & 1u, s_p08_enable_ra);
        fflush(stderr);
    } else if (pc == 0x80097978u) {
        uint32_t inp = 0, w4 = 0;
        (void)r4300_read_aligned_word(r4300, a0 + 0xA4u, &inp);
        if (p08_guest(inp)) {
            (void)r4300_read_aligned_word(r4300, inp + 4u, &w4);
        }
        fprintf(stderr,
                "[ud5-97978-query] runtime=AN glidx=%d caller_pc=%08x self=%08x id=%u w4=%08x bit0=%u\n",
                glidx, ra, a0, a1, w4, (w4 >> a1) & 1u);
        fflush(stderr);
    } else if (pc == 0x8009798Cu) {
        uint32_t v0 = (uint32_t)regs[2];
        fprintf(stderr,
                "[ud5-97978-leave] runtime=AN glidx=%d ret_v0=%08x\n",
                glidx, v0);
        fflush(stderr);
    } else if (pc == 0x8009811Cu || pc == 0x80098124u || pc == 0x8009812Cu || pc == 0x80098130u) {
        uint32_t inp = 0, w0 = 0, w4 = 0;
        (void)r4300_read_aligned_word(r4300, a0 + 0xA4u, &inp);
        if (p08_guest(inp)) {
            (void)r4300_read_aligned_word(r4300, inp + 0u, &w0);
            (void)r4300_read_aligned_word(r4300, inp + 4u, &w4);
        }
        fprintf(stderr,
                "[ud5-bainput-init] runtime=AN glidx=%d pc=%08x ra=%08x self=%08x inp=%08x w0=%08x w4=%08x\n",
                glidx, pc, ra, a0, inp, w0, w4);
        fflush(stderr);
    } else if (pc == 0x80098140u) {
        s_p08_enable_self = a0;
        s_p08_enable_in_id = a1;
        s_p08_enable_en = a2;
        s_p08_enable_ra = ra;
        s_p08_enable_inp = 0;
        s_p08_enable_old_w = 0;
        (void)r4300_read_aligned_word(r4300, a0 + 0xA4u, &s_p08_enable_inp);
        if (p08_guest(s_p08_enable_inp)) {
            (void)r4300_read_aligned_word(r4300, s_p08_enable_inp, &s_p08_enable_old_w);
        }
    } else if (pc == 0x8009815Cu || pc == 0x80098180u) {
        uint32_t new_word = 0;
        if (p08_guest(s_p08_enable_inp)) {
            (void)r4300_read_aligned_word(r4300, s_p08_enable_inp, &new_word);
        }
        const uint32_t old_bit0 = s_p08_enable_old_w & 1u;
        const uint32_t new_bit0 = new_word & 1u;
        fprintf(stderr,
                "[ud5-enable-write-helper] glidx=%d self=%08x input=%08x id=%u arg=%u pc=%08x ra=%08x old=%08x new=%08x old_bit=%u new_bit=%u\n",
                glidx, s_p08_enable_self, s_p08_enable_inp, s_p08_enable_in_id, s_p08_enable_en,
                pc, s_p08_enable_ra, s_p08_enable_old_w, new_word, old_bit0, new_bit0);
        fflush(stderr);
    } else if (pc == 0x80097A48u) {
        uint32_t v0 = (uint32_t)regs[2];
        fprintf(stderr,
                "[ud5-bainput-979d8] leave glidx=%d v0=%u (raw=%08x)\n",
                glidx, v0, v0);
        fflush(stderr);
    } else if (pc == 0x80110EFCu) {
        fprintf(stderr,
                "[ud5-cam-chain] enter 80110EFC glidx=%d node=%08x a1=%08x\n",
                glidx, a0, a1);
        fflush(stderr);
    } else if (pc == 0x80110E00u) {
        fprintf(stderr,
                "[ud5-cam-chain] enter 80110E00 glidx=%d node=%08x a1=%08x\n",
                glidx, a0, a1);
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
