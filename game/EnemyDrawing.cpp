#include "EnemyDrawing.hpp"
#include "GameMath.hpp"
#include <cmath>
namespace th08 {
namespace {
void rotate(AnmVm& vm,float angle){vm.rotation.z=angle;vm.updateRotation=true;}
void place(AnmVm& vm,const Vec3& p,const Vec3& extra,const Vec2& offset,float depth){
    vm.pos={Scalar::add(p.x,extra.x),Scalar::add(p.y,extra.y),depth};
    vm.pos.x=Scalar::add(offset.x,vm.pos.x);vm.pos.y=Scalar::add(offset.y,vm.pos.y);
}
Extended middle_angle(float a,float b){
    float direct,wrapped;
    if(b<=a){direct=Scalar::sub(a,b);wrapped=(number(b)+number(6.283185482025147f)-number(a)).to_float();a=b;}
    else{direct=Scalar::sub(b,a);wrapped=(number(a)+number(6.283185482025147f)-number(b)).to_float();}
    return number(wrapped<=direct?wrapped:direct)*number(.5f)+number(a);
}
bool trail_strip(EclVm& enemy,EnemyDrawActions& actions){
    auto& trail=enemy.trail;auto& vm=enemy.animation[0];
    i32 count=0;for(i32 j=0;j<trail.length&&!(trail.points[j].position.x<-990);j+=trail.step)count+=2;
    if(count<=2)return true;if(!vm.loadedSprite){enemy.invalid=true;enemy.failure=EclVm::Failure::MissingAnimation;return false;}
    const auto& sprite=*vm.loadedSprite;
    const float span=Scalar::sub(sprite.uvEnd.x,sprite.uvStart.x);
    const float increment=(number(span)/Extended::from_int((count+1)/2-1)).to_float();
    float u=Scalar::add(sprite.uvEnd.x,vm.uvScrollPos.x),previous=0;
    u32 written=0;
    for(i32 j=0;j<trail.length&&!(trail.points[j].position.x<-990);j+=trail.step,u=Scalar::sub(u,increment)){
        const auto& point=trail.points[j];const float angle=j?middle_angle(trail.points[j-1].angle,point.angle).to_float():point.angle;
        if((trail.flags&2)&&j>0&&j+trail.step<trail.length){
            // The original look-ahead's second operand is the fixed step-th
            // node, not j+step. Keep that resource-visible behavior.
            const float next=middle_angle(trail.points[j-1+trail.step].angle,trail.points[trail.step].angle).to_float();
            if(std::fabs(Scalar::sub(previous,angle))<1e-5f&&std::fabs(Scalar::sub(angle,next))<1e-5f){count-=2;continue;}
        }
        previous=angle;const float sin=sine(angle).to_float(),cos=cosine(angle).to_float();
        float x=0,y=(number(vm.scale.y)*number(sprite.heightPx)/number(2)).to_float();
        if(trail.flags&2){const float factor=(number(1)-Extended::from_int(j)/Extended::from_int(trail.length)).to_float();x=Scalar::mul(x,factor);y=Scalar::mul(y,factor);}
        if(written+2>194){enemy.invalid=true;return false;}auto& a=trail.vertices[written++];auto& b=trail.vertices[written++];
        a.color=b.color=u32(vm.color1.d3dColor);if(trail.flags&4){const u8 alpha=u8(vm.color1.a-i32(vm.color1.a)*j/trail.length);a.color=b.color=(a.color&0xffffff)|(u32(alpha)<<24);}
        a.pos=point.position;b.pos=point.position;
        a.pos.x=(number(cos)*number(x)-number(sin)*number(y)+number(32)+number(a.pos.x)).to_float();
        a.pos.y=(number(cos)*number(y)+number(sin)*number(x)+number(16)+number(a.pos.y)).to_float();
        b.pos.x=(number(sin)*number(y)+number(cos)*number(x)+number(32)+number(b.pos.x)).to_float();
        b.pos.y=(number(sin)*number(x)-number(cos)*number(y)+number(16)+number(b.pos.y)).to_float();
        a.uv={u,Scalar::add(sprite.uvStart.y,vm.uvScrollPos.y)};b.uv={u,Scalar::add(sprite.uvEnd.y,vm.uvScrollPos.y)};
    }
    if(count>2)actions.strip(vm,trail.vertices,count);return true;
}
bool draw_enemy(EclVm& enemy,const Vec2& offset,EnemyDrawActions& actions){
    auto& main=enemy.animation[0];
    const auto satellite=[&](u32 index){auto& vm=enemy.animation[index];if(vm.scriptIndex<0)return;if(vm.type)rotate(vm,index==1?enemy.direction.z:-enemy.direction.z);place(vm,enemy.resolved_position,(enemy.flags2&0x100)?main.pos2:vm.pos2,offset,.3f);actions.sprite(vm);};
    satellite(1);if(enemy.flags&0x2000000)rotate(main,enemy.direction.z);
    place(main,enemy.resolved_position,main.pos2,offset,.25f);
    auto& trail=enemy.trail;
    if(trail.flags){
        if(trail.length>96||trail.step<=0){enemy.invalid=true;return false;}
        const Vec2 scale=main.scale;const ZunColor color=main.color1;
        if(!(trail.flags&8)){
            for(i32 j=trail.length-1;j>0;j-=trail.step)if(!(trail.points[j].position.x<-990)){
                if(enemy.flags&0x2000000)rotate(main,trail.points[j].angle);
                if(trail.flags&2)main.scale.x=(number(scale.x)-Extended::from_int(j)*number(scale.x)/Extended::from_int(trail.length)).to_float();
                if(trail.flags&4)main.color1.a=u8(color.a-i32(color.a)*j/trail.length);
                place(main,trail.points[j].position,main.pos2,offset,.3f);actions.sprite(main);
            }
        }else if(!trail_strip(enemy,actions))return false;
        main.scale=scale;main.color1=color;
    }
    // Position and rotation deliberately retain the final trail sample.
    if(!(trail.flags&16)&&!(enemy.flags&32))actions.sprite(main);
    satellite(2);return true;
}
}
bool draw_enemy_layers(EclVm* const* layers,i32 first,i32 last,const Vec2& offset,EnemyDrawActions& actions){
    if(first<0||last>4)return false;
    for(i32 layer=first;layer<last;++layer){u32 count=0;for(auto* enemy=layers[layer];enemy;enemy=enemy->next_in_layer){if(++count>480||!draw_enemy(*enemy,offset,actions))return false;}}
    return true;
}
}
