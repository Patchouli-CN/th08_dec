#pragma once
#include "AnmManager.hpp"
#include "Global.hpp"
#include "Supervisor.hpp"
#include "ZunColor.hpp"
#include "ZunResult.hpp"
#include "diffbuild.hpp"
#include "inttypes.hpp"

#include <d3dx8.h>

#define ASCII_MAX_STRINGS 256
#define ASCII_MAX_SCORE_POPUPS 720
#define ASCII_MAX_PLAYER_POPUPS 3
#define ASCII_MAX_TIME_POPUPS 128

namespace th08
{

struct PauseMenu
{
    i32 OnUpdate();
    void OnDrawPauseMenu();

    u32 curState;
    i32 numFrames;
    AnmVm menuSprites[10];
    AnmVm menuBackground;
};

C_ASSERT(sizeof(PauseMenu) == 0x1d14);

struct RetryMenu
{
    i32 OnUpdate();
    void OnDrawRetryMenu();

    u32 curState;
    i32 numFrames;

    AnmVm menuSprites[6];
    AnmVm menuBackground;
};

C_ASSERT(sizeof(RetryMenu) == 0x1284);

struct AsciiManagerString
{
    char text[64];
    Float3 position;
    D3DCOLOR color;
    f32 scaleX;
    f32 scaleY;
    ZunBool isSelected;
    ZunBool isGui;
};

C_ASSERT(sizeof(AsciiManagerString) == 0x60);

struct AsciiManagerPopup
{
    char text[12];
    Float3 position;
    D3DCOLOR color;
    ZunTimer timer;
    f32 scaleX;
    f32 scaleY;
    bool inUse;
    BYTE characterCount;
    u32 unk0x34; // unused in current code
};

C_ASSERT(sizeof(AsciiManagerPopup) == 0x38);

struct AsciiManager
{
    static ChainCallbackResult OnUpdate(AsciiManager *mgr);
    static ChainCallbackResult OnDrawLowPrio(AsciiManager *mgr);
    static ChainCallbackResult OnDrawHighPrio(AsciiManager *mgr);
    static ZunResult RegisterChain();
    static ZunResult AddedCallback(AsciiManager *mgr);
    static ZunResult DeletedCallback(AsciiManager *mgr);
    static void CutChain();
    void AddString(Float3 *position, const char *string);
    void AddFormatText(Float3 *position, const char *fmt, ...);
    int AddFormatText2(Float3 *position, const char *fmt, ...);
    void OnDrawLowPrioImpl();
    void ResetStringsCount();
    void CreateScorePopup(Float3 *position, i32 number, D3DCOLOR color);
    void CreatePlayerPointPopup(Float3 *position, i32 number, D3DCOLOR color);
    void CreateTimePopup(Float3 *position, i32 number, i32 param3, D3DCOLOR color);
    void CreateFamiliarPopup(Float3 *position, i32 number, i32 param3, D3DCOLOR color);
    void OnDrawHighPrioImpl();
    void DrawPercentage(Float3 *position, i32 percentage, D3DCOLOR color);

    void Reset();
    void InitializeVms();
    void SetBossMarkerInterrupt(i32 idx, i16 interrupt);
    void SetBossMarkerPosition(i32 idx, Float3 *pos);

    void SetColor(D3DCOLOR color)
    {
        this->color.d3dColor = color;
    }

    void SetIsGuiMode(u32 isGuiMode);
    void SetBossMarkerState(i32 idx, u32 state);
    void SetIsSelected(ZunBool selected)
    {
        this->isSelected = selected;
    }

    void SetScale(float scaleX, float scaleY)
    {
        /* orig stores the float params as raw u32 bit patterns (mov, not fld/fstp) */
        *(u32 *)&this->scaleX = *(u32 *)&scaleX;
        *(u32 *)&this->scaleY = *(u32 *)&scaleY;
    }

    void UpdateVms()
    {
        g_AnmManager->ExecuteScript(&this->youkaiGauge);
        g_AnmManager->ExecuteScript(&this->youkaiGaugeHumanIcon);
        g_AnmManager->ExecuteScript(&this->youkaiGaugeYoukaiIcon);
        g_AnmManager->ExecuteScript(&this->youkaiGaugeCursor);
        g_AnmManager->ExecuteScript(&this->percentageText);
        g_AnmManager->ExecuteScript(&this->bossMarkers[0]);
        g_AnmManager->ExecuteScript(&this->bossMarkers[1]);
        g_AnmManager->ExecuteScript(&this->bossMarkers[2]);
        g_AnmManager->ExecuteScript(&this->bossMarkers[3]);
        g_AnmManager->ExecuteScript(&this->unk_1520);
    }

    void SetGaugeInterrupt(i32 interrupt);

    i32 GetGaugeInterrupt();

    void ResetStrings()
    {
        this->numStrings = 0;
    }

    void SetSpaceWidth(i32 spaceWidth)
    {
        this->spaceWidth = spaceWidth;
    }

    AnmVm largeText;
    AnmVm smallScoreText;
    AnmVm popupText;
    AnmVm youkaiGauge;
    AnmVm youkaiGaugeHumanIcon;
    AnmVm youkaiGaugeYoukaiIcon;
    AnmVm youkaiGaugeCursor;
    AnmVm percentageText;
    AnmVm unk_1520; // HUD anm vm (script 9), executed each frame; purpose still unknown

    AnmVm bossMarkers[4];
    i32 bossMarkerStates[4];

    AsciiManagerString strings[ASCII_MAX_STRINGS];
    i32 numStrings;

    ZunColor color;
    f32 scaleX;
    f32 scaleY;
    ZunBool isGui;
    ZunBool isSelected;

    i32 gaugeInterrupt;
    i32 spaceWidth;
    u32 frameCounter; // 0x8284  frame counter for blink timing (mod 8/4/2 in the HUD)

    AnmLoaded *asciiAnm;
    AnmLoaded *captureAnm;

    i32 nextScorePopupIndex;
    i32 nextPlayerPointPopupIndex;
    i32 nextTimePopupIndex;

    ZunBool unk0x829c; // zeroed in Reset; write-only so far

    PauseMenu pauseMenu;
    RetryMenu retryMenu;

    AnmVm demoIcon;

    AsciiManagerPopup scorePopups[ASCII_MAX_SCORE_POPUPS + ASCII_MAX_PLAYER_POPUPS];
    AsciiManagerPopup timePopups[ASCII_MAX_TIME_POPUPS];

    f32 boundaryIndicatorOffset; // 0x16f04  player boundary-indicator bar half-width/offset
    i32 boundaryIndicatorTimer;  // 0x16f08  >0 while the boundary indicator bars are shown; also the alpha

    AnmVm boundaryIndicatorVm;   // 0x16f0c  the boundary-indicator bar anm vm (script 0x69)
};

C_ASSERT(sizeof(AsciiManager) == 0x171b0);
DIFFABLE_EXTERN(AsciiManager, g_AsciiManager);

} // namespace th08
