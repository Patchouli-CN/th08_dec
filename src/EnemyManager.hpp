#pragma once
#include "Global.hpp"
#include "ZunResult.hpp"
#include "diffbuild.hpp"

namespace th08
{

struct Enemy
{
    void FUN_0042bc90();

    unknown_fields(0x0, 0x3384);
    void *dataPtrs[4];
    unknown_fields(0x3394, 0x203c);
};
C_ASSERT(sizeof(Enemy) == 0x53d0);

struct EnemyManager
{
    void Initialize();
    static ZunResult RegisterChain();
    static ChainCallbackResult OnUpdate();
    static ChainCallbackResult OnDrawHighPrio(EnemyManager *enemyManager);
    static ChainCallbackResult OnDrawImpl(EnemyManager *enemyManager, i32 arg1, i32 arg2);
    static ChainCallbackResult OnDrawLowPrio(EnemyManager *enemyManager);
    static ZunResult AddedCallback(EnemyManager *enemyManager);
    static ZunResult DeletedCallback(EnemyManager *enemyManager);
    static void CutChain();

    unknown_fields(0x0, 0x53d0);
    Enemy enemies[0x1e0];
};

DIFFABLE_EXTERN(EnemyManager, g_EnemyManager);

} /* namespace th08 */