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

    void SetCamera1()
    {
    }

    void SetCamera2()
    {
    }

    void FUN_00408d60(i32 idx, Float3 *p1, Float3 *p2, Float3 *p3, Float3 *p4, Float3 *p5);
    void FUN_00409f40();

    void *fileData;
    AnmVm unk0x4;
    AnmVm unk0x2a8;
    AnmVm unk0x54c;
    AnmLoaded *stageAnm;
    StdRawHeader *stdData;
    i32 quadCount;
    i32 objectsCount;
    StdRawObject **objects;
    StdRawInstance *objectInstances;
    StdRawInstr *beginningOfScript;
    ZunTimer timer0x80c;
    i32 unk0x818;
    u32 unk81c;
    i32 currentStage;
    Float3 unk0x824;
    i32 unk0x830;
    u8 unk0x834;
    unknown_fields(0x835, 0x3);
    ZunTimer timer0x838;
    AnmVm unk0x844;
    AnmVm *unk0xae8;
    StageFog fog;
    StageFog fogFadeFrom;
    StageFog fogFadeTo;
    u32 unk0xb10;
    ZunTimer timer0xb14;
    u8 skyFogNeedsSetup; // Leftover from earlier games. Never checked in IN
    unknown_fields(0xb21, 0x3);
    u32 unk0xb24;
    i32 unk0xb28;
    unknown_fields(0xb2c, 0x4);
    i32 unk0xb30;
    unknown_fields(0xb34, 0x4);
    AnmVm objectVms[0x20];
    AnmVm unk0x5fb8;
    void (*unk0x625c)(Background *background);
    u32 unk0x6260;
    BackgroundCamera camera0;
    BackgroundCamera camera1;
    BackgroundCamera camera2;
    BackgroundCamera camera3;
    BackgroundCamera camera4;
    u32 unk0x63e0[4];
    u32 unk0x63f0;
    ZunTimer timers0x63f4[5];
    i32 unk0x6430[5];
    Float3 unk0x6444;
    u32 unk0x6450;
    Float3 unk0x6454;
    unknown_fields(0x6460, 0x4);
    u8 unk0x6464;
    unknown_fields(0x6465, 0x7);
    u32 unk0x646c;
    f32 unk0x6470;
    u8 unk0x6474;
    unknown_fields(0x6475, 0x3);
    u8 unk0x6478;
    unknown_fields(0x6479, 0x3);
    u32 unk0x647c;
    Float3 unk0x6480[0x20];
};
C_ASSERT(sizeof(Background) == 0x6600);

DIFFABLE_EXTERN(Background, g_Background);
}; // Namespace th08
