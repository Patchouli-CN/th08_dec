#include "th_pch.h"

#include "BulletManager.hpp"
#include "ItemManager.hpp"
#include "Player.hpp"
#include "Supervisor.hpp"

#include <math.h>

namespace th08
{

// STUB: th08 0x433880
void __fastcall FUN_00433880(f32 *sinOut, f32 *cosOut, f32 angle)
{
    *cosOut = cos(angle);
    *sinOut = sin(angle);
}

DIFFABLE_STATIC(BulletManager, g_BulletManager);
DIFFABLE_STATIC(ChainElem, g_BulletManagerCalcChain);
DIFFABLE_STATIC(ChainElem, g_BulletManagerDrawChain);
DIFFABLE_STATIC(AnmLoaded *, g_BulletAnm);

DIFFABLE_STATIC_ARRAY(u8, 0x600 * 0x10b8, g_BulletPool);

// FUNCTION: th08 0x42f360
void BulletManager::Initialize()
{
    memset(this, 0, 0x6ba578);

    this->unk_6ba56c = (i32)((u8 *)this + 0x1a880);
    this->unk_660638 = 6;
    this->unk_6ba570 = 6;

    u8 *pool = (u8 *)g_BulletPool;
    for (i32 i = 0; i < 0x600; i++, pool += 0x10b8)
    {
        *(u16 *)(pool + 0x21a) = 0xffff;
        *(u16 *)(pool + 0xcaa) = 0xffff;
        *(u16 *)(pool + 0x4be) = 0xffff;
        *(u16 *)(pool + 0x762) = 0xffff;
        *(u16 *)(pool + 0xa06) = 0xffff;
    }
}

// FUNCTION: th08 0x430830 (99% FIXME: 浮点 <= 的 jp+jmp vs jne)
void BulletManager::RemoveAllBullets(i32 param)
{
    u8 *pool = (u8 *)g_BulletPool;
    u8 *b;
    i32 i;
    i32 r1;

    for (i = 0; i < 0x600; i++, pool += 0x10b8)
    {
        if (*(u16 *)(pool + 0xdb8) == 0 || *(u16 *)(pool + 0xdb8) == 5)
        {
            continue;
        }

        r1 = g_Player.FUN_00449ff0((Float3 *)(pool + 0xd44), pool + 0xd34);

        if (g_Player.FUN_00449ff0((Float3 *)(pool + 0xd44), pool + 0xd34) == 2)
        {
            g_ItemManager.SpawnItem((Float3 *)(pool + 0xd44), (ItemType) * (i32 *)0x18b8988, 1);
            memset(pool, 0, 0x10b8);
            continue;
        }

        if (param != 4)
        {
            g_ItemManager.SpawnItem((Float3 *)(pool + 0xd44), (ItemType)this->unk_6ba570, (ItemType)param);
            memset(pool, 0, 0x10b8);
        }
        else
        {
            *(u16 *)(pool + 0xdb8) = 5;
        }
    }

    b = (u8 *)this + 0x660938;

    Float3 pos;
    f32 sinX;
    f32 cosX;
    f32 speed;

    for (i = 0; i < 0x100; i++, b += 0x59c)
    {
        if (*(u32 *)(b + 0x584) == 0)
        {
            continue;
        }

        if (*(u16 *)(b + 0x594) & 0x4 && param != 4)
        {
            continue;
        }

        if (*(u8 *)(b + 0x598) < 2)
        {
            *(u8 *)(b + 0x598) = 2;
            ((ZunTimer *)(b + 0x588))->SetCurrent(0);
            *(u32 *)(b + 0x564) = *(u32 *)(b + 0x568);

            if (param != 4)
            {
                speed = *(f32 *)(b + 0x558);

                for (;;)
                {
                    FUN_00433880(&sinX, &cosX, *(f32 *)(b + 0x554));

                    if (*(f32 *)(b + 0x55c) <= speed)
                    {
                        break;
                    }

                    pos.x = cosX * speed + *(f32 *)(b + 0x548);
                    pos.y = sinX * speed + *(f32 *)(b + 0x54c);
                    pos.z = 0;
                    g_ItemManager.SpawnItem(&pos, (ItemType)this->unk_6ba570, (ItemType)param);
                    speed += *(f32 *)0x4b42cc;
                }
            }
        }

        *(u32 *)(b + 0x580) = 0;
    }

    *(i32 *)((u8 *)this + 0x6ba53c) = 0xa;
}

void BulletManager::bulletmanager_fun_00415c60()
{
    this->RemoveAllBullets(1);
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
