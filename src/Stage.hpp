#pragma once

#include "diffbuild.hpp"
#include "inttypes.hpp"
#include "utils.hpp"

namespace th08
{

// TODO: incomplete, only the fields referenced by currently decompiled code are mapped out.
struct StdRawHeader
{
    unknown_fields(0x0, 0x290);
    char bgmPaths[4][128];
};

// TODO: incomplete, same reason as above.
struct StageData
{
    unknown_fields(0x0, 0x7f4);
    StdRawHeader *stdData;
};

DIFFABLE_EXTERN(StageData, g_Stage);

} /* namespace th08 */
