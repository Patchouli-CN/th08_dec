#include "th_pch.h"

#include "Gui.hpp"
#include "ItemManager.hpp"
#include "Player.hpp"
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
    gui->FUN_00435900();
    gui->impl->RunMsg();
    if ((g_CurFrameInput & TH_BUTTON_SKIP) != 0)
    {
        if (g_Supervisor.unk174 < 8)
        {
            g_Supervisor.unk174 = 8;
        }
    }
    gui->unk_0++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult Gui::OnDraw(Gui *gui)
{
    if (gui->impl->msgState.unk1570 != 0)
    {
        gui->FUN_0043826b();
    }
    gui->impl->DrawDialogue();
    gui->FUN_0043741d();
    gui->DrawGameScene();
    gui->FUN_00438a89();
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

// STUB: th08 0x4390ee
ZunResult Gui::ActualAddedCallback()
{
    return ZUN_SUCCESS;
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
i32 Gui::FUN_00437d87()
{
    i32 result;

    if (*(i16 *)(*(i32 *)((u8 *)this + 8) + 0x398c) >= 0 &&
        FUN_004396f8((AnmVm *)(*(i32 *)((u8 *)this + 8) + 0x3778)) != 0)
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
i32 Gui::FUN_004358bb()
{
    i32 result;

    /* 无活动对象或对象未处于"等待"状态 → 0。 */
    if (*(i32 *)((u8 *)this + 8) == 0)
    {
        return 0;
    }
    if (*(i32 *)(*(i32 *)((u8 *)this + 8) + 0x2181c) < 0 &&
        *(i32 *)(*(i32 *)((u8 *)this + 8) + 0x2181c) != -2)
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
            g_Gui.FUN_00439810(this->msgState.musicSelection + 1);
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
            *(i32 *)((u8 *)this + 0x22df0) = g_GameManager.GetPower();
            *(i32 *)((u8 *)this + 0x22df4) = g_GameManager.globals->pointItemsCollectedInStage;
            *(i32 *)((u8 *)this + 0x22dfc) = g_GameManager.GetTimeOrbs();
            *(i32 *)((u8 *)this + 0x22df8) = g_GameManager.globals->grazeInStage;
            *(i32 *)((u8 *)this + 0x22e04) = g_GameManager.GetClockTime() * 30 + 660;
            *(i32 *)((u8 *)this + 0x22e00) = g_GameManager.GetClockTimeIncrement();
            g_GameManager.AddToClockTime((i8)*(i32 *)((u8 *)this + 0x22e00));
            *(i32 *)((u8 *)this + 0x22dec) = ((i32 *)0x4c7158)[g_GameManager.currentStage];
            *(i32 *)((u8 *)this + 0x22e08) = g_GameManager.GetClockTime() * 30 + 660;
            *(i32 *)((u8 *)this + 0x22e0c) = *(i32 *)((u8 *)this + 0x22e04);
            *(i32 *)((u8 *)this + 0x22e10) = 0;
            this->msgState.unk1570 = 1;
            g_GameManager.flags.unk9 = 1;
            if (g_GameManager.currentStage != 6 && g_GameManager.currentStage != 7 &&
                g_GameManager.currentStage != 8)
            {
                g_GuiStageClearAnmA->SetAndExecuteScriptIdx(&this->vmJ, 3);
                g_GuiStageClearAnmA->SetSprite(&this->vmJ, *(i32 *)((u8 *)this + 0x22e00) + 0x80);
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
                    *((u8 *)&this->vmsG[i9].prefix.color1.d3dColor + 3) = 0x40 - i9 * 2;
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
            ScreenEffect::RegisterChain(SCREEN_EFFECT_FULL_FADE_OUT, 0x1ba, 0xffffff, 0, 0, 0x15);
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
            this->msgState.unk156d = *(u8 *)&this->msgState.curInstr->args;
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

// STUB: th08 0x43396d (msg init, 0x446 bytes - separate task)
void GuiImpl::FUN_0043396d(i32 arg)
{
}

// FUNCTION: th08 0x439810
void Gui::FUN_00439810(i32 arg)
{
    this->impl->FUN_0043396d(arg);
}

void Gui::FUN_00423130(i32 a0)
{
}

void Gui::FUN_004230e0(i32 a0, f32 a1, f32 a2)
{
}

void Gui::FUN_00423110(i32 a0, i32 a1)
{
}

void Gui::FUN_00439007()
{
}

void Gui::FUN_00439050()
{
}

void Gui::FUN_00439093()
{
}

void Gui::FUN_004390d6()
{
}

// STUB: th08 0x43542b
void GuiImpl::DrawDialogue()
{
}

// STUB: th08 0x435900
void Gui::FUN_00435900()
{
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
        if (g_GameManager.flags.unk7 == 1 && g_Spellcard.spellcard_fun_004178a0())
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
            g_AsciiManager.SetColor(0xfffff0c0);
        }
        Float3 textDrawPos = Float3(488.0f, 184.0f, 0.0f);
        g_AsciiManager.AddFormatText2(&textDrawPos, "%d", g_GameManager.GetTimeOrbs());
        textDrawPos.x = textDrawPos.x + (f32)(g_GameManager.GetTimeOrbs() * 13);
        g_AsciiManager.SetScale(0.5f, 1.0f);
        g_AsciiManager.AddFormatText(&textDrawPos, "/");
        g_AsciiManager.SetScale(1.0f, 1.0f);
        textDrawPos.x = textDrawPos.x + 6.0f;
        g_AsciiManager.AddFormatText(&textDrawPos, "%d", g_GameManager.GetLastSpellTimeOrbThreshold());
        g_AsciiManager.SetColor(0xffffffff);
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
            powerBarVerts[0].diffuse = powerBarVerts[2].diffuse = 0xe0e0e0ff;
            powerBarVerts[1].diffuse = powerBarVerts[3].diffuse = 0x80e0e0ff;
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

// STUB: th08 0x43741d
void Gui::FUN_0043741d()
{
}

void Gui::FUN_00437e5d(i32 arg1, i32 arg2)
{
    this->impl->popupB.position = Float3(416.0f, 168.0f, 0.0f);
    this->impl->popupB.unk0x10 = arg2;
    this->impl->popupB.timer = 0;
    this->impl->popupB.unk0xc = arg1;
    g_Supervisor.unk174 = 2;
}

// STUB: th08 0x43826b
void Gui::FUN_0043826b()
{
}

// STUB: th08 0x438a89
void Gui::FUN_00438a89()
{
}

// FUNCTION: th08 0x438f58 (stage-clear text: run script and set the capture region)
void Gui::FUN_00438f58()
{
    g_GuiStageClearAnmB->SetAndExecuteScriptIdx(&this->impl->vmF, 1);
    g_AnmManager->SetTextureCaptureParams(
        0x3, 0x20, 0x10, 0x180, 0x1c0, (i32)this->impl->vmF.loadedSprite->startPixelInclusive.x,
        (i32)this->impl->vmF.loadedSprite->startPixelInclusive.y,
        (i32)this->impl->vmF.loadedSprite->widthPx, (i32)this->impl->vmF.loadedSprite->heightPx);
}

} /* namespace th08 */
