#include "th_pch.h"

#include "BulletManager.hpp"
#include "ItemManager.hpp"
#include "Player.hpp"
#include "Supervisor.hpp"

#include <math.h>

namespace th08
{

// FUNCTION: th08 0x433880
void __fastcall ComputeSinCos(f32 *sinOut, f32 *cosOut, f32 angle)
{
    __asm
    {
        fld [angle]
        fsincos
        mov eax, [cosOut]
        fstp float ptr [eax]
        mov eax, [sinOut]
        fstp float ptr [eax]
    }
}

DIFFABLE_STATIC(BulletManager, g_BulletManager);
DIFFABLE_STATIC(ChainElem, g_BulletManagerCalcChain);
DIFFABLE_STATIC(ChainElem, g_BulletManagerDrawChain);
DIFFABLE_STATIC(AnmLoaded *, g_BulletAnm);

DIFFABLE_STATIC_ARRAY(u8, 0x600 * 0x10b8, g_BulletPool);

// FUNCTION: th08 0x42f360
#pragma var_order(i, pool)
void BulletManager::Initialize()
{
    memset(this, 0, 0x6ba578);

    /* enemyBulletPool 是内嵌的子弹池数组起始。 */
    this->bulletPoolPtr = (i32)this->enemyBulletPool;
    this->bulletCount = 6;
    this->itemType = 6;

    /* 敌弹池：0x600 个 0x10b8 字节的槽位，各槽内若干 u16 索引字段初始化为 -1。 */
    EnemyBullet *pool = (EnemyBullet *)g_BulletPool;
    i32 i;
    for (i = 0; i < 0x600; i++, pool++)
    {
        /* 原版按 0x21a → 0xcaa → 0x4be → 0x762 → 0xa06 顺序写入（字节级保持）。 */
        pool->chainIndex0 = 0xffff;
        pool->chainIndex4 = 0xffff;
        pool->chainIndex1 = 0xffff;
        pool->chainIndex2 = 0xffff;
        pool->chainIndex3 = 0xffff;
    }
}

// FUNCTION: th08 0x430830 (99% FIXME: 浮点 <= 的 jp+jmp vs jne)
#pragma var_order(pos, r1, i, cosX, pool, b, sinX, speed)
void BulletManager::RemoveAllBullets(i32 param)
{
    EnemyBullet *pool = (EnemyBullet *)g_BulletPool;
    PlayerShotData *b;
    i32 i;
    i32 r1;

    for (i = 0; i < 0x600; i++, pool++)
    {
        if (pool->state == 0 || pool->state == 5)
        {
            continue;
        }

        r1 = g_Player.CheckShotCollision(&pool->pos, &pool->collisionSize);

        if (g_Player.CheckShotCollision(&pool->pos, &pool->collisionSize) == 2)
        {
            g_ItemManager.SpawnItem(&pool->pos, (ItemType) * (i32 *)0x18b8988, 1);
            memset(pool, 0, sizeof(EnemyBullet));
            continue;
        }

        if (param != 4)
        {
            g_ItemManager.SpawnItem(&pool->pos, (ItemType)this->itemType, (ItemType)param);
            memset(pool, 0, sizeof(EnemyBullet));
        }
        else
        {
            pool->state = 5;
        }
    }

    /* 0x660938：自机弹/激光对象数组，0x100 项、每项 0x59c 字节。 */
    b = (PlayerShotData *)((u8 *)this + 0x660938);

    Float3 pos;
    f32 sinX;
    f32 cosX;
    f32 speed;

    for (i = 0; i < 0x100; i++, b++)
    {
        if (b->isActive == 0)
        {
            continue;
        }

        if (b->flags & 0x4 && param != 4)
        {
            continue;
        }

        if (b->runState < 2)
        {
            b->runState = 2;
            b->timer.SetCurrent(0);
            /* phaseTimer 从 savedPhaseTimer 恢复（32 位拷贝，字节级保持）。 */
            *(u32 *)&b->phaseTimer = *(u32 *)&b->savedPhaseTimer;

            if (param != 4)
            {
                /* 沿当前角度每步 32.0f 掉落一组道具。 */
                speed = b->speed;

                for (;;)
                {
                    ComputeSinCos(&sinX, &cosX, b->angle);

                    if (b->targetSpeed <= speed)
                    {
                        break;
                    }

                    pos.x = cosX * speed + b->pos.x;
                    pos.y = sinX * speed + b->pos.y;
                    pos.z = 0;
                    g_ItemManager.SpawnItem(&pos, (ItemType)this->itemType, (ItemType)param);
                    speed += *(f32 *)MEM_FLOAT_32_0;
                }
            }
        }

        b->param4 = 0;
    }

    /* 0x6ba53c 清场计数/状态 (clearCount)：置 10。 */
    this->clearCount = 0xa;
}

void BulletManager::bulletmanager_fun_00415c60()
{
    this->RemoveAllBullets(1);
}

void BulletManager::ClearBulletsInRadius(Float3 *pos, f32 a1)
{
}

void BulletManager::SetupLaserMove(Float3 *pos)
{
}

EnemySubData *BulletManager::AllocShotSlot(EnemyShotData *src)
{
    return NULL;
}

ZunResult BulletManager::RegisterChain(char *path)
{
    BulletManager *bulletManager = &g_BulletManager;

    bulletManager->Initialize();
    bulletManager->etamaAnmPath = path;
    g_BulletManagerCalcChain.SetCallback((ChainCallback)OnUpdate);
    g_BulletManagerCalcChain.addedCallback = (ChainLifetimeCallback)AddedCallback;
    g_BulletManagerCalcChain.deletedCallback = (ChainLifetimeCallback)DeletedCallback;
    g_BulletManagerCalcChain.arg = bulletManager;
    if (g_Chain.AddToCalcChain(&g_BulletManagerCalcChain, 0xe))
    {
        return ZUN_ERROR;
    }
    g_BulletManagerDrawChain.SetCallback((ChainCallback)OnDraw);
    g_BulletManagerDrawChain.arg = bulletManager;
    g_Chain.AddToDrawChain(&g_BulletManagerDrawChain, 0xd);
    return ZUN_SUCCESS;
}

// STUB: th08 0x431240
ChainCallbackResult BulletManager::OnUpdate(BulletManager *bulletManager)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// STUB: th08 0x432b50
ChainCallbackResult BulletManager::OnDraw(BulletManager *bulletManager)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// STUB: th08 0x433070
ZunResult BulletManager::AddedCallback(BulletManager *bulletManager)
{
    return ZUN_SUCCESS;
}

ZunResult BulletManager::DeletedCallback(BulletManager *bulletManager)
{
    if (Supervisor::GetUnk168() != 0)
    {
        g_AnmManager->ReleaseAnm(6);
    }
    return ZUN_SUCCESS;
}

void BulletManager::CutChain()
{
    g_Chain.Cut(&g_BulletManagerCalcChain);
    g_Chain.Cut(&g_BulletManagerDrawChain);
}

} /* namespace th08 */
