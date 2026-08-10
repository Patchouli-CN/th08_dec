#pragma once

#include "AnmManager.hpp"
#include "ZunMath.hpp"
#include "ZunResult.hpp"

namespace th08
{

struct EffectManagerParticle
{
    unknown_fields(0x0, 0x2d4);
    Float3 unk2d4;                    // 0x2d4
    unknown_fields(0x2e0, 0x71);
    i8 unk351;                        // 0x351
    unknown_fields(0x352, 0xe);
};

C_ASSERT(sizeof(EffectManagerParticle) == 0x360);

struct EffectManager
{
    static ZunResult RegisterChain();
    static void CutChain();

    void ResetEffects();
    AnmVm *FUN_00425430(i32 a, Float3 *pos, i32 b, i32 c);
    AnmVm *FUN_00425870(i32 a, Float3 *pos, i32 b, i32 c, i32 d);

    unknown_fields(0x0, 0x1c);
    EffectManagerParticle particles[512]; // 0x1c
    unknown_fields(0x6c01c, 0x1f040);    // 补齐到 sizeof == 0x8b05c
};

C_ASSERT(sizeof(EffectManager) == 0x8b05c);

DIFFABLE_EXTERN(EffectManager, g_EffectManager);

} /* namespace th08 */
