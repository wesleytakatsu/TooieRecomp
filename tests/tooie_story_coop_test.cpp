// Production host wrappers + generated native control flow, read-only.
// World/assets/model instances/input devices are explicit synthetic leaves.
// No ROM, renderer, SDL window or real minigame traversal is exercised.
#include "tooie_story_coop.h"
#include "tooie_dev_warp_hook.h"
#include "tooie_dev_forms_hook.h"
#include "tooie_dev_forms.h"
#include "tooie_dev_cheats.h"
#include "tooie_analog_camera.h"
#include <cstdio>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cstring>
#include <set>
#include <vector>

static unsigned checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::exit(1); } } while (0)
static gpr guest(uint32_t p) { return int32_t(p); }
static constexpr uint32_t owners[8] = {0x80010000,0x80011000,0x80012000,0x80013000,
    0x80014000,0x80015000,0x80016000,0x80017000};
static int skip = 0, descriptors = 0, channels = 1, channel_calls = 0, view_calls = 0;
static int original_count = 0, native_entries = 0, route = 0, route_reads = 0;
static bool awaiting_native = false;
static bool section_mp = false, constructing = false, revert_goliath = false;
static unsigned native_count = 1, private_models = 0, placements = 0, serialized[8]{};
static uint32_t states[8]{};
static unsigned state_queries=0, state_changes=0;
static int provisional_queries = 0, returned_focus = -1, freed = 0;
static int last_goto = -1, frontend = -1, replay = 0;
static uint8_t entry_form = 1;
static recomp_context context() { recomp_context c{}; c.r29=guest(0x807FF000); return c; }
static void noop(uint8_t*,recomp_context*) {}
static void zero(uint8_t*,recomp_context* c) { c->r2=0; }
static void one(uint8_t*,recomp_context* c) { c->r2=1; }
static int index(uint8_t* rdram, uint32_t owner) { return MEM_W(0x184,guest(owner)); }
static uint8_t form(uint8_t* rdram,uint32_t owner) { return MEM_BU(0x29,MEM_W(0xA0,guest(owner))); }
static void set_form(uint8_t* rdram,uint32_t owner,uint8_t f) { MEM_B(0x29,MEM_W(0xA0,guest(owner)))=f; }
extern "C" {
void func_800F73C4(uint8_t*,recomp_context*);
void func_800F6140(uint8_t*,recomp_context*);
void func_800F6204(uint8_t*,recomp_context*);
void func_800F8B94(uint8_t*,recomp_context*);
void func_800F65D0(uint8_t*,recomp_context*);
void func_800F482C(uint8_t*,recomp_context*);
void func_800F3880(uint8_t*,recomp_context*);
void func_8008DAA8(uint8_t*,recomp_context*);
void func_800A8D70(uint8_t*,recomp_context*);
void func_80800BA8_chminisubgame(uint8_t*,recomp_context*);
// Count calls to the real original through a dispatch leaf, not a copy of its logic.
void func_800F43A8(uint8_t* rdram,recomp_context* c) {
    const int n=int(c->r4); CHECK(n>=0 && n<8);
    CHECK(MEM_W(n*4,guest(0x80135490))==0);
    const uint32_t self=owners[n];
    MEM_W(0x184,guest(self))=n;
    MEM_W(0xA0,guest(self))=guest(self+0x400);
    MEM_W(0xB8,guest(self))=guest(self+0x500);
    MEM_W(0xE4,guest(self))=guest(self+0x600);
    MEM_W(0xF8,guest(self))=guest(self+0x620);
    states[n]=n==0?1:0x74;
    set_form(rdram,self,MEM_BU(0,guest(0x8012704C)));
    auto model=*c; model.r4=guest(self);
    tooie_dev_on_primary_player_model(rdram,&model);
    c->r2=guest(self);
}
void _basetup_entrypoint_2(uint8_t* rdram,recomp_context* c) { set_form(rdram,uint32_t(c->r4),MEM_BU(0,guest(0x8012704C))); }
void func_80092EC8(uint8_t* rdram,recomp_context* c) {
    c->r6=0; c->r7=1; tooie_dev_on_model_instance(rdram,c);
}
void func_800941C8(uint8_t*,recomp_context*) {}
void func_8009359C(uint8_t*,recomp_context*) {}
void func_80100D24(uint8_t*,recomp_context* c) { private_models += c->r6==1; c->r2=1; }
void yaw_update(uint8_t*,recomp_context*) {}
void yaw_set(uint8_t*,recomp_context*) {}
void yaw_setIdeal(uint8_t*,recomp_context*) {}
void func_8009C0F8(uint8_t*,recomp_context*) { ++placements; }
void func_800A8FDC(uint8_t* rdram,recomp_context*) { ++view_calls; ++MEM_H(0,guest(0x801276D2)); }
void func_80016BDC(uint8_t*,recomp_context* c) { ++channel_calls; channels=int(c->r4); CHECK(channels==2); }
void func_800DA564(uint8_t*,recomp_context* c) { ++route_reads; CHECK(c->r4==0xA34 && c->r5==3); c->r2=route; c->r4=999; }
void func_8009E8D4(uint8_t*,recomp_context*) {}
void func_8009EA2C(uint8_t*,recomp_context* c) { c->r2=0; }
void func_8009EAAC(uint8_t*,recomp_context*) {}
void func_8009E9D8(uint8_t* rdram,recomp_context* c) { MEM_B(0,guest(0x8012704C))=uint8_t(c->r4); }
void func_8009EB00(uint8_t*,recomp_context*) {}
void func_800A3410(uint8_t* rdram,recomp_context* c) { set_form(rdram,uint32_t(c->r4),uint8_t(c->r5)); }
void func_80092778(uint8_t*,recomp_context*) {}
void func_80092B80(uint8_t*,recomp_context* c) { c->r2=0; }
void func_80093FD4(uint8_t*,recomp_context*) {}
void _badata_entrypoint_34(uint8_t*,recomp_context* c) { c->r2=1; }
void _badata_entrypoint_5(uint8_t* rdram,recomp_context* c) { MEM_W(0,c->r5)=0; MEM_W(0,c->r6)=0; }
void baanim_playForDuration_loopSmooth(uint8_t*,recomp_context*) {}
void func_800A8380(uint8_t*,recomp_context* c) { c->r2=0; }
void bs_setState(uint8_t* rdram,recomp_context* c) { ++state_changes; states[index(rdram,uint32_t(c->r4))]=uint32_t(c->r5); }
void bs_getCurrentState(uint8_t* rdram,recomp_context* c) { ++state_queries; c->r2=states[index(rdram,uint32_t(c->r4))]; }
void func_800991B0(uint8_t*,recomp_context*) {}
void _gcgoto_entrypoint_1(uint8_t*,recomp_context* c) { last_goto=int(c->r4); }
void bakey_getControllerIndex(uint8_t* rdram,recomp_context* c) { c->r2=index(rdram,uint32_t(c->r4)); }
}
namespace tooie_dev_enemy_ai { void reset() {} }
namespace tooie_dev_travel_menu { void reset() {} }

static void allocate_view(uint8_t*,recomp_context* c) { c->r2=c->r4; }
static void native_active(uint8_t* rdram,recomp_context* c) {
    const int n=int(c->r4);
    tooie_dev_on_player_active(rdram,c);
    if (constructing && n==1 && skip==0 && original_count==2 &&
        tooie_dev_cheats::two_players_active() && MEM_W(4,guest(0x80135490))!=0) { CHECK(c->r2==1); ++provisional_queries; }
}
static void serialize(uint8_t* rdram,recomp_context* c) {
    // 8009E880 has no owner argument; native teardown holds self in s0.
    const int n=index(rdram,uint32_t(c->r16)); ++serialized[n];
    if (n==1) { CHECK(!tooie_story_coop::running(rdram)); CHECK(MEM_W(0,guest(0x80135490))==0); }
    MEM_W(0,guest(0x80127044))=100+n;
}
static void free_owner(uint8_t* rdram,recomp_context* c) {
    CHECK(tooie_story_coop::owned_slot(rdram,uint32_t(c->r4))==-1); ++freed;
}
extern "C" {
int32_t* section_addresses=nullptr;
void switch_error(const char*,uint32_t,uint32_t) { CHECK(false); }
void do_break(uint32_t) { CHECK(false); }
void cop0_status_write(recomp_context*,gpr) { CHECK(false); }
gpr cop0_status_read(recomp_context*) { CHECK(false); return 0; }
void recomp_syscall_handler(uint8_t*,recomp_context*,int32_t) { CHECK(false); }
void pause_self(uint8_t*) { CHECK(false); }
bool ultramodern_preemption_hint() { return false; }
void ultramodern_service_preemption(uint8_t*,void*) { CHECK(false); }
recomp_func_t* get_function(int32_t address) {
    switch (uint32_t(address)) {
    case 0x800F73C4: return func_800F73C4;
    case 0x800F61E4: return tooie_dev_on_normal_player_allocate;
    case 0x800F6140: return func_800F6140;
    case 0x800F6204: return func_800F6204;
    case 0x800F43A8: return tooie_forms_on_allocate;
    case 0x800F8B94: return func_800F8B94;
    case 0x800F65D0: return native_active;
    case 0x800F3880: return func_800F3880;
    case 0x80098E64: return tooie_forms_on_teardown;
    case 0x8008DAA8: return func_8008DAA8;
    case 0x8001B084: return free_owner;
    case 0x800A3274: return [](uint8_t* rdram,recomp_context* c) { c->r2=form(rdram,uint32_t(c->r4)); };
    case 0x800A3410: return func_800A3410;
    case 0x80095738: return [](uint8_t*,recomp_context* c) { c->r2=revert_goliath; };
    case 0x8009E880: return serialize;
    case 0x8009E9D8: return tooie_forms_on_transition_form;
    case 0x800DB9B0: return [](uint8_t*,recomp_context* c) { c->r2=skip==1; };
    case 0x800887B0: return [](uint8_t*,recomp_context* c) { c->r2=skip==2; };
    case 0x800F99E8: return [](uint8_t*,recomp_context* c) { c->r2=skip==3; };
    case 0x800F80D8: return [](uint8_t*,recomp_context* c) { if (awaiting_native) { original_count=int(c->r17); ++native_entries; awaiting_native=false; } };
    case 0x80100094: return [](uint8_t*,recomp_context* c) { c->r2=guest(0x80070000); };
    case 0x8009055C: case 0x80090708: case 0x80101FDC: return noop;
    case 0x800F5898: return [](uint8_t* rdram,recomp_context* c) { int n=0; for(int i=0;i<8;++i) n+=MEM_W(i*4,guest(0x80135490))!=0; c->r2=n; };
    case 0x800F6BE4: return one; // Synthetic role predicate, independent of allocator provenance.
    case 0x800F8B88: return [](uint8_t*,recomp_context* c) { c->r2=2; };
    case 0x800878A0: return [](uint8_t*,recomp_context* c) { c->r2=c->r4==0xA?0:1; };
    case 0x800F8128: return [](uint8_t*,recomp_context* c) { returned_focus=int(c->r4); };
    case 0x800A907C: return allocate_view;
    case 0x800B28C4: return [](uint8_t*,recomp_context* c) { c->r2=guest(0x80060000); };
    case 0x800B296C: return [](uint8_t*,recomp_context* c) { c->r2=guest(0x80060000+descriptors*0x3C); };
    case 0x800EA05C: return [](uint8_t* rdram,recomp_context* c) { c->r2=MEM_HU(0,guest(0x80132DC2)); };
    case 0x80088318: case 0x80088258: return zero;
    case 0x80088260: return [](uint8_t*,recomp_context* c) { c->r2=section_mp; };
    case 0x80088368: return [](uint8_t*,recomp_context* c) { c->r2=native_count; };
    case 0x800A877C: return noop;
    // Teardown world/behavior/model leaves, not substitutes for its activity/form branch.
    case 0x8009E6F8: case 0x8009CBDC: case 0x800A81C4: case 0x800EA068:
    case 0x8008E39C: case 0x8009E674: case 0x80084FE0: case 0x8009E71C: return zero;
    case 0x8009EB0C: case 0x8009EAF4: case 0x8009E9FC: case 0x8009EB18:
    case 0x8009E5C8: case 0x80084450: case 0x8008E6BC: case 0x80091054:
    case 0x80084478: case 0x80091648: case 0x80084AD0: case 0x80094538:
    case 0x80092A1C: case 0x800A22A8: case 0x8008C864: case 0x80095C10:
    case 0x80084D28: case 0x80084F10: case 0x80084FB8: case 0x8009D5E0:
    case 0x800A2058: case 0x80084FF0: case 0x800850B8: case 0x800A2D70:
    case 0x80085348: case 0x80085170: case 0x80084908: case 0x80084770:
    case 0x80084940: case 0x8009E388: case 0x80084CB0: case 0x80091E18:
    case 0x80091AB8: case 0x80084620: case 0x80085508: case 0x80091EA8:
    case 0x800854C8: case 0x8009AD88: case 0x800A4160: case 0x800F8EBC:
    case 0x800A91F4: case 0x800A9768: case 0x800A93F8:
    case 0x800A93E4: case 0x80091BF0: case 0x800F7E64: case 0x800F8268:
    case 0x800F8294: case 0x800F832C: case 0x800F8358: case 0x800F608C:
    case 0x800F8D80: case 0x80084FB0: return noop;
    case 0x800D395C: return [](uint8_t*,recomp_context* c) { c->r2=replay; };
    case 0x80088188: return [](uint8_t*,recomp_context*) { frontend=11; };
    case 0x80088180: return [](uint8_t*,recomp_context*) { frontend=10; };
    case 0x80087A58: return _gcgoto_entrypoint_1;
    default: std::fprintf(stderr,"Unexpected native leaf %08X\n",uint32_t(address)); CHECK(false); return nullptr;
    }
}
}

static void create(uint8_t* rdram,unsigned map,int count=1,unsigned subtype=0) {
    MEM_H(0,guest(0x80132DC2))=map; MEM_B(0,guest(0x8012762C))=subtype;
    MEM_H(0,guest(0x80127632))=3;
    for(int n=0;n<descriptors;++n) MEM_H(0x22,guest(0x80060000+n*0x3C))=map;
    MEM_B(0,guest(0x8012704C))=entry_form;
    auto layout=context(); layout.r4=map; func_800A8D70(rdram,&layout);
    auto c=context(); c.r4=count; c.r16=123; c.r17=456;
    const auto saved_sp=c.r29;
    const int entries=native_entries; awaiting_native=true; constructing=true;
    tooie_dev_on_create_players(rdram,&c); constructing=false;
    CHECK(native_entries==entries+1 && !awaiting_native);
    CHECK(c.r29==saved_sp && c.r16==123 && c.r17==456);
}
int main() {
    using namespace tooie_story_coop;
    // Independent room list detects registry omissions, duplicates and broad ranges.
    constexpr unsigned rooms[]={0xC6,0xC8,0xC9,0xDC,0xDE,0xDF,0xE0,0xE5,0xE4,
        0x124,0x13B,0x13C,0x181,0x123,0x162,0x10A,0x12E,0x12F,0x130,0x13D,0x137,0x139,0x15F};
    std::set<unsigned> flags;
    for(unsigned n=0;n<stages.size();++n) { CHECK(stages[n].flag==0xBE3+n); CHECK(flags.insert(stages[n].flag).second); CHECK(challenge_room(stages[n].room)); }
    for(unsigned map: rooms) for(bool opt:{false,true}) for(bool unlock:{false,true})
        for(int count:{0,1,2,3,4}) for(unsigned subtype:{0u,0x1Bu})
            CHECK(!decide_creation({uint16_t(map),uint8_t(subtype),count,opt,unlock}).inject_second_player);
    for(unsigned map:{0xCA,0xCB,0xCC,0xCD,0xCE,0x164,0x165,0x17B,0x17C,0x17D,0x17F,0x180,0x182,0x183,0x185,0x186,0x188,0x189})
        CHECK(!decide_creation({uint16_t(map),0,1,true,true}).inject_second_player);
    for(unsigned map:{0xC7,0xBB,0xDD,0x12D,0x163,0x187,0x1A9,0xD6,0x136,0x108,0x10B,0x142,0xB8,0x161,0x16F,0x170,0x151,0x166,0x178,0xFFFF})
        for(unsigned r=0;r<8;++r) CHECK(decide_creation({uint16_t(map),0,1,true,true,uint8_t(r)}).inject_second_player==(map!=0xD6||r!=6));
    for(unsigned map=0;map<0x200;++map) for(int count:{0,1,2,3,4}) {
        CHECK(!decide_creation({uint16_t(map),0x1B,count,true,true}).inject_second_player);
        CHECK(!decide_creation({uint16_t(map),0,count,false,true}).inject_second_player);
        CHECK(!decide_creation({uint16_t(map),0,count,true,false}).inject_second_player);
    }
    std::vector<uint8_t> memory(8*1024*1024); auto* rdram=memory.data();
    auto settings=tooie_dev_cheats::Settings{}; settings.two_players=true;
    tooie_dev_cheats::set_settings(settings); tooie_dev_cheats::set_unlocked(true);
    create(rdram,0x142); CHECK(running(rdram)); CHECK(view_calls==1 && channel_calls==1 && channels==2);
    CHECK(original_count==2);
    CHECK(private_models==2 && placements==1 && provisional_queries>0 && returned_focus==1);
    // Actual delayed placement/release uses the production clock and owner guard.
    auto update=context(); update.r4=guest(owners[1]);
    tooie_dev_on_player_update(rdram,&update);
    const unsigned before_release=placements;
    std::this_thread::sleep_for(std::chrono::milliseconds(2250));
    update=context(); update.r4=guest(owners[1]); tooie_dev_on_player_update(rdram,&update);
    CHECK(placements==before_release+1 && states[1]==0x74);
    update=context(); update.r4=guest(owners[1]); tooie_dev_on_player_update(rdram,&update);
    CHECK(states[1]==1 && state_changes==1);
    for(unsigned map:rooms) {
        const int vc=view_calls, cc=channel_calls; create(rdram,map);
        CHECK(original_count==1 && !running(rdram) && view_calls==vc && channel_calls==cc && channels==2);
        CHECK(MEM_H(0,guest(0x801276D2))==1 && MEM_W(4,guest(0x80135490))==0);
        create(rdram,map); CHECK(!running(rdram)); // Same-room retry is still native.
    }
    create(rdram,0x142); CHECK(running(rdram));
    for(int p=0;p<2;++p) publish_input(rdram,owners[p],p==0?2:0);
    CHECK(input_player(0,true,0x8400,0,0)==-1);
    CHECK(input_player(0,false,0,0,0)==1); CHECK(input_player(2,false,0,0,0)==0);
    tooie_analog_camera::set_enabled(true); tooie_analog_camera::publish(0,true);
    create(rdram,0x181); CHECK(!tooie_analog_camera::reserve(0)); CHECK(input_player(0,true,0x8400,0,0)==-1);
    create(rdram,0x142); publish_input(rdram,owners[1],0);
    CHECK(input_player(0,true,0x8400,0,0)==-1); CHECK(input_player(0,false,0,0,0)==1);
    route=6; const int reads=route_reads; create(rdram,0xD6); CHECK(route_reads==reads+1 && !running(rdram));
    route=0; CHECK(!running(rdram)); create(rdram,0xD6); CHECK(running(rdram));
    for(int predicate:{1,2,3}) for(int records:{0,2}) {
        skip=predicate; descriptors=records; create(rdram,0x142); CHECK(!running(rdram));
        CHECK(MEM_W(0,guest(0x80135490))!=0);
        if(records==2) CHECK(MEM_W(4,guest(0x80135490))!=0);
    }
    skip=descriptors=0;
    for(unsigned count:{1,2,4}) {
        section_mp=true; native_count=count; const int vc=view_calls,cc=channel_calls;
        create(rdram,0x142,int(count),0x1B);
        CHECK(original_count==int(count) && !running(rdram) && view_calls==vc && channel_calls==cc);
        CHECK(MEM_HU(0,guest(0x801276D2))==count);
    }
    section_mp=false; create(rdram,0x142); CHECK(running(rdram));
    set_form(rdram,owners[0],6); set_form(rdram,owners[1],0xE); revert_goliath=true;
    auto destroy=context(); destroy.r4=0; func_800F482C(rdram,&destroy);
    CHECK(!running(rdram) && serialized[0]==1 && serialized[1]==1 && freed>=2);
    CHECK(MEM_W(0,guest(0x80127044))==100 && MEM_BU(0,guest(0x8012704C))==6);
    CHECK(owned_slot(rdram,owners[1])==-1); // Same-address native replacement is not J2.
    MEM_W(4,guest(0x80135490))=guest(owners[1]); CHECK(!extra_activity(rdram,1));
    revert_goliath=false; entry_form=0xC; create(rdram,0x181);
    CHECK(form(rdram,owners[0])==0xC); // Native event entry wins over cached Bee.
    // An auxiliary at the same address/camera must not overwrite saved J2 or release it.
    MEM_W(4,guest(0x80135490))=guest(owners[1]); set_form(rdram,owners[1],9);
    const auto prior_queries=state_queries, prior_places=placements;
    update=context(); update.r4=guest(owners[1]); tooie_dev_on_player_update(rdram,&update);
    CHECK(state_queries==prior_queries && placements==prior_places);
    auto auxiliary=context(); auxiliary.r4=guest(owners[1]); tooie_forms_on_teardown(rdram,&auxiliary);
    entry_form=1; create(rdram,0x142); CHECK(form(rdram,owners[1])==0xD); // Native Goliath -> Mumbo handoff.
    // Turning the option off does not destroy existing owners or their model isolation.
    settings.two_players=false; tooie_dev_cheats::set_settings(settings);
    CHECK(created_pair(rdram) && !running(rdram));
    const auto before_private=private_models;
    auto model=context(); model.r4=guest(owners[1]); tooie_dev_on_primary_player_model(rdram,&model);
    CHECK(private_models==before_private+1);
    // Native counts and disabled/locked state pass through without promotion helpers.
    for(int request:{0,1,2,3,4}) {
        const int vc=view_calls,cc=channel_calls;
        create(rdram,0x142,request);
        CHECK(original_count==request && view_calls==vc && channel_calls==cc && !running(rdram));
    }
    settings.two_players=true; tooie_dev_cheats::set_settings(settings);
    tooie_dev_cheats::set_unlocked(false);
    const int vc=view_calls,cc=channel_calls;
    create(rdram,0x142); CHECK(original_count==1 && view_calls==vc && channel_calls==cc);
    tooie_dev_cheats::set_unlocked(true); CHECK(!running(rdram));
    create(rdram,0x181); CHECK(!running(rdram)); // Enabling inside a challenge never spawns J2.
    create(rdram,0x142); CHECK(running(rdram));
    // The game can move a live player and rewrite its table slot with no teardown (seen on Windows in map 0xAF:
    // J1 801F5DA0 -> 801F55C0). The committed pair follows each slot; a slot holding another player is not adopted.
    {
        const uint32_t moved[2] = {0x80018000, 0x80019000};
        for (int n = 0; n < 2; ++n) {
            for (uint32_t off = 0; off < 0x700; off += 4) MEM_W(off, guest(moved[n])) = MEM_W(off, guest(owners[n]));
            MEM_W(n * 4, guest(0x80135490)) = guest(moved[n]);
            CHECK(running(rdram) && owned_slot(rdram, moved[n]) == n && owned_slot(rdram, owners[n]) == -1);
        }
        CHECK(owns_second(rdram, moved[1]) && extra_activity(rdram, 1));
        MEM_W(0x184, guest(owners[7])) = 0; MEM_W(4, guest(0x80135490)) = guest(owners[7]);
        CHECK(!running(rdram) && owned_slot(rdram, owners[7]) == -1);
        MEM_W(4, guest(0x80135490)) = guest(moved[1]); CHECK(running(rdram));
        for (int n = 0; n < 2; ++n) MEM_W(n * 4, guest(0x80135490)) = guest(owners[n]);
        CHECK(running(rdram) && owned_slot(rdram, owners[0]) == 0 && owned_slot(rdram, owners[1]) == 1);
    }
    // Cancellation/drain never consumes native MP/absent-owner input.
    auto& menus=tooie_dev_forms::menus(); menus.request_open(1); menus.publish(1,false,{},1,false);
    CHECK(!menus.input(1,true,true,0x8400,0,0,0)); CHECK(!menus.snapshot()[1].open);
    auto reset_ctx=context(); tooie_forms_on_reset(rdram,&reset_ctx); CHECK(!running(rdram));
    // Replay result frontend is independent of later player creation.
    for(int rep:{0,1}) for(int choice:{0,1}) {
        replay=rep; last_goto=frontend=-1;
        auto c=context(); c.r4=0; c.r5=0xCEB; c.r6=choice;
        func_80800BA8_chminisubgame(rdram,&c);
        if(rep) CHECK(frontend==(choice==1?11:10) && last_goto==-1);
        else CHECK(last_goto==(choice==1?0x181:0x1A9) && frontend==-1);
        CHECK(!running(rdram));
    }
    std::printf("PASS: %u checks; catalog, real creation/allocator/descriptor/layout/teardown/activity, input rearm, forms, Saucer and Mini-Sub result dispatch. World/assets are synthetic; gameplay NOT_RUN.\n",checks);
}
