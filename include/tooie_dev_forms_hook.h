#pragma once
#include "recomp.h"
namespace tooie_dev_forms {
// Game-thread scopes for native APIs that have no player argument (transition state and Dragon Kazooie).
class PlayerScope {
public:
    PlayerScope(uint8_t* rdram, uint32_t self);
    ~PlayerScope();
private:
    int previous;
};
void begin_map(bool enabled, bool native_entry = false);
void update_player(uint8_t* rdram, const recomp_context* ctx, uint32_t self);
uint32_t default_state(uint8_t* rdram, const recomp_context* ctx, uint32_t self);
}
extern "C" void tooie_forms_on_allocate(uint8_t*, recomp_context*);
extern "C" void tooie_forms_on_setup(uint8_t*, recomp_context*);
extern "C" void tooie_forms_on_teardown(uint8_t*, recomp_context*);
extern "C" void tooie_forms_on_reset(uint8_t*, recomp_context*);
extern "C" void tooie_forms_on_dragon_get(uint8_t*, recomp_context*);
extern "C" void tooie_forms_on_dragon_set(uint8_t*, recomp_context*);
extern "C" void tooie_forms_on_transition_form(uint8_t*, recomp_context*);
