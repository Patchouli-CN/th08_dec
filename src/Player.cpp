#include "th_pch.h"

#include "Player.hpp"
#include "AsciiManager.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
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

// FUNCTION: th08 0x44c390 (98.48% FIXME: || 短路跳板布局)
ChainCallbackResult Player::OnUpdate(Player *player)
{
    if (*(i8 *)0x160f534 != 0)
    {
        if (*(u32 *)((u8 *)player + 0xbe834) != 0)
        {
            *(u32 *)(*(u32 *)((u8 *)player + 0xbe834) + 0x1f8) |= 0x80000;
        }

        if (*(u32 *)((u8 *)player + 0xe2b24) != 0)
        {
            *(u32 *)(*(u32 *)((u8 *)player + 0xe2b24) + 0x1f8) |= 0x80000;
        }

        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    if (*(u32 *)((u8 *)player + 0xbe834) != 0)
    {
        *(u32 *)(*(u32 *)((u8 *)player + 0xbe834) + 0x1f8) &= 0xfff7ffff;
    }

    if (*(u32 *)((u8 *)player + 0xe2b24) != 0)
    {
        *(u32 *)(*(u32 *)((u8 *)player + 0xe2b24) + 0x1f8) &= 0xfff7ffff;
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
        *(u32 *)0x164d318 += 1;
        *(u32 *)0x164d31c += 1;

        if (g_GameManager.GaugeIsExtremelyHuman())
        {
            *(u32 *)0x164d324 += 1;
            *(u32 *)0x164d32c += 1;
            g_GameManager.AddScore(0x64);
        }
        else if (g_GameManager.GaugeIsExtremelyYoukai())
        {
            *(u32 *)0x164d320 += 1;
            *(u32 *)0x164d328 += 1;
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

// STUB: th08 0x40bc20
i32 Player::FUN_0040bc20()
{
    return 0;
}

// STUB: th08 0x40bc40
i32 Player::FUN_0040bc40()
{
    return 0;
}

// STUB: th08 0x44c5b0
void Player::FUN_0044c5b0()
{
}

// STUB: th08 0x44c650
void Player::FUN_0044c650()
{
}

// STUB: th08 0x44cbf0
i32 Player::FUN_0044cbf0()
{
    return 0;
}

// STUB: th08 0x44d180
void Player::FUN_0044d180()
{
}

// STUB: th08 0x44d2c0
void Player::FUN_0044d2c0()
{
}

// STUB: th08 0x44aec0
void Player::FUN_0044aec0()
{
}

// STUB: th08 0x451150
void Player::FUN_00451150()
{
}

// STUB: th08 0x451500
void Player::FUN_00451500()
{
}

// STUB: th08 0x44d420
void Player::FUN_0044d420()
{
}

// STUB: th08 0x44e370
void Player::FUN_0044e370()
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

// FUNCTION: th08 0x44d650 (32.7% FIXME: Float3 字段赋值已修，字段/分支待迭代)
ZunResult Player::AddedCallback(Player *player)
{
    i32 i;

    if (g_Supervisor.GetUnk164())
    {
        if (Player::LoadShtFile((PlayerRawShtFile **)&player->unkE2a74,
                                *(const char **)(0x4c7ce0 + *(u8 *)0x164d0b1 * 4)) != 0)
        {
            return (ZunResult)-1;
        }

        if (Player::LoadShtFile((PlayerRawShtFile **)&player->unkE2a78,
                                *(const char **)(0x4c7d10 + *(u8 *)0x164d0b1 * 4)) != 0)
        {
            return (ZunResult)-1;
        }

        *(AnmLoaded **)((u8 *)player + 0xc) =
            g_AnmManager->PreloadAnm(5, *(const char **)(0x4c7cb0 + *(u8 *)0x164d0b1 * 4));

        if (*(AnmLoaded **)((u8 *)player + 0xc) == NULL)
        {
            return (ZunResult)-1;
        }
    }
    else
    {
        *(AnmLoaded **)((u8 *)player + 0xc) = g_AnmManager->GetAnm(5);
    }

    if (*(u8 *)0x164d0b1 >= 4 && (*(u8 *)0x164d0b1 & 1))
    {
        (*(AnmLoaded **)((u8 *)player + 0xc))->SetAndExecuteScriptIdx((AnmVm *)((u8 *)player + 0x10), 5);
    }
    else
    {
        (*(AnmLoaded **)((u8 *)player + 0xc))->SetAndExecuteScriptIdx((AnmVm *)((u8 *)player + 0x10), 0);
    }

    {
        Float3 f0 = Float3(*(f32 *)0x164d2e4 / *(f32 *)0x4b42ec, *(f32 *)0x164d2e8 - *(f32 *)0x4b42c8, 0.48f);
        Float3 f1 = Float3(*(f32 *)0x164d2e4 / *(f32 *)0x4b42ec, *(f32 *)0x164d2e8 - *(f32 *)0x4b42c8, 0.48f);
        Float3 *dest0 = (Float3 *)((u8 *)player + 0x2c0);
        Float3 *dest1 = (Float3 *)((u8 *)player + 0x2e0);

        dest0->x = f0.x;
        dest0->y = f0.y;
        dest0->z = f0.z;
        dest1->x = f1.x;
        dest1->y = f1.y;
        dest1->z = f1.z;
    }

    for (i = 0; i < 0x180; i++)
    {
        player->FUN_0044e370();
    }

    *(f32 *)((u8 *)player + 0x3d8) = *(f32 *)((u8 *)g_PlayerShtFile + 0x3d8);
    *(f32 *)((u8 *)player + 0x3d4) = *(f32 *)((u8 *)player + 0x3d8);
    *(f32 *)((u8 *)player + 0x3dc) = 5.0f;
    *(f32 *)((u8 *)player + 0x3e4) = *(f32 *)((u8 *)g_PlayerShtFile + 0x3e4);
    *(f32 *)((u8 *)player + 0x3e0) = *(f32 *)((u8 *)player + 0x3e4);
    *(f32 *)((u8 *)player + 0x3e8) = 5.0f;
    *(f32 *)((u8 *)player + 0x3f0) = *(f32 *)((u8 *)g_PlayerShtFile + 0x3f0);
    *(f32 *)((u8 *)player + 0x3ec) = *(f32 *)((u8 *)player + 0x3f0);
    *(f32 *)((u8 *)player + 0x3f4) = 5.0f;

    *(i32 *)((u8 *)player + 0xe2a98) = 0;

    if (g_GameManager.GetFlag14())
    {
        player->FUN_0044d420();   // ? 需要精确
    }

    for (i = 0; i < 4; i++)
    {
        ((ZunTimer *)((u8 *)player + 0x410 + i * 0x40))->SetCurrent(0);
    }

    *(u8 *)((u8 *)player + 0x2) = 1;

    for (i = 0; i < 0x80; i++)
    {
        *(u16 *)((u8 *)player + 0x462 + i * 0x484) = 0;
    }

    for (i = 0; i < 0x20; i++)
    {
        ((ZunTimer *)((u8 *)player + 0x228 + i * 0x10))->SetCurrent(0);
    }

    /* 表数据拷贝 */
    for (i = 0; i < 0x1e; i++)
    {
        *(u32 *)((u8 *)player + 0x3d4 + i * 0x20) = *(u32 *)(0x18b896c + *(u8 *)0x164d0b1 * 4 + i * 0x20);
    }

    *(i32 *)((u8 *)player + 0xfdc) = 0;
    *(f32 *)((u8 *)player + 0xe2b0c) = -1.57f;
    *(f32 *)((u8 *)player + 0x408) = 1.0f;
    *(f32 *)((u8 *)player + 0x404) = 1.0f;
    *(i32 *)((u8 *)player + 0xe2a68) = *(i32 *)((u8 *)g_PlayerShtFile + 0x8);

    if (g_Supervisor.GetUnk164())
    {
        g_AsciiManager.SetGaugeInterrupt(1);
    }

    g_AsciiManager.SetBossMarkerInterrupt(0, 2);
    g_AsciiManager.SetBossMarkerInterrupt(1, 2);
    g_AsciiManager.SetBossMarkerInterrupt(2, 2);

    if (*(u8 *)0x164d0b1 != 3 && g_GameManager.IsSoloHuman() != 0 && g_GameManager.IsSoloYoukai() == 0)
    {
        *(u16 *)0x164d300 = 0xf830;
        *(u16 *)0x164d304 = 0xe0c0;
        *(u16 *)0x164d308 = 0xf82f;
    }

    *(i32 *)((u8 *)player + 0xe2b24) = 0;

    for (i = 0; i < 0x10; i++)
    {
        Float3 *dest = (Float3 *)((u8 *)player + 0x3d4 + i * 0xc);

        dest->x = 0.0f;
        dest->y = 0.0f;
        dest->z = 0.0f;
    }

    *(u8 *)((u8 *)player + 0x3) = 2;

    for (i = 0; i < 4; i++)
    {
        memset((void *)((u8 *)player + 0x3d4 + i * 0x100), 0, 0x100);
    }

    if (*(u8 *)0x164d0b1 < 4)
    {
        *(i32 *)((u8 *)player + 0x2ec) = *(i32 *)(0x4c7d40 + *(u8 *)0x164d0b1 * 4);
        *(i32 *)((u8 *)player + 0x2f0) = *(i32 *)(0x4c7e10 + *(u8 *)0x164d0b1 * 4);
    }

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

} /* namespace th08 */
