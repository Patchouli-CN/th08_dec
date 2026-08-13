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
    RESULT_SCREEN_CATEGORY_INIT = 0,
    RESULT_SCREEN_CATEGORY,
    RESULT_SCREEN_LOADING,      // 2: plays loading vms, then exits to 9 after 20 frames
    RESULT_SCREEN_HIGH_SCORE_DIFFICULTY,  // 3
    RESULT_SCREEN_HIGH_SCORE_CHARACTER,   // 4
    RESULT_SCREEN_HIGH_SCORE,             // 5
    RESULT_SCREEN_SPELL_DIFFICULTY,       // 6
    RESULT_SCREEN_SPELL_CHARACTER,        // 7
    RESULT_SCREEN_SPELL,                  // 8
    RESULT_SCREEN_RESULT,                 // 9: result / name entry
    RESULT_SCREEN_REPLAY_SAVE1,           // 10
    RESULT_SCREEN_REPLAY_SAVE2,           // 11
    RESULT_SCREEN_REPLAY_SAVE3,           // 12
    RESULT_SCREEN_REPLAY_SAVE4,           // 13
    RESULT_SCREEN_REPLAY_SAVE5,           // 14
    RESULT_SCREEN_CONFIRM,                // 15
    RESULT_SCREEN_CONFIRM2,               // 16
    RESULT_SCREEN_PRACTICE_RESULT,        // 17
    RESULT_SCREEN_ONE_SHOT,               // 18
    RESULT_SCREEN_OTHER_STATS1,           // 19
    RESULT_SCREEN_OTHER_STATS2,           // 20
    RESULT_SCREEN_OTHER_STATS3,           // 21
    RESULT_SCREEN_SPELL_PRACTICE_RESULT,  // 22
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
    i32 LinkScoreEx(void *out, int difficulty, i32 character);
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
    unknown_fields(0x20, 0xc);    // 0x20
    i32 nameCursor;                // 0x2c  name-entry cursor position (index into 0x4c7f48 alphabet)
    i32 shotTypeCursor;            // 0x30  shot-type cursor
    i32 previousShotTypeCursor;    // 0x34  last committed shotTypeCursor
    i32 shotCursorMoved;           // 0x38  set to 1 when the shot cursor moved
    i32 savedCharacterCursor;      // 0x3c  saved character selection (high score screen)
    i32 savedSpellPageCursor;      // 0x40  saved spell-card page cursor
    i32 savedDifficultyCursor;     // 0x44  saved difficulty selection (high score screen)
    i32 savedSpellDifficultyCursor; // 0x48  saved spell-card difficulty selection
    i32 cheatCodeProgress;         // 0x4c  hidden "clear all scores" button-sequence progress
    i32 nameEntryFlag;             // 0x50  non-zero while a new high score is being named
    i32 backPressed;               // 0x54  set while the back button is held
    char nameBuffer[0xc];          // 0x58  name being typed
    i32 scoreCounts[6][13];        // 0x64  per (difficulty, shot type) high-score counts
    u8 lastTotalSeconds;           // 0x19c  last-seen total-seconds (stats screen change detection)
    unknown_fields(0x19d, 0x3);
    AnmVm vms[0x48];               // 0x1a0 (0x2a4 each, ends 0xbfc0; vms[0x28]=menu highlight, vms[0x47]=menu timer)
    AnmVm scoreVms[0xa];           // 0xbfc0 前十得分文本 VM
    AnmVm textVm;                  // 0xda28 主文本 VM
    unknown_fields(0xdccc, 0x3780);
    ScoreListNode scores[5][12];   // 0x1144c
    Hscr hscrCache[5][12][10];       // 0x1171c..0x462dc  default high-score entries (added by AddedCallback)
    Hscr hscr;                       // 0x462dc  score being entered by the name-entry screen
    Th8k saveHeader;                 // 0x46444  "TH48" score-file header written by WriteScore
    char saveNameData[0x18];         // 0x46450  extra save block (LSNM-like; default name at +0xc)
    ChainElem *calcChain;          // 0x46468
    ChainElem *drawChain; // 0x4646c
    unknown_fields(0x46470, 0x1340);
};
C_ASSERT(sizeof(ResultScreen) == 0x477b0);
}; // namespace th08
