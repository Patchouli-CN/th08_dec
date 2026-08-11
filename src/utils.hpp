#pragma once

#include "inttypes.hpp"

// A raw ECL instruction argument: integer / float / pointer depending on the
// opcode and the instruction's parameter mask.
union AnyArg
{
    i32 i;
    u32 u;
    f32 f;
    i16 s[2];
    u16 us[2];
    i8 c[4];
    u8 b[4];
};

#define ZUN_BIT(a) (1 << (a))

#define MAKE_FOURCC(a, b, c, d) ((a & 0xff) | ((b & 0xff) << 8) | ((c & 0xff) << 16) | ((d & 0xff) << 24))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define ARRAY_SIZE_SIGNED(x) ((i32)sizeof(x) / (i32)sizeof(x[0]))

#define unknown_fields(offset, size) u8 unk##offset[size]

namespace th08
{
namespace utils
{
void DebugPrint(char *fmt, ...);
void GuiDebugPrint(char *fmt, ...);
}; // namespace utils
}; // namespace th08
