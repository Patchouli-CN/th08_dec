#include "th_pch.h"

#include "Background.hpp"
#include "AnmManager.hpp"
#include "EffectManager.hpp"
#include "Gui.hpp"
#include "Player.hpp"
#include "ScreenEffect.hpp"

namespace th08
{
void __fastcall FUN_00426d10(Float3 *pos);

Float3 *__fastcall FUN_004090d0(Float3 *self, Float3 *out, Float3 *p2);
Float3 *__fastcall FUN_00409120(Float3 *self, Float3 *tmp3, f32 f, Float3 *tmp2, Float3 *p2);
Float3 *__fastcall FUN_00409080(Float3 *self);
f32 __stdcall FUN_00408fc0(f32 a, f32 b, f32 c, f32 d, f32 e);
DIFFABLE_STATIC(Background, g_Background);
DIFFABLE_STATIC(ChainElem, g_BackgroundCalcChain);
DIFFABLE_STATIC(ChainElem, g_BackgroundDrawChainHighPrio);
DIFFABLE_STATIC(ChainElem, g_BackgroundDrawChainLowPrio);

DIFFABLE_STATIC_ARRAY_ASSIGN(const char *, 9, g_StageAnmFiles) = {
    "stg1bg.anm", "stg2bg.anm", "stg3bg.anm", "stg4abg.anm", "stg4abg.anm",
    "stg5bg.anm", "stg6bg.anm", "stg7bg.anm", "stg8bg.anm",
};
DIFFABLE_STATIC_ARRAY_ASSIGN(const char *, 9, g_StageStdFiles) = {
    "stage1.std", "stage2.std", "stage3.std", "stage4a.std", "stage4b.std",
    "stage5.std", "stage6.std", "stage7.std", "stage8.std",
};
DIFFABLE_STATIC_ARRAY_ASSIGN(const char *, 9, g_StageStdFilesSpellPractice) = {
    "stage1_s.std", "stage2_s.std", "stage3_s.std", "stage4a_s.std", "stage4b_s.std",
    "stage5_s.std", "stage6_s.std", "stage7_s.std", "stage8_s.std",
};

Background::Background()
{
    memset(this, 0, sizeof(Background));
    this->camera4.pos = Float3(0.0f, 0.0f, 1000.0f);
    this->camera4.target = Float3(0.0f, 0.0f, 0.0f);
    this->camera4.up = Float3(0.0f, 1.0f, 0.0f);
    this->camera4.fov = ZUN_PI / 6.0f;
    this->camera0 = this->camera4;
    this->camera1 = this->camera4;
}

// FUNCTION: th08 0x407400
ChainCallbackResult Background::OnUpdate(Background *background)
{
    Float3 pos;
    StdRawInstr *p;
    i32 layerIdx;
    i32 i;
    f32 ratio;
    f32 t;
    u8 *sprite;

    if (background->stdData == NULL)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    if (g_GameManager.flags.unk10)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    // ---- Stage 7 special handling ----
    if (g_GameManager.currentStage == 7)
    {
        if (background->unk0xae8 == NULL)
        {
            Float3 effectPos = Float3(0.0f, 0.0f, 0.0f);
            background->unk0xae8 = g_EffectManager.FUN_00425870(0x40, &effectPos, 0xc, 1, -1);
            background->stageAnm->SetAndExecuteScriptIdx(background->unk0xae8, 0xb);
        }
        if (background->unk0x6260 == 1)
        {
            background->stageAnm->SetAndExecuteScriptIdx(background->unk0xae8, 0xb);
        }
        if (background->unk0x6260 == 2)
        {
            AnmVm vmBackup = *(AnmVm *)background->unk0xae8;
            background->stageAnm->SetAndExecuteScriptIdx(background->unk0xae8, 0xc);
            background->unk0xae8->SetInterrupt(2);
            background->unk0xae8->posFinal = vmBackup.posFinal;
            background->unk0xae8->posInitial = vmBackup.posInitial;
            background->unk0xae8->prefix.interpCurrentTimers[0] = vmBackup.prefix.interpCurrentTimers[0];
            background->unk0xae8->prefix.interpEndTimers[0] = vmBackup.prefix.interpEndTimers[0];
            background->unk0xae8->prefix.interpModes[0] = vmBackup.prefix.interpModes[0];
            background->unk0xae8->prefix.color1 = vmBackup.prefix.color1;
        }
        if (background->unk0x6260 == 3)
        {
            AnmVm vmBackup = *(AnmVm *)background->unk0xae8;
            background->unk0xae8->SetInterrupt(3);
            background->unk0xae8->posFinal = vmBackup.posFinal;
            background->unk0xae8->posInitial = vmBackup.posInitial;
            background->unk0xae8->prefix.interpCurrentTimers[0] = vmBackup.prefix.interpCurrentTimers[0];
            background->unk0xae8->prefix.interpEndTimers[0] = vmBackup.prefix.interpEndTimers[0];
            background->unk0xae8->prefix.interpModes[0] = vmBackup.prefix.interpModes[0];
            background->unk0xae8->prefix.color1 = vmBackup.prefix.color1;
        }
        if (background->unk0x6260 == 4)
        {
            AnmVm vmBackup = *(AnmVm *)background->unk0xae8;
            background->unk0xae8->SetInterrupt(4);
            background->unk0xae8->posFinal = vmBackup.posFinal;
            background->unk0xae8->posInitial = vmBackup.posInitial;
            background->unk0xae8->prefix.interpCurrentTimers[0] = vmBackup.prefix.interpCurrentTimers[0];
            background->unk0xae8->prefix.interpEndTimers[0] = vmBackup.prefix.interpEndTimers[0];
            background->unk0xae8->prefix.interpModes[0] = vmBackup.prefix.interpModes[0];
            background->unk0xae8->prefix.color1 = vmBackup.prefix.color1;
        }
    }

    // ---- Scan for current element ----
    if (background->unk0x6260 != 0)
    {
        i32 index = 0;
        p = background->beginningOfScript;
        background->unk0x818 = 0;

        while (!(p->opcode == 0x1f && background->unk0x6260 == p->args.args[0].i) && p->frame != -1)
        {
            p++;
            index++;
        }
        if (p->frame != -1)
        {
            background->unk0x818 = index + 1;
            background->timer0x80c.SetCurrent(p->frame);
            background->unk0x6260 = 0;
        }
    }

    // ---- Dispatch loop ----
    for (;;)
    {
        p = &background->beginningOfScript[background->unk0x818];
        if (!(background->timer0x80c >= p->frame) || p->frame == -1)
        {
            break;
        }

        switch (p->opcode)
        {
        case 0: // opcode 0
            if (p->frame == -1)
            {
                background->unk0x6454 = *p->args.AsVec();
                background->unk0x824 = background->unk0x6454;
            }
            else
            {
                background->unk0x824 = *p->args.AsVec();
                background->unk0x6454 = *p->args.AsVec();
                *(i32 *)&background->unk0x6464 = p->frame;
                p++;
                background->unk0x6450 = p->frame;
                background->unk0x6444 = *p->args.AsVec();
            }
            break;
        case 1: // opcode 1
            background->fog.color.d3dColor = p->args.args[0].u;
            background->fog.nearPlane = p->args.args[1].f;
            background->fog.farPlane = p->args.args[2].f;
            background->fogFadeTo = background->fog;
            break;
        case 2: // opcode 2
            background->fogFadeFrom = background->fog;
            background->unk0xb10 = p->args.args[0].u;
            background->timer0xb14.SetCurrent(0);
            break;
        case 3: // opcode 3
            if (background->unk0x6260 != 0)
            {
                background->unk0x6260 = 0;
                break;
            }
            goto afterDispatchLoop;
        case 4: // opcode 4
            background->unk0x818 = p->args.args[0].i;
            background->timer0x80c.SetCurrent(p->args.args[1].i);
            background->unk0x63e0[0] = 0;
            background->unk0x6464 = 1;
            continue;
        case 5: // opcode 5
            if (background->unk0x6464)
            {
                Float3 local = *p->args.AsVec() - background->camera0.pos;
                FUN_00426d10(&local);
                background->unk0x6464 = 0;
            }
            background->camera1.pos = background->camera0.pos;
            background->camera0.pos = *p->args.AsVec();
            if (background->unk0x63e0[0] == 0)
            {
                background->camera4.pos = *p->args.AsVec();
            }
            break;
        case 6: // opcode 6
            background->unk0x63e0[0] = p->args.args[0].u;
            background->timers0x63f4[0].SetCurrent(0);
            background->unk0x6430[0] = p->args.args[1].i;
            break;
        case 7: // opcode 7
            background->camera1.target = background->camera0.target;
            background->camera0.target = *p->args.AsVec();
            if (background->unk0x63e0[1] == 0)
            {
                background->camera4.target = *p->args.AsVec();
            }
            break;
        case 8: // opcode 8
            background->unk0x63e0[1] = p->args.args[0].u;
            background->timers0x63f4[1].SetCurrent(0);
            background->unk0x6430[1] = p->args.args[1].i;
            break;
        case 9: // opcode 9
            background->camera1.up = background->camera0.up;
            background->camera0.up = *p->args.AsVec();
            if (background->unk0x63e0[2] == 0)
            {
                background->camera4.up = *p->args.AsVec();
            }
            break;
        case 10: // opcode 10
            background->unk0x63e0[2] = p->args.args[0].u;
            background->timers0x63f4[2].SetCurrent(0);
            background->unk0x6430[2] = p->args.args[1].i;
            break;
        case 11: // opcode 11
            background->camera4.unk0x24.x = background->camera0.fov;
            background->camera0.fov = p->args.args[0].f;
            if (background->unk0x63e0[3] == 0)
            {
                background->camera4.fov = p->args.args[0].f;
            }
            break;
        case 12: // opcode 12
            background->unk0x63e0[3] = p->args.args[0].u;
            background->timers0x63f4[3].SetCurrent(0);
            background->unk0x6430[3] = p->args.args[1].i;
            break;
        case 13: // opcode 13
            background->unk0x830 = p->args.args[0].i;
            break;
        case 14: // opcode 14
            background->camera1.pos = *p->args.AsVec();
            break;
        case 15: // opcode 15
            background->camera0.pos = *p->args.AsVec();
            break;
        case 16: // opcode 16
            background->camera3.pos = *p->args.AsVec();
            break;
        case 17: // opcode 17
            background->camera2.pos = *p->args.AsVec();
            break;
        case 18: // opcode 18
            background->unk0x63e0[0] = p->args.args[0].u;
            background->timers0x63f4[0].SetCurrent(0);
            background->unk0x6430[0] = 7;
            break;
        case 19: // opcode 19
            background->camera1.target = *p->args.AsVec();
            break;
        case 20: // opcode 20
            background->camera0.target = *p->args.AsVec();
            break;
        case 21: // opcode 21
            background->camera3.target = *p->args.AsVec();
            break;
        case 22: // opcode 22
            background->camera2.target = *p->args.AsVec();
            break;
        case 23: // opcode 23
            background->unk0x63e0[1] = p->args.args[0].u;
            background->timers0x63f4[1].SetCurrent(0);
            background->unk0x6430[1] = 7;
            break;
        case 24: // opcode 24
            background->camera1.up = *p->args.AsVec();
            break;
        case 25: // opcode 25
            background->camera0.up = *p->args.AsVec();
            break;
        case 26: // opcode 26
            background->camera3.up = *p->args.AsVec();
            break;
        case 27: // opcode 27
            background->camera2.up = *p->args.AsVec();
            break;
        case 28: // opcode 28
            background->unk0x63e0[2] = p->args.args[0].u;
            background->timers0x63f4[2].SetCurrent(0);
            background->unk0x6430[2] = 7;
            break;
        case 29: // opcode 29
            if (p->args.args[0].i >= 0)
            {
                background->stageAnm->ExecuteAnmIdx(&background->unk0x4, p->args.args[0].i);
            }
            else
            {
                background->unk0x4.activeSpriteIndex = -1;
            }
            break;
        case 30: // opcode 30
            if (p->args.args[0].i >= 0)
            {
                background->stageAnm->ExecuteAnmIdx(&background->unk0x2a8, p->args.args[0].i);
            }
            else
            {
                background->unk0x4.activeSpriteIndex = -1;
            }
            break;
        case 32: // opcode 32
            background->camera4.unk0x3c = *p->args.AsVec();
            if (p->args.args[0].i >= 0)
            {
                background->stageAnm->ExecuteAnmIdx(&background->unk0x54c, p->args.args[0].i);
            }
            else
            {
                background->unk0x54c.activeSpriteIndex = -1;
            }
            break;
        case 33: // opcode 33
            background->unk0x6474 = p->args.args[0].b[0];
            background->unk0x63f0 = 0;
            background->timers0x63f4[4].SetCurrent(0);
            background->unk0x6430[4] = 0;
            break;
        default:
            break;
        }

        background->unk0x818++;
    }

afterDispatchLoop:
    // ---- Layer update loop ----
    layerIdx = 0;
    if (background->unk0x63e0[0])
    {
        background->FUN_00408d60(0, &background->camera4.pos, &background->camera1.pos,
                                 &background->camera0.pos, &background->camera3.pos,
                                 &background->camera2.pos);
    }
    layerIdx = 1;
    if (background->unk0x63e0[1])
    {
        background->FUN_00408d60(1, &background->camera4.target, &background->camera1.target,
                                 &background->camera0.target, &background->camera3.target,
                                 &background->camera2.target);
    }
    layerIdx = 2;
    if (background->unk0x63e0[2])
    {
        background->FUN_00408d60(2, &background->camera4.up, &background->camera1.up,
                                 &background->camera0.up, &background->camera3.up,
                                 &background->camera2.up);
    }
    layerIdx = 3;
    if (background->unk0x63e0[3])
    {
        if (background->timers0x63f4[3] < (i32)background->unk0x63e0[3])
        {
            background->timers0x63f4[3]++;
            ratio = (f32)background->timers0x63f4[3] / (f32)background->unk0x63e0[3];
        }
        else
        {
            background->timers0x63f4[3].SetCurrent(background->unk0x63e0[3]);
            ratio = 1.0f;
            background->unk0x63e0[3] = 0;
        }
        switch (background->unk0x6430[3] - 1)
        {
        case 0:
            ratio = 1.0f - (1.0f - ratio) * (1.0f - ratio);
            break;
        case 1:
            ratio = 1.0f - (1.0f - ratio) * (1.0f - ratio) * (1.0f - ratio);
            break;
        case 2:
            ratio = 1.0f - (1.0f - ratio) * (1.0f - ratio) * (1.0f - ratio) * (1.0f - ratio);
            break;
        case 3:
            ratio = ratio * ratio;
            break;
        case 4:
            ratio = ratio * ratio * ratio;
            break;
        case 5:
            ratio = ratio * ratio * ratio * ratio;
            break;
        default:
            break;
        }
        background->camera4.fov = background->camera1.fov + (background->camera0.fov - background->camera1.fov) * ratio;
    }

    // ---- Wave animation ----
    switch (background->unk0x6474)
    {
    case 1:
        t = (f32)background->timers0x63f4[4] * ZUN_PI * 2.0f / 480.0f - ZUN_PI;
        background->camera4.unk0x3c.x = sinf(t) * 40.0f;
        background->timers0x63f4[4]++;
        if (background->timers0x63f4[4] >= 480)
        {
            background->timers0x63f4[4].SetCurrent(0);
        }
        break;
    case 2:
        t = (f32)background->timers0x63f4[4] * ZUN_PI * 2.0f / 480.0f - ZUN_PI;
        background->camera4.unk0x3c.x = sinf(t) * 70.0f;
        background->camera4.up.x = -sinf(t) * 0.1f;
        background->timers0x63f4[4]++;
        if (background->timers0x63f4[4] >= 480)
        {
            background->timers0x63f4[4].SetCurrent(0);
        }
        break;
    case 3:
        t = (f32)background->timers0x63f4[4] * ZUN_PI * 2.0f / 4800.0f - ZUN_PI;
        background->camera4.up.x = sinf(t) * 1.0f;
        background->camera4.unk0x24.x = cosf(t) * 1.0f;
        background->timers0x63f4[4]++;
        if (background->timers0x63f4[4] >= 4800)
        {
            background->timers0x63f4[4].SetCurrent(0);
        }
        break;
    default:
        break;
    }

    // ---- Fog fade ----
    if (background->unk0xb10)
    {
        background->timer0xb14++;
        t = (f32)background->timer0xb14 / (f32)background->unk0xb10;
        if (t >= 1.0f)
        {
            t = 1.0f;
        }
        for (i = 0; i < 4; i++)
        {
            ((u8 *)&background->fog.color)[i] =
                (u8)(i32)(((f32)(i32)((u8 *)&background->fogFadeTo.color)[i] - (f32)(i32)((u8 *)&background->fogFadeFrom.color)[i]) * t
                          + (f32)(i32)((u8 *)&background->fogFadeFrom.color)[i]);
        }
        background->fog.nearPlane = background->fogFadeFrom.nearPlane + (background->fogFadeTo.nearPlane - background->fogFadeFrom.nearPlane) * t;
        background->fog.farPlane = background->fogFadeFrom.farPlane + (background->fogFadeTo.farPlane - background->fogFadeFrom.farPlane) * t;
        if (background->timer0xb14 >= (i32)background->unk0xb10)
        {
            background->unk0xb10 = 0;
        }
    }

    if (p->opcode != 3)
    {
        background->timer0x80c++;
    }
    background->FUN_00409f40();

    if (background->unk0xb24 >= 1)
    {
        if (background->unk0xb28 == 60)
        {
            background->unk0xb24++;
        }
        background->unk0xb28++;
        for (i = 0; i < background->unk0xb30; i++)
        {
            g_AnmManager->ExecuteScript(&background->objectVms[i]);
        }
    }

    if (background->unk0x4.activeSpriteIndex > 0)
    {
        g_AnmManager->ExecuteScript(&background->unk0x4);
    }
    if (background->unk0x2a8.activeSpriteIndex > 0)
    {
        g_AnmManager->ExecuteScript(&background->unk0x2a8);
    }
    if (background->unk0x54c.activeSpriteIndex > 0)
    {
        g_AnmManager->ExecuteScript(&background->unk0x54c);
        background->unk0x830 = background->unk0x54c.prefix.color1.d3dColor;
    }

    if (background->unk81c % 3 == 0)
    {
        if (background->unk81c >= 700 || g_GameManager.flags.unk10)
        {
            if (background->unk0xb24 < 2)
            {
                for (i = 0; i < 12; i++)
                {
                    sprite = (u8 *)g_EffectManager.FUN_00425430(0x3e, &background->unk0x6480[i], 1, 0x20ffffff);
                    sprite[0x354] = 4;
                }
            }
        }
    }

    background->unk0x647c = 1;
    if (background->unk0xb24 >= 2)
    {
        background->unk0x6478 = 0;
    }
    background->unk81c++;
    if (background->unk81c % 500 == 250 && g_GameManager.IsTampered())
    {
        return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: th08 0x409200 (75% FIXME: 多个 D3D 虚函数 0xa0/0x90 寄存器分配)
ChainCallbackResult Background::OnDrawHighPrio(Background *background)
{
    i32 i;

    /* 清除 0x6478..0x647c 的 4 字节临时标志区。 */
    *(i32 *)&background->unk0x6478 = 0;

    for (i = 0; i < 0x10; i++)
    {
        background->unk0x6480[i] = Float3(0.0f, 0.0f, 0.0f);
    }

    /* 把固定视口 (32,16)-(416,464) 写入 D3D 设备内部视图结构。 */
    *(i32 *)(D3D_DEVICE_VIEWPORT + 0) = 0x20;
    *(i32 *)(D3D_DEVICE_VIEWPORT + 4) = 0x10;
    *(i32 *)(D3D_DEVICE_VIEWPORT + 8) = 0x180;
    *(i32 *)(D3D_DEVICE_VIEWPORT + 0xc) = 0x1c0;

    g_AnmManager->FUN_00462e00();
    g_AnmManager->FUN_0040b9f0();
    g_AnmManager->FUN_0040ba50();
    g_AnmManager->FUN_0040ba10();
    g_AnmManager->FUN_0040b9d0();
    g_AnmManager->FUN_0040b9b0();
    g_AnmManager->FUN_0040ba30();
    g_AnmManager->FUN_0040bb20();
    g_AnmManager->FUN_0040ba70();

    if (!g_Supervisor.IsFogDisabled())
    {
        g_Supervisor.DisableFog();
    }

    g_AnmManager->FlushVertexBuffer();

    if (background->screenClearNeeded != 0)
    {
        ZunRect rect;

        *(u32 *)&rect.left = 0x20;
        *(u32 *)&rect.top = 0x10;
        *(u32 *)&rect.right = 0x180;
        *(u32 *)&rect.bottom = 0x1c0;

        /* D3D virtual calls @ vtbl+0xa0 (SetViewport) / vtbl+0x90 (Clear) */
        ((void(__stdcall *)(void *, void *))(*(void ***) * (void **)D3D_DEVICE_OBJ)[0xa0 / 4])(
            *(void **)D3D_DEVICE_OBJ, &rect);
        ((void(__stdcall *)(void *, void *, void *, u32, u32, f32, u32))(*(void ***) * (void **)D3D_DEVICE_OBJ)[0x90 / 4])(
            *(void **)D3D_DEVICE_OBJ, 0, 0, 1, 0xff000000, 0x3f800000, 0);

        background->screenClearNeeded = 0;
    }

    /* D3D virtual call @ vtbl+0xa0 (SetViewport) */
    ((void(__stdcall *)(void *, void *))(*(void ***) * (void **)D3D_DEVICE_OBJ)[0xa0 / 4])(
        *(void **)D3D_DEVICE_OBJ, (void *)D3D_DEVICE_VIEWPORT);

    /* skyColor 最高字节非 0 表示颜色已设置：用它清屏（FUN_0040bad0），随后重置为默认 0x00808080。 */
    if (background->skyColor.a > 0)
    {
        ((void(__fastcall *)(AnmManager *, u32))0x40bad0)(g_AnmManager, background->skyColor.d3dColor);
    }


    background->skyColor.a = 0;
    background->skyColor.r = 0x80;
    background->skyColor.g = 0x80;
    background->skyColor.b = 0x80;

    if ((i32)background->unk0xb24 <= 1)
    {
        if (g_Gui.FUN_00437d87() == 0)
        {
            if (background->unk0x4.activeSpriteIndex > 0)
            {
                ((void(__fastcall *)(AnmManager *, AnmVm *))0x40baf0)(g_AnmManager, &background->unk0x4);
            }

            if (background->unk0x2a8.activeSpriteIndex > 0)
            {
                ((void(__fastcall *)(AnmManager *, AnmVm *))0x40baf0)(g_AnmManager, &background->unk0x2a8);
            }

            if (background->unk0xae8 != NULL)
            {
                void *p = background->unk0xae8;
                /* 0x34c：粒子对象偏移处的每帧更新回调。 */
                ((void(__fastcall *)(void *)) *(u32 *)((u8 *)p + 0x34c))(p);
            }
        }
    }

    if ((background->unk0x830 & 0xff000000) == 0xff000000)
    {
        /* D3D virtual call @ vtbl+0x90 (Clear) */
        ((void(__stdcall *)(void *, void *, void *, u32, u32, f32, u32))(*(void ***) * (void **)D3D_DEVICE_OBJ)[0x90 / 4])(
            *(void **)D3D_DEVICE_OBJ, 0, 0, 3, background->unk0x830, 0x3f800000, 0);
    }
    else if (background->unk0x830 != 0)
    {
        ZunRect rect = {32.0f, 16.0f, 416.0f, 464.0f};

        ScreenEffect::DrawSquare(&rect, background->unk0x830);

        /* D3D virtual call @ vtbl+0x90 (Clear) */
        ((void(__stdcall *)(void *, void *, void *, u32, u32, f32, u32))(*(void ***) * (void **)D3D_DEVICE_OBJ)[0x90 / 4])(
            *(void **)D3D_DEVICE_OBJ, 0, 0, 2, background->unk0x830, 0x3f800000, 0);
    }
    else
    {
        /* D3D virtual call @ vtbl+0x90 (Clear) */
        ((void(__stdcall *)(void *, void *, void *, u32, u32, f32, u32))(*(void ***) * (void **)D3D_DEVICE_OBJ)[0x90 / 4])(
            *(void **)D3D_DEVICE_OBJ, 0, 0, 2, background->unk0x830, 0x3f800000, 0);
    }

    g_Supervisor.SetRenderState((D3DRENDERSTATETYPE)0x17, 4);

    if (g_AnmManager->useMixColor == 0)
    {
        g_Supervisor.SetRenderState((D3DRENDERSTATETYPE)0x22, background->fog.color.d3dColor);
    }
    else
    {
        /* 用混合色 g_AnmManager->color 逐通道调制雾色（0x462750 = 通道相乘）。 */
        u32 c = background->fog.color.d3dColor;
        u8 c0 = (u8)(c >> 24);
        u8 c1 = (u8)(c >> 16);
        u8 c2 = (u8)(c >> 8);

        c2 = ((u8(__fastcall *)(u8, u8))0x462750)(c2, g_AnmManager->color.r);
        c1 = ((u8(__fastcall *)(u8, u8))0x462750)(c1, g_AnmManager->color.g);
        c0 = ((u8(__fastcall *)(u8, u8))0x462750)(c0, g_AnmManager->color.b);

        g_Supervisor.SetRenderState((D3DRENDERSTATETYPE)0x22, (c0 << 24) | (c1 << 16) | (c2 << 8) | (u8)c);
    }

    /* 雾的近/远平面距离（按位传给渲染状态）。 */
    g_Supervisor.SetRenderState((D3DRENDERSTATETYPE)0x24, *(u32 *)&background->fog.nearPlane);
    g_Supervisor.SetRenderState((D3DRENDERSTATETYPE)0x25, *(u32 *)&background->fog.farPlane);

    if (!g_Supervisor.IsFogDisabled())
    {
        g_Supervisor.EnableFog();
    }

    if ((i32)background->unk0xb24 <= 1)
    {
        if (g_Gui.FUN_00437d87() == 0)
        {
            background->FUN_0040a1b0(0);
            background->FUN_0040a1b0(1);
        }
    }

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: th08 0x409640 (93% FIXME: 625c/SetCamera1/虚函数寄存器)
ChainCallbackResult Background::OnDrawLowPrio(Background *background)
{
    i32 i;

    if ((i32)background->unk0xb24 <= 1)
    {
        if (g_Gui.FUN_00437d87() == 0)
        {
            background->FUN_0040a1b0(2);
            background->FUN_0040a1b0(3);

            if (!g_Supervisor.IsFogDisabled())
            {
                g_Supervisor.DisableFog();
            }

            g_EffectManager.FUN_004281e0();

            if (background->unk0xb24 == 1)
            {
                ZunRect rect = {32.0f, 16.0f, 416.0f, 464.0f};
                i32 alpha = (background->unk0xb28 * 0xff) / 0x3c;

                g_AnmManager->FlushVertexBuffer();
                g_Supervisor.SetRenderState((D3DRENDERSTATETYPE)0x17, 8);

                if (!g_Supervisor.IsFogDisabled())
                {
                    g_Supervisor.SetRenderState((D3DRENDERSTATETYPE)0x1c, 0);
                }

                ScreenEffect::DrawSquare(&rect, alpha << 0x18);
            }
        }
    }

    g_AnmManager->FlushVertexBuffer();
    g_Supervisor.SetRenderState((D3DRENDERSTATETYPE)0x17, 8);

    if (!g_Supervisor.IsFogDisabled())
    {
        g_Supervisor.DisableFog();
    }

    if ((i32)background->unk0xb24 >= 1)
    {
        for (i = 0; i < background->unk0xb30; i++)
        {
            g_AnmManager->FUN_0040baf0(&background->objectVms[i]);
        }

        if (background->unk0x625c != NULL)
        {
            background->unk0x625c(background);
        }
    }

    g_AnmManager->SetCameraMode(0);

    /* out-of-line SetCamera1 @ 0x40b5a0 */
    ((void(__fastcall *)(Background *))0x40b5a0)(background);

    /* D3D viewport-set virtual call @ vtbl+0xa0 */
    ((void(__stdcall *)(void *, void *))(*(void ***) * (void **)D3D_DEVICE_OBJ)[0xa0 / 4])(
        *(void **)D3D_DEVICE_OBJ, (void *)D3D_DEVICE_VIEWPORT);

    /* 雾距离（1000.0f / 2000.0f 的位模式），仅低优先级阶段设置。 */
    g_Supervisor.SetRenderState((D3DRENDERSTATETYPE)0x24, 0x447a0000);
    g_Supervisor.SetRenderState((D3DRENDERSTATETYPE)0x25, 0x44fa0000);

    if (background->unk0x646c == 0)
    {
        g_AnmManager->FUN_0040bab0();
    }

    background->unk0x646c = 0;
    background->unk0x647c = 0;

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// STUB: th08 0x40a1b0
void Background::FUN_0040a1b0(u32 param)
{
}

// STUB: th08 0x40b5a0
void Background::SetCamera1()
{
}

// FUNCTION: th08 0x409850
ZunResult Background::AddedCallback(Background *background)
{
    background->timer0x80c = 0;
    *(u32 *)&background->unk0x818 = 0;
    background->unk0x824.x = 0.0f;
    background->unk0x824.y = 0.0f;
    background->unk0x824.z = 0.0f;
    background->unk0xb24 = 0;
    background->unk0xb10 = 0;

    if (!IsDisableResourceReload())
    {
        background->stageAnm = g_AnmManager->PreloadAnm(4, g_StageAnmFiles[g_GameManager.currentStage]);
        if (background->stageAnm == NULL)
        {
            return ZUN_ERROR;
        }
    }
    else
    {
        background->stageAnm = g_AnmManager->GetAnm(4);
    }

    if (!g_GameManager.IsSpellPractice())
    {
        if (background->LoadStageData((char *)g_StageStdFiles[g_GameManager.currentStage]))
        {
            return ZUN_ERROR;
        }
    }
    else
    {
        if (background->LoadStageData((char *)g_StageStdFilesSpellPractice[g_GameManager.currentStage]))
        {
            return ZUN_ERROR;
        }
    }

    background->fog.color.d3dColor = 0xff000000;
    background->fog.nearPlane = 200.0f;
    background->fog.farPlane = 500.0f;

    background->camera4.pos = Float3(0.0f, 0.0f, 1000.0f);
    background->camera4.target = Float3(0.0f, 0.0f, 0.0f);
    background->camera4.unk0x3c = Float3(0.0f, 0.0f, 0.0f);
    background->camera4.up = Float3(0.0f, 1.0f, 0.0f);
    background->camera4.fov = ZUN_PI / 6.0f;

    background->camera0 = background->camera4;
    background->camera1 = background->camera4;

    background->unk0x6474 = 0;

    for (i32 i = 0; i < 4; i++)
    {
        background->unk0x63e0[i] = 0;
        background->timers0x63f4[i] = 0;
    }

    background->unk0x6260 = 0;
    *(u32 *)&background->unk0x6470 = 0x49a17020;

    if (g_GameManager.currentStage == 5)
    {
        *(u32 *)&background->unk0x6470 = 0x49de7920;
    }
    else if (g_GameManager.currentStage == 6 || g_GameManager.currentStage == 7)
    {
        *(u32 *)&background->unk0x6470 = 0x4a45c100;
    }

    return ZUN_SUCCESS;
}

ZunResult Background::RegisterChain(i32 stage)
{
    Background *background = &g_Background;
    StdRawHeader *savedStdData;

    if (IsDisableResourceReload())
    {
        savedStdData = background->stdData;
    }
    memset(background, 0, sizeof(Background));
    if (IsDisableResourceReload())
    {
        background->stdData = savedStdData;
    }
    background->unk81c = 0;
    background->currentStage = stage;
    g_BackgroundCalcChain.SetCallback((ChainCallback)OnUpdate);
    g_BackgroundCalcChain.addedCallback = (ChainLifetimeCallback)AddedCallback;
    g_BackgroundCalcChain.deletedCallback = (ChainLifetimeCallback)DeletedCallback;
    g_BackgroundCalcChain.arg = background;
    if (g_Chain.AddToCalcChain(&g_BackgroundCalcChain, 8))
    {
        return ZUN_ERROR;
    }
    g_BackgroundDrawChainHighPrio.SetCallback((ChainCallback)OnDrawHighPrio);
    g_BackgroundDrawChainHighPrio.arg = background;
    g_Chain.AddToDrawChain(&g_BackgroundDrawChainHighPrio, 6);
    g_BackgroundDrawChainLowPrio.SetCallback((ChainCallback)OnDrawLowPrio);
    g_BackgroundDrawChainLowPrio.arg = background;
    g_Chain.AddToDrawChain(&g_BackgroundDrawChainLowPrio, 7);
    return ZUN_SUCCESS;
}

ZunResult Background::DeletedCallback(Background *background)
{
    if (!IsDisableResourceReload())
    {
        g_AnmManager->ReleaseAnm(4);
    }
    if (background->fileData != NULL)
    {
        g_ZunMemory.RemoveFromRegistry(background->fileData);
        background->fileData = NULL;
    }
    if (!IsDisableResourceReload())
    {
        if (background->stdData != NULL)
        {
            g_ZunMemory.RemoveFromRegistry(background->stdData);
            background->stdData = NULL;
        }
    }
    return ZUN_SUCCESS;
}

void Background::CutChain()
{
    g_Chain.Cut(&g_BackgroundCalcChain);
    g_Chain.Cut(&g_BackgroundDrawChainHighPrio);
    g_Chain.Cut(&g_BackgroundDrawChainLowPrio);
}

#pragma var_order(vmIdx, i, obj, quad)
ZunResult Background::LoadStageData(char *stdPath)
{
    i32 vmIdx;
    i32 i;
    StdRawObject *obj;
    StdRawQuadBasic *quad;

    if (IsDisableResourceReload() == 0)
    {
        this->stdData = (StdRawHeader *)FileSystem::OpenFile(stdPath, NULL, FALSE);
        if (this->stdData == NULL)
        {
            g_GameErrorContext.Log("\x83X\x83" "e\x81[\x83W\x83" "f\x81[\x83^\x82\xaa\x8c\xa9\x82\xc2\x82\xa9\x82\xe8\x82\xdc\x82\xb9\x82\xf1\x81" "B\x83" "f\x81[\x83^\x82\xaa\x89\xf3\x82\xea\x82\xc4\x82\xa2\x82\xdc\x82\xb7\x0d\x0a");
            return ZUN_ERROR;
        }
    }
    this->objectsCount = this->stdData->objectsCount;
    this->quadCount = this->stdData->quadCount;
    this->objectInstances = (StdRawInstance *)(this->stdData->facesOffset + (i32)this->stdData);
    this->beginningOfScript = (StdRawInstr *)(this->stdData->scriptOffset + (i32)this->stdData);
    this->objects = (StdRawObject **)(this->stdData + 1);
    if (IsDisableResourceReload() == 0)
    {
        for (i = 0; i < this->objectsCount; i++)
        {
            this->objects[i] = (StdRawObject *)((i32)this->objects[i] + (i32)this->stdData);
        }
    }
    this->fileData = g_ZunMemory.Alloc(this->quadCount * sizeof(AnmVm), "bgscroll");
    for (i = 0, vmIdx = 0; i < this->objectsCount; i++)
    {
        obj = this->objects[i];
        obj->flags = 1;
        quad = &obj->firstQuad;
        while (quad->type >= 0)
        {
            this->stageAnm->ExecuteAnmIdx(&((AnmVm *)this->fileData)[vmIdx], quad->anmScript);
            quad->vmIndex = vmIdx++;
            quad = (StdRawQuadBasic *)((i32)quad + quad->byteSize);
        }
    }
    switch (g_GameManager.currentStage)
    {
    case 2:
        g_Supervisor.textAnm->SetAndExecuteScriptIdx(&this->stageTintVm, 0x21);
        break;
    default:
        g_Supervisor.textAnm->SetAndExecuteScriptIdx(&this->stageTintVm, 0x21);
        break;
    }
    this->stageTintVm.SetInterrupt(2);
    this->unk0x834 = 0;
    this->timer0x838 = 0;
    return ZUN_SUCCESS;
}


// FUNCTION: th08 0x408d60 (89.67% FIXME: *p1=*r 拷贝寄存器 ecx/edx)
void __fastcall Background::FUN_00408d60(i32 idx, Float3 *p1, Float3 *p2, Float3 *p3, Float3 *p4, Float3 *p5)
{
    f32 f;

    if (this->timers0x63f4[idx] < (i32)this->unk0x63e0[idx])
    {
        this->timers0x63f4[idx].Tick();
        f = (f32)this->timers0x63f4[idx] / (i32)this->unk0x63e0[idx];
    }
    else
    {
        this->timers0x63f4[idx].SetCurrent(this->unk0x63e0[idx]);
        f = 1.0f;
        this->unk0x63e0[idx] = 0;
    }

    switch (this->unk0x6430[idx])
    {
    case 1:
        f = 1.0f - f;
        f = 1.0f - f * f;
        break;
    case 2:
        f = 1.0f - f;
        f = 1.0f - f * f * f;
        break;
    case 3:
        f = 1.0f - f;
        f = 1.0f - f * f * f * f;
        break;
    case 4:
        f = f * f;
        break;
    case 5:
        f = f * f * f;
        break;
    case 6:
        f = f * f * f * f;
        break;
    }

    if (this->unk0x6430[idx] != 7)
    {
        f32 tmp1[3], tmp2[3], tmp3[3];

        *p1 = *FUN_004090d0(p3, (Float3 *)tmp1, p2);
        *p1 = *FUN_00409080(FUN_00409120(p1, (Float3 *)tmp3, f, (Float3 *)tmp2, p2));
    }
    else
    {
        p1->x = FUN_00408fc0(p2->x, p3->x, p4->x, p5->x, f);
        p1->y = FUN_00408fc0(p2->y, p3->y, p4->y, p5->y, f);
        p1->z = FUN_00408fc0(p2->z, p3->z, p4->z, p5->z, f);
    }
}

// FUNCTION: th08 0x409f40 (65% FIXME: 寄存器/寻址 lea vs add)
void Background::FUN_00409f40()
{
    i32 i;

    if (this->unk0x834 != 0)
    {
        if (g_Player.IsHuman() != 0)
        {
            this->unk0x834 = 0;
            this->timer0x838.SetCurrent(0);
            this->stageTintVm.SetInterrupt(2);
        }
    }
    else
    {
        if (g_Player.IsYoukai() != 0)
        {
            this->unk0x834 = 1;
            this->timer0x838.SetCurrent(0);
            this->stageTintVm.SetInterrupt(1);
        }
    }

    this->timer0x838.Tick();
    g_AnmManager->ExecuteScript(&this->stageTintVm);

    for (i = 0; i < this->objectsCount; i++)
    {
        void *elem = this->objects[i];
        StdRawQuadBasic *quad;
        AnmVm *vm;
        i32 count;

        if (((StdRawObject *)elem)->flags & 1)
        {
            count = 0;
            quad = &((StdRawObject *)elem)->firstQuad;

            while (quad->type >= 0)
            {
                /* 注意：与 LoadStageData 不同，这里按 `this + vmIndex*sizeof(AnmVm)` 取 VM（原版如此）。 */
                vm = (AnmVm *)((u8 *)this + quad->vmIndex * 0x2a4);

                switch (quad->type)
                {
                case 0:
                    g_AnmManager->ExecuteScript(vm);
                    break;
                case 1:
                    g_AnmManager->ExecuteScript(vm);
                    break;
                }

                if (vm->currentInstruction != 0)
                {
                    count++;
                }

                quad = (StdRawQuadBasic *)((u8 *)quad + quad->byteSize);
            }

            /* type==1：用 stageTintVm 的 color1 逐通道给物体 VM 染色。 */
            if (vm->prefix.type == 1)
            {
                vm->prefix.flags |= 0x20000;
                vm->prefix.color2.r = (u8)((vm->prefix.color1.r * this->stageTintVm.prefix.color1.r) >> 8);
                vm->prefix.color2.g = (u8)((vm->prefix.color1.g * this->stageTintVm.prefix.color1.g) >> 8);
                vm->prefix.color2.b = (u8)((vm->prefix.color1.b * this->stageTintVm.prefix.color1.b) >> 8);
                vm->prefix.color2.a = (u8)((vm->prefix.color1.a * this->stageTintVm.prefix.color1.a) >> 8);
            }

            if (count == 0)
            {
                ((StdRawObject *)elem)->flags &= ~0x1;
            }
        }
    }
}

// FUNCTION: th08 0x426d10
void __fastcall FUN_00426d10(Float3 *pos)
{
    EffectManagerParticle *p = &g_EffectManager.particles[0];

    for (i32 i = 0; i < 512; i++, p++)
    {
        if (p->unk351 == 0x33)
        {
            p->unk2d4 += *pos;
        }
    }
}

// STUB: th08 0x4090d0
Float3 *__fastcall FUN_004090d0(Float3 *self, Float3 *out, Float3 *p2)
{
    return NULL;
}

// STUB: th08 0x409120
Float3 *__fastcall FUN_00409120(Float3 *self, Float3 *tmp3, f32 f, Float3 *tmp2, Float3 *p2)
{
    return NULL;
}

// STUB: th08 0x409080
Float3 *__fastcall FUN_00409080(Float3 *self)
{
    return NULL;
}

// FUNCTION: th08 0x408fc0 (cubic interpolation between four samples, t in [0,1])
f32 __stdcall FUN_00408fc0(f32 a, f32 b, f32 c, f32 d, f32 t)
{
    f32 w0 = (t - 1.0f) * (t - 1.0f) * (2.0f * t + 1.0f);
    f32 w1 = t * t * (3.0f - 2.0f * t);
    f32 w2 = (1.0f - t) * (1.0f - t) * t;
    f32 w3 = (t - 1.0f) * t * t;

    return w0 * a + w1 * b + w2 * c + w3 * d;
}

// FUNCTION: th08 0x410a70
Float3 *Float3::operator+=(const Float3 &other)
{
    this->x += other.x;
    this->y += other.y;
    this->z += other.z;

    return this;
}

}; // Namespace th08
