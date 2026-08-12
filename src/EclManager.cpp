#include "th_pch.h"

#include "EclManager.hpp"
#include "EnemyManager.hpp"
#include "Global.hpp"
#include "GameManager.hpp"
#include "BulletManager.hpp"
#include "Player.hpp"
#include "SoundPlayer.hpp"
#include "Gui.hpp"
#include "ItemManager.hpp"
#include "EffectManager.hpp"
#include "AsciiManager.hpp"

namespace th08
{

// SpawnEnemy op 参数拷贝 (case 92/93): 7 dword = subId + Float3 + 3 int
struct EnemySpawnData
{
    i32 subId;   // +0
    Float3 pos;  // +4
    i32 a, b, c; // +0x10
};

DIFFABLE_STATIC(EclManager, g_EclManager);
DIFFABLE_STATIC_ARRAY(EclExInstr, 32, g_EclExInsn); // 0x4c6cb0
DIFFABLE_STATIC(EclGlobalObj, g_EclGlobalObj);      // 0x4ea670
DIFFABLE_STATIC(EclInterruptTable, g_EclInterruptTable); // 0x4eccb8

// ECL 魔法常量命名
enum
{
    ECL_SOUND_ENEMY_SPAWN = 0x24,      // op90-92 敌人生成音效
    ECL_SOUND_CLOCK_CHIME = 0x2d,      // op181 时钟整点报时
    ECL_EFFECT_SPAWN_COLOR = 0xff6060d0, // op128 全屏特效颜色
    ECL_SHOT_SLOT_ID_LASER = 0x73,     // op114-115 弹幕 ID == 0x73 判定激光模式
    ECL_SCORE_SAVE_MAGIC = 0x5f5e0f6,  // op155 计分屏保存时间戳

    // Enemy::flags (0x3324) 位定义 — 语义由各 opcode 行为确认
    ECL_FLAG_ENEMY_ACTIVE = 0x1,            // 敌人在场 (EnemyManager::Initialize 设置)
    ECL_FLAG_BOSS_MARKER = 0x2,             // boss 血条标记已注册 (op127)
    ECL_FLAG_BULLET_PATTERN = 0x4,          // 弹幕模式活跃 (op90-92 生成时清除)
    ECL_FLAG_BULLET_SPAWNED = 0x100,        // 子弹模式已生成 (op90-92)
    ECL_FLAG_ISYOUKAI_MODE = 0x800,         // 依玩家 youkai 状态调整 (op90-92)
    ECL_FLAG_MOVE_MODE_MASK = 0x3000,       // 移动模式位域 (bit12-13)
    ECL_FLAG_MOVE_MODE_ANGLE = 0x1000,      // 移动模式 = 角度 (op64-66)
    ECL_FLAG_LASER_ACTIVE = 0x20000,        // 激光活跃 (op107/108, op96-104 检查)
    ECL_FLAG_CLEAR_MOVE = 0x80000,          // 清除移动 (op76)
    ECL_FLAG_SPECIAL_EFFECT = 0x10000000,   // 特殊特效 (op79/80)
    ECL_FLAG_ITEM_SCATTER = 0x4000,         // (保留位, op79/80 相关)
    ECL_FLAG_NO_SAVE_ON_INTERRUPT = 0x4000000, // 中断时不保存上下文 (op151, handleInterrupt)
    ECL_FLAG_SCORE_MODE = 0x40000000,       // (op173/176)
    ECL_FLAG_TRACK_POS = 0x200,             // 按位置移动 (op92)
    ECL_FLAG_UNK27 = 0x8000000,             // (op155)
    ECL_FLAG_UNK31 = 0x80000000,            // (op183)

    // Enemy::anmFlags (0x3328) 位定义
    ECL_ANM_FLAG_SET_SUB_ANM = 0x4,         // 使用副动画集 (op54-60)
    ECL_ANM_FLAG_ISYOUKAI = 0x2,            // (op83)
    ECL_ANM_FLAG_EFFECT = 0x40,             // 特效标志 (op79/80)
    ECL_ANM_FLAG_ANM_LOADED = 0x100,        // (op182)

    // 撒物品随机偏移 (op142/168): 范围 128, 偏移 -32
    ECL_ITEM_SCATTER_RANGE = 0x4b443c,      // 随机半径 128.0f (内存浮点)
    ECL_ITEM_SCATTER_OFFSET = 0x4b42c8,     // 偏移 -32.0f (内存浮点)

    // case 168 (op169) 出口随机角常量 — 内存浮点地址 (值见注释)
    ECL_EXIT_LEFT_BOUND = 0x17d61ac,        // 左边界 (BSS 零初始化, 值 0.0f)
    ECL_EXIT_ANGLE_X_LOW = 0x4b42c4,        // 96.0  低 x 阈值 (pos.x > 96 走 +3π/4 分支)
    ECL_EXIT_ANGLE_X_HIGH = 0x4b4888,       // 288.0 高 x 阈值
    ECL_EXIT_ANGLE_ADD = 0x4b4884,          // 3π/4 ≈ 2.3562 随机角偏移 (AddNormalizeAngle 相加)
    ECL_EXIT_ANGLE_SUB = 0x4b4524,          // π/4 ≈ 0.7854 随机角偏移 (直接相减)
};

// op169 出口随机角范围 ≈ π/2 (字面量保留原版 1.5708f, 值 ≈ 0x3fc90ff9)
static const f32 ECL_RANDOM_ANGLE_RANGE = 1.5708f;

// ECL variable access helpers (th08 standalone functions; th07 had these as
// EclManager static methods). Stubs for now; RunEcl's call targets normalize to
// T in fn_diff so only the calling convention needs to be right here.
i32 __fastcall GetVarValue(Enemy *enemy, i32 varId);                       // 0x41f420
i32 *__fastcall GetIntPtr(Enemy *enemy, AnyArg *args, u16 paramMask, i32 argIdx = -1); // 0x41fe10
f32 *__fastcall GetFloatPtr(Enemy *enemy, AnyArg *args, u16 paramMask, i32 unused); // 0x420950
void __fastcall SetEclGlobalData(Enemy *enemy, EclRawInstr *instr);      // 0x421280 (op122: 调 EclGlobalObj::0x4152a0 set-data)
void __fastcall RunEclGlobal(Enemy *enemy, EclRawInstr *instr);          // 0x4212e0 (op123: 调 EclGlobalObj::0x4161b0 全局 ECL 更新)
void __fastcall EclLerp(Enemy *enemy, EclRawInstr *instr);               // 0x421300 (op35: *f0 = f2 + (f1-f2)*f3 线性插值)
void __fastcall SetupEclInterp(Enemy *enemy, EclRawInstr *instr);        // 0x4213f0 (op36: 填充 EclInterp 槽, fn 取 0x4c6c90 表)
EclRawInstr *__fastcall RunSubScript(Enemy *enemy, EclRawInstr *instr);   // 0x4215f0 子脚本 (op40-51)
void __fastcall MoveInterp(Enemy *enemy, EclRawInstr *instr);           // 0x420f40
void __fastcall StartSubContext(Enemy *enemy, EclRawInstr *instr, i32 arg0); // 0x421bd0
i32 __fastcall RunSubContext(Enemy *enemy, EclRawInstr *instr);            // 0x421cb0
void __fastcall SetMoveVelocity(Enemy *enemy, EclRawInstr *instr);       // 0x420d10 (op66/69 arg0>0: 角度→速度向量 + 插值起点)
void __fastcall SetSubVmAnm(Enemy *enemy, EclRawInstr *instr);           // 0x421e50 (op57/61: 在 enemy->vms[arg0] 播动画, arg1<0 停)
void __fastcall SetMoveAngleToPlayer(Enemy *enemy, EclRawInstr *instr);  // 0x422020 (op67: 依玩家角度+unk3340 阈值算瞄准角)
void __fastcall SetRandomExitAngle(Enemy *enemy, EclRawInstr *instr);    // 0x4224a0 (op178: 依 pos.x 计算出口随机角)
void __fastcall RunLaserScript(Enemy *enemy, EclRawInstr *instr);           // 0x422720 (laser op96-104)

// op90-93 底层 helper（内部逻辑无需逆向，call 目标归一化为 T，只需签名/返回类型正确）
Enemy *__fastcall GetLastSubEnemy(Enemy *enemy);                            // 0x41efc0 获取关联 Enemy (ecx)
Enemy *__fastcall InitBulletPattern(Enemy *enemy, EclRawInstr *instr);        // 0x41f110 初始化弹幕 Enemy (ecx,edx)
Enemy *__fastcall InitBulletPatternAbs(Enemy *enemy, EclRawInstr *instr);        // 0x41f280 初始化弹幕 Enemy (变体)
Enemy *__fastcall InitEnemySpawnData(Enemy *enemy);                            // 0x41f400 初始化 Enemy Float3 (ecx)

f32 __stdcall EclAtan2(f32 a, f32 b); // th08 0x41f090 (wraps CRT atan2)

i32 __fastcall GetVarValue(Enemy *enemy, i32 varId)
{
    return 0;
}

f32 __stdcall EclAtan2(f32 a, f32 b)
{
    return 0.0f;
}

f32 __fastcall EclAngleFromDxDy(f32 dx, f32 dy); // th08 0x40c7b0 (atan2 variant)

f32 __fastcall EclAngleFromDxDy(f32 dx, f32 dy)
{
    return 0.0f;
}

void Enemy::EclSubCall(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5)
{
}

void Enemy::InitMoveAfterSetPos()
{
}

void Enemy::ClearEffectSlots()
{
}

void Enemy::UpdateEnemyMove()
{
}

void Enemy::UpdateLaserScript()
{
}

i32 *__fastcall GetIntPtr(Enemy *enemy, AnyArg *args, u16 paramMask, i32 argIdx)
{
    return NULL;
}

f32 Enemy::GetEclFloatVar(i32 varId)
{
    return 0.0f;
}

f32 *__fastcall GetFloatPtr(Enemy *enemy, AnyArg *args, u16 paramMask, i32 unused)
{
    return NULL;
}

void __fastcall SetEclGlobalData(Enemy *enemy, EclRawInstr *instr)
{
}

void __fastcall RunEclGlobal(Enemy *enemy, EclRawInstr *instr)
{
}

void __fastcall EclLerp(Enemy *enemy, EclRawInstr *instr)
{
}

void __fastcall SetupEclInterp(Enemy *enemy, EclRawInstr *instr)
{
}

EclRawInstr *__fastcall RunSubScript(Enemy *enemy, EclRawInstr *instr)
{
    return NULL;
}

void __fastcall MoveInterp(Enemy *enemy, EclRawInstr *instr)
{
}

void __fastcall StartSubContext(Enemy *enemy, EclRawInstr *instr, i32 arg0)
{
}

i32 __fastcall RunSubContext(Enemy *enemy, EclRawInstr *instr)
{
    return 0;
}

void __fastcall SetMoveVelocity(Enemy *enemy, EclRawInstr *instr)
{
}

void __fastcall SetSubVmAnm(Enemy *enemy, EclRawInstr *instr)
{
}

void __fastcall SetMoveAngleToPlayer(Enemy *enemy, EclRawInstr *instr)
{
}

void __fastcall SetRandomExitAngle(Enemy *enemy, EclRawInstr *instr)
{
}

void __fastcall RunLaserScript(Enemy *enemy, EclRawInstr *instr)
{
}

Enemy *__fastcall GetLastSubEnemy(Enemy *enemy)
{
    return NULL;
}

Enemy *__fastcall InitBulletPattern(Enemy *enemy, EclRawInstr *instr)
{
    return NULL;
}

Enemy *__fastcall InitBulletPatternAbs(Enemy *enemy, EclRawInstr *instr)
{
    return NULL;
}

Enemy *__fastcall InitEnemySpawnData(Enemy *enemy)
{
    return NULL;
}

void EclGlobalObj::SetGlobalFlag(i32 a0)
{
}

void EclGlobalObj::SetGlobalFlag2(i32 a0)
{
}

void EclGlobalObj::SetTargetPos(f32 a0, f32 a1, f32 a2)
{
}

void EclInterruptTable::SetupEclContext(EclContext *ctx, i16 subId)
{
}

// FUNCTION: th08 0x418330
ZunResult EclManager::Load(const char *path)
{
    return ZUN_ERROR;
}

// FUNCTION: th08 0x4184b0 (逆向中)
#pragma var_order(arg, subCtxIdx, instr, v37n, v37m, v38dx, v38dy, p8,                                              \
                  v89node, v89head, v90node, v90head, v91node, v91head, v110ld, v113sd, v113args, p18, p19, \
                  v130i, v157v, p22, p23, p24, v141cnt, v141i, v141z, v141y, v141x, v167cnt, v167i, v167z, v167y, v167x, \
                  v92res, v92d6, v92d5, v92d4, v92d3, v92d2, v92d1, v92d0, v92pz, v92py, v92px,                      \
                  v93res, v93d6, v93d5, v93d4, v93d3, v93d2, v93d1, v93d0, v93pz, v93py, v93px,                      \
                  iInterp, t, flag, interp, savedPos, i, p65, p66,                                                    \
                  p67, p68, p69, p70, p71, p72, p73, v108z, v108y, v108x, p77, p78, p79, p80, p81, p82, v1, v4a, v4b, v5, \
                  v6, v7, v8a, v8b, v8c, v9a, v9b, v14a, v14b, v10a, v10b, v15a, v15b, v11a, v11b, v16a, v16b, \
                  v12a, v12b, v17a, v17b, v13a, v13b, v18b, v18a, v18c, v19a, v19b, v24a, v24b, v24c, v20a, v20b, \
                  v25a, v25b, v25c, v21a, v21b, v26a, v26b, v26c, v22a, v22b, v27a, v27b, v27c, v23a, v23b, \
                  v28a, v28b, v28c, v29a, v30a, v31a, p141, v32a, p143, v33a, v33b, v33c, v33d, v33e, v36a, v36b, \
                  v37a, v37b, v37c, v37d, v38a, v38b, v38c, v38d, v38e, v53a, v54a, v55a, v55b, v55c, v55d, v55e, \
                  v55f, v57a, v58a, v59a, v59b, v59c, v59d, v59e, v59f, v62a, v62b, v64a, v64b, v65a, v65b, v65c, \
                  v67a, v67b, v68a, v68b, v68c, v68d, v69a, v70a, v71a, v71b, v71c, v71d, v71e, v71f, v71g, \
                  v72a, v72b, v72c, v72d, v73a, v73b, v73c, v74a, v74b, v74c, v74d, v76a, v76b, v77a, v77b, \
                  v78a, v79a, v80a, v85a, v85b, v86a, v86b, v86c, v87a, v88a, v88b, v88c, v89a, v90a, v91a, \
                  v110a, v110b, v110c, v110d, v110e, v110f, v110g, v104a, v105a, v109a, v109b, v113a, v113b, \
                  v113c, v113d, v113e, v113f, v113g, v113h, v113i, v113j, v115a, v116a, v116b, v166a, v166b, \
                  v117a, v117b, v118a, v118b, v118c, v118d, v169a, v169b, v119a, v120a, v170a, v170b, \
                  v171a, v171b, v171c, v162a, v126a, v126b, v126c, v126d, v158a, v123a, v125a, v125b, \
                  v124a, v130a, v157a, v157b, v157c, v157d, v131a, v132a, v132b, v132c, v132d, v132e, \
                  v132f, v133a, v133b, v133c, v134a, v134b, v134c, v138a, v138b, v139a, v139b, v139c, \
                  v139d, v139e, v142a, v143a, v143b, v141a, v141b, v141c, v167a, v167b, v167c, \
                  v135a, v136a, v136b, v145a, v140a, v146a, v147a, v92a, v92b, v92c, v92d, v92e, \
                  v92f, v93a, v93b, v93c, v93d, v93e, v93f, v148a, v112a, v112b, v112c, v151a, \
                  v151b, v151c, v151d, v151e, v151f, v156a, v156b, v156c, v159a, v160a, v163a, \
                  v163b, v163c, v163d, v164a, v165a, v165b, v165c, v165d, v165e, v165f, v172a, \
                  v182a, v81a, v82a, v173a, v174a, v176a, v181a, v183a)
ZunResult EclManager::RunEcl(Enemy *enemy)
{
    EclRawInstr *instr;
    i32 arg;
    i32 subCtxIdx = -1;
    i32 i;
    f32 v37n;             // slot 4 (case 37 低槽: 归一化角度)
    f32 v37m;             // slot 5 (case 37 低槽: f3 副本)
    f32 v38dx;            // slot 6 (case 38 低槽: dx)
    f32 v38dy;            // slot 7 (case 38 低槽: dy)
    i32 p8;
    Enemy *v89node;       // slot 9 (case 89: 弹幕生成 node)
    Enemy *v89head;       // slot 10 (case 89: 弹幕生成 head)
    Enemy *v90node;       // slot 11 (case 90: 弹幕生成 node)
    Enemy *v90head;       // slot 12 (case 90: 弹幕生成 head)
    Enemy *v91node;       // slot 13 (case 91: 弹幕生成 node)
    Enemy *v91head;       // slot 14 (case 91: 弹幕生成 head)
    EnemyLaserData *v110ld; // slot 15 (case 110: 激光数据指针)
    EnemyShotData *v113sd;    // slot 16 (case 113: 弹幕数据指针)
    AnyArg *v113args;    // slot 17 (case 113: args 指针)
    i32 p18, p19;
    i32 v130i, v157v;  // slots 20-21 (case 130 loop counter / case 157 v0 copy)
    i32 p22, p23, p24;
    i32 v141cnt;                  // slot 25 (case 141 循环计数)
    i32 v141i;                    // slot 26 (case 141 循环变量)
    f32 v141z, v141y, v141x;      // slots 27/28/29 (case 141 Float3: &v141x 为基址)
    i32 v167cnt;                  // slot 30 (case 167 循环计数)
    i32 v167i;                    // slot 31 (case 167 循环变量)
    f32 v167z, v167y, v167x;      // slots 32/33/34 (case 167 Float3: &v167x 为基址)
    // case 92/93 低槽: 结果 + EnemySpawnData 拷贝(7 dword, 基址=&v92d0) + Float3(基址=&v92px)
    i32 v92res;                                        // slot 35
    i32 v92d6, v92d5, v92d4, v92d3, v92d2, v92d1, v92d0; // slots 36-42 (v92d0=-0xa8 基址)
    f32 v92pz, v92py, v92px;                           // slots 43-45 (v92px=-0xb4 基址)
    i32 v93res;                                        // slot 46
    i32 v93d6, v93d5, v93d4, v93d3, v93d2, v93d1, v93d0; // slots 47-53 (v93d0=-0xd4 基址)
    f32 v93pz, v93py, v93px;                           // slots 54-56 (v93px=-0xe0 基址)
    i32 p65, p66, p67, p68, p69, p70, p71, p72, p73;
    f32 v108z, v108y, v108x;  // slots 74-76 (case 108: pos+moveVec2 Float3 结果, &v108x 基址)
    i32 p77, p78, p79, p80;
    i32 p81, p82;
    // exit 块变量 (槽 57-65): iInterp@57, t@58, flag@59, interp@60, savedPos@61-63, i@64
    i32 iInterp;
    f32 t;
    i32 flag;
    EclInterp *interp;
    Float3 savedPos;
    i32 v1;   // slot 83 (case 1: SET_WAIT_TIMER)
    i32 *v4a; // slot 84 (case 4: DEC_JUMP GetIntPtr ptr)
    i32 v4b;  // slot 85 (case 4: DEC_JUMP arg2)
    i32 v5;   // slot 86 (case 5: SET_INT)
    f32 v6;   // slot 87 (case 6: SET_FLOAT)
    i32 v7;   // slot 88 (case 7: RAND_SIGN)
    f32 v8a, v8b, v8c;  // slots 89-91 (case 8: RAND_SIGN_FLOAT)
    i32 v9a; i32 *v9b;   // slots 92-93 (case 9: ADD)
    f32 v14a; f32 *v14b; // slots 94-95 (case 14: ADD_FLOAT)
    i32 v10a; i32 *v10b; // slots 96-97 (case 10: SUB)
    f32 v15a; f32 *v15b; // slots 98-99 (case 15: SUB_FLOAT)
    i32 v11a; i32 *v11b; // slots 100-101 (case 11: MUL)
    f32 v16a; f32 *v16b; // slots 102-103 (case 16: MUL_FLOAT)
    i32 v12a; i32 *v12b; // slots 104-105 (case 12: DIV)
    f32 v17a; f32 *v17b; // slots 106-107 (case 17: DIV_FLOAT)
    i32 v13a; i32 *v13b; // slots 108-109 (case 13: MOD)
    f32 v18b, v18a, v18c; // slots 110-112 (case 18: ATAN2; 先读 arg1 存低槽)
    i32 v19a, v19b;     // slots 113-114 (case 19: SET_ADD)
    f32 v24a, v24b, v24c; // slots 115-117 (case 24: SET_ADD_FLOAT)
    i32 v20a, v20b;     // slots 118-119 (case 20: SET_SUB)
    f32 v25a, v25b, v25c; // slots 120-122 (case 25: SET_SUB_FLOAT)
    i32 v21a, v21b;     // slots 123-124 (case 21: SET_MUL)
    f32 v26a, v26b, v26c; // slots 125-127 (case 26: SET_MUL_FLOAT)
    i32 v22a, v22b;     // slots 128-129 (case 22: SET_DIV)
    f32 v27a, v27b, v27c; // slots 130-132 (case 27: SET_DIV_FLOAT)
    i32 v23a, v23b;     // slots 133-134 (case 23: SET_MOD)
    f32 v28a, v28b, v28c; // slots 135-137 (case 28: ATAN2_SWAP)
    i32 *v29a;          // slot 138 (case 29: INC)
    i32 *v30a;          // slot 139 (case 30: DEC)
    f32 v31a;           // slot 140 (case 31: SIN)
    i32 p141;           // slot 141 (unused)
    f32 v32a;           // slot 142 (case 32: COS)
    i32 p143;           // slot 143 (unused)
    f32 v33a, v33b, v33c, v33d, v33e; // slots 144-148 (case 33: 两点角度; read order f3,f1,f4,f2,result)
    f32 v36a, v36b; // slots 149-150 (case 36: NORMALIZE_ANGLE; ECL_FVAL(0), result)
    f32 v37a, v37b, v37c, v37d; // slots 151-154 (case 37: 角度转坐标; f2 + f3 + cos + sin)
    f32 v38a, v38b, v38c, v38d, v38e; // slots 155-159 (case 38: 两点距离; f1 + f3 + f2 + f4 + result)
    i32 v53a;       // slot 160 (case 53: SET_ANM)
    i32 v54a;       // slot 161 (case 54: SET_ANM arg0 temp)
    i32 v55a, v55b, v55c, v55d, v55e, v55f; // slots 162-167 (case 55: SUB_CALL; eval order arg5..arg0)
    i32 v57a;       // slot 168 (case 57: SET_ANM_SUB)
    i32 v58a;       // slot 169 (case 58)
    i32 v59a, v59b, v59c, v59d, v59e, v59f; // slots 170-175 (case 59: SUB_CALL; eval order arg5..arg0)
    f32 v62a, v62b; // slots 176-177 (case 62: SET_POS)
    f32 v64a, v64b; // slots 178-179 (case 64: SET_MOVE_ANGLE; ECL_FVAL(0), ECL_FVAL(1))
    i32 v65a; f32 v65b, v65c; // slots 180-182 (case 65: SET_MOVE_ANGLE 条件; IVAL(0), FVAL(1), FVAL(2))
    f32 v67a, v67b; // slots 183-184 (case 67: 瞄准+速度; FVAL(0), FVAL(1))
    i32 v68a; f32 v68b, v68c; i32 v68d; // slots 185-188 (case 68: 瞄准条件; IVAL(0), FVAL(1), FVAL(2), IVAL(0))
    f32 v69a;       // slot 189 (case 69: SET_MOVE_SPEED)
    f32 v70a;       // slot 190 (case 70: 移动角度)
    i32 v71a; f32 v71b, v71c, v71d, v71e, v71f, v71g; // slots 191-197 (case 71: MOVE_INTERP; IVAL0,FVAL1-6)
    i32 v72a; f32 v72b, v72c, v72d; // slots 198-201 (case 72: MOVE_INTERP; IVAL0,FVAL1-3)
    i32 v73a; f32 v73b, v73c; // slots 202-204 (case 73: MOVE_INTERP; IVAL0,FVAL1-2)
    f32 v74a, v74b, v74c, v74d; // slots 205-208 (case 74: SET_MOVE_SPEED4; FVAL0-3)
    f32 v76a, v76b; // slots 209-210 (case 76: 写两 float; FVAL0, FVAL1)
    f32 v77a, v77b; // slots 211-212 (case 77: 写两 float; FVAL0, FVAL1)
    i32 v78a;       // slot 213 (case 78: 设置特效标志 flags)
    i32 v79a;       // slot 214 (case 79: 清除特效标志 flags)
    i32 v80a;       // slot 215 (case 80: 混合特效标志 flags)
    i32 v85a, v85b; // slots 216-217 (case 85: 子弹对象 int 变量)
    i32 v86a, v86b, v86c; // slots 218-220 (case 86: 子弹对象 float 变量)
    i32 v87a;       // slot 221 (case 87: 子弹对象子脚本)
    i32 v88a, v88b, v88c; // slots 222-224 (case 88: 子弹对象 interrupt)
    i32 v89a, v90a, v91a; // slots 225-227 (case 89-91: IsYoukai 临时)
    i32 v110a, v110b, v110c, v110d, v110e, v110f, v110g; // slots 228-234 (case 110: 激光数据)
    i32 v104a, v105a;     // slots 235-236 (case 104/105: unk3060)
    f32 v109a, v109b;     // slots 237-238 (case 109: 激光 moveVec2)
    i32 v113a, v113b, v113c, v113d, v113e, v113f, v113g, v113h, v113i, v113j; // slots 239-248 (case 113: 弹幕数据)
    i32 v115a;            // slot 249 (case 115: shotSlotIdx)
    i32 v116a; f32 v116b; // slots 250-251 (case 116: 子弹角度)
    i32 v166a; f32 v166b; // slots 252-253 (case 166: 子弹 angle)
    i32 v117a; f32 v117b; // slots 254-255 (case 117: 子弹角度到玩家)
    i32 v118a; f32 v118b, v118c, v118d; // slots 256-259 (case 118: 子弹 pos; idx + f1-3)
    i32 v169a; i32 v169b; // slots 260-261 (case 169: 子弹+0x599; idx + value)
    i32 v119a;        // slot 262 (case 119: curContext intVars)
    i32 v120a;        // slot 263 (case 120: RUN_EX_INS)
    i32 v170a; f32 v170b; // slots 264-265 (case 170: 子弹 unk560; idx + f)
    i32 v171a; f32 v171b, v171c; // slots 266-268 (case 171: 子弹 unk558/55c; idx + f1 + f2)
    i32 v162a;        // slot 269 (case 162: 写全局 g_f54cec)
    i32 v126a, v126b, v126c, v126d; // slots 270-273 (case 126: boss 血条标记)
    i32 v158a;        // slot 274 (case 158: 写 unk332f byte)
    i32 v123a;        // slot 275 (case 123: 播放音效)
    i32 v125a, v125b; // slots 276-277 (case 125: interrupts 数组; value + idx)
    i32 v124a;        // slot 278 (case 124: 触发中断子程序)
    i32 v130a;        // slot 279 (case 130: 激光字段; v130i@20 循环计数)
    i32 v157a, v157b, v157c, v157d; // slots 280-283 (case 157: Gui 数据; v0 + a1 + a2 + a3)
    i32 v131a;        // slot 284 (case 131: unk2e14 计时器)
    i32 v132a, v132b, v132c, v132d, v132e, v132f; // slots 285-290 (case 132: unk3358/unk3368; 非boss: value1+idx1+value2+idx2, boss: value1+idx1)
    i32 v133a, v133b, v133c; // slots 291-293 (case 133: unk3378/337c; 非boss: val0+val1, boss: val0)
    i32 v134a, v134b, v134c; // slots 294-296 (case 134: dataSlots 分配/释放; idx + subId, subId re-read)
    i32 v138a, v138b; // slots 297-298 (case 138: 生成特效; a1(bit2) + a0(bit1))
    f32 v139a, v139b, v139c; i32 v139d, v139e; // slots 299-303 (case 139: 生成特效含局部pos; f3-5 + a1 + a0)
    i32 v142a;        // slot 304 (case 142: unk3304)
    i32 v143a, v143b; // slots 305-306 (case 143: unk3308/330c)
    i32 v141a; f32 *v141b, *v141c; // slots 307-309 (case 141: 随机撒物品; count + Float3* x/y)
    i32 v167a; f32 *v167b, *v167c; // slots 310-312 (case 167: 随机撒物品; count + Float3* x/y)
    i32 v135a;        // slot 313 (case 135: 间接调用 g_EclExInsn)
    i32 v136a, v136b; // slots 314-315 (case 136: curContext func/eclExInstr)
    i32 v145a;        // slot 316 (case 145: curContext time +=)
    i32 v140a;        // slot 317 (case 140: 生成物品)
    i32 v146a;        // slot 318 (case 146)
    i32 v147a;        // slot 319 (case 147)
    f32 v92a, v92b, v92c; i32 v92d, v92e, v92f; // slots 320-325 (case 92: SpawnEnemy; f1-3 + arg4/5/6)
    f32 v93a, v93b, v93c; i32 v93d, v93e, v93f; // slots 326-331 (case 93: SpawnEnemy rel; f1-3 + arg4/5/6)
    i32 v148a;        // slot 332 (case 148: primaryVm 挂起中断)
    i32 v112a, v112b, v112c; // slots 333-335 (case 112: 生命回调; v0 + threshold + sub)
    f32 v151a, v151b; i32 v151c, v151d, v151e, v151f; // slots 336-341 (case 151: 移动界限字段)
    i32 v156a, v156b, v156c; // slots 342-344 (case 156: AI 字段)
    i32 v159a;        // slot 345 (case 159: unk5354 SetCurrent)
    f32 v160a;        // slot 346 (case 160: 生成特效 at movePos)
    i32 v163a, v163b, v163c, v163d; // slots 347-350 (case 163)
    i32 v164a;        // slot 351 (case 164)
    f32 v165a, v165b, v165c, v165d, v165e, v165f; // slots 352-357 (case 165)
    i32 v172a;        // slot 360 (case 172: flags bit30)
    i32 v182a;        // slot 361 (case 182: flags bit31)
    f32 v81a;         // slot 362 (case 81: unk3350 平方)
    i32 v82a;         // slot 363 (case 82: anmFlags bit1)
    i32 v173a;        // slot 364 (case 173: boss 特效)
    i32 v174a;        // slot 365 (case 174)
    i32 v176a;        // slot 366 (case 176)
    i32 v181a;        // slot 367 (case 181)
    i32 v183a;        // slot 368 (case 183)

    enemy->savedStackPtr = &enemy->savedContextStack[0];
    enemy->curContextPtr = &enemy->eclContext;
    enemy->stackDepth = enemy->unk2ce8;

restart:
    // ECL arg access: paramMask bit N set means arg N is a variable id.
#define ECL_IVAL(n) ((instr->paramMask & (1 << (n))) ? GetVarValue(enemy, instr->args[n].i) : instr->args[n].i)
#define ECL_FVAL(n) ((instr->paramMask & (1 << (n))) ? enemy->GetEclFloatVar(instr->args[n].i) : instr->args[n].f)
    // Boss-mode gate: bit14 (extra stage) AND bits7-8 set → several boss-only
    // ECL opcodes skip their non-boss write and take a shorter path.
#define IS_BOSS_MODE() ((((g_PlayerFlags >> 0xe) & 1) != 0) && (((g_PlayerFlags >> 7) & 3) != 0))
    instr = enemy->curContextPtr->curInstr;
    if (enemy->runInterrupt >= 0)
    {
        goto handleInterrupt;
    }
    for (;;)
    {
        enemy->movePos = enemy->pos + enemy->moveVec;

        if ((i32)enemy->curContextPtr->waitTimer > 0)
        {
            enemy->curContextPtr->waitTimer--;
            enemy->curContextPtr->time--;
            goto exit;
        }
        if (enemy->curContextPtr->time == (i32)instr->time)
        {
            // 原版: 仅当 skipInstr 完全覆盖 (difficultyMask|eclFlags) 时才执行本条指令
            if ((instr->skipInstrOnDifficulty & (g_GameManager.difficultyMask | enemy->eclFlags)) ==
                (g_GameManager.difficultyMask | enemy->eclFlags))
            {
            switch (instr->id - 1)
            {
            case 0: // ECL_UNIMP
                return ZUN_ERROR;
            case 1: // ECL_SET_WAIT_TIMER: waitTimer = arg0
                if (instr->paramMask & 1)
                    v1 = GetVarValue(enemy, instr->args[0].i);
                else
                    v1 = instr->args[0].i;
                enemy->curContextPtr->waitTimer.SetCurrent(v1);
                goto skipInstr;
            case 4: // ECL_DEC_JUMP: *arg2--; if (arg2 > 0) jump
                v4a = GetIntPtr(enemy, &instr->args[2], instr->paramMask, 2);
                *v4a -= 1;
                if (instr->paramMask & 0x4)
                    v4b = GetVarValue(enemy, instr->args[2].i);
                else
                    v4b = instr->args[2].i;
                if (v4b > 0)
                {
                    enemy->curContextPtr->time.current = instr->args[0].i;
                    instr = (EclRawInstr *)((u8 *)instr + instr->args[1].i);
                    continue;
                }
                goto skipInstr;
            case 3: // ECL_JUMP: time = arg0; jump by arg1
                enemy->curContextPtr->time.current = instr->args[0].i;
                instr = (EclRawInstr *)((u8 *)instr + instr->args[1].i);
                continue;
            case 5: // ECL_SET_INT: *arg0 = arg1
                if (instr->paramMask & 0x2)
                    v5 = GetVarValue(enemy, instr->args[1].i);
                else
                    v5 = instr->args[1].i;
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask, 0) = v5;
                goto skipInstr;
            case 6: // ECL_SET_FLOAT: *arg0 = arg1
                if (instr->paramMask & 0x2)
                    v6 = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v6 = instr->args[1].f;
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = v6;
                goto skipInstr;
            case 7: // ECL_RAND_SIGN: *arg0 = ±arg1
                if (instr->paramMask & 0x2)
                    v7 = GetVarValue(enemy, instr->args[1].i);
                else
                    v7 = instr->args[1].i;
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask, 0) =
                    (g_Rng.GetRandomU16() & 1 ? 1 : -1) * v7;
                goto skipInstr;
            case 8: // ECL_RAND_SIGN_FLOAT: *arg0 = ±arg1
                v8a = (g_Rng.GetRandomU16() & 1) ? 1.0f : -1.0f;
                if (instr->paramMask & 0x2)
                    v8b = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v8b = instr->args[1].f;
                v8c = v8a * v8b;
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = v8c;
                goto skipInstr;
            case 9: // ECL_ADD: *arg0 += arg1
                if (instr->paramMask & 0x2)
                    v9a = GetVarValue(enemy, instr->args[1].i);
                else
                    v9a = instr->args[1].i;
                v9b = GetIntPtr(enemy, &instr->args[0], instr->paramMask, 0);
                *v9b += v9a;
                goto skipInstr;
            case 14: // ECL_ADD_FLOAT: *arg0 += arg1
                if (instr->paramMask & 0x2)
                    v14a = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v14a = instr->args[1].f;
                v14b = GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0);
                *v14b += v14a;
                goto skipInstr;
            case 10: // ECL_SUB: *arg0 -= arg1
                if (instr->paramMask & 0x2)
                    v10a = GetVarValue(enemy, instr->args[1].i);
                else
                    v10a = instr->args[1].i;
                v10b = GetIntPtr(enemy, &instr->args[0], instr->paramMask, 0);
                *v10b -= v10a;
                goto skipInstr;
            case 15: // ECL_SUB_FLOAT: *arg0 -= arg1
                if (instr->paramMask & 0x2)
                    v15a = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v15a = instr->args[1].f;
                v15b = GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0);
                *v15b -= v15a;
                goto skipInstr;
            case 11: // ECL_MUL: *arg0 *= arg1
                if (instr->paramMask & 0x2)
                    v11a = GetVarValue(enemy, instr->args[1].i);
                else
                    v11a = instr->args[1].i;
                v11b = GetIntPtr(enemy, &instr->args[0], instr->paramMask, 0);
                *v11b *= v11a;
                goto skipInstr;
            case 16: // ECL_MUL_FLOAT: *arg0 *= arg1
                if (instr->paramMask & 0x2)
                    v16a = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v16a = instr->args[1].f;
                v16b = GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0);
                *v16b *= v16a;
                goto skipInstr;
            case 12: // ECL_DIV: *arg0 /= arg1
                if (instr->paramMask & 0x2)
                    v12a = GetVarValue(enemy, instr->args[1].i);
                else
                    v12a = instr->args[1].i;
                v12b = GetIntPtr(enemy, &instr->args[0], instr->paramMask, 0);
                *v12b /= v12a;
                goto skipInstr;
            case 17: // ECL_DIV_FLOAT: *arg0 /= arg1
                if (instr->paramMask & 0x2)
                    v17a = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v17a = instr->args[1].f;
                v17b = GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0);
                *v17b /= v17a;
                goto skipInstr;
            case 13: // ECL_MOD: *arg0 %= arg1
                if (instr->paramMask & 0x2)
                    v13a = GetVarValue(enemy, instr->args[1].i);
                else
                    v13a = instr->args[1].i;
                v13b = GetIntPtr(enemy, &instr->args[0], instr->paramMask, 0);
                *v13b %= v13a;
                goto skipInstr;
            case 18: // ECL_ATAN2: *arg0 = atan2(arg0, arg1) — 原版先读 arg1 后读 arg0
                if (instr->paramMask & 0x2)
                    v18b = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v18b = instr->args[1].f;
                if (instr->paramMask & 0x1)
                    v18a = enemy->GetEclFloatVar(instr->args[0].i);
                else
                    v18a = instr->args[0].f;
                v18c = EclAtan2(v18a, v18b);
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = v18c;
                goto skipInstr;
            case 19: // ECL_SET_ADD: *arg0 = arg1 + arg2
                if (instr->paramMask & 0x2)
                    v19a = GetVarValue(enemy, instr->args[1].i);
                else
                    v19a = instr->args[1].i;
                if (instr->paramMask & 0x4)
                    v19b = GetVarValue(enemy, instr->args[2].i);
                else
                    v19b = instr->args[2].i;
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask, 0) = v19a + v19b;
                goto skipInstr;
            case 24: // ECL_SET_ADD_FLOAT: *arg0 = arg1 + arg2
                if (instr->paramMask & 0x2)
                    v24a = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v24a = instr->args[1].f;
                if (instr->paramMask & 0x4)
                    v24b = enemy->GetEclFloatVar(instr->args[2].i);
                else
                    v24b = instr->args[2].f;
                v24c = v24a + v24b;
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = v24c;
                goto skipInstr;
            case 20: // ECL_SET_SUB: *arg0 = arg1 - arg2
                if (instr->paramMask & 0x2)
                    v20a = GetVarValue(enemy, instr->args[1].i);
                else
                    v20a = instr->args[1].i;
                if (instr->paramMask & 0x4)
                    v20b = GetVarValue(enemy, instr->args[2].i);
                else
                    v20b = instr->args[2].i;
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask, 0) = v20a - v20b;
                goto skipInstr;
            case 25: // ECL_SET_SUB_FLOAT: *arg0 = arg1 - arg2
                if (instr->paramMask & 0x2)
                    v25a = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v25a = instr->args[1].f;
                if (instr->paramMask & 0x4)
                    v25b = enemy->GetEclFloatVar(instr->args[2].i);
                else
                    v25b = instr->args[2].f;
                v25c = v25a - v25b;
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = v25c;
                goto skipInstr;
            case 21: // ECL_SET_MUL: *arg0 = arg1 * arg2
                if (instr->paramMask & 0x2)
                    v21a = GetVarValue(enemy, instr->args[1].i);
                else
                    v21a = instr->args[1].i;
                if (instr->paramMask & 0x4)
                    v21b = GetVarValue(enemy, instr->args[2].i);
                else
                    v21b = instr->args[2].i;
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask, 0) = v21a * v21b;
                goto skipInstr;
            case 26: // ECL_SET_MUL_FLOAT: *arg0 = arg1 * arg2
                if (instr->paramMask & 0x2)
                    v26a = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v26a = instr->args[1].f;
                if (instr->paramMask & 0x4)
                    v26b = enemy->GetEclFloatVar(instr->args[2].i);
                else
                    v26b = instr->args[2].f;
                v26c = v26a * v26b;
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = v26c;
                goto skipInstr;
            case 22: // ECL_SET_DIV: *arg0 = arg1 / arg2
                if (instr->paramMask & 0x2)
                    v22a = GetVarValue(enemy, instr->args[1].i);
                else
                    v22a = instr->args[1].i;
                if (instr->paramMask & 0x4)
                    v22b = GetVarValue(enemy, instr->args[2].i);
                else
                    v22b = instr->args[2].i;
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask, 0) = v22a / v22b;
                goto skipInstr;
            case 27: // ECL_SET_DIV_FLOAT: *arg0 = arg1 / arg2
                if (instr->paramMask & 0x2)
                    v27a = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v27a = instr->args[1].f;
                if (instr->paramMask & 0x4)
                    v27b = enemy->GetEclFloatVar(instr->args[2].i);
                else
                    v27b = instr->args[2].f;
                v27c = v27a / v27b;
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = v27c;
                goto skipInstr;
            case 23: // ECL_SET_MOD: *arg0 = arg1 %% arg2
                if (instr->paramMask & 0x2)
                    v23a = GetVarValue(enemy, instr->args[1].i);
                else
                    v23a = instr->args[1].i;
                if (instr->paramMask & 0x4)
                    v23b = GetVarValue(enemy, instr->args[2].i);
                else
                    v23b = instr->args[2].i;
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask, 0) = v23a % v23b;
                goto skipInstr;
            case 28: // ECL_ATAN2: *arg0 = atan2(arg1, arg2)
                if (instr->paramMask & 0x4)
                    v28a = enemy->GetEclFloatVar(instr->args[2].i);
                else
                    v28a = instr->args[2].f;
                if (instr->paramMask & 0x2)
                    v28b = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v28b = instr->args[1].f;
                v28c = EclAtan2(v28b, v28a);
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = v28c;
                goto skipInstr;
            case 29: // ECL_INC: *arg0 += 1
                v29a = GetIntPtr(enemy, &instr->args[0], instr->paramMask, 0);
                *v29a += 1;
                goto skipInstr;
            case 30: // ECL_DEC: *arg0 -= 1
                v30a = GetIntPtr(enemy, &instr->args[0], instr->paramMask, 0);
                *v30a -= 1;
                goto skipInstr;
            case 31: // ECL_SIN: *arg0 = sin(arg1)
                if (instr->paramMask & 0x2)
                    v31a = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v31a = instr->args[1].f;
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = sinf(v31a);
                goto skipInstr;
            case 32: // ECL_COS: *arg0 = cos(arg1)
                if (instr->paramMask & 0x2)
                    v32a = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v32a = instr->args[1].f;
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = cosf(v32a);
                goto skipInstr;
            case 33: // opcode 34 = 两点角度: *float[0]=EclAngleFromDxDy(f4-f2, f3-f1)
                // 读取序: f3(144) f1(145) f4(146) f2(147) → 结果(148)
                if (instr->paramMask & 0x8)
                    v33a = enemy->GetEclFloatVar(instr->args[3].i);
                else
                    v33a = instr->args[3].f;
                if (instr->paramMask & 0x2)
                    v33b = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v33b = instr->args[1].f;
                if (instr->paramMask & 0x10)
                    v33c = enemy->GetEclFloatVar(instr->args[4].i);
                else
                    v33c = instr->args[4].f;
                if (instr->paramMask & 0x4)
                    v33d = enemy->GetEclFloatVar(instr->args[2].i);
                else
                    v33d = instr->args[2].f;
                v33e = EclAngleFromDxDy(v33c - v33d, v33a - v33b);
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = v33e;
                goto skipInstr;
            case 36: // opcode 37 = ECL_NORMALIZE_ANGLE: *float[0] = AddNormalizeAngle(f0, 0)
                if (instr->paramMask & 0x1)
                    v36a = enemy->GetEclFloatVar(instr->args[0].i);
                else
                    v36a = instr->args[0].f;
                v36b = AddNormalizeAngle(v36a, 0);
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = v36b;
                goto skipInstr;
            case 34: // opcode 35 = 线性插值 (EclLerp: *f0 = f2 + (f1-f2)*f3)
                EclLerp(enemy, instr);
                goto skipInstr;
            case 35: // opcode 36 = 填充插值槽 (SetupEclInterp)
                SetupEclInterp(enemy, instr);
                goto skipInstr;
            case 37: // opcode 38 = 角度转坐标: *f[0]=cos(a)*m; *f[1]=sin(a)*m
                if (instr->paramMask & 0x4)
                    v37a = enemy->GetEclFloatVar(instr->args[2].i);
                else
                    v37a = instr->args[2].f;
                v37n = AddNormalizeAngle(v37a, 0);
                if (instr->paramMask & 0x8)
                    v37b = enemy->GetEclFloatVar(instr->args[3].i);
                else
                    v37b = instr->args[3].f;
                v37m = v37b;
                v37c = cosf(v37n) * v37m;
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = v37c;
                v37d = sinf(v37n) * v37m;
                *GetFloatPtr(enemy, &instr->args[1], instr->paramMask, 0) = v37d;
                goto skipInstr;
            case 38: // opcode 39 = 两点距离: *float[0]=sqrt((f1-f3)^2+(f2-f4)^2)
                if (instr->paramMask & 0x2)
                    v38a = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v38a = instr->args[1].f;
                if (instr->paramMask & 0x8)
                    v38b = enemy->GetEclFloatVar(instr->args[3].i);
                else
                    v38b = instr->args[3].f;
                v38dx = v38a - v38b;
                if (instr->paramMask & 0x4)
                    v38c = enemy->GetEclFloatVar(instr->args[2].i);
                else
                    v38c = instr->args[2].f;
                if (instr->paramMask & 0x10)
                    v38d = enemy->GetEclFloatVar(instr->args[4].i);
                else
                    v38d = instr->args[4].f;
                v38dy = v38c - v38d;
                v38e = sqrtf(v38dx * v38dx + v38dy * v38dy);
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = v38e;
                goto skipInstr;
            case 39: case 40: case 41: case 42: case 43: case 44:
            case 45: case 46: case 47: case 48: case 49: case 50:
                // opcode 40-51: 子脚本处理 (RunSubScript 内部再分派)
                {
                    EclRawInstr *subInstr = RunSubScript(enemy, instr);
                    if (subInstr != NULL)
                    {
                        instr = subInstr;
                        continue;
                    }
                }
                goto skipInstr;
            case 51: // opcode 52 = 子脚本上下文切换 (StartSubContext)
                StartSubContext(enemy, instr, instr->args[0].i);
                goto restart;
            case 52: // opcode 53 = 子脚本 (RunSubContext)
                if (RunSubContext(enemy, instr))
                {
                    enemy->curContextPtr->curInstr = instr;
                    goto exit;
                }
                goto restart;
            case 53: // ECL_SET_ANM: play animation arg0
                if (instr->paramMask & 0x1)
                    v53a = GetVarValue(enemy, instr->args[0].i);
                else
                    v53a = instr->args[0].i;
                g_EnemyAnmLoaded.SetAndExecuteScriptIdx(&enemy->primaryVm, v53a);
                enemy->anmFlags &= ~ECL_ANM_FLAG_SET_SUB_ANM;
                goto skipInstr;
            case 54: // opcode 55 = ECL_SUB_CALL: arg = arg0; EclSubCall(arg+5..arg)
                if (instr->paramMask & 0x1)
                    v54a = GetVarValue(enemy, instr->args[0].i);
                else
                    v54a = instr->args[0].i;
                arg = v54a;
                enemy->EclSubCall(arg + 5, arg + 4, arg + 3, arg + 2, arg + 1, arg);
                enemy->anmFlags &= ~ECL_ANM_FLAG_SET_SUB_ANM;
                goto skipInstr;
            case 55: // opcode 56 = ECL_SUB_CALL
                if (instr->paramMask & 0x20)
                    v55a = GetVarValue(enemy, instr->args[5].i);
                else
                    v55a = instr->args[5].i;
                if (instr->paramMask & 0x10)
                    v55b = GetVarValue(enemy, instr->args[4].i);
                else
                    v55b = instr->args[4].i;
                if (instr->paramMask & 0x8)
                    v55c = GetVarValue(enemy, instr->args[3].i);
                else
                    v55c = instr->args[3].i;
                if (instr->paramMask & 0x4)
                    v55d = GetVarValue(enemy, instr->args[2].i);
                else
                    v55d = instr->args[2].i;
                if (instr->paramMask & 0x2)
                    v55e = GetVarValue(enemy, instr->args[1].i);
                else
                    v55e = instr->args[1].i;
                if (instr->paramMask & 0x1)
                    v55f = GetVarValue(enemy, instr->args[0].i);
                else
                    v55f = instr->args[0].i;
                enemy->EclSubCall(v55f, v55e, v55d, v55c, v55b, v55a);
                enemy->anmFlags &= ~ECL_ANM_FLAG_SET_SUB_ANM;
                goto skipInstr;
            case 56: // opcode 57 = 子 VM 播动画 (SetSubVmAnm), 清 SUB_ANM 标志
                SetSubVmAnm(enemy, instr);
                enemy->anmFlags &= ~ECL_ANM_FLAG_SET_SUB_ANM;
                goto skipInstr;
            case 57: // ECL_SET_ANM_SUB: play on the secondary animation set
                if (instr->paramMask & 0x1)
                    v57a = GetVarValue(enemy, instr->args[0].i);
                else
                    v57a = instr->args[0].i;
                g_EnemyAnmLoaded2.SetAndExecuteScriptIdx(&enemy->primaryVm, v57a);
                enemy->anmFlags |= ECL_ANM_FLAG_SET_SUB_ANM;
                goto skipInstr;
            case 58: // opcode 59 = ECL_SUB_CALL
                if (instr->paramMask & 0x1)
                    v58a = GetVarValue(enemy, instr->args[0].i);
                else
                    v58a = instr->args[0].i;
                arg = v58a;
                enemy->EclSubCall(arg + 5, arg + 4, arg + 3, arg + 2, arg + 1, arg);
                enemy->anmFlags &= ~ECL_ANM_FLAG_SET_SUB_ANM;
                goto skipInstr;
            case 59: // opcode 60 = ECL_SUB_CALL
                if (instr->paramMask & 0x20)
                    v59a = GetVarValue(enemy, instr->args[5].i);
                else
                    v59a = instr->args[5].i;
                if (instr->paramMask & 0x10)
                    v59b = GetVarValue(enemy, instr->args[4].i);
                else
                    v59b = instr->args[4].i;
                if (instr->paramMask & 0x8)
                    v59c = GetVarValue(enemy, instr->args[3].i);
                else
                    v59c = instr->args[3].i;
                if (instr->paramMask & 0x4)
                    v59d = GetVarValue(enemy, instr->args[2].i);
                else
                    v59d = instr->args[2].i;
                if (instr->paramMask & 0x2)
                    v59e = GetVarValue(enemy, instr->args[1].i);
                else
                    v59e = instr->args[1].i;
                if (instr->paramMask & 0x1)
                    v59f = GetVarValue(enemy, instr->args[0].i);
                else
                    v59f = instr->args[0].i;
                enemy->EclSubCall(v59f, v59e, v59d, v59c, v59b, v59a);
                enemy->anmFlags &= ~ECL_ANM_FLAG_SET_SUB_ANM;
                goto skipInstr;
            case 60: // opcode 61 = 子 VM 播动画 (SetSubVmAnm), 置 SUB_ANM 标志
                enemy->anmFlags |= ECL_ANM_FLAG_SET_SUB_ANM;
                SetSubVmAnm(enemy, instr);
                goto skipInstr;
            case 61: // ECL_SET_ANM_SWITCH: play anim on the set chosen by anmFlags bit2
                if (((enemy->anmFlags >> 2) & 1) == 0)
                {
                    g_EnemyAnmLoaded.SetAndExecuteScriptIdx(&enemy->primaryVm, enemy->currentAnmIdx);
                }
                else
                {
                    g_EnemyAnmLoaded2.SetAndExecuteScriptIdx(&enemy->primaryVm, enemy->currentAnmIdx);
                }
                goto skipInstr;
            case 62: // opcode 63 = ECL_SET_POS: set position + move init
                if (instr->paramMask & 0x1)
                    v62a = enemy->GetEclFloatVar(instr->args[0].i);
                else
                    v62a = instr->args[0].f;
                if (instr->paramMask & 0x2)
                    v62b = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v62b = instr->args[1].f;
                enemy->pos.x = v62a;
                enemy->pos.y = v62b;
                enemy->pos.z = 0;
                enemy->InitMoveAfterSetPos();
                goto skipInstr;
            case 63: // opcode 64 = 移动插值 (MoveInterp)
                MoveInterp(enemy, instr);
                goto skipInstr;
            case 64: // opcode 65 = SET_MOVE_ANGLE
                // 完整操作: moveAngle=AddNormalizeAngle(f0,0); unk2da8=f1; flags bit; unk2de8=0; unk2ddc.SetCurrent(0)
                if (instr->paramMask & 0x1)
                    v64a = enemy->GetEclFloatVar(instr->args[0].i);
                else
                    v64a = instr->args[0].f;
                enemy->moveAngle = AddNormalizeAngle(v64a, 0);
                if (instr->paramMask & 0x2)
                    v64b = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v64b = instr->args[1].f;
                enemy->unk2da8 = v64b;
                enemy->flags = (enemy->flags & ~ECL_FLAG_MOVE_MODE_MASK) | ECL_FLAG_MOVE_MODE_ANGLE;
                enemy->unk2de8 = 0;
                enemy->unk2ddc.SetCurrent(0);
                goto skipInstr;
            case 177: // opcode 178 = 出口随机角 (SetRandomExitAngle)
                SetRandomExitAngle(enemy, instr);
                goto skipInstr;
            case 65: // ECL_SET_MOVE_ANGLE: arg0>0 走 SetMoveVelocity; 否则 moveAngle/unk2da8/flags/unk2de8/unk2ddc
                if (instr->paramMask & 0x1)
                    v65a = GetVarValue(enemy, instr->args[0].i);
                else
                    v65a = instr->args[0].i;
                if (v65a <= 0)
                {
                    if (instr->paramMask & 0x4)
                        v65b = enemy->GetEclFloatVar(instr->args[1].i);
                    else
                        v65b = instr->args[1].f;
                    enemy->moveAngle = AddNormalizeAngle(v65b, 0);
                    if (instr->paramMask & 0x8)
                        v65c = enemy->GetEclFloatVar(instr->args[2].i);
                    else
                        v65c = instr->args[2].f;
                    enemy->unk2da8 = v65c;
                    enemy->flags = (enemy->flags & ~ECL_FLAG_MOVE_MODE_MASK) | ECL_FLAG_MOVE_MODE_ANGLE;
                    enemy->unk2de8 = 0;
                    enemy->unk2ddc.SetCurrent(0);
                    goto skipInstr;
                }
                SetMoveVelocity(enemy, instr);
                goto skipInstr;
            case 66: // opcode 67 = 瞄准玩家移动 (SetMoveAngleToPlayer)
                SetMoveAngleToPlayer(enemy, instr);
                goto skipInstr;
            case 67: // opcode 68 = 设瞄准玩家角度 + 移动速度 (AngleToPlayer + AddNormalizeAngle)
                if (instr->paramMask & 0x1)
                    v67a = enemy->GetEclFloatVar(instr->args[0].i);
                else
                    v67a = instr->args[0].f;
                enemy->moveAngle = AddNormalizeAngle(v67a, g_Player.AngleToPlayer(&enemy->pos));
                if (instr->paramMask & 0x2)
                    v67b = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v67b = instr->args[1].f;
                enemy->unk2da8 = v67b;
                goto skipInstr;
            case 68: // opcode 69 = 瞄准+速度 (条件: arg0>0 走 SetMoveVelocity)
                if (instr->paramMask & 0x1)
                    v68a = GetVarValue(enemy, instr->args[0].i);
                else
                    v68a = instr->args[0].i;
                if (v68a <= 0)
                {
                    if (instr->paramMask & 0x4)
                        v68b = enemy->GetEclFloatVar(instr->args[1].i);
                    else
                        v68b = instr->args[1].f;
                    enemy->moveAngle = AddNormalizeAngle(v68b, g_Player.AngleToPlayer(&enemy->pos));
                    if (instr->paramMask & 0x8)
                        v68c = enemy->GetEclFloatVar(instr->args[2].i);
                    else
                        v68c = instr->args[2].f;
                    enemy->unk2da8 = v68c;
                    enemy->flags = (enemy->flags & ~ECL_FLAG_MOVE_MODE_MASK) | ECL_FLAG_MOVE_MODE_ANGLE;
                    if (instr->paramMask & 0x1)
                        v68d = GetVarValue(enemy, instr->args[0].i);
                    else
                        v68d = instr->args[0].i;
                    enemy->unk2de8 = v68d;
                    enemy->unk2ddc.SetCurrent(v68d);
                    goto skipInstr;
                }
                SetMoveVelocity(enemy, instr);
                goto skipInstr;
            case 69: // opcode 70 = SET_MOVE_SPEED: moveSpeed + flag bit12
                if (instr->paramMask & 0x1)
                    v69a = enemy->GetEclFloatVar(instr->args[0].i);
                else
                    v69a = instr->args[0].f;
                enemy->moveSpeed = v69a;
                enemy->flags = (enemy->flags & ~ECL_FLAG_MOVE_MODE_MASK) | ECL_FLAG_MOVE_MODE_ANGLE;
                goto skipInstr;
            case 70: // opcode 71 = 移动角度 (unk2dac) + flag bit12
                if (instr->paramMask & 0x1)
                    v70a = enemy->GetEclFloatVar(instr->args[0].i);
                else
                    v70a = instr->args[0].f;
                enemy->unk2dac = v70a;
                enemy->flags = (enemy->flags & ~ECL_FLAG_MOVE_MODE_MASK) | ECL_FLAG_MOVE_MODE_ANGLE;
                goto skipInstr;
            case 71: // opcode 72 = SET_MOVE_INTERP: 移动插值参数 (含 flags |= 0x3000)
                if (instr->paramMask & 0x1)
                    v71a = GetVarValue(enemy, instr->args[0].i);
                else
                    v71a = instr->args[0].i;
                enemy->unk2de8 = v71a;
                enemy->unk2ddc.SetCurrent(v71a);
                if (instr->paramMask & 0x2)
                    v71b = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v71b = instr->args[1].f;
                enemy->unk2dd0 = v71b;
                if (instr->paramMask & 0x4)
                    v71c = enemy->GetEclFloatVar(instr->args[2].i);
                else
                    v71c = instr->args[2].f;
                enemy->unk2dd4 = v71c;
                if (instr->paramMask & 0x8)
                    v71d = enemy->GetEclFloatVar(instr->args[3].i);
                else
                    v71d = instr->args[3].f;
                enemy->unk2d9c = v71d;
                if (instr->paramMask & 0x10)
                    v71e = enemy->GetEclFloatVar(instr->args[4].i);
                else
                    v71e = instr->args[4].f;
                enemy->unk2da0 = v71e;
                if (instr->paramMask & 0x20)
                    v71f = enemy->GetEclFloatVar(instr->args[5].i);
                else
                    v71f = instr->args[5].f;
                enemy->unk2db0 = v71f;
                if (instr->paramMask & 0x40)
                    v71g = enemy->GetEclFloatVar(instr->args[6].i);
                else
                    v71g = instr->args[6].f;
                enemy->unk2db4 = v71g;
                enemy->flags |= ECL_FLAG_MOVE_MODE_MASK;
                goto skipInstr;
            case 72: // opcode 73 = SET_MOVE_INTERP (含 flags |= 0x3000)
                if (instr->paramMask & 0x1)
                    v72a = GetVarValue(enemy, instr->args[0].i);
                else
                    v72a = instr->args[0].i;
                enemy->unk2de8 = v72a;
                enemy->unk2ddc.SetCurrent(v72a);
                if (instr->paramMask & 0x2)
                    v72b = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v72b = instr->args[1].f;
                enemy->unk2d9c = v72b;
                if (instr->paramMask & 0x4)
                    v72c = enemy->GetEclFloatVar(instr->args[2].i);
                else
                    v72c = instr->args[2].f;
                enemy->unk2da0 = v72c;
                enemy->unk2db0 = 0;
                if (instr->paramMask & 0x8)
                    v72d = enemy->GetEclFloatVar(instr->args[3].i);
                else
                    v72d = instr->args[3].f;
                enemy->unk2db4 = v72d;
                enemy->flags |= ECL_FLAG_MOVE_MODE_MASK;
                goto skipInstr;
            case 73: // opcode 74 = SET_MOVE_INTERP (含 flags |= 0x3000)
                if (instr->paramMask & 0x1)
                    v73a = GetVarValue(enemy, instr->args[0].i);
                else
                    v73a = instr->args[0].i;
                enemy->unk2de8 = v73a;
                enemy->unk2ddc.SetCurrent(v73a);
                if (instr->paramMask & 0x2)
                    v73b = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v73b = instr->args[1].f;
                enemy->unk2da0 = v73b;
                if (instr->paramMask & 0x4)
                    v73c = enemy->GetEclFloatVar(instr->args[2].i);
                else
                    v73c = instr->args[2].f;
                enemy->unk2db4 = v73c;
                enemy->flags |= ECL_FLAG_MOVE_MODE_MASK;
                goto skipInstr;
            case 74: // opcode 75 = SET_MOVE_SPEED4: unk3340/44/48/4c = f0..f3 (含 flags |= 0x80000)
                if (instr->paramMask & 0x1)
                    v74a = enemy->GetEclFloatVar(instr->args[0].i);
                else
                    v74a = instr->args[0].f;
                enemy->unk3340 = v74a;
                if (instr->paramMask & 0x2)
                    v74b = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v74b = instr->args[1].f;
                enemy->unk3344 = v74b;
                if (instr->paramMask & 0x4)
                    v74c = enemy->GetEclFloatVar(instr->args[2].i);
                else
                    v74c = instr->args[2].f;
                enemy->unk3348 = v74c;
                if (instr->paramMask & 0x8)
                    v74d = enemy->GetEclFloatVar(instr->args[3].i);
                else
                    v74d = instr->args[3].f;
                enemy->unk334c = v74d;
                enemy->flags |= ECL_FLAG_CLEAR_MOVE;
                goto skipInstr;
            case 75: // opcode 76 = 清除移动标志 (flags bit19)
                enemy->flags &= ~ECL_FLAG_CLEAR_MOVE;
                goto skipInstr;
            case 76: // opcode 77 = 写两个 float 字段
                if (instr->paramMask & 0x1)
                    v76a = enemy->GetEclFloatVar(instr->args[0].i);
                else
                    v76a = instr->args[0].f;
                enemy->unk2d70 = v76a;
                if (instr->paramMask & 0x2)
                    v76b = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v76b = instr->args[1].f;
                enemy->unk2d74 = v76b;
                goto skipInstr;
            case 77: // opcode 78 = 写两个 float 字段
                if (instr->paramMask & 0x1)
                    v77a = enemy->GetEclFloatVar(instr->args[0].i);
                else
                    v77a = instr->args[0].f;
                enemy->unk2d7c = v77a;
                if (instr->paramMask & 0x2)
                    v77b = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v77b = instr->args[1].f;
                enemy->unk2d80 = v77b;
                goto skipInstr;
            case 78: // opcode 79 = 设置特效标志 (flags bit → flags/3328 各 bit)
                if (instr->paramMask & 0x1)
                    v78a = GetVarValue(enemy, instr->args[0].i);
                else
                    v78a = instr->args[0].i;
                enemy->flags = (enemy->flags & ~0x40) | (((v78a >> 0) & 1) << 6);
                enemy->flags = (enemy->flags & ~0x4) | (((v78a >> 1) & 1) << 2);
                enemy->flags = (enemy->flags & ~0x8) | (((v78a >> 2) & 1) << 3);
                enemy->flags = (enemy->flags & ~0x10) | (((v78a >> 3) & 1) << 4);
                enemy->flags = (enemy->flags & ~0x10000000) | (((v78a >> 4) & 1) << 0x1c);
                enemy->anmFlags = (enemy->anmFlags & ~0x40) | (((v78a >> 5) & 1) << 6);
                goto skipInstr;
            case 79: // opcode 80 = 清除特效标志 (flags if 分支)
                if (instr->paramMask & 0x1)
                    v79a = GetVarValue(enemy, instr->args[0].i);
                else
                    v79a = instr->args[0].i;
                if (v79a & 1) enemy->flags &= ~0x40;
                if (v79a & 2)
                {
                    enemy->flags &= ~0x4;
                    if (enemy->unk53c8) ((EffectManagerParticle *)enemy->unk53c8)->flags &= ~0x20000;
                }
                if (v79a & 4) enemy->flags &= ~0x8;
                if (v79a & 8) enemy->flags |= 0x10;
                if (v79a & 0x10) enemy->flags |= ECL_FLAG_SPECIAL_EFFECT;
                if (v79a & 0x20) enemy->anmFlags |= ECL_ANM_FLAG_EFFECT;
                goto skipInstr;
            case 80: // opcode 81 = 混合设置/清除特效标志
                if (instr->paramMask & 0x1)
                    v80a = GetVarValue(enemy, instr->args[0].i);
                else
                    v80a = instr->args[0].i;
                if (v80a & 1) enemy->flags |= 0x40;
                if (v80a & 2)
                {
                    enemy->flags |= 0x4;
                    if (enemy->unk53c8) ((EffectManagerParticle *)enemy->unk53c8)->flags |= 0x20000;
                }
                if (v80a & 4) enemy->flags |= 0x8;
                if (v80a & 8) enemy->flags &= ~0x10;
                if (v80a & 0x10) enemy->flags &= ~ECL_FLAG_SPECIAL_EFFECT;
                if (v80a & 0x20) enemy->anmFlags &= ~ECL_ANM_FLAG_EFFECT;
                goto skipInstr;
            case 85: // opcode 86 = 读子弹对象 int 变量 (g_BulletObjects[arg2] 上的 var1)
                if (instr->paramMask & 0x4)
                    v85a = GetVarValue(enemy, instr->args[2].i);
                else
                    v85a = instr->args[2].i;
                if (instr->paramMask & 0x2)
                    v85b = GetVarValue((Enemy *)g_BulletObjects[v85a], instr->args[1].i);
                else
                    v85b = instr->args[1].i;
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask, 0) = v85b;
                goto skipInstr;
            case 86: // opcode 87 = 读子弹对象 float 变量 (g_BulletObjects[arg2] 上的 var1)
                if (instr->paramMask & 0x4)
                    v86a = GetVarValue(enemy, instr->args[2].i);
                else
                    v86a = instr->args[2].i;
                if (g_BulletObjects[v86a] != 0)
                {
                    if (instr->paramMask & 0x2)
                    {
                        if (instr->paramMask & 0x4)
                            v86b = GetVarValue(enemy, instr->args[2].i);
                        else
                            v86b = instr->args[2].i;
                        v86c = ((Enemy *)g_BulletObjects[v86b])->GetEclFloatVar(instr->args[1].i);
                    }
                    else
                    {
                        v86c = instr->args[1].f;
                    }
                    *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = v86c;
                }
                goto skipInstr;
            case 87: // opcode 88 = 子弹对象子脚本 (StartSubContext)
                if (instr->paramMask & 0x1)
                    v87a = GetVarValue(enemy, instr->args[0].i);
                else
                    v87a = instr->args[0].i;
                arg = v87a;
                StartSubContext((Enemy *)g_BulletObjects[arg],
                                ((Enemy *)g_BulletObjects[arg])->curContextPtr->curInstr,
                                instr->args[1].i);
                goto skipInstr;
            case 88: // opcode 89 = 写子弹对象+0x2d30 (若 g_BulletObjects[arg0] 有效)
                if (instr->paramMask & 0x1)
                    v88a = GetVarValue(enemy, instr->args[0].i);
                else
                    v88a = instr->args[0].i;
                if (g_BulletObjects[v88a] != 0)
                {
                    if (instr->paramMask & 0x2)
                        v88b = GetVarValue(enemy, instr->args[1].i);
                    else
                        v88b = instr->args[1].i;
                    if (instr->paramMask & 0x1)
                        v88c = GetVarValue(enemy, instr->args[0].i);
                    else
                        v88c = instr->args[0].i;
                    ((Enemy *)g_BulletObjects[v88c])->runInterrupt = (i16)v88b;
                }
                goto skipInstr;
            case 89: // opcode 90 = 生成弹幕子敌人 (GetLastSubEnemy/41f110 + 425b70)
                v89head = GetLastSubEnemy(enemy);
                v89node = InitBulletPattern(enemy, instr);
                if (g_BulletSpawnFlag == 0)
                {
                    v89node->flags |= ECL_FLAG_BULLET_SPAWNED;
                    v89a = g_Player.IsYoukai();
                    v89node->flags = (v89node->flags & ~ECL_FLAG_ISYOUKAI_MODE) | ((v89a & 1) << 0xb);
                    v89node->unk332f = (u8)(g_Player.IsYoukai() ? 0 : 2);
                    v89node->flags &= ~0x4;
                    if (v89node->unk53c8 == 0)
                    {
                        v89node->unk53c8 = (u32)g_EffectManager.AllocEffectSlot(0x20, &v89node->pos, 1, -1);
                        if (v89node->unk53c8 != 0)
                        {
                            AnmVm *obj = (AnmVm *)v89node->unk53c8;
                            obj->SetInterrupt(g_Player.IsYoukai() ? 2 : 1);
                            // 弹幕标志 bit2 → 特效 flags bit17 (屏幕方向)
                            ((EffectManagerParticle *)obj)->flags =
                                (((EffectManagerParticle *)obj)->flags & ~0x20000) |
                                (((v89node->flags >> 2) & 1) << 0x11);
                            if (v89node->unk2e0c & 1)
                                obj->prefix.angleVel.z = -obj->prefix.angleVel.z; // 反转角速度
                        }
                    }
                    v89node->ownerEnemy = enemy;
                    v89head->unk8 = (i32)v89node;
                    v89node->unk4 = (i32)v89head;
                    enemy->subEnemyCount++;
                }
                g_SoundPlayer.PlaySoundPositionedByIdx((SoundIdx)ECL_SOUND_ENEMY_SPAWN, enemy->pos.x);
                goto skipInstr;
            case 90: // opcode 91 = 生成弹幕子敌人 (41f280 变体)
                v90head = GetLastSubEnemy(enemy);
                v90node = InitBulletPatternAbs(enemy, instr);
                if (g_BulletSpawnFlag == 0)
                {
                    v90node->flags |= ECL_FLAG_BULLET_SPAWNED;
                    v90a = g_Player.IsYoukai();
                    v90node->flags = (v90node->flags & ~ECL_FLAG_ISYOUKAI_MODE) | ((v90a & 1) << 0xb);
                    v90node->unk332f = (u8)(g_Player.IsYoukai() ? 0 : 2);
                    v90node->flags &= ~0x4;
                    if (v90node->unk53c8 == 0)
                    {
                        v90node->unk53c8 = (u32)g_EffectManager.AllocEffectSlot(0x20, &v90node->pos, 1, -1);
                        if (v90node->unk53c8 != 0)
                        {
                            AnmVm *obj = (AnmVm *)v90node->unk53c8;
                            obj->SetInterrupt(g_Player.IsYoukai() ? 2 : 1);
                            // 弹幕标志 bit2 → 特效 flags bit17 (屏幕方向)
                            ((EffectManagerParticle *)obj)->flags =
                                (((EffectManagerParticle *)obj)->flags & ~0x20000) |
                                (((v90node->flags >> 2) & 1) << 0x11);
                            if (v90node->unk2e0c & 1)
                                obj->prefix.angleVel.z = -obj->prefix.angleVel.z; // 反转角速度
                        }
                    }
                    v90node->ownerEnemy = enemy;
                    v90head->unk8 = (i32)v90node;
                    v90node->unk4 = (i32)v90head;
                    enemy->subEnemyCount++;
                }
                g_SoundPlayer.PlaySoundPositionedByIdx((SoundIdx)ECL_SOUND_ENEMY_SPAWN, enemy->pos.x);
                goto skipInstr;
            case 91: // opcode 92 = 生成弹幕子敌人 (pos 复制 + 移动向量)
                v91head = GetLastSubEnemy(enemy);
                v91node = InitBulletPattern(enemy, instr);
                if (g_BulletSpawnFlag == 0)
                {
                    v91node->flags |= ECL_FLAG_BULLET_SPAWNED;
                    v91a = g_Player.IsYoukai();
                    v91node->flags = (v91node->flags & ~ECL_FLAG_ISYOUKAI_MODE) | ((v91a & 1) << 0xb);
                    v91node->unk332f = (u8)(g_Player.IsYoukai() ? 0 : 2);
                    v91node->moveVec = enemy->pos;
                    v91node->movePos = v91node->moveVec + v91node->pos;
                    v91node->flags &= ~0x4;
                    if (v91node->unk53c8 == 0)
                    {
                        v91node->unk53c8 = (u32)g_EffectManager.AllocEffectSlot(0x20, &v91node->movePos, 1, -1);
                        if (v91node->unk53c8 != 0)
                        {
                            AnmVm *obj = (AnmVm *)v91node->unk53c8;
                            obj->SetInterrupt(g_Player.IsYoukai() ? 2 : 1);
                            // 弹幕标志 bit2 → 特效 flags bit17 (屏幕方向)
                            ((EffectManagerParticle *)obj)->flags =
                                (((EffectManagerParticle *)obj)->flags & ~0x20000) |
                                (((v91node->flags >> 2) & 1) << 0x11);
                            if (v91node->unk2e0c & 1)
                                obj->prefix.angleVel.z = -obj->prefix.angleVel.z; // 反转角速度
                        }
                    }
                    v91node->flags |= ECL_FLAG_TRACK_POS;
                    v91node->ownerEnemy = enemy;
                    v91head->unk8 = (i32)v91node;
                    v91node->unk4 = (i32)v91head;
                    enemy->subEnemyCount++;
                }
                g_SoundPlayer.PlaySoundPositionedByIdx((SoundIdx)ECL_SOUND_ENEMY_SPAWN, enemy->pos.x);
                goto skipInstr;
            case 95: case 96: case 97: case 98: case 99: case 100:
            case 101: case 102: case 103:
                // opcode 96-104: 激光处理
                if (enemy->laserActive > 0)
                {
                    if (((enemy->flags >> 0x11) & 1) == 1)
                    {
                        memcpy(enemy->unk3034, instr, 0x2c);
                    }
                    else
                    {
                        RunLaserScript(enemy, instr);
                    }
                }
                goto skipInstr;
            case 110: // opcode 111 = 写激光数据槽 (0x2e44 + v0*0x18)
                if (instr->paramMask & 0x1)
                    v110a = GetVarValue(enemy, instr->args[0].i);
                else
                    v110a = instr->args[0].i;
                v110ld = &enemy->laserPatterns[v110a];
                if (instr->paramMask & 0x2)
                    v110b = GetVarValue(enemy, instr->args[1].i);
                else
                    v110b = instr->args[1].i;
                v110ld->c = v110b;
                if (instr->paramMask & 0x4)
                    v110c = GetVarValue(enemy, instr->args[2].i);
                else
                    v110c = instr->args[2].i;
                v110ld->d = v110c;
                if (instr->paramMask & 0x8)
                    v110d = GetVarValue(enemy, instr->args[3].i);
                else
                    v110d = instr->args[3].i;
                v110ld->a = v110d;
                if (instr->paramMask & 0x10)
                    v110e = GetVarValue(enemy, instr->args[4].i);
                else
                    v110e = instr->args[4].i;
                v110ld->b = v110e;
                if (instr->paramMask & 0x20)
                    v110f = enemy->GetEclFloatVar(instr->args[5].i);
                else
                    v110f = instr->args[5].f;
                v110ld->x = v110f;
                if (instr->paramMask & 0x40)
                    v110g = enemy->GetEclFloatVar(instr->args[6].i);
                else
                    v110g = instr->args[6].f;
                v110ld->y = v110g;
                goto skipInstr;
            case 137: // opcode 138 = 复制 3 字节到 unk3310/3311/3312
                enemy->unk3310 = instr->args[0].b[0];
                enemy->unk3311 = instr->args[0].b[1];
                enemy->unk3312 = instr->args[0].b[2];
                goto skipInstr;
            case 104: // opcode 105 = 设置 unk3060 + 按 rank 缩放 + unk3064 重置
                if (instr->paramMask & 0x1)
                    v104a = GetVarValue(enemy, instr->args[0].i);
                else
                    v104a = instr->args[0].i;
                enemy->unk3060 = v104a;
                if (enemy->unk3060 != 0)
                {
                    enemy->unk3060 += g_GameManager.ScaleIntBasedOnRank(enemy->unk3060 / 5, -(enemy->unk3060) / 5);
                    enemy->unk3064.SetCurrent(0);
                }
                goto skipInstr;
            case 105: // opcode 106 = 同 op105 但 unk3064 用随机值
                if (instr->paramMask & 0x1)
                    v105a = GetVarValue(enemy, instr->args[0].i);
                else
                    v105a = instr->args[0].i;
                enemy->unk3060 = v105a;
                if (enemy->unk3060 != 0)
                {
                    enemy->unk3060 += g_GameManager.ScaleIntBasedOnRank(enemy->unk3060 / 5, -(enemy->unk3060) / 5);
                    enemy->unk3064.SetCurrent(g_Rng.GetRandomU32InRange(enemy->unk3060));
                }
                goto skipInstr;
            case 106: // opcode 107 = 设置 flags bit17
                enemy->flags |= ECL_FLAG_LASER_ACTIVE;
                goto skipInstr;
            case 107: // opcode 108 = 清除 flags bit17
                enemy->flags &= ~ECL_FLAG_LASER_ACTIVE;
                goto skipInstr;
            case 108: // opcode 109 = 计算 pos+moveVec2 写入激光输出; SetupLaserMove(&输入)
                {
                    // 注: 原版输入 Float3(0x2e24) 与输出 Float3(0x2e28) 字节重叠, 故逐分量写
                    *(Float3 *)&v108x = enemy->pos + enemy->moveVec2;
                    enemy->laserMoveYZ = ((Float3 *)&v108x)->x;
                    enemy->laserMoveZ2 = ((Float3 *)&v108x)->y;
                    enemy->laserMoveResultZ = ((Float3 *)&v108x)->z;
                }
                g_BulletManager.SetupLaserMove((Float3 *)&enemy->laserMoveStartX);
                goto skipInstr;
            case 109: // opcode 110 = 移动插值 (moveVec2 向量)
                if (instr->paramMask & 0x1)
                    v109a = enemy->GetEclFloatVar(instr->args[0].i);
                else
                    v109a = instr->args[0].f;
                enemy->moveVec2.x = v109a;
                if (instr->paramMask & 0x2)
                    v109b = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v109b = instr->args[1].f;
                enemy->moveVec2.y = v109b;
                enemy->moveVec2.z = 0;
                goto skipInstr;
            case 113: case 114: // opcode 114-115 = 注册弹幕数据 (填充 shotData → AllocShotSlot)
                v113args = instr->args;
                v113sd = &enemy->shotData;
                v113sd->pos = enemy->movePos + enemy->moveVec2;
                v113sd->subId = (i16)v113args[0].i;
                if (instr->paramMask & 0x2)
                    v113a = GetVarValue(enemy, v113args[1].i);
                else
                    v113a = v113args[1].i;
                v113sd->anmIdx = (i16)v113a;
                if (instr->paramMask & 0x4)
                    v113b = enemy->GetEclFloatVar(v113args[2].i);
                else
                    v113b = v113args[2].f;
                v113sd->unk10 = v113b;
                if (instr->paramMask & 0x8)
                    v113c = enemy->GetEclFloatVar(v113args[3].i);
                else
                    v113c = v113args[3].f;
                v113sd->unk18 = v113c;
                if (instr->paramMask & 0x10)
                    v113d = enemy->GetEclFloatVar(v113args[4].i);
                else
                    v113d = v113args[4].f;
                v113sd->unk1d0 = v113d;
                if (instr->paramMask & 0x20)
                    v113e = enemy->GetEclFloatVar(v113args[5].i);
                else
                    v113e = v113args[5].f;
                v113sd->unk1d4 = v113e;
                if (instr->paramMask & 0x40)
                    v113f = enemy->GetEclFloatVar(v113args[6].i);
                else
                    v113f = v113args[6].f;
                v113sd->unk1d8 = v113f;
                if (instr->paramMask & 0x80)
                    v113g = enemy->GetEclFloatVar(v113args[7].i);
                else
                    v113g = v113args[7].f;
                v113sd->unk1dc = v113g;
                if (instr->paramMask & 0x100)
                    v113h = GetVarValue(enemy, v113args[8].i);
                else
                    v113h = v113args[8].i;
                v113sd->unk1e0 = v113h;
                if (instr->paramMask & 0x200)
                    v113i = GetVarValue(enemy, v113args[9].i);
                else
                    v113i = v113args[9].i;
                v113sd->unk1e4 = v113i;
                if (instr->paramMask & 0x400)
                    v113j = GetVarValue(enemy, v113args[10].i);
                else
                    v113j = v113args[10].i;
                v113sd->unk1e8 = v113j;
                v113sd->unk1ec = v113args[11].i;
                v113sd->unk1f0 = v113args[12].i;
                v113sd->unk1f8 = (instr->id == ECL_SHOT_SLOT_ID_LASER) ? 0 : 1;
                v113sd->unk1fc = v113args[13].i;
                enemy->shotSlots[enemy->shotSlotIdx] = g_BulletManager.AllocShotSlot(v113sd);
                goto skipInstr;
            case 115: // opcode 116 = 设置 shotSlotIdx
                if (instr->paramMask & 0x1)
                    v115a = GetVarValue(enemy, instr->args[0].i);
                else
                    v115a = instr->args[0].i;
                enemy->shotSlotIdx = v115a;
                goto skipInstr;
            case 116: // opcode 117 = 给子弹对象加角度 (shotSlots[idx] 的 unk554)
                if (instr->paramMask & 0x1)
                    v116a = GetVarValue(enemy, instr->args[0].i);
                else
                    v116a = instr->args[0].i;
                arg = v116a;
                if (enemy->shotSlots[arg] != 0)
                {
                    if (instr->paramMask & 0x2)
                        v116b = enemy->GetEclFloatVar(instr->args[1].i);
                    else
                        v116b = instr->args[1].f;
                    enemy->shotSlots[arg]->angle = AddNormalizeAngle(enemy->shotSlots[arg]->angle, v116b);
                }
                goto skipInstr;
            case 166: // opcode 167 = 设置子弹对象 unk554
                if (instr->paramMask & 0x1)
                    v166a = GetVarValue(enemy, instr->args[0].i);
                else
                    v166a = instr->args[0].i;
                arg = v166a;
                if (enemy->shotSlots[arg] != 0)
                {
                    if (instr->paramMask & 0x2)
                        v166b = enemy->GetEclFloatVar(instr->args[1].i);
                    else
                        v166b = instr->args[1].f;
                    enemy->shotSlots[arg]->angle = v166b;
                }
                goto skipInstr;
            case 117: // opcode 118 = 给子弹对象设角度: angle = AngleToPlayer(pos) + f1
                if (instr->paramMask & 0x1)
                    v117a = GetVarValue(enemy, instr->args[0].i);
                else
                    v117a = instr->args[0].i;
                arg = v117a;
                if (enemy->shotSlots[arg] != 0)
                {
                    if (instr->paramMask & 0x2)
                        v117b = enemy->GetEclFloatVar(instr->args[1].i);
                    else
                        v117b = instr->args[1].f;
                    enemy->shotSlots[arg]->angle =
                        g_Player.AngleToPlayer(&enemy->shotSlots[arg]->pos) + v117b;
                }
                goto skipInstr;
            case 118: // opcode 119 = 设置子弹对象位置 unk548/54c/550 = f + pos
                if (instr->paramMask & 0x1)
                    v118a = GetVarValue(enemy, instr->args[0].i);
                else
                    v118a = instr->args[0].i;
                arg = v118a;
                if (enemy->shotSlots[arg] != 0)
                {
                    if (instr->paramMask & 0x2)
                        v118b = enemy->GetEclFloatVar(instr->args[1].i);
                    else
                        v118b = instr->args[1].f;
                    if (instr->paramMask & 0x4)
                        v118c = enemy->GetEclFloatVar(instr->args[2].i);
                    else
                        v118c = instr->args[2].f;
                    if (instr->paramMask & 0x8)
                        v118d = enemy->GetEclFloatVar(instr->args[3].i);
                    else
                        v118d = instr->args[3].f;
                    enemy->shotSlots[arg]->pos.x = v118b + enemy->movePos.x;
                    enemy->shotSlots[arg]->pos.y = v118c + enemy->movePos.y;
                    enemy->shotSlots[arg]->pos.z = v118d + enemy->movePos.z;
                }
                goto skipInstr;
            case 169: // opcode 170 = 写子弹对象+0x599 (byte)
                if (instr->paramMask & 0x1)
                    v169a = GetVarValue(enemy, instr->args[0].i);
                else
                    v169a = instr->args[0].i;
                arg = v169a;
                if (enemy->shotSlots[arg] != 0)
                {
                    if (instr->paramMask & 0x2)
                        v169b = GetVarValue(enemy, instr->args[1].i);
                    else
                        v169b = instr->args[1].i;
                    enemy->shotSlots[arg]->unk599 = (u8)v169b;
                }
                goto skipInstr;
            case 119: // opcode 120 = 设置 curContext globalVar intVars[0] (依 shotSlots 子弹对象)
                if (instr->paramMask & 0x1)
                    v119a = GetVarValue(enemy, instr->args[0].i);
                else
                    v119a = instr->args[0].i;
                arg = v119a;
                if (enemy->shotSlots[arg] != 0 && enemy->shotSlots[arg]->isActive != 0)
                    enemy->curContextPtr->eclContextArgs.globalVars.intVars[0] = 1;
                else
                    enemy->curContextPtr->eclContextArgs.globalVars.intVars[0] = 0;
                goto skipInstr;
            case 120: // opcode 121 = RUN_EX_INS (子弹对象: unk598<2 时设状态)
                if (instr->paramMask & 0x1)
                    v120a = GetVarValue(enemy, instr->args[0].i);
                else
                    v120a = instr->args[0].i;
                arg = v120a;
                if (enemy->shotSlots[arg] != 0 && enemy->shotSlots[arg]->isActive != 0 &&
                    enemy->shotSlots[arg]->runState < 2)
                {
                    enemy->shotSlots[arg]->runState = 2;
                    enemy->shotSlots[arg]->timer.SetCurrent(0);
                    enemy->shotSlots[arg]->unk564 = enemy->shotSlots[arg]->unk568;
                }
                goto skipInstr;
            case 153: // opcode 154 = 清空 shotSlots[0x20]
                for (i = 0; i < 0x20; i++)
                {
                    enemy->shotSlots[i] = 0;
                }
                goto skipInstr;
            case 170: // opcode 171 = 设置子弹对象 unk560
                if (instr->paramMask & 0x1)
                    v170a = GetVarValue(enemy, instr->args[0].i);
                else
                    v170a = instr->args[0].i;
                arg = v170a;
                if (enemy->shotSlots[arg] != 0)
                {
                    if (instr->paramMask & 0x2)
                        v170b = enemy->GetEclFloatVar(instr->args[1].i);
                    else
                        v170b = instr->args[1].f;
                    enemy->shotSlots[arg]->unk560 = v170b;
                }
                goto skipInstr;
            case 171: // opcode 172 = 设置子弹对象 unk558/55c
                if (instr->paramMask & 0x1)
                    v171a = GetVarValue(enemy, instr->args[0].i);
                else
                    v171a = instr->args[0].i;
                arg = v171a;
                if (enemy->shotSlots[arg] != 0)
                {
                    if (instr->paramMask & 0x2)
                        v171b = enemy->GetEclFloatVar(instr->args[1].i);
                    else
                        v171b = instr->args[1].f;
                    enemy->shotSlots[arg]->unk558 = v171b;
                    if (instr->paramMask & 0x4)
                        v171c = enemy->GetEclFloatVar(instr->args[2].i);
                    else
                        v171c = instr->args[2].f;
                    enemy->shotSlots[arg]->unk55c = v171c;
                }
                goto skipInstr;
            case 162: // opcode 163 = 写全局 0xf54cec
                if (instr->paramMask & 0x1)
                    v162a = GetVarValue(enemy, instr->args[0].i);
                else
                    v162a = instr->args[0].i;
                g_f54cec = v162a;
                goto skipInstr;
            case 126: // opcode 127 = 设置/清除 boss 血条标记
                if (instr->paramMask & 0x1)
                    v126a = GetVarValue(enemy, instr->args[0].i);
                else
                    v126a = instr->args[0].i;
                if (v126a >= 0)
                {
                    if (instr->paramMask & 0x1)
                        v126b = GetVarValue(enemy, instr->args[0].i);
                    else
                        v126b = instr->args[0].i;
                    g_BulletObjects[v126b] = (u32)enemy;
                    if (instr->paramMask & 0x1)
                        v126c = GetVarValue(enemy, instr->args[0].i);
                    else
                        v126c = instr->args[0].i;
                    if (v126c == 0)
                    {
                        g_Gui.SetBossPresent(1);
                        g_Gui.SetBossLifeBarMaxSize(1.0f);
                    }
                    enemy->flags |= ECL_FLAG_BOSS_MARKER;
                    if (instr->paramMask & 0x1)
                        v126d = GetVarValue(enemy, instr->args[0].i);
                    else
                        v126d = instr->args[0].i;
                    enemy->bossMarkerIdx = (u8)v126d;
                    g_AsciiManager.SetBossMarkerInterrupt(enemy->bossMarkerIdx, 1);
                    enemy->unk3350 = 0;
                }
                else
                {
                    if (enemy->bossMarkerIdx < 4)
                        g_Gui.SetBossPresent(0);
                    g_BulletObjects[enemy->bossMarkerIdx] = 0;
                    enemy->flags &= ~ECL_FLAG_BOSS_MARKER;
                    g_AsciiManager.SetBossMarkerInterrupt(enemy->bossMarkerIdx, 2);
                    enemy->ClearEffectSlots();
                    Float3 offscreenPos(-999.0f, -999.0f, 0.0f);
                    g_AsciiManager.SetBossMarkerPosition(enemy->bossMarkerIdx, &offscreenPos);
                }
                goto skipInstr;
            case 127: // opcode 128 = 生成特效并存入 unk5360 槽
                {
                    i32 idx = enemy->unk53c0;
                    AnmVm *eff = g_EffectManager.FUN_00425430(0xd, &enemy->pos, 1, ECL_EFFECT_SPAWN_COLOR);
                    enemy->unk5360[idx] = eff;
                    ((EffectManagerParticle *)eff)->unk2ec = *(Float3 *)&instr->args[1];
                    enemy->unk53c4 = instr->args[4].i;
                    enemy->unk53c0++;
                }
                goto skipInstr;
            case 158: // opcode 159 = 设置 unk332f (byte)
                if (instr->paramMask & 0x1)
                    v158a = GetVarValue(enemy, instr->args[0].i);
                else
                    v158a = instr->args[0].i;
                enemy->unk332f = (u8)v158a;
                goto skipInstr;
            case 123: // opcode 124 = 播放音效 (声音索引 v0, 位置 pos.x)
                if (instr->paramMask & 0x1)
                    v123a = GetVarValue(enemy, instr->args[0].i);
                else
                    v123a = instr->args[0].i;
                g_SoundPlayer.PlaySoundPositionedByIdx((SoundIdx)v123a, enemy->pos.x);
                goto skipInstr;
            case 128: // opcode 129 = 设置 flags bit20-22 (boss 模式跳过)
                if (!IS_BOSS_MODE())
                    enemy->flags = (enemy->flags & 0xff8fffff) | ((instr->args[0].i & 0x7) << 0x14);
                goto skipInstr;
            case 129: // opcode 130 = 设置 unk2cee (boss 模式跳过)
                if (!IS_BOSS_MODE())
                    enemy->unk2cee = (i16)instr->args[0].i;
                goto skipInstr;
            case 125: // opcode 126 = 设置 interrupts 数组元素
                if (instr->paramMask & 0x1)
                    v125a = GetVarValue(enemy, instr->args[0].i);
                else
                    v125a = instr->args[0].i;
                if (instr->paramMask & 0x2)
                    v125b = GetVarValue(enemy, instr->args[1].i);
                else
                    v125b = instr->args[1].i;
                enemy->interrupts[v125b] = (i16)v125a;
                goto skipInstr;
            case 124: // opcode 125 = 触发中断子程序
                if (instr->paramMask & 0x1)
                    v124a = GetVarValue(enemy, instr->args[0].i);
                else
                    v124a = instr->args[0].i;
                enemy->runInterrupt = (i16)v124a;
                enemy->curContextPtr->curInstr = (EclRawInstr *)((u8 *)instr + instr->size);
                if (((enemy->flags >> 0x1a) & 1) == 0)
                    enemy->savedContextStack[enemy->stackDepth] = enemy->eclContext;
                g_EclInterruptTable.SetupEclContext(&enemy->eclContext, enemy->interrupts[enemy->runInterrupt]);
                if (enemy->stackDepth < 0xf)
                    enemy->stackDepth++;
                enemy->runInterrupt = 0xffff;
                goto restart;
            case 130: // opcode 131 = 设置激光字段 laserData/2dfc/2e04 + 若条件清 Gui
                if (instr->paramMask & 0x1)
                    v130a = GetVarValue(enemy, instr->args[0].i);
                else
                    v130a = instr->args[0].i;
                enemy->laserData = v130a;
                enemy->laserActive = v130a;
                enemy->unk2e04 = v130a;
                if (enemy->bossMarkerIdx == 0 && ((enemy->flags >> 1) & 1))
                {
                    for (v130i = 0; v130i < 8; v130i++)
                        g_Gui.SetBossLifeBarSegment(v130i, 0.0f, 0.0f);
                }
                goto skipInstr;
            case 157: // opcode 158 = 设置 Gui 数据 (v0, a1/laserData, a2/laserData) + 若 bit3 调 23110
                if (instr->paramMask & 0x1)
                    v157a = GetVarValue(enemy, instr->args[0].i);
                else
                    v157a = instr->args[0].i;
                v157v = v157a;
                if (instr->paramMask & 0x4)
                    v157b = GetVarValue(enemy, instr->args[2].i);
                else
                    v157b = instr->args[2].i;
                if (instr->paramMask & 0x2)
                    v157c = GetVarValue(enemy, instr->args[1].i);
                else
                    v157c = instr->args[1].i;
                g_Gui.SetBossLifeBarSegment(v157v, (f32)v157c / (f32)enemy->laserData,
                                            (f32)v157b / (f32)enemy->laserData);
                if (instr->paramMask & 0x8)
                {
                    if (instr->paramMask & 0x8)
                        v157d = GetVarValue(enemy, instr->args[3].i);
                    else
                        v157d = instr->args[3].i;
                    g_Gui.SetBossLifeSegmentColor(v157v, v157d);
                }
                goto skipInstr;
            case 121: // opcode 122 = 设置 ECL 全局对象数据 (SetEclGlobalData)
                SetEclGlobalData(enemy, instr);
                goto skipInstr;
            case 122: // opcode 123 = ECL 全局对象更新 (RunEclGlobal)
                RunEclGlobal(enemy, instr);
                goto skipInstr;
            case 131: // opcode 132 = 设置 unk2e14 计时器
                if (instr->paramMask & 0x1)
                    v131a = GetVarValue(enemy, instr->args[0].i);
                else
                    v131a = instr->args[0].i;
                enemy->unk2e14.SetCurrent(v131a);
                goto skipInstr;
            case 132: // opcode 133 = 设置 unk3358/unk3368 (boss 模式只写 3358)
                if (!IS_BOSS_MODE())
                {
                    if (instr->paramMask & 0x2)
                        v132a = GetVarValue(enemy, instr->args[1].i);
                    else
                        v132a = instr->args[1].i;
                    if (instr->paramMask & 0x1)
                        v132b = GetVarValue(enemy, instr->args[0].i);
                    else
                        v132b = instr->args[0].i;
                    enemy->unk3358[v132b] = v132a;
                    if (instr->paramMask & 0x4)
                        v132c = GetVarValue(enemy, instr->args[2].i);
                    else
                        v132c = instr->args[2].i;
                    if (instr->paramMask & 0x1)
                        v132d = GetVarValue(enemy, instr->args[0].i);
                    else
                        v132d = instr->args[0].i;
                    enemy->unk3368[v132d] = v132c;
                }
                else
                {
                    if (instr->paramMask & 0x2)
                        v132e = GetVarValue(enemy, instr->args[1].i);
                    else
                        v132e = instr->args[1].i;
                    if (instr->paramMask & 0x1)
                        v132f = GetVarValue(enemy, instr->args[0].i);
                    else
                        v132f = instr->args[0].i;
                    enemy->unk3358[v132f] = v132e;
                }
                goto skipInstr;
            case 133: // opcode 134 = 设置 unk3378/unk337c (boss 模式只写 3378) + unk2e14 重置
                if (!IS_BOSS_MODE())
                {
                    if (instr->paramMask & 0x1)
                        v133a = GetVarValue(enemy, instr->args[0].i);
                    else
                        v133a = instr->args[0].i;
                    enemy->unk3378 = v133a;
                    if (instr->paramMask & 0x2)
                        v133b = GetVarValue(enemy, instr->args[1].i);
                    else
                        v133b = instr->args[1].i;
                    enemy->unk337c = v133b;
                }
                else
                {
                    if (instr->paramMask & 0x1)
                        v133c = GetVarValue(enemy, instr->args[0].i);
                    else
                        v133c = instr->args[0].i;
                    enemy->unk3378 = v133c;
                }
                enemy->unk2e14.SetCurrent(0);
                goto skipInstr;
            case 134: // opcode 135 = 分配/释放数据缓冲 (dataSlots[v0])
                if (instr->paramMask & 0x1)
                    v134a = GetVarValue(enemy, instr->args[0].i);
                else
                    v134a = instr->args[0].i;
                arg = v134a;
                if (enemy->dataSlots[arg] != 0)
                    g_ZunMemory.Free(enemy->dataSlots[arg]);
                enemy->dataSlots[arg] = NULL;
                if (instr->paramMask & 0x2)
                    v134b = GetVarValue(enemy, instr->args[1].i);
                else
                    v134b = instr->args[1].i;
                if (v134b >= 0)
                {
                    enemy->dataSlots[arg] = (EclDataSlot *)g_ZunMemory.Alloc(sizeof(EclDataSlot), "ECLInt");
                    if (enemy->dataSlots[arg] != 0)
                    {
                        memset(enemy->dataSlots[arg], 0, 0x24b0);
                        if (instr->paramMask & 0x2)
                            v134c = GetVarValue(enemy, instr->args[1].i);
                        else
                            v134c = instr->args[1].i;
                        enemy->dataSlots[arg]->subId = v134c;
                        g_EclInterruptTable.SetupEclContext(&enemy->dataSlots[arg]->context,
                                                                   (i16)enemy->dataSlots[arg]->subId);
                        memcpy(&enemy->dataSlots[arg]->context.eclContextArgs,
                               &enemy->curContextPtr->eclContextArgs, 0x1e * 4);
                    }
                }
                goto skipInstr;
            case 138: // opcode 139 = 生成特效 (位置 enemy->pos)
                if (instr->paramMask & 0x2)
                    v138a = GetVarValue(enemy, instr->args[1].i);
                else
                    v138a = instr->args[1].i;
                if (instr->paramMask & 0x1)
                    v138b = GetVarValue(enemy, instr->args[0].i);
                else
                    v138b = instr->args[0].i;
                g_EffectManager.FUN_00425430(v138b, &enemy->pos, v138a,
                                              *GetIntPtr(enemy, &instr->args[2], instr->paramMask, 2));
                goto skipInstr;
            case 139: // opcode 140 = 生成特效 (含局部位置 Float3)
                {
                    Float3 localPos;
                    if (instr->paramMask & 0x8)
                        v139a = enemy->GetEclFloatVar(instr->args[3].i);
                    else
                        v139a = instr->args[3].f;
                    localPos.x = v139a;
                    if (instr->paramMask & 0x10)
                        v139b = enemy->GetEclFloatVar(instr->args[4].i);
                    else
                        v139b = instr->args[4].f;
                    localPos.y = v139b;
                    if (instr->paramMask & 0x20)
                        v139c = enemy->GetEclFloatVar(instr->args[5].i);
                    else
                        v139c = instr->args[5].f;
                    localPos.z = v139c;
                    if (instr->paramMask & 0x2)
                        v139d = GetVarValue(enemy, instr->args[1].i);
                    else
                        v139d = instr->args[1].i;
                    if (instr->paramMask & 0x1)
                        v139e = GetVarValue(enemy, instr->args[0].i);
                    else
                        v139e = instr->args[0].i;
                    g_EffectManager.FUN_00425650(v139e, &enemy->pos, &localPos, v139d,
                                                  *GetIntPtr(enemy, &instr->args[2], instr->paramMask, 2));
                }
                goto skipInstr;
            case 142: // opcode 143 = 设置 unk3304
                if (instr->paramMask & 0x1)
                    v142a = GetVarValue(enemy, instr->args[0].i);
                else
                    v142a = instr->args[0].i;
                enemy->unk3304 = v142a;
                goto skipInstr;
            case 143: // opcode 144 = 设置 unk3308/330c
                if (instr->paramMask & 0x1)
                    v143a = GetVarValue(enemy, instr->args[0].i);
                else
                    v143a = instr->args[0].i;
                enemy->unk3308 = v143a;
                if (instr->paramMask & 0x2)
                    v143b = GetVarValue(enemy, instr->args[1].i);
                else
                    v143b = instr->args[1].i;
                enemy->unk330c = v143b;
                goto skipInstr;
            case 141: // opcode 142 = 随机撒物品 (依 power 决定类型)
                if (instr->paramMask & 0x1)
                    v141a = GetVarValue(enemy, instr->args[0].i);
                else
                    v141a = instr->args[0].i;
                v141cnt = v141a;
                for (v141i = 0; v141i < v141cnt; v141i++)
                {
                    v141x = enemy->pos.x;
                    v141y = enemy->pos.y;
                    v141z = enemy->pos.z;
                    v141b = &((Float3 *)&v141x)->x;
                    *v141b += g_Rng.GetRandomF32() * *(f32 *)ECL_ITEM_SCATTER_RANGE - *(f32 *)ECL_ITEM_SCATTER_OFFSET; // *128 - 32
                    v141c = &((Float3 *)&v141x)->y;
                    *v141c += g_Rng.GetRandomF32() * *(f32 *)ECL_ITEM_SCATTER_RANGE - *(f32 *)ECL_ITEM_SCATTER_OFFSET;
                    if (g_GameManager.GetPower() < 0x80)
                        g_ItemManager.SpawnItem((Float3 *)&v141x, (ItemType)(v141i == 0 ? 2 : 0), 0);
                    else
                        g_ItemManager.SpawnItem((Float3 *)&v141x, (ItemType)1, 0);
                }
                goto skipInstr;
            case 167: // opcode 168 = 随机撒物品 (固定类型 1)
                if (instr->paramMask & 0x1)
                    v167a = GetVarValue(enemy, instr->args[0].i);
                else
                    v167a = instr->args[0].i;
                v167cnt = v167a;
                for (v167i = 0; v167i < v167cnt; v167i++)
                {
                    v167x = enemy->pos.x;
                    v167y = enemy->pos.y;
                    v167z = enemy->pos.z;
                    v167b = &((Float3 *)&v167x)->x;
                    *v167b += g_Rng.GetRandomF32() * *(f32 *)ECL_ITEM_SCATTER_RANGE - *(f32 *)ECL_ITEM_SCATTER_OFFSET; // *128 - 32
                    v167c = &((Float3 *)&v167x)->y;
                    *v167c += g_Rng.GetRandomF32() * *(f32 *)ECL_ITEM_SCATTER_RANGE - *(f32 *)ECL_ITEM_SCATTER_OFFSET;
                    g_ItemManager.SpawnItem((Float3 *)&v167x, (ItemType)1, 0);
                }
                goto skipInstr;
            case 144: // opcode 145 = 设置 flags bit25
                enemy->flags = (enemy->flags & ~0x2000000) | ((instr->args[0].b[0] & 1) << 0x19);
                goto skipInstr;
            case 135: // opcode 136 = 间接调用 ECL ex-instr 表项 (g_EclExInsn[v0], 无参数)
                if (instr->paramMask & 0x1)
                    v135a = GetVarValue(enemy, instr->args[0].i);
                else
                    v135a = instr->args[0].i;
                ((void (*)())g_EclExInsn[v135a])();
                goto skipInstr;
            case 136: // opcode 137 = 设置 curContextPtr->func/eclExInstr (依函数表 0x4c6cb0)
                if (instr->paramMask & 0x1)
                    v136a = GetVarValue(enemy, instr->args[0].i);
                else
                    v136a = instr->args[0].i;
                if (v136a >= 0)
                {
                    if (instr->paramMask & 0x1)
                        v136b = GetVarValue(enemy, instr->args[0].i);
                    else
                        v136b = instr->args[0].i;
                    enemy->curContextPtr->func = g_EclExInsn[v136b];
                    enemy->curContextPtr->eclExInstr = instr;
                }
                else
                {
                    enemy->curContextPtr->func = NULL;
                }
                goto skipInstr;
            case 145: // opcode 146 = curContextPtr->time += v0
                if (instr->paramMask & 0x1)
                    v145a = GetVarValue(enemy, instr->args[0].i);
                else
                    v145a = instr->args[0].i;
                enemy->curContextPtr->time += v145a;
                goto skipInstr;
            case 140: // opcode 141 = 生成物品
                if (instr->paramMask & 0x1)
                    v140a = GetVarValue(enemy, instr->args[0].i);
                else
                    v140a = instr->args[0].i;
                g_ItemManager.SpawnItem(&enemy->pos, (ItemType)v140a, 0);
                goto skipInstr;
            case 146: // opcode 147 = 写全局 0x4ea290
                if (instr->paramMask & 0x1)
                    v146a = GetVarValue(enemy, instr->args[0].i);
                else
                    v146a = instr->args[0].i;
                g_BossPhaseState = v146a;
                goto skipInstr;
            case 147: // opcode 148 = g_Gui.SetEclLives(v0); 全局 0x164d30c += 0x708
                if (instr->paramMask & 0x1)
                    v147a = GetVarValue(enemy, instr->args[0].i);
                else
                    v147a = instr->args[0].i;
                g_Gui.SetEclLives(v147a);
                g_164d30c += 0x708;
                goto skipInstr;
            case 92: // opcode 93 = SPAWN_ENEMY_ABS
                if (enemy->laserActive > 0)
                {
                    *(EnemySpawnData *)&v92d0 = *(EnemySpawnData *)&instr->args[0];
                    if (instr->paramMask & 0x2)
                        v92a = enemy->GetEclFloatVar(((EnemySpawnData *)&v92d0)->pos.x);
                    else
                        v92a = ((EnemySpawnData *)&v92d0)->pos.x;
                    ((Float3 *)&v92px)->x = v92a;
                    if (instr->paramMask & 0x4)
                        v92b = enemy->GetEclFloatVar(((EnemySpawnData *)&v92d0)->pos.y);
                    else
                        v92b = ((EnemySpawnData *)&v92d0)->pos.y;
                    ((Float3 *)&v92px)->y = v92b;
                    if (instr->paramMask & 0x8)
                        v92c = enemy->GetEclFloatVar(((EnemySpawnData *)&v92d0)->pos.z);
                    else
                        v92c = ((EnemySpawnData *)&v92d0)->pos.z;
                    ((Float3 *)&v92px)->z = v92c;
                    if (instr->paramMask & 0x40)
                        v92d = GetVarValue(enemy, instr->args[6].i);
                    else
                        v92d = instr->args[6].i;
                    if (instr->paramMask & 0x20)
                        v92e = GetVarValue(enemy, instr->args[5].i);
                    else
                        v92e = instr->args[5].i;
                    if (instr->paramMask & 0x10)
                        v92f = GetVarValue(enemy, instr->args[4].i);
                    else
                        v92f = instr->args[4].i;
                    v92res = (i32)g_EnemyManager.SpawnEnemy2(((EnemySpawnData *)&v92d0)->subId,
                                               (Float3 *)&v92px, v92f, v92e, v92d,
                                               &enemy->curContextPtr->eclContextArgs);
                }
                goto skipInstr;
            case 93: // opcode 94 = SPAWN_ENEMY_REL (pos 相对敌人)
                if (enemy->laserActive > 0)
                {
                    *(EnemySpawnData *)&v93d0 = *(EnemySpawnData *)&instr->args[0];
                    if (instr->paramMask & 0x2)
                        v93a = enemy->GetEclFloatVar(((EnemySpawnData *)&v93d0)->pos.x);
                    else
                        v93a = ((EnemySpawnData *)&v93d0)->pos.x;
                    ((Float3 *)&v93px)->x = v93a;
                    if (instr->paramMask & 0x4)
                        v93b = enemy->GetEclFloatVar(((EnemySpawnData *)&v93d0)->pos.y);
                    else
                        v93b = ((EnemySpawnData *)&v93d0)->pos.y;
                    ((Float3 *)&v93px)->y = v93b;
                    if (instr->paramMask & 0x8)
                        v93c = enemy->GetEclFloatVar(((EnemySpawnData *)&v93d0)->pos.z);
                    else
                        v93c = ((EnemySpawnData *)&v93d0)->pos.z;
                    ((Float3 *)&v93px)->z = v93c;
                    *(Float3 *)&v93px += enemy->pos;
                    if (instr->paramMask & 0x40)
                        v93d = GetVarValue(enemy, instr->args[6].i);
                    else
                        v93d = instr->args[6].i;
                    if (instr->paramMask & 0x20)
                        v93e = GetVarValue(enemy, instr->args[5].i);
                    else
                        v93e = instr->args[5].i;
                    if (instr->paramMask & 0x10)
                        v93f = GetVarValue(enemy, instr->args[4].i);
                    else
                        v93f = instr->args[4].i;
                    v93res = (i32)g_EnemyManager.SpawnEnemy2(((EnemySpawnData *)&v93d0)->subId,
                                               (Float3 *)&v93px, v93f, v93e, v93d,
                                               &enemy->curContextPtr->eclContextArgs);
                }
                goto skipInstr;
            case 94: // opcode 95 = 遍历删敌人 (g_EnemyManager.RemoveEnemiesByScore)
                g_EnemyManager.RemoveEnemiesByScore(0x1f40, 0);
                goto skipInstr;
            case 148: // opcode 149 = 设置 primaryVm 挂起中断
                if (instr->paramMask & 0x1)
                    v148a = GetVarValue(enemy, instr->args[0].i);
                else
                    v148a = instr->args[0].i;
                enemy->primaryVm.prefix.pendingInterrupt = (i16)v148a;
                goto skipInstr;
            case 149: // opcode 150 = 设置 vms[idx] 挂起中断
                {
                    i32 idx = instr->args[0].i;
                    enemy->vms[idx].prefix.pendingInterrupt = (i16)instr->args[1].i;
                }
                goto skipInstr;
            case 111: // opcode 112 = RemoveAllBullets(1)
                g_BulletManager.bulletmanager_fun_00415c60();
                goto skipInstr;
            case 112: // opcode 113 = 生命回调阈值/子脚本: lifeCallbackState/24/28
                if (instr->paramMask & 0x1)
                    v112a = GetVarValue(enemy, instr->args[0].i);
                else
                    v112a = instr->args[0].i;
                if (v112a >= 0)
                {
                    if (instr->paramMask & 0x1)
                        v112b = GetVarValue(enemy, instr->args[0].i);
                    else
                        v112b = instr->args[0].i;
                    enemy->lifeCallbackThreshold = v112b;
                    enemy->lifeCallbackState |= 0x200;
                }
                else
                {
                    enemy->lifeCallbackState &= ~0x200;
                }
                if (instr->paramMask & 0x2)
                    v112c = GetVarValue(enemy, instr->args[1].i);
                else
                    v112c = instr->args[1].i;
                enemy->lifeCallbackSub = v112c;
                goto skipInstr;
            case 150: // opcode 151 = 设置 flags bit26
                enemy->flags = (enemy->flags & ~ECL_FLAG_NO_SAVE_ON_INTERRUPT) | ((instr->args[0].b[0] & 1) << 0x1a);
                goto skipInstr;
            case 151: // opcode 152 = 写移动界限字段 unk2dec/2df0/2df4..2dfa
                if (instr->paramMask & 0x1)
                    v151a = enemy->GetEclFloatVar(instr->args[0].i);
                else
                    v151a = instr->args[0].f;
                enemy->unk2dec = v151a;
                if (instr->paramMask & 0x2)
                    v151b = enemy->GetEclFloatVar(instr->args[1].i);
                else
                    v151b = instr->args[1].f;
                enemy->unk2df0 = v151b;
                if (instr->paramMask & 0x4)
                    v151c = GetVarValue(enemy, instr->args[2].i);
                else
                    v151c = instr->args[2].i;
                enemy->unk2df4 = (i16)v151c;
                if (instr->paramMask & 0x8)
                    v151d = GetVarValue(enemy, instr->args[3].i);
                else
                    v151d = instr->args[3].i;
                enemy->unk2df6 = (i16)v151d;
                if (instr->paramMask & 0x10)
                    v151e = GetVarValue(enemy, instr->args[4].i);
                else
                    v151e = instr->args[4].i;
                enemy->unk2df8 = (i16)v151e;
                if (instr->paramMask & 0x20)
                    v151f = GetVarValue(enemy, instr->args[5].i);
                else
                    v151f = instr->args[5].i;
                enemy->unk2dfa = (i16)v151f;
                goto skipInstr;
            case 152: // opcode 153 = unk337c = unk2cee; unk2e14.SetCurrent(0)
                enemy->unk337c = enemy->unk2cee;
                enemy->unk2e14.SetCurrent(0);
                goto skipInstr;
            case 154: // opcode 155 = 设置 flags bit27 + 写保存时间戳全局
                enemy->flags = (enemy->flags & ~ECL_FLAG_UNK27) | ((instr->args[0].b[0] & 1) << 0x1b);
                g_4ecca8 = ECL_SCORE_SAVE_MAGIC;
                goto skipInstr;
            case 155: // opcode 156 = 设置 flags bit7 + unk332f=2
                enemy->flags = (enemy->flags & ~0x80) | ((instr->args[0].b[0] & 1) << 7);
                enemy->unk332f = 2;
                goto skipInstr;
            case 156: // opcode 157 = 设置 AI 字段 unk534c/534e/5350/5352 + 若 bit3 调 AnmManager
                enemy->unk534c = instr->args[0].b[0];
                if (instr->paramMask & 0x2)
                    v156a = GetVarValue(enemy, instr->args[1].i);
                else
                    v156a = instr->args[1].i;
                enemy->unk534e = (i16)v156a;
                if (instr->paramMask & 0x4)
                    v156b = GetVarValue(enemy, instr->args[2].i);
                else
                    v156b = instr->args[2].i;
                enemy->unk5350 = (i16)v156b;
                if (instr->paramMask & 0x8)
                    v156c = GetVarValue(enemy, instr->args[3].i);
                else
                    v156c = instr->args[3].i;
                enemy->unk5352 = (i16)v156c;
                if (enemy->unk534c & 0x8)
                {
                    g_AnmManager->SetupSpriteStrip(&enemy->primaryVm, (void *)&enemy->eclContext,
                                                   (i32)(((i32)(i16)enemy->unk5352 / (i32)(i16)enemy->unk534e) << 1));
                }
                goto skipInstr;
            case 159: // opcode 160 = unk5354 ZunTimer.SetCurrent(v0)
                if (instr->paramMask & 0x1)
                    v159a = GetVarValue(enemy, instr->args[0].i);
                else
                    v159a = instr->args[0].i;
                enemy->unk5354.SetCurrent(v159a);
                goto skipInstr;
            case 160: // opcode 161 = 清除 movePos 半径内子弹 (g_BulletManager.ClearBulletsInRadius)
                if (instr->paramMask & 0x1)
                    v160a = enemy->GetEclFloatVar(instr->args[0].i);
                else
                    v160a = instr->args[0].f;
                g_BulletManager.ClearBulletsInRadius(&enemy->movePos, v160a);
                goto skipInstr;
            case 161: // opcode 162 = RemoveAllBullets(4)
                g_BulletManager.RemoveAllBullets(4);
                goto skipInstr;
            case 163: // opcode 164 = 设置全局标志 + 目标位置
                if (instr->paramMask & 0x1)
                    v163a = GetVarValue(enemy, instr->args[0].i);
                else
                    v163a = instr->args[0].i;
                arg = v163a;
                g_EclGlobalObj.SetGlobalFlag(arg);
                if (arg == 0)
                {
                    if (instr->paramMask & 0x8)
                        v163b = enemy->GetEclFloatVar(instr->args[3].i);
                    else
                        v163b = instr->args[3].f;
                    if (instr->paramMask & 0x4)
                        v163c = enemy->GetEclFloatVar(instr->args[2].i);
                    else
                        v163c = instr->args[2].f;
                    if (instr->paramMask & 0x2)
                        v163d = enemy->GetEclFloatVar(instr->args[1].i);
                    else
                        v163d = instr->args[1].f;
                    g_EclGlobalObj.SetTargetPos(v163d, v163c, v163b);
                }
                goto skipInstr;
            case 164: // opcode 165 = 写 primaryVm Z 旋转 (prefix.rotation.z)
                if (instr->paramMask & 0x1)
                    v164a = enemy->GetEclFloatVar(instr->args[0].i);
                else
                    v164a = instr->args[0].f;
                enemy->primaryVm.prefix.rotation.z = v164a;
                goto skipInstr;
            case 165: // opcode 166 = 极坐标→直角: *float[1]=sin(f2)*f3; *float[0]=cos(f2)*f3
                if (instr->paramMask & 0x4)
                    v165a = enemy->GetEclFloatVar(instr->args[2].i);
                else
                    v165a = instr->args[2].f;
                if (instr->paramMask & 0x8)
                    v165b = enemy->GetEclFloatVar(instr->args[3].i);
                else
                    v165b = instr->args[3].f;
                v165c = sinf(v165a) * v165b;
                *GetFloatPtr(enemy, &instr->args[1], instr->paramMask, 0) = v165c;
                if (instr->paramMask & 0x4)
                    v165d = enemy->GetEclFloatVar(instr->args[2].i);
                else
                    v165d = instr->args[2].f;
                if (instr->paramMask & 0x8)
                    v165e = enemy->GetEclFloatVar(instr->args[3].i);
                else
                    v165e = instr->args[3].f;
                v165f = cosf(v165d) * v165e;
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = v165f;
                goto skipInstr;
            case 168: // opcode 169 = 依 pos.x 位置阈值决定随机角度 (出口角度)
                {
                    // 位置阈值: 96.0 (ECL_EXIT_ANGLE_X_LOW) / 288.0 (ECL_EXIT_ANGLE_X_HIGH) / 左边界
                    // 角度偏移: +3π/4 (ECL_EXIT_ANGLE_ADD≈2.356) 或 -π/4 (ECL_EXIT_ANGLE_SUB≈0.785)
                    f32 *out = GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0);
                    f32 exitLeftBound = *(f32 *)ECL_EXIT_LEFT_BOUND;
                    if ((enemy->pos.x > exitLeftBound || enemy->pos.x < exitLeftBound) && enemy->pos.x > *(f32 *)ECL_EXIT_ANGLE_X_LOW)
                    {
                        *out = AddNormalizeAngle(g_Rng.GetRandomF32InRange(ECL_RANDOM_ANGLE_RANGE) + *(f32 *)ECL_EXIT_ANGLE_ADD, 0.0f);
                    }
                    else if (enemy->pos.x > *(f32 *)ECL_EXIT_ANGLE_X_HIGH)
                    {
                        *out = AddNormalizeAngle(g_Rng.GetRandomF32InRange(ECL_RANDOM_ANGLE_RANGE) + *(f32 *)ECL_EXIT_ANGLE_ADD, 0.0f);
                    }
                    else
                    {
                        *out = g_Rng.GetRandomF32InRange(ECL_RANDOM_ANGLE_RANGE) - *(f32 *)ECL_EXIT_ANGLE_SUB;
                    }
                }
                goto skipInstr;
            case 172: // opcode 173 = 设置 flags bit30
                if (instr->paramMask & 0x1)
                    v172a = GetVarValue(enemy, instr->args[0].i);
                else
                    v172a = instr->args[0].i;
                enemy->flags = (enemy->flags & ~ECL_FLAG_SCORE_MODE) | ((v172a & 1) << 0x1e);
                goto skipInstr;
            case 182: // opcode 183 = 设置 flags bit31
                if (instr->paramMask & 0x1)
                    v182a = GetVarValue(enemy, instr->args[0].i);
                else
                    v182a = instr->args[0].i;
                enemy->flags = (enemy->flags & ~ECL_FLAG_UNK31) | ((v182a & 1) << 0x1f);
                goto skipInstr;
            case 175: // opcode 176 = 全局弹幕/特殊事件状态 + flags bit30
                g_PlayerFlags |= 0x80;
                g_PlayerFlags &= ~0x2000;
                if (!(g_PlayerFlags & 0x4000))
                {
                    if (g_Unknown164d2cc == GAME_STATE_EVENT_6 || g_Unknown164d2cc == GAME_STATE_EVENT_7)
                        g_PlayerFlags |= 0x2000;
                }
                else if ((g_CurrentSpellcardNumber >= 0x8f && g_CurrentSpellcardNumber <= 0x92) ||
                         (g_CurrentSpellcardNumber >= 0xab && g_CurrentSpellcardNumber <= 0xbe))
                {
                    g_PlayerFlags |= 0x2000;
                }
                enemy->flags |= ECL_FLAG_SCORE_MODE;
                goto skipInstr;
            case 81: // opcode 82 = 速度平方: unk3350 = f0*f0
                if (instr->paramMask & 0x1)
                    v81a = enemy->GetEclFloatVar(instr->args[0].i);
                else
                    v81a = instr->args[0].f;
                enemy->unk3350 = v81a;
                enemy->unk3350 *= enemy->unk3350;
                goto skipInstr;
            case 82: // opcode 83 = 设置 anmFlags bit1
                if (instr->paramMask & 0x1)
                    v82a = GetVarValue(enemy, instr->args[0].i);
                else
                    v82a = instr->args[0].i;
                enemy->anmFlags = (enemy->anmFlags & ~ECL_ANM_FLAG_ISYOUKAI) | ((v82a & 1) << 1);
                goto skipInstr;
            case 83: case 84: // opcode 84/85 = NOP
                goto skipInstr;
            case 173: // opcode 174 = 生成 boss 特效 (AllocEffectSlot + SetInterrupt)
                if (enemy->unk53c8 != 0)
                    ((EffectManagerParticle *)enemy->unk53c8)->unk350 = 0;
                if (instr->paramMask & 0x1)
                    v173a = GetVarValue(enemy, instr->args[0].i);
                else
                    v173a = instr->args[0].i;
                enemy->unk53c8 =
                    (u32)g_EffectManager.AllocEffectSlot(v173a + 0x20, &enemy->movePos, 1, -1);
                ((AnmVm *)enemy->unk53c8)->SetInterrupt(g_Player.IsYoukai() ? 2 : 1);
                if (enemy->unk2e0c & 1)
                    ((AnmVm *)enemy->unk53c8)->prefix.angleVel.z =
                        -((AnmVm *)enemy->unk53c8)->prefix.angleVel.z;
                goto skipInstr;
            case 174: // opcode 175 = 写全局 0xf54e2c
                if (instr->paramMask & 0x1)
                    v174a = GetVarValue(enemy, instr->args[0].i);
                else
                    v174a = instr->args[0].i;
                g_BulletSpawnFlag2 = v174a;
                goto skipInstr;
            case 176: // opcode 177 = 设置 unk2e04
                if (instr->paramMask & 0x1)
                    v176a = GetVarValue(enemy, instr->args[0].i);
                else
                    v176a = instr->args[0].i;
                enemy->unk2e04 = v176a;
                goto skipInstr;
            case 178: // opcode 179 = 显示时钟 (ShowClock)
                g_Gui.ShowClock();
                goto skipInstr;
            case 179: // opcode 180 = 重置时钟显示 (ResetClock)
                g_Gui.ResetClock();
                goto skipInstr;
            case 180: // opcode 181 = 若时钟<12h 播声+加时钟+依是否=12h 调 Gui
                if (g_GameManager.GetClockTime() < 0xc)
                {
                    g_SoundPlayer.PlaySoundByIdx((SoundIdx)ECL_SOUND_CLOCK_CHIME, 0);
                    g_GameManager.AddToClockTime(1);
                    if (g_GameManager.GetClockTime() == 0xc)
                        g_Gui.UpdateClockNoon();
                    else
                        g_Gui.UpdateClockHour();
                }
                goto skipInstr;
            case 181: // opcode 182 = 设置 anmFlags bit8
                if (instr->paramMask & 0x1)
                    v181a = GetVarValue(enemy, instr->args[0].i);
                else
                    v181a = instr->args[0].i;
                enemy->anmFlags = (enemy->anmFlags & ~ECL_ANM_FLAG_ANM_LOADED) | ((v181a & 1) << 8);
                goto skipInstr;
            case 183: // opcode 184 = g_EclGlobalObj.SetGlobalFlag2(ECL_IVAL(0))
                if (instr->paramMask & 0x1)
                    v183a = GetVarValue(enemy, instr->args[0].i);
                else
                    v183a = instr->args[0].i;
                g_EclGlobalObj.SetGlobalFlag2(v183a);
                goto skipInstr;
            case 2: // ECL_NOP
                goto skipInstr;
            default:
                goto skipInstr;
            }
            }
        }
    skipInstr:
        instr = (EclRawInstr *)((u8 *)instr + instr->size);
    }

#undef ECL_IVAL
#undef ECL_FVAL

handleInterrupt:
    // Save the current context and run the interrupt subroutine.
    enemy->curContextPtr->curInstr = (EclRawInstr *)((u8 *)instr + instr->size);
    if (((enemy->flags >> 0x1a) & 1) == 0)
    {
        enemy->savedContextStack[enemy->stackDepth] = enemy->eclContext;
    }
    if (enemy->stackDepth < 0xf)
    {
        enemy->stackDepth++;
    }
    enemy->runInterrupt = -1;
    goto restart;

exit:
    // Laser/interp wind-down, then switch to any remaining sub-context.
    if (enemy->laserActive > 0)
    {
        flag = 0;
        interp = &enemy->curContextPtr->interps[0];
        savedPos = enemy->pos;
        if (enemy->curContextPtr->func != NULL)
        {
            enemy->curContextPtr->func(enemy, enemy->curContextPtr->eclExInstr);
        }
        for (iInterp = 0; iInterp < 8; iInterp++, interp++)
        {
            if (interp->fn != NULL)
            {
                i32 type;
                interp->timer.Tick();
                if (interp->timer >= interp->args[0].i)
                {
                    interp->timer.SetCurrent(interp->args[0].i);
                }
                t = interp->timer.AsFramesFloat() / (f32)interp->args[0].i;
                type = interp->args[2].i - 1;
                switch (type)
                {
                case 0:
                    t = t * t;
                    break;
                case 1:
                    t = t * t * t;
                    break;
                case 2:
                    t = t * t * t * t;
                    break;
                case 3:
                    t = 1.0f - (1.0f - t) * (1.0f - t);
                    break;
                case 4:
                    t = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
                    break;
                case 5:
                    t = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t) * (1.0f - t);
                    break;
                }
                interp->fn(enemy, interp, t);
                if (interp->timer >= interp->args[0].i)
                {
                    interp->fn = NULL;
                }
                if (interp->args[7].f != interp->args[7].f)
                {
                    flag = 1;
                }
            }
        }
        if (flag != 0)
        {
            enemy->unk2d4c = enemy->pos.x - savedPos.x;
            enemy->unk2d50 = enemy->pos.y - savedPos.y;
            enemy->moveAngle = EclAngleFromDxDy(enemy->unk2d4c, enemy->unk2d50);
            enemy->pos = savedPos;
        }
    }

    if (subCtxIdx == -1)
    {
        enemy->unk2ce8 = enemy->stackDepth;
    }
    else
    {
        enemy->dataSlots[subCtxIdx]->stackDepth = enemy->stackDepth;
    }
    enemy->curContextPtr->curInstr = instr;
    enemy->curContextPtr->time.SetCurrent(0);
    for (i = subCtxIdx + 1; i < 4; i++)
    {
        if (enemy->dataSlots[i] != NULL)
        {
            enemy->savedStackPtr = &enemy->dataSlots[i]->savedStack[0];
            enemy->curContextPtr = &enemy->dataSlots[i]->context;
            instr = enemy->curContextPtr->curInstr;
            enemy->curContextPtr->unk220 = i + 1;
            enemy->stackDepth = enemy->dataSlots[i]->stackDepth;
            subCtxIdx = i;
            goto restart;
        }
    }
    enemy->savedStackPtr = &enemy->savedContextStack[0];
    enemy->curContextPtr = &enemy->eclContext;
    enemy->UpdateEnemyMove();
    enemy->UpdateLaserScript();
    return ZUN_SUCCESS;
}

} // namespace th08
