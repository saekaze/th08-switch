#pragma once
#include "../platform/BrowserRuntime.hpp"
namespace touhou::sdl {class Renderer;}
namespace th08 {bool sdl_attach(BrowserRuntime*);void sdl_detach();i32 sdl_graphics(u32,u32,u32,u32);touhou::sdl::Renderer* sdl_renderer();}
