#include "BackgroundView.hpp"
#include <algorithm>
namespace th08 {
JobResult BackgroundView::high(){
    auto& s=state;auto& r=renderer;s.effect_flags=0;for(u32 i=0;i<16;++i)s.effect_positions[i]={};
    auto viewport=r.viewport;viewport.x=32;viewport.y=16;viewport.width=384;viewport.height=448;r.viewport=viewport;r.begin_background();
    if(!r.fog_disabled)r.set_fog(false);r.flush();
    if(s.spell_flag){r.set_viewport({32,16,384,448,0,1});r.clear_target(1,0xff000000);s.spell_flag=0;}
    r.set_viewport(viewport);
    if(s.tint_color.a){r.mix_enabled=true;r.mix_color=s.tint_color.d3dColor;}s.tint_color.d3dColor=0x00808080;
    if(s.spell_state<2&&!actions.stage_finished()){for(u32 i=0;i<2;++i)if(s.layers[i].activeSpriteIndex>0)layer(s.layers[i]);if(s.moon_effect)actions.moon(*s.moon_effect);}
    if((s.clear_color&0xff000000)==0xff000000)r.clear_target(3,s.clear_color);
    else{if(s.clear_color)rectangle(s.clear_color);r.clear_target(2,s.clear_color);}
    r.set_depth_func(DepthFunc::LessEqual);u32 fog=s.fog.color.d3dColor;
    if(r.mix_enabled){u32 result=fog&0xff000000;for(u32 shift=0;shift<24;shift+=8)result|=std::min(255u,(((fog>>shift)&255)*((r.mix_color>>shift)&255))>>7)<<shift;fog=result;}
    r.set_fog_color(fog);r.set_fog_range(s.fog.near_plane,s.fog.far_plane);if(!r.fog_disabled)r.set_fog(true);
    if(s.spell_state<2&&!actions.stage_finished()){objects.draw(0);objects.draw(1);}return JobResult::Continue;
}
JobResult BackgroundView::low(){
    auto& s=state;auto& r=renderer;
    if(s.spell_state<2&&!actions.stage_finished()){
        objects.draw(2);objects.draw(3);if(!r.fog_disabled)r.set_fog(false);actions.effects();
        if(s.spell_state==1){const i32 alpha=signed_bits(u32(s.spell_frames)*255)/60;r.flush();r.set_depth_func(DepthFunc::Always);if(!r.fog_disabled)r.set_fog_state(false);rectangle(u32(alpha)<<24);}
    }
    r.flush();r.set_depth_func(DepthFunc::Always);if(!r.fog_disabled)r.set_fog(false);
    if(s.spell_state>0){for(i32 i=0;i<s.spell_vm_count&&i<32;++i)layer(s.spell_vms[i]);if(s.callback)s.callback(s,r,callback_context);}
    r.screen_camera();r.set_viewport(r.viewport);r.set_fog_range(1000,2000);
    if(!s.use_tint){r.mix_enabled=false;r.mix_color=0x80808080;}s.use_tint=0;s.effect_visible=0;return JobResult::Continue;
}
}
