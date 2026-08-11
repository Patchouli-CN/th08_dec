#include "th_pch.h"

#include "EclManager.hpp"
#include "EnemyManager.hpp"
#include "Global.hpp"
#include "GameManager.hpp"

namespace th08
{

DIFFABLE_STATIC(EclManager, g_EclManager);

// ECL variable access helpers (th08 standalone functions; th07 had these as
// EclManager static methods). Stubs for now; RunEcl's call targets normalize to
// T in fn_diff so only the calling convention needs to be right here.
i32 __fastcall GetVarValue(Enemy *enemy, i32 varId);                       // 0x41f420
i32 *__fastcall GetIntPtr(Enemy *enemy, AnyArg *args, u16 paramMask);      // 0x41fe10
f32 *__fastcall GetFloatPtr(Enemy *enemy, AnyArg *args, u16 paramMask, i32 unused); // 0x420950

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

void Enemy::FUN_00421de0(u8 *a, u8 *b, u8 *c, u8 *d, u8 *e)
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

// FUNCTION: th08 0x418330
ZunResult EclManager::Load(const char *path)
{
    return ZUN_ERROR;
}

// FUNCTION: th08 0x4184b0 (逆向中)
ZunResult EclManager::RunEcl(Enemy *enemy)
{
    EclRawInstr *instr;
    i32 arg;
    i32 subCtxIdx = -1;
    i32 i;

    enemy->savedStackPtr = &enemy->savedContextStack[0];
    enemy->curContextPtr = &enemy->eclContext;
    enemy->stackDepth = enemy->unk2ce8;

restart:
    // ECL arg access: paramMask bit N set means arg N is a variable id.
#define ECL_IVAL(n) ((instr->paramMask & (1 << (n))) ? GetVarValue(enemy, instr->args[n].i) : instr->args[n].i)
#define ECL_FVAL(n) ((instr->paramMask & (1 << (n))) ? enemy->GetEclFloatVar(instr->args[n].i) : instr->args[n].f)
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
                {
                    i32 v0 = ECL_IVAL(0);
                    enemy->curContextPtr->waitTimer.SetCurrent(v0);
                }
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
            case 53: // ECL_SET_ANM: play animation arg0
                g_EnemyAnmLoaded.SetAndExecuteScriptIdx(&enemy->primaryVm, ECL_IVAL(0));
                enemy->unk3328 &= ~0x4;
                goto skipInstr;
            case 54: // ECL_SUB_CALL
                enemy->FUN_00421de0((u8 *)instr, (u8 *)instr + 1, (u8 *)instr + 2, (u8 *)instr + 3, (u8 *)instr + 4);
                enemy->unk3328 &= ~0x4;
                goto skipInstr;
            case 57: // ECL_SET_ANM_SUB: play on the secondary animation set
                g_EnemyAnmLoaded2.SetAndExecuteScriptIdx(&enemy->primaryVm, ECL_IVAL(0));
                enemy->unk3328 |= 0x4;
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
            case 65: // ECL_SET_MOVE_ANGLE: set angle + speed
                enemy->unk2d94 = AddNormalizeAngle(ECL_FVAL(0), 0);
                enemy->unk2da8 = ECL_FVAL(1);
                enemy->unk3324 = (enemy->unk3324 & ~0x3000) | 0x1000;
                enemy->unk2de8 = 0;
                enemy->unk2ddc.SetCurrent(0);
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
