#include "LoadingScreen.hpp"
namespace th08 {
bool LoadingScreen::show(const Vec3& position,bool capture){
    if(!phase){for(i32 i=0;i<3;i++)if(!library.start(2,i,vms[i],animations))return false;phase=1;for(auto& vm:vms)vm.pos=position;}
    if(capture)actions.capture_screen();return true;
}
void LoadingScreen::fade(){if(phase==1){for(auto& vm:vms)vm.pendingInterrupt=1;phase=0;}}
void LoadingScreen::hide(){if(phase==1){for(auto& vm:vms)vm.pendingInterrupt=2;phase=2;}}
void LoadingScreen::update(){for(auto& vm:vms)animations.execute(vm);}
void LoadingScreen::draw(){
    for(auto& vm:vms){vm.pos={Scalar::add(vm.pos.x,vm.pos2.x),Scalar::add(vm.pos.y,vm.pos2.y),Scalar::add(vm.pos.z,vm.pos2.z)};renderer.draw_2d(vm);vm.pos={Scalar::sub(vm.pos.x,vm.pos2.x),Scalar::sub(vm.pos.y,vm.pos2.y),Scalar::sub(vm.pos.z,vm.pos2.z)};}
}
void LoadingScreen::background(){
    if(phase>=2){phase=wrapping_add(phase,1);if(phase>=5){ascii.state.scale_x=ascii.state.scale_y=.5f;const i32 alpha=255-(phase<35?(phase-5)*128/30:(65-phase)*128/30);ascii.state.color=(ascii.state.color&0xffffff)|(u32(u8(alpha))<<24);ascii.add_string({288,454,0},"Press Shot Button",software_texturing);ascii.state.scale_x=ascii.state.scale_y=1;actions.draw_text();ascii.state.string_count=0;if(phase>=65)phase=5;}}
    if(phase)actions.draw_capture();else if(actions.has_capture())actions.release_capture();
}
}
