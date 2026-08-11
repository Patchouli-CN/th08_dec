#pragma once
#include "AnmManager.hpp"
#include "EclManager.hpp"
#include "Global.hpp"
#include "ZunResult.hpp"
#include "diffbuild.hpp"

namespace th08
{

// The 0x200-byte shot-pattern descriptor at Enemy+0x3070, filled by op114-115
// and handed to BulletManager::AllocShotSlot to claim an EnemySubData slot.
struct EnemyShotData
{
    i16 subId;                    // 0x0  (0x3070)
    i16 anmIdx;                   // 0x2  (0x3072)
    Float3 pos;                   // 0x4  (0x3074)
    f32 unk10;                    // 0x10 (0x3080)
    unknown_fields(0x14, 0x4);    // 0x14-0x18
    f32 unk18;                    // 0x18 (0x3088)
    unknown_fields(0x1c, 0x1b4);  // 0x1c-0x1d0
    f32 unk1d0;                   // 0x1d0
    f32 unk1d4;                   // 0x1d4
    f32 unk1d8;                   // 0x1d8
    f32 unk1dc;                   // 0x1dc
    i32 unk1e0;                   // 0x1e0
    i32 unk1e4;                   // 0x1e4
    i32 unk1e8;                   // 0x1e8
    i32 unk1ec;                   // 0x1ec
    i32 unk1f0;                   // 0x1f0
    unknown_fields(0x1f4, 0x4);   // 0x1f4-0x1f8
    i16 unk1f8;                   // 0x1f8  (0 if subId==0x73 else 1)
    unknown_fields(0x1fa, 0x2);   // 0x1fa-0x1fc
    i32 unk1fc;                   // 0x1fc
};
C_ASSERT(sizeof(EnemyShotData) == 0x200);

// A 0x18-byte laser-pattern entry in Enemy's 0x2e44 area (op111).
struct EnemyLaserData
{
    f32 x;    // 0x0
    f32 y;    // 0x4
    i32 a;    // 0x8
    i32 b;    // 0xc
    i32 c;    // 0x10
    i32 d;    // 0x14
};
C_ASSERT(sizeof(EnemyLaserData) == 0x18);

// The per-slot ECL data structure each Enemy::unk3280[] pointer points to.
// A slot holds one shot-pattern/effect descriptor that RunEcl opcodes read and
// mutate (op117-121, op166-172). The base is allocated by op114 (0x430f20).
struct EnemySubData
{
    unknown_fields(0x0, 0x548);
    Float3 pos;              // 0x548  world position of the pattern
    f32 angle;               // 0x554
    f32 unk558;              // 0x558
    f32 unk55c;              // 0x55c
    f32 unk560;              // 0x560
    f32 unk564;              // 0x564
    f32 unk568;              // 0x568
    unknown_fields(0x56c, 0x18);
    u32 isActive;            // 0x584  0 => slot empty (checked by op120/121 & AllocShotSlot)
    ZunTimer timer;          // 0x588 (0xc)
    unknown_fields(0x594, 0x4);
    u8 runState;             // 0x598  0/1 => can run, 2 => running (op121)
    u8 unk599;               // 0x599
    unknown_fields(0x59a, 0x6);
};
C_ASSERT(sizeof(EnemySubData) == 0x5a0);

struct Enemy
{
    void FUN_0042bc90();
    void FUN_00422c40();
    void FUN_00423150();
    void FUN_00421de0(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5); // ECL sub-call (0x421de0)
    void FUN_0042c180(); // move init after SET_POS (0x42c180)
    void ClearEffectSlots(); // 0x42a820 (op127 boss-marker setup)
    f32 GetEclFloatVar(i32 varId); // ECL var helper (th08 0x420120), thiscall style

    unknown_fields(0x0, 0x4);
    i32 unk4;                          // 0x4   (prev in the sub-enemy chain)
    i32 unk8;                          // 0x8   (next in the sub-enemy chain)
    AnmVm primaryVm;                   // 0xc   (0x2a4)
    AnmVm vms[2];                      // 0x2b0  (2 * 0x2a4)
    EclContext eclContext;             // 0x7f8  current ECL execution context
    EclContext savedContextStack[16];  // 0xa20  (16 * 0x228 = 0x2280)
    EclContext *curContextPtr;         // 0x2ca0 pointer to the active EclContext
    EclContext *savedStackPtr;         // 0x2ca4 pointer to savedContextStack[0]
    unknown_fields(0x2ca8, 0x40);
    i16 unk2ce8;                       // 0x2ce8
    i16 stackDepth;                    // 0x2cea  saved-context stack depth
    unknown_fields(0x2cec, 0x2);
    i16 unk2cee;                       // 0x2cee
    i16 interrupts[32];                // 0x2cf0  (indexed by eclState)
    i16 runInterrupt;                  // 0x2d30  >= 0 forces the interrupt path
    unknown_fields(0x2d32, 0x2);
    Float3 pos;                        // 0x2d34
    Float3 unk2d40;                    // 0x2d40  per-frame movement vector
    f32 unk2d4c;                       // 0x2d4c  (dx from laser interp)
    f32 unk2d50;                       // 0x2d50  (dy)
    unknown_fields(0x2d54, 0x1c);      // 0x2d54-0x2d70
    f32 unk2d70;                       // 0x2d70
    f32 unk2d74;                       // 0x2d74
    unknown_fields(0x2d78, 0x4);       // 0x2d78-0x2d7c
    f32 unk2d7c;                       // 0x2d7c
    f32 unk2d80;                       // 0x2d80
    unknown_fields(0x2d84, 0x4);       // 0x2d84-0x2d88
    Float3 unk2d88;                    // 0x2d88  pos + unk2d40 (computed each ECL frame)
    f32 unk2d94;                       // 0x2d94  (angle from dx/dy)
    f32 unk2d98;                       // 0x2d98  (move speed)
    f32 unk2d9c;                       // 0x2d9c
    f32 unk2da0;                       // 0x2da0
    Enemy *unk2da4;                    // 0x2da4  (owner enemy, set by bullet spawn)
    f32 unk2da8;                       // 0x2da8  (move angle)
    f32 unk2dac;                       // 0x2dac
    f32 unk2db0;                       // 0x2db0
    f32 unk2db4;                       // 0x2db4
    f32 unk2db8;                       // 0x2db8
    f32 unk2dbc;                       // 0x2dbc
    f32 unk2dc0;                       // 0x2dc0
    unknown_fields(0x2dc4, 0xc);       // 0x2dc4-0x2dd0
    f32 unk2dd0;                       // 0x2dd0
    f32 unk2dd4;                       // 0x2dd4
    unknown_fields(0x2dd8, 0x4);       // 0x2dd8-0x2ddc
    ZunTimer unk2ddc;                  // 0x2ddc
    i32 unk2de8;                       // 0x2de8
    f32 unk2dec;                       // 0x2dec
    f32 unk2df0;                       // 0x2df0
    i16 unk2df4;                       // 0x2df4
    i16 unk2df6;                       // 0x2df6
    i16 unk2df8;                       // 0x2df8
    i16 unk2dfa;                       // 0x2dfa
    i32 unk2dfc;                       // 0x2dfc  laser-in-use flag
    i32 unk2e00;                       // 0x2e00  (laser-related, divide base)
    i32 unk2e04;                       // 0x2e04  (laser-related)
    unknown_fields(0x2e08, 0x4);       // 0x2e08-0x2e0c
    i32 unk2e0c;                       // 0x2e0c
    unknown_fields(0x2e10, 0x4);       // 0x2e10-0x2e14
    ZunTimer unk2e14;                  // 0x2e14 (0xc bytes)
    unknown_fields(0x2e20, 0x200);     // 0x2e20-0x3020
    i32 unk3020;                       // 0x3020  (boss spellcard-related)
    i32 unk3024;                       // 0x3024  (set by SET_LIFE_CALLBACK_THRESHOLD)
    i32 unk3028;                       // 0x3028  (set by SET_LIFE_CALLBACK_SUB)
    unknown_fields(0x302c, 0x8);       // 0x302c-0x3034
    u8 unk3034[0x2c];                  // 0x3034 (0x2c bytes)
    i32 unk3060;                       // 0x3060  (movement/scaling base)
    ZunTimer unk3064;                  // 0x3064 (0xc bytes)
    EnemyShotData shotData;            // 0x3070 (0x200 bytes, op114-115 fill it)
    unknown_fields(0x3270, 0x10);      // 0x3270-0x3280
    EnemySubData *unk3280[0x20];       // 0x3280 (0x20 shot-pattern slots)
    i32 unk3300;                       // 0x3300
    i32 unk3304;                       // 0x3304
    i32 unk3308;                       // 0x3308
    i32 unk330c;                       // 0x330c
    u8 unk3310;                        // 0x3310
    u8 unk3311;                        // 0x3311
    u8 unk3312;                        // 0x3312
    u8 unk3313;                        // 0x3313
    unknown_fields(0x3314, 0x10);      // 0x3314-0x3324
    u32 unk3324;                       // 0x3324  bit26 = don't save context on interrupt
    u32 unk3328;                       // 0x3328  (anm script flags, bit2 cleared by SET_ANM)
    unknown_fields(0x332c, 0x3);       // 0x332c-0x332f
    u8 unk332f;                        // 0x332f
    u8 eclFlags;                       // 0x3330
    unknown_fields(0x3331, 0xb);       // 0x3331-0x333c
    i16 unk333c;                       // 0x333c  current animation id
    unknown_fields(0x333e, 0x2);       // 0x333e-0x3340
    f32 unk3340;                       // 0x3340
    f32 unk3344;                       // 0x3344
    f32 unk3348;                       // 0x3348
    f32 unk334c;                       // 0x334c
    f32 unk3350;                       // 0x3350
    unknown_fields(0x3354, 0x4);       // 0x3354-0x3358
    i32 unk3358[4];                    // 0x3358  (indexed by ECL arg; boss data / effect slots)
    i32 unk3368[4];                    // 0x3368  (same indexing; set with unk3358)
    i32 unk3378;                       // 0x3378  (initialized to -1)
    i32 unk337c;                       // 0x337c
    i32 unk3380;                       // 0x3380  (sub-enemy chain count)
    void *dataPtrs[4];                 // 0x3384
    unknown_fields(0x3394, 0x1fb8);    // 0x3394-0x534c
    u8 unk534c;                        // 0x534c  (tutorial/boss-ai flags, bit3 gate)
    i16 unk534e;                       // 0x534e
    i16 unk5350;                       // 0x5350
    i16 unk5352;                       // 0x5352
    ZunTimer unk5354;                  // 0x5354 (0xc bytes)
    void *unk5360[24];                 // 0x5360 (24 effect-slot pointers)
    i32 unk53c0;                       // 0x53c0  (effect-slot counter)
    i32 unk53c4;                       // 0x53c4
    u32 unk53c8;                       // 0x53c8  (pointer to linked enemy/struct)
    unknown_fields(0x53cc, 0x4);       // 0x53cc-0x53d0
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
    Enemy *SpawnEnemy2(i32 eclSubId, Float3 *pos, i32 life, i32 itemDrop, i32 score,
                       EclContextArgs *args); // 0x42a680 (th07 SpawnEnemyEx equivalent)
    void RemoveEnemiesByScore(i32 a0, i32 a1);       // 0x42efb0 (遍历删敌人)

    unknown_fields(0x0, 0x53d0);
    Enemy enemies[0x1e0];
};

DIFFABLE_EXTERN(EnemyManager, g_EnemyManager);
DIFFABLE_EXTERN(AnmLoaded, g_EnemyAnmLoaded); // 0xf54e0c (enemy sprite animations)
DIFFABLE_EXTERN(AnmLoaded, g_EnemyAnmLoaded2); // 0xf54e10

} /* namespace th08 */