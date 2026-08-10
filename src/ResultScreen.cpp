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

// STUB: th08 0x454cb2
void ResultScreen::HandleCategorySelectScreen()
{
}

// FUNCTION: th08 0x4550b7
void ResultScreen::SetState(ResultScreenState state)
{
    this->unk14 = this->screenMode;
    this->screenMode = state;
    this->unk0c = state;
    this->unk10 = 0;
    this->unk18 = 0;
    this->unk4 = 0;
    this->unk54 = 0;
}

// FUNCTION: th08 0x4550fc (99% FIXME: moveResult 槽/vmBase 死代码)
i32 ResultScreen::HandleHighScoreDifficultySelect()
{
    i32 i;
    i32 j;
    i32 k;
    i32 moveResult;

    switch (this->unk10)
    {
    case 0:
        if (this->unk18 == 0)
        {
            this->unk1c = this->unk44;

            for (i = 4; i <= 8; i++)
            {
                *(u16 *)((u8 *)this + 0x39e + i * 0x2a4) = 0x16;
                g_AnmManager->ExecuteScript((AnmVm *)((u8 *)this + 0x1a0 + i * 0x2a4));

                if (i - 4 == this->unk1c)
                {
                    *(u16 *)((u8 *)this + 0x39e + i * 0x2a4) = 0x14;
                }
                else
                {
                    *(u16 *)((u8 *)this + 0x39e + i * 0x2a4) = 0x15;
                }
            }
        }

        if (this->unk18 < 6)
        {
            break;
        }

        this->unk10++;
        this->unk18 = 0;
        /* fallthrough */
    case 1:
        moveResult = this->MoveCursor(5);

        if (moveResult != 0)
        {
            for (i = 4; i <= 8; i++)
            {
                if (i - 4 == this->unk1c)
                {
                    *(u16 *)((u8 *)this + 0x39e + i * 0x2a4) = 0x14;
                }
                else
                {
                    *(u16 *)((u8 *)this + 0x39e + i * 0x2a4) = 0x15;
                }
            }
        }

        if (WAS_PRESSED(0xa))
        {
            this->SetState((ResultScreenState)1);
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            this->unk44 = this->unk1c;
            this->unk1c = 0;
            return 1;
        }

        if (WAS_PRESSED(0x1001))
        {
            for (i = 4; i <= 8; i++)
            {
                if (i - 4 == this->unk1c)
                {
                    *(u16 *)((u8 *)this + 0x39e + i * 0x2a4) = 0x17;
                }
                else
                {
                    *(u16 *)((u8 *)this + 0x39e + i * 0x2a4) = 0x1;
                }
            }

            this->unk44 = this->unk1c;
            this->SetState((ResultScreenState)4);
            this->unk1c = 0;
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            return 1;
        }
        break;
    }

    if (IS_PRESSED(0x4) || IS_PRESSED(0x100))
    {
        if (this->unk4c < 4)
        {
            if (WAS_PRESSED(0x80))
            {
                this->unk4c++;
            }
            else if (WAS_PRESSED(0x160b))
            {
                this->unk4c = 0;
            }
        }
        else if (this->unk4c < 5)
        {
            if (WAS_PRESSED(0x40))
            {
                this->unk4c++;
            }
            else if (WAS_PRESSED(0x160b))
            {
                this->unk4c = 0;
            }
        }
        else if (this->unk4c < 7)
        {
            if (WAS_PRESSED(0x2000))
            {
                this->unk4c++;
            }
            else if (WAS_PRESSED(0x160b))
            {
                this->unk4c = 0;
            }
        }
        else if (this->unk4c < 0xa)
        {
            if (WAS_PRESSED(0x200))
            {
                this->unk4c++;
            }
            else if (WAS_PRESSED(0x160b))
            {
                this->unk4c = 0;
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

            this->unk4c = 0;
            g_SoundPlayer.PlaySoundByIdx(SOUND_1UP, 0);
        }
    }
    else
    {
        this->unk4c = 0;
    }

    this->unk18++;

    return 0;
}

// STUB: th08 0x45567d
i32 ResultScreen::HandleHighScoreCharacterSelect()
{
    return 0;
}

// FUNCTION: th08 0x455925
i32 ResultScreen::HandleHighScoreScreen()
{
    if (this->unk3c != this->unk1c && this->unk4 == 0xa)
    {
        this->unk3c = this->unk1c;
    }

    if (this->unk4 < 6)
    {
        return 0;
    }

    i32 sel = this->unk1c;

    if (this->MoveCursorHorizontally(0xc))
    {
        this->unk4 = 0;
        *(u16 *)((u8 *)this + 0x6d3e) = (u16)(this->unk44 + 3);
        *(u16 *)((u8 *)this + 0x39e + (sel + 0xf) * 0x2a4) = 0x18;
        *(u16 *)((u8 *)this + 0x39e + (this->unk1c + 0xf) * 0x2a4) = 0x19;
    }

    if (WAS_PRESSED(0xa))
    {
        this->unk3c = this->unk1c;
        this->SetState((ResultScreenState)4);
        *(u16 *)((u8 *)this + 0x6d3e) = 1;
        g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
        return 1;
    }

    this->unk18++;
    return 0;
}

// FUNCTION: th08 0x455a33
i32 ResultScreen::HandleSpellCardDifficultySelect()
{
    i32 state = this->unk10;

    if (state == 0)
    {
        goto case0;
    }
    else if (state == 1)
    {
        goto case1;
    }
    goto end;

case0:
    if (this->unk18 == 0)
    {
        this->unk1c = this->unk48;
        for (i32 i = 9; i <= 0xe; i++)
        {
            *(u16 *)((u8 *)this + 0x39e + i * 0x2a4) = 0x16;
            g_AnmManager->ExecuteScript((AnmVm *)((u8 *)this + 0x1a0 + i * 0x2a4));
            if (i - 9 == this->unk1c)
            {
                *(u16 *)((u8 *)this + 0x39e + i * 0x2a4) = 0x14;
            }
            else
            {
                *(u16 *)((u8 *)this + 0x39e + i * 0x2a4) = 0x15;
            }
        }
    }

    if (this->unk18 >= 6)
    {
        this->unk10++;
        this->unk18 = 0;
        goto case1;
    }
    goto end;

case1:
{
    i32 ret = this->MoveCursor(6);
    if (ret != 0)
    {
        for (i32 i = 9; i <= 0xe; i++)
        {
            if (i - 9 == this->unk1c)
            {
                *(u16 *)((u8 *)this + 0x39e + i * 0x2a4) = 0x14;
            }
            else
            {
                *(u16 *)((u8 *)this + 0x39e + i * 0x2a4) = 0x15;
            }
        }
    }

    if (WAS_PRESSED(0xa))
    {
        this->SetState((ResultScreenState)1);
        g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
        this->unk48 = this->unk1c;
        this->unk1c = 1;
        return 1;
    }

    if (WAS_PRESSED(0x1001))
    {
        for (i32 i = 9; i <= 0xe; i++)
        {
            if (i - 9 == this->unk1c)
            {
                *(u16 *)((u8 *)this + 0x39e + i * 0x2a4) = 0x17;
            }
            else
            {
                *(u16 *)((u8 *)this + 0x39e + i * 0x2a4) = 0x1;
            }
        }
        this->unk48 = this->unk1c;
        this->SetState((ResultScreenState)7);
        g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
        return 1;
    }
    goto end;
}

end:
    this->unk18++;
    return 0;
}

// STUB: th08 0x455cb0
i32 ResultScreen::HandleSpellCardCharacterSelect()
{
    return 0;
}

// STUB: th08 0x455f6b
i32 ResultScreen::HandleSpellCardScreen()
{
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
    if (this->unk4 <= 0x1e)
    {
        menuTimerField = (u16 *)((u8 *)this + 0xbd1c);
        menuTimerField[0xff] = 0x12;
    }

    if (this->unk4 >= 0x5a && WAS_PRESSED(0x1001))
    {
        menuTimerField = (u16 *)((u8 *)this + 0xbd1c);
        menuTimerField[0xff] = 0x2;
        this->unk4 = 0;
        this->screenMode = 0x10;
    }
    goto end;

case_10:
    if (this->unk4 >= 0x1e)
    {
        this->unk4 = 9;
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
    if (resultScreen->unk0 != NULL)
    {
        resultScreen->WriteScore();
        ScoreDat::ReleaseScore((ScoreDat *)resultScreen->unk0);
    }
    resultScreen->unk0 = NULL;

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
        this->unk1c--;
        if (this->unk1c < 0)
        {
            this->unk1c += length;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return -1;
    }

    if (WAS_PRESSED_SCROLLING(0x20))
    {
        this->unk1c++;
        if (this->unk1c >= length)
        {
            this->unk1c -= length;
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
        this->unk30--;
        if (this->unk30 < 0)
        {
            this->unk30 += length;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return -1;
    }

    if (WAS_PRESSED_SCROLLING(0x20))
    {
        this->unk30++;
        if (this->unk30 >= length)
        {
            this->unk30 -= length;
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
        this->unk1c--;
        if (this->unk1c < 0)
        {
            this->unk1c += length;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return -1;
    }

    if (WAS_PRESSED_SCROLLING(0x80))
    {
        this->unk1c++;
        if (this->unk1c >= length)
        {
            this->unk1c -= length;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return 1;
    }

    return 0;
}

} /* namespace th08 */
