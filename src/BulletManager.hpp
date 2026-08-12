#pragma once

#include "AnmManager.hpp"
#include "Global.hpp"

namespace th08
{

struct EnemySubData; // forward decl (defined in EnemyManager.hpp)
struct EnemyShotData; // forward decl (defined in EnemyManager.hpp)

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
    void ClearBulletsInRadius(Float3 *pos, f32 a1); // 0x430d30 (清除 pos 半径内敌弹并掉落物品)
    void SetupLaserMove(Float3 *pos);          // 0x430e10 (ECL laser/move setup)
    EnemySubData *AllocShotSlot(EnemyShotData *src); // 0x430f20 (claim a data slot)

    unknown_fields(0x0, 0x1a880);
    u8 unk_1a880[0x4a5b8];       // 0x1a880
    u16 unk_660638;              // 0x660638
    unknown_fields(0x66063a, 0x45116);  // 0x66063a
    char *etamaAnmPath;          // 0x6ba550
    unknown_fields(0x6ba554, 0x18);     // 0x6ba554
    i32 unk_6ba56c;              // 0x6ba56c
    i32 unk_6ba570;              // 0x6ba570
    unknown_fields(0x6ba574, 0x4);      // 0x6ba574

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