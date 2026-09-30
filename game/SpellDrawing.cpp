#include "SpellDrawing.hpp"
namespace th08 {
namespace {
float add(float x,float delta){return Scalar::add(x,delta);}
}
bool SpellDrawing::digit(i32 value){
    const i32 index=wrapping_add(value,136);if(!digits||!digits->sprites||index<0||u32(index)>=digits->spriteCount)return false;
    auto& vm=state.spell_vms[12];vm.loadedSprite=&digits->sprites[index];renderer.draw_no_rotation(vm);return true;
}
bool SpellDrawing::draw(){
    auto& v=state.spell_vms;auto& r=renderer;
    if(v[0].visible){r.draw_no_rotation(v[0]);r.draw_no_rotation(v[2]);r.draw_2d(v[4]);}
    if(v[1].visible){
        const Vec3 saved=v[1].pos;v[1].pos={add(v[1].pos.x,v[1].pos2.x),add(v[1].pos.y,v[1].pos2.y),add(v[1].pos.z,v[1].pos2.z)};
        r.draw_no_rotation(v[1]);v[1].pos=saved;r.draw_no_rotation(v[3]);r.draw_2d(v[5]);
    }
    if(v[6].visible){v[10].pos=v[6].pos;v[10].pos.x=add(v[10].pos.x,-32);r.draw_no_rotation(v[10]);r.draw_2d(v[6]);}
    if(!v[7].visible)return true;
    r.mix_enabled=true;r.mix_color=state.spell_panel_color;v[11].pos=v[7].pos;
    r.draw_no_rotation(v[11]);r.draw_2d(v[7]);r.draw_2d(v[8]);r.draw_2d(v[9]);r.draw_no_rotation(v[13]);
    bool valid=true;
    if(!(state.spell_flags&1024)){
        if(!records||state.spell_number>=spell_count||state.shot<0||state.shot>=12)valid=false;
        else {
            auto& pos=v[12].pos;pos=v[13].pos;pos.x=add(pos.x,-40);pos.y=add(pos.y,1);
            i32 value=(state.spell_flags&4)?signed_bits(state.spell_bonus):0,divisor=10000000;bool leading=false;
            for(i32 i=0;i<8;i++){
                const i32 n=value/divisor;if(n)leading=true;if(leading||divisor==1)valid=digit(n)&&valid;pos.x=add(pos.x,7);value%=divisor;divisor/=10;
            }
            const auto& history=(state.game_flags&0x4000)?records[state.spell_number].practice:records[state.spell_number].game;
            leading=false;
            const auto three=[&](i32 n){
                if(n>999)n=999;if(n/100){valid=digit(n/100)&&valid;n%=100;leading=true;}pos.x=add(pos.x,7);
                if(n/10||leading){valid=digit(n/10)&&valid;n%=10;}pos.x=add(pos.x,7);valid=digit(n%10)&&valid;
            };
            pos.x=add(pos.x,32);three(signed_bits(history.captures[state.shot]));pos.x=add(pos.x,13);three(signed_bits(history.attempts[state.shot]));
        }
    }
    r.mix_enabled=false;r.mix_color=0x80808080;return valid;
}
}
