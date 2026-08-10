#include "th_pch.h"

#include "EnemyManager.hpp"

#include <string.h>
#include "AsciiManager.hpp"
#include "EffectManager.hpp"
#include "Spellcard.hpp"
#include "Supervisor.hpp"

namespace th08
{

DIFFABLE_STATIC(EnemyManager, g_EnemyManager);
DIFFABLE_STATIC(ChainElem, g_EnemyManagerCalcChain);
DIFFABLE_STATIC(ChainElem, g_EnemyManagerDrawChainHighPrio);
DIFFABLE_STATIC(ChainElem, g_EnemyManagerDrawChainLowPrio);

// STUB: th08 0x429e00
void EnemyManager::Initialize()
{
}

ZunResult EnemyManager::RegisterChain()
{
    EnemyManager *enemyManager = &g_EnemyManager;
    i32 unused = 0;

    enemyManager->Initialize();
    g_EnemyManagerCalcChain.SetCallback((ChainCallback)OnUpdate);
    g_EnemyManagerCalcChain.addedCallback = (ChainLifetimeCallback)AddedCallback;
    g_EnemyManagerCalcChain.deletedCallback = (ChainLifetimeCallback)DeletedCallback;
    g_EnemyManagerCalcChain.arg = enemyManager;
    if (g_Chain.AddToCalcChain(&g_EnemyManagerCalcChain, 0xb))
    {
        return ZUN_ERROR;
    }
    g_EnemyManagerDrawChainHighPrio.SetCallback((ChainCallback)OnDrawHighPrio);
    g_EnemyManagerDrawChainHighPrio.arg = enemyManager;
    if (g_Chain.AddToDrawChain(&g_EnemyManagerDrawChainHighPrio, 8))
    {
        return ZUN_ERROR;
    }
    g_EnemyManagerDrawChainLowPrio.SetCallback((ChainCallback)OnDrawLowPrio);
    g_EnemyManagerDrawChainLowPrio.arg = enemyManager;
    if (g_Chain.AddToDrawChain(&g_EnemyManagerDrawChainLowPrio, 0xb))
    {
        return ZUN_ERROR;
    }
    return ZUN_SUCCESS;
}

// STUB: th08 0x42c660
ChainCallbackResult EnemyManager::OnUpdate()
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult EnemyManager::OnDrawHighPrio(EnemyManager *enemyManager)
{
    return OnDrawImpl(enemyManager, 0, 2);
}

// STUB: th08 0x42e140
ChainCallbackResult EnemyManager::OnDrawImpl(EnemyManager *enemyManager, i32 arg1, i32 arg2)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult EnemyManager::OnDrawLowPrio(EnemyManager *enemyManager)
{
    ChainCallbackResult result;

    if (g_GameManager.flags.unk10)
    {
        g_AnmManager->SetMixColor(0xfff01010);
    }
    result = OnDrawImpl(enemyManager, 2, 4);
    if (g_GameManager.flags.unk10)
    {
        g_AnmManager->SetMixColorDefault();
    }
    return result;
}

// STUB: th08 0x42ebf0
ZunResult EnemyManager::AddedCallback(EnemyManager *enemyManager)
{
    return ZUN_ERROR;
}

ZunResult EnemyManager::DeletedCallback(EnemyManager *enemyManager)
{
    Enemy *enemy;
    i32 i;

    enemy = &enemyManager->enemies[0];
    for (i = 0; i < 0x1e0; i++, enemy++)
    {
        enemy->FUN_0042bc90();
    }
    if (!IsDisableResourceReload())
    {
        g_AnmManager->ReleaseAnm(8);
    }
    if (Supervisor::GetUnk168() != 0)
    {
        g_AnmManager->ReleaseAnm(7);
    }
    if (!IsDisableResourceReload())
    {
        g_Spellcard.unk2648.FreeData();
    }
    Float3 markerPos(-9999.0f, -9999.0f, -9999.0f);
    g_AsciiManager.SetBossMarkerPosition(0, &markerPos);
    g_AsciiManager.SetBossMarkerPosition(1, &markerPos);
    g_AsciiManager.SetBossMarkerPosition(2, &markerPos);
    g_AsciiManager.SetBossMarkerPosition(3, &markerPos);
    return ZUN_SUCCESS;
}

void EnemyManager::CutChain()
{
    g_Chain.Cut(&g_EnemyManagerCalcChain);
    g_Chain.Cut(&g_EnemyManagerDrawChainHighPrio);
    g_Chain.Cut(&g_EnemyManagerDrawChainLowPrio);
}

void Enemy::FUN_0042bc90()
{
    i32 i;

    for (i = 0; i < 4; i++)
    {
        if (this->dataPtrs[i] != NULL)
        {
            g_ZunMemory.RemoveFromRegistry(this->dataPtrs[i]);
            this->dataPtrs[i] = NULL;
        }
    }
}

// TODO: move to a dedicated EffectManager.cpp once the unit is added to the build.
DIFFABLE_STATIC(EffectManager, g_EffectManager);
DIFFABLE_STATIC(ChainElem, g_EffectManagerCalcChain);
DIFFABLE_STATIC(ChainElem, g_EffectManagerDrawChain);

// FUNCTION: th08 0x425410
void EffectManager::ResetEffects()
{
    memset(this, 0, sizeof(EffectManager));
}

// FUNCTION: th08 0x428620
ZunResult EffectManager::RegisterChain()
{
    EffectManager *obj = &g_EffectManager;

    obj->ResetEffects();

    g_EffectManagerCalcChain.SetCallback((ChainCallback)0x427bf0);
    g_EffectManagerCalcChain.addedCallback = (ChainLifetimeCallback)0x4284b0;
    g_EffectManagerCalcChain.deletedCallback = (ChainLifetimeCallback)0x428590;
    g_EffectManagerCalcChain.arg = obj;

    if (g_Chain.AddToCalcChain(&g_EffectManagerCalcChain, 0xd) != 0)
    {
        return ZUN_ERROR;
    }

    g_EffectManagerDrawChain.SetCallback((ChainCallback)0x427f00);
    g_EffectManagerDrawChain.arg = obj;

    g_Chain.AddToDrawChain(&g_EffectManagerDrawChain, 0xc);

    return ZUN_SUCCESS;
}

// STUB: th08 0x425430
AnmVm *EffectManager::FUN_00425430(i32 a, Float3 *pos, i32 b, i32 c)
{
    return NULL;
}

// STUB: th08 0x425870
AnmVm *EffectManager::FUN_00425870(i32 a, Float3 *pos, i32 b, i32 c, i32 d)
{
    return NULL;
}

void EffectManager::CutChain()
{
    g_Chain.Cut(&g_EffectManagerCalcChain);
    g_Chain.Cut(&g_EffectManagerDrawChain);
}

} /* namespace th08 */
