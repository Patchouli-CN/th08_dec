#include "th_pch.h"

#include "Gui.hpp"
#include "ItemManager.hpp"
#include "Player.hpp"

namespace th08
{

DIFFABLE_STATIC(Gui, g_Gui);
DIFFABLE_STATIC(ChainElem, g_GuiCalcChain);
DIFFABLE_STATIC(ChainElem, g_GuiDrawChain);
DIFFABLE_STATIC_ARRAY(AnmLoaded *, 4, g_GuiPortraitAnms);

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
        // WIP: cases laid out in the original's source order (opcode values 0-22).
        // Only case 0 (DELETE) is implemented; the rest need decoding from 0x433db3.
        switch (this->msgState.curInstr->opcode)
        {
        case 0: // MSG_DELETE
            this->msgState.currentMsgIdx = -1;
            return ZUN_ERROR;
        case 15: // MSG_SHOW_PORTRAIT
        {
            MsgRawInstrArgs *args = &this->msgState.curInstr->args;
            i32 i;
            i32 portraitIdx = args->showPortrait.portraitIdx;
            i32 currentPortrait = this->msgState.currentPortrait;

            if (currentPortrait != portraitIdx)
            {
                for (i = 0; i < 4; i++)
                {
                    if (currentPortrait == i)
                    {
                        this->msgState.vms[i].prefix.pendingInterrupt =
                            (currentPortrait / 2 == portraitIdx / 2) ? 4 : 6;
                    }
                    else
                    {
                        this->msgState.vms[i].prefix.pendingInterrupt = 4;
                    }
                }
            }
            this->msgState.vms[portraitIdx].prefix.pendingInterrupt = 3;
            this->msgState.currentPortrait = portraitIdx;

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

            this->msgState.currentFace = (u8)portraitIdx;
            this->msgState.portraitVisible = 1;
            break;
        }
        case 17: // MSG_CHANGE_FACE
        {
            MsgRawInstrArgs *args = &this->msgState.curInstr->args;
            i32 i;
            i32 portraitIdx = args->portrait.portraitIdx;
            i32 currentPortrait = this->msgState.currentPortrait;

            if (currentPortrait != portraitIdx)
            {
                for (i = 0; i < 4; i++)
                {
                    if (currentPortrait == i)
                    {
                        this->msgState.vms[i].prefix.pendingInterrupt =
                            (currentPortrait / 2 == portraitIdx / 2) ? 4 : 6;
                    }
                    else
                    {
                        this->msgState.vms[i].prefix.pendingInterrupt = 4;
                    }
                }
            }
            this->msgState.vms[portraitIdx].prefix.pendingInterrupt = 3;
            this->msgState.currentPortrait = portraitIdx;

            if (args->portrait.anmScriptIdx >= 0)
            {
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
            }

            this->msgState.currentFace = (u8)portraitIdx;
            this->msgState.portraitVisible = 1;
            break;
        }
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        case 16:
            break;
        case 19:
            break;
        case 20:
            break;
        case 21:
            break;
        case 22:
            break;
        case 4:
            break;
        case 5:
            break;
        case 6:
            break;
        case 7:
            break;
        case 8:
            break;
        case 9:
            break;
        case 10:
            break;
        case 12:
            break;
        case 14:
            break;
        case 11:
            break;
        case 13:
            break;
        case 18:
            break;
        default:
            break;
        }
        this->msgState.curInstr = (MsgRawInstr *)((u8 *)this->msgState.curInstr + 4 + this->msgState.curInstr->argsize);
    }

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
