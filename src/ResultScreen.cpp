#include "th_pch.h"

#include "ResultScreen.hpp"
#include "ScreenEffect.hpp"

#include <time.h>

namespace th08
{

// FUNCTION: th08 0x453cd1
const char *ResultScreen::GetStageName(i32 stage)
{
    const char *name;

    if (stage >= 9)
    {
        name = (const char *)0x4b7108;
    }
    else
    {
        name = ((const char **)0x4c7fac)[stage];
    }
    return name;
}

// FUNCTION: th08 0x453cfa
const char *ResultScreen::GetCharacterName(i32 character)
{
    return ((const char **)0x4c7f4c)[character];
}

// STUB: th08 0x453d0d
void ResultScreen::WriteScore()
{
}

// STUB: th08 0x454298
void ResultScreen::LogScoreDataToFile(ResultScreen *resultScreen)
{
}

// FUNCTION: th08 0x454c59
void ResultScreen::LinkScoreEx(void *out, int difficulty, i32 character)
{
    ScoreDat::LinkScore(&this->scores[difficulty][character], (Hscr *)out);
}

// FUNCTION: th08 0x454c87
void ResultScreen::FreeScore(i32 difficulty, i32 character)
{
    ScoreDat::FreeAllScores(&this->scores[difficulty][character]);
}

// FUNCTION: th08 0x454cb2 (97% FIXME: switch 临时槽副本)
i32 ResultScreen::HandleCategorySelectScreen()
{
    i32 i;
    i32 selected;
    AnmVm *vm;

    switch (this->subState)
    {
    case 0:
        if (this->subStateTimer == 0)
        {
            vm = this->vms;

            for (i = 0; i < 0x48; i++, vm++)
            {
                vm->prefix.pendingInterrupt = 0x1;
            }

            for (i = 0; i <= 3; i++)
            {
                this->vms[i].prefix.pendingInterrupt = 0x16;
                g_AnmManager->ExecuteScript(&this->vms[i]);

                if (i == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (this->subStateTimer < 0x14)
        {
            break;
        }

        this->subState++;
        this->subStateTimer = 0;
        /* fallthrough */
    case 1:
        i = this->MoveCursor(4);

        if (i != 0)
        {
            for (i = 0; i <= 3; i++)
            {
                if (i == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (WAS_PRESSED(0x2000))
        {
            this->LogScoreDataToFile(this);
        }

        if (WAS_PRESSED(0xa))
        {
            if (this->cursor == 3)
            {
                goto case3;
            }

            this->cursor = 3;
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);

            for (i = 0; i <= 3; i++)
            {
                if (i == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (WAS_PRESSED(0x1001))
        {
            vm = this->vms;
            selected = this->cursor;

            switch (selected)
            {
            case 0:
                this->SetState((ResultScreenState)3);

                for (i = 0; i <= 3; i++)
                {
                    if (i == this->cursor)
                    {
                        this->vms[i].prefix.pendingInterrupt = 0x17;
                    }
                    else
                    {
                        this->vms[i].prefix.pendingInterrupt = 0x1;
                    }
                }
                break;
            case 1:
                this->SetState((ResultScreenState)6);

                for (i = 0; i <= 3; i++)
                {
                    if (i == this->cursor)
                    {
                        this->vms[i].prefix.pendingInterrupt = 0x17;
                    }
                    else
                    {
                        this->vms[i].prefix.pendingInterrupt = 0x1;
                    }
                }
                break;
            case 2:
                for (i = 0; i <= 3; i++)
                {
                    if (i == this->cursor)
                    {
                        this->vms[i].prefix.pendingInterrupt = 0x17;
                    }
                    else
                    {
                        this->vms[i].prefix.pendingInterrupt = 0x1;
                    }
                }

                this->SetState((ResultScreenState)0x13);
                break;
            case3:
            case 3:
                this->SetState((ResultScreenState)2);
                g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
                return 1;
            }

            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            return 1;
        }
        break;
    }

    this->subStateTimer++;

    return 0;
}

// FUNCTION: th08 0x4550b7
void ResultScreen::SetState(ResultScreenState state)
{
    this->previousScreenMode = this->screenMode;
    this->screenMode = state;
    this->stateCopy = state;
    this->subState = 0;
    this->subStateTimer = 0;
    this->screenTimer = 0;
    this->backPressed = 0;
}

// FUNCTION: th08 0x4550fc (99% FIXME: moveResult 槽/vmBase 死代码)
i32 ResultScreen::HandleHighScoreDifficultySelect()
{
    i32 i;
    i32 j;
    i32 k;
    i32 moveResult;

    switch (this->subState)
    {
    case 0:
        if (this->subStateTimer == 0)
        {
            this->cursor = this->savedDifficultyCursor;

            for (i = 4; i <= 8; i++)
            {
                this->vms[i].prefix.pendingInterrupt = 0x16;
                g_AnmManager->ExecuteScript(&this->vms[i]);

                if (i - 4 == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (this->subStateTimer < 6)
        {
            break;
        }

        this->subState++;
        this->subStateTimer = 0;
        /* fallthrough */
    case 1:
        moveResult = this->MoveCursor(5);

        if (moveResult != 0)
        {
            for (i = 4; i <= 8; i++)
            {
                if (i - 4 == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (WAS_PRESSED(0xa))
        {
            this->SetState((ResultScreenState)1);
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            this->savedDifficultyCursor = this->cursor;
            this->cursor = 0;
            return 1;
        }

        if (WAS_PRESSED(0x1001))
        {
            for (i = 4; i <= 8; i++)
            {
                if (i - 4 == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x17;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x1;
                }
            }

            this->savedDifficultyCursor = this->cursor;
            this->SetState((ResultScreenState)4);
            this->cursor = 0;
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            return 1;
        }
        break;
    }

    if (IS_PRESSED(0x4) || IS_PRESSED(0x100))
    {
        if (this->cheatCodeProgress < 4)
        {
            if (WAS_PRESSED(0x80))
            {
                this->cheatCodeProgress++;
            }
            else if (WAS_PRESSED(0x160b))
            {
                this->cheatCodeProgress = 0;
            }
        }
        else if (this->cheatCodeProgress < 5)
        {
            if (WAS_PRESSED(0x40))
            {
                this->cheatCodeProgress++;
            }
            else if (WAS_PRESSED(0x160b))
            {
                this->cheatCodeProgress = 0;
            }
        }
        else if (this->cheatCodeProgress < 7)
        {
            if (WAS_PRESSED(0x2000))
            {
                this->cheatCodeProgress++;
            }
            else if (WAS_PRESSED(0x160b))
            {
                this->cheatCodeProgress = 0;
            }
        }
        else if (this->cheatCodeProgress < 0xa)
        {
            if (WAS_PRESSED(0x200))
            {
                this->cheatCodeProgress++;
            }
            else if (WAS_PRESSED(0x160b))
            {
                this->cheatCodeProgress = 0;
            }
        }
        else
        {
            for (j = 0; j < 0xd; j++)
            {
                for (k = 0; k < 0x5; k++)
                {
                    *(u16 *)(j * 0x24 + k * 2 + 0x164b9a4) |= 0xffff;
                    *(u16 *)(j * 0x24 + k * 2 + 0x164b9ae) |= 0xffff;
                }
            }

            for (j = 0; j < 0xde; j++)
            {
                for (k = 0; k < 0xd; k++)
                {
                    (*(u32 *)(j * 0x22c + k * 4 + 0x160f66c))++;
                }
            }

            this->cheatCodeProgress = 0;
            g_SoundPlayer.PlaySoundByIdx(SOUND_1UP, 0);
        }
    }
    else
    {
        this->cheatCodeProgress = 0;
    }

    this->subStateTimer++;

    return 0;
}

// FUNCTION: th08 0x45567d
i32 ResultScreen::HandleHighScoreCharacterSelect()
{
    i32 i;

    switch (this->subState)
    {
    case 0:
        if (this->subStateTimer == 0)
        {
            this->cursor = this->savedCharacterCursor;

            for (i = 0xf; i <= 0x1a; i++)
            {
                this->vms[i].prefix.pendingInterrupt = 0x16;
                g_AnmManager->ExecuteScript(&this->vms[i]);

                if (i - 0xf == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (this->subStateTimer < 6)
        {
            break;
        }

        this->subState++;
        this->subStateTimer = 0;
        /* fallthrough */
    case 1:
        i = this->MoveCursor(0xc);

        if (i != 0)
        {
            for (i = 0xf; i <= 0x1a; i++)
            {
                if (i - 0xf == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (WAS_PRESSED(0xa))
        {
            this->SetState((ResultScreenState)3);
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            this->savedCharacterCursor = this->cursor;

            for (i = 0xf; i <= 0x1a; i++)
            {
                this->vms[i].prefix.pendingInterrupt = 0x1;
            }

            return 1;
        }

        if (WAS_PRESSED(0x1001))
        {
            AnmVm *vmBase = this->vms;

            for (i = 0xf; i <= 0x1a; i++)
            {
                if (i - 0xf == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x17;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x1;
                }
            }

            this->vms[0x28].prefix.pendingInterrupt = 0x3;
            this->savedCharacterCursor |= 0xffffffff;
            this->SetState((ResultScreenState)5);
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            return 1;
        }
        break;
    }

    this->subStateTimer++;

    return 0;
}

// FUNCTION: th08 0x455925
i32 ResultScreen::HandleHighScoreScreen()
{
    if (this->savedCharacterCursor != this->cursor && this->screenTimer == 0xa)
    {
        this->savedCharacterCursor = this->cursor;
    }

    if (this->screenTimer < 6)
    {
        return 0;
    }

    i32 sel = this->cursor;

    if (this->MoveCursorHorizontally(0xc))
    {
        this->screenTimer = 0;
        this->vms[0x28].prefix.pendingInterrupt = (u16)(this->savedDifficultyCursor + 3);
        this->vms[sel + 0xf].prefix.pendingInterrupt = 0x18;
        this->vms[this->cursor + 0xf].prefix.pendingInterrupt = 0x19;
    }

    if (WAS_PRESSED(0xa))
    {
        this->savedCharacterCursor = this->cursor;
        this->SetState((ResultScreenState)4);
        this->vms[0x28].prefix.pendingInterrupt = 1;
        g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
        return 1;
    }

    this->subStateTimer++;
    return 0;
}

// FUNCTION: th08 0x455a33
i32 ResultScreen::HandleSpellCardDifficultySelect()
{
    i32 i;

    switch (this->subState)
    {
    case 0:
        if (this->subStateTimer == 0)
        {
            this->cursor = this->savedSpellDifficultyCursor;

            for (i = 0x9; i <= 0xe; i++)
            {
                this->vms[i].prefix.pendingInterrupt = 0x16;
                g_AnmManager->ExecuteScript(&this->vms[i]);

                if (i - 0x9 == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (this->subStateTimer < 6)
        {
            break;
        }

        this->subState++;
        this->subStateTimer = 0;
        /* fallthrough */
    case 1:
        i = this->MoveCursor(6);

        if (i != 0)
        {
            for (i = 0x9; i <= 0xe; i++)
            {
                if (i - 0x9 == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (WAS_PRESSED(0xa))
        {
            this->SetState((ResultScreenState)1);
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            this->savedSpellDifficultyCursor = this->cursor;
            this->cursor = 1;
            return 1;
        }

        if (WAS_PRESSED(0x1001))
        {
            AnmVm *vmBase = this->vms;

            for (i = 0x9; i <= 0xe; i++)
            {
                if (i - 0x9 == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x17;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x1;
                }
            }

            this->savedSpellDifficultyCursor = this->cursor;
            this->SetState((ResultScreenState)7);
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            return 1;
        }
        break;
    }

    this->subStateTimer++;

    return 0;
}

// FUNCTION: th08 0x455cb0
i32 ResultScreen::HandleSpellCardCharacterSelect()
{
    i32 i;

    switch (this->subState)
    {
    case 0:
        if (this->subStateTimer == 0)
        {
            this->cursor = this->shotTypeCursor;

            for (i = 0x1b; i <= 0x27; i++)
            {
                this->vms[i].prefix.pendingInterrupt = 0x16;
                g_AnmManager->ExecuteScript(&this->vms[i]);

                if (i - 0x1b == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (this->subStateTimer < 6)
        {
            break;
        }

        this->subState++;
        this->subStateTimer = 0;
        /* fallthrough */
    case 1:
        i = this->MoveCursor(0xd);

        if (i != 0)
        {
            for (i = 0x1b; i <= 0x27; i++)
            {
                if (i - 0x1b == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x14;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x15;
                }
            }
        }

        if (WAS_PRESSED(0xa))
        {
            this->SetState((ResultScreenState)6);
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            this->shotTypeCursor = this->cursor;

            for (i = 0x1b; i <= 0x27; i++)
            {
                this->vms[i].prefix.pendingInterrupt = 0x1;
            }

            return 1;
        }

        if (WAS_PRESSED(0x1001))
        {
            AnmVm *vmBase = this->vms;

            for (i = 0x1b; i <= 0x27; i++)
            {
                if (i - 0x1b == this->cursor)
                {
                    this->vms[i].prefix.pendingInterrupt = 0x17;
                }
                else
                {
                    this->vms[i].prefix.pendingInterrupt = 0x1;
                }
            }

            this->shotTypeCursor = this->cursor;
            this->SetState((ResultScreenState)8);
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            this->vms[0x28].prefix.pendingInterrupt = 0x3;
            this->cursor = 0;
            this->savedSpellPageCursor |= 0xffffffff;
            return 1;
        }
        break;
    }

    this->subStateTimer++;

    return 0;
}

// FUNCTION: th08 0x455f6b (93% FIXME: esi 缓存除数/or 展开/分支布局)
i32 ResultScreen::HandleSpellCardScreen()
{
    i32 spellcardCount;
    i32 spellIdx;
    i32 spellcardEntry;

    if (this->backPressed != 0 && this->screenTimer >= 0xa)
    {
        this->SetState((ResultScreenState)7);
    }

    spellcardCount = *(u32 *)(this->savedSpellDifficultyCursor * 4 + 0x4c67e8);

    if (this->savedSpellPageCursor != this->cursor || this->previousShotTypeCursor != this->shotTypeCursor)
    {
        if (this->screenTimer == 0xa)
        {
            this->savedSpellPageCursor = this->cursor;
            this->previousShotTypeCursor = this->shotTypeCursor;

            for (spellIdx = this->savedSpellPageCursor * 0xa; spellIdx < this->savedSpellPageCursor * 0xa + 0xa; spellIdx++)
            {
                if (spellIdx < spellcardCount)
                {
                    spellcardEntry = *(u32 *)(*(u32 *)(this->savedSpellDifficultyCursor * 4 + 0x4c67d0) + spellIdx * 4);

                    if (*(u32 *)(spellcardEntry * 0x22c + 0x160f69c) == 0)
                    {
                        g_AnmManager->DrawTextLeft(&this->scoreVms[spellIdx % 0xa], 0xffffff, 0, (const char *)0x4b77d8);
                    }
                    else
                    {
                        g_AnmManager->DrawTextLeft(&this->scoreVms[spellIdx % 0xa], 0xffffff, 0, (const char *)(spellcardEntry * 0x22c + 0x160f558));
                    }
                }

                this->scoreVms[spellIdx % 0xa].prefix.color1.a |= 0xff;
            }

            /* 0x64 + shotTypeCursor*4 + savedSpellDifficultyCursor*0x34：得分表数组索引。 */
            g_AnmManager->DrawTextLeft(&this->textVm, 0xffffff, 0, (const char *)0x4b795c, *(u32 *)((u8 *)this + this->savedSpellDifficultyCursor * 0x34 + 0x64 + this->shotTypeCursor * 4), spellcardCount);
            this->textVm.prefix.color1.a |= 0xff;
        }
    }

    if (this->screenTimer < 6)
    {
        return 0;
    }

    if (this->MoveCursorHorizontally((spellcardCount + 9) / 0xa))
    {
        this->screenTimer = 0;
        this->vms[0x28].prefix.pendingInterrupt = 0xa;
    }
    else if (this->MoveShotTypeCursor(0xd))
    {
        this->screenTimer = 0;
        this->shotCursorMoved = 1;
        this->vms[this->previousShotTypeCursor + 0x1b].prefix.pendingInterrupt = 0x18;
        this->vms[this->shotTypeCursor + 0x1b].prefix.pendingInterrupt = 0x19;
    }

    if (WAS_PRESSED(0xa))
    {
        this->backPressed = 1;
        this->screenTimer = 0;
        g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
        this->vms[0x28].prefix.pendingInterrupt = 0x1;
        return 1;
    }

    this->subStateTimer++;

    return 0;
}

// STUB: th08 0x45621e
i32 ResultScreen::HandleResultKeyboard()
{
    return 0;
}

// FUNCTION: th08 0x456938
void ResultScreen::FormatDate(char *buffer)
{
    time_t seconds;
    tm *timeinfo;

    time(&seconds);
    timeinfo = localtime(&seconds);
    strftime(buffer, 6, "%m/%d", timeinfo);
}

// STUB: th08 0x45696f
i32 ResultScreen::HandleReplaySaveKeyboard()
{
    return 0;
}

#pragma var_order(menuTimerField, state)
ZunResult ResultScreen::CheckConfirmButton()
{
    u16 *menuTimerField;
    i32 state;

    state = this->screenMode;

    if (state == 0xf)
    {
        goto case_f;
    }
    if (state == 0x10)
    {
        goto case_10;
    }
    goto end;

case_f:
    if (this->screenTimer <= 0x1e)
    {
        menuTimerField = (u16 *)&this->vms[0x47];
        menuTimerField[0xff] = 0x12;
    }

    if (this->screenTimer >= 0x5a && WAS_PRESSED(0x1001))
    {
        menuTimerField = (u16 *)&this->vms[0x47];
        menuTimerField[0xff] = 0x2;
        this->screenTimer = 0;
        this->screenMode = 0x10;
    }
    goto end;

case_10:
    if (this->screenTimer >= 0x1e)
    {
        this->screenTimer = 9;
        this->screenMode = 0xa;
    }

end:
    return ZUN_SUCCESS;
}

// STUB: th08 0x4578aa
i32 ResultScreen::HandleOtherStatsScreen()
{
    return 0;
}

// STUB: th08 0x457e00
i32 ResultScreen::DrawFinalStats()
{
    return 0;
}

// STUB: th08 0x4582a0
// FUNCTION: th08 0x4582a0
ZunResult ResultScreen::RegisterChain(u32 unk)
{
    ResultScreen *resultScreen =
        (ResultScreen *)g_ZunMemory.AddToRegistry(new ResultScreen(), sizeof(ResultScreen), "ResultSysInf");
    g_ScreenEffectCounter = 0;
    utils::GuiDebugPrint("Stg.PlayTimeAll = %d\r\n", g_GameManager.unk3de04);

    if (unk == 1)
    {
        if (!g_GameManager.IsPracticeMode())
        {
            resultScreen->screenMode = 9;
        }
        else if (g_GameManager.flags.isSpellPractice)
        {
            resultScreen->screenMode = 0x16;
        }
        else
        {
            resultScreen->screenMode = 0x11;
        }
    }
    else if (unk == 2)
    {
        resultScreen->screenMode = 0x12;
        resultScreen->AddedCallback(resultScreen);
        return ZUN_SUCCESS;
    }

    resultScreen->calcChain = g_Chain.CreateElem((ChainCallback)OnUpdate);
    resultScreen->calcChain->addedCallback = (ChainLifetimeCallback)AddedCallback;
    resultScreen->calcChain->deletedCallback = (ChainLifetimeCallback)DeletedCallback;
    resultScreen->calcChain->arg = resultScreen;
    if (g_Chain.AddToCalcChain(resultScreen->calcChain, 0x10))
    {
        return ZUN_ERROR;
    }

    resultScreen->drawChain = g_Chain.CreateElem((ChainCallback)OnDraw);
    resultScreen->drawChain->arg = resultScreen;
    g_Chain.AddToDrawChain(resultScreen->drawChain, 0x12);

    return ZUN_SUCCESS;
}

// STUB: th08 0x4584b0
ChainCallbackResult ResultScreen::OnUpdate(ResultScreen *resultScreen)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// STUB: th08 0x4586b4
ChainCallbackResult ResultScreen::OnDraw(ResultScreen *resultScreen)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// STUB: th08 0x45964d
ZunResult ResultScreen::AddedCallback(ResultScreen *resultScreen)
{
    return ZUN_SUCCESS;
}

// FUNCTION: th08 0x459fd2
ZunResult ResultScreen::DeletedCallback(ResultScreen *resultScreen)
{
    if (resultScreen->scoreData != NULL)
    {
        resultScreen->WriteScore();
        ScoreDat::ReleaseScore((ScoreDat *)resultScreen->scoreData);
    }
    resultScreen->scoreData = NULL;

    for (i32 i = 0; i < 5; i++)
    {
        for (i32 j = 0; j < 12; j++)
        {
            resultScreen->FreeScore(i, j);
        }
    }

    g_AnmManager->ReleaseAnm(0x15);
    g_AnmManager->ReleaseAnm(0x16);
    g_AnmManager->ReplaceSurface(8, 0);

    g_Chain.Cut(resultScreen->drawChain);
    resultScreen->drawChain = NULL;

    g_ZunMemory.RemoveFromRegistry(resultScreen);

    delete resultScreen;
    resultScreen = NULL;

    return ZUN_SUCCESS;
}

// FUNCTION: th08 0x45a0f4
i32 ResultScreen::MoveCursor(i32 length)
{
    if (WAS_PRESSED_SCROLLING(0x10))
    {
        this->cursor--;
        if (this->cursor < 0)
        {
            this->cursor += length;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return -1;
    }

    if (WAS_PRESSED_SCROLLING(0x20))
    {
        this->cursor++;
        if (this->cursor >= length)
        {
            this->cursor -= length;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return 1;
    }

    return 0;
}

// FUNCTION: th08 0x45a1f3
i32 ResultScreen::MoveShotTypeCursor(i32 length)
{
    if (WAS_PRESSED_SCROLLING(0x10))
    {
        this->shotTypeCursor--;
        if (this->shotTypeCursor < 0)
        {
            this->shotTypeCursor += length;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return -1;
    }

    if (WAS_PRESSED_SCROLLING(0x20))
    {
        this->shotTypeCursor++;
        if (this->shotTypeCursor >= length)
        {
            this->shotTypeCursor -= length;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return 1;
    }

    return 0;
}

// FUNCTION: th08 0x45a2f2
i32 ResultScreen::MoveCursorHorizontally(i32 length)
{
    if (WAS_PRESSED_SCROLLING(0x40))
    {
        this->cursor--;
        if (this->cursor < 0)
        {
            this->cursor += length;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return -1;
    }

    if (WAS_PRESSED_SCROLLING(0x80))
    {
        this->cursor++;
        if (this->cursor >= length)
        {
            this->cursor -= length;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return 1;
    }

    return 0;
}

} /* namespace th08 */
