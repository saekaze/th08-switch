#pragma once
#include "BulletState.hpp"
namespace th08 {
struct BulletDrawingActions {
    virtual ~BulletDrawingActions()=default;
    virtual void tint(u32 color)=0;
    virtual void clear_tint()=0;
    virtual void items()=0;
    virtual void effects()=0;
    virtual void draw(AnmVm& vm)=0;
};
// Original 00432b50 / 00432f20, including the six linked drawing layers,
// laser origins and the deathbomb tint shared with the item/effect passes.
class BulletDrawing {
    BulletManagerState& state;BulletDrawingActions& actions;
    void laser(LaserState&,const Vec2&);
public:
    BulletDrawing(BulletManagerState& s,BulletDrawingActions& a):state(s),actions(a){}
    void bullet(BulletState&,const Vec2&);
    bool draw(u32 game_flags,const Vec2& arcade);
};
}
