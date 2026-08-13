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

    i32 scoreData;                 // 0x0  ScoreDat handle for the current score table
    i32 screenTimer;               // 0x4  per-screen frame counter (gates input after 6 frames)
    i32 screenMode;                // 0x8  current screen state
    i32 stateCopy;                 // 0xc  copy of the state set by SetState
    i32 subState;                  // 0x10  per-screen sub-state (switch case in the handlers)
    i32 previousScreenMode;        // 0x14  previous screen mode (saved by SetState)
    i32 subStateTimer;             // 0x18  frame timer within the sub-state
    i32 cursor;                    // 0x1c  menu cursor position
    unknown_fields(0x20, 0x10);    // 0x20
    i32 shotTypeCursor;            // 0x30  shot-type cursor
    i32 previousShotTypeCursor;    // 0x34  last committed shotTypeCursor
    i32 shotCursorMoved;           // 0x38  set to 1 when the shot cursor moved
    i32 savedCharacterCursor;      // 0x3c  saved character selection (high score screen)
    i32 savedSpellPageCursor;      // 0x40  saved spell-card page cursor
    i32 savedDifficultyCursor;     // 0x44  saved difficulty selection (high score screen)
    i32 savedSpellDifficultyCursor; // 0x48  saved spell-card difficulty selection
    i32 cheatCodeProgress;         // 0x4c  hidden "clear all scores" button-sequence progress
    unknown_fields(0x50, 0x4);
    i32 backPressed;               // 0x54  set while the back button is held
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
