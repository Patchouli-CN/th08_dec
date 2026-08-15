#include "th_pch.h"

#include "EnemyManager.hpp"

#include <string.h>
#include "AsciiManager.hpp"
#include "BulletManager.hpp"
#include "EffectManager.hpp"
#include "Gui.hpp"
#include "ItemManager.hpp"
#include "Player.hpp"
#include "ReplayManager.hpp"
#include "Spellcard.hpp"
#include "Supervisor.hpp"

// 0x42eb10: standalone angle-interpolation helper (a→b by factor c, wrapping).
// Declared/defined at global scope (outside namespace th08) so its PDB symbol
// matches the CSV name "FUN_0042eb10" and OnDrawImpl's call target normalizes.
f32 FUN_0042eb10(f32 a, f32 b, f32 c);

namespace th08
{

DIFFABLE_STATIC(EnemyManager, g_EnemyManager);
DIFFABLE_STATIC(AnmLoaded, g_EnemyAnmLoaded);
DIFFABLE_STATIC(AnmLoaded, g_EnemyAnmLoaded2);
DIFFABLE_STATIC(ChainElem, g_EnemyManagerCalcChain);
DIFFABLE_STATIC(ChainElem, g_EnemyManagerDrawChainHighPrio);
DIFFABLE_STATIC(ChainElem, g_EnemyManagerDrawChainLowPrio);
DIFFABLE_EXTERN(f32, g_EclExitLeftBound); /* EclManager.cpp 定义, 0x17d61ac (玩家位置 x) */
/* 未登记全局补登（reccmp-globals.csv 同步；语义见注释与 OnUpdate 反汇编）。 */
DIFFABLE_STATIC(i32, g_Unknown164d0a8);   /* 0x164d0a8 人类模式(非 youkai)累计帧数 (OnUpdate 条件自增) */
DIFFABLE_STATIC(i32, g_Unknown164d0ac);   /* 0x164d0ac 游戏内累计帧数 (OnUpdate 每帧自增; 0x43ae8f 清零) */
DIFFABLE_STATIC(i32, g_17d6ed4);          /* 0x17d6ed4 只读状态标志 (全库无写; var14 来源) */
DIFFABLE_STATIC(Float3, g_BossPos);       /* 0x18b899c boss 敌人位置跟踪 (OnUpdate 写 enemy->movePos) */
DIFFABLE_STATIC(Enemy *, g_BossEnemy);    /* 0x18b89b4 当前 boss 敌人指针 (移除时清零) */
DIFFABLE_STATIC(i32, g_BossPosFlag);      /* 0x18b89b8 boss 位置有效标志 (OnUpdate 置 1) */
DIFFABLE_STATIC(ZunTimer, g_18b89ec);     /* 0x18b89ec playerState 0→3 切换计时器 */

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

// EclTimeline is not defined elsewhere in the project; declare a minimal local shape so the
// OnUpdate timeline-loop call site (thiscall on this+0x9dcdd0 + i*0x10) gets a direct call to 0x42a8a0.
struct EclTimeline
{
    void FUN_0042a8a0(); // 0x42a8a0
};

void EclTimeline::FUN_0042a8a0()
{
}

/* 跨类 thiscall 帮手 stub（内部未逆向；PDB 名对齐 reccmp-functions.csv，
 * 使 OnUpdate/OnDrawImpl 的调用目标可归一化）。 */
i32 Player::FUN_00451670(Float3 *, Float3 *, void *, i32 *)
{
    return 0;
}
i32 Spellcard::FUN_0042dff0()
{
    return 0;
}
i32 EclManager::GetTimelineCount()
{
    return 0;
}
void *EclManager::GetTimeline(i32)
{
    return NULL;
}
i32 ZunTimer::operator%(i32)
{
    return 0;
}
void *BulletManager::FUN_00430aa0(i32, i32)
{
    return NULL;
}
ZunResult AnmManager::DrawVertices(AnmVm *, void *, i32)
{
    return ZUN_SUCCESS;
}
void Gui::FUN_00437ddd(i32)
{
}

// OnUpdate 遍历阶段调用的 Enemy 辅助 stub (thiscall 视图; 内部未逆向, 保持 stub)。
void Enemy::enemy_fun_00415c80()
{
}
void Enemy::FUN_0042c420()
{
}
void Enemy::FUN_0042bcf0()
{
}
i32 Enemy::FUN_0042b490()
{
    return 0;
}
i32 Enemy::FUN_0042b930()
{
    return 0;
}
void Enemy::FUN_0042deb0()
{
}
void Enemy::FUN_0042c290(Float3 *, Float3 *)
{
}
void Enemy::FUN_0042b370(i32)
{
}
void Enemy::FUN_0042adb0(i32)
{
}
void Enemy::FUN_0042bea0(i32)
{
}
void Enemy::FUN_0042e010()
{
}
void EnemyManager::FUN_0042c3b0()
{
}

// FUNCTION: th08 0x42c660
/* 栈槽位对齐原版（var_order 从 -0x4 向下；Float3 占 3 槽，x 在最大负偏移）。
 * 槽位证据：原版反汇编 0x42c660-0x42de95 — movePosL x@-0xc, difficulty@-0x10,
 * var14@-0x14, var18@-0x18, i@-0x1c, hitbox x@-0x28, var2c@-0x2c, j@-0x30,
 * damaged@-0x34, enemy@-0x38, scoreGain@-0x3c, bossDelta x@-0x48,
 * damageBox1 x@-0x54, damageBox0 x@-0x60, loopVar64@-0x64, deadFloat70@-0x70,
 * newEnemy@-0x74, loopVar78@-0x78, markerPos x@-0x84。 */
#pragma var_order(movePosL, difficulty, var14, var18, i, hitbox, var2c, j, damaged, enemy, scoreGain, bossDelta, damageBox1, damageBox0, loopVar64, deadFloat70, newEnemy, loopVar78, markerPos)
ChainCallbackResult EnemyManager::OnUpdate(EnemyManager *self)
{
    /* 声明顺序（初始化/构造顺序）对齐原版 0x42c671-0x42c690：
     * var14=0 → hitbox 构造 → bossDelta 构造 → movePosL 构造 → difficulty=0xa。
     * 槽位由上方 var_order 控制（与声明顺序独立）。 */
    i32 var14 = 0;      // -0x14
    Float3 hitbox;      // -0x28
    Float3 bossDelta;   // -0x48
    Float3 movePosL;    // -0xc
    i32 difficulty = 0xa; // -0x10
    i32 var18;          // -0x18
    i32 i;              // -0x1c
    i32 var2c;          // -0x2c
    i32 j;              // -0x30
    i32 damaged;        // -0x34
    Enemy *enemy;       // -0x38
    i32 scoreGain;      // -0x3c
    i32 loopVar64;      // -0x64 激光区循环变量 (eclDataArray0/dataSlots 清零循环)
    Enemy *newEnemy;    // -0x74
    i32 loopVar78;      // -0x78 after_bullet 区循环变量 (eclDataArray0 清零循环)

    if (g_Gui.IsMsgActive() == 0)
    {
        g_164d30c++;
        if (((ZunTimer *)((u8 *)self + 0x9dced0))->AsFrames() >= 0x10)
        {
            g_Unknown164d0ac++;
            if (g_Player.isYoukaiMode == 0)
            {
                g_Unknown164d0a8++;
            }
        }
    }

    if ((g_PlayerFlags >> 0xa) & 1)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    if ((((g_PlayerFlags >> 0xd) & 1) != 0) && (*(i32 *)((u8 *)self + 0x9dcda0) != 0))
    {
        Float3 damageBox0 = Float3(384.0f, 448.0f, 0.0f);
        Float3 damageBox1 = Float3(192.0f, 224.0f, 0.0f);
        g_Player.FUN_00451670(&damageBox1, &damageBox0,
                              (void *)(*(i32 *)((u8 *)self + 0x9dcda0) + 0x2e10), &var14);
    }

    self->FUN_0042c3b0();

    *(i32 *)((u8 *)self + 0x9dcee8) = 0;
    *(i32 *)((u8 *)self + 0x9dcee4) = 0;
    *(i32 *)((u8 *)self + 0x9dcee0) = 0;
    *(i32 *)((u8 *)self + 0x9dcedc) = 0;

    for (i = 0; i < ((EclManager *)&g_EclInterruptTable)->GetTimelineCount(); i++)
    {
        if (*(i32 *)((u8 *)self + 0x9dcddc + i * 0x10) == 0)
        {
            *(i32 *)((u8 *)self + 0x9dcddc + i * 0x10) =
                (i32)((EclManager *)&g_EclInterruptTable)->GetTimeline(i);
        }
        ((EclTimeline *)((u8 *)self + 0x9dcdd0 + i * 0x10))->FUN_0042a8a0();
    }

    enemy = &self->enemies[0];
    *(i32 *)((u8 *)self + 0x9dcdc4) = 0;
    for (i = 0; i < 0x1e0; i++, enemy++)
    {
        if ((enemy->flags & 1) == 0)
        {
            if (g_BossEnemy == enemy)
            {
                g_BossEnemy = NULL;
            }
            continue;
        }

        damaged = 0;

        if ((enemy->flags >> 0xa) & 1)
        {
            enemy->movePos = enemy->pos + enemy->moveVec;
            *(i32 *)((u8 *)enemy + 0x2d90) = 0;
            goto laser_or_move;
        }

        (*(i32 *)((u8 *)self + 0x9dcdc4))++;

        if ((((enemy->flags >> 0x1e) & 1) != 0 && g_17d6ed4 == 0 &&
             g_Player.playerState == 0) ||
            ((enemy->anmFlags >> 7) & 1))
        {
            enemy->eclTimer--;
            goto after_bullet;
        }

        if ((enemy->flags >> 8) & 1)
        {
            enemy->FUN_0042c420();
        }

    run_ecl:
        if (((EclManager *)&g_EclInterruptTable)->RunEcl(enemy) == -1)
        {
            enemy->flags &= ~1u;
            enemy->FUN_0042bcf0();
            continue;
        }

        if ((enemy->flags >> 0x1d) & 1)
        {
            enemy->movePos = enemy->pos + enemy->moveVec;
            *(i32 *)((u8 *)enemy + 0x2d90) = 0;
        }
        else
        {
            enemy->InitMoveAfterSetPos();
            enemy->FUN_0042deb0();
            enemy->InitMoveAfterSetPos();

            if (enemy->ownerEnemy != NULL && ((enemy->flags >> 9) & 1))
            {
                enemy->moveVec = enemy->ownerEnemy->pos;
            }
            enemy->movePos = enemy->pos + enemy->moveVec;
            *(i32 *)((u8 *)enemy + 0x2d90) = 0;
        }

        if (enemy->linkedEffect != 0)
        {
            *(Float3 *)(enemy->linkedEffect + 0x2a4) = enemy->movePos;
        }

        if (enemy->aiFlags != 0)
        {
            j = (i16)enemy->aiParam0 - 1;
            for (; j > 0; j--)
            {
                *(Float3 *)((u8 *)enemy + 0x3394 + j * 0x1c) =
                    *(Float3 *)((u8 *)enemy + 0x3394 + (j - 1) * 0x1c);
                *(Float3 *)((u8 *)enemy + 0x33a0 + j * 0x1c) =
                    *(Float3 *)((u8 *)enemy + 0x33a0 + (j - 1) * 0x1c);
                *(i32 *)((u8 *)enemy + 0x33ac + j * 0x1c) =
                    *(i32 *)((u8 *)enemy + 0x33ac + (j - 1) * 0x1c);
            }
            *(Float3 *)((u8 *)enemy + 0x3394) = enemy->movePos;
            *(Float3 *)((u8 *)enemy + 0x33a0) = *(Float3 *)((u8 *)enemy + 0x2d4c);
            *(i32 *)((u8 *)enemy + 0x33ac) = *(i32 *)((u8 *)enemy + 0x2d94);
        }

        if (*(i32 *)((u8 *)enemy + 0x230) == 0)
        {
            enemy->flags |= 0x10;
        }

        if (((enemy->flags >> 4) & 1) == 0 && ((enemy->flags >> 0x18) & 1) == 0)
        {
            if (g_GameManager.IsWithinPlayfield(enemy->movePos.x, enemy->movePos.y,
                                                *(f32 *)(*(i32 *)((u8 *)enemy + 0x230) + 0x30),
                                                *(f32 *)(*(i32 *)((u8 *)enemy + 0x230) + 0x34)))
            {
                enemy->flags |= 0x1000000;
                goto bullet_section;
            }
        }

        if (((enemy->flags >> 0x18) & 1) && ((enemy->flags >> 0x1c) & 1) == 0)
        {
            if (enemy->aiFlags == 0)
            {
                if (g_GameManager.IsWithinPlayfield(enemy->movePos.x, enemy->movePos.y,
                                                    *(f32 *)(*(i32 *)((u8 *)enemy + 0x230) + 0x30),
                                                    *(f32 *)(*(i32 *)((u8 *)enemy + 0x230) + 0x34)) == 0)
                {
                    goto enemy_offscreen;
                }
            }
            if (enemy->aiFlags != 0)
            {
                if (g_GameManager.IsWithinPlayfield(
                        *(f32 *)((u8 *)enemy + 0x3394 + ((i16)enemy->aiParam1 - 1) * 0x1c),
                        *(f32 *)((u8 *)enemy + 0x3398 + ((i16)enemy->aiParam1 - 1) * 0x1c),
                        *(f32 *)(*(i32 *)((u8 *)enemy + 0x230) + 0x30),
                        *(f32 *)(*(i32 *)((u8 *)enemy + 0x230) + 0x34)) == 0)
                {
                    goto enemy_offscreen;
                }
            }
            goto bullet_section;
        }

    bullet_section:
        if (enemy->FUN_0042b490() != 0)
        {
            goto run_ecl;
        }
        if (enemy->eclDataValue0 >= 0 && enemy->FUN_0042b930() != 0)
        {
            goto run_ecl;
        }

        *(i32 *)((u8 *)enemy + 0x1fc) = *(i32 *)((u8 *)enemy + 0x2e20);
        g_AnmManager->ExecuteScript((AnmVm *)((u8 *)enemy + 0xc));
        *(i32 *)((u8 *)enemy + 0x2e20) = *(i32 *)((u8 *)enemy + 0x1fc);

        for (j = 0; j < 2; j++)
        {
            if (*(i16 *)((u8 *)enemy + 0x4ca + j * 0x2a4) >= 0)
            {
                if (g_AnmManager->ExecuteScript((AnmVm *)((u8 *)enemy + 0x2b0 + j * 0x2a4)) != 0)
                {
                    *(i16 *)((u8 *)enemy + 0x4ca + j * 0x2a4) = 0xffff;
                }
            }
        }

        var14 = g_17d6ed4;

        if (((enemy->flags >> 4) & 1) == 0 && ((enemy->flags >> 5) & 1) == 0 &&
            ((enemy->flags >> 0xb) & 1) == 0 &&
            (((enemy->flags >> 0x1f) & 1) == 0 || g_17d6ed4 != 0))
        {
            if ((enemy->flags >> 2) & 1)
            {
                enemy->FUN_0042c290(&enemy->movePos, (Float3 *)((u8 *)enemy + 0x2d70));
                if (enemy->aiFlags != 0)
                {
                    hitbox = *(Float3 *)((u8 *)enemy + 0x2d70);
                    for (j = 1; j < (i16)enemy->aiParam1; j += 6)
                    {
                        if ((enemy->aiFlags & 2) != 0)
                        {
                            hitbox = *(Float3 *)((u8 *)enemy + 0x2d70) -
                                     *(Float3 *)((u8 *)enemy + 0x2d70) * (f32)j /
                                         (f32)(i16)enemy->aiParam1;
                        }
                        enemy->FUN_0042c290(&hitbox,
                                            (Float3 *)((u8 *)enemy + 0x3394 + j * 0x1c));
                    }
                }
            }

            *(i32 *)((u8 *)enemy + 0x3354) = 0;

            if ((enemy->flags >> 6) & 1)
            {
                if (((Spellcard *)&g_EclGlobalObj)->IsSpellcardActive() != 0 &&
                    enemy->HasOwnerEnemy() != 0 && g_17d6ed4 == 0)
                {
                    var18 = 0;
                }
                else
                {
                    var18 = g_Player.FUN_00451670(&enemy->movePos, (Float3 *)((u8 *)enemy + 0x2d70),
                                                  (void *)((u8 *)enemy + 0x2e10), &var14);
                }
            }

            if (enemy->moveParam1 > 0.0f)
            {
                var2c = g_Player.FUN_00451670(&enemy->movePos, (Float3 *)((u8 *)enemy + 0x2d7c),
                                              (void *)((u8 *)enemy + 0x2e10), &var14);
                if (var14 == 0)
                {
                    if (g_PlayerCharacter == 3 || g_PlayerCharacter == 0xb)
                    {
                        var18 = (i32)((f32)var18 + (f32)var2c / 6.5f);
                    }
                    else
                    {
                        var18 = (i32)((f32)var18 + (f32)var2c / 1.7f);
                    }
                }
            }

            if (var18 > 0)
            {
                if (!((enemy->flags >> 1) & 1) && g_Player.isYoukaiMode != 0)
                {
                    goto damage_score;
                }
                if (g_17d6ed4 != 0)
                {
                    goto damage_score;
                }

                if (((enemy->flags >> 1) & 1) && g_Player.isYoukaiMode == 0)
                {
                    scoreGain = (var18 / (10 - difficulty / 3)) * 10;
                }
                else
                {
                    scoreGain = (var18 / (30 - difficulty)) * 10;
                }
                if (scoreGain > 0x46)
                {
                    scoreGain = 0x46;
                }
                if (scoreGain == 0)
                {
                    if (g_Player.isYoukaiMode == 0 || !(enemy->eclTimer.AsFrames() & 1))
                    {
                        scoreGain = 0xa;
                    }
                }

            damage_score:
                if (var18 > 0x46)
                {
                    var18 = 0x46;
                }
                g_GameManager.AddScore((var18 / 5) * 10);

                if ((enemy->flags >> 3) & 1)
                {
                    if (((Spellcard *)&g_EclGlobalObj)->IsSpellcardActive() != 0)
                    {
                        if (var14 != 0)
                        {
                            if (((Spellcard *)&g_EclGlobalObj)->FUN_0042dff0() != 0 &&
                                enemy->HasOwnerEnemy() == 0)
                            {
                                if (var18 > 2)
                                {
                                    var18 = (i32)((f32)var18 / 2.5f);
                                }
                                else if (var18 != 0)
                                {
                                    var18 = 1;
                                }
                            }
                            else
                            {
                                var18 = 0;
                            }
                        }
                        else
                        {
                            if (var18 > 7)
                            {
                                var18 /= 7;
                            }
                            else if (var18 != 0)
                            {
                                var18 = 1;
                            }
                        }
                    }

                    if (enemy->aiTimer > 0)
                    {
                        if ((enemy->flags >> 1) & 1)
                        {
                            var18 /= 9;
                        }
                        else
                        {
                            var18 = 0;
                        }
                    }
                    enemy->laserActive -= var18;
                    *(i32 *)((u8 *)enemy + 0x3354) = var18;
                    enemy->FUN_0042b370(var18);
                }

                damaged = 1;
            }
        }

        if ((enemy->flags >> 1) & 1)
        {
            bossDelta = g_BossPos - *(Float3 *)&g_EclExitLeftBound;
            movePosL = enemy->movePos - *(Float3 *)&g_EclExitLeftBound;

            /* 比较方向对齐原版 0x42d3df（先求 bossDelta.x 存 -0xf0，再求 movePosL.x 作 ST0）。 */
            if (g_BossPosFlag == 0 || fabsf(bossDelta.x) > fabsf(movePosL.x))
            {
                g_BossPos = enemy->movePos;
            }
            g_BossPosFlag = 1;
        }
        /* 原版是两个独立 if（0x42d425 后 fallthrough 到 0x42d42f 检查），非 else-if。 */
        if (g_BossPosFlag == 0 && g_BossPos.y < enemy->movePos.y)
        {
            g_BossPos = enemy->movePos;
        }

        if (fabsf(enemy->movePos.x - g_EclExitLeftBound) < 64.0f && enemy->HasOwnerEnemy() == 0 &&
            (g_BossEnemy == NULL || g_BossEnemy->pos.y <= enemy->movePos.y))
        {
            g_BossEnemy = enemy;
        }

        if ((enemy->anmFlags & 8) != 0 && enemy->laserActive > 0)
        {
            enemy->anmFlags &= ~8u;
        }
        if (enemy->laserActive > 0 || (enemy->anmFlags & 8) || (enemy->anmFlags & 0x40))
        {
            goto after_bullet;
        }

    laser_or_move:
        enemy->anmFlags |= 8;
        /* ZUN bloat: 声明未使用的 Float3（原版 0x42d566 有无条件构造调用，-0x70 槽）。 */
        Float3 deadFloat70;
        *(i32 *)((u8 *)enemy + 0x53cc) = (enemy->eclDataValue0 - enemy->eclTimer.AsFrames()) / 0x3c;
        enemy->eclDataValue0 = -1;

        for (loopVar64 = 0; loopVar64 < 4; loopVar64++)
        {
            enemy->eclDataArray0[loopVar64] = -1;
        }
        for (loopVar64 = 0; loopVar64 < 4; loopVar64++)
        {
            if (enemy->dataSlots[loopVar64] != NULL)
            {
                g_ZunMemory.Free(enemy->dataSlots[loopVar64]);
                enemy->dataSlots[loopVar64] = NULL;
            }
        }

        if (enemy->HasOwnerEnemy() != 0)
        {
            enemy->ownerEnemy->subEnemyCount--;
        }

        enemy->FUN_0042adb0(1);

        if (g_Player.isYoukaiMode != 0)
        {
            g_GameManager.AddToYoukaiGauge(0xc8, 0);
        }
        else
        {
            g_GameManager.AddToYoukaiGauge(-0xc8, 0);
        }

        switch ((enemy->flags >> 0x14) & 7)
        {
        case 0:
            g_GameManager.AddScore(*(i32 *)((u8 *)enemy + 0x2e08));
            enemy->flags &= ~1u;
            if (enemy->linkedEffect != 0)
            {
                ((AnmVm *)enemy->linkedEffect)->SetInterrupt(3);
                enemy->linkedEffect = 0;
            }
            goto death_common;
        case 1:
            g_GameManager.AddScore(*(i32 *)((u8 *)enemy + 0x2e08));
            enemy->flags |= 0x800000;
            enemy->flags &= ~4u;
            enemy->flags &= ~8u;
            enemy->flags &= ~0x40u;
            goto death_common;
        case 2:
            goto death_continue;
        case 3:
            enemy->laserActive = 1;
            enemy->flags &= ~8u;
            enemy->flags &= ~0x700000u;
            g_Gui.SetBossPresent(0);
            *(u16 *)((u8 *)g_ReplayManager + 0xda) |= 0x20;
            if ((i8)enemy->byteFlag0 >= 0)
            {
                g_EffectManager.SpawnEffect(enemy->byteFlag0, &enemy->movePos, 1, -1);
                g_EffectManager.SpawnEffect(enemy->byteFlag0, &enemy->movePos, 1, -1);
                g_EffectManager.SpawnEffect(enemy->byteFlag0, &enemy->movePos, 1, -1);
            }
            if (enemy->linkedEffect != 0)
            {
                ((AnmVm *)enemy->linkedEffect)->SetInterrupt(3);
                enemy->linkedEffect = 0;
            }
            if (g_Player.playerState == 0)
            {
                g_18b89ec.SetCurrent(0x5a);
                g_Player.playerState = 3;
            }
            enemy->flags &= ~0x40000000u;
            enemy->flags &= ~0x80000000u;
            goto death_done;
        default:
            goto death_done;
        }

    death_common:
        if (enemy->flags & 2)
        {
            g_Gui.SetBossPresent(0);
            enemy->ClearEffectSlots();
        }
    death_continue:
        enemy->FUN_0042bea0(var14);

        if ((enemy->flags & 2) && ((Spellcard *)&g_EclGlobalObj)->IsSpellcardActive() == 0)
        {
            newEnemy = g_EnemyManager.RemoveEnemiesByScore(
                0x1f40, (i32)g_BulletManager.FUN_00430aa0(0x1f40, 1));
            if (newEnemy != NULL)
            {
                g_GameManager.AddScore((i32)newEnemy);
                /* 原版 0x42d961: Gui::FUN_00437ddd (0x437ddd, this=g_Gui, arg=newEnemy) thiscall。 */
                (&g_Gui)->FUN_00437ddd((i32)newEnemy);
            }
        }
        enemy->laserActive = 0;
        *(u16 *)((u8 *)g_ReplayManager + 0xda) |= 0x20;

    death_done:
        if ((enemy->flags >> 0xa) & 1)
        {
            goto after_sound;
        }

        g_SoundPlayer.PlaySoundPositionedByIdx((SoundIdx)(i % 2 + 2), enemy->movePos.x);

        if ((i8)enemy->byteFlag0 >= 0)
        {
            g_EffectManager.SpawnEffect(enemy->byteFlag0, &enemy->movePos, 1, -1);
            g_EffectManager.SpawnEffect((u8)enemy->byteFlag1 + 4, &enemy->movePos, 4, -1);
        }

        if (g_GameManager.GaugeIsExtremelyHuman() || g_GameManager.GaugeIsExtremelyYoukai())
        {
            g_ItemManager.SpawnItem(&enemy->movePos, (ItemType)7, 1);
        }

    after_sound:
        if ((i16)enemy->savedEclDataValue < 0)
        {
            goto after_bullet;
        }

        enemy->enemy_fun_00415c80();
        enemy->stackDepth = 0;
        for (loopVar78 = 0; loopVar78 < 4; loopVar78++)
        {
            enemy->eclDataArray0[loopVar78] = -1;
        }
        enemy->eclDataValue0 = -1;
        enemy->ClearDataSlots();
        memcpy((u8 *)enemy + 0x2e24, (const void *)0x57ad44, 0x84);
        enemy->rankScaledValue = 0;
        g_EclInterruptTable.SetupEclContext(&enemy->eclContext, enemy->savedEclDataValue);
        enemy->savedEclDataValue = 0xffff;

    after_bullet:
        if ((enemy->flags >> 0xb) & 1)
        {
            *(u8 *)((u8 *)enemy + 0x200) = 0xc0;
            *(u8 *)((u8 *)enemy + 0x201) = 0x20;
            *(u8 *)((u8 *)enemy + 0x202) = 0x20;
            *(u8 *)((u8 *)enemy + 0x203) = (u8)(*(i8 *)((u8 *)enemy + 0x1ff) >> 1);
            *(u32 *)((u8 *)enemy + 0x204) |= 0x20000;
        }
        else if (*(u8 *)((u8 *)enemy + 0x3314) != 0)
        {
            (*(u8 *)((u8 *)enemy + 0x3314))--;
            *(u32 *)((u8 *)enemy + 0x204) &= ~0x20000u;
        }
        else if (damaged != 0)
        {
            if (((enemy->anmFlags >> 4) & 3) < 2)
            {
                g_SoundPlayer.PlaySoundPositionedByIdx((SoundIdx)0x14, enemy->movePos.x);
            }
            else
            {
                g_SoundPlayer.PlaySoundPositionedByIdx((SoundIdx)0x25, enemy->movePos.x);
            }
            *(u8 *)((u8 *)enemy + 0x200) = 0x80;
            *(u8 *)((u8 *)enemy + 0x201) = 0x60;
            *(u8 *)((u8 *)enemy + 0x202) = 0xff;
            *(u8 *)((u8 *)enemy + 0x203) = *(u8 *)((u8 *)enemy + 0x1ff);
            *(u32 *)((u8 *)enemy + 0x204) |= 0x20000;
            *(u8 *)((u8 *)enemy + 0x3314) = 1;
        }
        else
        {
            *(u32 *)((u8 *)enemy + 0x204) &= ~0x20000u;
        }

        if (enemy->flags & 2)
        {
            Float3 markerPos;

            if (!g_Gui.IsMsgActive() && enemy->bossMarkerIdx == 0)
            {
                g_Gui.SetBossLifeBarMaxSize((f32)enemy->laserActive / enemy->laserData);
            }

            if ((enemy->flags >> 4) & 1)
            {
                markerPos.x = -1000.0f;
            }
            else
            {
                markerPos.x = enemy->movePos.x + 32.0f;
            }
            markerPos.y = 472.0f; /* 原版 0x42dd12 常量 0x43ec0000 (= 472.0f) */
            markerPos.z = 0.0f;
            g_AsciiManager.SetBossMarkerPosition(enemy->bossMarkerIdx, &markerPos);

            if (((enemy->anmFlags >> 4) & 3) == 0)
            {
                g_AsciiManager.SetBossMarkerState(enemy->bossMarkerIdx,
                                                  (*(u32 *)((u8 *)enemy + 0x204) >> 0x11) & 1);
            }
            else
            {
                g_AsciiManager.SetBossMarkerState(enemy->bossMarkerIdx,
                                                  ((enemy->anmFlags >> 4) & 3) + 1);
            }
        }

        enemy->FUN_0042e010();

        if (g_GameManager.unk2C == 0)
        {
            enemy->eclTimer.Tick();
        }
        if (enemy->aiTimer > 0)
        {
            enemy->aiTimer--;
        }
        if (((enemy->flags >> 4) & 1) == 0 && (enemy->flags & 1))
        {
            *(i32 *)enemy = *(i32 *)((u8 *)self + 0x9dcedc + enemy->enemyType * 4);
            *(i32 *)((u8 *)self + 0x9dcedc + enemy->enemyType * 4) = (i32)enemy;
        }
        continue;

    enemy_offscreen:
        enemy->flags &= ~1u;
        enemy->FUN_0042bcf0();
        continue;
    }

    if (((ZunTimer *)((u8 *)self + 0x9dced0))->operator%(0xc8) == 0 &&
        g_GameManager.IsTampered())
    {
        return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
    }
    ((ZunTimer *)((u8 *)self + 0x9dced0))->Tick();
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult EnemyManager::OnDrawHighPrio(EnemyManager *enemyManager)
{
    return OnDrawImpl(enemyManager, 0, 2);
}

// FUNCTION: th08 0x42e140
/* 栈槽位对齐原版（var_order 从 -0x4 向下；18 个函数作用域变量）。
 * 槽位证据：原版反汇编 0x42e140-0x42eb05 — savedScaleY@-0x4, savedScaleX@-0x8,
 * i@-0xc, saved1fc@-0x10, ec@-0x14, j@-0x18, enemy@-0x1c, var2@-0x20, var@-0x24,
 * count@-0x28, angle2@-0x2c, acc@-0x30, prevAngle@-0x34, ptr@-0x38, step@-0x3c,
 * angle@-0x40, cosA@-0x44, span@-0x48。块变量（PathB sinA/dataPtr）默认分配。 */
#pragma var_order(savedScaleY, savedScaleX, i, saved1fc, ec, j, enemy, var2, var, count, angle2, acc, prevAngle, ptr, step, angle, cosA, span)
// OnDrawImpl 绘制敌人。OnDrawHighPrio 遍历敌机类型链表 0..1 (arg1=0,arg2=2),
// OnDrawLowPrio 遍历 2..3 (arg1=2,arg2=4)。链表头数组在 enemyManager+0x9dcedc,
// 每节点 next 指针在 Enemy+0x0 (OnUpdate 建立)。
// 每个敌人: 先画两个 vms (ec 指针跨两段循环共享, /Od 不重置), 再设置 primaryVm
// 并进入 aiFlags 路径 (单点痕迹 PathA 或 带状激光 PathB), 最后补画 primaryVm 与 vms[1]。
ChainCallbackResult EnemyManager::OnDrawImpl(EnemyManager *enemyManager, i32 arg1, i32 arg2)
{
    f32 savedScaleY;   // -0x4  (aiFlags 块保存 primaryVm.prefix.scale.y)
    f32 savedScaleX;   // -0x8  (aiFlags 块保存 primaryVm.prefix.scale.x)
    i32 i;             // -0xc
    i32 saved1fc;      // -0x10 (aiFlags 块保存 enemy+0x1fc)
    AnmVm *ec;         // -0x14
    i32 j;             // -0x18 (vm 下标 / PathA idx / PathB idx 共用)
    Enemy *enemy;      // -0x1c
    f32 var2;          // -0x20 (PathB)
    f32 var;           // -0x24 (PathB)
    i32 count;         // -0x28 (PathB 顶点计数)
    f32 angle2;        // -0x2c (PathB; sinA 复用)
    f32 acc;           // -0x30 (PathB)
    f32 prevAngle;     // -0x34 (PathB)
    u8 *ptr;           // -0x38 (PathB)
    f32 step;          // -0x3c (PathB)
    f32 angle;         // -0x40 (PathB)
    f32 cosA;          // -0x44 (PathB)
    f32 span;          // -0x48 (PathB)

    for (i = arg1; i < arg2; i++)
    {
        enemy = *(Enemy **)((u8 *)enemyManager + 0x9dcedc + i * 4);

        while (enemy != NULL)
        {
            ec = &enemy->vms[0];

            for (j = 0; j < 1; j++, ec++)
            {
                if (ec->scriptIndex >= 0)
                {
                    if (*(i16 *)((u8 *)ec + 0x1fc) != 0)
                        ec->SetZRotation(enemy->moveAngle);
                    if (((enemy->anmFlags >> 8) & 1) == 0)
                        ec->pos = enemy->movePos + ec->pos2;
                    else
                        ec->pos = enemy->movePos + *(Float3 *)((u8 *)enemy + 0x294);
                    ec->pos.z = 0.3f;
                    ec->pos.x += g_PlayerPos.x;
                    ec->pos.y += g_PlayerPos.y;
                    g_AnmManager->Draw2D(ec);
                }
            }

            if ((enemy->flags >> 0x19) & 1)
            {
                enemy->primaryVm.SetZRotation(enemy->moveAngle);
            }

            enemy->primaryVm.pos = enemy->movePos + *(Float3 *)((u8 *)enemy + 0x294);
            enemy->primaryVm.pos.x += g_PlayerPos.x;
            enemy->primaryVm.pos.y += g_PlayerPos.y;
            enemy->primaryVm.pos.z = 0.25f;

            if (enemy->aiFlags != 0)
            {
                savedScaleX = enemy->primaryVm.prefix.scale.x;
                savedScaleY = enemy->primaryVm.prefix.scale.y;
                saved1fc = *(i32 *)((u8 *)enemy + 0x1fc);

                if ((enemy->aiFlags & 8) == 0)
                {
                    // PathA: 沿 aiParam0-1 递减 aiParam2 的单点痕迹绘制 (跳过哨兵 -990)。
                    for (j = (i16)enemy->aiParam0 - 1; j > 0; j -= (i16)enemy->aiParam2)
                    {
                        if (*(f32 *)((u8 *)enemy + 0x3394 + j * 0x1c) >= -990.0f)
                        {
                            if ((enemy->flags >> 0x19) & 1)
                            {
                                enemy->primaryVm.SetZRotation(*(f32 *)((u8 *)enemy + 0x33ac + j * 0x1c));
                            }
                            if (enemy->aiFlags & 2)
                            {
                                enemy->primaryVm.prefix.scale.x =
                                    savedScaleX - (f32)j * savedScaleX / (f32)(i16)enemy->aiParam0;
                            }
                            if (enemy->aiFlags & 4)
                            {
                                /* alpha 内联到 saved1fc 高字节（原版 42e467/42e46b 读两次 -0xd）。 */
                                *(u8 *)((u8 *)enemy + 0x1ff) =
                                    (u8)((i32)*(u8 *)((u8 *)&saved1fc + 3) -
                                         (i32)*(u8 *)((u8 *)&saved1fc + 3) * j /
                                             (i32)(i16)enemy->aiParam0);
                            }
                            enemy->primaryVm.pos =
                                *(Float3 *)((u8 *)enemy + 0x3394 + j * 0x1c) +
                                *(Float3 *)((u8 *)enemy + 0x294);
                            enemy->primaryVm.pos.z = 0.3f;
                            enemy->primaryVm.pos.x += g_PlayerPos.x;
                            enemy->primaryVm.pos.y += g_PlayerPos.y;
                            g_AnmManager->Draw2D(&enemy->primaryVm);
                        }
                    }
                }
                else
                {
                    // PathB: 累积 trail 点, 按角度线性/共线剔除后生成带状顶点 (0x1c 步进) 提交 DrawVertices。
                    /* 原版块变量不占独立槽：sinA 复用 angle2(-0x2c)、fade 复用 angle(-0x40)、
                     * dataPtr 不物化（每次读 enemy+0x230）、alpha/b 复用 saved1fc 高字节(-0xd)。 */

                    count = 0;
                    for (j = 0; j < (i16)enemy->aiParam0; j += (i16)enemy->aiParam2)
                    {
                        if (*(f32 *)((u8 *)enemy + 0x3394 + j * 0x1c) < -990.0f)
                            break;
                        count += 2;
                    }

                    if (count > 2)
                    {
                        span = *(f32 *)((u8 *)(*(i32 **)((u8 *)enemy + 0x230)) + 0x28) -
                               *(f32 *)((u8 *)(*(i32 **)((u8 *)enemy + 0x230)) + 0x20);
                        step = span / (f32)((count + 1) / 2 - 1);
                        acc = *(f32 *)((u8 *)(*(i32 **)((u8 *)enemy + 0x230)) + 0x28) +
                              enemy->primaryVm.prefix.uvScrollPos.x;
                        ptr = (u8 *)enemy + 0x3e14;

                        for (j = 0; j < (i16)enemy->aiParam0 &&
                                     *(f32 *)((u8 *)enemy + 0x3394 + j * 0x1c) >= -990.0f;
                             j += (i16)enemy->aiParam2, acc -= step)
                        {
                            if (j == 0)
                            {
                                angle = *(f32 *)((u8 *)enemy + 0x33ac);
                            }
                            else
                            {
                                angle = FUN_0042eb10(*(f32 *)((u8 *)enemy + 0x33ac + (j - 1) * 0x1c),
                                                     *(f32 *)((u8 *)enemy + 0x33ac + j * 0x1c), 0.5f);
                            }

                            if ((enemy->aiFlags & 2) && j > 0 &&
                                j + (i16)enemy->aiParam2 < (i16)enemy->aiParam0)
                            {
                                angle2 = FUN_0042eb10(*(f32 *)((u8 *)enemy + 0x33ac +
                                                               (j + (i16)enemy->aiParam2 - 1) * 0x1c),
                                                      /* 参数2 原版 0x42e6c5-0x42e6dc 不含 j（照抄 ZUN）。 */
                                                      *(f32 *)((u8 *)enemy + 0x33ac +
                                                               (i16)enemy->aiParam2 * 0x1c),
                                                      0.5f);
                                if (fabsf(prevAngle - angle) < 1.0e-5f && fabsf(angle - angle2) < 1.0e-5f)
                                {
                                    count -= 2;
                                    continue;
                                }
                            }
                            prevAngle = angle;

                            /* sinf 提前到 cosf 前（原版 0x42e754/0x42e760），
                             * 结果存 angle2 槽（-0x2c, 与剔除判断的 angle2 复用）。 */
                            angle2 = sinf(angle);
                            cosA = cosf(angle);
                            var = 0.0f;
                            var2 = savedScaleY * *(f32 *)((u8 *)(*(i32 **)((u8 *)enemy + 0x230)) + 0x30) / 2.0f;

                            if (enemy->aiFlags & 2)
                            {
                                /* fade 复用 angle 槽（-0x40, 原版 0x42e7b7 存 -0x40）。 */
                                angle = 1.0f - (f32)j / (f32)(i16)enemy->aiParam0;
                                var *= angle;
                                var2 *= angle;
                            }

                            *(i32 *)(ptr + 0x2c) = *(i32 *)((u8 *)enemy + 0x1fc);
                            *(i32 *)(ptr + 0x10) = *(i32 *)(ptr + 0x2c);
                            if (enemy->aiFlags & 4)
                            {
                                /* b 内联到 saved1fc 高字节（原版 42e7f8/42e7fc 读两次 -0xd），
                                 * 第二次写从 ptr+0x2f 读回（原版 42e81f）。 */
                                *(u8 *)(ptr + 0x2f) =
                                    (u8)((i32)*(u8 *)((u8 *)&saved1fc + 3) -
                                         (i32)*(u8 *)((u8 *)&saved1fc + 3) * j /
                                             (i32)(i16)enemy->aiParam0);
                                *(u8 *)(ptr + 0x13) = *(u8 *)(ptr + 0x2f);
                            }

                            *(Float3 *)ptr = *(Float3 *)((u8 *)enemy + 0x3394 + j * 0x1c);
                            *(f32 *)(ptr + 0x0) += cosA * var - angle2 * var2 + 32.0f;
                            *(f32 *)(ptr + 0x4) += angle2 * var + cosA * var2 + 16.0f;
                            *(f32 *)(ptr + 0x14) = acc;
                            *(f32 *)(ptr + 0x18) =
                                *(f32 *)((u8 *)(*(i32 **)((u8 *)enemy + 0x230)) + 0x24) +
                                enemy->primaryVm.prefix.uvScrollPos.y;
                            ptr += 0x1c;

                            *(Float3 *)ptr = *(Float3 *)((u8 *)enemy + 0x3394 + j * 0x1c);
                            *(f32 *)(ptr + 0x0) += cosA * var + angle2 * var2 + 32.0f;
                            *(f32 *)(ptr + 0x4) += angle2 * var - cosA * var2 + 16.0f;
                            *(f32 *)(ptr + 0x14) = acc;
                            *(f32 *)(ptr + 0x18) =
                                *(f32 *)((u8 *)(*(i32 **)((u8 *)enemy + 0x230)) + 0x2c) +
                                enemy->primaryVm.prefix.uvScrollPos.y;
                            ptr += 0x1c;
                        }

                        if (count > 2)
                        {
                            g_AnmManager->DrawVertices(&enemy->primaryVm,
                                                       (void *)((u8 *)enemy + 0x3e14), count);
                        }
                    }
                }

                enemy->primaryVm.prefix.scale.x = savedScaleX;
                enemy->primaryVm.prefix.scale.y = savedScaleY;
                *(i32 *)((u8 *)enemy + 0x1fc) = saved1fc;
            }

            if (!(enemy->aiFlags & 0x10) && !((enemy->flags >> 5) & 1))
            {
                g_AnmManager->Draw2D(&enemy->primaryVm);
            }

            for (j = 1; j < 2; j++, ec++)
            {
                if (ec->scriptIndex >= 0)
                {
                    if (*(i16 *)((u8 *)ec + 0x1fc) != 0)
                        ec->SetZRotation(-enemy->moveAngle);
                    if (((enemy->anmFlags >> 8) & 1) == 0)
                        ec->pos = enemy->movePos + ec->pos2;
                    else
                        ec->pos = enemy->movePos + *(Float3 *)((u8 *)enemy + 0x294);
                    ec->pos.z = 0.3f;
                    ec->pos.x += g_PlayerPos.x;
                    ec->pos.y += g_PlayerPos.y;
                    g_AnmManager->Draw2D(ec);
                }
            }

            enemy = *(Enemy **)enemy;
        }
    }

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
        if ((g_PlayerFlags >> 0xe) & 1)
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
        if ((g_PlayerFlags >> 0xe) & 1)
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

// FUNCTION: th08 0x42ee80
/* 栈槽位对齐原版：i@-0x4、enemy@-0x8、markerPos x@-0x14（原版 42ee91/42ee94/42ef13；
 * this 副本 -0x18 编译器生成，sub esp 0x18）。markerPos 是外层变量（1298 行），
 * 不列入 var_order 会默认占位把 enemy 推后（实测 enemy@-0x14）。 */
#pragma var_order(i, enemy, markerPos)
ZunResult EnemyManager::DeletedCallback(EnemyManager *enemyManager)
{
    i32 i;
    Enemy *enemy;

    enemy = &enemyManager->enemies[0];
    for (i = 0; i < 0x1e0; i++, enemy++)
    {
        enemy->ClearDataSlots();
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
        g_Spellcard.dataHolder.FreeData();
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

Enemy *EnemyManager::SpawnEnemy2(i32 eclSubId, Float3 *pos, i32 life, i32 itemDrop, i32 score,
                                 EclContextArgs *args)
{
    return NULL;
}

Enemy *EnemyManager::RemoveEnemiesByScore(i32 a0, i32 a1)
{
    return NULL;
}

void Enemy::ClearDataSlots()
{
    i32 i;

    for (i = 0; i < 4; i++)
    {
        if (this->dataSlots[i] != NULL)
        {
            g_ZunMemory.RemoveFromRegistry(this->dataSlots[i]);
            this->dataSlots[i] = NULL;
        }
    }
}

// TODO: move to a dedicated EffectManager.cpp once the unit is added to the build.
DIFFABLE_STATIC(EffectManager, g_EffectManager);
DIFFABLE_STATIC(EffectTemplate, g_EffectTemplates[0x100]);
DIFFABLE_STATIC(ChainElem, g_EffectManagerCalcChain);
DIFFABLE_STATIC(ChainElem, g_EffectManagerDrawChain);

// STUB: th08 0x4281e0
void EffectManager::DrawParticles()
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
AnmVm *EffectManager::SpawnEffect(i32 a, Float3 *pos, i32 b, i32 c)
{
    EffectManagerParticle *p = &this->particles[this->nextSlotIdx];
    AnmVm *result;
    i32 i;

    for (i = 0; i < 0x200; i++)
    {
        this->nextSlotIdx = this->nextSlotIdx + 1;
        if (this->nextSlotIdx >= 0x200)
        {
            this->nextSlotIdx = 0;
        }

        if (p->alive != 0)
        {
            if (this->nextSlotIdx == 0)
            {
                p = &this->particles[0];
            }
            else
            {
                p = p + 1;
            }
            continue;
        }

        if (p->dataPtr != NULL)
        {
            g_ZunMemory.Free(p->dataPtr);
        }

        memset(p, 0, sizeof(EffectManagerParticle));

        p->alive = 1;
        p->type = (i8)a;
        p->spawnPos = *pos;
        this->effectAnm->SetAndExecuteScriptIdx((AnmVm *)p, g_EffectTemplates[a].field0);

        p->flags |= 0x2000;
        p->spawnParam = c;
        p->drawOffsetX = 0;
        p->drawOffsetY = 0;
        p->drawOffsetZ = 0;
        p->updateFn = g_EffectTemplates[a].field4;

        if (g_EffectTemplates[a].field8 != NULL)
        {
            if (g_EffectTemplates[a].field8(p) != 0)
            {
                p->alive = 0;
            }
        }

        b--;
        if (b == 0)
        {
            break;
        }

        if (this->nextSlotIdx == 0)
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
AnmVm *EffectManager::SpawnEffectAtSlot(i32 a, Float3 *pos, i32 b, i32 c, i32 d)
{
    EffectManagerParticle *p = &this->particles[b + 0x280];

    if (p->dataPtr != NULL)
    {
        g_ZunMemory.Free(p->dataPtr);
    }

    memset(p, 0, sizeof(EffectManagerParticle));

    p->slotIdx = b;
    p->alive = 1;
    p->type = (i8)a;
    p->spawnPos = *pos;

    if (g_EffectTemplates[a].field0 >= 0)
    {
        this->effectAnm->SetAndExecuteScriptIdx((AnmVm *)p, g_EffectTemplates[a].field0);
    }

    p->flags |= 0x2000;
    p->spawnParam = d;
    p->drawOffsetX = 0;
    p->drawOffsetY = 0;
    p->drawOffsetZ = 0;
    p->updateFn = g_EffectTemplates[a].field4;

    if (g_EffectTemplates[a].field8 != NULL)
    {
        if (g_EffectTemplates[a].field8(p) != 0)
        {
            p->alive = 0;
        }
    }

    g_ReplayManager->replayEventFlags |= 0x400;

    return (AnmVm *)p;
}

void *EffectManager::AllocEffectSlot(i32 type, Float3 *pos, i32 b, i32 c)
{
    return NULL;
}

void EffectManager::SpawnEffectLocal(i32 a0, Float3 *pos, Float3 *localPos, i32 a1, i32 a2)
{
}

void EffectManager::CutChain()
{
    g_Chain.Cut(&g_EffectManagerCalcChain);
    g_Chain.Cut(&g_EffectManagerDrawChain);
}

// 子敌人链 helper (GetVarValue/GetEclFloatVar varId 0x2770 调用; call 目标归一化为 T)
// FUNCTION: th08 0x41f000
// True when this enemy is the root of a sub-enemy chain
// (ownerEnemy == 0 && nextSubEnemy != 0).
i32 Enemy::IsSubEnemyChainRoot()
{
    return 0;
}

// FUNCTION: th08 0x41fd20
// True when this enemy belongs to an owner (ownerEnemy != 0).
i32 Enemy::HasOwnerEnemy()
{
    return 0;
}

// FUNCTION: th08 0x41fd40
// Returns the number of enemies in this enemy's sub-enemy chain.
i32 Enemy::GetSubEnemyChainCount()
{
    return 0;
}

} /* namespace th08 */

// STUB: th08 0x42eb10
// 角度插值: 在 a→b 间按 c 线性插值 (处理角度环绕)。OnDrawImpl PathB 使用。
f32 FUN_0042eb10(f32 a, f32 b, f32 c)
{
    (void)a;
    (void)b;
    (void)c;
    return 0.0f;
}
