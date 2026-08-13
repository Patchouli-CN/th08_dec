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
    i32 framesElapsedDuringPause; // 0x18
    i32 pauseLimit;               // 0x1c
    AnmVm vms[4];
    AnmVm vms2[2];
    AnmVm vms3[2];
    i32 textColorsA[4];          // 0x1540
    i32 textColorsB[4];          // 0x1550
    u8 fontSize;                 // 0x1560
    unknown_fields(0x1561, 0x3);
    u32 ignoreWaitCounter; // 0x1564
    u8 dialogueSkippable;  // 0x1568
    u8 currentFace;        // 0x1569
    u8 portraitVisible;    // 0x156a
    u8 currentDialogueLine; // 0x156b
    u8 currentPortrait;    // 0x156c
    u8 dialogueBoxVisible; // 0x156d  (set by msg opcode 18)
    u8 musicSelection;     // 0x156e
    u8 unk156f;            // 0x156f  (write-only so far)
    i32 stageClearActive;  // 0x1570  (set when the stage-clear result sequence runs)
    i32 stageClearScore;   // 0x1574  (computed by UpdateBossHud, drawn by DrawStageClearHud)
};
C_ASSERT(sizeof(GuiMsgState) == 0x1578);

struct GuiPopup
{
    Float3 position;
    i32 unk0xc;  // popup param (ShowPopupB arg1; write-only so far)
    i32 unk0x10; // popup param (ShowPopupB arg2; write-only so far)
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
        i32 portraitIdx;
        i32 anmScriptIdx0;
        i32 anmScriptIdx1;
        i32 anmScriptIdx2;
        i32 anmScriptIdx3;
    } showPortrait;
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
    void InitMsg(i32 arg);
    ZunResult DrawDialogue();

    AnmVm vmsA[16];
    u8 bossHudState; // 0x2a40 (boss HUD anim state: 0=hidden, 1=fade in, 2=shown, 3=fade out)
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
    i32 inactiveVmsICount; // 0x21810 (168 minus the number of vmsI that finished their script)
    GuiMsgState msgState;
    GuiPopup popupA;
    GuiPopup popupB;
    GuiPopup popupC;
    i32 resultStage;            /* 0x22dec 按关卡查表（0x4c7158） */
    i32 resultPower;            /* 0x22df0 */
    i32 resultPointItems;       /* 0x22df4 */
    i32 resultGraze;            /* 0x22df8 */
    i32 resultTimeOrbs;         /* 0x22dfc */
    i32 resultTime;             /* 0x22e00 时间（秒增量，也用于时间精灵 +0x80） */
    i32 resultTimeFrames;       /* 0x22e04 时钟帧数*30+660 */
    i32 resultTimeFrames2;      /* 0x22e08 */
    i32 resultTimeFramesCopy;   /* 0x22e0c 0x22e04 的副本 */
    i32 unk22e10;               /* 0x22e10 (zeroed with the result fields) */
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
    void InitMsg(i32 arg);
    void SetEclLives(i32 a0);          // 0x423130 (ECL 设定剩余命数)
    void SetBossLifeBarSegment(i32 a0, f32 a1, f32 a2); // 0x4230e0 (boss 血条段 start/stop)
    void SetBossLifeSegmentColor(i32 a0, i32 a1);       // 0x423110 (boss 血条段颜色)
    void SetBossPresent(i32 a0);       // 0x422c20 (boss 在场标记可见性)
    void SetBossLifeBarMaxSize(f32 a0); // 0x4230c0 (boss 血条最大尺寸)
    ZunResult ShowClock();             // 0x439007 (显示当前小时时钟)
    ZunResult UpdateClockHour();       // 0x439050 (时钟报时+1, 非 12 时)
    ZunResult UpdateClockNoon();       // 0x439093 (时钟报时到 12 时)
    ZunResult ResetClock();            // 0x4390d6 (清时钟显示标志)
    static ChainCallbackResult OnUpdate(Gui *gui);
    static ChainCallbackResult OnDraw(Gui *gui);

    static ZunResult AddedCallback(Gui *gui);
    static ZunResult DeletedCallback(Gui *gui);

    static ZunResult RegisterChain();
    static void CutChain();

    ZunResult ActualAddedCallback();
    void FUN_004396b8();              // 0x4396b8 (reset boss portrait vmsD/E/F + inactiveVmsICount)
    ZunResult LoadMsg(const char *path);
    void FreeMsgFile();
    i32 MsgWait();
    i32 IsBossPortraitVisible();      // 0x437d87 (boss portrait vmD active & animating)
    i32 IsMsgActive();                // 0x4358bb (message currently displayed)
    void UpdateBossHud();             // 0x435900 (per-frame boss life-bar/spell timer update)
    void DrawGameScene();
    void DrawBossHud();               // 0x43741d (boss life bar, portrait and related vms)
    void ShowPopupB(i32, i32);        // 0x437e5d (popupB at (416,168), g_Supervisor.unk174 = 2)
    void DrawStageClearHud();         // 0x43826b (stage-clear result text)
    void DrawHud();                   // 0x438a89 (state-based HUD/gauge display)
    void UpdateStageClearText();      // 0x438f58 (stage-clear text script + capture region)

    u32 frameCounter;
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
