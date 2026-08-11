#include "th_pch.h"

#include "GameManager.hpp"
#include "AsciiManager.hpp"
#include "Background.hpp"
#include "BulletManager.hpp"
#include "EffectManager.hpp"
#include "EnemyManager.hpp"
#include "Global.hpp"
#include "Gui.hpp"
#include "ItemManager.hpp"
#include "Midi.hpp"
#include "Player.hpp"
#include "ReplayManager.hpp"
#include "ResultScreen.hpp"
#include "ScoreDat.hpp"
#include "ScreenEffect.hpp"
#include "SoundPlayer.hpp"
#include "Spellcard.hpp"
#include "Supervisor.hpp"

namespace th08
{

DIFFABLE_STATIC(GameManager, g_GameManager);
DIFFABLE_STATIC(ChainElem, g_GameManagerCalcChain);
DIFFABLE_STATIC(ChainElem, g_GameManagerDrawChain);
DIFFABLE_STATIC(u32, g_UnkEnemyDataPtr);

void IncrementCappedAgain(u32 *param, u32 cap);

ZunBool GameManager::IsWithinPlayfield(f32 x, f32 y, f32 width, f32 height)
{
    if (x + width / 2.0f < 0.0f)
    {
        return FALSE;
    }
    if (x - width / 2.0f > 384.0f)
    {
        return FALSE;
    }
    if (y + height / 2.0f < 0.0f)
    {
        return FALSE;
    }
    if (y - height / 2.0f > 448.0f)
    {
        return FALSE;
    }
    return TRUE;
}

i32 GameManager::CalcAntiTamperChecksum()
{
    i32 sum;

    // There is zero chance ZUN actually used intptr_t here, but the codegen matches
    // and not making assumptions about pointer size is always nice
    sum = CalcChecksum((u8 *)&g_GameManager.globals->rng1,
                       (intptr_t)&globals->antiTamperValue - (intptr_t)&globals->rng1);
    sum += CalcChecksum((u8 *)&g_GameManager.globals->rng8, sizeof(g_GameManager.globals->rng8));
    sum += CalcChecksum((u8 *)g_GameManager.cfg, sizeof(GameConfiguration));
    sum += CalcChecksum((u8 *)&g_Supervisor.cfg, sizeof(GameConfiguration));
    sum += CalcChecksum((u8 *)&this->hscr, sizeof(Hscr));

    return sum;
}

i32 GameManager::CalcChecksum(u8 *address, i32 size)
{
    i32 sum;
    i32 i;

    for (sum = 0, i = 0; i < size; i++, address++)
    {
        sum += *address;
        g_GameManager.globals->antiTamperValue += g_GameManager.globals->rng8[2];
    }

    return sum;
}

void GameManager::CollectExtend()
{
    if (this->GetLives() < 8)
    {
        this->AddLives(1);
        g_SoundPlayer.PlaySoundByIdx(SOUND_1UP, 0);
        this->IncreaseSubrank(200);
        g_Gui.flags.lifeDisplayUpdateFrames = 2;
    }
    else if (this->GetBombsRemaining() < 8)
    {
        this->AddToBombCount(1);
        g_SoundPlayer.PlaySoundByIdx(SOUND_1UP, 0);
        this->IncreaseSubrank(200);
        g_Gui.flags.bombDisplayUpdateFrames = 2;
    }
}

// STUB: th08 0x439bc7
ChainCallbackResult GameManager::OnUpdate(GameManager *gameManager)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

struct SpellcardBgmEntry
{
    i32 maxSpellcardNumber;
    i32 bgmIndex;
    char *path;
    i32 isBossTheme;
    i32 isLastWordTheme;
};

DIFFABLE_STATIC_ARRAY_ASSIGN(SpellcardBgmEntry, 19, g_SpellcardBgmTable) = {
    {1, 1, "th08_00.mid", 0, 0},     {12, 2, "th08_03.mid", 1, 0},  {16, 3, "th08_04.mid", 0, 0},
    {31, 4, "th08_05.mid", 1, 0},    {35, 5, "th08_06.mid", 0, 0},  {53, 6, "th08_07.mid", 1, 0},
    {76, 8, "th08_09.mid", 1, 0},    {99, 9, "th08_10.mid", 1, 0},  {118, 11, "th08_12.mid", 1, 0},
    {122, 12, "th08_13.mid", 0, 0},  {142, 13, "th08_14.mid", 1, 0}, {146, 15, "th08_13b.mid", 2, 1},
    {150, 12, "th08_13.mid", 0, 0},  {170, 14, "th08_15.mid", 1, 0}, {190, 15, "th08_13b.mid", 2, 1},
    {193, 16, "th08_18.mid", 0, 0},  {204, 17, "th08_19.mid", 1, 0}, {222, 20, "th08_20.mid", 2, 0},
    {-1, 0, " ", 0, 0},
};

DIFFABLE_STATIC(i32, g_SpellcardBgmOverride);

static u32 g_TimeRequirementParams[9][4] = {
    {2000, 2500, 2700, 3000}, {6500, 7200, 7200, 7200}, {7500, 8500, 8800, 8800},
    {9999, 9999, 9999, 9999}, {7500, 8500, 8500, 8500}, {9999, 9999, 9999, 9999},
    {0, 0, 0, 0},             {0, 0, 0, 0},             {0, 0, 0, 0},
};

i32 GameManager::GetSpellcardBgmIsLastWord(i32 spellcardNumber)
{
    i32 i;

    i = 0;
    while (g_SpellcardBgmTable[i].maxSpellcardNumber >= 0)
    {
        if (g_GameManager.currentSpellCardNumber <= g_SpellcardBgmTable[i].maxSpellcardNumber)
        {
            return g_SpellcardBgmTable[i].isLastWordTheme;
        }
        i++;
    }
    return 0;
}

ChainCallbackResult GameManager::OnDraw(GameManager *gameManager)
{
    if (gameManager->isInGameMenu)
    {
        gameManager->isInGameMenu = 2;
    }
    if (g_Supervisor.curState != SupervisorState_GameManager)
    {
        return CHAIN_CALLBACK_RESULT_BREAK;
    }
    if (gameManager->flags.unk5 == 1)
    {
        return CHAIN_CALLBACK_RESULT_BREAK;
    }
    if (gameManager->unk38 != 0)
    {
        return CHAIN_CALLBACK_RESULT_BREAK;
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ZunResult GameManager::RegisterChain()
{
    GameManager *gameManager = &g_GameManager;

    g_GameManagerCalcChain.SetCallback((ChainCallback)OnUpdate);
    g_GameManagerCalcChain.addedCallback = (ChainLifetimeCallback)AddedCallback;
    g_GameManagerCalcChain.deletedCallback = (ChainLifetimeCallback)DeletedCallback;
    g_GameManagerCalcChain.arg = gameManager;
    gameManager->unk3ddc0 = 0;
    if (g_Chain.AddToCalcChain(&g_GameManagerCalcChain, 2))
    {
        return ZUN_ERROR;
    }
    g_GameManagerDrawChain.SetCallback((ChainCallback)OnDraw);
    g_GameManagerDrawChain.arg = gameManager;
    g_Chain.AddToDrawChain(&g_GameManagerDrawChain, 5);
    return ZUN_SUCCESS;
}

ZunResult GameManager::AddedCallback(GameManager *gameManager)
{
    if (g_Supervisor.curState != SupervisorState_GameManagerReInit &&
        g_Supervisor.curState != SupervisorState_SpellcardPracticeRestart &&
        g_Supervisor.curState != SupervisorState_GameManagerNextStageWeird)
    {
        g_Supervisor.unk164 = 1;
    }
    else
    {
        g_Supervisor.unk164 = 0;
    }
    g_GameManager.unk38 = 1;
    if (g_Supervisor.wantedState2 == SupervisorState_TitleScreen)
    {
        Float3 position;
        position.x = 500.0f;
        position.y = 440.0f;
        position.z = 0.0f;
        g_Supervisor.SetupLoadingVms(&position);
        g_Supervisor.StartEffect(0);
    }
    else
    {
        Float3 position;
        position.x = 280.0f;
        position.y = 430.0f;
        position.z = 0.0f;
        g_Supervisor.SetupLoadingVms(&position);
    }
    if (gameManager->flags.unk5 >= 2)
    {
        gameManager->flags.unk5 = 1;
    }
    g_Supervisor.ThreadStart((LPTHREAD_START_ROUTINE)GameplaySetupThread, NULL);
    return ZUN_SUCCESS;
}

void GameManager::GameplaySetupThread(void *param)
{
    GameManager *gameManager = &g_GameManager;
    u16 oldSeed;
    void *unk0x0;
    i32 i;
    i32 j;
    i32 spellcardBgmCheck;

    gameManager->unk3c = 0;
    g_Supervisor.systemTime = timeGetTime();
    gameManager->unk3DDD0 = 1 << gameManager->currentStage;
    gameManager->currentStage2 = gameManager->currentStage;
    if (gameManager->difficulty < 4)
    {
        gameManager->difficultyMask = 1 << gameManager->difficulty;
    }
    else
    {
        gameManager->difficultyMask = 0xf;
    }
    gameManager->unk3dbaa = gameManager->shotType + gameManager->fullShotType;
    g_Supervisor.framerateMultiplier = 1.0f;
    gameManager->flags.unk10 = 0;
    if (g_Supervisor.unk164 != 0 || gameManager->flags.isSpellPractice || g_GameManager.flags.isPracticeMode ||
        g_GameManager.difficulty >= 4)
    {
        if (gameManager->cfg != NULL)
        {
            delete gameManager->cfg;
            gameManager->cfg = NULL;
        }
        if (gameManager->globals != NULL)
        {
            delete gameManager->globals;
            gameManager->globals = NULL;
        }
        i = g_Rng.GetRandomU32InRange(0xffff) + 0x10;
        gameManager->unk0x0 = malloc(i);
        gameManager->cfg = new GameConfiguration;
        gameManager->globals = new ZunGlobals;
        g_GameManager.InitializeAntiTamper();
        *gameManager->cfg = g_Supervisor.cfg;
        unk0x0 = gameManager->unk0x0;
        free(unk0x0);
        gameManager->character = 0;
        gameManager->globals->youkaiGauge = 0;
        gameManager->globals->clockTime = g_GameManager.currentStage == 8 ? 6 : 0;
        if (g_GameManager.difficulty >= 4)
        {
            gameManager->cfg->lifeCount = 2;
        }
        if (g_GameManager.flags.isPracticeMode)
        {
            gameManager->cfg->lifeCount = 8;
        }
        if (Player::RegisterChain(0) != ZUN_SUCCESS)
        {
            if (g_Supervisor.subthreadCloseRequestActive)
            {
                return;
            }
            g_GameErrorContext.Log("error : \x83v\x83\x8c\x83" "C\x83\x84\x81[\x82\xcc\x8f\x89\x8a\xfa\x89\xbb\x82\xc9\x8e\xb8\x94s\x82\xb5\x82\xdc\x82\xb5\x82\xbd\x0d\x0a");
            goto error_exit;
        }
        if (!g_GameManager.flags.isReplay)
        {
            g_GameManager.globals->livesRemaining = (f32)gameManager->cfg->lifeCount;
            g_GameManager.UpdateAntiTamper();
            g_GameManager.SetBombCount((i32)g_PlayerShtFile->unk4);
        }
        gameManager->InitArcadeRegionParams();
        gameManager->globals->playerPower = 0.0f;
        gameManager->UpdateAntiTamper();
        gameManager->unk3de04 = 0;
        g_GameManager.unk3D2A4 = 0;
        g_GameManager.unk3D2A0 = 0;
        gameManager->globals->displayScore = 0;
        gameManager->globals->score = 0;
        gameManager->globals->unk0x10 = 0;
        gameManager->globals->displayedHighScore = 0x186a0;
        gameManager->globals->numRetries = 0;
        gameManager->globals->graze = 0;
        gameManager->globals->pointItemsCollected = 0;
        if (gameManager->difficulty >= 4 || gameManager->flags.isPracticeMode || gameManager->flags.isSpellPractice)
        {
            gameManager->cfg->slowMode = 0;
        }
        switch (g_GameManager.difficulty)
        {
        case 0:
            gameManager->globals->pointItemValue = 0xea60;
            break;
        case 1:
            gameManager->globals->pointItemValue = 0x186a0;
            break;
        case 2:
            gameManager->globals->pointItemValue = 0x30d40;
            break;
        case 3:
            gameManager->globals->pointItemValue = 0x493e0;
            break;
        case 4:
            gameManager->globals->pointItemValue = 0x493e0;
            break;
        }
        gameManager->globals->pointItemExtendsSoFar = 0;
        ItemManager::UpdatePointItemExtendThreshold();
        if (GameManager::LoadScoreData() != ZUN_SUCCESS)
        {
            goto error_exit;
        }
        gameManager->InitRankParams();
        gameManager->globals->deaths = 0.0f;
        gameManager->globals->deathInStage = 0.0f;
        gameManager->UpdateAntiTamper();
        gameManager->globals->bombsUsed = 0.0f;
        gameManager->globals->bombsUsedInStage = 0.0f;
        gameManager->UpdateAntiTamper();
        gameManager->globals->unk1C = 0;
        gameManager->unk3de10 = 0;
        gameManager->unk3de18 = 0;
        gameManager->unk3de1c = 0;
        if (!g_GameManager.flags.isReplay && !g_GameManager.flags.isSpellPractice)
        {
            if (gameManager->cfg->slowMode == 0)
            {
                IncrementCappedAgain(&g_GameManager.plst.playDataByDifficulty[g_GameManager.difficulty].attemptsTotal,
                                     999999);
                IncrementCappedAgain(&g_GameManager.plst.playDataByDifficulty[6].attemptsTotal, 999999);
                IncrementCappedAgain(&g_GameManager.plst.playDataByDifficulty[g_GameManager.difficulty]
                                          .attemptsPerCharacter[gameManager->shotType],
                                     999999);
                IncrementCappedAgain(
                    &g_GameManager.plst.playDataByDifficulty[6].attemptsPerCharacter[gameManager->shotType], 999999);
                if (g_Supervisor.curState == SupervisorState_GameManagerRestartFromBeginning)
                {
                    IncrementCappedAgain(&g_GameManager.plst.playDataByDifficulty[g_GameManager.difficulty].unk0x34,
                                         999999);
                    IncrementCappedAgain(&g_GameManager.plst.playDataByDifficulty[6].unk0x34, 999999);
                }
                if (g_GameManager.flags.isPracticeMode && !g_GameManager.flags.isSpellPractice)
                {
                    IncrementCappedAgain(&g_GameManager.plst.playDataByDifficulty[g_GameManager.difficulty].practices,
                                         999999);
                    IncrementCappedAgain(&g_GameManager.plst.playDataByDifficulty[6].practices, 999999);
                }
            }
        }
        else
        {
            gameManager->cfg->slowMode = 0;
        }
    }
    else
    {
        gameManager->globals->displayScore = gameManager->globals->score;
        gameManager->globals->unk0x10 = 0;
        gameManager->globals->deathInStage = 0.0f;
        gameManager->UpdateAntiTamper();
        gameManager->globals->bombsUsedInStage = 0.0f;
        gameManager->UpdateAntiTamper();
        if (Player::RegisterChain(0) != ZUN_SUCCESS)
        {
            if (g_Supervisor.subthreadCloseRequestActive)
            {
                return;
            }
            g_GameErrorContext.Log("error : \x83v\x83\x8c\x83" "C\x83\x84\x81[\x82\xcc\x8f\x89\x8a\xfa\x89\xbb\x82\xc9\x8e\xb8\x94s\x82\xb5\x82\xdc\x82\xb5\x82\xbd\x0d\x0a");
            goto error_exit;
        }
    }
    gameManager->subRank = 0;
    gameManager->globals->pointItemsCollectedInStage = 0;
    gameManager->globals->grazeInStage = 0;
    gameManager->isInGameMenu = 0;
    gameManager->flags.unk7 = 0;
    gameManager->flags.unk13 = 0;
    gameManager->unk3de14 = 0;
    gameManager->unk3de20 = 0;
    gameManager->unk3de24 = 0;
    gameManager->globals->youkaiGaugeCopy = gameManager->globals->youkaiGauge;
    gameManager->globals->currentTimeOrbs = 0;
    gameManager->globals->totalTimeOrbs = 0;
    if (!g_GameManager.flags.isSpellPractice)
    {
        gameManager->globals->lastSpellTimeOrbThreshold =
            g_TimeRequirementParams[gameManager->currentStage][g_GameManager.difficulty];
    }
    else
    {
        gameManager->globals->lastSpellTimeOrbThreshold = 0;
    }
    if (gameManager->flags.isPracticeMode)
    {
        if (!gameManager->flags.isSpellPractice)
        {
            switch (gameManager->currentStage)
            {
            case 0:
                gameManager->globals->playerPower = 0.0f;
                gameManager->UpdateAntiTamper();
                break;
            case 1:
                gameManager->globals->playerPower = 112.0f;
                gameManager->UpdateAntiTamper();
                break;
            default:
                gameManager->globals->playerPower = 128.0f;
                gameManager->UpdateAntiTamper();
                break;
            }
        }
        else
        {
            if (gameManager->currentSpellCardNumber <= 1)
            {
                gameManager->globals->playerPower = 30.0f;
                gameManager->UpdateAntiTamper();
            }
            else if (gameManager->currentSpellCardNumber <= 12)
            {
                gameManager->globals->playerPower = 80.0f;
                gameManager->UpdateAntiTamper();
            }
            else
            {
                gameManager->globals->playerPower = 128.0f;
                gameManager->UpdateAntiTamper();
            }
        }
    }
    if (g_GameManager.flags.isReplay)
    {
        gameManager->InitRankParams();
        ReplayManager::RegisterChain(1, g_GameManager.replayFilename);
        oldSeed = g_Rng.seed;
        gameManager->UpdateAntiTamper();
        g_Rng.seed = oldSeed;
    }
    gameManager->stageRngSeed = g_Rng.seed;
    if (Background::RegisterChain(gameManager->currentStage) != ZUN_SUCCESS)
    {
        if (g_Supervisor.subthreadCloseRequestActive)
        {
            return;
        }
        g_GameErrorContext.Log("error : \x94w\x8ci\x83" "f\x81[\x83^\x82\xcc\x8f\x89\x8a\xfa\x89\xbb\x82\xc9\x8e\xb8\x94s\x82\xb5\x82\xdc\x82\xb5\x82\xbd\x0d\x0a");
        goto error_exit;
    }
    if (BulletManager::RegisterChain("etama.anm") != ZUN_SUCCESS)
    {
        if (g_Supervisor.subthreadCloseRequestActive)
        {
            return;
        }
        g_GameErrorContext.Log("error : \x93G\x92" "e\x82\xcc\x8f\x89\x8a\xfa\x89\xbb\x82\xc9\x8e\xb8\x94s\x82\xb5\x82\xdc\x82\xb5\x82\xbd\x0d\x0a");
        goto error_exit;
    }
    if (EnemyManager::RegisterChain() != ZUN_SUCCESS)
    {
        if (g_Supervisor.subthreadCloseRequestActive)
        {
            return;
        }
        g_GameErrorContext.Log("error : \x93G\x82\xcc\x8f\x89\x8a\xfa\x89\xbb\x82\xc9\x8e\xb8\x94s\x82\xb5\x82\xdc\x82\xb5\x82\xbd\x0d\x0a");
        goto error_exit;
    }
    if (EffectManager::RegisterChain() != ZUN_SUCCESS)
    {
        if (g_Supervisor.subthreadCloseRequestActive)
        {
            return;
        }
        g_GameErrorContext.Log("error : \x83G\x83t\x83" "F\x83N\x83g\x82\xcc\x8f\x89\x8a\xfa\x89\xbb\x82\xc9\x8e\xb8\x94s\x82\xb5\x82\xdc\x82\xb5\x82\xbd\x0d\x0a");
        goto error_exit;
    }
    if (Gui::RegisterChain() != ZUN_SUCCESS)
    {
        if (g_Supervisor.subthreadCloseRequestActive)
        {
            return;
        }
        g_GameErrorContext.Log("error : 2D\x95\x5c\x8e\xa6\x82\xcc\x8f\x89\x8a\xfa\x89\xbb\x82\xc9\x8e\xb8\x94s\x82\xb5\x82\xdc\x82\xb5\x82\xbd\x0d\x0a");
        goto error_exit;
    }
    if (Spellcard::RegisterChain() != ZUN_SUCCESS)
    {
        if (g_Supervisor.subthreadCloseRequestActive)
        {
            return;
        }
        g_GameErrorContext.Log("error : \x83X\x83y\x83\x8b\x83J\x81[\x83h\x82\xcc\x8f\x89\x8a\xfa\x89\xbb\x82\xc9\x8e\xb8\x94s\x82\xb5\x82\xdc\x82\xb5\x82\xbd\x0d\x0a");
        goto error_exit;
    }
    if (!g_GameManager.flags.isReplay)
    {
        ReplayManager::RegisterChain(0, "replay/th8_00.rpy");
    }
    if (g_GameManager.flags.isSpellPractice)
    {
        switch (g_GameManager.currentStage)
        {
        case 5:
            if (g_GameManager.flags.isSpellPractice)
            {
                spellcardBgmCheck = g_GameManager.currentSpellCardNumber == 0xd4;
            }
            else
            {
                spellcardBgmCheck = 0;
            }
            break;
        case 6:
            if ((g_GameManager.flags.isSpellPractice ? (g_GameManager.currentSpellCardNumber >= 0x77 &&
                                                        g_GameManager.currentSpellCardNumber <= 0x7a)
                                                     : 0) == 0)
            {
                g_SpellcardBgmOverride = 2;
            }
            break;
        case 7:
            if ((g_GameManager.flags.isSpellPractice ? (g_GameManager.currentSpellCardNumber >= 0x93 &&
                                                        g_GameManager.currentSpellCardNumber <= 0x96)
                                                     : 0) == 0)
            {
                g_SpellcardBgmOverride = 2;
            }
            break;
        case 8:
            if ((g_GameManager.flags.isSpellPractice ? (g_GameManager.currentSpellCardNumber >= 0xbf &&
                                                        g_GameManager.currentSpellCardNumber <= 0xc1)
                                                     : 0) == 0)
            {
                i32 isLastWordCard;
                if (g_GameManager.flags.isSpellPractice)
                {
                    isLastWordCard = g_GameManager.currentSpellCardNumber == 0xd5;
                }
                else
                {
                    isLastWordCard = 0;
                }
                if (isLastWordCard == 0)
                {
                    g_SpellcardBgmOverride = 2;
                }
            }
            break;
        }
    }
    if (g_Supervisor.unk16c == 0)
    {
        if (g_GameManager.flags.isSpellPractice)
        {
            j = 0;
            while (g_SpellcardBgmTable[j].maxSpellcardNumber >= 0)
            {
                if (g_GameManager.currentSpellCardNumber <= g_SpellcardBgmTable[j].maxSpellcardNumber)
                {
                    g_Supervisor.LoadMusic(0, g_SpellcardBgmTable[j].path);
                    break;
                }
                j++;
            }
        }
        else
        {
            g_Supervisor.LoadMusic(0, g_Background.stdData->bgmPaths[0]);
            if (g_Background.stdData->bgmPaths[1][0] != ' ')
            {
                g_Supervisor.LoadMusic(1, g_Background.stdData->bgmPaths[1]);
            }
            if (g_Background.stdData->bgmPaths[2][0] != ' ')
            {
                g_Supervisor.LoadMusic(2, g_Background.stdData->bgmPaths[2]);
            }
        }
    }
    gameManager->showRetryMenu = 0;
    gameManager->flags.unk2 = 1;
    if (g_Supervisor.unk16c != 0 && g_GameManager.flags.isSpellPractice &&
        GameManager::GetSpellcardBgmIsLastWord(g_GameManager.currentSpellCardNumber) == 0)
    {
        gameManager->unk3de28 = 2;
    }
    else
    {
        gameManager->unk3de28 = 1;
    }
    if (g_Supervisor.curState != SupervisorState_GameManagerReInit)
    {
        g_Supervisor.lagNumerator = 0.0f;
        g_Supervisor.lagDenominator = 0.0f;
    }
    gameManager->unk2C = 0;
    gameManager->globals->score = 0;
    gameManager->flags.unk4 = 0;
    g_AsciiManager.Reset();
    g_AsciiManager.InitializeVms();
    g_GameManager.unk2D = 0;
    g_UnkEnemyDataPtr = 0;
    Supervisor::CalculateFps(0);
    if (g_GameManager.flags.isReplay)
    {
        while (gameManager->unk3c < 0x50)
        {
            Sleep(0x11);
        }
    }
    else
    {
        while (gameManager->unk3c < 0x1e)
        {
            Sleep(0x11);
        }
    }
    g_Supervisor.HideLoadingVms();
    while (gameManager->flags.unk5 != 0)
    {
        Sleep(0x11);
    }
    g_GameManager.unk38 = 0;
    g_Supervisor.runningSubthreadHandle = NULL;
    g_Supervisor.subthreadCloseRequestActive = FALSE;
    g_Supervisor.unk290 = 0;
    g_Supervisor.unk174 = 0x3c;
    gameManager->flags.unk9 = 0;
    g_Supervisor.unk16c = 0;
    g_ScreenEffectCounter = 2;
    return;

error_exit:
    g_GameManager.unk38 = 2;
    g_Supervisor.HideLoadingVms();
    g_Supervisor.runningSubthreadHandle = NULL;
    g_Supervisor.subthreadCloseRequestActive = FALSE;
    g_Supervisor.unk290 = 0;
    g_Supervisor.unk16c = 0;
    g_ScreenEffectCounter = 2;
    return;
}

#pragma var_order(sum, i)
void GameManager::InitializeAntiTamper()
{
    i32 sum;
    u32 i;

    g_GameManager.globals->rng6 = g_Rng.GetRandomU32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
    for (i = 0; i < ARRAY_SIZE(g_GameManager.globals->rng1); i++)
    {
        g_GameManager.globals->rng1[i] = g_Rng.GetRandomU32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
    }
    for (i = 0; i < ARRAY_SIZE(g_GameManager.globals->rng7); i++)
    {
        g_GameManager.globals->rng7[i] = g_Rng.GetRandomU32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
    }
    for (i = 0; i < ARRAY_SIZE(g_GameManager.globals->rng2); i++)
    {
        g_GameManager.globals->rng2[i] = g_Rng.GetRandomF32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
    }
    for (i = 0; i < ARRAY_SIZE(g_GameManager.globals->rng3); i++)
    {
        g_GameManager.globals->rng3[i] = g_Rng.GetRandomF32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
    }
    for (i = 0; i < ARRAY_SIZE(g_GameManager.globals->rng4); i++)
    {
        g_GameManager.globals->rng4[i] = g_Rng.GetRandomF32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
    }
    for (i = 0; i < ARRAY_SIZE(g_GameManager.globals->rng5); i++)
    {
        g_GameManager.globals->rng5[i] = g_Rng.GetRandomF32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
    }
    for (i = 0; i < ARRAY_SIZE(g_GameManager.globals->rng8); i++)
    {
        g_GameManager.globals->rng8[i] = g_Rng.GetRandomU32InRange(ANTITAMPER_RNG_RANGE) + ANTITAMPER_RNG_ADD;
    }
    g_GameManager.globals->antiTamperValue = g_GameManager.globals->rng1[2];
    sum = g_GameManager.CalcAntiTamperChecksum();
    g_GameManager.globals->antiTamperChecksum = sum;
    g_GameManager.antiTamperExpectedValue = (f32)sum + (f32)g_GameManager.globals->rng7[3];
}

void IncrementCappedAgain(u32 *param, u32 cap)
{
    // cap seemingly completely unused here
    if (*param < 999999)
    {
        (*param)++;
    }
}

struct RankParams
{
    i32 initial;
    i32 min;
    i32 max;
};

DIFFABLE_STATIC_ARRAY_ASSIGN(RankParams, 6, g_RankParams) = {
    {10, 16, 8}, {10, 16, 8}, {8, 12, 8}, {8, 12, 8}, {16, 16, 15}, {16, 16, 15},
};

void GameManager::InitRankParams()
{
    this->rank = g_RankParams[g_GameManager.difficulty].initial;
    this->minRank = g_RankParams[g_GameManager.difficulty].min;
    this->maxRank = g_RankParams[g_GameManager.difficulty].max;
}

#pragma var_order(catk, i, scoreDat, j)
ZunResult GameManager::LoadScoreData()
{
    Catk *catk = g_GameManager.catkData;
    i32 i;
    ScoreDat *scoreDat;
    i32 j;

    ResultScreen::RegisterChain(2);
    memset(g_GameManager.catkData, 0, sizeof(g_GameManager.catkData));
    for (i = 0; i < SPELLCARD_COUNT_SPELLCARDS; i++, catk++)
    {
        catk->base.magic = 0x4b544143;
        catk->base.unkLen = sizeof(Catk);
        catk->base.th8kLen = sizeof(Catk);
        catk->base.version = 3;
        catk->spellcardNumber = (i16)i;
        for (j = 0; j < 7; j++)
        {
            catk->inGameHistory.attempts[j] = 0;
            catk->inGameHistory.captures[j] = 0;
            catk->inGameHistory.maxBonus[j] = 0;
        }
    }
    scoreDat = ScoreDat::OpenScore("score.dat");
    if (scoreDat == NULL)
    {
        g_GameErrorContext.Log("error : \x83X\x83R\x83" "A\x83t\x83@\x83" "C\x83\x8b\x82\xcc\x93\xc7\x82\xdd\x8e\xe6\x82\xe8\x82\xc9\x8e\xb8\x94s\x82\xb5\x82\xdc\x82\xb5\x82\xbd\x0d\x0a");
        return ZUN_ERROR;
    }
    g_GameManager.globals->displayedHighScore =
        ScoreDat::GetHighScore(scoreDat, 0, g_GameManager.shotType, g_GameManager.difficulty,
                               &g_GameManager.globals->ontinuesUsedInHighScore);
    ScoreDat::ParseCATK(scoreDat, g_GameManager.catkData);
    ScoreDat::ParseCLRD(scoreDat, g_GameManager.clrdData);
    ScoreDat::ParsePSCR(scoreDat, g_GameManager.pscrData);
    if (g_GameManager.flags.isPracticeMode)
    {
        g_GameManager.globals->displayedHighScore =
            g_GameManager.pscrData[g_GameManager.shotType].highScores[g_GameManager.currentStage][g_GameManager.difficulty];
        g_GameManager.pscrData[g_GameManager.shotType].attempts[g_GameManager.currentStage][g_GameManager.difficulty]++;
        g_GameManager.pscrData[g_GameManager.shotType].unk0x175 = 1;
    }
    ScoreDat::ReleaseScore(scoreDat);
    memcpy(g_GameManager.catkDataBackup, g_GameManager.catkData, sizeof(g_GameManager.catkDataBackup));
    memset(&g_GameManager.hscr, 0, sizeof(g_GameManager.hscr));
    g_GameManager.hscr.character = g_GameManager.shotType;
    g_GameManager.hscr.difficulty = g_GameManager.difficulty;
    g_GameManager.hscr.cfg = g_Supervisor.cfg;
    g_GameManager.unk3D294 = 0;
    return ZUN_SUCCESS;
}

ZunResult GameManager::DeletedCallback(GameManager *gameManager)
{
    MidiOutput *midiOutput;

    g_ScreenEffectCounter = 1;
    g_UnkEnemyDataPtr = 0;
    if (g_Supervisor.curState != SupervisorState_GameManagerReInit &&
        g_Supervisor.curState != SupervisorState_SpellcardPracticeRestart &&
        g_Supervisor.curState != SupervisorState_GameManagerNextStageWeird)
    {
        g_Supervisor.unk168 = 1;
    }
    else
    {
        g_Supervisor.unk168 = 0;
    }
    if (!g_GameManager.flags.isSpellPractice || g_Supervisor.unk168 != 0)
    {
        g_Supervisor.StopAudio();
        if (g_Supervisor.cfg.musicMode == 2 && g_Supervisor.midiOutput != NULL)
        {
            midiOutput = g_Supervisor.midiOutput;
            midiOutput->PlayFile(0x1e);
        }
    }
    while (g_SoundPlayer.ProcessQueues() != 0)
    {
    }
    Spellcard::CutChain();
    Background::CutChain();
    BulletManager::CutChain();
    Player::CutChain();
    EnemyManager::CutChain();
    EffectManager::CutChain();
    Gui::CutChain();
    if (!g_GameManager.flags.isReplay)
    {
        ReplayManager::StopRecording();
    }
    if (!g_GameManager.flags.isReplay)
    {
        Supervisor::UpdateGameTime(&g_Supervisor);
    }
    g_Supervisor.systemTime = 0;
    Supervisor::UpdatePlayTime(&g_Supervisor);
    gameManager->flags.unk2 = 0;
    g_AsciiManager.Reset();
    g_GameManager.unk2D = 0;
    g_GameManager.unk3ddc0 = 0;
    return ZUN_SUCCESS;
}

void GameManager::IncreaseSubrank(int amount)
{
    this->subRank += amount;
    while (this->subRank >= 100)
    {
        this->rank++;
        this->subRank -= 100;
    }
    if (this->rank > this->maxRank)
    {
        this->rank = this->maxRank;
    }
}

void GameManager::DecreaseSubrank(int amount)
{
    this->subRank -= amount;
    while (this->subRank < 0)
    {
        this->rank--;
        this->subRank += 100;
    }
    if (this->rank < this->minRank)
    {
        this->rank = this->minRank;
    }
}

void GameManager::AddToYoukaiGauge(i32 param_1, i32 param_2)
{
    if (g_Player.unkFdc != 0 && param_2 == 0)
    {
        return;
    }
    this->globals->youkaiGauge += param_1;
    if (this->globals->youkaiGauge < this->youkaiGaugeHumanLimit)
    {
        this->globals->youkaiGauge = this->youkaiGaugeHumanLimit;
    }
    else if (this->globals->youkaiGauge > this->youkaiGaugeYoukaiLimit)
    {
        this->globals->youkaiGauge = this->youkaiGaugeYoukaiLimit;
    }
    this->globals->youkaiGaugeCopy = this->globals->youkaiGauge;
}

// Leftover from PCB.
ZunBool GameManager::IsPhantasmUnlocked()
{
    return FALSE;
}

void GameManager::CutChain()
{
    g_Chain.Cut(&g_GameManagerCalcChain);
    g_Chain.Cut(&g_GameManagerDrawChain);
    if (g_GameManager.globals->score >= 1000000000)
    {
        g_GameManager.globals->score = 999999999;
    }
    g_GameManager.globals->displayScore = g_GameManager.globals->score;
    g_Supervisor.framerateMultiplier = 1.0f;
}




i32 GameManager::GetTimeOrbs()
{
    return this->globals->currentTimeOrbs;
}

i32 GameManager::GetLastSpellTimeOrbThreshold()
{
    return this->globals->lastSpellTimeOrbThreshold;
}

// FUNCTION: th08 0x43c15f
i32 GameManager::FinalBCleared(i32 difficulty)
{
    i32 result;

    if (difficulty > 3)
    {
        goto set1;
    }
    if (this->clrdData[difficulty].difficultiesClearedWithoutRetries[0] & EXTRA_UNLOCKED_FLAG)
    {
        goto set1;
    }
    if (this->clrdData[difficulty].difficultiesClearedWithoutRetries[1] & EXTRA_UNLOCKED_FLAG)
    {
        goto set1;
    }
    if (this->clrdData[difficulty].difficultiesClearedWithoutRetries[2] & EXTRA_UNLOCKED_FLAG)
    {
        goto set1;
    }
    if (this->clrdData[difficulty].difficultiesClearedWithoutRetries[3] & EXTRA_UNLOCKED_FLAG)
    {
        goto set1;
    }

    result = 0;
    goto end;

set1:
    result = 1;

end:
    return result;
}

// FUNCTION: th08 0x439829
ZunBool GameManager::IsStageClearedWithoutRetries(i32 stage, i32 character, i32 difficulty)
{
    return IS_STAGE_CLEARED(this->clrdData[character].difficultiesClearedWithoutRetries[difficulty], stage);
}

// FUNCTION: th08 0x439856
ZunBool GameManager::IsStageClearedWithRetries(i32 stage, i32 character, i32 difficulty)
{
    return IS_STAGE_CLEARED(this->clrdData[character].difficultiesClearedWithRetries[difficulty], stage);
}

// STUB: th08 0x406d70
ZunBool GameManager::GaugeIsExtremelyYoukai()
{
    return FALSE;
}

// FUNCTION: th08 0x40bb80
ZunBool GameManager::IsTampered()
{
    // There is zero chance ZUN actually used intptr_t here, but the codegen matches and not making
    // assumptions about pointer size is always nice
    return this->globals->antiTamperValue !=
               this->globals->rng1[2] + this->globals->rng8[2] * ((intptr_t)&this->globals->antiTamperValue -
                                                                  (intptr_t)&this->globals->rng1 + 500) ||
           this->globals->antiTamperChecksum + this->globals->rng7[3] != (i32)this->antiTamperExpectedValue;
}

// FUNCTION: th08 0x43c322
u32 GameManager::FUN_0043c322()
{
    return this->flags.isReplay && g_ReplayManager->unk8->isPractice != 0;
}

void GameManager::SetClockTime(u8 clockTime)
{
    this->globals->clockTime = clockTime;
}

i32 GameManager::GetDeaths()
{
    return (i32)this->globals->deaths;
}

i32 GameManager::GetBombsUsed()
{
    return (i32)this->globals->bombsUsed;
}

void GameManager::AddToDeaths(i32 deaths)
{
    if (IsTampered())
    {
        CRASH_GAME();
    }
    this->globals->deaths += (f32)deaths;
}

void GameManager::AddToBombsUsed(i32 bombs)
{
    if (IsTampered())
    {
        CRASH_GAME();
    }
    this->globals->bombsUsed += (f32)bombs;
}

i32 GameManager::ScaleIntBasedOnRank(i32 min, i32 max)
{
    return min + ((max - min) * this->rank + 0x1f) >> 5;
}

f32 GameManager::ScaleFloatBasedOnRank(f32 min, f32 max)
{
    return min + (f32)this->rank * (max - min) / 32.0f;
}

i32 GameManager::IsSoloHuman()
{
    return this->shotType >= 4 && (this->shotType & 1) == 0;
}

i32 GameManager::IsSoloYoukai()
{
    return this->shotType >= 4 && (this->shotType & 1) != 0;
}

u32 GameManager::GetFlag0()
{
    return this->flags.isPracticeMode;
}

u32 GameManager::GetFlag1()
{
    return this->flags.isDemoMode;
}

u32 GameManager::GetFlag3()
{
    return this->flags.isReplay;
}

u32 GameManager::GetFlag14()
{
    return this->flags.isSpellPractice;
}

i32 GameManager::GetYoukaiGauge()
{
    return this->globals->youkaiGauge;
}

bool GameManager::GaugeIsModeratelyHuman()
{
    return this->globals->youkaiGauge <= this->youkaiGaugeHumanEffectsThreshold;
}

void GameManager::SetLives(i32 lives)
{
    this->globals->livesRemaining = (f32)lives;
    this->UpdateAntiTamper();
}

void GameManager::SetYoukaiGauge(u16 gauge)
{
    this->globals->youkaiGauge = gauge;
}

i32 GameManager::GetPower()
{
    return this->globals->playerPower;
}

// FUNCTION: th08 0x406dd0
i8 GameManager::GetClockTime()
{
    return (i8)this->globals->clockTime;
}

// FUNCTION: th08 0x406df0
void GameManager::AddToClockTime(i8 value)
{
    this->globals->clockTime = (u8)((i8)this->globals->clockTime + value);
}

i32 GameManager::GetClockTimeIncrement()
{
    // ZUN bloat: Why not use switch case fallthrough?
    switch (g_GameManager.currentStage)
    {
    case STAGE1:
        if (g_GameManager.GetTimeOrbs() >= g_GameManager.GetLastSpellTimeOrbThreshold())
        {
            return 1;
        }
        else
        {
            return 2;
        }
    case STAGE2:
        if (g_GameManager.GetTimeOrbs() >= g_GameManager.GetLastSpellTimeOrbThreshold())
        {
            return 1;
        }
        else
        {
            return 2;
        }
    case STAGE3:
        if (g_GameManager.GetTimeOrbs() >= g_GameManager.GetLastSpellTimeOrbThreshold())
        {
            return 1;
        }
        else
        {
            return 2;
        }
    case STAGE4A:
        if (g_GameManager.GetTimeOrbs() >= g_GameManager.GetLastSpellTimeOrbThreshold())
        {
            return 1;
        }
        else
        {
            return 2;
        }
    case STAGE4B:
        if (g_GameManager.GetTimeOrbs() >= g_GameManager.GetLastSpellTimeOrbThreshold())
        {
            return 1;
        }
        else
        {
            return 2;
        }
    case STAGE5:
        if (g_GameManager.GetTimeOrbs() >= g_GameManager.GetLastSpellTimeOrbThreshold())
        {
            return 1;
        }
        else
        {
            return 2;
        }
    case STAGE6A:
        return 0;
    case STAGE6B:
        return 0;
    default:
        return 4;
    }
}

void GameManager::AdvanceToNextStage()
{
    switch (this->currentStage)
    {
    case STAGE1:
        this->currentStage = STAGE2;
        break;
    case STAGE2:
        this->currentStage = STAGE3;
        break;
    case STAGE3:
        switch (g_GameManager.shotType)
        {
        case SHOT_REIMU_YUKARI:
        case SHOT_REIMU:
        case SHOT_YUKARI:
            this->currentStage = STAGE4B;
            break;
        case SHOT_MARISA_ALICE:
        case SHOT_MARISA:
        case SHOT_ALICE:
            this->currentStage = STAGE4A;
            break;
        case SHOT_SAKUYA_REMILIA:
        case SHOT_SAKUYA:
        case SHOT_REMILIA:
            this->currentStage = STAGE4A;
            break;
        case SHOT_YOUMU_YUYUKO:
        case SHOT_YOUMU:
        case SHOT_YUYUKO:
            this->currentStage = STAGE4B;
            break;
        }
        break;
    case STAGE4A:
    case STAGE4B:
        this->currentStage = STAGE5;
        break;
    case STAGE5:
        this->currentStage = g_GameManager.flags.isGoingToFinalB ? STAGE6B : STAGE6A;
        break;
    case STAGE6A:
        this->currentStage = STAGE6B; // Was Kaguya meant to be a TLB at one point???
        break;
    }
}

GameManager::GameManager()
{
    memset(this, 0, sizeof(GameManager));
    this->arcadeRegionTopLeftPos.x = 32.0f;
    this->arcadeRegionTopLeftPos.y = 16.0f;
    this->arcadeRegionSize.x = 384.0f;
    this->arcadeRegionSize.y = 448.0f;
    this->currentDemoReplay = 3;
}

void GameManager::InitArcadeRegionParams()
{
    this->arcadeRegionTopLeftPos.x = 32.0f;
    this->arcadeRegionTopLeftPos.y = 16.0f;
    this->arcadeRegionSize.x = 384.0f;
    this->arcadeRegionSize.y = 448.0f;
    this->playerMovementTopLeftPos.x = 8.0f;
    this->playerMovementTopLeftPos.y = 16.0f;
    this->playerMovementAreaSize.x = 368.0f;
    this->playerMovementAreaSize.y = 416.0f;
}

}; // Namespace th08
