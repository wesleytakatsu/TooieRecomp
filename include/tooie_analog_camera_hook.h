#pragma once
#include "recomp.h"

namespace tooie_analog_camera {
void refresh_player(uint8_t* rdram, const recomp_context* ctx, uint32_t self);
// Scope is limited to one native camera update, before its orbit filters/collision solver.
class UpdateScope {
public:
    UpdateScope(uint8_t* rdram, const recomp_context* ctx);
    ~UpdateScope();
private:
    uint32_t previous_node;
    float previous_yaw, previous_pitch;
};
void reset_motion();
}
extern "C" void tooie_camera_filter_yaw(uint8_t*, recomp_context*);
extern "C" void tooie_camera_filter_pitch(uint8_t*, recomp_context*);
