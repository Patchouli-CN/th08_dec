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
typedef void (__fastcall *EclInterpFn)(Enemy *, struct EclInterp *, f32 t);
typedef void (*EclExInstr)(Enemy *, EclRawInstr *);

struct EclInterp
{
    EclInterpFn fn;    // 0x0
    ZunTimer timer;    // 0x4 (0xc)
    AnyArg args[8];    // 0x10 (0x20)
};
C_ASSERT(sizeof(EclInterp) == 0x30);

// The per-enemy ECL execution context. In th08 this lives inline in the Enemy
// at +0x7f8 (0x228 bytes); RunEcl reaches it through Enemy.curContextPtr.
struct EclContext
{
    EclRawInstr *curInstr;         // 0x0
    ZunTimer time;                 // 0x4
    EclExInstr func;               // 0x10
    EclRawInstr *eclExInstr;       // 0x14
    EclContextArgs eclContextArgs; // 0x18 (0x68)
    unknown_fields(0x80, 0x10);
    ZunTimer waitTimer;            // 0x90
    EclInterp interps[8];          // 0x9c (8 * 0x30 = 0x180)
    i32 laserNotInUse;             // 0x21c
    i32 unk220;                    // 0x220  sub-context index (set by exit)
    i16 subId;                     // 0x224
    unknown_fields(0x226, 0x2);
};
C_ASSERT(sizeof(EclContext) == 0x228);

struct EclManager
{
    ZunResult Load(const char *path);
    ZunResult RunEcl(Enemy *enemy);

    void *eclFile;
    void **subTable;
};
extern EclManager g_EclManager;
DIFFABLE_EXTERN_ARRAY(EclExInstr, 32, g_EclExInsn); // 0x4c6cb0 ECL ex-instr table

// The global ECL state object at 0x4ea670 (op164 touches its helpers).
struct EclGlobalObj
{
    f32 SetGlobalFlag(i32 a0);              // 0x41f0b0
    void SetTargetPos(f32 a0, f32 a1, f32 a2); // 0x41f040
};
DIFFABLE_EXTERN(EclGlobalObj, g_EclGlobalObj); // 0x4ea670

} // namespace th08
