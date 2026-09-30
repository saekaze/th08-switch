// Switch port of th08_web/cpp/sdl/FileHost.cpp: stdio instead of SDL IO /
// IDBFS. th08.dat is read in place; saves (score.dat, th08.cfg, replays) live
// next to the data on the SD card, like the PC release. thbgm.dat is no
// longer copied into memory (it is ~150 MB): only its header is registered
// for the game's WAV-mode check, and AudioHost streams music from the file.
#ifdef TH_NATIVE_PLATFORM
#include "PlatformHost.hpp"
#include "Platform.hpp"
#include "../platform/PlatformDevices.hpp"
#include <dirent.h>
#include <ctime>
#include <cctype>
#include <cstdio>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "../portable/sdl/third_party/stb_image.h"
namespace th08 {
namespace {
struct ArchiveFile final:ArchiveSource {
    FILE* file=nullptr;std::vector<char> buffer;u32 length=0;
    ArchiveFile(){file=std::fopen(host::paths().data_file("th08.dat").c_str(),"rb");
        if(file){buffer.resize(256*1024);std::setvbuf(file,buffer.data(),_IOFBF,buffer.size());std::fseek(file,0,SEEK_END);const long n=std::ftell(file);length=n>0&&n<=128*1024*1024?u32(n):0;}}
    ~ArchiveFile(){if(file)std::fclose(file);}
    u32 size()const override{return length;}
    bool read(u32 offset,u8* bytes,u32 count)override{return file&&std::fseek(file,long(offset),SEEK_SET)==0&&std::fread(bytes,1,count,file)==count;}
};
bool save_name(const std::string& name){
    if(name=="score.dat"||name=="th08.cfg")return true;
    if(name.rfind("replay/th8_",0)||name.size()<16||name.size()>24||name.substr(name.size()-4)!=".rpy")return false;
    const auto id=name.substr(11,name.size()-15);if(id.size()!=2&&(id.size()!=6||id.substr(0,2)!="ud"))return false;
    for(char c:id)if(!std::isalnum(static_cast<unsigned char>(c)))return false;return true;
}
}
bool sdl_read_file(const char* name,std::vector<u8>& result){return host::read_file(name,result,256u*1024*1024);}
bool sdl_load_assets(BrowserRuntime& r){
    std::vector<u8> bytes;if(!r.mount_archive(std::make_unique<ArchiveFile>())){host::log("th08.dat could not be mounted");return false;}
    if(!r.native_fonts()){host::log("font tables failed");return false;}
    // Only the 16-byte ZWAV header: GameConfiguration picks WAV music when it
    // is valid. The PCM itself is streamed by AudioHost.
    if(FILE* f=std::fopen(host::paths().data_file("thbgm.dat").c_str(),"rb")){u8 header[16]{};const bool ok=std::fread(header,1,16,f)==16;std::fclose(f);if(ok&&!r.put("thbgm.dat",header,16))return false;}
    else host::log("thbgm.dat not found, music disabled");
    const auto root=host::paths().save;host::make_directory(root);host::make_directory(root+"/replay");
    for(const auto* sub:{"","replay"}){
        const auto directory=*sub?root+"/"+sub:root;auto* dir=opendir(directory.c_str());if(!dir)continue;while(auto* entry=readdir(dir)){
            const auto relative=std::string(*sub?"replay/":"")+entry->d_name;
            if(save_name(relative)&&sdl_read_file((directory+"/"+entry->d_name).c_str(),bytes))r.put(relative.c_str(),bytes.data(),bytes.size());
        }closedir(dir);
    }
    GameConfiguration configuration;const auto config=r.read("th08.cfg"),wave=r.read_prefix("thbgm.dat",16);
    if(!load_configuration(configuration,config.data(),config.size(),wave.empty()?nullptr:wave.data(),wave.size()))return false;
    r.app.library.force_16bit=configuration.options&4;return true;
}
bool sdl_prepare_asset(BrowserRuntime& r,u32 index){
    if(index>=r.resources().size())return false;const auto& name=r.resources()[index].name;const auto& bytes=r.file(name.c_str());
    if(bytes.empty()&&r.resources()[index].size)return false;
    // Endings are not expanded into dozens of RGBA images at startup. Their
    // compressed source stays available and is decoded when the scene needs it.
    if(name=="title00.png"||name=="select00.png"||name=="music.jpg"||name=="th08logo.jpg")return sdl_decode_image(r,name.c_str(),bytes);
    if(name=="title01.anm"||name=="resulttext.anm"||name=="result00.anm"||name=="music00.anm")return r.app.library.preload(bytes.data(),bytes.size());
    return true;
}
bool sdl_decode_image(BrowserRuntime& r,const char* name,const std::vector<u8>& bytes){int width=0,height=0,channels=0;auto* rgba=stbi_load_from_memory(bytes.data(),int(bytes.size()),&width,&height,&channels,4);if(!rgba)return false;
    const bool result=r.put_image(name,width,height,rgba,u32(width)*height*4);stbi_image_free(rgba);return result;}
struct SDLFiles final:FileDevice {
 bool save(const char* path,const u8* bytes,u32 size)override{const auto name=BrowserRuntime::path(path);if(!save_name(name))return false;
    const bool ok=host::write_file(host::paths().save_file(name.c_str()),bytes,size);if(!ok)host::log("could not save %s",name.c_str());return ok;}
 void calendar(char* date,char* stamp)override{const auto now=std::time(nullptr);const auto* local=std::localtime(&now);if(!local)return;std::strftime(date,6,"%m/%d",local);std::strftime(stamp,20,"%y/%m/%d %H:%M:%S",local);}
 u32 milliseconds()override{return sdl_game_time();}u16 supplemental_input()override{return 0;}
 void replay_error()override{host::log("replay could not be read");}
};
FileDevice& file_device(){static SDLFiles files;return files;}

}
#endif
