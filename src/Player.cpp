#include "th_pch.h"

#include "Player.hpp"
#include "AsciiManager.hpp"
#include "GameManager.hpp"
#include "Supervisor.hpp"

namespace th08
{

DIFFABLE_STATIC(Player, g_Player);
DIFFABLE_STATIC(PlayerRawShtFile *, g_PlayerShtFile);
DIFFABLE_STATIC(PlayerRawShtFile *, g_PlayerShtFile2);
DIFFABLE_STATIC(ChainElem *, g_PlayerCalcChain);
DIFFABLE_STATIC(ChainElem *, g_PlayerDrawChainHighPrio);
DIFFABLE_STATIC(ChainElem *, g_PlayerDrawChainLowPrio);

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
    memset(player, 0, offsetof(Player, unk0xe2ab0));
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

// STUB: th08 0x44c390
ChainCallbackResult Player::OnUpdate(Player *player)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// STUB: th08 0x44d530
ChainCallbackResult Player::OnDrawHighPrio(Player *player)
{
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

// STUB: th08 0x44d650
ZunResult Player::AddedCallback(Player *player)
{
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

// STUB: th08 0x44dd70
ZunResult Player::LoadShtFile(PlayerRawShtFile **header, const char *path)
{
    return ZUN_SUCCESS;
}

} /* namespace th08 */
