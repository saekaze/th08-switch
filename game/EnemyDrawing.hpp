#pragma once
#include "EclVm.hpp"
namespace th08 {
struct EnemyDrawActions {
    virtual ~EnemyDrawActions()=default;
    virtual void sprite(AnmVm&)=0;
    virtual void strip(AnmVm&,const SpriteVertex*,i32 count)=0;
};
// Original layer traversal and trail tessellation (0042e140).
bool draw_enemy_layers(EclVm* const* layers,i32 first,i32 last,const Vec2& offset,EnemyDrawActions&);
class EnemyDrawing:private EnemyDrawActions {
    AnmRenderer& renderer;
    void sprite(AnmVm& vm)override{renderer.draw_2d(vm);}
    void strip(AnmVm& vm,const SpriteVertex* vertices,i32 count)override{renderer.draw_vertices(vm,vertices,count);}
public:
    explicit EnemyDrawing(AnmRenderer& renderer):renderer(renderer){}
    bool draw(EclVm* const* layers,i32 first,i32 last,const Vec2& offset){return draw_enemy_layers(layers,first,last,offset,*this);}
    bool low(EclVm* const* layers,u32 flags,const Vec2& offset){const bool tinted=flags&1024;if(tinted){renderer.mix_enabled=true;renderer.mix_color=0xfff01010;}const bool result=draw(layers,2,4,offset);if(tinted){renderer.mix_enabled=false;renderer.mix_color=0x80808080;}return result;}
};
}
