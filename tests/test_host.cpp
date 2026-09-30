// Host-side tests for the TH08 Switch port layer (Linux; no game data needed).
//   arithmetic   - SoftFloat x87 emulation (the port patched its amalgamation
//                  and the rounding-mode globals' linkage)
//   thbgm stream - PCM offsets, seeking and intro/loop points
//   fonts        - SDL2_ttf glyphs and the CP932/blend tables (needs a CJK font)
//   renderer     - SDL2/GLES3 port of the shared renderer (needs a display)
#include "../sdl/PlatformHost.hpp"
#include "../sdl/Platform.hpp"
#include "../sdl/BgmStream.hpp"
#include "../game/Arithmetic.hpp"
#include "../game/JapaneseFonts.hpp"
#include "Renderer.hpp"
#include <SDL.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace th08;
namespace th08 {bool sdl_glyph(i32,i32,u16,JapaneseFont::Glyph&,std::vector<u8>&);u32 sdl_game_time(){return 0;}}
static int failures=0;
#define CHECK(cond) do{if(!(cond)){std::printf("  FAIL %s:%d: %s\n",__FILE__,__LINE__,#cond);++failures;}}while(0)

static void test_arithmetic(){
    std::puts("arithmetic");
    arithmetic_mode(Precision::Extended,Rounding::NearestEven);
    CHECK(arithmetic_precision()==Precision::Extended&&arithmetic_rounding()==Rounding::NearestEven);
    const auto third=Extended::from_int(1)/Extended::from_int(3);
    CHECK(third.to_double()==1.0/3.0);CHECK((third*Extended::from_int(3)).to_double()==1.0);
    CHECK(Extended::from_double(2.0).square_root().to_double()==std::sqrt(2.0));
    CHECK(Extended::from_double(-2.75).truncate_int()==-2);CHECK(Extended::from_double(2.75).truncate_int()==2);
    // 2^62 + 1 fits the 64-bit x87 mantissa but not a double's 53 bits.
    const auto big=Extended::from_int64(i64(1)<<62);
    CHECK(((big+Extended::from_int(1))-big).to_double()==1.0);
    // Single precision (the game's usual FPU mode) rounds every result to 24 bits.
    arithmetic_mode(Precision::Single,Rounding::NearestEven);
    CHECK((Extended::from_int(1)/Extended::from_int(3)).to_double()==double(1.f/3.f));
    CHECK((Extended::from_int(16777216)+Extended::from_int(1)).to_double()==16777216.0);
    // Tiny values go through the subnormal paths (softfloat_shiftRightJam*).
    arithmetic_mode(Precision::Extended,Rounding::NearestEven);
    const double tiny=std::ldexp(1.0,-1060);
    CHECK((Extended::from_double(tiny)*Extended::from_double(0.5)).to_double()==std::ldexp(1.0,-1061));
    CHECK((Extended::from_double(1e-300)*Extended::from_double(1e-300)).to_double()==0.0);
}

static void test_bgm_stream(const std::string& dir){
    std::puts("thbgm stream");
    const std::string path=dir+"/thbgm.dat";const u32 offset=20,frames=10000,intro=2500;
    {FILE* f=std::fopen(path.c_str(),"wb");std::fwrite("ZWAV",1,4,f);for(u32 i=4;i<offset;++i)std::fputc(0,f);
     for(u32 n=0;n<frames;++n){const i16 l=i16(n),r=i16(-i32(n));std::fwrite(&l,2,1,f);std::fwrite(&r,2,1,f);}std::fclose(f);}
    host::PcmStream s;s.file=std::fopen(path.c_str(),"rb");s.offset=offset;s.frames=frames;
    auto config=ma_data_source_config_init();config.vtable=&host::pcm_vtable;
    CHECK(ma_data_source_init(&config,&s.base)==MA_SUCCESS);CHECK(host::pcm_seek(&s,0)==MA_SUCCESS);
    CHECK(ma_data_source_set_loop_point_in_pcm_frames(&s,intro,frames)==MA_SUCCESS);CHECK(ma_data_source_set_looping(&s,MA_TRUE)==MA_SUCCESS);
    // Read in engine-sized chunks across several loops.
    std::vector<float> pcm(2*1024);bool ordered=true;u32 at=0;
    while(at<40000&&ordered){ma_uint64 read=0;ma_data_source_read_pcm_frames(&s,pcm.data(),1024,&read);if(read!=1024){ordered=false;break;}
        for(u32 i=0;i<1024;++i,++at){const u32 expect=at<frames?at:intro+(at-frames)%(frames-intro);if(pcm[i*2]!=float(i16(expect))/32768.f){ordered=false;std::printf("  frame %u wrong\n",at);break;}}}
    CHECK(ordered);ma_data_source_uninit(&s.base);std::fclose(s.file);
}

static void test_fonts(){
    std::puts("fonts");
    auto runtime=std::make_unique<BrowserRuntime>();auto& r=*runtime;const bool ok=r.native_fonts(); // several MB: heap, as in mainCHECK(ok);if(!ok)return;
    for(const auto& profile:{std::pair<i32,i32>{16,400},{28,600}}){
        JapaneseFont::Glyph g;std::vector<u8> pixels;
        CHECK(sdl_glyph(profile.first,profile.second,0x6771,g,pixels)); // 東
        u32 lit=0;for(u8 c:pixels)lit+=c>7;
        std::printf("  %dpx: %ux%u, advance %d, %u lit\n",profile.first,g.width,g.height,g.advance,lit);
        CHECK(g.width>0&&g.height>0&&g.height<=u32(profile.first)+2);CHECK(lit>u32(profile.first)*2);
    }
    sdl_fonts_shutdown();
}

namespace {
struct Surfaces {std::map<u32,std::vector<u8>> pixels;std::map<u32,touhou::sdl::Surface> info;};
touhou::sdl::Surface resolve(void* owner,u32 id){auto& s=*static_cast<Surfaces*>(owner);auto it=s.info.find(id);if(it==s.info.end())return {};auto r=it->second;auto& p=s.pixels[id];r.data=p.data();r.size=u32(p.size());return r;}
void add(Surfaces& s,u32 id,u32 w,u32 h,touhou::graphics::PixelFormat f,u32 bpp){s.pixels[id].assign(size_t(w)*h*bpp,0);s.info[id]={id,w,h,f,w*bpp,nullptr,0,0};}
}
static bool test_renderer(){
    std::puts("renderer (SDL2 + GLES3)");
    if(!std::getenv("DISPLAY")&&!std::getenv("WAYLAND_DISPLAY")){std::puts("  skipped: no display");return false;}
    using namespace touhou::graphics;
    Surfaces surfaces;add(surfaces,1,640,480,touhou::graphics::PixelFormat::Bgra8,4);add(surfaces,2,640,480,touhou::graphics::PixelFormat::Depth16,2);add(surfaces,3,2,2,touhou::graphics::PixelFormat::Bgra8,4);
    for(u32 n=0;n<4;++n){auto* p=surfaces.pixels[3].data()+n*4;p[0]=0;p[1]=255;p[2]=0;p[3]=255;} // green texture
    touhou::sdl::Renderer r(10,resolve,&surfaces);r.title="th08-switch test";
    const bool ok=r.initialize();CHECK(ok);if(!ok){std::printf("  %s\n",r.error());return true;}
    r.state.target=1;r.state.depth=2;r.viewport({0,0,640,480,0,1});
    r.clear(3,0xff0000ff,1,0); // blue
    struct V {float x,y,z,w;u32 diffuse;float u,v;};
    const V quad[4]{{100,100,0,1,0xffffffff,0,0},{200,100,0,1,0xffffffff,1,0},{100,200,0,1,0xffffffff,0,1},{200,200,0,1,0xffffffff,1,1}};
    auto& p=r.pipeline();p=PipelineState{};p.depthTest=false;p.color.operation=p.alpha.operation=ColorOperation::First;p.color.first=p.alpha.first={ArgumentSource::Texture};
    r.state.texture=3;r.state.layout=attributes(VertexLayout::ScreenColorUv);
    r.draw(Topology::Strip,2,quad,sizeof(V));
    r.read(1);
    auto pixel=[&](u32 x,u32 y){const auto* q=surfaces.pixels[1].data()+(y*640+x)*4;return u32(q[2])<<16|u32(q[1])<<8|q[0];};
    std::printf("  centre %06x corner %06x\n",pixel(150,150),pixel(10,10));
    CHECK(pixel(150,150)==0x00ff00);CHECK(pixel(10,10)==0x0000ff);CHECK(pixel(99,150)==0x0000ff);CHECK(pixel(150,99)==0x0000ff);
    ++surfaces.info[1].version;r.present(1);
    std::printf("  picture %dx%d at %d,%d in %dx%d\n",r.picture.width,r.picture.height,r.picture.x,r.picture.y,r.picture.drawable_width,r.picture.drawable_height);
    CHECK(r.picture.width*3==r.picture.height*4);CHECK(r.picture.x*2+r.picture.width==r.picture.drawable_width);
    CHECK(glGetError()==GL_NO_ERROR);
    return true;
}

int main(){
    const std::string dir=std::string(std::getenv("TMPDIR")?std::getenv("TMPDIR"):"/tmp")+"/th08-switch-test";
    host::make_directory(dir);
    test_arithmetic();test_bgm_stream(dir);
    if(SDL_Init(SDL_INIT_VIDEO)==0){test_fonts();test_renderer();SDL_Quit();}else std::printf("fonts/renderer skipped: %s\n",SDL_GetError());
    std::printf(failures?"FAILED (%d)\n":"all host tests passed\n",failures);
    return failures?1:0;
}
