#pragma once

#include "AnmManager.hpp"
#include "ZunMath.hpp"
#include "ZunResult.hpp"

namespace th08
{

typedef void (AnmLoaded::*AnmLoadedSetAndExecuteScriptIdxFn)(AnmVm *, int);

struct EffectTemplate
{
    i32 field0;                            // 0x0
    i32 field4;                            // 0x4
    i32 (__fastcall *field8)(void *param); // 0x8
};

C_ASSERT(sizeof(EffectTemplate) == 0xc);

struct EffectManagerParticle
{
    unknown_fields(0x0, 0x1f0);
    u32 unk1f0;                       // 0x1f0
    unknown_fields(0x1f4, 0x4);
    u32 flags;                        // 0x1f8  (0x2000 set by spawn, 0x20000 = screen-flip marker)
    unknown_fields(0x1fc, 0x8c);
    u32 unk288;                       // 0x288
    u32 unk28c;                       // 0x28c
    u32 unk290;                       // 0x290
    unknown_fields(0x294, 0x10);
    Float3 unk2a4;                    // 0x2a4
    unknown_fields(0x2b0, 0x24);
    Float3 unk2d4;                    // 0x2d4
    unknown_fields(0x2e0, 0xc);
    Float3 unk2ec;                    // 0x2ec  (op128 写入的特效位置)
    unknown_fields(0x2f8, 0x30);
    u32 unk328;                       // 0x328
    unknown_fields(0x32c, 0x1c);
    u32 unk348;                       // 0x348
    unknown_fields(0x34c, 0x4);
    i8 unk350;                        // 0x350
    i8 unk351;                        // 0x351
    unknown_fields(0x352, 0x6);
    void *unk358;                     // 0x358
    unknown_fields(0x35c, 0x4);
};

C_ASSERT(sizeof(EffectManagerParticle) == 0x360);

struct EffectManager
{
    static ZunResult RegisterChain();
    static void CutChain();

    void ResetEffects();
    void FUN_004281e0();
    AnmVm *FUN_00425430(i32 a, Float3 *pos, i32 b, i32 c);
    AnmVm *FUN_00425870(i32 a, Float3 *pos, i32 b, i32 c, i32 d);
    void *AllocEffectSlot(i32 type, Float3 *pos, i32 b, i32 c); // 0x425b70 (claim an effect slot)
    void FUN_00425650(i32 a0, Float3 *pos, Float3 *localPos, i32 a1, i32 a2, i32 a3); // 0x425650 (op140)

    i32 unk0;                            // 0x0
    unknown_fields(0x4, 0x18);
    EffectManagerParticle particles[512]; // 0x1c
    unknown_fields(0x6c01c, 0x1f038);    // 0x6c01c..0x8b054
    AnmLoaded *unk8b054;                 // 0x8b054
    unknown_fields(0x8b058, 0x4);        // 到 0x8b05c
};

C_ASSERT(sizeof(EffectManager) == 0x8b05c);

DIFFABLE_EXTERN(EffectManager, g_EffectManager);

} /* namespace th08 */
