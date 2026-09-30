#include "PlayerSimulation.hpp"
namespace th08 {
PlayerSimulation::PlayerSimulation(PlayerSimulationState& s,ShotResource (&r)[2],GameGlobals& v,GameGauge& g,GaugeThresholds& t,GameRank& rank,Rng& random,PlayerSimulationServices a)
    :state(s),resources(r),gauge(g),thresholds(t),services(a),values(v),rank(rank),life(s.life,s.context,s.motion.movement,s.motion.animation,a.life),shots(s.shots,random),patterns(s.bomb_objects,s.bomb,s.life,s.context,s.motion.movement,s.bomb_input,s.shots.regions,random,a.patterns),collisions(s.motion.movement,s.life,s.context,s.shots.regions,s.cancel_item,*this){
    shots.actions=&services.shots;patterns.frame.options=s.motion.options;patterns.frame.main_animation=&s.motion.animation;
}
bool PlayerSimulation::initialize(const PlayerSetupContext& context){
    failed=!initialize_player(state.motion,state.life,state.bomb,state.shots,thresholds,resources[0].settings(),context,services.setup);
    initialized=!failed;if(initialized){state.context.character=state.input.character=context.character;state.context.extent=context.extent;synchronize_shots();}return initialized;
}
void PlayerSimulation::synchronize_shots(){
    auto& s=state.shots;s.position=state.motion.movement.position;for(u32 i=0;i<4;++i)s.options[i]=state.motion.options[i].position;
    s.homing_target=state.frame.homing_target;s.target=state.frame.shot_target;s.enemy_target=state.enemy_origin;s.enemy_available=state.input.enemy_present;s.orbit_angle=state.motion.options[2].shot_angle;
    s.bomb=state.bomb.active;s.game_flags=state.context.game_flags;s.focused=state.motion.form.focused;s.youkai_bonus=gauge.youkai_bonus();s.character=state.context.character;
    s.player_state=state.life.state;s.gui_blocked=state.input.gui_blocked;s.option_active=state.motion.options[0].state!=0;s.collision_timer=state.life.timer;s.time_spell=state.context.time_spell;s.human_bonus=gauge.human_bonus();shots.timing=timing;
}
bool PlayerSimulation::update(){
    if(!initialized)return false;failed=false;state.context.focused=state.motion.form.focused;state.context.gauge=gauge.value();state.bomb_input.buttons=state.input.buttons;state.bomb_input.gui_blocked=state.input.gui_blocked;state.bomb_input.tampered=state.input.tampered;
    synchronize_shots();update_player_frame(state.frame,state.motion,state.life,state.shots.regions,gauge,state.context.pause!=0,*this);synchronize_shots();return !failed;
}
void PlayerSimulation::update_bomb(){failed|=!update_player_bomb(state.bomb,state.bomb_input,state.life,state.context,state.motion.movement,state.motion.animation,resources[0].settings(),timing,*this);}
void PlayerSimulation::update(PlayerBombKind kind){patterns.frame.timing=timing;patterns.frame.homing_target=state.frame.homing_target;patterns.frame.shooting_timer=state.shots.shooting_timer;failed|=!patterns.update(kind);}
bool PlayerSimulation::resolve_death(){return life.resolve_death(resources[0].settings());}
void PlayerSimulation::respawn(){life.respawn(resources[0].settings());}
void PlayerSimulation::update_invincibility(){life.update_invincibility(timing);}
void PlayerSimulation::update_motion(){
    state.input.bomb=state.bomb.active;state.input.bomb_type=state.bomb.type;state.input.character=state.context.character;
    update_player_motion(state.motion,state.input,state.shots.shooting_timer,gauge,resources[0].settings(),resources[1].settings(),timing,services.motion);state.context.focused=state.motion.form.focused;
    if(!state.input.enemy_present)state.frame.target_reference=nullptr;
}
void PlayerSimulation::step_animation(AnmVm& vm){services.shots.step_animation(vm);}
void PlayerSimulation::update_shots(){synchronize_shots();failed|=!shots.update();}
void PlayerSimulation::update_shooting(){
    const ShotFiringInputs input{state.stage_play_frames,state.bomb.active,state.bomb.type,state.input.buttons,state.context.character,state.input.gui_blocked,state.life.state,{}};
    th08::update_shooting(state.shots.shooting_timer,input,timing,*this);
}
void PlayerSimulation::fire(i32 frame){
    synchronize_shots();const auto& resource=resources[state.motion.form.focused!=0];const i32 index=resource.select(state.context.power,state.context.character,state.bomb.active,state.bomb.type,state.bomb.timer.current>=60);
    const auto* stream=index<0?nullptr:resource.stream(index);if(!stream){failed=true;return;}shots.emit(*stream,frame);failed|=shots.failure!=PlayerShots::Failure::None;
}
void PlayerSimulation::die(){state.context.focused=state.motion.form.focused;state.context.gauge=gauge.value();life.die();gauge.set(state.context.gauge);synchronize_shots();}
void PlayerSimulation::graze(const Vec3& position,bool laser){
    PlayerGrazeContext context{state.motion.movement.position,state.bomb.active,state.context.hud_flags,state.context.replay_flags,state.context.character,state.motion.form.youkai,state.context.time_spell,u8(services.world.boss_present()),{}};
    graze_player(context,values,gauge,rank,position,laser,*this);state.context.hud_flags=context.hud_flags;state.context.replay_flags=context.replay_flags;state.context.gauge=gauge.value();
}
i32 PlayerSimulation::damage(const Vec3& position,const Vec3& size,i32& time_items,i32* bomb_hit){synchronize_shots();const i32 result=shots.damage(position,size,time_items,bomb_hit);failed|=shots.failure!=PlayerShots::Failure::None;return result;}
bool PlayerSimulation::draw(const Vec2& offset,bool impacts){
    synchronize_shots();shots.draw(impacts,offset);failed|=shots.failure!=PlayerShots::Failure::None;if(!impacts){if(state.bomb.active)failed|=!patterns.draw(player_bomb_kind(state.context.character,state.bomb.type),offset);draw_player_motion(state.motion,offset,state.context.game_over,services.motion);}return !failed;
}
}
