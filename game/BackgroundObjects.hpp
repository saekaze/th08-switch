#pragma once
#include "BackgroundState.hpp"
#include "AnmRenderer.hpp"
namespace th08 {
class BackgroundObjects {
public:
    BackgroundObjects(BackgroundState& state,AnmRenderer& renderer):state(state),renderer(renderer){}
    void draw(i32 layer);
private:
    BackgroundState& state;AnmRenderer& renderer;
    Vec3 projection_input;
    void sprite(AnmVm& vm,const StageSpriteQuad& quad,const StageInstance& instance,const Vec3& right,i32& fog_mode);
    void beam(AnmVm& vm,const StageBeamQuad& quad,const StageInstance& instance,const Vec3& right,i32& fog_mode);
    Vec3 project(const Vec3& position)const;
    float fog_amount(float distance)const;
    u32 fog_color(u32 color,float amount)const;
};
}
