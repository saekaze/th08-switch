// One miniaudio configuration for every translation unit that includes it
// (the implementation lives in AudioDevice.cpp).
#pragma once
#define MA_NO_DEVICE_IO
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_MP3
#define MA_NO_FLAC
#define MA_NO_ENCODING
#define MA_NO_THREADING
#define MA_NO_VORBIS

#include "../portable/sdl/third_party/miniaudio.h"
