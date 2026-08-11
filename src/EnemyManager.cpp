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

// FUNCTION: th08 0x429e00
void EnemyManager::Initialize()
{
    u8 *p = (u8 *)this + 0x53d0;
    i32 i;

    memset(this, 0, 0x9cef10);

    for (i = 0; i < 4u; i++)
    {
        *(i32 *)((u8 *)this + 0x9dcefc + i * 4) = -1;
    }

    p = (u8 *)this;
    memset(p, 0, 0x53d0);

    for (i = 0; i < 2; i++)
    {
        *(u16 *)(p + i * 0x2a4 + 0x4ca) = 0xffff;
    }

    for (i = 0; i < 0x60; i++)
    {
        *(f32 *)(p + 0x3394 + i * 0x1c) = -1000.0f;
    }

    *(u32 *)(p + 0x3324) |= 0x1;
    ((ZunTimer *)(p + 0x2e14))->SetCurrent(0);
    *(u32 *)(p + 0x3324) &= 0xfeffffff;
    *(Float3 *)(p + 0x2d70) = Float3(24.0f, 24.0f, 24.0f);
    *(Float3 *)(p + 0x2d4c) = Float3(0.0f, 0.0f, 0.0f);
    *(u32 *)(p + 0x2d98) = 0;
    *(u32 *)(p + 0x2d94) = 0;
    *(u32 *)(p + 0x2dac) = 0;
    *(u32 *)(p + 0x2da8) = 0;
    *(u32 *)(p + 0x3324) &= 0xffffcfff;
    *(u32 *)(p + 0x3324) &= 0xfffdffff;
    *(u32 *)(p + 0x3324) &= 0xfffbffff;
    *(u32 *)(p + 0x3324) &= 0xfffffffd;
    *(u16 *)(p + 0x2cea) = 0;
    *(u32 *)(p + 0x2dfc) = 1;
    *(u32 *)(p + 0x2e08) = 0x64;
    *(u8 *)(p + 0x3310) = 0;
    *(u8 *)(p + 0x3311) = 0;
    *(u8 *)(p + 0x3312) = 0;
    *(u32 *)(p + 0x3060) = 0;
    ((ZunTimer *)(p + 0x3064))->SetCurrent(0);
    *(Float3 *)(p + 0x2db8) = Float3(0.0f, 0.0f, 0.0f);
    *(u16 *)(p + 0x3338) = 0xffff;
    *(u16 *)(p + 0x333a) = 0xffff;
    *(u16 *)(p + 0x3332) = 0xffff;
    *(u32 *)(p + 0x3324) |= 0x4;
    *(u32 *)(p + 0x3324) |= 0x8;
    *(u32 *)(p + 0x3324) &= 0xffffffef;
    *(u32 *)(p + 0x3324) |= 0x40;
    *(u32 *)(p + 0x3324) &= 0xffffff7f;
    *(u32 *)(p + 0x3324) &= 0xff8fffff;
    *(u16 *)(p + 0x2cee) = 0xffff;
    *(u32 *)(p + 0x3324) &= 0xfff7ffff;
    *(i32 *)(p + 0x53c0) = 0;
    *(u16 *)(p + 0x2d30) = 0xffff;

    for (i = 0; i < 4; i++)
    {
        *(i32 *)(p + 0x3358 + i * 4) = -1;
    }

    *(i32 *)(p + 0x3378) = -1;
    *(i32 *)(p + 0x3300) = 0;
    *(u8 *)(p + 0x3314) = 0;
    *(u32 *)(p + 0x3324) &= 0xfdffffff;
    *(f32 *)(p + 0x2dec) = -0.15f;
    *(f32 *)(p + 0x2df0) = 0.15f;
    *(i32 *)(p + 0x3024) = 7;
    *(i32 *)(p + 0x3028) = 0x19;
    *(f32 *)(p + 0x3350) = 1024.0f;
    *(i32 *)(p + 0x2e10) = *(i32 *)0x18b8a24;
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

// FUNCTION: th08 0x42ebf0 (79.65% FIXME: Anm 分支共用布局/寄存器)
ZunResult EnemyManager::AddedCallback(EnemyManager *enemyManager)
{
    u8 *p = (u8 *)enemyManager + 0x53d0;

    if (g_Supervisor.GetUnk164())
    {
        *(void **)((u8 *)enemyManager + 0x9dceec) = g_AnmManager->PreloadAnm(7, (const char *)0x4b4ac8);
        if (*(void **)((u8 *)enemyManager + 0x9dceec) == NULL)
        {
            return (ZunResult)-1;
        }
    }
    else
    {
        *(void **)((u8 *)enemyManager + 0x9dceec) = g_AnmManager->GetAnm(7);
    }

    if (!IsDisableResourceReload())
    {
        if ((*(u32 *)0x164d0b4 >> 0xe) & 1)
        {
            if (*(i16 *)0x164d0b8 < 0xcd)
            {
                *(void **)((u8 *)enemyManager + 0x9dcef0) =
                    g_AnmManager->PreloadAnm(8, *(const char **)(0x4c7364 + *(u32 *)0x164d2cc * 4));

                if (*(void **)((u8 *)enemyManager + 0x9dcef0) == NULL)
                {
                    return (ZunResult)-1;
                }
            }
            else
            {
                *(void **)((u8 *)enemyManager + 0x9dcef0) =
                    g_AnmManager->PreloadAnm(8, *(const char **)(0x4c7054 + *(i16 *)0x164d0b8 * 4));

                if (*(void **)((u8 *)enemyManager + 0x9dcef0) == NULL)
                {
                    return (ZunResult)-1;
                }
            }
        }
        else
        {
            *(void **)((u8 *)enemyManager + 0x9dcef0) =
                g_AnmManager->PreloadAnm(8, *(const char **)(0x4c7364 + *(u32 *)0x164d2cc * 4));

            if (*(void **)((u8 *)enemyManager + 0x9dcef0) == NULL)
            {
                return (ZunResult)-1;
            }
        }
    }
    else
    {
        *(void **)((u8 *)enemyManager + 0x9dcef0) = g_AnmManager->GetAnm(8);
    }

    if (!IsDisableResourceReload())
    {
        if ((*(u32 *)0x164d0b4 >> 0xe) & 1)
        {
            if (*(i16 *)0x164d0b8 >= 0xcd)
            {
                if (((u32(__cdecl *)(const char *))0x418330)(*(const char **)(0x4c70e4 + *(i16 *)0x164d0b8 * 4)) != 0)
                {
                    /* return Load 的返回值（eax 残留） */
                }
            }
            else
            {
                if (((u32(__cdecl *)(const char *))0x418330)(*(const char **)(0x4c73f0 + *(u32 *)0x164d2cc * 4)) != 0)
                {
                }
            }
        }
        else
        {
            if (((u32(__cdecl *)(const char *))0x418330)(*(const char **)(0x4c73cc + *(u32 *)0x164d2cc * 4)) != 0)
            {
            }
        }
    }
    else
    {
        /* 0x42edb7: 全局 0x4eccb8/0x4eccbc 保存+清零+恢复（死代码） */
        i32 old0 = *(i32 *)0x4eccb8;
        i32 old1 = *(i32 *)0x4eccbc;

        memset((void *)0x4eccb8, 0, 0x188);
        *(i32 *)0x4eccb8 = old0;
        *(i32 *)0x4eccbc = old1;
    }

    *(u16 *)((u8 *)enemyManager + 0x9dcdc0) = g_Rng.GetRandomU16InRange(3);
    *(u16 *)((u8 *)enemyManager + 0x9dcdc2) = g_Rng.GetRandomU16InRange(8);

    Float3 markerPos(-1000.0f, -1000.0f, -1000.0f);
    g_AsciiManager.SetBossMarkerPosition(0, &markerPos);
    g_AsciiManager.SetBossMarkerPosition(1, &markerPos);
    g_AsciiManager.SetBossMarkerPosition(2, &markerPos);
    g_AsciiManager.SetBossMarkerPosition(3, &markerPos);

    return ZUN_SUCCESS;
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

// STUB: th08 0x4281e0
void EffectManager::FUN_004281e0()
{
}

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
