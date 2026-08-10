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
    unknown_fields(0x818, 0x4);
    u32 unk81c;
    i32 currentStage;
    Float3 unk0x824;
    unknown_fields(0x830, 0x4);
    u8 unk0x834;
    unknown_fields(0x835, 0x3);
    ZunTimer timer0x838;
    AnmVm unk0x844;
    unknown_fields(0xae8, 0x4);
    StageFog fog;
    unknown_fields(0xaf8, 0x18);
    u32 unk0xb10;
    ZunTimer timer0xb14;
    u8 skyFogNeedsSetup; // Leftover from earlier games. Never checked in IN
    unknown_fields(0xb21, 0x3);
    u32 unk0xb24;
    unknown_fields(0xb28, 0x10);
    AnmVm objectVms[0x20];
    AnmVm unk0x5fb8;
    unknown_fields(0x625c, 0x4);
    u32 unk0x6260;
    BackgroundCamera camera0;
    BackgroundCamera camera1;
    BackgroundCamera camera2;
    BackgroundCamera camera3;
    BackgroundCamera camera4;
    u32 unk0x63e0[4];
    unknown_fields(0x63f0, 0x4);
    ZunTimer timers0x63f4[5];
    unknown_fields(0x6430, 0x14);
    Float3 unk0x6444;
    u32 unk0x6450;
    Float3 unk0x6454;
    unknown_fields(0x6460, 0x10);
    f32 unk0x6470;
    u8 unk0x6474;
    unknown_fields(0x6475, 0xb);
    Float3 unk0x6480[0x20];
};
C_ASSERT(sizeof(Background) == 0x6600);

DIFFABLE_EXTERN(Background, g_Background);
}; // Namespace th08
