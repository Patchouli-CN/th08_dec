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
void FUN_00438046();

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

DIFFABLE_STATIC(u32, g_CurFrameCount);            // 0x164d09c
DIFFABLE_STATIC(i32, g_GameManagerState);         // 0x164d0a4
DIFFABLE_STATIC(u16, g_StageClearFlag);           // 0x164d2d8 (bit mask of the cleared stage)
DIFFABLE_STATIC(f32, g_CsumFloat);                // 0x164d2fc (anti-tamper integrity float)
DIFFABLE_STATIC_ARRAY(u16, 5, g_GlobalNoRetryClears);   // 0x164bb54 (no-continue clear flags per difficulty)
DIFFABLE_STATIC_ARRAY(u16, 5, g_GlobalWithRetryClears); // 0x164bb5e (with-continue clear flags per difficulty)
DIFFABLE_STATIC_ARRAY(ClearCountEntry, 5, g_ClearCountByDifficulty); // 0x164cd70 (per-difficulty clear counters, 0x44 stride)
DIFFABLE_STATIC(u32, g_ScreenClearColor);         // 0x4e4b24

DIFFABLE_STATIC_ARRAY_ASSIGN(StageBgmEntry, 9, g_StageBgmTable) = {
    {1, 2, 0},  {3, 4, 0},  {5, 6, 0},  {7, 8, 0},  {7, 9, 0},
    {10, 11, 0}, {12, 13, 15}, {12, 14, 15}, {16, 17, 0},
};

// FUNCTION: th08 0x439bc7 (98.26% FIXME: /Os 跳转 trampoline —— 原版直接 jcc，
// MSVC 对 `if (cond) goto` 生成 jcc+jmp；cameraMode u8 |= 0xff 生成 movzbl+or+mov
// 而非原版 or byte；找角色循环原版 setne dl+test edx。均系统性不可修。)
#pragma var_order(foundIdx, nextIdx, clockTimeTmp, bgmIdx, anmManager, csum, i, notInMenu, scoreIncrement)
ChainCallbackResult GameManager::OnUpdate(GameManager *gameManager)
{
    i32 foundIdx;
    i32 nextIdx;
    i8 clockTimeTmp;
    i32 bgmIdx;
    AnmManager *anmManager;
    i32 csum;
    i32 i;
    i32 notInMenu;
    u32 scoreIncrement;

    g_CurFrameCount++;

    if (gameManager->flags.unk5 != 0)
    {
        if (gameManager->flags.unk5 == 2)
        {
            // Stage clear: write the clear records for this stage/difficulty.
            gameManager->flags.unk5 = 3;
            g_GameManager.unk38 = 1;
            g_GameManagerState |= -1;
            if (!((g_PlayerFlags >> PLAYER_FLAG_SWAP_MODE_BIT) & 1))
            {
                if (g_GameManager.globals->numRetries == 0)
                {
                    g_GameManager.clrdData[g_PlayerCharacter].difficultiesClearedWithoutRetries[g_GameManager.difficulty] |=
                        g_StageClearFlag;
                    g_GlobalNoRetryClears[g_GameManager.difficulty] |= g_StageClearFlag;
                }
                g_GameManager.clrdData[g_PlayerCharacter].difficultiesClearedWithRetries[g_GameManager.difficulty] |=
                    g_StageClearFlag;
                g_GlobalWithRetryClears[g_GameManager.difficulty] |= g_StageClearFlag;
            }
            gameManager->globals->displayScore = gameManager->globals->score;
            if (gameManager->flags.isPracticeMode)
            {
                g_GameManager.globals->displayScore = g_GameManager.globals->score;
                g_GameManagerState = 6;
                return CHAIN_CALLBACK_RESULT_BREAK;
            }
            if (g_Unknown164d2cc == GAME_STATE_EVENT_6 || g_Unknown164d2cc == GAME_STATE_EVENT_7 || g_Unknown164d2cc == GAME_STATE_EVENT_8)
            {
                goto stageE34;
            }
            if ((g_PlayerFlags >> PLAYER_FLAG_SWAP_MODE_BIT) & 1)
            {
                foundIdx = 0;
                for (nextIdx = g_Unknown164d2cc + 1; nextIdx < STAGE_PROGRESS_MAX; nextIdx++)
                {
                    if (g_ReplayManager->unk8->header.stageReplayData[nextIdx] != NULL)
                    {
                        foundIdx = nextIdx;
                        break;
                    }
                }
                if (foundIdx == 0)
                {
                    g_Supervisor.curState = 7;
                }
                else
                {
                    g_Unknown164d2cc = foundIdx;
                    g_Supervisor.curState = 3;
                }
                goto stageE2f;
            }
            clockTimeTmp = g_GameManager.globals->clockTime;
            if (clockTimeTmp >= CLOCK_TIME_12H_LIMIT)
            {
                g_PlayerFlags &= ~ZUN_BIT(PLAYER_FLAG_FORCE_SWITCH_BIT);
                g_GameManagerState = 9;
                return CHAIN_CALLBACK_RESULT_BREAK;
            }
            g_GameManager.AdvanceToNextStage();
            g_Supervisor.curState = 3;
            goto stageE2f;

        stageE34:
            if ((g_PlayerFlags >> PLAYER_FLAG_SWAP_MODE_BIT) & 1)
            {
                g_GameManagerState = 7;
            }
            else
            {
                if (g_GameManager.difficulty >= 4)
                {
                    if (g_GameManager.difficulty == 4)
                    {
                        g_GameManager.clrdData[g_PlayerCharacter].difficultiesClearedWithoutRetries[g_GameManager.difficulty] |= EXTRA_STAGE_CLEARED_FLAG;
                        g_GlobalWithRetryClears[g_GameManager.difficulty] |= EXTRA_STAGE_CLEARED_FLAG;
                    }
                    g_ClearCountByDifficulty[g_GameManager.difficulty].count++;
                    g_PlayerFlags |= ZUN_BIT(PLAYER_FLAG_FORCE_SWITCH_BIT);
                    g_GameManager.globals->displayScore = g_GameManager.globals->score;
                    g_GameManagerState = 6;
                    return CHAIN_CALLBACK_RESULT_BREAK;
                }
                g_PlayerFlags |= ZUN_BIT(PLAYER_FLAG_FORCE_SWITCH_BIT);
                g_GameManagerState = 9;
                return CHAIN_CALLBACK_RESULT_BREAK;
            }
            goto stageE2f;
        }
        else
        {
            goto stage1;
        }
    }
    else
    {
        goto skipStageComplete;
    }

stageE2f:
    if (g_GameManagerState < 0)
    {
        g_Gui.FUN_00438f58();
    }

stage1:
    // Pause handling. The pause key (0x1001 = pause + up) pauses right away when
    // pressed, or whenever the game is in a special mode (character switch etc).
    if ((g_CurFrameInput & PAUSE_KEY_MASK) != 0)
    {
        if ((g_CurFrameInput & PAUSE_KEY_MASK) == (g_LastFrameInput & PAUSE_KEY_MASK))
        {
            // Same state as last frame: continue to noPauseKey.
        }
        else
        {
            goto pause;
        }
    }

noPauseKey:
    if (!((g_PlayerFlags >> PLAYER_FLAG_SWAP_MODE_BIT) & 1))
    {
        if (g_Unknown164d2cc != GAME_STATE_EVENT_6)
        {
            if (g_Unknown164d2cc != GAME_STATE_EVENT_7)
            {
                if (g_Unknown164d2cc == GAME_STATE_EVENT_8)
                {
                    // Fall through into the pause block when in mode 8.
                }
                else
                {
                    goto skipStageComplete;
                }
            }
            else
            {
                goto pause;
            }
        }
        else
        {
            goto pause;
        }
    }
    else
    {
        goto pause;
    }

pause:
    gameManager->flags.unk5 = 0;
    if (g_GameManagerState >= 0)
    {
        g_Supervisor.curState = g_GameManagerState;
    }

skipStageComplete:
    if (gameManager->unk38 != 0)
    {
        if (gameManager->unk38 == 2)
        {
            return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
        }
        gameManager->unk3c++;
        return CHAIN_CALLBACK_RESULT_BREAK;
    }

    if (gameManager->unk3de28 != 0)
    {
        FUN_00438046();
        g_AnmManager->ReleaseSurface(8);
        g_Supervisor.loadingVmsHaveBeenSetup = 0;
        if (gameManager->unk3de28 == 1)
        {
            if (!((g_PlayerFlags >> PLAYER_FLAG_EXTRA_BIT) & 1))
            {
                g_Supervisor.PlayMusic(0, g_StageBgmTable[g_Unknown164d2cc].stageBgm);
            }
            else
            {
                for (bgmIdx = 0; ; bgmIdx++)
                {
                    if (g_SpellcardBgmTable[bgmIdx].maxSpellcardNumber < 0)
                    {
                        break;
                    }
                    if (g_CurrentSpellcardNumber <= g_SpellcardBgmTable[bgmIdx].maxSpellcardNumber)
                    {
                        g_Supervisor.PlayMusic(0, g_SpellcardBgmTable[bgmIdx].bgmIndex);
                        break;
                    }
                }
            }
        }
        gameManager->unk3de28 = 0;
    }

    // Focus (slow) mode toggled on this frame: snap the player box and remember
    // the RNG state around the pause so the game continues deterministically.
    if (gameManager->showRetryMenu == 0 && gameManager->isInGameMenu == 0 &&
        gameManager->flags.isDemoMode == 0 && gameManager->unk2D == 0)
    {
        if ((g_CurFrameInput & FOCUS_KEY_BIT) != 0 && (g_CurFrameInput & FOCUS_KEY_BIT) != (g_LastFrameInput & FOCUS_KEY_BIT))
        {
            gameManager->isInGameMenu = 1;
            g_PlayerPos.x = 32.0f;
            g_PlayerPos.y = 16.0f;
            g_PlayerTargetX = 384.0f;
            g_PlayerTargetY = 448.0f;
            gameManager->unk3D298 = 1;
            g_SoundPlayer.QueueCommand(6, 0, "Pause");
            g_SoundPlayer.PlaySoundByIdx(SOUND_PAUSE, 0);
            g_Supervisor.UpdateGameTime(&g_Supervisor);
            g_Rng.seedBackup = g_Rng.seed;
            gameManager->hscr.numPauses++;
            g_GameManager.UpdateAntiTamper();
            g_Rng.seed = g_Rng.seedBackup;
        }
    }

    g_Supervisor.viewport.X = gameManager->arcadeRegionTopLeftPos.x;
    g_Supervisor.viewport.Y = gameManager->arcadeRegionTopLeftPos.y;
    g_Supervisor.viewport.Width = gameManager->arcadeRegionSize.x;
    g_Supervisor.viewport.Height = gameManager->arcadeRegionSize.y;
    g_Supervisor.viewport.MinZ = 0.0f;
    g_Supervisor.viewport.MaxZ = 1.0f;
    anmManager = g_AnmManager;
    anmManager->cameraMode |= 0xff;

    // In the swap mode with a single player alive, drop frames so the game stays
    // at a manageable speed depending on the current FPS.
    if ((g_PlayerFlags & PLAYER_FLAG_SWAP_MODE_BIT) && g_PlayerUnknown0bd == 1)
    {
        if (g_Gui.FUN_004358bb() == 0)
        {
            gameManager->unk3de08++;
            if ((g_Supervisor.curFps < 20 && gameManager->unk3de08 % 3 != 0) ||
                (g_Supervisor.curFps >= 20 && g_Supervisor.curFps < 30 && gameManager->unk3de08 % 2 != 0) ||
                (g_Supervisor.curFps >= 30 && g_Supervisor.curFps < 40 && gameManager->unk3de08 % 3 == 0) ||
                (g_Supervisor.curFps >= 40 && g_Supervisor.curFps < 50 && gameManager->unk3de08 % 6 == 0))
            {
                return CHAIN_CALLBACK_RESULT_BREAK;
            }
        }
    }

    // Replay / demo auto-play: advance the demo counter and exit the demo when it
    // has run long enough (or the player pressed any button).
    if (gameManager->flags.isDemoMode)
    {
        if ((g_CurFrameInput & ANY_INPUT_MASK) != 0 && (g_CurFrameInput & ANY_INPUT_MASK) != (g_LastFrameInput & ANY_INPUT_MASK))
        {
            g_Supervisor.curState = 1;
        }
        gameManager->demoFrameCount++;
        if ((gameManager->currentDemoReplay == 0 && gameManager->demoFrameCount == DEMO_BOMB_FRAME_0) ||
            (gameManager->currentDemoReplay == 1 && gameManager->demoFrameCount == DEMO_BOMB_FRAME_1) ||
            (gameManager->currentDemoReplay == 2 && gameManager->demoFrameCount == DEMO_BOMB_FRAME_2) ||
            (gameManager->currentDemoReplay == 3 && gameManager->demoFrameCount == DEMO_BOMB_FRAME_3))
        {
            ScreenEffect::RegisterChain(SCREEN_EFFECT_ARCADE_FADE_OUT, 0x78, 0, 0, 0, 0x15);
            g_Supervisor.FadeOutMusic(3.0f);
        }
        if ((gameManager->currentDemoReplay == 0 && gameManager->demoFrameCount >= DEMO_END_FRAME_0) ||
            (gameManager->currentDemoReplay == 1 && gameManager->demoFrameCount >= DEMO_END_FRAME_1) ||
            (gameManager->currentDemoReplay == 2 && gameManager->demoFrameCount >= DEMO_END_FRAME_2) ||
            (gameManager->currentDemoReplay == 3 && gameManager->demoFrameCount == DEMO_END_FRAME_3))
        {
            g_Supervisor.curState = 1;
            return CHAIN_CALLBACK_RESULT_BREAK;
        }
    }

    // Anti-tamper: recompute the checksum and verify the RNG registers are still
    // in the expected range; an out-of-range value flags the game as tampered.
    g_GameManager.globals->antiTamperValue = g_GameManager.globals->rng1[2];
    csum = gameManager->CalcAntiTamperChecksum();
    g_CsumFloat = (f32)csum + (f32)g_GameManager.globals->rng7[3];

    for (i = 0; i < 7u; i++)
    {
        if (gameManager->globals->rng1[i] < ANTITAMPER_RANGE_MIN || gameManager->globals->rng1[i] > ANTITAMPER_RANGE_MAX)
        {
            g_CsumFloat = -9999.0f;
        }
    }
    for (i = 0; i < 2u; i++)
    {
        if (gameManager->globals->rng3[i] < ANTITAMPER_RANGE_MIN_FLOAT || gameManager->globals->rng3[i] > ANTITAMPER_RANGE_MAX_FLOAT)
        {
            g_CsumFloat = -9999.0f;
        }
    }

    gameManager->flags.unk2 = !gameManager->showRetryMenu && !gameManager->isInGameMenu;

    for (i = 0; i < 2u; i++)
    {
        if (gameManager->globals->rng2[i] < ANTITAMPER_RANGE_MIN_FLOAT || gameManager->globals->rng2[i] > ANTITAMPER_RANGE_MAX_FLOAT)
        {
            g_CsumFloat = -9999.0f;
        }
    }
    for (i = 0; i < 8u; i++)
    {
        if (gameManager->globals->rng7[i] < ANTITAMPER_RANGE_MIN || gameManager->globals->rng7[i] > ANTITAMPER_RANGE_MAX)
        {
            g_CsumFloat = -9999.0f;
        }
    }

    g_Supervisor.d3dDevice->Clear(0, NULL, 2, g_ScreenClearColor, 1.0f, 0);

    if (gameManager->isInGameMenu == 1 || gameManager->isInGameMenu == 2 || gameManager->showRetryMenu)
    {
        return CHAIN_CALLBACK_RESULT_BREAK;
    }

    // Animate the displayed score towards the actual score.
    if (gameManager->globals->score >= SCORE_DISPLAY_CAP)
    {
        gameManager->globals->score = SCORE_DISPLAY_CAP_ONE_LESS;
    }
    if (gameManager->globals->displayScore != gameManager->globals->score)
    {
        if (gameManager->globals->score < gameManager->globals->displayScore)
        {
            gameManager->globals->score = gameManager->globals->displayScore;
        }
        scoreIncrement = (gameManager->globals->score - gameManager->globals->displayScore) >> 5;
        if (scoreIncrement >= SCORE_INCREMENT_CAP)
        {
            scoreIncrement = SCORE_INCREMENT_CAP;
        }
        else if (scoreIncrement == 0)
        {
            scoreIncrement = 1;
        }
        if (gameManager->globals->unk0x10 < scoreIncrement)
        {
            gameManager->globals->unk0x10 = scoreIncrement;
        }
        if (gameManager->globals->displayScore + gameManager->globals->unk0x10 > gameManager->globals->score)
        {
            gameManager->globals->unk0x10 = gameManager->globals->score - gameManager->globals->displayScore;
        }
        gameManager->globals->displayScore = gameManager->globals->displayScore + gameManager->globals->unk0x10;
        if (gameManager->globals->displayScore >= gameManager->globals->score)
        {
            gameManager->globals->unk0x10 = 0;
            gameManager->globals->displayScore = gameManager->globals->score;
        }
        if (gameManager->globals->displayedHighScore < gameManager->globals->displayScore)
        {
            gameManager->globals->displayedHighScore = gameManager->globals->displayScore;
            gameManager->globals->ontinuesUsedInHighScore = gameManager->globals->numRetries;
        }
    }

    for (i = 0; i < 3u; i++)
    {
        if (gameManager->globals->rng4[i] < ANTITAMPER_RANGE_MIN_FLOAT || gameManager->globals->rng4[i] > ANTITAMPER_RANGE_MAX_FLOAT)
        {
            g_CsumFloat = -9999.0f;
        }
    }
    for (i = 0; i < 2u; i++)
    {
        if (gameManager->globals->rng5[i] < ANTITAMPER_RANGE_MIN_FLOAT || gameManager->globals->rng5[i] > ANTITAMPER_RANGE_MAX_FLOAT)
        {
            g_CsumFloat = -9999.0f;
        }
    }
    for (i = 0; i < 5u; i++)
    {
        if (gameManager->globals->rng8[i] < ANTITAMPER_RANGE_MIN || gameManager->globals->rng8[i] > ANTITAMPER_RANGE_MAX)
        {
            g_CsumFloat = -9999.0f;
        }
    }

    // Global slow-mode: with lots of bullets on screen, skip frames.
    if (g_GameManager.cfg->slowMode != 0)
    {
        g_GameManager.unk2D = 0;
        gameManager->unk3de08++;
        if ((g_BulletCount >= SLOW_BULLET_COUNT_MAX && gameManager->unk3de08 % 3 == 0) ||
            (g_BulletCount < SLOW_BULLET_COUNT_MAX && g_BulletCount >= SLOW_BULLET_COUNT_MID && gameManager->unk3de08 % 4 == 0) ||
            (g_BulletCount < SLOW_BULLET_COUNT_MID && g_BulletCount >= SLOW_BULLET_COUNT_MIN && gameManager->unk3de08 % 5 == 0))
        {
            g_GameManager.unk2D = 1;
            return CHAIN_CALLBACK_RESULT_BREAK;
        }
        if (g_BulletCount < SLOW_BULLET_COUNT_MIN)
        {
            gameManager->unk3de08 = 0;
        }
    }

    g_GameManager.IsTampered();
    gameManager->unk3ddc0++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

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
    gameManager->flags.unk5 = 0;
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

// FUNCTION: th08 0x406d70 (FIXME: /Os leave epilogue vs 原版 mov/pop)
i32 GameManager::GaugeIsExtremelyYoukai()
{
    return this->globals->youkaiGauge >= this->youkaiGaugeYoukaiLimit;
}

// FUNCTION: th08 0x406da0
i32 GameManager::GaugeIsModeratelyYoukai()
{
    return this->globals->youkaiGauge >= this->youkaiGaugeYoukaiEffectsThreshold;
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

i32 GameManager::GaugeIsModeratelyHuman()
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


// STUB: th08 0x437f5c (spellcard collect screen)
void __fastcall FUN_00437f5c(i32 param)
{
}

// FUNCTION: th08 0x438046 (93.46% FIXME: switch 判别式槽 + default case 位置)
void FUN_00438046()
{
    i32 state = g_Unknown164d2cc;
    switch (state - 1)
    {
    case 0:
        FUN_00437f5c(0x11);
        break;
    case 1:
        FUN_00437f5c(0x12);
        break;
    case 2:
        if (g_GameManager.GetFlag14() != 0 && g_GameManager.IsSpellNumberEqualTo(0xd6) != 0)
            FUN_00437f5c(0x13);
        else if (g_GameManager.IsSpellNumberEqualTo(0xd8) != 0)
            FUN_00437f5c(0x1a);
        else if (g_GameManager.IsSpellNumberEqualTo(0xd9) != 0)
            FUN_00437f5c(0x1b);
        else if (g_GameManager.IsSpellNumberEqualTo(0xda) != 0)
            FUN_00437f5c(0x1c);
        else if (g_GameManager.IsSpellNumberEqualTo(0xdb) != 0)
            FUN_00437f5c(0x1d);
        else if (g_GameManager.IsSpellNumberEqualTo(0xdc) != 0)
            FUN_00437f5c(0x1e);
        else if (g_GameManager.IsSpellNumberEqualTo(0xdd) != 0)
            FUN_00437f5c(0x1f);
        break;
    case 3:
        FUN_00437f5c(0x14);
        break;
    case 4:
        if (g_GameManager.GetFlag14() != 0 && g_GameManager.IsSpellNumberEqualTo(0xd4) != 0)
            FUN_00437f5c(0x15);
        else
            FUN_00437f5c(0x16);
        break;
    case 5:
        FUN_00437f5c(0x17);
        break;
    case 6:
        if (g_GameManager.GetFlag14() != 0)
        {
            if (g_GameManager.IsSpellNumberInRange(0x93, 0x96) != 0)
                FUN_00437f5c(0x17);
            else
                FUN_00437f5c(0x18);
        }
        else
            FUN_00437f5c(0x17);
        break;
    case 7:
        if (g_GameManager.GetFlag14() != 0 && g_GameManager.IsSpellNumberInRange(0xbf, 0xc1) == 0 &&
            g_GameManager.IsSpellNumberEqualTo(0xd5) != 0)
            FUN_00437f5c(0x19);
        else
            FUN_00437f5c(0x20);
        break;
    default:
        FUN_00437f5c(0x10);
        break;
    }
}

}; // Namespace th08
