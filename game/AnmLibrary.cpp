#include "AnmLibrary.hpp"
namespace th08 {
AnmLibrary::~AnmLibrary() { for(i32 i=0;i<256;++i)release(i);clear_preloads(); }
void AnmLibrary::clear_preloads(){
    // Active entries refer to their pristine template. Release them before
    // dropping the cache; ordinary scene release keeps resident textures alive.
    for(i32 i=0;i<256;++i)if(files[i]&&files[i]->prepared)release(i);
    for(const auto& entry:prepared)for(u32 handle:entry->resident)textures.release(handle);
    prepared.clear();prepared_bytes=0;
}
AnmLoaded* AnmLibrary::get(i32 index) noexcept { return index>=0&&index<256&&files[index]?&files[index]->resource.view():nullptr; }
const AnmResource* AnmLibrary::resource(i32 index) const noexcept { return index>=0&&index<256&&files[index]?&files[index]->resource:nullptr; }
void AnmLibrary::release(i32 index) {
    if(index<0||index>=256||!files[index])return;
    for(auto& entry:files[index]->textures)textures.release(entry.texture);
    files[index].reset();
}
AnmLoaded* AnmLibrary::load(i32 index,Archive& archive,const char* name,bool deferred) {
    if(index<0||index>=25)return nullptr;
    release(index);
    std::vector<u8> bytes;
    if(!archive.read(name,bytes))return nullptr;
    return load(index,bytes.data(),bytes.size(),deferred);
}
AnmLoaded* AnmLibrary::load(i32 index,const u8* bytes,u32 size,bool deferred) {
    if(index<0||index>=25)return nullptr;
    release(index);
    auto entry=std::make_unique<Entry>();
    for(const auto& candidate:prepared)if(candidate->force_16bit==force_16bit&&candidate->resource.data().size()==size&&!std::memcmp(candidate->resource.data().data(),bytes,size)){entry->prepared=candidate.get();break;}
    if(entry->prepared){entry->resource.clone_from(entry->prepared->resource,index);++cache_hits;}
    else if(!entry->resource.load(index,bytes,size))return nullptr;
    // Every original TH08 ANM embeds THTX pixels or allocates an empty atlas.
    for(const auto& source:entry->resource.textures())if(!source.embedded&&!source.empty)return nullptr;
    entry->textures.resize(entry->resource.textures().size());
    auto& view=entry->resource.view();view.textures=entry->textures.data();
    files[index]=std::move(entry);
    if(deferred) view.numberEntriesToBeLoaded=1;
    else for(u32 i=0;i<files[index]->textures.size();++i)if(!materialize(*files[index],i)){release(index);return nullptr;}
    return get(index);
}
bool AnmLibrary::materialize(Entry& entry,u32 index) {
    if(index>=entry.textures.size())return false;
    const auto& source=entry.resource.textures()[index];
    if(entry.prepared){
        const u32 handle=entry.prepared->resident[index];auto* resident=textures.get(handle);
        // Only an idle static texture may be borrowed. Text/capture atlases and
        // simultaneous scene instances always retain independent writable data.
        if(resident&&resident->references==1){
            const auto& pristine=entry.prepared->pixels[index];
            if(resident->revision!=entry.prepared->resident_revision[index]||resident->image.pixels!=pristine.pixels){resident->image=pristine;textures.changed(handle);entry.prepared->resident_revision[index]=resident->revision;}
            textures.retain(handle);textures.release(entry.textures[index].texture);
            entry.textures[index].texture=handle;++texture_cache_hits;
            return entry.resource.configure_texture(index,handle,resident->image.width,resident->image.height);
        }
    }
    TexturePixels image;
    if(entry.prepared)image=entry.prepared->pixels[index];
    else if(source.embedded) {
        if(!image.from_anm(entry.resource.data().data()+source.pixel_offset-16,source.pixel_size+16,source.format,force_16bit))return false;
    } else {
        // Original empty atlases bypass GetAnmFormat's force-16-bit override.
        u32 format=TexturePixels::anm_format(source.format,false);if(source.format==0)format=21;
        if(!image.create(source.width,source.height,format))return false;
    }
    const u32 width=image.width,height=image.height,handle=textures.insert(std::move(image),source.priority,source.empty);
    if(!handle)return false;
    textures.release(entry.textures[index].texture);entry.textures[index].texture=handle;
    return entry.resource.configure_texture(index,handle,width,height);
}
bool AnmLibrary::preload(const u8* bytes,u32 size){
    const bool reduced=force_16bit;{
        for(const auto& entry:prepared)if(entry->force_16bit==reduced&&entry->resource.data().size()==size&&!std::memcmp(entry->resource.data().data(),bytes,size))return true;
        auto entry=std::make_unique<Prepared>();entry->force_16bit=reduced;if(!entry->resource.load(0,bytes,size))return false;
        u32 cost=size;
        for(const auto& source:entry->resource.textures()){
            TexturePixels image;
            if(source.embedded){if(!image.from_anm(entry->resource.data().data()+source.pixel_offset-16,source.pixel_size+16,source.format,reduced))return false;}
            else if(source.empty){u32 format=TexturePixels::anm_format(source.format,false);if(source.format==0)format=21;if(!image.create(source.width,source.height,format))return false;}
            else return false;
            cost+=image.pixels.size();entry->pixels.push_back(std::move(image));
        }
        // Bound the shared/menu and opening-stage cache on mobile devices.
        if(prepared_bytes+cost>32*1024*1024)return false;
        for(u32 i=0;i<entry->pixels.size();++i){
            const auto& source=entry->resource.textures()[i];u32 handle=0;
            if(!source.empty){auto pixels=entry->pixels[i];handle=textures.insert(std::move(pixels),source.priority);}
            entry->resident.push_back(handle);entry->resident_revision.push_back(handle?textures.get(handle)->revision:0);if(handle)textures.prepare(handle);
        }
        prepared_bytes+=cost;prepared.push_back(std::move(entry));
    }return true;
}
bool AnmLibrary::postload(i32 index) {
    auto* view=get(index);if(!view)return false;
    if(view->numberEntriesToBeLoaded==0)return true;
    const u32 entry=u32(view->numberEntriesToBeLoaded-1);
    if(!materialize(*files[index],entry)){view->numberEntriesToBeLoaded=0;return false;}
    view->numberEntriesToBeLoaded=entry+1<files[index]->textures.size()?i32(entry+2):0;
    return true;
}
bool AnmLibrary::service() {
    for(i32 i=0;i<256;++i)if(files[i]&&files[i]->resource.view().numberEntriesToBeLoaded&&!postload(i))return false;
    return true;
}
bool AnmLibrary::start(i32 index,i32 script,AnmVm& vm,AnmExecutor& executor,bool reset_position) {
    auto* entry=get(index);
    if(!entry||script<0||u32(script)>=files[index]->resource.script_count())return false;
    vm.scriptIndex=i16(script);
    if(reset_position) { vm.pos={};vm.pos2={};vm.fontWidth=vm.fontHeight=15; }
    executor.start(*entry,vm,entry->scripts[script]);
    return true;
}
bool AnmLibrary::has_texture(const AnmVm& vm) const noexcept {
    if(!vm.loadedSprite||vm.loadedSprite->anmIdx<0||vm.loadedSprite->anmIdx>=256)return false;
    const auto& file=files[vm.loadedSprite->anmIdx];
    return file && file->textures.data()!=nullptr;
}
}
