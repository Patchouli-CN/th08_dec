#include "th_pch.h"

#include "BulletManager.hpp"
#include "EffectManager.hpp"
#include "ItemManager.hpp"
#include "Player.hpp"
#include "Supervisor.hpp"

#include <math.h>

/* 被调用子函数的 stub 前向声明（全局作用域；PDB 名与 CSV 原名一致，见文件尾定义）。 */
void FUN_0042ffc0(th08::EnemyBullet *slot);
void FUN_004337f0(th08::ItemManager *self);

namespace th08
{

/* 0x40c7b0 (EclManager.cpp 定义): atan2(dx, dy) __stdcall 包装 */
f32 __stdcall EclAngleFromDxDy(f32 dx, f32 dy);

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
DIFFABLE_EXTERN(f32, g_ShotSpeed); // 0x17ce8e0 (Player.cpp 定义)

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

    /* 敌弹池：0x600 个 0x10b8 字节的槽位，各槽内 5 个 AnmVm 的 scriptIndex 初始化为 -1。 */
    EnemyBullet *pool = (EnemyBullet *)g_BulletPool;
    i32 i;
    for (i = 0; i < 0x600; i++, pool++)
    {
        /* 原版按 0x21a → 0xcaa → 0x4be → 0x762 → 0xa06 顺序写入（字节级保持）。 */
        pool->vms[0].scriptIndex = 0xffff;
        pool->vms[4].scriptIndex = 0xffff;
        pool->vms[1].scriptIndex = 0xffff;
        pool->vms[2].scriptIndex = 0xffff;
        pool->vms[3].scriptIndex = 0xffff;
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

/* OnUpdate/OnDraw 调用的跨类 thiscall 帮手均为对应类成员 stub（声明见各 hpp，定义见文件尾）。 */

/* 21 种敌弹 x 5 个 AnmVm 的脚本索引表 (0x4b4ad8)。 */
static const i32 sBulletScriptTable[21][5] = {
    {0, 18, 19, 20, 15},   {1, 21, 22, 23, 16},   {2, 21, 22, 23, 16},
    {3, 21, 22, 23, 16},   {4, 21, 22, 23, 16},   {5, 21, 22, 23, 16},
    {6, 21, 22, 23, 16},   {7, 24, 24, 24, 17},   {8, 24, 24, 24, 17},
    {9, 24, 24, 24, 17},   {25, 27, 27, 27, 26},  {106, 21, 22, 23, 16},
    {107, 21, 22, 23, 16}, {108, 21, 22, 23, 16}, {109, 24, 24, 24, 17},
    {110, 24, 24, 24, 17}, {111, 21, 22, 23, 16}, {112, 21, 22, 23, 16},
    {113, 24, 24, 24, 17}, {114, 24, 24, 24, 17}, {115, 24, 24, 24, 17},
};

/* 碰撞尺寸 switch 的字节表 (0x433706, 下标 = 脚本号 - 2)。 */
static const u8 sBulletHitSizeTable1[0x6f] = {
    0x00, 0x05, 0x01, 0x02, 0x01, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x03, 0x04, 0x04, 0x05, 0x05, 0x00, 0x00,
};

/* 碰撞尺寸 switch 的字节表 (0x433781, 下标 = 脚本号 - 8)。 */
static const u8 sBulletHitSizeTable2[0x6c] = {
    0x00, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x02, 0x02, 0x00, 0x00, 0x00,
};

// FUNCTION: th08 0x431240
#pragma var_order(checkResult, i, varF4, idx, varE4, pool, alpha, b, laserPos)
ChainCallbackResult BulletManager::OnUpdate(BulletManager *bulletManager)
{
    i32 checkResult;         /* -0x4  碰撞返回值 (FUN_0044a470/0044a230, cmp 1/2) */
    i32 i;                   /* -0x8  外层 0x600 与激光 0x100 循环计数 */
    f32 varF4;               /* -0xc  浮点临时 (savedPhaseTimer/scale 链, 431e34 写 1.2f) */
    i32 idx = 0;             /* -0x10 槽索引 (循环尾 431b3f 递减, <0 回绕 0x5ff) */
    EnemyBullet *pool;       /* -0x20 当前敌弹槽指针 */
    i32 alpha;               /* -0x24 渐隐 alpha (431da6/431fda 与 0xff 比较钳制) */
    PlayerShotData *b;       /* -0x28 自机弹/激光槽指针 (this+0x660938) */

    /* varE4(-0x1c)/laserPos(-0x34) 是 Float3, 在激光段开头声明 (原版 431b7d/431b85
     * 各有一处 Float3::Float3() 构造调用); var_order 固定其槽位。 */

    pool = (EnemyBullet *)((u8 *)bulletManager + 0x1a880);

    if (((g_PlayerFlags >> 0xa) & 1) != 0)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    g_ItemManager.OnUpdate();
    bulletManager->unk6ba538 = 0;
    bulletManager->FUN_004321b0();

    for (i = 0; i < 0x600; i++)
    {
        if (pool->state != 0)
        {
            bulletManager->unk6ba538++;

            /* 原版 4312e6-4312f2: movzx→store→reload→sub→store 的分发序列;
             * 命名 stateTmp 使其落在 -0x38 (原版物化副本槽), 保持帧 0x78。 */
            i32 stateTmp = pool->state;
            stateTmp--;
            switch (stateTmp)
            {
            state1_update:
                /* 出生减速结束 → 转正式飞行 (原版 431306-43131d): state=1 + timerD80 复位。
                 * 仅由 case 1/2/3 的 goto 到达; 原版跳表 [0]=0x431322, 即 case 0 从
                 * FUN_0042ffc0 直入主更新 (跳过此初始化), 中间无多余 jmp。 */
                pool->state = 1;
                pool->timerD80.SetCurrent(0);
            case 0:
                /* state 1 主更新入口 (0x431322)。 */
                FUN_0042ffc0(pool);

                if (pool->flagsDAC != 0)
                {
                    if (pool->flagsDAC & 1)
                        ((Bullet *)pool)->FUN_00432210();
                    if (pool->flagsDAC & 0x10)
                        ((Bullet *)pool)->FUN_004322b0();
                    if (pool->flagsDAC & 0x20)
                        ((Bullet *)pool)->FUN_00432390();
                    if (pool->flagsDAC & 0x40)
                        ((Bullet *)pool)->FUN_00432460();
                    if (pool->flagsDAC & 0x100)
                        ((Bullet *)pool)->FUN_004325a0();
                    if (pool->flagsDAC & 0x80)
                        ((Bullet *)pool)->FUN_004326e0();
                    if (pool->flagsDAC & 0xc00)
                        ((Bullet *)pool)->FUN_00432830();
                    if (pool->flagsDAC & 0x400000)
                        ((Bullet *)pool)->FUN_004329f0();
                    if (pool->flagsDAC & 0x800000)
                        ((Bullet *)pool)->FUN_00432aa0();
                    if (pool->flagsDAC & 0x20000)
                    {
                        if (pool->timer105C <= 0)
                        {
                            pool->flagsDAC ^= 0x20000;
                        }
                        else
                        {
                            pool->timer105C--;
                        }
                    }
                }

                if (pool->unkDA8 != 0)
                {
                    pool->unkDA8--;
                }

                if (*(i8 *)0x160f534 == 0)
                {
                    pool->pos += pool->vel;
                }

                if (pool->unkDA8 == 0)
                {
                    /* 原版 4314e6/4314f8 对 pool->pos 显式调用 Float3::Float3() 再取 x/y
                     * (ZUN 源码怪癖, 项目 ItemManager/Player 同款写法); 无 sprite 局部槽。 */
                    if (!g_GameManager.IsWithinPlayfield(pool->pos.Float3::Float3().x, pool->pos.Float3::Float3().y,
                                                         *(f32 *)((u8 *)pool->vms[0].loadedSprite + 0x34),
                                                         *(f32 *)((u8 *)pool->vms[0].loadedSprite + 0x30)))
                    {
                        if (pool->flagsDAC & 0xdc0)
                        {
                            pool->unkDBA++;
                            if (pool->unkDBA >= 0x80)
                            {
                                ((Bullet *)pool)->FUN_00432170();
                                goto epilogue;
                            }
                        }
                        else
                        {
                            if (pool->unkDBA == 0)
                            {
                                ((Bullet *)pool)->FUN_00432170();
                                goto epilogue;
                            }
                            pool->unkDBA--;
                        }
                    }
                    else
                    {
                        pool->unkDBA = 0;
                    }
                }

                if (pool->unk10B4 != 0)
                {
                    goto collision_done;
                }

                if (pool->unkDBD == 0 && pool->timerD8C.AsFrames() >= 0x10)
                {
                    checkResult = g_Player.FUN_0044a470(&pool->pos, &pool->collisionSize);
                    if (checkResult == 1)
                    {
                        pool->unkDBD = 1;
                    }
                    else if (checkResult == 2)
                    {
                        if ((pool->flagsDB0 & 0x1000) == 0)
                        {
                            pool->state = 5;
                            if (*(i32 *)0x18b8988 == 9)
                            {
                                g_ItemManager.SpawnItem(&pool->pos, (ItemType)7, 1);
                                g_ItemManager.SpawnItem(&pool->pos, (ItemType)7, 1);
                            }
                            else if (*(i32 *)0x18b8988 >= 0)
                            {
                                g_ItemManager.SpawnItem(&pool->pos, (ItemType) * (i32 *)0x18b8988, 1);
                            }
                        }
                        goto collision_done;
                    }
                }

                checkResult = g_Player.FUN_0044a230(&pool->pos, &pool->collisionSize);
                if (checkResult != 0)
                {
                    if (checkResult != 2 || (pool->flagsDB0 & 0x1000) == 0)
                    {
                        pool->state = 5;
                        if (checkResult == 2)
                        {
                            if (*(i32 *)0x18b8988 == 9)
                            {
                                g_ItemManager.SpawnItem(&pool->pos, (ItemType)7, 1);
                                g_ItemManager.SpawnItem(&pool->pos, (ItemType)7, 1);
                            }
                            else if (*(i32 *)0x18b8988 >= 0)
                            {
                                g_ItemManager.SpawnItem(&pool->pos, (ItemType) * (i32 *)0x18b8988, 1);
                            }
                        }
                    }
                }

            collision_done:
                if (pool->vms[0].currentInstruction != NULL)
                {
                    g_AnmManager->ExecuteScript(&pool->vms[0]);
                }
                goto dispatch_common;

            case 1:
                /* state 2: 出生减速 1/2 */
                pool->timerD8C--;
                pool->pos += pool->vel / 2.0f;
                if ((pool->flagsDB0 & 0x1000) == 0)
                {
                    if (g_Player.CheckShotCollision(&pool->pos, &pool->collisionSize) == 2)
                    {
                        pool->unkDBE = 1;
                    }
                }
                if (g_AnmManager->ExecuteScript(&pool->vms[1]) == 0)
                {
                    goto dispatch_common;
                }
                if (pool->unkDBE != 0)
                {
                    pool->state = 5;
                    if (*(i32 *)0x18b8988 == 9)
                    {
                        g_ItemManager.SpawnItem(&pool->pos, (ItemType)7, 1);
                        g_ItemManager.SpawnItem(&pool->pos, (ItemType)7, 1);
                    }
                    else if (*(i32 *)0x18b8988 >= 0)
                    {
                        g_ItemManager.SpawnItem(&pool->pos, (ItemType) * (i32 *)0x18b8988, 1);
                    }
                }
                goto state1_update;

            case 2:
                /* state 3: 出生减速 1/2.5 */
                pool->timerD8C--;
                pool->pos += pool->vel / 2.5f;
                if ((pool->flagsDB0 & 0x1000) == 0)
                {
                    if (g_Player.CheckShotCollision(&pool->pos, &pool->collisionSize) == 2)
                    {
                        pool->unkDBE = 1;
                    }
                }
                if (g_AnmManager->ExecuteScript(&pool->vms[2]) == 0)
                {
                    goto dispatch_common;
                }
                if (pool->unkDBE != 0)
                {
                    pool->state = 5;
                    if (*(i32 *)0x18b8988 == 9)
                    {
                        g_ItemManager.SpawnItem(&pool->pos, (ItemType)7, 1);
                        g_ItemManager.SpawnItem(&pool->pos, (ItemType)7, 1);
                    }
                    else if (*(i32 *)0x18b8988 >= 0)
                    {
                        g_ItemManager.SpawnItem(&pool->pos, (ItemType) * (i32 *)0x18b8988, 1);
                    }
                }
                goto state1_update;

            case 3:
                /* state 4: 出生减速 1/3 */
                pool->timerD8C--;
                pool->pos += pool->vel / 3.0f;
                if ((pool->flagsDB0 & 0x1000) == 0)
                {
                    if (g_Player.CheckShotCollision(&pool->pos, &pool->collisionSize) == 2)
                    {
                        pool->unkDBE = 1;
                    }
                }
                if (g_AnmManager->ExecuteScript(&pool->vms[3]) == 0)
                {
                    goto dispatch_common;
                }
                if (pool->unkDBE != 0)
                {
                    pool->state = 5;
                    if (*(i32 *)0x18b8988 == 9)
                    {
                        g_ItemManager.SpawnItem(&pool->pos, (ItemType)7, 1);
                        g_ItemManager.SpawnItem(&pool->pos, (ItemType)7, 1);
                    }
                    else if (*(i32 *)0x18b8988 >= 0)
                    {
                        g_ItemManager.SpawnItem(&pool->pos, (ItemType) * (i32 *)0x18b8988, 1);
                    }
                }
                goto state1_update;

            case 4:
                /* state 5: 死亡清场 */
                pool->pos += pool->vel / 2.0f;
                if (g_AnmManager->ExecuteScript(&pool->vms[4]) != 0)
                {
                    ((Bullet *)pool)->FUN_00432170();
                    goto epilogue;
                }
                /* fall through */
            default:
                /* 空 default: 原版 ja 直落 dispatch_common (431aeb), 无中间 jmp */
                ;
            }

        dispatch_common:
            pool->timerD80.Tick();
            pool->timerD8C.Tick();
            pool->next = bulletManager->chainHeads[pool->unkD42];
            bulletManager->chainHeads[pool->unkD42] = pool;
        }

    epilogue:
        idx--;
        if (idx < 0)
        {
            idx = 0x5ff;
            pool = (EnemyBullet *)((u8 *)pool + 0x645000);
        }
        pool = (EnemyBullet *)((u8 *)pool - 0x10b8);
    }

    /* 自机弹/激光更新 (0x100 x 0x59c)。laserPos/varE4 在此声明: 原版 431b7d/431b85
     * 的 Float3 构造调用在激光段开头, var_order 把它们固定在 -0x34/-0x1c。 */
    b = (PlayerShotData *)((u8 *)bulletManager + 0x660938);
    Float3 laserPos;
    Float3 varE4;

    for (i = 0; i < 0x100; i++, b++)
    {
        if (b->isActive == 0)
        {
            continue;
        }

        /* 原版 431bc9 直接 fld [0x17ce8e0] 读全局 g_ShotSpeed, 不存局部槽。 */
        b->targetSpeed += g_ShotSpeed * b->speedStep;
        if (b->targetSpeed - b->speed > b->speedLimit)
        {
            b->speed = b->targetSpeed - b->speedLimit;
        }
        if (b->speed < 0.0f)
        {
            b->speed = 0.0f;
        }
        /* 431c56 的死值写入是 varE4.y (varE4 是 12 字节 Float3, y 槽在 -0x18)。 */
        varE4.y = b->phaseTimer / 2.0f;
        if (b->speed > 0.0f)
        {
            varE4.x = (b->targetSpeed - b->speed) * 0.7f;
        }
        else
        {
            varE4.x = b->targetSpeed - b->speed;
        }
        /* 431ca1-431ccb: 求值顺序为 (t-s)/2 + speed + pos.x。 */
        laserPos.x = (b->targetSpeed - b->speed) / 2.0f + b->speed + b->pos.x;
        laserPos.y = b->pos.y;
        b->vms[0].prefix.scale.x = b->phaseTimer / *(f32 *)((u8 *)b->vms[0].loadedSprite + 0x34);
        b->vms[0].prefix.scale.y = (b->targetSpeed - b->speed) / *(f32 *)((u8 *)b->vms[0].loadedSprite + 0x30);
        /* 431d1f-431d40: vms[0].SetZRotation(AddNormalizeAngle(π/2 + angle, 0)) */
        b->vms[0].SetZRotation(AddNormalizeAngle(1.5707964f + b->angle, 0.0f));

        switch (b->runState)
        {
        case 0:
            if (b->flags & 1)
            {
                alpha = (i32)(b->timer.AsFramesFloat() * 255.0f / b->delay);
                if (alpha > 0xff)
                {
                    alpha = 0xff;
                }
                b->vms[0].prefix.color1.d3dColor = alpha << 24;
            }
            else
            {
                /* 原版 431dcd-431de8: 直接 cmp [b+0x570],0x1e 分两支写 delayTmp。
                 * 命名 delayTmp (块局部) 是项目环境下唯一能保持帧 0x78 的形式
                 * (内联三元在项目构建中不产生独立槽, 帧缩为 0x74)。 */
                i32 delayTmp;
                if (b->delay > 0x1e)
                {
                    delayTmp = 0x1e;
                }
                else
                {
                    delayTmp = b->delay;
                }
                if (b->delay - delayTmp < b->timer.AsFrames())
                {
                    varF4 = b->timer.AsFramesFloat() * b->phaseTimer / b->delay;
                }
                else
                {
                    varF4 = 1.2f;
                }
                b->savedPhaseTimer = varF4;
                b->vms[0].prefix.scale.x = varF4 / 16.0f;
                varE4.x = varF4 / 2.0f;
            }
            if (b->timer >= b->trigger)
            {
                g_Player.CalcLaserHitbox(&laserPos, &varE4.x, &b->pos, b->angle, 0);
            }
            if (b->timer < b->delay)
            {
                goto shot_slot_end;
            }
            b->timer.SetCurrent(0);
            b->runState++;
            b->savedPhaseTimer = b->phaseTimer;
            /* fall through */
        case 1:
            g_Player.CalcLaserHitbox(&laserPos, &varE4.x, &b->pos, b->angle, (b->timer.AsFrames() % 0x14) == 0);
            if (b->timer < b->phase)
            {
                goto shot_slot_end;
            }
            b->timer.SetCurrent(0);
            b->runState++;
            if (b->fadeTime == 0)
            {
                b->isActive = 0;
                continue;
            }
            /* fall through */
        case 2:
            if (b->flags & 1)
            {
                alpha = (i32)(b->timer.AsFramesFloat() * 255.0f / b->delay);
                if (alpha > 0xff)
                {
                    alpha = 0xff;
                }
                b->vms[0].prefix.color1.d3dColor = alpha << 24;
            }
            else if (b->fadeTime > 0)
            {
                varF4 = b->phaseTimer - b->timer.AsFramesFloat() * b->phaseTimer / b->fadeTime;
                b->vms[0].prefix.scale.x = varF4 / 16.0f;
                varE4.x = varF4 / 2.0f;
            }
            if (b->timer < b->param4)
            {
                g_Player.CalcLaserHitbox(&laserPos, &varE4.x, &b->pos, b->angle, 0);
            }
            if (b->timer < b->fadeTime)
            {
                goto shot_slot_end;
            }
            b->isActive = 0;
            continue;
        }

    shot_slot_end:
        if (b->speed >= 640.0f)
        {
            b->isActive = 0;
        }
        b->timer.Tick();
        g_AnmManager->ExecuteScript(&b->vms[0]);
    }

    if (bulletManager->clearCount != 0)
    {
        bulletManager->clearCount--;
    }
    bulletManager->timer6ba540.Tick();
    bulletManager->unk6ba54c++;

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: th08 0x432b50
#pragma var_order(i, sinX, b, var10, cosX, slot)
ChainCallbackResult BulletManager::OnDraw(BulletManager *bulletManager)
{
    i32 i;              /* -0x4 */
    f32 sinX;           /* -0x8 */
    PlayerShotData *b;  /* -0xc */
    f32 var10;          /* -0x10 */
    f32 cosX;           /* -0x14 */
    EnemyBullet *slot;  /* -0x18 */

    if (((g_PlayerFlags >> 0xa) & 1) != 0)
    {
        g_AnmManager->SetMixColor(0xfff01010);
    }

    b = (PlayerShotData *)((u8 *)bulletManager + 0x660938);
    g_ItemManager.OnDraw();

    for (i = 0; i < 0x100; i++, b++)
    {
        /* varE0/varE4/varD8/varD4 是循环块局部 (原版在 this 槽 -0x1c 之下: -0x20/-0x24/-0x28/-0x2c)。
         * 第二层绘制用独立的 varD8/varD4, 不复用第一层的 varE0/varE4。 */
        f32 varE0;
        f32 varE4;
        f32 varD8;
        f32 varD4;

        if (b->isActive == 0)
        {
            continue;
        }

        ComputeSinCos(&sinX, &cosX, b->angle);
        /* 432bdb-432bfc: (t-s)/2 + speed 的顺序 */
        var10 = (b->targetSpeed - b->speed) / 2.0f + b->speed;

        /* 原版对 b->pos / vms[].pos 的成员访问带 Float3::Float3() 显式构造调用
         * (432bff/432c18/432c2b/432c45/432c59/432d20/432d3c/432d4f/432d6f/432d83);
         * 而 pos.x/y += g_PlayerPos 处没有 (432c8e/432ca6/432e5c/432e74)。 */
        varE0 = cosX * var10 + b->pos.Float3::Float3().x;
        b->vms[0].pos.Float3::Float3().x = varE0;
        varE4 = sinX * var10 + b->pos.Float3::Float3().y;
        b->vms[0].pos.Float3::Float3().y = varE4;
        b->vms[0].pos.Float3::Float3().z = 0.06f;
        *(u16 *)&b->unk596 = (u16)((*(i16 *)&b->unk596 & 0xff000000) | 0xffffff);
        b->vms[0].pos.x += g_PlayerPos.x;
        b->vms[0].pos.y += g_PlayerPos.y;
        g_AnmManager->Draw2D(&b->vms[0]);

        /* 432ccd-432d17: (speed<16 || speedStep==0) && (unk599[0]==0 || runState!=0) 才画第二层 */
        if ((b->speed < 16.0f || b->speedStep == 0.0f) && (b->unk599[0] == 0 || b->runState != 0))
        {
            varD8 = cosX * b->speed + b->pos.Float3::Float3().x;
            b->vms[1].pos.Float3::Float3().x = varD8;
            varD4 = sinX * b->speed + b->pos.Float3::Float3().y;
            b->vms[1].pos.Float3::Float3().y = varD4;
            b->vms[1].pos.Float3::Float3().z = 0.05f;
            b->vms[1].prefix.color1.d3dColor = b->vms[0].prefix.color1.d3dColor;
            b->vms[1].prefix.flags |= 0x40;
            b->vms[1].prefix.color1.d3dColor = (b->vms[1].prefix.color1.d3dColor & 0xffffff) | 0xff000000;
            b->vms[1].prefix.scale.x = (b->phaseTimer / 10.0f) * ((16.0f - b->speed) / 16.0f);
            b->vms[1].prefix.scale.y = b->vms[1].prefix.scale.x;
            if (b->vms[1].prefix.scale.y <= 0.0f)
            {
                b->vms[1].prefix.scale.x = b->phaseTimer / 10.0f;
                b->vms[1].prefix.scale.y = b->vms[1].prefix.scale.x;
            }
            b->vms[1].pos.x += g_PlayerPos.x;
            b->vms[1].pos.y += g_PlayerPos.y;
            g_AnmManager->Draw2D(&b->vms[1]);
        }
    }

    for (i = 0; i < 6; i++)
    {
        slot = bulletManager->chainHeads[i];
        while (slot != NULL)
        {
            ((BulletManager *)slot)->DrawSingleBullet();
            slot = slot->next;
        }
    }

    g_EffectManager.DrawUnkTypeEffects();
    if (((g_PlayerFlags >> 0xa) & 1) != 0)
    {
        g_AnmManager->SetMixColorDefault();
    }

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

/* AddedCallback: marker 移除（VC7 起始行错位导致 linerec 错配，见 .memory/EXP.md） */
ZunResult BulletManager::AddedCallback(BulletManager *bulletManager)
{
    i32 i;              /* -0x4 */

    if (g_Supervisor.GetUnk164() != 0)
    {
        g_BulletAnm = g_AnmManager->PreloadAnm(6, (const char *)0x4b4ca0);
        bulletManager->bulletAnm = g_BulletAnm;
        if (bulletManager->bulletAnm == NULL)
        {
            return (ZunResult)-1;
        }
    }
    else
    {
        bulletManager->bulletAnm = g_AnmManager->GetAnm(6);
    }

    for (i = 0; i < 0x15; i++)
    {
        /* 原版无 slot 局部槽: 每次访问都重算 bulletManager + i*0xd44 (imul+add,
         * 见 4330f1/43311b/4331d7/4332b1/4332d9 等); spriteId(-0xc)/spriteId2(-0x10)
         * 是各自 if 分支内的块局部。 */
#define BULLET_TEMPLATE() ((EnemyBullet *)((u8 *)bulletManager + (i) * 0xd44))

        bulletManager->bulletAnm->SetAndExecuteScriptIdx(&BULLET_TEMPLATE()->vms[0], sBulletScriptTable[i][0]);
        bulletManager->bulletAnm->SetAndExecuteScriptIdx(&BULLET_TEMPLATE()->vms[1], sBulletScriptTable[i][1]);
        bulletManager->bulletAnm->SetAndExecuteScriptIdx(&BULLET_TEMPLATE()->vms[2], sBulletScriptTable[i][2]);
        bulletManager->bulletAnm->SetAndExecuteScriptIdx(&BULLET_TEMPLATE()->vms[3], sBulletScriptTable[i][3]);
        bulletManager->bulletAnm->SetAndExecuteScriptIdx(&BULLET_TEMPLATE()->vms[4], sBulletScriptTable[i][4]);

        BULLET_TEMPLATE()->vms[0].prefix.flags |= 0x2000;
        BULLET_TEMPLATE()->vms[1].prefix.flags |= 0x2000;
        BULLET_TEMPLATE()->vms[2].prefix.flags |= 0x2000;
        BULLET_TEMPLATE()->vms[3].prefix.flags |= 0x2000;
        BULLET_TEMPLATE()->vms[4].prefix.flags |= 0x2000;

        BULLET_TEMPLATE()->vms[0].baseSpriteIndex = BULLET_TEMPLATE()->vms[0].activeSpriteIndex;

        BULLET_TEMPLATE()->unkD41 = (u8)(i32)(*(f32 *)((u8 *)BULLET_TEMPLATE()->vms[0].loadedSprite + 0x30));

        if (*(f32 *)((u8 *)BULLET_TEMPLATE()->vms[0].loadedSprite + 0x30) <= 8.0f)
        {
            BULLET_TEMPLATE()->collisionSize.x = 4.0f;
            BULLET_TEMPLATE()->collisionSize.y = 4.0f;
            BULLET_TEMPLATE()->unkD42 = 5;
        }
        else if (*(f32 *)((u8 *)BULLET_TEMPLATE()->vms[0].loadedSprite + 0x30) <= 16.0f)
        {
            /* 4333a4/4333a7: spriteId 原地 -2 复用同一槽 (-0xc) */
            i32 spriteId = sBulletScriptTable[i][0];
            spriteId -= 2;
            /* 原版 4333b0: ja 直接跳 default 体 (三元常量 5 → case 5 → default),
             * 跳表 [0..5] 中 4/5 也指向 default (6.0/6.0/3)。 */
            switch ((u32)spriteId > 0x6e ? 5 : sBulletHitSizeTable1[spriteId])
            {
            case 0:
            case 1:
            case 3:
                BULLET_TEMPLATE()->collisionSize.x = 4.0f;
                BULLET_TEMPLATE()->collisionSize.y = 4.0f;
                BULLET_TEMPLATE()->unkD42 = 4;
                break;
            case 2:
                BULLET_TEMPLATE()->collisionSize.x = 4.0f;
                BULLET_TEMPLATE()->collisionSize.y = 4.0f;
                BULLET_TEMPLATE()->unkD42 = 3;
                break;
            default:
                BULLET_TEMPLATE()->collisionSize.x = 6.0f;
                BULLET_TEMPLATE()->collisionSize.y = 6.0f;
                BULLET_TEMPLATE()->unkD42 = 3;
                break;
            }
        }
        else if (*(f32 *)((u8 *)BULLET_TEMPLATE()->vms[0].loadedSprite + 0x30) <= 32.0f)
        {
            /* 4335a0/4335a3: spriteId2 原地 -8 复用同一槽 (-0x10) */
            i32 spriteId2 = sBulletScriptTable[i][0];
            spriteId2 -= 8;
            switch ((u32)spriteId2 > 0x6b ? 2 : sBulletHitSizeTable2[spriteId2])
            {
            case 0:
                BULLET_TEMPLATE()->collisionSize.x = 5.0f;
                BULLET_TEMPLATE()->collisionSize.y = 5.0f;
                BULLET_TEMPLATE()->unkD42 = 2;
                break;
            case 1:
                BULLET_TEMPLATE()->collisionSize.x = 8.0f;
                BULLET_TEMPLATE()->collisionSize.y = 8.0f;
                BULLET_TEMPLATE()->unkD42 = 1;
                break;
            default:
                BULLET_TEMPLATE()->collisionSize.x = 10.0f;
                BULLET_TEMPLATE()->collisionSize.y = 10.0f;
                BULLET_TEMPLATE()->unkD42 = 1;
                break;
            }
        }
        else
        {
            BULLET_TEMPLATE()->unkD42 = 0;
            BULLET_TEMPLATE()->collisionSize.x = 24.0f;
            BULLET_TEMPLATE()->collisionSize.y = 24.0f;
        }
#undef BULLET_TEMPLATE
    }

    FUN_004337f0(&g_ItemManager);

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

/* ============================================================================
 * 帮手 stub（PDB 符号名与 reccmp-functions.csv 原名一致以便调用目标归一化）。
 * 类成员 stub 的类名/成员名对齐 CSV（th08::Bullet::FUN_* 等）；内部未逆向，
 * 保持 stub（不参与本任务的字节级匹配）。
 * ==========================================================================*/

/* 全局 fastcall 单参 stub（CSV 裸名）：FUN_0042ffc0 (ecx=slot)、FUN_004337f0 (ecx=ItemManager)。 */
void FUN_0042ffc0(th08::EnemyBullet *slot);
void FUN_004337f0(th08::ItemManager *self);

/* 42ffc0-4307da: 弹型行为安装器 (OnUpdate state1 每帧调用)。
 * 遍历槽内行为数组 (0x18 结构 @ 0xdd0, 按 dcc 索引), 找 flagsDB0 匹配项,
 * 按 data->flags 分派 (跳表 0x4307db + 索引表 0x4307ef: case 1/0x10/0x1f/0x3f;
 * cmp 链: 0x80/0x100/0x400/0x800/0x2000/0x4000/0x20000/0x40000/0x80000/0x400000/0x800000/0x1000000)。
 * 多数 case 安装行为参数后结束 (dcc++); 0x2000/0x4000/0x80000/0x1000000 继续循环。 */
#pragma var_order(data, d8Sign, ls, ls2)
void FUN_0042ffc0(th08::EnemyBullet *slot)
{
    using namespace th08;
    void *data;
    i32 d8Sign;
    i32 ls;
    i32 ls2;

    do
    {
        if (*(i32 *)((u8 *)slot + 0xdcc) >= 0x12)
            return;
        data = (void *)((u8 *)slot + 0xdd0 + *(i32 *)((u8 *)slot + 0xdcc) * 0x18);
        if (*(u32 *)((u8 *)data + 0x10) == 0)
            return;
        if (*(u32 *)((u8 *)data + 0x14) == 0 && slot->flagsDAC != 0)
            return;
        /* 原版 430043: test; jne switch (匹配时跳入), 落 = 不匹配 → dcc++ 继续循环 */
        if ((slot->flagsDB0 & *(u32 *)((u8 *)data + 0x10)) == 0)
        {
            *(i32 *)((u8 *)slot + 0xdcc) = *(i32 *)((u8 *)slot + 0xdcc) + 1;
            continue;
        }
        switch (*(u32 *)((u8 *)data + 0x10))
        {
            case 0x1:
                /* 430198: 设 bit0x1, 复位 f80 计时器与 f9c */
                slot->flagsDAC |= 0x1;
                ((ZunTimer *)((u8 *)slot + 0xf80))->SetCurrent(0);
                *(i32 *)((u8 *)slot + 0xf9c) = 0;
                break;
            case 0x10:
                /* 4301db: 加速 (bit0x10): fb8=d0, fbc=d4(>= -990 哨兵否则 angle), (fac)复位, fcc=d8,
                 * (fc0).FromAngleMagnitude(fbc, g_ShotSpeed*fb8); 播放 dc8 音效。
                 * 命名 fbcTmp (块级) 使 this@-0x21c 布局对齐; 原版结果槽 -0x224。 */
                {
                    i32 fbcTmp;
                    slot->flagsDAC |= 0x10;
                    *(f32 *)((u8 *)slot + 0xfb8) = *(f32 *)((u8 *)data);
                    if (*(f32 *)((u8 *)data + 4) >= -990.0f)
                        fbcTmp = *(u32 *)((u8 *)data + 4);
                    else
                        fbcTmp = *(u32 *)((u8 *)slot + 0xd74);
                    *(u32 *)((u8 *)slot + 0xfbc) = fbcTmp;
                    ((ZunTimer *)((u8 *)slot + 0xfac))->SetCurrent(0);
                    *(i32 *)((u8 *)slot + 0xfcc) = *(i32 *)((u8 *)data + 8);
                    (*(Float3 *)((u8 *)slot + 0xfc0)).FromAngleMagnitude(
                        *(f32 *)((u8 *)slot + 0xfbc), g_ShotSpeed * *(f32 *)((u8 *)slot + 0xfb8));
                    if (*(i32 *)((u8 *)slot + 0xdcc) != 0 && *(i32 *)((u8 *)slot + 0xdc8) >= 0)
                        g_SoundPlayer.PlaySoundByIdx((SoundIdx) * (i32 *)((u8 *)slot + 0xdc8), 0);
                }
                break;
            case 0x1f:
                /* 4302e1: 螺旋 (bit0x20): fe4=d0, fe8=d4, (fd8)复位, ff8=d8; 播放 dc8 音效 */
                slot->flagsDAC |= 0x20;
                *(f32 *)((u8 *)slot + 0xfe4) = *(f32 *)((u8 *)data);
                *(f32 *)((u8 *)slot + 0xfe8) = *(f32 *)((u8 *)data + 4);
                ((ZunTimer *)((u8 *)slot + 0xfd8))->SetCurrent(0);
                *(i32 *)((u8 *)slot + 0xff8) = *(i32 *)((u8 *)data + 8);
                if (*(i32 *)((u8 *)slot + 0xdcc) != 0 && *(i32 *)((u8 *)slot + 0xdc8) >= 0)
                    g_SoundPlayer.PlaySoundByIdx((SoundIdx) * (i32 *)((u8 *)slot + 0xdc8), 0);
                break;
            case 0x3f:
            case 0x80:
            case 0x100:
                /* 430380: 减速族: 1014=d0, 1010=d4(>= -999 哨兵否则 d68), (1004)复位,
                 * 1024=d8, 1028=dC, 102c=0 */
                {
                    i32 tmp1010;
                    slot->flagsDAC |= *(u32 *)((u8 *)data + 0x10);
                    *(f32 *)((u8 *)slot + 0x1014) = *(f32 *)((u8 *)data);
                    if (*(f32 *)((u8 *)data + 4) >= -999.0f)
                        tmp1010 = *(u32 *)((u8 *)data + 4);
                    else
                        tmp1010 = *(u32 *)((u8 *)slot + 0xd68);
                    *(u32 *)((u8 *)slot + 0x1010) = tmp1010;
                    ((ZunTimer *)((u8 *)slot + 0x1004))->SetCurrent(0);
                    *(i32 *)((u8 *)slot + 0x1024) = *(i32 *)((u8 *)data + 8);
                    *(i32 *)((u8 *)slot + 0x1028) = *(i32 *)((u8 *)data + 0xc);
                    *(i32 *)((u8 *)slot + 0x102c) = 0;
                }
                break;
            case 0x400:
            case 0x800:
                /* 430440: 反弹族: 103c=d0(>=0 否则 d68), 1054=d8, 1050=0 */
                slot->flagsDAC |= *(u32 *)((u8 *)data + 0x10);
                *(f32 *)((u8 *)slot + 0x103c) =
                    *(f32 *)((u8 *)data) >= 0.0f ? *(f32 *)((u8 *)data) : *(f32 *)((u8 *)slot + 0xd68);
                *(i32 *)((u8 *)slot + 0x1054) = *(i32 *)((u8 *)data + 8);
                *(i32 *)((u8 *)slot + 0x1050) = 0;
                break;
            case 0x400000:
            case 0x800000:
                /* 4304c2/4304fd: 环绕族: (1088).SetCurrent(d8) */
                slot->flagsDAC |= *(u32 *)((u8 *)data + 0x10);
                ((ZunTimer *)((u8 *)slot + 0x1088))->SetCurrent(*(i32 *)((u8 *)data + 8));
                break;
            case 0x20000:
                /* 430538: (105c).SetCurrent(d8) */
                slot->flagsDAC |= *(u32 *)((u8 *)data + 0x10);
                ((ZunTimer *)((u8 *)slot + 0x105c))->SetCurrent(*(i32 *)((u8 *)data + 8));
                break;
            case 0x2000:
                /* 430573: da8=d8; 继续循环 */
                slot->unkDA8 = *(i32 *)((u8 *)data + 8);
                *(i32 *)((u8 *)slot + 0xdcc) = *(i32 *)((u8 *)slot + 0xdcc) + 1;
                continue;
            case 0x4000:
                /* 4305a5: 从 g_BulletManager + d8*0xd44 复制 0xd44 字节到槽; SetSprite; 继续循环 */
                memcpy(slot, (u8 *)&g_BulletManager + *(i32 *)((u8 *)data + 8) * 0xd44, 0xd44);
                g_BulletAnm->SetSprite((AnmVm *)slot,
                                       slot->vms[0].activeSpriteIndex + *(i32 *)((u8 *)data + 0xc));
                *(i32 *)((u8 *)slot + 0xdcc) = *(i32 *)((u8 *)slot + 0xdcc) + 1;
                continue;
            case 0x40000:
                /* 43060a: state = 5 (死亡清场) */
                slot->state = 5;
                break;
            case 0x80000:
                /* 43061e: 位置音效; 继续循环 */
                g_SoundPlayer.PlaySoundPositionedByIdx((SoundIdx) * (i32 *)((u8 *)data + 8), slot->pos.x);
                *(i32 *)((u8 *)slot + 0xdcc) = *(i32 *)((u8 *)slot + 0xdcc) + 1;
                continue;
            case 0x1000000:
                /* 43065c: 激光安装 (SetupLaserMove)。原版在 [ebp-0x218] 构造激光数据
                 * (0x42a410), 填 pos/位域/双 data 参数, 复制 data 数组头部, 调用
                 * g_BulletManager.SetupLaserMove; d8 高位符号位设置时 state=5。 */
                {
                    /* 原版 laser 结构 @ [ebp-0x218], 大小 0x1fa: copy 区 (+0x20..+0x1d0) 之后
                     * 有 0x24 字节间隙 (原版未使用), 尾部 u16 字段在 +0x1f4/+0x1f6/+0x1f8。
                     * pack(2) 使结构大小 = 0x1fa (非 4 对齐), 与 ls2@-0x1c 紧邻布局一致。 */
#pragma pack(push, 2)
                    struct LaserData
                    {
                        u16 b8hi;    /* +0x00 (d8>>16)&0xff */
                        u16 b8mid;   /* +0x02 (d8>>8)&0xff */
                        Float3 pos;  /* +0x04 */
                        i32 d0b;     /* +0x10 第二 data */
                        i32 d4b;     /* +0x14 */
                        i32 d0a;     /* +0x18 第一 data */
                        i32 d4a;     /* +0x1c */
                        u8 copy[0x1b0]; /* +0x20 自 0xdd0 复制 (到 +0x1d0) */
                        u8 pad[0x24]; /* +0x1d0 原版间隙 */
                        u16 unk1f4;  /* +0x1f4 data->dC */
                        u16 unk1f6;  /* +0x1f6 第二 data->d8 */
                        u16 unk1f8;  /* +0x1f8 (d8>>24)&0x7f */
                    };
#pragma pack(pop)
                    LaserData laser;
                    d8Sign = *(i32 *)((u8 *)data + 8) & 0x80000000;
                    ((void(__fastcall *)(void *))0x42a410)(&laser);
                    laser.unk1f8 = (u16)((*(u32 *)((u8 *)data + 8) >> 24) & 0x7f);
                    laser.b8hi = (u16)((*(u32 *)((u8 *)data + 8) >> 16) & 0xff);
                    laser.b8mid = (u16)((*(u32 *)((u8 *)data + 8) >> 8) & 0xff);
                    ls = *(u32 *)((u8 *)data + 8) & 0xff;
                    laser.unk1f4 = (u16) * (u32 *)((u8 *)data + 0xc);
                    laser.d0a = *(u32 *)((u8 *)data);
                    laser.d4a = *(u32 *)((u8 *)data + 4);
                    laser.pos = slot->pos;
                    data = (u8 *)data + 0x18;
                    *(i32 *)((u8 *)slot + 0xdcc) = *(i32 *)((u8 *)slot + 0xdcc) + 1;
                    laser.unk1f6 = *(u16 *)((u8 *)data + 8);
                    ls2 = *(u32 *)((u8 *)data + 0xc);
                    laser.d0b = *(u32 *)((u8 *)data);
                    laser.d4b = *(u32 *)((u8 *)data + 4);
                    memcpy(laser.copy, (u8 *)slot + 0xdd0, 0x1b0);
                    g_BulletManager.SetupLaserMove((Float3 *)&laser);
                    *(i32 *)((u8 *)slot + 0xdcc) = *(i32 *)((u8 *)slot + 0xdcc) + 1;
                    if (d8Sign != 0)
                    {
                        slot->state = 5;
                        break;
                    }
                    continue;
                }
            default:
                break;
            }
            *(i32 *)((u8 *)slot + 0xdcc) = *(i32 *)((u8 *)slot + 0xdcc) + 1;
            break;
        }
    while (true);
}
/* 4337f0-433819: ItemManager 初始化。
 * 反汇编: rep stosd 清零 0x5ec25 dwords (= sizeof(ItemManager) 0x17b094),
 * 然后 itemListTail = &itemListHead (0x17b090 = this + 0x17adac)。 */
void FUN_004337f0(th08::ItemManager *self)
{
    memset(self, 0, sizeof(th08::ItemManager));
    self->itemListTail = &self->itemListHead;
}

namespace th08
{

/* 敌弹槽级行为 helper（this = EnemyBullet 槽；CSV 名 th08::Bullet::FUN_*）。 */
/* 432170-4321a6: 复位弹型槽。
 * 反汇编: state(u16@0xdb8)=0; timerD80.SetCurrent(0); timerD8C.SetCurrent(0)。
 * 无命名局部: this 占 -0x4 (原版 push ecx; mov [ebp-0x4],ecx)。 */
void Bullet::FUN_00432170()
{
    *(u16 *)((u8 *)this + 0xdb8) = 0;
    ((ZunTimer *)((u8 *)this + 0xd80))->SetCurrent(0);
    ((ZunTimer *)((u8 *)this + 0xd8c))->SetCurrent(0);
}
/* 432210-4322a8: flagsDAC bit0 行为。
 * 反汇编槽位: tmp@-0x4, this@-0x8 (var_order(tmp) 复现);
 * 0xf80 计时器 <= 0x10 时减速转向, 否则翻转 flagsDAC bit0; 最后 Tick。 */
#pragma var_order(tmp)
void Bullet::FUN_00432210()
{
    f32 tmp;
    if (((ZunTimer *)((u8 *)this + 0xf80))->operator<=(0x10))
    {
        /* 43223b-43224d: tmp = 5.0f - AsFramesFloat()*5.0f/16.0f (0x4b4304=5.0, 0x4b42d4=16.0) */
        tmp = 5.0f - ((ZunTimer *)((u8 *)this + 0xf80))->AsFramesFloat() * 5.0f / 16.0f;
        /* 432253-432279: vel.FromAngleMagnitude(angle@0xd74, (tmp + 0xd68)*g_ShotSpeed) */
        ((EnemyBullet *)this)->vel.FromAngleMagnitude(
            *(f32 *)((u8 *)this + 0xd74), (tmp + *(f32 *)((u8 *)this + 0xd68)) * g_ShotSpeed);
    }
    else
    {
        /* 432280-43228f: flagsDAC ^= 1 */
        ((EnemyBullet *)this)->flagsDAC ^= 1;
    }
    ((ZunTimer *)((u8 *)this + 0xf80))->Tick();
}
/* 4322b0-432384: flagsDAC bit0x10 — 目标角转向。
 * 反汇编槽位 (无命名局部): this@-0x10, operator* 返回槽(Float3)@-0xc..-0x4。
 * (fac)计时器 >= fcc 时清 bit0x10; 否则 vel += (fc0)*g_ShotSpeed,
 * |vel.x|/|vel.y| > 0.0001 (0x4b456c) 时 angle = atan2(vel.y, vel.x) (0x40c7b0)。 */
void Bullet::FUN_004322b0()
{
    if (((ZunTimer *)((u8 *)this + 0xfac))->operator>=(*(i32 *)((u8 *)this + 0xfcc)))
    {
        ((EnemyBullet *)this)->flagsDAC &= ~0x10;
    }
    else
    {
        ((EnemyBullet *)this)->vel += *(Float3 *)((u8 *)this + 0xfc0) * g_ShotSpeed;
        if (fabsf(((EnemyBullet *)this)->vel.x) > 0.0001f ||
            fabsf(((EnemyBullet *)this)->vel.y) > 0.0001f)
        {
            /* 原版 push vel.x 先/push vel.y 后 → EclAngleFromDxDy(dx=vel.y, dy=vel.x) = atan2(vel.y, vel.x) */
            *(f32 *)((u8 *)this + 0xd74) =
                EclAngleFromDxDy(((EnemyBullet *)this)->vel.y, ((EnemyBullet *)this)->vel.x);
        }
    }
    ((ZunTimer *)((u8 *)this + 0xfac))->Tick();
}
/* 432390-432454: flagsDAC bit0x20 — 匀速螺旋。
 * 反汇编: this@-0x4, 无命名局部。(fd8)>=ff8 时清 bit0x20;
 * 否则 angle += g_ShotSpeed*fe8, d68 += g_ShotSpeed*fe4,
 * vel.FromAngleMagnitude(angle, g_ShotSpeed*d68)。 */
void Bullet::FUN_00432390()
{
    if (((ZunTimer *)((u8 *)this + 0xfd8))->operator>=(*(i32 *)((u8 *)this + 0xff8)))
    {
        ((EnemyBullet *)this)->flagsDAC &= ~0x20;
    }
    else
    {
        *(f32 *)((u8 *)this + 0xd74) =
            AddNormalizeAngle(*(f32 *)((u8 *)this + 0xd74), g_ShotSpeed * *(f32 *)((u8 *)this + 0xfe8));
        *(f32 *)((u8 *)this + 0xd68) += g_ShotSpeed * *(f32 *)((u8 *)this + 0xfe4);
        ((EnemyBullet *)this)->vel.FromAngleMagnitude(
            *(f32 *)((u8 *)this + 0xd74), g_ShotSpeed * *(f32 *)((u8 *)this + 0xd68));
    }
    ((ZunTimer *)((u8 *)this + 0xfd8))->Tick();
}
/* 432460-432598: flagsDAC bit0x40 — 减速 + 转向递增。
 * 反汇编槽位 (var_order(tmp)): tmp@-0x4, this@-0x8。
 * (1004)>=1024: 播 dc8 音效, 102c++, 102c>=1028 时清 bit0x40,
 * angle += 1014, d68=1010, tmp=d68, (1004).SetCurrent(0);
 * else: tmp = d68 - AsFramesFloat()*d68/1024 (fidiv); 汇合后 FromAngleMagnitude(angle, tmp*g_ShotSpeed), Tick。 */
#pragma var_order(tmp)
void Bullet::FUN_00432460()
{
    f32 tmp;
    if (((ZunTimer *)((u8 *)this + 0x1004))->operator>=(*(i32 *)((u8 *)this + 0x1024)))
    {
        if (*(i32 *)((u8 *)this + 0xdc8) >= 0)
        {
            g_SoundPlayer.PlaySoundByIdx((SoundIdx) * (i32 *)((u8 *)this + 0xdc8), 0);
        }
        (*(i32 *)((u8 *)this + 0x102c))++;
        if (*(i32 *)((u8 *)this + 0x102c) >= *(i32 *)((u8 *)this + 0x1028))
        {
            ((EnemyBullet *)this)->flagsDAC &= ~0x40;
        }
        *(f32 *)((u8 *)this + 0xd74) += *(f32 *)((u8 *)this + 0x1014);
        *(f32 *)((u8 *)this + 0xd68) = *(f32 *)((u8 *)this + 0x1010);
        tmp = *(f32 *)((u8 *)this + 0xd68);
        ((ZunTimer *)((u8 *)this + 0x1004))->SetCurrent(0);
    }
    else
    {
        tmp = *(f32 *)((u8 *)this + 0xd68) -
              ((ZunTimer *)((u8 *)this + 0x1004))->AsFramesFloat() * *(f32 *)((u8 *)this + 0xd68) /
                  *(i32 *)((u8 *)this + 0x1024);
    }
    ((EnemyBullet *)this)->vel.FromAngleMagnitude(*(f32 *)((u8 *)this + 0xd74), tmp * g_ShotSpeed);
    ((ZunTimer *)((u8 *)this + 0x1004))->Tick();
}
/* 4325a0-4326ce: flagsDAC bit0x100 — 同 bit0x40 但 angle = 1014 (mov 非 +=), 清 bit0x100。 */
#pragma var_order(tmp)
void Bullet::FUN_004325a0()
{
    f32 tmp;
    if (((ZunTimer *)((u8 *)this + 0x1004))->operator>=(*(i32 *)((u8 *)this + 0x1024)))
    {
        if (*(i32 *)((u8 *)this + 0xdc8) >= 0)
        {
            g_SoundPlayer.PlaySoundByIdx((SoundIdx) * (i32 *)((u8 *)this + 0xdc8), 0);
        }
        (*(i32 *)((u8 *)this + 0x102c))++;
        if (*(i32 *)((u8 *)this + 0x102c) >= *(i32 *)((u8 *)this + 0x1028))
        {
            ((EnemyBullet *)this)->flagsDAC &= ~0x100;
        }
        *(f32 *)((u8 *)this + 0xd74) = *(f32 *)((u8 *)this + 0x1014);
        *(f32 *)((u8 *)this + 0xd68) = *(f32 *)((u8 *)this + 0x1010);
        tmp = *(f32 *)((u8 *)this + 0xd68);
        ((ZunTimer *)((u8 *)this + 0x1004))->SetCurrent(0);
    }
    else
    {
        tmp = *(f32 *)((u8 *)this + 0xd68) -
              ((ZunTimer *)((u8 *)this + 0x1004))->AsFramesFloat() * *(f32 *)((u8 *)this + 0xd68) /
                  *(i32 *)((u8 *)this + 0x1024);
    }
    ((EnemyBullet *)this)->vel.FromAngleMagnitude(*(f32 *)((u8 *)this + 0xd74), tmp * g_ShotSpeed);
    ((ZunTimer *)((u8 *)this + 0x1004))->Tick();
}
/* 4326e0-43282c: flagsDAC bit0x80 — 同族但 angle = AddNormalizeAngle(AngleToPlayer(pos), 1014)。
 * 反汇编 43276b-432795: 先 push 1014 (AddNormalizeAngle 第二参预压), 再 push pos 调 AngleToPlayer,
 * 结果压栈调 AddNormalizeAngle (嵌套参数顺序已 standalone 验证)。清 bit0x80。 */
#pragma var_order(tmp)
void Bullet::FUN_004326e0()
{
    f32 tmp;
    if (((ZunTimer *)((u8 *)this + 0x1004))->operator>=(*(i32 *)((u8 *)this + 0x1024)))
    {
        if (*(i32 *)((u8 *)this + 0xdc8) >= 0)
        {
            g_SoundPlayer.PlaySoundByIdx((SoundIdx) * (i32 *)((u8 *)this + 0xdc8), 0);
        }
        (*(i32 *)((u8 *)this + 0x102c))++;
        if (*(i32 *)((u8 *)this + 0x102c) >= *(i32 *)((u8 *)this + 0x1028))
        {
            ((EnemyBullet *)this)->flagsDAC &= ~0x80;
        }
        *(f32 *)((u8 *)this + 0xd74) =
            AddNormalizeAngle(g_Player.AngleToPlayer(&((EnemyBullet *)this)->pos), *(f32 *)((u8 *)this + 0x1014));
        *(f32 *)((u8 *)this + 0xd68) = *(f32 *)((u8 *)this + 0x1010);
        tmp = *(f32 *)((u8 *)this + 0xd68);
        ((ZunTimer *)((u8 *)this + 0x1004))->SetCurrent(0);
    }
    else
    {
        tmp = *(f32 *)((u8 *)this + 0xd68) -
              ((ZunTimer *)((u8 *)this + 0x1004))->AsFramesFloat() * *(f32 *)((u8 *)this + 0xd68) /
                  *(i32 *)((u8 *)this + 0x1024);
    }
    ((EnemyBullet *)this)->vel.FromAngleMagnitude(*(f32 *)((u8 *)this + 0xd74), tmp * g_ShotSpeed);
    ((ZunTimer *)((u8 *)this + 0x1004))->Tick();
}
/* 432830-4329e3: flagsDAC bit0xc00 — 边界反弹。
 * 反汇编槽位 (var_order(tmp)): tmp@-0x4, this@-0x8。
 * 界内 (IsWithinPlayfield 4参, w=loadedSprite+0x34, h=+0x30) 直接结束;
 * 否则播 dc8 音效, x<0 || x>=384 水平反弹 (angle=-angle-π 后归一化, 原版两条语句),
 * y<0 || (y>=448 && flagsDAC&0x400) 垂直反弹 (angle=-angle),
 * d68=103c, tmp=d68, FromAngleMagnitude(angle, tmp*g_ShotSpeed), 1050++, 1050>=1054 时清 bit0xc00。 */
#pragma var_order(tmp)
void Bullet::FUN_00432830()
{
    f32 tmp;
    if (!g_GameManager.IsWithinPlayfield(
            ((EnemyBullet *)this)->pos.Float3::Float3().x, ((EnemyBullet *)this)->pos.Float3::Float3().y,
            *(f32 *)((u8 *)((EnemyBullet *)this)->vms[0].loadedSprite + 0x34),
            *(f32 *)((u8 *)((EnemyBullet *)this)->vms[0].loadedSprite + 0x30)))
    {
        if (*(i32 *)((u8 *)this + 0xdc8) >= 0)
        {
            g_SoundPlayer.PlaySoundByIdx((SoundIdx) * (i32 *)((u8 *)this + 0xdc8), 0);
        }
        if (((EnemyBullet *)this)->pos.x < 0.0f || ((EnemyBullet *)this)->pos.x >= 384.0f)
        {
            /* 4328d6-432904: 原版先 angle = -angle - π (fstp [d74]) 再 AddNormalizeAngle(angle, 0) 两条语句 */
            *(f32 *)((u8 *)this + 0xd74) = -*(f32 *)((u8 *)this + 0xd74) - 3.141593f;
            *(f32 *)((u8 *)this + 0xd74) = AddNormalizeAngle(*(f32 *)((u8 *)this + 0xd74), 0.0f);
        }
        if (((EnemyBullet *)this)->pos.y < 0.0f ||
            (((EnemyBullet *)this)->pos.y >= 448.0f && (((EnemyBullet *)this)->flagsDAC & 0x400)))
        {
            *(f32 *)((u8 *)this + 0xd74) = -*(f32 *)((u8 *)this + 0xd74);
        }
        *(f32 *)((u8 *)this + 0xd68) = *(f32 *)((u8 *)this + 0x103c);
        tmp = *(f32 *)((u8 *)this + 0xd68);
        ((EnemyBullet *)this)->vel.FromAngleMagnitude(*(f32 *)((u8 *)this + 0xd74), tmp * g_ShotSpeed);
        (*(i32 *)((u8 *)this + 0x1050))++;
        if (*(i32 *)((u8 *)this + 0x1050) >= *(i32 *)((u8 *)this + 0x1054))
        {
            ((EnemyBullet *)this)->flagsDAC &= ~0xc00;
        }
    }
}
/* 4329f0-432a96: flagsDAC bit0x400000 — x 环绕。
 * 反汇编: this@-0x4。pos.x<0.0 (double 比较, fcomp qword [0.0]) 时 += 384.0f,
 * else if pos.x>384.0 (double) 时 -= 384.0f; (1088)<=0 时翻转 bit0x400000 否则 (1088)--。 */
void Bullet::FUN_004329f0()
{
    if (((EnemyBullet *)this)->pos.x < 0.0)
    {
        ((EnemyBullet *)this)->pos.x += 384.0f;
    }
    else if (((EnemyBullet *)this)->pos.x > 384.0)
    {
        ((EnemyBullet *)this)->pos.x -= 384.0f;
    }
    if (((ZunTimer *)((u8 *)this + 0x1088))->operator<=(0))
    {
        ((EnemyBullet *)this)->flagsDAC ^= 0x400000;
    }
    else
    {
        (*(ZunTimer *)((u8 *)this + 0x1088))--;
    }
}
/* 432aa0-432b46: flagsDAC bit0x800000 — y 环绕 (同 bit0x400000, 448.0 边界)。 */
void Bullet::FUN_00432aa0()
{
    if (((EnemyBullet *)this)->pos.y < 0.0)
    {
        ((EnemyBullet *)this)->pos.y += 448.0f;
    }
    else if (((EnemyBullet *)this)->pos.y > 448.0)
    {
        ((EnemyBullet *)this)->pos.y -= 448.0f;
    }
    if (((ZunTimer *)((u8 *)this + 0x1088))->operator<=(0))
    {
        ((EnemyBullet *)this)->flagsDAC ^= 0x800000;
    }
    else
    {
        (*(ZunTimer *)((u8 *)this + 0x1088))--;
    }
}

/* BulletManager 帮手（CSV 名 th08::BulletManager::FUN_004321b0 / DrawSingleBullet）。 */
/* 4321b0-432208: 6 个弹型链头清零。
 * 反汇编: 0x6ba568..0x6ba554 倒序 6 个 mov dword,0 (寄存器 eax/ecx/edx 交替);
 * 无命名局部, this@-0x4。 */
void BulletManager::FUN_004321b0()
{
    this->chainHeads[5] = 0;
    this->chainHeads[4] = 0;
    this->chainHeads[3] = 0;
    this->chainHeads[2] = 0;
    this->chainHeads[1] = 0;
    this->chainHeads[0] = 0;
}
/* 432f20-43305f: 绘制单个敌弹槽 (OnDraw 按链遍历调用, ecx=EnemyBullet 槽)。
 * 槽位 (var_order(vm) 复现): vm@-0x4, this@-0x8, switch 判别@-0xc,
 * 两个表达式临时@-0x10/-0x14 (左右 Float3 ctor 调用迫使右侧物化, 已 standalone 验证)。
 * 依据 unkDB8(state) 选 vms[1..4]/vms[0] (跳表 0x433060), 写视口偏移坐标,
 * 白化 color1 (保留 alpha), type 非零时按 π/2+angle 旋转, 最后 Draw2D。 */
#pragma var_order(vm)
void BulletManager::DrawSingleBullet()
{
    AnmVm *vm;
    switch (((EnemyBullet *)this)->state - 2)
    {
    case 0: vm = &((EnemyBullet *)this)->vms[1]; break;
    case 1: vm = &((EnemyBullet *)this)->vms[2]; break;
    case 2: vm = &((EnemyBullet *)this)->vms[3]; break;
    case 3: vm = &((EnemyBullet *)this)->vms[4]; break;
    default: vm = &((EnemyBullet *)this)->vms[0]; break;
    }
    /* 432f8c-432ffa: 左右 .Float3::Float3() 显式构造调用 (ZUN 怪癖) */
    vm->pos.Float3::Float3().x = *(f32 *)GAME_VIEWPORT_FLOATS + ((EnemyBullet *)this)->pos.Float3::Float3().x;
    vm->pos.Float3::Float3().y = *(f32 *)(GAME_VIEWPORT_FLOATS + 4) + ((EnemyBullet *)this)->pos.Float3::Float3().y;
    vm->pos.Float3::Float3().z = 0.05f;
    /* 432ffb-433018: color1 = (color1 & 0xff000000) | 0xffffff (保留 alpha 的白) */
    vm->prefix.color1.d3dColor = (vm->prefix.color1.d3dColor & 0xff000000) | 0xffffff;
    /* 433019-43304b: type(i16@0x1fc) 非零时 SetZRotation(AddNormalizeAngle(π/2 + angle, 0)) */
    if (vm->prefix.type != 0)
    {
        vm->SetZRotation(AddNormalizeAngle(1.5707964f + *(f32 *)((u8 *)this + 0xd74), 0.0f));
    }
    /* 43304d-43305b: g_AnmManager->Draw2D(vm) */
    g_AnmManager->Draw2D(vm);
}

/* Player 弹幕碰撞帮手（CSV 名 th08::Player::FUN_0044a230 / FUN_0044a470 / CalcLaserHitbox）。 */
i32 Player::FUN_0044a230(Float3 *, Float3 *)
{
    return 0;
}
i32 Player::FUN_0044a470(Float3 *, Float3 *)
{
    return 0;
}
i32 Player::CalcLaserHitbox(Float3 *, f32 *, Float3 *, f32, i32)
{
    return 0;
}

/* EffectManager 绘制帮手（CSV 名 th08::EffectManager::DrawUnkTypeEffects）。 */
void EffectManager::DrawUnkTypeEffects() {}

/* Float3::operator/(f32) 定义在 Background.cpp (0x40c7d0)，此处无 stub。 */

} /* namespace th08 */
