#include "tooie_analog_camera.h"
#include "tooie_story_coop.h"
#include "tooie_analog_camera_hook.h"
#include <algorithm>
#include <bit>
#include <cmath>

extern "C" {
void func_80112C3C(uint8_t*, recomp_context*);
void func_80115750(uint8_t*, recomp_context*);
void func_80091E80(uint8_t*, recomp_context*);
void func_8009CC68(uint8_t*, recomp_context*);
void bakey_getControllerIndex(uint8_t*, recomp_context*);
void bakey_held(uint8_t*, recomp_context*);
void func_800D8FF8(uint8_t*, recomp_context*);
}

namespace {
gpr guest(uint32_t address) { return int32_t(address); }
bool pointer(uint32_t p) { return p >= 0x80000000u && p < 0x807FFC00u && !(p & 3); }
struct Frame { uint32_t node = 0; float yaw = 0, pitch = 0; };
thread_local Frame frame;
struct PlayerMotion { uint32_t node = 0, mode = 0; int controller = -1; tooie_analog_camera::Motion motion; };
std::array<PlayerMotion, tooie_analog_camera::kControllers> motions{};
recomp_context local_context(const recomp_context* ctx, uint32_t a0, uint32_t a1 = 0) {
    auto local = *ctx;
    local.r29 = ADD32(ctx->r29, -0x60);
    local.r4 = guest(a0);
    local.r5 = guest(a1);
    return local;
}
int controller_index(uint8_t* rdram, const recomp_context* ctx, uint32_t self) {
    if (!pointer(self) || !pointer(MEM_W(0x40, guest(self)))) return -1;
    const uint32_t key = MEM_W(0x40, guest(self));
    if (!pointer(MEM_W(0, guest(key)))) return -1;
    auto local = local_context(ctx, self);
    bakey_getControllerIndex(rdram, &local);
    const int n = int32_t(local.r2);
    return n >= 0 && n < tooie_analog_camera::kControllers ? n : -1;
}
uint32_t player_node(uint8_t* rdram, uint32_t self) {
    const unsigned camera = MEM_W(0x184, guest(self));
    if (camera >= 8) return 0; // native camera table has eight entries (800A95C4)
    const uint32_t top = MEM_W(camera * 4, guest(0x80127728));
    return pointer(top) ? uint32_t(MEM_W(4, guest(top))) : 0;
}
bool eligible(uint8_t* rdram, const recomp_context* ctx, uint32_t self, uint32_t node) {
    using namespace tooie_analog_camera;
    if (!settings().enabled || !pointer(self) || !pointer(node) ||
        MEM_H(0, guest(0x80127632)) != 3 || MEM_BU(0, guest(0x8012762C)) == 0x1B ||
        MEM_BU(0, guest(0x80127642)) || special_map(MEM_HU(0, guest(0x80132DC2)))) return false;
    const uint32_t form = MEM_W(0xA0, guest(self));
    if (!pointer(form) || MEM_BU(0x29, guest(form)) == 9) return false; // Breegull Blaster
    if (player_node(rdram, self) != node || !MEM_BU(0x70, guest(node)) || MEM_BU(0x72, guest(node))) return false;
    const unsigned owner = MEM_W(0x68, guest(node));
    if (owner >= kControllers || uint32_t(MEM_W(owner * 4, guest(0x80135490))) != self) return false;
    if (owner != 0 && (!tooie_story_coop::created_pair(rdram) ||
        tooie_story_coop::owned_slot(rdram, self) != 1)) return false;
    const uint32_t top = MEM_W(0x64, guest(node));
    if (!pointer(top) || MEM_H(0x16, guest(top)) != 2) return false; // native ba-camera class; special callbacks excluded below
    const unsigned mode = MEM_W(0x6C, guest(node));
    if (mode >= 27 || !MEM_H(0xC + mode * 16, guest(0x801247A0))) return false;
    const uint32_t update = MEM_W(4 + mode * 16, guest(0x801247A0));
    // Resident thunks of string/pan/pivot only. Unknown or special camera controllers stay native.
    if (update != 0x80089160u && update != 0x800891B8u && update != 0x80089488u) return false;
    if (!pointer(MEM_W(0x48, guest(self))) || !pointer(MEM_W(0xF0, guest(self)))) return false;
    auto local = local_context(ctx, self, 0x40);
    func_80091E80(rdram, &local);
    if (!local.r2) return false; // native player input is disabled
    local = local_context(ctx, self);
    func_8009CC68(rdram, &local);
    return local.r2 == 0; // same additional player-state gate used by native C-rotation (800A4878)
}
}

namespace tooie_analog_camera {
void refresh_player(uint8_t* rdram, const recomp_context* ctx, uint32_t self) {
    if (!settings().enabled) return;
    const int controller = controller_index(rdram, ctx, self);
    if (controller >= 0) publish(controller, eligible(rdram, ctx, self, player_node(rdram, self)));
}

UpdateScope::UpdateScope(uint8_t* rdram, const recomp_context* ctx)
    : previous_node(frame.node), previous_yaw(frame.yaw), previous_pitch(frame.pitch) {
    frame = {};
    if (!settings().enabled) { motions = {}; return; }
    const uint32_t node = ctx->r4;
    if (!pointer(node)) return;
    const unsigned owner = MEM_W(0x68, guest(node));
    if (owner >= kControllers) return;
    const uint32_t self = MEM_W(owner * 4, guest(0x80135490));
    // An auxiliary/retired node must not revoke the live owner's input publication or smoothing.
    if (!pointer(self) || player_node(rdram, self) != node) return;
    const int controller = controller_index(rdram, ctx, self);
    if (controller < 0) return;
    const bool active = eligible(rdram, ctx, self, node);
    publish(controller, active);
    auto& state = motions[owner];
    const uint32_t mode = MEM_W(0x6C, guest(node));
    if (!active || state.node != node || state.mode != mode || state.controller != controller)
        state = {node, mode, controller, {}};
    if (!active) return;
    const Sample input = sample(controller);
    auto local = local_context(ctx, self, 1); // BUTTON_Z, independent of host binding
    bakey_held(rdram, &local);
    if (!input.reserved || local.r2) { state.motion = {}; return; }
    local = local_context(ctx, self, 3); // R retains native recenter priority
    bakey_held(rdram, &local);
    if (local.r2) { state.motion = {}; return; }
    local = local_context(ctx, 0);
    func_800D8FF8(rdram, &local);
    const Axes delta = state.motion.step(input.axes, local.f0.fl, settings());
    frame = {node, delta.x, delta.y};
}
UpdateScope::~UpdateScope() { frame = {previous_node, previous_yaw, previous_pitch}; }
void reset_motion() { motions = {}; frame = {}; reset(); }
}

extern "C" void tooie_camera_filter_yaw(uint8_t* rdram, recomp_context* ctx) {
    const float angle = std::bit_cast<float>(uint32_t(ctx->r5));
    if (tooie_analog_camera::settings().enabled && frame.node == uint32_t(ctx->r4) && frame.yaw != 0 && std::isfinite(angle)) {
        float target = std::fmod(angle + frame.yaw, 360.0f);
        if (target < 0) target += 360.0f;
        ctx->r5 = guest(std::bit_cast<uint32_t>(target));
        frame.yaw = 0; // one axis application per native camera tick
    }
    func_80112C3C(rdram, ctx); // native angular restrictions, followed by native collision
}
extern "C" void tooie_camera_filter_pitch(uint8_t* rdram, recomp_context* ctx) {
    const float angle = std::bit_cast<float>(uint32_t(ctx->r5));
    if (tooie_analog_camera::settings().enabled && frame.node == uint32_t(ctx->r4) && frame.pitch != 0 && std::isfinite(angle)) {
        const float target = std::clamp(std::remainder(angle, 360.0f) + frame.pitch, -75.0f, 75.0f);
        ctx->r5 = guest(std::bit_cast<uint32_t>(target));
        frame.pitch = 0;
    }
    func_80115750(rdram, ctx); // native pitch/terrain limits still apply
}
