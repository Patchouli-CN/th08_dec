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
    void HandleCategorySelectScreen();
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
    unknown_fields(0x34, 0x20);    // 0x34
    i32 unk54;                     // 0x54
    unknown_fields(0x58, 0x113f4); // 0x58
    ScoreListNode scores[5][12];   // 0x1144c
    unknown_fields(0x1171c, 0x34d4c); // 0x1171c
    ChainElem *calcChain;          // 0x46468
    ChainElem *drawChain; // 0x4646c
    unknown_fields(0x46470, 0x1340);
};
C_ASSERT(sizeof(ResultScreen) == 0x477b0);
}; // namespace th08
