#pragma once

#include "Global.hpp"
#include "ScoreDat.hpp"
#include "ZunResult.hpp"
#include "inttypes.hpp"
#include "utils.hpp"

namespace th08
{

enum ResultScreenState
{
};

struct ResultScreen
{
    ResultScreen()
    {
        memset(this, 0, sizeof(ResultScreen));
    }

    ~ResultScreen()
    {
    }

    static const char *GetStageName(i32 stage);
    static const char *GetCharacterName(i32 character);
    void WriteScore();
    static void LogScoreDataToFile(ResultScreen *resultScreen);
    void LinkScoreEx(void *out, int difficulty, i32 character);
    void FreeScore(i32 difficulty, i32 character);
    i32 HandleCategorySelectScreen();
    void SetState(ResultScreenState state);

    i32 HandleHighScoreDifficultySelect();
    i32 HandleHighScoreCharacterSelect();
    i32 HandleHighScoreScreen();
    i32 HandleSpellCardDifficultySelect();
    i32 HandleSpellCardCharacterSelect();
    i32 HandleSpellCardScreen();
    i32 HandleResultKeyboard();

    static void FormatDate(char *buffer);

    i32 HandleReplaySaveKeyboard();
    ZunResult CheckConfirmButton();
    i32 HandleOtherStatsScreen();
    i32 DrawFinalStats();

    static ZunResult RegisterChain(u32 unk);
    static ChainCallbackResult OnUpdate(ResultScreen *resultScreen);
    static ChainCallbackResult OnDraw(ResultScreen *resultScreen);
    static ZunResult AddedCallback(ResultScreen *resultScreen);
    static ZunResult __fastcall DeletedCallback(ResultScreen *resultScreen);

    i32 __fastcall MoveCursor(i32 length);
    i32 __fastcall MoveShotTypeCursor(i32 length);
    i32 __fastcall MoveCursorHorizontally(i32 length);

    i32 unk0;                      // 0x0
    i32 unk4;                      // 0x4
    i32 screenMode;                // 0x8
    i32 unk0c;                     // 0xc
    i32 unk10;                     // 0x10
    i32 unk14;                     // 0x14
    i32 unk18;                     // 0x18
    i32 unk1c;                     // 0x1c (cursor position)
    unknown_fields(0x20, 0x10);    // 0x20
    i32 unk30;                     // 0x30 (shot type cursor)
    i32 unk34;                     // 0x34
    i32 unk38;                     // 0x38
    i32 unk3c;                     // 0x3c
    i32 unk40;                     // 0x40
    i32 unk44;                     // 0x44
    i32 unk48;                     // 0x48
    i32 unk4c;                     // 0x4c
    unknown_fields(0x50, 0x4);
    i32 unk54;                     // 0x54
    unknown_fields(0x58, 0x148);
    AnmVm vms[0x48];               // 0x1a0 (0x2a4 each, ends 0xbfc0; vms[0x28]=menu highlight, vms[0x47]=menu timer)
    AnmVm scoreVms[0xa];           // 0xbfc0 前十得分文本 VM
    AnmVm textVm;                  // 0xda28 主文本 VM
    unknown_fields(0xdccc, 0x3780);
    ScoreListNode scores[5][12];   // 0x1144c
    unknown_fields(0x1171c, 0x34d4c); // 0x1171c
    ChainElem *calcChain;          // 0x46468
    ChainElem *drawChain; // 0x4646c
    unknown_fields(0x46470, 0x1340);
};
C_ASSERT(sizeof(ResultScreen) == 0x477b0);
}; // namespace th08
