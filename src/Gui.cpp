#include "th_pch.h"

#include "Gui.hpp"
#include "BulletManager.hpp"
#include "EnemyManager.hpp"
#include "ItemManager.hpp"
#include "Player.hpp"
#include "ReplayManager.hpp"
#include "ScreenEffect.hpp"

namespace th08
{

// STUB: th08 0x4396f8 (AnmVm::FUN_004396f8)
u32 __fastcall FUN_004396f8(AnmVm *vm)
{
    return 0;
}

DIFFABLE_STATIC(Gui, g_Gui);
DIFFABLE_STATIC(ChainElem, g_GuiCalcChain);
DIFFABLE_STATIC(ChainElem, g_GuiDrawChain);
DIFFABLE_STATIC_ARRAY(AnmLoaded *, 4, g_GuiPortraitAnms);
DIFFABLE_STATIC(AnmLoaded *, g_GuiStageClearAnmA); // 0x4d50a8
DIFFABLE_STATIC(AnmLoaded *, g_GuiStageClearAnmB); // 0x4d50ac
DIFFABLE_STATIC(u8 *, g_GuiBgmPathBase); // 0x4e4824

DIFFABLE_EXTERN(i32, g_SpellcardBgmOverride); // 0x4e4b64 (defined in GameManager.cpp)
DIFFABLE_EXTERN(f32, g_EclExitLeftBound);     // 0x17d61ac (defined in EclManager.cpp)
DIFFABLE_EXTERN(f32, g_17d61b0);              // 0x17d61b0 (defined in EclManager.cpp)

void __fastcall FUN_00437f5c(i32 param);      // 0x437f5c (defined in GameManager.cpp, spellcard collect screen)

/* 0x438fe9 / 0x438ffd：读取 D3D 设备对象标志区的两个 u32（原版为独立小函数，Gui 链回调使用）。 */
u32 FUN_00438fe9(); // 0x438fe9
u32 FUN_00438ffd(); // 0x438ffd

// FUNCTION: th08 0x438fe9
u32 FUN_00438fe9()
{
    return *(u32 *)0x17ce8bc;
}

// FUNCTION: th08 0x438ffd
u32 FUN_00438ffd()
{
    return *(u32 *)0x17ce8c4;
}

/* AsciiManager::CreatePopup4 (0x403600) 尚未在 AsciiManager.hpp 声明；用 stub thiscall 类生成同构调用。 */
class StubThiscallAsciiManagerCreatePopup4
{
  public:
    void CreatePopup4(Float3 *position, i32 number, i32 param3, u32 color);
};

void StubThiscallAsciiManagerCreatePopup4::CreatePopup4(Float3 *position, i32 number, i32 param3, u32 color)
{
}

/* ScreenEffect::DrawSquareShaded 在头文件里声明为 static(__cdecl)，而原版是 __fastcall；
 * 用 __fastcall 函数指针直调 0x45b490 以匹配原版寄存器传参。 */
typedef void(__fastcall *GuiDrawSquareShadedProc)(ZunRect *, D3DCOLOR, D3DCOLOR, D3DCOLOR, D3DCOLOR);

#define DRAW_SQUARE_SHADED(rect_, topLeft_, topRight_, bottomLeft_, bottomRight_)                              \
    ((GuiDrawSquareShadedProc)0x45b490)((rect_), (topLeft_), (topRight_), (bottomLeft_), (bottomRight_))

#define GAME_STATE_EVENT_5 5

// FUNCTION: th08 0x4353ec — XOR-0x77-obfuscated message text, copied from src to dst.
static void DecryptMsgText(char *dst, const char *src)
{
    i8 ch;
    do
    {
        ch = (i8)((i8)*src++ ^ 0x77);
        *dst++ = (char)ch;
    } while (ch != 0);
}

ChainCallbackResult Gui::OnUpdate(Gui *gui)
{
    if (g_GameManager.unk2C != 0)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    gui->UpdateBossHud();
    gui->impl->RunMsg();
    if ((g_CurFrameInput & TH_BUTTON_SKIP) != 0)
    {
        if (g_Supervisor.unk174 < 8)
        {
            g_Supervisor.unk174 = 8;
        }
    }
    gui->frameCounter++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult Gui::OnDraw(Gui *gui)
{
    if (gui->impl->msgState.stageClearActive != 0)
    {
        gui->DrawStageClearHud();
    }
    gui->impl->DrawDialogue();
    gui->DrawBossHud();
    gui->DrawGameScene();
    gui->DrawHud();
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ZunResult Gui::AddedCallback(Gui *gui)
{
    return gui->ActualAddedCallback();
}

ZunResult Gui::DeletedCallback(Gui *gui)
{
    if (Supervisor::GetUnk168() == 0)
    {
        g_AnmManager->ReleaseAnm(0xd);
    }
    gui->FreeMsgFile();
    if (Supervisor::GetUnk164() != 0)
    {
        g_AnmManager->ReleaseAnm(0xa);
        g_AnmManager->ReleaseAnm(0xc);
        g_AnmManager->ReleaseAnm(0xb);
        g_AnmManager->ReleaseAnm(0xe);
        ZUN_DELETE(gui->impl);
    }
    return ZUN_SUCCESS;
}

#pragma var_order(gui, newImpl, impl)
ZunResult Gui::RegisterChain()
{
    Gui *gui = &g_Gui;

    if (Supervisor::GetUnk164())
    {
        memset(gui, 0, sizeof(Gui));
        gui->impl = ZUN_NEW(GuiImpl, "FRScreenImplInf");
    }
    g_GuiCalcChain.SetCallback((ChainCallback)OnUpdate);
    g_GuiCalcChain.addedCallback = (ChainLifetimeCallback)AddedCallback;
    g_GuiCalcChain.deletedCallback = (ChainLifetimeCallback)DeletedCallback;
    g_GuiCalcChain.arg = gui;
    if (g_Chain.AddToCalcChain(&g_GuiCalcChain, 0xf))
    {
        return ZUN_ERROR;
    }
    g_GuiDrawChain.SetCallback((ChainCallback)OnDraw);
    g_GuiDrawChain.arg = gui;
    g_Chain.AddToDrawChain(&g_GuiDrawChain, 0x11);
    return ZUN_SUCCESS;
}

void Gui::CutChain()
{
    g_Chain.Cut(&g_GuiCalcChain);
    g_Chain.Cut(&g_GuiDrawChain);
}

// FUNCTION: th08 0x4390ee — Gui chain added-callback: load front/times/loading ANMs, init boss vms,
// preload the per-stage msg .dat and stage text ANM, reset the clock, and set HUD flags.
ZunResult Gui::ActualAddedCallback()
{
    i32 i;
    i32 j;

    if (FUN_00438fe9() != 0)
    {
        memset(this->impl, 0, 0x8c2e);
        this->frontAnm = g_AnmManager->PreloadAnm(0xa, "front.anm");
        if (this->frontAnm == NULL)
        {
            return ZUN_ERROR;
        }
        this->FUN_004396b8();
        this->timesAnm = g_AnmManager->PreloadAnm(0xe, "times.anm");
        if (this->timesAnm == NULL)
        {
            return ZUN_ERROR;
        }
        /* 0x4c72c4：按 g_PlayerCharacter 索引的 loading 立绘 ANM 名表。 */
        this->loadingPortraitAnm = g_AnmManager->PreloadAnm(0xc, *(const char **)(0x4c72c4 + g_PlayerCharacter * 4));
        if (this->loadingPortraitAnm == NULL)
        {
            return ZUN_ERROR;
        }
        g_GuiStageClearAnmA->SetAndExecuteScriptIdx(&this->impl->vmH, 0x1a);
        g_GuiStageClearAnmA->SetAndExecuteScriptIdx(&this->impl->vmL, 0x19);
        if (g_GameManager.GetFlag14() != 0)
        {
            if (g_CurrentSpellcardNumber >= 0xcd)
            {
                g_GuiStageClearAnmA->SetSprite(&this->impl->vmL, 0x120);
            }
            else
            {
                /* 0x160f538：进行中的符卡/流程计数。 */
                g_GuiStageClearAnmA->SetSprite(&this->impl->vmL, *(i32 *)0x160f538 + 0x11b);
            }
        }
    }
    else
    {
        this->FUN_004396b8();
        g_GuiStageClearAnmB->SetAndExecuteScriptIdx(&this->impl->vmF, 1);
        /* vmE 尾部 2 字节（未知字段）。 */
        *(u16 *)((u8 *)this->impl + 0x3ebe) = 1;
        for (i = 0; i < 0xe; i++)
        {
            for (j = 0; j < 0xc; j++)
            {
                g_GuiStageClearAnmB->SetAndExecuteScriptIdx(&this->impl->vmsI[i * 0xc + j], ((i + j) & 1) + 3);
                this->impl->vmsI[i * 0xc + j].prefix.counterVar0 = i + j * 2;
                this->impl->vmsI[i * 0xc + j].pos.x = (f32)j * 32.0f - 0.5f + 16.0f;
                this->impl->vmsI[i * 0xc + j].pos.y = (f32)i * 32.0f - 0.5f + 16.0f;
                this->impl->vmsI[i * 0xc + j].pos.z = 0.0f;
                this->impl->vmsI[i * 0xc + j].prefix.uvScrollPos.x = (f32)j * 32.0f / 512.0f;
                this->impl->vmsI[i * 0xc + j].prefix.uvScrollPos.y = (f32)i * 32.0f / 512.0f;
            }
        }
        this->impl->inactiveVmsICount = 0xa8;
    }

    g_Gui.ResetClock();
    this->timesAnm->ExecuteAnmIdx(&this->impl->vmC, 0);
    this->timesAnm->SetSprite(&this->impl->vmC, (i32)g_GameManager.GetClockTime());
    if (g_GameManager.GetFlag14() == 0)
    {
        /* 0x4c74c0：[关卡][角色] 二维 msg .dat 文件名表。 */
        if (this->LoadMsg((const char *)*(i32 *)(0x4c74c0 + g_Unknown164d2cc * 0x30 + g_PlayerCharacter * 4)) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
    }
    if (FUN_00438ffd() == 0)
    {
        if (((g_PlayerFlags >> 0xe) & 1) != 0 && g_CurrentSpellcardNumber >= 0xcd)
        {
            /* 0x4c747c：stg8txt.anm（Lunatic 最后一张符卡文本）。 */
            this->stageTextAnm = g_AnmManager->PreloadAnm(0xd, *(const char **)0x4c747c);
            if (this->stageTextAnm == NULL)
            {
                return ZUN_ERROR;
            }
        }
        else
        {
            /* 0x4c745c：按 g_Unknown164d2cc 索引的 stage 文本 ANM 名表。 */
            this->stageTextAnm = g_AnmManager->PreloadAnm(0xd, *(const char **)(0x4c745c + g_Unknown164d2cc * 4));
            if (this->stageTextAnm == NULL)
            {
                return ZUN_ERROR;
            }
        }
    }
    if (FUN_00438fe9() != 0)
    {
        for (i = 0; i < 0x10; i++)
        {
            this->frontAnm->SetAndExecuteScriptIdx(&this->impl->vmsA[i], i);
        }
    }
    this->frameCounter = 0;
    this->bossPresent = false;
    this->impl->bossHudState = 0;
    this->bossLifeBarMaxSize = 0.0f;
    this->bossLifeBarSize = 0.0f;
    if (((g_PlayerFlags >> 0xe) & 1) == 0)
    {
        this->timesAnm->ExecuteAnmIdxArray(&this->impl->vmsB[0], 0, 4);
    }
    else
    {
        /* 符卡 BGM 是否"last word"检查（0x439916 原版经 ecx 传参，忽略实参值）。 */
        if (FUN_00438ffd() == 0 || ((i32(__fastcall *)(i32))0x439916)(g_CurrentSpellcardNumber) != 0)
        {
            this->timesAnm->ExecuteAnmIdxArray(&this->impl->vmsB[0], 3, 1);
            this->timesAnm->SetSprite(&this->impl->vmsB[0],
                                      ((i32(__fastcall *)(i32))0x439961)(g_CurrentSpellcardNumber) + 3);
        }
    }
    this->impl->msgState.currentMsgIdx = -1;
    this->impl->msgState.vms[2].pos.x = 0.0f;
    this->impl->msgState.vms[2].currentInstruction = NULL;
    this->impl->msgState.vms[2].posFinal.x = 0.0f;
    this->impl->msgState.vms[2].rotateFinal.z = 0.0f;
    this->flags.lifeDisplayUpdateFrames = 2;
    this->flags.bombDisplayUpdateFrames = 2;
    this->flags.grazeDisplayUpdateFrames = 2;
    this->flags.pointDisplayUpdateFrames = 2;
    this->flags.powerDisplayUpdateFrames = 2;
    this->flags.timeDisplayUpdateFrames = 2;
    g_GuiStageClearAnmA->SetAndExecuteScriptIdx(&this->impl->vmJ, 3);
    *(i32 *)0x17ce8cc = 0x10;
    this->impl->resultTimeFramesCopy = 0;
    return ZUN_SUCCESS;
}

// FUNCTION: th08 0x4396b8 — reset boss portrait vmsD/E/F (activeSpriteIndex = -1) and inactiveVmsICount.
void Gui::FUN_004396b8()
{
    this->impl->vmD.activeSpriteIndex = -1;
    this->impl->vmE.activeSpriteIndex = -1;
    this->impl->vmF.activeSpriteIndex = -1;
    this->impl->inactiveVmsICount = 0;
}

ZunResult Gui::LoadMsg(const char *path)
{
    i32 i;

    this->FreeMsgFile();
    this->impl->msgState.msgFileData = FileSystem::OpenFile((char *)path, NULL, 0);
    if (this->impl->msgState.msgFileData == NULL)
    {
        g_GameErrorContext.Log("error : \x83\x81\x83" "b\x83Z\x81[\x83W\x83t\x83@\x83" "C\x83\x8b %s \x82\xaa\x93\xc7\x82\xdd\x8d\x9e\x82\xdf\x82\xdc\x82\xb9\x82\xf1\x82\xc5\x82\xb5\x82\xbd\x0d\x0a", path);
        return ZUN_ERROR;
    }
    this->impl->msgState.currentMsgIdx = -1;
    this->impl->msgState.curInstr = NULL;
    for (i = 0; i < (i32)((GuiMsgData *)this->impl->msgState.msgFileData)->entryCount; i++)
    {
        ((GuiMsgData *)this->impl->msgState.msgFileData)->offsets[i] += (u32)this->impl->msgState.msgFileData;
    }
    return ZUN_SUCCESS;
}

void Gui::FreeMsgFile(void)
{
    if (this->impl->msgState.msgFileData != NULL)
    {
        g_ZunMemory.RemoveFromRegistry(this->impl->msgState.msgFileData);
        this->impl->msgState.msgFileData = NULL;
    }
}

// FUNCTION: th08 0x437d87
i32 Gui::IsBossPortraitVisible()
{
    i32 result;

    if (this->impl->vmD.activeSpriteIndex >= 0 &&
        FUN_004396f8(&this->impl->vmD) != 0)
    {
        result = 1;
    }
    else
    {
        result = 0;
    }
    return result;
}

// FUNCTION: th08 0x4358bb
i32 Gui::IsMsgActive()
{
    i32 result;

    /* 无活动对象或对象未处于"等待"状态 → 0。 */
    if (this->impl == 0)
    {
        return 0;
    }
    if (this->impl->msgState.currentMsgIdx < 0 &&
        this->impl->msgState.currentMsgIdx != -2)
    {
        result = 0;
    }
    else
    {
        result = 1;
    }
    return result;
}

// FUNCTION: th08 0x43587e
i32 Gui::MsgWait()
{
    if (this->impl == NULL)
    {
        return 0;
    }

    if (this->impl->msgState.ignoreWaitCounter > 0)
    {
        return 0;
    }

    return this->impl->msgState.currentMsgIdx >= 0;
}

// FUNCTION: th08 0x433db3
#pragma var_order(args, i15, args15, i17, args17, buf3, buf16, buf19, buf20, i9)
ZunResult GuiImpl::RunMsg()
{
    MsgRawInstrArgs *args;
    u32 i15;
    MsgRawInstrArgs *args15;
    u32 i17;
    MsgRawInstrArgs *args17;
    char buf3[0x40];
    char buf16[0x40];
    char buf19[0x40];
    char buf20[0x40];
    u32 i9;

    if (this->msgState.currentMsgIdx < 0)
    {
        return ZUN_ERROR;
    }
    if (this->msgState.ignoreWaitCounter > 0)
    {
        this->msgState.ignoreWaitCounter--;
    }
    if (this->msgState.dialogueSkippable && IS_PRESSED(TH_BUTTON_SKIP))
    {
        this->msgState.timer = this->msgState.curInstr->time;
    }
    if (g_Player.playerState != PLAYER_STATE_DEAD)
    {
        g_ItemManager.AutoCollectAllItems();
    }

    while (this->msgState.timer >= (i32)this->msgState.curInstr->time)
    {
        // Cases laid out in the original's source order (opcode values 0-22).
        switch (this->msgState.curInstr->opcode)
        {
        case 0: // MSG_DELETE
            this->msgState.currentMsgIdx = -1;
            return ZUN_ERROR;
        case 15: // MSG_SHOW_PORTRAIT
        {
            args15 = &this->msgState.curInstr->args;

            if (this->msgState.currentPortrait != args15->showPortrait.portraitIdx)
            {
                for (i15 = 0; i15 < 4; i15++)
                {
                    if (this->msgState.currentPortrait == i15)
                    {
                        if (this->msgState.currentPortrait / 2 != args15->showPortrait.portraitIdx / 2)
                        {
                            this->msgState.vms[i15].prefix.pendingInterrupt = 6;
                        }
                        else
                        {
                            this->msgState.vms[i15].prefix.pendingInterrupt = 4;
                        }
                    }
                    else
                    {
                        this->msgState.vms[i15].prefix.pendingInterrupt = 4;
                    }
                }
            }
            this->msgState.vms[args15->showPortrait.portraitIdx].prefix.pendingInterrupt = 3;
            this->msgState.currentPortrait = (u8)args15->showPortrait.portraitIdx;

            if (args15->showPortrait.anmScriptIdx0 >= 0)
            {
                g_GuiPortraitAnms[0]->SetSprite(&this->msgState.vms[0], args15->showPortrait.anmScriptIdx0);
            }
            if (args15->showPortrait.anmScriptIdx1 >= 0)
            {
                g_GuiPortraitAnms[1]->SetSprite(&this->msgState.vms[1], args15->showPortrait.anmScriptIdx1);
            }
            if (args15->showPortrait.anmScriptIdx2 >= 0)
            {
                g_GuiPortraitAnms[2]->SetSprite(&this->msgState.vms[2], args15->showPortrait.anmScriptIdx2);
            }
            if (args15->showPortrait.anmScriptIdx3 >= 0)
            {
                g_GuiPortraitAnms[3]->SetSprite(&this->msgState.vms[3], args15->showPortrait.anmScriptIdx3);
            }

            this->msgState.currentFace = (u8)args15->showPortrait.portraitIdx;
            this->msgState.portraitVisible = 1;
            break;
        }
        case 17: // MSG_CHANGE_FACE
        {
            args17 = &this->msgState.curInstr->args;

            if (this->msgState.currentPortrait != *(i32 *)&args17->portrait)
            {
                for (i17 = 0; i17 < 4; i17++)
                {
                    if (this->msgState.currentPortrait == i17)
                    {
                        if (this->msgState.currentPortrait / 2 != *(i32 *)&args17->portrait / 2)
                        {
                            this->msgState.vms[i17].prefix.pendingInterrupt = 6;
                        }
                        else
                        {
                            this->msgState.vms[i17].prefix.pendingInterrupt = 4;
                        }
                    }
                    else
                    {
                        this->msgState.vms[i17].prefix.pendingInterrupt = 4;
                    }
                }
            }
            this->msgState.vms[*(i32 *)&args17->portrait].prefix.pendingInterrupt = 3;
            this->msgState.currentPortrait = (u8)*(i32 *)&args17->portrait;

            if (args17->showPortrait.anmScriptIdx0 >= 0)
            {
                switch (*(i32 *)&args17->portrait)
                {
                case 0:
                    g_GuiPortraitAnms[0]->SetSprite(&this->msgState.vms[0], args17->showPortrait.anmScriptIdx0);
                    break;
                case 1:
                    g_GuiPortraitAnms[1]->SetSprite(&this->msgState.vms[1], args17->showPortrait.anmScriptIdx0);
                    break;
                case 2:
                    g_GuiPortraitAnms[2]->SetSprite(&this->msgState.vms[2], args17->showPortrait.anmScriptIdx0);
                    break;
                case 3:
                    g_GuiPortraitAnms[3]->SetSprite(&this->msgState.vms[3], args17->showPortrait.anmScriptIdx0);
                    break;
                }
            }

            this->msgState.currentFace = (u8)*(i32 *)&args17->portrait;
            this->msgState.portraitVisible = 1;
            break;
        }
        case 1: // MSG_SHOW_PORTRAIT (single)
        {
            args = &this->msgState.curInstr->args;

            switch (args->portrait.portraitIdx)
            {
            case 0:
                g_GuiPortraitAnms[0]->SetAndExecuteScriptIdx(&this->msgState.vms[0], args->portrait.anmScriptIdx);
                break;
            case 1:
                g_GuiPortraitAnms[1]->SetAndExecuteScriptIdx(&this->msgState.vms[1], args->portrait.anmScriptIdx);
                break;
            case 2:
                g_GuiPortraitAnms[2]->SetAndExecuteScriptIdx(&this->msgState.vms[2], args->portrait.anmScriptIdx);
                break;
            case 3:
                g_GuiPortraitAnms[3]->SetAndExecuteScriptIdx(&this->msgState.vms[3], args->portrait.anmScriptIdx);
                break;
            }
            if (this->msgState.vms[args->portrait.portraitIdx].loadedSprite->widthPx > 128.0f)
            {
                this->msgState.vms[args->portrait.portraitIdx].pos2.x = -112.0f;
            }
            else
            {
                this->msgState.vms[args->portrait.portraitIdx].pos2.x = 0.0f;
            }
            break;
        }
        case 2: // MSG_CHANGE_FACE (single)
        {
            args = &this->msgState.curInstr->args;

            switch (args->portrait.portraitIdx)
            {
            case 0:
                g_GuiPortraitAnms[0]->SetSprite(&this->msgState.vms[0], args->portrait.anmScriptIdx);
                break;
            case 1:
                g_GuiPortraitAnms[1]->SetSprite(&this->msgState.vms[1], args->portrait.anmScriptIdx);
                break;
            case 2:
                g_GuiPortraitAnms[2]->SetSprite(&this->msgState.vms[2], args->portrait.anmScriptIdx);
                break;
            case 3:
                g_GuiPortraitAnms[3]->SetSprite(&this->msgState.vms[3], args->portrait.anmScriptIdx);
                break;
            }
            if (this->msgState.vms[args->portrait.portraitIdx].loadedSprite->widthPx > 256.0f)
            {
                this->msgState.vms[args->portrait.portraitIdx].pos2.x = -208.0f;
                this->msgState.vms[args->portrait.portraitIdx].pos2.y = -50.0f;
            }
            else if (this->msgState.vms[args->portrait.portraitIdx].loadedSprite->widthPx > 128.0f)
            {
                this->msgState.vms[args->portrait.portraitIdx].pos2.x = -80.0f;
            }
            else
            {
                this->msgState.vms[args->portrait.portraitIdx].pos2.x = 0.0f;
            }
            break;
        }
        case 3: // MSG_DIALOGUE
        {
            args = &this->msgState.curInstr->args;

            if (args->dialogue.textLine == 0 && (i32)this->msgState.vms2[1].scriptIndex >= 0)
            {
                g_AnmManager->DrawTextLeft(&this->msgState.vms2[1], this->msgState.textColorsA[args->dialogue.textColor],
                                           this->msgState.textColorsB[args->dialogue.textColor], " ");
            }
            g_Supervisor.textAnm->SetAndExecuteScriptIdx(&this->msgState.vms2[args->dialogue.textLine],
                                                         args->dialogue.textLine);
            this->msgState.vms2[args->dialogue.textLine].fontHeight = this->msgState.fontSize;
            this->msgState.vms2[args->dialogue.textLine].fontWidth =
                this->msgState.vms2[args->dialogue.textLine].fontHeight;
            DecryptMsgText(buf3, args->dialogue.text);
            g_AnmManager->DrawTextLeft(&this->msgState.vms2[args->dialogue.textLine],
                                       this->msgState.textColorsA[args->dialogue.textColor],
                                       this->msgState.textColorsB[args->dialogue.textColor], buf3);
            this->msgState.framesElapsedDuringPause = 0;
            break;
        }
        case 16: // MSG_DIALOGUE variant
        {
            args = &this->msgState.curInstr->args;

            if (this->msgState.portraitVisible != 0)
            {
                if ((i32)this->msgState.vms2[1].scriptIndex >= 0)
                {
                    g_AnmManager->DrawTextLeft(&this->msgState.vms2[1],
                                               this->msgState.textColorsA[this->msgState.currentFace],
                                               this->msgState.textColorsB[this->msgState.currentFace], " ");
                }
                this->msgState.currentDialogueLine = 0;
            }
            g_Supervisor.textAnm->SetAndExecuteScriptIdx(&this->msgState.vms2[this->msgState.currentDialogueLine],
                                                         this->msgState.currentDialogueLine);
            this->msgState.vms2[this->msgState.currentDialogueLine].fontHeight = this->msgState.fontSize;
            this->msgState.vms2[this->msgState.currentDialogueLine].fontWidth =
                this->msgState.vms2[this->msgState.currentDialogueLine].fontHeight;
            DecryptMsgText(buf16, (char *)args);
            g_AnmManager->DrawTextLeft(&this->msgState.vms2[this->msgState.currentDialogueLine],
                                       this->msgState.textColorsA[this->msgState.currentFace],
                                       this->msgState.textColorsB[this->msgState.currentFace], buf16);
            this->msgState.framesElapsedDuringPause = 0;
            this->msgState.portraitVisible = 0;
            this->msgState.currentDialogueLine++;
            break;
        }
        case 19: // MSG_DIALOGUE variant
        {
            args = &this->msgState.curInstr->args;

            g_Supervisor.textAnm->SetAndExecuteScriptIdx(&this->msgState.vms2[0], 0);
            this->msgState.vms2[0].fontHeight = this->msgState.fontSize;
            this->msgState.vms2[0].fontWidth = this->msgState.vms2[0].fontHeight;
            DecryptMsgText(buf19, (char *)args);
            g_AnmManager->DrawTextLeft(&this->msgState.vms2[0], this->msgState.textColorsA[0],
                                       this->msgState.textColorsB[0], buf19);
            this->msgState.framesElapsedDuringPause = 0;
            break;
        }
        case 20: // MSG_DIALOGUE variant
        {
            args = &this->msgState.curInstr->args;

            g_Supervisor.textAnm->SetAndExecuteScriptIdx(&this->msgState.vms2[1], 1);
            this->msgState.vms2[1].fontHeight = this->msgState.fontSize;
            this->msgState.vms2[1].fontWidth = this->msgState.vms2[1].fontHeight;
            DecryptMsgText(buf20, (char *)args);
            g_AnmManager->DrawTextLeft(&this->msgState.vms2[1], this->msgState.textColorsA[0],
                                       this->msgState.textColorsB[0], buf20);
            this->msgState.framesElapsedDuringPause = 0;
            break;
        }
        case 21: // MSG_MUSIC_SELECT
        {
            if (WAS_PRESSED(TH_BUTTON_UP))
            {
                if (this->msgState.musicSelection == 1)
                {
                    g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
                }
                this->msgState.musicSelection = 0;
            }
            if (WAS_PRESSED(TH_BUTTON_DOWN))
            {
                if (this->msgState.musicSelection == 0)
                {
                    g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
                }
                this->msgState.musicSelection = 1;
            }
            this->msgState.vms2[this->msgState.musicSelection].prefix.color2.d3dColor = 0xFFFFFFFF;
            this->msgState.vms2[1 - this->msgState.musicSelection].prefix.color2.d3dColor = 0xE0606060;

            if (!(WAS_PRESSED(TH_BUTTON_SHOOT) && this->msgState.framesElapsedDuringPause >= 0x3c))
            {
                if (this->msgState.framesElapsedDuringPause >= this->msgState.curInstr->args.pause.duration)
                {
                    this->msgState.portraitVisible = 1;
                    this->msgState.pauseLimit = 0x1e;
                    break;
                }
                this->msgState.framesElapsedDuringPause++;
                goto SKIP_TIME_INCREMENT;
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            break;
        }
        case 22: // MSG_MUSIC_SELECT confirm
        {
            g_GameManager.flags.isGoingToFinalB = this->msgState.musicSelection;
            g_Gui.InitMsg(this->msgState.musicSelection + 1);
            continue;
        }
        case 4: // MSG_PAUSE
        {
            if (!(this->msgState.dialogueSkippable != 0 && IS_PRESSED(TH_BUTTON_SKIP)))
            {
                if (!(WAS_PRESSED(TH_BUTTON_SHOOT) && this->msgState.framesElapsedDuringPause >= this->msgState.pauseLimit))
                {
                    if (this->msgState.framesElapsedDuringPause >= this->msgState.curInstr->args.pause.duration)
                    {
                        this->msgState.portraitVisible = 1;
                        this->msgState.pauseLimit = 0x1e;
                        break;
                    }
                    this->msgState.framesElapsedDuringPause++;
                    goto SKIP_TIME_INCREMENT;
                }
                goto CASE4_CONFIRM;
            CASE4_CONFIRM:
                this->msgState.portraitVisible = 1;
                this->msgState.pauseLimit = 8;
                goto CASE4_BREAK;
            }
            goto CASE4_BREAK;
        CASE4_BREAK:
            break;
        }
        case 5: // MSG_SWITCH
        {
            args = &this->msgState.curInstr->args;
            this->msgState.vms[args->msgSwitch.unkIdx].prefix.pendingInterrupt = args->msgSwitch.interrupt;
            break;
        }
        case 6: // MSG_APPEAR_ENEMY
            this->msgState.ignoreWaitCounter++;
            break;
        case 7: // MSG_MUSIC
        {
            if (this->msgState.curInstr->args.music.musicIdx < 0)
            {
                g_Supervisor.StopAudio();
            }
            else
            {
                g_Gui.stageTextAnm->SetAndExecuteScriptIdx(&this->vmsB[3], 3);
                g_Gui.stageTextAnm->SetSprite(&this->vmsB[3], this->msgState.curInstr->args.music.musicIdx + 3);
                /* 0x4c7240：按 [关卡][BGM] 索引的只读数据表。 */
                if (g_Supervisor.PlayMusic(this->msgState.curInstr->args.music.musicIdx,
                                           *(i32 *)(0x4c7240 + g_GameManager.currentStage * 12 +
                                                    this->msgState.curInstr->args.music.musicIdx * 4)))
                {
                    g_Supervisor.PlayAudio((char *)&g_GuiBgmPathBase[this->msgState.curInstr->args.music.musicIdx * 0x80 + 0x290],
                                           *(i32 *)(0x4c7240 + g_GameManager.currentStage * 12 +
                                                    this->msgState.curInstr->args.music.musicIdx * 4));
                }
            }
            break;
        }
        case 8: // MSG_TEXT_INTRODUCE
            args = &this->msgState.curInstr->args;
            g_GuiPortraitAnms[2]->SetAndExecuteScriptIdx(&this->msgState.vms3[0], 1);
            this->msgState.framesElapsedDuringPause = 0;
            break;
        case 9: // MSG_STAGERESULTS
        {
            this->resultPower = g_GameManager.GetPower();
            this->resultPointItems = g_GameManager.globals->pointItemsCollectedInStage;
            this->resultTimeOrbs = g_GameManager.GetTimeOrbs();
            this->resultGraze = g_GameManager.globals->grazeInStage;
            this->resultTimeFrames = g_GameManager.GetClockTime() * 30 + 660;
            this->resultTime = g_GameManager.GetClockTimeIncrement();
            g_GameManager.AddToClockTime((i8)this->resultTime);
            /* 0x4c7158：按关卡索引的只读数据表。 */
            this->resultStage = ((i32 *)0x4c7158)[g_GameManager.currentStage];
            this->resultTimeFrames2 = g_GameManager.GetClockTime() * 30 + 660;
            this->resultTimeFramesCopy = this->resultTimeFrames;
            this->unk22e10 = 0;
            this->msgState.stageClearActive = 1;
            g_GameManager.flags.unk9 = 1;
            if (g_GameManager.currentStage != 6 && g_GameManager.currentStage != 7 &&
                g_GameManager.currentStage != 8)
            {
                g_GuiStageClearAnmA->SetAndExecuteScriptIdx(&this->vmJ, 3);
                g_GuiStageClearAnmA->SetSprite(&this->vmJ, this->resultTime + 0x80);
            }
            else
            {
                this->vmJ.currentInstruction = NULL;
            }
            this->vmJ.SetInterrupt(1);
            if (g_GameManager.currentStage != 6 && g_GameManager.currentStage != 7 &&
                g_GameManager.currentStage != 8)
            {
                g_Gui.loadingPortraitAnm->SetAndExecuteScriptIdx(&this->vmD, 0);
                g_GuiStageClearAnmB->SetAndExecuteScriptIdx(&this->vmF, 1);
                g_AnmManager->SetTextureCaptureParams(
                    3, 0x20, 0x10, 0x180, 0x1c0, (i32)this->vmF.loadedSprite->startPixelInclusive.x,
                    (i32)this->vmF.loadedSprite->startPixelInclusive.y, (i32)this->vmF.loadedSprite->widthPx,
                    (i32)this->vmF.loadedSprite->heightPx);
                for (i9 = 0; i9 < 8; i9++)
                {
                    g_GuiStageClearAnmB->SetAndExecuteScriptIdx(&this->vmsG[i9], 2);
                    this->vmsG[i9].prefix.counterVar0 = i9 * 4 + 3;
                    this->vmsG[i9].prefix.color1.a = 0x40 - i9 * 2;
                }
            }
            else
            {
                g_GameManager.globals->pointItemExtendsSoFar = -1;
            }
            if (g_GameManager.currentStage != 7 && g_GameManager.currentStage != 6 &&
                g_GameManager.currentStage != 8 && g_GameManager.GetBombsRemaining() < 3 &&
                (g_GameManager.shotType == 3 || g_GameManager.shotType == 10 ||
                 g_GameManager.shotType == 11))
            {
                g_GameManager.AddToBombCount(1);
                g_SoundPlayer.PlaySoundByIdx(SOUND_SPELL_CAPTURE, 0);
                g_Gui.flags.bombDisplayUpdateFrames = 2;
            }
            break;
        }
        case 10: // MSG_FREEZE
            goto SKIP_TIME_INCREMENT;
        case 12: // MSG_FADEOUT_MUSIC
            g_Supervisor.FadeOutMusic(4.0f);
            break;
        case 14: // MSG_FADE_IN_EFFECT
            ScreenEffect::RegisterChain(SCREEN_EFFECT_FULL_FADE_OUT, 0x1ba, COLOR_TEXT_WHITE, 0, 0, 0x15);
            g_Supervisor.unk174 = 0x1ba;
            break;
        case 11: // stage 6/7/8 progress mark
            if (g_GameManager.currentStage == 6 || g_GameManager.currentStage == 7 ||
                g_GameManager.currentStage == 8)
            {
                g_GameManager.flags.unk5 = 2;
            }
            goto SKIP_TIME_INCREMENT;
        case 13: // MSG_ALLOW_SKIP
            this->msgState.dialogueSkippable = *(u8 *)&this->msgState.curInstr->args;
            break;
        case 18: // dialogue box visibility
            this->msgState.dialogueBoxVisible = *(u8 *)&this->msgState.curInstr->args;
            break;
        default:
            break;
        }
        this->msgState.curInstr = (MsgRawInstr *)((u8 *)this->msgState.curInstr + 4 + this->msgState.curInstr->argsize);
    }
    this->msgState.timer++;
SKIP_TIME_INCREMENT:
    g_AnmManager->ExecuteScript(&this->msgState.vms[0]);
    g_AnmManager->ExecuteScript(&this->msgState.vms[1]);
    g_AnmManager->ExecuteScript(&this->msgState.vms[2]);
    g_AnmManager->ExecuteScript(&this->msgState.vms[3]);
    g_AnmManager->ExecuteScript(&this->msgState.vms2[0]);
    g_AnmManager->ExecuteScript(&this->msgState.vms2[1]);
    g_AnmManager->ExecuteScript(&this->msgState.vms3[0]);
    g_AnmManager->ExecuteScript(&this->msgState.vms3[1]);

    if (this->msgState.timer < 0x3c && this->msgState.dialogueSkippable && IS_PRESSED(TH_BUTTON_SKIP))
    {
        this->msgState.timer = 0x3c;
    }

    return ZUN_SUCCESS;
}

// FUNCTION: th08 0x43396d — start a message (select msg entry `arg`), loading the per-character text colors.
#pragma var_order(savedMsgFile, swapTmp7, swapTmp8)
void GuiImpl::InitMsg(i32 arg)
{
    u8 *savedMsgFile;
    i32 swapTmp7;
    i32 swapTmp8;

    utils::GuiDebugPrint("msg start %d\n\r", arg);
    savedMsgFile = this->msgState.msgFileData;
    memset(&this->msgState, 0, 0x1570);
    this->msgState.msgFileData = savedMsgFile;

    if (arg == 0)
    {
        switch (g_Unknown164d2cc)
        {
        case GAME_STATE_EVENT_5:
            FUN_00437f5c(0x16);
            break;
        case GAME_STATE_EVENT_6:
            g_SpellcardBgmOverride = 2;
            break;
        case GAME_STATE_EVENT_7:
            swapTmp7 = *(i32 *)0x4ecc9c;
            *(i32 *)0x4ecc9c = *(i32 *)0x4ecca0;
            *(i32 *)0x4ecca0 = swapTmp7;
            g_SpellcardBgmOverride = 2;
            FUN_00437f5c(0x18);
            break;
        case GAME_STATE_EVENT_8:
            swapTmp8 = *(i32 *)0x4ecc9c;
            *(i32 *)0x4ecc9c = *(i32 *)0x4ecca0;
            *(i32 *)0x4ecca0 = swapTmp8;
            g_SpellcardBgmOverride = 2;
            FUN_00437f5c(0x19);
            break;
        }
    }
    else if (arg == 0xa)
    {
        if (g_Unknown164d2cc == GAME_STATE_EVENT_5)
        {
            if (g_GameManager.globals->numRetries > 0)
            {
                arg = 1;
                this->msgState.musicSelection = 0;
            }
            else if (g_GameManager.IsReplay())
            {
                switch ((i8)g_ReplayManager->replayData->clearState)
                {
                case 2:
                    arg = 3;
                    this->msgState.musicSelection = 1;
                    break;
                case 1:
                    arg = 2;
                    this->msgState.musicSelection = 1;
                    break;
                default:
                    arg = 1;
                    this->msgState.musicSelection = 0;
                    break;
                }
            }
            else if (g_GameManager.IsStageClearedWithoutRetries(7, g_PlayerCharacter, 0) ||
                     g_GameManager.IsStageClearedWithoutRetries(7, g_PlayerCharacter, 1) ||
                     g_GameManager.IsStageClearedWithoutRetries(7, g_PlayerCharacter, 2) ||
                     g_GameManager.IsStageClearedWithoutRetries(7, g_PlayerCharacter, 3))
            {
                arg = 3;
                this->msgState.musicSelection = 1;
            }
            else if (g_GameManager.IsStageClearedWithRetries(6, g_PlayerCharacter, 0) ||
                     g_GameManager.IsStageClearedWithRetries(6, g_PlayerCharacter, 1) ||
                     g_GameManager.IsStageClearedWithRetries(6, g_PlayerCharacter, 2) ||
                     g_GameManager.IsStageClearedWithRetries(6, g_PlayerCharacter, 3))
            {
                arg = 2;
                this->msgState.musicSelection = 1;
            }
            else
            {
                arg = 1;
                this->msgState.musicSelection = 0;
            }
            g_PlayerFlags = (g_PlayerFlags & ~0x1800) | ((this->msgState.musicSelection & 3) << 0xb);
        }
    }
    else if (arg >= 6)
    {
        if (g_Unknown164d2cc == GAME_STATE_EVENT_7 && g_GameManager.GetClockTime() >= 0xc)
        {
            arg = 5;
        }
    }

    this->msgState.currentMsgIdx = arg;
    this->msgState.curInstr = (MsgRawInstr *)((GuiMsgData *)this->msgState.msgFileData)->offsets[arg];
    this->msgState.vms2[0].scriptIndex |= 0xffff;
    this->msgState.vms2[1].scriptIndex |= 0xffff;
    this->msgState.dialogueBoxVisible = 1;
    *(u32 *)&this->msgState.fontSize = 0xf;
    this->msgState.textColorsA[0] = *(i32 *)(0x4c7180 + g_PlayerCharacter * 16);
    this->msgState.textColorsA[1] = *(i32 *)(0x4c7180 + g_PlayerCharacter * 16 + 4);
    this->msgState.textColorsA[2] = *(i32 *)(0x4c7180 + g_PlayerCharacter * 16 + 8);
    this->msgState.textColorsA[3] = *(i32 *)(0x4c7180 + g_PlayerCharacter * 16 + 12);
    this->msgState.textColorsB[0] = 0;
    this->msgState.textColorsB[1] = 0;
    this->msgState.textColorsB[2] = 0;
    this->msgState.textColorsB[3] = 0;
    this->msgState.dialogueSkippable = 1;
    this->msgState.pauseLimit = 6;
    this->msgState.currentFace = 0;
    this->msgState.portraitVisible = 1;
    this->msgState.currentDialogueLine = 0;
    this->msgState.currentPortrait |= 0xff;
    g_BulletManager.bulletmanager_fun_00415c60();
    g_EnemyManager.RemoveEnemiesByScore(0, 0);
    g_ItemManager.AutoCollectAllItems();
}

// FUNCTION: th08 0x439810
void Gui::InitMsg(i32 arg)
{
    this->impl->InitMsg(arg);
}

// 原版这些 boss 状态 setter 属于 /Od 编译单元（纯 dword/byte 直传 + mov/pop epilogue）。
// 注意：本文件以 /Os 编译，VC7 #pragma optimize 不生效，代码生成与 /Od 略有差异
// （f32 赋值用 fld/fstp 而非原版 mov；SetBossPresent 缺 /Od 的冗余重读）。
// 语义正确，指令级匹配受编译选项限制。
// FUNCTION: th08 0x423130
void Gui::SetEclLives(i32 a0)
{
    this->eclSetLives = a0;
}

// FUNCTION: th08 0x4230e0
void Gui::SetBossLifeBarSegment(i32 a0, f32 a1, f32 a2)
{
    this->bossLifeBarSegmentStart[a0] = a1;
    this->bossLifeBarSegmentStop[a0] = a2;
}

// FUNCTION: th08 0x423110
void Gui::SetBossLifeSegmentColor(i32 a0, i32 a1)
{
    this->bossLifeBarSegmentColor[a0] = a1;
}

// FUNCTION: th08 0x422c20
void Gui::SetBossPresent(i32 a0)
{
    this->bossPresent = (u8)a0;
}

// FUNCTION: th08 0x4230c0
void Gui::SetBossLifeBarMaxSize(f32 a0)
{
    this->bossLifeBarMaxSize = a0;
}

// FUNCTION: th08 0x439007
ZunResult Gui::ShowClock()
{
    this->timesAnm->ExecuteAnmIdx(&this->impl->vmK, 2);
    this->timesAnm->SetSprite(&this->impl->vmK, g_GameManager.GetClockTime());
    return ZUN_SUCCESS;
}

// FUNCTION: th08 0x439050
ZunResult Gui::UpdateClockHour()
{
    this->timesAnm->SetSprite(&this->impl->vmK, g_GameManager.GetClockTime());
    this->impl->vmK.SetInterrupt(1);
    return ZUN_SUCCESS;
}

// FUNCTION: th08 0x439093
ZunResult Gui::UpdateClockNoon()
{
    this->timesAnm->SetSprite(&this->impl->vmK, g_GameManager.GetClockTime());
    this->impl->vmK.SetInterrupt(2);
    return ZUN_SUCCESS;
}

// FUNCTION: th08 0x4390d6
ZunResult Gui::ResetClock()
{
    this->impl->vmK.prefix.color1.a &= 0;
    return ZUN_SUCCESS;
}

// FUNCTION: th08 0x43542b — draw the dialogue box + the two portrait pairs (painter's order) + text VMs.
ZunResult GuiImpl::DrawDialogue()
{
    f32 alpha;

    if (this->msgState.currentMsgIdx < 0)
    {
        return ZUN_ERROR;
    }
    if (this->msgState.timer < 0x3c)
    {
        alpha = this->msgState.timer.AsFramesFloat() * 48.0f / 60.0f;
    }
    else
    {
        alpha = 48.0f;
    }
    VertexDiffuseXyzrhw verts[4];
    verts[0].pos = Float3(g_PlayerPos.x + 16.0f, 384.0f, 0.0f);
    verts[1].pos = Float3(g_PlayerPos.x + 384.0f - 16.0f, 384.0f, 0.0f);
    verts[2].pos = Float3(g_PlayerPos.x + 16.0f, 384.0f + alpha, 0.0f);
    verts[3].pos = Float3(g_PlayerPos.x + 384.0f - 16.0f, 384.0f + alpha, 0.0f);
    verts[0].diffuse = verts[1].diffuse = 0xd0000000;
    verts[2].diffuse = verts[3].diffuse = 0x90000000;
    verts[0].w = verts[1].w = verts[2].w = verts[3].w = 1.0f;

    if (this->msgState.vms[0].pos.z >= this->msgState.vms[1].pos.z)
    {
        g_AnmManager->DrawNoRotation(&this->msgState.vms[0]);
        g_AnmManager->DrawNoRotation(&this->msgState.vms[1]);
    }
    else
    {
        g_AnmManager->DrawNoRotation(&this->msgState.vms[1]);
        g_AnmManager->DrawNoRotation(&this->msgState.vms[0]);
    }
    if (this->msgState.vms[2].pos.z >= this->msgState.vms[3].pos.z)
    {
        g_AnmManager->DrawNoRotation(&this->msgState.vms[2]);
        g_AnmManager->DrawNoRotation(&this->msgState.vms[3]);
    }
    else
    {
        g_AnmManager->DrawNoRotation(&this->msgState.vms[3]);
        g_AnmManager->DrawNoRotation(&this->msgState.vms[2]);
    }
    g_AnmManager->FlushVertexBuffer();

    if (this->msgState.dialogueBoxVisible != 0)
    {
        if (!g_Supervisor.IsColorCompositingDisabled())
        {
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        }
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
        if (!g_Supervisor.IsDepthTestDisabled())
        {
            g_Supervisor.SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        }
        g_Supervisor.d3dDevice->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
        g_Supervisor.d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &verts, sizeof(VertexDiffuseXyzrhw));
        g_AnmManager->ClearVertexShader();
        g_AnmManager->ClearColorOp();
        g_AnmManager->ClearBlendMode();
        g_AnmManager->ClearZWriteSetting();
        if (!g_Supervisor.IsColorCompositingDisabled())
        {
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        }
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    }

    g_AnmManager->DrawNoRotation(&this->msgState.vms2[0]);
    g_AnmManager->DrawNoRotation(&this->msgState.vms2[1]);
    g_AnmManager->DrawNoRotation(&this->msgState.vms3[0]);
    g_AnmManager->DrawNoRotation(&this->msgState.vms3[1]);
    return ZUN_SUCCESS;
}

// FUNCTION: th08 0x435900 — boss HUD fade in/out + script exec for the HUD VMs + stage-clear score calc + time count-up.
#pragma var_order(i, inactiveCounter, j, result, k)
void Gui::UpdateBossHud()
{
    GuiImpl *impl;
    i32 i;
    i32 inactiveCounter;
    i32 j;
    i32 result;
    i32 k;

    impl = this->impl;
    if (impl->msgState.currentMsgIdx < 0)
    {
        if (this->bossPresent != 0)
        {
            if (impl->bossHudState == 0)
            {
                impl->vmsA[12].SetInterrupt(1);
                impl->bossHudState = 1;
                this->bossUIOpacity = 0;
            }
            else
            {
                if (FUN_004396f8(&impl->vmsA[12]) != 0)
                {
                    impl->bossHudState = 2;
                }
                if (this->bossUIOpacity < 0xfc)
                {
                    this->bossUIOpacity += 4;
                }
                else
                {
                    this->bossUIOpacity = 0xff;
                }
            }
        }
        else
        {
            if (impl->bossHudState != 0)
            {
                if (impl->bossHudState <= 2)
                {
                    impl->vmsA[12].SetInterrupt(2);
                    impl->bossHudState = 3;
                }
                if (this->bossUIOpacity > 0)
                {
                    this->bossUIOpacity -= 4;
                }
                else
                {
                    this->bossUIOpacity = 0;
                }
                if (FUN_004396f8(&impl->vmsA[12]) != 0)
                {
                    impl->bossHudState = 0;
                    this->bossLifeBarSize = 0.0f;
                    this->bossUIOpacity = 0;
                }
            }
        }
    }
    if (impl->bossHudState >= 2)
    {
        if (this->bossLifeBarMaxSize > this->bossLifeBarSize)
        {
            this->bossLifeBarSize += 0.01f;
            if (this->bossLifeBarMaxSize < this->bossLifeBarSize)
            {
                this->bossLifeBarSize = this->bossLifeBarMaxSize;
            }
        }
        else
        {
            this->bossLifeBarSize -= 0.02f;
            if (this->bossLifeBarMaxSize > this->bossLifeBarSize)
            {
                this->bossLifeBarSize = this->bossLifeBarMaxSize;
            }
        }
    }

    g_AnmManager->ExecuteScriptArray(&impl->vmsA[0], 0x10);
    g_AnmManager->ExecuteScriptArray(&impl->vmsB[0], 4);
    if (!((g_PlayerFlags >> 0xe) & 1))
    {
        if (impl->vmsB[0].prefix.color1.a != 0)
        {
            g_AnmManager->ExecuteScriptArray(&impl->vmC, 1);
        }
    }
    g_AnmManager->ExecuteScript(&impl->vmJ);
    g_AnmManager->ExecuteScript(&impl->vmK);
    if (impl->vmK.prefix.color1.a != 0)
    {
        if (g_EclExitLeftBound >= 64.0f && g_17d61b0 <= 128.0f)
        {
            if (impl->vmK.prefix.color1.a > 0x40)
            {
                impl->vmK.prefix.color1.a -= 4;
            }
        }
        else
        {
            if (impl->vmK.prefix.color1.a < 0xff)
            {
                if (impl->vmK.prefix.color1.a > 0xfb)
                {
                    impl->vmK.prefix.color1.a |= 0xff;
                }
                else
                {
                    impl->vmK.prefix.color1.a += 4;
                }
            }
        }
    }
    g_AnmManager->ExecuteScript(&impl->vmH);
    g_AnmManager->ExecuteScript(&impl->vmL);
    if (impl->vmD.activeSpriteIndex >= 0)
    {
        if (g_AnmManager->ExecuteScript(&impl->vmD) != 0)
        {
            impl->vmD.activeSpriteIndex = -1;
        }
        if (g_AnmManager->ExecuteScript(&impl->vmF) != 0)
        {
            impl->vmF.activeSpriteIndex = -1;
        }
    }
    for (i = 0; i < 8; i++)
    {
        g_AnmManager->ExecuteScript(&impl->vmsG[i]);
    }
    if (impl->popupA.unk0x10 != 0)
    {
        if (impl->popupA.timer < 0x1e)
        {
            impl->popupA.position.y = impl->popupA.timer.AsFramesFloat() * -312.0f / 30.0f + 416.0f;
        }
        else
        {
            impl->popupA.position.y = 104.0f;
        }
        if (impl->popupA.timer >= 0xfa)
        {
            impl->popupA.unk0x10 = 0;
        }
        impl->popupA.timer.Tick(0);
    }
    if (impl->popupB.unk0x10 != 0)
    {
        if (impl->popupB.timer < 0x1e)
        {
            impl->popupB.position.y = impl->popupB.timer.AsFramesFloat() * -312.0f / 30.0f + 416.0f;
        }
        else
        {
            impl->popupB.position.y = 104.0f;
        }
        if (impl->popupB.timer >= 0xb4)
        {
            impl->popupB.unk0x10 = 0;
        }
        impl->popupB.timer.Tick(0);
    }
    if (impl->popupC.unk0x10 != 0)
    {
        if (impl->popupC.timer >= 0x118)
        {
            impl->popupC.unk0x10 = 0;
        }
        impl->popupC.timer.Tick(0);
    }
    if (impl->msgState.stageClearActive == 1)
    {
        result = impl->resultStage;
        result += impl->resultGraze * 0x32;
        result += impl->resultPointItems * 0x1388;
        result += impl->resultTimeOrbs * 0x64;
        if (g_Unknown164d2cc >= 6)
        {
            if (!g_GameManager.IsPracticeMode())
            {
                result += g_GameManager.GetLives() * 0x2625a0;
                result += g_GameManager.GetBombsRemaining() * 0x7a120;
            }
        }
        if (g_Unknown164d2cc == 7)
        {
            result += (0xc - g_GameManager.GetClockTime()) * 0x1e8480;
        }
        switch (g_GameManager.difficulty)
        {
        case 0:
            result /= 2;
            break;
        case 2:
            result = result * 0xc / 0xa;
            break;
        case 3:
            result = result * 0xf / 0xa;
            break;
        case 4:
            result *= 2;
            break;
        }
        switch (*(u8 *)((u8 *)g_GameManager.cfg + 0x1c))
        {
        case 3:
            result = result * 5 / 10;
            break;
        case 4:
            result = result * 2 / 10;
            break;
        case 5:
            result = result / 10;
            break;
        case 6:
            result = result / 0x14;
            break;
        }
        impl->msgState.stageClearScore = result;
        for (k = 0; k < 0xa; k++)
        {
            g_GameManager.AddScore(result);
        }
        impl->msgState.stageClearActive++;
    }
    if (g_Unknown164d2cc < 6)
    {
        if (impl->resultTimeFramesCopy != 0 && impl->resultTimeFramesCopy >= impl->resultTimeFrames2)
        {
            if (((g_PlayerFlags >> 5) & 3) == 0)
            {
                g_PlayerFlags = (g_PlayerFlags & ~0x60) | 0x40;
            }
        }
    }
    if (impl->resultTimeFramesCopy != 0 && impl->resultTimeFramesCopy != impl->resultTimeFrames2)
    {
        if (impl->unk22e10 < 0x3c)
        {
            impl->unk22e10++;
        }
        else if (impl->resultTimeFramesCopy < impl->resultTimeFrames2)
        {
            impl->resultTimeFramesCopy++;
            if ((g_KeyInput & 1) || (g_KeyInput & 0x100))
            {
                impl->resultTimeFramesCopy += 3;
            }
            if (impl->resultTimeFramesCopy > impl->resultTimeFrames2)
            {
                impl->resultTimeFramesCopy = impl->resultTimeFrames2;
            }
        }
        else
        {
            impl->unk22e10++;
        }
    }
}

// FUNCTION: th08 0x43625d
#pragma var_order(x, y, i, vm)
void Gui::DrawGameScene()
{
    f32 x;
    f32 y;
    i32 i;
    AnmVm *vm;
    g_AnmManager->FlushVertexBuffer();
    g_Supervisor.viewport.X = 0;
    g_Supervisor.viewport.Y = 0;
    g_Supervisor.viewport.Width = 640;
    g_Supervisor.viewport.Height = 480;
    g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
    if (!g_Supervisor.IsMinimumGraphicsMode())
    {
        vm = (AnmVm *)((u8 *)this->impl + 0x279c);
        y = 480.0f;

        vm->pos = Float3(y, 40.0f, 0.49f);
        g_AnmManager->DrawNoRotation(vm);
        vm->pos = Float3(y, 56.0f, 0.49f);
        g_AnmManager->DrawNoRotation(vm);
        if (this->flags.lifeDisplayUpdateFrames)
        {
            vm->pos = Float3(y, 88.0f, 0.48f);
            g_AnmManager->DrawNoRotation(vm);
        }
        if (this->flags.bombDisplayUpdateFrames)
        {
            vm->pos = Float3(y, 104.0f, 0.48f);
            g_AnmManager->DrawNoRotation(vm);
        }
        if (this->flags.powerDisplayUpdateFrames)
        {
            vm->pos = Float3(y, 136.0f, 0.48f);
            g_AnmManager->DrawNoRotation(vm);
        }
        if (this->flags.grazeDisplayUpdateFrames)
        {
            vm->pos = Float3(y, 152.0f, 0.48f);
            g_AnmManager->DrawNoRotation(vm);
        }
        if (this->flags.pointDisplayUpdateFrames)
        {
            vm->pos = Float3(y, 168.0f, 0.48f);
            g_AnmManager->DrawNoRotation(vm);
        }
        if (this->flags.timeDisplayUpdateFrames)
        {
            vm->pos = Float3(y, 184.0f, 0.48f);
            g_AnmManager->DrawNoRotation(vm);
        }
        vm->pos = Float3(512.0f, 464.0f, 0.48f);
        g_AnmManager->DrawNoRotation(vm);
    }

    vm = (AnmVm *)((u8 *)this->impl + 0x2254);
    if (g_Supervisor.IsHUDRedrawEnabled() || vm->currentInstruction != NULL || g_Supervisor.unk174 != 0)
    {
        for (x = 0.0f; x < 464.0f; x = x + 32.0f)
        {
            vm->pos = Float3(0.0f, x, 0.49f);
            g_AnmManager->DrawNoRotation(vm);
        }
        for (y = 416.0f; y < 624.0f; y = y + 32.0f)
        {
            for (x = 16.0f; x < 464.0f; x = x + 32.0f)
            {
                vm->pos = Float3(y, x, 0.49f);
                g_AnmManager->DrawNoRotation(vm);
            }
        }
        vm = (AnmVm *)((u8 *)this->impl + 0x24f8);
        for (y = 0.0f; y < 624.0f; y = y + 128.0f)
        {
            vm->pos = Float3(y, 0.0f, 0.49f);
            g_AnmManager->DrawNoRotation(vm);
            vm->pos = Float3(y, 464.0f, 0.49f);
            g_AnmManager->DrawNoRotation(vm);
        }
        g_AnmManager->DrawNoRotation(&this->impl->vmsA[0]);
        g_AnmManager->Draw2D(&this->impl->vmsA[1]);
        g_AnmManager->DrawNoRotation(&this->impl->vmsA[2]);
        g_AnmManager->DrawNoRotation(&this->impl->vmsA[3]);
        g_AnmManager->DrawNoRotation(&this->impl->vmsA[4]);
        g_AnmManager->DrawNoRotation(&this->impl->vmsA[5]);
        g_AnmManager->DrawNoRotation(&this->impl->vmsA[6]);
        g_AnmManager->DrawNoRotation(&this->impl->vmsA[7]);
        g_AnmManager->DrawNoRotation(&this->impl->vmsA[8]);
        g_AnmManager->DrawNoRotation(&this->impl->vmsA[9]);
        g_AnmManager->DrawNoRotation((AnmVm *)((u8 *)this->impl + 0x22e14));
        this->flags.lifeDisplayUpdateFrames = 2;
        this->flags.bombDisplayUpdateFrames = 2;
        this->flags.grazeDisplayUpdateFrames = 2;
        this->flags.pointDisplayUpdateFrames = 2;
        this->flags.powerDisplayUpdateFrames = 2;
        this->flags.timeDisplayUpdateFrames = 2;
    }
    if (this->flags.lifeDisplayUpdateFrames)
    {
        vm = (AnmVm *)((u8 *)this->impl + 0x1a68);
        for (i = 0, y = 488.0f; i < (i32)g_GameManager.GetLives(); i++, y = y + 16.0f)
        {
            vm->pos = Float3(y, 88.0f, 0.46f);
            g_AnmManager->DrawNoRotation(vm);
        }
    }
    if (this->flags.bombDisplayUpdateFrames)
    {
        vm = (AnmVm *)((u8 *)this->impl + 0x1d0c);
        for (i = 0, y = 488.0f; i < (i32)g_GameManager.GetBombsRemaining(); i++, y = y + 16.0f)
        {
            vm->pos = Float3(y, 104.0f, 0.46f);
            g_AnmManager->DrawNoRotation(vm);
        }
    }
    if (this->flags.bombDisplayUpdateFrames || this->flags.lifeDisplayUpdateFrames)
    {
        if (g_GameManager.flags.unk7 == 1 && g_Spellcard.IsSpellcardActive())
        {
            g_AnmManager->DrawNoRotation((AnmVm *)((u8 *)this->impl + 0x5484));
        }
    }
    vm = (AnmVm *)((u8 *)this->impl + 0x24f8);
    for (x = 32.0f; x < 368.0f; x = x + 128.0f)
    {
        vm->pos = Float3(x, 40.0f, 0.49f);
        g_AnmManager->DrawNoRotation(vm);
    }

    {
        Float3 textDrawPos = Float3(488.0f, 56.0f, 0.0f);
        g_AsciiManager.AddFormatText(&textDrawPos, "%.9d", g_GameManager.globals->displayScore);
        textDrawPos.x = textDrawPos.x + 117.0f;
        g_AsciiManager.AddFormatText(&textDrawPos, "%1d", (i32)g_GameManager.globals->numRetries > 9 ? 9 : (i32)g_GameManager.globals->numRetries);
        g_AsciiManager.SetScale(1.0f, 1.0f);
    }
    {
        Float3 textDrawPos = Float3(488.0f, 40.0f, 0.0f);
        g_AsciiManager.AddFormatText(&textDrawPos, "%.9d", g_GameManager.globals->displayedHighScore);
        textDrawPos.x = textDrawPos.x + 117.0f;
        g_AsciiManager.AddFormatText(&textDrawPos, "%1d", (i32)g_GameManager.globals->ontinuesUsedInHighScore > 9 ? 9 : (i32)g_GameManager.globals->ontinuesUsedInHighScore);
        g_AsciiManager.SetScale(1.0f, 1.0f);
    }
    if (this->flags.grazeDisplayUpdateFrames || g_Supervisor.IsMinimumGraphicsMode())
    {
        Float3 textDrawPos = Float3(488.0f, 152.0f, 0.0f);
        g_AsciiManager.AddFormatText(&textDrawPos, "%d", g_GameManager.globals->graze);
    }
    if (this->flags.pointDisplayUpdateFrames || g_Supervisor.IsMinimumGraphicsMode())
    {
        Float3 textDrawPos = Float3(488.0f, 168.0f, 0.0f);
        g_AsciiManager.AddFormatText2(&textDrawPos, "%d", g_GameManager.globals->pointItemsCollected);
        textDrawPos.x = textDrawPos.x + (f32)(g_GameManager.globals->pointItemsCollected * 13);
        g_AsciiManager.SetScale(0.5f, 1.0f);
        g_AsciiManager.AddFormatText(&textDrawPos, "/");
        g_AsciiManager.SetScale(1.0f, 1.0f);
        textDrawPos.x = textDrawPos.x + 6.0f;
        g_AsciiManager.AddFormatText(&textDrawPos, "%d", g_GameManager.globals->nextPointItemExtendThreshold);
    }
    if (this->flags.timeDisplayUpdateFrames || g_Supervisor.IsMinimumGraphicsMode())
    {
        if (g_GameManager.GetTimeOrbs() >= g_GameManager.GetLastSpellTimeOrbThreshold())
        {
            g_AsciiManager.SetColor(COLOR_TIME_ORBS_LIMIT);
        }
        Float3 textDrawPos = Float3(488.0f, 184.0f, 0.0f);
        g_AsciiManager.AddFormatText2(&textDrawPos, "%d", g_GameManager.GetTimeOrbs());
        textDrawPos.x = textDrawPos.x + (f32)(g_GameManager.GetTimeOrbs() * 13);
        g_AsciiManager.SetScale(0.5f, 1.0f);
        g_AsciiManager.AddFormatText(&textDrawPos, "/");
        g_AsciiManager.SetScale(1.0f, 1.0f);
        textDrawPos.x = textDrawPos.x + 6.0f;
        g_AsciiManager.AddFormatText(&textDrawPos, "%d", g_GameManager.GetLastSpellTimeOrbThreshold());
        g_AsciiManager.SetColor(COLOR_WHITE);
    }
    g_AnmManager->FlushVertexBuffer();
    if (this->flags.powerDisplayUpdateFrames || g_Supervisor.IsMinimumGraphicsMode())
    {
        VertexDiffuseXyzrhw powerBarVerts[4];

        if (g_GameManager.GetPower() > 0)
        {
            powerBarVerts[0].pos = Float3(488.0f, 136.0f, 0.1f);
            powerBarVerts[1].pos = Float3((f32)(g_GameManager.GetPower() + 0x1e8) + 0.0f, 136.0f, 0.1f);
            powerBarVerts[2].pos = Float3(488.0f, 152.0f, 0.1f);
            powerBarVerts[3].pos = Float3((f32)(g_GameManager.GetPower() + 0x1e8) + 0.0f, 152.0f, 0.1f);
            powerBarVerts[0].diffuse = powerBarVerts[2].diffuse = COLOR_POWER_BAR_MAIN;
            powerBarVerts[1].diffuse = powerBarVerts[3].diffuse = COLOR_POWER_BAR_EDGE;
            powerBarVerts[0].w = powerBarVerts[1].w = powerBarVerts[2].w = powerBarVerts[3].w = 1.0f;
            if (!g_Supervisor.IsColorCompositingDisabled())
            {
                g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
                g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
            }
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
            if (!g_Supervisor.IsDepthTestDisabled())
            {
                g_Supervisor.SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            }
            g_Supervisor.d3dDevice->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
            g_Supervisor.d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &powerBarVerts, sizeof(VertexDiffuseXyzrhw));
            g_AnmManager->ClearVertexShader();
            g_AnmManager->ClearColorOp();
            g_AnmManager->ClearBlendMode();
            g_AnmManager->ClearZWriteSetting();
            if (!g_Supervisor.IsColorCompositingDisabled())
            {
                g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
                g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
            }
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        }
        if (g_GameManager.GetPower() < 128)
        {
            g_AsciiManager.AddFormatText(&Float3(488.0f, 136.0f, 0.0f), "%d", g_GameManager.GetPower());
        }
        else
        {
            g_AsciiManager.AddFormatText(&Float3(488.0f, 136.0f, 0.0f), "MAX");
        }
    }
    if (this->flags.lifeDisplayUpdateFrames)
    {
        this->flags.lifeDisplayUpdateFrames--;
    }
    if (this->flags.powerDisplayUpdateFrames)
    {
        this->flags.powerDisplayUpdateFrames--;
    }
    if (this->flags.bombDisplayUpdateFrames)
    {
        this->flags.bombDisplayUpdateFrames--;
    }
    if (this->flags.grazeDisplayUpdateFrames)
    {
        this->flags.grazeDisplayUpdateFrames--;
    }
    if (this->flags.pointDisplayUpdateFrames)
    {
        this->flags.pointDisplayUpdateFrames--;
    }
    if (this->flags.timeDisplayUpdateFrames)
    {
        this->flags.timeDisplayUpdateFrames--;
    }
}

// FUNCTION: th08 0x43741d — draw boss HUD: life bar (white bar + colored segments + spellcard countdown ticks),
// boss portrait vms (vmD/E/F, vmsG, vmsI) and the spellcard countdown text (AsciiManager).
#pragma var_order(i, color2, color1, rect, count, j, v, colWidth)
void Gui::DrawBossHud()
{
    i32 i;
    i32 j;
    f32 v;
    ZunRect rect;
    i32 color1;
    i32 color2;
    i32 count;
    i32 colWidth;

    for (i = 0; i < 4; i++)
    {
        g_AnmManager->Draw2D(&this->impl->vmsB[i]);
    }
    g_AnmManager->Draw2D(&this->impl->vmC);
    g_AnmManager->Draw2D(&this->impl->vmK);

    if (this->impl->vmD.activeSpriteIndex >= 0)
    {
        g_AnmManager->DrawNoRotation(&this->impl->vmD);
        g_AnmManager->DrawScaledBullet(&this->impl->vmF);
        for (i = 0; i < 8; i++)
        {
            g_AnmManager->DrawScaledBullet(&this->impl->vmsG[i]);
        }
        if (this->impl->vmE.activeSpriteIndex >= 0)
        {
            this->impl->vmE.pos = Float3(304.0f, 448.0f, 0.0f);
            g_AnmManager->DrawNoRotation(&this->impl->vmE);
        }
    }
    if (this->impl->inactiveVmsICount != 0)
    {
        for (i = 0; i < 0xa8; i++)
        {
            g_AnmManager->DrawScaledBullet(&this->impl->vmsI[i]);
            g_AnmManager->ClearSprite();
        }
    }
    if (this->impl->msgState.currentMsgIdx >= 0)
    {
        return;
    }
    if ((i32)this->bossPresent + this->impl->bossHudState <= 0)
    {
        return;
    }

    /* boss 血条主体（白色部分 + 半透明灰边）。 */
    rect.left = 64.0f;
    rect.top = 19.0f;
    rect.right = this->bossLifeBarSize * 320.0f + 64.0f;
    rect.bottom = 23.0f;
    color1 = (this->bossUIOpacity << 24) | 0xffffff;
    color2 = (this->bossUIOpacity << 24) | 0x202060;
    DRAW_SQUARE_SHADED(&rect, color1, color1, color2, color2);

    /* 血条彩色分段（segmentStart > 0 且 segmentStop != 当前血量时绘制）。 */
    for (j = 0; j < 8; j++)
    {
        if (this->bossLifeBarSegmentStart[j] != 0.0f &&
            this->bossLifeBarSegmentStop[j] != this->bossLifeBarSize)
        {
            v = this->bossLifeBarSegmentStart[j];
            if (this->bossLifeBarSize < v)
            {
                v = this->bossLifeBarSize;
            }
            rect.left = this->bossLifeBarSegmentStop[j] * 320.0f + 64.0f;
            rect.top = 19.0f;
            rect.right = v * 320.0f + 64.0f;
            rect.bottom = 23.0f;
            color1 = (this->bossUIOpacity << 24) | (this->bossLifeBarSegmentColor[j] & 0xffffff);
            color2 = (this->bossUIOpacity << 24) | ((this->bossLifeBarSegmentColor[j] >> 2) & 0x3f3f3f);
            DRAW_SQUARE_SHADED(&rect, color1, color1, color2, color2);
        }
    }

    g_AnmManager->DrawNoRotation(&this->impl->vmsA[12]);

    /* 符卡倒计时秒格（spellcardSecondsRemaining 个 tick，剩余 ≤5 秒时格宽 +1）。 */
    rect.left = 33.0f;
    rect.top = 19.0f;
    rect.right = rect.left + 3.0f;
    rect.bottom = rect.top + 4.0f;
    count = this->spellcardSecondsRemaining;
    colWidth = (count <= 5) ? 2 : 1;
    for (j = 0; j < count; j++)
    {
        rect.left = (f32)j * 26.0f / count + 35.0f;
        rect.right = (f32)(j + 1) * 26.0f / count + 35.0f - colWidth;
        color1 = (this->bossUIOpacity << 24) | (0xffffff - j * 0xff / 9);
        color2 = (this->bossUIOpacity << 24) | 0x202020;
        DRAW_SQUARE_SHADED(&rect, color1, color1, color2, color2);
    }

    /* 符卡倒计时数字（颜色随剩余秒数变化）。 */
    {
        Float3 textPos = Float3(384.0f, 16.0f, 0.0f);
        i32 color;

        if (this->spellcardSecondsRemaining >= 0x14)
        {
            color = *(i32 *)0x4c72ac;
        }
        else if (this->spellcardSecondsRemaining >= 0xa)
        {
            color = *(i32 *)0x4c72b0;
        }
        else if (this->spellcardSecondsRemaining >= 0x5)
        {
            color = *(i32 *)0x4c72b4;
        }
        else
        {
            color = *(i32 *)0x4c72b8;
        }
        g_AsciiManager.SetColor((this->bossUIOpacity << 24) | color);

        i32 seconds = this->spellcardSecondsRemaining;
        if (seconds > 0x63)
        {
            seconds = 0x63;
        }
        if (this->previousSpellcardSecondsRemaining != this->spellcardSecondsRemaining)
        {
            if (seconds < 3)
            {
                g_SoundPlayer.PlaySoundByIdx(SOUND_TIMEOUT_2, 0);
            }
            else if (seconds < 10)
            {
                g_SoundPlayer.PlaySoundByIdx(SOUND_TIMEOUT, 0);
            }
        }
        g_AsciiManager.AddFormatText(&textPos, "%.2d", seconds);
        g_AsciiManager.SetColor(0xffffffff);
        this->previousSpellcardSecondsRemaining = this->spellcardSecondsRemaining;

        /* 暂停/重试菜单不显示，且当前有子弹对象时，弹出连击数。 */
        if (*(u8 *)0x164d0ba == 0 && *(u8 *)0x164d0bb == 0 && ((g_PlayerFlags >> 0xa) & 1) == 0 &&
            g_BulletObjects[0] != 0)
        {
            textPos = Float3(2.0f, 29.0f, 0.0f);
            g_AsciiManager.SetScale(1.0f, 1.0f);
            ((StubThiscallAsciiManagerCreatePopup4 *)&g_AsciiManager)
                ->CreatePopup4(&textPos, ((Enemy *)g_BulletObjects[0])->GetSubEnemyChainCount(),
                               *(i32 *)((u8 *)g_BulletObjects[0] + 0x3380), 0xfff0f00f);
        }
    }

    g_AnmManager->DrawNoRotation(&this->impl->vmJ);
}

void Gui::ShowPopupB(i32 arg1, i32 arg2)
{
    this->impl->popupB.position = Float3(416.0f, 168.0f, 0.0f);
    this->impl->popupB.unk0x10 = arg2;
    this->impl->popupB.timer = 0;
    this->impl->popupB.unk0xc = arg1;
    g_Supervisor.unk174 = 2;
}

// STUB: th08 0x43826b
void Gui::DrawStageClearHud()
{
}

// STUB: th08 0x438a89
void Gui::DrawHud()
{
}

// FUNCTION: th08 0x438f58 (stage-clear text: run script and set the capture region)
void Gui::UpdateStageClearText()
{
    g_GuiStageClearAnmB->SetAndExecuteScriptIdx(&this->impl->vmF, 1);
    g_AnmManager->SetTextureCaptureParams(
        0x3, 0x20, 0x10, 0x180, 0x1c0, (i32)this->impl->vmF.loadedSprite->startPixelInclusive.x,
        (i32)this->impl->vmF.loadedSprite->startPixelInclusive.y,
        (i32)this->impl->vmF.loadedSprite->widthPx, (i32)this->impl->vmF.loadedSprite->heightPx);
}

} /* namespace th08 */
