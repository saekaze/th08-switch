#pragma once
#include "AsciiManager.hpp"
#include "GameValues.hpp"
namespace th08 {
struct MenuContext {
    u32 flags=0;
    i32 difficulty=0,stage=0,character=0,spell=0;
    u16 keys=0,previous_keys=0;
    u8 pause_state=0,show_retry=0;
    bool lockable_backbuffer=false,spell_captured=false;
    i32 shot_bombs=2,supervisor_state=2;
    u32 system_time=0;
    i32 timing_level=0;
    Vec2 arcade_origin{32,16},arcade_size{384,448};
    AnmLoaded* times=nullptr;
    bool replay()const{return flags&8;}
    bool practice()const{return flags&1;}
    bool spell_practice()const{return flags&16384;}
    bool pressed(u16 mask)const{return (keys&mask)&&(keys&mask)!=(previous_keys&mask);}
};
struct TextureCaptureRequest {i32 index,source_x,source_y,source_width,source_height,dest_x,dest_y,dest_width,dest_height;};
enum class MenuMusic {Pause,Resume,Stop,FadeIn,PartialFadeOut,PartialFadeIn};
struct MenuActions {
    virtual ~MenuActions()=default;
    virtual void sound(i32 index)=0;
    virtual void music(MenuMusic action,float seconds=0)=0;
    virtual bool capture(const TextureCaptureRequest& request)=0;
    virtual void capture_arcade()=0;
    virtual void save_score()=0;
    virtual u32 now()=0;
    virtual void update_game_time()=0;
};
class UiMenus {
public:
    MenuContext context;
    UiMenus(AsciiState& ascii,AnmExecutor& executor,AnmRenderer& renderer,GameGlobals& globals,GameValues& values,
            GameConfiguration& config,PlayRecord& statistics,MenuActions& actions)
       :ascii(ascii),executor(executor),renderer(renderer),globals(globals),values(values),config(config),statistics(statistics),actions(actions){}
    i32 update_pause();
    i32 update_retry();
    void draw_pause();
    void draw_retry();
private:
    AsciiState& ascii;
    AnmExecutor& executor;
    AnmRenderer& renderer;
    GameGlobals& globals;
    GameValues& values;
    GameConfiguration& config;
    PlayRecord& statistics;
    MenuActions& actions;
    void start(AnmVm& vm,AnmLoaded* file,i32 script);
    bool capture(AnmVm& vm);
    void viewport();
    void continue_game();
};
}
