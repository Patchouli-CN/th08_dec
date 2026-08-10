#include "th_pch.h"

#include "Gui.hpp"
#include "ItemManager.hpp"
#include "Player.hpp"
#include "ScreenEffect.hpp"

namespace th08
{

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

// STUB: th08 0x433db3
// FUNCTION: th08 0x433db3
ZunResult GuiImpl::RunMsg()
{
    MsgRawInstr *curInstr;

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
            MsgRawInstrArgs *args = &this->msgState.curInstr->args;
            i32 i;

            if (this->msgState.currentPortrait != args->showPortrait.portraitIdx)
            {
                for (i = 0; i < 4; i++)
                {
                    if (this->msgState.currentPortrait == i)
                    {
                        if (this->msgState.currentPortrait / 2 == args->showPortrait.portraitIdx / 2)
                        {
                            this->msgState.vms[i].prefix.pendingInterrupt = 4;
                        }
                        else
                        {
                            this->msgState.vms[i].prefix.pendingInterrupt = 6;
                        }
                    }
                    else
                    {
                        this->msgState.vms[i].prefix.pendingInterrupt = 4;
                    }
                }
            }
            this->msgState.vms[args->showPortrait.portraitIdx].prefix.pendingInterrupt = 3;
            this->msgState.currentPortrait = (u8)args->showPortrait.portraitIdx;

            if (args->showPortrait.anmScriptIdx0 >= 0)
            {
                g_GuiPortraitAnms[0]->SetSprite(&this->msgState.vms[0], args->showPortrait.anmScriptIdx0);
            }
            if (args->showPortrait.anmScriptIdx1 >= 0)
            {
                g_GuiPortraitAnms[1]->SetSprite(&this->msgState.vms[1], args->showPortrait.anmScriptIdx1);
            }
            if (args->showPortrait.anmScriptIdx2 >= 0)
            {
                g_GuiPortraitAnms[2]->SetSprite(&this->msgState.vms[2], args->showPortrait.anmScriptIdx2);
            }
            if (args->showPortrait.anmScriptIdx3 >= 0)
            {
                g_GuiPortraitAnms[3]->SetSprite(&this->msgState.vms[3], args->showPortrait.anmScriptIdx3);
            }

            this->msgState.currentFace = (u8)args->showPortrait.portraitIdx;
            this->msgState.portraitVisible = 1;
            break;
        }
        case 17: // MSG_CHANGE_FACE
        {
            MsgRawInstrArgs *args = &this->msgState.curInstr->args;
            u32 i;

            if (this->msgState.currentPortrait != *(i32 *)&args->portrait)
            {
                for (i = 0; i < 4; i++)
                {
                    if (this->msgState.currentPortrait == i)
                    {
                        if (this->msgState.currentPortrait / 2 == *(i32 *)&args->portrait / 2)
                        {
                            this->msgState.vms[i].prefix.pendingInterrupt = 4;
                        }
                        else
                        {
                            this->msgState.vms[i].prefix.pendingInterrupt = 6;
                        }
                    }
                    else
                    {
                        this->msgState.vms[i].prefix.pendingInterrupt = 4;
                    }
                }
            }
            this->msgState.vms[*(i32 *)&args->portrait].prefix.pendingInterrupt = 3;
            this->msgState.currentPortrait = (u8)*(i32 *)&args->portrait;

            if (args->portrait.anmScriptIdx >= 0)
            {
                switch (*(i32 *)&args->portrait)
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
            }

            this->msgState.currentFace = (u8)*(i32 *)&args->portrait;
            this->msgState.portraitVisible = 1;
            break;
        }
        case 1: // MSG_SHOW_PORTRAIT (single)
        {
            MsgRawInstrArgs *args = &this->msgState.curInstr->args;
            i32 portraitIdx = args->portrait.portraitIdx;

            switch (portraitIdx)
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
            if (this->msgState.vms[portraitIdx].loadedSprite->widthPx > 128.0f)
            {
                this->msgState.vms[portraitIdx].pos2.x = -112.0f;
            }
            else
            {
                this->msgState.vms[portraitIdx].pos2.x = 0.0f;
            }
            break;
        }
        case 2: // MSG_CHANGE_FACE (single)
        {
            MsgRawInstrArgs *args = &this->msgState.curInstr->args;
            i32 portraitIdx = args->portrait.portraitIdx;

            switch (portraitIdx)
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
            if (this->msgState.vms[portraitIdx].loadedSprite->widthPx > 256.0f)
            {
                this->msgState.vms[portraitIdx].pos2.x = -208.0f;
                this->msgState.vms[portraitIdx].pos2.y = -50.0f;
            }
            else if (this->msgState.vms[portraitIdx].loadedSprite->widthPx > 128.0f)
            {
                this->msgState.vms[portraitIdx].pos2.x = -80.0f;
            }
            else
            {
                this->msgState.vms[portraitIdx].pos2.x = 0.0f;
            }
            break;
        }
        case 3: // MSG_DIALOGUE
        {
            MsgRawInstrArgs *args = &this->msgState.curInstr->args;
            i32 textLine = args->dialogue.textLine;
            i32 textColor = args->dialogue.textColor;
            char textBuffer[8];

            if (textLine == 0 && (i32)this->msgState.vms2[1].scriptIndex >= 0)
            {
                g_AnmManager->DrawTextLeft(&this->msgState.vms2[1], this->msgState.textColorsA[textColor],
                                           this->msgState.textColorsB[textColor], " ");
            }
            g_Supervisor.textAnm->SetAndExecuteScriptIdx(&this->msgState.vms2[textLine], (i16)textLine);
            this->msgState.vms2[textLine].fontHeight = this->msgState.fontSize;
            this->msgState.vms2[textLine].fontWidth = this->msgState.vms2[textLine].fontHeight;
            DecryptMsgText(textBuffer, args->dialogue.text);
            g_AnmManager->DrawTextLeft(&this->msgState.vms2[textLine], this->msgState.textColorsA[textColor],
                                       this->msgState.textColorsB[textColor], textBuffer);
            this->msgState.framesElapsedDuringPause = 0;
            break;
        }
        case 16: // MSG_DIALOGUE variant
        {
            MsgRawInstrArgs *args = &this->msgState.curInstr->args;
            i32 textColor = this->msgState.currentFace;
            char textBuffer[8];

            if (this->msgState.portraitVisible != 0)
            {
                if ((i32)this->msgState.vms2[1].scriptIndex >= 0)
                {
                    g_AnmManager->DrawTextLeft(&this->msgState.vms2[1], this->msgState.textColorsA[textColor],
                                               this->msgState.textColorsB[textColor], " ");
                }
                this->msgState.currentDialogueLine = 0;
            }
            g_Supervisor.textAnm->SetAndExecuteScriptIdx(&this->msgState.vms2[this->msgState.currentDialogueLine],
                                                         this->msgState.currentDialogueLine);
            this->msgState.vms2[this->msgState.currentDialogueLine].fontHeight = this->msgState.fontSize;
            this->msgState.vms2[this->msgState.currentDialogueLine].fontWidth =
                this->msgState.vms2[this->msgState.currentDialogueLine].fontHeight;
            DecryptMsgText(textBuffer, args->dialogue.text);
            g_AnmManager->DrawTextLeft(&this->msgState.vms2[this->msgState.currentDialogueLine],
                                       this->msgState.textColorsA[textColor], this->msgState.textColorsB[textColor],
                                       textBuffer);
            this->msgState.framesElapsedDuringPause = 0;
            this->msgState.portraitVisible = 0;
            this->msgState.currentDialogueLine++;
            break;
        }
        case 19: // MSG_DIALOGUE variant
        {
            MsgRawInstrArgs *args = &this->msgState.curInstr->args;
            char textBuffer[8];

            g_Supervisor.textAnm->SetAndExecuteScriptIdx(&this->msgState.vms2[0], 0);
            this->msgState.vms2[0].fontHeight = this->msgState.fontSize;
            this->msgState.vms2[0].fontWidth = this->msgState.vms2[0].fontHeight;
            DecryptMsgText(textBuffer, args->dialogue.text);
            g_AnmManager->DrawTextLeft(&this->msgState.vms2[0], this->msgState.textColorsA[0],
                                       this->msgState.textColorsB[0], textBuffer);
            this->msgState.framesElapsedDuringPause = 0;
            break;
        }
        case 20: // MSG_DIALOGUE variant
        {
            MsgRawInstrArgs *args = &this->msgState.curInstr->args;
            char textBuffer[8];

            g_Supervisor.textAnm->SetAndExecuteScriptIdx(&this->msgState.vms2[1], 1);
            this->msgState.vms2[1].fontHeight = this->msgState.fontSize;
            this->msgState.vms2[1].fontWidth = this->msgState.vms2[1].fontHeight;
            DecryptMsgText(textBuffer, args->dialogue.text);
            g_AnmManager->DrawTextLeft(&this->msgState.vms2[1], this->msgState.textColorsA[0],
                                       this->msgState.textColorsB[0], textBuffer);
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
                    g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
                }
                this->msgState.musicSelection = 1;
            }
            this->msgState.vms2[this->msgState.musicSelection].prefix.color2.d3dColor = 0xFFFFFFFF;
            this->msgState.vms2[1 - this->msgState.musicSelection].prefix.color2.d3dColor = 0xE0606060;

            if (WAS_PRESSED(TH_BUTTON_SHOOT) && this->msgState.framesElapsedDuringPause >= 0x3c)
            {
                g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
                break;
            }
            if (this->msgState.framesElapsedDuringPause >= (u32)this->msgState.curInstr->args.pause.duration)
            {
                this->msgState.portraitVisible = 1;
                this->msgState.pauseLimit = 0x1e;
                break;
            }
            this->msgState.framesElapsedDuringPause++;
            goto SKIP_TIME_INCREMENT;
        }
        case 22: // MSG_MUSIC_SELECT confirm
        {
            g_GameManager.flags.isGoingToFinalB = this->msgState.musicSelection;
            g_Gui.FUN_00439810(this->msgState.musicSelection + 1);
            continue;
        }
        case 4: // MSG_PAUSE
        {
            if (this->msgState.dialogueSkippable != 0 && IS_PRESSED(TH_BUTTON_SKIP))
            {
                break;
            }
            if (WAS_PRESSED(TH_BUTTON_SHOOT) && this->msgState.framesElapsedDuringPause >= this->msgState.pauseLimit)
            {
                this->msgState.portraitVisible = 1;
                this->msgState.pauseLimit = 8;
                break;
            }
            if (this->msgState.framesElapsedDuringPause >= (u32)this->msgState.curInstr->args.pause.duration)
            {
                this->msgState.portraitVisible = 1;
                this->msgState.pauseLimit = 0x1e;
                break;
            }
            this->msgState.framesElapsedDuringPause++;
            goto SKIP_TIME_INCREMENT;
        }
        case 5: // MSG_SWITCH
        {
            MsgRawInstrArgs *args = &this->msgState.curInstr->args;
            this->msgState.vms[args->msgSwitch.unkIdx].prefix.pendingInterrupt = args->msgSwitch.interrupt;
            break;
        }
        case 6: // MSG_APPEAR_ENEMY
            this->msgState.ignoreWaitCounter++;
            break;
        case 7: // MSG_MUSIC
        {
            i32 musicIdx = this->msgState.curInstr->args.music.musicIdx;
            if (musicIdx < 0)
            {
                g_Supervisor.StopAudio();
            }
            else
            {
                g_Gui.stageTextAnm->SetAndExecuteScriptIdx(&this->vmsB[3], 3);
                g_Gui.stageTextAnm->SetSprite(&this->vmsB[3], musicIdx + 3);
                g_Supervisor.PlayMusic(musicIdx, (char *)&g_GuiBgmPathBase[musicIdx * 0x80 + 0x290]);
            }
            break;
        }
        case 8: // MSG_TEXT_INTRODUCE
            g_GuiPortraitAnms[2]->SetAndExecuteScriptIdx(&this->msgState.vms3[0], 1);
            this->msgState.framesElapsedDuringPause = 0;
            break;
        case 9: // MSG_STAGERESULTS
        {
            i32 i;
            i32 *stageResults = (i32 *)((u8 *)this + 0x22dec);

            stageResults[1] = g_GameManager.GetPower();
            stageResults[2] = g_GameManager.globals->pointItemsCollectedInStage;
            stageResults[4] = g_GameManager.GetTimeOrbs();
            stageResults[3] = g_GameManager.globals->grazeInStage;
            stageResults[6] = g_GameManager.GetClockTime() * 30 + 660;
            stageResults[5] = g_GameManager.GetClockTimeIncrement();
            g_GameManager.AddToClockTime((i8)stageResults[5]);
            stageResults[0] = ((i32 *)0x4c7158)[g_GameManager.currentStage];
            stageResults[7] = g_GameManager.GetClockTime() * 30 + 660;
            stageResults[8] = stageResults[6];
            stageResults[9] = 0;
            this->msgState.unk1570 = 1;
            g_GameManager.flags.unk9 = 1;
            if (g_GameManager.currentStage == 6 || g_GameManager.currentStage == 7 ||
                g_GameManager.currentStage == 8)
            {
                this->vmJ.currentInstruction = NULL;
            }
            else
            {
                g_GuiStageClearAnmA->SetAndExecuteScriptIdx(&this->vmJ, 3);
                g_GuiStageClearAnmA->SetSprite(&this->vmJ, stageResults[5] + 0x80);
            }
            this->vmJ.SetInterrupt(1);
            if (g_GameManager.currentStage == 6 || g_GameManager.currentStage == 7 ||
                g_GameManager.currentStage == 8)
            {
                g_GameManager.globals->pointItemExtendsSoFar = -1;
            }
            else
            {
                g_Gui.loadingPortraitAnm->SetAndExecuteScriptIdx(&this->vmD, 0);
                g_GuiStageClearAnmB->SetAndExecuteScriptIdx(&this->vmF, 1);
                g_AnmManager->SetTextureCaptureParams(
                    3, 0x20, 0x10, 0x180, 0x1c0, (i32)this->vmF.loadedSprite->startPixelInclusive.x,
                    (i32)this->vmF.loadedSprite->startPixelInclusive.y, (i32)this->vmF.loadedSprite->widthPx,
                    (i32)this->vmF.loadedSprite->heightPx);
                for (i = 0; i < 8; i++)
                {
                    g_GuiStageClearAnmB->SetAndExecuteScriptIdx(&this->vmsG[i], 2);
                    this->vmsG[i].prefix.counterVar0 = i * 4 + 3;
                    *((u8 *)&this->vmsG[i].prefix.color1.d3dColor + 3) = 0x40 - i * 2;
                }
                if (g_GameManager.GetBombsRemaining() < 3 &&
                    (g_GameManager.unk3dbaa == 3 || g_GameManager.unk3dbaa == 10 ||
                     g_GameManager.unk3dbaa == 11))
                {
                    g_GameManager.AddToBombCount(1);
                    g_SoundPlayer.PlaySoundByIdx(SOUND_SPELL_CAPTURE, 0);
                    g_Gui.flags.bombDisplayUpdateFrames = 2;
                }
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

// STUB: th08 0x43542b
void GuiImpl::DrawDialogue()
{
}

// STUB: th08 0x435900
void Gui::FUN_00435900()
{
}

// STUB: th08 0x43625d
void Gui::DrawGameScene()
{
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

} /* namespace th08 */
