#pragma once
#include "Global.hpp"
#include "ZunResult.hpp"
#include "diffbuild.hpp"
#include "inttypes.hpp"
#include "utils.hpp"
#include <windows.h>

namespace th08
{

// TODO: incomplete, only the fields referenced by currently decompiled code are mapped out.
struct StdRawHeader
{
    i16 objectsCount;
    i16 quadCount;
    u32 facesOffset;
    u32 scriptOffset;
    unknown_fields(0xc, 0x284);
    char bgmPaths[4][128];
};

struct StdRawQuadBasic
{
    i16 type;
    i16 byteSize;
    i16 anmScript;
    i16 vmIndex;
    Float3 pos;
    Float2 size;
};

struct StdRawObject
{
    u16 id;
    i8 zLevel;
    i8 flags;
    Float3 pos;
    Float3 size;
    StdRawQuadBasic firstQuad;
};

struct StdRawInstance
{
    i16 id;
    i16 field1_0x2;
    Float3 pos;
};

union AnyArg {
    i32 i;
    u32 u;
    f32 f;
    i16 s[2];
    u16 us[2];
    i8 c[4];
    u8 b[4];
};

struct StdRawInstrArgs
{
    AnyArg args[3];

    Float3 *AsVec()
    {
        return (Float3 *)args;
    }
};

struct StdRawInstr
{
    i32 frame;
    i16 opcode;
    i16 size;
    StdRawInstrArgs args;
};

struct StageFog
{
    f32 nearPlane;
    f32 farPlane;
    ZunColor color;
};

struct BackgroundCamera
{
    Float3 pos;
    Float3 target;
    Float3 up;
    Float3 unk0x24;
    Float3 unk0x30;
    Float3 unk0x3c;
    f32 fov;
};

struct Background
{
    Background();

    static ChainCallbackResult OnUpdate(Background *background);
    static ChainCallbackResult OnDrawHighPrio(Background *background);
    static ChainCallbackResult OnDrawLowPrio(Background *background);
    static ZunResult AddedCallback(Background *background);
    static ZunResult RegisterChain(i32 stage);
    static ZunResult DeletedCallback(Background *background);
    static void CutChain();
    ZunResult LoadStageData(char *stdPath);

    void SetCamera1();

    void SetCamera2()
    {
    }

    void __fastcall InterpolateCamera(i32 idx, Float3 *p1, Float3 *p2, Float3 *p3, Float3 *p4, Float3 *p5); // 0x408d60 (camera/pos ease between p2/p3, mode 7 = cubic via p4/p5)
    void UpdateStageTint();  // 0x409f40 (stage-tint transition on human/youkai switch)
    void RenderObjects(u32 param);

    void *fileData;
    AnmVm unk0x4;    // boss-background layer vm (drawn when the boss portrait is hidden)
    AnmVm unk0x2a8;  // boss-background layer vm (second layer)
    AnmVm unk0x54c;  // tint/fade layer vm; its color1 feeds screenTintColor
    AnmLoaded *stageAnm;
    StdRawHeader *stdData;
    i32 quadCount;
    i32 objectsCount;
    StdRawObject **objects;
    StdRawInstance *objectInstances;
    StdRawInstr *beginningOfScript;
    ZunTimer timer0x80c;   // 0x80c script frame timer
    i32 scriptIndex;       // 0x818 current position in the script dispatch
    u32 frameCounter;      // 0x81c per-frame counter (also drives particle bursts)
    i32 currentStage;
    Float3 bgPosition;     // 0x824 background position offset
    i32 screenTintColor;   // 0x830 full-screen tint color
    u8 unk0x834;           // 0x834 stage-tint change pending flag (gated by UpdateStageTint)
    unknown_fields(0x835, 0x3);
    ZunTimer timer0x838;   // 0x838 stage-tint transition timer
    AnmVm stageTintVm;          /* 0x844 场景色调 VM（color1 作为物体 VM 的逐通道染色；中断 1=妖/2=人） */
    AnmVm *stage7Effect;   // 0xae8 stage-7/EX special effect vm (moon/eientei effect)
    StageFog fog;
    StageFog fogFadeFrom;
    StageFog fogFadeTo;
    u32 fogFadeDuration;   // 0xb10 fog fade duration in frames
    ZunTimer timer0xb14;   // 0xb14 fog fade progress timer
    u8 skyFogNeedsSetup; // Leftover from earlier games. Never checked in IN
    unknown_fields(0xb21, 0x3);
    u32 bgPhase;           // 0xb24 background phase (0/1 normal, >=2 "clear" phase)
    i32 bgPhaseTimer;      // 0xb28 frames since the current phase began
    i32 screenClearNeeded;       /* 0xb2c 非 0 时清屏一次（设置视口后清零） */
    i32 objectVmCount;     // 0xb30 number of objectVms to execute this frame
    unknown_fields(0xb34, 0x4);
    AnmVm objectVms[0x20];
    AnmVm unk0x5fb8;
    void (*drawCallback)(Background *background); // 0x625c per-stage extra draw hook
    u32 seekElementId;     // 0x6260 nonzero -> jump the script to the element with this id (op 0x1f)
    BackgroundCamera camera0;
    BackgroundCamera camera1;
    BackgroundCamera camera2;
    BackgroundCamera camera3;
    BackgroundCamera camera4;
    u32 cameraInterpDurations[4]; // 0x63e0 interp durations for cameras 0-3
    u32 unk0x63f0;
    ZunTimer timers0x63f4[5];     // 0x63f4 interp timers (0-3 camera, 4 = fog/tint)
    i32 cameraInterpModes[5];     // 0x6430 interp easing modes (0-3 camera, 4 = fog/tint)
    Float3 unk0x6444;      // 0x6444 target position for the position transition
    u32 unk0x6450;         // 0x6450 target frame for the position transition
    Float3 unk0x6454;      // 0x6454 current position (mirror of bgPosition)
    unknown_fields(0x6460, 0x4);
    u8 unk0x6464;          // 0x6464 (start frame of the position transition; overlaps 0x6460 block)
    unknown_fields(0x6465, 0x3);
    ZunColor skyColor;           /* 0x6468 天空/背景颜色，最高字节(alpha)兼作"颜色已设置"标志 */
    u32 mixColorSet;       // 0x646c nonzero while a custom mix color is applied
    f32 unk0x6470;         // 0x6470 (constant 0x49a17020 bit pattern)
    u8 unk0x6474;          // 0x6474 sky/fog draw mode (switch in OnUpdate)
    unknown_fields(0x6475, 0x3);
    u8 unk0x6478;          // 0x6478 flag (cleared when bgPhase >= 2)
    unknown_fields(0x6479, 0x3);
    u32 needsRedraw;       // 0x647c set each OnUpdate, cleared after the draw pass
    Float3 effectPositions[0x20]; // 0x6480 spawn positions for ambient effects
};
C_ASSERT(sizeof(Background) == 0x6600);

DIFFABLE_EXTERN(Background, g_Background);
}; // Namespace th08
