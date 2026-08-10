#include "th_pch.h"

#include "BulletManager.hpp"
#include "Supervisor.hpp"

namespace th08
{

DIFFABLE_STATIC(BulletManager, g_BulletManager);
DIFFABLE_STATIC(ChainElem, g_BulletManagerCalcChain);
DIFFABLE_STATIC(ChainElem, g_BulletManagerDrawChain);
DIFFABLE_STATIC(AnmLoaded *, g_BulletAnm);

// STUB: th08 0x42f360
void BulletManager::Initialize()
{
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
