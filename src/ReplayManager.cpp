#include "th_pch.h"

#include "Global.hpp"
#include "GameManager.hpp"
#include "Player.hpp"
#include "ReplayManager.hpp"

#include "pbg/Lzss.hpp"

#include <stddef.h>

namespace th08
{

DIFFABLE_STATIC(ReplayManager *, g_ReplayManager);

void ReplayManager::StopRecording()
{
    ReplayManager *replayManager = g_ReplayManager;

    if (replayManager != NULL)
    {
        replayManager->unk50 += 2;
        *(u16 *)replayManager->unk50 = 0;
        replayManager->unk54[g_GameManager.currentStage] = replayManager->unk50 + 6;
    }
}

// STUB: th08 0x4531f0
void ReplayManager::SaveReplay(const char *replayPath, const char *replayName)
{
}

#pragma var_order(decodedReplay, i, replayData, obfuscateOffset, obfuscateCursor, checksum, checksumCursor)
ReplayData *ReplayManager::LoadReplayData(void *data, int fileSize)
{
    u8 *obfuscateCursor;
    u8 obfuscateOffset;
    u8 *checksumCursor;
    u32 checksum;
    i32 i;
    ReplayData *decodedReplay;
    ReplayData *replayData = (ReplayData *)data;

    if (replayData == NULL)
    {
        goto err1;
    }

    if (replayData->header.magic != *(u32 *)REPLAY_MAGIC)
    {
        goto err1;
    }

    if (replayData->header.version != REPLAY_VERSION)
    {
        goto err1;
    }

    obfuscateCursor = (u8 *)&replayData->header.compressedSize;
    obfuscateOffset = replayData->header.value1;

    for (i = 0; i < replayData->header.fileSize - (i32)offsetof(ReplayDataHeader, compressedSize);
         i++, obfuscateCursor++)
    {
        *obfuscateCursor -= obfuscateOffset;
        obfuscateOffset += 7;
    }

    checksumCursor = &replayData->header.value1;
    checksum = REPLAY_OBFUSCATION_VALUE;

    for (i = 0; i < replayData->header.fileSize - (i32)offsetof(ReplayDataHeader, value1); i++, checksumCursor++)
    {
        checksum += *checksumCursor;
    }

    if (checksum != replayData->header.checksum)
    {
        goto err1;
    }

    decodedReplay = (ReplayData *)g_ZunMemory.Alloc(replayData->header.decompressedSize + sizeof(ReplayDataHeader) +
                                                    (fileSize - replayData->header.fileSize));

    memcpy(&decodedReplay->header, data, sizeof(ReplayDataHeader));

    Lzss::Decode((u8 *)replayData + sizeof(ReplayDataHeader), replayData->header.compressedSize,
                 (u8 *)decodedReplay + sizeof(ReplayDataHeader), replayData->header.decompressedSize);

    memcpy((u8 *)decodedReplay + sizeof(ReplayDataHeader) + replayData->header.decompressedSize,
           (u8 *)data + replayData->header.fileSize, fileSize - replayData->header.fileSize);

    replayData = decodedReplay;

    if (replayData->gameConfiguration.slowMode != 0)
    {
        goto err2;
    }

    if (g_Supervisor.CheckVersion(replayData->exeVersion, replayData->exeSize, replayData->exeChecksum) != ZUN_SUCCESS)
    {
        goto err2;
    }

    g_ZunMemory.Free(data);

    return decodedReplay;

err1:
    g_ZunMemory.Free(data);
    return NULL;

err2:
    g_ZunMemory.Free(data);
    g_ZunMemory.Free(decodedReplay);

    return NULL;
}

// STUB: th08 0x453160
ReplayManager::ReplayManager()
{
    this->unk18.Float3::Float3();
    this->unk24.Float3::Float3();
    this->unk30.Float3::Float3();
}

// STUB: th08 0x4522a0
void ReplayManager::FUN_004522a0()
{
}

// STUB: th08 0x452830
void ReplayManager::FUN_00452830()
{
}

// STUB: th08 0x452d60
void ReplayManager::FUN_00452d60()
{
}

// FUNCTION: th08 0x451f90 (46.7% FIXME: SEH 帧 /EHsc 生成差异 push ebx/esi + add esp)
ZunResult ReplayManager::RegisterChain(i32 param_1, char *path)
{
    ReplayManager *rm;
    ChainElem *elem;

    __try
    {
        g_KeyInput2 = 0;
        g_KeyInput = 0;
        if (g_ReplayManager == NULL)
        {
            rm = (ReplayManager *)operator new(0xdc);
            if (rm != NULL)
            {
                rm->ReplayManager::ReplayManager();
            }
            g_ReplayManager = rm;
            memset(rm, 0, 0xdc);
            rm->unk8 = NULL;
            rm->unk10 = param_1;
            rm->unk14 = path;
            switch (param_1)
            {
            case 0:
                elem = g_Chain.CreateElem((ChainCallback)0x452310);
                rm->unkC8 = elem;
                elem->addedCallback = (ChainLifetimeCallback)0x452830;
                elem->deletedCallback = (ChainLifetimeCallback)0x453080;
                elem->arg = rm;
                if (g_Chain.AddToCalcChain(elem, 0x11) != 0)
                {
                    return (ZunResult)-1;
                }
                rm->unkD0 = NULL;
                elem = g_Chain.CreateElem((ChainCallback)0x4522a0);
                rm->unkD4 = elem;
                elem->arg = rm;
                g_Chain.AddToCalcChain(elem, 7);
                rm->FUN_004522a0();
                break;
            case 1:
                elem = g_Chain.CreateElem((ChainCallback)0x452550);
                rm->unkC8 = elem;
                elem->addedCallback = (ChainLifetimeCallback)0x452d60;
                elem->deletedCallback = (ChainLifetimeCallback)0x453080;
                elem->arg = rm;
                if (g_Chain.AddToCalcChain(elem, 6) != 0)
                {
                    return (ZunResult)-1;
                }
                if (((ReplayDataHeader *)rm->unk8)->unk0x6 != 0)
                {
                    elem->callback = (ChainCallback)0x4526c0;
                }
                elem = g_Chain.CreateElem((ChainCallback)0x452490);
                rm->unkD0 = elem;
                elem->arg = rm;
                g_Chain.AddToCalcChain(elem, 0x12);
                rm->unkD4 = NULL;
                if (((ReplayDataHeader *)rm->unk8)->unk0x6 != 0)
                {
                    elem = g_Chain.CreateElem((ChainCallback)0x4522a0);
                    rm->unkD4 = elem;
                    elem->arg = rm;
                    g_Chain.AddToCalcChain(elem, 7);
                    rm->FUN_004522a0();
                }
                break;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        switch (param_1)
        {
        case 0:
            g_ReplayManager->FUN_00452830();
            break;
        case 1:
            g_ReplayManager->FUN_00452d60();
            break;
        }
    }
    return ZUN_SUCCESS;
}

} // namespace th08
