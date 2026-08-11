#pragma once
#include "EclManager.hpp"
#include "Global.hpp"
#include "ZunResult.hpp"
#include "diffbuild.hpp"

namespace th08
{

struct Enemy
{
    void FUN_0042bc90();

    unknown_fields(0x0, 0x7f8);
    EclContext eclContext;             // 0x7f8  current ECL execution context
    EclContext savedContextStack[16];  // 0xa20  (16 * 0x228 = 0x2280)
    EclContext *curContextPtr;         // 0x2ca0 pointer to the active EclContext
    EclContext *savedStackPtr;         // 0x2ca4 pointer to savedContextStack[0]
    unknown_fields(0x2ca8, 0x40);
    i16 unk2ce8;                       // 0x2ce8
    i16 stackDepth;                    // 0x2cea  saved-context stack depth
    unknown_fields(0x2cec, 0x4);
    i16 interrupts[32];                // 0x2cf0  (indexed by eclState)
    i16 runInterrupt;                  // 0x2d30  >= 0 forces the interrupt path
    unknown_fields(0x2d32, 0x2);
    Float3 pos;                        // 0x2d34
    unknown_fields(0x2d40, 0x54);      // 0x2d40-0x2d94 (incl. a Float3 copied to +0x2d88)
    unknown_fields(0x2d94, 0x59c);     // 0x2d94-0x3330
    u8 eclFlags;                       // 0x3330
    unknown_fields(0x3331, 0x53);      // 0x3331-0x3384
    void *dataPtrs[4];                 // 0x3384
    unknown_fields(0x3394, 0x203c);    // 0x3394-0x53d0
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