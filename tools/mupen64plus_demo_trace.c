/* tools/mupen64plus_demo_trace.c
 *
 * Official-Mupen64Plus video plugin used only as an RDRAM/VI probe.
 * Built against the local official mupen64plus-core headers. Not a
 * production video plugin: ProcessDList is a no-op, UpdateScreen dumps
 * one CSV row whenever glrecord +0x8 advances during mode-5 attract.
 *
 * Endian: guest-byte addressing via rdram[phys ^ 3] (mupen LE RDRAM).
 * Live landmarks must confirm this before a row is trusted.
 */
#define M64P_PLUGIN_PROTOTYPES 1
#include "m64p_common.h"
#include "m64p_plugin.h"
#include "m64p_types.h"

#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VIDEO_PLUGIN_API_VERSION 0x020200
#define PLUGIN_VERSION           0x00010000

static void (*l_DebugCallback)(void *, int, const char *) = NULL;
static void *l_DebugCallContext = NULL;
static int l_PluginInit = 0;
static GFX_INFO g_gfx;
static unsigned char *g_rdram = NULL;
static unsigned int g_rdram_size = 0x800000;
static FILE *g_log = NULL;
static int g_prev_post = -1;
static unsigned int g_vi = 0;
static unsigned int g_vi_at_last_sample = 0;
static int g_layout_ok = -1;
static int g_xor = 3; /* mupen LE RDRAM guest-byte address: phys ^ 3 */
static int g_samples = 0;
static int g_mode5_seen = 0;

static void dbg(int level, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    if (!l_DebugCallback) {
        return;
    }
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    l_DebugCallback(l_DebugCallContext, level, buf);
}

static int is_guest_ptr(uint32_t a)
{
    return (a >= 0x80000000u) && (a < 0x80800000u) && ((a & 3u) == 0u);
}

static uint32_t phys(uint32_t vaddr)
{
    return vaddr & 0x007FFFFFu;
}

static uint8_t r8(uint32_t vaddr)
{
    uint32_t p = phys(vaddr);
    if (p >= g_rdram_size || !g_rdram) {
        return 0;
    }
    return g_rdram[p ^ (unsigned)g_xor];
}

static int8_t rs8(uint32_t vaddr)
{
    return (int8_t)r8(vaddr);
}

static uint16_t r16be(uint32_t vaddr)
{
    return (uint16_t)((r8(vaddr) << 8) | r8(vaddr + 1));
}

static int16_t rs16be(uint32_t vaddr)
{
    return (int16_t)r16be(vaddr);
}

static uint32_t r32be(uint32_t vaddr)
{
    return ((uint32_t)r8(vaddr) << 24) | ((uint32_t)r8(vaddr + 1) << 16) |
           ((uint32_t)r8(vaddr + 2) << 8) | (uint32_t)r8(vaddr + 3);
}

static float rf32(uint32_t vaddr)
{
    uint32_t u = r32be(vaddr);
    float f;
    memcpy(&f, &u, sizeof(f));
    return f;
}

static int is_finite_f(float f)
{
    return isfinite(f);
}

typedef struct {
    int x, y;
    unsigned btn;
} Landmark;

static int landmark_ok(int consumed, int x, int y, unsigned btn)
{
    static const Landmark lm[] = {
        /* filled by index below */
    };
    (void)lm;
    if (consumed >= 70 && consumed <= 73) {
        return x == -2 && y == 80 && btn == 0x0000;
    }
    if (consumed == 74) {
        return x == -18 && y == 82 && btn == 0x0000;
    }
    if (consumed == 75) {
        return x == -47 && y == 78 && btn == 0x0000;
    }
    if (consumed == 76) {
        return x == -63 && y == 74 && btn == 0x0000;
    }
    if (consumed == 322) {
        return x == -9 && y == 84 && btn == 0x2008;
    }
    return 1;
}

static void try_detect_xor(void)
{
    /* Probe mode/map words at D_80127630. Mode 5 is small; map 335 is 0x014F. */
    int saved = g_xor;
    for (int xorv = 0; xorv <= 3; xorv += 3) {
        g_xor = xorv;
        int16_t mode = rs16be(0x80127632);
        int16_t map = rs16be(0x80127640);
        uint32_t gl = r32be(0x8012D5B0);
        /* mode==0 is also the all-zero false-positive; require a live pointer. */
        if ((mode == 1 || mode == 2 || mode == 3 || mode == 5) &&
            (is_guest_ptr(gl) || is_guest_ptr(r32be(0x80135490)))) {
            dbg(M64MSG_INFO, "endian probe xor=%d mode=%d map=%d gl=%08x", xorv,
                (int)mode, (int)map, gl);
            return;
        }
    }
    g_xor = saved;
}

EXPORT m64p_error CALL PluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context,
                                     void (*DebugCallback)(void *, int, const char *))
{
    (void)CoreLibHandle;
    if (l_PluginInit) {
        return M64ERR_ALREADY_INIT;
    }
    l_DebugCallback = DebugCallback;
    l_DebugCallContext = Context;
    l_PluginInit = 1;
    const char *path = getenv("TOOIE_ORIGINAL_TRACE");
    if (!path || !path[0]) {
        path = "/tmp/p08-original-wooded-hollow.csv";
    }
    g_log = fopen(path, "w");
    if (!g_log) {
        dbg(M64MSG_ERROR, "cannot open trace %s", path);
        return M64ERR_FILES;
    }
    fprintf(g_log,
            "consumed_glidx,post_glidx,vi_count,gfcnt,mode,map,pad_x,pad_y,pad_btn,"
            "bs,px,py,pz,ppx,ppy,ppz,vx,vy,vz,velxz,tspd,yaw,ideal_yaw,phys_yaw,"
            "mult,accel,cam_x,cam_y,cam_z,cam_yaw,digger_x,digger_y,digger_z,"
            "act2_mid,act2_sys,act2_x,act2_y,act2_z\n");
    fflush(g_log);
    dbg(M64MSG_INFO, "demo trace writing %s", path);
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginShutdown(void)
{
    if (g_log) {
        fclose(g_log);
        g_log = NULL;
    }
    l_PluginInit = 0;
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion,
                                        int *APIVersion, const char **PluginNamePtr,
                                        int *Capabilities)
{
    if (PluginType) {
        *PluginType = M64PLUGIN_GFX;
    }
    if (PluginVersion) {
        *PluginVersion = PLUGIN_VERSION;
    }
    if (APIVersion) {
        *APIVersion = VIDEO_PLUGIN_API_VERSION;
    }
    if (PluginNamePtr) {
        *PluginNamePtr = "TooieRecomp-OriginalDemoTrace";
    }
    if (Capabilities) {
        *Capabilities = 0;
    }
    return M64ERR_SUCCESS;
}

EXPORT void CALL ChangeWindow(void) {}

EXPORT int CALL InitiateGFX(GFX_INFO Gfx_Info)
{
    g_gfx = Gfx_Info;
    g_rdram = Gfx_Info.RDRAM;
    if (Gfx_Info.version >= 2 && Gfx_Info.RDRAM_SIZE && *Gfx_Info.RDRAM_SIZE) {
        g_rdram_size = *Gfx_Info.RDRAM_SIZE;
    }
    dbg(M64MSG_INFO, "InitiateGFX rdram=%p size=%u", (void *)g_rdram, g_rdram_size);
    return 1;
}

EXPORT void CALL MoveScreen(int x, int y)
{
    (void)x;
    (void)y;
}

EXPORT void CALL ProcessDList(void) {}
EXPORT void CALL ProcessRDPList(void) {}
EXPORT void CALL RomClosed(void) {}
EXPORT int CALL RomOpen(void) { return 1; }
EXPORT void CALL ShowCFB(void) {}
EXPORT void CALL ViStatusChanged(void) {}
EXPORT void CALL ViWidthChanged(void) {}
EXPORT void CALL ReadScreen2(void *dest, int *width, int *height, int front)
{
    (void)dest;
    (void)width;
    (void)height;
    (void)front;
}
EXPORT void CALL SetRenderingCallback(void (*callback)(int)) { (void)callback; }
EXPORT void CALL FBRead(unsigned int addr) { (void)addr; }
EXPORT void CALL FBWrite(unsigned int addr, unsigned int size)
{
    (void)addr;
    (void)size;
}
EXPORT void CALL FBGetFrameBufferInfo(void *p) { (void)p; }
EXPORT void CALL ResizeVideoOutput(int width, int height)
{
    (void)width;
    (void)height;
}

EXPORT void CALL UpdateScreen(void)
{
    g_vi += 1;
    if (!g_rdram || !g_log) {
        return;
    }

    if ((g_vi & 0x3F) == 1) {
        try_detect_xor();
    }
    if (g_vi == 60 || g_vi == 600 || (g_vi % 1800) == 0) {
        int saved = g_xor;
        for (int xorv = 0; xorv <= 3; xorv += 3) {
            g_xor = xorv;
            dbg(M64MSG_INFO,
                "hex xor=%d @80127630 %02x%02x%02x%02x %02x%02x%02x%02x mode16=%d map16=%d gl=%08x ps=%08x",
                xorv, r8(0x80127630), r8(0x80127631), r8(0x80127632), r8(0x80127633),
                r8(0x80127634), r8(0x80127635), r8(0x80127636), r8(0x80127637),
                (int)rs16be(0x80127632), (int)rs16be(0x80127640), r32be(0x8012D5B0),
                r32be(0x80135490));
        }
        g_xor = saved;
    }

    int16_t mode = rs16be(0x80127632);
    int16_t map = rs16be(0x80127640);
    if (mode != 5) {
        if ((g_vi % 120) == 0) {
            dbg(M64MSG_STATUS, "vi=%u mode=%d map=%d xor=%d waiting-for-mode5", g_vi,
                (int)mode, (int)map, g_xor);
        }
        return;
    }
    if (!g_mode5_seen) {
        g_mode5_seen = 1;
        dbg(M64MSG_INFO, "mode5 reached at vi=%u map=%d xor=%d", g_vi, (int)map, g_xor);
    }

    uint32_t gl_base = r32be(0x8012D5B0);
    int32_t post_glidx = (int32_t)r32be(0x8012D5B8);
    int32_t gl_cnt = (int32_t)r32be(0x8012D5BC);
    (void)gl_cnt;
    if (post_glidx == g_prev_post || post_glidx <= 0 || !is_guest_ptr(gl_base)) {
        return;
    }
    g_prev_post = post_glidx;
    int32_t consumed = post_glidx - 1;
    uint32_t sample = gl_base + (uint32_t)consumed * 6u;
    int pad_x = (int)rs8(sample);
    int pad_y = (int)rs8(sample + 1);
    unsigned pad_btn = r16be(sample + 2);

    if (!landmark_ok(consumed, pad_x, pad_y, pad_btn)) {
        dbg(M64MSG_WARNING,
            "LANDMARK FAIL consumed=%d pad=(%d,%d,0x%04x) xor=%d — trying xor flip",
            consumed, pad_x, pad_y, pad_btn, g_xor);
        g_xor = (g_xor == 3) ? 0 : 3;
        pad_x = (int)rs8(sample);
        pad_y = (int)rs8(sample + 1);
        pad_btn = r16be(sample + 2);
        if (!landmark_ok(consumed, pad_x, pad_y, pad_btn)) {
            dbg(M64MSG_ERROR, "LANDMARK FAIL after xor flip; layout untrusted");
            g_layout_ok = 0;
        }
    } else if (consumed == 70 || consumed == 74 || consumed == 322) {
        g_layout_ok = 1;
        dbg(M64MSG_INFO, "LANDMARK PASS consumed=%d pad=(%d,%d,0x%04x)", consumed, pad_x,
            pad_y, pad_btn);
    }

    uint32_t ps_addr = r32be(0x80135490);
    float px = 0, py = 0, pz = 0, ppx = 0, ppy = 0, ppz = 0;
    float vx = 0, vy = 0, vz = 0, tspd = 0, yaw = 0, ideal = 0, phys_yaw = 0;
    float mult = 0, accel = 0;
    int32_t bs = -1;
    if (is_guest_ptr(ps_addr)) {
        uint32_t pos = r32be(ps_addr + 0xE4);
        uint32_t yawp = r32be(ps_addr + 0xF8);
        uint32_t phys = r32be(ps_addr + 0xC8);
        uint32_t bsp = r32be(ps_addr + 0x120);
        if (is_guest_ptr(pos)) {
            px = rf32(pos);
            py = rf32(pos + 4);
            pz = rf32(pos + 8);
        }
        if (is_guest_ptr(yawp)) {
            yaw = rf32(yawp);
            ideal = rf32(yawp + 4);
        }
        if (is_guest_ptr(phys)) {
            ppx = rf32(phys + 0x04);
            ppy = rf32(phys + 0x08);
            ppz = rf32(phys + 0x0C);
            vx = rf32(phys + 0x10);
            vy = rf32(phys + 0x14);
            vz = rf32(phys + 0x18);
            tspd = rf32(phys + 0x3C);
            phys_yaw = rf32(phys + 0x40);
            accel = rf32(phys + 0x44);
            mult = rf32(phys + 0x50);
        }
        if (is_guest_ptr(bsp)) {
            bs = (int32_t)r32be(bsp);
        }
    }

    float cx = 0, cy = 0, cz = 0, cam_yaw = 0;
    uint32_t cam = r32be(0x8012D500);
    if (is_guest_ptr(cam)) {
        cx = rf32(cam);
        cy = rf32(cam + 4);
        cz = rf32(cam + 8);
        float m00 = rf32(cam + 0xA4);
        float m02 = rf32(cam + 0xAC);
        cam_yaw = atan2f(-m02, m00) * (180.0f / 3.14159265f);
        if (cam_yaw < 0.0f) {
            cam_yaw += 360.0f;
        }
    }

    float dgx = 0, dgy = 0, dgz = 0;
    float a2x = 0, a2y = 0, a2z = 0;
    uint32_t a2mid = 0, a2sys = 0;
    uint32_t vec = r32be(0x80136EE0);
    if (is_guest_ptr(vec)) {
        uint32_t elem = r32be(vec);
        uint32_t begin = r32be(vec + 4);
        uint32_t end = r32be(vec + 8);
        if (elem == 0x9Cu && is_guest_ptr(begin) && is_guest_ptr(end) && end >= begin) {
            uint32_t count = (end - begin) / elem;
            for (uint32_t i = 0; i < count && i < 64; ++i) {
                uint32_t act = begin + i * 0x9Cu;
                uint32_t idw = r32be(act + 0x6C);
                uint32_t mid = idw & 0x1FFu;
                if (mid == 337u) {
                    dgx = rf32(act + 0x04);
                    dgy = rf32(act + 0x08);
                    dgz = rf32(act + 0x0C);
                } else if (a2mid == 0 && mid != 0 && mid != 1u) {
                    a2mid = mid;
                    a2sys = r32be(act + 0x10);
                    a2x = rf32(act + 0x04);
                    a2y = rf32(act + 0x08);
                    a2z = rf32(act + 0x0C);
                }
            }
        }
    }

    int32_t gfcnt = (int32_t)r32be(0x8012C764);
    float velxz = sqrtf(vx * vx + vz * vz);
    if (!is_finite_f(px) || !is_finite_f(py) || !is_finite_f(pz) || !is_finite_f(cx)) {
        dbg(M64MSG_WARNING, "non-finite float at consumed=%d", consumed);
    }

    fprintf(g_log,
            "%d,%d,%u,%d,%d,%d,%d,%d,%04x,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,"
            "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.4f,%.2f,%.2f,%.2f,%.2f,%.1f,%.2f,%.2f,%.2f,"
            "%u,%08x,%.2f,%.2f,%.2f\n",
            consumed, post_glidx, g_vi, gfcnt, (int)mode, (int)map, pad_x, pad_y, pad_btn,
            bs, px, py, pz, ppx, ppy, ppz, vx, vy, vz, velxz, tspd, yaw, ideal, phys_yaw,
            mult, accel, cx, cy, cz, cam_yaw, dgx, dgy, dgz, a2mid, a2sys, a2x, a2y, a2z);
    fflush(g_log);
    g_samples += 1;
    unsigned int dvi = g_vi - g_vi_at_last_sample;
    g_vi_at_last_sample = g_vi;
    if (consumed % 50 == 0 || consumed == 74 || consumed == 76 || consumed == 322) {
        dbg(M64MSG_INFO,
            "[DEMO-TRACE] consumed=%d post=%d vi=%u dvi=%u pos=(%.1f,%.1f,%.1f) "
            "velxz=%.1f yaw=%.1f cam_yaw=%.1f samples=%d",
            consumed, post_glidx, g_vi, dvi, px, py, pz, velxz, yaw, cam_yaw, g_samples);
    }
    /* Wooded Hollow clip ends around consumed 765/766. Stop shortly after. */
    if (consumed >= 770 && map != 335) {
        dbg(M64MSG_INFO, "demo left map 335 after consumed=%d; %d samples", consumed,
            g_samples);
    }
}
