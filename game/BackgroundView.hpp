#pragma once
#include "BackgroundObjects.hpp"
#include "Chain.hpp"
namespace th08 {
struct BackgroundDrawActions {
    virtual ~BackgroundDrawActions()=default;
    virtual bool stage_finished()=0;
    virtual void moon(AnmVm& effect)=0;
    virtual void effects()=0;
};
class BackgroundView {
public:
    BackgroundView(BackgroundState& state,AnmRenderer& renderer,BackgroundDrawActions& actions):state(state),renderer(renderer),objects(state,renderer),actions(actions){}
    JobResult high();JobResult low();
    void* callback_context=nullptr;
private:
    BackgroundState& state;AnmRenderer& renderer;BackgroundObjects objects;BackgroundDrawActions& actions;
    void layer(AnmVm& vm){renderer.draw_2d(vm);renderer.flush();}
    void rectangle(u32 color){const u32 colors[4]={color,color,color,color};renderer.draw_rectangle(32,16,416,464,colors);}
};
}
