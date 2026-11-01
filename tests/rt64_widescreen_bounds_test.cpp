#include "hle/rt64_state.h"
#include "hle/rt64_rsp.h"
#include "hle/rt64_rdp.h"
#include "hle/rt64_workload_queue.h"
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>
#include <cstring>

static int checks=0;
#define CHECK(x) do { ++checks; if(!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::exit(1); } } while(0)
static void interrupt() {}
// Diagnostic bridge is unreachable in this CPU-only submission regression.
extern "C" uint32_t tooie_diag_p09_local_pair_witnesses(uint64_t, uint32_t*, uint32_t*,
    uint32_t*, uint32_t*, uint32_t*, uint8_t*, uint32_t) { CHECK(false); return 0; }
int main(int argc, char** argv) {
    const bool disabled=argc==2 && std::strcmp(argv[1],"--disabled")==0;
    CHECK(argc==1 || disabled);
#ifdef _WIN32
    _putenv_s("TOOIE_TEST_WS_DRAW_BOUNDS",disabled ? "0":"");
#else
    if(disabled) setenv("TOOIE_TEST_WS_DRAW_BOUNDS","0",1);
    else unsetenv("TOOIE_TEST_WS_DRAW_BOUNDS");
#endif
    // Real RSP triangle submission, no device, shaders, window, ROM or game.
    std::vector<uint8_t> ram(8*1024*1024);
    uint32_t interrupts=0;
    auto state=std::make_unique<RT64::State>(ram.data(),&interrupts,interrupt);
    auto queue=std::make_unique<RT64::WorkloadQueue>();
    RT64::UserConfiguration config;
    config.aspectRatio=RT64::UserConfiguration::AspectRatio::Expand;
    state->ext={}; state->ext.workloadQueue=queue.get(); state->ext.userConfig=&config;
    auto& w=queue->workloads[0];
    w.addFramebufferPair(0x600000,0,2,304,0x640000);
    auto& fb=w.fbPairs[0]; fb.changeProjection(0,RT64::Projection::Type::Perspective);
    state->rsp->curViewProjIndex=0;
    state->rsp->projMatrixStack[0]=hlslpp::float4x4::identity();
    state->rsp->projMatrixStack[0][3][3]=0;
    state->rsp->projMatrixStack[0][1][1]=1;
    state->rsp->geometryModeStack[0]=0;
    state->rsp->cullBothMask=0x600; state->rsp->cullFrontMask=0x200;
    state->rdp->colorImage.changed=false; state->rdp->depthImage.changed=false;
    state->rdp->scissorRectStack[0]={0,0,304*4,228*4};
    state->rsp->textureState.on=state->drawCall.textureOn;
    state->rsp->textureState.tile=state->drawCall.textureTile;
    state->rsp->textureState.levels=state->drawCall.textureLevels;
    state->drawStatus.changed=0;
    w.drawData.tcFloats={0,0,0,0,0,0};
    w.drawData.worldIndices={0,0,0};
    using Aspect=RT64::UserConfiguration::AspectRatio;
    auto run=[&](Aspect aspect,double target,bool perspective,bool zUpdate,
                 float left,float right,float top,float bottom,int nativeHeight,int expectedHeight) {
        config.aspectRatio=aspect; config.aspectTarget=target;
        const bool enhanced=!disabled && perspective && (aspect==Aspect::Expand ||
            (aspect==Aspect::Manual && target>4.0/3.0));
        state->rsp->projMatrixStack[0][3][3]=perspective ? 0:1;
        fb.reset(); fb.changeProjection(0,perspective ? RT64::Projection::Type::Perspective:RT64::Projection::Type::Orthographic);
        fb.drawColorRect={0,0,304*4,nativeHeight*4};
        fb.drawDepthRect={0,0,304*4,nativeHeight*4};
        state->rsp->otherModeStack[0].L=zUpdate ? 0x20:0;
        state->drawCall.triangleCount=0; w.drawData.faceIndices.clear();
        w.drawData.posScreen={{left,top,0.8f},{(left+right)/2,bottom,0.8f},{right,top,0.8f}};
        state->rsp->drawIndexedTri(0,1,2,true);
        CHECK(w.drawData.faceIndices==std::vector<uint32_t>({0,1,2}));
        CHECK(state->drawCall.triangleCount==1);
        const int expected=enhanced ? expectedHeight:nativeHeight;
        CHECK(fb.drawColorRect.bottom(true)==expected);
        CHECK(fb.drawDepthRect.bottom(true)==(zUpdate ? expected:nativeHeight));
    };
    // A submitted triangle wholly outside original X still occupies the added
    // strip after widening. Before the fix its Y bounds were completely lost.
    for(bool z : {false,true}) {
        run(Aspect::Expand,16.0/9,true,z,320,340,190,210,180,210);
        run(Aspect::Expand,16.0/9,true,z,-35,-10,190,210,180,210);
        run(Aspect::Expand,16.0/9,true,z,-35,-10,80,110,60,110);
        run(Aspect::Expand,16.0/9,true,z,320,340,220,240,180,228);
        run(Aspect::Expand,16.0/9,true,z,320,340,-30,-10,180,180);
        run(Aspect::Expand,16.0/9,true,z,320,340,240,260,180,180);
    }
    run(Aspect::Original,4.0/3,true,true,320,340,190,210,180,210);
    run(Aspect::Manual,4.0/3,true,true,320,340,190,210,180,210);
    run(Aspect::Manual,16.0/10,true,true,320,340,190,210,180,210);
    run(Aspect::Manual,21.0/9,true,true,320,340,190,210,180,210);
    run(Aspect::Expand,16.0/9,false,true,320,340,190,210,180,210);
    // Retaining upper extents matters too when other geometry occupies lower rows.
    config.aspectRatio=Aspect::Expand; state->rsp->projMatrixStack[0][3][3]=0;
    fb.reset(); fb.changeProjection(0,RT64::Projection::Type::Perspective);
    fb.drawColorRect={0,180*4,304*4,210*4}; fb.drawDepthRect=fb.drawColorRect;
    state->rsp->otherModeStack[0].L=0x20;
    w.drawData.posScreen={{320,80,0.8f},{330,110,0.8f},{340,90,0.8f}};
    state->rsp->drawIndexedTri(0,1,2,true);
    CHECK(fb.drawColorRect.top(false)==(disabled ? 180:80));
    CHECK(fb.drawDepthRect.top(false)==(disabled ? 180:80));
    CHECK(fb.drawColorRect.bottom(true)==210);
    std::printf("PASS real RSP widescreen bounds: %d checks (%s), no renderer/game\n",checks,disabled ? "kill switch":"enabled");
    return 0;
}
