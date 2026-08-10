#include "th_pch.h"

#include "Gui.hpp"

namespace th08
{

DIFFABLE_STATIC(Gui, g_Gui);
DIFFABLE_STATIC(ChainElem, g_GuiCalcChain);
DIFFABLE_STATIC(ChainElem, g_GuiDrawChain);

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
    this->impl->msgState.unk08 = -1;
    this->impl->msgState.unk04 = 0;
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
void GuiImpl::RunMsg()
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
