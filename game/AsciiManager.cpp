// TH08 1.00d display logic. Names and behavior cross-checked against the
// MIT GensokyoClub/th08 reference; original instruction results are the oracle.
#include "AsciiManager.hpp"
#include <cmath>
#include <cstdio>
#include <cstdarg>
#include <algorithm>
namespace th08 {
namespace {
float add(float a,float b){return Scalar::add(a,b);}
float sub(float a,float b){return Scalar::sub(a,b);}
float mul(float a,float b){return Scalar::mul(a,b);}
Extended integer(i32 x){return Extended::from_int(x);}
i32 popup_alpha(const Vec3& player,const Vec3& position){
    const float x=sub(player.x,position.x),y=sub(player.y,position.y);
    const i32 distance=(number(x)*number(x)+number(y)*number(y)).truncate_int();
    return distance>4096?208:distance>1024?80+(distance-1024)*128/3072:80;
}
}
void AsciiManager::start(AnmVm& vm,AnmLoaded& file,i32 script){
    vm.anmFile=&file;vm.scriptIndex=i16(script);executor.start(file,vm,file.scripts[script]);
}
void AsciiManager::set_sprite(AnmVm& vm,i32 sprite,bool initialize){
    if(!state.ascii)return;
    if(initialize){vm.Initialize();vm.anmFile=state.ascii;}
    state.ascii->SetSprite(&vm,sprite);
}
void AsciiManager::direct_sprite(AnmVm& vm,i32 sprite){
    if(state.ascii&&sprite>=0&&u32(sprite)<state.ascii->spriteCount)vm.loadedSprite=&state.ascii->sprites[sprite];
}
void AsciiManager::reset(){
    auto& s=state;
    std::memset(&s.small_score_text,0,sizeof(AnmVm));std::memset(&s.popup_text,0,sizeof(AnmVm));std::memset(&s.large_text,0,sizeof(AnmVm));
    std::memset(s.strings,0,sizeof(s.strings));std::memset(&s.pause,0,sizeof(s.pause));std::memset(&s.retry,0,sizeof(s.retry));
    std::memset(s.score_popups,0,sizeof(s.score_popups));std::memset(s.time_popups,0,sizeof(s.time_popups));
    s.string_count=s.gui=s.selected=s.next_score=s.next_player=s.unused=0;
    // Reset deliberately preserves the next time-popup index and gauge state.
    s.color=0xffffffff;s.scale_x=s.scale_y=1;
    s.small_score_text.anchor=s.popup_text.anchor=3;
    set_sprite(s.small_score_text,0,true);set_sprite(s.popup_text,136,true);set_sprite(s.large_text,32,true);
    s.small_score_text.pos.z=.1f;s.space_width=13;
}
void AsciiManager::initialize_vms(const AsciiContext& c){
    if(!state.ascii)return;
    auto& s=state;
    for(const auto pair:{std::pair<AnmVm*,i32>{&s.gauge,5},{&s.youkai_icon,7},{&s.human_icon,6},{&s.cursor,8},{&s.percentage,4},{&s.border,9},
        {&s.boss_markers[0],10},{&s.boss_markers[1],10},{&s.boss_markers[2],10},{&s.boss_markers[3],10}})start(*pair.first,*s.ascii,pair.second);
    s.human_icon.pos.x=(number(s.human_icon.pos.x)-(integer(c.human_limit)*number(56)/number(-10000))).to_float();
    s.youkai_icon.pos.x=(number(s.youkai_icon.pos.x)+(integer(c.youkai_limit)*number(56)/number(10000))).to_float();
    set_gauge_interrupt(s.gauge_interrupt);
}
void AsciiManager::set_gauge_interrupt(i32 interrupt){
    for(auto* vm:{&state.gauge,&state.human_icon,&state.youkai_icon,&state.cursor})vm->pendingInterrupt=i16(interrupt);
    state.gauge_interrupt=interrupt;
}
void AsciiManager::tick_popups(const AsciiContext& c,const FrameTiming& timing){
    if(c.paused||c.retry||c.freeze_popups)return;
    for(auto& p:state.score_popups)if(p.in_use){p.position.y=(number(p.position.y)-number(.5f)*number(timing.rate)).to_float();p.timer.tick(timing);if(p.timer.current>60)p.in_use=0;}
    for(auto& p:state.time_popups)if(p.in_use){p.timer.tick(timing);if(p.timer.current>90)p.in_use=0;}
}
void AsciiManager::tick_vms(bool demo){
    auto& s=state;
    for(auto* vm:{&s.gauge,&s.human_icon,&s.youkai_icon,&s.cursor,&s.percentage,&s.boss_markers[0],&s.boss_markers[1],&s.boss_markers[2],&s.boss_markers[3],&s.border})executor.execute(*vm);
    if(demo){if(s.demo.scriptIndex==0&&s.ascii)start(s.demo,*s.ascii,11);executor.execute(s.demo);}else s.demo.scriptIndex=0;
    ++s.frame;
}
bool AsciiManager::add_string(const Vec3& position,const char* text,bool software_texturing){
    if(!text||state.string_count>=256||state.string_count<0)return false;
    // Game strings fit the original 64-byte field. Reject overflow instead of
    // reproducing strcpy's overwrite of adjacent game state for external text.
    if(std::strlen(text)>=64)return false;
    auto& next=state.strings[state.string_count++];std::strcpy(next.text,text);
    next.position=position;next.color=state.color;next.scale_x=state.scale_x;next.scale_y=state.scale_y;next.gui=state.gui;next.selected=software_texturing?state.selected:0;return true;
}
i32 AsciiManager::add_format(const Vec3& position,bool software,const char* format,...){
    char buffer[512];va_list args;va_start(args,format);const i32 length=std::vsnprintf(buffer,sizeof(buffer),format,args);va_end(args);
    if(length<0||length>=i32(sizeof(buffer)))return -1;add_string(position,buffer,software);return length;
}
void AsciiManager::create_score(const Vec3& position,i32 value,u32 color,const AsciiContext& c,bool player){
    auto& s=state;auto& next=player?s.next_player:s.next_score;const i32 capacity=player?3:720;
    if(next>=capacity||next<0)next=0;
    auto& p=s.score_popups[(player?720:0)+next++];p.in_use=1;
    i32 count=0;
    if(value>=0){while(value){p.text[count++]=u8(value%10);value/=10;}}else p.text[count++]=10;
    if(!count)p.text[count++]=0;p.characters=u8(count);p.color=color;p.timer.set(0);p.position=position;
    p.position.x=add(p.position.x,c.arcade_origin.x);p.position.y=add(p.position.y,c.arcade_origin.y);
}
void AsciiManager::create_time(const Vec3& position,i32 value,i32 multiplier,u32 color,const AsciiContext& c,bool familiar){
    auto& s=state;if(s.next_time>=128||s.next_time<0)s.next_time=0;
    auto& p=s.time_popups[s.next_time++];p.in_use=1;i32 count=0;
    // Original gameplay caps both quantities; preserve all valid twelve-glyph
    // popups while bounding malformed external values to this record.
    auto append=[&](u8 value){if(count<12)p.text[count++]=value;};
    if(multiplier>0){append(15);while(multiplier){append(u8(multiplier%10));multiplier/=10;}append(14);}
    if(value>0){while(value){append(u8(value%10));value/=10;}}else append(0);append(13);
    p.characters=u8(count);p.color=color;p.timer.set(familiar?88:0);p.position=position;
    p.position.x=((familiar?number(3.5f)*integer(count)+number(c.arcade_origin.x):number(c.arcade_origin.x))+number(p.position.x)).to_float();
    p.position.y=add(p.position.y,c.arcade_origin.y);p.scale_x=s.scale_x;p.scale_y=s.scale_y;
}
void AsciiManager::draw_strings(const AsciiContext& c){
    auto& s=state;auto& vm=s.large_text;vm.visible=true;vm.anchor=3;bool gui=true;
    auto viewport=[&](bool arcade){auto v=renderer.viewport;v.x=arcade?u32(Scalar::truncate(c.arcade_origin.x)):0;v.y=arcade?u32(Scalar::truncate(c.arcade_origin.y)):0;
        v.width=arcade?u32(Scalar::truncate(c.arcade_size.x)):640;v.height=arcade?u32(Scalar::truncate(c.arcade_size.y)):480;renderer.set_viewport(v);};
    for(i32 i=0;i<s.string_count&&i<256;++i){const auto& str=s.strings[i];vm.pos=str.position;vm.scale={str.scale_x,str.scale_y};const float space=(integer(s.space_width)*number(str.scale_x)).to_float();
        if(gui!=bool(str.gui)){gui=str.gui;viewport(gui);}
        for(const auto* p=reinterpret_cast<const u8*>(str.text);p<reinterpret_cast<const u8*>(str.text)+64&&*p;++p){
            if(*p=='\n'){vm.pos.y=(number(vm.pos.y)+number(16)*number(str.scale_y)).to_float();vm.pos.x=str.position.x;}
            else {if(*p!=' '){direct_sprite(vm,*p+(str.selected?138:-1));vm.color1.d3dColor=str.selected?0xffffffff:str.color;renderer.draw_no_rotation(vm);}vm.pos.x=add(vm.pos.x,space);}
        }
    }
    if(gui)viewport(false);
    for(u32 i=0;i<4;++i){auto& marker=s.boss_markers[i];if(marker.pos.x<56||marker.pos.x>392)continue;
        const float distance=std::fabs((number(marker.pos.x)-number(32)-number(c.player.x)).to_float());direct_sprite(marker,157);
        bool normal=false;
        switch(s.boss_states[i]){
        case 0:normal=true;break;
        case 1:marker.color1.d3dColor=0x80ff4040;break;
        case 2:case 3:case 4:if(s.frame%(1u<<(5-s.boss_states[i]))==0){direct_sprite(marker,158);marker.color1.d3dColor=0xffffffff;}else normal=true;break;
        }
        if(normal){const u8 alpha=distance<64?u8((number(distance)*number(64)/number(64)+number(96)).truncate_int()):160;marker.color1.d3dColor=0xffffff|(u32(alpha)<<24);}
        renderer.draw_no_rotation(marker);
    }
}
void AsciiManager::draw_percentage(const Vec3& position,i32 percentage,u32 color){
    auto& vm=state.percentage;const u32 absolute=percentage<0?0u-u32(percentage):u32(percentage);
    const i32 count=4+(percentage<0)+(absolute>=10000?3:absolute>=1000?2:1);
    const float offset=(integer(count)*number(3.5f)-number(3.5f)-number(4)).to_float();
    vm.pos=position;vm.pos.x=sub(vm.pos.x,offset);vm.color1.d3dColor=color;
    auto digit=[&](i32 sprite,float advance){set_sprite(vm,sprite);renderer.draw_no_rotation(vm);vm.pos.x=add(vm.pos.x,advance);};
    if(percentage<0)digit(148,7);
    u32 value=absolute;
    if(absolute>=10000){digit(137,7);digit(136,7);digit(136,7);value=0;}
    else {if(absolute>=1000){digit(136+value/1000,7);value%=1000;}digit(136+value/100,7);value%=100;}
    digit(147,5);vm.scale={.8f,.8f};vm.pos.y=add(vm.pos.y,2);
    digit(136+value/10,5);digit(136+value%10,7);vm.scale={1,1};vm.pos.y=sub(vm.pos.y,2);set_sprite(vm,146);renderer.draw_no_rotation(vm);
}
void AsciiManager::draw_overlays(const AsciiContext& c){
    auto& s=state;overlay.begin(!c.fog_disabled);
    auto& small=s.small_score_text;
    for(const auto& p:s.score_popups)if(p.in_use){small.pos.x=(number(p.position.x)-integer(p.characters*4)).to_float();small.pos.y=p.position.y;small.color1.d3dColor=p.color;small.scale={s.scale_x,s.scale_y};
        const i32 alpha=popup_alpha(c.player,p.position);for(i32 i=p.characters-1;i>=0;--i){direct_sprite(small,p.text[i]+(p.timer.current<52?0:p.timer.current<56?11:21));small.color1.a=u8(alpha);
            if(small.loadedSprite)small.spriteSize.x=small.loadedSprite->widthPx;renderer.draw_no_rotation(small);small.pos.x=add(small.pos.x,8);}}
    if(s.blindness_color){
        const u32 color=(s.blindness_color&255)<<24;
        const auto x=number(c.player.x)+number(32),y=number(c.player.y)+number(16),radius=number(s.blindness_radius);
        OverlayRect rect{32,16,(x-radius+number(renderer.shake.x)).to_float(),464};
        if(rect.right>rect.left)overlay.rectangle(rect,color);
        rect={(x+radius+number(renderer.shake.x)).to_float(),16,416,464};if(rect.right>rect.left)overlay.rectangle(rect,color);
        rect={std::max(32.f,(x-radius+number(renderer.shake.x)).to_float()),16,std::min(416.f,(x+radius+number(renderer.shake.x)).to_float()),(y-radius+number(renderer.shake.y)).to_float()};
        if(rect.bottom>rect.top)overlay.rectangle(rect,color);
        rect.top=(y+radius+number(renderer.shake.y)).to_float();rect.bottom=464;if(rect.bottom>rect.top)overlay.rectangle(rect,color);
        if(c.effects){start(s.blindness,*c.effects,105);s.blindness.scale.x=s.blindness.scale.y=(radius/number(63)).to_float();s.blindness.pos=c.player;s.blindness.pos.x=add(s.blindness.pos.x,32);s.blindness.pos.y=add(s.blindness.pos.y,16);s.blindness.color1.a=u8(s.blindness_color);renderer.draw_no_rotation(s.blindness);}
    }
    auto& popup=s.popup_text;
    for(const auto& p:s.time_popups)if(p.in_use){popup.pos.x=(number(p.position.x)-integer(p.characters)*number(3.5f)).to_float();popup.pos.y=p.position.y;popup.color1.d3dColor=p.color;popup.scale={p.scale_x,p.scale_y};
        const i32 alpha=popup_alpha(c.player,p.position);for(i32 i=p.characters-1;i>=0;--i){direct_sprite(popup,p.text[i]+136);popup.color1.a=u8(alpha);if(popup.loadedSprite)popup.spriteSize.x=popup.loadedSprite->widthPx;
            renderer.draw_no_rotation(popup);popup.pos.x=(number(popup.pos.x)+number(7)*number(p.scale_x)).to_float();}}
    renderer.shake={};
    if(s.gauge.visible){
        s.cursor.pos.x=(integer(c.gauge)*number(112)/number(2)/number(10000)+number(s.gauge.pos.x)+number(64)).to_float();renderer.draw_2d(s.cursor,true);
        s.percentage.pos.x=(integer(c.gauge)*number(80)/number(2)/number(10000)+number(s.gauge.pos.x)+number(64)).to_float();s.percentage.pos.y=sub(s.cursor.pos.y,7);s.percentage.pos.z=s.cursor.pos.z;
        const u32 rgb=c.gauge<=c.human_effects?0x7070ff:c.gauge<=c.human_tint?0xb0b0ff:c.gauge>=c.youkai_effects?0xff7070:c.gauge>=c.youkai_tint?0xffb0b0:0xffffff;
        s.percentage.color1.d3dColor=(s.gauge.color1.d3dColor&0xff000000)|rgb;s.gauge.color1=s.percentage.color1;
        renderer.draw_no_rotation(s.gauge);renderer.draw_no_rotation(s.human_icon);renderer.draw_no_rotation(s.youkai_icon);draw_percentage(s.percentage.pos,c.gauge,s.percentage.color1.d3dColor);
        s.percentage.pos.x=(number(s.gauge.pos.x)+number(62)-number(14)).to_float();s.percentage.pos.y=(number(s.gauge.pos.y)+number(3)+number(8)).to_float();
        i32 divisor=10000000,value=c.point_value,seen=0;
        for(i32 i=0;i<8;++i){seen+=value/divisor;if(seen){set_sprite(s.percentage,value/divisor+136);renderer.draw_no_rotation(s.percentage);s.percentage.pos.x=add(s.percentage.pos.x,7);}value%=divisor;divisor/=10;}
    }
}
}
