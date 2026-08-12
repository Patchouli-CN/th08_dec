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
    i16 subId;                    // 0x0  (0x3070)  shot sub-id (ECL arg0)
    i16 anmIdx;                   // 0x2  (0x3072)  bullet animation index (ECL arg1)
    Float3 pos;                   // 0x4  (0x3074)  world position (movePos + moveVec2)
    f32 angle;                    // 0x10 (0x3080)  shot pattern angle (ECL arg2; -> EnemySubData.angle)
    unknown_fields(0x14, 0x4);    // 0x14-0x18
    f32 speedStep;                // 0x18 (0x3088)  per-frame speed ramp step (ECL arg3; -> slot+0x56c)
    unknown_fields(0x1c, 0x1b4);  // 0x1c-0x1d0
    f32 speed;                    // 0x1d0  initial bullet speed (ECL arg4; -> slot+0x558)
    f32 targetSpeed;              // 0x1d4  speed ramp target (ECL arg5; -> slot+0x55c)
    f32 speedLimit;               // 0x1d8  speed adjustment threshold (ECL arg6; -> slot+0x560)
    f32 rampTime;                 // 0x1dc  ramp/progress time (ECL arg7; -> slot+0x564)
    i32 delay;                    // 0x1e0  frames before pattern runs (ECL arg8; -> slot+0x570, gates runState)
    i32 param1;                   // 0x1e4  (ECL arg9; -> slot+0x578)
    i32 param2;                   // 0x1e8  (ECL arg10; -> slot+0x57c)
    i32 param3;                   // 0x1ec  (ECL arg11; -> slot+0x574)
    i32 param4;                   // 0x1f0  (ECL arg12; -> slot+0x580)
    unknown_fields(0x1f4, 0x4);   // 0x1f4-0x1f8
    i16 shotType;                 // 0x1f8  0 = laser (subId==0x73), 1 = normal bullet (ECL-set by op114)
    unknown_fields(0x1fa, 0x2);   // 0x1fa-0x1fc
    i32 flags;                    // 0x1fc  shot pattern flags (ECL arg13; bit2 checked by AllocShotSlot; -> slot+0x594)
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

// The per-slot ECL data structure each Enemy::shotSlots[] pointer points to.
// A slot holds one shot-pattern/effect descriptor that RunEcl opcodes read and
// mutate (op117-121, op166-172). The base is allocated by op114 (0x430f20).
struct EnemySubData
{
    unknown_fields(0x0, 0x548);
    Float3 pos;              // 0x548  world position of the pattern
    f32 angle;               // 0x554  current shot angle
    f32 speed;               // 0x558  current bullet speed (set by op172, filled from shotData.speed)
    f32 targetSpeed;         // 0x55c  speed ramp target, incremented by slot+0x56c each frame (op172)
    f32 speedLimit;          // 0x560  speed adjustment threshold (op171)
    f32 phaseTimer;          // 0x564  phase progress (used /200 in the shot update; reset from savedPhaseTimer on op121)
    f32 savedPhaseTimer;     // 0x568  saved phase value restored into phaseTimer by op121
    unknown_fields(0x56c, 0x18);
    u32 isActive;            // 0x584  0 => slot empty (checked by op120/121 & AllocShotSlot)
    ZunTimer timer;          // 0x588 (0xc)
    unknown_fields(0x594, 0x4);
    u8 runState;             // 0x598  0/1 => can run, 2 => running (op121)
    u8 shotMode;             // 0x599  shot behavior byte (set by op170)
    unknown_fields(0x59a, 0x6);
};
C_ASSERT(sizeof(EnemySubData) == 0x5a0);

struct Enemy
{
    void ClearDataSlots();    // 0x42bc90 (遍历 dataSlots 释放)
    void UpdateEnemyMove();   // 0x422c40 (逐帧按移动模式更新位置)
    void UpdateLaserScript(); // 0x423150 (运行缓冲的激光子脚本)
    void EclSubCall(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5); // ECL sub-call (0x421de0)
    void InitMoveAfterSetPos(); // move init after SET_POS (0x42c180)
    void ClearEffectSlots(); // 0x42a820 (op127 boss-marker setup)
    f32 GetEclFloatVar(i32 varId); // ECL var helper (th08 0x420120), thiscall style

    unknown_fields(0x0, 0x4);
    i32 prevSubEnemy;                  // 0x4   (prev in the sub-enemy chain)
    i32 nextSubEnemy;                  // 0x8   (next in the sub-enemy chain)
    AnmVm primaryVm;                   // 0xc   (0x2a4)
    AnmVm vms[2];                      // 0x2b0  (2 * 0x2a4)
    EclContext eclContext;             // 0x7f8  current ECL execution context
    EclContext savedContextStack[16];  // 0xa20  (16 * 0x228 = 0x2280)
    EclContext *curContextPtr;         // 0x2ca0 pointer to the active EclContext
    EclContext *savedStackPtr;         // 0x2ca4 pointer to savedContextStack[0]
    unknown_fields(0x2ca8, 0x40);
    i16 savedStackDepth;               // 0x2ce8  stackDepth persisted across RunEcl calls (exit restores it)
    i16 stackDepth;                    // 0x2cea  saved-context stack depth
    unknown_fields(0x2cec, 0x2);
    i16 savedEclDataValue;             // 0x2cee  value stored by op130, copied to eclDataValue1 by op153
    i16 interrupts[32];                // 0x2cf0  (indexed by eclState)
    i16 runInterrupt;                  // 0x2d30  >= 0 forces the interrupt path
    unknown_fields(0x2d32, 0x2);
    Float3 pos;                        // 0x2d34
    Float3 moveVec;                    // 0x2d40  per-frame movement vector
    f32 moveDeltaX;                    // 0x2d4c  (dx of the per-frame move delta; Float3 at 0x2d4c)
    f32 moveDeltaY;                    // 0x2d50  (dy)
    unknown_fields(0x2d54, 0x1c);      // 0x2d54-0x2d70
    f32 hitboxSizeX;                   // 0x2d70  (Initialize: Float3(24,24,24) 尺寸; op77 sets x/y)
    f32 hitboxSizeY;                   // 0x2d74
    unknown_fields(0x2d78, 0x4);       // 0x2d78-0x2d7c
    f32 moveParam1;                    // 0x2d7c  (movement param, op78)
    f32 moveParam2;                    // 0x2d80  (movement param, op78)
    unknown_fields(0x2d84, 0x4);       // 0x2d84-0x2d88
    Float3 movePos;                    // 0x2d88  pos + moveVec (computed each ECL frame)
    f32 moveAngle;                       // 0x2d94  (angle from dx/dy)
    f32 moveSpeed;                       // 0x2d98  (move speed)
    f32 moveInterp3;                   // 0x2d9c  SET_MOVE_INTERP arg3 (mode3 螺旋角累计)
    f32 moveInterp4;                   // 0x2da0  SET_MOVE_INTERP arg4 (mode3 角速度)
    Enemy *ownerEnemy;                    // 0x2da4  (owner enemy, set by bullet spawn)
    f32 moveRadius;                    // 0x2da8  (angle-mode polar radius/速度; op64-68 与 moveAngle 并列设置, 每帧 += moveRadiusStep)
    f32 moveRadiusStep;                // 0x2dac  (per-frame radial step for moveRadius; op71)
    f32 moveInterp5;                   // 0x2db0  SET_MOVE_INTERP arg5 (mode3 螺旋半径)
    f32 moveInterp6;                   // 0x2db4  SET_MOVE_INTERP arg6 (mode3 径向速度)
    Float3 moveVec2;                    // 0x2db8  (movement vector, op110 sets it)
    Float3 moveVelVec;                  // 0x2dc4  (角度→速度向量; SetMoveVelocity op66/69 写入, 逐帧累加到 movePos)
    f32 moveInterp1;                   // 0x2dd0  SET_MOVE_INTERP arg1 (mode3 螺旋中心 x / mode2 插值起点)
    f32 moveInterp2;                   // 0x2dd4  SET_MOVE_INTERP arg2 (mode3 螺旋中心 y)
    unknown_fields(0x2dd8, 0x4);       // 0x2dd8-0x2ddc
    ZunTimer moveTimer;                // 0x2ddc  (movement timer; op65-73 SetCurrent)
    i32 moveTicks;                     // 0x2de8  (movement duration in frames; op65-73)
    f32 moveBoundMin;                  // 0x2dec  (movement bound; Initialize -0.15, op152)
    f32 moveBoundMax;                  // 0x2df0  (movement bound; Initialize 0.15, op152)
    i16 moveBound1;                    // 0x2df4  (movement bound i16, op152)
    i16 moveBound2;                    // 0x2df6  (movement bound i16, op152)
    i16 moveBound3;                    // 0x2df8  (movement bound i16, op152)
    i16 moveBound4;                    // 0x2dfa  (movement bound i16, op152)
    i32 laserActive;                       // 0x2dfc  laser-in-use flag
    i32 laserData;                       // 0x2e00  (laser-related, divide base)
    i32 laserParam;                    // 0x2e04  (laser param; set with laserData by op131, alone by op177)
    unknown_fields(0x2e08, 0x4);       // 0x2e08-0x2e0c
    i32 bulletFlags;                   // 0x2e0c  (bit0 = 反转特效角速度, checked by op89-91/173)
    unknown_fields(0x2e10, 0x4);       // 0x2e10-0x2e14
    ZunTimer eclTimer;                 // 0x2e14 (0xc bytes; general ECL timer, op132/133/153 SetCurrent)
    unknown_fields(0x2e20, 0x4);       // 0x2e20-0x2e24
    // 0x2e24: SetupLaserMove 输入/输出块 — 原版输入 Float3(0x2e24) 与输出 Float3(0x2e28) 字节重叠
    f32 laserMoveStartX;   // 0x2e24
    f32 laserMoveYZ;       // 0x2e28  (startY 与 resultX 共享)
    f32 laserMoveZ2;       // 0x2e2c  (startZ 与 resultY 共享)
    f32 laserMoveResultZ;  // 0x2e30
    unknown_fields(0x2e34, 0x10);      // 0x2e34-0x2e44
    EnemyLaserData laserPatterns[0x13]; // 0x2e44 (op111 按 0x18 步进索引)
    unknown_fields(0x300c, 0x14);      // 0x300c-0x3020
    i32 lifeCallbackState;                       // 0x3020  (boss spellcard-related)
    i32 lifeCallbackThreshold;                       // 0x3024  (set by SET_LIFE_CALLBACK_THRESHOLD)
    i32 lifeCallbackSub;                       // 0x3028  (set by SET_LIFE_CALLBACK_SUB)
    unknown_fields(0x302c, 0x8);       // 0x302c-0x3034
    u8 laserInstrBuf[0x2c];            // 0x3034 (0x2c bytes; op96-104 缓冲的激光 ECL 指令)
    i32 rankScaledValue;               // 0x3060  (按 rank 缩放的生命值/分数基值, op105/106)
    ZunTimer rankScaledTimer;          // 0x3064 (0xc bytes; 与 rankScaledValue 一同重置)
    EnemyShotData shotData;            // 0x3070 (0x200 bytes, op114-115 fill it)
    unknown_fields(0x3270, 0x10);      // 0x3270-0x3280
    EnemySubData *shotSlots[0x20];       // 0x3280 (0x20 shot-pattern slots)
    i32 shotSlotIdx;                       // 0x3300
    i32 eclData0;                      // 0x3304  (ECL data value, op143)
    i32 eclData1;                      // 0x3308  (ECL data value, op144)
    i32 eclData2;                      // 0x330c  (ECL data value, op144)
    u8 byteFlag0;                      // 0x3310  (op138 复制的 3 字节标志)
    u8 byteFlag1;                      // 0x3311
    u8 byteFlag2;                      // 0x3312
    u8 bossMarkerIdx;                        // 0x3313
    unknown_fields(0x3314, 0x10);      // 0x3314-0x3324
    // 0x3324 ECL enemy state flags (bits as observed in RunEcl opcodes):
    //   bit1  = boss-marker registered (op127)
    //   bit2  = bullet pattern active (op90-92 clear on spawn, op127 set)
    //   bit8  = bullet pattern spawned (op90-92)
    //   bit11 = player-is-youkai modifier (op90-92)
    //   bit12-13 = movement mode (op64-66)
    //   bit17 = laser active (op107/108, checked by op96-104)
    //   bit26 = do NOT save context when taking an interrupt
    //   bit30/31 = misc flags (op173/176, op183)
    u32 flags;                       // 0x3324
    u32 anmFlags;                    // 0x3328  (anm script flags; bit2 cleared by SET_ANM)
    unknown_fields(0x332c, 0x3);       // 0x332c-0x332f
    u8 enemyType;                     // 0x332f  (敌机类型标志: 0=youkai / 2=human; op89-91 设 IsYoukai()?0:2, op156/159)
    u8 eclFlags;                       // 0x3330
    unknown_fields(0x3331, 0xb);       // 0x3331-0x333c
    i16 currentAnmIdx;                       // 0x333c  current animation id
    unknown_fields(0x333e, 0x2);       // 0x333e-0x3340
    f32 moveSpeedA;                    // 0x3340  SET_MOVE_SPEED4 分量 1 (op75; 亦作瞄准角度阈值)
    f32 moveSpeedB;                    // 0x3344  SET_MOVE_SPEED4 分量 2 (op75)
    f32 moveSpeedC;                    // 0x3348  SET_MOVE_SPEED4 分量 3 (op75)
    f32 moveSpeedD;                    // 0x334c  SET_MOVE_SPEED4 分量 4 (op75)
    f32 speedSquared;                  // 0x3350  (速度平方, op82 设 f0*f0; Initialize 1024.0)
    unknown_fields(0x3354, 0x4);       // 0x3354-0x3358
    i32 eclDataArray0[4];              // 0x3358  (indexed by ECL arg; boss data / effect slots, op133/134)
    i32 eclDataArray1[4];              // 0x3368  (same indexing; set with eclDataArray0, non-boss only)
    i32 eclDataValue0;                 // 0x3378  (data slot value; initialized to -1, op134)
    i32 eclDataValue1;                 // 0x337c  (data slot value; op134, op153 copies savedEclDataValue)
    i32 subEnemyCount;                       // 0x3380  (sub-enemy chain count)
    EclDataSlot *dataSlots[4];             // 0x3384
    unknown_fields(0x3394, 0x1fb8);    // 0x3394-0x534c
    u8 aiFlags;                        // 0x534c  (tutorial/boss-ai flags, bit3 gate)
    i16 aiParam0;                      // 0x534e
    i16 aiParam1;                      // 0x5350
    i16 aiParam2;                      // 0x5352
    ZunTimer aiTimer;                  // 0x5354 (0xc bytes)
    void *effectSlots[24];             // 0x5360 (24 effect-slot pointers, op128)
    i32 effectSlotIdx;                 // 0x53c0  (effect-slot counter/index)
    i32 effectSlotValue;               // 0x53c4  (effect-slot value, op128)
    u32 linkedEffect;                  // 0x53c8  (pointer to linked effect/enemy object)
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