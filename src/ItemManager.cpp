#include "th_pch.h"

#include "AsciiManager.hpp"
#include "BulletManager.hpp"
#include "EffectManager.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "ItemManager.hpp"
#include "Player.hpp"
#include "ReplayManager.hpp"
#include "SoundPlayer.hpp"

namespace th08
{

DIFFABLE_STATIC(ItemManager, g_ItemManager);
DIFFABLE_STATIC(AnmLoaded *, g_EffectAnmLoaded);
DIFFABLE_STATIC(i32, g_UnkTimeOrbValueFlag);
DIFFABLE_STATIC(ZunTimer, g_ItemAutoCollectTimer);
DIFFABLE_STATIC(ZunTimer, g_UnkAutoCollectTimer);

#pragma var_order(i, item)
Item *ItemManager::SpawnItem(Float3 *position, ItemType itemType, i32 state)
{
    i32 i;
    Item *item = &this->items[this->nextIndex];

    if (position->x < -64.0f || position->x > 448.0f)
    {
        return &this->items[MAX_ITEMS];
    }

    if (g_GameManager.GetPower() >= 128 && (itemType == ITEM_POWER_SMALL || itemType == ITEM_POWER_BIG))
    {
        itemType = ITEM_POINT_SMALL;
    }
    if (itemType == ITEM_TIME)
    {
        state = ITEM_STATE_UNK3;
    }
    else if (itemType == ITEM_TIME2)
    {
        state = ITEM_STATE_UNK5;
        itemType = ITEM_TIME;
    }

    for (i = 0; i < MAX_ITEMS; i++)
    {
        this->nextIndex++;

        if (item->isInUse)
        {
            if (this->nextIndex >= MAX_ITEMS)
            {
                this->nextIndex = 0;
                item = &this->items[0];
            }
            else
            {
                item++;
            }

            if (itemType == ITEM_TIME)
            {
                return &this->items[MAX_ITEMS];
            }

            continue;
        }

        if (this->nextIndex >= MAX_ITEMS)
        {
            this->nextIndex = 0;
        }

        item->isInUse = true;
        item->currentPosition = *position;
        item->startPositionOrVelocity.x = 0.0f;
        item->startPositionOrVelocity.y = -2.2f;
        item->startPositionOrVelocity.z = 0.0f;
        item->itemType = itemType;
        item->state = state;
        item->timer = 0;

        if (state == ITEM_STATE_UNK2)
        {
            item->targetPosition.x = g_Rng.GetRandomF32InRange(288.0f) + 48.0f;
            item->targetPosition.y = g_Rng.GetRandomF32InRange(192.0f) - 64.0f;
            item->targetPosition.z = 0.0f;
            item->startPositionOrVelocity = item->currentPosition;
        }
        else if (state == ITEM_STATE_UNK3)
        {
            item->startPositionOrVelocity.y = -2.0f - g_Rng.GetRandomF32InRange(0.2f);
            item->startPositionOrVelocity.x = g_Rng.GetRandomF32SignedInRange(0.6f);

            if (g_Player.playerState == PLAYER_STATE_DEAD)
            {
                item->state = ITEM_STATE_DEFAULT;
                item->startPositionOrVelocity.x = 0.0f;
                item->startPositionOrVelocity.y = -0.9f;
                item->startPositionOrVelocity.z = 0.0f;
            }
        }
        // ZUN bloat: This is just a duplicate of the above state!
        else if (state == ITEM_STATE_UNK5)
        {
            item->startPositionOrVelocity.y = -2.0f - g_Rng.GetRandomF32InRange(0.2f);
            item->startPositionOrVelocity.x = g_Rng.GetRandomF32SignedInRange(0.6f);

            if (g_Player.playerState == PLAYER_STATE_DEAD)
            {
                item->state = ITEM_STATE_DEFAULT;
                item->startPositionOrVelocity.x = 0.0f;
                item->startPositionOrVelocity.y = -0.9f;
                item->startPositionOrVelocity.z = 0.0f;
            }
        }

        g_BulletAnm->SetAndExecuteScriptIdx(&item->sprite, itemType + 61);

        item->sprite.prefix.color1.d3dColor = 0xFFFFFFFF;
        item->sprite.prefix.zWriteDisabled = true;
        item->isMaxValue = false;
        item->isOnscreen = true;
        this->itemListTail->next = item;
        item->prev = this->itemListTail;
        item->next = NULL;
        this->itemListTail = item;

        return i < MAX_ITEMS ? item : &this->items[MAX_ITEMS];
    }
}

DIFFABLE_STATIC_ARRAY_ASSIGN(i32, 6, g_PowerUpThresholds) = {8, 24, 48, 80, 128, 999};
DIFFABLE_STATIC_ARRAY_ASSIGN(i32, 6, g_PointItemExtendThresholds) = {100, 250, 500, 800, 1100, 9999};
DIFFABLE_STATIC_ARRAY_ASSIGN(i32, 4, g_ExPointItemExtendThresholds) = {200, 666, 9999, 1};

void ItemManager::UpdatePointItemExtendThreshold()
{
    if (g_GameManager.difficulty < 4)
    {
        if (g_GameManager.globals->pointItemExtendsSoFar < 6)
        {
            g_GameManager.globals->nextPointItemExtendThreshold =
                g_PointItemExtendThresholds[g_GameManager.globals->pointItemExtendsSoFar];
        }
        else
        {
            g_GameManager.globals->nextPointItemExtendThreshold =
                (g_GameManager.globals->pointItemExtendsSoFar - 5) * 500 + g_PointItemExtendThresholds[5];
        }
    }
    else
    {
        if (g_GameManager.globals->pointItemExtendsSoFar < 3)
        {
            g_GameManager.globals->nextPointItemExtendThreshold =
                g_ExPointItemExtendThresholds[g_GameManager.globals->pointItemExtendsSoFar];
        }
        else
        {
            g_GameManager.globals->nextPointItemExtendThreshold = 99999;
        }
    }
}

#pragma var_order(collectSpeed, itemTimerSecs, itemScore, playerAngle, local_1c, itemSound, item)
void ItemManager::OnUpdate()
{
    f32 collectSpeed;
    f32 itemTimerSecs;
    i32 itemScore;
    f32 playerAngle;
    i32 itemSound;
    Item *item;

    itemSound = 0;
    item = this->itemListHead.next;
    Float3 local_1c(g_PlayerShtFile->itemCollectRadius, g_PlayerShtFile->itemCollectRadius, 16.0f);
    this->itemCount = 0;
    collectSpeed = g_Player.isYoukaiMode ? g_PlayerShtFile2->unk34 : g_PlayerShtFile->unk34;
    collectSpeed *= g_Supervisor.framerateMultiplier;

    while (item != NULL)
    {
        this->itemCount++;
        if (item->state == ITEM_STATE_UNK2)
        {
            if (item->timer < 0x3c)
            {
                itemTimerSecs = (f32)item->timer / 60.0f;
                item->currentPosition =
                    item->targetPosition * itemTimerSecs + item->startPositionOrVelocity * (1.0f - itemTimerSecs);
                goto checkCollision;
            }
            else if (item->timer == 0x3c)
            {
                item->startPositionOrVelocity = Float3(0.0f, 0.0f, 0.0f);
                item->state = ITEM_STATE_DEFAULT;
            }
        }
        else if (item->state == ITEM_STATE_UNK3)
        {
            item->startPositionOrVelocity.y += 0.05f * g_Supervisor.framerateMultiplier;
            if (item->startPositionOrVelocity.y > 0.0f || g_UnkAutoCollectTimer < 0)
            {
                item->state = ITEM_STATE_AUTOCOLLECT;
            }
            if (g_Player.playerState == PLAYER_STATE_DEAD)
            {
                item->state = ITEM_STATE_DEFAULT;
                item->startPositionOrVelocity.x = 0.0f;
                item->startPositionOrVelocity.y = -0.7f;
                item->startPositionOrVelocity.z = 0.0f;
            }
        }
        else if (item->state == ITEM_STATE_UNK5)
        {
            item->startPositionOrVelocity.y += 0.05f * g_Supervisor.framerateMultiplier;
            item->currentPosition += item->startPositionOrVelocity * collectSpeed;
            if (item->startPositionOrVelocity.y > 0.0f)
            {
                item->state = ITEM_STATE_AUTOCOLLECT;
            }
            else
            {
                goto advanceItem;
            }
            if (g_Player.playerState == PLAYER_STATE_DEAD)
            {
                item->state = ITEM_STATE_DEFAULT;
                item->startPositionOrVelocity.x = 0.0f;
                item->startPositionOrVelocity.y = -0.7f;
                item->startPositionOrVelocity.z = 0.0f;
            }
        }
        else
        {
            if (item->state == ITEM_STATE_AUTOCOLLECT ||
                (g_Player.positionCenter.y < g_PlayerShtFile->pocY &&
                 (g_GameManager.GetPower() >= 128.0 || g_Player.isYoukaiMode || g_GameManager.shotType == 1 ||
                  g_GameManager.shotType == 6)))
            {
                if (g_Player.playerState != PLAYER_STATE_DEAD && g_Player.playerState != PLAYER_STATE_SPAWNING)
                {
                    playerAngle = g_Player.AngleToPlayer(&item->currentPosition);
                    item->startPositionOrVelocity.FromAngleMagnitude(playerAngle, g_PlayerShtFile->itemCollectSpeed);
                    item->state = ITEM_STATE_AUTOCOLLECT;
                    item->currentPosition += item->startPositionOrVelocity * g_Supervisor.framerateMultiplier;
                    goto checkCollision;
                }
                else
                {
                    item->startPositionOrVelocity.y = -0.7f;
                    item->state = ITEM_STATE_DEFAULT;
                }
            }
            else
            {
                item->startPositionOrVelocity.x = 0.0f;
                item->startPositionOrVelocity.z = 0.0f;
                if (item->startPositionOrVelocity.y < -2.2f)
                {
                    item->startPositionOrVelocity.y = -2.2f;
                }
            }
        }

        item->currentPosition += item->startPositionOrVelocity * collectSpeed;
        if (item->state == ITEM_STATE_DEFAULT && g_GameManager.arcadeRegionSize.y + 16.0f <= item->currentPosition.y)
        {
            g_GameManager.DecreaseSubrank(3);
            item->Delete();
            item = item->next;
            continue;
        }
        if (item->startPositionOrVelocity.Float3::Float3().y < 3.0f)
        {
            item->startPositionOrVelocity.y += 0.03f * collectSpeed;
        }
        else
        {
            item->startPositionOrVelocity.y = 3.0f;
        }

    checkCollision:
        if (item->state != ITEM_STATE_UNK3 && g_Player.CalcItemBoxCollision(&item->currentPosition, &local_1c))
        {
            g_ReplayManager->replayEventFlags |= 0x40;
            switch (item->itemType)
            {
            case ITEM_POWER_SMALL:
                item->CollectPowerSmall();
                break;
            case ITEM_POINT:
                item->CollectPoint();
                break;
            case ITEM_POINT_SMALL:
                item->CollectPointSmall();
                break;
            case ITEM_POWER_BIG:
                item->CollectPowerBig();
                break;
            case ITEM_BOMB:
                if (g_GameManager.GetBombsRemaining() < 8)
                {
                    g_GameManager.AddToBombCount(1);
                    g_Gui.flags.bombDisplayUpdateFrames = 2;
                }
                g_GameManager.IncreaseSubrank(5);
                break;
            case ITEM_EXTEND:
                g_GameManager.CollectExtend();
                break;
            case ITEM_POWER_FULL:
                if (g_GameManager.GetPower() < 0x80)
                {
                    g_BulletManager.bulletmanager_fun_00415c60();
                    g_Gui.ShowPopupB(0, 1);
                    g_SoundPlayer.PlaySoundByIdx(SOUND_POWERUP, 0);
                    g_AsciiManager.CreateScorePopup(&item->currentPosition, -1, COLOR_SCORE_POPUP);
                    this->ConvertAllPowerItemsToTimeOrbs(item);
                }
                g_GameManager.SetPower(0x80);
                g_GameManager.AddScore(0x3e8);
                g_AsciiManager.CreateScorePopup(&item->currentPosition, 0x3e8, -1);
                g_Gui.flags.powerDisplayUpdateFrames = 2;
                break;
            case ITEM_POINT_STAR:
                if (g_UnkTimeOrbValueFlag == 0)
                {
                    itemScore = g_GameManager.globals->graze / 0x28 * 10 + 0x12c;
                    if (itemScore <= 0)
                    {
                        itemScore = 0xa;
                    }
                }
                else
                {
                    itemScore = 0x64;
                }
                g_AsciiManager.CreatePlayerPointPopup(&item->currentPosition, itemScore, -1);
                g_GameManager.AddScore(itemScore);
                break;
            case ITEM_TIME:
                item->CollectTimeOrb();
                break;
            }
            if (itemSound <= 0x15)
            {
                itemSound = item->isMaxValue ? 0x2c : 0x15;
            }
            item->Delete();
            item = item->next;
            continue;
        }

    advanceItem:
        item->timer++;
        if (item->sprite.currentInstruction != NULL)
        {
            g_AnmManager->ExecuteScript(&item->sprite);
        }
        item = item->next;
    }

    if (itemSound != 0)
    {
        g_SoundPlayer.PlaySoundByIdx((SoundIdx)itemSound, 0);
    }
    if (g_ItemAutoCollectTimer != 0)
    {
        g_ItemAutoCollectTimer--;
        if (g_ItemAutoCollectTimer <= 0)
        {
            g_ItemAutoCollectTimer = 0;
        }
    }
}

#pragma var_order(i, powerLevel)
void Item::CollectPowerSmall()
{
    i32 i;
    i32 powerLevel;

    if (g_GameManager.GetPower() >= 0x80)
    {
        goto increaseSubrank;
    }
    i = 0;
    while (g_GameManager.GetPower() >= g_PowerUpThresholds[i])
    {
        i++;
    }
    powerLevel = i;
    g_GameManager.character = 0;
    g_GameManager.AddPower(1);
    if (g_GameManager.GetPower() >= 0x80)
    {
        g_GameManager.SetPower(0x80);
        if (g_Spellcard.IsSpellcardActive() == 0)
        {
            g_BulletManager.bulletmanager_fun_00415c60();
        }
        g_Gui.ShowPopupB(0, 1);
        g_ItemManager.ConvertAllPowerItemsToTimeOrbs(this);
    }
    g_GameManager.AddScore(0xa);
    g_Gui.flags.powerDisplayUpdateFrames = 2;
    while (g_GameManager.GetPower() >= g_PowerUpThresholds[i])
    {
        i++;
    }
    if (i != powerLevel)
    {
        g_AsciiManager.CreateScorePopup(&this->currentPosition, -1, COLOR_SCORE_POPUP);
        g_SoundPlayer.PlaySoundByIdx(SOUND_POWERUP, 0);
    }
    else
    {
        g_AsciiManager.CreateScorePopup(&this->currentPosition, 0xa, -1);
    }
increaseSubrank:
    g_GameManager.IncreaseSubrank(1);
}

#pragma var_order(pointItemValue, itemScore)
void Item::CollectPoint()
{
    i32 pointItemValue;
    i32 itemScore;

    pointItemValue = g_GameManager.globals->pointItemValue;
    itemScore = (ZunBool)(this->currentPosition.y < g_PlayerShtFile->pocY)
                    ? pointItemValue
                    : pointItemValue / 2 -
                          (i32)(this->currentPosition.y - g_PlayerShtFile->pocY) *
                              (g_GameManager.globals->pointItemValue / 1000);
    if (this->isMaxValue == 1)
    {
        itemScore = pointItemValue;
    }
    itemScore -= itemScore % 10;
    if (g_GameManager.GaugeIsExtremelyHuman())
    {
        itemScore += itemScore;
    }
    g_AsciiManager.CreateScorePopup(&this->currentPosition, itemScore,
                                    itemScore >= pointItemValue ? COLOR_YELLOW : COLOR_WHITE);
    if (itemScore >= pointItemValue)
    {
        this->isMaxValue = true;
    }
    g_GameManager.AddScore(itemScore);
    g_GameManager.globals->pointItemsCollectedInStage++;
    g_GameManager.globals->pointItemsCollected++;
    g_Gui.flags.pointDisplayUpdateFrames = 2;
    if (itemScore >= pointItemValue)
    {
        g_GameManager.IncreaseSubrank(10);
    }
    else
    {
        g_GameManager.IncreaseSubrank(3);
    }
    if ((i32)g_GameManager.globals->pointItemExtendsSoFar >= 0)
    {
        while (ItemManager::UpdatePointItemExtendThreshold(),
               g_GameManager.globals->pointItemsCollected >= g_GameManager.globals->nextPointItemExtendThreshold)
        {
            g_GameManager.CollectExtend();
            g_GameManager.globals->pointItemExtendsSoFar++;
        }
    }
    g_GameManager.hscr.numPointItemsCollected++;
    g_GameManager.UpdateAntiTamper();
}

#pragma var_order(pointItemValue, itemScore)
void Item::CollectPointSmall()
{
    i32 pointItemValue;
    i32 itemScore;

    pointItemValue = g_GameManager.globals->pointItemValue;
    itemScore = (ZunBool)(this->currentPosition.y < g_PlayerShtFile->pocY)
                    ? pointItemValue
                    : pointItemValue / 2 -
                          (i32)(this->currentPosition.y - g_PlayerShtFile->pocY) *
                              (g_GameManager.globals->pointItemValue / 1000);
    if (this->isMaxValue == 1)
    {
        itemScore = pointItemValue;
    }
    pointItemValue /= 10;
    pointItemValue -= pointItemValue % 10;
    itemScore /= 10;
    itemScore -= itemScore % 10;
    if (g_GameManager.GaugeIsExtremelyHuman())
    {
        itemScore += itemScore;
    }
    g_AsciiManager.CreateScorePopup(&this->currentPosition, itemScore,
                                    itemScore >= pointItemValue ? COLOR_YELLOW : COLOR_WHITE);
    g_GameManager.AddScore(itemScore);
    if (itemScore >= pointItemValue)
    {
        this->isMaxValue = true;
    }
}

#pragma var_order(i, powerLevel)
void Item::CollectPowerBig()
{
    i32 i;
    i32 powerLevel;

    if (g_GameManager.GetPower() >= 0x80)
    {
        goto end;
    }
    i = 0;
    while (g_GameManager.GetPower() >= g_PowerUpThresholds[i])
    {
        i++;
    }
    powerLevel = i;
    g_GameManager.AddPower(8);
    if (g_GameManager.GetPower() >= 0x80)
    {
        g_GameManager.SetPower(0x80);
        if (g_Spellcard.IsSpellcardActive() == 0)
        {
            g_BulletManager.bulletmanager_fun_00415c60();
        }
        g_Gui.ShowPopupB(0, 1);
        g_ItemManager.ConvertAllPowerItemsToTimeOrbs(this);
    }
    g_Gui.flags.powerDisplayUpdateFrames = 2;
    g_GameManager.AddScore(0xa);
    while (g_GameManager.GetPower() >= g_PowerUpThresholds[i])
    {
        i++;
    }
    if (i != powerLevel)
    {
        g_AsciiManager.CreateScorePopup(&this->currentPosition, -1, COLOR_SCORE_POPUP);
        g_SoundPlayer.PlaySoundByIdx(SOUND_POWERUP, 0);
    }
    else
    {
        g_AsciiManager.CreateScorePopup(&this->currentPosition, 0xa, -1);
    }
end:
    ;
}

void Item::CollectTimeOrb()
{
    i32 itemScore;

    if (g_UnkTimeOrbValueFlag == 0)
    {
        if (g_GameManager.globals->pointItemsCollectedInStage >= 0x7d0)
        {
            itemScore = 0x2710;
        }
        else
        {
            itemScore = (g_GameManager.globals->pointItemsCollected / 2) * 10;
            if (itemScore < 0x64)
            {
                itemScore = 0x64;
            }
        }
    }
    else
    {
        itemScore = 0x64;
    }
    if (this != NULL)
    {
        g_AsciiManager.CreatePlayerPointPopup(&this->currentPosition, itemScore,
                                              g_GameManager.GetTimeOrbs() < g_GameManager.GetLastSpellTimeOrbThreshold()
                                                  ? 0xdfffffff
                                                  : 0xdfffef80);
    }
    g_Gui.flags.timeDisplayUpdateFrames = 2;
    g_GameManager.AddScore(itemScore);
    g_GameManager.AddTimeOrbs(1);
    g_Spellcard.AddSpellcardTime(0x1f40);
    if (g_ItemAutoCollectTimer == 0)
    {
        itemScore = 0x6f;
        g_GameManager.AddToYoukaiGauge(g_Player.isYoukaiMode ? itemScore : -itemScore, 0);
    }
}

void ItemManager::AutoCollectAllItems()
{
    Item *item = this->itemListHead.next;
    while (item != NULL)
    {
        item->state = ITEM_STATE_AUTOCOLLECT;
        item->startPositionOrVelocity = Float3(0.0f, -0.5f, 0.0f);
        item = item->next;
    }
}

void ItemManager::ConvertAllPowerItemsToTimeOrbs(Item *item)
{
    Item *cur;

    cur = this->itemListHead.next;
    while (cur != NULL)
    {
        if (cur != item)
        {
            if (cur->itemType == ITEM_POWER_SMALL || cur->itemType == ITEM_POWER_BIG)
            {
                if (cur->startPositionOrVelocity.y > -0.5f)
                {
                    cur->startPositionOrVelocity.x = 0.0f;
                    cur->startPositionOrVelocity.y = -0.5f;
                    cur->startPositionOrVelocity.z = 0.0f;
                }
                g_EffectManager.SpawnEffect(0, &cur->currentPosition, 1, -1);
                cur->itemType = ITEM_POINT_SMALL;
                g_EffectAnmLoaded->SetAndExecuteScriptIdx(&cur->sprite, 0x45);
            }
        }
        cur = cur->next;
    }
}

void ItemManager::CancelAutoCollect()
{
    Item *item = this->itemListHead.next;
    while (item != NULL)
    {
        if (item->state == ITEM_STATE_AUTOCOLLECT)
        {
            item->state = ITEM_STATE_DEFAULT;
            item->startPositionOrVelocity.x = 0.0f;
            item->startPositionOrVelocity.y = -0.9f;
            item->startPositionOrVelocity.z = 0.0f;
        }
        item = item->next;
    }
}

#pragma var_order(fadeAlpha, item)
void ItemManager::OnDraw()
{
    i32 fadeAlpha;
    Item *item;

    item = this->itemListHead.next;
    while (item != NULL)
    {
        item->sprite.pos.x = g_GameManager.arcadeRegionTopLeftPos.x + item->currentPosition.x;
        item->sprite.pos.y = g_GameManager.arcadeRegionTopLeftPos.y + item->currentPosition.y;
        item->sprite.pos.z = 0.15f;
        if (item->currentPosition.Float3::Float3().y < -8.0f)
        {
            item->sprite.pos.y = 8.0f + g_GameManager.arcadeRegionTopLeftPos.y;
            if (item->isOnscreen)
            {
                g_BulletAnm->SetSprite(&item->sprite, item->itemType + 0xb6);
                item->isOnscreen = false;
                item->sprite.prefix.zWriteDisabled = true;
            }
            fadeAlpha = 0xff - (i32)((8.0f - item->currentPosition.Float3::Float3().y) * 255.0f / 128.0f);
            if (fadeAlpha < 0x40)
            {
                fadeAlpha = 0x40;
            }
            item->sprite.prefix.color1.d3dColor = (item->sprite.prefix.color1.d3dColor & 0xffffff) | (fadeAlpha << 0x18);
        }
        else
        {
            if (!item->isOnscreen)
            {
                g_BulletAnm->SetSprite(&item->sprite, item->itemType + 0xac);
                item->isOnscreen = true;
                item->sprite.prefix.color1.d3dColor = COLOR_WHITE;
                item->sprite.prefix.zWriteDisabled = true;
            }
        }
        g_AnmManager->Draw2D(&item->sprite);
        item = item->next;
    }
}

void Item::Delete()
{
    this->isInUse = false;
    this->prev->next = this->next;
    if (this->next != NULL)
    {
        this->next->prev = this->prev;
    }
    if (g_ItemManager.itemListTail == this)
    {
        g_ItemManager.itemListTail = this->prev;
    }
}

i32 ItemManager::GetTimeOrbCount()
{
    Item *next = this->itemListHead.next;
    i32 count = 0;

    while (next != NULL)
    {
        if (next->itemType == ITEM_TIME)
        {
            count++;
        }
        next = next->next;
    }

    return count;
}

// STUB: th08 0x441530
void ItemManager::FUN_00441530()
{
}
} /* namespace th08 */
