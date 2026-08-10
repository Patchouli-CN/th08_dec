#pragma once

#include "AnmManager.hpp"
#include "ZunResult.hpp"

namespace th08
{

struct EffectManager
{
    static ZunResult RegisterChain();
    static void CutChain();

    ZunResult FUN_00425410();
    AnmVm *FUN_00425430(i32 a, Float3 *pos, i32 b, i32 c);
    AnmVm *FUN_00425870(i32 a, Float3 *pos, i32 b, i32 c, i32 d);
};

DIFFABLE_EXTERN(EffectManager, g_EffectManager);

} /* namespace th08 */
