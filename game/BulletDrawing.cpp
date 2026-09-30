#include "BulletDrawing.hpp"
#include "GameMath.hpp"
namespace th08 {
void BulletDrawing::bullet(BulletState& b,const Vec2& origin){
    auto& vm=b.sprites.animation[b.state>=2&&b.state<=5?b.state-1:0];
    vm.pos={Scalar::add(origin.x,b.position.x),Scalar::add(origin.y,b.position.y),.05f};
    vm.color1.d3dColor=i32((u32(vm.color1.d3dColor)&0xff000000)|0xffffff);
    if(vm.type){vm.rotation.z=add_angle(Scalar::add(1.5707964f,b.angle),0);vm.updateRotation=true;}
    actions.draw(vm);
}
void BulletDrawing::laser(LaserState& l,const Vec2& origin){
    const float cosine_value=cosine(l.angle).to_float(),sine_value=sine(l.angle).to_float();
    const float midpoint=((number(l.end_offset)-number(l.start_offset))/number(2)+number(l.start_offset)).to_float();
    const auto position=[&](float distance,float depth){return Vec3{(number(cosine_value)*number(distance)+number(l.position.x)).to_float(),(number(sine_value)*number(distance)+number(l.position.y)).to_float(),depth};};
    const auto offset=[&](AnmVm& vm){vm.pos.x=Scalar::add(origin.x,vm.pos.x);vm.pos.y=Scalar::add(origin.y,vm.pos.y);};
    auto& body=l.animation[0];body.pos=position(midpoint,.06f);l.color=-1;offset(body);actions.draw(body);
    if((l.start_offset<16||l.speed==0)&&(!l.unknown599||l.state)){
        auto& cap=l.animation[1];cap.pos=position(l.start_offset,.05f);cap.color1=body.color1;cap.flag6=1;cap.color1.a=255;
        cap.scale.x=((number(l.width)/number(10))*((number(16)-number(l.start_offset))/number(16))).to_float();cap.scale.y=cap.scale.x;
        if(cap.scale.y<=0){cap.scale.x=Scalar::div(l.width,10);cap.scale.y=cap.scale.x;}
        offset(cap);actions.draw(cap);
    }
}
bool BulletDrawing::draw(u32 flags,const Vec2& origin){
    const bool tint=flags&1024;if(tint)actions.tint(0xfff01010);actions.items();
    for(auto& l:state.lasers)if(l.in_use)laser(l,origin);
    for(auto* layer:state.layers){u32 count=0;for(auto* b=layer;b;b=b->next_in_layer){if(++count>1536){if(tint)actions.clear_tint();return false;}bullet(*b,origin);}}
    actions.effects();if(tint)actions.clear_tint();return true;
}
}
