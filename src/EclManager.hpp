#pragma once

#include "Supervisor.hpp" // ZunTimer
#include "ZunResult.hpp"
#include "inttypes.hpp"
#include "utils.hpp"

namespace th08
{

struct Enemy;

// A single ECL script invocation (an opcode + its parameter mask). The raw
// layout matches th07 (and the original th08.exe): time@0, opcode@4,
// skipInstrOnDifficulty@9, paramMask@a, args@c.
#pragma warning(disable : 4200)
struct EclRawInstr
{
    u32 time;                 // 0x0
    i16 id;                   // 0x4  opcode (1-based; RunEcl dispatches on id-1)
    i16 size;                 // 0x6  total instruction size in bytes
    u8 unused_8;              // 0x8
    u8 skipInstrOnDifficulty; // 0x9  difficulty flag mask
    u16 paramMask;            // 0xa  bitmask: bit N set => arg N is a variable id
    AnyArg args[];            // 0xc
};

// Per-enemy ECL variable context (four ints + eight floats + four ints + two
// floats, then the global ECL vars).
struct EclGlobalVars
{
    i32 intVars[4];
    f32 floatVars[4];
};
C_ASSERT(sizeof(EclGlobalVars) == 0x20);

struct EclContextArgs
{
    i32 intVars1[4];
    f32 floatVars1[8];
    i32 intVars2[4];
    f32 floatVars2[2];
    EclGlobalVars globalVars;
};
C_ASSERT(sizeof(EclContextArgs) == 0x68);

// An interpolation entry used by the move/lerp ECL instructions.
struct EclInterp
{
    void (*fn)(Enemy *, EclInterp *, f32 t);
    ZunTimer timer;
    AnyArg args[8];
};

struct EclManager
{
    ZunResult Load(const char *path);
    ZunResult RunEcl(Enemy *enemy);

    void *eclFile;
    void **subTable;
};
extern EclManager g_EclManager;

} // namespace th08
