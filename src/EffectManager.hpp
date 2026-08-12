#pragma once

#include "AnmManager.hpp"
#include "ZunMath.hpp"
#include "ZunResult.hpp"

namespace th08
{

typedef void (AnmLoaded::*AnmLoadedSetAndExecuteScriptIdxFn)(AnmVm *, int);

struct EffectTemplate
{
    i32 field0;                            // 0x0
    i32 field4;                            // 0x4
    i32 (__fastcall *field8)(void *param); // 0x8
};

C_ASSERT(sizeof(EffectTemplate) == 0xc);

struct EffectManagerParticle
{
    unknown_fields(0x0, 0x1f0);
    u32 spawnParam;                   // 0x1f0  (spawn param; SpawnEffect arg c / SpawnEffectAtSlot arg d)
    unknown_fields(0x1f4, 0x4);
    u32 flags;                        // 0x1f8  (0x2000 set by spawn, 0x20000 = screen-flip marker)
    unknown_fields(0x1fc, 0x8c);
    u32 drawOffsetX;                  // 0x288  (Float3 draw offset added to render pos; zeroed on spawn)
    u32 drawOffsetY;                  // 0x28c
    u32 drawOffsetZ;                  // 0x290
    unknown_fields(0x294, 0x10);
    Float3 spawnPos;                  // 0x2a4  (spawn position, from spawn *pos)
    unknown_fields(0x2b0, 0x24);
    Float3 pos;                       // 0x2d4  (effect animated position, updated by template callbacks)
    unknown_fields(0x2e0, 0xc);
    Float3 targetPos;                 // 0x2ec  (position set by op128)
    unknown_fields(0x2f8, 0x30);
    u32 slotIdx;                      // 0x328  (slot index; set by SpawnEffectAtSlot)
    unknown_fields(0x32c, 0x1c);
    u32 updateFn;                     // 0x348  (EffectTemplate.field4; per-frame update callback)
    unknown_fields(0x34c, 0x4);
    i8 alive;                         // 0x350  (1 = active, 0 = done)
    i8 type;                          // 0x351  (effect template type a)
    i8 despawnFlag;                   // 0x352  (set to 1 by Enemy::ClearEffectSlots to kill the effect)
    unknown_fields(0x353, 0x5);
    void *dataPtr;                    // 0x358  (allocated data pointer freed on despawn)
    unknown_fields(0x35c, 0x4);
};

C_ASSERT(sizeof(EffectManagerParticle) == 0x360);

struct EffectManager
{
    static ZunResult RegisterChain();
    static void CutChain();

    void ResetEffects();
    void DrawParticles();                              // 0x4281e0 (遍历特效链表绘制)
    AnmVm *SpawnEffect(i32 a, Float3 *pos, i32 b, i32 c);        // 0x425430 (轮转分配特效槽)
    AnmVm *SpawnEffectAtSlot(i32 a, Float3 *pos, i32 b, i32 c, i32 d); // 0x425870 (在指定槽 particles[b+0x280] 生成特效)
    void *AllocEffectSlot(i32 type, Float3 *pos, i32 b, i32 c); // 0x425b70 (claim an effect slot)
    void SpawnEffectLocal(i32 a0, Float3 *pos, Float3 *localPos, i32 a1, i32 a2); // 0x425650 (op140; 带局部位置生成特效)

    i32 nextSlotIdx;                     // 0x0  (SpawnEffect 轮转分配游标: particles[nextSlotIdx++], 绕 0x200)
    unknown_fields(0x4, 0x18);
    EffectManagerParticle particles[512]; // 0x1c
    unknown_fields(0x6c01c, 0x1f038);    // 0x6c01c..0x8b054
    AnmLoaded *effectAnm;                // 0x8b054  (SetAndExecuteScriptIdx 用的特效 AnmLoaded)
    unknown_fields(0x8b058, 0x4);        // 到 0x8b05c
};

C_ASSERT(sizeof(EffectManager) == 0x8b05c);

DIFFABLE_EXTERN(EffectManager, g_EffectManager);

} /* namespace th08 */
