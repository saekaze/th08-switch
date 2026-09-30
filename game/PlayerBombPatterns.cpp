#include "PlayerBombPatterns.hpp"
#include "PlayerBombNames.hpp"
#include "AnmTransitions.hpp"
#include "GameMath.hpp"
namespace th08 {
namespace {float raw_float(u32 bits){float value;std::memcpy(&value,&bits,4);return value;}Vec3 add(const Vec3& a,const Vec3& b){return {Scalar::add(a.x,b.x),Scalar::add(a.y,b.y),Scalar::add(a.z,b.z)};}}
void PlayerBombPatterns::begin(PlayerBombKind kind,i32 sprite,i32 duration,i32 invincibility,i32 variant){begin_player_bomb(objects,bomb,life,movement.position,sprite,player_bomb_name(kind),duration,invincibility,variant,actions);}
void PlayerBombPatterns::step(AnmVm* vm,u32 count){for(u32 i=0;i<count;++i)if(vm[i].scriptIndex>=0)actions.step_animation(vm[i]);}
void PlayerBombPatterns::tint(u32 color){actions.background_color(player_bomb_color(color,bomb.timer,bomb.duration));}
bool PlayerBombPatterns::update(PlayerBombKind kind){switch(kind){case PlayerBombKind::Youmu:if(!frame.main_animation)return false;youmu(false);break;case PlayerBombKind::YoumuLast:if(!frame.main_animation)return false;youmu(true);break;case PlayerBombKind::Yuyuko:yuyuko(false);break;case PlayerBombKind::YuyukoLast:yuyuko(true);break;case PlayerBombKind::Sakuya:sakuya(false);break;case PlayerBombKind::SakuyaLast:sakuya(true);break;case PlayerBombKind::Remilia:if(!frame.options)return false;remilia(false);break;case PlayerBombKind::RemiliaLast:if(!frame.options)return false;remilia(true);break;case PlayerBombKind::Alice:if(!frame.options)return false;alice(false);break;case PlayerBombKind::AliceLast:if(!frame.options)return false;alice(true);break;case PlayerBombKind::Reimu:reimu(false);break;case PlayerBombKind::ReimuLast:reimu(true);break;case PlayerBombKind::Marisa:marisa(false);break;case PlayerBombKind::MarisaLast:marisa(true);break;case PlayerBombKind::Yukari:yukari(false);break;case PlayerBombKind::YukariLast:yukari(true);break;case PlayerBombKind::LastWord:last_word();break;default:return false;}return true;}
bool PlayerBombPatterns::draw(PlayerBombKind kind,const Vec2& offset){switch(kind){case PlayerBombKind::Youmu:draw_youmu(false);break;case PlayerBombKind::YoumuLast:draw_youmu(true);break;case PlayerBombKind::Yuyuko:draw_yuyuko(false,offset);break;case PlayerBombKind::YuyukoLast:draw_yuyuko(true,offset);break;case PlayerBombKind::Sakuya:draw_sakuya(false,offset);break;case PlayerBombKind::SakuyaLast:draw_sakuya(true,offset);break;case PlayerBombKind::Remilia:tint(0x80d02020);break;case PlayerBombKind::RemiliaLast:tint(0x80f00000);break;case PlayerBombKind::Alice:tint(0x80404040);break;case PlayerBombKind::AliceLast:draw_alice();break;case PlayerBombKind::Reimu:draw_reimu(false,offset);break;case PlayerBombKind::ReimuLast:draw_reimu(true,offset);break;case PlayerBombKind::Marisa:case PlayerBombKind::MarisaLast:return draw_marisa(offset);case PlayerBombKind::Yukari:tint(0x80404040);break;case PlayerBombKind::YukariLast:draw_yukari(offset);break;case PlayerBombKind::LastWord:{u32 color=0x80404040;if(bomb.timer.current>=60){const u32 component=u32(wrapping_add(signed_bits(u32(wrapping_sub(bomb.timer.current,60))*176)/60,64));color=0x80000000|(component<<16)|(component<<8)|component;}tint(color);break;}default:return false;}return true;}
void PlayerBombPatterns::marisa(bool last){
    auto& object=objects.objects[0];
    if(bomb.timer.changed()&&bomb.timer.current==0){
        begin(last?PlayerBombKind::MarisaLast:PlayerBombKind::Marisa,0,last?350:300,last?380:350,last);
        if(last)actions.sound(13,0);actions.sound(19,0);object.position=movement.position;
        for(i32 i=0;i<5;++i)actions.animation(object.animation[i],(last?35:30)+i,false);
        if(!last)movement.multiplier={.2f,.2f};actions.screen(ScreenEffectType::EnvelopeShake,16,120,60,120,21);
        if(last){movement.multiplier={.2f,.2f};bomb.sequence=0;}
    }
    if(last&&bomb.timer.changed()&&bomb.timer.current%10==0){
        if(auto* e=actions.fixed_effect(53,movement.position,bomb.sequence%4+4,0xffffffff)){
            if(bomb.sequence&1)actions.animation(*e,92,true);e->segments=32;e->frequency=0;
            animation_position_transition(*e,30,4,{0,0,0},{128,0,0});animation_scale_transition(*e,30,1,{32,0},{64,0});
            animation_alpha_transition(*e,30,3,255,0);animation_rgb_transition(*e,30,0,0xffffffff,0xffff0000);actions.step_animation(*e);
        }
        bomb.sequence=wrapping_add(bomb.sequence,1);actions.panned_sound(17,movement.position.x);
    }
    if(bomb.timer.changed()&&bomb.timer.current%4!=0){
        const float y=Scalar::div(movement.position.y,2),height=Scalar::add(y,y);
        regions.rectangle(false,{192,y},384,height,6,0);
        regions.rectangle(true,{movement.position.x,y},128,height,12,0).suppress_effect=1;
        regions.rectangle(true,{192,y},384,height,last?7:6,0).suppress_effect=1;
    }
    step(object.animation,5);
}
void PlayerBombPatterns::yukari(bool last){
    const bool changed=bomb.timer.changed();auto& object=objects.objects[0];
    if(changed&&bomb.timer.current==0){
        begin(last?PlayerBombKind::YukariLast:PlayerBombKind::Yukari,1,last?250:150,last?300:200,last);actions.sound(13,0);object.position=movement.position;
        if(last){actions.animation(object.animation[0],21,false);actions.animation(object.animation[1],22,false);}
    }
    if(changed&&(bomb.timer.current==0||bomb.timer.current==10||bomb.timer.current==20||bomb.timer.current==30)){
        const i32 index=bomb.timer.current/10;const auto& pos=movement.position;
        regions.circle(false,{pos.x,pos.y},100,1,6,last&&index!=1?100:40);
        auto& damage=regions.circle(true,{pos.x,pos.y},100,1,70,40);damage.interval=5;
        static const u32 angles[]{0x3f490fdb,0x3f96cbe4,0x3fc90fdb,0x3ffb53d2};
        auto* effect=actions.parameter_effect(last?37:36,last?object.position:pos,{raw_float(angles[index]),1,4},index+4,0xffffffff);
        if(index&&effect)actions.animation(*effect,(last?92:88)+index,true);
        if(index)objects.objects[index].position=pos;
    }
    step(object.animation,2);
}
void PlayerBombPatterns::last_word(){
    if(!bomb.timer.changed()||bomb.timer.current!=0)return;const bool practice=context.game_flags&0x4000;
    begin(PlayerBombKind::LastWord,-1,practice?40:120,200,0);actions.spawn_effect(12,movement.position,1,0xff4040ff);
    if(auto* e=actions.fixed_effect(50,movement.position,4,0xff4040ff)){
        e->interpCurrentTimers[0].set(0);e->interpEndTimers[0].set(practice?30:90);e->interpModes[0]=5;
        e->posInitial.x=8;e->posInitial.y=64;e->posFinal.x=128;e->posFinal.y=0;e->pos.x=8;e->pos.y=64;
        e->segments=64;e->angle=0;e->radius=8;e->width=15;e->frequency=6;
    }
    actions.sound(13,0);movement.multiplier={0,0};for(auto* boss:input.bosses)if(boss)boss->flags&=~0x40u;
}
bool PlayerBombPatterns::draw_marisa(const Vec2& offset){
    tint(0x80404040);const float step=raw_float(0x3e567750);
    for(i32 i=0;i<5;++i){auto& vm=objects.objects[0].animation[i];if(!vm.loadedSprite)return false;
        float angle=(Extended::from_int(i)*number(step)-number(raw_float(0x3fc90fdb))-(number(step)+number(step))).to_float();if(angle<-3.1415927410125732f)angle=Scalar::add(angle,6.2831854820251465f);
        vm.pos=movement.position;vm.pos.x=(cosine(angle)*number(vm.loadedSprite->widthPx)*number(vm.scale.x)/number(2)+number(vm.pos.x)).to_float();vm.pos.y=(sine(angle)*number(vm.loadedSprite->widthPx)*number(vm.scale.x)/number(2)+number(vm.pos.y)).to_float();
        vm.rotation.z=angle;vm.updateRotation=1;vm.pos.x=Scalar::add(offset.x,vm.pos.x);vm.pos.y=Scalar::add(offset.y,vm.pos.y);vm.pos.z=0;actions.draw(vm,true);
    }return true;
}
void PlayerBombPatterns::draw_yukari(const Vec2& offset){
    tint(0x802020d0);auto& object=objects.objects[0];for(i32 i=0;i<2;++i){auto& vm=object.animation[i];vm.pos=add(object.position,vm.pos2);vm.pos.x=Scalar::add(offset.x,vm.pos.x);vm.pos.y=Scalar::add(offset.y,vm.pos.y);vm.pos.z=i?0:.01f;actions.draw(vm,true);}
}
}
