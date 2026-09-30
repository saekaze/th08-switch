// Touhou 8 for Nintendo Switch: native entry point.
//
// Replaces th08_web/cpp/sdl/GameHost.cpp's browser exports with a native
// loop. The same staged start-up (archive, fonts, warm animations - now with
// a loading bar), the same BrowserRuntime step/audio_tick per 60 Hz tick and
// the same touch controller. Host changes: vsync-paced loop, Switch
// controller fed to TH08's own gamepad input (so the in-game key config can
// remap it), SD-card files, a snapshot key.
#ifdef TH_NATIVE_PLATFORM
#include "PlatformHost.hpp"
#include "GraphicsHost.hpp"
#include "Platform.hpp"
#include "FrameCadence.hpp"
#include "Renderer.hpp"
#include "../portable/input/TouchController.hpp"
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#ifdef __SWITCH__
#include <switch.h>
#endif
namespace th08 {
void sdl_validate_capture();
namespace {
std::unique_ptr<BrowserRuntime> runtime;touhou::input::TouchController touch;
struct Key{const char* code;const char* sdl;u32 scan,vk;bool hosted=false;SDL_Scancode native=SDL_SCANCODE_UNKNOWN;};
#include "../portable/input/KeyboardMap.inc"
u32 prepared=0,frames=0,warm_mask=0;double elapsed=0;
constexpr const char* warmAnimations[]={"etama.anm","enemy.anm","front.anm","times.anm","stg1bg.anm","stg1enm.anm","eff01.anm","stg1txt.anm","stg2bg.anm","stg2enm.anm","eff02.anm","stg2txt.anm","player00.anm","player01.anm","player02.anm","player03.anm","staff01.anm"};
constexpr u32 warmCount=sizeof(warmAnimations)/sizeof(*warmAnimations);
touhou::input::TouchState touch_state(){touhou::input::TouchState s;if(!runtime)return s;const auto& a=runtime->app;const auto& g=a.game;const auto& p=g.player_state;
    if(a.in_game()&&(g.globals.game_flags&8)){s.context=3;return s;}
    if(!a.in_game()||a.loading_game()||!g.ready()||g.paused||g.menus.context.pause_state||p.context.game_over||(g.globals.game_flags&0x60))return s;
    s.context=g.dialogue.present()?2:1;s.ready=p.life.state!=1&&p.life.state!=2;s.instance=g.globals.stage+1;
    s.x=p.motion.movement.position.x;s.y=p.motion.movement.position.y;s.fast=g.shots[0].settings().normal_speed*g.player.timing.rate;s.slow=g.shots[1].settings().focus_speed*g.player.timing.rate;
    s.min_x=p.input.minimum.x;s.min_y=p.input.minimum.y;s.max_x=s.min_x+p.input.extent.x;s.max_y=s.min_y+p.input.extent.y;return s;
}
void pointer(int type,int id,float x,float y){touch.pointer(type,id,x,y,SDL_GetTicks(),touch_state(),runtime&&runtime->keyboard_state()[16]);}

// Switch controller as TH08's DirectInput pad. Button numbers match TH08's
// default pad config (shot 0, bomb 1, focus 2, skip 3, pause 4), which gives
// the classic layout on first launch; the in-game key config can remap:
//   0 B  1 A  2 L/ZL  3 R/ZR  4 +  5 X  6 Y  7 left-stick click  8 right-stick click
bool snapshot_held=false,snapshot_requested=false;
#ifdef __SWITCH__
PadState pad;
bool read_pad(i32& x,i32& y,u8* buttons){
    padUpdate(&pad);if(!padIsConnected(&pad))return false;
    const u64 held=padGetButtons(&pad);const auto stick=padGetStickPos(&pad,0);
    const u64 map[9]{HidNpadButton_B,HidNpadButton_A,HidNpadButton_L|HidNpadButton_ZL,HidNpadButton_R|HidNpadButton_ZR,HidNpadButton_Plus,HidNpadButton_X,HidNpadButton_Y,HidNpadButton_StickL,HidNpadButton_StickR};
    for(u32 b=0;b<9;++b)buttons[b]=held&map[b]?128:0;
    // Stick in DirectInput units (±1000, threshold 600); the d-pad pushes it
    // to the end like a digital pad.
    x=i32(stick.x)*1000/32767;y=-i32(stick.y)*1000/32767;
    if(held&HidNpadButton_Left)x=-1000;if(held&HidNpadButton_Right)x=1000;if(held&HidNpadButton_Up)y=-1000;if(held&HidNpadButton_Down)y=1000;
    const bool minus=held&HidNpadButton_Minus;if(minus&&!snapshot_held)snapshot_requested=true;snapshot_held=minus;
    return true;
}
#else
bool read_pad(i32&,i32&,u8*){return false;}
#endif
void poll(){if(!runtime)return;
    auto* keys=runtime->keyboard_state();std::memset(keys,0,256);const u8* physical=SDL_GetKeyboardState(nullptr);
    for(const auto& k:keyboard_map)if(k.hosted||(k.native!=SDL_SCANCODE_UNKNOWN&&physical[k.native])){keys[k.vk]=128;if(k.vk>=160&&k.vk<=165)keys[16+(k.vk-160)/2]=128;}
    i32 x=0,y=0;u8 buttons[128]{};
    if(read_pad(x,y,buttons))runtime->controller_state(x,y,buttons,128,true);else runtime->controller_state(0,0,nullptr,0,false);
    const auto input=touch.sample(touch_state(),SDL_GetTicks(),keys[16],keys[37]||keys[38]||keys[39]||keys[40]);for(int i=0;i<256;i++)if(input.keys[i])keys[i]=128;
    runtime->motion.target(input.motion,input.x,input.y);
}
int tick(){poll();if(!runtime||!runtime->step(true))return runtime&&(runtime->status(2)||runtime->status(4))?2:1;
    ++frames;return runtime->audio_tick(u32(elapsed*1000))?0:2;}

// A simple bar while start-up prepares assets, fonts and warm animations.
// The game's backbuffer does not exist yet, so this clears rectangles of the
// window's own framebuffer (640x480 layout, pillarboxed like the game).
void loading_bar(float fraction){
    static double last=-1;const double now=host::seconds();if(fraction>0&&fraction<1&&now-last<1.0/30)return;last=now;
    auto* gpu=sdl_renderer();if(!gpu||!gpu->sdl_window())return;
    int w=0,h=0;SDL_GL_GetDrawableSize(gpu->sdl_window(),&w,&h);if(w<=0||h<=0)return;
    const int pw=h*4/3<=w?h*4/3:w,px=(w-pw)/2,py=(h-(pw*3/4))/2;const float s=float(pw)/640.f;
    auto rect=[&](i32 x0,i32 y0,i32 x1,i32 y1,float grey){ // 640x480 coordinates, y down
        glScissor(px+int(x0*s),py+int((480-y1)*s),std::max(1,int((x1-x0)*s)),std::max(1,int((y1-y0)*s)));glClearColor(grey,grey,grey,1);glClear(GL_COLOR_BUFFER_BIT);};
    // The renderer caches bindings and masks: save and restore what is touched.
    GLint draw_fb=0,read_fb=0,view[4]{},box[4]{};GLboolean mask[4]{},scissor=glIsEnabled(GL_SCISSOR_TEST);GLfloat clear[4]{};
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&draw_fb);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read_fb);glGetIntegerv(GL_VIEWPORT,view);
    glGetIntegerv(GL_SCISSOR_BOX,box);glGetBooleanv(GL_COLOR_WRITEMASK,mask);glGetFloatv(GL_COLOR_CLEAR_VALUE,clear);
    glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,w,h);glDisable(GL_SCISSOR_TEST);glColorMask(1,1,1,1);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);glEnable(GL_SCISSOR_TEST);
    const i32 x0=160,x1=480,y0=400,y1=412,fill=x0+2+i32(float(x1-x0-4)*std::clamp(fraction,0.f,1.f));
    rect(x0,y0,x1,y1,.376f);rect(x0+2,y0+2,x1-2,y1-2,0);if(fill>x0+2)rect(x0+2,y0+2,fill,y1-2,.878f);
    glDisable(GL_SCISSOR_TEST);SDL_GL_SwapWindow(gpu->sdl_window());SDL_PumpEvents();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,GLuint(draw_fb));glBindFramebuffer(GL_READ_FRAMEBUFFER,GLuint(read_fb));glViewport(view[0],view[1],view[2],view[3]);
    glScissor(box[0],box[1],box[2],box[3]);if(scissor)glEnable(GL_SCISSOR_TEST);glColorMask(mask[0],mask[1],mask[2],mask[3]);glClearColor(clear[0],clear[1],clear[2],clear[3]);
}
// sdl_prepare_total / sdl_prepare_next from GameHost.cpp.
u32 prepare_total(){return runtime?u32(runtime->resources().size())+runtime->native_font_steps()+warmCount:0;}
bool prepare_next(){const auto assets=u32(runtime->resources().size());const auto fonts=runtime->native_font_steps();bool ok=true;
    if(prepared<assets)ok=sdl_prepare_asset(*runtime,prepared);
    else if(prepared<assets+fonts)ok=runtime->native_font_step(prepared-assets);
    else {const auto& bytes=runtime->file(warmAnimations[prepared-assets-fonts]);
        // Optional bounded cache: exhaustion must never prevent launching.
        if(!bytes.empty()&&runtime->app.library.preload(bytes.data(),bytes.size()))warm_mask|=1u<<(prepared-assets-fonts);}
    ++prepared;return ok;}

// '−' saves the current 640x480 frame as a BMP into snapshot/.
void snapshot(){
    auto* gpu=sdl_renderer();if(!gpu||!runtime)return;gpu->flush();gpu->read(runtime->backbuffer());const auto* t=runtime->texture(runtime->backbuffer());
    if(!t||!t->data||t->pitch<t->width*4)return;
    host::make_directory(host::paths().save_file("snapshot"));char file[64];std::string path;
    for(u32 n=0;n<1000;++n){std::snprintf(file,sizeof(file),"snapshot/th08_%03u.bmp",n);path=host::paths().save_file(file);if(!host::exists(path))break;}
    const u32 w=t->width,h=t->height,row=w*3,pad=(4-row%4)%4,size=54+(row+pad)*h;std::vector<u8> bmp(size,0);
    auto put=[&](u32 at,u32 v,u32 n){for(u32 i=0;i<n;++i)bmp[at+i]=u8(v>>(8*i));};
    bmp[0]='B';bmp[1]='M';put(2,size,4);put(10,54,4);put(14,40,4);put(18,w,4);put(22,h,4);put(26,1,2);put(28,24,2);put(34,size-54,4);
    for(u32 y=0;y<h;++y){const u8* src=t->data+(h-1-y)*t->pitch;u8* dst=bmp.data()+54+y*(row+pad);for(u32 x=0;x<w;++x){dst[x*3]=src[x*4];dst[x*3+1]=src[x*4+1];dst[x*3+2]=src[x*4+2];}}
    host::log("snapshot %s: %s",path.c_str(),host::write_file(path,bmp)?"ok":"failed");
}
void fatal(const std::string& message){
    host::log("fatal: %s",message.c_str());
    runtime.reset();sdl_audio_shutdown();sdl_fonts_shutdown();sdl_detach();SDL_Quit();
#ifdef __SWITCH__
    consoleInit(nullptr);
    std::printf("\n  Touhou 8: Imperishable Night - Switch port\n\n  Error: %s\n\n",message.c_str());
    std::printf("  Data folder: %s\n  Needed: th08.dat, msgothic.ttc (or the system font)\n  Optional: thbgm.dat\n  Details: th08-switch.log\n\n  Press + to exit.\n",host::paths().data.c_str());
    consoleUpdate(nullptr);
    while(appletMainLoop()){padUpdate(&pad);if(padGetButtonsDown(&pad)&HidNpadButton_Plus)break;consoleUpdate(nullptr);}
    consoleExit(nullptr);
#else
    std::fprintf(stderr,"%s\n",message.c_str());
#endif
}
}
u32 sdl_game_time(){return u32(elapsed*1000);}
}

int main(int argc,char** argv){
    using namespace th08;
    host::locate_data(argc,argv);
#ifdef __SWITCH__
    // Same CPU boost the other native ports use; the GPU stays with the applet.
    if(R_SUCCEEDED(clkrstInitialize())){ClkrstSession cpu;if(R_SUCCEEDED(clkrstOpenSession(&cpu,PcvModuleId_CpuBus,3))){clkrstSetClockRate(&cpu,1785000000);clkrstCloseSession(&cpu);}clkrstExit();}
    padConfigureInput(1,HidNpadStyleSet_NpadStandard);padInitializeDefault(&pad);
    appletSetFocusHandlingMode(AppletFocusHandlingMode_SuspendHomeSleep);
#endif
    host::log("th08-switch: data %s",host::paths().data.c_str());
    if(!host::exists(host::paths().data_file("th08.dat"))){fatal("th08.dat not found");return 1;}
    // sdl_game_open
    for(auto& k:keyboard_map)k.native=SDL_GetScancodeFromName(k.sdl);
    runtime=std::make_unique<BrowserRuntime>();
    if(!sdl_attach(runtime.get())){fatal("Graphics initialisation failed");return 1;}
    if(!sdl_load_assets(*runtime)){fatal("Game data could not be loaded (see th08-switch.log)");return 1;}
    // sdl_prepare_next x total, with a loading bar.
    const u32 total=prepare_total();
    while(prepared<total){if(!prepare_next()){fatal("Preparing game resources failed");return 1;}loading_bar(float(prepared)/float(total));}
    if(!runtime->initialize()){fatal("Game initialisation failed");return 1;}
    sdl_validate_capture();
    // Vsync paces the loop at the panel's 60 Hz: one game tick per swap.
    // Measured time only adds catch-up ticks after a real stall (bounded to
    // 4 like upstream FrameCadence), so vsync jitter never drops a frame.
    constexpr double interval=touhou::sdl::FrameCadence::interval;
    auto* gpu=sdl_renderer();double debt=0,previous=host::seconds();int result=0;bool running=true;
    while(running&&!result
#ifdef __SWITCH__
          &&appletMainLoop()
#endif
    ){
        SDL_Event e;while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT)running=false;
            else if(e.type==SDL_FINGERDOWN||e.type==SDL_FINGERUP||e.type==SDL_FINGERMOTION){
                // Touch is normalised to the whole screen; the game wants the pillarboxed picture.
                const auto& p=gpu->picture;if(p.width<=0||p.height<=0)continue;
                const float x=(e.tfinger.x*p.drawable_width-p.x)/p.width,y=(e.tfinger.y*p.drawable_height-p.y)/p.height;
                const int type=e.type==SDL_FINGERDOWN?0:e.type==SDL_FINGERUP?2:1;if(type==0&&(x<0||x>1||y<0||y>1))continue;
                pointer(type,int(e.tfinger.fingerId),std::clamp(x,0.f,1.f),std::clamp(y,0.f,1.f));
            }
            else if(e.type==SDL_WINDOWEVENT&&e.window.event==SDL_WINDOWEVENT_FOCUS_LOST){for(auto& k:keyboard_map)k.hosted=false;touch.reset();}
        }
        const double now=host::seconds();debt+=std::clamp(now-previous,0.,.1);previous=now;
        u32 ticks=u32(std::floor((debt+interval*.5)/interval));ticks=std::min(4u,std::max(ticks,1u));
        debt=std::clamp(debt-ticks*interval,-interval*.5,.1);
        gpu->defer=true;
        for(u32 i=0;i<ticks&&!result;++i){elapsed+=interval;result=tick();}
        if(!result&&snapshot_requested){snapshot_requested=false;snapshot();}
        gpu->commit();gpu->defer=false;
        sdl_audio_pump();
    }
    if(result==2){fatal("The game stopped with an error (see th08-switch.log)");return 1;}
    // sdl_game_close
    touch.reset();runtime.reset();sdl_audio_shutdown();sdl_fonts_shutdown();sdl_detach();
    SDL_Quit();
    return 0;
}
#endif
