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

i32 __fastcall GetVarValue(Enemy *enemy, i32 varId)
{
    return 0;
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

// FUNCTION: th08 0x4184b0 (逆向中：opcode 1-16 已确认，其余 default)
ZunResult EclManager::RunEcl(Enemy *enemy)
{
    EclRawInstr *instr;
    i32 arg;

    enemy->savedStackPtr = &enemy->savedContextStack[0];
    enemy->curContextPtr = &enemy->eclContext;
    enemy->stackDepth = enemy->unk2ce8;

restart:
    instr = enemy->curContextPtr->curInstr;
    if (enemy->runInterrupt >= 0)
    {
        goto handleInterrupt;
    }
    for (;;)
    {
        enemy->unk2d88 = enemy->pos + enemy->unk2d40;

        if (enemy->curContextPtr->waitTimer.AsFrames() > 0)
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
                enemy->curContextPtr->waitTimer.SetCurrent(GetVarValue(enemy, instr->args[0].i));
                goto skipInstr;
            case 2: // ECL_NOP
                goto skipInstr;
            case 3: // ECL_JUMP: time = arg0; jump by arg1
                enemy->curContextPtr->time.current = instr->args[0].i;
                instr = (EclRawInstr *)((u8 *)instr + instr->args[1].i);
                continue;
            case 4: // ECL_DEC_JUMP: *arg2--; if (arg1 > 0) jump
                *GetIntPtr(enemy, &instr->args[2], instr->paramMask) -= 1;
                arg = (instr->paramMask & 0x4) != 0 ? GetVarValue(enemy, instr->args[1].i) : instr->args[1].i;
                if (arg > 0)
                {
                    enemy->curContextPtr->time.current = instr->args[0].i;
                    instr = (EclRawInstr *)((u8 *)instr + instr->args[1].i);
                    continue;
                }
                goto skipInstr;
            case 5: // ECL_SET_INT: *arg0 = arg1
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) =
                    (instr->paramMask & 0x2) != 0 ? GetVarValue(enemy, instr->args[1].i) : instr->args[1].i;
                goto skipInstr;
            case 6: // ECL_SET_FLOAT: *arg0 = arg1
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) =
                    (instr->paramMask & 0x2) != 0 ? enemy->GetEclFloatVar(instr->args[1].i) : instr->args[1].f;
                goto skipInstr;
            case 7: // ECL_RAND_SIGN: *arg0 = ±arg1
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) =
                    (g_Rng.GetRandomU16() & 1 ? 1 : -1) * GetVarValue(enemy, instr->args[1].i);
                goto skipInstr;
            case 8: // ECL_RAND_SIGN_FLOAT: *arg0 = ±arg1
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) =
                    (g_Rng.GetRandomU16() & 1 ? 1.0f : -1.0f) * enemy->GetEclFloatVar(instr->args[1].i);
                goto skipInstr;
            case 9: // ECL_ADD: *arg0 += arg1
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) += GetVarValue(enemy, instr->args[1].i);
                goto skipInstr;
            case 10: // ECL_SUB: *arg0 -= arg1
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) -= GetVarValue(enemy, instr->args[1].i);
                goto skipInstr;
            case 11: // ECL_MUL: *arg0 *= arg1
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) *= GetVarValue(enemy, instr->args[1].i);
                goto skipInstr;
            case 12: // ECL_DIV: *arg0 /= arg1
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) /= GetVarValue(enemy, instr->args[1].i);
                goto skipInstr;
            case 13: // ECL_MOD: *arg0 %= arg1
                *GetIntPtr(enemy, &instr->args[0], instr->paramMask) %= GetVarValue(enemy, instr->args[1].i);
                goto skipInstr;
            case 14: // ECL_ADD_FLOAT: *arg0 += arg1
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) += enemy->GetEclFloatVar(instr->args[1].i);
                goto skipInstr;
            case 15: // ECL_SUB_FLOAT: *arg0 -= arg1
                *GetFloatPtr(enemy, &instr->args[0], instr->paramMask, 0) -= enemy->GetEclFloatVar(instr->args[1].i);
                goto skipInstr;
            default:
                goto skipInstr;
            }
        }
    skipInstr:
        instr = (EclRawInstr *)((u8 *)instr + instr->size);
    }

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
    return ZUN_SUCCESS;
}

} // namespace th08
