#include "th_pch.h"

#include "ResultScreen.hpp"
#include "ScreenEffect.hpp"

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

// STUB: th08 0x4550fc
i32 ResultScreen::HandleHighScoreDifficultySelect()
{
    return 0;
}

// STUB: th08 0x45567d
i32 ResultScreen::HandleHighScoreCharacterSelect()
{
    return 0;
}

// STUB: th08 0x455925
i32 ResultScreen::HandleHighScoreScreen()
{
    return 0;
}

// STUB: th08 0x455a33
i32 ResultScreen::HandleSpellCardDifficultySelect()
{
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

// STUB: th08 0x456938
void ResultScreen::FormatDate(char *buffer)
{
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
