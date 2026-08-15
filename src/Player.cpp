#include "th_pch.h"

#include "Player.hpp"
#include "AsciiManager.hpp"
#include "EffectManager.hpp"
#include "EnemyManager.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "ItemManager.hpp"
#include "ReplayManager.hpp"
#include "ScreenEffect.hpp"
#include "SoundPlayer.hpp"
#include "Supervisor.hpp"

namespace th08
{

DIFFABLE_STATIC(Player, g_Player);
DIFFABLE_STATIC(u8, g_PlayerUnknown0b0); // 0x164d0b0
DIFFABLE_STATIC(u8, g_PlayerUnknown0bb);
DIFFABLE_STATIC(u8, g_PlayerUnknown0bd); // 0x164d0bd (player respawn marker)
DIFFABLE_STATIC(i16, g_CurrentSpellcardNumber); // 0x164d0b8
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
DIFFABLE_STATIC(i32, g_Unknown164d2cc);           // 0x164d2cc
DIFFABLE_STATIC(f32, g_PlayerTargetX);            // 0x164d2e4
DIFFABLE_STATIC(f32, g_PlayerTargetY);            // 0x164d2e8
DIFFABLE_STATIC(ChainElem *, g_PlayerCalcChain);
DIFFABLE_STATIC(ChainElem *, g_PlayerDrawChainHighPrio);
DIFFABLE_STATIC(ChainElem *, g_PlayerDrawChainLowPrio);
DIFFABLE_STATIC(i32, g_Unknown57ad30);
DIFFABLE_STATIC(f32, g_PlayerBoundaryLeft);    // 0x164d2ec player movement x minimum
DIFFABLE_STATIC(f32, g_PlayerBoundaryTop);     // 0x164d2f0 player movement y minimum
DIFFABLE_STATIC(f32, g_PlayerBoundaryWidth);   // 0x164d2f4 movement x extent
DIFFABLE_STATIC(f32, g_PlayerBoundaryHeight);  // 0x164d2f8 movement y extent
// Per-character option callback tables (indexed by [g_PlayerCharacter][option idx]).
DIFFABLE_STATIC(i32, g_OptionInitCallbacks[8][4]);      // 0x4c7d40
DIFFABLE_STATIC(i32, g_OptionUpdateCallbacks[8][4]);    // 0x4c7e10
DIFFABLE_STATIC_ARRAY(i32, 4, g_SpiritOptionInitCallbacks);   // 0x4c7e00
DIFFABLE_STATIC_ARRAY(i32, 4, g_SpiritOptionUpdateCallbacks); // 0x4c7ed0

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

// FUNCTION: th08 0x451d50
/* 当前是否处于第 4 号射击（妖形态特殊射击）进行中的状态。
   非 0 时禁止再发动新射击（UpdateChargeShotTimer 检查）。 */
i32 __fastcall IsSpecialShotActive(Player *player)
{
    return (player->shotActive != 0 && player->shotType == 4) ? 1 : 0;
}

// STUB: th08 0x450f60
void __fastcall FUN_00450f60(Player *player, i32 frames)
{
}

// STUB: th08 0x416130
void __fastcall FUN_00416130(void *p)
{
}

// STUB: th08 0x44cba0
void __fastcall FUN_0044cba0(void *p)
{
}

// STUB: th08 0x451640
void Player::ClampChargeShotTimer()
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
    u32 savedSpeedNormal;
    u32 savedSpeedSpirit;

    if (IsUnk164Clear())
    {
        savedSpeedNormal = (u32)player->moveSpeedNormal;
        savedSpeedSpirit = (u32)player->moveSpeedSpirit;
    }
    memset(player, 0, offsetof(Player, boundaryIndicatorRight));
    if (IsUnk164Clear())
    {
        player->moveSpeedNormal = (PlayerMoveSpeed *)savedSpeedNormal;
        player->moveSpeedSpirit = (PlayerMoveSpeedSpirit *)savedSpeedSpirit;
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
    /* 0x160f534：全局暂停标志（非 0 时冻结所有子弹动画）。 */
    if (*(i8 *)0x160f534 != 0)
    {
        if (player->effectVm != 0)
        {
            player->effectVm->prefix.flags |= 0x80000;
        }

        if (player->barrierParticle != 0)
        {
            player->barrierParticle->flags |= 0x80000;
        }

        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    if (player->effectVm != 0)
    {
        player->effectVm->prefix.flags &= ~0x80000;
    }

    if (player->barrierParticle != 0)
    {
        player->barrierParticle->flags &= ~0x80000;
    }

    player->UpdateShots();
    player->UpdateShooting();

    if ((player->playerState == 2 && player->HandleDeath() != 0) || player->playerState == 1)
    {
        player->UpdateDeathAnimation();
    }

    player->UpdateInvulnerability();

    if (player->playerState != 2 && player->playerState != 1)
    {
        player->UpdateMovement();
    }

    g_AnmManager->ExecuteScript((AnmVm *)player->unk_10);
    player->UpdateBullets();
    player->UpdateChargeShotTimer();
    player->UpdateBoundaryIndicatorTargets();

    if (g_Gui.IsMsgActive() == 0)
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
void Player::UpdateBulletVms()
{
    PlayerBulletVm *p = this->bullets;

    for (i32 i = 0; i < 0x80; i++, p++)
    {
        if (p->state == 1)
        {
            if (p->vm.prefix.type != 0)
            {
                p->vm.SetZRotation(p->rotation);
            }

            p->vm.pos.x = g_PlayerPos.x + p->offsetX;
            p->vm.pos.y = g_PlayerPos.y + p->offsetY;
            p->vm.pos.z = 0.4f;

            if (p->hasCustomColor != 0)
            {
                p->vm.prefix.color1.r = 0xff;
                p->vm.prefix.color1.g = 0x40;
                p->vm.prefix.color1.b = 0x40;
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

// FUNCTION: th08 0x449ff0 (bullet collision: circle / rotated rect / AABB against pos)
i32 Player::CheckShotCollision(Float3 *pos, void *unkD34)
{
    ShotSlot *slot;
    i32 i;
    f32 dx;
    f32 dy;
    Float3 diff;
    Float3 rotated;
    Float3 box;   // half-extent (rotated rect) or left/top (AABB)
    Float3 box2;  // right/bottom (AABB)

    slot = &this->shots[0xc0];
    for (i = 0; i < 0xc0; i++, slot++)
    {
        if (slot->active == 0)
        {
            goto nextSlot;
        }
        if (slot->unk8 != 0.0)
        {
            /* Circle test: distance to the slot center vs its radius. */
            dx = pos->x - slot->posX;
            dy = pos->y - slot->posY;
            if (dx * dx + dy * dy < slot->unk8 * slot->unk8)
            {
                goto hit;
            }
        }
        else if (slot->unk20 != 0.0f)
        {
            /* Rotated rect: rotate the offset into slot space, then AABB test. */
            diff.x = pos->x - slot->posX;
            diff.y = pos->y - slot->posY;
            Rotate(&rotated, &diff, -slot->unk20);
            box.x = slot->targetX / 2.0f;
            box.y = slot->targetY / 2.0f;
            if (-box.x <= rotated.x && rotated.x <= box.x && -box.y <= rotated.y && rotated.y <= box.y)
                goto hit;
            goto nextSlot;
        }
        else
        {
            /* Axis-aligned rect: build the box and test pos against it. */
            box.x = slot->posX - slot->targetX / 2.0f;
            box.y = slot->posY - slot->targetY / 2.0f;
            box2.x = slot->posX + slot->targetX / 2.0f;
            box2.y = slot->posY + slot->targetY / 2.0f;
            if (box.x > pos->x)
                goto nextSlot;
            if (box2.x < pos->x)
                goto nextSlot;
            if (box.y > pos->y)
                goto nextSlot;
            if (box2.y < pos->y)
                goto nextSlot;
            goto hit;
        }
    nextSlot:
        ;
    }
    return 0;
hit:
    this->lastShotHitType = slot->unk28;
    slot->unk30++;
    return 2;
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
void Player::DeactivateShotSlot()
{
    ((ShotSlot *)this)->active = 0;
}

// FUNCTION: th08 0x44c5b0
void Player::UpdateShots()
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
            ((Player *)slot)->DeactivateShotSlot();
        }
    }
}

// FUNCTION: th08 0x44c650 (70% FIXME: 射击状态机，多处寄存器/跳板布局不可修)
void Player::UpdateShooting()
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

    if (this->shotActive != 0)
    {
        /* 射击进行中：到期则清场/结束，未到期则推进当前射击并涨妖气槽。 */
        if (FUN_0040d3d0(&this->shotTimer) != 0)
        {
            g_GuiDisplayState = (g_GuiDisplayState & ~0x300) | 0x200;
        }

        if (this->shotTimer.operator>=(this->shotInterval))
        {
            FUN_00416130(&g_Spellcard);
            this->shotActive = 0;
            this->unk408 = 1.0f;
            this->unk404 = 1.0f;

            if (this->shotType == 4)
            {
                /* 切换回人类射击形态，清掉"射击形态"标志位。 */
                g_PlayerFlags &= ~PLAYER_FLAG_SHOT_MODE_MASK;
                for (i = 0; i < 8u; i++)
                {
                    if (g_BulletObjects[i] != 0)
                    {
                        ((Enemy *)g_BulletObjects[i])->FUN_0042adb0(0);
                        /* 子弹对象内：0x2dfc 状态字清零、0x3324 标志清 bit30。 */
                        *(i32 *)(g_BulletObjects[i] + 0x2dfc) = 0;
                        *(i32 *)(g_BulletObjects[i] + 0x3324) &= 0xbfffffff;
                    }
                }
                ScreenEffect::RegisterChain((ScreenEffectType)0x3, 0x1e, 0x1, 0xffffffff, 0, 0x15);
            }
        }
        else
        {
            (this->*this->shotFuncs[this->shotType])();
            this->shotTimer.Tick(0);
        }

        if (this->shotType < 4)
        {
            if (this->shotType & 1)
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
    if (g_KeyInput & TH_BUTTON_BOMB && !g_GameManager.IsTampered() &&
        !g_Gui.IsMsgActive() && this->shotIndex != 0 &&
        g_GameManager.GetBombsRemaining() > 0 && this->shotCooldown == 0)
    {
        if (((g_PlayerFlags >> PLAYER_FLAG_SHOT_MODE_SHIFT) & 3) == 0)
        {
            if ((g_PlayerFlags >> PLAYER_FLAG_EXTRA_SHIFT & 1) == 0)
            {
                goto switch_shot;
            }
        }
    }
sound_check:
    if (g_KeyInput & TH_BUTTON_BOMB && (g_KeyInput & TH_BUTTON_BOMB) != (g_KeyInput2 & TH_BUTTON_BOMB))
    {
        g_SoundPlayer.PlaySoundByIdx((SoundIdx)0x29, 0);
    }
    return;
    this->shotState = 0;
    return;

switch_shot:
    /* 切换射击：设置射击类型/消耗炸弹/初始化。 */
    /* 0x18b8a28：某全局对象指针，偏移 0xda 置 bit0（射击切换标记）。 */
    *(u16 *)(*(i32 *)0x18b8a28 + 0xda) |= 1;
    this->unk6 = 0;

    if (g_PlayerFlags >> PLAYER_FLAG_SHOT_MODE_SHIFT & 3)
    {
        this->shotType = 4;
    }
    else
    {
        ((AnmVm *)this->unk_10)->prefix.flags &= ~0x20000;
        if (this->unkE2b28 != 0)
        {
            *(u8 *)(this->unkE2b28 + 0x350) = 0;
            this->unkE2b28 = 0;
        }
        g_PlayerFlags &= ~PLAYER_FLAG_ANIM_PAUSE_MASK;
        g_AnmManager->SetMixColorDefault();

        this->shotType = this->isYoukaiMode;
        if (this->unk4 != 0)
        {
            this->shotType = 1 - this->shotType;
        }

        if (this->unk4 != 0)
        {
            this->shotType += 2;
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
            /* 0x164cfac：妖怪形态累计使用炸弹次数。 */
            *(i32 *)0x164cfac += 1;
        }
        else
        {
            /* 0x164cfa8：人类形态累计使用炸弹次数。 */
            *(i32 *)0x164cfa8 += 1;
            g_GameManager.AddToBombCount(-1);
        }
        g_GameManager.AddToBombsUsed(1);
    }

    this->unk4 = 0;
    g_GuiDisplayState = (g_GuiDisplayState & ~0xc) | 8;
    this->shotActive = 1;
    this->shotState = 1;
    this->shotTimer.SetCurrent(0);
    this->shotInterval = 0x3e7;
    (this->*this->shotFuncs[this->shotType])();
    this->shotTimer.Tick(0);
    g_GameManager.DecreaseSubrank(0xc8);
    FUN_0044cba0(&g_Spellcard);
    this->shotIndex += 6;
    if (this->shotIndex > g_PlayerShtFile->unk8)
    {
        this->shotIndex = g_PlayerShtFile->unk8;
    }
}

// FUNCTION: th08 0x44cbf0 (death: bomb out -> item drops; respawn: invulnerability
// flash -> reposition at the target point)
i32 Player::HandleDeath()
{
    i32 timeOrbPenalty;
    f32 invulnRatio;
    f32 targetX;
    f32 targetY;

    if (this->shotIndex != 0)
    {
        /* Death / bomb-out: consume a stock and spawn the drop items. */
        g_GameManager.AddTimeOrbs(-15);
        this->shotIndex--;
        this->unk4 = 1;
        if (this->shotIndex == 0)
        {
            if (this->unkE2b28 != 0)
            {
                *(u8 *)(this->unkE2b28 + 0x350) = 0;
                this->unkE2b28 = 0;
            }
            g_EffectManager.SpawnEffectAtSlot(0xc, &this->positionCenter, 3, 1, 0xff4040ff);
            g_EffectManager.SpawnEffect(0x6, &this->positionCenter, 0x10, -1);
            g_SoundPlayer.PlaySoundPositionedByIdx((SoundIdx)0xf, this->positionCenter.x);
            g_PlayerFlags &= ~PLAYER_FLAG_ANIM_PAUSE_MASK;
            g_AnmManager->SetMixColorDefault();
            ((AnmVm *)this->unk_10)->prefix.flags &= ~0x20000;
            g_ReplayManager->replayEventFlags |= 4;
            g_PlayerUnknown0b0 = 0;
            this->unk4 = 0;
            g_Spellcard.ResetSpellcard();
            g_GameManager.AddToDeaths(1);
            g_GuiDisplayState = (g_GuiDisplayState & ~0xC00) | 0x800;
            if (g_GameManager.globals->currentTimeOrbs > 0x1388)
                timeOrbPenalty = -500;
            else
                timeOrbPenalty = -g_GameManager.globals->currentTimeOrbs / 10;
            g_GameManager.AddTimeOrbs(timeOrbPenalty);
            if (g_GameManager.GetLives() > 0)
            {
                if (g_GameManager.GetPower() <= 0x10)
                    g_GameManager.SetPower(0);
                else
                    g_GameManager.AddPower(-16);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_POWER_BIG, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_POWER_SMALL, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_POWER_SMALL, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_POWER_SMALL, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_POWER_SMALL, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_POWER_SMALL, 2);
                if (g_GameManager.GetBombsRemaining() > 0)
                {
                    if (g_PlayerCharacter == 2 || g_PlayerCharacter == 8 || g_PlayerCharacter == 9)
                    {
                        g_ItemManager.SpawnItem(&this->positionCenter, ITEM_BOMB, 2);
                    }
                }
                g_GuiDisplayState = (g_GuiDisplayState & ~0x30) | 0x20;
                g_ItemManager.FUN_00441530();
            }
            else
            {
                g_GameManager.SetPower(0);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_POWER_FULL, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_POWER_FULL, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_POWER_FULL, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_POWER_FULL, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_POWER_FULL, 2);
                g_GuiDisplayState = (g_GuiDisplayState & ~0x30) | 0x20;
            }
            g_GameManager.DecreaseSubrank(0x640);
        }
        goto ret0;
    }

    /* Respawn: interpolate the invulnerability flash, then place the player at the
     * respawn point and hand back control. */
    invulnRatio = this->invulnerabilityTimer.AsFramesFloat() / 120.0f;
    ((AnmVm *)this->unk_10)->prefix.scale.y = 3.0f * invulnRatio + 1.0f;
    ((AnmVm *)this->unk_10)->prefix.scale.x = 1.0f - 1.0f * invulnRatio;
    ((AnmVm *)this->unk_10)->prefix.color1.d3dColor =
        (u32)((i32)(255.0f - this->invulnerabilityTimer.AsFramesFloat() * 255.0f / 120.0f) << 24) | 0xffffff;
    ((Player *)this->unk_10)->SetAdditiveBlend();
    this->velocityX = 0;
    this->velocityY = 0;
    if (this->invulnerabilityTimer.AsFrames() < 0x1e)
    {
        goto ret0;
    }
    this->playerState = 1;
    targetX = g_PlayerTargetX / 2.0f;
    PlayerPosCenter(&this->positionCenter)->x = targetX;
    targetY = g_PlayerTargetY - 64.0f;
    PlayerPosCenter(&this->positionCenter)->y = targetY;
    PlayerPosCenter(&this->positionCenter)->z = 0.2f;
    this->invulnerabilityTimer.SetCurrent(0);
    ((AnmVm *)this->unk_10)->prefix.scale.x = 3.0f;
    ((AnmVm *)this->unk_10)->prefix.scale.y = 3.0f;
    if (g_PlayerCharacter < 4 && this->isYoukaiMode == 0)
        goto setForm0;
    if (g_PlayerCharacter & 1)
        goto setForm5;
setForm0:
    this->anm->SetAndExecuteScriptIdx((AnmVm *)this->unk_10, 0);
    goto setFormDone;
setForm5:
    this->anm->SetAndExecuteScriptIdx((AnmVm *)this->unk_10, 5);
setFormDone:
    (void)0;
    if (g_GameManager.GetLives() <= 0)
    {
        g_PlayerUnknown0bb = 1;
        goto ret0;
    }
    g_GameManager.AddLives(-1);
    g_GuiDisplayState = (g_GuiDisplayState & ~0x3) | 0x2;
    g_GameManager.SetBombCount((i32)g_PlayerShtFile->unk4);
    g_GuiDisplayState = (g_GuiDisplayState & ~0xC) | 0x8;
    return 1;
ret0:
    return 0;
}

// FUNCTION: th08 0x44e0f0
void Player::SetAdditiveBlend()
{
    ((AnmVm *)this)->prefix.flags = (((AnmVm *)this)->prefix.flags & ~0x30) | 0x10;
}

// FUNCTION: th08 0x44e120
void Player::ClearAdditiveBlend()
{
    ((AnmVm *)this)->prefix.flags &= ~0x30;
}

// FUNCTION: th08 0x44d180
void Player::UpdateDeathAnimation()
{
    this->unkE2a70 = 0x3c;

    f32 temp = 1.0f - this->invulnerabilityTimer.AsFramesFloat() / 30.0f;
    ((AnmVm *)this->unk_10)->prefix.scale.y = 2.0f * temp + 1.0f;
    ((AnmVm *)this->unk_10)->prefix.scale.x = 1.0f - 1.0f * temp;

    ((Player *)this->unk_10)->SetAdditiveBlend();

    this->unk408 = 1.0f;
    this->unk404 = 1.0f;

    ((AnmVm *)this->unk_10)->prefix.color1.d3dColor =
        ((this->invulnerabilityTimer.AsFrames() * 0xff) / 30 << 0x18) | 0xffffff;

    this->shotIndex = 0;

    if (this->invulnerabilityTimer.AsFrames() >= 30)
    {
        this->playerState = 3;
        ((AnmVm *)this->unk_10)->prefix.scale.x = 1.0f;
        ((AnmVm *)this->unk_10)->prefix.scale.y = 1.0f;
        ((AnmVm *)this->unk_10)->prefix.color1.d3dColor = 0xffffffff;
        ((Player *)this->unk_10)->ClearAdditiveBlend();

        if (!(g_PlayerFlags >> PLAYER_FLAG_EXTRA_SHIFT & 1))
        {
            this->invulnerabilityTimer.SetCurrent(0xf0);
        }
        this->shotIndex = g_PlayerShtFile->unk8;
    }
}

// FUNCTION: th08 0x44de60 (97% FIXME: 找空槽循环的 jne/je 布局镜像不可修)
u32 Player::SpawnShot(Float3 *spawnPos, f32 targetX, f32 targetY, i32 unk28, i32 unk24)
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
    ((Player *)slot)->ResetShotSlot();
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
void Player::UpdateInvulnerability()
{
    if (this->unkE2a70 != 0)
    {
        this->unkE2a70 -= 1;
        this->SpawnShot(&this->positionCenter, 768.0f, 896.0f, -1, 0);
    }

    if (this->playerState == 3)
    {
        this->unk4 = 0;
        if (this->unkE2b1c != 0)
        {
            *(Float3 *)(this->unkE2b1c + 0x2a4) = this->positionCenter;
        }
        this->invulnerabilityTimer.operator--(0);
        if (this->invulnerabilityTimer.AsFrames() <= 0)
        {
            if (this->unkE2b1c != 0)
            {
                *(u8 *)(this->unkE2b1c + 0x350) = 0;
                this->unkE2b1c = 0;
            }
            *(u8 *)((u8 *)this) = 0;
            this->invulnerabilityTimer.SetCurrent(0);
            ((AnmVm *)this->unk_10)->prefix.color1.d3dColor = 0xffffffff;
        }
        else
        {
            if (this->invulnerabilityTimer.AsFrames() % 8 < 2)
            {
                ((AnmVm *)this->unk_10)->prefix.color1.d3dColor = 0xfff02020;
            }
            else
            {
                ((AnmVm *)this->unk_10)->prefix.color1.d3dColor = 0xffffffff;
            }
        }
    }
    else
    {
        this->invulnerabilityTimer.Tick(0);
    }
}

// Pick the movement animation (normal 1-4, youkai 6-9) based on the x-velocity
// change between this frame (speedX) and the last one (velocityX).
#define SET_MOVE_ANIM(a, b, c, d)                                                    \
    if (speedX < 0.0f && this->velocityX >= 0.0f)                                    \
        this->anm->SetAndExecuteScriptIdx((AnmVm *)this->unk_10, a);                 \
    else if (speedX == 0.0f && this->velocityX < 0.0f)                               \
        this->anm->SetAndExecuteScriptIdx((AnmVm *)this->unk_10, b);                 \
    else if (speedX > 0.0f && this->velocityX <= 0.0f)                               \
        this->anm->SetAndExecuteScriptIdx((AnmVm *)this->unk_10, c);                 \
    else if (speedX == 0.0f && this->velocityX > 0.0f)                               \
        this->anm->SetAndExecuteScriptIdx((AnmVm *)this->unk_10, d)

// FUNCTION: th08 0x44aec0 (Player main update: direction input, shot swap,
// movement with per-form speeds, shot direction vectors, option update,
// youkai-gauge logic and the afterimage trail)
i32 Player::UpdateMovement()
{
    i32 oldDirection;
    i32 swapInput;
    f32 speedX = 0.0f;
    f32 speedY = 0.0f;
    PlayerOption *opt;
    PlayerOption *opt2;
    i32 i, i2, i3, j, k;
    i32 youkaiDelta;
    f32 varA0;
    i32 gauge;
    i32 trailI;

    oldDirection = this->movementDirection;

    /* Direction input -> PlayerDirection. Diagonals take priority over singles. */
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
    else
        this->movementDirection = MOVEMENT_NONE;

    /* Shot-swap input: while a shot is active the held shot type decides, else the
     * Swap key (input bit 2). */
    swapInput = (this->shotActive != 0) ? (this->shotType & 1) : (g_KeyInput & 0x4);

    if (swapInput != 0)
    {
        if (this->isYoukaiMode != 1)
        {
            /* Begin shot-swap: re-arm the options for the new shot type. */
            if (g_PlayerCharacter <= 3)
            {
                opt = &this->options[0];
                for (i = 0; i < 4u; i++, opt++)
                {
                    memset(opt, 0, sizeof(PlayerOption));
                    *(i32 *)&opt->unk2ec = g_OptionInitCallbacks[g_PlayerCharacter][i];
                    (i32 &)opt->func = g_OptionUpdateCallbacks[g_PlayerCharacter][i];
                    if (*(i32 *)&opt->unk2ec != 0)
                    {
                        opt->unk2c8 = 1;
                        opt->timer2e0.SetCurrent(0);
                        opt->unk2d0 = i;
                    }
                    else
                    {
                        opt->unk2c8 = 0;
                    }
                }
            }
            if (g_PlayerCharacter < 4)
            {
                this->anm->SetAndExecuteScriptIdx((AnmVm *)this->unk_10, 5);
                this->velocityX = 0;
                if (this->unk8 >= 4)
                {
                    g_EffectManager.SpawnEffect(0x1d, &this->positionCenter, 1, 0x80ff8080);
                }
            }
            if (this->effectVm == 0)
            {
                this->effectVm = g_EffectManager.SpawnEffectAtSlot(0x16, &this->positionCenter, 2, 1, -1);
            }
            this->unk8 = 0;
            this->unkE2ae8.SetCurrent(0);
        }
        else
        {
            this->unk8++;
        }
        if (this->unk8 >= 7)
        {
            this->unk5 = 1;
        }
        this->isYoukaiMode = 1;
    }
    else
    {
        if (this->isYoukaiMode != 0)
        {
            /* End shot-swap: retire the now-inactive options. */
            opt2 = &this->options[0];
            if (g_PlayerCharacter < 3)
            {
                for (i2 = 0; i2 < 4u; i2++, opt2++)
                {
                    if (opt2->unk2c8 != 0 && opt2->unk2c8 != 3)
                    {
                        opt2->unk2c8 = 3;
                        opt2->timer2e0.SetCurrent(0);
                    }
                }
            }
            else if (g_PlayerCharacter == 3)
            {
                for (i3 = 0; i3 < 2; i3++, opt2++)
                {
                    if (opt2->unk2c8 != 0 && opt2->unk2c8 != 3)
                    {
                        opt2->unk2c8 = 3;
                        opt2->timer2e0.SetCurrent(0);
                    }
                }
                memset(opt2, 0, sizeof(PlayerOption));
                *(i32 *)&opt2->unk2ec = g_SpiritOptionInitCallbacks[i3];
                (i32 &)opt2->func = g_SpiritOptionUpdateCallbacks[i3];
                opt2->unk2c8 = 1;
                opt2->timer2e0.SetCurrent(0);
                opt2->unk2d0 = i3;
                for (j = 0; j < 0x10u; j++)
                {
                    this->trailPos[j] = this->positionCenter;
                }
            }
            if (g_PlayerCharacter < 4)
            {
                this->anm->SetAndExecuteScriptIdx((AnmVm *)this->unk_10, 5);
                this->velocityX = 0;
                if (this->unk8 >= 4)
                {
                    g_EffectManager.SpawnEffect(0x1c, &this->positionCenter, 1, 0x808080ff);
                }
            }
            if (this->effectVm != 0)
            {
                this->effectVm->SetInterrupt(1);
            }
            this->effectVm = 0;
            this->unk8 = 0;
            this->unkE2ae8.SetCurrent(0);
        }
        else
        {
            this->unk8++;
        }
        if (this->unk8 >= 7)
        {
            this->unk5 = 0;
        }
        this->isYoukaiMode = 0;
    }

    /* Solo characters always keep the form marker of their forced character. */
    if (g_PlayerCharacter >= 4)
    {
        if (g_PlayerCharacter & 1)
            this->unk5 = 1;
        else
            this->unk5 = 0;
    }

    /* Movement speed from the table of the currently active form. */
    if (this->isYoukaiMode != 0)
    {
        switch (this->movementDirection - 1)
        {
        case 0: speedX = this->moveSpeedSpirit->speed; break;
        case 1: speedX = -this->moveSpeedSpirit->speed; break;
        case 2: speedY = -this->moveSpeedSpirit->speed; break;
        case 3: speedY = this->moveSpeedSpirit->speed; break;
        case 4: speedX = -this->moveSpeedSpirit->diagonalSpeed; speedY = speedX; break;
        case 5: speedY = this->moveSpeedSpirit->diagonalSpeed; speedX = -speedY; break;
        case 6: speedX = this->moveSpeedSpirit->diagonalSpeed; speedY = -speedX; break;
        case 7: speedX = this->moveSpeedSpirit->diagonalSpeed; speedY = speedX; break;
        }
    }
    else
    {
        switch (this->movementDirection - 1)
        {
        case 0: speedX = this->moveSpeedNormal->speed; break;
        case 1: speedX = -this->moveSpeedNormal->speed; break;
        case 2: speedY = -this->moveSpeedNormal->speed; break;
        case 3: speedY = this->moveSpeedNormal->speed; break;
        case 4: speedX = -this->moveSpeedNormal->diagonalSpeed; speedY = speedX; break;
        case 5: speedY = this->moveSpeedNormal->diagonalSpeed; speedX = -speedY; break;
        case 6: speedX = this->moveSpeedNormal->diagonalSpeed; speedY = -speedX; break;
        case 7: speedX = this->moveSpeedNormal->diagonalSpeed; speedY = speedX; break;
        }
    }

    speedX *= this->unk404;
    speedY *= this->unk408;

    /* Movement animation: compare this frame's x velocity against the last one. */
    if (g_PlayerCharacter < 4)
    {
        if (this->isYoukaiMode != 0)
        {
            SET_MOVE_ANIM(6, 7, 8, 9);
        }
        else
        {
            SET_MOVE_ANIM(1, 2, 3, 4);
        }
    }
    else
    {
        if (g_PlayerCharacter & 1)
        {
            SET_MOVE_ANIM(6, 7, 8, 9);
        }
        else
        {
            SET_MOVE_ANIM(1, 2, 3, 4);
        }
    }

    this->velocityX = speedX;
    this->velocityY = speedY;
    this->moveSpeedX = speedX * g_ShotSpeed;
    this->moveSpeedY = speedY * g_ShotSpeed;

    PlayerPosCenter(&this->positionCenter)->x += this->moveSpeedX;
    PlayerPosCenter(&this->positionCenter)->y += this->moveSpeedY;

    /* Clamp position to the movement field. */
    if (PlayerPosCenter(&this->positionCenter)->x < g_PlayerBoundaryLeft)
        PlayerPosCenter(&this->positionCenter)->x = g_PlayerBoundaryLeft;
    else if (PlayerPosCenter(&this->positionCenter)->x > g_PlayerBoundaryLeft + g_PlayerBoundaryWidth)
        PlayerPosCenter(&this->positionCenter)->x = g_PlayerBoundaryLeft + g_PlayerBoundaryWidth;
    if (PlayerPosCenter(&this->positionCenter)->y < g_PlayerBoundaryTop)
        PlayerPosCenter(&this->positionCenter)->y = g_PlayerBoundaryTop;
    else if (PlayerPosCenter(&this->positionCenter)->y > g_PlayerBoundaryTop + g_PlayerBoundaryHeight)
        PlayerPosCenter(&this->positionCenter)->y = g_PlayerBoundaryTop + g_PlayerBoundaryHeight;

    /* Recompute the shot direction vectors (and the item grab box) for the three
     * shot speed settings. 原版 0x44bc02-0x44bd0b: Float3::operator-/+ (0x4090d0/0x409080)
     * thiscall, 隐藏返回槽; shotSpeed3d4/3e0/3ec 为 f32 标量但按 Float3 引用读取。 */
    this->shotVector1 = this->positionCenter - *(Float3 *)&this->shotSpeed3d4;
    this->shotVector2 = this->positionCenter + *(Float3 *)&this->shotSpeed3d4;
    this->shotVector3 = this->positionCenter - *(Float3 *)&this->shotSpeed3e0;
    this->shotVector4 = this->positionCenter + *(Float3 *)&this->shotSpeed3e0;
    this->grabItemTopLeft = this->positionCenter - *(Float3 *)&this->shotSpeed3ec;
    this->grabItemBottomRight = this->positionCenter + *(Float3 *)&this->shotSpeed3ec;

    /* Run the update callback of each armed option. */
    for (k = 0; k < 4u; k++)
    {
        if (*(i32 *)&this->options[k].unk2ec != 0)
        {
            (this->*this->options[k].unk2ec)();
            g_AnmManager->ExecuteScript((AnmVm *)&this->options[k]);
            this->options[k].timer2e0.Tick(0);
        }
    }

    /* Firing: grant the invulnerability border while the gauge is charging. */
    if ((g_KeyInput & 0x1) != 0 && g_Gui.IsMsgActive() == 0 && g_GameManager.IsTampered() == 0)
    {
        this->ClampChargeShotTimer();
    }

    if (g_Gui.IsMsgActive() == 0 && this->unk8 >= 0x1e && this->shotActive == 0)
    {
        youkaiDelta = 0;
        if (this->shotTimer2.operator>=(0))
        {
            if (this->unkE2ad0.operator>(0))
            {
                this->unkE2ad0.operator--(0);
            }
            else
            {
                if (this->unkE2ae8.AsFramesFloat() < 300.0f)
                    varA0 = this->unkE2ae8.AsFramesFloat() / 15.0f;
                else
                    varA0 = 21.0f;
                youkaiDelta = (i32)varA0;
                if (this->isYoukaiMode == 0)
                    youkaiDelta = -youkaiDelta;
                g_GameManager.AddToYoukaiGauge((i32)(youkaiDelta * g_ShotSpeed), 0);
                this->unkE2ae8.Tick(0);
            }
        }
        else
        {
            if (this->unkE2ad0.operator>=(4))
            {
                this->unkE2ae8.SetCurrent(0);
            }
            if (this->unkE2ad0.operator>=(0x1e))
            {
                gauge = g_GameManager.GetYoukaiGauge();
                if (fabs((double)gauge) > 9.0)
                    goto spellJudge;
                g_GameManager.SetYoukaiGauge(0);
                goto gaugeDone;
            spellJudge:
                if (g_GameManager.GaugeIsExtremelyYoukai())
                    youkaiDelta = -5;
                else if (g_GameManager.GaugeIsModeratelyYoukai())
                    youkaiDelta = -3;
                else if (g_GameManager.GetYoukaiGauge() > 0)
                    youkaiDelta = -2;
                else if (g_GameManager.GaugeIsModeratelyHuman() == 0)
                    youkaiDelta = 2;
                else if (g_GameManager.GaugeIsExtremelyHuman() == 0)
                    youkaiDelta = 3;
                else
                    youkaiDelta = 5;
                g_GameManager.AddToYoukaiGauge((i32)(youkaiDelta * g_ShotSpeed), 0);
            gaugeDone:
                (void)0;
            }
            else
            {
                this->unkE2ad0.Tick(0);
            }
        }
    }

    /* While the gauge is extreme, keep a barrier effect on the player. */
    if (g_GameManager.GaugeIsExtremelyHuman() != 0 || g_GameManager.GaugeIsExtremelyYoukai() != 0)
    {
        if (this->barrierParticle == 0)
        {
            this->barrierParticle = (EffectManagerParticle *)g_EffectManager.SpawnEffectAtSlot(0x19, &this->positionCenter, 8, 1, -1);
        }
    }
    if (this->barrierParticle != 0)
    {
        this->barrierParticle->spawnPos = this->positionCenter;
        if (g_GameManager.GaugeIsExtremelyHuman() == 0 && g_GameManager.GaugeIsExtremelyYoukai() == 0)
        {
            this->barrierParticle->alive = 0;
            this->barrierParticle = NULL;
        }
    }

    /* When not moving, shift the afterimage trail. */
    if (speedY == 0.0f)
        goto doTrail;
    if (speedX != 0.0f)
        goto done;
doTrail:
    for (trailI = 0xf; trailI > 0; trailI--)
    {
        this->trailPos[trailI] = this->trailPos[trailI - 1];
    }
    this->trailPos[0] = this->positionCenter;
done:
    return 0;
}

#undef SET_MOVE_ANIM

// FUNCTION: th08 0x451150 (68% FIXME: 寄存器分配差异)
void Player::UpdateBullets()
{
    i32 i;
    PlayerBulletVm *b;

    /* 子弹动画暂停标志（0x164d0b4 bit 10）。 */
    if (g_PlayerFlags >> PLAYER_FLAG_ANIM_PAUSE_SHIFT & 1)
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
        PlayerPosCenter((Float3 *)&b->offsetX)->x += g_ShotSpeed * b->unk43c;
        PlayerPosCenter((Float3 *)&b->offsetX)->y += g_ShotSpeed * b->unk440;

        /* 非"遗留型"子弹离开活动区域则销毁。 */
        if (b->state464 != 4 && b->state464 != 5)
        {
            if (!g_GameManager.IsWithinPlayfield(
                    PlayerPosCenter((Float3 *)&b->offsetX)->x,
                    PlayerPosCenter((Float3 *)&b->offsetX)->y,
                    b->vm.loadedSprite->heightPx,
                    b->vm.loadedSprite->widthPx))
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
i32 Player::UpdateChargeShotTimer()
{
    if (g_Unknown164d2c8 < 0x14)
    {
        return 0;
    }

    if (this->shotTimer2.AsFrames() < 0)
    {
        return 0;
    }

    if (IsSpecialShotActive(this) != 0)
    {
        return 0;
    }

    if (FUN_0040d3d0(&this->shotTimer2) != 0)
    {
        /* 0x17d6ed4：当前角色是否为"特殊射击型"的标志。 */
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

    if (g_KeyInput & TH_BUTTON_SHOOT)
    {
        if (this->shotTimer2.AsFrames() < 0)
        {
            if (g_Gui.IsMsgActive() == 0)
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
void Player::UpdateBoundaryIndicatorTargets()
{
    this->boundaryIndicatorLeft = Float3(-999.0f, -999.0f, 0.0f);
    this->boundaryIndicatorRight = Float3(-999.0f, -999.0f, 0.0f);
    this->boundaryIndicatorTimer = 0;

    if (this->positionCenter.y >= 400.0f)
    {
        if (g_AsciiManager.GetGaugeInterrupt() != 2)
        {
            if (this->positionCenter.x < 160.0f)
            {
                g_AsciiManager.SetGaugeInterrupt(2);
                goto merge;
            }
        }
        if (g_AsciiManager.GetGaugeInterrupt() == 2)
        {
            if (this->positionCenter.x > 160.0f)
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
void Player::ResetShotSlot()
{
    memset((ShotSlot *)this, 0, 0x40);
    ((ShotSlot *)this)->unk38 = 1;
}

// FUNCTION: th08 0x44d530
ChainCallbackResult Player::OnDrawHighPrio(Player *player)
{
    player->UpdateBulletVms();

    if (player->shotActive != 0)
    {
        (player->*player->unk_1014[player->shotType])();
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
        if (Player::LoadShtFile((PlayerRawShtFile **)&player->moveSpeedNormal,
                                *(const char **)(0x4c7ce0 + g_PlayerCharacter * 4)) != 0)
        {
            return (ZunResult)-1;
        }

        if (Player::LoadShtFile((PlayerRawShtFile **)&player->moveSpeedSpirit,
                                *(const char **)(0x4c7d10 + g_PlayerCharacter * 4)) != 0)
        {
            return (ZunResult)-1;
        }

        player->anm =
            g_AnmManager->PreloadAnm(5, *(const char **)(0x4c7cb0 + g_PlayerCharacter * 4));

        if (player->anm == NULL)
        {
            return (ZunResult)-1;
        }
    }
    else
    {
        player->anm = g_AnmManager->GetAnm(5);
    }

    if (!(g_PlayerCharacter >= 4 && (g_PlayerCharacter & 1)))
    {
        (player->anm)->SetAndExecuteScriptIdx((AnmVm *)player->unk_10, 0);
    }
    else
    {
        (player->anm)->SetAndExecuteScriptIdx((AnmVm *)player->unk_10, 5);
    }

    /* 原版对 `positionCenter = Float3(...)` 编译成三次构造器调用，见 PlayerPosCenter。 */
    PlayerPosCenter(&player->positionCenter)->x = g_PlayerTargetX / *(f32 *)MEM_FLOAT_2_0;
    PlayerPosCenter(&player->positionCenter)->y = g_PlayerTargetY - *(f32 *)MEM_FLOAT_64_0;
    PlayerPosCenter(&player->positionCenter)->z = 0.48f;

    for (i = 0; i < 0x180u; i++)
    {
        ((Player *)&player->shots[i])->ResetShotSlot();
    }

    /* .sht 文件 0xc/0x10/0x18 处为三档射击速度，除以 2.0f 得帧速度。 */
    player->shotSpeed3d8 = *(f32 *)((u8 *)g_PlayerShtFile + 0xc) / *(f32 *)MEM_FLOAT_2_0;
    player->shotSpeed3d4 = player->shotSpeed3d8;
    player->unk3dc = 5.0f;
    player->shotSpeed3e4 = *(f32 *)((u8 *)g_PlayerShtFile + 0x10) / *(f32 *)MEM_FLOAT_2_0;
    player->shotSpeed3e0 = player->shotSpeed3e4;
    player->unk3e8 = 5.0f;
    player->shotSpeed3f0 = *(f32 *)((u8 *)g_PlayerShtFile + 0x18) / *(f32 *)MEM_FLOAT_2_0;
    player->shotSpeed3ec = player->shotSpeed3f0;
    player->unk3f4 = 5.0f;

    player->movementDirection = 0;

    player->playerState = 1;

    player->invulnerabilityTimer.SetCurrent(g_GameManager.IsSpellPractice() ? 0xa : 0x78);

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
    memcpy((u8 *)player->shotFuncs, (void *)(0x4c7ad0 + (g_PlayerCharacter * 2) * 0x14), 0x14);
    memcpy((u8 *)player->unk_1014, (void *)(0x4c7ad0 + (g_PlayerCharacter * 2 + 1) * 0x14), 0x14);

    player->shotActive = 0;
    player->unkE2b0c = -1.57f;
    player->unk408 = 1.0f;
    player->unk404 = 1.0f;
    player->shotIndex = g_PlayerShtFile->unk8;

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

    player->barrierParticle = NULL;

    for (i = 0; i < 0x10u; i++)
    {
        player->trailPos[i] = player->positionCenter;
    }

    player->isYoukaiMode = 2;

    if (g_PlayerCharacter > 3)
    {
        PlayerOption *opt = &player->options[0];
        for (u32 k = 0; k < 4; k++, opt++)
        {
            memset(opt, 0, 0x2f4);
            *(i32 *)&opt->unk2ec = g_OptionInitCallbacks[g_PlayerCharacter][k];
            (i32 &)opt->func = g_OptionUpdateCallbacks[g_PlayerCharacter][k];
            if (*(i32 *)&opt->unk2ec != 0)
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
