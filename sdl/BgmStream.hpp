// Switch host: a thbgm.dat track as a miniaudio data source.
#pragma once
#include "MiniaudioConfig.hpp"
#include "../game/Types.hpp"
#include <cstdio>
#include <vector>
namespace th08::host {
struct PcmStream {
    ma_data_source_base base{};FILE* file=nullptr;u64 offset=0,frames=0,cursor=0;u32 channels=2,rate=44100;
    std::vector<i16> scratch;
};
ma_result pcm_seek(ma_data_source*,ma_uint64 frame);
extern const ma_data_source_vtable pcm_vtable;
}
