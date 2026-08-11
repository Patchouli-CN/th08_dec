#include "th_pch.h"

#include "EclManager.hpp"
#include "EnemyManager.hpp"
#include "Global.hpp"
#include "GameManager.hpp"
#include "BulletManager.hpp"
#include "Player.hpp"
#include "SoundPlayer.hpp"
#include "Gui.hpp"

namespace th08
{

DIFFABLE_STATIC(EclManager, g_EclManager);

// ECL variable access helpers (th08 standalone functions; th07 had these as
// EclManager static methods). Stubs for now; RunEcl's call targets normalize to
// T in fn_diff so only the calling convention needs to be right here.
i32 __fastcall GetVarValue(Enemy *enemy, i32 varId);                       // 0x41f420
i32 *__fastcall GetIntPtr(Enemy *enemy, AnyArg *args, u16 paramMask);      // 0x41fe10
f32 *__fastcall GetFloatPtr(Enemy *enemy, AnyArg *args, u16 paramMask, i32 unused); // 0x420950
void __fastcall FUN_00421280(Enemy *enemy, EclRawInstr *instr);           // 0x421280
void __fastcall FUN_004212e0(Enemy *enemy, EclRawInstr *instr);           // 0x4212e0
void __fastcall FUN_00421300(Enemy *enemy, EclRawInstr *instr);           // 0x421300
void __fastcall FUN_004213f0(Enemy *enemy, EclRawInstr *instr);           // 0x4213f0
EclRawInstr *__fastcall FUN_004215f0(Enemy *enemy, EclRawInstr *instr);   // 0x4215f0 子脚本 (op40-51)
void __fastcall FUN_00420f40(Enemy *enemy, EclRawInstr *instr);           // 0x420f40
void __fastcall FUN_00421bd0(Enemy *enemy, EclRawInstr *instr, i32 arg0); // 0x421bd0
i32 __fastcall FUN_00421cb0(Enemy *enemy, EclRawInstr *instr);            // 0x421cb0
void __fastcall FUN_00421e50(Enemy *enemy, EclRawInstr *instr);           // 0x421e50
void __fastcall FUN_00422020(Enemy *enemy, EclRawInstr *instr);           // 0x422020
void __fastcall FUN_004224a0(Enemy *enemy, EclRawInstr *instr);           // 0x4224a0
void __fastcall FUN_00422720(Enemy *enemy, EclRawInstr *instr);           // 0x422720 (laser op96-104)

f32 __cdecl EclAtan2(f32 a, f32 b); // th08 0x41f090 (wraps CRT atan2)

i32 __fastcall GetVarValue(Enemy *enemy, i32 varId)
{
    return 0;
}

f32 __cdecl EclAtan2(f32 a, f32 b)
{
    return 0.0f;
}

f32 __fastcall EclAngleFromDxDy(f32 dx, f32 dy); // th08 0x40c7b0 (atan2 variant)

f32 __fastcall EclAngleFromDxDy(f32 dx, f32 dy)
{
    return 0.0f;
}

void Enemy::FUN_00421de0(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5)
{
}

void Enemy::FUN_0042c180()
{
}

void Enemy::FUN_00422c40()
{
}

void Enemy::FUN_00423150()
{
}

i32 *__fastcall GetIntPtr(Enemy *enemy, AnyArg *args, u16 paramMask)
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

void __fastcall FUN_00421280(Enemy *enemy, EclRawInstr *instr)
{
}

void __fastcall FUN_004212e0(Enemy *enemy, EclRawInstr *instr)
{
}

void __fastcall FUN_00421300(Enemy *enemy, EclRawInstr *instr)
{
}

void __fastcall FUN_004213f0(Enemy *enemy, EclRawInstr *instr)
{
}

EclRawInstr *__fastcall FUN_004215f0(Enemy *enemy, EclRawInstr *instr)
{
    return NULL;
}

void __fastcall FUN_00420f40(Enemy *enemy, EclRawInstr *instr)
{
}

void __fastcall FUN_00421bd0(Enemy *enemy, EclRawInstr *instr, i32 arg0)
{
}

i32 __fastcall FUN_00421cb0(Enemy *enemy, EclRawInstr *instr)
{
    return 0;
}

void __fastcall FUN_00421e50(Enemy *enemy, EclRawInstr *instr)
{
}

void __fastcall FUN_00422020(Enemy *enemy, EclRawInstr *instr)
{
}

void __fastcall FUN_004224a0(Enemy *enemy, EclRawInstr *instr)
{
}

void __fastcall FUN_00422720(Enemy *enemy, EclRawInstr *instr)
{
}

// FUNCTION: th08 0x418330
ZunResult EclManager::Load(const char *path)
{
    return ZUN_ERROR;
}

// FUNCTION: th08 0x4184b0 (逆向中)
#pragma var_order(arg, subCtxIdx, instr, i,                                                                           \
                  p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,                              \
                  p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34,                         \
                  p35, p36, p37, p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,                         \
                  p50, p51, p52, p53, p54, p55, p56, p57)
ZunResult EclManager::RunEcl(Enemy *enemy)
{
    EclRawInstr *instr;
    i32 arg;
    i32 subCtxIdx = -1;
    i32 i;
    i32 p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19;
    i32 p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34;
    i32 p35, p36, p37, p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49;
    i32 p50, p51, p52, p53, p54, p55, p56, p57;

    enemy->savedStackPtr = &enemy->savedContextStack[0];
    enemy->curContextPtr = &enemy->eclContext;
    enemy->stackDepth = enemy->unk2ce8;

restart:
    // ECL arg access: paramMask bit N set means arg N is a variable id.
#define ECL_IVAL(n) ((instr->paramMask & (1 << (n))) ? GetVarValue(enemy, instr->args[n].i) : instr->args[n].i)
#define ECL_FVAL(n) ((instr->paramMask & (1 << (n))) ? enemy->GetEclFloatVar(instr->args[n].i) : instr->args[n].f)
    // Boss-mode gate: bit14 (extra stage) AND bits7-8 set → several boss-only
    // ECL opcodes skip their non-boss write and take a shorter path.
#define IS_BOSS_MODE() ((g_PlayerFlags & 0x4000) && (((g_PlayerFlags >> 7) & 3) != 0))
    instr = enemy->curContextPtr->curInstr;
    if (enemy->runInterrupt >= 0)
    {
        goto handleInterrupt;
    }
    for (;;)
    {
        enemy->unk2d88 = enemy->pos + enemy->unk2d40;

        if ((i32)enemy->curContextPtr->waitTimer > 0)
        {
            enemy->curContextPtr->waitTimer--;
            enemy->curContextPtr->time--;
            goto exit;
        }
        if (enemy->curContextPtr->time == (i32)instr->time)
        {
            if ((instr->skipInstrOnDifficulty & (g_GameManager.difficultyMask | enemy->eclFlags)) == 0)
            {
                goto skipInstr;
            }
            switch (instr->id - 1)
            {
            case 0: // ECL_UNIMP
                return ZUN_ERROR;
            case 1: // ECL_SET_WAIT_TIMER: waitTimer = arg0
                enemy->curContextPtr->waitTimer.SetCurrent(ECL_IVAL(0));
                goto skipInstr;
            case 2: // ECL_NOP
                goto skipInstr;
            case 3: // ECL_JUMP: time = arg0; jump by arg1
                enemy->curContextPtr->time.current = instr->args[0].i;
                instr = (EclRawInstr *)((u8 *)instr + instr->args[1].i);
                continue;
            case 4: // ECL_DEC_JUMP: *arg2--; if (arg1 > 0) jump
                *GetIntPtr(enemy, &instr->args[2], instr->paramMask) -= 1;
                arg = ECL_IVAL(1);
                if (arg > 0)
                {
                    enemy->curContextPtr->time.current = instr->args[0].i;
                    instr = (EclRawInstr *)((u8 *)instr + instr->args[1].i);
                    continue;
                }
                goto skipInstr;
            case 5: // ECL_SET_INT: *arg0 = arg1
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) = ECL_IVAL(1);
                goto skipInstr;
            case 6: // ECL_SET_FLOAT: *arg0 = arg1
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = ECL_FVAL(1);
                goto skipInstr;
            case 7: // ECL_RAND_SIGN: *arg0 = ±arg1
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) =
                    (g_Rng.GetRandomU16() & 1 ? 1 : -1) * ECL_IVAL(1);
                goto skipInstr;
            case 8: // ECL_RAND_SIGN_FLOAT: *arg0 = ±arg1
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) =
                    (g_Rng.GetRandomU16() & 1 ? 1.0f : -1.0f) * ECL_FVAL(1);
                goto skipInstr;
            case 9: // ECL_ADD: *arg0 += arg1
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) += ECL_IVAL(1);
                goto skipInstr;
            case 10: // ECL_SUB: *arg0 -= arg1
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) -= ECL_IVAL(1);
                goto skipInstr;
            case 11: // ECL_MUL: *arg0 *= arg1
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) *= ECL_IVAL(1);
                goto skipInstr;
            case 12: // ECL_DIV: *arg0 /= arg1
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) /= ECL_IVAL(1);
                goto skipInstr;
            case 13: // ECL_MOD: *arg0 %= arg1
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) %= ECL_IVAL(1);
                goto skipInstr;
            case 14: // ECL_ADD_FLOAT: *arg0 += arg1
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) += ECL_FVAL(1);
                goto skipInstr;
            case 15: // ECL_SUB_FLOAT: *arg0 -= arg1
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) -= ECL_FVAL(1);
                goto skipInstr;
            case 16: // ECL_MUL_FLOAT: *arg0 *= arg1
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) *= ECL_FVAL(1);
                goto skipInstr;
            case 17: // ECL_DIV_FLOAT: *arg0 /= arg1
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) /= ECL_FVAL(1);
                goto skipInstr;
            case 18: // ECL_ATAN2: *arg0 = atan2(arg1, arg2)
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) =
                    EclAtan2(ECL_FVAL(1), ECL_FVAL(2));
                goto skipInstr;
            case 19: // ECL_SET_ADD: *arg0 = arg1 + arg2
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) =
                    ECL_IVAL(1) + ECL_IVAL(2);
                goto skipInstr;
            case 20: // ECL_SET_SUB: *arg0 = arg1 - arg2
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) =
                    ECL_IVAL(1) - ECL_IVAL(2);
                goto skipInstr;
            case 21: // ECL_SET_MUL: *arg0 = arg1 * arg2
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) =
                    ECL_IVAL(1) * ECL_IVAL(2);
                goto skipInstr;
            case 22: // ECL_SET_DIV: *arg0 = arg1 / arg2
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) =
                    ECL_IVAL(1) / ECL_IVAL(2);
                goto skipInstr;
            case 23: // ECL_SET_MOD: *arg0 = arg1 %% arg2
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) =
                    ECL_IVAL(1) % ECL_IVAL(2);
                goto skipInstr;
            case 24: // ECL_SET_ADD_FLOAT: *arg0 = arg1 + arg2
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) =
                    ECL_FVAL(1) + ECL_FVAL(2);
                goto skipInstr;
            case 25: // ECL_SET_SUB_FLOAT: *arg0 = arg1 - arg2
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) =
                    ECL_FVAL(1) - ECL_FVAL(2);
                goto skipInstr;
            case 26: // ECL_SET_MUL_FLOAT: *arg0 = arg1 * arg2
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) =
                    ECL_FVAL(1) * ECL_FVAL(2);
                goto skipInstr;
            case 27: // ECL_SET_DIV_FLOAT: *arg0 = arg1 / arg2
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) =
                    ECL_FVAL(1) / ECL_FVAL(2);
                goto skipInstr;
            case 28: // ECL_ATAN2_SWAP: *arg0 = atan2(arg2, arg1) (args 1/2 swapped)
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) =
                    EclAtan2(ECL_FVAL(2), ECL_FVAL(1));
                goto skipInstr;
            case 29: // ECL_INC: *arg0 += 1
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) += 1;
                goto skipInstr;
            case 30: // ECL_DEC: *arg0 -= 1
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) -= 1;
                goto skipInstr;
            case 31: // ECL_SIN: *arg0 = sin(arg1)
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = sinf(ECL_FVAL(1));
                goto skipInstr;
            case 32: // ECL_COS: *arg0 = cos(arg1)
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = cosf(ECL_FVAL(1));
                goto skipInstr;
            case 33: // opcode 34 = 两点角度: *float[0]=EclAngleFromDxDy(f4-f2, f3-f1)
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) =
                    EclAngleFromDxDy(ECL_FVAL(4) - ECL_FVAL(2), ECL_FVAL(3) - ECL_FVAL(1));
                goto skipInstr;
            case 34: // opcode 35 = 子脚本 (FUN_00421300)
                FUN_00421300(enemy, instr);
                goto skipInstr;
            case 35: // opcode 36 = 子脚本 (FUN_004213f0)
                FUN_004213f0(enemy, instr);
                goto skipInstr;
            case 36: // opcode 37 = ECL_NORMALIZE_ANGLE: *float[0] = AddNormalizeAngle(f1, f2)
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) =
                    AddNormalizeAngle(ECL_FVAL(1), ECL_FVAL(2));
                goto skipInstr;
            case 37: // opcode 38 = 角度转坐标: *f[0]=cos(a)*m; *f[1]=sin(a)*m
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) =
                    cosf(AddNormalizeAngle(ECL_FVAL(2), 0)) * ECL_FVAL(3);
                *GetFloatPtr(enemy, &instr->args[1], instr->paramMask, 0) =
                    sinf(AddNormalizeAngle(ECL_FVAL(2), 0)) * ECL_FVAL(3);
                goto skipInstr;
            case 38: // opcode 39 = 两点距离: *float[0]=sqrt((f1-f3)^2+(f2-f4)^2)
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) =
                    sqrtf((ECL_FVAL(1) - ECL_FVAL(3)) * (ECL_FVAL(1) - ECL_FVAL(3)) +
                          (ECL_FVAL(2) - ECL_FVAL(4)) * (ECL_FVAL(2) - ECL_FVAL(4)));
                goto skipInstr;
            case 39: case 40: case 41: case 42: case 43: case 44:
            case 45: case 46: case 47: case 48: case 49: case 50:
                // opcode 40-51: 子脚本处理 (FUN_004215f0 内部再分派)
                {
                    EclRawInstr *subInstr = FUN_004215f0(enemy, instr);
                    if (subInstr != NULL)
                    {
                        instr = subInstr;
                        continue;
                    }
                }
                goto skipInstr;
            case 51: // opcode 52 = 子脚本上下文切换 (FUN_00421bd0)
                FUN_00421bd0(enemy, instr, instr->args[0].i);
                goto restart;
            case 52: // opcode 53 = 子脚本 (FUN_00421cb0)
                if (FUN_00421cb0(enemy, instr))
                {
                    enemy->curContextPtr->curInstr = instr;
                    goto exit;
                }
                goto restart;
            case 53: // ECL_SET_ANM: play animation arg0
                g_EnemyAnmLoaded.SetAndExecuteScriptIdx(&enemy->primaryVm, ECL_IVAL(0));
                enemy->unk3328 &= ~0x4;
                goto skipInstr;
            case 54: // opcode 55 = ECL_SUB_CALL
                enemy->FUN_00421de0(ECL_IVAL(0), ECL_IVAL(1), ECL_IVAL(2), ECL_IVAL(3), ECL_IVAL(4), ECL_IVAL(5));
                enemy->unk3328 &= ~0x4;
                goto skipInstr;
            case 55: // opcode 56 = ECL_SUB_CALL
                enemy->FUN_00421de0(ECL_IVAL(0), ECL_IVAL(1), ECL_IVAL(2), ECL_IVAL(3), ECL_IVAL(4), ECL_IVAL(5));
                enemy->unk3328 &= ~0x4;
                goto skipInstr;
            case 56: // opcode 57 = 子脚本 (FUN_00421e50)
                FUN_00421e50(enemy, instr);
                enemy->unk3328 &= ~0x4;
                goto skipInstr;
            case 57: // ECL_SET_ANM_SUB: play on the secondary animation set
                g_EnemyAnmLoaded2.SetAndExecuteScriptIdx(&enemy->primaryVm, ECL_IVAL(0));
                enemy->unk3328 |= 0x4;
                goto skipInstr;
            case 58: // opcode 59 = ECL_SUB_CALL
                enemy->FUN_00421de0(ECL_IVAL(0), ECL_IVAL(1), ECL_IVAL(2), ECL_IVAL(3), ECL_IVAL(4), ECL_IVAL(5));
                enemy->unk3328 &= ~0x4;
                goto skipInstr;
            case 59: // opcode 60 = ECL_SUB_CALL
                enemy->FUN_00421de0(ECL_IVAL(0), ECL_IVAL(1), ECL_IVAL(2), ECL_IVAL(3), ECL_IVAL(4), ECL_IVAL(5));
                enemy->unk3328 &= ~0x4;
                goto skipInstr;
            case 60: // opcode 61 = 子脚本 (FUN_00421e50)
                enemy->unk3328 |= 0x4;
                FUN_00421e50(enemy, instr);
                goto skipInstr;
            case 61: // ECL_SET_ANM_SWITCH: play anim on the set chosen by unk3328 bit2
                if (((enemy->unk3328 >> 2) & 1) == 0)
                {
                    g_EnemyAnmLoaded.SetAndExecuteScriptIdx(&enemy->primaryVm, enemy->unk333c);
                }
                else
                {
                    g_EnemyAnmLoaded2.SetAndExecuteScriptIdx(&enemy->primaryVm, enemy->unk333c);
                }
                goto skipInstr;
            case 62: // opcode 63 = ECL_SET_POS: set position + move init
                enemy->pos.x = ECL_FVAL(0);
                enemy->pos.y = ECL_FVAL(1);
                enemy->pos.z = 0;
                enemy->FUN_0042c180();
                goto skipInstr;
            case 63: // opcode 64 = 移动插值 (FUN_00420f40)
                FUN_00420f40(enemy, instr);
                goto skipInstr;
            case 64: // opcode 65 = SET_MOVE_ANGLE (与 op66 相同)
                enemy->unk2d94 = AddNormalizeAngle(ECL_FVAL(0), 0);
                enemy->unk2da8 = ECL_FVAL(1);
                enemy->unk3324 = (enemy->unk3324 & ~0x3000) | 0x1000;
                enemy->unk2de8 = 0;
                enemy->unk2ddc.SetCurrent(0);
                goto skipInstr;
            case 65: // ECL_SET_MOVE_ANGLE: set angle + speed
                enemy->unk2d94 = AddNormalizeAngle(ECL_FVAL(0), 0);
                enemy->unk2da8 = ECL_FVAL(1);
                enemy->unk3324 = (enemy->unk3324 & ~0x3000) | 0x1000;
                enemy->unk2de8 = 0;
                enemy->unk2ddc.SetCurrent(0);
                goto skipInstr;
            case 66: // opcode 67 = 子脚本 (FUN_00422020)
                FUN_00422020(enemy, instr);
                goto skipInstr;
            case 69: // opcode 70 = SET_MOVE_SPEED: moveSpeed + flag bit12
                enemy->unk2d98 = ECL_FVAL(0);
                enemy->unk3324 = (enemy->unk3324 & ~0x3000) | 0x1000;
                goto skipInstr;
            case 70: // opcode 71 = 移动角度 (unk2dac) + flag bit12
                enemy->unk2dac = ECL_FVAL(0);
                enemy->unk3324 = (enemy->unk3324 & ~0x3000) | 0x1000;
                goto skipInstr;
            case 71: // opcode 72 = SET_MOVE_INTERP: 移动插值参数
                enemy->unk2de8 = ECL_IVAL(0);
                enemy->unk2ddc.SetCurrent(ECL_IVAL(0));
                enemy->unk2dd0 = ECL_FVAL(1);
                enemy->unk2dd4 = ECL_FVAL(2);
                enemy->unk2d9c = ECL_FVAL(3);
                enemy->unk2da0 = ECL_FVAL(4);
                enemy->unk2db0 = ECL_FVAL(5);
                enemy->unk2db4 = ECL_FVAL(6);
                goto skipInstr;
            case 72: // opcode 73 = SET_MOVE_INTERP
                enemy->unk2de8 = ECL_IVAL(0);
                enemy->unk2ddc.SetCurrent(ECL_IVAL(0));
                enemy->unk2d9c = ECL_FVAL(1);
                enemy->unk2da0 = ECL_FVAL(2);
                enemy->unk2db0 = 0;
                enemy->unk2db4 = ECL_FVAL(3);
                goto skipInstr;
            case 73: // opcode 74 = SET_MOVE_INTERP
                enemy->unk2de8 = ECL_IVAL(0);
                enemy->unk2ddc.SetCurrent(ECL_IVAL(0));
                enemy->unk2da0 = ECL_FVAL(1);
                enemy->unk2db4 = ECL_FVAL(2);
                goto skipInstr;
            case 74: // opcode 75 = SET_MOVE_SPEED4: unk3340/44/48/4c = f0..f3
                enemy->unk3340 = ECL_FVAL(0);
                enemy->unk3344 = ECL_FVAL(1);
                enemy->unk3348 = ECL_FVAL(2);
                enemy->unk334c = ECL_FVAL(3);
                goto skipInstr;
            case 75: // opcode 76 = 清除移动标志 (unk3324 bit19)
                enemy->unk3324 &= ~0x80000;
                goto skipInstr;
            case 76: // opcode 77 = 写两个 float 字段
                enemy->unk2d70 = ECL_FVAL(0);
                enemy->unk2d74 = ECL_FVAL(1);
                goto skipInstr;
            case 77: // opcode 78 = 写两个 float 字段
                enemy->unk2d7c = ECL_FVAL(0);
                enemy->unk2d80 = ECL_FVAL(1);
                goto skipInstr;
            case 78: // opcode 79 = 设置特效标志 (flags bit → unk3324/3328 各 bit)
                {
                    i32 flags = ECL_IVAL(0);
                    enemy->unk3324 = (enemy->unk3324 & ~0x40) | (((flags >> 0) & 1) << 6);
                    enemy->unk3324 = (enemy->unk3324 & ~0x4) | (((flags >> 1) & 1) << 2);
                    enemy->unk3324 = (enemy->unk3324 & ~0x8) | (((flags >> 2) & 1) << 3);
                    enemy->unk3324 = (enemy->unk3324 & ~0x10) | (((flags >> 3) & 1) << 4);
                    enemy->unk3324 = (enemy->unk3324 & ~0x10000000) | (((flags >> 4) & 1) << 0x1c);
                    enemy->unk3328 = (enemy->unk3328 & ~0x40) | (((flags >> 5) & 1) << 6);
                }
                goto skipInstr;
            case 79: // opcode 80 = 清除特效标志 (flags if 分支)
                {
                    i32 flags = ECL_IVAL(0);
                    if (flags & 1) enemy->unk3324 &= ~0x40;
                    if (flags & 2)
                    {
                        enemy->unk3324 &= ~0x4;
                        if (enemy->unk53c8) *(u32 *)((u8 *)enemy->unk53c8 + 0x1f8) &= ~0x20000;
                    }
                    if (flags & 4) enemy->unk3324 &= ~0x8;
                    if (flags & 8) enemy->unk3324 |= 0x10;
                    if (flags & 0x10) enemy->unk3324 |= 0x10000000;
                    if (flags & 0x20) enemy->unk3328 |= 0x40;
                }
                goto skipInstr;
            case 80: // opcode 81 = 混合设置/清除特效标志
                {
                    i32 flags = ECL_IVAL(0);
                    if (flags & 1) enemy->unk3324 |= 0x40;
                    if (flags & 2)
                    {
                        enemy->unk3324 |= 0x4;
                        if (enemy->unk53c8) *(u32 *)((u8 *)enemy->unk53c8 + 0x1f8) |= 0x20000;
                    }
                    if (flags & 4) enemy->unk3324 |= 0x8;
                    if (flags & 8) enemy->unk3324 &= ~0x10;
                    if (flags & 0x10) enemy->unk3324 &= ~0x10000000;
                    if (flags & 0x20) enemy->unk3328 &= ~0x40;
                }
                goto skipInstr;
            case 81: // opcode 82 = 速度平方: unk3350 = f0*f0
                enemy->unk3350 = ECL_FVAL(0);
                enemy->unk3350 *= enemy->unk3350;
                goto skipInstr;
            case 82: // opcode 83 = 设置 unk3328 bit1
                enemy->unk3328 = (enemy->unk3328 & ~0x2) | ((ECL_IVAL(0) & 1) << 1);
                goto skipInstr;
            case 83: // opcode 84 = NOP
            case 84: // opcode 85 = NOP
                goto skipInstr;
            case 85: // opcode 86 = 读子弹对象 int 变量 (g_BulletObjects[arg2] 上的 var1)
                {
                    i32 result;
                    if (instr->paramMask & 0x2)
                        result = GetVarValue((Enemy *)*(u32 *)(0xf54cc0 + ECL_IVAL(2) * 4), instr->args[1].i);
                    else
                        result = instr->args[1].i;
                    *GetIntPtr(enemy, &instr->args[0], instr->paramMask) = result;
                }
                goto skipInstr;
            case 86: // opcode 87 = 读子弹对象 float 变量 (g_BulletObjects[arg2] 上的 var1)
                {
                    i32 idx = ECL_IVAL(2);
                    if (*(u32 *)(0xf54cc0 + idx * 4) != 0)
                    {
                        f32 result;
                        if (instr->paramMask & 0x2)
                            result = ((Enemy *)*(u32 *)(0xf54cc0 + ECL_IVAL(2) * 4))->GetEclFloatVar(instr->args[1].i);
                        else
                            result = instr->args[1].f;
                        *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) = result;
                    }
                }
                goto skipInstr;
            case 88: // opcode 89 = 写子弹对象+0x2d30 (若 g_BulletObjects[arg0] 有效)
                {
                    i32 idx = ECL_IVAL(0);
                    if (*(u32 *)(0xf54cc0 + idx * 4) != 0)
                        *(i16 *)(*(u32 *)(0xf54cc0 + ECL_IVAL(0) * 4) + 0x2d30) = (i16)ECL_IVAL(1);
                }
                goto skipInstr;
            case 95: case 96: case 97: case 98: case 99: case 100:
            case 101: case 102: case 103:
                // opcode 96-104: 激光处理
                if (enemy->unk2dfc > 0)
                {
                    if (((enemy->unk3324 >> 0x11) & 1) == 1)
                    {
                        memcpy(enemy->unk3034, instr, 0x2c);
                    }
                    else
                    {
                        FUN_00422720(enemy, instr);
                    }
                }
                goto skipInstr;
            case 106: // opcode 107 = 设置 unk3324 bit17
                enemy->unk3324 |= 0x20000;
                goto skipInstr;
            case 107: // opcode 108 = 清除 unk3324 bit17
                enemy->unk3324 &= ~0x20000;
                goto skipInstr;
            case 109: // opcode 110 = 移动插值 (unk2db8/2dbc/2dc0)
                enemy->unk2db8 = ECL_FVAL(0);
                enemy->unk2dbc = ECL_FVAL(1);
                enemy->unk2dc0 = 0;
                goto skipInstr;
            case 111: // opcode 112 = RemoveAllBullets(1)
                g_BulletManager.bulletmanager_fun_00415c60();
                goto skipInstr;
            case 112: // opcode 113 = 生命回调阈值/子脚本: unk3020/24/28
                {
                    i32 v0 = ECL_IVAL(0);
                    if (v0 >= 0)
                    {
                        enemy->unk3024 = ECL_IVAL(0);
                        enemy->unk3020 |= 0x200;
                    }
                    else
                    {
                        enemy->unk3020 &= ~0x200;
                    }
                    enemy->unk3028 = ECL_IVAL(1);
                }
                goto skipInstr;
            case 115: // opcode 116 = 设置 unk3300
                enemy->unk3300 = ECL_IVAL(0);
                goto skipInstr;
            case 116: // opcode 117 = 给子弹对象加角度 (unk3280[idx] 的 unk554)
                {
                    i32 idx = ECL_IVAL(0);
                    if (enemy->unk3280[idx] != 0)
                        *(f32 *)((u8 *)enemy->unk3280[idx] + 0x554) = AddNormalizeAngle(
                            *(f32 *)((u8 *)enemy->unk3280[idx] + 0x554), ECL_FVAL(1));
                }
                goto skipInstr;
            case 119: // opcode 120 = 设置 curContext globalVar intVars[0] (依 unk3280 子弹对象)
                {
                    i32 v0 = ECL_IVAL(0);
                    if (enemy->unk3280[v0] != 0 && *(u32 *)((u8 *)enemy->unk3280[v0] + 0x584) != 0)
                        enemy->curContextPtr->eclContextArgs.globalVars.intVars[0] = 1;
                    else
                        enemy->curContextPtr->eclContextArgs.globalVars.intVars[0] = 0;
                }
                goto skipInstr;
            case 125: // opcode 126 = 设置 interrupts 数组元素
                enemy->interrupts[ECL_IVAL(1)] = (i16)ECL_IVAL(0);
                goto skipInstr;
            case 121: // opcode 122 = 子脚本 (FUN_00421280)
                FUN_00421280(enemy, instr);
                goto skipInstr;
            case 122: // opcode 123 = 子脚本 (FUN_004212e0)
                FUN_004212e0(enemy, instr);
                goto skipInstr;
            case 123: // opcode 124 = 播放音效 (声音索引 v0, 位置 pos.x)
                g_SoundPlayer.PlaySoundPositionedByIdx((SoundIdx)ECL_IVAL(0), enemy->pos.x);
                goto skipInstr;
            case 130: // opcode 131 = 设置激光字段 unk2e00/2dfc/2e04 + 若条件清 Gui
                enemy->unk2e00 = ECL_IVAL(0);
                enemy->unk2dfc = ECL_IVAL(0);
                enemy->unk2e04 = ECL_IVAL(0);
                if (enemy->unk3313 == 0 && ((enemy->unk3324 >> 1) & 1))
                {
                    for (i = 0; i < 8; i++)
                        g_Gui.FUN_004230e0(i, 0.0f, 0.0f);
                }
                goto skipInstr;
            case 131: // opcode 132 = 设置 unk2e14 计时器
                enemy->unk2e14.SetCurrent(ECL_IVAL(0));
                goto skipInstr;
            case 128: // opcode 129 = 设置 unk3324 bit20-22 (boss 模式跳过)
                if (!IS_BOSS_MODE())
                    enemy->unk3324 = (enemy->unk3324 & 0xff8fffff) | ((instr->args[0].i & 0x7) << 0x14);
                goto skipInstr;
            case 129: // opcode 130 = 设置 unk2cee (boss 模式跳过)
                if (!IS_BOSS_MODE())
                    enemy->unk2cee = (i16)instr->args[0].i;
                goto skipInstr;
            case 132: // opcode 133 = 设置 unk3358/unk3368 (boss 模式只写 3358)
                if (IS_BOSS_MODE())
                {
                    enemy->unk3358[ECL_IVAL(0)] = ECL_IVAL(1);
                }
                else
                {
                    enemy->unk3358[ECL_IVAL(0)] = ECL_IVAL(1);
                    enemy->unk3368[ECL_IVAL(0)] = ECL_IVAL(2);
                }
                goto skipInstr;
            case 136: // opcode 137 = 设置 curContextPtr->func/eclExInstr (依函数表 0x4c6cb0)
                if (ECL_IVAL(0) >= 0)
                {
                    enemy->curContextPtr->func = (EclExInstr)(*(void **)(0x4c6cb0 + ECL_IVAL(0) * 4));
                    enemy->curContextPtr->eclExInstr = instr;
                }
                else
                {
                    enemy->curContextPtr->func = NULL;
                }
                goto skipInstr;
            case 137: // opcode 138 = 复制 3 字节到 unk3310/3311/3312
                enemy->unk3310 = instr->args[0].b[0];
                enemy->unk3311 = instr->args[0].b[1];
                enemy->unk3312 = instr->args[0].b[2];
                goto skipInstr;
            case 143: // opcode 144 = 设置 unk3308/330c
                enemy->unk3308 = ECL_IVAL(0);
                enemy->unk330c = ECL_IVAL(1);
                goto skipInstr;
            case 152: // opcode 153 = unk337c = unk2cee; unk2e14.SetCurrent(0)
                enemy->unk337c = enemy->unk2cee;
                enemy->unk2e14.SetCurrent(0);
                goto skipInstr;
            case 150: // opcode 151 = 设置 unk3324 bit26
                enemy->unk3324 = (enemy->unk3324 & ~0x4000000) | ((instr->args[0].b[0] & 1) << 0x1a);
                goto skipInstr;
            case 148: // opcode 149 = 写 enemy+0x20a (i16)
                *(i16 *)((u8 *)enemy + 0x20a) = (i16)ECL_IVAL(0);
                goto skipInstr;
            case 142: // opcode 143 = 设置 unk3304
                enemy->unk3304 = ECL_IVAL(0);
                goto skipInstr;
            case 145: // opcode 146 = curContextPtr->time += v0
                enemy->curContextPtr->time += ECL_IVAL(0);
                goto skipInstr;
            case 146: // opcode 147 = 写全局 0x4ea290
                *(volatile u32 *)0x4ea290 = ECL_IVAL(0);
                goto skipInstr;
            case 147: // opcode 148 = g_Gui.FUN_00423130(v0); 全局 0x164d30c += 0x708
                g_Gui.FUN_00423130(ECL_IVAL(0));
                *(u32 *)0x164d30c += 0x708;
                goto skipInstr;
            case 151: // opcode 152 = 写移动界限字段 unk2dec/2df0/2df4..2dfa
                enemy->unk2dec = ECL_FVAL(0);
                enemy->unk2df0 = ECL_FVAL(1);
                enemy->unk2df4 = (i16)ECL_IVAL(2);
                enemy->unk2df6 = (i16)ECL_IVAL(3);
                enemy->unk2df8 = (i16)ECL_IVAL(4);
                enemy->unk2dfa = (i16)ECL_IVAL(5);
                goto skipInstr;
            case 156: // opcode 157 = 设置 AI 字段 unk534c/534e/5350/5352 + 若 bit3 调 AnmManager
                enemy->unk534c = instr->args[0].b[0];
                enemy->unk534e = (i16)ECL_IVAL(1);
                enemy->unk5350 = (i16)ECL_IVAL(2);
                enemy->unk5352 = (i16)ECL_IVAL(3);
                if (enemy->unk534c & 0x8)
                {
                    g_AnmManager->FUN_004649a0(&enemy->primaryVm, (void *)((u8 *)enemy + 0x3e14),
                                               (i32)(((i32)(i16)enemy->unk5352 / (i32)(i16)enemy->unk534e) << 1));
                }
                goto skipInstr;
            case 157: // opcode 158 = 设置 Gui 数据 (v0, a1/unk2e00, a2/unk2e00) + 若 bit3 调 23110
                {
                    i32 v0 = ECL_IVAL(0);
                    g_Gui.FUN_004230e0(v0, (f32)ECL_IVAL(1) / (f32)enemy->unk2e00,
                                       (f32)ECL_IVAL(2) / (f32)enemy->unk2e00);
                    if (instr->paramMask & 0x8)
                        g_Gui.FUN_00423110(v0, ECL_IVAL(3));
                }
                goto skipInstr;
            case 159: // opcode 160 = unk5354 ZunTimer.SetCurrent(v0)
                enemy->unk5354.SetCurrent(ECL_IVAL(0));
                goto skipInstr;
            case 160: // opcode 161 = 生成特效 at unk2d88 (g_BulletManager.FUN_00430d30)
                g_BulletManager.FUN_00430d30(&enemy->unk2d88, ECL_FVAL(0));
                goto skipInstr;
            case 144: // opcode 145 = 设置 unk3324 bit25
                enemy->unk3324 = (enemy->unk3324 & ~0x2000000) | ((instr->args[0].b[0] & 1) << 0x19);
                goto skipInstr;
            case 153: // opcode 154 = 清空 unk3280[0x20]
                for (i = 0; i < 0x20; i++)
                {
                    enemy->unk3280[i] = 0;
                }
                goto skipInstr;
            case 162: // opcode 163 = 写全局 0xf54cec
                *(volatile u32 *)0xf54cec = ECL_IVAL(0);
                goto skipInstr;
            case 158: // opcode 159 = 设置 unk332f (byte)
                enemy->unk332f = (u8)ECL_IVAL(0);
                goto skipInstr;
            case 154: // opcode 155 = 设置 unk3324 bit27 + 全局写
                enemy->unk3324 = (enemy->unk3324 & ~0x8000000) | ((instr->args[0].b[0] & 1) << 0x1b);
                *(volatile u32 *)0x4ecca8 = 0x5f5e0f6;
                goto skipInstr;
            case 155: // opcode 156 = 设置 unk3324 bit7 + unk332f=2
                enemy->unk3324 = (enemy->unk3324 & ~0x80) | ((instr->args[0].b[0] & 1) << 7);
                enemy->unk332f = 2;
                goto skipInstr;
            case 169: // opcode 170 = 写子弹对象+0x599 (byte)
                {
                    i32 idx = ECL_IVAL(0);
                    if (enemy->unk3280[idx] != 0)
                        *(u8 *)((u8 *)enemy->unk3280[idx] + 0x599) = (u8)ECL_IVAL(1);
                }
                goto skipInstr;
            case 175: // opcode 176 = 全局弹幕/特殊事件状态 + unk3324 bit30
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
                enemy->unk3324 |= 0x40000000;
                goto skipInstr;
            case 177: // opcode 178 = 子脚本 (FUN_004224a0)
                FUN_004224a0(enemy, instr);
                goto skipInstr;
            default:
                goto skipInstr;
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
    if (((enemy->unk3324 >> 0x1a) & 1) == 0)
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
    if (enemy->unk2dfc > 0)
    {
        i32 flag = 0;
        EclInterp *interp = &enemy->curContextPtr->interps[0];
        Float3 savedPos = enemy->pos;
        if (enemy->curContextPtr->func != NULL)
        {
            enemy->curContextPtr->func(enemy, enemy->curContextPtr->eclExInstr);
        }
        for (i = 0; i < 8; i++, interp++)
        {
            if (interp->fn != NULL)
            {
                f32 t;
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
            enemy->unk2d94 = EclAngleFromDxDy(enemy->unk2d4c, enemy->unk2d50);
            enemy->pos = savedPos;
        }
    }

    if (subCtxIdx == -1)
    {
        enemy->unk2ce8 = enemy->stackDepth;
    }
    else
    {
        *(i16 *)((u8 *)enemy->dataPtrs[subCtxIdx] + 0x6) = enemy->stackDepth;
    }
    enemy->curContextPtr->curInstr = instr;
    enemy->curContextPtr->time.SetCurrent(0);
    for (i = subCtxIdx + 1; i < 4; i++)
    {
        if (enemy->dataPtrs[i] != NULL)
        {
            enemy->savedStackPtr = (EclContext *)((u8 *)enemy->dataPtrs[i] + 0x230);
            enemy->curContextPtr = (EclContext *)((u8 *)enemy->dataPtrs[i] + 0x8);
            instr = enemy->curContextPtr->curInstr;
            enemy->curContextPtr->unk220 = i + 1;
            enemy->stackDepth = *(i16 *)((u8 *)enemy->dataPtrs[i] + 0x6);
            subCtxIdx = i;
            goto restart;
        }
    }
    enemy->savedStackPtr = &enemy->savedContextStack[0];
    enemy->curContextPtr = &enemy->eclContext;
    enemy->FUN_00422c40();
    enemy->FUN_00423150();
    return ZUN_SUCCESS;
}

} // namespace th08
