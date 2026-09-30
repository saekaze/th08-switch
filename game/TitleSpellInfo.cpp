// Spell information's delayed line updates recovered from TH08, including
// separate normal/Last Word statistics and the original hint table indexing.
#include "TitleMenus.hpp"
#include "TitleSpellText.hpp"
#include <cstdio>
#include <cstdarg>
namespace th08 {
void TitleMenus::DrawTextFormatted(AnmVm* vm,const char* format,...){char buffer[512];va_list args;va_start(args,format);std::vsnprintf(buffer,sizeof(buffer),format,args);va_end(args);text.draw(*vm,TextAlignment::Left,0xffffff,0,buffer);}
void TitleMenus::FormatSpellCardInfo(){
    if(state.currentScreenState==1&&!state.unk0xc29c)return;
    if(context.currentStage<0||context.currentStage>=10||state.cursor<0||state.cursor>=spell_stage_counts[context.currentStage])return;
    const i32 number=spells_by_stage[context.currentStage][state.cursor],difficulty=spell_difficulty(number),shot=context.character;
    if(shot<0||shot>=12)return;
    auto& record=context.spells[number];const u32 attempts=record.practice.attempts[12]+record.game.attempts[12];
    auto* info=state.spellCardInfoVms;const auto line=[&](i32 delay){return state.currentScreenState==0||state.unk0xc29c==delay;};
    char name[49]{},owner[49]{};std::memcpy(name,record.name,48);std::memcpy(owner,record.owner,48);
    if(line(11)){
        char digits[7]{};i32 value=number+1;for(i32 i=2;i>=0;--i){std::memcpy(digits+i*2,spell_info_digits[value%10],2);value/=10;}
        DrawTextFormatted(info,spell_info_title,digits,attempts?name:spell_info_unknown);
    }
    if(line(9))DrawTextFormatted(info+1,spell_info_owner,attempts?owner:spell_info_unknown,spell_info_difficulties[difficulty],is_last_spell(number)?spell_info_last_spell:spell_info_empty);
    if(state.currentScreenState==0)DrawTextFormatted(info+2,spell_info_character,spell_info_characters[shot]);
    const bool encountered=context.HasSpellCardBeenEncountered(number,12);
    if(line(7)){
        if(!encountered)DrawTextLeft(info+3,0xffffff,0,spell_info_unknown_stats);
        else if(difficulty<=4)DrawTextFormatted(info+3,spell_info_normal_stats,
            record.practice.captures[shot],record.practice.attempts[shot],record.game.captures[shot],record.game.attempts[shot],record.practice.max_bonus[shot],
            record.practice.captures[12],record.practice.attempts[12],record.game.captures[12],record.game.attempts[12],record.practice.max_bonus[12]);
        else DrawTextFormatted(info+3,spell_info_last_word_stats,record.practice.captures[shot],record.practice.attempts[shot],record.practice.max_bonus[shot],record.practice.captures[12],record.practice.attempts[12],record.practice.max_bonus[12]);
    }
    if(state.currentScreenState==0)DrawTextLeft(info+4,0xffffff,0,spell_info_comments);
    for(i32 i=0;i<2;++i)if(line(i?3:5)){
        if(encountered||number<204||number>221||context.IsLastWordSpellCardAttempted(number)){
            char comment[128]{};std::memcpy(comment,i?record.comment2:record.comment1,64);
            DrawTextLeft(info+5+i,0xffffff,0,record.practice.captures[12]?comment:spell_info_locked_comment);
        }else{
            const auto& hint=spell_unlock_hints[number-204][i];DrawTextFormatted(info+5+i,hint.format,hint.arguments[0],hint.arguments[1],hint.arguments[2],hint.arguments[3],hint.arguments[4]);
        }
    }
    info[5].pos.x=info[6].pos.x=96;
    if(line(3))for(i32 i=0;i<7;++i)info[i].color1.a=255;
    if(state.unk0xc29c)state.unk0xc29c=wrapping_sub(state.unk0xc29c,1);
}
}
