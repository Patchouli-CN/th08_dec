#pragma once

#include "AnmManager.hpp"
#include "Global.hpp"
#include "utils.hpp"

namespace th08
{

struct MsgRawInstr;

struct GuiMsgState
{
    u8 *msgFileData;
    MsgRawInstr *curInstr;
    i32 currentMsgIdx;
    ZunTimer timer;
    unknown_fields(0x18, 0x8);
    AnmVm vms[4];
    AnmVm vms2[2];
    AnmVm vms3[2];
    unknown_fields(0x1540, 0x24);
    u32 ignoreWaitCounter; // 0x1564
    u8 dialogueSkippable;  // 0x1568
    unknown_fields(0x1569, 0x3);
    i32 unk156c; // 0x156c
    i32 unk1570;
    unknown_fields(0x1574, 0x4);
};
C_ASSERT(sizeof(GuiMsgState) == 0x1578);

struct GuiPopup
{
    Float3 position;
    i32 unk0xc;
    i32 unk0x10;
    ZunTimer timer;
};
C_ASSERT(sizeof(GuiPopup) == 0x20);

struct GuiMsgData
{
    u32 entryCount;
    u32 offsets[1];
};

union MsgRawInstrArgs {
    struct
    {
        i16 portraitIdx;
        i16 anmScriptIdx;
    } portrait;
    struct
    {
        i16 textColor;
        i16 textLine;
        char text[5];
    } dialogue;
    struct
    {
        i32 duration;
    } pause;
    struct
    {
        i16 unkIdx;
        u8 interrupt;
    } msgSwitch;
    struct
    {
        i32 musicIdx;
    } music;
};

struct MsgRawInstr
{
    u16 time; // 0x0
    u8 opcode; // 0x2
    u8 argsize; // 0x3
    MsgRawInstrArgs args; // 0x4
};

struct GuiImpl
{
    ZunResult RunMsg();
    void DrawDialogue();

    AnmVm vmsA[16];
    u32 unk2a40;
    AnmVm vmsB[4];
    AnmVm vmC;
    AnmVm vmD;
    AnmVm vmE;
    AnmVm vmF;
    AnmVm vmsG[8];
    AnmVm vmH;
    AnmVm vmsI[168];
    AnmVm vmJ;
    AnmVm vmK;
    u32 unk21810;
    GuiMsgState msgState;
    GuiPopup popupA;
    GuiPopup popupB;
    GuiPopup popupC;
    unknown_fields(0x22dec, 0x28);
    AnmVm vmL;
};
C_ASSERT(sizeof(GuiImpl) == 0x230b8);

struct GuiFlags
{
    u32 lifeDisplayUpdateFrames : 2;
    u32 bombDisplayUpdateFrames : 2;
    u32 powerDisplayUpdateFrames : 2;
    u32 grazeDisplayUpdateFrames : 2;
    u32 pointDisplayUpdateFrames : 2;
    u32 timeDisplayUpdateFrames : 2;
};

#define MAX_BOSS_LIFEBAR_SEGMENTS 8

struct Gui
{
    static ChainCallbackResult OnUpdate(Gui *gui);
    static ChainCallbackResult OnDraw(Gui *gui);

    static ZunResult AddedCallback(Gui *gui);
    static ZunResult DeletedCallback(Gui *gui);

    static ZunResult RegisterChain();
    static void CutChain();

    ZunResult ActualAddedCallback();
    ZunResult LoadMsg(const char *path);
    void FreeMsgFile();
    void FUN_00435900();
    void DrawGameScene();
    void FUN_0043741d();
    void FUN_00437e5d(i32, i32);
    void FUN_0043826b();
    void FUN_00438a89();

    u32 unk_0;
    GuiFlags flags;
    GuiImpl *impl;
    AnmLoaded *frontAnm;
    AnmLoaded *stageTextAnm;
    AnmLoaded *timesAnm;
    AnmLoaded *loadingPortraitAnm;
    u32 bossUIOpacity;
    i32 eclSetLives;
    i32 spellcardSecondsRemaining;
    i32 previousSpellcardSecondsRemaining;
    bool bossPresent;
    f32 bossLifeBarMaxSize;
    f32 bossLifeBarSize;
    unknown_fields(0x38, 0x4);
    f32 bossLifeBarSegmentStop[MAX_BOSS_LIFEBAR_SEGMENTS];
    f32 bossLifeBarSegmentStart[MAX_BOSS_LIFEBAR_SEGMENTS];
    i32 bossLifeBarSegmentColor[MAX_BOSS_LIFEBAR_SEGMENTS];
};
C_ASSERT(sizeof(Gui) == 0x9c);

DIFFABLE_EXTERN(Gui, g_Gui);

} /* namespace th08 */
