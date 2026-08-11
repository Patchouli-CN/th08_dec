#pragma once

#include "ScoreDat.hpp"
#include "Spellcard.hpp"
#include "Supervisor.hpp"
#include "ZunMath.hpp"
#include "ZunResult.hpp"
#include "diffbuild.hpp"
#include "inttypes.hpp"
#include "utils.hpp"
#include <stddef.h>
#include <windows.h>

#define EXTRA_UNLOCKED_FLAG ZUN_BIT(14)
#define SPELL_PRACTICE_UNLOCKED_FLAG ZUN_BIT(15)

// ---- GameManager::OnUpdate magic constants ----
#define PAUSE_KEY_MASK 0x1001           // pause + up buttons held together
#define FOCUS_KEY_BIT 0x8               // focus (slow) button
#define ANY_INPUT_MASK 0xffff           // any-button mask used by the demo check

// g_PlayerFlags (0x164d0b4) bit positions.
#define PLAYER_FLAG_SWAP_MODE_BIT 3     // both characters active (swap mode)
#define PLAYER_FLAG_FORCE_SWITCH_BIT 4  // force a character switch
#define PLAYER_FLAG_EXTRA_BIT 14        // extra stage unlocked

// g_Unknown164d2cc special event states (behave like an in-progress event).
#define GAME_STATE_EVENT_6 6
#define GAME_STATE_EVENT_7 7
#define GAME_STATE_EVENT_8 8

#define STAGE_PROGRESS_MAX 9            // number of stage-progress slots
#define CLOCK_TIME_12H_LIMIT 0xc        // clock reaches 12h: game can no longer continue
#define EXTRA_STAGE_CLEARED_FLAG 0x8000 // high bit marking the extra stage as cleared

// Anti-tamper register validity bounds (6543..106543).
#define ANTITAMPER_RANGE_MIN 6543
#define ANTITAMPER_RANGE_MAX 106543
#define ANTITAMPER_RANGE_MIN_FLOAT 6543.0f
#define ANTITAMPER_RANGE_MAX_FLOAT 106543.0f

// Displayed-score animation caps.
#define SCORE_DISPLAY_CAP 1000000000
#define SCORE_DISPLAY_CAP_ONE_LESS 999999999
#define SCORE_INCREMENT_CAP 0x8d55e

// Global-slowdown bullet-count thresholds.
#define SLOW_BULLET_COUNT_MAX 0x140
#define SLOW_BULLET_COUNT_MID 0xe0
#define SLOW_BULLET_COUNT_MIN 0x80

// Demo auto-play frame thresholds, indexed by currentDemoReplay (0-3).
#define DEMO_BOMB_FRAME_0 0x1770
#define DEMO_BOMB_FRAME_1 0x12c0
#define DEMO_BOMB_FRAME_2 0x1338
#define DEMO_BOMB_FRAME_3 0x1af4
#define DEMO_END_FRAME_0 0x17e8
#define DEMO_END_FRAME_1 0x1338
#define DEMO_END_FRAME_2 0x13b0
#define DEMO_END_FRAME_3 0x1b6c

#define IS_STAGE_CLEARED(difficulty, stage) (difficulty & ZUN_BIT(stage))

#define ANTITAMPER_RNG_RANGE 100000
#define ANTITAMPER_RNG_ADD 6543

namespace th08
{

struct GameManagerFlags
{
    u32 isPracticeMode : 1;
    u32 isDemoMode : 1;
    u32 unk2 : 1;
    u32 isReplay : 1;
    u32 unk4 : 1;
    u32 unk5 : 2;
    u32 unk7 : 2;
    u32 unk9 : 1;
    u32 unk10 : 1;
    u32 isGoingToFinalB : 2; // why 2 bits?
    u32 unk13 : 1;
    u32 isSpellPractice : 1;

    u32 isExtraUnlocked : 1;
    u32 isSpellPracticeUnlocked : 1;
    u32 isExtraUnlockedWithAllTeams : 1;
};

enum
{
    REPLAY_MODE_NORMAL,
    REPLAY_MODE_SLOWDOWN,
    REPLAY_MODE_BOSS,
};

struct GameManager
{
    GameManager();

    ZunBool IsWithinPlayfield(f32 x, f32 y, f32 width, f32 height);
    i32 CalcAntiTamperChecksum();
    static i32 CalcChecksum(u8 *address, i32 size);
    void CollectExtend();

    static ChainCallbackResult OnUpdate(GameManager *gameManager);
    static ChainCallbackResult OnDraw(GameManager *gameManager);

    static ZunResult RegisterChain();

    static ZunResult AddedCallback(GameManager *gameManager);
    static void GameplaySetupThread(void *param);

    void InitRankParams();

    static void InitializeAntiTamper();

    i32 GetTimeOrbs();
    i32 GetLastSpellTimeOrbThreshold();
    void SetClockTime(u8 clockTime);
    i32 GetDeaths();
    i32 GetBombsUsed();
    void AddToDeaths(i32 deaths);
    void AddToBombsUsed(i32 bombs);
    i32 ScaleIntBasedOnRank(i32 min, i32 max);
    f32 ScaleFloatBasedOnRank(f32 min, f32 max);
    i32 IsSoloHuman();
    i32 IsSoloYoukai();
    u32 GetFlag0();
    u32 GetFlag1();
    u32 GetFlag3();
    u32 GetFlag14();
    i32 GetYoukaiGauge();
    i32 FUN_00418130(i32 spellcardIdx);
    u32 FUN_00418180(i32 a, i32 b);
    i32 GaugeIsModeratelyHuman();
    void SetLives(i32 lives);
    void SetYoukaiGauge(u16 gauge);

    i32 GetLives()
    {
        return this->globals->livesRemaining;
    }

    i32 GetBombsRemaining()
    {
        return this->globals->bombsRemaining;
    }

    void UpdateAntiTamper()
    {
        this->globals->rng1[2] = g_Rng.GetRandomU32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
        this->globals->rng7[3] = g_Rng.GetRandomU32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
        this->globals->antiTamperValue = this->globals->rng1[2];
        this->globals->antiTamperChecksum = CalcAntiTamperChecksum();
        this->antiTamperExpectedValue = this->globals->antiTamperChecksum + this->globals->rng7[3];
    }

    void RandomizeAntiTamper()
    {
        this->globals->rng1[0] = g_Rng.GetRandomU32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
        this->globals->rng1[1] = g_Rng.GetRandomU32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
        this->globals->rng1[2] = g_Rng.GetRandomU32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
        this->globals->rng1[3] = g_Rng.GetRandomU32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
        this->globals->rng1[4] = g_Rng.GetRandomU32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
        this->globals->rng4[0] = g_Rng.GetRandomF32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
        this->globals->rng4[1] = g_Rng.GetRandomF32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
        this->globals->rng4[2] = g_Rng.GetRandomF32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
    }

    ZunBool IsTampered();

    static ZunResult DeletedCallback(GameManager *gameManager);

    static void CutChain();

    void IncreaseSubrank(int amount);
    void DecreaseSubrank(int amount);
    void AddToYoukaiGauge(i32 param_1, i32 param_2);
    void AddTimeOrbs(i32 orbs)
    {
        if (orbs >= 0 || this->globals->currentTimeOrbs >= -orbs)
        {
            this->globals->currentTimeOrbs += orbs;
            this->globals->totalTimeOrbs += orbs;
            this->hscr.numTimeOrbsCollected += orbs;
            this->UpdateAntiTamper();
            if (orbs > 0)
            {
                orbs += this->globals->totalTimeOrbs & 1;
                this->globals->pointItemValue += (orbs / 2) * 10;
            }
        }
        else
        {
            this->globals->currentTimeOrbs = 0;
        }
    }

    ZunBool IsPhantasmUnlocked();

    /* I know it's dumb but this is the only way to get it matching */
    void SetIsReplayWeird(ZunBool value)
    {
        ZunBool res = value;

        this->flags.isReplay = res;
    }

    ZunBool IsPracticeMode()
    {
        return this->flags.isPracticeMode;
    }

    ZunBool IsReplay()
    {
        return this->flags.isReplay;
    }

    ZunBool IsSpellPractice()
    {
        return this->flags.isSpellPractice;
    }

    ZunBool IsDemoMode()
    {
        return this->flags.isDemoMode;
    }

    i32 GaugeIsExtremelyYoukai();
    i32 GaugeIsModeratelyYoukai();

    ZunBool GaugeIsExtremelyHuman()
    {
        return this->globals->youkaiGauge <= this->youkaiGaugeHumanEffectsThreshold;
    }

    ZunBool IsStageClearedWithRetries(i32 stage, i32 character, i32 difficulty);
    ZunBool IsStageClearedWithoutRetries(i32 stage, i32 character, i32 difficulty);

    ZunBool IsExtraUnlockedForCharacter(i32 character)
    {
        return (character > SHOT_YOUMU_YUYUKO) ||
               (this->clrdData[character].difficultiesClearedWithoutRetries[EASY] & EXTRA_UNLOCKED_FLAG ||
                this->clrdData[character].difficultiesClearedWithoutRetries[NORMAL] & EXTRA_UNLOCKED_FLAG ||
                this->clrdData[character].difficultiesClearedWithoutRetries[HARD] & EXTRA_UNLOCKED_FLAG ||
                this->clrdData[character].difficultiesClearedWithoutRetries[LUNATIC] & EXTRA_UNLOCKED_FLAG);
    }

    ZunBool IsExtraUnlocked()
    {
        return this->IsExtraUnlockedForCharacter(SHOT_REIMU_YUKARI) ||
               this->IsExtraUnlockedForCharacter(SHOT_MARISA_ALICE) ||
               this->IsExtraUnlockedForCharacter(SHOT_SAKUYA_REMILIA) ||
               this->IsExtraUnlockedForCharacter(SHOT_YOUMU_YUYUKO);
    }

    i32 FinalBCleared(i32 difficulty);
    u32 FUN_0043c322();

    ZunBool IsSpellPracticeUnlockedForCharacter(i32 character)
    {
        return (character > SHOT_YOUMU_YUYUKO) ||
               (this->clrdData[character].difficultiesClearedWithRetries[EASY] & SPELL_PRACTICE_UNLOCKED_FLAG ||
                this->clrdData[character].difficultiesClearedWithRetries[NORMAL] & SPELL_PRACTICE_UNLOCKED_FLAG ||
                this->clrdData[character].difficultiesClearedWithRetries[HARD] & SPELL_PRACTICE_UNLOCKED_FLAG ||
                this->clrdData[character].difficultiesClearedWithRetries[LUNATIC] & SPELL_PRACTICE_UNLOCKED_FLAG);
    }

    ZunBool IsSpellPracticeUnlocked()
    {
        return this->IsSpellPracticeUnlockedForCharacter(SHOT_REIMU_YUKARI) ||
               this->IsSpellPracticeUnlockedForCharacter(SHOT_MARISA_ALICE) ||
               this->IsSpellPracticeUnlockedForCharacter(SHOT_SAKUYA_REMILIA) ||
               this->IsSpellPracticeUnlockedForCharacter(SHOT_YOUMU_YUYUKO);
    }

    ZunBool IsExtraUnlockedWithAllTeams()
    {
        return this->IsExtraUnlockedForCharacter(SHOT_REIMU_YUKARI) &&
               this->IsExtraUnlockedForCharacter(SHOT_MARISA_ALICE) &&
               this->IsExtraUnlockedForCharacter(SHOT_SAKUYA_REMILIA) &&
               this->IsExtraUnlockedForCharacter(SHOT_YOUMU_YUYUKO);
    }

    ZunBool HasSpellCardBeenEncountered(i32 spellCardNumber, i32 shotType)
    {
        Catk *catk = &this->catkData[spellCardNumber];

        return catk->inGameHistory.attempts[shotType] > 0 || catk->spellPracticeHistory.attempts[shotType] != 0;
    }

    ZunBool IsLastWordSpellCardAttempted(i32 spellCardNumber)
    {
        return spellCardNumber < SPELLCARD_LAST_WORD_START &&
                   (this->catkData[spellCardNumber].inGameHistory.attempts[SHOT_ALL] != 0 ||
                    this->catkData[spellCardNumber].spellPracticeHistory.attempts[SHOT_ALL] != 0) ||
               this->flsp.unlockedLastWordSpellCards[spellCardNumber - SPELLCARD_LAST_WORD_START] == spellCardNumber;
    }

    i32 GetPower();

    i32 GetClockTimeIncrement();
    i8 GetClockTime();
    void AddToClockTime(i8 value);
    static ZunResult LoadScoreData();
    static i32 GetSpellcardBgmIsLastWord(i32 spellcardNumber);
    void AdvanceToNextStage();

    void AddLives(int lives)
    {
        if (this->IsTampered())
        {
            CRASH_GAME();
        }
        this->globals->livesRemaining += lives;
        this->UpdateAntiTamper();
    }

    void AddPower(int power)
    {
        if (this->IsTampered())
        {
            CRASH_GAME();
        }
        this->globals->playerPower += power;
        this->UpdateAntiTamper();
    }

    void SetPower(i32 power)
    {
        this->globals->playerPower = (f32)power;
        this->UpdateAntiTamper();
    }

    void AddScore(i32 score)
    {
        this->globals->score += score / 10;
    }

    void AddToBombCount(int amount)
    {
        if (this->IsTampered())
        {
            CRASH_GAME();
        }
        this->globals->bombsRemaining += amount;
        this->UpdateAntiTamper();
    }

    void SetBombCount(i32 bombs)
    {
        this->globals->bombsRemaining = (f32)bombs;
        this->globals->antiTamperValue = this->globals->rng1[2];
        this->globals->antiTamperChecksum = this->CalcAntiTamperChecksum();
        this->antiTamperExpectedValue = (f32)(this->globals->antiTamperChecksum + this->globals->rng7[3]);
    }

    void InitArcadeRegionParams();

    ZunBool IsUnknown()
    {
        return this->unk2D;
    }

    void *unk0x0;
    GameConfiguration *cfg;
    ZunGlobals *globals;
    Flsp flsp;
    i8 unk2C;
    i8 unk2D;
    /* 2 bytes pad */
    i32 difficulty;
    i32 difficultyMask;
    u32 unk38;
    i32 unk3c;
    Catk catkData[SPELLCARD_COUNT_SPELLCARDS];
    Catk catkDataBackup[SPELLCARD_COUNT_SPELLCARDS];
    Clrd clrdData[SHOT_ALL + 1];
    Pscr pscrData[SHOT_ALL];
    Plst plst;
    Hscr hscr;
    i32 unk3D294;
    i32 unk3D298;
    i32 unk3D29C;
    i32 unk3D2A0;
    i32 unk3D2A4;
    u8 character;
    u8 shotType;
    u8 fullShotType;
    u8 unk3dbaa;
    GameManagerFlags flags;
    i16 currentSpellCardNumber;
    u8 isInGameMenu;
    u8 showRetryMenu;
    u8 currentDemoReplay;
    u8 replayMode;
    u8 unk3DBB6;
    u8 unk3DBB7;

    i32 demoFrameCount;
    char replayFilename[512];
    u16 stageRngSeed;
    u16 unk3ddbe;
    u32 unk3ddc0;
    i32 currentStage;
    i32 currentStage2;
    u32 unk3ddcc;
    u16 unk3DDD0;
    u16 unk3DDD2;
    Float2 arcadeRegionTopLeftPos;
    Float2 arcadeRegionSize;
    Float2 playerMovementTopLeftPos;
    Float2 playerMovementAreaSize;
    f32 antiTamperExpectedValue;
    i16 youkaiGaugeHumanLimit;
    i16 youkaiGaugeYoukaiLimit;
    i16 youkaiGaugeHumanEffectsThreshold;
    i16 youkaiGaugeYoukaiEffectsThreshold;
    i16 youkaiGaugeHumanTintThreshold;
    i16 youkaiGaugeYoukaiTintThreshold;

    u32 unk3de04;
    u32 unk3de08;
    u32 unk3de0c;
    u32 unk3de10;
    u32 unk3de14;
    u32 unk3de18;
    u32 unk3de1c;
    u32 unk3de20;
    u32 unk3de24;
    u32 unk3de28;

    i32 rank;
    i32 maxRank;
    i32 minRank;
    i32 subRank;
};

C_ASSERT(sizeof(GameManager) == 0x3de3c);

// Stage BGM table: one {stage BGM, boss BGM, unused} entry per game state
// (indexed by g_Unknown164d2cc, which is also the current stage).
struct StageBgmEntry
{
    i32 stageBgm;
    i32 bossBgm;
    i32 unused;
};

DIFFABLE_EXTERN(GameManager, g_GameManager);
DIFFABLE_EXTERN(u32, g_CurFrameCount);
DIFFABLE_EXTERN(i32, g_GameManagerState);
DIFFABLE_EXTERN(u16, g_StageClearFlag);
DIFFABLE_EXTERN(f32, g_CsumFloat);
DIFFABLE_EXTERN(u16, g_GlobalNoRetryClears[5]);
DIFFABLE_EXTERN(u16, g_GlobalWithRetryClears[5]);

// One clear-counter entry per difficulty; each entry is 0x44 bytes, only the
// leading i32 is used (this matches the original's `[difficulty * 0x44]` stride).
struct ClearCountEntry
{
    i32 count;
    unknown_fields(0x4, 0x40);
};

DIFFABLE_EXTERN(ClearCountEntry, g_ClearCountByDifficulty[5]);
DIFFABLE_EXTERN(u32, g_ScreenClearColor);
DIFFABLE_EXTERN(StageBgmEntry, g_StageBgmTable[9]);
}; // Namespace th08
