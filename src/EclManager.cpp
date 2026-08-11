#include "th_pch.h"

#include "EclManager.hpp"
#include "EnemyManager.hpp"
#include "Global.hpp"

namespace th08
{

DIFFABLE_STATIC(EclManager, g_EclManager);

// FUNCTION: th08 0x418330
ZunResult EclManager::Load(const char *path)
{
    return ZUN_ERROR;
}

// STUB: th08 0x4184b0 (0x680e, 全项目最大函数：184 opcode 的 ECL 解释器)
ZunResult EclManager::RunEcl(Enemy *enemy)
{
    return ZUN_ERROR;
}

} // namespace th08
