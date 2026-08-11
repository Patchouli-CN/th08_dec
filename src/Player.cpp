#include "th_pch.h"

#include "Player.hpp"
#include "AsciiManager.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "ScreenEffect.hpp"
#include "SoundPlayer.hpp"
#include "Supervisor.hpp"

namespace th08
{

DIFFABLE_STATIC(Player, g_Player);
DIFFABLE_STATIC(u8, g_PlayerUnknown0bb);
DIFFABLE_STATIC(Float2, g_PlayerPos);
DIFFABLE_STATIC(PlayerRawShtFile *, g_PlayerShtFile);
DIFFABLE_STATIC(PlayerRawShtFile *, g_PlayerShtFile2);
DIFFABLE_STATIC(u8, g_PlayerCharacter); // 0x164d0b1
DIFFABLE_STATIC(u32, g_PlayerFlags);    // 0x164d0b4
DIFFABLE_STATIC_ARRAY(u16, 6, g_PlayerPalette);   // 0x164d300 main/sub colors
DIFFABLE_STATIC_ARRAY(u32, 6, g_GaugeStats);      // 0x164d318
DIFFABLE_STATIC(u16, g_KeyInput);                 // 0x164d52c
DIFFABLE_STATIC(u16, g_KeyInput2);                // 0x164d534
DIFFABLE_STATIC(i32, g_GuiDisplayState);          // 0x160f42c
DIFFABLE_STATIC_ARRAY(u32, 8, g_BulletObjects);   // 0xf54cc0
DIFFABLE_STATIC(f32, g_ShotSpeed);                // 0x17ce8e0
DIFFABLE_STATIC(i32, g_Unknown164d2c8);           // 0x164d2c8
DIFFABLE_STATIC(f32, g_PlayerTargetX);            // 0x164d2e4
DIFFABLE_STATIC(f32, g_PlayerTargetY);            // 0x164d2e8
DIFFABLE_STATIC(ChainElem *, g_PlayerCalcChain);
DIFFABLE_STATIC(ChainElem *, g_PlayerDrawChainHighPrio);
DIFFABLE_STATIC(ChainElem *, g_PlayerDrawChainLowPrio);
DIFFABLE_STATIC(i32, g_Unknown57ad30);

// Simulates D3DXVECTOR3::D3DXVECTOR3() (0x40b460) at the per-field positionCenter construction.
Float3 *__fastcall PlayerPosCenter(Float3 *vec)
{
    return vec;
}

// STUB: th08 0x40d3d0
i32 __fastcall FUN_0040d3d0(ZunTimer *timer)
{
    return 0;
}

// STUB: th08 0x451d50
i32 __fastcall FUN_00451d50(Player *player)
{
    return 0;
}

// STUB: th08 0x450f60
void __fastcall FUN_00450f60(Player *player, i32 frames)
{
}

// STUB: th08 0x416130
void __fastcall FUN_00416130(void *p)
{
}

// STUB: th08 0x42adb0 (thiscall: ecx + one stack arg, callee cleans up)
class StubThiscall42adb0
{
  public:
    void FUN_0042adb0(i32 arg);
};

void StubThiscall42adb0::FUN_0042adb0(i32 arg)
{
}

// STUB: th08 0x44cba0
void __fastcall FUN_0044cba0(void *p)
{
}

// FUN_0044e0e0
static ZunBool IsUnk164Clear()
{
    return Supervisor::GetUnk164() == 0;
}

#pragma var_order(itemBottomRight, itemTopLeft)
ZunBool Player::CalcItemBoxCollision(Float3 *pos, Float3 *size)
{
    Float3 itemTopLeft;
    Float3 itemBottomRight;

    if (this->playerState != PLAYER_STATE_ALIVE && this->playerState != PLAYER_STATE_INVULNERABLE &&
        this->playerState != PLAYER_STATE_BORDER)
    {
        return FALSE;
    }
    itemTopLeft = *pos - *size / 2.0f;
    itemBottomRight = *pos + *size / 2.0f;
    if (this->grabItemTopLeft.x > itemBottomRight.x || this->grabItemBottomRight.x < itemTopLeft.x ||
        this->grabItemTopLeft.y > itemBottomRight.y || this->grabItemBottomRight.y < itemTopLeft.y)
    {
        return FALSE;
    }
    return TRUE;
}

#pragma var_order(dy, dx)
f32 Player::AngleToPlayer(Float3 *pos)
{
    f32 dy;
    f32 dx;

    dx = this->positionCenter.Float3::Float3().x - pos->x;
    dy = this->positionCenter.Float3::Float3().y - pos->y;
    if (dy == 0.0f && dx == 0.0f)
    {
        return ZUN_PI / 2.0f;
    }
    return atan2f(dy, dx);
}

ZunResult Player::RegisterChain(u32 param)
{
    Player *player = &g_Player;
    u32 savedUnkE2a74;
    u32 savedUnkE2a78;

    if (IsUnk164Clear())
    {
        savedUnkE2a74 = player->unkE2a74;
        savedUnkE2a78 = player->unkE2a78;
    }
    memset(player, 0, offsetof(Player, unkE2ab0));
    if (IsUnk164Clear())
    {
        player->unkE2a74 = savedUnkE2a74;
        player->unkE2a78 = savedUnkE2a78;
    }
    player->invulnerabilityTimer.SetCurrent(0);
    player->initParam = param;
    player->calcChain = g_Chain.CreateElem((ChainCallback)OnUpdate);
    player->calcChain->arg = player;
    player->calcChain->addedCallback = (ChainLifetimeCallback)AddedCallback;
    player->calcChain->deletedCallback = (ChainLifetimeCallback)DeletedCallback;
    if (g_Chain.AddToCalcChain(player->calcChain, 9))
    {
        return ZUN_ERROR;
    }
    player->drawChainHighPrio = g_Chain.CreateElem((ChainCallback)OnDrawHighPrio);
    player->drawChainLowPrio = g_Chain.CreateElem((ChainCallback)OnDrawLowPrio);
    player->drawChainHighPrio->arg = player;
    player->drawChainLowPrio->arg = player;
    g_Chain.AddToDrawChain(player->drawChainHighPrio, 9);
    g_Chain.AddToDrawChain(player->drawChainLowPrio, 10);
    return ZUN_SUCCESS;
}

// FUNCTION: th08 0x44c390 (98.48% FIXME: || 短路跳板布局)
ChainCallbackResult Player::OnUpdate(Player *player)
{
    if (*(i8 *)0x160f534 != 0)
    {
        if (*(u32 *)((u8 *)player + 0xbe834) != 0)
        {
            *(u32 *)(*(u32 *)((u8 *)player + 0xbe834) + 0x1f8) |= 0x80000;
        }

        if (player->unkE2b24 != 0)
        {
            *(u32 *)(player->unkE2b24 + 0x1f8) |= 0x80000;
        }

        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    if (*(u32 *)((u8 *)player + 0xbe834) != 0)
    {
        *(u32 *)(*(u32 *)((u8 *)player + 0xbe834) + 0x1f8) &= 0xfff7ffff;
    }

    if (player->unkE2b24 != 0)
    {
        *(u32 *)(player->unkE2b24 + 0x1f8) &= 0xfff7ffff;
    }

    player->FUN_0044c5b0();
    player->FUN_0044c650();

    if ((player->playerState == 2 && player->FUN_0044cbf0() != 0) || player->playerState == 1)
    {
        player->FUN_0044d180();
    }

    player->FUN_0044d2c0();

    if (player->playerState != 2 && player->playerState != 1)
    {
        player->FUN_0044aec0();
    }

    g_AnmManager->ExecuteScript((AnmVm *)((u8 *)player + 0x10));
    player->FUN_00451150();
    player->FUN_00451500();
    player->FUN_0044d420();

    if (g_Gui.FUN_004358bb() == 0)
    {
        g_GaugeStats[0] += 1;
        g_GaugeStats[1] += 1;

        if (g_GameManager.GaugeIsExtremelyHuman())
        {
            g_GaugeStats[3] += 1;
            g_GaugeStats[5] += 1;
            g_GameManager.AddScore(0x64);
        }
        else if (g_GameManager.GaugeIsExtremelyYoukai())
        {
            g_GaugeStats[2] += 1;
            g_GaugeStats[4] += 1;
            g_GameManager.AddScore(0x64);
        }
    }

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: th08 0x4512f0 (97% FIXME: if 下沉布局 + unk478 冗余 edx)
void Player::FUN_004512f0()
{
    PlayerBulletVm *p = this->bullets;

    for (i32 i = 0; i < 0x80; i++, p++)
    {
        if (p->state == 1)
        {
            if (*(i16 *)((u8 *)&p->vm + 0x1fc) != 0)
            {
                p->vm.SetZRotation(p->rotation);
            }

            *(f32 *)((u8 *)&p->vm + 0x208) = g_PlayerPos.x + p->offsetX;
            *(f32 *)((u8 *)&p->vm + 0x20c) = g_PlayerPos.y + p->offsetY;
            *(u32 *)((u8 *)&p->vm + 0x210) = 0x3ecccccd;

            if (p->hasCustomColor != 0)
            {
                *(u8 *)((u8 *)&p->vm + 0x1f2) = 0xff;
                *(u8 *)((u8 *)&p->vm + 0x1f1) = 0x40;
                *(u8 *)((u8 *)&p->vm + 0x1f0) = 0x40;
            }

            g_AnmManager->Draw2D(&p->vm);

            if (p->unk478 != NULL)
            {
                /* thiscall callback: ecx=this, call [p+0x478] */
                ((void (__fastcall *)(Player *))p->unk478)(this);
            }
        }
    }
}

// STUB: th08 0x449ff0
i32 Player::FUN_00449ff0(void *unkD34, void *unkD44)
{
    return 0;
}

// FUNCTION: th08 0x40bc20
i32 Player::IsHuman()
{
    return this->unk5 == 0;
}

// FUNCTION: th08 0x40bc40
i32 Player::IsYoukai()
{
    return this->unk5;
}

// FUNCTION: th08 0x44e350 (this is actually a ShotSlot*)
void Player::FUN_0044e350()
{
    ((ShotSlot *)this)->active = 0;
}

// FUNCTION: th08 0x44c5b0
void Player::FUN_0044c5b0()
{
    i32 i;
    ShotSlot *slot = this->shots;

    for (i = 0; i < 0x180; i++, slot++)
    {
        if (slot->lifespan < 0)
        {
            continue;
        }
        slot->lifespan -= 1;
        slot->unk8 += slot->unkC;
        slot->targetX += slot->unk18;
        slot->targetY += slot->unk1C;
        if (slot->lifespan <= 0)
        {
            ((Player *)slot)->FUN_0044e350();
        }
    }
}

// FUNCTION: th08 0x44c650 (70% FIXME: 射击状态机，多处寄存器/跳板布局不可修)
void Player::FUN_0044c650()
{
    i32 flag = 0;
    i32 i;

    /* 若处于"残机用完强制换机"状态，直接走换机流程。 */
    if (this->unk6 != 0 && this->shotIndex == 1)
    {
        flag = 1;
        goto switch_shot;
    }

    if (this->shotCooldown != 0)
    {
        this->shotCooldown -= 1;
    }

    if (*(i32 *)((u8 *)this + 0xfdc) != 0)
    {
        /* 射击进行中：到期则清场/结束，未到期则推进当前射击并涨妖气槽。 */
        if (FUN_0040d3d0(&this->shotTimer) != 0)
        {
            *(i32 *)0x160f42c = (*(i32 *)0x160f42c & 0xfffffcff) | 0x200;
        }

        if (this->shotTimer.operator>=(this->shotInterval))
        {
            FUN_00416130(&g_Spellcard);
            *(i32 *)((u8 *)this + 0xfdc) = 0;
            this->unk408 = 1.0f;
            this->unk404 = 1.0f;

            if (*(i32 *)((u8 *)this + 0xfe0) == 4)
            {
                *(i32 *)0x164d0b4 &= 0xfffffe7f;
                for (i = 0; i < 8u; i++)
                {
                    if (g_BulletObjects[i] != 0)
                    {
                        ((StubThiscall42adb0 *)(g_BulletObjects[i]))->FUN_0042adb0(0);
                        *(i32 *)(g_BulletObjects[i] + 0x2dfc) = 0;
                        *(i32 *)(g_BulletObjects[i] + 0x3324) &= 0xbfffffff;
                    }
                }
                ScreenEffect::RegisterChain((ScreenEffectType)0x3, 0x1e, 0x1, 0xffffffff, 0, 0x15);
            }
        }
        else
        {
            (this->*this->shotFuncs[this->unkFe0])();
            this->shotTimer.Tick(0);
        }

        if (*(i32 *)((u8 *)this + 0xfe0) < 4)
        {
            if (*(i32 *)((u8 *)this + 0xfe0) & 1)
            {
                g_GameManager.AddToYoukaiGauge(0x6590 / this->shotInterval, 1);
            }
            else
            {
                g_GameManager.AddToYoukaiGauge(-0x6590 / this->shotInterval, 1);
            }
        }
        return;
    }

    /* 未射击：检测"按了决死结界键且满足条件"则切换射击。 */
    if (*(u16 *)0x164d52c & 2 && !g_GameManager.IsTampered() &&
        !g_Gui.FUN_004358bb() && this->shotIndex != 0 &&
        g_GameManager.GetBombsRemaining() > 0 && this->shotCooldown == 0)
    {
        if ((g_PlayerFlags >> 7 & 3) == 0)
        {
            if ((g_PlayerFlags >> 0xe & 1) == 0)
            {
                goto switch_shot;
            }
        }
    }
sound_check:
    if (*(u16 *)0x164d52c & 2 && (*(u16 *)0x164d52c & 2) != (*(u16 *)0x164d534 & 2))
    {
        g_SoundPlayer.PlaySoundByIdx((SoundIdx)0x29, 0);
    }
    return;
    this->shotState = 0;
    return;

switch_shot:
    /* 切换射击：设置射击类型/消耗炸弹/初始化。 */
    *(u16 *)(*(i32 *)0x18b8a28 + 0xda) |= 1;
    this->unk6 = 0;

    if (g_PlayerFlags >> 7 & 3)
    {
        *(i32 *)((u8 *)this + 0xfe0) = 4;
    }
    else
    {
        *(u32 *)((u8 *)this + 0x208) &= 0xfffdffff;
        if (this->unkE2b28 != 0)
        {
            *(u8 *)(this->unkE2b28 + 0x350) = 0;
            this->unkE2b28 = 0;
        }
        *(i32 *)0x164d0b4 &= 0xfffffbff;
        g_AnmManager->FUN_0040bab0();

        *(i32 *)((u8 *)this + 0xfe0) = *(u8 *)((u8 *)this + 0x3);
        if (this->unk4 != 0)
        {
            *(i32 *)((u8 *)this + 0xfe0) = 1 - *(i32 *)((u8 *)this + 0xfe0);
        }

        if (this->unk4 != 0)
        {
            *(i32 *)((u8 *)this + 0xfe0) += 2;
            if (flag)
            {
                this->powerLevel = g_GameManager.GetBombsRemaining();
                g_GameManager.SetBombCount(0);
            }
            else if (g_GameManager.GetBombsRemaining() < 2)
            {
                this->powerLevel = g_GameManager.GetBombsRemaining();
                g_GameManager.SetBombCount(0);
            }
            else
            {
                this->powerLevel = 2;
                g_GameManager.AddToBombCount(-2);
            }
            *(i32 *)0x164cfac += 1;
        }
        else
        {
            *(i32 *)0x164cfa8 += 1;
            g_GameManager.AddToBombCount(-1);
        }
        g_GameManager.AddToBombsUsed(1);
    }

    this->unk4 = 0;
    *(i32 *)0x160f42c = (*(i32 *)0x160f42c & 0xfffffff3) | 8;
    *(i32 *)((u8 *)this + 0xfdc) = 1;
    this->shotState = 1;
    this->shotTimer.SetCurrent(0);
    this->shotInterval = 0x3e7;
    (this->*this->shotFuncs[this->unkFe0])();
    this->shotTimer.Tick(0);
    g_GameManager.DecreaseSubrank(0xc8);
    FUN_0044cba0(&g_Spellcard);
    this->shotIndex += 6;
    if (this->shotIndex > *(i32 *)((u8 *)g_PlayerShtFile + 8))
    {
        this->shotIndex = *(i32 *)((u8 *)g_PlayerShtFile + 8);
    }
}

// STUB: th08 0x44cbf0
i32 Player::FUN_0044cbf0()
{
    return 0;
}

// FUNCTION: th08 0x44e0f0
void Player::FUN_0044e0f0()
{
    *(u32 *)((u8 *)this + 0x1f8) = (*(u32 *)((u8 *)this + 0x1f8) & 0xffffffcf) | 0x10;
}

// FUNCTION: th08 0x44e120
void Player::FUN_0044e120()
{
    *(u32 *)((u8 *)this + 0x1f8) &= 0xffffffcf;
}

// FUNCTION: th08 0x44d180
void Player::FUN_0044d180()
{
    this->unkE2a70 = 0x3c;

    f32 temp = 1.0f - ((ZunTimer *)((u8 *)this + 0xe2af4))->AsFramesFloat() / 30.0f;
    *(f32 *)((u8 *)this + 0x2c) = 2.0f * temp + 1.0f;
    *(f32 *)((u8 *)this + 0x28) = 1.0f - 1.0f * temp;

    ((Player *)((u8 *)this + 0x10))->FUN_0044e0f0();

    this->unk408 = 1.0f;
    this->unk404 = 1.0f;

    *(i32 *)((u8 *)this + 0x200) =
        ((((ZunTimer *)((u8 *)this + 0xe2af4))->AsFrames() * 0xff) / 30 << 0x18) | 0xffffff;

    this->shotIndex = 0;

    if (((ZunTimer *)((u8 *)this + 0xe2af4))->AsFrames() >= 30)
    {
        *(u8 *)((u8 *)this + 0) = 3;
        *(f32 *)((u8 *)this + 0x28) = 1.0f;
        *(f32 *)((u8 *)this + 0x2c) = 1.0f;
        *(i32 *)((u8 *)this + 0x200) = 0xffffffff;
        ((Player *)((u8 *)this + 0x10))->FUN_0044e120();

        if (!(g_PlayerFlags >> 0xe & 1))
        {
            ((ZunTimer *)((u8 *)this + 0xe2af4))->SetCurrent(0xf0);
        }
        this->shotIndex = *(i32 *)((u8 *)g_PlayerShtFile + 8);
    }
}

// FUNCTION: th08 0x44de60 (97% FIXME: 找空槽循环的 jne/je 布局镜像不可修)
u32 Player::FUN_0044de60(Float3 *spawnPos, f32 targetX, f32 targetY, i32 unk28, i32 unk24)
{
    ShotSlot *slot = &this->shots[0xc0]; // 0xbb834, sub-range of shots[]
    i32 i = 0;

    goto check;
loop:
    i++;
    slot++;
check:
    if (i < 0xbf && slot->active != 0)
        goto loop;
    goto exit;
exit:
    ((Player *)slot)->FUN_0044e370();
    slot->active = 1;
    slot->posX = spawnPos->x;
    slot->posY = spawnPos->y;
    slot->targetX = targetX;
    slot->targetY = targetY;
    slot->lifespan = unk24;
    slot->unk28 = unk28;
    return (u32)slot;
}

// FUNCTION: th08 0x44d2c0
void Player::FUN_0044d2c0()
{
    if (this->unkE2a70 != 0)
    {
        this->unkE2a70 -= 1;
        this->FUN_0044de60(&this->positionCenter, 768.0f, 896.0f, -1, 0);
    }

    if (this->playerState == 3)
    {
        this->unk4 = 0;
        if (this->unkE2b1c != 0)
        {
            *(Float3 *)(this->unkE2b1c + 0x2a4) = this->positionCenter;
        }
        ((ZunTimer *)((u8 *)this + 0xe2af4))->operator--(0);
        if (((ZunTimer *)((u8 *)this + 0xe2af4))->AsFrames() <= 0)
        {
            if (this->unkE2b1c != 0)
            {
                *(u8 *)(this->unkE2b1c + 0x350) = 0;
                this->unkE2b1c = 0;
            }
            *(u8 *)((u8 *)this) = 0;
            ((ZunTimer *)((u8 *)this + 0xe2af4))->SetCurrent(0);
            *(i32 *)((u8 *)this + 0x200) = 0xffffffff;
        }
        else
        {
            if (((ZunTimer *)((u8 *)this + 0xe2af4))->AsFrames() % 8 < 2)
            {
                *(i32 *)((u8 *)this + 0x200) = 0xfff02020;
            }
            else
            {
                *(i32 *)((u8 *)this + 0x200) = 0xffffffff;
            }
        }
    }
    else
    {
        ((ZunTimer *)((u8 *)this + 0xe2af4))->Tick(0);
    }
}

// FUNCTION: th08 0x44aec0 (Player main update: direction -> option init -> firing.
// FIXME: 0x12a1 超大，目前只实现方向检测部分)
void Player::FUN_0044aec0()
{
    i32 oldDirection;

    /* 方向输入 → PlayerDirection。优先级：斜向 > 单方向。 */
    oldDirection = this->movementDirection;
    if ((g_KeyInput & 0x50) == 0x50)
        this->movementDirection = MOVEMENT_UP_LEFT;
    else if ((g_KeyInput & 0x60) == 0x60)
        this->movementDirection = MOVEMENT_DOWN_LEFT;
    else if ((g_KeyInput & 0x90) == 0x90)
        this->movementDirection = MOVEMENT_UP_RIGHT;
    else if ((g_KeyInput & 0xa0) == 0xa0)
        this->movementDirection = MOVEMENT_DOWN_RIGHT;
    else if (g_KeyInput & 0x20)
        this->movementDirection = MOVEMENT_DOWN;
    else if (g_KeyInput & 0x10)
        this->movementDirection = MOVEMENT_UP;
    else if (g_KeyInput & 0x40)
        this->movementDirection = MOVEMENT_LEFT;
    else if (g_KeyInput & 0x80)
        this->movementDirection = MOVEMENT_RIGHT;
}

// FUNCTION: th08 0x451150 (68% FIXME: 寄存器分配差异)
void Player::FUN_00451150()
{
    i32 i;
    PlayerBulletVm *b;

    /* 子弹动画暂停标志（0x164d0b4 bit 10）。 */
    if (g_PlayerFlags >> 0xa & 1)
    {
        return;
    }

    b = &this->bullets[0];
    for (i = 0; i < 0x80; i++, b++)
    {
        if (b->state == 0)
        {
            continue;
        }

        /* 每个子弹有独立的更新回调，返回非 0 则销毁。 */
        if (b->unk474 != 0)
        {
            if (((i32(__fastcall *)(Player *))b->unk474)(this) != 0)
            {
                b->state = 0;
                continue;
            }
        }

        /* 按速度推进子弹位置。 */
        PlayerPosCenter((Float3 *)&b->offsetX)->x += *(f32 *)0x17ce8e0 * b->unk43c;
        PlayerPosCenter((Float3 *)&b->offsetX)->y += *(f32 *)0x17ce8e0 * b->unk440;

        /* 非"遗留型"子弹离开活动区域则销毁。 */
        if (b->state464 != 4 && b->state464 != 5)
        {
            if (!g_GameManager.IsWithinPlayfield(
                    PlayerPosCenter((Float3 *)&b->offsetX)->x,
                    PlayerPosCenter((Float3 *)&b->offsetX)->y,
                    *(f32 *)(*(i32 *)((u8 *)b + 0x224) + 0x30),
                    *(f32 *)(*(i32 *)((u8 *)b + 0x224) + 0x34)))
            {
                b->state = 0;
            }
        }

        if (g_AnmManager->ExecuteScript((AnmVm *)b) != 0)
        {
            b->state = 0;
        }

        b->timer454.Tick(0);
    }
}

// FUNCTION: th08 0x451500
i32 Player::FUN_00451500()
{
    if (*(i32 *)0x164d2c8 < 0x14)
    {
        return 0;
    }

    if (this->shotTimer2.AsFrames() < 0)
    {
        return 0;
    }

    if (FUN_00451d50(this) != 0)
    {
        return 0;
    }

    if (FUN_0040d3d0(&this->shotTimer2) != 0)
    {
        if (*(i32 *)0x17d6ed4 != 0)
        {
            if (g_PlayerCharacter != 1 && g_PlayerCharacter != 7 && g_PlayerCharacter != 6)
            {
                FUN_00450f60(this, this->shotTimer2.AsFrames());
            }
        }
    }

    this->shotTimer2.Tick(0);

    if (this->shotTimer2.AsFrames() >= 0x14)
    {
        this->shotTimer2.SetCurrent(-1);
    }

    if (*(u16 *)0x164d52c & 1)
    {
        if (this->shotTimer2.AsFrames() < 0)
        {
            if (g_Gui.FUN_004358bb() == 0)
            {
                this->shotTimer2.SetCurrent(0);
            }
        }
    }

    if (this->playerState == 2 || this->playerState == 1)
    {
        this->shotTimer2.SetCurrent(-1);
    }

    return 0;
}

// FUNCTION: th08 0x44d420
void Player::FUN_0044d420()
{
    this->unkE2aa4 = Float3(-999.0f, -999.0f, 0.0f);
    this->unkE2ab0 = Float3(-999.0f, -999.0f, 0.0f);
    this->unkE2ac0 = 0;

    if (*(f32 *)((u8 *)this + 0x2b8) >= 400.0f)
    {
        if (g_AsciiManager.GetGaugeInterrupt() != 2)
        {
            if (*(f32 *)((u8 *)this + 0x2b4) < 160.0f)
            {
                g_AsciiManager.SetGaugeInterrupt(2);
                goto merge;
            }
        }
        if (g_AsciiManager.GetGaugeInterrupt() == 2)
        {
            if (*(f32 *)((u8 *)this + 0x2b4) > 160.0f)
            {
                g_AsciiManager.SetGaugeInterrupt(3);
            }
        }
    merge:;
    }
    else
    {
        if (g_AsciiManager.GetGaugeInterrupt() == 2)
        {
            g_AsciiManager.SetGaugeInterrupt(3);
        }
    }
}

// FUNCTION: th08 0x44e370 (this is actually a ShotSlot*)
void Player::FUN_0044e370()
{
    memset((ShotSlot *)this, 0, 0x40);
    ((ShotSlot *)this)->unk38 = 1;
}

// FUNCTION: th08 0x44d530
ChainCallbackResult Player::OnDrawHighPrio(Player *player)
{
    player->FUN_004512f0();

    if (player->unkFdc != 0)
    {
        (player->*player->unk_1014[player->unkFe0])();
    }

    if (g_PlayerUnknown0bb == 0)
    {
        player->unk_218 = g_PlayerPos.x + player->positionCenter.x;
        player->unk_21c = g_PlayerPos.y + player->positionCenter.y;
        player->unk_220 = 0.1f;
        g_AnmManager->DrawNoRotation((AnmVm *)&player->unk_10);
    }

    for (u32 i = 0; i < 4; i++)
    {
        if (player->options[i].func != NULL)
        {
            (player->*player->options[i].func)();
        }
    }

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

void Player::DrawBulletVms()
{
    PlayerBulletVm *bullet;
    i32 i;

    bullet = &this->bullets[0];
    for (i = 0; i < 0x80; i++, bullet++)
    {
        if (bullet->state != 2)
        {
            continue;
        }
        if (bullet->vm.prefix.type != 0)
        {
            bullet->vm.SetZRotation(bullet->rotation);
        }
        bullet->vm.pos.x = g_GameManager.arcadeRegionTopLeftPos.x + bullet->offsetX;
        bullet->vm.pos.y = g_GameManager.arcadeRegionTopLeftPos.y + bullet->offsetY;
        bullet->vm.pos.z = 0.2f;
        if (bullet->hasCustomColor)
        {
            bullet->vm.prefix.color1.r = 0xff;
            bullet->vm.prefix.color1.g = 0x40;
            bullet->vm.prefix.color1.b = 0x40;
        }
        g_AnmManager->DrawPlayerBullet(&bullet->vm);
    }
}

ChainCallbackResult Player::OnDrawLowPrio(Player *player)
{
    player->DrawBulletVms();
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: th08 0x44d650 (32.7% FIXME: Float3 字段赋值已修，字段/分支待迭代)
ZunResult Player::AddedCallback(Player *player)
{
    i32 i;
    PlayerBulletVm *bullet;

    if (g_Supervisor.GetUnk164())
    {
        if (Player::LoadShtFile((PlayerRawShtFile **)&player->unkE2a74,
                                *(const char **)(0x4c7ce0 + g_PlayerCharacter * 4)) != 0)
        {
            return (ZunResult)-1;
        }

        if (Player::LoadShtFile((PlayerRawShtFile **)&player->unkE2a78,
                                *(const char **)(0x4c7d10 + g_PlayerCharacter * 4)) != 0)
        {
            return (ZunResult)-1;
        }

        *(AnmLoaded **)((u8 *)player + 0xc) =
            g_AnmManager->PreloadAnm(5, *(const char **)(0x4c7cb0 + g_PlayerCharacter * 4));

        if (*(AnmLoaded **)((u8 *)player + 0xc) == NULL)
        {
            return (ZunResult)-1;
        }
    }
    else
    {
        *(AnmLoaded **)((u8 *)player + 0xc) = g_AnmManager->GetAnm(5);
    }

    if (!(g_PlayerCharacter >= 4 && (g_PlayerCharacter & 1)))
    {
        (*(AnmLoaded **)((u8 *)player + 0xc))->SetAndExecuteScriptIdx((AnmVm *)((u8 *)player + 0x10), 0);
    }
    else
    {
        (*(AnmLoaded **)((u8 *)player + 0xc))->SetAndExecuteScriptIdx((AnmVm *)((u8 *)player + 0x10), 5);
    }

    /* 原版对 `positionCenter = Float3(...)` 编译成三次构造器调用，见 PlayerPosCenter。 */
    PlayerPosCenter(&player->positionCenter)->x = *(f32 *)0x164d2e4 / *(f32 *)0x4b42ec;
    PlayerPosCenter(&player->positionCenter)->y = *(f32 *)0x164d2e8 - *(f32 *)0x4b42c8;
    PlayerPosCenter(&player->positionCenter)->z = 0.48f;

    for (i = 0; i < 0x180u; i++)
    {
        ((Player *)&player->shots[i])->FUN_0044e370();
    }

    player->shotSpeed3d8 = *(f32 *)((u8 *)g_PlayerShtFile + 0xc) / *(f32 *)0x4b42ec;
    player->shotSpeed3d4 = player->shotSpeed3d8;
    player->unk3dc = 5.0f;
    player->shotSpeed3e4 = *(f32 *)((u8 *)g_PlayerShtFile + 0x10) / *(f32 *)0x4b42ec;
    player->shotSpeed3e0 = player->shotSpeed3e4;
    player->unk3e8 = 5.0f;
    player->shotSpeed3f0 = *(f32 *)((u8 *)g_PlayerShtFile + 0x18) / *(f32 *)0x4b42ec;
    player->shotSpeed3ec = player->shotSpeed3f0;
    player->unk3f4 = 5.0f;

    player->movementDirection = 0;

    player->playerState = 1;

    ((ZunTimer *)((u8 *)player + 0xe2af4))->SetCurrent(g_GameManager.GetFlag14() ? 0xa : 0x78);

    player->unk2 = 1;

    bullet = &player->bullets[0];
    for (i = 0; i < 0x80; i++, bullet++)
    {
        bullet->state = 0;
    }

    player->shotTimer2.SetCurrent(-1);
    player->unkE2ad0.SetCurrent(0);
    player->unkE2ae8.SetCurrent(0);

    /* 角色射击回调表拷贝（两次，均为 5 dword）。 */
    memcpy((u8 *)player + 0x1000, (void *)(0x4c7ad0 + (g_PlayerCharacter * 2) * 0x14), 0x14);
    memcpy((u8 *)player + 0x1014, (void *)(0x4c7ad0 + (g_PlayerCharacter * 2 + 1) * 0x14), 0x14);

    *(i32 *)((u8 *)player + 0xfdc) = 0;
    player->unkE2b0c = -1.57f;
    player->unk408 = 1.0f;
    player->unk404 = 1.0f;
    player->shotIndex = *(i32 *)((u8 *)g_PlayerShtFile + 0x8);

    if (g_Supervisor.GetUnk164())
    {
        g_AsciiManager.SetGaugeInterrupt(1);
    }

    g_AsciiManager.SetBossMarkerInterrupt(0, 2);
    g_AsciiManager.SetBossMarkerInterrupt(1, 2);
    g_AsciiManager.SetBossMarkerInterrupt(2, 2);

    /* 主/次颜色默认值，再按角色覆盖。 */
    g_PlayerPalette[0] = 0xd8f0;
    g_PlayerPalette[2] = 0xe0c0;
    g_PlayerPalette[4] = 0xf830;
    g_PlayerPalette[1] = 0x2710;
    g_PlayerPalette[3] = 0x1f40;
    g_PlayerPalette[5] = 0x7d0;

    if (g_PlayerCharacter == 3)
    {
        g_PlayerPalette[0] = 0xec78;
        g_PlayerPalette[2] = 0xf448;
        g_PlayerPalette[4] = 0xf830;
    }
    else if (g_PlayerCharacter == 0xa)
    {
        g_PlayerPalette[0] = 0xec78;
        g_PlayerPalette[2] = 0xf448;
        g_PlayerPalette[4] = 0xf830;
        g_PlayerPalette[1] = 0x1388;
        g_PlayerPalette[3] = 0xbb8;
        g_PlayerPalette[5] = 0x7d0;
    }
    else if (g_GameManager.IsSoloHuman())
    {
        g_PlayerPalette[1] = 0x7d0;
        g_PlayerPalette[3] = 0x1f40;
        g_PlayerPalette[5] = 0x7d1;
    }
    else if (g_GameManager.IsSoloYoukai())
    {
        g_PlayerPalette[0] = 0xf830;
        g_PlayerPalette[2] = 0xe0c0;
        g_PlayerPalette[4] = 0xf82f;
    }

    player->unkE2b24 = 0;

    for (i = 0; i < 0x10u; i++)
    {
        *(Float3 *)((u8 *)player + 0x2cc + i * 0xc) = player->positionCenter;
    }

    *(u8 *)((u8 *)player + 0x3) = 2;

    if (g_PlayerCharacter > 3)
    {
        PlayerOption *opt = &player->options[0];
        for (u32 k = 0; k < 4; k++, opt++)
        {
            memset(opt, 0, 0x2f4);
            opt->unk2ec = *(i32 *)(0x4c7d40 + g_PlayerCharacter * 0x10 + k * 4);
            (i32 &)opt->func = *(i32 *)(0x4c7e10 + g_PlayerCharacter * 0x10 + k * 4);
            if (opt->unk2ec != 0)
            {
                opt->unk2c8 = 1;
                opt->timer2e0.SetCurrent(0);
                opt->unk2d0 = k;
            }
            else
            {
                opt->unk2c8 = 0;
            }
        }
    }

    if (g_GameManager.IsSoloHuman())
    {
        player->unkE2b2c = 0x1b;
    }
    else
    {
        player->unkE2b2c = 0x28;
    }
    *(i32 *)&g_Unknown57ad30 = player->unkE2b2c;

    return ZUN_SUCCESS;
}

ZunResult Player::DeletedCallback(Player *player)
{
    if (Supervisor::GetUnk168() != 0)
    {
        g_AnmManager->ReleaseAnm(5);
        g_AsciiManager.SetGaugeInterrupt(99);
        g_AsciiManager.SetBossMarkerInterrupt(0, 99);
        g_AsciiManager.SetBossMarkerInterrupt(1, 99);
        g_AsciiManager.SetBossMarkerInterrupt(2, 99);
        if (g_PlayerShtFile != NULL)
        {
            g_ZunMemory.RemoveFromRegistry(g_PlayerShtFile);
            g_PlayerShtFile = NULL;
        }
        if (g_PlayerShtFile2 != NULL)
        {
            g_ZunMemory.RemoveFromRegistry(g_PlayerShtFile2);
            g_PlayerShtFile2 = NULL;
        }
    }
    return ZUN_SUCCESS;
}

void Player::CutChain()
{
    g_Chain.Cut(g_PlayerCalcChain);
    g_PlayerCalcChain = NULL;
    g_Chain.Cut(g_PlayerDrawChainHighPrio);
    g_PlayerDrawChainHighPrio = NULL;
    g_Chain.Cut(g_PlayerDrawChainLowPrio);
    g_PlayerDrawChainLowPrio = NULL;
}

DIFFABLE_STATIC_ARRAY_ASSIGN(u32, 9, g_PlayerShotTable1) = {
    0x00000000, 0x00450240, 0x0044fdd0, 0x0044fdd0, 0x0044fe20, 0x0044ffa0,
    0x00450080, 0x004501b0, 0x00450110,
};
DIFFABLE_STATIC_ARRAY_ASSIGN(u32, 6, g_PlayerShotTable2) = {
    0x00000000, 0x00450320, 0x00000000, 0x00450580, 0x004505d0, 0x00450840,
};
DIFFABLE_STATIC_ARRAY_ASSIGN(u32, 8, g_PlayerShotTable3) = {
    0x00000000, 0x00450ad0, 0x00000000, 0x00450c50, 0x00450ee0, 0x004b704c,
    0x004b7044, 0x004b703c,
};
DIFFABLE_STATIC_ARRAY_ASSIGN(u32, 8, g_PlayerShotTable4) = {
    0x00000000, 0x00450c50, 0x00450ee0, 0x004b704c, 0x004b7044, 0x004b703c,
    0x004b7034, 0x004b702c,
};

// FUNCTION: th08 0x44dd70
ZunResult __fastcall Player::LoadShtFile(PlayerRawShtFile **header, const char *path)
{
    *header = (PlayerRawShtFile *)FileSystem::OpenFile(path, NULL, FALSE);
    if (*header == NULL)
    {
        return ZUN_ERROR;
    }

    for (i32 i = 0; i < (*header)->entryCount; i++)
    {
        (*header)->entries[i].entry = (PlayerShotEntry *)((u32)(*header)->entries[i].entry + (u32)*header);
        PlayerShotEntry *entry = (*header)->entries[i].entry;

        while (entry->unk0 >= 0)
        {
            entry->unk28 = (i32)g_PlayerShotTable1[entry->unk28];
            entry->unk2c = (i32)g_PlayerShotTable2[entry->unk2c];
            entry->unk30 = (i32)g_PlayerShotTable3[entry->unk30];
            entry->unk34 = (i32)g_PlayerShotTable4[entry->unk34];
            entry = (PlayerShotEntry *)((u8 *)entry + sizeof(PlayerShotEntry));
        }
    }

    return ZUN_SUCCESS;
}

/* ZunTimer 方法原本在 Supervisor.cpp（/Os），epilogue 会生成 leave；原版这些方法
 * 用 mov esp,ebp; pop ebp，所以放在 Player.cpp（/Od）编译以匹配。 */
i32 ZunTimer::AsFrames()
{
    return this->current;
}

f32 ZunTimer::AsFramesFloat()
{
    return (f32)this->current + this->subFrame;
}

void ZunTimer::SetCurrent(i32 value)
{
    this->SetCurrentImpl(value);
}

void ZunTimer::SetCurrentImpl(i32 value)
{
    this->current = value;
    *(i32 *)&this->subFrame = 0;
    this->previous = -999;
}

void ZunTimer::Tick(i32 unused)
{
    this->TickImpl();
}

u32 ZunTimer::TickImpl()
{
    this->previous = this->current;
    g_Supervisor.TickTimer(&this->current, &this->subFrame);
    return this->current;
}

} /* namespace th08 */
