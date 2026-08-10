#pragma once

#include "AnmManager.hpp"
#include "Global.hpp"

namespace th08
{

enum PlayerState
{
    PLAYER_STATE_ALIVE,
    PLAYER_STATE_SPAWNING,
    PLAYER_STATE_DEAD,
    PLAYER_STATE_INVULNERABLE,
    PLAYER_STATE_BORDER,
};

// A player shot behavior entry in the .sht file. The fields at 0x28-0x34 are
// indices into the player shot callback tables, replaced with pointers on load.
struct PlayerShotEntry
{
    i16 unk0; // 0x0
    unknown_fields(0x2, 0x26);
    i32 unk28; // 0x28
    i32 unk2c; // 0x2c
    i32 unk30; // 0x30
    i32 unk34; // 0x34
};
C_ASSERT(sizeof(PlayerShotEntry) == 0x38);

// One 8-byte slot in the .sht file's entry-pointer array.
struct PlayerShotEntrySlot
{
    PlayerShotEntry *entry; // 0x0, a file-relative offset, relocated at load
    u32 unk4;               // 0x4
};

struct PlayerRawShtFile
{
    unknown_fields(0x0, 0x2);
    u16 entryCount;
    f32 unk4;
    unknown_fields(0x8, 0xc);
    f32 itemCollectSpeed;
    f32 itemCollectRadius;
    f32 pocY;
    unknown_fields(0x20, 0x14);
    f32 unk34;
    PlayerShotEntrySlot entries[]; // 0x38, 8-byte stride
};

// The player's 128 option/bullet sprite VMs, iterated by the draw helper.
struct PlayerBulletVm
{
    AnmVm vm;
    f32 offsetX;
    f32 offsetY;
    unknown_fields(0x2ac, 0x1a4);
    f32 rotation;
    unknown_fields(0x454, 0xe);
    i16 state;
    unknown_fields(0x464, 0xc);
    i8 hasCustomColor;
    unknown_fields(0x471, 0x13);
};
C_ASSERT(sizeof(PlayerBulletVm) == 0x484);

struct Player
{
    i8 playerState;
    i8 initParam;
    i8 unk2;
    u8 isYoukaiMode;
    unknown_fields(0x4, 0x2b0);
    Float3 positionCenter;
    unknown_fields(0x2c0, 0xfc);
    Float3 grabItemTopLeft;
    Float3 grabItemBottomRight;
    unknown_fields(0x3d4, 0xc08);
    i32 unkFdc;
    unknown_fields(0xfe0, 0xbe258);
    PlayerBulletVm bullets[0x80];
    unknown_fields(0xe2a38, 0x3c);
    u32 unkE2a74;
    u32 unkE2a78;
    unknown_fields(0xe2a7c, 0x34);
    unknown_fields(0xe2ab0, 0x44);
    ZunTimer invulnerabilityTimer;
    unknown_fields(0xe2b00, 0x10);
    ChainElem *calcChain;
    ChainElem *drawChainHighPrio;
    ChainElem *drawChainLowPrio;

    static ZunResult RegisterChain(u32 param);
    static ChainCallbackResult OnUpdate(Player *player);
    static ChainCallbackResult OnDrawHighPrio(Player *player);
    static ChainCallbackResult OnDrawLowPrio(Player *player);
    static ZunResult AddedCallback(Player *player);
    static ZunResult DeletedCallback(Player *player);
    static void CutChain();
    void DrawBulletVms();

    static ZunResult LoadShtFile(PlayerRawShtFile **header, const char *path);
    ZunBool CalcItemBoxCollision(Float3 *pos, Float3 *size);
    f32 AngleToPlayer(Float3 *pos);
};

DIFFABLE_EXTERN(Player, g_Player);
DIFFABLE_EXTERN(PlayerRawShtFile *, g_PlayerShtFile);
DIFFABLE_EXTERN(PlayerRawShtFile *, g_PlayerShtFile2);

} /* namespace th08 */
