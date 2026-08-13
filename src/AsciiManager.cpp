#include "th_pch.h"

#include <stdarg.h>
#include <stdio.h>

namespace th08
{

DIFFABLE_STATIC(ChainElem, g_AsciiManagerDrawChainLowPrio);
DIFFABLE_STATIC(AsciiManager, g_AsciiManager);
DIFFABLE_STATIC(ChainElem, g_AsciiManagerCalcChain);
DIFFABLE_STATIC(ChainElem, g_AsciiManagerDrawChainHighPrio);

// FUNCTION: th08 0x402200
ChainCallbackResult AsciiManager::OnUpdate(AsciiManager *ascii)
{
    if (g_GameManager.isInGameMenu == 0 && g_GameManager.showRetryMenu == 0)
    {
        AsciiManagerPopup *popup = &ascii->scorePopups[0];
        if (!g_GameManager.flags.unk10)
        {
            i32 i;
            for (i = 0; i < ASCII_MAX_SCORE_POPUPS + ASCII_MAX_PLAYER_POPUPS; i++, popup++)
            {
                // NOTE: the empty if is needed so /Od lays the body out-of-line (jne body; jmp continue)
                if (!popup->inUse)
                {
                }
                else
                {
                    popup->position.y -= 0.5f * g_Supervisor.framerateMultiplier;
                    popup->timer++;
                    if (popup->timer > 60)
                    {
                        popup->inUse = false;
                    }
                }
            }

            popup = &ascii->timePopups[0];
            for (i = 0; i < ASCII_MAX_TIME_POPUPS; i++, popup++)
            {
                if (!popup->inUse)
                {
                }
                else
                {
                    popup->timer++;
                    if (popup->timer > 90)
                    {
                        popup->inUse = false;
                    }
                }
            }
        }
    }
    else
    {
        if (g_GameManager.isInGameMenu)
        {
            ascii->pauseMenu.OnUpdate();
        }
        if (g_GameManager.showRetryMenu)
        {
            ascii->retryMenu.OnUpdate();
        }
    }

    ascii->UpdateVms();

    if (g_GameManager.IsDemoMode())
    {
        if (ascii->demoIcon.scriptIndex == 0)
        {
            ascii->asciiAnm->SetAndExecuteScriptIdx(&ascii->demoIcon, 0xb);
        }
        g_AnmManager->ExecuteScript(&ascii->demoIcon);
    }
    else
    {
        ascii->demoIcon.scriptIndex = 0;
    }
    ascii->unk_8284++;

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult AsciiManager::OnDrawLowPrio(AsciiManager *ascii)
{
    ascii->DrawStrings();
    ascii->ResetStringsCount();
    ascii->pauseMenu.OnDrawPauseMenu();
    ascii->retryMenu.OnDrawRetryMenu();
    if (ascii->demoIcon.scriptIndex != 0)
    {
        g_AnmManager->DrawNoRotation(&ascii->demoIcon);
    }

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: th08 0x4070b0
void AsciiManager::SetGaugeInterrupt(i32 interrupt)
{
    this->youkaiGauge.SetInterrupt(interrupt);
    this->youkaiGaugeHumanIcon.SetInterrupt(interrupt);
    this->youkaiGaugeYoukaiIcon.SetInterrupt(interrupt);
    this->youkaiGaugeCursor.SetInterrupt(interrupt);

    this->gaugeInterrupt = interrupt;
}

// FUNCTION: th08 0x407140
i32 AsciiManager::GetGaugeInterrupt()
{
    return this->gaugeInterrupt;
}

// FUNCTION: th08 0x4398ff
void AsciiManager::SetIsGuiMode(u32 isGuiMode)
{
    this->isGui = isGuiMode;
}

// FUNCTION: th08 0x42f2d0
/* 设置第 idx 个 Boss 标记的状态（OnDrawLowPrioImpl 用 bossMarkerStates[i] 决定闪烁样式）。 */
void AsciiManager::SetBossMarkerState(i32 idx, u32 state)
{
    this->bossMarkerStates[idx] = state;
}

// STUB: th08 0x402b20
void AsciiManager::DrawStrings()
{
}

// FUNCTION: th08 0x407160
/* 清空字符串计数，下一帧从头分配字符串槽位。 */
void AsciiManager::ResetStringsCount()
{
    this->numStrings = 0;
}

ChainCallbackResult AsciiManager::OnDrawHighPrio(AsciiManager *ascii)
{
    ascii->OnDrawHighPrioImpl();

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

void AsciiManager::Reset()
{
    memset(&this->smallScoreText, 0, sizeof(AnmVm));
    memset(&this->popupText, 0, sizeof(AnmVm));
    memset(&this->largeText, 0, sizeof(AnmVm));
    memset(&this->strings, 0, sizeof(this->strings));
    memset(&this->pauseMenu, 0, sizeof(PauseMenu));
    memset(&this->retryMenu, 0, sizeof(RetryMenu));
    memset(&this->scorePopups, 0, sizeof(this->scorePopups));
    memset(&this->timePopups, 0, sizeof(this->timePopups));

    this->numStrings = 0;
    this->isGui = FALSE;
    this->isSelected = FALSE;
    this->nextScorePopupIndex = 0;
    this->nextPlayerPointPopupIndex = 0;
    /* nextTimePopupIndex is not set to 0?  */
    this->unk0x829c = 0;
    this->color.d3dColor = COLOR_WHITE;
    this->scaleX = 1.0f;
    this->scaleY = 1.0f;
    this->smallScoreText.prefix.anchor = 3;
    this->popupText.prefix.anchor = 3;
    this->asciiAnm->InitializeAndSetSprite(&this->smallScoreText, 0);
    this->asciiAnm->InitializeAndSetSprite(&this->popupText, 136);
    this->asciiAnm->InitializeAndSetSprite(&this->largeText, 32);
    this->smallScoreText.pos.z = 0.1f;
    /* This was already set to FALSE ? */
    this->isSelected = FALSE;
    this->SetSpaceWidth(13);
}

void AsciiManager::SetBossMarkerInterrupt(i32 idx, i16 interrupt)
{
    this->bossMarkers[idx].SetInterrupt(interrupt);
}

void AsciiManager::SetBossMarkerPosition(i32 idx, Float3 *pos)
{
    this->bossMarkers[idx].pos = *pos;
}

void AsciiManager::InitializeVms()
{
    this->asciiAnm->SetAndExecuteScriptIdx(&this->youkaiGauge, 5);
    this->asciiAnm->SetAndExecuteScriptIdx(&this->youkaiGaugeYoukaiIcon, 7);
    this->asciiAnm->SetAndExecuteScriptIdx(&this->youkaiGaugeHumanIcon, 6);
    this->asciiAnm->SetAndExecuteScriptIdx(&this->youkaiGaugeCursor, 8);
    this->asciiAnm->SetAndExecuteScriptIdx(&this->percentageText, 4);
    this->asciiAnm->SetAndExecuteScriptIdx(&this->unk_1520, 9);
    this->asciiAnm->SetAndExecuteScriptIdx(&this->bossMarkers[0], 10);
    this->asciiAnm->SetAndExecuteScriptIdx(&this->bossMarkers[1], 10);
    this->asciiAnm->SetAndExecuteScriptIdx(&this->bossMarkers[2], 10);
    this->asciiAnm->SetAndExecuteScriptIdx(&this->bossMarkers[3], 10);

    this->youkaiGaugeHumanIcon.pos.x -= (g_GameManager.youkaiGaugeHumanLimit * 56.0f) / -10000.0f;
    this->youkaiGaugeYoukaiIcon.pos.x += (g_GameManager.youkaiGaugeYoukaiLimit * 56.0f) / 10000.0f;

    this->SetGaugeInterrupt(this->GetGaugeInterrupt());
}

ZunResult AsciiManager::RegisterChain()
{
    AsciiManager *ascii = &g_AsciiManager;

    g_AsciiManagerCalcChain.SetCallback((ChainCallback)AsciiManager::OnUpdate);
    g_AsciiManagerCalcChain.addedCallback = (ChainLifetimeCallback)AsciiManager::AddedCallback;
    g_AsciiManagerCalcChain.deletedCallback = (ChainLifetimeCallback)AsciiManager::DeletedCallback;
    g_AsciiManagerCalcChain.arg = ascii;
    if (g_Chain.AddToCalcChain(&g_AsciiManagerCalcChain, 1) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    g_AsciiManagerDrawChainLowPrio.SetCallback((ChainCallback)AsciiManager::OnDrawLowPrio);
    g_AsciiManagerDrawChainLowPrio.arg = ascii;
    g_Chain.AddToDrawChain(&g_AsciiManagerDrawChainLowPrio, 20);

    g_AsciiManagerDrawChainHighPrio.SetCallback((ChainCallback)AsciiManager::OnDrawHighPrio);
    g_AsciiManagerDrawChainHighPrio.arg = ascii;
    g_Chain.AddToDrawChain(&g_AsciiManagerDrawChainHighPrio, 14);

    return ZUN_SUCCESS;
}

ZunResult AsciiManager::AddedCallback(AsciiManager *ascii)
{
    memset(ascii, 0, sizeof(AsciiManager));

    ascii->asciiAnm = g_AnmManager->PreloadAnm(1, "ascii.anm");
    if (ascii->asciiAnm == NULL)
    {
        return ZUN_ERROR;
    }

    ascii->captureAnm = g_AnmManager->PreloadAnm(3, "capture.anm");
    if (ascii->captureAnm == NULL)
    {
        return ZUN_ERROR;
    }

    ascii->Reset();
    ascii->InitializeVms();

    return ZUN_SUCCESS;
}

ZunResult AsciiManager::DeletedCallback(AsciiManager *ascii)
{
    g_AnmManager->ReleaseAnm(1);
    g_AnmManager->ReleaseAnm(3);

    return ZUN_SUCCESS;
}

void AsciiManager::CutChain()
{
    g_Chain.Cut(&g_AsciiManagerCalcChain);
    g_Chain.Cut(&g_AsciiManagerDrawChainLowPrio);
    /* ZUN seemingly forgot this: g_Chain.Cut(&g_AsciiManagerDrawChainHighPrio); */
}

#pragma var_order(nextString)
void AsciiManager::AddString(Float3 *position, const char *string)
{
    AsciiManagerString *nextString;

    if (this->numStrings >= ARRAY_SIZE_SIGNED(this->strings))
    {
        return;
    }

    nextString = &this->strings[this->numStrings];
    this->numStrings++;

    strcpy(nextString->text, string);

    nextString->position = *position;

    nextString->color = this->color.d3dColor;
    nextString->scaleX = this->scaleX;
    nextString->scaleY = this->scaleY;
    nextString->isGui = this->isGui;

    if (g_Supervisor.IsSoftwareTexturing())
    {
        nextString->isSelected = this->isSelected;
    }
    else
    {
        nextString->isSelected = FALSE;
    }
}

void AsciiManager::AddFormatText(Float3 *position, const char *fmt, ...)
{
    char buf[512];
    va_list va;

    va_start(va, fmt);
    vsprintf(buf, fmt, va);
    this->AddString(position, buf);
    va_end(va);
}

int AsciiManager::AddFormatText2(Float3 *position, const char *fmt, ...)
{
    char buf[512];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    this->AddString(position, buf);
    va_end(args);

    /* Did you know that vsprintf returns the number of characters added to the
     * buffer? So ZUN did not have to call strlen here.
     */
    return strlen(buf);
}

#pragma var_order(spaceWidth, i, curString, text, isGui, vector)
void AsciiManager::OnDrawLowPrioImpl()
{
    Float3 vector;
    ZunBool isGui = TRUE;
    int i;
    AsciiManagerString *curString = &this->strings[0];
    u8 *text;
    float spaceWidth;

    this->largeText.prefix.visible = true;
    this->largeText.prefix.anchor = 3;

    for (i = 0; i < this->numStrings; i++, curString++)
    {
        this->largeText.pos = curString->position;

        text = (u8 *)curString->text;

        this->largeText.prefix.scale.x = curString->scaleX;
        this->largeText.prefix.scale.y = curString->scaleY;
        spaceWidth = this->spaceWidth * curString->scaleX;

        if (isGui != curString->isGui)
        {
            isGui = curString->isGui;

            g_AnmManager->FlushVertexBuffer();

            if (isGui)
            {
                g_Supervisor.viewport.X = g_GameManager.arcadeRegionTopLeftPos.x;
                g_Supervisor.viewport.Y = g_GameManager.arcadeRegionTopLeftPos.y;
                g_Supervisor.viewport.Width = g_GameManager.arcadeRegionSize.x;
                g_Supervisor.viewport.Height = g_GameManager.arcadeRegionSize.y;
                g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
            }
            else
            {
                g_Supervisor.viewport.X = 0;
                g_Supervisor.viewport.Y = 0;
                g_Supervisor.viewport.Width = 640;
                g_Supervisor.viewport.Height = 480;
                g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
            }
        }

        while (*text)
        {
            if (*text == '\n')
            {
                this->largeText.pos.y += 16.0f * curString->scaleY;
                this->largeText.pos.x = curString->position.x;
            }
            else if (*text == ' ')
            {
                this->largeText.pos.x += spaceWidth;
            }
            else
            {
                if (!curString->isSelected)
                {
                    this->largeText.loadedSprite = this->asciiAnm->GetSprite(*text + (31 - ' '));
                    this->largeText.prefix.color1.d3dColor = curString->color;
                }
                else
                {
                    this->largeText.loadedSprite = this->asciiAnm->GetSprite(*text + (170 - ' '));
                    this->largeText.prefix.color1.d3dColor = COLOR_WHITE;
                }

                g_AnmManager->DrawNoRotation(&this->largeText);
                this->largeText.pos.x += spaceWidth;
            }

            text++;
        }
    }

    if (isGui)
    {
        g_AnmManager->FlushVertexBuffer();
        g_Supervisor.viewport.X = 0;
        g_Supervisor.viewport.Y = 0;
        g_Supervisor.viewport.Width = 640;
        g_Supervisor.viewport.Height = 480;
        g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
    }

    for (i = 0; i < ARRAY_SIZE_SIGNED(this->bossMarkers); i++)
    {
        if (this->bossMarkers[i].pos.x >= 56.0f && this->bossMarkers[i].pos.x <= 392.0f)
        {
            // TODO: This line is not done! The player position is needed in this calculation
            spaceWidth = fabsf(this->bossMarkers[i].pos.x - 32.0f);

            this->bossMarkers[i].loadedSprite = this->asciiAnm->GetSprite(157);

            switch (this->bossMarkerStates[i])
            {
            case 0:
            no_flicker:
                this->bossMarkers[i].prefix.color1.r = 255;
                this->bossMarkers[i].prefix.color1.g = 255;
                this->bossMarkers[i].prefix.color1.b = 255;
                if (spaceWidth < 64.0f)
                {
                    this->bossMarkers[i].prefix.color1.a = (spaceWidth * 64.0f) / 64.0f + 96.0f;
                }
                else
                {
                    this->bossMarkers[i].prefix.color1.a = 160;
                }
                break;
            case 1:
                this->bossMarkers[i].prefix.color1.a = 128;
                this->bossMarkers[i].prefix.color1.r = 255;
                this->bossMarkers[i].prefix.color1.g = 64;
                this->bossMarkers[i].prefix.color1.b = 64;
                break;
            case 2:
                if (this->unk_8284 % 8 == 0)
                {
                    this->bossMarkers[i].loadedSprite = this->asciiAnm->GetSprite(158);
                    this->bossMarkers[i].prefix.color1.a = 255;
                    this->bossMarkers[i].prefix.color1.r = 255;
                    this->bossMarkers[i].prefix.color1.g = 255;
                    this->bossMarkers[i].prefix.color1.b = 255;
                }
                else
                {
                    goto no_flicker;
                }
                break;
            case 3:
                if (this->unk_8284 % 4 == 0)
                {
                    this->bossMarkers[i].loadedSprite = this->asciiAnm->GetSprite(158);
                    this->bossMarkers[i].prefix.color1.a = 255;
                    this->bossMarkers[i].prefix.color1.r = 255;
                    this->bossMarkers[i].prefix.color1.g = 255;
                    this->bossMarkers[i].prefix.color1.b = 255;
                }
                else
                {
                    goto no_flicker;
                }
                break;
            case 4:
                if (this->unk_8284 % 2 == 0)
                {
                    this->bossMarkers[i].loadedSprite = this->asciiAnm->GetSprite(158);
                    this->bossMarkers[i].prefix.color1.a = 255;
                    this->bossMarkers[i].prefix.color1.r = 255;
                    this->bossMarkers[i].prefix.color1.g = 255;
                    this->bossMarkers[i].prefix.color1.b = 255;
                }
                else
                {
                    goto no_flicker;
                }
                break;
            }

            g_AnmManager->DrawNoRotation(&this->bossMarkers[i]);
        }
    }
}

void AsciiManager::CreateScorePopup(Float3 *position, i32 number, D3DCOLOR color)
{
    AsciiManagerPopup *popup;
    int characterCount;

    if (this->nextScorePopupIndex >= ASCII_MAX_SCORE_POPUPS)
    {
        this->nextScorePopupIndex = 0;
    }
    popup = &this->scorePopups[nextScorePopupIndex];
    popup->inUse = true;

    characterCount = 0;
    if (number >= 0)
    {
        while (number != 0)
        {
            popup->text[characterCount] = number % 10;
            characterCount++;
            number /= 10;
        }
    }
    else
    {
        popup->text[characterCount] = 10;
        characterCount++;
    }

    if (characterCount == 0)
    {
        popup->text[characterCount] = 0;
        characterCount++;
    }

    popup->characterCount = characterCount;
    popup->color = color;
    popup->timer = 0;
    popup->position = *position;
    popup->position.x += g_GameManager.arcadeRegionTopLeftPos.x;
    popup->position.y += g_GameManager.arcadeRegionTopLeftPos.y;
    this->nextScorePopupIndex++;
}

void AsciiManager::CreatePlayerPointPopup(Float3 *position, i32 number, D3DCOLOR color)
{
    AsciiManagerPopup *popup;
    int characterCount;

    if (this->nextPlayerPointPopupIndex >= ASCII_MAX_PLAYER_POPUPS)
    {
        this->nextPlayerPointPopupIndex = 0;
    }
    popup = &this->scorePopups[ASCII_MAX_SCORE_POPUPS + nextPlayerPointPopupIndex];
    popup->inUse = true;

    characterCount = 0;
    if (number >= 0)
    {
        while (number != 0)
        {
            popup->text[characterCount] = number % 10;
            characterCount++;
            number /= 10;
        }
    }
    else
    {
        popup->text[characterCount] = 10;
        characterCount++;
    }

    if (characterCount == 0)
    {
        popup->text[characterCount] = 0;
        characterCount++;
    }

    popup->characterCount = characterCount;
    popup->color = color;
    popup->timer = 0;
    popup->position = *position;
    popup->position.x += g_GameManager.arcadeRegionTopLeftPos.x;
    popup->position.y += g_GameManager.arcadeRegionTopLeftPos.y;
    this->nextPlayerPointPopupIndex++;
}

void AsciiManager::CreateTimePopup(Float3 *position, i32 number, i32 param3, D3DCOLOR color)
{
    AsciiManagerPopup *popup;
    int characterCount;

    if (this->nextTimePopupIndex >= ASCII_MAX_TIME_POPUPS)
    {
        this->nextTimePopupIndex = 0;
    }
    popup = &this->timePopups[nextTimePopupIndex];
    popup->inUse = true;

    characterCount = 0;
    if (param3 > 0)
    {
        popup->text[characterCount] = 15;
        characterCount++;
        while (param3 != 0)
        {
            popup->text[characterCount] = param3 % 10;
            characterCount++;
            param3 /= 10;
        }
        popup->text[characterCount] = 14;
        characterCount++;
    }

    if (number > 0)
    {
        while (number != 0)
        {
            popup->text[characterCount] = number % 10;
            characterCount++;
            number /= 10;
        }
    }
    else
    {
        popup->text[characterCount] = 0;
        characterCount++;
    }

    popup->text[characterCount] = 13;
    characterCount++;

    popup->characterCount = characterCount;
    popup->color = color;
    popup->timer = 0;
    popup->position = *position;
    popup->position.x += g_GameManager.arcadeRegionTopLeftPos.x;
    popup->position.y += g_GameManager.arcadeRegionTopLeftPos.y;
    popup->scaleX = this->scaleX;
    popup->scaleY = this->scaleY;
    this->nextTimePopupIndex++;
}

void AsciiManager::CreateFamiliarPopup(Float3 *position, i32 number, i32 param3, D3DCOLOR color)
{
    AsciiManagerPopup *popup;
    int characterCount;

    if (this->nextTimePopupIndex >= ASCII_MAX_TIME_POPUPS)
    {
        this->nextTimePopupIndex = 0;
    }
    popup = &this->timePopups[nextTimePopupIndex];
    popup->inUse = true;

    characterCount = 0;
    if (param3 > 0)
    {
        popup->text[characterCount] = 15;
        characterCount++;
        while (param3 != 0)
        {
            popup->text[characterCount] = param3 % 10;
            characterCount++;
            param3 /= 10;
        }
        popup->text[characterCount] = 14;
        characterCount++;
    }

    if (number > 0)
    {
        while (number != 0)
        {
            popup->text[characterCount] = number % 10;
            characterCount++;
            number /= 10;
        }
    }
    else
    {
        popup->text[characterCount] = 0;
        characterCount++;
    }

    popup->text[characterCount] = 13;
    characterCount++;

    popup->characterCount = characterCount;
    popup->color = color;
    popup->timer = 88;
    popup->position = *position;
    popup->position.x += g_GameManager.arcadeRegionTopLeftPos.x + 3.5f * characterCount;
    popup->position.y += g_GameManager.arcadeRegionTopLeftPos.y;
    popup->scaleX = this->scaleX;
    popup->scaleY = this->scaleY;
    this->nextTimePopupIndex++;
}

// STUB: th08 0x4037b0
i32 PauseMenu::OnUpdate()
{
    return 0;
}

// FUNCTION: th08 0x404750 (94.74% FIXME: D3D 虚函数寄存器分配)
void PauseMenu::OnDrawPauseMenu()
{
    u32 i;

    /* 0x164d0ba：暂停菜单可见标志（暂停生效时非 0）。 */
    if (*(u8 *)0x164d0ba != 0)
    {
        g_AnmManager->FlushVertexBuffer();

        /* 把游戏浮点视图矩形（0x164d2dc.. 的 f32 x,y,w,h）写进 D3D 设备内部视图
         * 结构（0x17ce820），再通过设备 vtable 的 SetViewport（slot 0x28）应用。 */
        *(i32 *)(D3D_DEVICE_VIEWPORT + 0) = (i32)*(f32 *)(GAME_VIEWPORT_FLOATS + 0);
        *(i32 *)(D3D_DEVICE_VIEWPORT + 4) = (i32)*(f32 *)(GAME_VIEWPORT_FLOATS + 4);
        *(i32 *)(D3D_DEVICE_VIEWPORT + 8) = (i32)*(f32 *)(GAME_VIEWPORT_FLOATS + 8);
        *(i32 *)(D3D_DEVICE_VIEWPORT + 0xc) = (i32)*(f32 *)(GAME_VIEWPORT_FLOATS + 0xc);

        /* D3D viewport-set virtual call @ vtbl+0xa0 */
        ((void(__stdcall *)(void *, void *))(*(void ***) * (void **)D3D_DEVICE_OBJ)[0xa0 / 4])(
            *(void **)D3D_DEVICE_OBJ, (void *)D3D_DEVICE_VIEWPORT);

        /* 0x17ce8fc bit1：本次设置视图后是否要重绘菜单背景。 */
        if ((*(u32 *)D3D_DEVICE_FLAG_WORD >> 1) & 1)
        {
            if (this->curState != 0)
            {
                u8 local[sizeof(AnmVm)];

                /* 复制菜单背景 VM，置位 0xb8 处（AnmPrefix 计时器区）的临时标记后绘制。 */
                memcpy(local, &this->menuBackground, sizeof(AnmVm));
                *(u32 *)(local + 0xb8) |= 0x2000;
                g_AnmManager->DrawNoRotation((AnmVm *)local);
            }
        }

        for (i = 0; i < ARRAY_SIZE_SIGNED(this->menuSprites); i++)
        {
            if (this->menuSprites[i].IsVisible())
            {
                g_AnmManager->DrawNoRotation(&this->menuSprites[i]);
            }
        }
    }
}

// STUB: th08 0x404890
i32 RetryMenu::OnUpdate()
{
    return 0;
}

// FUNCTION: th08 0x4052b0 (93.81% FIXME: D3D 虚函数寄存器分配)
void RetryMenu::OnDrawRetryMenu()
{
    u32 i;

    /* 0x164d0bb：重试菜单可见标志（倒地/续关时非 0）。 */
    if (*(u8 *)0x164d0bb != 0)
    {
        g_AnmManager->FlushVertexBuffer();

        /* 同步游戏浮点视图矩形到 D3D 设备内部视图并调用 SetViewport。 */
        *(i32 *)(D3D_DEVICE_VIEWPORT + 0) = (i32)*(f32 *)(GAME_VIEWPORT_FLOATS + 0);
        *(i32 *)(D3D_DEVICE_VIEWPORT + 4) = (i32)*(f32 *)(GAME_VIEWPORT_FLOATS + 4);
        *(i32 *)(D3D_DEVICE_VIEWPORT + 8) = (i32)*(f32 *)(GAME_VIEWPORT_FLOATS + 8);
        *(i32 *)(D3D_DEVICE_VIEWPORT + 0xc) = (i32)*(f32 *)(GAME_VIEWPORT_FLOATS + 0xc);

        /* D3D viewport-set virtual call @ vtbl+0xa0 */
        ((void(__stdcall *)(void *, void *))(*(void ***) * (void **)D3D_DEVICE_OBJ)[0xa0 / 4])(
            *(void **)D3D_DEVICE_OBJ, (void *)D3D_DEVICE_VIEWPORT);

        /* 0x17ce8fc bit1：本次设置视图后是否要重绘菜单背景。 */
        if ((*(u32 *)D3D_DEVICE_FLAG_WORD >> 1) & 1)
        {
            if (this->curState != 0 || this->numFrames > 2)
            {
                g_AnmManager->DrawNoRotation(&this->menuBackground);
            }
        }

        /* 0x160f538：当前进行中的符卡/流程计数，小于 4 时显示 4 个菜单项。 */
        if (g_GameManager.GetFlag14() == 0 && *(i32 *)0x160f538 < 4)
        {
            for (i = 0; i < 4; i++)
            {
                if (this->menuSprites[i].IsVisible())
                {
                    g_AnmManager->DrawNoRotation(&this->menuSprites[i]);
                }
            }
        }
        else
        {
            for (i = 0; i < 3; i++)
            {
                if (this->menuSprites[i].IsVisible())
                {
                    g_AnmManager->DrawNoRotation(&this->menuSprites[i]);
                }
            }
        }
    }
}

// STUB: th08 0x405420
void AsciiManager::OnDrawHighPrioImpl()
{
}

// FUNCTION: th08 0x405e10
/* 绘制百分比数字（例如妖率槽 "12.34%"）：按数值位数排布各数字精灵。
   sprite 索引：0x88='0'..0x91='9'，0x92='%'，0x93='.'，0x94='-'。
   >=10000 时固定显示 "100.00"（妖率上限）。 */
#pragma var_order(xPos, numDigits, absVal)
void AsciiManager::DrawPercentage(Float3 *position, i32 percentage, D3DCOLOR color)
{
    i32 numDigits;
    i32 absVal;
    f32 xPos;

    numDigits = 4;
    if (percentage < 0)
    {
        numDigits++;
    }

    absVal = abs(percentage);
    if (absVal >= 10000)
    {
        numDigits += 3;
    }
    else if (absVal >= 1000)
    {
        numDigits += 2;
    }
    else
    {
        numDigits += 1;
    }

    xPos = (f32)numDigits * 3.5f - 3.5f - 4.0f;
    this->percentageText.pos = *position;
    this->percentageText.pos.x -= xPos;
    this->percentageText.prefix.color1.d3dColor = color;

    if (percentage < 0)
    {
        this->asciiAnm->SetSprite(&this->percentageText, 0x94); /* '-' */
        g_AnmManager->DrawNoRotation(&this->percentageText);
        this->percentageText.pos.x += 7.0f;
    }

    if (absVal >= 10000)
    {
        /* 固定显示 "100.00" */
        this->asciiAnm->SetSprite(&this->percentageText, 0x89); /* '1' */
        g_AnmManager->DrawNoRotation(&this->percentageText);
        this->percentageText.pos.x += 7.0f;
        this->asciiAnm->SetSprite(&this->percentageText, 0x88); /* '0' */
        g_AnmManager->DrawNoRotation(&this->percentageText);
        this->percentageText.pos.x += 7.0f;
        this->asciiAnm->SetSprite(&this->percentageText, 0x88); /* '0' */
        g_AnmManager->DrawNoRotation(&this->percentageText);
        this->percentageText.pos.x += 7.0f;
        this->asciiAnm->SetSprite(&this->percentageText, 0x93); /* '.' */
        g_AnmManager->DrawNoRotation(&this->percentageText);
        this->percentageText.pos.x += 5.0f;
        this->percentageText.prefix.scale.y = 0.8f;
        this->percentageText.prefix.scale.x = 0.8f;
        this->percentageText.pos.y += 2.0f;
        this->asciiAnm->SetSprite(&this->percentageText, 0x88); /* '0' */
        g_AnmManager->DrawNoRotation(&this->percentageText);
        this->percentageText.pos.x += 5.0f;
        this->asciiAnm->SetSprite(&this->percentageText, 0x88); /* '0' */
        g_AnmManager->DrawNoRotation(&this->percentageText);
        this->percentageText.pos.x += 7.0f;
    }
    else if (absVal >= 1000)
    {
        numDigits = absVal;
        this->asciiAnm->SetSprite(&this->percentageText, numDigits / 1000 + 0x88);
        numDigits %= 1000;
        g_AnmManager->DrawNoRotation(&this->percentageText);
        this->percentageText.pos.x += 7.0f;
        this->asciiAnm->SetSprite(&this->percentageText, numDigits / 100 + 0x88);
        numDigits %= 100;
        g_AnmManager->DrawNoRotation(&this->percentageText);
        this->percentageText.pos.x += 7.0f;
        this->asciiAnm->SetSprite(&this->percentageText, 0x93); /* '.' */
        g_AnmManager->DrawNoRotation(&this->percentageText);
        this->percentageText.pos.x += 5.0f;
        this->percentageText.prefix.scale.y = 0.8f;
        this->percentageText.prefix.scale.x = 0.8f;
        this->percentageText.pos.y += 2.0f;
        this->asciiAnm->SetSprite(&this->percentageText, numDigits / 10 + 0x88);
        numDigits %= 10;
        g_AnmManager->DrawNoRotation(&this->percentageText);
        this->percentageText.pos.x += 5.0f;
        this->asciiAnm->SetSprite(&this->percentageText, numDigits + 0x88);
        g_AnmManager->DrawNoRotation(&this->percentageText);
        this->percentageText.pos.x += 7.0f;
    }
    else
    {
        numDigits = absVal;
        this->asciiAnm->SetSprite(&this->percentageText, numDigits / 100 + 0x88);
        numDigits %= 100;
        g_AnmManager->DrawNoRotation(&this->percentageText);
        this->percentageText.pos.x += 7.0f;
        this->asciiAnm->SetSprite(&this->percentageText, 0x93); /* '.' */
        g_AnmManager->DrawNoRotation(&this->percentageText);
        this->percentageText.pos.x += 5.0f;
        this->percentageText.prefix.scale.y = 0.8f;
        this->percentageText.prefix.scale.x = 0.8f;
        this->percentageText.pos.y += 2.0f;
        this->asciiAnm->SetSprite(&this->percentageText, numDigits / 10 + 0x88);
        numDigits %= 10;
        g_AnmManager->DrawNoRotation(&this->percentageText);
        this->percentageText.pos.x += 5.0f;
        this->asciiAnm->SetSprite(&this->percentageText, numDigits + 0x88);
        g_AnmManager->DrawNoRotation(&this->percentageText);
        this->percentageText.pos.x += 7.0f;
    }

    this->percentageText.prefix.scale.y = 1.0f;
    this->percentageText.prefix.scale.x = 1.0f;
    this->percentageText.pos.y -= 2.0f;
    this->asciiAnm->SetSprite(&this->percentageText, 0x92); /* '%' */
    g_AnmManager->DrawNoRotation(&this->percentageText);
}

} /* namespace th08 */
