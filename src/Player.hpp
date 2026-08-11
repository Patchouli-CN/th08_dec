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

struct Player;

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
    unknown_fields(0x471, 0x7);
    u32 unk478;                       // 0x478
    unknown_fields(0x47c, 0x8);
};
C_ASSERT(sizeof(PlayerBulletVm) == 0x484);

// A player shot slot. shots[] and the slot sub-range at 0xbb834 both alias this
// array (FUN_0044de60 starts from &shots[0xc0]).
struct ShotSlot  // 0x40
{
    f32 posX;      // 0x0  spawn position (from FUN_0044de60 spawnPos)
    f32 posY;      // 0x4
    f32 unk8;      // 0x8
    f32 unkC;      // 0xc
    f32 targetX;   // 0x10
    f32 targetY;   // 0x14
    f32 unk18;     // 0x18
    f32 unk1C;     // 0x1c
    i32 unk20;     // 0x20
    i32 lifespan;  // 0x24  decremented each frame by FUN_0044c5b0
    i32 unk28;     // 0x28
    unknown_fields(0x2c, 0xc);
    i32 unk38;     // 0x38  set to 1 by FUN_0044e370
    u8 active;     // 0x3c  slot-in-use flag
    unknown_fields(0x3d, 0x3);
};
C_ASSERT(sizeof(ShotSlot) == 0x40);

struct Player
{
    i8 playerState;
    i8 initParam;
    i8 unk2;
    u8 isYoukaiMode;
    unknown_fields(0x4, 0xc);
    u8 unk_10[0x208];            // 0x10 (DrawNoRotation 取 &unk_10 作 AnmVm*)
    f32 unk_218;                 // 0x218
    f32 unk_21c;                 // 0x21c
    f32 unk_220;                 // 0x220
    unknown_fields(0x224, 0x90);
    Float3 positionCenter;       // 0x2b4
    unknown_fields(0x2c0, 0xfc);
    Float3 grabItemTopLeft;
    Float3 grabItemBottomRight;
    unknown_fields(0x3d4, 0x38);
    struct PlayerOption
    {
        u8 unk[0x2f0];
        void (Player::*func)();  // 0x2f0
    };
    PlayerOption options[4];     // 0x40c
    i32 unkFdc;                  // 0xfdc
    i32 unkFe0;                  // 0xfe0
    unknown_fields(0xfe4, 0x1c);
    void (Player::*shotFuncs[5])(); // 0x1000
    void (Player::*unk_1014[4])(); // 0x1014
    unknown_fields(0x1024, 0xb7810);
    ShotSlot shots[0x180];       // 0xb8834 (0xbb834 = &shots[0xc0] aliases a sub-range)
    unknown_fields(0xbe834, 0x4);
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
    static ChainCallbackResult __fastcall OnDrawHighPrio(Player *player);
    static ChainCallbackResult __fastcall OnDrawLowPrio(Player *player);
    static ZunResult AddedCallback(Player *player);
    static ZunResult DeletedCallback(Player *player);
    static void CutChain();
    void DrawBulletVms();

    static ZunResult __fastcall LoadShtFile(PlayerRawShtFile **header, const char *path);
    ZunBool CalcItemBoxCollision(Float3 *pos, Float3 *size);
    f32 AngleToPlayer(Float3 *pos);
    void FUN_004512f0();
    i32 FUN_00449ff0(void *unkD34, void *unkD44);
    i32 IsHuman();
    i32 IsYoukai();
    void FUN_0044c5b0();
    void FUN_0044c650();
    i32 FUN_0044cbf0();
    void FUN_0044d180();
    void FUN_0044d2c0();
    void FUN_0044aec0();
    void FUN_00451150();
    i32 FUN_00451500();
    void FUN_0044d420();
    u32 FUN_0044de60(Float3 *spawnPos, f32 targetX, f32 targetY, i32 unk28, i32 unk24);
    void FUN_0044e0f0();
    void FUN_0044e120();
    void FUN_0044e350();
    void FUN_0044e370();
};

DIFFABLE_EXTERN(Player, g_Player);
DIFFABLE_EXTERN(u8, g_PlayerUnknown0bb);
DIFFABLE_EXTERN(Float2, g_PlayerPos);
DIFFABLE_EXTERN(PlayerRawShtFile *, g_PlayerShtFile);
DIFFABLE_EXTERN(PlayerRawShtFile *, g_PlayerShtFile2);

} /* namespace th08 */
