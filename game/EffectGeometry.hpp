#pragma once
#include "EffectState.hpp"
#include "AnmRenderer.hpp"
namespace th08 {
class EffectGeometry {
    AnmRenderer& renderer;
public:
    Vec2 arcade{32,16};bool invalid=false;
    explicit EffectGeometry(AnmRenderer& r):renderer(r){}
    static i32 initialize(EffectState&,EffectDraw callback,bool alternative=false);
    static void release(EffectState&);
    i32 draw(EffectState&);
};
}
