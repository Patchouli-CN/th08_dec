#pragma once

#include "AnmManager.hpp"
#include "Global.hpp"

namespace th08
{

struct EnemySubData; // forward decl (defined in EnemyManager.hpp)
struct EnemyShotData; // forward decl (defined in EnemyManager.hpp)

/* One 0x10b8-byte enemy-bullet slot in g_BulletPool (0x600 slots).
 * The slot begins with 5 AnmVms (0x0 / 0x2a4 / 0x548 / 0x7ec / 0xa90), then the
 * tail data (0xd34..0x10b8). Layout verified against the original (0x431240
 * OnUpdate iterates the same 0x600 x 0x10b8 pool at BulletManager+0x1a880). */
struct EnemyBullet
{
    AnmVm vms[5];               /* 0x0   (5 * 0x2a4 = 0xd34) */
    Float3 collisionSize;       /* 0xd34 碰撞尺寸 (Float3; x 分量即碰撞半径 —
                                  *       原版各弹型写 4.0/5.0/6.0/8.0/9.0/10.0/24.0) */
    u8 unkD40;                  /* 0xd40 */
    u8 unkD41;                  /* 0xd41 碰撞半径取整 (AddedCallback: ftol(loadedSprite[0x30])) */
    u8 unkD42;                  /* 0xd42 弹型索引 (链头数组 chainHeads 下标, AddedCallback 设 0..5) */
    u8 unkD43;                  /* 0xd43 */
    Float3 pos;                 /* 0xd44 世界位置 (RemoveAllBullets 掉落道具用) */
    Float3 vel;                 /* 0xd50 逐帧速度向量 (pos += vel 移动) */
    u8 unkD5C[0x24];            /* 0xd5c */
    ZunTimer timerD80;          /* 0xd80 每帧 Tick 的动画计时器 */
    ZunTimer timerD8C;          /* 0xd8c 生命周期计时器 (AsFrames 检查 / 递减) */
    u32 unkD98;                 /* 0xd98 */
    u32 unkD9C;                 /* 0xd9c */
    u32 unkDA0;                 /* 0xda0 */
    u32 unkDA4;                 /* 0xda4 */
    u32 unkDA8;                 /* 0xda8 离屏计次 (OnUpdate 递减/判离屏) */
    u32 flagsDAC;               /* 0xdac 行为标志位 (bit0/10/20/40/80/100/800/2000/20000/400000/800000) */
    u32 flagsDB0;               /* 0xdb0 碰撞标志位 (bit0x1000: 免碰撞) */
    u32 unkDB4;                 /* 0xdb4 */
    u16 state;                  /* 0xdb8 0=空槽 / 1=飞行 / 2..4=出生减速 / 5=死亡清场 (OnUpdate 状态机) */
    u16 unkDBA;                 /* 0xdba 离屏计数 */
    u8 unkDBC;                  /* 0xdbc */
    u8 unkDBD;                  /* 0xdbd 碰撞阶段标志 */
    u8 unkDBE;                  /* 0xdbe 出生碰撞标志 */
    u8 unkDBF;                  /* 0xdbf */
    EnemyBullet *next;          /* 0xdc0 同类型链下一个 (OnUpdate 挂到 chainHeads[type]) */
    u8 unkDC4[0x298];           /* 0xdc4 */
    ZunTimer timer105C;         /* 0x105c 20000 标志位计时器 */
    u8 unk1068[0x4c];           /* 0x1068 */
    i8 unk10B4;                 /* 0x10b4 跳碰撞检查标志 */
    u8 unk10B5[3];              /* 0x10b5 */
};
C_ASSERT(sizeof(EnemyBullet) == 0x10b8);

/* One 0x59c-byte player-shot/laser slot at BulletManager+0x660938 (0x100 slots).
 * The first 0x548 bytes are 2 AnmVms (0x0 / 0x2a4), then the tail data overlaps
 * the EnemySubData layout (see EnemyManager.hpp); field names follow that struct
 * where the offsets match. */
struct PlayerShotData
{
    AnmVm vms[2];               /* 0x0   (2 * 0x2a4 = 0x548) */
    Float3 pos;                  /* 0x548 世界位置 */
    f32 angle;                   /* 0x554 当前角度 */
    f32 speed;                   /* 0x558 当前速度 */
    f32 targetSpeed;             /* 0x55c 速度爬升目标 (RemoveAllBullets: targetSpeed <= speed 时停止) */
    f32 speedLimit;              /* 0x560 速度调整阈值 */
    f32 phaseTimer;              /* 0x564 阶段计时 (从 savedPhaseTimer 恢复) */
    f32 savedPhaseTimer;         /* 0x568 保存的阶段计时 */
    f32 speedStep;               /* 0x56c 每帧速度增量 (targetSpeed += g_ShotSpeed * speedStep) */
    i32 delay;                   /* 0x570 初始延迟 (runState 0 阶段长度) */
    i32 trigger;                 /* 0x574 runState 0 阶段碰撞触发阈值 */
    i32 phase;                   /* 0x578 runState 1 阶段长度 */
    i32 fadeTime;                /* 0x57c runState 2 收尾时长 (<=0 立即结束) */
    i32 param4;                  /* 0x580 (EnemyShotData.param4 语义; RemoveAllBullets 结尾置 0) */
    u32 isActive;                /* 0x584 0 => 槽空闲 (RemoveAllBullets 跳过) */
    ZunTimer timer;              /* 0x588 */
    u16 flags;                   /* 0x594 bit2: RemoveAllBullets 检查 (flags&4 且 param!=4 则跳过); bit0: alpha 渐隐 */
    u8 unk596[2];                /* 0x596 (OnDraw 覆写为 0xffff) */
    u8 runState;                 /* 0x598 0/1/2 激光状态机 (RemoveAllBullets 复位为 2) */
    u8 unk599[3];                /* 0x599 */
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

/* 敌弹槽级行为 helper（this = EnemyBullet 槽指针）。
 * 内部未逆向，保持 stub；类名/成员名对齐 reccmp-functions.csv
 * （th08::Bullet::FUN_*）以便 OnUpdate 调用目标归一化。 */
struct Bullet
{
    void FUN_00432170(); /* 0x432170 */
    void FUN_00432210(); /* 0x432210 */
    void FUN_004322b0(); /* 0x4322b0 */
    void FUN_00432390(); /* 0x432390 */
    void FUN_00432460(); /* 0x432460 */
    void FUN_004325a0(); /* 0x4325a0 */
    void FUN_004326e0(); /* 0x4326e0 */
    void FUN_00432830(); /* 0x432830 */
    void FUN_004329f0(); /* 0x4329f0 */
    void FUN_00432aa0(); /* 0x432aa0 */
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
    /* 0x1a880..0x660638: 敌弹池区域 (0x600 x 0x10b8 槽 + 0xdb8 其余数据)。
     * 原版 OnUpdate 用 this+0x1a880 作 0x600 槽池基址; Initialize/RemoveAllBullets
     * 经 g_BulletPool (即 g_BulletManager+0x1a880, 同一块内存) 访问。 */
    u8 enemyBulletPool[0x645db8];        /* 0x1a880 */
    u16 bulletCount;                     /* 0x660638 计数/索引 (Initialize 设 6; 原版仅此处写入) */
    unknown_fields(0x66063a, 0x59efe);   /* 0x66063a..0x6ba538 */
    i32 unk6ba538;                       /* 0x6ba538 每帧活跃敌弹计数 (OnUpdate 清零/累加) */
    i32 clearCount;                      /* 0x6ba53c 清场计数/状态 (RemoveAllBullets 置 10) */
    ZunTimer timer6ba540;                /* 0x6ba540 每帧 Tick 的总计时器 */
    i32 unk6ba54c;                       /* 0x6ba54c 每帧自增计数器 */
    char *etamaAnmPath;                  /* 0x6ba550 */
    EnemyBullet *chainHeads[6];          /* 0x6ba554 弹型链头数组 (OnUpdate 按 slot->unkD42 挂链) */
    i32 bulletPoolPtr;                   /* 0x6ba56c 弹池起始/当前槽指针 (Initialize: (i32)&enemyBulletPool) */
    i32 itemType;                        /* 0x6ba570 清场掉落的道具类型 (SpawnItem itemType; Initialize 设 6) */
    AnmLoaded *bulletAnm;                /* 0x6ba574 etama.anm 加载句柄 (AddedCallback) */

    static ZunResult RegisterChain(char *path);
    static ChainCallbackResult OnUpdate(BulletManager *bulletManager);
    static ChainCallbackResult OnDraw(BulletManager *bulletManager);
    static ZunResult AddedCallback(BulletManager *bulletManager);
    static ZunResult DeletedCallback(BulletManager *bulletManager);
    static void CutChain();

    /* 跨类 thiscall 帮手 stub（内部未逆向；PDB 名对齐 reccmp-functions.csv）。
     * FUN_004321b0: this=BulletManager (OnUpdate 调用, 0x431290)。
     * DrawSingleBullet: this=EnemyBullet 槽 (OnDraw 调用, ecx=slot, 0x432ed6)。 */
    void FUN_004321b0();              /* 0x4321b0 */
    void DrawSingleBullet();          /* 0x432f20 */
    void *FUN_00430aa0(i32 a, i32 b); /* 0x430aa0 */
};
C_ASSERT(sizeof(BulletManager) == 0x6ba578);

DIFFABLE_EXTERN(BulletManager, g_BulletManager);
DIFFABLE_EXTERN(AnmLoaded *, g_BulletAnm);

} /* namespace th08 */
