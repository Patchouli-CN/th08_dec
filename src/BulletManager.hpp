#pragma once

#include "AnmManager.hpp"
#include "Global.hpp"

namespace th08
{

struct EnemySubData; // forward decl (defined in EnemyManager.hpp)
struct EnemyShotData; // forward decl (defined in EnemyManager.hpp)

/* One 0x10b8-byte enemy-bullet slot in g_BulletPool (0x600 slots).
 * Fields used by BulletManager::Initialize / RemoveAllBullets are named; the
 * rest is raw padding. Layout verified against the original (0x431240 OnUpdate
 * iterates the same 0x600 x 0x10b8 pool at BulletManager+0x1a880). */
struct EnemyBullet
{
    unknown_fields(0x0, 0x21a);
    u16 chainIndex0;             /* 0x21a 索引/链字段; Initialize 写 0xffff (-1), 仅此处使用 */
    unknown_fields(0x21c, 0x2a2);
    u16 chainIndex1;             /* 0x4be 索引/链字段 (同上) */
    unknown_fields(0x4c0, 0x2a2);
    u16 chainIndex2;             /* 0x762 索引/链字段 (同上) */
    unknown_fields(0x764, 0x2a2);
    u16 chainIndex3;             /* 0xa06 索引/链字段 (同上) */
    unknown_fields(0xa08, 0x2a2);
    u16 chainIndex4;             /* 0xcaa 索引/链字段 (同上) */
    unknown_fields(0xcac, 0x88);
    Float3 collisionSize;        /* 0xd34 碰撞尺寸 (Float3; x 分量即碰撞半径 —
                                  *       原版各弹型写 4.0/5.0/6.0/8.0/9.0/10.0/24.0) */
    unknown_fields(0xd40, 0x4);
    Float3 pos;                  /* 0xd44 世界位置 (RemoveAllBullets 掉落道具用) */
    unknown_fields(0xd50, 0x68);
    u16 state;                   /* 0xdb8 0=空槽 / 5=死亡清场中 (OnUpdate 状态机; RemoveAllBullets 设 5) */
    unknown_fields(0xdba, 0x2fe);
};
C_ASSERT(sizeof(EnemyBullet) == 0x10b8);

/* One 0x59c-byte player-shot/laser slot at BulletManager+0x660938 (0x100 slots).
 * The first 0x59c bytes overlap the EnemySubData layout (see EnemyManager.hpp);
 * field names follow that struct where the offsets match. */
struct PlayerShotData
{
    unknown_fields(0x0, 0x548);
    Float3 pos;                  /* 0x548 世界位置 */
    f32 angle;                   /* 0x554 当前角度 */
    f32 speed;                   /* 0x558 当前速度 */
    f32 targetSpeed;             /* 0x55c 速度爬升目标 (RemoveAllBullets: targetSpeed <= speed 时停止) */
    f32 speedLimit;              /* 0x560 速度调整阈值 */
    f32 phaseTimer;              /* 0x564 阶段计时 (从 savedPhaseTimer 恢复) */
    f32 savedPhaseTimer;         /* 0x568 保存的阶段计时 */
    unknown_fields(0x56c, 0x14);
    u32 param4;                  /* 0x580 (EnemyShotData.param4 语义; RemoveAllBullets 结尾置 0) */
    u32 isActive;                /* 0x584 0 => 槽空闲 (RemoveAllBullets 跳过) */
    ZunTimer timer;              /* 0x588 */
    u16 flags;                   /* 0x594 bit2: RemoveAllBullets 检查 (flags&4 且 param!=4 则跳过) */
    unknown_fields(0x596, 0x2);
    u8 runState;                 /* 0x598 <2 设 2 (RemoveAllBullets 复位) */
    unknown_fields(0x599, 0x3);
};
C_ASSERT(sizeof(PlayerShotData) == 0x59c);

struct BulletTypeSprites
{
    /* 敌弹类型 -> 精灵 的映射表 (暂未定义/使用) */
};

struct Laser
{
    /* 激光对象 (暂未定义/使用) */
};

struct Bullet
{
    /* 子弹对象 (暂未定义/使用) */
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
    /* NOTE: 原版该区域为 0x1a880..0x660638 (0x645db8 字节, 敌弹池 + 0xdb8 字节其余数据)。
     * 此处按现状声明 0x4a5b8 字节——修正大小会改变 Initialize/RemoveAllBullets 的
     * 编译字节输出，故为保持字节级一致而保留。 */
    u8 enemyBulletPool[0x4a5b8]; /* 0x1a880 敌弹池区域 (原版 OnUpdate 用 this+0x1a880 作 0x600 槽池基址) */
    u16 bulletCount;             /* 0x660638 计数/索引 (Initialize 设 6; 原版仅此处写入) */
    unknown_fields(0x66063a, 0x45116);  /* 0x66063a */
    char *etamaAnmPath;          /* 0x6ba550 */
    unknown_fields(0x6ba554, 0x18);     /* 0x6ba554 */
    i32 bulletPoolPtr;           /* 0x6ba56c 弹池起始/当前槽指针 (Initialize: (i32)&enemyBulletPool) */
    i32 itemType;                /* 0x6ba570 清场掉落的道具类型 (SpawnItem itemType; Initialize 设 6) */
    unknown_fields(0x6ba574, 0x4);      /* 0x6ba574 */
    /* 0x6ba53c 清场计数/状态 (clearCount): RemoveAllBullets 置 10——因结构体成员尚未对齐到
     * 该偏移，cpp 中以裸指针访问 (*(i32 *)((u8 *)this + 0x6ba53c))。 */

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
