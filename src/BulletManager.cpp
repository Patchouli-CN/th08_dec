#include "th_pch.h"

#include "BulletManager.hpp"
#include "Supervisor.hpp"

namespace th08
{

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

// STUB: th08 0x430830
void BulletManager::RemoveAllBullets(i32)
{
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
