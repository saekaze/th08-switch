#pragma once
#include "EffectTransforms.hpp"
#include "EffectSpace.hpp"
#include "EffectGeometry.hpp"
#include "EffectBomb.hpp"
#include "AnmExecutor.hpp"
#include "GameValues.hpp"
namespace th08 {
struct EffectPoolState {
    i32 cursor;u32 reserved4;i32 active_count;u32 reserved0c[4];
    EffectState objects[654],sentinels[5];EffectState* tails[5];i32 frames;AnmLoaded* base_animation;AnmLoaded* stage_animation;
    EffectPoolState(){std::memset(this,0,sizeof(*this));}
};
TH_LAYOUT_ASSERT(sizeof(EffectPoolState)==0x8b05c&&offsetof(EffectPoolState,objects)==0x1c);
TH_LAYOUT_ASSERT(offsetof(EffectPoolState,sentinels)==0x89f5c&&offsetof(EffectPoolState,frames)==0x8b050);
struct EffectDefinition {i32 script;EffectUpdate update,initialize;};
class EffectSystem {
    AnmExecutor& anm;AnmRenderer& renderer;GameValues& values;u16& replay_flags;
    void begin(EffectState&,i32 kind,u32 color,bool depth);
    void initialize(EffectState&,i32 kind);
    void draw_list(u32 index,float depth,bool offset_before_depth);
    static void projected(AnmVm&,Vec3&,void*);
public:
    EffectPoolState& state;EffectEnvironment& environment;Vec2 arcade{32,16};bool paused=false,invalid=false;u8 quality=2;
    EffectTransforms transforms;EffectSpace space;EffectGeometry geometry;EffectBomb bomb;
    EffectSystem(EffectPoolState&,EffectEnvironment&,AnmExecutor&,AnmRenderer&,Rng&,ScreenEffects&,DamageRegions&,GameValues&,u16& replay_flags);
    ~EffectSystem(){release();}
    static const EffectDefinition& definition(u32 kind);
    void reset();void release();
    EffectState* spawn(i32 kind,Vec3 position,i32 count,u32 color,const Vec3* parameters=nullptr);
    EffectState* fixed(i32 kind,Vec3 position,i32 slot,u32 color,const Vec3* parameters=nullptr);
    EffectState* overlay(i32 kind,Vec3 position,i32 count,u32 color);
    EffectState* group(i32 slot){return slot>=0&&slot<13?&state.objects[640+slot]:nullptr;}
    void shift_glows(const Vec3& offset);
    JobResult update();JobResult draw();JobResult draw_alternative();JobResult draw_background();
};
}
