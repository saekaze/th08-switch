#ifdef TH_NATIVE_PLATFORM
// Switch port of th08_web/cpp/sdl/AudioHost.cpp: the same miniaudio mixer,
// with SDL2 queued audio instead of an SDL3 stream, and BGM streamed straight
// out of thbgm.dat (original PCM, loop points from thbgm.fmt) instead of
// pre-converted OGG files. MIDI mode has no synthesizer on Horizon; WAV music
// (the default when thbgm.dat is present) is used instead.
// MusicControl, MIDI sequencing, fades and SoundEffects remain TH08 owners.
#define MINIAUDIO_IMPLEMENTATION
#include "MiniaudioConfig.hpp"
#include "BgmStream.hpp"
#include "Platform.hpp"
#include "PlatformHost.hpp"
#include "../platform/PlatformDevices.hpp"
#include <algorithm>
#include <SDL.h>
#include <cmath>
#include <cstdio>
#include <memory>
namespace th08 {
namespace {
struct Buffer {
    WaveFormat format{};std::vector<u8> pcm;ma_audio_buffer data{};host::PcmStream stream;ma_sound sound{};
    bool data_ready=false,stream_ready=false,sound_ready=false;bool loop=false;u32 cursor=0;std::string name;i32 volume=0,pan=0;
    ~Buffer(){if(sound_ready)ma_sound_uninit(&sound);if(data_ready)ma_audio_buffer_uninit(&data);if(stream_ready)ma_data_source_uninit(&stream.base);}
};
struct Audio {
    ma_engine engine{};SDL_AudioDeviceID device=0;bool ready=false,paused=false,music=true,refill=true;u32 error=0,pumps=0,mixed=0,min_queue=~0u;float rms=0;
    std::map<u32,std::unique_ptr<Buffer>> buffers;FILE* bgm=nullptr;std::vector<char> bgm_buffer;
    bool initialize(){if(ready)return true;auto config=ma_engine_config_init();config.noDevice=MA_TRUE;config.channels=2;config.sampleRate=44100;config.defaultVolumeSmoothTimeInPCMFrames=0;
        if(ma_engine_init(&config,&engine)!=MA_SUCCESS){error=1;return false;}
        if(SDL_InitSubSystem(SDL_INIT_AUDIO)==0){SDL_AudioSpec want{},have{};want.freq=44100;want.format=AUDIO_F32SYS;want.channels=2;want.samples=2048;device=SDL_OpenAudioDevice(nullptr,0,&want,&have,0);}
        if(!device){ma_engine_uninit(&engine);error=2;host::log("audio: %s",SDL_GetError());return false;}
        bgm=std::fopen(host::paths().data_file("thbgm.dat").c_str(),"rb");if(bgm){bgm_buffer.resize(64*1024);std::setvbuf(bgm,bgm_buffer.data(),_IOFBF,bgm_buffer.size());}
        ready=true;SDL_PauseAudioDevice(device,0);return true;}
    u32 queued()const{return device?SDL_GetQueuedAudioSize(device)/8:0;}
    bool attach(Buffer& b,ma_data_source* source){if(ma_sound_init_from_data_source(&engine,source,MA_SOUND_FLAG_NO_SPATIALIZATION,nullptr,&b.sound)!=MA_SUCCESS)return false;b.sound_ready=true;return true;}
    void controls(Buffer& b){ma_sound_set_volume(&b.sound,std::pow(10.f,float(b.volume)/2000.f));ma_sound_set_pan_mode(&b.sound,ma_pan_mode_balance);
        ma_sound_set_pan(&b.sound,b.pan>=0?1.f-std::pow(10.f,-float(b.pan)/2000.f):std::pow(10.f,float(b.pan)/2000.f)-1.f);}
    void pump(){if(!ready||paused)return;auto queued=queued_frames();min_queue=std::min(min_queue,u32(queued));
        if(!refill&&queued<4096)refill=true;if(!refill)return;if(queued>=6144){refill=false;return;}
        for(int block=0;block<6&&queued<6144;++block){
        float pcm[2048]{};ma_uint64 frames=0;if(ma_engine_read_pcm_frames(&engine,pcm,1024,&frames)!=MA_SUCCESS){error=3;return;}
        double energy=0;for(auto& v:pcm){v=std::clamp(v,-1.f,1.f);energy+=double(v)*v;}rms=std::sqrt(energy/2048);
        if(SDL_QueueAudio(device,pcm,sizeof(pcm))!=0){error=4;return;}++pumps;mixed+=1024;queued+=1024;
        }if(queued>=6144)refill=false;
    }
    i32 queued_frames()const{return i32(queued());}
}audio;
}
struct SDLSound final:SoundDevice {
 i32 create_pcm(u32 handle,const WaveFormat& format,const u8* data,u32 size)override{if(!audio.initialize())return -1;
if(!PcmWave::valid(format)||size%format.align)return -1;
        auto buf=std::make_unique<Buffer>();buf->format=format;buf->pcm.assign(data,data+size);
        auto config=ma_audio_buffer_config_init(format.bits==8?ma_format_u8:ma_format_s16,format.channels,size/format.align,buf->pcm.data(),nullptr);config.sampleRate=format.rate;
        if(ma_audio_buffer_init(&config,&buf->data)!=MA_SUCCESS)return -1;buf->data_ready=true;if(!audio.attach(*buf,&buf->data))return -1;audio.buffers[handle]=std::move(buf);return 0;
 }
 i32 music_format(u32 handle,const BgmFormat& f,bool loop)override{if(!audio.initialize())return -1;
if(!PcmWave::valid(f.format)||f.intro<0||f.total<=f.intro)return -1;
        const std::string name(f.name,std::find(f.name,f.name+16,'\0'));auto found=audio.buffers.find(handle);
        if(found!=audio.buffers.end()&&found->second->name==name){found->second->loop=loop;ma_sound_set_looping(&found->second->sound,loop);return 0;}
        if(name.find('/')!=std::string::npos||name.find('\\')!=std::string::npos||name.find("..")!=std::string::npos)return -1;
        if(!audio.bgm||f.format.bits!=16||f.format.align!=f.format.channels*2||f.start<0)return -1;
        auto buf=std::make_unique<Buffer>();buf->format=f.format;buf->name=name;buf->loop=loop;
        auto& s=buf->stream;s.file=audio.bgm;s.offset=u32(f.start);s.frames=u32(f.total)/f.format.align;s.channels=f.format.channels;s.rate=f.format.rate;
        auto config=ma_data_source_config_init();config.vtable=&host::pcm_vtable;
        if(ma_data_source_init(&config,&s.base)!=MA_SUCCESS)return -1;buf->stream_ready=true;if(host::pcm_seek(&s,0)!=MA_SUCCESS)return -1;
        ma_data_source_set_loop_point_in_pcm_frames(&s,u32(f.intro)/f.format.align,s.frames);
        if(!audio.attach(*buf,&s))return -1;ma_sound_set_looping(&buf->sound,loop);audio.buffers[handle]=std::move(buf);return 0;
 }
 Buffer* get(u32 id){auto it=audio.buffers.find(id);return it==audio.buffers.end()?nullptr:it->second.get();}
 i32 stop(u32 id)override{if(!audio.initialize())return -1;if(auto* b=get(id))ma_sound_stop(&b->sound);return 0;}
 i32 position(u32 id,u32 cursor)override{if(!audio.initialize())return -1;if(auto* b=get(id)){b->cursor=cursor;return ma_sound_seek_to_pcm_frame(&b->sound,cursor/b->format.align)==MA_SUCCESS?0:-1;}return 0;}
 i32 pan(u32 id,i32 value)override{if(!audio.initialize())return -1;if(auto* b=get(id)){b->pan=value;audio.controls(*b);}return 0;}
 i32 volume(u32 id,i32 value)override{if(!audio.initialize())return -1;if(auto* b=get(id)){b->volume=value;audio.controls(*b);}return 0;}
 i32 play(u32 id,u32,u32 flags)override{if(!audio.initialize())return -1;if(auto* b=get(id)){if(id==1000&&!audio.music)return 0;if(id!=1000){b->loop=flags&1;ma_sound_set_looping(&b->sound,b->loop);}return ma_sound_start(&b->sound)==MA_SUCCESS?0:-1;}return 0;}
 void release(u32 id)override{if(audio.initialize())audio.buffers.erase(id);}
 // No MIDI synthesizer on Horizon: MIDI music mode is silent (WAV mode is the default).
 void midi_open()override{}
 void midi_close(u32)override{}
 void midi_message(const u8*,u32,u32)override{}
};
SoundDevice& sound_device(){static SDLSound sound;return sound;}
void sdl_audio_pump(){audio.pump();}
void sdl_audio_pause(bool pause){if(audio.paused==pause)return;audio.paused=pause;if(audio.device)SDL_PauseAudioDevice(audio.device,pause?1:0);}
void sdl_audio_shutdown(){audio.buffers.clear();if(audio.device)SDL_CloseAudioDevice(audio.device);audio.device=0;if(audio.ready)ma_engine_uninit(&audio.engine);audio.ready=false;if(audio.bgm)std::fclose(audio.bgm);audio.bgm=nullptr;}
}
#endif
