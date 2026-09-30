// Streams signed 16-bit interleaved PCM out of thbgm.dat at a byte offset,
// converted to f32 on read. Looping is left to miniaudio's data-source loop
// points, exactly like the upstream decoder path.
#include "BgmStream.hpp"
#include <algorithm>
namespace th08::host {
ma_result pcm_read(ma_data_source* source,void* out,ma_uint64 count,ma_uint64* read){
    auto& s=*static_cast<PcmStream*>(source);if(read)*read=0;
    const u64 n=std::min<u64>(count,s.frames-std::min(s.cursor,s.frames));if(!n)return MA_AT_END;
    s.scratch.resize(size_t(n*s.channels));
    const size_t got=std::fread(s.scratch.data(),s.channels*2,size_t(n),s.file);
    auto* pcm=static_cast<float*>(out);for(size_t i=0;i<got*s.channels;++i)pcm[i]=float(s.scratch[i])/32768.f;
    // Report MA_AT_END together with the final frames: miniaudio's looping
    // treats a later empty MA_AT_END read as "no audio" and stops the loop.
    s.cursor+=got;if(read)*read=got;return got&&s.cursor<s.frames?MA_SUCCESS:MA_AT_END;
}
ma_result pcm_seek(ma_data_source* source,ma_uint64 frame){
    auto& s=*static_cast<PcmStream*>(source);if(frame>s.frames)return MA_INVALID_ARGS;
    if(std::fseek(s.file,long(s.offset+frame*s.channels*2),SEEK_SET)!=0)return MA_ERROR;s.cursor=frame;return MA_SUCCESS;
}
ma_result pcm_format(ma_data_source* source,ma_format* format,ma_uint32* channels,ma_uint32* rate,ma_channel* map,size_t cap){
    auto& s=*static_cast<PcmStream*>(source);if(format)*format=ma_format_f32;if(channels)*channels=s.channels;if(rate)*rate=s.rate;
    if(map)ma_channel_map_init_standard(ma_standard_channel_map_default,map,cap,s.channels);return MA_SUCCESS;
}
ma_result pcm_cursor(ma_data_source* source,ma_uint64* cursor){*cursor=static_cast<PcmStream*>(source)->cursor;return MA_SUCCESS;}
ma_result pcm_length(ma_data_source* source,ma_uint64* length){*length=static_cast<PcmStream*>(source)->frames;return MA_SUCCESS;}
const ma_data_source_vtable pcm_vtable{pcm_read,pcm_seek,pcm_format,pcm_cursor,pcm_length,nullptr,0};
}
