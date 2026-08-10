#include "th_pch.h"

#include "EnemyManager.hpp"

#include <string.h>
#include "AsciiManager.hpp"
#include "EffectManager.hpp"
#include "ReplayManager.hpp"
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
DIFFABLE_STATIC(EffectTemplate, g_EffectTemplates[0x100]);
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

// FUNCTION: th08 0x425430
AnmVm *EffectManager::FUN_00425430(i32 a, Float3 *pos, i32 b, i32 c)
{
    EffectManagerParticle *p = &this->particles[this->unk0];
    AnmVm *result;
    i32 i;

    for (i = 0; i < 0x200; i++)
    {
        this->unk0 = this->unk0 + 1;
        if (this->unk0 >= 0x200)
        {
            this->unk0 = 0;
        }

        if (p->unk350 != 0)
        {
            if (this->unk0 == 0)
            {
                p = &this->particles[0];
            }
            else
            {
                p = p + 1;
            }
            continue;
        }

        if (p->unk358 != NULL)
        {
            g_ZunMemory.Free(p->unk358);
        }

        memset(p, 0, sizeof(EffectManagerParticle));

        p->unk350 = 1;
        p->unk351 = (i8)a;
        p->unk2a4 = *pos;
        this->unk8b054->SetAndExecuteScriptIdx((AnmVm *)p, g_EffectTemplates[a].field0);

        p->unk1f8 |= 0x2000;
        p->unk1f0 = c;
        p->unk288 = 0;
        p->unk28c = 0;
        p->unk290 = 0;
        p->unk348 = g_EffectTemplates[a].field4;

        if (g_EffectTemplates[a].field8 != NULL)
        {
            if (g_EffectTemplates[a].field8(p) != 0)
            {
                p->unk350 = 0;
            }
        }

        b--;
        if (b == 0)
        {
            break;
        }

        if (this->unk0 == 0)
        {
            p = &this->particles[0];
        }
        else
        {
            p = p + 1;
        }
    }

    g_ReplayManager->replayEventFlags |= 0x400;

    if (i >= 0x200)
    {
        result = (AnmVm *)((u8 *)this + 0x89bfc);
    }
    else
    {
        result = (AnmVm *)p;
    }

    return result;
}

// FUNCTION: th08 0x425870
AnmVm *EffectManager::FUN_00425870(i32 a, Float3 *pos, i32 b, i32 c, i32 d)
{
    EffectManagerParticle *p = &this->particles[b + 0x280];

    if (p->unk358 != NULL)
    {
        g_ZunMemory.Free(p->unk358);
    }

    memset(p, 0, sizeof(EffectManagerParticle));

    p->unk328 = b;
    p->unk350 = 1;
    p->unk351 = (i8)a;
    p->unk2a4 = *pos;

    if (g_EffectTemplates[a].field0 >= 0)
    {
        this->unk8b054->SetAndExecuteScriptIdx((AnmVm *)p, g_EffectTemplates[a].field0);
    }

    p->unk1f8 |= 0x2000;
    p->unk1f0 = d;
    p->unk288 = 0;
    p->unk28c = 0;
    p->unk290 = 0;
    p->unk348 = g_EffectTemplates[a].field4;

    if (g_EffectTemplates[a].field8 != NULL)
    {
        if (g_EffectTemplates[a].field8(p) != 0)
        {
            p->unk350 = 0;
        }
    }

    g_ReplayManager->replayEventFlags |= 0x400;

    return (AnmVm *)p;
}

void EffectManager::CutChain()
{
    g_Chain.Cut(&g_EffectManagerCalcChain);
    g_Chain.Cut(&g_EffectManagerDrawChain);
}

} /* namespace th08 */
