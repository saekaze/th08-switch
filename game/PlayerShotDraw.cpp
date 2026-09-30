#include "PlayerShots.hpp"
namespace th08 {
void PlayerShots::draw_trail(PlayerShot& shot,const Vec2& offset){
    const u8 alpha=shot.animation.color1.a;const u32 faded=(u32(alpha)*3)>>2;
    if(shot.interval>16){failure=Failure::InvalidInterval;return;}
    for(i32 i=0;i<shot.interval*2&&shot.history[i].x!=-999;i+=2){
        shot.animation.pos=shot.history[i];if(i)shot.animation.color1.a=u8(faded-((faded/2)*u32(i))/u32(shot.interval));
        shot.animation.pos.x=Scalar::add(offset.x,shot.animation.pos.x);shot.animation.pos.y=Scalar::add(offset.y,shot.animation.pos.y);
        if(state.youkai_bonus){shot.animation.color1.r=255;shot.animation.color1.g=shot.animation.color1.b=64;}
        if(actions)actions->draw(shot.animation,false);
    }
    shot.animation.color1.a=alpha;
}
void PlayerShots::draw(bool impact,const Vec2& offset){
    failure=Failure::None;if(!actions){failure=Failure::MissingActions;return;}
    for(auto& shot:state.shots){if(shot.state!=(impact?2:1))continue;
        if(shot.animation.type){shot.animation.rotation.z=shot.angle;shot.animation.updateRotation=true;}
        shot.animation.pos={Scalar::add(offset.x,shot.position.x),Scalar::add(offset.y,shot.position.y),impact?.2f:.4f};
        if(shot.gauge_bonus){shot.animation.color1.r=255;shot.animation.color1.g=shot.animation.color1.b=64;}
        actions->draw(shot.animation,impact);if(!impact&&shot.draw==ShotDraw::Laser)draw_trail(shot,offset);
    }
}
}
