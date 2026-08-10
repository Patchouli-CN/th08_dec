#pragma once

#include "AnmManager.hpp"
#include "Global.hpp"

namespace th08
{

struct BulletTypeSprites
{
};

struct Laser
{
};

struct Bullet
{
};

struct BulletManager
{
    void Initialize();
    void RemoveAllBullets(i32);
    void bulletmanager_fun_00415c60();

    unknown_fields(0x0, 0x6ba550);
    char *etamaAnmPath;

    static ZunResult RegisterChain(char *path);
    static ChainCallbackResult OnUpdate(BulletManager *bulletManager);
    static ChainCallbackResult OnDraw(BulletManager *bulletManager);
    static ZunResult AddedCallback(BulletManager *bulletManager);
    static ZunResult DeletedCallback(BulletManager *bulletManager);
    static void CutChain();
};

DIFFABLE_EXTERN(BulletManager, g_BulletManager);
DIFFABLE_EXTERN(AnmLoaded *, g_BulletAnm);

} /* namespace th08 */