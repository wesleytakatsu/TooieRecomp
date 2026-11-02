#include "support/story_coop_fixture.h"
namespace tooie_dev_cheats { bool two_players_active() { return true; } }
// Production input policy, config callbacks, scopes and native orbit filters/controller.
// Synthetic memory, vector math services and collision sink; no ROM, game or renderer.
#include "tooie_analog_camera.h"
#include "tooie_analog_camera_hook.h"
#include "banjo_config.h"
#include "librecomp/config.hpp"
#include "librecomp/files.hpp"
#include "librecomp/game.hpp"
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <vector>

using namespace tooie_analog_camera;
static int checks, collisions;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::exit(1); } } while(0)
static bool approximately(float a, float b, float eps = 0.0001f) { return std::fabs(a-b) < eps; }
static gpr guest(uint32_t a) { return int32_t(a); }
static uint32_t bits(float f) { return std::bit_cast<uint32_t>(f); }
static void put(uint8_t* rdram, uint32_t a, float f) { MEM_W(0, guest(a)) = bits(f); }
static float value(uint8_t* rdram, uint32_t a) { return std::bit_cast<float>(uint32_t(MEM_W(0,guest(a)))); }
static std::filesystem::path config_dir;
std::filesystem::path recomp::get_config_path() { return config_dir; }
std::ifstream recomp::open_input_backup_file(const std::filesystem::path& p, std::ios_base::openmode m) { return std::ifstream(p.string()+".bak",m); }
std::ofstream recomp::open_output_file_with_backup(const std::filesystem::path& p, std::ios_base::openmode m) { return std::ofstream(p,m); }
bool recomp::finalize_output_file_with_backup(const std::filesystem::path&) { return true; }
const recomp::Version& recomp::get_project_version() { static recomp::Version v; return v; }
extern "C" {
void ncbastring_entrypoint_3(uint8_t*, recomp_context*);
void func_800EE7F8(uint8_t*, recomp_context*);
void func_800EFB24(uint8_t*, recomp_context*);
void func_800EE780(uint8_t*, recomp_context*);
void func_800BCCF4(uint8_t*, recomp_context*);
void func_801107F0(uint8_t*, recomp_context*);
void func_800F1DCC(uint8_t*, recomp_context*);
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
static constexpr uint32_t player(int n) { return 0x80010000 + n*0x1000; }
static constexpr uint32_t node(int n) { return 0x80020000 + n*0x1000; }
static constexpr uint32_t key(int n) { return player(n)+0x300; }
static constexpr uint32_t buffer(int n) { return player(n)+0x400; }
static constexpr uint32_t top(int n) { return node(n)+0x500; }
static constexpr uint32_t pitch(int n) { return node(n)+0x600; }
static constexpr uint32_t yaw(int n) { return node(n)+0x700; }
static void zero(uint8_t*, recomp_context* c) { c->r2=0; c->f0.fl=0; }
static void noop(uint8_t*, recomp_context*) {}
static void target(uint8_t* rdram, recomp_context* c) {
    for(int i=0;i<12;i+=4) put(rdram,uint32_t(c->r5)+i,0);
}
static void spherical(uint8_t* rdram, recomp_context* c) {
    const float x=value(rdram,uint32_t(c->r4)), y=value(rdram,uint32_t(c->r4)+4), z=value(rdram,uint32_t(c->r4)+8);
    put(rdram,uint32_t(c->r5),std::atan2(y,std::hypot(x,z))*180.0f/3.14159265f);
    put(rdram,uint32_t(c->r5)+4,std::atan2(x,z)*180.0f/3.14159265f);
}
static void cartesian(uint8_t* rdram, recomp_context* c) {
    const float p=value(rdram,uint32_t(c->r5))*3.14159265f/180.0f;
    const float y=value(rdram,uint32_t(c->r5)+4)*3.14159265f/180.0f;
    const float length=std::bit_cast<float>(uint32_t(c->r6));
    put(rdram,uint32_t(c->r4),std::sin(y)*std::cos(p)*length);
    put(rdram,uint32_t(c->r4)+4,std::sin(p)*length);
    put(rdram,uint32_t(c->r4)+8,std::cos(y)*std::cos(p)*length);
}
extern "C" recomp_func_t* get_function(int32_t address) {
    switch(uint32_t(address)) {
    case 0x800BCCF4: return func_800BCCF4;
    case 0x800BCE84: return [](uint8_t* rdram,recomp_context* c) { c->r2=MEM_BU(0x20+int(c->r5),c->r4); };
    case 0x800F1DCC: return func_800F1DCC;
    case 0x80013728: return [](uint8_t*,recomp_context* c) { c->f0.fl=std::remainder(c->f12.fl,360.0f); };
    case 0x80089160: return ncbastring_entrypoint_3;
    case 0x801107F0: return func_801107F0;
    case 0x800EE7F8: return func_800EE7F8;
    case 0x800EFB24: return func_800EFB24;
    case 0x800EE780: return func_800EE780;
    case 0x80089468: return zero;
    case 0x80113978: return noop;
    case 0x80112524: case 0x80112550: return target;
    case 0x80115358: return [](uint8_t*,recomp_context* c) { c->f0.fl=std::bit_cast<float>(uint32_t(c->r5)); };
    case 0x800EEF94: return [](uint8_t* rdram,recomp_context* c) {
        c->f0.fl=std::hypot(value(rdram,uint32_t(c->r4)),value(rdram,uint32_t(c->r4)+4),value(rdram,uint32_t(c->r4)+8));
    };
    case 0x800F1A88: return spherical;
    case 0x800EEBF0: return cartesian;
    case 0x80115750: return tooie_camera_filter_pitch;
    case 0x80112C3C: return tooie_camera_filter_yaw;
    case 0x80111CC8: return [](uint8_t* rdram,recomp_context* c) {
        ++collisions;
        // Collision receives the adjusted candidate; stand in for a clear world and publish it.
        for(int i=0;i<12;i+=4) MEM_W(0x74+i,c->r4)=MEM_W(i,c->r5);
        c->r2=0;
    };
    case 0x80110A68: return target;
    case 0x80110790: return noop;
    default: std::fprintf(stderr,"Unexpected native call %08x\n",unsigned(address)); CHECK(false); return nullptr;
    }
}

static recomp_context context(uint32_t camera) {
    recomp_context c{}; c.r29=guest(0x807FF000); c.r4=guest(camera); return c;
}
static void prepare(uint8_t* rdram,int n) {
    MEM_H(0,guest(0x80127632))=3;
    MEM_H(0,guest(0x80132DC2))=0x142;
    put(rdram,0x8012C760,1.0f/30);
    MEM_W(n*4,guest(0x80135490))=player(n);
    MEM_W(0x40,guest(player(n)))=key(n);
    MEM_W(0,guest(key(n)))=buffer(n);
    MEM_B(6,guest(buffer(n)))=n;
    MEM_W(0x48,guest(player(n)))=player(n)+0x450;
    MEM_B(3,guest(player(n)+0x450))=1;
    MEM_B(0,guest(player(n)+0x450))=0x40;
    MEM_W(0xF0,guest(player(n)))=player(n)+0x500;
    MEM_W(0xA0,guest(player(n)))=player(n)+0x600;
    MEM_B(0x29,guest(player(n)+0x600))=1;
    MEM_W(0x184,guest(player(n)))=n;
    MEM_W(n*4,guest(0x80127728))=top(n);
    MEM_W(4,guest(top(n)))=node(n);
    MEM_H(0x16,guest(top(n)))=2;
    MEM_W(0x64,guest(node(n)))=top(n);
    MEM_W(0x68,guest(node(n)))=n;
    MEM_W(0x6C,guest(node(n)))=8;
    MEM_B(0x70,guest(node(n)))=1;
    MEM_W(8*16+4,guest(0x801247A0))=0x80089160;
    MEM_H(8*16+0xC,guest(0x801247A0))=1;
    MEM_W(0x3C,guest(node(n)))=pitch(n);
    MEM_W(0x60,guest(node(n)))=yaw(n);
    MEM_W(0x5C,guest(node(n)))=node(n)+0x800;
    put(rdram,node(n)+0x74,0); put(rdram,node(n)+0x78,30); put(rdram,node(n)+0x7C,300);
}
static std::array<float,3> tick(uint8_t* rdram,int n,Axes axes) {
    auto c=context(node(n));
    on_input(MEM_BU(6,guest(buffer(n))),axes,0,0,false,true);
    {
        UpdateScope scope(rdram,&c);
        ncbastring_entrypoint_3(rdram,&c);
    }
    return {value(rdram,node(n)+0x74),value(rdram,node(n)+0x78),value(rdram,node(n)+0x7C)};
}
static void native_tests() {
    reset_motion(); set_enabled(true); set_invert(0); set_sensitivity(3);
    std::vector<uint8_t> memory(8*1024*1024); auto* rdram=memory.data();
    prepare(rdram,0); prepare(rdram,1);
    commit_fixture_story_pair(rdram);
    for(int n=0;n<2;++n) { auto c=context(player(n)); refresh_player(rdram,&c,player(n)); CHECK(reserve(n)); }
    const auto p0=tick(rdram,0,{1,0}), p1=tick(rdram,1,{-1,0});
    CHECK(p0[0]>0 && p1[0]<0); CHECK(approximately(p0[0],-p1[0])); CHECK(collisions==2);
    const auto still=tick(rdram,0,{}); CHECK(approximately(still[0],p0[0]));
    const auto up=tick(rdram,0,{0,1}); CHECK(up[1]>still[1]);
    // Native pitch restriction wins over manual input, before collision receives the candidate.
    MEM_B(0x12,guest(pitch(0)))=1; put(rdram,pitch(0)+4,5);
    const auto limited=tick(rdram,0,{0,1});
    CHECK(approximately(std::atan2(limited[1],std::hypot(limited[0],limited[2]))*180.0f/3.14159265f,5,0.001f));
    MEM_B(0x12,guest(pitch(0)))=0;
    // Native Z is checked again, even if the latest host sample predates that button press.
    MEM_B(0x21,guest(buffer(0)))=1;
    const auto crouched=tick(rdram,0,{1,1}); CHECK(approximately(crouched[0],limited[0])); CHECK(approximately(crouched[1],limited[1]));
    MEM_B(0x21,guest(buffer(0)))=0;
    MEM_B(0x23,guest(buffer(0)))=1;
    const auto recenter=tick(rdram,0,{1,1}); CHECK(approximately(recenter[0],crouched[0]));
    MEM_B(0x23,guest(buffer(0)))=0;
    auto c=context(player(0));
    for(unsigned map : {0xC6u,0xDCu,0xE4u,0x10Au,0x123u,0x124u,0x137u,0x139u,0x13Du,0x181u,0x13Bu,0x13Cu,0x161u,0x162u,0x182u}) {
        MEM_H(0,guest(0x80132DC2))=map; refresh_player(rdram,&c,player(0)); CHECK(!reserve(0)); CHECK(reserve(1));
    }
    MEM_H(0,guest(0x80132DC2))=0x142;
    MEM_B(0x29,guest(player(0)+0x600))=9; refresh_player(rdram,&c,player(0)); CHECK(!reserve(0));
    MEM_B(0x29,guest(player(0)+0x600))=1;
    for(unsigned mode : {0u,1u,2u,4u,5u}) { MEM_H(0,guest(0x80127632))=mode; refresh_player(rdram,&c,player(0)); CHECK(!reserve(0)); }
    MEM_H(0,guest(0x80127632))=3;
    MEM_B(0,guest(0x8012762C))=0x1B; refresh_player(rdram,&c,player(0)); CHECK(!reserve(0));
    MEM_B(0,guest(0x8012762C))=0;
    MEM_H(0x16,guest(top(0)))=3; refresh_player(rdram,&c,player(0)); CHECK(!reserve(0));
    MEM_H(0x16,guest(top(0)))=2;
    MEM_B(0x72,guest(node(0)))=1; refresh_player(rdram,&c,player(0)); CHECK(!reserve(0));
    MEM_B(0x72,guest(node(0)))=0;
    MEM_W(8*16+4,guest(0x801247A0))=0x80000000; refresh_player(rdram,&c,player(0)); CHECK(!reserve(0));
    MEM_W(8*16+4,guest(0x801247A0))=0x80089160;
    refresh_player(rdram,&c,player(0)); CHECK(reserve(0));
    {
        const uint32_t auxiliary=0x80030000;
        MEM_W(0x68,guest(auxiliary))=0;
        auto other=context(auxiliary);
        UpdateScope scope(rdram,&other);
        CHECK(reserve(0)); // unrelated camera updates cannot disable the live player's reservation
        other=context(node(0)); other.r5=guest(bits(12.5f)); tooie_camera_filter_yaw(rdram,&other);
        CHECK(approximately(other.f0.fl,12.5f));
    }
    // No active scope: native angle is unchanged, regardless of stale input.
    c=context(node(0)); c.r5=guest(bits(12.5f)); tooie_camera_filter_yaw(rdram,&c); CHECK(approximately(c.f0.fl,12.5f));
    // Player slot, camera index and controller port are three separate identities.
    MEM_W(0x184,guest(player(1)))=5;
    MEM_W(5*4,guest(0x80127728))=top(1);
    MEM_W(1*4,guest(0x80127728))=0;
    MEM_B(6,guest(buffer(1)))=2;
    c=context(player(1)); refresh_player(rdram,&c,player(1)); CHECK(reserve(2));
    const auto reassigned=tick(rdram,1,{1,0}); CHECK(reassigned[0]>p1[0]);
    set_enabled(false); const auto off=tick(rdram,0,{1,1}); CHECK(approximately(off[0],recenter[0]));
    reset_motion(); CHECK(!reserve(0) && !reserve(1)); CHECK(!sample(0).reserved);
}

static void policy_tests() {
    const auto a=radial_deadzone({0.3f,0.6f},0.15f); CHECK(approximately(a.y,2*a.x));
    CHECK(radial_deadzone({0.05f,0},0.15f).x==0);
    CHECK(approximately(std::hypot(radial_deadzone({1,1},0.15f).x,radial_deadzone({1,1},0.15f).y),1));
    CHECK(radial_deadzone({std::numeric_limits<float>::quiet_NaN(),0},0).x==0);
    Settings config{true,3,0}; Motion full,half;
    CHECK(approximately(full.step({1,0},1.0f/30,config).x,half.step({0.5f,0},1.0f/30,config).x*2));
    CHECK(full.step({},1.0f/30,config).x==0);
    float at30=0,at60=0; Motion m30,m60;
    for(int i=0;i<30;++i) at30+=m30.step({1,0},1.0f/30,config).x;
    for(int i=0;i<60;++i) at60+=m60.step({1,0},1.0f/60,config).x;
    CHECK(approximately(at30,at60,0.001f));
    Motion invalid; CHECK(invalid.step({1,1},1,config).x==0);
    config.invert=3; Motion inverted; const auto d=inverted.step({1,1},1.0f/30,config); CHECK(d.x<0 && d.y<0);
    set_enabled(true); publish(0,true); publish(1,true);
    on_input(0,{1,1},0,0x2000,false,true); on_input(1,{1,1},0,0,false,true);
    CHECK(!sample(0).reserved && sample(1).reserved);
    on_input(1,{1,1},0,0,true,true); CHECK(!sample(1).reserved);
    on_input(1,{1,1},0,0,false,false); CHECK(!sample(1).reserved);
}
static void config_tests() {
    using namespace recomp::config; using namespace banjo::configkeys::general;
    config_dir=std::filesystem::temp_directory_path()/std::filesystem::path("tooie-camera-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(config_dir);
    auto make=[] {
        Config c("General","general",true);
        c.add_enum_option(analog_cam_mode,"Analog Camera","",{{0u,"On"},{1u,"Off"}},1u);
        c.add_number_option(analog_camera_sensitivity,"Sensitivity","",1,10,1,0,false,3);
        c.add_enum_option(third_person_camera_invert_mode,"Invert","",{{0u,"None"},{1u,"X"},{2u,"Y"},{3u,"Both"}},1u);
        install_config(c); CHECK(c.load_config()); return c;
    };
    set_enabled(false); auto c=make(); CHECK(!settings().enabled);
    c.set_option_value(analog_cam_mode,0u); CHECK(!settings().enabled);
    c.revert_temp_config(); CHECK(!settings().enabled);
    c.set_option_value(analog_cam_mode,0u); CHECK(c.save_config()); CHECK(settings().enabled);
    c.set_option_value(analog_camera_sensitivity,8.0); c.set_option_value(third_person_camera_invert_mode,2u);
    CHECK(settings().sensitivity==3 && settings().invert==1);
    CHECK(c.save_config()); CHECK(settings().sensitivity==8 && settings().invert==2);
    set_enabled(false); set_sensitivity(1); set_invert(0);
    auto restored=make(); CHECK(settings().enabled && settings().sensitivity==8 && settings().invert==2);
    std::filesystem::remove(config_dir/"general.json"); std::filesystem::remove(config_dir);
}
int main() {
    policy_tests(); native_tests(); config_tests();
    std::printf("tooie_analog_camera_test PASS (%d checks)\n",checks);
}
