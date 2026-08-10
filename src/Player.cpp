#include "th_pch.h"

#include "Player.hpp"
#include "AsciiManager.hpp"
#include "GameManager.hpp"
#include "Supervisor.hpp"

namespace th08
{

DIFFABLE_STATIC(Player, g_Player);
DIFFABLE_STATIC(u8, g_PlayerUnknown0bb);
DIFFABLE_STATIC(Float2, g_PlayerPos);
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
// STUB: th08 0x4512f0
void Player::FUN_004512f0()
{
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
ZunResult Player::LoadShtFile(PlayerRawShtFile **header, const char *path)
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

} /* namespace th08 */
